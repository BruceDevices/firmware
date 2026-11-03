#include "core/display.h"
#include "core/powerSave.h"
#include <interface.h>

// QRP Labs LightTracker Plus v1.0 — board glue for Bruce
// Display: SSD1306 128x64 I2C OLED (LovyanGFX backend, configured in lighttracker-plus.ini)
// Input:   single BOOT button (GPIO0)
//            short press  (< 400 ms)       -> Next
//            medium press (400 - 1200 ms)  -> Select / OK
//            long press   (>= 1200 ms)     -> Back / Esc

#ifndef LT_BUTTON
#define LT_BUTTON 0
#endif
#ifndef LT_VBAT_ADC
#define LT_VBAT_ADC 1
#endif
#ifndef LT_LED
#define LT_LED 16
#endif

#define LT_SHORT_MS 400
#define LT_LONG_MS 1200

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // Power up the E22-400M30S LoRa module
#ifdef LORA_POWER_CTRL
    pinMode(LORA_POWER_CTRL, OUTPUT);
    digitalWrite(LORA_POWER_CTRL, HIGH);
#endif
    // RF switch: default to RX path (RadioLib switches it during TX)
#if defined(LORA_RXEN) && defined(LORA_TXEN)
    pinMode(LORA_RXEN, OUTPUT);
    pinMode(LORA_TXEN, OUTPUT);
    digitalWrite(LORA_RXEN, HIGH);
    digitalWrite(LORA_TXEN, LOW);
#endif
    // Keep LoRa CS high so it does not disturb the bus
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);

    // GPS power (PNP high-side switch: LOW = on)
#ifdef GPS_VCC_PIN
    pinMode(GPS_VCC_PIN, OUTPUT);
    digitalWrite(GPS_VCC_PIN, LOW);
#endif

    pinMode(LT_LED, OUTPUT);
    digitalWrite(LT_LED, LOW);

    pinMode(LT_BUTTON, INPUT_PULLUP);
    analogReadResolution(12);
    analogSetPinAttenuation(LT_VBAT_ADC, ADC_11db);
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
** Divider: 560k (high) + 100k (low)  -> Vbat = Vadc * 6.6
***************************************************************************************/
int getBattery() {
    uint32_t mv = 0;
    for (int i = 0; i < 8; i++) mv += analogReadMilliVolts(LT_VBAT_ADC);
    mv = (mv / 8) * 660 / 100;
    // Li-ion: 3.3V = 0%, 4.2V = 100%
    int percent = ((int)mv - 3300) * 100 / (4200 - 3300);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}

/***************************************************************************************
** Function name: isCharging()
***************************************************************************************/
bool isCharging() { return false; }

/*********************************************************************
** Function: setBrightness
** OLED: brightness is not adjustable through a backlight pin
**********************************************************************/
void _setBrightness(uint8_t brightval) {}

/*********************************************************************
** Function: InputHandler
** Single button, classified on release by press duration.
**********************************************************************/
void InputHandler(void) {
    static bool wasDown = false;
    static unsigned long downAt = 0;
    static bool longFired = false;

    bool down = digitalRead(LT_BUTTON) == LOW;
    unsigned long now = millis();

    if (down && !wasDown) { // pressed
        wasDown = true;
        downAt = now;
        longFired = false;
        digitalWrite(LT_LED, HIGH);
        return;
    }

    if (down && wasDown) { // held: fire Back as soon as the long threshold is reached
        if (!longFired && now - downAt >= LT_LONG_MS) {
            longFired = true;
            digitalWrite(LT_LED, LOW);
            if (!wakeUpScreen()) {
                AnyKeyPress = true;
                EscPress = true;
            }
        }
        return;
    }

    if (!down && wasDown) { // released
        wasDown = false;
        digitalWrite(LT_LED, LOW);
        unsigned long held = now - downAt;
        if (longFired || held < 30) return; // already handled / bounce
        if (wakeUpScreen()) return;
        AnyKeyPress = true;
        if (held < LT_SHORT_MS) NextPress = true;
        else SelPress = true;
    }
}

/*********************************************************************
** Function: powerOff
**********************************************************************/
void powerOff() {
#ifdef LORA_POWER_CTRL
    digitalWrite(LORA_POWER_CTRL, LOW);
#endif
#ifdef GPS_VCC_PIN
    digitalWrite(GPS_VCC_PIN, HIGH); // PNP: HIGH = off
#endif
    tft.sleep(true);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)LT_BUTTON, LOW);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
**********************************************************************/
void checkReboot() {}
