#include "esl_ir_driver.h"

#include "esl_pp4.h"
#include "esl_proto.h"
#include "modules/ir/ir_utils.h"
#include <Arduino.h>
#include <driver/rmt_tx.h>
#include <esp_rom_sys.h>
#include <globals.h>

static_assert(ESL_IR_MAX_FRAME_LEN >= TAGTINKER_MAX_FRAME_SIZE,
              "IR driver frame bound must cover every ESL protocol frame");

/* 80 MHz resolution gives 12.5 ns ticks: fine enough that the derived PP4
 * durations stay well inside RMT's 15-bit duration field (max gap ~19353). */
#define ESL_RMT_RESOLUTION_HZ 80000000u

/* 80 MHz / 64 = 1.25 MHz, within 0.4% of the Flipper's 64 MHz / 51. */
#define ESL_CARRIER_HZ 1250000u
#define ESL_CARRIER_DUTY 0.49f

#define ESL_RMT_MEM_BLOCK_SYMBOLS 64
#define ESL_RMT_QUEUE_DEPTH 2
#define ESL_RMT_DONE_TIMEOUT_MS 1000

#define ESL_IR_MAX_SYMBOLS (ESL_IR_MAX_FRAME_LEN * 4 + 1) /* 385 */

static rmt_channel_handle_t s_channel = nullptr;
static rmt_encoder_handle_t s_encoder = nullptr;
static int s_gpio = -1;
static volatile bool s_stop = false;
static volatile bool s_busy = false;

static EslPp4Symbol s_symbols[ESL_IR_MAX_SYMBOLS];
static rmt_symbol_word_t s_words[ESL_IR_MAX_SYMBOLS];

bool esl_ir_init(int gpio) {
    if (s_channel != nullptr) return true;

    s_gpio = gpio;
    setup_ir_pin(gpio, OUTPUT);
    digitalWrite(gpio, LED_OFF);

    rmt_tx_channel_config_t chan_cfg = {};
    chan_cfg.gpio_num = (gpio_num_t)gpio;
    chan_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
    chan_cfg.resolution_hz = ESL_RMT_RESOLUTION_HZ;
    chan_cfg.mem_block_symbols = ESL_RMT_MEM_BLOCK_SYMBOLS;
    chan_cfg.trans_queue_depth = ESL_RMT_QUEUE_DEPTH;
    if (rmt_new_tx_channel(&chan_cfg, &s_channel) != ESP_OK) {
        s_channel = nullptr;
        return false;
    }

    rmt_carrier_config_t carrier_cfg = {};
    carrier_cfg.frequency_hz = ESL_CARRIER_HZ;
    carrier_cfg.duty_cycle = ESL_CARRIER_DUTY;
    /* Carrier rides the high (mark) level, matching the active-high IR LED. */
    carrier_cfg.flags.polarity_active_low = false;
    carrier_cfg.flags.always_on = false;
    if (rmt_apply_carrier(s_channel, &carrier_cfg) != ESP_OK) {
        esl_ir_deinit();
        return false;
    }

    rmt_copy_encoder_config_t enc_cfg = {};
    if (rmt_new_copy_encoder(&enc_cfg, &s_encoder) != ESP_OK) {
        esl_ir_deinit();
        return false;
    }

    if (rmt_enable(s_channel) != ESP_OK) {
        esl_ir_deinit();
        return false;
    }

    s_stop = false;
    s_busy = false;
    return true;
}

void esl_ir_deinit(void) {
    if (s_channel != nullptr) {
        rmt_disable(s_channel);
    }
    if (s_encoder != nullptr) {
        rmt_del_encoder(s_encoder);
        s_encoder = nullptr;
    }
    if (s_channel != nullptr) {
        rmt_del_channel(s_channel);
        s_channel = nullptr;
    }
    if (s_gpio >= 0) {
        pinMode(s_gpio, OUTPUT);
        digitalWrite(s_gpio, LED_OFF);
    }
    s_busy = false;
    s_stop = false;
}

bool esl_ir_transmit(const uint8_t *data, size_t len, uint16_t repeats,
                     uint8_t delay) {
    if (s_channel == nullptr || s_encoder == nullptr || data == nullptr) {
        return false;
    }
    if (len == 0u || len > ESL_IR_MAX_FRAME_LEN) return false;

    const size_t n = esl_pp4_encode(data, len, ESL_RMT_RESOLUTION_HZ, s_symbols,
                                    ESL_IR_MAX_SYMBOLS);
    if (n == 0u) return false;

    for (size_t i = 0; i < n; i++) {
        s_words[i].level0 = 1;
        s_words[i].duration0 = s_symbols[i].burst_ticks;
        s_words[i].level1 = 0;
        s_words[i].duration1 = s_symbols[i].gap_ticks;
    }

    rmt_transmit_config_t tx_cfg = {};
    tx_cfg.loop_count = 0;
    /* Park the line low after each frame so the IR LED never idles on. */
    tx_cfg.flags.eot_level = 0;

    const uint32_t reps = (uint32_t)(repeats & 0x7FFFu);
    s_stop = false;
    s_busy = true;
    bool ok = true;

    for (uint32_t r = 0; r <= reps; r++) {
        if (s_stop) {
            ok = false;
            break;
        }
        /* The copy encoder takes a byte count, not a symbol count. */
        if (rmt_transmit(s_channel, s_encoder, s_words,
                         n * sizeof(rmt_symbol_word_t), &tx_cfg) != ESP_OK) {
            ok = false;
            break;
        }
        /* Blocks on a semaphore, so this yields to FreeRTOS and cannot starve
         * the watchdog even across hundreds of repeats. */
        if (rmt_tx_wait_all_done(s_channel, ESL_RMT_DONE_TIMEOUT_MS) != ESP_OK) {
            ok = false;
            break;
        }
        if (r < reps && delay > 0u) {
            esp_rom_delay_us((uint32_t)delay * 500u);
        }
    }

    s_busy = false;
    if (s_gpio >= 0) digitalWrite(s_gpio, LED_OFF);
    return ok;
}

void esl_ir_stop(void) { s_stop = true; }

bool esl_ir_is_busy(void) { return s_busy; }
