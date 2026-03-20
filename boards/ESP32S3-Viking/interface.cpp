/*
 * ESP32S3-Viking - Board interface implementation for Bruce firmware
 *
 * Hardware:
 *   - ESP32-S3-WROOM-1 N16R8 (16MB Flash, 8MB OPI PSRAM)
 *   - ESP32-S3 GPIO Extension Board KIT B
 *   - ILI9341 2.8" SPI TFT 240x320 (TENSTAR ROBOT)
 *   - XPT2046 Resistive Touchscreen (shared SPI, TOUCH_CS=IO18, T_IRQ=IO8)
 *   - WS2812 RGB LED (GPIO48 on bare board)
 *   - BOOT button (GPIO0)
 *
 * Wiring (left side of KIT B, top to bottom, no cable crossing):
 *   GND -> GND, 3V3 -> VCC
 *   IO4 -> LCD_RST, IO5 -> LCD_CS, IO6 -> LCD_DC
 *   IO7 -> LCD_MOSI/T_DIN, IO15 -> LCD_SCK/T_CLK
 *   IO16 -> LCD_MISO/T_DO, IO17 -> LCD_BL
 *   IO18 -> T_CS, IO8 -> T_IRQ
 */
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <SPI.h>
#include <FastLED.h>
#include <globals.h>
#include <interface.h>

// =============================================
// WS2812 RGB LED (GPIO48) via FastLED
// =============================================
static CRGB _leds[1];
static bool _ledsInitialized = false;

static void initLeds() {
    if (!_ledsInitialized) {
        FastLED.addLeds<WS2812B, RGB_LED, GRB>(_leds, 1);
        FastLED.setBrightness(0);
        _leds[0] = CRGB::Black;
        FastLED.show();
        _ledsInitialized = true;
    }
}

static void setLedOff() {
    initLeds();
    _leds[0] = CRGB::Black;
    FastLED.show();
}

// =============================================
// XPT2046 Touch via SPI (shared bus with ILI9341)
// Uses hardware SPI.transfer() - no bit-bang needed
// =============================================
#define XPT_CMD_X   0xD0  // Measure X (differential, 12-bit)
#define XPT_CMD_Y   0x90  // Measure Y (differential, 12-bit)

static bool touchInitialized = false;

// Read a 12-bit ADC value from XPT2046 via hardware SPI
static uint16_t xpt_read(uint8_t cmd) {
    SPI.transfer(cmd);
    uint8_t hi = SPI.transfer(0x00);
    uint8_t lo = SPI.transfer(0x00);
    return ((hi << 8) | lo) >> 3; // 12-bit result
}

// Read touch position using hardware SPI transaction
static bool xpt2046_read_touch(int16_t &tx, int16_t &ty) {
    // Check IRQ pin (active LOW when touched)
    if (digitalRead(TOUCH_IRQ) != LOW) return false;

    // Take SPI bus for touch (lower frequency needed)
    SPI.beginTransaction(SPISettings(SPI_TOUCH_FREQUENCY, MSBFIRST, SPI_MODE0));
    digitalWrite(TOUCH_CS, LOW);
    delayMicroseconds(20);

    // Average multiple readings for stability
    int32_t sum_x = 0, sum_y = 0;
    const int SAMPLES = 4;
    for (int i = 0; i < SAMPLES; i++) {
        sum_x += xpt_read(XPT_CMD_X);
        sum_y += xpt_read(XPT_CMD_Y);
    }
    int16_t raw_x = sum_x / SAMPLES;
    int16_t raw_y = sum_y / SAMPLES;

    digitalWrite(TOUCH_CS, HIGH);
    SPI.endTransaction();

    // Skip if values out of valid range (noise)
    if (raw_x < 100 || raw_x > 4000 || raw_y < 100 || raw_y > 4000) return false;

    // Map raw ADC to screen coordinates
    // XPT2046 raw range ~200-3900, screen 240x320 landscape
    // In landscape (rotation=1): screen_x maps to raw_y, screen_y maps to raw_x
    int16_t scr_x = map(raw_y, XPT2046_Y_MIN, XPT2046_Y_MAX, 0, TFT_HEIGHT - 1);
    int16_t scr_y = map(raw_x, XPT2046_X_MIN, XPT2046_X_MAX, 0, TFT_WIDTH - 1);

    // Clamp to screen bounds
    scr_x = constrain(scr_x, 0, TFT_HEIGHT - 1);
    scr_y = constrain(scr_y, 0, TFT_WIDTH - 1);

    tx = scr_x;
    ty = scr_y;
    return true;
}

/******************************************************************************
 ** Function name:       _setup_gpio()
 ** Description:         Initial GPIO setup for the device
 ******************************************************************************/
void _setup_gpio() {
    // ---- WS2812 LED off at startup (was glowing purple on boot) ----
    setLedOff();

    // ---- SPI bus pins for TFT + Touch ----
    // TFT_eSPI handles MOSI/SCK/MISO/CS/DC/RST via its own init
    // We only need to pre-configure TOUCH_CS and TOUCH_IRQ here
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH); // Deselect touch
    pinMode(TOUCH_IRQ, INPUT_PULLUP); // T_IRQ is active LOW, needs pull-up

    // ---- BOOT button ----
    pinMode(BTN_PIN, INPUT);

    touchInitialized = true;

    // ---- Default module config ----
    bruceConfigPins.rfModule   = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;
    bruceConfigPins.irRx = RXLED;
    bruceConfigPins.irTx = TXLED;

    Serial.begin(115200);
}

/******************************************************************************
 ** Function name:       _post_setup_gpio()
 ** Description:         Second stage GPIO setup after TFT is initialized
 ******************************************************************************/
void _post_setup_gpio() {
    // Backlight on (TFT_BL = IO17)
    pinMode(TFT_BL, OUTPUT);
    analogWrite(TFT_BL, 255); // Full brightness

    // Ensure LED is still off after TFT init
    setLedOff();
}

/******************************************************************************
 ** Function:            getBattery
 ** Description:         Battery not available on bare ESP32-S3 N16R8 - returns -1
 ******************************************************************************/
int getBattery() {
    return -1;
}

/******************************************************************************
 ** Function:            _setBrightness
 ** Set backlight brightness value (0-100)
 ******************************************************************************/
void _setBrightness(uint8_t brightval) {
    if (TFT_BL < 0) return;
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100.0f));
        analogWrite(TFT_BL, bl);
    }
}

/******************************************************************************
 ** Function:            InputHandler
 ** Handles XPT2046 touch + BOOT button inputs
 ** Maps touch to touchPoint and AnyKeyPress / SelPress etc.
 ******************************************************************************/
void InputHandler(void) {
    static long tm = 0;
    if (millis() - tm > 200 || LongPress) {
        // ---- XPT2046 Touch Input ----
        if (touchInitialized) {
            int16_t t_x, t_y;
            if (xpt2046_read_touch(t_x, t_y)) {
                tm = millis();
                if (!wakeUpScreen()) AnyKeyPress = true;
                else return;
                touchPoint.x = t_x;
                touchPoint.y = t_y;
                touchPoint.pressed = true;
                touchHeatMap(touchPoint);
            }
        }

        // ---- BOOT Button Input ----
        if (digitalRead(BTN_PIN) == BTN_ACT) {
            if (!wakeUpScreen()) {
                AnyKeyPress = true;
                SelPress    = true;
            }
            // Debounce
            while (digitalRead(BTN_PIN) == BTN_ACT) delay(10);
        }
    }
}

/******************************************************************************
 ** Function:            powerOff
 ** Turns off the device (deep sleep, wakeup on BOOT button)
 ******************************************************************************/
void powerOff() {
    // LED off
    setLedOff();
    // Backlight off
    if (TFT_BL >= 0) analogWrite(TFT_BL, 0);
    // Display sleep
    tft.writecommand(0x10); // SLPIN
    // Wake on BOOT button
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PIN, BTN_ACT);
    esp_deep_sleep_start();
}

void goToDeepSleep() { powerOff(); }

/******************************************************************************
 ** Function:            checkReboot
 ** Long press BOOT button (2s) to power off
 ******************************************************************************/
void checkReboot() {
    int c = 0;
    while (digitalRead(BTN_PIN) == BTN_ACT) {
        delay(100);
        if (++c > 20) powerOff();
    }
}

/******************************************************************************
 ** Function:            isCharging
 ** Not available on bare board
 ******************************************************************************/
bool isCharging() {
    return false;
}
