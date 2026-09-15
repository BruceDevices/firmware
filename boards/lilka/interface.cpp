#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <Wire.h>
#include <globals.h>
#include <interface.h>

// Keep this app "unconfirmed" so it can be launched as a temporary/guest app
// from the Lilka keira launcher (on reboot the device rolls back to keira).
// Harmless for plain USB flashing (rollback isn't armed there).
extern "C" bool verifyRollbackLater() { return true; }

/***************************************************************************************
** Function: _setup_gpio()  — initial device setup (called from main.cpp)
***************************************************************************************/
void _setup_gpio() {
    // Buttons — Lilka has NO external pull-ups (verified on schematic + working ESPHome),
    // so use INPUT_PULLUP. All buttons are active LOW (wired straight to GND).
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
    pinMode(L_BTN, INPUT_PULLUP);
    pinMode(R_BTN, INPUT_PULLUP);
    pinMode(SEL_BTN, INPUT_PULLUP); // A
    pinMode(ESC_BTN, INPUT_PULLUP); // B

    // Keep radio module chip-selects idle (HIGH) at boot so they don't talk on the bus.
    // CC1101 and NRF24 share SS = 47 on Lilka.
    pinMode(CC1101_SS_PIN, OUTPUT);
    digitalWrite(CC1101_SS_PIN, HIGH);
    pinMode(NRF24_SS_PIN, OUTPUT);
    digitalWrite(NRF24_SS_PIN, HIGH);

    // Default external modules
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.irRx = RXLED;

    // Default I2C bus (extension header)
    Wire.setPins(GROVE_SDA, GROVE_SCL);
}

/***************************************************************************************
** Function: getBattery()  — battery percentage via ADC (no PMIC on Lilka)
**   GPIO3 (ADC1_CH2). Divider ratio 1.33 and 3.0-4.2V range taken from the
**   working Lilka ESPHome config (multiply: 1.33, attenuation 12db).
***************************************************************************************/
int getBattery() {
    static bool adcInit = false;
    if (!adcInit) {
        pinMode(ANALOG_BAT_PIN, INPUT);
        analogSetAttenuation(ADC_11db); // full 0..3.3V range (ESPHome 12db ~ Arduino 11db)
        adcInit = true;
    }

    uint32_t mv = analogReadMilliVolts(ANALOG_BAT_PIN);
    float voltage = (float)mv * 1.33f; // Lilka divider ratio (from working ESPHome, NOT x2)

    const float MIN_VOLTAGE = 3000.0f; // 3.0V ~0%  (LiPo)
    const float MAX_VOLTAGE = 4200.0f; // 4.2V ~100%

    int percent = (int)(((voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 100.0f);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}

/***************************************************************************************
** Function: isCharging()  — Lilka uses a bare TP4056, no charge-status GPIO
***************************************************************************************/
bool isCharging() { return false; }

/*********************************************************************
** Function: _setBrightness  — display backlight / power (TFT_BL = GPIO46)
**   Note: on Lilka GPIO46 is the display power/SLEEP line, so PWM acts
**   mostly as on/off rather than smooth dimming.
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

/*********************************************************************
** Function: InputHandler
** Sets PrevPress / NextPress / UpPress / DownPress / SelPress / EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool _u = digitalRead(UP_BTN);
    bool _d = digitalRead(DW_BTN);
    bool _l = digitalRead(L_BTN);
    bool _r = digitalRead(R_BTN);
    bool _s = digitalRead(SEL_BTN); // A
    bool _e = digitalRead(ESC_BTN); // B

    if (!_u || !_d || !_l || !_r || !_s || !_e) {
        tm = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }

    if (!_l) { PrevPress = true; }
    if (!_r) { NextPress = true; }
    if (!_u) {
        UpPress = true;
        PrevPagePress = true;
    }
    if (!_d) {
        DownPress = true;
        NextPagePress = true;
    }
    if (!_s) { SelPress = true; }
    if (!_e) { EscPress = true; }
}

/*********************************************************************
** Function: powerOff  — deep sleep, wake on Select (GPIO0)
**********************************************************************/
void powerOff() {
    analogWrite(TFT_BL, 0); // backlight/display off
    esp_sleep_enable_ext0_wakeup((gpio_num_t)DEEPSLEEP_WAKEUP_PIN, DEEPSLEEP_PIN_ACT);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot  — hold Left+Right ~2s to power off
**********************************************************************/
void checkReboot() {
    if (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
        uint32_t t0 = millis();
        while (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
            if (millis() - t0 > 2000) { powerOff(); }
            delay(10);
        }
    }
}
