/*
 * ESP32S3-Viking - Board interface implementation for Bruce firmware
 *
 * Hardware:
 *   - ESP32-S3-WROOM-1 N16R8 (16MB Flash, 8MB OPI PSRAM)
 *   - ESP32-S3 GPIO Extension Board KIT B
 *   - ILI9341 2.8" SPI TFT 240x320 (TENSTAR ROBOT)
 *   - XPT2046 Resistive Touchscreen (TOUCH_CS=IO18, T_IRQ=IO8)
 *     handled by TFT_eSPI built-in touch support (USE_TFT_eSPI_TOUCH)
 *   - WS2812 RGB LED (GPIO48)
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
#include <Arduino.h>
#include <globals.h>
#include <interface.h>

/******************************************************************************
 ** Function name:       _setup_gpio()
 ** Description:         Initial GPIO setup for the device
 ******************************************************************************/
void _setup_gpio() {
    // ---- WS2812 LED off at startup (neopixelWrite built into Arduino-ESP32) ----
    neopixelWrite(RGB_LED, 0, 0, 0);

    // ---- Touch CS pin - TFT_eSPI will handle the rest ----
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);

    // ---- BOOT button ----
    pinMode(BTN_PIN, INPUT);

    // ---- Default module config ----
    bruceConfigPins.rfModule    = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule  = PN532_I2C_MODULE;
    bruceConfigPins.irRx        = RXLED;
    bruceConfigPins.irTx        = TXLED;

    Serial.begin(115200);
}

/******************************************************************************
 ** Function name:       _post_setup_gpio()
 ** Description:         Second stage GPIO setup - runs after TFT init
 ******************************************************************************/
void _post_setup_gpio() {
    // ---- Touch calibration via TFT_eSPI built-in ----
    pinMode(TOUCH_CS, OUTPUT);

    uint16_t calData[5];
    File caldata = LittleFS.open("/calData", "r");
    if (!caldata) {
        // No calibration data - run calibration
        tft.setRotation(ROTATION);
        tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, 10);
        caldata = LittleFS.open("/calData", "w");
        if (caldata) {
            caldata.printf("%d\n%d\n%d\n%d\n%d\n",
                calData[0], calData[1], calData[2], calData[3], calData[4]);
            caldata.close();
        }
    } else {
        Serial.print("\nTouch calibration data: ");
        for (int i = 0; i < 5; i++) {
            String line = caldata.readStringUntil('\n');
            calData[i] = line.toInt();
            Serial.printf("%d, ", calData[i]);
        }
        Serial.println();
        caldata.close();
    }
    tft.setTouch(calData);

    // ---- Backlight on ----
    pinMode(TFT_BL, OUTPUT);
    analogWrite(TFT_BL, 255);

    // ---- LED off after TFT init ----
    neopixelWrite(RGB_LED, 0, 0, 0);
}

/******************************************************************************
 ** Function:            getBattery
 ******************************************************************************/
int getBattery() { return -1; }

/******************************************************************************
 ** Function:            _setBrightness
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
 ** Uses TFT_eSPI built-in XPT2046 touch + BOOT button
 ******************************************************************************/
void InputHandler(void) {
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        // ---- Touch via TFT_eSPI built-in (USE_TFT_eSPI_TOUCH) ----
        uint16_t t_x, t_y;
        bool touched = tft.getTouch(&t_x, &t_y);
        if (touched) {
            d_tmp = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else goto END;
            touchPoint.x = t_x;
            touchPoint.y = t_y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
            END:
            d_tmp = millis();
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
}

/******************************************************************************
 ** Function:            powerOff
 ******************************************************************************/
void powerOff() {
    neopixelWrite(RGB_LED, 0, 0, 0);
    if (TFT_BL >= 0) analogWrite(TFT_BL, 0);
    tft.writecommand(0x10); // SLPIN
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PIN, BTN_ACT);
    esp_deep_sleep_start();
}

void goToDeepSleep() { powerOff(); }

/******************************************************************************
 ** Function:            checkReboot
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
 ******************************************************************************/
bool isCharging() { return false; }
