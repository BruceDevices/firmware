#include "core/powerSave.h"
#include "core/utils.h"
#include <M5Unified.h>
#include <interface.h>

#define TFT_BRIGHT_CHANNEL 0
#define TFT_BRIGHT_Bits 8
#define TFT_BRIGHT_FREQ 5000

constexpr uint32_t kDwDoublePressWindowMs = 250;
constexpr uint32_t kDwLongPressMs = 600;
constexpr uint32_t kDwDebounceMs = 8;

static volatile uint32_t dw_last_isr_ms = 0;
static volatile uint32_t dw_press_ms = 0;
static volatile uint32_t dw_first_release_ms = 0;
static volatile bool dw_is_down = false;
static volatile bool dw_waiting = false;
static volatile bool dw_double_ready = false;
static volatile bool dw_long_seen = false;

void IRAM_ATTR isr_dw_btn() {
    uint32_t now = millis();
    if (now - dw_last_isr_ms < kDwDebounceMs) return;
    dw_last_isr_ms = now;
    bool pressed = (digitalRead(DW_BTN) == BTN_ACT);
    if (pressed) {
        dw_is_down = true;
        dw_press_ms = now;
        return;
    }

    dw_is_down = false;
    if (dw_long_seen) {
        dw_long_seen = false;
        dw_waiting = false;
        return;
    }

    if ((now - dw_press_ms) < kDwLongPressMs) {
        if (dw_waiting && (now - dw_first_release_ms) <= kDwDoublePressWindowMs) {
            dw_double_ready = true;
            dw_waiting = false;
        } else {
            dw_waiting = true;
            dw_first_release_ms = now;
        }
    } else {
        dw_waiting = false;
    }
}
/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    M5.begin();
    Wire1.begin(47, 48);

    pinMode(SEL_BTN, INPUT);
    pinMode(DW_BTN, INPUT);

    M5.Power.setExtOutput(false); // It buzzes it ext power is turned on

    /*
| Device  | SCK   | MISO  | MOSI  | CS    | GDO0/CE   |
| ---     | :---: | :---: | :---: | :---: | :---:     |
| SD Card | 5     | 4     | 6     | 7     | ---       |
| CC1101  | 5     | 4     | 6     | 2     | 3         |
| NRF24   | 5     | 4     | 6     | 8     | 1         |
| PN532   | 5     | 4     | 6     | 43    | --        |
| WS500   | 5     | 4     | 6     | **    | **        |
| LoRa    | 5     | 4     | 6     | **    | **        |
    */
    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH); // SD Card CS
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH); // CC1101 CS
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH); // nRF24L01 CS
    pinMode(43, OUTPUT);
    digitalWrite(43, HIGH); // PN532 CS
    pinMode(9, OUTPUT);
    digitalWrite(9, LOW); // M5RF433 avoid Jamming
    pinMode(46, OUTPUT);
    digitalWrite(46, LOW); // Infrared LED Off

    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
    attachInterrupt(DW_BTN, isr_dw_btn, CHANGE);
    pinMode(TFT_BL, OUTPUT);
    bruceConfig.colorInverted = 0;
}
/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    // PWM backlight setup
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 250);
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 250;
    else if (brightval == 75) dutyCycle = 130;
    else if (brightval == 50) dutyCycle = 70;
    else if (brightval == 25) dutyCycle = 20;
    else if (brightval == 0) dutyCycle = 5;
    else dutyCycle = ((brightval * 250) / 100);

    // Serial.printf("dutyCycle for bright 0-255: %d\n", dutyCycle);

    vTaskDelay(10 / portTICK_PERIOD_MS);
    if (!ledcWrite(TFT_BL, dutyCycle)) {
        // Serial.println("Failed to set brightness");
        ledcDetach(TFT_BL);
        ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
        ledcWrite(TFT_BL, dutyCycle);
    }
}

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
    static unsigned long tm = 0;
    static bool dwLongFired = false;
    unsigned long now = millis();
    if (now - tm < 200 && !LongPress) return;
    if (!wakeUpScreen()) AnyKeyPress = true;
    else return;

    bool selPressed = (digitalRead(SEL_BTN) == BTN_ACT);
    bool dwPressed = dw_is_down;
    bool dwWaiting = dw_waiting;
    bool dwDoubleReady = dw_double_ready;
    unsigned long dwPressStart = dw_press_ms;
    unsigned long dwFirstRelease = dw_first_release_ms;

    AnyKeyPress = selPressed || dwPressed || dwWaiting || dwDoubleReady;

    if (selPressed) {
        SelPress = true;
        tm = now;
    }
    if (dwPressed) {
        if (!dwLongFired && (now - dwPressStart) > kDwLongPressMs) {
            EscPress = true;
            dwLongFired = true;
            dw_waiting = false;
            dw_double_ready = false;
            dw_long_seen = true;
            tm = now;
        }
    } else if (dwLongFired) {
        dwLongFired = false;
    }

    if (dwDoubleReady) {
        PrevPress = true;
        dw_double_ready = false;
        dw_waiting = false;
        tm = now;
    } else if (dwWaiting && !dwPressed && (now - dwFirstRelease) > kDwDoublePressWindowMs) {
        NextPress = true;
        dw_waiting = false;
        tm = now;
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { M5.Power.powerOff(); }

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {}

bool isCharging() {
    // Strategy to stop buzzing
    static int lastState = -1;
    bool charging = M5.Power.isCharging();
    if (charging && lastState != 1) {
        lastState = 1;
        M5.Power.setExtOutput(false);
    } else if (!charging && lastState != 0) {
        lastState = 0;
        M5.Power.setExtOutput(true);
    }
    return charging;
}

/*********************************************************************
** Function: _setup_codec_speaker
** location: modules/others/audio.cpp
** Handles audio CODEC to enable/disable speaker
**********************************************************************/
void _setup_codec_speaker(bool enable) {
    M5.Speaker.setVolume(bruceConfig.soundVolume);
    if (enable) M5.Speaker.begin();
    else M5.Speaker.end();
}

/*********************************************************************
** Function: _setup_codec_mic
** location: modules/others/mic.cpp
** Handles audio CODEC to enable/disable microphone
**********************************************************************/
void _setup_codec_mic(bool enable) {
    if (enable) M5.Mic.begin();
    else M5.Mic.end();

    mic_bclk_pin = (gpio_num_t)17;
}
