#include "core/powerSave.h"
#include "core/utils.h"
#include <Adafruit_TCA8418.h>
#include <Keyboard.h>
#include <Wire.h>
#include <interface.h>

// Cardputer and 1.1 keyboard
Keyboard_Class Keyboard;
// TCA8418 keyboard controller for ADV variant
Adafruit_TCA8418 tca;
bool UseTCA8418 = false; // Set to true to use TCA8418 (Cardputer ADV)

// Keyboard state variables
bool fn_key_pressed = false;
bool shift_key_pressed = false;
bool caps_lock = false;

int handleSpecialKeys(uint8_t row, uint8_t col, bool pressed);
void mapRawKeyToPhysical(uint8_t rawValue, uint8_t &row, uint8_t &col);

char getKeyChar(uint8_t row, uint8_t col) {
    char keyVal;
    if (shift_key_pressed ^ caps_lock) {
        keyVal = _key_value_map[row][col].value_second;
    } else {
        keyVal = _key_value_map[row][col].value_first;
    }
    return keyVal;
}

int handleSpecialKeys(uint8_t row, uint8_t col, bool pressed) {
    char keyVal = _key_value_map[row][col].value_first;
    switch (keyVal) {
        case 0xFF:
            fn_key_pressed = pressed;
            if (fn_key_pressed) Serial.println("FN Pressed");
            else Serial.println("FN Released");
            return 1;
        case 0x81:
            shift_key_pressed = pressed;
            if (shift_key_pressed) Serial.println("Shift Pressed");
            else Serial.println("Shift Released");
            if (shift_key_pressed && fn_key_pressed) {
                caps_lock = !caps_lock;
                if (caps_lock) Serial.println("CAPS Lock activated");
                else Serial.println("CAPS Lock DEactivated");
                shift_key_pressed = false;
                fn_key_pressed = false;
            }
            return 1;
        default: break;
    }
    return 0;
}

/***************************************************************************************
** Function name: mapRawKeyToPhysical()
** Location: interface.cpp
** Description:   initial mapping for keyboard
***************************************************************************************/
inline void mapRawKeyToPhysical(uint8_t keyvalue, uint8_t &row, uint8_t &col) {
    const uint8_t u = keyvalue % 10; // 1..8
    const uint8_t t = keyvalue / 10; // 0..6

    if (u >= 1 && u <= 8 && t <= 6) {
        const uint8_t u0 = u - 1;   // 0..7
        row = u0 & 0x03;            // bits [1:0] => 0..3
        col = (t << 1) | (u0 >> 2); // t*2 + bit2(u0) => 0..13
    } else {
        row = 0xFF; // invalid
        col = 0xFF;
    }
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    //    Keyboard.begin();
    pinMode(0, INPUT);
    pinMode(5, OUTPUT);
    // Set GPIO5 HIGH for SD card compatibility (thx for the tip @bmorcelli & 7h30th3r0n3)
    digitalWrite(5, HIGH);
}
bool kb_interrupt = false;
void IRAM_ATTR gpio_isr_handler(void *arg) {
    kb_interrupt = true;
    // static long i = 0;
    // Serial.printf("interrupt %ld\n", i++);
}
void _post_setup_gpio() {
    // Initialize TCA8418 I2C keyboard controller
    Serial.println("DEBUG: Cardputer ADV - Initializing TCA8418 keyboard");

    // Use correct I2C pins for Cardputer ADV
    Serial.printf("DEBUG: Initializing I2C with SDA=%d, SCL=%d\n", TCA8418_SDA_PIN, TCA8418_SCL_PIN);
    Wire1.begin(TCA8418_SDA_PIN, TCA8418_SCL_PIN);
    delay(100);

    // Scan I2C bus to see what's available
    Serial.println("DEBUG: Scanning I2C bus...");
    byte found_devices = 0;
    for (byte i = 1; i < 127; i++) {
        Wire1.beginTransmission(i);
        if (Wire1.endTransmission() == 0) {
            Serial.printf("DEBUG: Found I2C device at address 0x%02X\n", i);
            found_devices++;
        }
    }
    Serial.printf("DEBUG: Found %d I2C devices\n", found_devices);

    // Try to initialize TCA8418
    Serial.printf("DEBUG: Attempting to initialize TCA8418 at address 0x%02X\n", TCA8418_I2C_ADDR);
    UseTCA8418 = tca.begin(TCA8418_I2C_ADDR, &Wire1);

    if (!UseTCA8418) {
        Serial.println("ADV  : Failed to initialize TCA8418!");
        Serial.println("Probable standard Cardputer detected, switching to Keyboard library");
        Wire1.end();
        Keyboard.begin();
        return;
    }
    bruceConfigPins.gps_bus.rx = (gpio_num_t)15;
    bruceConfigPins.gps_bus.tx = (gpio_num_t)13;
    bruceConfigPins.gpsBaudrate = 115200;

    tca.matrix(7, 8);
    tca.flush();
    pinMode(11, INPUT);
    attachInterruptArg(digitalPinToInterrupt(11), gpio_isr_handler, &kb_interrupt, CHANGE);
    tca.enableInterrupts();
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
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
    static unsigned long tm = 0;

    static bool sel = false;
    static bool prev = false;
    static bool next = false;
    static bool up = false;
    static bool down = false;
    static bool esc = false;
    static bool del = false;
    static bool gui = false;
    static bool alt = false;
    static bool ctrl = false;

    bool arrow_up = false;
    bool arrow_dw = false;
    bool arrow_ry = false;
    bool arrow_le = false;
    if (millis() - tm < 200 && !LongPress) return;

    if (digitalRead(0) == LOW) { // GPIO0 button, shoulder button
        tm = millis();
        if (!wakeUpScreen()) yield();
        else return;
        SelPress = true;
        AnyKeyPress = true;
    }

    if (UseTCA8418) {
        if (!kb_interrupt) {
            if (digitalRead(11) == LOW) {
                detachInterrupt(digitalPinToInterrupt(11));
                attachInterruptArg(digitalPinToInterrupt(11), gpio_isr_handler, &kb_interrupt, CHANGE);
                Serial.println("Forcing keyboard interrupt, Restoring Interruptions.");
                kb_interrupt = true;
            }
            if (!LongPress) {
                sel = false; // avoid multiple selections
                esc = false; // avoid multiple escapes
            }
            NextPress = next;
            PrevPress = prev;
            UpPress = up;
            DownPress = down;
            if (!SelPress) SelPress = sel;
            EscPress = esc;
            if (del) {
                KeyStroke.del = del;
                KeyStroke.pressed = true;
            }
            tm = millis();
            return;
        }

        //  try to clear the IRQ flag
        //  if there are pending events it is not cleared
        tca.writeRegister(TCA8418_REG_INT_STAT, 1);
        int intstat = tca.readRegister(TCA8418_REG_INT_STAT);
        if ((intstat & 0x01) == 0) { kb_interrupt = false; }

        // if (tca.available() <= 0) return;
        int keyEvent = tca.getEvent();
        bool pressed = (keyEvent & 0x80); // Bit 7: 1 Pressed, 0 Released
        uint8_t value = keyEvent & 0x7F;  // Bits 0-6: key value

        // Map raw value to physical position
        uint8_t row, col;
        mapRawKeyToPhysical(value, row, col);

        // Serial.printf("Key event: raw=%d, pressed=%d, row=%d, col=%d\n", value, pressed, row, col);

        if (row >= 4 || col >= 14) return;

        if (wakeUpScreen()) return;

        AnyKeyPress = true;

        if (handleSpecialKeys(row, col, pressed) > 0) return;

        if (!pressed) { KeyStroke.Clear(); }

        keyStroke key;
        char keyVal = getKeyChar(row, col);

        // Serial.printf("Key pressed: %c (0x%02X) at row=%d, col=%d\n", keyVal, keyVal, row, col);

        if (keyVal == KEY_BACKSPACE && col == 13) { // KEY_BACKSPACE = '*' = 0x2a
            del = pressed;
            esc = pressed;
        } else if (keyVal == '`') {
            esc = pressed;
        } else if (keyVal == KEY_ENTER && col == 13) { // KEY_ENTER = '(' = 0x28
            sel = pressed;
        } else if (keyVal == ',' || keyVal == ';') {
            prev = pressed;
            if (keyVal == ',') arrow_le = pressed;
            if (keyVal == ';') arrow_up = pressed;
        } else if (keyVal == '/' || keyVal == '.') {
            next = pressed;
            if (keyVal == '/') arrow_ry = pressed;
            if (keyVal == '.') arrow_dw = pressed;
        } else if (keyVal == 0xFF) {
            key.fn = pressed;
        } else if (keyVal == KEY_LEFT_CTRL) {
            ctrl = pressed;
        } else if (keyVal == KEY_LEFT_ALT) {
            alt = pressed;
        } else if (keyVal == KEY_OPT) {
            gui = pressed;
        }

        if (gui) {
            key.gui = true;
            key.modifier_keys.emplace_back(KEY_OPT);
            key.hid_keys.emplace_back(KEY_OPT);
        }
        if (alt) {
            key.alt = true;
            key.modifier_keys.emplace_back(KEY_LEFT_ALT);
            key.hid_keys.emplace_back(KEY_LEFT_ALT);
        }
        if (ctrl) {
            key.ctrl = true;
            key.modifier_keys.emplace_back(KEY_LEFT_CTRL);
            key.hid_keys.emplace_back(KEY_LEFT_CTRL);
        }
        if (shift_key_pressed) {
            key.fn = true;
            key.modifier_keys.emplace_back(KEY_LEFT_SHIFT);
            key.hid_keys.emplace_back(KEY_LEFT_SHIFT);
        }
        if (sel) {
            key.enter = true;
            key.exit_key = true;
        }
        if (fn_key_pressed) key.fn = true;

        if (keyVal != 0xFF && !sel && !gui && !alt && !ctrl && !del && keyVal != KEY_LEFT_SHIFT) {
            if (fn_key_pressed && arrow_up) key.word.emplace_back(0xDA);
            else if (fn_key_pressed && arrow_dw) key.word.emplace_back(0xD9);
            else if (fn_key_pressed && arrow_ry) key.word.emplace_back(0xD7);
            else if (fn_key_pressed && arrow_le) key.word.emplace_back(0xD8);
            else if (fn_key_pressed && keyVal == '`') key.word.emplace_back(0xB1);
            else key.word.emplace_back(keyVal);
        }
        key.pressed = pressed;
        if (del) {
            key.del = del;
            key.pressed = true;
        }
        if (fn_key_pressed && del) {
            key.word.emplace_back(0xD4);
            key.del = false;
            key.fn = false;
            del = false;
            fn_key_pressed = false;
        }
        KeyStroke = key;
        NextPress = next;
        PrevPress = prev;
        UpPress = up;
        DownPress = down;
        SelPress = sel;
        EscPress = esc;
        tm = millis();
    } else {
        Keyboard.update();
        if (Keyboard.isPressed()) {
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;
            keyStroke key;
            Keyboard_Class::KeysState status = Keyboard.keysState();
            for (auto i : status.hid_keys) key.hid_keys.emplace_back(i);
            for (auto i : status.word) {
                if (i == '`' || i == KEY_BACKSPACE) EscPress = true;

                if (i == ';') {
                    arrow_up = true;
                    PrevPress = true;
                }
                if (i == '.') {
                    arrow_dw = true;
                    NextPress = true;
                }
                if (i == '/') {
                    arrow_ry = true;
                    NextPress = true;
                    NextPagePress = true;
                }
                if (i == ',') {
                    arrow_le = true;
                    PrevPress = true;
                    PrevPagePress = true;
                }
                if (status.fn && arrow_up) key.word.emplace_back(0xDA);
                else if (status.fn && arrow_dw) key.word.emplace_back(0xD9);
                else if (status.fn && arrow_ry) key.word.emplace_back(0xD7);
                else if (status.fn && arrow_le) key.word.emplace_back(0xD8);
                else if (status.fn && i == '`') key.word.emplace_back(0xB1);
                else key.word.emplace_back(i);
            }
            // Add CTRL, ALT and Tab to keytroke without modifier
            key.alt = status.alt;
            key.ctrl = status.ctrl;
            key.gui = status.opt;
            // Add Tab key
            if (status.tab) key.word.emplace_back(0xB3);

            for (auto i : status.modifier_keys) key.modifier_keys.emplace_back(i);
            if (status.del) key.del = true;
            if (status.enter) {
                key.enter = true;
                key.exit_key = true;
                SelPress = true;
            }
            if (status.fn) key.fn = true;
            if (key.fn && key.del) {
                key.word.emplace_back(0xD4);
                key.del = false;
                key.fn = false;
            }
            key.pressed = true;
            KeyStroke = key;
        } else KeyStroke.Clear();
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {}

/*********************************************************************
** Function: _setup_codec_speaker
** location: modules/others/audio.cpp
** Handles audio CODEC to enable/disable speaker
**********************************************************************/
void _setup_codec_speaker(bool enable) {
    if (!UseTCA8418) return;

    static constexpr const uint8_t enabled_bulk_data[] = {
        2, 0x00, 0x80, // 0x00 RESET/  CSM POWER ON
        2, 0x01, 0xB5, // 0x01 CLOCK_MANAGER/ MCLK=BCLK
        2, 0x02, 0x18, // 0x02 CLOCK_MANAGER/ MULT_PRE=3
        2, 0x0D, 0x01, // 0x0D SYSTEM/ Power up analog circuitry
        2, 0x12, 0x00, // 0x12 SYSTEM/ power-up DAC - NOT default
        2, 0x13, 0x10, // 0x13 SYSTEM/ Enable output to HP drive - NOT default
        2, 0x32, 0xBF, // 0x32 DAC/ DAC volume (0xBF == ±0 dB )
        2, 0x37, 0x08, // 0x37 DAC/ Bypass DAC equalizer - NOT default
        0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {0};

    i2c_bulk_write(&Wire1, ES8311_ADDR, enable ? enabled_bulk_data : disabled_bulk_data);
}

/*********************************************************************
** Function: _setup_codec_mic
** location: modules/others/mic.cpp
** Handles audio CODEC to enable/disable microphone
**********************************************************************/
void _setup_codec_mic(bool enable) {
    if (!UseTCA8418) return;
    // Set microfone pin for ADV
    mic_bclk_pin = (gpio_num_t)41;

    static constexpr const uint8_t enabled_bulk_data[] = {
        2, 0x00, 0x80, // 0x00 RESET/  CSM POWER ON
        2, 0x01, 0xBA, // 0x01 CLOCK_MANAGER/ MCLK=BCLK
        2, 0x02, 0x18, // 0x02 CLOCK_MANAGER/ MULT_PRE=3
        2, 0x0D, 0x01, // 0x0D SYSTEM/ Power up analog circuitry
        2, 0x0E, 0x02, // 0x0E SYSTEM/ : Enable analog PGA, enable ADC modulator
        2, 0x14, 0x10, // ES8311_ADC_REG14 : select Mic1p-Mic1n / PGA GAIN (minimum)
        2, 0x17, 0xBF, // ES8311_ADC_REG17 : ADC_VOLUME 0xBF == ± 0 dB
        2, 0x1C, 0x6A, // ES8311_ADC_REG1C : ADC Equalizer bypass, cancel DC offset in digital domain
        0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {
        2,
        0x0D,
        0xFC, // 0x0D SYSTEM/ Power down analog circuitry
        2,
        0x0E,
        0x6A, // 0x0E SYSTEM
        2,
        0x00,
        0x00, // 0x00 RESET/  CSM POWER DOWN
        0
    };

    i2c_bulk_write(&Wire1, ES8311_ADDR, enable ? enabled_bulk_data : disabled_bulk_data);
}
