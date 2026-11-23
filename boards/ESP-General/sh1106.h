// SPDX-FileCopyrightText: 2026 bmorcelli
//
// SPDX-License-Identifier: MIT

#ifndef LIB_HAL_SH1106_H
#define LIB_HAL_SH1106_H
#include <pins_arduino.h>

#if defined(USE_SH1106)

#include <Wire.h>
#include <WString.h>
#include <SPI.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "tft_defines.h"

#if !defined(TFT_WIDTH)
#define TFT_WIDTH 128
#endif
#if !defined(TFT_HEIGHT)
#define TFT_HEIGHT 64
#endif

#if !defined(SH1106_CHAR_WIDTH)
#define SH1106_CHAR_WIDTH 6
#endif
#if !defined(SH1106_CHAR_HEIGHT)
#define SH1106_CHAR_HEIGHT 8
#endif

static const unsigned char sh1106_font5x7[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x3E, 0x5B, 0x4F, 0x5B, 0x3E,
	0x3E, 0x6B, 0x4F, 0x6B, 0x3E,
	0x1C, 0x3E, 0x7C, 0x3E, 0x1C,
	0x18, 0x3C, 0x7E, 0x3C, 0x18,
	0x1C, 0x57, 0x7D, 0x57, 0x1C,
	0x1C, 0x5E, 0x7F, 0x5E, 0x1C,
	0x00, 0x18, 0x3C, 0x18, 0x00,
	0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
	0x00, 0x18, 0x24, 0x18, 0x00,
	0xFF, 0xE7, 0xDB, 0xE7, 0xFF,
	0x30, 0x48, 0x3A, 0x06, 0x0E,
	0x26, 0x29, 0x79, 0x29, 0x26,
	0x40, 0x7F, 0x05, 0x05, 0x07,
	0x40, 0x7F, 0x05, 0x25, 0x3F,
	0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
	0x7F, 0x3E, 0x1C, 0x1C, 0x08,
	0x08, 0x1C, 0x1C, 0x3E, 0x7F,
	0x14, 0x22, 0x7F, 0x22, 0x14,
	0x5F, 0x5F, 0x00, 0x5F, 0x5F,
	0x06, 0x09, 0x7F, 0x01, 0x7F,
	0x00, 0x66, 0x89, 0x95, 0x6A,
	0x60, 0x60, 0x60, 0x60, 0x60,
	0x94, 0xA2, 0xFF, 0xA2, 0x94,
	0x08, 0x04, 0x7E, 0x04, 0x08,
	0x10, 0x20, 0x7E, 0x20, 0x10,
	0x08, 0x08, 0x2A, 0x1C, 0x08,
	0x08, 0x1C, 0x2A, 0x08, 0x08,
	0x1E, 0x10, 0x10, 0x10, 0x10,
	0x0C, 0x1E, 0x0C, 0x1E, 0x0C,
	0x30, 0x38, 0x3E, 0x38, 0x30,
	0x06, 0x0E, 0x3E, 0x0E, 0x06,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x5F, 0x00, 0x00,
	0x00, 0x07, 0x00, 0x07, 0x00,
	0x14, 0x7F, 0x14, 0x7F, 0x14,
	0x24, 0x2A, 0x7F, 0x2A, 0x12,
	0x23, 0x13, 0x08, 0x64, 0x62,
	0x36, 0x49, 0x56, 0x20, 0x50,
	0x00, 0x08, 0x07, 0x03, 0x00,
	0x00, 0x1C, 0x22, 0x41, 0x00,
	0x00, 0x41, 0x22, 0x1C, 0x00,
	0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
	0x08, 0x08, 0x3E, 0x08, 0x08,
	0x00, 0x80, 0x70, 0x30, 0x00,
	0x08, 0x08, 0x08, 0x08, 0x08,
	0x00, 0x00, 0x60, 0x60, 0x00,
	0x20, 0x10, 0x08, 0x04, 0x02,
	0x3E, 0x51, 0x49, 0x45, 0x3E,
	0x00, 0x42, 0x7F, 0x40, 0x00,
	0x72, 0x49, 0x49, 0x49, 0x46,
	0x21, 0x41, 0x49, 0x4D, 0x33,
	0x18, 0x14, 0x12, 0x7F, 0x10,
	0x27, 0x45, 0x45, 0x45, 0x39,
	0x3C, 0x4A, 0x49, 0x49, 0x31,
	0x41, 0x21, 0x11, 0x09, 0x07,
	0x36, 0x49, 0x49, 0x49, 0x36,
	0x46, 0x49, 0x49, 0x29, 0x1E,
	0x00, 0x00, 0x14, 0x00, 0x00,
	0x00, 0x40, 0x34, 0x00, 0x00,
	0x00, 0x08, 0x14, 0x22, 0x41,
	0x14, 0x14, 0x14, 0x14, 0x14,
	0x00, 0x41, 0x22, 0x14, 0x08,
	0x02, 0x01, 0x59, 0x09, 0x06,
	0x3E, 0x41, 0x5D, 0x59, 0x4E,
	0x7C, 0x12, 0x11, 0x12, 0x7C,
	0x7F, 0x49, 0x49, 0x49, 0x36,
	0x3E, 0x41, 0x41, 0x41, 0x22,
	0x7F, 0x41, 0x41, 0x41, 0x3E,
	0x7F, 0x49, 0x49, 0x49, 0x41,
	0x7F, 0x09, 0x09, 0x09, 0x01,
	0x3E, 0x41, 0x41, 0x51, 0x73,
	0x7F, 0x08, 0x08, 0x08, 0x7F,
	0x00, 0x41, 0x7F, 0x41, 0x00,
	0x20, 0x40, 0x41, 0x3F, 0x01,
	0x7F, 0x08, 0x14, 0x22, 0x41,
	0x7F, 0x40, 0x40, 0x40, 0x40,
	0x7F, 0x02, 0x1C, 0x02, 0x7F,
	0x7F, 0x04, 0x08, 0x10, 0x7F,
	0x3E, 0x41, 0x41, 0x41, 0x3E,
	0x7F, 0x09, 0x09, 0x09, 0x06,
	0x3E, 0x41, 0x51, 0x21, 0x5E,
	0x7F, 0x09, 0x19, 0x29, 0x46,
	0x26, 0x49, 0x49, 0x49, 0x32,
	0x03, 0x01, 0x7F, 0x01, 0x03,
	0x3F, 0x40, 0x40, 0x40, 0x3F,
	0x1F, 0x20, 0x40, 0x20, 0x1F,
	0x3F, 0x40, 0x38, 0x40, 0x3F,
	0x63, 0x14, 0x08, 0x14, 0x63,
	0x03, 0x04, 0x78, 0x04, 0x03,
	0x61, 0x59, 0x49, 0x4D, 0x43,
	0x00, 0x7F, 0x41, 0x41, 0x41,
	0x02, 0x04, 0x08, 0x10, 0x20,
	0x00, 0x41, 0x41, 0x41, 0x7F,
	0x04, 0x02, 0x01, 0x02, 0x04,
	0x40, 0x40, 0x40, 0x40, 0x40,
	0x00, 0x03, 0x07, 0x08, 0x00,
	0x20, 0x54, 0x54, 0x78, 0x40,
	0x7F, 0x28, 0x44, 0x44, 0x38,
	0x38, 0x44, 0x44, 0x44, 0x28,
	0x38, 0x44, 0x44, 0x28, 0x7F,
	0x38, 0x54, 0x54, 0x54, 0x18,
	0x00, 0x08, 0x7E, 0x09, 0x02,
	0x18, 0xA4, 0xA4, 0x9C, 0x78,
	0x7F, 0x08, 0x04, 0x04, 0x78,
	0x00, 0x44, 0x7D, 0x40, 0x00,
	0x20, 0x40, 0x40, 0x3D, 0x00,
	0x7F, 0x10, 0x28, 0x44, 0x00,
	0x00, 0x41, 0x7F, 0x40, 0x00,
	0x7C, 0x04, 0x78, 0x04, 0x78,
	0x7C, 0x08, 0x04, 0x04, 0x78,
	0x38, 0x44, 0x44, 0x44, 0x38,
	0xFC, 0x18, 0x24, 0x24, 0x18,
	0x18, 0x24, 0x24, 0x18, 0xFC,
	0x7C, 0x08, 0x04, 0x04, 0x08,
	0x48, 0x54, 0x54, 0x54, 0x24,
	0x04, 0x04, 0x3F, 0x44, 0x24,
	0x3C, 0x40, 0x40, 0x20, 0x7C,
	0x1C, 0x20, 0x40, 0x20, 0x1C,
	0x3C, 0x40, 0x30, 0x40, 0x3C,
	0x44, 0x28, 0x10, 0x28, 0x44,
	0x4C, 0x90, 0x90, 0x90, 0x7C,
	0x44, 0x64, 0x54, 0x4C, 0x44,
	0x00, 0x08, 0x36, 0x41, 0x00,
	0x00, 0x00, 0x77, 0x00, 0x00,
	0x00, 0x41, 0x36, 0x08, 0x00,
	0x02, 0x01, 0x02, 0x04, 0x02,
	0x3C, 0x26, 0x23, 0x26, 0x3C,
	0x1E, 0xA1, 0xA1, 0x61, 0x12,
	0x3A, 0x40, 0x40, 0x20, 0x7A,
	0x38, 0x54, 0x54, 0x55, 0x59,
	0x21, 0x55, 0x55, 0x79, 0x41,
	0x21, 0x54, 0x54, 0x78, 0x41,
	0x21, 0x55, 0x54, 0x78, 0x40,
	0x20, 0x54, 0x55, 0x79, 0x40,
	0x0C, 0x1E, 0x52, 0x72, 0x12,
	0x39, 0x55, 0x55, 0x55, 0x59,
	0x39, 0x54, 0x54, 0x54, 0x59,
	0x39, 0x55, 0x54, 0x54, 0x58,
	0x00, 0x00, 0x45, 0x7C, 0x41,
	0x00, 0x02, 0x45, 0x7D, 0x42,
	0x00, 0x01, 0x45, 0x7C, 0x40,
	0xF0, 0x29, 0x24, 0x29, 0xF0,
	0xF0, 0x28, 0x25, 0x28, 0xF0,
	0x7C, 0x54, 0x55, 0x45, 0x00,
	0x20, 0x54, 0x54, 0x7C, 0x54,
	0x7C, 0x0A, 0x09, 0x7F, 0x49,
	0x32, 0x49, 0x49, 0x49, 0x32,
	0x32, 0x48, 0x48, 0x48, 0x32,
	0x32, 0x4A, 0x48, 0x48, 0x30,
	0x3A, 0x41, 0x41, 0x21, 0x7A,
	0x3A, 0x42, 0x40, 0x20, 0x78,
	0x00, 0x9D, 0xA0, 0xA0, 0x7D,
	0x39, 0x44, 0x44, 0x44, 0x39,
	0x3D, 0x40, 0x40, 0x40, 0x3D,
	0x3C, 0x24, 0xFF, 0x24, 0x24,
	0x48, 0x7E, 0x49, 0x43, 0x66,
	0x2B, 0x2F, 0xFC, 0x2F, 0x2B,
	0xFF, 0x09, 0x29, 0xF6, 0x20,
	0xC0, 0x88, 0x7E, 0x09, 0x03,
	0x20, 0x54, 0x54, 0x79, 0x41,
	0x00, 0x00, 0x44, 0x7D, 0x41,
	0x30, 0x48, 0x48, 0x4A, 0x32,
	0x38, 0x40, 0x40, 0x22, 0x7A,
	0x00, 0x7A, 0x0A, 0x0A, 0x72,
	0x7D, 0x0D, 0x19, 0x31, 0x7D,
	0x26, 0x29, 0x29, 0x2F, 0x28,
	0x26, 0x29, 0x29, 0x29, 0x26,
	0x30, 0x48, 0x4D, 0x40, 0x20,
	0x38, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x38,
	0x2F, 0x10, 0xC8, 0xAC, 0xBA,
	0x2F, 0x10, 0x28, 0x34, 0xFA,
	0x00, 0x00, 0x7B, 0x00, 0x00,
	0x08, 0x14, 0x2A, 0x14, 0x22,
	0x22, 0x14, 0x2A, 0x14, 0x08,
	0x55, 0x00, 0x55, 0x00, 0x55,    // #176 (25% block) missing in old code
	0xAA, 0x55, 0xAA, 0x55, 0xAA,    // 50% block
	0xFF, 0x55, 0xFF, 0x55, 0xFF,    // 75% block
	0x00, 0x00, 0x00, 0xFF, 0x00,
	0x10, 0x10, 0x10, 0xFF, 0x00,
	0x14, 0x14, 0x14, 0xFF, 0x00,
	0x10, 0x10, 0xFF, 0x00, 0xFF,
	0x10, 0x10, 0xF0, 0x10, 0xF0,
	0x14, 0x14, 0x14, 0xFC, 0x00,
	0x14, 0x14, 0xF7, 0x00, 0xFF,
	0x00, 0x00, 0xFF, 0x00, 0xFF,
	0x14, 0x14, 0xF4, 0x04, 0xFC,
	0x14, 0x14, 0x17, 0x10, 0x1F,
	0x10, 0x10, 0x1F, 0x10, 0x1F,
	0x14, 0x14, 0x14, 0x1F, 0x00,
	0x10, 0x10, 0x10, 0xF0, 0x00,
	0x00, 0x00, 0x00, 0x1F, 0x10,
	0x10, 0x10, 0x10, 0x1F, 0x10,
	0x10, 0x10, 0x10, 0xF0, 0x10,
	0x00, 0x00, 0x00, 0xFF, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0xFF, 0x10,
	0x00, 0x00, 0x00, 0xFF, 0x14,
	0x00, 0x00, 0xFF, 0x00, 0xFF,
	0x00, 0x00, 0x1F, 0x10, 0x17,
	0x00, 0x00, 0xFC, 0x04, 0xF4,
	0x14, 0x14, 0x17, 0x10, 0x17,
	0x14, 0x14, 0xF4, 0x04, 0xF4,
	0x00, 0x00, 0xFF, 0x00, 0xF7,
	0x14, 0x14, 0x14, 0x14, 0x14,
	0x14, 0x14, 0xF7, 0x00, 0xF7,
	0x14, 0x14, 0x14, 0x17, 0x14,
	0x10, 0x10, 0x1F, 0x10, 0x1F,
	0x14, 0x14, 0x14, 0xF4, 0x14,
	0x10, 0x10, 0xF0, 0x10, 0xF0,
	0x00, 0x00, 0x1F, 0x10, 0x1F,
	0x00, 0x00, 0x00, 0x1F, 0x14,
	0x00, 0x00, 0x00, 0xFC, 0x14,
	0x00, 0x00, 0xF0, 0x10, 0xF0,
	0x10, 0x10, 0xFF, 0x10, 0xFF,
	0x14, 0x14, 0x14, 0xFF, 0x14,
	0x10, 0x10, 0x10, 0x1F, 0x00,
	0x00, 0x00, 0x00, 0xF0, 0x10,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0xFF, 0xFF, 0xFF, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xFF, 0xFF,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0x38, 0x44, 0x44, 0x38, 0x44,
	0x7C, 0x2A, 0x2A, 0x3E, 0x14,
	0x7E, 0x02, 0x02, 0x06, 0x06,
	0x02, 0x7E, 0x02, 0x7E, 0x02,
	0x63, 0x55, 0x49, 0x41, 0x63,
	0x38, 0x44, 0x44, 0x3C, 0x04,
	0x40, 0x7E, 0x20, 0x1E, 0x20,
	0x06, 0x02, 0x7E, 0x02, 0x02,
	0x99, 0xA5, 0xE7, 0xA5, 0x99,
	0x1C, 0x2A, 0x49, 0x2A, 0x1C,
	0x4C, 0x72, 0x01, 0x72, 0x4C,
	0x30, 0x4A, 0x4D, 0x4D, 0x30,
	0x30, 0x48, 0x78, 0x48, 0x30,
	0xBC, 0x62, 0x5A, 0x46, 0x3D,
	0x3E, 0x49, 0x49, 0x49, 0x00,
	0x7E, 0x01, 0x01, 0x01, 0x7E,
	0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
	0x44, 0x44, 0x5F, 0x44, 0x44,
	0x40, 0x51, 0x4A, 0x44, 0x40,
	0x40, 0x44, 0x4A, 0x51, 0x40,
	0x00, 0x00, 0xFF, 0x01, 0x03,
	0xE0, 0x80, 0xFF, 0x00, 0x00,
	0x08, 0x08, 0x6B, 0x6B, 0x08,
	0x36, 0x12, 0x36, 0x24, 0x36,
	0x06, 0x0F, 0x09, 0x0F, 0x06,
	0x00, 0x00, 0x18, 0x18, 0x00,
	0x00, 0x00, 0x10, 0x10, 0x00,
	0x30, 0x40, 0xFF, 0x01, 0x01,
	0x00, 0x1F, 0x01, 0x01, 0x1E,
	0x00, 0x19, 0x1D, 0x17, 0x12,
	0x00, 0x3C, 0x3C, 0x3C, 0x3C,
	0x00, 0x00, 0x00, 0x00, 0x00
};

class tft_display {
public:
    explicit tft_display(int16_t _W = TFT_WIDTH, int16_t _H = TFT_HEIGHT)
        : _width(static_cast<uint16_t>(_W)), _height(static_cast<uint16_t>(_H)) {
        memset(_framebuffer, 0, sizeof(_framebuffer));
    }
    friend class tft_sprite;
    friend class tft_logger;

    void begin(uint32_t speed = 400000) {
        if (_initialized) return;
        _wire = &Wire;
        _wire->begin(7, 8, speed);
        _wire->setTimeOut(50);

        // Default to 0x3C, fallback to 0x3D
        _wire->beginTransmission(0x3C);
        if (_wire->endTransmission() == 0) {
            _i2cAddr = 0x3C;
            _present = true;
        } else {
            _wire->beginTransmission(0x3D);
            if (_wire->endTransmission() == 0) {
                _i2cAddr = 0x3D;
                _present = true;
            } else {
                _i2cAddr = 0x3C;
                _present = false;
            }
        }

        if (_present) {
            static const uint8_t initSeq[] = {
                0xAE,         // Display OFF
                0xD5, 0x80,   // Set Display Clock Divide Ratio
                0xA8, 0x3F,   // Set Multiplex Ratio (64 lines)
                0xD3, 0x00,   // Set Display Offset (0)
                0x40,         // Set Start Line (0)
                0xAD, 0x8B,   // Set DC-DC: Enable (0x8B)
                0xA1,         // Set Segment Re-map (0xA1 = column 127 mapped to SEG0)
                0xC8,         // Set COM Scan Direction (0xC8 = COM63 to COM0)
                0xDA, 0x12,   // Set COM Pins hardware config
                0x81, 0x80,   // Set Contrast (0x80)
                0xD9, 0x22,   // Set Pre-charge Period (0x22)
                0xDB, 0x40,   // Set VCOMH Deselect Level (0x40)
                0x30,         // Set Pump Voltage (7.4V)
                0xA4,         // Entire Display ON (resume to RAM content)
                0xA6,         // Set Normal Display (0xA6)
                0xAF          // Display ON
            };
            _wire->beginTransmission(_i2cAddr);
            _wire->write(0x00); // Command stream
            for (size_t i = 0; i < sizeof(initSeq); ++i) {
                _wire->write(initSeq[i]);
            }
            _wire->endTransmission();
        }

        _initialized = true;
        memset(_framebuffer, 0, sizeof(_framebuffer));
        _dirtyPages = 0xFF;
        flushDirty();
    }

    void init(uint8_t tc = 0) {
        (void)tc;
        begin(400000);
    }

    void flushDirty() {
        if (!_initialized || !_dirtyPages || !_wire) return;
        if (!_present) {
            _wire->beginTransmission(_i2cAddr);
            if (_wire->endTransmission() == 0) {
                _present = true;
            } else {
                _dirtyPages = 0;
                return;
            }
        }
        for (uint8_t p = 0; p < 8; ++p) {
            if (_dirtyPages & (1 << p)) {
                // SH1106 132x64 GRAM with column offset = 2
                _wire->beginTransmission(_i2cAddr);
                _wire->write(0x00);
                _wire->write(0xB0 | p); // Page address
                _wire->write(0x02);     // Lower column address = 2
                _wire->write(0x10);     // Higher column address = 0
                _wire->endTransmission();

                const uint8_t *pagePtr = &_framebuffer[p * 128];
                for (uint8_t chunk = 0; chunk < 128; chunk += 32) {
                    _wire->beginTransmission(_i2cAddr);
                    _wire->write(0x40); // Data stream
                    _wire->write(pagePtr + chunk, 32);
                    _wire->endTransmission();
                }
            }
        }
        _dirtyPages = 0;
        _lastFlush = millis();
    }

    void startWrite() { _batchDepth++; }
    void endWrite() {
        if (_batchDepth > 0) _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void setPixelInternal(int32_t x, int32_t y, uint32_t color) {
        if (color == TFT_TRANSPARENT) return;
        int32_t px = x;
        int32_t py = y;
        if (_rotation == 2) {
            px = _width - 1 - x;
            py = _height - 1 - y;
        } else if (_rotation == 1) {
            px = _height - 1 - y;
            py = x;
        } else if (_rotation == 3) {
            px = y;
            py = _width - 1 - x;
        }

        if (px < 0 || px >= 128 || py < 0 || py >= 64) return;

        uint8_t page = py >> 3;
        uint8_t bitMask = 1 << (py & 7);
        size_t idx = page * 128 + px;

        if (color != 0x0000) {
            _framebuffer[idx] |= bitMask;
        } else {
            _framebuffer[idx] &= ~bitMask;
        }
        _dirtyPages |= (1 << page);
    }

    void drawPixel(int32_t x, int32_t y, uint32_t color) {
        setPixelInternal(x, y, color);
        if (_batchDepth == 0) {
            uint32_t now = millis();
            if (now - _lastFlush >= 15) {
                flushDirty();
            }
        }
    }

    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
        if (w <= 0 || h <= 0) return;
        int32_t x0 = std::max<int32_t>(0, x);
        int32_t y0 = std::max<int32_t>(0, y);
        int32_t x1 = std::min<int32_t>(_width, x + w);
        int32_t y1 = std::min<int32_t>(_height, y + h);
        for (int32_t cy = y0; cy < y1; ++cy) {
            for (int32_t cx = x0; cx < x1; ++cx) {
                setPixelInternal(cx, cy, color);
            }
        }
        if (_batchDepth == 0) flushDirty();
    }

    void fillScreen(uint32_t color) {
        uint8_t val = (color != 0x0000) ? 0xFF : 0x00;
        memset(_framebuffer, val, sizeof(_framebuffer));
        _dirtyPages = 0xFF;
        if (_batchDepth == 0) flushDirty();
    }

    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) {
        if (w <= 0 || y < 0 || y >= _height) return;
        int32_t x0 = std::max<int32_t>(0, x);
        int32_t x1 = std::min<int32_t>(_width, x + w);
        for (int32_t cx = x0; cx < x1; ++cx) {
            setPixelInternal(cx, y, color);
        }
        if (_batchDepth == 0) flushDirty();
    }

    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) {
        if (h <= 0 || x < 0 || x >= _width) return;
        int32_t y0 = std::max<int32_t>(0, y);
        int32_t y1 = std::min<int32_t>(_height, y + h);
        for (int32_t cy = y0; cy < y1; ++cy) {
            setPixelInternal(x, cy, color);
        }
        if (_batchDepth == 0) flushDirty();
    }

    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
        if (x0 == x1) {
            drawFastVLine(x0, std::min(y0, y1), abs(y1 - y0) + 1, color);
            return;
        }
        if (y0 == y1) {
            drawFastHLine(std::min(x0, x1), y0, abs(x1 - x0) + 1, color);
            return;
        }
        int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int32_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int32_t err = dx + dy, e2;
        while (true) {
            setPixelInternal(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
        if (_batchDepth == 0) flushDirty();
    }

    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
        _batchDepth++;
        drawFastHLine(x, y, w, color);
        drawFastHLine(x, y + h - 1, w, color);
        drawFastVLine(x, y, h, color);
        drawFastVLine(x + w - 1, y, h, color);
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
        if (r <= 0) { setPixelInternal(x0, y0, color); if (_batchDepth == 0) flushDirty(); return; }
        _batchDepth++;
        int32_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
        setPixelInternal(x0, y0 + r, color);
        setPixelInternal(x0, y0 - r, color);
        setPixelInternal(x0 + r, y0, color);
        setPixelInternal(x0 - r, y0, color);
        while (x < y) {
            if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
            x++; ddF_x += 2; f += ddF_x;
            setPixelInternal(x0 + x, y0 + y, color);
            setPixelInternal(x0 - x, y0 + y, color);
            setPixelInternal(x0 + x, y0 - y, color);
            setPixelInternal(x0 - x, y0 - y, color);
            setPixelInternal(x0 + y, y0 + x, color);
            setPixelInternal(x0 - y, y0 + x, color);
            setPixelInternal(x0 + y, y0 - x, color);
            setPixelInternal(x0 - y, y0 - x, color);
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
        if (r <= 0) { setPixelInternal(x0, y0, color); if (_batchDepth == 0) flushDirty(); return; }
        _batchDepth++;
        drawFastVLine(x0, y0 - r, 2 * r + 1, color);
        int32_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
        while (x < y) {
            if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
            x++; ddF_x += 2; f += ddF_x;
            drawFastVLine(x0 + x, y0 - y, 2 * y + 1, color);
            drawFastVLine(x0 - x, y0 - y, 2 * y + 1, color);
            drawFastVLine(x0 + y, y0 - x, 2 * x + 1, color);
            drawFastVLine(x0 - y, y0 - x, 2 * x + 1, color);
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
        if (r <= 0) { drawRect(x, y, w, h, color); return; }
        if (r > w / 2) r = w / 2;
        if (r > h / 2) r = h / 2;
        _batchDepth++;
        drawFastHLine(x + r, y, w - 2 * r, color);
        drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
        drawFastVLine(x, y + r, h - 2 * r, color);
        drawFastVLine(x + w - 1, y + r, h - 2 * r, color);
        int32_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, cx = 0, cy = r;
        while (cx < cy) {
            if (f >= 0) { cy--; ddF_y += 2; f += ddF_y; }
            cx++; ddF_x += 2; f += ddF_x;
            setPixelInternal(x + w - 1 - r + cx, y + r - cy, color);
            setPixelInternal(x + w - 1 - r + cy, y + r - cx, color);
            setPixelInternal(x + r - cx, y + r - cy, color);
            setPixelInternal(x + r - cy, y + r - cx, color);
            setPixelInternal(x + w - 1 - r + cx, y + h - 1 - r + cy, color);
            setPixelInternal(x + w - 1 - r + cy, y + h - 1 - r + cx, color);
            setPixelInternal(x + r - cx, y + h - 1 - r + cy, color);
            setPixelInternal(x + r - cy, y + h - 1 - r + cx, color);
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
        if (r <= 0) { fillRect(x, y, w, h, color); return; }
        if (r > w / 2) r = w / 2;
        if (r > h / 2) r = h / 2;
        _batchDepth++;
        fillRect(x + r, y, w - 2 * r, h, color);
        int32_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, cx = 0, cy = r;
        while (cx < cy) {
            if (f >= 0) { cy--; ddF_y += 2; f += ddF_y; }
            cx++; ddF_x += 2; f += ddF_x;
            drawFastVLine(x + r - cx, y + r - cy, h - 2 * r + 2 * cy, color);
            drawFastVLine(x + w - 1 - r + cx, y + r - cy, h - 2 * r + 2 * cy, color);
            drawFastVLine(x + r - cy, y + r - cx, h - 2 * r + 2 * cx, color);
            drawFastVLine(x + w - 1 - r + cy, y + r - cx, h - 2 * r + 2 * cx, color);
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
        _batchDepth++;
        drawLine(x0, y0, x1, y1, color);
        drawLine(x1, y1, x2, y2, color);
        drawLine(x2, y2, x0, y0, color);
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
        if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
        if (y0 == y2) return;
        _batchDepth++;
        int32_t totalHeight = y2 - y0;
        for (int32_t i = 0; i < totalHeight; i++) {
            bool secondHalf = i > y1 - y0 || y1 == y0;
            int32_t segmentHeight = secondHalf ? y2 - y1 : y1 - y0;
            float alpha = (float)i / totalHeight;
            float beta = (float)(i - (secondHalf ? y1 - y0 : 0)) / segmentHeight;
            int32_t ax = x0 + (x2 - x0) * alpha;
            int32_t bx = secondHalf ? x1 + (x2 - x1) * beta : x0 + (x1 - x0) * beta;
            if (ax > bx) std::swap(ax, bx);
            drawFastHLine(ax, y0 + i, bx - ax + 1, color);
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
        if (rx <= 0 || ry <= 0) return;
        _batchDepth++;
        int32_t x, y;
        int32_t rx2 = rx * rx, ry2 = ry * ry;
        int32_t fx2 = 4 * rx2, fy2 = 4 * ry2;
        int32_t s;
        for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) {
            setPixelInternal(x0 + x, y0 + y, color);
            setPixelInternal(x0 - x, y0 + y, color);
            setPixelInternal(x0 - x, y0 - y, color);
            setPixelInternal(x0 + x, y0 - y, color);
            if (s >= 0) { s += fx2 * (1 - y); y--; }
            s += ry2 * ((4 * x) + 6);
        }
        for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) {
            setPixelInternal(x0 + x, y0 + y, color);
            setPixelInternal(x0 - x, y0 + y, color);
            setPixelInternal(x0 - x, y0 - y, color);
            setPixelInternal(x0 + x, y0 - y, color);
            if (s >= 0) { s += fy2 * (1 - x); x--; }
            s += rx2 * ((4 * y) + 6);
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
        if (rx <= 0 || ry <= 0) return;
        _batchDepth++;
        int32_t rx2 = rx * rx, ry2 = ry * ry;
        for (int32_t y = -ry; y <= ry; y++) {
            int32_t dx = (int32_t)sqrt((float)(rx2 * (1.0f - (float)(y * y) / (float)ry2)));
            drawFastHLine(x0 - dx, y0 + y, 2 * dx + 1, color);
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void drawArc(
        int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle,
        uint32_t fg_color, uint32_t bg_color, bool smoothArc = true
    ) {
        (void)ir; (void)startAngle; (void)endAngle; (void)bg_color; (void)smoothArc;
        drawCircle(x, y, r, fg_color);
    }

    void drawWideLine(
        float ax, float ay, float bx, float by, float wd, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF
    ) {
        (void)wd; (void)bg_color;
        drawLine((int32_t)ax, (int32_t)ay, (int32_t)bx, (int32_t)by, fg_color);
    }

    void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
        if (!bitmap || w <= 0 || h <= 0) return;
        _batchDepth++;
        int16_t byteWidth = (w + 7) / 8;
        for (int16_t j = 0; j < h; j++) {
            for (int16_t i = 0; i < w; i++) {
                if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (1 << (i & 7))) {
                    setPixelInternal(x + i, y + j, color);
                }
            }
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void drawXBitmap(
        int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg
    ) {
        if (!bitmap || w <= 0 || h <= 0) return;
        _batchDepth++;
        int16_t byteWidth = (w + 7) / 8;
        for (int16_t j = 0; j < h; j++) {
            for (int16_t i = 0; i < w; i++) {
                if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (1 << (i & 7))) {
                    setPixelInternal(x + i, y + j, color);
                } else if (bg != TFT_TRANSPARENT) {
                    setPixelInternal(x + i, y + j, bg);
                }
            }
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data) {
        if (!data || w <= 0 || h <= 0) return;
        _batchDepth++;
        for (int32_t cy = 0; cy < h; ++cy) {
            for (int32_t cx = 0; cx < w; ++cx) {
                uint16_t c = data[cy * w + cx];
                setPixelInternal(x + cx, y + cy, c);
            }
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
    }

    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data) {
        pushImage(x, y, w, h, const_cast<const uint16_t *>(data));
    }

    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8, uint16_t *cmap) {
        (void)x; (void)y; (void)w; (void)h; (void)data; (void)bpp8; (void)cmap;
    }

    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8, uint16_t *cmap) {
        (void)x; (void)y; (void)w; (void)h; (void)data; (void)bpp8; (void)cmap;
    }

    void fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2) {
        (void)color2;
        fillRect(x, y, w, h, color1);
    }

    void fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2) {
        (void)color2;
        fillRect(x, y, w, h, color1);
    }

    void invertDisplay(bool i) {
        if (!_wire) return;
        _wire->beginTransmission(_i2cAddr);
        _wire->write(0x00);
        _wire->write(i ? 0xA7 : 0xA6);
        _wire->endTransmission();
    }

    void sleep(bool value) {
        if (!_wire) return;
        _wire->beginTransmission(_i2cAddr);
        _wire->write(0x00);
        _wire->write(value ? 0xAE : 0xAF);
        _wire->endTransmission();
    }

    void setSwapBytes(bool swap) { _swapBytes = swap; }
    bool getSwapBytes() const { return _swapBytes; }

    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) const {
        return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }

    int16_t textWidth(const String &s, uint8_t font = 1) const { return textWidth(s.c_str(), font); }
    int16_t textWidth(const char *s, uint8_t font = 1) const {
        (void)font;
        if (!s) return 0;
        return static_cast<int16_t>(strlen(s) * SH1106_CHAR_WIDTH * (_textSize ? _textSize : 1));
    }

    void setCursor(int16_t x, int16_t y) {
        _cursorX = x;
        _cursorY = y;
    }
    int16_t getCursorX() const { return _cursorX; }
    int16_t getCursorY() const { return _cursorY; }
    void setTextSize(uint8_t s) { _textSize = s ? s : 1; }
    void setTextColor(uint16_t c) { _textColor = c; }
    void setTextColor(uint16_t c, uint16_t b, bool bgfill = false) {
        (void)bgfill;
        _textColor = c;
        _textBgColor = b;
    }
    void setTextDatum(uint8_t d) { _textDatum = d; }
    uint8_t getTextDatum() const { return _textDatum; }
    void setTextFont(uint8_t f) { _textFont = f; }
    void setTextWrap(bool wrapX, bool wrapY = false) { (void)wrapX; (void)wrapY; }

    void renderChar(uint8_t c, int32_t x, int32_t y) {
        uint8_t sz = _textSize ? _textSize : 1;
        bool bgDraw = (_textBgColor != TFT_TRANSPARENT && _textBgColor != _textColor);

        for (uint8_t col = 0; col < 5; ++col) {
            uint8_t line = pgm_read_byte(&sh1106_font5x7[c * 5 + col]);
            for (uint8_t row = 0; row < 8; ++row) {
                bool on = (line & (1 << row)) != 0;
                if (on) {
                    if (sz == 1) setPixelInternal(x + col, y + row, _textColor);
                    else fillRectInternal(x + col * sz, y + row * sz, sz, sz, _textColor);
                } else if (bgDraw) {
                    if (sz == 1) setPixelInternal(x + col, y + row, _textBgColor);
                    else fillRectInternal(x + col * sz, y + row * sz, sz, sz, _textBgColor);
                }
            }
        }
        if (bgDraw) {
            if (sz == 1) {
                for (uint8_t row = 0; row < 8; ++row) {
                    setPixelInternal(x + 5, y + row, _textBgColor);
                }
            } else {
                fillRectInternal(x + 5 * sz, y, sz, 8 * sz, _textBgColor);
            }
        }
    }

    void fillRectInternal(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
        if (w <= 0 || h <= 0) return;
        int32_t x0 = std::max<int32_t>(0, x);
        int32_t y0 = std::max<int32_t>(0, y);
        int32_t x1 = std::min<int32_t>(_width, x + w);
        int32_t y1 = std::min<int32_t>(_height, y + h);
        for (int32_t cy = y0; cy < y1; ++cy) {
            for (int32_t cx = x0; cx < x1; ++cx) {
                setPixelInternal(cx, cy, color);
            }
        }
    }

    void renderStringAt(const char *s, int32_t x, int32_t y) {
        if (!s) return;
        _batchDepth++;
        uint8_t sz = _textSize ? _textSize : 1;
        int32_t curX = x;
        while (*s) {
            renderChar(static_cast<uint8_t>(*s), curX, y);
            curX += 6 * sz;
            s++;
        }
        _batchDepth--;
    }

    int16_t drawString(const String &string, int32_t x, int32_t y, uint8_t font = 1) {
        (void)font;
        int16_t strW = textWidth(string, font);
        int16_t strH = fontHeight(font);
        int32_t drawX = x;
        int32_t drawY = y;

        switch (_textDatum) {
            case 0: break; // TL_DATUM
            case 1: drawX -= strW / 2; break; // TC_DATUM
            case 2: drawX -= strW; break;     // TR_DATUM
            case 3: drawY -= strH / 2; break; // ML_DATUM
            case 4: drawX -= strW / 2; drawY -= strH / 2; break; // MC_DATUM
            case 5: drawX -= strW; drawY -= strH / 2; break;     // MR_DATUM
            case 6: drawY -= strH; break;     // BL_DATUM
            case 7: drawX -= strW / 2; drawY -= strH; break;     // BC_DATUM
            case 8: drawX -= strW; drawY -= strH; break;         // BR_DATUM
            default: break;
        }

        renderStringAt(string.c_str(), drawX, drawY);
        if (_batchDepth == 0) flushDirty();
        return strW;
    }

    int16_t drawCentreString(const String &string, int32_t x, int32_t y, uint8_t font = 1) {
        uint8_t oldDatum = _textDatum;
        _textDatum = 1; // TC_DATUM
        int16_t w = drawString(string, x, y, font);
        _textDatum = oldDatum;
        return w;
    }

    int16_t drawRightString(const String &string, int32_t x, int32_t y, uint8_t font = 1) {
        uint8_t oldDatum = _textDatum;
        _textDatum = 2; // TR_DATUM
        int16_t w = drawString(string, x, y, font);
        _textDatum = oldDatum;
        return w;
    }

    size_t write(uint8_t c) {
        uint8_t sz = _textSize ? _textSize : 1;
        if (c == '\n') {
            _cursorX = 0;
            _cursorY += fontHeight();
            return 1;
        }
        if (c == '\r') {
            _cursorX = 0;
            return 1;
        }
        renderChar(c, _cursorX, _cursorY);
        _cursorX += 6 * sz;
        if (_cursorX + 6 * sz > _width) {
            _cursorX = 0;
            _cursorY += fontHeight();
        }
        return 1;
    }

    size_t write(const uint8_t *buffer, size_t size) {
        if (!buffer || size == 0) return 0;
        _batchDepth++;
        for (size_t i = 0; i < size; ++i) {
            write(buffer[i]);
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
        return size;
    }

    size_t print(const char *val) {
        if (!val) return 0;
        _batchDepth++;
        size_t n = 0;
        while (*val) {
            write(static_cast<uint8_t>(*val++));
            n++;
        }
        _batchDepth--;
        if (_batchDepth == 0) flushDirty();
        return n;
    }

    size_t println(const char *val) {
        size_t n = print(val);
        n += print("\n");
        return n;
    }

    size_t print(const String &s) { return print(s.c_str()); }
    size_t println(const String &s) { return println(s.c_str()); }
    size_t print(char c) {
        char buf[2] = {c, 0};
        return print(buf);
    }
    size_t println(char c) {
        char buf[2] = {c, 0};
        return println(buf);
    }
    template <typename T> size_t print(const T &val) { return print(String(val).c_str()); }
    template <typename T> size_t println(const T &val) { return println(String(val).c_str()); }
    size_t println() { return print("\n"); }

    size_t printf(const char *fmt, ...) {
        if (!fmt) return 0;
        char loc_buf[128];
        va_list arg;
        va_start(arg, fmt);
        int len = vsnprintf(loc_buf, sizeof(loc_buf), fmt, arg);
        va_end(arg);
        if (len < 0) return 0;
        if (static_cast<size_t>(len) >= sizeof(loc_buf)) {
            char *temp = (char *)malloc(len + 1);
            if (!temp) return 0;
            va_start(arg, fmt);
            vsnprintf(temp, len + 1, fmt, arg);
            va_end(arg);
            size_t n = print(temp);
            free(temp);
            return n;
        }
        return print(loc_buf);
    }

    int16_t width() const { return static_cast<int16_t>((_rotation & 1) ? _height : _width); }
    int16_t height() const { return static_cast<int16_t>((_rotation & 1) ? _width : _height); }
    SPIClass &getSPIinstance() const { return SPI; }
    void writecommand(uint8_t c) {
        if (_wire && _initialized) {
            _wire->beginTransmission(_i2cAddr);
            _wire->write(0x00);
            _wire->write(c);
            _wire->endTransmission();
        }
    }

    uint32_t getTextColor() const { return _textColor; }
    uint32_t getTextBgColor() const { return _textBgColor; }
    uint8_t getTextSize() const { return _textSize; }
    uint8_t getRotation() const { return _rotation; }
    void setRotation(uint8_t r) {
        _rotation = static_cast<uint8_t>(r & 3);
    }
    int16_t fontHeight(int16_t font = 1) const {
        (void)font;
        return static_cast<int16_t>(SH1106_CHAR_HEIGHT * (_textSize ? _textSize : 1));
    }

    void *native() { return this; }
    void display(bool fullRefresh = false) {
        if (fullRefresh) _dirtyPages = 0xFF;
        flushDirty();
    }

private:
    uint16_t _width = TFT_WIDTH;
    uint16_t _height = TFT_HEIGHT;
    uint8_t _framebuffer[1024];
    uint8_t _dirtyPages = 0xFF;
    TwoWire *_wire = nullptr;
    uint8_t _i2cAddr = 0x3C;
    bool _initialized = false;
    bool _present = false;
    int16_t _cursorX = 0;
    int16_t _cursorY = 0;
    bool _swapBytes = false;
    uint32_t _textColor = TFT_WHITE;
    uint32_t _textBgColor = TFT_BLACK;
    uint8_t _textSize = 1;
    uint8_t _textDatum = 0;
    uint8_t _textFont = 1;
    uint8_t _rotation = 0;
    uint32_t _lastFlush = 0;
    uint8_t _batchDepth = 0;
};

class tft_sprite {
public:
    explicit tft_sprite(tft_display *parent) : _display(parent) {}
    ~tft_sprite() = default;

    void *createSprite(int16_t w, int16_t h, uint8_t frames = 1) {
        (void)frames;
        _width = w;
        _height = h;
        return nullptr;
    }
    void deleteSprite() {
        _width = 0;
        _height = 0;
    }

    void fillScreen(uint32_t color) { (void)color; }
    void setColorDepth(uint8_t depth) { _colorDepth = depth; }
    void setCursor(int16_t x, int16_t y) {
        _cursorX = x;
        _cursorY = y;
    }
    void setTextColor(uint16_t c) { _textColor = c; }
    void setTextColor(uint16_t c, uint16_t b) {
        _textColor = c;
        _textBgColor = b;
    }
    void setTextSize(uint8_t s) { _textSize = s; }
    void setTextDatum(uint8_t d) { _textDatum = d; }

    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
        (void)x; (void)y; (void)w; (void)h; (void)color;
    }
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) {
        (void)x; (void)y; (void)h; (void)color;
    }
    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
        (void)x0; (void)y0; (void)r; (void)color;
    }
    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
        (void)x0; (void)y0; (void)r; (void)color;
    }
    void fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
        (void)x0; (void)y0; (void)rx; (void)ry; (void)color;
    }
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
        (void)x0; (void)y0; (void)x1; (void)y1; (void)x2; (void)y2; (void)color;
    }
    void pushSprite(int32_t x, int32_t y, uint32_t transparent = TFT_TRANSPARENT) {
        (void)x; (void)y; (void)transparent;
    }
    void pushToSprite(tft_sprite *dest, int32_t x, int32_t y, uint32_t transparent = TFT_TRANSPARENT) {
        (void)dest; (void)x; (void)y; (void)transparent;
    }

    int16_t width() const { return _width; }
    int16_t height() const { return _height; }
    int16_t fontHeight(int16_t font = 1) const {
        (void)font;
        return static_cast<int16_t>(SH1106_CHAR_HEIGHT * (_textSize ? _textSize : 1));
    }

    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
        (void)x0; (void)y0; (void)x1; (void)y1; (void)color;
    }
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
        (void)x; (void)y; (void)w; (void)h; (void)color;
    }
    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
        (void)x; (void)y; (void)w; (void)h; (void)r; (void)color;
    }
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
        (void)x; (void)y; (void)w; (void)h; (void)r; (void)color;
    }
    void drawPixel(int32_t x, int32_t y, uint32_t color) {
        (void)x; (void)y; (void)color;
    }
    void drawXBitmap(
        int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg = 0
    ) {
        (void)x; (void)y; (void)bitmap; (void)w; (void)h; (void)color; (void)bg;
    }
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data) {
        (void)x; (void)y; (void)w; (void)h; (void)data;
    }
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8, uint16_t *cmap) {
        (void)x; (void)y; (void)w; (void)h; (void)data; (void)bpp8; (void)cmap;
    }
    void pushImage(
        int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8, uint16_t *cmap
    ) {
        (void)x; (void)y; (void)w; (void)h; (void)data; (void)bpp8; (void)cmap;
    }
    void fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2) {
        (void)x; (void)y; (void)w; (void)h; (void)color1; (void)color2;
    }
    void fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2) {
        (void)x; (void)y; (void)w; (void)h; (void)color1; (void)color2;
    }
    int16_t drawString(const String &string, int32_t x, int32_t y, uint8_t font = 1) {
        (void)x; (void)y; (void)font;
        return static_cast<int16_t>(string.length() * SH1106_CHAR_WIDTH * (_textSize ? _textSize : 1));
    }

private:
    tft_display *_display = nullptr;
    int16_t _width = 0;
    int16_t _height = 0;
    uint8_t _colorDepth = 16;
    int16_t _cursorX = 0;
    int16_t _cursorY = 0;
    uint32_t _textColor = TFT_WHITE;
    uint32_t _textBgColor = TFT_BLACK;
    uint8_t _textSize = 1;
    uint8_t _textDatum = 0;
};

#endif // USE_SH1106
#endif // LIB_HAL_SH1106_H
