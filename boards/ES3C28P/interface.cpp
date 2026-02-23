/*
 * ES3C28P - 2.8" IPS ESP32-S3 Display Module
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
#include <Arduino.h>
#include <Wire.h>
#include <globals.h>
#include <interface.h>

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
#define ES3C28P_TOUCH_SDA 16
#define ES3C28P_TOUCH_SCL 15
#define ES3C28P_TOUCH_RST 18
#define ES3C28P_TOUCH_INT 17
#define ES3C28P_TOUCH_ADDR 0x38

// =============================================
// Audio & GPIO local defines
// =============================================
#define ES3C28P_AMP_EN 1  // FM8002E amplifier enable (active LOW)
#define ES3C28P_BTN_PIN 0 // BOOT button
#define ES3C28P_BTN_ACT LOW

// =============================================
// Touch Screen (FT6336G via I2C)
// =============================================
// The FT6336G uses the same register protocol as CST816S/FT6236
// Register 0x02: number of touch points
// Register 0x03-0x06: touch point 1 X/Y coordinates
#define FT6336_REG_NUM_TOUCHES 0x02
#define FT6336_REG_TOUCH_DATA 0x03

static bool touchInitialized = false;

static uint8_t ft6336_read_reg(uint8_t reg) {
    Wire.beginTransmission(ES3C28P_TOUCH_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(ES3C28P_TOUCH_ADDR, 1);
    if (Wire.available()) return Wire.read();
    return 0;
}

static bool ft6336_read_touch(int16_t &x, int16_t &y) {
    uint8_t touches = ft6336_read_reg(FT6336_REG_NUM_TOUCHES);
    if (touches == 0 || touches > 2) return false;

    uint8_t data[4];
    Wire.beginTransmission(ES3C28P_TOUCH_ADDR);
    Wire.write(FT6336_REG_TOUCH_DATA);
    Wire.endTransmission(false);
    Wire.requestFrom(ES3C28P_TOUCH_ADDR, 4);
    for (int i = 0; i < 4; i++) { data[i] = Wire.read(); }

    x = ((data[0] & 0x0F) << 8) | data[1];
    y = ((data[2] & 0x0F) << 8) | data[3];
    return true;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
#ifdef USE_SD_MMC
    // ---- SD Card (SDIO mode, 1-bit) ----
    SD.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
#endif

    // ---- Touch Screen Init (FT6336G) ----
    // Reset touch controller
    pinMode(ES3C28P_TOUCH_RST, OUTPUT);
    digitalWrite(ES3C28P_TOUCH_RST, LOW);
    delay(10);
    digitalWrite(ES3C28P_TOUCH_RST, HIGH);
    delay(300);

    // Touch interrupt pin
    pinMode(ES3C28P_TOUCH_INT, INPUT);

    // Initialize I2C bus (shared with audio codec ES8311)
    Wire.begin(ES3C28P_TOUCH_SDA, ES3C28P_TOUCH_SCL);
    touchInitialized = true;

    // ---- Amplifier: disable by default (active LOW, so HIGH = disabled) ----
    pinMode(ES3C28P_AMP_EN, OUTPUT);
    digitalWrite(ES3C28P_AMP_EN, HIGH); // FM8002E: HIGH = disabled

    // ---- Start with default module configs ----
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;
    bruceConfigPins.irRx = RXLED;
    bruceConfigPins.irTx = TXLED;

    Serial.begin(115200);
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Description:   second stage gpio setup after TFT is initialized
***************************************************************************************/
void _post_setup_gpio() {
    // Backlight control via PWM (must be after TFT init)
    pinMode(TFT_BL, OUTPUT);
    analogWrite(TFT_BL, 255); // Full brightness initially
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 0-100
**                Uses GPIO9 (ADC1_CH8) with x2 voltage divider
**                Battery range: 2500mV (0%) to 4200mV (100%)
***************************************************************************************/
int getBattery() {
    static bool adcInitialized = false;
    if (!adcInitialized) {
        pinMode(ANALOG_BAT_PIN, INPUT);
        analogSetAttenuation(ADC_11db); // Full range for 0-3.3V input
        adcInitialized = true;
    }

    // Read ADC and convert to actual battery voltage (with x2 divider)
    uint32_t adcReading = analogReadMilliVolts(ANALOG_BAT_PIN);
    float actualVoltage = (float)adcReading * 2.0f; // x2 voltage divider

    // Battery voltage range per ES3C28P specs:
    // Min: 2500mV (cutoff/empty), Max: 4200mV (fully charged)
    const float MIN_VOLTAGE = 2500.0f;
    const float MAX_VOLTAGE = 4200.0f;

    int percent = (int)(((actualVoltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 100.0f);

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    return percent;
}

/*********************************************************************
** Function: setBrightness
** set brightness value (0-100)
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

/*********************************************************************
** Function: InputHandler
** Handles touch and button inputs
** Maps to PrevPress, NextPress, SelPress, AnyKeyPress, EscPress
**********************************************************************/
void InputHandler(void) {
    static long tm = 0;

    if (millis() - tm > 200 || LongPress) {
        // ---- Touch Screen Input ----
        if (touchInitialized) {
            int16_t raw_x, raw_y;
            if (ft6336_read_touch(raw_x, raw_y)) {
                tm = millis();

                // Apply rotation transformation
                int16_t t_x = raw_x;
                int16_t t_y = raw_y;

                if (bruceConfigPins.rotation == 1) {
                    // Landscape: swap and mirror
                    t_x = raw_y;
                    t_y = (TFT_WIDTH - 1) - raw_x;
                } else if (bruceConfigPins.rotation == 2) {
                    // Portrait inverted
                    t_x = (TFT_WIDTH - 1) - raw_x;
                    t_y = (TFT_HEIGHT - 1) - raw_y;
                } else if (bruceConfigPins.rotation == 3) {
                    // Landscape inverted
                    t_x = (TFT_HEIGHT - 1) - raw_y;
                    t_y = raw_x;
                }
                // rotation == 0: portrait default, no transform needed

                if (!wakeUpScreen()) AnyKeyPress = true;
                else return;

                // Set global touch point
                touchPoint.x = t_x;
                touchPoint.y = t_y;
                touchPoint.pressed = true;
                touchHeatMap(touchPoint);
            }
        }

        // ---- BOOT Button Input ----
        if (digitalRead(ES3C28P_BTN_PIN) == ES3C28P_BTN_ACT) {
            if (!wakeUpScreen()) {
                AnyKeyPress = true;
                SelPress = true;
            }
            // Debounce
            while (digitalRead(ES3C28P_BTN_PIN) == ES3C28P_BTN_ACT) delay(10);
        }
    }
}

/*********************************************************************
** Function: powerOff
** Turns off the device (deep sleep with BOOT button wakeup)
**********************************************************************/
void powerOff() {
    // Turn off backlight
    analogWrite(TFT_BL, 0);
    // Turn off amplifier
    digitalWrite(ES3C28P_AMP_EN, HIGH);
    // Send display to sleep
    tft.writecommand(0x10); // SLPIN
    // Wake on BOOT button press
    esp_sleep_enable_ext0_wakeup((gpio_num_t)ES3C28P_BTN_PIN, ES3C28P_BTN_ACT);
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
    while (digitalRead(ES3C28P_BTN_PIN) == ES3C28P_BTN_ACT) {
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
    digitalWrite(ES3C28P_AMP_EN, enable ? LOW : HIGH);

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
    // Set microphone I2S pin for Bruce's mic module
    extern gpio_num_t mic_bclk_pin;
    mic_bclk_pin = (gpio_num_t)BCLK;

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
