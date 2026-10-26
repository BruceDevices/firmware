/*
 * Freenove FNK0104B (2.8" ILI9341 ESP32-S3) - based on the ES3C28P design
 * Board interface implementation for Bruce firmware
 *
 * Hardware:
 *   - ESP32-S3R8 (8MB OPI PSRAM, 16MB QSPI Flash)
 *   - ILI9341V 240x320 IPS TFT (SPI)
 *   - FT6336G Capacitive Touch (I2C @ 0x38)
 *   - ES8311 Audio Codec (I2C @ 0x18) + FM8002E Amplifier
 *   - MicroSD Card (SDIO mode)
 *   - WS2812 RGB LED (GPIO42)
 *   - Battery ADC (GPIO9, x2 divider)
 *   - BOOT button (GPIO0)
 */

#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <Arduino.h>
#include <Wire.h>
#include <globals.h>
#include <interface.h>

#define ES8311_ADDR 0x18
#define ES8311_CODEC 1

// =============================================
// SD Card SDIO pins (defined locally for USE_SD_MMC)
// =============================================
#ifdef USE_SD_MMC
#define PIN_SD_CLK 38
#define PIN_SD_CMD 40
#define PIN_SD_D0 39
#endif

// =============================================
// Touch Screen local defines
// =============================================
#define FNK0104AB_TOUCH_SDA 16
#define FNK0104AB_TOUCH_SCL 15
#define FNK0104AB_TOUCH_RST 18
#define FNK0104AB_TOUCH_INT 17
#define FNK0104AB_TOUCH_ADDR 0x38

// =============================================
// Audio & GPIO local defines
// =============================================
#define FNK0104AB_AMP_EN 1  // FM8002E amplifier enable (active LOW)
#define FNK0104AB_BTN_PIN 0 // BOOT button
#define FNK0104AB_BTN_ACT LOW

// =============================================
// Touch Screen (FT6336G via I2C, hal_touch_init/hal_touch_read -- src/hal/inputs/touch.cpp)
// =============================================
// Raw touch reads portrait-native (rotation 0 needs no transform in the old code): derived
// algebraically from InputHandler's old per-rotation remap block -- see src/hal/README.md.
static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    cfg.pin_sda = FNK0104AB_TOUCH_SDA;
    cfg.pin_scl = FNK0104AB_TOUCH_SCL;
    cfg.pin_rst = FNK0104AB_TOUCH_RST;
    cfg.pin_irq = FNK0104AB_TOUCH_INT;
    // rotation:        0      1      2      3
    const bool swapXY[4] = {false, true, false, true};
    const bool mirrorX[4] = {false, false, true, true};
    const bool mirrorY[4] = {false, true, true, false};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // bclk,ws,dout,mclk
    bruceConfigPins.speaker_bus = {(gpio_num_t)5, (gpio_num_t)7, (gpio_num_t)8, (gpio_num_t)4};
    // ES8311 ADC: clocked off the speaker BCLK, and its LRCLK doubles as the mic WS
    // clk,data,ws,type
    bruceConfigPins.mic_bus = {(gpio_num_t)5, (gpio_num_t)6, (gpio_num_t)7, MIC_TYPE_I2S_MSB};
    bruceConfigPins.i2c_bus = {(gpio_num_t)16, (gpio_num_t)15}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 16;
    bruceConfigPins.rfRx = 15;
    bruceConfigPins.irTx = 2;
    bruceConfigPins.irRx = 3;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)15, (gpio_num_t)16}; // rx, tx (CH9329)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)14, (gpio_num_t)2, (gpio_num_t)3, (gpio_num_t)21};
    bruceConfigPins.PN532_bus = {(gpio_num_t)14, (gpio_num_t)2, (gpio_num_t)3, (gpio_num_t)21};
    // CC1101/NRF24/W5500 share the same expansion-pin SPI bus (sck=14, miso=2, mosi=3)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)14, (gpio_num_t)2, (gpio_num_t)3, (gpio_num_t)21, (gpio_num_t)2, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)14, (gpio_num_t)2, (gpio_num_t)3, (gpio_num_t)21, (gpio_num_t)3
    }; // sck,miso,mosi,cs(ss),ce
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)14, (gpio_num_t)2, (gpio_num_t)3, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
#endif

#ifdef USE_SD_MMC
    // ---- SD Card (SDIO mode, 1-bit) ----
    SD.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
#endif

    // ---- Touch Screen Init (FT6336G) ----
    if (!hal_touch_init(touchCfg(), FNK0104AB_TOUCH_ADDR)) {
        Serial.println("Touch IC not Started");
    }

    // ---- Amplifier: disable by default (active LOW, so HIGH = disabled) ----
    pinMode(FNK0104AB_AMP_EN, OUTPUT);
    digitalWrite(FNK0104AB_AMP_EN, HIGH); // FM8002E: HIGH = disabled

    // ---- Start with default module configs ----
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;

    Serial.begin(115200);
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Description:   second stage gpio setup after TFT is initialized
***************************************************************************************/
void _post_setup_gpio() {
    // Backlight control via PWM (must be after TFT init)
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100); // Full brightness initially
}

/*********************************************************************
** Function: setBrightness
** set brightness value (0-100)
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles touch and button inputs
** Maps to PrevPress, NextPress, SelPress, AnyKeyPress, EscPress
**********************************************************************/
void InputHandler(void) {
    static long tm = 0;

    if (millis() - tm > 200 || LongPress) {
        // ---- Touch Screen Input ----
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            tm = millis();
            if (!hal_touch_apply(t)) return;
        }

        // ---- BOOT Button Input ----
        if (digitalRead(FNK0104AB_BTN_PIN) == FNK0104AB_BTN_ACT) {
            if (!wakeUpScreen()) {
                AnyKeyPress = true;
                SelPress = true;
            }
            // Debounce
            while (digitalRead(FNK0104AB_BTN_PIN) == FNK0104AB_BTN_ACT) delay(10);
        }
    }
}

/*********************************************************************
** Function: powerOff
** Turns off the device (deep sleep with BOOT button wakeup)
**********************************************************************/
void powerOff() {
    // Turn off backlight
    hal_bright_set(TFT_BL, 0);
    // Turn off amplifier
    digitalWrite(FNK0104AB_AMP_EN, HIGH);
    // Send display to sleep
    tft.writecommand(0x10); // SLPIN
    // Wake on BOOT button press
    esp_sleep_enable_ext0_wakeup((gpio_num_t)FNK0104AB_BTN_PIN, FNK0104AB_BTN_ACT);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: goToDeepSleep
** Puts the device into DeepSleep
**********************************************************************/
void goToDeepSleep() { powerOff(); }

/*********************************************************************
** Function: checkReboot
** Button logic to turn off the device
**********************************************************************/
void checkReboot() {
    // Long press BOOT button to power off
    int c = 0;
    while (digitalRead(FNK0104AB_BTN_PIN) == FNK0104AB_BTN_ACT) {
        delay(100);
        c++;
        if (c > 20) { // 2 second long press
            powerOff();
        }
    }
}

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging (not available on ES3C28P)
***************************************************************************************/
bool isCharging() { return false; }

// =============================================
// ES8311 Audio Codec Setup (via I2C register writes)
// =============================================
#ifdef ES8311_CODEC
#include "core/utils.h"

// ES8311 Register Addresses
#define ES8311_REG00_RESET 0x00
#define ES8311_REG01_CLK_MGR 0x01
#define ES8311_REG02_CLK_MGR 0x02
#define ES8311_REG03_CLK_MGR 0x03
#define ES8311_REG04_CLK_MGR 0x04
#define ES8311_REG05_CLK_MGR 0x05
#define ES8311_REG06_CLK_MGR 0x06
#define ES8311_REG07_CLK_MGR 0x07
#define ES8311_REG08_CLK_MGR 0x08
#define ES8311_REG09_SDPIN 0x09
#define ES8311_REG0A_SDPOUT 0x0A
#define ES8311_REG0B_SYSTEM 0x0B
#define ES8311_REG0C_SYSTEM 0x0C
#define ES8311_REG0D_SYSTEM 0x0D
#define ES8311_REG0E_SYSTEM 0x0E
#define ES8311_REG0F_SYSTEM 0x0F
#define ES8311_REG10_SYSTEM 0x10
#define ES8311_REG11_SYSTEM 0x11
#define ES8311_REG12_SYSTEM 0x12
#define ES8311_REG13_SYSTEM 0x13
#define ES8311_REG14_ADC 0x14
#define ES8311_REG15_ADC 0x15
#define ES8311_REG16_ADC 0x16
#define ES8311_REG17_ADC 0x17
#define ES8311_REG18_ADC 0x18
#define ES8311_REG19_ADC 0x19
#define ES8311_REG1A_ADC 0x1A
#define ES8311_REG1B_ADC 0x1B
#define ES8311_REG1C_ADC 0x1C
#define ES8311_REG32_DAC 0x32
#define ES8311_REG37_DAC 0x37
#define ES8311_REG44_GPIO 0x44
#define ES8311_REG45_GPIO 0x45

static void es8311_write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

static uint8_t es8311_read_reg(uint8_t reg) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)ES8311_ADDR, (uint8_t)1);
    if (Wire.available()) return Wire.read();
    return 0;
}

/*********************************************************************
** Function: _setup_codec_speaker
** Handles audio CODEC to enable/disable speaker
** Configures ES8311 for I2S slave mode, 16-bit, with MCLK from ESP32
**********************************************************************/
void _setup_codec_speaker(bool enable) {
    // Enable/disable amplifier (FM8002E is active LOW)
    digitalWrite(FNK0104AB_AMP_EN, enable ? LOW : HIGH);

    if (enable) {
        // Reset codec (exact sequence from LCD Wiki es8311_init)
        es8311_write_reg(ES8311_REG00_RESET, 0x1F);
        delay(20);
        es8311_write_reg(ES8311_REG00_RESET, 0x00); // Clear reset
        es8311_write_reg(ES8311_REG00_RESET, 0x80); // Power on

        // Clock config: MCLK from MCLK pin, enable all clocks
        es8311_write_reg(ES8311_REG01_CLK_MGR, 0x3F);

        // Clock dividers for 44100Hz sample rate with MCLK from I2S master
        // The ESP8266Audio library generates MCLK automatically
        // Use coefficients matching the I2S MCLK output
        // pre_div=1, pre_multi=0, adc_div=1, dac_div=1, fs=0, lrck=0x00FF, bclk_div=4
        es8311_write_reg(ES8311_REG02_CLK_MGR, 0x00); // pre_div=1, pre_multi=1x
        es8311_write_reg(ES8311_REG03_CLK_MGR, 0x10); // ADC OSR = 0x10
        es8311_write_reg(ES8311_REG04_CLK_MGR, 0x10); // DAC OSR = 0x10
        es8311_write_reg(ES8311_REG05_CLK_MGR, 0x00); // ADC/DAC clk div = 1
        es8311_write_reg(ES8311_REG06_CLK_MGR, 0x03); // BCLK divider
        es8311_write_reg(ES8311_REG07_CLK_MGR, 0x00); // LRCK divider high
        es8311_write_reg(ES8311_REG08_CLK_MGR, 0xFF); // LRCK divider low

        // SDP format: slave mode, 16-bit I2S (bits[3:2] = 11 = 16-bit)
        es8311_write_reg(ES8311_REG09_SDPIN, 0x0C);  // SDP in: 16-bit
        es8311_write_reg(ES8311_REG0A_SDPOUT, 0x0C); // SDP out: 16-bit

        // Power up analog and DAC (from LCD Wiki es8311_init)
        es8311_write_reg(ES8311_REG0D_SYSTEM, 0x01); // Power up analog circuitry
        es8311_write_reg(ES8311_REG0E_SYSTEM, 0x02); // Enable analog PGA, ADC modulator
        es8311_write_reg(ES8311_REG12_SYSTEM, 0x00); // Power up DAC
        es8311_write_reg(ES8311_REG13_SYSTEM, 0x10); // Enable output to HP drive
        es8311_write_reg(ES8311_REG1C_ADC, 0x6A);    // ADC EQ bypass, cancel DC offset
        es8311_write_reg(ES8311_REG37_DAC, 0x08);    // Bypass DAC equalizer

        // Set volume (85% like LCD Wiki example)
        es8311_write_reg(ES8311_REG32_DAC, 0xD8); // ~85% volume

        Serial.println("ES8311 speaker enabled");
    } else {
        // Power down codec
        es8311_write_reg(ES8311_REG0D_SYSTEM, 0xFC); // Power down analog
        es8311_write_reg(ES8311_REG00_RESET, 0x00);  // Power down
        Serial.println("ES8311 speaker disabled");
    }
}

/*********************************************************************
** Function: _setup_codec_mic
** Handles audio CODEC to enable/disable microphone
**********************************************************************/
void _setup_codec_mic(bool enable) {
    if (enable) {
        // Reset codec (exact sequence from LCD Wiki)
        es8311_write_reg(ES8311_REG00_RESET, 0x1F);
        delay(20);
        es8311_write_reg(ES8311_REG00_RESET, 0x00);
        es8311_write_reg(ES8311_REG00_RESET, 0x80);

        // Clock config: MCLK derived from BCLK (bit7=1) since Bruce's mic
        // I2S driver doesn't output MCLK on a GPIO pin.
        // BCLK = 48000 * 16 * 2 = 1,536,000 Hz
        // pre_multi=8x (0x03) to get internal 12.288 MHz from 1.536 MHz BCLK
        // Matches coefficient table: {1536000, 48000, 0x01, 0x03, ...}
        es8311_write_reg(ES8311_REG01_CLK_MGR, 0xBF); // bit7=1: MCLK from BCLK
        es8311_write_reg(ES8311_REG02_CLK_MGR, 0x18); // pre_div=1, pre_multi=8x
        es8311_write_reg(ES8311_REG03_CLK_MGR, 0x10); // ADC OSR
        es8311_write_reg(ES8311_REG04_CLK_MGR, 0x10); // DAC OSR
        es8311_write_reg(ES8311_REG05_CLK_MGR, 0x00); // ADC/DAC clk div = 1
        es8311_write_reg(ES8311_REG06_CLK_MGR, 0x03); // BCLK divider
        es8311_write_reg(ES8311_REG07_CLK_MGR, 0x00); // LRCK divider high
        es8311_write_reg(ES8311_REG08_CLK_MGR, 0xFF); // LRCK divider low

        // I2S format: 16-bit
        es8311_write_reg(ES8311_REG09_SDPIN, 0x0C);
        es8311_write_reg(ES8311_REG0A_SDPOUT, 0x0C);

        // Power up analog and enable ADC
        es8311_write_reg(ES8311_REG0D_SYSTEM, 0x01); // Power up analog circuitry
        es8311_write_reg(ES8311_REG0E_SYSTEM, 0x02); // Enable analog PGA, ADC modulator
        es8311_write_reg(ES8311_REG12_SYSTEM, 0x00); // Power up DAC too
        es8311_write_reg(ES8311_REG13_SYSTEM, 0x10); // Enable HP driver

        // Microphone config (from LCD Wiki es8311_microphone_config)
        es8311_write_reg(ES8311_REG14_ADC, 0x1A); // Enable analog MIC, max PGA gain
        es8311_write_reg(ES8311_REG17_ADC, 0xC8); // ADC gain
        es8311_write_reg(ES8311_REG1C_ADC, 0x6A); // ADC EQ bypass, DC offset cancel

        Serial.println("ES8311 microphone enabled");
    } else {
        es8311_write_reg(ES8311_REG0D_SYSTEM, 0xFC);
        es8311_write_reg(ES8311_REG0E_SYSTEM, 0x6A);
        es8311_write_reg(ES8311_REG00_RESET, 0x00);
        Serial.println("ES8311 microphone disabled");
    }
}

#endif // ES8311_CODEC
