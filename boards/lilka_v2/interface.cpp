/**
 * @file interface.cpp
 * @brief Lilka V2 hardware abstraction layer for Bruce firmware
 *
 * Implements the Bruce interface.h for the Lilka V2 ESP32-S3 handheld.
 * Hardware: https://github.com/and3rson/lilka
 */

#include "core/powerSave.h"
#include <Arduino.h>
#include <algorithm>
#include <array>
#include <esp_sleep.h>
#include <interface.h>

// =============================================================================
//  GPIO Setup
// =============================================================================

void _setup_gpio() {
    // Power control - MUST be done first!
    // GPIO 45 enables display power (discovered from Rustilka firmware)
    pinMode(DISPLAY_EN_PIN, OUTPUT);
    digitalWrite(DISPLAY_EN_PIN, HIGH);

    // GPIO 46 controls sleep state
    pinMode(SLEEP_PIN, OUTPUT);
    digitalWrite(SLEEP_PIN, HIGH);

    // Allow power rails to stabilize
    delay(100);

    // Prevent SPI bus conflicts during init
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    // Navigation buttons (D-pad + select)
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
    pinMode(L_BTN, INPUT_PULLUP);
    pinMode(R_BTN, INPUT_PULLUP);

    // Action buttons
    pinMode(START_BTN, INPUT_PULLUP);
    pinMode(A_BTN, INPUT_PULLUP);
    pinMode(B_BTN, INPUT_PULLUP);
    pinMode(C_BTN, INPUT_PULLUP);
    pinMode(D_BTN, INPUT_PULLUP);

    // Peripherals
    pinMode(BAT_PIN, INPUT);

#ifdef BUZZ_PIN
    pinMode(BUZZ_PIN, OUTPUT);
    digitalWrite(BUZZ_PIN, LOW);
#endif
}

// =============================================================================
//  Post-Setup (called after TFT_eSPI init)
// =============================================================================
void _post_setup_gpio() {
    // Reserved for display orientation fixes if needed
}

// =============================================================================
//  Battery
// =============================================================================

int getBattery() {
    // Multi-sample median filter for stable readings
    constexpr int samples = 16;
    std::array<uint16_t, samples> values{};

    for (int i = 0; i < samples; i++) { values[i] = analogRead(BAT_PIN); }

    std::nth_element(values.begin(), values.begin() + samples / 2, values.end());
    uint16_t rawValue = values[samples / 2];

    // Voltage divider: 100K/(33K+100K) = 0.752
    // Battery voltage = (raw / 4095) * 3.1V / 0.752
    float voltage = (float)rawValue * 4.123f / 4095.0f;

    // No battery connected?
    if (voltage < 0.5f) return -1;

    // Map 3.2V-4.1V to 0-100%
    float emptyVoltage = 3.2f;
    float fullVoltage = 4.1f;
    int percent = (int)((voltage - emptyVoltage) / (fullVoltage - emptyVoltage) * 100.0f);

    return constrain(percent, 0, 100);
}

bool isCharging() {
    return false; // No charging detection on Lilka V2
}

// =============================================================================
//  Display Brightness
// =============================================================================

void _setBrightness(uint8_t brightval) {
    // Lilka V2 has no PWM backlight control
    // Display is always on when DISPLAY_EN_PIN is HIGH
}

// =============================================================================
//  Input Handler
// =============================================================================

void InputHandler(void) {
    static unsigned long lastPress = 0;

    // Debounce (200ms unless long-press mode)
    if (millis() - lastPress < 200 && !LongPress) return;

    // Read all buttons (active LOW)
    bool up = digitalRead(UP_BTN);
    bool down = digitalRead(DW_BTN);
    bool left = digitalRead(L_BTN);
    bool right = digitalRead(R_BTN);
    bool sel = digitalRead(SEL_BTN);
    bool a = digitalRead(A_BTN);
    bool b = digitalRead(B_BTN);
    bool start = digitalRead(START_BTN);
    bool c = digitalRead(C_BTN);

    // Any button pressed?
    if (!sel || !up || !down || !right || !left || !a || !b || !start || !c) {
        lastPress = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }

    // D-pad navigation
    if (!left) PrevPress = true;
    if (!right) NextPress = true;
    if (!up) UpPress = true;
    if (!down) DownPress = true;

    // A/SELECT = confirm, B/START/C = escape
    if (!sel || !a) SelPress = true;
    if (!b || !start || !c) EscPress = true;

    // L+R combo = escape (quick exit)
    if (!left && !right) {
        EscPress = true;
        NextPress = false;
        PrevPress = false;
    }
}

// =============================================================================
//  Power Management
// =============================================================================

void powerOff() {
    // Disable display
    digitalWrite(SLEEP_PIN, LOW);

    // Deep sleep with SELECT button wake
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, LOW);
    esp_deep_sleep_start();
}

void checkReboot() {
    // Could implement hold-to-reboot if needed
}
