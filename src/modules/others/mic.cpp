#include "mic.h"
#if defined(MIC_SPM1423) || defined(MIC_INMP441)
#include "core/mykeyboard.h"
#include "core/powerSave.h"
#include "core/settings.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include <cmath>
#include <esp_heap_caps.h>

#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"
static i2s_chan_handle_t i2s_chan = nullptr;
#define I2S_NO_PIN I2S_GPIO_UNUSED
#ifndef I2S_PIN_NO_CHANGE
#define I2S_PIN_NO_CHANGE I2S_GPIO_UNUSED
#endif

#define FFT_SIZE 1024
#define SPECTRUM_WIDTH 200
#define SPECTRUM_HEIGHT 124
#define HISTORY_LEN (SPECTRUM_WIDTH + 1)

static int16_t *i2s_buffer = nullptr;
static uint8_t *fftHistory = nullptr; // Linear buffer [WIDTH + 1][HEIGHT]
static uint16_t posData = 0;
#define MIC_SAMPLE_RATE 48000

#ifndef PIN_CLK
#define PIN_CLK I2S_PIN_NO_CHANGE
#endif
#ifndef PIN_DATA
#define PIN_DATA I2S_PIN_NO_CHANGE
#endif

#ifdef PIN_BCLK
gpio_num_t mic_bclk_pin = (gpio_num_t)PIN_BCLK;
#else
gpio_num_t mic_bclk_pin = (gpio_num_t)I2S_PIN_NO_CHANGE;
#endif

static MicConfig mic_config = {
    .record_time_ms = 10000, // default 10 sec
    .gain = 2.0f,            // default gain (1.0f = no gain apply)
    .stealth_mode = false    // default stealth off
};

static void apply_gain_to_buffer(int16_t *buffer, size_t samples, float gain) {
    if (gain == 1.0f) return;

    // Exponential gain: 1.5x becomes ~2.25x effective
    float effectiveGain = pow(gain, 1.5f);

    for (size_t i = 0; i < samples; i++) {
        int32_t sample = (int32_t)(buffer[i] * effectiveGain);

        if (sample > 32767) sample = 32767;
        if (sample < -32768) sample = -32768;

        buffer[i] = (int16_t)sample;
    }
}
void _setup_codec_mic(bool enable) __attribute__((weak));
void _setup_codec_mic(bool enable) {}

const unsigned char ImageData[768] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x07, 0x00, 0x01, 0x09, 0x00, 0x01, 0x0D, 0x00, 0x02, 0x10, 0x00, 0x02, 0x14, 0x00, 0x01, 0x17, 0x00,
    0x02, 0x1C, 0x00, 0x02, 0x20, 0x00, 0x02, 0x24, 0x00, 0x03, 0x28, 0x00, 0x03, 0x2D, 0x00, 0x03, 0x32,
    0x00, 0x04, 0x37, 0x00, 0x05, 0x3C, 0x00, 0x04, 0x42, 0x00, 0x05, 0x46, 0x00, 0x05, 0x4D, 0x00, 0x06,
    0x51, 0x00, 0x06, 0x57, 0x00, 0x06, 0x5D, 0x00, 0x07, 0x62, 0x00, 0x07, 0x68, 0x00, 0x07, 0x6E, 0x00,
    0x09, 0x74, 0x00, 0x08, 0x7A, 0x00, 0x09, 0x7F, 0x00, 0x09, 0x86, 0x00, 0x0A, 0x8B, 0x00, 0x0A, 0x91,
    0x00, 0x0B, 0x97, 0x00, 0x0B, 0x9D, 0x00, 0x0C, 0xA2, 0x00, 0x0C, 0xA8, 0x00, 0x0C, 0xAD, 0x00, 0x0D,
    0xB2, 0x00, 0x0D, 0xB7, 0x00, 0x0E, 0xBC, 0x00, 0x0E, 0xC2, 0x00, 0x0E, 0xC7, 0x00, 0x0E, 0xCB, 0x00,
    0x0F, 0xD0, 0x00, 0x0F, 0xD5, 0x00, 0x10, 0xD9, 0x00, 0x0F, 0xDD, 0x00, 0x10, 0xE2, 0x00, 0x11, 0xE5,
    0x00, 0x11, 0xE8, 0x00, 0x11, 0xEC, 0x00, 0x11, 0xEF, 0x00, 0x11, 0xF1, 0x00, 0x11, 0xF5, 0x00, 0x11,
    0xF6, 0x00, 0x12, 0xF9, 0x00, 0x11, 0xFA, 0x00, 0x11, 0xFC, 0x00, 0x12, 0xFD, 0x00, 0x12, 0xFE, 0x00,
    0x12, 0xFF, 0x00, 0x12, 0xFF, 0x01, 0x12, 0xFF, 0x04, 0x12, 0xFE, 0x06, 0x12, 0xFE, 0x09, 0x11, 0xFD,
    0x0B, 0x11, 0xFB, 0x0E, 0x11, 0xFB, 0x11, 0x11, 0xF8, 0x14, 0x10, 0xF7, 0x17, 0x0F, 0xF5, 0x1B, 0x0F,
    0xF2, 0x1E, 0x0E, 0xEF, 0x22, 0x0E, 0xED, 0x26, 0x0D, 0xE9, 0x29, 0x0C, 0xE7, 0x2D, 0x0B, 0xE3, 0x32,
    0x0A, 0xE0, 0x36, 0x09, 0xDC, 0x3A, 0x08, 0xD7, 0x3F, 0x07, 0xD4, 0x44, 0x07, 0xCF, 0x48, 0x06, 0xCB,
    0x4C, 0x04, 0xC6, 0x51, 0x04, 0xC2, 0x55, 0x02, 0xBD, 0x5A, 0x02, 0xB8, 0x5F, 0x01, 0xB4, 0x63, 0x00,
    0xAF, 0x68, 0x00, 0xAA, 0x6D, 0x00, 0xA5, 0x73, 0x00, 0xA0, 0x78, 0x00, 0x9A, 0x7C, 0x00, 0x95, 0x81,
    0x00, 0x90, 0x86, 0x00, 0x8A, 0x8B, 0x00, 0x85, 0x90, 0x00, 0x7E, 0x96, 0x00, 0x78, 0x9B, 0x00, 0x73,
    0xA0, 0x00, 0x6E, 0xA5, 0x00, 0x68, 0xA9, 0x00, 0x63, 0xAF, 0x00, 0x5D, 0xB3, 0x00, 0x57, 0xB8, 0x00,
    0x53, 0xBC, 0x00, 0x4D, 0xC1, 0x00, 0x48, 0xC5, 0x00, 0x43, 0xCA, 0x00, 0x3D, 0xCE, 0x00, 0x38, 0xD3,
    0x00, 0x33, 0xD6, 0x00, 0x2F, 0xDA, 0x00, 0x2B, 0xDE, 0x00, 0x26, 0xE2, 0x00, 0x22, 0xE6, 0x00, 0x1D,
    0xE8, 0x00, 0x1A, 0xEC, 0x00, 0x16, 0xEF, 0x00, 0x12, 0xF2, 0x00, 0x0E, 0xF5, 0x00, 0x0B, 0xF7, 0x00,
    0x09, 0xF9, 0x00, 0x06, 0xFC, 0x00, 0x04, 0xFE, 0x00, 0x01, 0xFF, 0x01, 0x00, 0xFF, 0x03, 0x00, 0xFF,
    0x05, 0x00, 0xFF, 0x07, 0x00, 0xFF, 0x0A, 0x00, 0xFF, 0x0D, 0x00, 0xFF, 0x10, 0x00, 0xFF, 0x13, 0x00,
    0xFF, 0x16, 0x00, 0xFF, 0x19, 0x00, 0xFF, 0x1C, 0x00, 0xFF, 0x21, 0x00, 0xFF, 0x24, 0x00, 0xFF, 0x28,
    0x00, 0xFF, 0x2C, 0x00, 0xFF, 0x31, 0x00, 0xFF, 0x35, 0x00, 0xFF, 0x38, 0x00, 0xFF, 0x3D, 0x00, 0xFF,
    0x41, 0x00, 0xFF, 0x46, 0x00, 0xFF, 0x4B, 0x00, 0xFF, 0x50, 0x00, 0xFF, 0x54, 0x00, 0xFF, 0x59, 0x00,
    0xFF, 0x5D, 0x00, 0xFF, 0x63, 0x00, 0xFF, 0x67, 0x00, 0xFF, 0x6C, 0x00, 0xFF, 0x71, 0x00, 0xFF, 0x76,
    0x00, 0xFF, 0x7B, 0x00, 0xFF, 0x81, 0x00, 0xFF, 0x85, 0x00, 0xFD, 0x8A, 0x00, 0xFC, 0x8F, 0x00, 0xFB,
    0x95, 0x00, 0xFA, 0x9A, 0x00, 0xF8, 0x9E, 0x00, 0xF8, 0xA3, 0x00, 0xF6, 0xA7, 0x00, 0xF5, 0xAD, 0x00,
    0xF4, 0xB1, 0x00, 0xF3, 0xB6, 0x00, 0xF1, 0xBA, 0x00, 0xF0, 0xBF, 0x00, 0xF0, 0xC4, 0x00, 0xEE, 0xC8,
    0x00, 0xED, 0xCD, 0x00, 0xEC, 0xD0, 0x00, 0xEB, 0xD4, 0x00, 0xEB, 0xD8, 0x00, 0xE9, 0xDD, 0x00, 0xE8,
    0xE0, 0x00, 0xE8, 0xE4, 0x00, 0xE7, 0xE7, 0x00, 0xE7, 0xEB, 0x00, 0xE6, 0xED, 0x00, 0xE6, 0xF0, 0x00,
    0xE5, 0xF4, 0x00, 0xE4, 0xF7, 0x00, 0xE4, 0xF9, 0x00, 0xE4, 0xFB, 0x00, 0xE4, 0xFE, 0x00, 0xE4, 0xFF,
    0x01, 0xE4, 0xFF, 0x02, 0xE5, 0xFF, 0x05, 0xE4, 0xFF, 0x07, 0xE5, 0xFF, 0x0B, 0xE4, 0xFF, 0x0D, 0xE4,
    0xFF, 0x10, 0xE5, 0xFF, 0x13, 0xE5, 0xFF, 0x16, 0xE6, 0xFF, 0x1A, 0xE5, 0xFF, 0x1D, 0xE5, 0xFF, 0x21,
    0xE6, 0xFF, 0x24, 0xE6, 0xFF, 0x29, 0xE7, 0xFF, 0x2C, 0xE7, 0xFF, 0x30, 0xE8, 0xFF, 0x34, 0xE8, 0xFF,
    0x39, 0xE9, 0xFF, 0x3D, 0xE9, 0xFF, 0x41, 0xE9, 0xFF, 0x46, 0xEA, 0xFF, 0x4A, 0xEB, 0xFF, 0x50, 0xEB,
    0xFF, 0x54, 0xEC, 0xFF, 0x59, 0xEC, 0xFF, 0x5E, 0xED, 0xFF, 0x62, 0xED, 0xFF, 0x67, 0xEE, 0xFF, 0x6C,
    0xEF, 0xFF, 0x71, 0xEF, 0xFF, 0x76, 0xF0, 0xFF, 0x7B, 0xF0, 0xFF, 0x80, 0xF0, 0xFF, 0x85, 0xF1, 0xFF,
    0x8A, 0xF2, 0xFF, 0x8F, 0xF2, 0xFF, 0x94, 0xF3, 0xFF, 0x99, 0xF3, 0xFF, 0x9D, 0xF4, 0xFF, 0xA3, 0xF5,
    0xFF, 0xA7, 0xF5, 0xFF, 0xAC, 0xF6, 0xFF, 0xB1, 0xF6, 0xFF, 0xB5, 0xF6, 0xFF, 0xBA, 0xF7, 0xFF, 0xBE,
    0xF8, 0xFF, 0xC3, 0xF8, 0xFF, 0xC7, 0xF9, 0xFF, 0xCB, 0xF9, 0xFF, 0xD0, 0xFA, 0xFF, 0xD4, 0xFB, 0xFF,
    0xD8, 0xFB, 0xFF, 0xDC, 0xFB, 0xFF, 0xDF, 0xFC, 0xFF, 0xE2, 0xFC, 0xFF, 0xE6, 0xFC, 0xFF, 0xEA, 0xFD,
    0xFF, 0xEC, 0xFD, 0xFF, 0xF0, 0xFD, 0xFF, 0xF3, 0xFE, 0xFF, 0xF6, 0xFE, 0xFF, 0xF8, 0xFF, 0xFF, 0xFB,
    0xFF, 0xFF, 0xFD,
};

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (r << (5 + 6)) | (g << 5) | b;
    // return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

bool deinitMicroPhone() {
    // Disable codec, if exists
    _setup_codec_mic(false);
    esp_err_t err = ESP_OK;
    if (i2s_chan) {
        i2s_channel_disable(i2s_chan);
        err |= i2s_del_channel(i2s_chan);
        i2s_chan = nullptr;
    }
    gpio_reset_pin(GPIO_NUM_0);
    return err;
}

bool InitI2SMicroPhone() {
    // Enable codec, if exists
    _setup_codec_mic(true);
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = SPECTRUM_HEIGHT;
    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &i2s_chan);
#if defined(MIC_INMP441) // #ifdef PIN_WS // INMP441
    i2s_std_slot_config_t slot_cfg =
        I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
    slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_16BIT;
    const i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MIC_SAMPLE_RATE),
        .slot_cfg = slot_cfg,
        .gpio_cfg = {
                     .mclk = I2S_GPIO_UNUSED,
                     .bclk = (gpio_num_t)PIN_CLK,
                     .ws = (gpio_num_t)PIN_WS,
                     .dout = I2S_GPIO_UNUSED,
                     .din = (gpio_num_t)PIN_DATA,
                     .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
                     },
    };
    if (err == ESP_OK) err = i2s_channel_init_std_mode(i2s_chan, &std_cfg);
#else

    if (mic_bclk_pin != I2S_PIN_NO_CHANGE) {
        gpio_num_t mic_ws_pin = (gpio_num_t)PIN_CLK;
        i2s_std_config_t i2s_config;
        memset(&i2s_config, 0, sizeof(i2s_std_config_t));
#if defined(CONFIG_IDF_TARGET_ESP32P4)
        i2s_config.clk_cfg.clk_src = i2s_clock_src_t::I2S_CLK_SRC_DEFAULT;
#else
        i2s_config.clk_cfg.clk_src = i2s_clock_src_t::I2S_CLK_SRC_PLL_160M;
#endif
        i2s_config.clk_cfg.sample_rate_hz = MIC_SAMPLE_RATE;                           // dummy setting
        i2s_config.clk_cfg.mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256; // dummy setting
        i2s_config.slot_cfg.data_bit_width = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
        i2s_config.slot_cfg.slot_bit_width = i2s_slot_bit_width_t::I2S_SLOT_BIT_WIDTH_16BIT;
        i2s_config.slot_cfg.slot_mode = i2s_slot_mode_t::I2S_SLOT_MODE_MONO;
        i2s_config.slot_cfg.slot_mask = i2s_std_slot_mask_t::I2S_STD_SLOT_LEFT;
        i2s_config.slot_cfg.ws_width = 16;
        i2s_config.slot_cfg.bit_shift = true;
#if SOC_I2S_HW_VERSION_1 // For esp32/esp32-s2
        i2s_config.slot_cfg.msb_right = false;
#else
        i2s_config.slot_cfg.left_align = true;
        i2s_config.slot_cfg.big_endian = false;
        i2s_config.slot_cfg.bit_order_lsb = false;
#endif
        i2s_config.gpio_cfg.bclk = (gpio_num_t)mic_bclk_pin;
        i2s_config.gpio_cfg.ws = (gpio_num_t)mic_ws_pin;
        i2s_config.gpio_cfg.dout = (gpio_num_t)I2S_PIN_NO_CHANGE;
        i2s_config.gpio_cfg.mclk = (gpio_num_t)I2S_PIN_NO_CHANGE;
        i2s_config.gpio_cfg.din = (gpio_num_t)PIN_DATA;
        err = i2s_channel_init_std_mode(i2s_chan, &i2s_config);
    } else {

        i2s_pdm_rx_clk_config_t clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(MIC_SAMPLE_RATE);
        i2s_pdm_rx_slot_config_t slot_cfg =
            I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
        slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_16BIT;
        const i2s_pdm_rx_config_t pdm_cfg = {
            .clk_cfg = clk_cfg,
            .slot_cfg = slot_cfg,
            .gpio_cfg = {
                         .clk = (gpio_num_t)PIN_CLK,
                         .din = (gpio_num_t)PIN_DATA,
                         .invert_flags = {.clk_inv = false},
                         },
        };
        if (err == ESP_OK) err = i2s_channel_init_pdm_rx_mode(i2s_chan, &pdm_cfg);
    }
#endif
    if (err == ESP_OK) err = i2s_channel_enable(i2s_chan);
    return (err == ESP_OK);
}

void mic_test_one_task() {
    tft.fillScreen(TFT_BLACK);

    // ===== CALCULATE DYNAMIC DIMENSIONS =====
    const int MARGIN_X = (tftWidth > 200) ? 10 : 5;  // Change for margin
    const int MARGIN_Y = (tftHeight > 200) ? 10 : 5; // Change for margin
    const int displayWidth = tftWidth - (2 * MARGIN_X);
    const int displayHeight = tftHeight - (2 * MARGIN_Y);
    const int displayX = MARGIN_X;
    const int displayY = MARGIN_Y;

    // Alloc framebuffer
    uint16_t *frameBuffer;
    if (psramFound()) frameBuffer = (uint16_t *)ps_malloc(displayWidth * displayHeight * sizeof(uint16_t));
    else {
        closeSdCard();
        frameBuffer = (uint16_t *)malloc(displayWidth * displayHeight * sizeof(uint16_t));
    }

    if (!frameBuffer) {
        Serial.println("Error alloc drawing frameBuffer, exiting");
        displayError("Not Enough RAM", true);
        return;
    }

    // Border around the spectrogram
    tft.drawRect(displayX - 2, displayY - 2, displayWidth + 4, displayHeight + 4, bruceConfig.priColor);

    while (1) {
        fft_config_t *plan = fft_init(FFT_SIZE, FFT_REAL, FFT_FORWARD, NULL, NULL);
        size_t bytesread;
        i2s_channel_read(i2s_chan, (char *)i2s_buffer, FFT_SIZE * sizeof(int16_t), &bytesread, portMAX_DELAY);

        int16_t *samples = (int16_t *)i2s_buffer;

        for (int i = 0; i < FFT_SIZE; i++) { plan->input[i] = (float)samples[i] / 32768.0f; }

        fft_execute(plan);

        for (int i = 1; i < FFT_SIZE / 4 && i < SPECTRUM_HEIGHT; i++) {
            float re = plan->output[2 * i];
            float im = plan->output[2 * i + 1];
            float mag = re * re + im * im;
            if (mag > 1.0f) mag = 1.0f;
            uint8_t value = map(mag * 2000, 0, 2000, 0, 255);
            fftHistory[posData * SPECTRUM_HEIGHT + (SPECTRUM_HEIGHT - i)] = value;
        }

        posData = (posData + 1) % HISTORY_LEN;

        fft_destroy(plan);

        // ===== RENDER WITH SCALING =====
        for (int y = 0; y < displayHeight; y++) {
            // Original spectrum y-display y-map
            int srcY = (y * SPECTRUM_HEIGHT) / displayHeight;

            for (int x = 0; x < displayWidth; x++) {
                // Original spectrum display x-map
                int srcX = (x * SPECTRUM_WIDTH) / displayWidth;
                int index = (srcX + posData) % HISTORY_LEN;

                uint8_t val = fftHistory[index * SPECTRUM_HEIGHT + srcY];
                uint16_t color = rgb565(
                    pgm_read_byte(&ImageData[val * 3 + 0]),
                    pgm_read_byte(&ImageData[val * 3 + 1]),
                    pgm_read_byte(&ImageData[val * 3 + 2])
                );
                frameBuffer[y * displayWidth + x] = color;
            }
        }

        tft.pushImage(displayX, displayY, displayWidth, displayHeight, frameBuffer);
        wakeUpScreen();
        if (check(SelPress) || check(EscPress)) break;
    }
    i2s_channel_disable(i2s_chan);

    free(frameBuffer);
}

bool isGPIOOutput(gpio_num_t gpio) {
    if (gpio < 0 || gpio > 39) return false;

    if (gpio <= 31) {
        uint32_t reg_val = REG_READ(GPIO_ENABLE_REG);
        return reg_val & (1UL << gpio);
    } else {
        uint32_t reg_val = REG_READ(GPIO_ENABLE1_REG);
        return reg_val & (1UL << (gpio - 32));
    }
}

void mic_test() {
    ioExpander.turnPinOnOff(IO_EXP_MIC, HIGH);
    // Devices that use GPIO 0 to navigation (or any other purposes) will break after start mic
    bool gpioInput = false;
    if (!isGPIOOutput(GPIO_NUM_0)) {
        gpioInput = true;
        gpio_hold_en(GPIO_NUM_0);
    }
    Serial.println("Mic Spectrum start");
    InitI2SMicroPhone();
    // Alloc buffers in PSRAM if available
    if (psramFound()) {
        i2s_buffer = (int16_t *)ps_malloc(FFT_SIZE * sizeof(int16_t));
        fftHistory = (uint8_t *)ps_malloc(HISTORY_LEN * SPECTRUM_HEIGHT);
    } else {
        i2s_buffer = (int16_t *)malloc(FFT_SIZE * sizeof(int16_t));
        fftHistory = (uint8_t *)malloc(HISTORY_LEN * SPECTRUM_HEIGHT);
    }
    if (!i2s_buffer || !fftHistory) {
        displayError("Fail to alloc buffers, exiting", true);
        return;
    }

    memset(fftHistory, 0, HISTORY_LEN * SPECTRUM_HEIGHT);

    mic_test_one_task();

    free(i2s_buffer);
    free(fftHistory);
    i2s_buffer = nullptr;
    fftHistory = nullptr;

    delay(10);
    if (deinitMicroPhone()) Serial.println("Fail disabling I2S Driver");
    if (gpioInput) {
        gpio_hold_dis(GPIO_NUM_0);
        pinMode(GPIO_NUM_0, INPUT);
    } else {
        pinMode(GPIO_NUM_0, OUTPUT);
        digitalWrite(GPIO_NUM_0, LOW);
    }
    Serial.println("Spectrum finished");
    ioExpander.turnPinOnOff(IO_EXP_MIC, LOW);
}

// https://github.com/MhageGH/esp32_SoundRecorder/tree/master

void CreateWavHeader(byte *header, int waveDataSize) {
    if (waveDataSize < 0 || waveDataSize > 0x7FFFFFFF - 44) {
        Serial.println("Invalid WAV size");
        waveDataSize = 0; // Fallback
    }
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    unsigned int fileSizeMinus8 = waveDataSize + 44 - 8;
    header[4] = (byte)(fileSizeMinus8 & 0xFF);
    header[5] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
    header[6] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
    header[7] = (byte)((fileSizeMinus8 >> 24) & 0xFF);
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    header[16] = 0x10; // linear PCM
    header[17] = 0x00;
    header[18] = 0x00;
    header[19] = 0x00;
    header[20] = 0x01; // linear PCM
    header[21] = 0x00;
    header[22] = 0x01; // monoral
    header[23] = 0x00;
    header[24] = 0x80; // sampling rate 48000
    header[25] = 0xBB;
    header[26] = 0x00;
    header[27] = 0x00;
    header[28] = 0x00; // Byte/sec = 48000x2x1 = 96000
    header[29] = 0x77;
    header[30] = 0x01;
    header[31] = 0x00;
    header[32] = 0x02; // 16bit monoral
    header[33] = 0x00;
    header[34] = 0x10; // 16bit
    header[35] = 0x00;
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    header[40] = (byte)(waveDataSize & 0xFF);
    header[41] = (byte)((waveDataSize >> 8) & 0xFF);
    header[42] = (byte)((waveDataSize >> 16) & 0xFF);
    header[43] = (byte)((waveDataSize >> 24) & 0xFF);
}

bool mic_record_wav_to_path(
    FS *fs, const String &path, uint32_t max_ms, uint32_t *out_bytes, float gain,
    std::function<bool(void)> onProgress
) {
    if (out_bytes) *out_bytes = 0;
    if (fs == nullptr) return false;
    if (path.length() == 0) return false;

    ioExpander.turnPinOnOff(IO_EXP_MIC, HIGH);

    // GPIO PROTECTION: Centrally managed here (safe for JS and GUI)
    bool gpioInput = false;
    if (!isGPIOOutput(GPIO_NUM_0)) {
        gpioInput = true;
        gpio_hold_en(GPIO_NUM_0);
    }

    bool ok = false;
    do {
        if (!InitI2SMicroPhone()) break;

        // Buffer Allocation
        if (psramFound()) i2s_buffer = (int16_t *)ps_malloc(FFT_SIZE * sizeof(int16_t));
        else i2s_buffer = (int16_t *)malloc(FFT_SIZE * sizeof(int16_t));
        if (!i2s_buffer) break;

        // Path Management
        String fixedPath = path;
        if (!fixedPath.startsWith("/")) fixedPath = "/" + fixedPath;

        // Creating folders if necessary
        int lastSlash = fixedPath.lastIndexOf('/');
        if (lastSlash > 0) {
            String dir = fixedPath.substring(0, lastSlash);
            if (!fs->exists(dir)) fs->mkdir(dir);
        }

        File audioFile = fs->open(fixedPath, FILE_WRITE, true);
        if (!audioFile) break;

        const int headerSize = 44;
        byte header[headerSize] = {0};
        audioFile.write(header, headerSize);

        uint32_t dataSize = 0;
        const int bytesPerRead = FFT_SIZE * sizeof(int16_t);
        const unsigned long startMillis = millis();

        while (true) {
            // 1. Timeout
            if (max_ms > 0 && (millis() - startMillis) >= max_ms) break;

            // 2. EXTERNAL CALLBACK (GUI update or custom check stop)
            // If the callback exists and returns FALSE, stop recording.
            if (onProgress && !onProgress()) break;

            size_t bytesRead = 0;
            esp_err_t err = i2s_channel_read(i2s_chan, (char *)i2s_buffer, bytesPerRead, &bytesRead, 1000);
            if (err != ESP_OK) {
                Serial.printf("I2S read error: %s\n", esp_err_to_name(err));
                break; // Exit Loop
            }
            if (bytesRead > 0) {
                apply_gain_to_buffer(i2s_buffer, bytesRead / sizeof(int16_t), gain);
                audioFile.write((const uint8_t *)i2s_buffer, bytesRead);
                dataSize += (uint32_t)bytesRead;
            }

            // Watchdog reset and yield to other tasks
            delay(1);
            yield();
        }

        audioFile.seek(0);
        CreateWavHeader(header, dataSize);
        audioFile.write(header, headerSize);
        audioFile.close();

        if (out_bytes) *out_bytes = dataSize + headerSize;
        ok = true;
    } while (0);

    // Cleanup
    if (i2s_buffer) {
        free(i2s_buffer);
        i2s_buffer = nullptr;
    }

    delay(10);
    deinitMicroPhone();

    // Restore GPIO
    if (gpioInput) {
        gpio_hold_dis(GPIO_NUM_0);
        pinMode(GPIO_NUM_0, INPUT);
    } else {
        pinMode(GPIO_NUM_0, OUTPUT);
        digitalWrite(GPIO_NUM_0, LOW);
    }

    ioExpander.turnPinOnOff(IO_EXP_MIC, LOW);
    return ok;
}

void mic_record_app() {

    // ===== LAYOUT CONSTANTS =====
    const int MARGIN = (tftWidth > 200) ? 10 : 5;
    const int HEADER_HEIGHT = (tftHeight > 200) ? 35 : 25;
    const int ITEM_HEIGHT = (tftHeight > 200) ? 30 : 22;
    const int BUTTON_HEIGHT = (tftHeight > 200) ? 40 : 30;
    const int TEXT_SIZE_LARGE = (tftWidth > 200) ? 2 : 1;
    const int TEXT_SIZE_SMALL = 1;
    const int START_Y = HEADER_HEIGHT + ((tftHeight > 200) ? 15 : 8);

    // UI Parameters
    int selected_item = 0;
    const int ITEM_TIME = 0;
    const int ITEM_GAIN = 1;
    const int ITEM_STEALTH = 2;
    const int ITEM_START = 3;
    const int NUM_ITEMS = 4;

    uint8_t originalBrightness = currentScreenBrightness; // ← Backup Brightness value

    bool editing = false;
    int prev_selected = 0;
    unsigned long last_input = millis();

    // Valori configurabili
    int time_seconds = mic_config.record_time_ms / 1000;
    float gain_value = mic_config.gain;
    bool stealth_enabled = mic_config.stealth_mode;

    // ===== HELPER FOR UI DRAWING =====

    auto drawItem = [&](int itemIndex, bool isSelected, bool isEdit) {
        int yPos = 0;

        // Calculate Y based on the item
        if (itemIndex < ITEM_START) {
            yPos = START_Y + itemIndex * (ITEM_HEIGHT + 8);
        } else {
            yPos = START_Y + 2 * (ITEM_HEIGHT + 8) + ITEM_HEIGHT + 15;
        }

        // 1. CLEAN: Delete only the area of ​​this specific item
        // Draw a rectangle of the background color over the old row
        int clearHeight = (itemIndex == ITEM_START) ? BUTTON_HEIGHT : ITEM_HEIGHT;
        // Let's enlarge the cleanup area a little to cover the old edges (+4 px)
        tft.fillRect(MARGIN - 5, yPos - 5, tftWidth - 2 * MARGIN + 10, clearHeight + 10, bruceConfig.bgColor);

        tft.setTextSize(TEXT_SIZE_LARGE);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

        // 2. DRAW BORDER if selected (except for START button)
        if (isSelected && itemIndex != ITEM_START) {
            uint16_t borderColor = isEdit ? TFT_GREEN : bruceConfig.priColor;
            int borderWidth = isEdit ? 2 : 1;
            for (int i = 0; i < borderWidth; i++) {
                tft.drawRoundRect(
                    MARGIN - 3 + i,
                    yPos - 3 + i,
                    tftWidth - 2 * MARGIN + 6 - 2 * i,
                    ITEM_HEIGHT + 6 - 2 * i,
                    5,
                    borderColor
                );
            }
        }

        // 3. DRAW CONTENT
        int contentY = yPos + (ITEM_HEIGHT - TEXT_SIZE_LARGE * 8) / 2;
        int rightMargin = 15;

        switch (itemIndex) {
            case ITEM_TIME: {
                tft.setCursor(MARGIN + 2, contentY);
                tft.print("Time:");

                int unitX = tftWidth - MARGIN - rightMargin;

                if (time_seconds == 0) {
                    const char *infText = (tftWidth > 200) ? "Unlim" : "INF";
                    int textWidth = strlen(infText) * 6 * TEXT_SIZE_LARGE;
                    tft.setCursor(unitX + 6 * TEXT_SIZE_LARGE - textWidth, contentY);
                    if (isEdit && isSelected) tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                    tft.print(infText);
                } else {
                    char timeStr[8];
                    snprintf(timeStr, sizeof(timeStr), "%d", time_seconds);
                    int numWidth = strlen(timeStr) * 6 * TEXT_SIZE_LARGE;
                    tft.setCursor(unitX - numWidth, contentY);
                    tft.print(timeStr);
                    tft.print("s");
                }
                break;
            }
            case ITEM_GAIN: {
                tft.setCursor(MARGIN + 2, contentY);
                tft.print("Gain:");
                tft.setCursor(tftWidth - MARGIN - 50, contentY);
                tft.print(gain_value, 1);
                tft.print("x");
                break;
            }
            case ITEM_STEALTH: {
                tft.setCursor(MARGIN + 2, contentY);
                tft.print("Stealth:");
                tft.setCursor(tftWidth - MARGIN - 35, contentY);
                tft.print(stealth_enabled ? "ON" : "OFF");
                break;
            }
            case ITEM_START: {
                uint16_t btnColor = isSelected ? TFT_RED : TFT_DARKGREY;
                tft.fillRoundRect(MARGIN, yPos, tftWidth - 2 * MARGIN, BUTTON_HEIGHT, 8, btnColor);
                tft.setTextColor(TFT_WHITE, btnColor);
                const char *btnText = (tftWidth > 200) ? "START REC" : "START";
                int textWidth = strlen(btnText) * 6 * TEXT_SIZE_LARGE;
                tft.setCursor((tftWidth - textWidth) / 2, yPos + (BUTTON_HEIGHT - TEXT_SIZE_LARGE * 8) / 2);
                tft.print(btnText);
                break;
            }
        }
    };

    // ===== INITIAL DESIGN (Only once) =====
    tft.fillScreen(bruceConfig.bgColor);

    // Header
    tft.fillRect(0, 0, tftWidth, HEADER_HEIGHT, bruceConfig.priColor);
    tft.setTextColor(bruceConfig.bgColor, bruceConfig.priColor);
    tft.setTextSize(TEXT_SIZE_LARGE);
    tft.setCursor(MARGIN, (HEADER_HEIGHT - (TEXT_SIZE_LARGE * 8)) / 2);
    tft.println("MIC RECORDER");

    // Footer Instructions (Static)
    if (tftHeight > 200) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setCursor(MARGIN, tftHeight - 15);
        tft.print("NAV: ^v | EDIT: Sel | REC: Start");
    }

    // Draw all the elements for the first time
    for (int i = 0; i < NUM_ITEMS; i++) { drawItem(i, (i == selected_item), editing); }

    // ===== MAIN LOOP =====
    while (true) {
        InputHandler();
        wakeUpScreen();

        bool selection_changed = false;
        bool value_changed = false;
        bool edit_mode_changed = false;

        if (!editing) {
            // Navigation mode
            if (check(PrevPress)) {
                prev_selected = selected_item;
                selected_item = (selected_item - 1 + NUM_ITEMS) % NUM_ITEMS;
                selection_changed = true;
                last_input = millis();
            }
            if (check(NextPress)) {
                prev_selected = selected_item;
                selected_item = (selected_item + 1) % NUM_ITEMS;
                selection_changed = true;
                last_input = millis();
            }
            if (check(SelPress)) {
                if (selected_item == ITEM_START) break; // Start
                editing = true;
                edit_mode_changed = true;
                last_input = millis();
            }
            if (check(EscPress)) { goto cleanup_and_exit; }
        } else {
            // Editing mode
            switch (selected_item) {
                case ITEM_TIME:
                    if (check(PrevPress)) {
                        // Logic for time selection

                        if (time_seconds > 0)
                            time_seconds = (time_seconds <= 10) ? time_seconds - 1 : time_seconds - 5;
                        value_changed = true;
                    }
                    if (check(NextPress)) {
                        time_seconds = (time_seconds < 300) ? time_seconds + 5 : 0;
                        value_changed = true;
                    }
                    break;
                case ITEM_GAIN:
                    if (check(PrevPress)) {
                        if (gain_value > 0.5f) gain_value -= 0.1f;
                        value_changed = true;
                    }
                    if (check(NextPress)) {
                        if (gain_value < 4.0f) gain_value += 0.1f;
                        value_changed = true;
                    }
                    break;
                case ITEM_STEALTH:
                    if (check(PrevPress) || check(NextPress)) {
                        stealth_enabled = !stealth_enabled;
                        value_changed = true;
                    }
                    break;
            }

            if (check(SelPress) || check(EscPress)) {
                editing = false;
                edit_mode_changed = true;
                last_input = millis();
            }
        }

        // ===== OPTIMIZED REDraw LOGIC =====

        if (selection_changed) {
            drawItem(prev_selected, false, false);
            drawItem(selected_item, true, false);
        }

        if (edit_mode_changed) { drawItem(selected_item, true, editing); }

        if (value_changed) {
            drawItem(selected_item, true, editing);
            last_input = millis();
        }

        delay(20); // Small delay for stability
        if (millis() - last_input > 120000) { goto cleanup_and_exit; }
    }

    // ===== RECORDING BLOCK =====
    {
        mic_config.record_time_ms = time_seconds * 1000;
        mic_config.gain = gain_value;
        mic_config.stealth_mode = stealth_enabled;

        FS *fs = nullptr;
        if (!getFsStorage(fs) || fs == nullptr) {
            displayError("No storage", true);
            goto cleanup_and_exit;
        }

        if (!fs->exists("/BruceMIC")) {
            if (!fs->mkdir("/BruceMIC")) {
                displayError("Dir creation failed", true);
                goto cleanup_and_exit;
            }
        }

        char filename[64];
        int index = 0;
        do {
            snprintf(filename, sizeof(filename), "/BruceMIC/recording_%d.wav", index++);
        } while (fs->exists(filename));

        //===== UI CLEANING AND SETUP =====

        if (stealth_enabled) {
            setBrightness(10, false);
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_RED);
            tft.setTextSize(1);
            tft.setCursor(5, 5);
            tft.print(".");
        } else {
            // Clean the screen completely
            tft.fillScreen(bruceConfig.bgColor);

            // ===== HEADER RECORDING =====
            const int REC_HEADER_HEIGHT = (tftHeight > 200) ? 40 : 30;
            tft.fillRect(0, 0, tftWidth, REC_HEADER_HEIGHT, TFT_RED);
            tft.setTextSize((tftWidth > 200) ? 2 : 1);
            tft.setTextColor(TFT_WHITE, TFT_RED);

            const char *headerText = (tftWidth > 200) ? "● RECORDING" : "● REC";
            int headerWidth = strlen(headerText) * 6 * ((tftWidth > 200) ? 2 : 1);
            tft.setCursor((tftWidth - headerWidth) / 2, (REC_HEADER_HEIGHT - 16) / 2);
            tft.print(headerText);

            // ===== STATIC INFO (Gain, Filename, Instructions) =====
            const int INFO_START_Y = REC_HEADER_HEIGHT + 10;
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setTextSize(TEXT_SIZE_SMALL);

            // Gain
            tft.setCursor(MARGIN, INFO_START_Y);
            tft.print("Gain: ");
            tft.print(gain_value, 1);
            tft.print("x");

            // Filename (truncated if too long)
            tft.setCursor(MARGIN, INFO_START_Y + 12);
            const char *displayName = filename;
            int nameLen = strlen(filename);
            if (nameLen > (tftWidth / 6)) {
                displayName = filename + (nameLen - (tftWidth / 6) + 3);
                tft.print("...");
            }
            tft.print(displayName);

            // Stop instructions
            tft.setCursor(MARGIN, INFO_START_Y + 24);
            tft.print("Press SEL to stop");
        }

        uint32_t max_ms = mic_config.record_time_ms;
        uint32_t out_bytes = 0;
        unsigned long startRecTime = millis();

        // Variables for the display timer
        const int TIMER_Y = tftHeight / 2 + 20;
        const int TIMER_SIZE = (tftWidth > 200) ? 3 : 2;
        unsigned long lastUpdate = 0;
        String lastTimerStr = "";

        // ===== CALLBACK DURING RECORDING =====
        auto onRecordingLoop = [&]() -> bool {
            InputHandler();
            if (check(SelPress)) return false;

            if (!stealth_enabled) {
                if (millis() - lastUpdate > 500) {
                    lastUpdate = millis();

                    uint32_t elapsedMs = millis() - startRecTime;
                    int elapsedSec = elapsedMs / 1000;
                    int elapsedMin = elapsedSec / 60;
                    int elapsedSecRem = elapsedSec % 60;

                    char timerStr[32];

                    if (time_seconds == 0) {
                        snprintf(timerStr, sizeof(timerStr), "%02d:%02d", elapsedMin, elapsedSecRem);
                    } else {
                        int totalMin = time_seconds / 60;
                        int totalSec = time_seconds % 60;
                        snprintf(
                            timerStr,
                            sizeof(timerStr),
                            "%02d:%02d / %02d:%02d",
                            elapsedMin,
                            elapsedSecRem,
                            totalMin,
                            totalSec
                        );
                    }

                    if (String(timerStr) != lastTimerStr) {
                        lastTimerStr = String(timerStr);

                        tft.fillRect(0, TIMER_Y - 5, tftWidth, TIMER_SIZE * 8 + 10, bruceConfig.bgColor);

                        tft.setTextSize(TIMER_SIZE);
                        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
                        int timerWidth = strlen(timerStr) * 6 * TIMER_SIZE;
                        tft.setCursor((tftWidth - timerWidth) / 2, TIMER_Y);
                        tft.print(timerStr);

                        tft.setTextSize(TEXT_SIZE_SMALL);
                        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                        const char *label = (time_seconds == 0) ? "Elapsed" : "Elapsed / Total";
                        int labelWidth = strlen(label) * 6;
                        tft.setCursor((tftWidth - labelWidth) / 2, TIMER_Y + TIMER_SIZE * 8 + 5);
                        tft.print(label);
                    }
                }
            }
            return true;
        };

        bool success =
            mic_record_wav_to_path(fs, String(filename), max_ms, &out_bytes, gain_value, onRecordingLoop);

        if (success) {
            Serial.print("Recording saved: ");
            Serial.println(filename);
            Serial.print("Size: ");
            Serial.print(out_bytes);
            Serial.println(" bytes");

            if (!stealth_enabled) {
                tft.fillScreen(bruceConfig.bgColor);

                tft.fillRect(0, 0, tftWidth, HEADER_HEIGHT, TFT_DARKGREEN);
                tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
                tft.setTextSize(TEXT_SIZE_LARGE);
                const char *successText = "SAVED";
                int successWidth = strlen(successText) * 6 * TEXT_SIZE_LARGE;
                tft.setCursor((tftWidth - successWidth) / 2, (HEADER_HEIGHT - 16) / 2);
                tft.print(successText);

                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.setTextSize(TEXT_SIZE_SMALL);

                int infoY = HEADER_HEIGHT + 20;
                tft.setCursor(MARGIN, infoY);
                tft.print("File: ");
                String shortName = String(filename);
                if (shortName.length() > 25) {
                    shortName = "..." + shortName.substring(shortName.length() - 22);
                }
                tft.println(shortName);

                tft.setCursor(MARGIN, infoY + 15);
                tft.print("Size: ");
                if (out_bytes > 0) {
                    float sizeKB = out_bytes / 1024.0f;
                    tft.print(sizeKB, 1);
                    tft.print(" KB");
                } else {
                    tft.print("Unknown");
                }

                tft.setCursor(MARGIN, infoY + 30);
                tft.print("Duration: ");
                unsigned long totalMs = millis() - startRecTime;
                int finalSec = (int)(totalMs / 1000);
                char durStr[16];
                snprintf(durStr, sizeof(durStr), "%d", finalSec);
                tft.print(durStr);
                tft.print("s");

                delay(2500);
            }

        } else {
            displayError("Recording failed", true);
        }
    }

// ===== CLEANUP =====
cleanup_and_exit:
    if (stealth_enabled) { setBrightness(originalBrightness, false); }
}

/**
 * Capture raw audio samples from microphone
 * Supports configurable sample rate for better performance
 * Caller must free() the returned buffer
 */
bool mic_capture_samples(
    uint32_t numSamples, uint32_t sampleRate, float gain, int16_t **outSamples, uint32_t *outSampleRate
) {
    if (!outSamples || !outSampleRate) return false;

    *outSamples = nullptr;
    *outSampleRate = 0;

    // Validate parameters
    if (numSamples > 4096 || numSamples < 64) return false;
    if (gain < 0.5f || gain > 4.0f) return false;

    // Validate and set sample rate (supported values)
    // Lower sample rates = faster processing for pitch detection
    switch (sampleRate) {
        case 8000:
        case 16000:
        case 22050:
        case 32000:
        case 44100:
        case 48000: break;
        default: sampleRate = 16000; // Default to 16kHz (good for voice/guitar)
    }

    *outSampleRate = sampleRate;

    // GPIO PROTECTION (fixed: IO_EXP_MIC not IOEXP_MIC)
    ioExpander.turnPinOnOff(IO_EXP_MIC, HIGH);
    bool gpioInput = false;
    if (!isGPIOOutput(GPIO_NUM_0)) {
        gpioInput = true;
        gpio_hold_en(GPIO_NUM_0);
    }

    bool ok = false;
    i2s_chan_handle_t temp_i2s_chan = nullptr;

    do {
        // Initialize I2S with custom sample rate
        _setup_codec_mic(true);

        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        chan_cfg.dma_desc_num = 8;
        chan_cfg.dma_frame_num = 256;

        esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &temp_i2s_chan);
        if (err != ESP_OK) break;

        // Configure I2S with custom sample rate
#if defined(MIC_INMP441)
        i2s_std_slot_config_t slot_cfg =
            I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
        slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_16BIT;

        const i2s_std_config_t std_cfg = {
            .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sampleRate), // Custom sample rate
            .slot_cfg = slot_cfg,
            .gpio_cfg = {
                         .mclk = I2S_GPIO_UNUSED,
                         .bclk = (gpio_num_t)PIN_CLK,
                         .ws = (gpio_num_t)PIN_WS,
                         .din = (gpio_num_t)PIN_DATA,
                         .dout = I2S_GPIO_UNUSED,
                         .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
                         },
        };
        err = i2s_channel_init_std_mode(temp_i2s_chan, &std_cfg);
#else
        if (mic_bclk_pin != I2S_PIN_NO_CHANGE) {
            // Standard I2S mode
            gpio_num_t mic_ws_pin = (gpio_num_t)PIN_CLK;
            i2s_std_config_t i2s_config;
            memset(&i2s_config, 0, sizeof(i2s_std_config_t));
#if defined(CONFIG_IDF_TARGET_ESP32P4)
            i2s_config.clk_cfg.clk_src = i2s_clock_src_t::I2S_CLK_SRC_DEFAULT;
#else
            i2s_config.clk_cfg.clk_src = i2s_clock_src_t::I2S_CLK_SRC_PLL_160M;
#endif
            i2s_config.clk_cfg.sample_rate_hz = sampleRate; // Custom sample rate
            i2s_config.clk_cfg.mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
            i2s_config.slot_cfg.data_bit_width = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
            i2s_config.slot_cfg.slot_bit_width = i2s_slot_bit_width_t::I2S_SLOT_BIT_WIDTH_16BIT;
            i2s_config.slot_cfg.slot_mode = i2s_slot_mode_t::I2S_SLOT_MODE_MONO;
            i2s_config.slot_cfg.slot_mask = i2s_std_slot_mask_t::I2S_STD_SLOT_LEFT;
            i2s_config.slot_cfg.ws_width = 16;
            i2s_config.slot_cfg.bit_shift = true;
#if SOC_I2S_HW_VERSION_1
            i2s_config.slot_cfg.msb_right = false;
#else
            i2s_config.slot_cfg.left_align = true;
            i2s_config.slot_cfg.big_endian = false;
            i2s_config.slot_cfg.bit_order_lsb = false;
#endif
            i2s_config.gpio_cfg.bclk = (gpio_num_t)mic_bclk_pin;
            i2s_config.gpio_cfg.ws = (gpio_num_t)mic_ws_pin;
            i2s_config.gpio_cfg.dout = (gpio_num_t)I2S_PIN_NO_CHANGE;
            i2s_config.gpio_cfg.mclk = (gpio_num_t)I2S_PIN_NO_CHANGE;
            i2s_config.gpio_cfg.din = (gpio_num_t)PIN_DATA;
            err = i2s_channel_init_std_mode(temp_i2s_chan, &i2s_config);
        } else {
            // PDM mode
            i2s_pdm_rx_clk_config_t clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(sampleRate); // Custom sample rate
            i2s_pdm_rx_slot_config_t slot_cfg =
                I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
            slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_16BIT;

            const i2s_pdm_rx_config_t pdm_cfg = {
                .clk_cfg = clk_cfg,
                .slot_cfg = slot_cfg,
                .gpio_cfg = {
                             .clk = (gpio_num_t)PIN_CLK,
                             .din = (gpio_num_t)PIN_DATA,
                             .invert_flags = {.clk_inv = false},
                             },
            };
            err = i2s_channel_init_pdm_rx_mode(temp_i2s_chan, &pdm_cfg);
        }
#endif

        if (err != ESP_OK) break;

        err = i2s_channel_enable(temp_i2s_chan);
        if (err != ESP_OK) break;

        // Allocate buffer
        int16_t *buffer;
        if (psramFound()) {
            buffer = (int16_t *)ps_malloc(numSamples * sizeof(int16_t));
        } else {
            buffer = (int16_t *)malloc(numSamples * sizeof(int16_t));
        }
        if (!buffer) break;

        // Capture samples
        uint32_t microsPerSample = 1000000 / sampleRate;
        for (uint32_t i = 0; i < numSamples; i++) {
            size_t bytesRead = 0;
            err = i2s_channel_read(temp_i2s_chan, (char *)&buffer[i], sizeof(int16_t), &bytesRead, 1000);
            if (err != ESP_OK || bytesRead == 0) {
                free(buffer);
                goto cleanup;
            }

            // Timing for sample rate
            delayMicroseconds(microsPerSample);
            yield();
        }

        // Apply gain
        apply_gain_to_buffer(buffer, numSamples, gain);

        *outSamples = buffer;
        ok = true;

    } while (0);

cleanup:
    // Cleanup I2S
    if (temp_i2s_chan) {
        i2s_channel_disable(temp_i2s_chan);
        i2s_del_channel(temp_i2s_chan);
    }
    _setup_codec_mic(false);

    delay(10);

    // Restore GPIO
    if (gpioInput) {
        gpio_hold_dis(GPIO_NUM_0);
        pinMode(GPIO_NUM_0, INPUT);
    } else {
        pinMode(GPIO_NUM_0, OUTPUT);
        digitalWrite(GPIO_NUM_0, LOW);
    }

    ioExpander.turnPinOnOff(IO_EXP_MIC, LOW);

    return ok;
}

#else
void mic_test() {}
void mic_test_one_task() {}
void mic_record_app() {}
bool mic_record_wav_to_path(
    FS *fs, const String &path, uint32_t max_ms, uint32_t *out_bytes, float gain,
    std::function<bool(void)> onProgress
) {
    (void)fs;
    (void)path;
    (void)max_ms;
    (void)out_bytes;
    (void)gain;
    (void)onProgress;
    return false;
}
bool mic_capture_samples(
    uint32_t numSamples, uint32_t sampleRate, float gain, int16_t **outSamples, uint32_t *outSampleRate
) {
    (void)numSamples;
    (void)sampleRate;
    (void)gain;
    (void)outSamples;
    (void)outSampleRate;
    return false;
}
#endif
