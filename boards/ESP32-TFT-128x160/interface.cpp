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
 *   GPIO25 ---- button ---- GND select
 *   GPIO33 ---- button ---- GND up
 *   GPIO27 ---- button ---- GND down
 *   GPIO26 ---- button ---- GND esc
 *
 * INPUT_PULLUP is used, therefore:
 *   HIGH = released
 *   LOW  = pressed
 */

void _setup_gpio() {
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
    pinMode(ESC_BTN, INPUT_PULLUP);

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
    static unsigned long lastPress = 0;

    bool selPressed  = (digitalRead(SEL_BTN) == LOW);
    bool upPressed   = (digitalRead(UP_BTN) == LOW);
    bool downPressed = (digitalRead(DW_BTN) == LOW);
    bool escPressed  = (digitalRead(ESC_BTN) == LOW);

    if (!selPressed && !upPressed && !downPressed && !escPressed) {
        return;
    }

    // Debounce(try removing or decreasing or increasing if issue occur)
    if (millis() - lastPress < 150) {
        return;
    }

    lastPress = millis();

    // Wake screen if sleeping
    if (wakeUpScreen()) {
        return;
    }

    if (upPressed) {
        UpPress = true;
        AnyKeyPress = true;
    }

    if (downPressed) {
        DownPress = true;
        AnyKeyPress = true;
    }

    if (selPressed) {
        SelPress = true;
        AnyKeyPress = true;
    }

    if (escPressed) {
        EscPress = true;
        AnyKeyPress = true;
    }
}
