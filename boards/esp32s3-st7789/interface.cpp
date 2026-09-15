#include "CYD28_TouchscreenR.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <interface.h>

#define CYD28_DISPLAY_HOR_RES_MAX 320
#define CYD28_DISPLAY_VER_RES_MAX 240
CYD28_TouchR touch(CYD28_DISPLAY_HOR_RES_MAX, CYD28_DISPLAY_VER_RES_MAX);

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // Keep XPT2046 CS high until needed
    pinMode(XPT2046_SPI_CONFIG_CS_GPIO_NUM, OUTPUT);
    digitalWrite(XPT2046_SPI_CONFIG_CS_GPIO_NUM, HIGH);

    bruceConfig.colorInverted = 1;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup — runs AFTER tft.init()
***************************************************************************************/
void _post_setup_gpio() {
    // Initialize XPT2046 touch using hardware SPI (shares bus with display)
    if (!touch.begin(&tft.getSPIinstance())) {
        Serial.println("[TOUCH] XPT2046 not started");
    } else {
        Serial.println("[TOUCH] XPT2046 started OK");
    }
    touch.setRotation(ROTATION);

    // Backlight on
    pinMode(TFT_BL, OUTPUT);
    analogWrite(TFT_BL, 255);
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { return 0; }

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
bool isCharging() { return false; }

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
    } else {
        analogWrite(TFT_BL, brightval);
    }
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        if (touch.touched()) {
            auto t = touch.getPointScaled();

            // Coordinate transform based on rotation
            if (bruceConfigPins.rotation == 3) {
                t.y = (tftHeight + 20) - t.y;
                t.x = tftWidth - t.x;
            }
            if (bruceConfigPins.rotation == 0) {
                int tmp = t.x;
                t.x = tftWidth - t.y;
                t.y = tmp;
            }
            if (bruceConfigPins.rotation == 2) {
                int tmp = t.x;
                t.x = t.y;
                t.y = (tftHeight + 20) - tmp;
            }

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

#ifdef HAS_BTN
    checkPowerSaveTime();
    if (digitalRead(BTN_PIN) == BTN_ACT) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        SelPress = true;
        long tmp = millis();
        while ((millis() - tmp) < 200 && digitalRead(BTN_PIN) == BTN_ACT);
    }
#endif
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { esp_deep_sleep_start(); }

/*********************************************************************
** Function: goToDeepSleep
** location: mykeyboard.cpp
** Puts the device into DeepSleep
**********************************************************************/
void goToDeepSleep() { esp_deep_sleep_start(); }

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device
**********************************************************************/
void checkReboot() {}
