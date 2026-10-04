#ifdef ARDUINO_M5STACK_CARDPUTER
/**
 * @file keyboard.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <iostream>
#include <vector>
#include "Arduino.h"

struct Chart_t
{
    uint8_t value;
    uint8_t x_1;
    uint8_t x_2;
};

struct Point2D_t
{
    int x;
    int y;
};

// Cardputer ASCII keymap (moved here from boards/m5stack-cardputer/pins_arduino.h
// so it no longer depends on that header being force-included by the toolchain).
const uint8_t _kb_asciimap[128] = {
    0x00,          // NUL
    0x00,          // SOH
    0x00,          // STX
    0x00,          // ETX
    0x00,          // EOT
    0x00,          // ENQ
    0x00,          // ACK
    0x00,          // BEL
    KEY_BACKSPACE, // BS	Backspace
    KEY_TAB,       // TAB	Tab
    KEY_ENTER,     // LF	Enter
    0x00,          // VT
    0x00,          // FF
    0x00,          // CR
    0x00,          // SO
    0x00,          // SI
    0x00,          // DEL
    0x00,          // DC1
    0x00,          // DC2
    0x00,          // DC3
    0x00,          // DC4
    0x00,          // NAK
    0x00,          // SYN
    0x00,          // ETB
    0x00,          // CAN
    0x00,          // EM
    0x00,          // SUB
    0x00,          // ESC
    0x00,          // FS
    0x00,          // GS
    0x00,          // RS
    0x00,          // US

    0x2c,         //  ' '
    0x1e | SHIFT, // !
    0x34 | SHIFT, // "
    0x20 | SHIFT, // #
    0x21 | SHIFT, // $
    0x22 | SHIFT, // %
    0x24 | SHIFT, // &
    0x34,         // '
    0x26 | SHIFT, // (
    0x27 | SHIFT, // )
    0x25 | SHIFT, // *
    0x2e | SHIFT, // +
    0x36,         // ,
    0x2d,         // -
    0x37,         // .
    0x38,         // /
    0x27,         // 0
    0x1e,         // 1
    0x1f,         // 2
    0x20,         // 3
    0x21,         // 4
    0x22,         // 5
    0x23,         // 6
    0x24,         // 7
    0x25,         // 8
    0x26,         // 9
    0x33 | SHIFT, // :
    0x33,         // ;
    0x36 | SHIFT, // <
    0x2e,         // =
    0x37 | SHIFT, // >
    0x38 | SHIFT, // ?
    0x1f | SHIFT, // @
    0x04 | SHIFT, // A
    0x05 | SHIFT, // B
    0x06 | SHIFT, // C
    0x07 | SHIFT, // D
    0x08 | SHIFT, // E
    0x09 | SHIFT, // F
    0x0a | SHIFT, // G
    0x0b | SHIFT, // H
    0x0c | SHIFT, // I
    0x0d | SHIFT, // J
    0x0e | SHIFT, // K
    0x0f | SHIFT, // L
    0x10 | SHIFT, // M
    0x11 | SHIFT, // N
    0x12 | SHIFT, // O
    0x13 | SHIFT, // P
    0x14 | SHIFT, // Q
    0x15 | SHIFT, // R
    0x16 | SHIFT, // S
    0x17 | SHIFT, // T
    0x18 | SHIFT, // U
    0x19 | SHIFT, // V
    0x1a | SHIFT, // W
    0x1b | SHIFT, // X
    0x1c | SHIFT, // Y
    0x1d | SHIFT, // Z
    0x2f,         // [
    0x31,         // bslash
    0x30,         // ]
    0x23 | SHIFT, // ^
    0x2d | SHIFT, // _
    0x35,         // `
    0x04,         // a
    0x05,         // b
    0x06,         // c
    0x07,         // d
    0x08,         // e
    0x09,         // f
    0x0a,         // g
    0x0b,         // h
    0x0c,         // i
    0x0d,         // j
    0x0e,         // k
    0x0f,         // l
    0x10,         // m
    0x11,         // n
    0x12,         // o
    0x13,         // p
    0x14,         // q
    0x15,         // r
    0x16,         // s
    0x17,         // t
    0x18,         // u
    0x19,         // v
    0x1a,         // w
    0x1b,         // x
    0x1c,         // y
    0x1d,         // z
    0x2f | SHIFT, // {
    0x31 | SHIFT, // |
    0x30 | SHIFT, // }
    0x35 | SHIFT, // ~
    0             // DEL
};

const std::vector<int> output_list = {8, 9, 11};
const std::vector<int> input_list = {13, 15, 3, 4, 5, 6, 7};

const Chart_t X_map_chart[7] = {{1, 0, 1}, {2, 2, 3}, {4, 4, 5}, {8, 6, 7}, {16, 8, 9}, {32, 10, 11}, {64, 12, 13}};

struct KeyValue_t
{
    const char value_first;
    const char value_second;
};

const KeyValue_t _key_value_map[4][14] = {{{'`', '~'},
                                           {'1', '!'},
                                           {'2', '@'},
                                           {'3', '#'},
                                           {'4', '$'},
                                           {'5', '%'},
                                           {'6', '^'},
                                           {'7', '&'},
                                           {'8', '*'},
                                           {'9', '('},
                                           {'0', ')'},
                                           {'-', '_'},
                                           {'=', '+'},
                                           {KEY_BACKSPACE, KEY_BACKSPACE}},
                                          {{KEY_TAB, KEY_TAB},
                                           {'q', 'Q'},
                                           {'w', 'W'},
                                           {'e', 'E'},
                                           {'r', 'R'},
                                           {'t', 'T'},
                                           {'y', 'Y'},
                                           {'u', 'U'},
                                           {'i', 'I'},
                                           {'o', 'O'},
                                           {'p', 'P'},
                                           {'[', '{'},
                                           {']', '}'},
                                           {'\\', '|'}},
                                          {{KEY_FN, KEY_FN},
                                           {KEY_LEFT_SHIFT, KEY_LEFT_SHIFT},
                                           {'a', 'A'},
                                           {'s', 'S'},
                                           {'d', 'D'},
                                           {'f', 'F'},
                                           {'g', 'G'},
                                           {'h', 'H'},
                                           {'j', 'J'},
                                           {'k', 'K'},
                                           {'l', 'L'},
                                           {';', ':'},
                                           {'\'', '\"'},
                                           {KEY_ENTER, KEY_ENTER}},
                                          {{KEY_LEFT_CTRL, KEY_LEFT_CTRL},
                                           {KEY_OPT, KEY_OPT},
                                           {KEY_LEFT_ALT, KEY_LEFT_ALT},
                                           {'z', 'Z'},
                                           {'x', 'X'},
                                           {'c', 'C'},
                                           {'v', 'V'},
                                           {'b', 'B'},
                                           {'n', 'N'},
                                           {'m', 'M'},
                                           {',', '<'},
                                           {'.', '>'},
                                           {'/', '?'},
                                           {' ', ' '}}};

class Keyboard_Class
{
public:
    struct KeysState
    {
        bool tab = false;
        bool fn = false;
        bool shift = false;
        bool ctrl = false;
        bool opt = false;
        bool alt = false;
        bool del = false;
        bool enter = false;
        bool space = false;
        uint8_t modifiers = 0;

        std::vector<char> word;
        std::vector<uint8_t> hid_keys;
        std::vector<uint8_t> modifier_keys;

        void reset()
        {
            tab = false;
            fn = false;
            shift = false;
            ctrl = false;
            opt = false;
            alt = false;
            del = false;
            enter = false;
            space = false;
            modifiers = 0;
            word.clear();
            hid_keys.clear();
            modifier_keys.clear();
        }
    };

private:
    std::vector<Point2D_t> _key_list_buffer;
    std::vector<Point2D_t> _key_pos_print_keys; // only text: eg A,B,C
    std::vector<Point2D_t> _key_pos_hid_keys;   // print key + space, enter, del
    std::vector<Point2D_t>
        _key_pos_modifier_keys; // modifier key: eg shift, ctrl, alt
    KeysState _keys_state_buffer;
    bool _is_caps_locked;
    uint8_t _last_key_size;

    void _set_output(const std::vector<int> &pinList, uint8_t output);
    uint8_t _get_input(const std::vector<int> &pinList);

public:
    Keyboard_Class() : _is_caps_locked(false)
    {
    }

    void begin();
    uint8_t getKey(Point2D_t keyCoor);

    void updateKeyList();

    inline std::vector<Point2D_t> &keyList()
    {
        return _key_list_buffer;
    }

    inline KeyValue_t getKeyValue(const Point2D_t &keyCoor)
    {
        return _key_value_map[keyCoor.y][keyCoor.x];
    }

    uint8_t isPressed();
    bool isChange();
    bool isKeyPressed(char c);

    void updateKeysState();

    void update()
    {
        updateKeyList();
        updateKeysState();
    }
    inline KeysState &keysState()
    {
        return _keys_state_buffer;
    }

    inline bool capslocked(void)
    {
        return _is_caps_locked;
    }
    inline void setCapsLocked(bool isLocked)
    {
        _is_caps_locked = isLocked;
    }
};
#endif
