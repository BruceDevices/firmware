#include "core/powerSave.h"
#include "core/utils.h"
#include <Wire.h>
#include <interface.h>

// Touch controller CST328
#define TOUCH_MODULES_CST_SELF
#include <TouchDrvCSTXXX.hpp>
TouchDrvCSTXXX touch;

// Keyboard controller TCA8418
#include <Adafruit_TCA8418.h>
Adafruit_TCA8418 *keyboard;

// Pin definitions for T-Deck Pro E-Paper
#define BOARD_I2C_SDA 13
#define BOARD_I2C_SCL 14
#define BOARD_TOUCH_INT 12
#define BOARD_TOUCH_RST 45
#define BOARD_KEYBOARD_INT 15
#define BOARD_KEYBOARD_LED 42
#define PIN_POWER_ON 10
#define SEL_BTN 0  // Placeholder for function key

// I2C addresses
#define CST328_SLAVE_ADDRESS 0x1A
#define TCA8418_KEYBOARD_ADDRESS 0x34
#define BQ25896_CHARGER_ADDRESS 0x6B
#define BQ27220_FUEL_GAUGE_ADDRESS 0x55

// E-Paper display pins
#define EPD_BUSY 37
#define EPD_DC 35
#define EPD_CS 34

struct TouchPoint {
    int16_t x = 0;
    int16_t y = 0;
};

// Keyboard state variables
bool fn_key_pressed = false;
bool shift_key_pressed = false;
bool caps_lock = false;

// T-Deck Pro keyboard matrix: 4 rows x 10 columns (40 keys)
// Based on standard QWERTY layout similar to T-LoRa Pager
#define KB_ROWS 4
#define KB_COLS 10

struct KeyValue_t {
    const char value_first;   // Normal key
    const char value_second;  // Shift key
    const char value_third;   // Fn key
};

// T-Deck Pro QWERTY Keymap
// Row 0: Q W E R T Y U I O P
// Row 1: A S D F G H J K L Enter
// Row 2: Fn Z X C V B N M Shift Backspace
// Row 3: Space (spanning multiple columns)
const KeyValue_t _key_value_map[KB_ROWS][KB_COLS] = {
    // Row 0: Top row - QWERTYUIOP
    {{'q', 'Q', '1'},
     {'w', 'W', '2'},
     {'e', 'E', '3'},
     {'r', 'R', '4'},
     {'t', 'T', '5'},
     {'y', 'Y', '6'},
     {'u', 'U', '7'},
     {'i', 'I', '8'},
     {'o', 'O', '9'},
     {'p', 'P', '0'}},

    // Row 1: Middle row - ASDFGHJKL + Enter
    {{'a', 'A', '!'},
     {'s', 'S', '@'},
     {'d', 'D', '#'},
     {'f', 'F', '$'},
     {'g', 'G', '%'},
     {'h', 'H', '^'},
     {'j', 'J', '&'},
     {'k', 'K', '*'},
     {'l', 'L', '('},
     {KEY_ENTER, KEY_ENTER, ')'}},

    // Row 2: Bottom letter row - Fn + ZXCVBNM + Shift + Backspace
    {{KEY_FN, KEY_FN, KEY_FN},
     {'z', 'Z', '-'},
     {'x', 'X', '_'},
     {'c', 'C', '='},
     {'v', 'V', '+'},
     {'b', 'B', '['},
     {'n', 'N', ']'},
     {'m', 'M', ';'},
     {KEY_SHIFT, KEY_SHIFT, CAPS_LOCK},
     {KEY_BACKSPACE, KEY_BACKSPACE, KEY_BACKSPACE}},

    // Row 3: Space bar
    {{' ', ' ', ' '},
     {' ', ' ', ' '},
     {' ', ' ', ' '},
     {' ', ' ', ' '},
     {' ', ' ', ' '},
     {' ', ' ', ' '},
     {' ', ' ', ' '},
     {',', '<', ','},
     {'.', '>', '.'},
     {'/', '?', '/'}}
};

/***************************************************************************************
** Function name: getKeyChar
** Description:   Get character based on modifier keys
***************************************************************************************/
char getKeyChar(uint8_t k) {
    if (k >= KB_ROWS * KB_COLS) return '\0';

    char keyVal;
    uint8_t row = k / KB_COLS;
    uint8_t col = k % KB_COLS;

    if (fn_key_pressed) {
        keyVal = _key_value_map[row][col].value_third;
    } else if (shift_key_pressed ^ caps_lock) {
        keyVal = _key_value_map[row][col].value_second;
    } else {
        keyVal = _key_value_map[row][col].value_first;
    }
    return keyVal;
}

/***************************************************************************************
** Function name: handleSpecialKeys
** Description:   Handle Fn, Shift, and Caps Lock keys
** Returns:       1 if special key handled, 0 otherwise
***************************************************************************************/
int handleSpecialKeys(uint8_t k, bool pressed) {
    if (k >= KB_ROWS * KB_COLS) return 0;

    char keyVal = _key_value_map[k / KB_COLS][k % KB_COLS].value_first;

    switch (keyVal) {
        case KEY_FN:
            fn_key_pressed = pressed;
            return 1;

        case KEY_SHIFT:
            shift_key_pressed = pressed;
            // Fn + Shift = Caps Lock toggle
            if (fn_key_pressed && shift_key_pressed) {
                caps_lock = !caps_lock;
            }
            return 1;

        default:
            break;
    }
    return 0;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   Initial setup for the T-Deck Pro E-Paper
***************************************************************************************/
void _setup_gpio() {
    // Power control
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);

    // Initialize I2C bus
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    delay(100);

    // Initialize Touch Controller CST328
    pinMode(BOARD_TOUCH_INT, INPUT);
    pinMode(BOARD_TOUCH_RST, OUTPUT);
    digitalWrite(BOARD_TOUCH_RST, LOW);
    delay(100);
    digitalWrite(BOARD_TOUCH_RST, HIGH);
    delay(200);

    touch.setPins(BOARD_TOUCH_RST, BOARD_TOUCH_INT);
    if (touch.begin(Wire, CST328_SLAVE_ADDRESS, BOARD_I2C_SDA, BOARD_I2C_SCL)) {
        Serial.println("CST328 Touch initialized");
        // Set coordinates for 320x240 E-Paper in landscape (rotation 1)
        touch.setMaxCoordinates(320, 240);
        touch.setSwapXY(false);
        touch.setMirrorXY(false, false);
    } else {
        Serial.println("Failed to initialize CST328 Touch");
    }

    // Initialize TCA8418 Keyboard
    pinMode(BOARD_KEYBOARD_INT, INPUT_PULLUP);
    pinMode(BOARD_KEYBOARD_LED, OUTPUT);
    digitalWrite(BOARD_KEYBOARD_LED, LOW);

    keyboard = new Adafruit_TCA8418();
    if (!keyboard->begin(TCA8418_KEYBOARD_ADDRESS, &Wire)) {
        Serial.println("Failed to find TCA8418 Keyboard");
    } else {
        Serial.println("TCA8418 Keyboard initialized successfully");
        // Configure keyboard matrix: 4 rows, 10 columns
        keyboard->matrix(KB_ROWS, KB_COLS);
        // Clear any pending events
        keyboard->flush();
    }

    // Initialize placeholder button
    pinMode(SEL_BTN, INPUT_PULLUP);

    // Set LoRa CS pin HIGH (disable LoRa on shared SPI)
    pinMode(3, OUTPUT);
    digitalWrite(3, HIGH);

    // Set SD Card CS pin HIGH (disable SD on shared SPI)
    pinMode(48, OUTPUT);
    digitalWrite(48, HIGH);

    // E-Paper BUSY pin input
    pinMode(EPD_BUSY, INPUT);

    // GPS setup
    bruceConfigPins.gpsBaudrate = 9600; // MIA-M10Q default baud rate

    Serial.println("T-Deck Pro E-Paper GPIO setup complete");
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   Second stage gpio setup
***************************************************************************************/
void _post_setup_gpio() {
    // E-Paper has no backlight, so no PWM setup needed
    // Turn on keyboard LED for status indication
    digitalWrite(BOARD_KEYBOARD_LED, HIGH);
    delay(500);
    digitalWrite(BOARD_KEYBOARD_LED, LOW);
}

/***************************************************************************************
** Function name: _setBrightness
** Location: settings.cpp
** Description:   Set brightness (controls keyboard LED for E-Paper)
***************************************************************************************/
void _setBrightness(uint8_t brightval) {
    // E-Paper displays have no backlight
    // Use keyboard LED to indicate brightness setting
    if (brightval > 0) {
        analogWrite(BOARD_KEYBOARD_LED, map(brightval, 0, 100, 0, 255));
    } else {
        digitalWrite(BOARD_KEYBOARD_LED, LOW);
    }
}

/***************************************************************************************
** Function name: getBattery
** Location: display.cpp
** Description:   Get battery percentage from ADC
***************************************************************************************/
int getBattery() {
    // TODO: Implement BQ27220 fuel gauge reading for accurate percentage
    // For now, use simple ADC reading from battery pin
    int adcValue = analogRead(4); // ANALOG_BAT_PIN

    // Simple voltage divider calculation
    // Adjust multiplier based on actual voltage divider ratio
    float voltage = (adcValue / 4095.0) * 3.3 * 2.0;

    // LiPo voltage range: 3.3V (0%) to 4.2V (100%)
    int percent = ((voltage - 3.3) / (4.2 - 3.3)) * 100;

    return (percent < 0) ? 1 : (percent > 100) ? 100 : percent;
}

/***************************************************************************************
** Function name: isCharging
** Location: interface.h
** Description:   Check if device is charging
***************************************************************************************/
bool isCharging() {
    // TODO: Implement BQ25896 charger status reading
    // For now, return false
    return false;
}

/***************************************************************************************
** Function name: InputHandler
** Description:   Handle input from touch screen and keyboard
***************************************************************************************/
void InputHandler(void) {
    static unsigned long lastInputTime = millis();
    static unsigned long lastKeyTime = millis();
    TouchPoint t;
    uint8_t touched = 0;

    // Check keyboard input first (higher priority)
    if (keyboard->available() > 0) {
        int keyEvent = keyboard->getEvent();
        bool pressed = keyEvent & 0x80;  // Bit 7: 1=pressed, 0=released
        uint8_t keyValue = keyEvent & 0x7F;  // Bits 0-6: key position
        keyValue--;  // Adjust for 0-based indexing

        if (keyValue < KB_ROWS * KB_COLS) {
            // Handle special keys (Fn, Shift, Caps Lock)
            if (handleSpecialKeys(keyValue, pressed) > 0) {
                lastKeyTime = millis();
                return;
            }

            // Get character for this key
            char keyChar = getKeyChar(keyValue);

            if (pressed && keyChar != '\0') {
                if (!wakeUpScreen()) {
                    AnyKeyPress = true;
                    KeyStroke.Clear();
                    KeyStroke.hid_keys.push_back(keyChar);

                    // Handle special characters
                    if (keyChar == KEY_BACKSPACE) {
                        KeyStroke.del = true;
                        EscPress = true;
                    }
                    if (keyChar == KEY_ENTER) {
                        KeyStroke.enter = true;
                        SelPress = true;
                    }
                    if (keyChar == KEY_FN) {
                        KeyStroke.fn = true;
                    }

                    KeyStroke.word.push_back(keyChar);
                    KeyStroke.pressed = true;

                    Serial.printf("Key: '%c' (0x%02X) at pos %d\n", keyChar, keyChar, keyValue);

                    // Brief LED blink for feedback
                    digitalWrite(BOARD_KEYBOARD_LED, HIGH);
                    delay(20);
                    digitalWrite(BOARD_KEYBOARD_LED, LOW);
                }
                lastKeyTime = millis();
                return;
            }
        }
    } else {
        // Clear keystroke if no keys in buffer
        if (millis() - lastKeyTime > 100) {
            KeyStroke.pressed = false;
        }
    }

    // Check touch input (lower priority, only if no keyboard input)
    if (millis() - lastInputTime > 100) { // Debounce 100ms
        if (touch.isPressed()) {
            touched = touch.getPoint(&t.x, &t.y);

            if (touched) {
                lastInputTime = millis();

                if (!wakeUpScreen()) {
                    AnyKeyPress = true;
                    touchPoint.x = t.x;
                    touchPoint.y = t.y;
                    touchPoint.pressed = true;
                    touchHeatMap(touchPoint);

                    Serial.printf("Touch: x=%d, y=%d\n", t.x, t.y);
                } else {
                    return; // Screen was asleep, just wake it
                }
            }
        }
    }

    // Check function button (if available)
    if (digitalRead(SEL_BTN) == LOW) {
        if (millis() - lastInputTime > 200) {
            lastInputTime = millis();
            if (!wakeUpScreen()) {
                AnyKeyPress = true;
                SelPress = true;
                Serial.println("Function button pressed");
            } else {
                return;
            }
        }
    }
}

/***************************************************************************************
** Function name: powerOff
** Location: mykeyboard.cpp
** Description:   Power off the device
***************************************************************************************/
void powerOff() {
    // Turn off keyboard LED
    digitalWrite(BOARD_KEYBOARD_LED, LOW);

    // Set power pin low to turn off
    digitalWrite(PIN_POWER_ON, LOW);
    delay(1000);

    // If still running, enter deep sleep
    esp_deep_sleep_start();
}

/***************************************************************************************
** Function name: checkReboot
** Location: mykeyboard.cpp
** Description:   Check for reboot condition
***************************************************************************************/
void checkReboot() {
    // Could implement long-press detection here for reboot/reset
    // For now, no-op
}
