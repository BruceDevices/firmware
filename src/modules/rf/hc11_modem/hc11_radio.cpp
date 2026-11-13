#include "hc11_radio.h"

#include "core/bus_HAL.h"
#include "globals.h"
#include "modules/rf/rf_utils.h" // initCC1101once(), setMHZ()
#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

/*
 * HC-11 radio layer (real RF via CC1101).
 *
 * Register preset: applied on top of ELECHOUSE_cc1101.Init(), which
 * SRES-resets the chip (all registers -> chip defaults) and then loads the
 * ELECHOUSE defaults. Every register that matters for the HC-11 format is
 * written explicitly to establish the required CC1101 state: SYNC=0xD391,
 * PKTLEN=0xFF, MDMCFG1=0x22 = 4 preamble bytes,
 * FSCTRL0=0x00, TEST2/1=0x88/0x31. The ELECHOUSE defaults differ on all
 * of these (SYNC untouched but preceded by its own config, PKTLEN=0x00,
 * MDMCFG1=0x02, FSCTRL0~=35 after Calibrate(), TEST2/1=0x81/0x35), hence
 * the explicit overrides below.
 *
 * FREND0=0x10 selects PATABLE[0], where the P1..P8 level is written, so
 * AT+Px changes the output power.
 *
 * GDO0 (IOCFG0=0x06) asserts on sync word and deasserts at the end of the
 * packet, so a falling edge means "packet complete": in RX the FIFO then
 * holds the full packet ([len][payload...][RSSI][LQI|CRC_OK]) and can be
 * read in one burst; in TX it means the packet was sent and the chain can
 * continue.
 *
 * MCSM1 is 0x3C instead of the original preset value 0x30. RXOFF_MODE (bits 3:2)
 * encoding, bench-deduced 2026-10-13: 00=IDLE, 01=FSTXON, 10=TX, 11=RX.
 * Evidence: 00 (0x30) leaves the radio deaf in IDLE after a packet; an
 * interrupt-driven receiver re-arms RX from a separate thread
 * within ~1 ms; 01 left it deaf in FSTXON (MARCSTATE 0x12); 10 sent it to
 * TX (MARCSTATE 0x13=TX read right at the completed packet), deaf until the
 * watchdog re-armed it ~6 ms later. Back-to-back single-byte packets were
 * lost during that deaf window.
 * RXOFF=11 (RX) keeps the radio listening continuously, so a burst is
 * received whole; TXOFF (bits 5:4) = 11 = RX likewise returns the radio to
 * RX after each transmitted packet of a TX chain. The transmitted waveform
 * is unaffected (MCSM1 only controls the local state machine), so interop
 * is preserved. A MARCSTATE watchdog below re-arms RX defensively if the
 * chip ever drops out.
 */

// ------------------------------------------------------------------
// State (all static: USB init can corrupt stack variables - see
// hc11_modem.cpp; same policy here)
// ------------------------------------------------------------------

// Completed-packet counter: the GDO0 ISR counts falling edges ("packet
// complete"); the poll loop consumes them one packet per FIFO read. A counter
// (not a flag) keeps back-to-back bursts from coalescing into one edge.
static volatile uint8_t gdo0PendingPkts = 0;
static portMUX_TYPE gdo0Mux = portMUX_INITIALIZER_UNLOCKED;

static void IRAM_ATTR gdo0IsrIncrement() {
    portENTER_CRITICAL_ISR(&gdo0Mux);
    if (gdo0PendingPkts < 255) gdo0PendingPkts++;
    portEXIT_CRITICAL_ISR(&gdo0Mux);
}

static void gdo0PktsClear() {
    portENTER_CRITICAL(&gdo0Mux);
    gdo0PendingPkts = 0;
    portEXIT_CRITICAL(&gdo0Mux);
}

static bool gdo0PktsTakeOne() { // true if a packet completion was pending
    portENTER_CRITICAL(&gdo0Mux);
    bool had = gdo0PendingPkts > 0;
    if (had) gdo0PendingPkts--;
    portEXIT_CRITICAL(&gdo0Mux);
    return had;
}

static bool gdo0PktsTakeAll() { // true if at least one completion was pending
    portENTER_CRITICAL(&gdo0Mux);
    bool had = gdo0PendingPkts > 0;
    gdo0PendingPkts = 0;
    portEXIT_CRITICAL(&gdo0Mux);
    return had;
}

static Hc11RadioState radioState = HC11_RADIO_OFF;

struct Hc11RadioConfig {
    uint16_t channel = 1;
    uint16_t address = 0;
    uint8_t txPower = 8;
    uint8_t fuMode = 1; // 1..4 (FU1..FU4)
};

static Hc11RadioConfig cfg;
static Hc11RadioConfig pendingCfg;
static bool cfgDirty = false;

static uint32_t *txByteCounter = nullptr; // session's hostToRadioBytes

// TX ring buffer (host -> air)
static uint8_t txBuf[HC11_RADIO_TX_BUF_SIZE];
static size_t txHead = 0, txTail = 0, txCount = 0;

static uint32_t droppedBytes = 0;
static bool overflowFlag = false;

static float lastRssiDbm = 0.0f;
static uint8_t lastLqi = 0;

static uint8_t rxBadStateCtr = 0; // watchdog: consecutive non-RX MARCSTATE reads

static Hc11RadioRxCb rxCb = nullptr;
static void *rxCtx = nullptr;

// Scratch packet buffer: [LEN][ADDR][0x44][payload...] (FU3) or the FU1 forms
static uint8_t pktBuf[HC11_RADIO_MAX_FIFO_PKT];
static uint8_t pktLen = 0;     // total bytes to write to the TX FIFO
static uint8_t pktPayload = 0; // payload bytes in flight (0 = wake-up pkt)

// FU3: fixed packet-size ramp at the start of a transmission
static const uint8_t fu3Sizes[] = {3, 34, 47, 52, 55, 56, 57};
static uint8_t fu3Idx = 0;

// FU1: wake-up packets before data
#define FU1_WAKEUP_PKTS 64
#define FU1_WAKEUP_IDLE_MS 100
static uint8_t fu1WakeCtr = 0;
static bool neverSent = true;
static uint32_t lastPktMs = 0;

// Guards against a stuck GDO0 / lost edge
static uint32_t txWatchdogMs = 0;
static uint32_t gdo0HighSinceMs = 0;

// ------------------------------------------------------------------
// Radio preset
// ------------------------------------------------------------------

// PATABLE[0] values for the 8 HC-11 power levels; bench-verify monotonicity.
static const uint8_t hc11Patable[8] = {0x12, 0x0e, 0x1d, 0x34, 0x60, 0x84, 0xc8, 0xc0};

static uint8_t patableFor(uint8_t p) {
    if (p < 1) p = 1;
    if (p > 8) p = 8;
    return hc11Patable[p - 1];
}

static void applyPreset() {
    // CHANNR/ADDR/PATABLE are config-dependent and written separately.
    // HC-11 preset values plus explicit chip-default restores (see file
    // header) establish a consistent register state on both ends.
    static const uint8_t preset[][2] = {
        {CC1101_IOCFG2,   0x29}, // chip default (GDO2 unused; driver wrote 0x0D)
        {CC1101_IOCFG0,   0x06}, // GDO0: sync-word / packet event
        {CC1101_FIFOTHR,  0x0f}, //
        {CC1101_SYNC1,    0xD3}, // chip default sync word
        {CC1101_SYNC0,    0x91}, //
        {CC1101_PKTLEN,   0xFF}, // chip default (driver wrote 0x00)
        {CC1101_PKTCTRL1, 0x45}, // addr check (no broadcast), append status, no CRC_AUTOFLUSH
        {CC1101_PKTCTRL0, 0x05}, // variable length, CRC on, FIFO mode
        {CC1101_FSCTRL1,  0x0c}, //
        {CC1101_FSCTRL0,  0x00}, // chip default (driver's Calibrate() wrote ~35)
        {CC1101_FREQ2,    0x10}, // ~434 MHz base (433 band)
        {CC1101_FREQ1,    0xb1}, //
        {CC1101_FREQ0,    0x31}, //
        {CC1101_MDMCFG4,  0x2d}, // data rate
        {CC1101_MDMCFG3,  0x3b}, //
        {CC1101_MDMCFG2,  0x13}, // GFSK, sync 30/32 bits
        {CC1101_MDMCFG1,  0x22}, // chip default: 4 preamble bytes (driver wrote 0x02)
        {CC1101_MDMCFG0,  0xF8}, // channel spacing (chip default)
        {CC1101_DEVIATN,  0x62}, //
        {CC1101_MCSM2,    0x07}, // chip default
        {CC1101_MCSM1,    0x3C}, // RXOFF=11 (RX): stay in RX after a packet (see file header)
        {CC1101_MCSM0,    0x18}, // autocalibrate IDLE->RX/TX
        {CC1101_FOCCFG,   0x1d}, //
        {CC1101_BSCFG,    0x1c}, //
        {CC1101_AGCCTRL2, 0x07}, //
        {CC1101_AGCCTRL1, 0x00}, //
        {CC1101_AGCCTRL0, 0xb0}, //
        {CC1101_WOREVT1,  0x01}, //
        {CC1101_WOREVT0,  0xf1}, //
        {CC1101_WORCTRL,  0x78}, //
        {CC1101_FREND1,   0xb6}, //
        {CC1101_FREND0,   0x10}, // PA_POWER=0 -> PATABLE[0] (see file header)
        {CC1101_FSCAL3,   0xea}, //
        {CC1101_FSCAL2,   0x2a}, //
        {CC1101_FSCAL1,   0x00}, //
        {CC1101_FSCAL0,   0x1f}, //
        {CC1101_TEST2,    0x88}, // chip default (driver wrote 0x81)
        {CC1101_TEST1,    0x31}, // chip default (driver wrote 0x35)
        {CC1101_TEST0,    0x09}, //
    };
    for (size_t i = 0; i < sizeof(preset) / sizeof(preset[0]); i++)
        ELECHOUSE_cc1101.SpiWriteReg(preset[i][0], preset[i][1]);

    // Channel (C001..C127 -> CHANNR 2..254, stays inside the 433 ISM band
    // by construction) and address (hardware addr check)
    uint16_t ch = cfg.channel;
    if (ch < 1) ch = 1;
    if (ch > 127) ch = 127;
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_CHANNR, (uint8_t)(ch << 1));
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_ADDR, (uint8_t)cfg.address);
    // Single write with CS toggle -> PATABLE[0]
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_PATABLE, patableFor(cfg.txPower));
}

// ------------------------------------------------------------------
// GDO0 interrupt
// ------------------------------------------------------------------

static void IRAM_ATTR hc11Gdo0Isr() {
    // Falling edge = packet complete (RX: full packet in the FIFO; TX: packet
    // sent). With RXOFF=RX the radio stays in RX and the next packet can
    // already be streaming in, so edges are counted, not just latched.
    // Everything else happens in hc11RadioPoll().
    gdo0IsrIncrement();
}

static bool gdo0Pin() { return digitalRead((int)bruceConfigPins.CC1101_bus.io0) == HIGH; }

// ------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------

// RXBYTES must be read twice with the same result (CC1101 errata, swrz020e)
static uint8_t rxFifoBytes(bool *overflowed) {
    uint8_t prev = ELECHOUSE_cc1101.SpiReadStatus(CC1101_RXBYTES);
    uint8_t cur = ELECHOUSE_cc1101.SpiReadStatus(CC1101_RXBYTES);
    while (prev != cur) {
        prev = cur;
        cur = ELECHOUSE_cc1101.SpiReadStatus(CC1101_RXBYTES);
    }
    if (overflowed) *overflowed = (cur & 0x80) != 0;
    return cur & 0x7F;
}

static void flushRx() {
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SFRX);
    ELECHOUSE_cc1101.SetRx(); // SIDLE + SRX
}

static void antennaTx(bool tx) {
    // Antenna path direction switch, pattern from initRfModule() tx/rx modes
    ioExpander.turnPinOnOff(IO_EXP_CC_RX, tx ? LOW : HIGH);
    ioExpander.turnPinOnOff(IO_EXP_CC_TX, tx ? HIGH : LOW);
}

static void deliver(const uint8_t *data, size_t len) {
    if (rxCb && len > 0) rxCb(data, len, rxCtx);
}

// ------------------------------------------------------------------
// RX path
// ------------------------------------------------------------------

static void processRxPacket() {
    bool ovf = false;
    uint8_t n = rxFifoBytes(&ovf);
    if (ovf || n < 5) { // len + >=2 payload + 2 status bytes minimum
        flushRx();
        return;
    }

    uint8_t len = ELECHOUSE_cc1101.SpiReadReg(CC1101_RXFIFO); // length byte
    if (len < 2 || len > HC11_RADIO_MAX_FIFO_PKT) {
        flushRx();
        return;
    }
    if (rxFifoBytes(nullptr) < (uint8_t)(len + 2)) { // payload + 2 status bytes
        flushRx();
        return;
    }

    ELECHOUSE_cc1101.SpiReadBurstReg(CC1101_RXFIFO, pktBuf, len);
    uint8_t st[2];
    ELECHOUSE_cc1101.SpiReadBurstReg(CC1101_RXFIFO, st, 2);

    // Append-status bytes: [RSSI][LQI | CRC_OK<<7]
    lastRssiDbm = (st[0] >= 128 ? st[0] - 256.0f : (float)st[0]) / 2.0f - 74.0f;
    lastLqi = st[1] & 0x7F;

    // MCSM1 RXOFF=RX: the radio stayed in RX and is already listening for the
    // next packet - no re-arm strobe here (a SIDLE+SRX would kill an
    // in-flight packet of a back-to-back burst, which is exactly the race
    // that lost every packet after the first one on the bench).

    if (!(st[1] & 0x80)) { // CRC bad: silently drop the packet
        return;
    }

    // Decode per the HC-11 mode (FU2/FU4 use the FU3 profile - research R-04)
    if (cfg.fuMode == 1) {
        // FU1: 56-byte packets = addr + 55 data; 57-byte = addr + x data +
        // filler + x (x = last byte)
        uint8_t l = 0;
        if (len == 57) l = pktBuf[56];
        else if (len == 56) l = 55;
        if (l == 0 || l > 55 || pktBuf[0] != (uint8_t)cfg.address) { return; }
        deliver(pktBuf + 1, l);
    } else {
        // FU3: [addr][0x44][data...]
        if (len < 3 || pktBuf[0] != (uint8_t)cfg.address || pktBuf[1] != 0x44) { return; }
        deliver(pktBuf + 2, len - 2);
    }
}

// ------------------------------------------------------------------
// TX path
// ------------------------------------------------------------------

static uint8_t pullTx(uint8_t *dst, uint8_t max) {
    uint8_t n = 0;
    while (n < max && txCount > 0) {
        dst[n++] = txBuf[txTail];
        txTail = (txTail + 1) % HC11_RADIO_TX_BUF_SIZE;
        txCount--;
    }
    return n;
}

static void buildPacket() {
    size_t avail = txCount;
    pktPayload = 0;

    if (cfg.fuMode == 1) {
        // FU1: wake-up packets first, then 55-byte data packets
        if (fu1WakeCtr > 0) {
            fu1WakeCtr--;
            pktBuf[0] = 2;
            pktBuf[1] = (uint8_t)cfg.address;
            pktBuf[2] = 0;
            pktLen = 3;
            return;
        }
        if (avail >= 55) {
            pktBuf[0] = 55 + 1; // 56
            pktBuf[1] = (uint8_t)cfg.address;
            pullTx(pktBuf + 2, 55);
            pktLen = 57;
            pktPayload = 55;
        } else {
            pktBuf[0] = 55 + 2; // 57
            pktBuf[1] = (uint8_t)cfg.address;
            uint8_t got = pullTx(pktBuf + 2, 55);
            memset(pktBuf + 2 + got, 0, 55 - got); // undefined filler bytes
            pktBuf[57] = got;                      // real data length
            pktLen = 58;
            pktPayload = got;
        }
        return;
    }

    // FU3 (and the FU2/FU4 profile): fixed size ramp {3,34,47,52,55,56,57},
    // then locked at 57; the last packet of a burst carries the remainder.
    uint8_t size = fu3Sizes[fu3Idx];
    if (fu3Idx < sizeof(fu3Sizes) / sizeof(fu3Sizes[0]) - 1) fu3Idx++;
    uint8_t l = size - 2; // payload bytes
    if (avail < l) l = avail;
    pktBuf[0] = l + 2;
    pktBuf[1] = (uint8_t)cfg.address;
    pktBuf[2] = 0x44;
    pullTx(pktBuf + 3, l);
    pktLen = l + 3;
    pktPayload = l;
}

static void sendNextPacket() {
    buildPacket();
    gdo0PktsClear(); // drop stale edges before STX
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SFTX);
    ELECHOUSE_cc1101.SpiWriteBurstReg(CC1101_TXFIFO, pktBuf, pktLen);
    ELECHOUSE_cc1101.SpiStrobe(CC1101_STX);
    neverSent = false;
    lastPktMs = millis();
    txWatchdogMs = lastPktMs;
}

static void startTxBurst() {
    antennaTx(true);
    ELECHOUSE_cc1101.setSidle();
    // Half-duplex: any packet being received at this moment is lost anyway -
    // flush its partial bytes so the next RX parse doesn't see stale FIFO.
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SFRX);
    gdo0PktsClear();
    // Reset packetization state for each burst
    fu3Idx = 0;
    if (neverSent || millis() - lastPktMs > FU1_WAKEUP_IDLE_MS) fu1WakeCtr = FU1_WAKEUP_PKTS;
    else fu1WakeCtr = 0;
    radioState = HC11_RADIO_TX_CHAIN;
    sendNextPacket();
}

static void finishTxBurst() {
    antennaTx(false);
    ELECHOUSE_cc1101.SetRx();
    radioState = HC11_RADIO_RX;
}

// ------------------------------------------------------------------
// Config
// ------------------------------------------------------------------

static void applyConfigNow() {
    cfg = pendingCfg;
    cfgDirty = false;
    ELECHOUSE_cc1101.setSidle();
    uint16_t ch = cfg.channel;
    if (ch < 1) ch = 1;
    if (ch > 127) ch = 127;
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_CHANNR, (uint8_t)(ch << 1));
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_ADDR, (uint8_t)cfg.address);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_PATABLE, patableFor(cfg.txPower));
    ELECHOUSE_cc1101.SetRx();
}

// ------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------

bool hc11RadioInit(
    uint16_t channel, uint16_t address, uint8_t txPower, uint8_t fuMode, uint32_t *txByteCounterP
) {
    hc11RadioStop();
    txByteCounter = txByteCounterP;
    cfg.channel = channel;
    cfg.address = address;
    cfg.txPower = txPower;
    cfg.fuMode = fuMode;
    cfgDirty = false;
    txHead = txTail = txCount = 0;
    droppedBytes = 0;
    overflowFlag = false;
    lastRssiDbm = 0.0f;
    lastLqi = 0;
    neverSent = true;
    lastPktMs = 0;

    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) return false;

    // SPI bus setup, pattern from initRfModule() (rf_utils.cpp): the bus
    // may be shared with the display
    SPIClass *ccSpi = acquireSPIBus(
        bruceConfigPins.CC1101_bus.sck, bruceConfigPins.CC1101_bus.miso, bruceConfigPins.CC1101_bus.mosi
    );
    if (ccSpi) {
        ELECHOUSE_cc1101.setBeginEndLogic(false);
        initCC1101once(ccSpi);
    } else {
        ELECHOUSE_cc1101.setBeginEndLogic(true);
        initCC1101once(NULL);
    }
    if (!ELECHOUSE_cc1101.Init()) return false;
    if (!ELECHOUSE_cc1101.getCC1101()) return false; // no transceiver answering

    // Antenna path for the 433 band (T-Embed SW pins) + driver baseline
    // (FREQ/FSCTRL0/TEST regs); the HC-11 preset overwrites them right
    // after, so the order matters.
    setMHZ(433.92);
    applyPreset();

    // GDO0: falling edge = packet complete (RX or TX)
    pinMode(bruceConfigPins.CC1101_bus.io0, INPUT);
    attachInterrupt(digitalPinToInterrupt((int)bruceConfigPins.CC1101_bus.io0), hc11Gdo0Isr, FALLING);
    gdo0PktsClear();
    gdo0HighSinceMs = 0;

    antennaTx(false);
    ELECHOUSE_cc1101.SetRx();
    radioState = HC11_RADIO_RX;
    return true;
}

void hc11RadioStop() {
    if (radioState == HC11_RADIO_OFF) return;
    detachInterrupt(digitalPinToInterrupt((int)bruceConfigPins.CC1101_bus.io0));
    ELECHOUSE_cc1101.setSidle();
    digitalWrite(bruceConfigPins.CC1101_bus.cs, HIGH);
    // Antenna path off, pattern from deinitRfModule() (rf_utils.cpp)
    ioExpander.turnPinOnOff(IO_EXP_CC_RX, LOW);
    ioExpander.turnPinOnOff(IO_EXP_CC_TX, LOW);
    radioState = HC11_RADIO_OFF;
    gdo0PktsClear();
}

void hc11RadioSetRxCallback(Hc11RadioRxCb cb, void *ctx) {
    rxCb = cb;
    rxCtx = ctx;
}

void hc11RadioApplyConfig(uint16_t channel, uint16_t address, uint8_t txPower, uint8_t fuMode) {
    pendingCfg.channel = channel;
    pendingCfg.address = address;
    pendingCfg.txPower = txPower;
    pendingCfg.fuMode = fuMode;
    cfgDirty = true; // applied at the next safe moment in hc11RadioPoll()
}

void hc11RadioFeed(const uint8_t *data, size_t len) {
    if (radioState == HC11_RADIO_OFF) return; // defensive; app exits without radio
    for (size_t i = 0; i < len; i++) {
        if (txCount < HC11_RADIO_TX_BUF_SIZE) {
            txBuf[txHead] = data[i];
            txHead = (txHead + 1) % HC11_RADIO_TX_BUF_SIZE;
            txCount++;
        } else {
            // Bounded buffer: drop new bytes and flag the UI
            droppedBytes++;
            overflowFlag = true;
        }
    }
}

void hc11RadioPoll() {
    if (radioState == HC11_RADIO_OFF) return;

    if (radioState == HC11_RADIO_RX) {
        // Completed received packets (GDO0 fell; the radio stayed in RX and
        // the FIFO holds one packet per counted edge). Drain them all so a
        // back-to-back burst is not lost to a coalesced flag.
        bool processedAny = false;
        while (gdo0PktsTakeOne()) {
            processedAny = true;
            processRxPacket();
        }
        if (processedAny) gdo0HighSinceMs = 0;
        else if (gdo0Pin()) {
            // Guard: GDO0 stuck high (desync) - flush and listen again. The
            // threshold must exceed the longest on-air packet (a 61-byte FU1
            // packet at the preset's low data rate), hence 1000 ms.
            if (gdo0HighSinceMs == 0) gdo0HighSinceMs = millis();
            else if (millis() - gdo0HighSinceMs > 1000) {
                gdo0HighSinceMs = 0;
                flushRx();
            }
        } else {
            gdo0HighSinceMs = 0;
        }

        // Apply live config changes between packets, with nothing in flight
        if (cfgDirty && !gdo0Pin()) applyConfigNow();

        // RX health watchdog (bench 2026-10-13: with a wrong RXOFF the chip
        // dropped out of RX after the first received packet and stayed deaf).
        // MARCSTATE is a proper status register: 0x0D = RX. Right after a
        // packet the MARC passes through RX_END-like states before landing
        // back in RX, and a status read can also be stale -
        // so only re-arm after several consecutive non-RX readings, never
        // mid-transition (an eager watchdog was itself killing the
        // RX_END -> RX transition and with it every packet 2+ of a burst).
        uint8_t marcState = ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE);
        if (marcState != 0x0D && !gdo0Pin()) { // 0x0D = MARCSTATE_RX
            if (++rxBadStateCtr >= 3) {
                rxBadStateCtr = 0;
                ELECHOUSE_cc1101.SetRx();
            }
        } else {
            rxBadStateCtr = 0;
        }

        // Host data pending: start a burst
        if (radioState == HC11_RADIO_RX && txCount > 0) startTxBurst();
    } else if (radioState == HC11_RADIO_TX_CHAIN) {
        bool done = gdo0PktsTakeAll();
        if (!done && millis() - txWatchdogMs > 100) done = true; // lost edge guard
        if (!done) return;

        // Packet sent: count the payload bytes that actually went out
        if (pktPayload > 0 && txByteCounter) *txByteCounter += pktPayload;

        if (txCount > 0) sendNextPacket();
        else finishTxBurst();
    }
}

Hc11RadioStatus hc11RadioGetStatus() {
    Hc11RadioStatus st;
    st.state = radioState;
    st.lastRssiDbm = lastRssiDbm;
    st.lastLqi = lastLqi;
    st.droppedBytes = droppedBytes;
    st.overflow = overflowFlag;
    return st;
}
