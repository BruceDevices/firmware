/*
 * ESP32S3-Viking - Board interface implementation for Bruce firmware
 *
 * Hardware:
 *   - ESP32-S3-WROOM-1 N16R8 (16MB Flash, 8MB OPI PSRAM)
 *   - ESP32-S3 GPIO Extension Board KIT B
 *   - ILI9341 2.8" SPI TFT 240x320 (TENSTAR ROBOT)
 *   - XPT2046 Resistive Touchscreen (TOUCH_CS=IO18, T_IRQ=IO8)
 *     handled by TFT_eSPI built-in touch support (USE_TFT_eSPI_TOUCH)
 *   - PN532 NFC/RFID (SPI - osobna magistrala, GPIO 9-14)
 *   - WS2812 RGB LED (GPIO48)
 *   - BOOT button (GPIO0)
 *
 * Wiring TFT (left side of KIT B, top to bottom, no cable crossing):
 *   GND -> GND, 3V3 -> VCC
 *   IO4 -> T_IRQ, IO5 -> T_DO/LCD_MISO, IO6 -> T_DIN/LCD_MOSI
 *   IO7 -> T_CS, IO15 -> T_CLK/LCD_SCK
 *   IO16 -> LCD_BL, IO17 -> LCD_DC
 *   IO18 -> LCD_RST, IO8 -> LCD_CS *
 * Wiring PN532 SPI (left side of KIT B, lower block):
 *   IO9  -> PN532 SCK
 *   IO10 -> PN532 MISO
 *   IO11 -> PN532 MOSI
 *   IO12 -> PN532 SS (CS)
 *   IO13 -> PN532 IRQ
 *   IO14 -> PN532 RSTO
 *
 * Wiring RF 433MHz (right side of KIT B):
 *   IO19 -> RF TX DATA (nadajnik)
 *   IO20 -> RF RX DATA (odbiornik)
 *
 * Touch recalibration: hold BOOT button during startup to erase saved
 * calibration data and run the calibration wizard again.
 */
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <globals.h>
#include <interface.h>

/******************************************************************************
 ** Function name:      _setup_gpio()
 ** Description:        Initial GPIO setup for the device
 ******************************************************************************/
void _setup_gpio() {
    // ---- WS2812 LED off at startup ----
    rgbLedWrite(RGB_LED, 0, 0, 0);

    // ---- Touch CS pin - TFT_eSPI will handle the rest ----
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);

    // ---- BOOT button ----
    pinMode(BTN_PIN, INPUT);

    // ---- Default module config ----
    bruceConfigPins.rfModule    = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule  = PN532_SPI_MODULE;
    bruceConfigPins.irRx        = RXLED;
    bruceConfigPins.irTx        = TXLED;
    Serial.begin(115200);
}

/******************************************************************************
 ** Function name:      _post_setup_gpio()
 ** Description:        Second stage GPIO setup - runs after TFT init
 ******************************************************************************/
void _post_setup_gpio() {
    // ---- Touch calibration via TFT_eSPI built-in ----
    pinMode(TOUCH_CS, OUTPUT);

    uint16_t calData[5];
    bool doCalibration = false;

    // Check if BOOT button is held at startup -> force recalibration
    if (digitalRead(BTN_PIN) == BTN_ACT) {
        // Wait to confirm intentional press (debounce)
        delay(100);
        if (digitalRead(BTN_PIN) == BTN_ACT) {
            // Show message on screen
            tft.setRotation(ROTATION);
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setTextSize(2);
            tft.setCursor(20, 100);
            tft.println("Recalibrating touch...");
            tft.setCursor(20, 130);
            tft.println("Release BOOT button");
            // Wait for button release
            while (digitalRead(BTN_PIN) == BTN_ACT) delay(10);
            delay(500);
            // Delete saved calibration data
            LittleFS.remove("/calData");
            doCalibration = true;
        }
    }

    File caldata = LittleFS.open("/calData", "r");
    if (!caldata || doCalibration) {
        if (caldata) caldata.close();
        // No calibration data (or forced) - run calibration
        tft.setRotation(ROTATION);
        tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, 10);
        caldata = LittleFS.open("/calData", "w");
        if (caldata) {
            caldata.printf("%d\n%d\n%d\n%d\n%d\n",
                calData[0], calData[1], calData[2], calData[3], calData[4]);
            caldata.close();
        }
    } else {
        // Load saved calibration data
        for (int i = 0; i < 5; i++) {
            calData[i] = caldata.parseInt();
        }
        caldata.close();
        tft.setTouch(calData);
    }

    // ---- Backlight on ----
    if (TFT_BL >= 0) analogWrite(TFT_BL, 255);

    // ---- LED off ----
    rgbLedWrite(RGB_LED, 0, 0, 0);
}

/******************************************************************************
 ** Function name:      _setBrightness()
 ** Description:        Set TFT backlight brightness
 ******************************************************************************/
void _setBrightness(uint8_t brightness) {
    int bl = map(brightness, 0, 100, MINBRIGHT, 255);
    analogWrite(TFT_BL, bl);
}

/******************************************************************************
 ** Function name:      InputHandler()
 ** Description:        Handles touch and button input for Bruce UI
 ******************************************************************************/
void InputHandler() {
    static unsigned long lastTouch = 0;
    unsigned long now = millis();

    // ---- Touch input (poll every ~200ms or on LongPress) ----
    if (now - lastTouch > 200 || LongPress) {
        lastTouch = now;
        uint16_t t_x = 0, t_y = 0;
        bool touched = tft.getTouch(&t_x, &t_y);
        if (touched) {
            touchPoint.x = t_x;
            touchPoint.y = t_y;
            touchPoint.pressed = true;
            if (wakeUpScreen()) AnyKeyPress = true;
            else {
                AnyKeyPress = true;
                touchHeatMap(touchPoint);
            }
        } else {
            touchPoint.pressed = false;
        }
    }

    // ---- BOOT Button ----
    if (digitalRead(BTN_PIN) == BTN_ACT) {
        if (!wakeUpScreen()) {
            AnyKeyPress = true;
            SelPress = true;
        }
        while (digitalRead(BTN_PIN) == BTN_ACT) delay(10);
    }
}

/******************************************************************************
 ** Function name:      powerOff()
 ******************************************************************************/
void powerOff() {
    rgbLedWrite(RGB_LED, 0, 0, 0);
    if (TFT_BL >= 0) analogWrite(TFT_BL, 0);
    tft.writecommand(0x10); // SLPIN
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PIN, BTN_ACT);
    esp_deep_sleep_start();
}

void goToDeepSleep() { powerOff(); }

/******************************************************************************
 ** Function name:      checkReboot()
 ******************************************************************************/
void checkReboot() {
    int c = 0;
    while (digitalRead(BTN_PIN) == BTN_ACT) {
        delay(100);
        if (++c > 20) powerOff();
    }
}

/******************************************************************************
 ** Function name:      isCharging()
 ******************************************************************************/
bool isCharging() { return false; }
