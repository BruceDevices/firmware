#include "core/powerSave.h"
#include <M5Unified.h>
#include <interface.h>

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    M5.begin(); // Need to test if SDCard inits with the new setup
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    uint8_t percent = 0;
    percent = M5.Power.getBatteryLevel();
    return (percent < 0) ? 1 : (percent >= 100) ? 100 : percent;
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    uint8_t _tmp = (255 * brightval) / 100;
    M5.Lcd.setBrightness(_tmp);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, check(SelPress), AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    M5.update();
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool aPressed = (M5.BtnA.isPressed());
    bool bPressed = (M5.BtnB.isPressed());
    bool cPressed = (M5.BtnC.isPressed());

    bool anyPressed = aPressed || bPressed || cPressed;
    if (anyPressed) tm = millis();
    if (anyPressed && wakeUpScreen()) return;

    AnyKeyPress = anyPressed;
    PrevPress = aPressed;
    EscPress = aPressed;
    NextPress = cPressed;
    SelPress = bPressed;
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { M5.Power.powerOff(); }

void goToDeepSleep() { M5.Power.deepSleep(); }

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {}

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
bool isCharging() {
    if (M5.Power.getBatteryCurrent() > 0 || M5.Power.getBatteryCurrent()) return true;
    else return false;
}
