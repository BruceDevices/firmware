/*
  Keyboard.cpp

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

#include "CH9329_Keyboard.h"
#include "KeyboardLayout.h"

//================================================================================
//================================================================================
//  Keyboard

CH9329_Keyboard_::CH9329_Keyboard_(void) { _asciimap = KeyboardLayout_en_US; }

void CH9329_Keyboard_::begin(Stream &stream, const uint8_t *layout) {
    _asciimap = layout;
    _stream = &stream;
}

void CH9329_Keyboard_::begin(const uint8_t *layout) {
    _asciimap = layout;
    _stream = nullptr;
}

void CH9329_Keyboard_::end(void) {}

int CH9329_Keyboard_::getReportData(uint8_t *buffer, size_t size) {
    return getReportData(&_keyReport, buffer, size);
}

int CH9329_Keyboard_::getReportData(CH9329_KeyReport *keys, uint8_t *buffer, size_t size) {
    if (size < KEY_REPORT_DATA_LENGTH) { return 0; }

    if (buffer == nullptr) { return 0; }

    buffer[0] = 0x57;
    buffer[1] = 0xAB;
    buffer[2] = 0x00;
    buffer[3] = 0x02;
    buffer[4] = 0x08;
    buffer[5] = keys->modifiers;
    buffer[6] = 0;
    buffer[7] = keys->keys[0];
    buffer[8] = keys->keys[1];
    buffer[9] = keys->keys[2];
    buffer[10] = keys->keys[3];
    buffer[11] = keys->keys[4];
    buffer[12] = keys->keys[5];
    int sum = 0;
    for (size_t i = 0; i < 13; i++) { sum += buffer[i]; }
    buffer[13] = (uint8_t)(sum & 0xff);
    return KEY_REPORT_DATA_LENGTH;
}

void CH9329_Keyboard_::sendReport(CH9329_KeyReport *keys) {
    if (_stream == nullptr) { return; }

    int length = getReportData(keys, _reportData, KEY_REPORT_DATA_LENGTH);
    _stream->write(_reportData, length);
}

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t CH9329_Keyboard_::press(uint8_t k) {
    uint8_t i;
    if (k >= 0xE0 && k < 0xE8) {
        // k is not to be changed
    } else if (k >= 136) { // it's a non-printing key (not a modifier)
        k = k - 136;
    } else if (k >= 128) { // it's a modifier key
        _keyReport.modifiers |= (1 << (k - 128));
        k = 0;
    } else { // it's a printing key
        k = pgm_read_byte(_asciimap + k);
        if (!k) { return 0; }
        if ((k & ALT_GR) == ALT_GR) {
            _keyReport.modifiers |= 0x40; // AltGr = right Alt
            k &= 0x3F;
        } else if ((k & SHIFT) == SHIFT) {
            _keyReport.modifiers |= 0x02; // the left shift modifier
            k &= 0x7F;
        }
        if (k == ISO_REPLACEMENT) { k = ISO_KEY; }
    }

    // Add k to the key report only if it's not already present
    // and if there is an empty slot.
    if (_keyReport.keys[0] != k && _keyReport.keys[1] != k && _keyReport.keys[2] != k &&
        _keyReport.keys[3] != k && _keyReport.keys[4] != k && _keyReport.keys[5] != k) {

        for (i = 0; i < 6; i++) {
            if (_keyReport.keys[i] == 0x00) {
                _keyReport.keys[i] = k;
                break;
            }
        }
        if (i == 6) { return 0; }
    }
    sendReport(&_keyReport);
    return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t CH9329_Keyboard_::release(uint8_t k) {
    uint8_t i;
    if (k >= 136) { // it's a non-printing key (not a modifier)
        k = k - 136;
    } else if (k >= 128) { // it's a modifier key
        _keyReport.modifiers &= ~(1 << (k - 128));
        k = 0;
    } else { // it's a printing key
        k = pgm_read_byte(_asciimap + k);
        if (!k) { return 0; }
        if ((k & ALT_GR) == ALT_GR) {
            _keyReport.modifiers &= ~(0x40); // AltGr = right Alt
            k &= 0x3F;
        } else if ((k & SHIFT) == SHIFT) {
            _keyReport.modifiers &= ~(0x02); // the left shift modifier
            k &= 0x7F;
        }
        if (k == ISO_REPLACEMENT) { k = ISO_KEY; }
    }

    // Test the key report to see if k is present.  Clear it if it exists.
    // Check all positions in case the key is present more than once (which it shouldn't be)
    for (i = 0; i < 6; i++) {
        if (0 != k && _keyReport.keys[i] == k) { _keyReport.keys[i] = 0x00; }
    }

    sendReport(&_keyReport);
    return 1;
}

void CH9329_Keyboard_::releaseAll(void) {
    _keyReport.keys[0] = 0;
    _keyReport.keys[1] = 0;
    _keyReport.keys[2] = 0;
    _keyReport.keys[3] = 0;
    _keyReport.keys[4] = 0;
    _keyReport.keys[5] = 0;
    _keyReport.modifiers = 0;
    sendReport(&_keyReport);
}

size_t CH9329_Keyboard_::write(uint8_t c) {
    uint8_t p = press(c); // Keydown
    delay(this->_delay_ms);
    release(c); // Keyup
    delay(this->_delay_ms);
    return p; // just return the result of press() since release() almost always returns 1
}

size_t CH9329_Keyboard_::write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    while (size--) {
        if (*buffer != '\r') {
            if (write(*buffer)) {
                n++;
            } else {
                break;
            }
        }
        buffer++;
    }
    return n;
}

void CH9329_Keyboard_::setDelay(uint32_t ms) { this->_delay_ms = ms; }

CH9329_Keyboard_ CH9329_Keyboard;
