#include "core/powerSave.h"
#include "core/utils.h"
#include <interface.h>

/***************************************************************************************
** LILYGO T-Display-C5 — Bruce board interface
** ST7789 170x320 (no touch), two physical buttons (IO0 + BOOT).
***************************************************************************************/

/***************************************************************************************
** Function name: _setup_gpio()
***************************************************************************************/
void _setup_gpio() {
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(TFT_MOSI, OUTPUT);
    digitalWrite(TFT_MOSI, HIGH);
    pinMode(TFT_SCLK, OUTPUT);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    pinMode(TFT_RST, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);

#ifdef HAS_2_BUTTONS
    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
#endif

    // All external SPI radios share CS=4 / control=5 on this board.
    pinMode(NRF24_SS_PIN, OUTPUT);
    pinMode(CC1101_SS_PIN, OUTPUT);
    pinMode(W5500_SS_PIN, OUTPUT);
    digitalWrite(NRF24_SS_PIN, HIGH);
    digitalWrite(CC1101_SS_PIN, HIGH);
    digitalWrite(W5500_SS_PIN, HIGH);

    if (SDCARD_CS >= 0) { // no microSD on the T-Display-C5 (SDCARD_CS = -1)
        pinMode(SDCARD_CS, OUTPUT);
        digitalWrite(SDCARD_CS, HIGH);
    }

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
}

/***************************************************************************************
** Function name: _post_setup_gpio()
***************************************************************************************/
void _post_setup_gpio() {
    // No touch controller on this board — nothing to calibrate.
}

/***************************************************************************************
** Function name: getBattery()
** NOTE: battery gauge lives in the AXP2602 PMU over I2C (SDA=2, SCL=3, INT=10).
**       Returning 0 for now — implement AXP2602 readout here if you want % / charge.
***************************************************************************************/
int getBattery() { return 0; }

/***************************************************************************************
** Function name: isCharging()
***************************************************************************************/
bool isCharging() { return false; }

/*********************************************************************
** Function: setBrightness
** Backlight is a plain GPIO (BL=25) on PWM. The display does NOT
** depend on the AXP2602 — its begin() only wakes the battery fuel
** gauge, it gates no display rail — so no PMU init is needed to get
** a picture. AXP2602 is only required for the battery readouts below.
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    // NOTE: analogWrite()/LEDC auto-attach on GPIO25 can fail silently on the
    // ESP32-C5 with this core and leave the backlight pin undriven (black
    // screen). Drive it as a plain digital on/off instead so the panel is
    // always lit. PWM dimming can be restored later with an explicit
    // ledcAttach(TFT_BL, 5000, 8) once confirmed working.
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, brightval > 0 ? HIGH : LOW);
}

/*********************************************************************
** Function: InputHandler   (two-button scheme)
**   BTN_A short = Next      BTN_A long = Prev
**   BTN_B short = Select    BTN_B long = Back/Esc
** Sets PrevPress / NextPress / SelPress / EscPress / AnyKeyPress.
**********************************************************************/
void InputHandler(void) {
    static bool aWasDown = false, bWasDown = false;
    static unsigned long aDownAt = 0, bDownAt = 0;
    static unsigned long lastAction = 0;
    const unsigned long LONG_MS = 400;    // hold beyond this = long press
    const unsigned long LOCKOUT_MS = 150; // debounce between registered actions

    bool aDown = (digitalRead(BTN_A) == LOW); // BTN_ACT == LOW
    bool bDown = (digitalRead(BTN_B) == LOW);

    AnyKeyPress = (aDown || bDown);

    // Wake from power-save on any press; swallow that press.
    if (AnyKeyPress && wakeUpScreen()) {
        aWasDown = aDown;
        bWasDown = bDown;
        return;
    }

    if (millis() - lastAction >= LOCKOUT_MS) {
        // Button A: release decides short (Next) vs long (Prev)
        if (aDown && !aWasDown) aDownAt = millis();
        if (!aDown && aWasDown) {
            if (millis() - aDownAt >= LONG_MS) PrevPress = true;
            else NextPress = true;
            lastAction = millis();
        }
        // Button B: release decides short (Select) vs long (Esc)
        if (bDown && !bWasDown) bDownAt = millis();
        if (!bDown && bWasDown) {
            if (millis() - bDownAt >= LONG_MS) EscPress = true;
            else SelPress = true;
            lastAction = millis();
        }
    }

    aWasDown = aDown;
    bWasDown = bDown;
}

/*********************************************************************
** Function: powerOff
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
**********************************************************************/
void checkReboot() {}
