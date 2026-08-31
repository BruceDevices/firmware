#include "core/powerSave.h"
#include <interface.h>

/*
 * ESP32 TFT 128x160
 *
 * Buttons:
 *   GPIO32 = SELECT
 *   GPIO33 = UP
 *   GPIO27 = DOWN
 *
 * Wiring:
 *   GPIO32 ---- button ---- GND
 *   GPIO33 ---- button ---- GND
 *   GPIO27 ---- button ---- GND
 *
 * INPUT_PULLUP is used, therefore:
 *   HIGH = released
 *   LOW  = pressed
 */

void _setup_gpio() {
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);

    bruceConfig.colorInverted = 0;
    bruceConfigPins.rotation = 0;
}

void _post_setup_gpio() {
}

int getBattery() {
    return 0;
}

void _setBrightness(uint8_t brightval) {
    // Your TFT has no software-controlled backlight.
}

void InputHandler(void) {
    static unsigned long tm = 0;

    // Small debounce
    if (millis() - tm < 120 && !LongPress) {
        return;
    }

    bool selPressed = (digitalRead(SEL_BTN) == LOW);
    bool upPressed  = (digitalRead(UP_BTN) == LOW);
    bool downPressed = (digitalRead(DW_BTN) == LOW);

    if (!selPressed && !upPressed && !downPressed) {
        return;
    }

    tm = millis();

    // Wake screen if necessary
    if (wakeUpScreen()) {
        return;
    }

    if (upPressed) {
        UpPress = true;
        PrevPress = true;
        AnyKeyPress = true;
    }

    if (downPressed) {
        DownPress = true;
        NextPress = true;
        AnyKeyPress = true;
    }

    if (selPressed) {
        SelPress = true;
        AnyKeyPress = true;
    }
}

void powerOff() {
}

void checkReboot() {
}
