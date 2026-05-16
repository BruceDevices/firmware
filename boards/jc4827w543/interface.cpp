#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <interface.h>

#define XPT2046_CS TOUCH_CS

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    pinMode(XPT2046_CS, OUTPUT);
    digitalWrite(XPT2046_CS, HIGH);
    bruceConfigPins.rotation = 0;  // 0 degrees for JC4827W543 (480x272 landscape)
    bruceConfig.colorInverted = 0; // No color inversion for NV3041A
    tft.setRotation(bruceConfigPins.rotation);
    // Touch calibration for XPT2046 - may need adjustment
    uint16_t calData[5] = {275, 3500, 280, 3590, 3};
    tft.setTouch(calData);
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description: second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    // Brightness control
    pinMode(TFT_BL, OUTPUT);
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_BITS);
    ledcWrite(TFT_BL, 255);
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
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
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm > 200 || LongPress) {
        TouchPoint t;
        checkPowerSaveTime();
        digitalWrite(TFT_CS, HIGH);
        digitalWrite(TOUCH_CS, LOW);
        bool _IH_touched = tft.getTouch(&t.x, &t.y);
        digitalWrite(TOUCH_CS, HIGH);
        if (_IH_touched) {
            NextPress = false;
            PrevPress = false;
            UpPress = false;
            DownPress = false;
            SelPress = false;
            EscPress = false;
            AnyKeyPress = false;
            NextPagePress = false;
            PrevPagePress = false;
            touchPoint.pressed = false;
            _IH_touched = false;

            // Handle rotation for 480x272 display
            if (bruceConfigPins.rotation == 0) {
                t.y = (tftHeight + 20) - t.y;
                t.x = tftWidth - t.x;
            }
            if (bruceConfigPins.rotation == 3) {
                uint16_t tmp = t.x;
                t.x = map((tftHeight + 20) - t.y, 0, 272, 0, 480);
                t.y = map(tmp, 0, 480, 0, 272);
            }
            if (bruceConfigPins.rotation == 1) {
                uint16_t tmp = t.x;
                t.x = map(t.y, 0, 272, 0, 480);
                t.y = map(tftWidth - tmp, 0, 480, 0, 272);
            }
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;

            // Touch point global variable
            touchPoint.x = t.x;
            touchPoint.y = t.y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
        }
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    // Implement power off for this board
    // May need external circuitry or GPIO control
    log_i("Power off requested");
}

void goToDeepSleep() {
    powerOff();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device
**********************************************************************/
void checkReboot() {
    // Not implemented for this board
}

/***************************************************************************************
** Function name: getBattery()
** Description: Returns battery level 1-100
***************************************************************************************/
int getBattery() {
    // No battery ADC connected by default
    return 100;
}

/***************************************************************************************
** Function name: isCharging()
** Description: Determines if device is charging
***************************************************************************************/
bool isCharging() {
    return false;
}