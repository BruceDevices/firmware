#include "mic.h"
#if defined(MIC_SPM1423) || defined(MIC_INMP441)
#include "core/mykeyboard.h"
#include "core/powerSave.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
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

#ifndef PIN_CLK
#define PIN_CLK I2S_PIN_NO_CHANGE
#endif
#ifndef PIN_DATA
#define PIN_DATA I2S_PIN_NO_CHANGE
#endif

#ifdef PIN_BCLK
gpio_num_t mic_bclk_pin = (gpio_num_t)PIN_BCLK;
#else
gpio_num_t mic_bclk_pin = I2S_PIN_NO_CHANGE;
#endif

void _setup_codec_mic(bool enable) __attribute__((weak));
void _setup_codec_mic(bool enable) {}

const unsigned char ImageData[768] = {
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
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
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
        i2s_config.clk_cfg.sample_rate_hz = 48000;                                     // dummy setting
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

        i2s_pdm_rx_clk_config_t clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(48000);
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

    // Alloc framebuffer
    uint16_t *frameBuffer;
    if (psramFound())
        frameBuffer = (uint16_t *)ps_malloc(SPECTRUM_WIDTH * SPECTRUM_HEIGHT * sizeof(uint16_t));
    else {
        closeSdCard(); // Close SDCard to release RAM, as it won't be used
        frameBuffer = (uint16_t *)malloc(SPECTRUM_WIDTH * SPECTRUM_HEIGHT * sizeof(uint16_t));
    }

    if (!frameBuffer) {
        Serial.println("Error alloc drawing frameBuffer, exiting");
        displayError("Not Enough RAM", true);
        return;
    }
    tft.drawRect(
        tftWidth / 2 - SPECTRUM_WIDTH / 2 - 2,
        tftHeight / 2 - SPECTRUM_HEIGHT / 2 - 2,
        SPECTRUM_WIDTH + 4,
        SPECTRUM_HEIGHT + 4,
        bruceConfig.priColor
    );

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

        // Render
        for (int y = 0; y < SPECTRUM_HEIGHT; y++) {
            for (int x = 0; x < SPECTRUM_WIDTH; x++) {
                int index = (x + posData) % HISTORY_LEN;
                uint8_t val = fftHistory[index * SPECTRUM_HEIGHT + y];
                uint16_t color =
                    rgb565(ImageData[val * 3 + 0], ImageData[val * 3 + 1], ImageData[val * 3 + 2]);
                frameBuffer[y * SPECTRUM_WIDTH + x] = color;
            }
        }

        tft.pushImage(
            tftWidth / 2 - SPECTRUM_WIDTH / 2,
            tftHeight / 2 - SPECTRUM_HEIGHT / 2,
            SPECTRUM_WIDTH,
            SPECTRUM_HEIGHT,
            frameBuffer
        );
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

void mic_record() {
    ioExpander.turnPinOnOff(IO_EXP_MIC, HIGH);

    bool gpioInput = false;
    if (!isGPIOOutput(GPIO_NUM_0)) {
        gpioInput = true;
        gpio_hold_en(GPIO_NUM_0);
    }
    InitI2SMicroPhone();

    // Alloc buffers in PSRAM if available
    if (psramFound()) i2s_buffer = (int16_t *)ps_malloc(FFT_SIZE * sizeof(int16_t));
    else i2s_buffer = (int16_t *)malloc(FFT_SIZE * sizeof(int16_t));
    if (!i2s_buffer) {
        displayError("Fail to alloc buffers, exiting", true);
        return;
    }

    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) {
        displayError("No space left on device", true);
        return;
    }

    char filename[32];
    int index = 0;

    if (!fs->exists("/BruceMIC")) {
        if (!fs->mkdir("/BruceMIC")) {
            displayError("Error creating directory", true);
            return;
        }
    }

    do {
        snprintf(filename, sizeof(filename), "/BruceMIC/recording_%d.wav", index++);
    } while (fs->exists(filename));
    File audioFile = fs->open(filename, FILE_WRITE, true);
    if (!audioFile) {
        displayError("Error creating file", true);
        return;
    }

    int record_time = 3;
    int last_record_time = -1;
    bool redraw = false;

    while (!check(SelPress)) {
        if (check(PrevPress)) { record_time--; }
        if (check(NextPress)) { record_time++; }

        record_time = constrain(record_time, 0, 300);
        if (record_time != last_record_time) {
            redraw = true;
            last_record_time = record_time;
        } else {
            redraw = false;
        }

        if (redraw) {
            String text;
            if (record_time != 0) {
                text = String("Length: ") + String(record_time) + String("s");
            } else {
                text = String("Length: Unlimited");
            }
            displayRedStripe(text, getComplementaryColor2(bruceConfig.priColor), bruceConfig.priColor);
        }
    }

    const int headerSize = 44;
    byte header[headerSize] = {0};

    audioFile.write(header, headerSize);

    unsigned long dataSize = 0;

    int bytesPerRead = FFT_SIZE * sizeof(int16_t);
    unsigned long startMillis = millis();
    if (record_time != 0) {
        displayRedStripe("Recording...", 0xffff, 0x5db9);
        while (millis() - startMillis < (unsigned long)record_time * 1000) {
            size_t bytesRead = 0;
            i2s_channel_read(i2s_chan, i2s_buffer, bytesPerRead, &bytesRead, 1000);
            if (bytesRead > 0) {
                audioFile.write((const uint8_t *)i2s_buffer, bytesRead);
                dataSize += bytesRead;
            }
        }
    } else {
        displayRedStripe("Rec... Press Sel to stop", 0xffff, 0x5db9);
        while (!check(SelPress)) {
            size_t bytesRead = 0;
            i2s_channel_read(i2s_chan, (char *)i2s_buffer, bytesPerRead, &bytesRead, 1000);

            if (bytesRead > 0) {
                audioFile.write((const uint8_t *)i2s_buffer, bytesRead);
                dataSize += bytesRead;
            }
        }
    }

    audioFile.seek(0);
    CreateWavHeader(header, dataSize);
    audioFile.write(header, headerSize);
    audioFile.close();

    delay(10);
    if (deinitMicroPhone()) Serial.println("Fail disabling I2S Driver");
    if (gpioInput) {
        gpio_hold_dis(GPIO_NUM_0);
        pinMode(GPIO_NUM_0, INPUT);
    } else {
        pinMode(GPIO_NUM_0, OUTPUT);
        digitalWrite(GPIO_NUM_0, LOW);
    }
    Serial.println("Recording finished");
    displaySuccess("Recording Finished", true);
    ioExpander.turnPinOnOff(IO_EXP_MIC, LOW);
}

#else
void mic_test() {}
void mic_test_one_task() {}
void mic_record() {}
#endif
