#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include <driver/rtc_io.h>
#include <globals.h>
#include <interface.h>

// ATS Mini (ESP32-S3 + SI4732) board glue for Bruce
// Display: ST7789 170x320 8-bit parallel (TFT_eSPI, configured in pins_arduino.h)
// Input:   rotary encoder
//            rotate            -> Prev / Next
//            short push        -> Select (fires on release)
//            long push (600ms) -> Back / Esc
//            BOOT button       -> Back / Esc (only on units where it is soldered)
//          Rotating while the button is held is ignored as a Select, so push-and-turn never selects by accident.

#include <rotary_decoder.h>
RotaryDecoder *encoder = nullptr;
void pollEncoder(void) {
    if (encoder) encoder->poll();
}

#define ATS_LONG_PRESS_MS 600
#define ATS_DEBOUNCE_MS 30

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // powerOff() holds these pads through deep sleep, release them after wake-up
    gpio_hold_dis((gpio_num_t)PIN_POWER_ON);
    gpio_hold_dis((gpio_num_t)PIN_AMP_EN);
    gpio_hold_dis((gpio_num_t)TFT_BL);
    gpio_deep_sleep_hold_dis();

    // Receiver LDO on, same as the stock ATS Mini firmware does at boot
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);

    // Bruce does not use the receiver audio path: keep the amplifier off and L/R muted
    pinMode(PIN_AMP_EN, OUTPUT);
    digitalWrite(PIN_AMP_EN, LOW);
    pinMode(AUDIO_MUTE, OUTPUT);
    digitalWrite(AUDIO_MUTE, HIGH);

    pinMode(ENCODER_KEY, INPUT_PULLUP);
    pinMode(ENCODER_INA, INPUT_PULLUP);
    pinMode(ENCODER_INB, INPUT_PULLUP);
    pinMode(ESC_BTN, INPUT_PULLUP);
    encoder = new RotaryDecoder();
    encoder->begin(ENCODER_INA, ENCODER_INB, 2);
}

/*********************************************************************
**  Function: setBrightness
**  set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long lastEncoderMoveMs = 0;
    static int posDifference = 0;
    static int lastPos = 0;

    static bool keyDown = false;       // encoder push button currently held
    static bool longFired = false;     // Esc already sent for the current hold
    static bool ignoreRelease = false; // button was used for push-and-turn
    static unsigned long keyDownAt = 0;

    unsigned long now = millis();

    int newPos = encoder->getPosition();
    if (newPos != lastPos) {
        posDifference += (newPos - lastPos);
        // Independent running total for consumers that apply the full backlog at once
        // (see drainRotarySteps() in globals.h)
        RotaryNetSteps += (newPos - lastPos);
        lastPos = newPos;
        lastEncoderMoveMs = now;
    } else if (posDifference != 0 && now - lastEncoderMoveMs > 30) {
        // Drop any stale queued steps once the encoder has stopped moving
        posDifference = 0;
    }

    bool selEvent = false;
    bool escEvent = digitalRead(ESC_BTN) == BTN_ACT;
    bool pushed = digitalRead(SEL_BTN) == BTN_ACT;

    if (pushed && !keyDown) { // pressed
        keyDown = true;
        longFired = false;
        ignoreRelease = false;
        keyDownAt = now;
    } else if (pushed && keyDown) { // held
        if (posDifference != 0) ignoreRelease = true;
        if (!longFired && !ignoreRelease && now - keyDownAt >= ATS_LONG_PRESS_MS) {
            longFired = true;
            escEvent = true;
        }
    } else if (!pushed && keyDown) { // released
        keyDown = false;
        if (!longFired && !ignoreRelease && now - keyDownAt >= ATS_DEBOUNCE_MS) selEvent = true;
    }

    if (posDifference != 0 || selEvent || escEvent) {
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
    if (selEvent) {
        posDifference = 0;
        SelPress = true;
    }
    if (escEvent) { EscPress = true; }
}

/*********************************************************************
** Function: powerOff
** Deep sleep with the receiver LDO, amplifier and backlight kept off.
** Wake-up: push the encoder button.
**********************************************************************/
void powerOff() {
    tft.fillScreen(bruceConfig.bgColor);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);
    tft.writecommand(0x10); // ST7789 SLPIN
    delay(120);

    const int offPins[] = {TFT_BL, PIN_AMP_EN, PIN_POWER_ON};
    for (int pin : offPins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        gpio_hold_en((gpio_num_t)pin);
    }
    gpio_deep_sleep_hold_en();

    esp_sleep_enable_ext0_wakeup((gpio_num_t)ENCODER_KEY, BTN_ACT);
    rtc_gpio_pullup_en((gpio_num_t)ENCODER_KEY);
    rtc_gpio_pulldown_dis((gpio_num_t)ENCODER_KEY);
    esp_deep_sleep_start();
}

void checkReboot() {}
