#include "core/powerSave.h"
#include <M5Unified.h>
#include <interface.h>

// Rotary encoder
#include <RotaryEncoder.h>
RotaryEncoder *encoder = nullptr;
IRAM_ATTR void checkPosition() { encoder->tick(); }

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    M5.begin();
    bruceConfig.colorInverted = 0;
    pinMode(ENCODER_KEY, INPUT);
    encoder = new RotaryEncoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);
    attachInterrupt(digitalPinToInterrupt(ENCODER_INA), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_INB), checkPosition, CHANGE);
}
/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { M5.Display.setBrightness(brightval); }

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int level = M5.Power.getBatteryLevel();
    return (level < 0) ? 0 : (level >= 100) ? 100 : level;
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = millis(); // debauce for buttons
    static unsigned long lastEncoderMoveMs = 0;
    static int posDifference = 0;
    static int lastPos = 0;
    bool sel = !LOW;

    int newPos = encoder->getPosition();
    if (newPos != lastPos) {
        posDifference += (newPos - lastPos);
        lastPos = newPos;
        lastEncoderMoveMs = millis();
    } else if (posDifference != 0 && millis() - lastEncoderMoveMs > 30) {
        // Drop any stale queued steps once the encoder has stopped moving.
        posDifference = 0;
    }

    if (millis() - tm < 200 && !LongPress) return;

    sel = digitalRead(ENCODER_KEY);

    if (posDifference != 0 || sel == LOW) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }
    if (posDifference > 0) {
        PrevPress = true;
        posDifference--;
    }
    if (posDifference < 0) {
        NextPress = true;
        posDifference++;
    }

    if (sel == LOW) {
        posDifference = 0;
        SelPress = true;
        tm = millis();
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { M5.Power.powerOff(); }
