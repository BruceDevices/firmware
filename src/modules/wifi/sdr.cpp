/*
 * SDR — raw I/Q takeover of the ESP32-S3 WiFi radio, in the spirit of
 * PBJ3000-ESP32-Jammer (https://github.com/pinchepasta/PBJ3000-ESP32-Jammer,
 * GPL-3.0-or-later), which is based on ESPARGOS/esp-sdr. This file is a port of
 * the esp_sdr core (radio bring-up, LO tuning, front-end switching, the
 * undocumented MAC dump engine used as ADC and DAC) to Bruce's Arduino/IDF5
 * environment, plus a Bruce-style menu on top.
 *
 * How it works, briefly:
 *  - esp_wifi is brought up normally and parked in promiscuous mode on a
 *    channel; that leaves the radio powered with the PHY calibrated.
 *  - A 64 KiB SRAM "dump bank" is taken away from the heap at boot
 *    (SOC_RESERVE_MEMORY_REGION) and handed to the undocumented MAC dump
 *    engine (SENSITIVE_INTERNAL_SRAM_USAGE_3_REG one-hot). The engine streams
 *    the bank through the radio: as ADC (raw I/Q capture) or DAC (raw I/Q
 *    playback = RF transmission) at 16/40/80 MS/s.
 *  - Tuning uses libphy's set_chanfreq() for Wi-Fi channels and
 *    set_rf_freq_offset() for off-grid requests (2210-2650 MHz direct PLL,
 *    ~1817-2209 MHz through the 6/5-divided LO); the analog front end is
 *    switched with ROM pbus functions and the force-TX bit documented in
 *    PBJ3000's doc/undocumented-registers-esp32s3.md. Off-grid requests the
 *    VCO cannot lock are rejected via the RFPLL window flag (their -ERANGE).
 *
 * ESP32-C5: not supported here, on two counts. (1) The C5/C6 dump engine has
 * no known DAC trigger, so raw I/Q TX is impossible — PBJ3000 ships the C5
 * receive-only. (2) Even the receive path needs a full 128 KiB HP SRAM block
 * (block 1, 0x40820000, the only one that serves), and the Arduino/IDF C5
 * layout puts .noinit/.bss inside it (measured: 0x408246e9-0x40836B60), so
 * donating the block would corrupt static data. PBJ3000 avoids this with a
 * Zephyr linker snippet that moves the statics; Bruce's C5 linker scripts
 * would have to be forked the same way before a C5 port is possible.
 *
 * Unlike PBJ3000 we do not ship the vendor librftest.a blob, so there are no
 * forced RX/TX gain ladders: the radio runs on the gain tables the PHY
 * calibrated for the channel; TX power is set with esp_wifi_set_max_tx_power()
 * before the takeover.
 *
 * WARNING: this makes the device emit real RF power. Jamming radio frequencies
 * is illegal in most jurisdictions — use only against your own equipment in a
 * shielded environment.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#if !defined(LITE_VERSION) && !defined(BRUCE_NO_SDR_JAMMER)
// Everything below needs the ESP32-S3: the undocumented MAC dump engine with
// its DAC side only exists there. CONFIG_IDF_TARGET_ESP32S3 comes from
// sdkconfig.h, pulled in by the Arduino headers above — it is not visible to a
// bare file-top guard, so the target check happens right after the includes.

#include "sdr.h"

#include <Arduino.h> // pulls sdkconfig.h

#include <esp_heap_caps.h>
#include <esp_private/regi2c_ctrl.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <fft.h>
#include <globals.h>
#include <heap_memory_layout.h>
#include <nvs_flash.h>
#include <soc/soc.h>
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include <soc/sensitive_reg.h>
#endif

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/wifi/webInterface.h"
#include "core/wifi/wifi_common.h"

#if defined(CONFIG_IDF_TARGET_ESP32S3)

// ---------------------------------------------------------------------------
// Radio entry points: ROM stubs (esp32s3.rom.ld, linked on every Arduino S3
// build) and libphy.a (linked with every Arduino S3 WiFi build).
// ---------------------------------------------------------------------------
extern "C" {
unsigned int rom_chip_i2c_readReg(int block, int host, int reg_add);
void rom_chip_i2c_writeReg(int block, int host, int reg_add, int data);
void rom_pbus_workmode(void);
void rom_pbus_xpd_rx_on(unsigned int en);
void rom_pbus_xpd_tx_off(void);
void rom_pbus_xpd_tx_on(unsigned int en);
void rom_set_rxclk_en(unsigned int en);
// libphy.a
void set_chanfreq(unsigned int mhz, unsigned int cbw);
void set_rf_freq_offset(unsigned int xtal, unsigned int mhz, int khz);
void stop_tx_tone(unsigned int en);
}

// ---------------------------------------------------------------------------
// Undocumented hardware (addresses/bits per PBJ3000 doc/undocumented-registers-esp32s3.md)
// ---------------------------------------------------------------------------
#define J_DUMP_CTRL_REG 0x60033d5cU // undocumented WiFi MAC dump engine, ADC side
#define J_DUMP_CTRL_RUN BIT(31)
#define J_DUMP_CTRL_TRIG BIT(19)
#define J_DUMP_CTRL_DONE BIT(18)
#define J_DUMP_CTRL_16M BIT(16)
#define J_DUMP_CTRL_40M BIT(15)
#define J_DUMP_CTRL_COUNT 0x3fffU // 13:0
#define J_DUMP_CFG_REG 0x60033d90U
#define J_DUMP_CFG_IQ 0x000c2040U  // four 6-bit lane selectors: receive I/Q
#define J_DAC_TRIG_REG 0x60033d64U // DAC-side mirror, 8 bytes above the ADC control
#define J_DAC_CTRL_80M BIT(15)
#define J_DUMP_TIMEOUT_US 20000U

#define J_FE_TX_CTRL_REG 0x60006000U // front end: bit 1 = force TX on, 17:10 TX gain index, 25:18 base index
#define J_FE_FORCE_TXON BIT(1)
#define J_FE_GAIN_IDX_MSK (0xffU << 10)

// Wi-Fi AGC (PBJ3000 doc): forced index 31:24, force-enable 23, largest
// calibrated index 14:8. S3 maxima above ~82 point at uncalibrated table slots.
#define J_AGC_FORCE_REG 0x6001c02cU
#define J_AGC_FORCE_EN BIT(23)
#define J_AGC_GAIN_MAX_MSK 0x7f00U // 14:8
#define J_AGC_GAIN_MAX_S 8
#define J_AGC_GAIN_VALID_MAX 82U
// bt_rx_force(0) steps this register's bits 9:8 through 0x200, 0x300, 0
#define J_FE_R110_REG 0x60006110U
#define J_FE_R110_B9_8_MSK 0x300U

// The dump engine's bank 1: 64 KiB of SRAM the engine streams to/from the
// radio. Bank 1 (0x3FCC0000), not bank 0, because Bruce's .bss extends past
// 0x3FCB0000 (bank 0 overlaps static data; the IDF reserve validator aborts
// at boot on overlap). Bank 1 lies wholly inside the heap arena, below the
// ROM-reserved DRAM at 0x3FCDB700+.
#define J_BANK_ADDR 0x3FCC0000U
#define J_BANK_LEN 0x10000U
#define J_BANK_WORDS (J_BANK_LEN / sizeof(uint32_t)) // 16384
#define J_BANK_USAGE (BIT(1) << 0)                   // MAC_DUMP_USAGE one-hot, bank 1

// Upstream rx_lo.h: the LO selector lives in analog I2C block 0x65, register 0,
// bit 4 (0 = normal LO, 1 = 6/5-divided LO).
#define J_LO_BLOCK 0x65
#define J_LO_REG 0
#define J_LO_MASK 0x10U

// RF PLL status after the vendor calibration (analog I2C block 0x62): bits 3:2
// compare the request with the VCO window: 0 inside, 1 below, 2 above.
#define J_RFPLL_BLOCK 0x62
#define J_RFPLL_STATUS_REG 12
#define J_RFPLL_STATUS_VCO 0x0cU

#define J_CAL_CHANNEL_MHZ 2412U
#define J_LO_DIVIDED_BELOW_MHZ 2210U // VCO cannot reach below ~2180 MHz; lower requests use the 6/5 divider
#define J_RETUNE_SETTLE_US 3000U     // PLL lock / front-end turnaround settling

// Off-grid tuning limits, per PBJ3000 (README + esp_sdr.c): the API ceiling is
// 6000 MHz, but the S3's VCO only locks about 1.82-2.65 GHz. Below 2210 MHz the
// 6/5-divided LO reaches down to VCO_min / 1.2 (~1817 MHz); anything the VCO
// cannot lock is rejected by the PLL window check, like PBJ's -ERANGE.
#define J_FREQ_API_MIN_MHZ 100U
#define J_FREQ_API_MAX_MHZ 6000U
#define J_FREQ_ADV_MIN_MHZ 1817U // advisory only; the PLL window decides
#define J_FREQ_ADV_MAX_MHZ 2660U // advisory only; the PLL window decides

#define J_SAMPLES_MIN 256U
#define J_SAMPLES_MAX 16380U // engine count field is 14 bits; keep a multiple of 4

// Take the dump bank away from the heap at boot (before heap_caps_init runs):
// .reserved_memory_address is collected by the IDF linker script and subtracted
// from the DRAM heap regions. Cost: exactly 64 KiB less internal RAM on every
// S3 build, PSRAM or not (the dump engine only reaches internal SRAM).
// Measured on Cardputer: ~90 kB free heap at the main menu without, ~24 kB with.
// Boards that cannot spare it set -DBRUCE_NO_SDR_JAMMER in their env
// build_flags, which also removes the menu entry.
#if !defined(BRUCE_NO_SDR_JAMMER)
#define J_RESERVE_BANK 1
#else
#define J_RESERVE_BANK 0
#endif

#if J_RESERVE_BANK
SOC_RESERVE_MEMORY_REGION(J_BANK_ADDR, J_BANK_ADDR + J_BANK_LEN, bruce_sdr_bank);
#endif

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static volatile uint32_t *s_bank = nullptr; // bank as seen by the CPU
static uint32_t s_usage_saved = 0;          // SENSITIVE_INTERNAL_SRAM_USAGE_3_REG before we touched it
static uint32_t s_usage_bank = 0;           // one-hot bit handing the bank to the engine
static bool s_ready = false;
static uint32_t s_freq_mhz = J_CAL_CHANNEL_MHZ;

enum JamRate : uint8_t { RATE_16M = 0, RATE_40M = 1, RATE_80M = 2 };
static enum JamRate s_rate = RATE_40M;
static int s_tx_power_qdbm = 78;   // 0.25 dBm units, applied before each takeover
static uint8_t s_ch = 1;           // Wi-Fi channel when !s_custom
static bool s_custom = false;      // off-grid frequency instead of a Wi-Fi channel
static uint32_t s_pending_mhz = 0; // custom frequency chosen in the menu, applied at takeover

static const uint32_t J_CH_FREQ[14] = {
    2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484
};

static uint32_t rate_hz(enum JamRate r) {
    switch (r) {
        case RATE_16M: return 16000000U;
        case RATE_40M: return 40000000U;
        case RATE_80M: return 80000000U;
    }
    return 0;
}

static const char *rate_name(enum JamRate r) {
    switch (r) {
        case RATE_16M: return "16 MS/s";
        case RATE_40M: return "40 MS/s";
        case RATE_80M: return "80 MS/s";
    }
    return "?";
}

static uint32_t rate_ctrl_bits(enum JamRate r) {
    switch (r) {
        case RATE_16M: return J_DUMP_CTRL_16M;
        case RATE_40M: return J_DUMP_CTRL_40M;
        case RATE_80M: return 0; // ADC side runs 80 MS/s with no rate bits set
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Receive gain: manual force through the documented AGC register (no librftest
// needed — the bits are in PBJ3000's register doc). -1 = AGC free-running.
// ---------------------------------------------------------------------------
static int s_rx_gain = -1;

static uint32_t rx_gain_max(void) {
    uint32_t max = (REG_READ(J_AGC_FORCE_REG) & J_AGC_GAIN_MAX_MSK) >> J_AGC_GAIN_MAX_S;
    return max && max <= J_AGC_GAIN_VALID_MAX ? max : J_AGC_GAIN_VALID_MAX;
}

// force_rx_gain(force, index, bt=0): AGC index + force bit, then the
// bt_rx_force(0) step dance on FE_R110 bits 9:8 (per PBJ3000's decompile).
static void rx_gain_apply_locked(bool manual, uint32_t index) {
    uint32_t agc = REG_READ(J_AGC_FORCE_REG);
    agc &= ~((0xffU << 24) | J_AGC_FORCE_EN);
    if (manual) agc |= ((index & 0xffU) << 24) | J_AGC_FORCE_EN;
    REG_WRITE(J_AGC_FORCE_REG, agc);

    uint32_t r110 = REG_READ(J_FE_R110_REG);
    REG_WRITE(J_FE_R110_REG, (r110 & ~J_FE_R110_B9_8_MSK) | 0x200U);
    REG_WRITE(J_FE_R110_REG, (r110 & ~J_FE_R110_B9_8_MSK) | 0x300U);
    REG_WRITE(J_FE_R110_REG, (r110 & ~J_FE_R110_B9_8_MSK));
}

static void rx_gain_apply(void) {
    if (s_rx_gain >= 0) rx_gain_apply_locked(true, (uint32_t)s_rx_gain);
}

static void rx_gain_release(void) { rx_gain_apply_locked(false, 0); }

// ---------------------------------------------------------------------------
// Analog I2C helpers (regi2c critical section is the vendor PHY's own lock)
// ---------------------------------------------------------------------------
// rx_lo_select() from upstream esp-sdr rx_lo.h, ESP32-S3 constants.
static void rx_lo_select(bool alternate) {
    unsigned int old = rom_chip_i2c_readReg(J_LO_BLOCK, 1, J_LO_REG);
    unsigned int value = (old & ~J_LO_MASK) | (alternate ? J_LO_MASK : 0u);
    if (value != old) rom_chip_i2c_writeReg(J_LO_BLOCK, 1, J_LO_REG, value);
}

static bool lo_divided(void) { return s_freq_mhz < J_LO_DIVIDED_BELOW_MHZ; }

// rx_lo_plan() from upstream esp-sdr rx_lo.h: the 6/5-divided LO maps
// 1842..2209 MHz into the conservative 2210..2651 MHz PLL envelope; other
// requests try the PLL directly.
struct LoPlan {
    uint32_t mhz;
    int offset_khz;
    bool alternate;
};
static struct LoPlan rx_lo_plan(uint32_t mhz) {
    bool alternate = mhz >= 1842 && mhz < J_LO_DIVIDED_BELOW_MHZ;
    uint32_t khz = mhz * (alternate ? 1200U : 1000U);
    return {khz / 1000U, (int)(khz % 1000U), alternate};
}

// True on a 2.4 GHz Wi-Fi channel frequency: those go through the vendor's
// calibrated channel setup instead of raw PLL programming.
static bool is_channel(uint32_t mhz) {
    return (mhz >= 2412U && mhz <= 2472U && (mhz - 2412U) % 5U == 0U) || mhz == 2484U;
}

// PBJ3000 esp_sdr.c: 0 in bits 3:2 of block 0x62 reg 12 means the VCO locked
// inside its tuning window; outside it the PLL may still catch the edge of the
// window for ~12 MHz, so treat anything else as -ERANGE.
static bool pll_in_window(void) {
    unsigned int status = rom_chip_i2c_readReg(J_RFPLL_BLOCK, 1, J_RFPLL_STATUS_REG);
    return (status & J_RFPLL_STATUS_VCO) == 0U;
}

// ---------------------------------------------------------------------------
// Front ends (ported from esp_sdr.c / esp_sdr_rx.c / esp_sdr_tx.c, minus the
// librftest gain ladders we do not have: gains stay on the PHY's calibrated
// tables and the AGC stays free-running)
// ---------------------------------------------------------------------------

// force_txon_mode(en, 0, 0): bit 1 forces the front end to stay in TX mode —
// without it the MAC never enables the transmitter outside a real frame.
static void fe_force_txon(bool en) {
    uint32_t v = REG_READ(J_FE_TX_CTRL_REG);
    uint32_t base = (v >> 18) & 0xffU; // keep the front end on its current base gain index
    v &= ~J_FE_GAIN_IDX_MSK;
    v |= base << 10;
    if (en) v |= J_FE_FORCE_TXON;
    else v &= ~J_FE_FORCE_TXON;
    REG_WRITE(J_FE_TX_CTRL_REG, v);
}

// Retune the LO to s_freq_mhz (PBJ3000 esp_sdr.c esp_sdr_tune(), S3 path).
// Wi-Fi channels go through the vendor channel setup so the PHY uses its
// calibrated tables; anything else programs the VCO directly, through the 6/5
// divider below 2210 MHz.
static void sdr_tune(void) {
    struct LoPlan plan = rx_lo_plan(s_freq_mhz);
    bool channel = is_channel(s_freq_mhz);

    regi2c_enter_critical();
    rx_lo_select(false); // normal LO while the vendor setup runs
    regi2c_exit_critical();
    set_chanfreq(channel ? s_freq_mhz : J_CAL_CHANNEL_MHZ, 0); // 0 = 20 MHz bw flag
    if (!channel) {
        // The channel call goes through mhz2ieee and loses off-grid requests;
        // the raw PLL programming does not. Crystal selector 0 = 40 MHz.
        set_rf_freq_offset(0, plan.mhz, plan.offset_khz);
    }
}

// Front end to RX (the module's resting state); retune=true also programs the LO.
static void rx_front_end(bool retune, uint32_t settle_us) {
    if (retune) sdr_tune();
    stop_tx_tone(1);
    fe_force_txon(false);
    rom_pbus_workmode();
    rom_pbus_xpd_tx_off();
    rom_pbus_xpd_rx_on(1);
    rom_set_rxclk_en(1);
    rx_gain_apply();
    regi2c_enter_critical();
    rx_lo_select(lo_divided()); // divided-LO band selected after RX setup, as upstream
    regi2c_exit_critical();
    delayMicroseconds(settle_us);
}

// Front end to TX; caller supplies the settle delay (PLL/front-end turnaround).
static void tx_front_end(void) {
    sdr_tune();
    rom_pbus_workmode();
    rom_pbus_xpd_rx_on(0);
    rom_pbus_xpd_tx_on(1);
    regi2c_enter_critical();
    rx_lo_select(lo_divided());
    regi2c_exit_critical();
    fe_force_txon(true); // must come last: without it the DAC burst stays silent
}

static void tx_prepare(void) {
    tx_front_end();
    delayMicroseconds(J_RETUNE_SETTLE_US);
}

static void sdr_init_radio(void) {
    // esp_wifi must be running: the PHY brings the radio up and calibrates it.
    esp_wifi_set_ps(WIFI_PS_NONE);
    // Promiscuous first: NULL mode only accepts a channel in promiscuous mode.
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    s_usage_saved = REG_READ(SENSITIVE_INTERNAL_SRAM_USAGE_3_REG);
    s_usage_bank = J_BANK_USAGE;
    s_bank = (volatile uint32_t *)J_BANK_ADDR;
    // s_freq_mhz is chosen by the caller (Wi-Fi channel or custom MHz).

    rx_front_end(true, J_RETUNE_SETTLE_US);
    s_ready = true;
}

static void sdr_deinit_radio(void) {
    if (!s_ready) return;
    REG_WRITE(J_DAC_TRIG_REG, 0);
    REG_WRITE(J_DUMP_CTRL_REG, 0);
    REG_WRITE(SENSITIVE_INTERNAL_SRAM_USAGE_3_REG, s_usage_saved);
    fe_force_txon(false);
    rx_gain_release();                      // hand the AGC back before resting
    rx_front_end(true, J_RETUNE_SETTLE_US); // rest in the RX-ready state, like upstream
    esp_wifi_set_promiscuous(false);
    s_ready = false;
}

// Hand the bank to the engine (CPU keeps the rest of the SRAM field untouched).
static inline void bank_to_engine(void) {
    REG_WRITE(
        SENSITIVE_INTERNAL_SRAM_USAGE_3_REG,
        (s_usage_saved & ~SENSITIVE_INTERNAL_SRAM_MAC_DUMP_USAGE_M) | s_usage_bank
    );
}

static inline void bank_to_cpu(void) { REG_WRITE(SENSITIVE_INTERNAL_SRAM_USAGE_3_REG, s_usage_saved); }

// ---------------------------------------------------------------------------
// Dump engine: DAC playback (TX) and ADC capture (RX)
// ---------------------------------------------------------------------------

// One DAC burst over the first count words of the bank. Caller has already
// handed the bank to the engine.
static void dac_trigger(bool rate80, uint32_t count) {
    uint32_t ctrl = J_DUMP_CTRL_RUN | (rate80 ? J_DAC_CTRL_80M : 0U) | (count & J_DUMP_CTRL_COUNT);
    REG_WRITE(J_DAC_TRIG_REG, 0);
    REG_WRITE(J_DAC_TRIG_REG, ctrl);
    REG_WRITE(J_DAC_TRIG_REG, ctrl | J_DUMP_CTRL_TRIG);
    REG_WRITE(J_DAC_TRIG_REG, ctrl);
}

static bool dac_wait_done(void) {
    int64_t t0 = esp_timer_get_time();
    while (!(REG_READ(J_DAC_TRIG_REG) & J_DUMP_CTRL_DONE)) {
        if (esp_timer_get_time() - t0 > J_DUMP_TIMEOUT_US) return false;
    }
    return true;
}

// Capture count raw I/Q words into the bank. Returns false on timeout.
static bool adc_capture(enum JamRate rate, uint32_t count) {
    if (count < J_SAMPLES_MIN || count > J_SAMPLES_MAX) return false;

    // A forced gain is occasionally lost (PBJ3000: after some thousand
    // front-end switches the AGC takes over and the floor jumps); re-forcing
    // at every capture is cheap and keeps the floor flat.
    rx_gain_apply();

    REG_WRITE(J_DUMP_CTRL_REG, 0);
    REG_WRITE(J_DUMP_CFG_REG, J_DUMP_CFG_IQ);

    // Sentinels: the engine fills the bank in order; survivors mean a short
    // capture. Written while the CPU still owns the bank — the engine only
    // sees CPU writes made before the handover.
    s_bank[0] = 0xa5a0055aU;
    s_bank[count - 1] = 0xa5a0055aU;

    bank_to_engine();

    uint32_t ctrl = J_DUMP_CTRL_RUN | rate_ctrl_bits(rate) | (count & J_DUMP_CTRL_COUNT);
    REG_WRITE(J_DUMP_CTRL_REG, ctrl);
    REG_WRITE(J_DUMP_CTRL_REG, ctrl | J_DUMP_CTRL_TRIG);
    REG_WRITE(J_DUMP_CTRL_REG, ctrl);

    int64_t t0 = esp_timer_get_time();
    bool done;
    do {
        done = (REG_READ(J_DUMP_CTRL_REG) & J_DUMP_CTRL_DONE) != 0U;
    } while (!done && esp_timer_get_time() - t0 < J_DUMP_TIMEOUT_US);

    REG_WRITE(J_DUMP_CTRL_REG, 0);
    bank_to_cpu();

    if (!done) return false;
    return s_bank[0] != 0xa5a0055aU && s_bank[count - 1] != 0xa5a0055aU;
}

// RX word layout: Q in bits 21:12, I in bits 31:22 (both 10-bit, left aligned).
static inline int16_t rx_word_i(uint32_t w) { return (int16_t)((int32_t)(w << 12) >> 22); }
static inline int16_t rx_word_q(uint32_t w) { return (int16_t)((int32_t)(w << 22) >> 22); }

// TX word layout: I in bits 9:0, Q in bits 19:10.
static inline uint32_t tx_word(int16_t i, int16_t q) {
    return ((uint32_t)i & 0x3ffU) | (((uint32_t)q & 0x3ffU) << 10);
}

// ---------------------------------------------------------------------------
// Waveforms
// ---------------------------------------------------------------------------

// xorshift32: fast white noise, one update per complex sample.
struct NoiseGen {
    uint32_t st = 0x1234abcd;
    uint16_t amp = 400;
    uint32_t next() {
        st ^= st << 13;
        st ^= st >> 17;
        st ^= st << 5;
        return st;
    }
    uint32_t word() {
        uint32_t r = next();
        int16_t i = (int16_t)((r & 0x3ffU) - 512);
        int16_t q = (int16_t)(((r >> 10) & 0x3ffU) - 512);
        return tx_word((int16_t)((i * amp) >> 9), (int16_t)((q * amp) >> 9));
    }
};

struct ToneGen {
    uint32_t fs = 40000000U;
    int32_t f_off_hz = 0; // tone offset from the LO
    uint16_t amp = 400;
    double phase = 0.0;
    uint32_t word() {
        phase += 2.0 * M_PI * (double)f_off_hz / (double)fs;
        if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
        return tx_word((int16_t)(sinf((float)phase) * amp), (int16_t)(cosf((float)phase) * amp));
    }
};

static void fill_bank(NoiseGen &g) {
    bank_to_cpu();
    for (uint32_t j = 0; j < J_SAMPLES_MAX; j++) s_bank[j] = g.word();
}

static void fill_bank(ToneGen &g) {
    bank_to_cpu();
    for (uint32_t j = 0; j < J_SAMPLES_MAX; j++) s_bank[j] = g.word();
}

// ---------------------------------------------------------------------------
// WiFi bring-up/down (pattern from jam_detect.cpp)
// ---------------------------------------------------------------------------
static bool jammer_wifi_start(void) {
    ensureWifiPlatform();
    nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t e = esp_wifi_init(&cfg);
    if (e != ESP_OK && e != ESP_ERR_WIFI_INIT_STATE) {
        Serial.printf("[SDR] wifi_init: %s\n", esp_err_to_name(e));
        return false;
    }
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    e = esp_wifi_start();
    if (e != ESP_OK && e != ESP_ERR_WIFI_INIT_STATE) {
        Serial.printf("[SDR] wifi_start: %s\n", esp_err_to_name(e));
        return false;
    }
    esp_wifi_disconnect();
    // Only knob we have without the vendor librftest blob: the PHY applies its
    // power setting to the gain tables it loaded for the channel.
    esp_wifi_set_max_tx_power(s_tx_power_qdbm);
    return true;
}

static void jammer_wifi_stop(void) {
    esp_wifi_stop();
    wifiDisconnect();
    drawStatusBar();
}

static bool jammer_start(void) {
    // stopWebUi() itself would crash if the WebUI never started (it
    // dereferences the server pointer unconditionally); use the guarded one.
    cleanlyStopWebUiForWiFiFeature();
    Serial.println("[SDR] webui stopped, starting wifi");
    if (!jammer_wifi_start()) return false;
    Serial.println("[SDR] wifi up, taking over the radio");
    sdr_init_radio();
    Serial.println("[SDR] radio takeover complete");
    return true;
}

static void jammer_stop(void) {
    sdr_deinit_radio();
    jammer_wifi_stop();
}

// ---------------------------------------------------------------------------
// UI helpers
// ---------------------------------------------------------------------------

// Shared live screen frame: title, one status line, hint line.
static void jammer_frame(const char *subtitle) {
    drawMainBorderWithTitle("SDR");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawCentreString(subtitle, tft.width() / 2, 32, 1);
    tft.drawCentreString("ESC: stop   < >: tune freq", tft.width() / 2, tft.height() - 18, 1);
}

static void jammer_status(const String &l1, const String &l2 = "") {
    tft.fillRect(6, 50, tft.width() - 12, 40, bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawCentreString(l1, tft.width() / 2, 58, 1);
    if (l2.length()) tft.drawCentreString(l2, tft.width() / 2, 70, 1);
}

// Label for the current tuning: "CH2 2417 MHz" on Wi-Fi channels, "2450 MHz"
// off-grid.
static String freq_label() {
    if (s_custom) return String(s_freq_mhz) + " MHz";
    return "CH" + String(s_ch) + " " + String(s_freq_mhz) + " MHz";
}

// Compact variant for menu rows.
static String freq_short() { return s_custom ? String(s_pending_mhz) + "M" : "CH" + String(s_ch); }

static uint32_t run_start_freq(void) { return s_custom ? s_pending_mhz : J_CH_FREQ[s_ch - 1]; }

// Switch the LO after a custom-frequency step failed the PLL window check.
static void retune_front_end(bool tx) {
    if (tx) {
        rx_front_end(false, 0);
        tx_prepare();
        return;
    }
    rx_front_end(true, J_RETUNE_SETTLE_US);
}

// Live tuning during a run: < > steps Wi-Fi channels, or +-1 MHz off-grid.
// Custom requests the VCO cannot lock are rejected by the PLL window check and
// stepped back. Returns true if the LO moved.
static bool live_retune(bool tx) {
    uint32_t old = s_freq_mhz;
    bool changed = false;
    if (s_custom) {
        if (check(NextPress) && s_freq_mhz < J_FREQ_API_MAX_MHZ) {
            s_freq_mhz++;
            changed = true;
        }
        if (check(PrevPress) && s_freq_mhz > J_FREQ_API_MIN_MHZ) {
            s_freq_mhz--;
            changed = true;
        }
    } else {
        if (check(NextPress) && s_ch < 14) {
            s_ch++;
            s_freq_mhz = J_CH_FREQ[s_ch - 1];
            changed = true;
        }
        if (check(PrevPress) && s_ch > 1) {
            s_ch--;
            s_freq_mhz = J_CH_FREQ[s_ch - 1];
            changed = true;
        }
    }
    if (!changed) return false;

    retune_front_end(tx);
    if (s_custom && !pll_in_window()) {
        s_freq_mhz = old;
        retune_front_end(tx);
    }
    return true;
}

static uint8_t pick_channel(void) {
    std::vector<Option> opts;
    int sel = -1;
    for (uint8_t c = 1; c <= 14; c++) {
        uint8_t ch = c;
        opts.push_back({"CH " + String(ch), [&sel, ch]() { sel = ch; }});
    }
    loopOptions(opts, MENU_TYPE_SUBMENU, "Channel");
    opts.clear();
    if (sel < 1 || sel > 14) return s_ch;
    return (uint8_t)sel;
}

// ---------------------------------------------------------------------------
// Run screens
// ---------------------------------------------------------------------------
static void run_noise_jam(void) {
    NoiseGen gen;
    rx_front_end(true, J_RETUNE_SETTLE_US);
    tx_prepare();
    fill_bank(gen);

    jammer_frame("NOISE");
    int64_t t0 = esp_timer_get_time();
    uint32_t bursts = 0;
    int64_t last_draw = 0;
    bool dirty = true; // refill the bank only when amp/freq changed: bursts must stay back-to-back
    bool timeout = false;
    while (!returnToMenu && !check(EscPress)) {
        bool changed = live_retune(true);
        if (check(UpPress)) {
            gen.amp = gen.amp < 500 ? gen.amp + 100 : 100;
            changed = true;
        }
        if (check(DownPress)) {
            gen.amp = gen.amp > 100 ? gen.amp - 100 : 500;
            changed = true;
        }
        dirty |= changed;

        if (dirty) {
            fill_bank(gen);
            dirty = false;
        }
        bank_to_engine();
        dac_trigger(s_rate == RATE_80M, J_SAMPLES_MAX);
        if (!dac_wait_done()) {
            timeout = true;
            bank_to_cpu();
            break;
        }
        bursts++;

        if (esp_timer_get_time() - last_draw > 250000) {
            last_draw = esp_timer_get_time();
            jammer_status(
                freq_label() + "  " + rate_name(s_rate),
                "amp " + String(gen.amp) + "  " + String((int)((esp_timer_get_time() - t0) / 1000000)) +
                    "s  " + String(bursts) + " bursts"
            );
        }
    }
    bank_to_cpu();
    REG_WRITE(J_DAC_TRIG_REG, 0);
    if (timeout) jammer_status("DAC timeout");
    rx_front_end(false, J_RETUNE_SETTLE_US); // resting RX state
}

static void run_tone_jam(void) {
    ToneGen gen;
    gen.fs = rate_hz(s_rate);
    rx_front_end(true, J_RETUNE_SETTLE_US);
    tx_prepare();
    fill_bank(gen);

    jammer_frame("CW TONE");
    int64_t t0 = esp_timer_get_time();
    uint32_t bursts = 0;
    int64_t last_draw = 0;
    bool dirty = true; // refill the bank only when offset/freq changed
    bool timeout = false;
    while (!returnToMenu && !check(EscPress)) {
        bool changed = live_retune(true);
        if (check(UpPress) && gen.f_off_hz < 5000000) {
            gen.f_off_hz += 100000;
            changed = true;
        }
        if (check(DownPress) && gen.f_off_hz > -5000000) {
            gen.f_off_hz -= 100000;
            changed = true;
        }
        dirty |= changed;

        if (dirty) {
            fill_bank(gen);
            dirty = false;
        }
        bank_to_engine();
        dac_trigger(s_rate == RATE_80M, J_SAMPLES_MAX);
        if (!dac_wait_done()) {
            timeout = true;
            bank_to_cpu();
            break;
        }
        bursts++;

        if (esp_timer_get_time() - last_draw > 250000) {
            last_draw = esp_timer_get_time();
            jammer_status(
                freq_label() + String(gen.f_off_hz >= 0 ? " +" : " ") + String(gen.f_off_hz / 1000) + " kHz",
                String((int)((esp_timer_get_time() - t0) / 1000000)) + "s  " + String(bursts) + " bursts"
            );
        }
    }
    bank_to_cpu();
    REG_WRITE(J_DAC_TRIG_REG, 0);
    if (timeout) jammer_status("DAC timeout");
    rx_front_end(false, J_RETUNE_SETTLE_US);
}

// Hop channels 1..14, broadcasting noise bursts on each — broadband interference
// across the whole 2.4 GHz band, the classic "jammer sweep".
static void run_sweep_jam(void) {
    // Hops Wi-Fi channels 1..14 regardless of the custom-frequency setting;
    // the next run re-applies the chosen frequency.
    NoiseGen gen;
    const uint32_t DWELL_BURSTS = 320; // ~130 ms of TX per channel at 40 MS/s

    jammer_frame("SWEEP");
    fill_bank(gen);
    int64_t t0 = esp_timer_get_time();
    bool timeout = false;
    while (!returnToMenu && !check(EscPress)) {
        for (uint8_t ch = 1; ch <= 14 && !returnToMenu && !check(EscPress); ch++) {
            s_freq_mhz = J_CH_FREQ[ch - 1];
            rx_front_end(true, 0); // retune + RX-ready turnaround
            tx_prepare();
            jammer_status(
                "CH" + String(ch) + "  " + String(s_freq_mhz) + " MHz  " +
                String((int)((esp_timer_get_time() - t0) / 1000000)) + "s"
            );
            for (uint32_t b = 0; b < DWELL_BURSTS && !returnToMenu && !check(EscPress); b++) {
                bank_to_engine();
                dac_trigger(s_rate == RATE_80M, J_SAMPLES_MAX);
                if (!dac_wait_done()) {
                    timeout = true;
                    break;
                }
            }
            bank_to_cpu();
            if (timeout) break;
        }
        if (timeout) break;
    }
    REG_WRITE(J_DAC_TRIG_REG, 0);
    if (timeout) jammer_status("DAC timeout");
    rx_front_end(true, J_RETUNE_SETTLE_US);
}

// Raw I/Q spectrum analyzer: ADC captures at the current rate, FFT, live plot.
static void run_iq_spectrum(void) {
    // 256-point complex FFT: the plan is three 2 KiB arrays (~6 KiB total), which
    // fits the fragmented heap left once WiFi is up. 256 is also adc_capture's
    // minimum and gives one bin per plot pixel, so a bigger N buys no resolution.
    const uint32_t N = 256;

    // Responsive layout: derive every Y from the screen size so nothing overlaps
    // on short panels (e.g. Cardputer 240x135) or wastes space on tall ones.
    // jammer_frame() owns the title + "IQ SPECTRUM" subtitle at the top and the
    // "ESC: stop" line at height-18; the plot and its freq/rate readout live in
    // the gap between the two.
    const int PLOT_X = 8;
    const int PLOT_Y = 42;                       // just under the subtitle
    const int PLOT_W = tft.width() - 2 * PLOT_X;
    const int INFO_Y = tft.height() - 18 - 10;   // freq/rate line, above the footer
    const int PLOT_H = (INFO_Y - 4) - PLOT_Y;    // plot fills everything in between

    rx_front_end(true, J_RETUNE_SETTLE_US);

    jammer_frame("IQ SPECTRUM");

    // The FFT library mallocs its twiddle/input/output buffers and never checks
    // the result, so a failed alloc stores straight through a NULL pointer and
    // panics (StoreProhibited in fft_init). Build the plan ONCE, up front, where
    // the heap is least fragmented -- never per frame -- and bail cleanly if we
    // can't. The three arrays are 2*N floats each; require one contiguous block
    // big enough for the whole plan so none of the mallocs can fail mid-init.
    const size_t PLAN_BYTES = 3 * (2 * N * sizeof(float)) + 256; // +config & malloc headers
    if (ESP.getMaxAllocHeap() < PLAN_BYTES) {
        jammer_status("FFT out of memory", "close other radios");
        Serial.printf(
            "[SDR] FFT init skipped: max alloc heap %u < %u needed\n",
            ESP.getMaxAllocHeap(),
            (unsigned)PLAN_BYTES
        );
        delay(1500);
        rx_front_end(true, J_RETUNE_SETTLE_US);
        return;
    }
    fft_config_t *plan = fft_init(N, FFT_COMPLEX, FFT_FORWARD, NULL, NULL);
    if (!plan) {
        jammer_status("FFT init failed", "");
        Serial.println("[SDR] FFT init failed: fft_init returned NULL");
        delay(1500);
        rx_front_end(true, J_RETUNE_SETTLE_US);
        return;
    }

    tft.drawRect(PLOT_X - 2, PLOT_Y - 2, PLOT_W + 4, PLOT_H + 4, bruceConfig.priColor);

    int64_t last_draw = 0;
    uint32_t captures = 0, timeouts = 0, failStreak = 0;
    String lastInfo; // redraw the freq/rate line only when it changes (no flicker)
    while (!returnToMenu && !check(EscPress)) {
        live_retune(false);

        if (!adc_capture(s_rate, N)) {
            timeouts++;
            // Never spin silently: give up if the engine stops delivering.
            if (++failStreak > 20) {
                jammer_status("capture failing", "check radio state");
                delay(1200);
                break;
            }
            continue;
        }
        failStreak = 0;
        captures++;

        if (esp_timer_get_time() - last_draw < 40000) continue; // ~25 fps cap
        last_draw = esp_timer_get_time();

        for (uint32_t j = 0; j < N; j++) {
            plan->input[2 * j] = (float)rx_word_i(s_bank[j]);
            plan->input[2 * j + 1] = (float)rx_word_q(s_bank[j]);
        }
        fft_execute(plan);

        // FFT shift: bins wrap around DC; plot the full span with DC centered.
        // Repaint each column in place -- bar in priColor, the space above it in
        // bgColor -- instead of clearing the whole rect first, which flickers.
        uint32_t half = N / 2;
        for (uint16_t x = 0; x < PLOT_W; x++) {
            uint32_t bin = ((uint32_t)x * N / PLOT_W + half) % N; // wrap so DC sits in the middle
            float re = plan->output[2 * bin];
            float im = plan->output[2 * bin + 1];
            float mag = sqrtf(re * re + im * im) / (float)N;
            float db = 20.0f * log10f(mag + 1e-6f);
            int h = (int)((db + 60.0f) * PLOT_H / 60.0f); // -60..0 dB window
            if (h < 0) h = 0;
            if (h > PLOT_H - 1) h = PLOT_H - 1;
            if (h < PLOT_H) tft.drawFastVLine(PLOT_X + x, PLOT_Y, PLOT_H - h, bruceConfig.bgColor);
            if (h > 0) tft.drawFastVLine(PLOT_X + x, PLOT_Y + PLOT_H - h, h, bruceConfig.priColor);
        }

        String info = freq_label() + "  " + rate_name(s_rate);
        if (info != lastInfo) {
            tft.fillRect(PLOT_X, INFO_Y, PLOT_W, 10, bruceConfig.bgColor);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setTextSize(FP);
            tft.drawCentreString(info, tft.width() / 2, INFO_Y, 1);
            lastInfo = info;
        }
    }

    fft_destroy(plan);

    jammer_status(String(captures) + " captures", String(timeouts) + " timeouts");
    delay(600);
    rx_front_end(true, J_RETUNE_SETTLE_US);
}

// ---------------------------------------------------------------------------
// SDR file modes: raw I/Q to/from SD as CS16 (interleaved int16 I/Q, the
// SDR#/URH standard; our 10-bit samples go in the top bits). Record keeps the
// CPU side of the radio: SD runs on its own SPI bus, the dump engine only
// owns the SRAM bank between capture and hand-back.
// ---------------------------------------------------------------------------
static const char *IQ_DIR = "/BruceIQ";

// Convert one bank of raw words to CS16 bytes in chunks; returns bytes written
// or 0 on failure. buf must hold 2 * CHUNK words * 4 bytes.
static size_t bank_to_cs16(File &f, uint8_t *buf, const size_t bufBytes) {
    const uint32_t chunkWords = bufBytes / 4; // 2 int16 per word
    size_t total = 0;
    for (uint32_t j = 0; j < J_SAMPLES_MAX; j += chunkWords) {
        uint32_t n = chunkWords < J_SAMPLES_MAX - j ? chunkWords : J_SAMPLES_MAX - j;
        int16_t *out = (int16_t *)buf;
        for (uint32_t k = 0; k < n; k++) {
            uint32_t w = s_bank[j + k];
            out[2 * k] = (int16_t)(rx_word_i(w) << 6);
            out[2 * k + 1] = (int16_t)(rx_word_q(w) << 6);
        }
        size_t bytes = n * 4;
        if (f.write(buf, bytes) != bytes) return total;
        total += bytes;
    }
    return total;
}

// The heap after the radio takeover is small and fragmented (Cardputer:
// ~24 kB free at the menu, largest block ~16 kB) — take the biggest chunk it
// will still give. The converters are chunk-based, so any size works.
static uint8_t *alloc_convert_buf(size_t &bufBytes) {
    static const size_t wants[] = {8192, 4096, 2048, 1024};
    for (size_t want : wants) {
        uint8_t *p = (uint8_t *)malloc(want);
        if (p) {
            bufBytes = want;
            return p;
        }
    }
    return nullptr;
}

// Capture snapshots back-to-back into one CS16 file until ESC.
static void run_iq_record(void) {
    size_t bufBytes = 0;
    uint8_t *buf = alloc_convert_buf(bufBytes);
    if (!buf) {
        Serial.printf(
            "[SDR] record OOM: free=%u largest=%u\n",
            (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
            (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)
        );
        displayError("Out of memory", true);
        return;
    }

    SD.mkdir(IQ_DIR);
    String path;
    for (int i = 1; i < 1000; i++) {
        path = String(IQ_DIR) + "/iq_" + String(i) + ".cs16";
        if (!SD.exists(path)) break;
        path = "";
    }
    if (!path.length()) {
        free(buf);
        displayError("No free iq_N name", true);
        return;
    }
    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        free(buf);
        displayError("SD open failed", true);
        return;
    }

    rx_front_end(true, J_RETUNE_SETTLE_US);
    jammer_frame("IQ RECORD");
    int64_t t0 = esp_timer_get_time();
    uint32_t snaps = 0;
    size_t total = 0;
    int64_t last_draw = 0;
    while (!returnToMenu && !check(EscPress)) {
        live_retune(false);

        if (!adc_capture(s_rate, J_SAMPLES_MAX)) continue;
        snaps++;
        total += bank_to_cs16(f, buf, bufBytes);

        if (esp_timer_get_time() - last_draw > 500000) {
            last_draw = esp_timer_get_time();
            jammer_status(
                freq_label() + "  " + rate_name(s_rate),
                String(total / 1024) + " KiB  " + String(snaps) + " snaps"
            );
        }
    }
    f.close();
    free(buf);

    jammer_status(path, String(total / 1024) + " KiB, " + String(snaps) + " snapshots");
    Serial.println("[SDR] recorded " + path + " (" + String(total) + " bytes)");
    delay(1200);
    rx_front_end(true, J_RETUNE_SETTLE_US);
}

// Replay a CS16 file through the DAC: PBJ3000's arbitrary I/Q transmission,
// fed from SD instead of the Android app's USB stream. Loops until ESC.
static void run_iq_play(File &f) {
    size_t bufBytes = 0;
    uint8_t *buf = alloc_convert_buf(bufBytes);
    if (!buf) {
        Serial.printf(
            "[SDR] play OOM: free=%u largest=%u\n",
            (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
            (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)
        );
        displayError("Out of memory", true);
        return;
    }

    rx_front_end(true, J_RETUNE_SETTLE_US);
    tx_prepare();
    jammer_frame("IQ PLAY");
    int64_t t0 = esp_timer_get_time();
    uint32_t bursts = 0;
    int64_t last_draw = 0;
    bool timeout = false;
    while (!returnToMenu && !check(EscPress)) {
        size_t got = f.read(buf, bufBytes);
        if (got < 4) { // EOF: loop the file
            f.seek(0);
            continue;
        }
        size_t samples = got / 4;

        // Tile the chunk across the whole bank: fewer retrigger holes.
        bank_to_cpu();
        for (uint32_t j = 0; j < J_SAMPLES_MAX; j++) {
            const uint8_t *s = buf + (j % samples) * 4;
            int16_t i = (int16_t)(s[0] | (s[1] << 8)) >> 6;
            int16_t q = (int16_t)(s[2] | (s[3] << 8)) >> 6;
            s_bank[j] = tx_word(i, q);
        }
        bank_to_engine();
        dac_trigger(s_rate == RATE_80M, J_SAMPLES_MAX);
        if (!dac_wait_done()) {
            timeout = true;
            bank_to_cpu();
            break;
        }
        bursts++;

        if (esp_timer_get_time() - last_draw > 500000) {
            last_draw = esp_timer_get_time();
            jammer_status(
                freq_label() + "  " + rate_name(s_rate),
                String((int)((esp_timer_get_time() - t0) / 1000000)) + "s  " + String(bursts) + " bursts"
            );
        }
    }
    free(buf);
    REG_WRITE(J_DAC_TRIG_REG, 0);
    if (timeout) jammer_status("DAC timeout");
    rx_front_end(false, J_RETUNE_SETTLE_US);
}

// ---------------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------------
static void pick_tx_power(void) {
    std::vector<Option> opts;
    opts.push_back({"+19.5 dBm", [&]() { s_tx_power_qdbm = 78; }});
    opts.push_back({"+17 dBm", [&]() { s_tx_power_qdbm = 68; }});
    opts.push_back({"+13 dBm", [&]() { s_tx_power_qdbm = 52; }});
    opts.push_back({"+10 dBm", [&]() { s_tx_power_qdbm = 40; }});
    opts.push_back({"+4 dBm", [&]() { s_tx_power_qdbm = 16; }});
    loopOptions(opts, MENU_TYPE_SUBMENU, "TX Power");
    opts.clear();
}

static void pick_rate(void) {
    std::vector<Option> opts;
    opts.push_back({"80 MS/s", [&]() { s_rate = RATE_80M; }});
    opts.push_back({"40 MS/s", [&]() { s_rate = RATE_40M; }});
    opts.push_back({"16 MS/s", [&]() { s_rate = RATE_16M; }});
    loopOptions(opts, MENU_TYPE_SUBMENU, "Sample rate");
    opts.clear();
}

static void pick_rx_gain(void) {
    uint32_t gmax = rx_gain_max();
    std::vector<Option> opts;
    opts.push_back({"Auto", [&]() { s_rx_gain = -1; }});
    const uint32_t steps[] = {24, 40, 56, 72};
    for (uint32_t g : steps) {
        if (g >= gmax) break;
        uint32_t idx = g;
        opts.push_back({String(idx), [&]() { s_rx_gain = (int)idx; }});
    }
    opts.push_back({"Max " + String(gmax), [&]() { s_rx_gain = (int)gmax; }});
    if (s_rx_gain > (int)gmax) s_rx_gain = (int)gmax;
    loopOptions(opts, MENU_TYPE_SUBMENU, "RX Gain");
    opts.clear();
}

// Wi-Fi channel or an off-grid frequency. Off-grid requests outside the VCO's
// reach are still accepted here and rejected with the PLL window check when a
// jam starts (like PBJ3000's -ERANGE), because the window can only be read
// from the radio after the takeover.
static void pick_frequency(void) {
    std::vector<Option> opts;
    opts.push_back({"WiFi channel", [&]() {
                        s_custom = false;
                        s_ch = pick_channel();
                    }});
    opts.push_back({"Custom MHz...", [&]() {
                        String v = num_keyboard(
                            String(s_pending_mhz),
                            6,
                            "MHz (" + String(J_FREQ_ADV_MIN_MHZ) + "-" + String(J_FREQ_ADV_MAX_MHZ) + "):"
                        );
                        if (!v.length()) return;
                        uint32_t mhz = (uint32_t)v.toInt();
                        if (mhz >= J_FREQ_API_MIN_MHZ && mhz <= J_FREQ_API_MAX_MHZ) {
                            s_custom = true;
                            s_pending_mhz = mhz;
                        } else {
                            displayError("100-6000 MHz only", true);
                        }
                    }});
    loopOptions(opts, MENU_TYPE_SUBMENU, "Frequency");
    opts.clear();
}

static void run_jammer(std::function<void()> run) {
    s_freq_mhz = run_start_freq();
    if (!jammer_start()) {
        displayError("WiFi init failed", true);
        return;
    }
    if (s_custom && !pll_in_window()) {
        // Like PBJ3000's -ERANGE: the S3 VCO only locks about 1.82-2.65 GHz.
        jammer_stop();
        displayError("PLL won't lock! Range ~1.82-2.65 GHz", true);
        return;
    }
    run();
    jammer_stop();
}

void sdr_menu() {
    returnToMenu = false;

    displayError("EMITS REAL RF POWER! Jamming may be ILLEGAL", true);

    std::vector<Option> opts;
    for (;;) {
        opts.clear();
        opts.push_back({"Noise Jam", [&]() {
                            if (!s_custom) s_ch = pick_channel();
                            run_jammer(run_noise_jam);
                        }});
        opts.push_back({"Tone Jam", [&]() {
                            if (!s_custom) s_ch = pick_channel();
                            run_jammer(run_tone_jam);
                        }});
        opts.push_back({"Sweep Jam", [&]() { run_jammer(run_sweep_jam); }});
        opts.push_back({"Spectrum", [&]() {
                            if (!s_custom) s_ch = pick_channel();
                            run_jammer(run_iq_spectrum);
                        }});
        opts.push_back({"Record IQ", [&]() {
                            if (!setupSdCard()) {
                                displayError("No SD card", true);
                                return;
                            }
                            if (!s_custom) s_ch = pick_channel();
                            run_jammer(run_iq_record);
                        }});
        opts.push_back({"Play IQ", [&]() {
                            if (!setupSdCard()) {
                                displayError("No SD card", true);
                                return;
                            }
                            String path = loopSD(SD, true, "cs16", "/");
                            if (!path.length()) return;
                            File f = SD.open(path, FILE_READ);
                            if (!f) {
                                displayError("SD open failed", true);
                                return;
                            }
                            run_jammer([&f]() { run_iq_play(f); });
                            f.close();
                        }});
        opts.push_back({"Freq: " + freq_short(), pick_frequency});
        opts.push_back({"TX Power", pick_tx_power});
        opts.push_back({"Gain: " + String(s_rx_gain < 0 ? "Auto" : String(s_rx_gain)), pick_rx_gain});
        opts.push_back({"Rate: " + String(rate_hz(s_rate) / 1000000U) + "M", pick_rate});
        opts.push_back({"Back", [&]() { returnToMenu = true; }});

        loopOptions(opts, MENU_TYPE_SUBMENU, "SDR");
        if (returnToMenu) break;
    }
    opts.clear();
}

#endif // CONFIG_IDF_TARGET_ESP32S3
#endif // !LITE_VERSION && !BRUCE_NO_SDR_JAMMER
