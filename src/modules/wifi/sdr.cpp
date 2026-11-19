/*
 * IQ Record — raw I/Q capture from the ESP32-S3 WiFi radio to the SD card
 * (CS16 files), the receive-only half of the esp_sdr port, in the spirit of
 * PBJ3000-ESP32-Jammer (https://github.com/pinchepasta/PBJ3000-ESP32-Jammer,
 * GPL-3.0-or-later), which is based on ESPARGOS/esp-sdr. The TX/jammer half of
 * the port (the DAC side) was removed.
 *
 * How it works, briefly:
 *  - esp_wifi is brought up normally and parked in promiscuous mode on a
 *    channel; that leaves the radio powered with the PHY calibrated.
 *  - A 64 KiB SRAM "dump bank" is taken away from the heap at boot
 *    (SOC_RESERVE_MEMORY_REGION) and handed to the undocumented MAC dump
 *    engine (SENSITIVE_INTERNAL_SRAM_USAGE_3_REG one-hot). The engine streams
 *    the bank through the radio as ADC (raw I/Q capture) at 16/40/80 MS/s.
 *  - Tuning uses libphy's set_chanfreq() for Wi-Fi channels and
 *    set_rf_freq_offset() for off-grid requests (2210-2650 MHz direct PLL,
 *    ~1817-2209 MHz through the 6/5-divided LO); the analog front end is
 *    switched with ROM pbus functions. Off-grid requests the VCO cannot lock
 *    are rejected via the RFPLL window flag (PBJ3000's -ERANGE).
 *
 * ESP32-C5: not supported here — even the receive path needs a full 128 KiB HP
 * SRAM block (block 1, 0x40820000, the only one that serves), and the
 * Arduino/IDF C5 layout puts .noinit/.bss inside it (measured:
 * 0x408246e9-0x40836B60), so donating the block would corrupt static data.
 * PBJ3000 avoids this with a Zephyr linker snippet that moves the statics;
 * Bruce's C5 linker scripts would have to be forked the same way before a C5
 * port is possible.
 *
 * Unlike PBJ3000 we do not ship the vendor librftest.a blob, so there are no
 * forced RX gain ladders: the radio runs on the gain tables the PHY calibrated
 * for the channel; the only manual knob is the documented AGC force register.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <Arduino.h> // pulls sdkconfig.h (CONFIG_SPIRAM / CONFIG_IDF_TARGET_ESP32S3)
#include "core/iq_record_gate.h"

#if !defined(BRUCE_IQ_RECORD_UNAVAILABLE)
// IQ Record — the capture engine reserves a 64 KiB internal-SRAM bank out of
// the heap at boot; see core/iq_record_gate.h for the availability rules.
// Everything below also needs the ESP32-S3: the undocumented MAC dump engine
// only exists there.

#include "sdr.h"

#include <esp_heap_caps.h>
#include <esp_heap_caps_init.h> // heap_caps_add_region: SDR bank <-> heap hand-off
#include <esp_private/regi2c_ctrl.h>
#include <esp_timer.h>
#include <esp_wifi.h>
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
// from the DRAM heap regions. Cost: exactly 64 KiB less internal RAM. Only
// compiled on PSRAM boards (core/iq_record_gate.h gates the whole file), which
// can afford it — PSRAM-less boards keep the heap intact for WiFi features.
// Measured on Cardputer-class boards: the reservation pushed the post-WiFi
// largest heap block below the Wifite worker stack, which is why this feature
// requires SPIRAM builds.
#define J_RESERVE_BANK 1

SOC_RESERVE_MEMORY_REGION(J_BANK_ADDR, J_BANK_ADDR + J_BANK_LEN, bruce_sdr_bank);

// ---------------------------------------------------------------------------
// Bank <-> heap hand-off
//
// The reserved bank is the ADC dump target, so it must never hold live heap
// data while a capture runs — but leaving it out of the heap permanently
// starves Wi-Fi/BLE bring-up (esp_bt_controller_init needs a big contiguous
// DMA block; on Cardputer the menu heap drops from ~90 kB to ~24 kB free).
//
// Fence protocol:
//   boot        bank reserved, invisible to the heap (SDR always usable)
//   sdrBankHeapInit()   (lazy, first radio bring-up) returns the bank to the
//                       heap and immediately allocates it all back as a fence;
//                       the fence keeps every other allocation out of the bank
//   radio low on memory radio_mem.h frees the fence so Wi-Fi/BLE can use the
//                       bank; SDR then refuses to start until reboot (clean
//                       error, never a crash)
// ---------------------------------------------------------------------------
static uint8_t *s_fence = nullptr;     // whole-bank allocation; null once a radio took the bank
static bool s_region_in_heap = false;  // heap_caps_add_region already done

void sdrBankHeapInit() {
    if (s_region_in_heap) return;
    if (heap_caps_add_region(J_BANK_ADDR, J_BANK_ADDR + J_BANK_LEN) != ESP_OK) return;
    s_region_in_heap = true;
    // The freshly added region is the largest free block, so this allocation
    // lands inside it and reserves the whole bank for the SDR engine.
    s_fence = (uint8_t *)heap_caps_malloc(J_BANK_LEN, MALLOC_CAP_8BIT);
}

// Called by radio_mem.h before Wi-Fi/BLE bring-up when the heap looks tight.
void sdrBankReleaseForRadio() {
    sdrBankHeapInit();
    if (s_fence != nullptr) {
        heap_caps_free(s_fence);
        s_fence = nullptr;
    }
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static volatile uint32_t *s_bank = nullptr; // bank as seen by the CPU
static uint32_t s_usage_saved = 0;          // SENSITIVE_INTERNAL_SRAM_USAGE_3_REG before we touched it
static uint32_t s_usage_bank = 0;           // one-hot bit handing the bank to the engine
static bool s_ready = false;
static uint32_t s_freq_mhz = J_CAL_CHANNEL_MHZ;

enum CaptureRate : uint8_t { RATE_16M = 0, RATE_40M = 1, RATE_80M = 2 };
static enum CaptureRate s_rate = RATE_40M;
static uint8_t s_ch = 1;           // Wi-Fi channel when !s_custom
static bool s_custom = false;      // off-grid frequency instead of a Wi-Fi channel
static uint32_t s_pending_mhz = 0; // custom frequency chosen in the menu, applied at takeover

static const uint32_t J_CH_FREQ[14] = {
    2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484
};

static uint32_t rate_hz(enum CaptureRate r) {
    switch (r) {
        case RATE_16M: return 16000000U;
        case RATE_40M: return 40000000U;
        case RATE_80M: return 80000000U;
    }
    return 0;
}

static const char *rate_name(enum CaptureRate r) {
    switch (r) {
        case RATE_16M: return "16 MS/s";
        case RATE_40M: return "40 MS/s";
        case RATE_80M: return "80 MS/s";
    }
    return "?";
}

static uint32_t rate_ctrl_bits(enum CaptureRate r) {
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

// force_txon_mode(false, 0, 0): bit 1 forces the front end to stay in TX mode;
// the RX path clears it so the front end can receive outside a real frame.
static void fe_force_txoff(void) {
    uint32_t v = REG_READ(J_FE_TX_CTRL_REG);
    uint32_t base = (v >> 18) & 0xffU; // keep the front end on its current base gain index
    v &= ~J_FE_GAIN_IDX_MSK;
    v |= base << 10;
    v &= ~J_FE_FORCE_TXON;
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
    fe_force_txoff();
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

static bool sdr_init_radio(void) {
    // If a radio session released the bank fence, heap allocations may live in
    // the bank now; capturing would corrupt the heap. Refuse cleanly instead.
    if (s_region_in_heap && s_fence == nullptr) {
        displayError("IQ Record needs reboot", true);
        return false;
    }
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
    return true;
}

static void sdr_deinit_radio(void) {
    if (!s_ready) return;
    REG_WRITE(J_DUMP_CTRL_REG, 0);
    REG_WRITE(SENSITIVE_INTERNAL_SRAM_USAGE_3_REG, s_usage_saved);
    fe_force_txoff();
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
// Dump engine: ADC capture (RX)
// ---------------------------------------------------------------------------

// Capture count raw I/Q words into the bank. Returns false on timeout.
static bool adc_capture(enum CaptureRate rate, uint32_t count) {
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

// ---------------------------------------------------------------------------
// WiFi bring-up/down (pattern from jam_detect.cpp)
// ---------------------------------------------------------------------------
static bool sdr_wifi_start(void) {
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
    return true;
}

static void sdr_wifi_stop(void) {
    esp_wifi_stop();
    wifiDisconnect();
    drawStatusBar();
}

// Take the radio over for one RX session: stop the WebUI, bring esp_wifi up
// (the PHY calibrates the radio), then hand the radio to the dump engine.
static bool sdr_start(void) {
    // stopWebUi() itself would crash if the WebUI never started (it
    // dereferences the server pointer unconditionally); use the guarded one.
    cleanlyStopWebUiForWiFiFeature();
    Serial.println("[SDR] webui stopped, starting wifi");
    if (!sdr_wifi_start()) return false;
    Serial.println("[SDR] wifi up, taking over the radio");
    if (!sdr_init_radio()) return false;
    Serial.println("[SDR] radio takeover complete");
    return true;
}

static void sdr_stop(void) {
    sdr_deinit_radio();
    sdr_wifi_stop();
}

// ---------------------------------------------------------------------------
// UI helpers
// ---------------------------------------------------------------------------

// Shared live screen frame: title, one status line, hint line.
static void sdr_frame(const char *subtitle) {
    drawMainBorderWithTitle("IQ Record");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawCentreString(subtitle, tft.width() / 2, 32, 1);
    tft.drawCentreString("ESC: stop   < >: tune freq", tft.width() / 2, tft.height() - 18, 1);
}

static void sdr_status(const String &l1, const String &l2 = "") {
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

// Live tuning during a run: < > steps Wi-Fi channels, or +-1 MHz off-grid.
// Custom requests the VCO cannot lock are rejected by the PLL window check and
// stepped back. Returns true if the LO moved.
static bool live_retune(void) {
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

    rx_front_end(true, J_RETUNE_SETTLE_US);
    if (s_custom && !pll_in_window()) {
        s_freq_mhz = old;
        rx_front_end(true, J_RETUNE_SETTLE_US);
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
// File mode: raw I/Q to SD as CS16 (interleaved int16 I/Q, the SDR#/URH
// standard; our 10-bit samples go in the top bits). Record keeps the CPU side
// of the radio: SD runs on its own SPI bus, the dump engine only owns the
// SRAM bank between capture and hand-back.
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
    sdr_frame("IQ RECORD");
    int64_t t0 = esp_timer_get_time();
    uint32_t snaps = 0;
    size_t total = 0;
    int64_t last_draw = 0;
    while (!returnToMenu && !check(EscPress)) {
        live_retune();

        if (!adc_capture(s_rate, J_SAMPLES_MAX)) continue;
        snaps++;
        total += bank_to_cs16(f, buf, bufBytes);

        if (esp_timer_get_time() - last_draw > 500000) {
            last_draw = esp_timer_get_time();
            sdr_status(
                freq_label() + "  " + rate_name(s_rate),
                String(total / 1024) + " KiB  " + String(snaps) + " snaps"
            );
        }
    }
    f.close();
    free(buf);

    sdr_status(path, String(total / 1024) + " KiB, " + String(snaps) + " snapshots");
    Serial.println("[SDR] recorded " + path + " (" + String(total) + " bytes)");
    delay(1200);
    rx_front_end(true, J_RETUNE_SETTLE_US);
}

// ---------------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------------
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
// reach are still accepted here and rejected with the PLL window check when the
// capture starts (like PBJ3000's -ERANGE), because the window can only be read
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

// Bring the radio up, run one capture session, take it down.
static void run_capture(std::function<void()> run) {
    s_freq_mhz = run_start_freq();
    if (!sdr_start()) {
        displayError("WiFi init failed", true);
        return;
    }
    if (s_custom && !pll_in_window()) {
        // Like PBJ3000's -ERANGE: the S3 VCO only locks about 1.82-2.65 GHz.
        sdr_stop();
        displayError("PLL won't lock! Range ~1.82-2.65 GHz", true);
        return;
    }
    run();
    sdr_stop();
}

void sdr_menu() {
    returnToMenu = false;

    std::vector<Option> opts;
    for (;;) {
        opts.clear();
        opts.push_back({"Record IQ", [&]() {
                            if (!setupSdCard()) {
                                displayError("No SD card", true);
                                return;
                            }
                            if (!s_custom) s_ch = pick_channel();
                            run_capture(run_iq_record);
                        }});
        opts.push_back({"Freq: " + freq_short(), pick_frequency});
        opts.push_back({"Gain: " + String(s_rx_gain < 0 ? "Auto" : String(s_rx_gain)), pick_rx_gain});
        opts.push_back({"Rate: " + String(rate_hz(s_rate) / 1000000U) + "M", pick_rate});
        opts.push_back({"Back", [&]() { returnToMenu = true; }});

        loopOptions(opts, MENU_TYPE_SUBMENU, "IQ Record");
        if (returnToMenu) break;
    }
    opts.clear();
}

#endif // CONFIG_IDF_TARGET_ESP32S3
#endif // !BRUCE_IQ_RECORD_UNAVAILABLE
