#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Wire.h>
#include <interface.h>
#include <Adafruit_SH110X.h>

// OLED display object
Adafruit_SH1106G display(128, 64, &Wire, -1); // 128x64, I2C, no reset pin

// Button debounce tracking
struct ButtonState {
    uint32_t lastPress;
    bool lastState;
};

static ButtonState btnStates[6] = {{0, HIGH}, {0, HIGH}, {0, HIGH}, {0, HIGH}, {0, HIGH}, {0, HIGH}};
static const uint8_t BTN_PINS[6] = {UP_BTN, DOWN_BTN, LEFT_BTN, RIGHT_BTN, SEL_BTN, BACK_BTN};
static const uint32_t DEBOUNCE_MS = 50;

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description: Initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // Initialize I2C for OLED display
    Wire.begin(SYS_I2C_SDA, SYS_I2C_SCL);
    Wire.setClock(400000); // 400kHz I2C clock

    // Initialize OLED display
    if (!display.begin(OLED_I2C_ADDR, true)) {
        Serial.println(F("SH110X OLED initialization failed"));
        while (1);
    }

    // Configure display
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(F("Bruce - OLED"));
    display.println(F("Initializing..."));
    display.display();

    // Initialize button pins
    for (uint8_t i = 0; i < 6; i++) {
        pinMode(BTN_PINS[i], INPUT_PULLUP);
        btnStates[i].lastState = HIGH;
        btnStates[i].lastPress = 0;
    }

    Serial.println(F("ESP32-S3 DevKit initialized"));
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description: Second stage GPIO setup
***************************************************************************************/
void _post_setup_gpio() {
    // Display is already initialized in _setup_gpio
    display.clearDisplay();
    display.display();
}

/***************************************************************************************
** Function name: _pre_storage_gpio()
** Location: main.cpp
** Description: Pre-storage setup (display initialization)
***************************************************************************************/
void _pre_storage_gpio() {
    // OLED already initialized
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description: Delivers the battery value from 1-100
** Note: Not implemented for this device
***************************************************************************************/
int getBattery() {
    return 100; // Return full battery (not monitored)
}

/*********************************************************************
** Function: _setBrightness()
** location: settings.cpp
** Description: Set display brightness value
** Note: SH1106 OLED doesn't support adjustable brightness
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    // OLED displays don't support brightness adjustment
    // This is a stub for compatibility
    (void)brightval;
}

/*********************************************************************
** Function: setBrightness()
** Called from main firmware
** Sets brightness if supported by device
**********************************************************************/
void setBrightness(uint8_t brightval, bool persist = true) {
    _setBrightness(brightval);
}

/*********************************************************************
** Function: readButton()
** Helper function to read button with debouncing
** Returns true if button is pressed and debounce time has passed
**********************************************************************/
bool readButton(uint8_t btnIndex, uint32_t currentTime) {
    bool currentState = digitalRead(BTN_PINS[btnIndex]);

    // Check if button state changed and debounce time has passed
    if (currentState != btnStates[btnIndex].lastState) {
        btnStates[btnIndex].lastState = currentState;
        btnStates[btnIndex].lastPress = currentTime;
        return false; // Debouncing
    }

    // Button is pressed (LOW) and debounce time has passed
    if (currentState == LOW && (currentTime - btnStates[btnIndex].lastPress) >= DEBOUNCE_MS) {
        btnStates[btnIndex].lastPress = currentTime; // Prevent rapid repeats
        return true;
    }

    return false;
}

/*********************************************************************
** Function: InputHandler()
** Handles button inputs and maps them to navigation flags
** UpPress, DownPress, SelPress, EscPress, etc.
**********************************************************************/
void InputHandler(void) {
    static uint32_t lastCheckTime = 0;
    uint32_t currentTime = millis();

    // Check buttons every 10ms to avoid excessive polling
    if (currentTime - lastCheckTime < 10) {
        return;
    }
    lastCheckTime = currentTime;

    // Read UP button (GPIO 0)
    if (readButton(0, currentTime)) {
        UpPress = true;
        AnyKeyPress = true;
        if (!wakeUpScreen()) return;
    }

    // Read DOWN button (GPIO 1)
    if (readButton(1, currentTime)) {
        DownPress = true;
        AnyKeyPress = true;
        if (!wakeUpScreen()) return;
    }

    // Read LEFT button (GPIO 2) -> maps to PrevPress
    if (readButton(2, currentTime)) {
        PrevPress = true;
        AnyKeyPress = true;
        if (!wakeUpScreen()) return;
    }

    // Read RIGHT button (GPIO 4) -> maps to NextPress
    if (readButton(3, currentTime)) {
        NextPress = true;
        AnyKeyPress = true;
        if (!wakeUpScreen()) return;
    }

    // Read SELECT button (GPIO 5)
    if (readButton(4, currentTime)) {
        SelPress = true;
        AnyKeyPress = true;
        if (!wakeUpScreen()) return;
    }

    // Read BACK button (GPIO 6) -> maps to EscPress
    if (readButton(5, currentTime)) {
        EscPress = true;
        AnyKeyPress = true;
        if (!wakeUpScreen()) return;
    }
}

/*********************************************************************
** Function: powerOff()
** location: mykeyboard.cpp
** Turns off the device
**********************************************************************/
void powerOff() {
    // Display shutdown
    display.clearDisplay();
    display.println(F("Powering off..."));
    display.display();
    delay(1000);
    display.ssd1306_command(SH110X_DISPLAYOFF);

    // Deep sleep
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot()
** location: mykeyboard.cpp
** Btn logic to reboot device
**********************************************************************/
void checkReboot() {
    // Could implement power button long-press detection here
}
