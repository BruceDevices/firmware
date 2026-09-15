/*
  Keyboard.h

  Copyright (c) 2015, Arduino LLC
  Original code (pre-library): Copyright (c) 2011, Peter Barrett

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef CH9329_KEYBOARD_h
#define CH9329_KEYBOARD_h

#include "Bad_Usb_Lib.h"
#include <Arduino.h>
//================================================================================
//================================================================================
//  Keyboard

#include "keys.h"
// baud rate
#define CH9329_DEFAULT_BAUDRATE 9600

#define KEY_REPORT_DATA_LENGTH 14

// Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct CH9329_KeyReport {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} CH9329_KeyReport;

#if defined(FLASHEND) && FLASHEND <= 0x7FF
class CH9329_Keyboard_ : public HIDInterface
#else
class CH9329_Keyboard_ : public HIDInterface
#endif
{
private:
    CH9329_KeyReport _keyReport;
    const uint8_t *_asciimap;
    Stream *_stream;
    uint8_t _reportData[KEY_REPORT_DATA_LENGTH];
    void sendReport(CH9329_KeyReport *keys);
    int getReportData(CH9329_KeyReport *keys, uint8_t *buffer, size_t size);
    uint32_t _delay_ms = 25;

public:
    CH9329_Keyboard_(void);
    void begin(Stream &stream, const uint8_t *layout = KeyboardLayout_en_US) override;
    void begin(const uint8_t *layout = KeyboardLayout_en_US) override;
    void end(void) override;
    int getReportData(uint8_t *buffer, size_t size) override;
    size_t write(uint8_t k) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    size_t press(uint8_t k) override;
    size_t release(uint8_t k) override;
    void releaseAll(void) override;
    void setLayout(const uint8_t *layout) override { _asciimap = layout; };
    void setDelay(uint32_t ms);
};
extern CH9329_Keyboard_ CH9329_Keyboard;

#endif
