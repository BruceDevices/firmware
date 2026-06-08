#include "core/powerSave.h"
#include "core/utils.h"
#include <M5Unified.h>
#include <interface.h>

constexpr uint32_t kBtnBDoublePressWindowMs = 270;
constexpr uint32_t kBtnBLongPressMs = 500;

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    M5.begin(); // Need to test if SDCard inits with the new setup
    bruceConfig.colorInverted = 0;
    M5.BtnA.setDebounceThresh(8);
    M5.BtnB.setDebounceThresh(8);
    M5.BtnB.setHoldThresh(kBtnBLongPressMs);
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int percent = 0;
    percent = M5.Power.getBatteryLevel();
    return (percent < 0) ? 0 : (percent >= 100) ? 100 : percent;
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { M5.Display.setBrightness(brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    static uint32_t btnBFirstReleaseMs = 0;
    static bool btnBWaitingSecondClick = false;
    static bool btnBLongPressFired = false;
    if (millis() - tm < 200 && !LongPress) return;
    M5.update();

    bool emitNext = false;
    bool emitPrev = false;
    bool emitEsc = false;
    uint32_t now = millis();

    auto t = M5.Touch.getDetail();
    if (t.isPressed() || t.isHolding()) {
        tm = millis();
        if (wakeUpScreen()) return;

        touchPoint.x = t.x;
        touchPoint.y = t.y;
        touchPoint.pressed = true;
        touchHeatMap(touchPoint);
    } else touchPoint.pressed = false;

    bool btnAActive = M5.BtnA.isPressed() || M5.BtnA.isHolding();
    bool btnBActive = M5.BtnB.isPressed() || M5.BtnB.isHolding();

    if (M5.BtnB.wasPressed()) btnBLongPressFired = false;

    if (btnBActive && !btnBLongPressFired && M5.BtnB.pressedFor(kBtnBLongPressMs)) {
        btnBLongPressFired = true;
        btnBWaitingSecondClick = false;
        emitEsc = true;
    }

    if (M5.BtnB.wasReleased()) {
        if (btnBLongPressFired) {
            btnBLongPressFired = false;
        } else if (btnBWaitingSecondClick && now - btnBFirstReleaseMs <= kBtnBDoublePressWindowMs) {
            btnBWaitingSecondClick = false;
            emitPrev = true;
        } else {
            btnBWaitingSecondClick = true;
            btnBFirstReleaseMs = now;
        }
    }

    if (btnBWaitingSecondClick && !btnBActive && now - btnBFirstReleaseMs > kBtnBDoublePressWindowMs) {
        btnBWaitingSecondClick = false;
        emitNext = true;
    }

    AnyKeyPress = btnAActive || btnBActive || btnBWaitingSecondClick || M5.BtnA.wasClicked() || emitNext ||
                  emitPrev || emitEsc;
    if (!AnyKeyPress) return;

    if ((btnAActive || btnBActive) && wakeUpScreen()) return;

    if (M5.BtnA.wasClicked()) SelPress = true;
    if (emitNext) NextPress = true;
    if (emitPrev) PrevPress = true;
    if (emitEsc) EscPress = true;
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { M5.Power.powerOff(); }
