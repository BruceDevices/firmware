#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include <M5Unified.h>
#include <interface.h>

// Rotary encoder
#include <rotary_decoder.h>
RotaryDecoder *encoder = nullptr;
void pollEncoder(void) { encoder->poll(); }

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    M5.begin();
    setSysI2CBus(M5.In_I2C.getPort() == I2C_NUM_1 ? &Wire1 : &Wire);
    bruceConfig.colorInverted = 0;
    pinMode(ENCODER_KEY, INPUT);
    pinMode(ENCODER_INA, INPUT_PULLUP);
    pinMode(ENCODER_INB, INPUT_PULLUP);
    encoder = new RotaryDecoder();
    encoder->begin(ENCODER_INA, ENCODER_INB, 2);
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
        // Independent running total for consumers that want to apply the
        // full pending backlog in one pass instead of one step at a time
        // (see drainRotarySteps() in globals.h). Never cleared by the
        // stale-drop below -- it's drained exactly, not time-limited.
        RotaryNetSteps += (newPos - lastPos);
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

    if (PrevPress && SelPress) {
        EscPress = true;
        SelPress = false;
        PrevPress = false;
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { M5.Power.powerOff(); }
