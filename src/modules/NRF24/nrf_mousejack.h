/**
 * @file nrf_mousejack.h
 * @brief MouseJack wireless mouse/keyboard attack module for Bruce firmware.
 *
 * Supports scanning for vulnerable Microsoft and Logitech wireless
 * devices, fingerprinting, HID keystroke injection, and DuckyScript
 * execution over nRF24L01+ (including PA+LNA modules like E01-ML01SP2).
 *
 * Credits: Based on uC_mousejack / WHID / EvilMouse research,
 *          adapted for Bruce multi-device architecture.
 */
#ifndef __NRF_MOUSEJACK_H
#define __NRF_MOUSEJACK_H

#include "modules/NRF24/nrf_common.h"

// ── Maximum targets ───────────────────────────────────────────
#define MJ_MAX_TARGETS 16

// ── Device type identification ────────────────────────────────
enum MjDeviceType : uint8_t {
    MJ_DEVICE_UNKNOWN = 0,
    MJ_DEVICE_MICROSOFT = 1,
    MJ_DEVICE_MS_CRYPT = 2, // Microsoft encrypted
    MJ_DEVICE_LOGITECH = 3,
};

// ── Target structure ──────────────────────────────────────────
struct MjTarget {
    uint8_t address[5];
    uint8_t addrLen;
    uint8_t channel;
    MjDeviceType type;
    bool active;
};

// ── HID key mapping ──────────────────────────────────────────
struct MjHidKey {
    uint8_t modifier;
    uint8_t keycode;
};

// HID Modifier bits
#define MJ_MOD_NONE 0x00
#define MJ_MOD_LCTRL 0x01
#define MJ_MOD_LSHIFT 0x02
#define MJ_MOD_LALT 0x04
#define MJ_MOD_LGUI 0x08
#define MJ_MOD_RCTRL 0x10
#define MJ_MOD_RSHIFT 0x20
#define MJ_MOD_RALT 0x40
#define MJ_MOD_RGUI 0x80

// HID Keycodes (USB HID Usage Table)
#define MJ_KEY_NONE 0x00
#define MJ_KEY_A 0x04
#define MJ_KEY_Z 0x1D
#define MJ_KEY_1 0x1E
#define MJ_KEY_2 0x1F
#define MJ_KEY_3 0x20
#define MJ_KEY_4 0x21
#define MJ_KEY_5 0x22
#define MJ_KEY_6 0x23
#define MJ_KEY_7 0x24
#define MJ_KEY_8 0x25
#define MJ_KEY_9 0x26
#define MJ_KEY_0 0x27
#define MJ_KEY_ENTER 0x28
#define MJ_KEY_ESC 0x29
#define MJ_KEY_BACKSPACE 0x2A
#define MJ_KEY_TAB 0x2B
#define MJ_KEY_SPACE 0x2C
#define MJ_KEY_MINUS 0x2D
#define MJ_KEY_EQUAL 0x2E
#define MJ_KEY_LBRACKET 0x2F
#define MJ_KEY_RBRACKET 0x30
#define MJ_KEY_BACKSLASH 0x31
#define MJ_KEY_SEMICOLON 0x33
#define MJ_KEY_QUOTE 0x34
#define MJ_KEY_GRAVE 0x35
#define MJ_KEY_COMMA 0x36
#define MJ_KEY_DOT 0x37
#define MJ_KEY_SLASH 0x38
#define MJ_KEY_CAPSLOCK 0x39
#define MJ_KEY_F1 0x3A
#define MJ_KEY_F12 0x45
#define MJ_KEY_PRINTSCR 0x46
#define MJ_KEY_SCROLLLOCK 0x47
#define MJ_KEY_PAUSE 0x48
#define MJ_KEY_INSERT 0x49
#define MJ_KEY_HOME 0x4A
#define MJ_KEY_PAGEUP 0x4B
#define MJ_KEY_DELETE 0x4C
#define MJ_KEY_END 0x4D
#define MJ_KEY_PAGEDOWN 0x4E
#define MJ_KEY_RIGHT 0x4F
#define MJ_KEY_LEFT 0x50
#define MJ_KEY_DOWN 0x51
#define MJ_KEY_UP 0x52

// ── DuckyScript key name entry ────────────────────────────────
struct MjDuckyKey {
    const char *name;
    uint8_t modifier;
    uint8_t keycode;
};

// ── Public functions ──────────────────────────────────────────

/// Main MouseJack menu entry
void nrf_mousejack();

#endif // __NRF_MOUSEJACK_H
