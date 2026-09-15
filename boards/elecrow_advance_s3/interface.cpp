#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <Wire.h>
#include <interface.h>

// =============================================================================
//  CrowPanel Advance 3.5" (ESP32-S3) interface
//  - Display: ILI9488 over SPI (handled by TFT_eSPI)
//  - Touch:   GT911 capacitive, directly on I2C (SDA=15, SCL=16, INT=47),
//             no IO expander, no dedicated RST line.
//  - Backlight: direct PWM on GPIO38.
// =============================================================================

#if defined(HAS_CAPACITIVE_TOUCH) && defined(TOUCH_GT911_I2C)
#include "TouchDrvGT911.hpp"
TouchDrvGT911 touch;
struct TouchPointPro {
    int16_t x = 0;
    int16_t y = 0;
};
#endif

/***************************************************************************************
** Function name: _setup_gpio()
***************************************************************************************/
void _setup_gpio() {
    bruceConfig.colorInverted = 0;

#if defined(HAS_CAPACITIVE_TOUCH) && defined(TOUCH_GT911_I2C)
    // Bring up the I2C bus the GT911 lives on.
    setSysI2CBus(&Wire1);
    Wire1.begin(SYS_I2C_SDA, SYS_I2C_SCL);

    // GT911 power-on reset sequence. No RST line on this board, so hold INT low
    // briefly to keep address 0x5D, then release it as an input.
    pinMode(BOARD_TOUCH_INT, OUTPUT);
    digitalWrite(BOARD_TOUCH_INT, LOW);
    delay(10);
    pinMode(BOARD_TOUCH_INT, INPUT);
    delay(50);

    // No reset line available -> pass -1 for RST.
    touch.setPins(-1, BOARD_TOUCH_INT);
    if (!touch.begin(Wire1, GT911_SLAVE_ADDRESS_L, SYS_I2C_SDA, SYS_I2C_SCL)) {
        Serial.println("Failed to find GT911 touch - check wiring!");
    } else {
        Serial.println("GT911 touch started");
    }
#endif
}

/***************************************************************************************
** Function name: _post_setup_gpio()
***************************************************************************************/
void _post_setup_gpio() {
    pinMode(TFT_BL, OUTPUT);
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 255);
}

/***************************************************************************************
** Function name: getBattery()
***************************************************************************************/
int getBattery() { return 100; }

/*********************************************************************
** Function: _setBrightness
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 255;
    else if (brightval == 75) dutyCycle = 130;
    else if (brightval == 50) dutyCycle = 70;
    else if (brightval == 25) dutyCycle = 20;
    else if (brightval == 0) dutyCycle = 0;
    else dutyCycle = ((brightval * 255) / 100);
    ledcWrite(TFT_BL, dutyCycle);
}

/*********************************************************************
** Function: InputHandler (GT911 capacitive)
**********************************************************************/
void InputHandler(void) {
#if defined(HAS_CAPACITIVE_TOUCH) && defined(TOUCH_GT911_I2C)
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        static unsigned long tm = millis();
        TouchPointPro t;
        uint8_t touched = 0;
        static uint8_t rot = 5;

        if (rot != bruceConfigPins.rotation) {
            if (bruceConfigPins.rotation == 1) {
                touch.setMaxCoordinates(TFT_HEIGHT, TFT_WIDTH);
                touch.setSwapXY(true);
                touch.setMirrorXY(false, true);
            }
            if (bruceConfigPins.rotation == 3) {
                touch.setMaxCoordinates(TFT_HEIGHT, TFT_WIDTH);
                touch.setSwapXY(true);
                touch.setMirrorXY(true, false);
            }
            if (bruceConfigPins.rotation == 0) {
                touch.setMaxCoordinates(TFT_WIDTH, TFT_HEIGHT);
                touch.setSwapXY(false);
                touch.setMirrorXY(false, false);
            }
            if (bruceConfigPins.rotation == 2) {
                touch.setMaxCoordinates(TFT_WIDTH, TFT_HEIGHT);
                touch.setSwapXY(false);
                touch.setMirrorXY(true, true);
            }
            rot = bruceConfigPins.rotation;
        }

        static bool lastTouchState = false;
        static unsigned long lastTouchTime = 0;

        touched = touch.getPoint(&t.x, &t.y);
        bool currentTouchState = touched > 0;

        if (currentTouchState && !lastTouchState && (millis() - lastTouchTime) > 100) {
            lastTouchTime = millis();
        } else if (!currentTouchState || lastTouchState) {
            touched = 0;
        }
        lastTouchState = currentTouchState;

        if (((millis() - tm) > 190 || LongPress) && touched) {
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else goto END;

            touchPoint.x = t.x;
            touchPoint.y = t.y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
        END:
            d_tmp = millis();
        }
    }
#else
    checkPowerSaveTime();
    PrevPress = false;
    NextPress = false;
    SelPress = false;
    AnyKeyPress = false;
    EscPress = false;
#endif
}

/*********************************************************************
** Function: powerOff
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
**********************************************************************/
void checkReboot() {}
