/* Touchscreen library for XPT2046 Touch Controller Chip
 * Copyright (c) 2015, Paul Stoffregen, paul@pjrc.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ----------------------- CYD28 (2.8" cheap yellow display) version --------------------------
 *   using software bit banged SPI and fixed GPIOs  + manual calibration
 * 08.2023 - Piotr Zapart www.hexefx.com
 *
 *
 */

#include "CYD28_TouchscreenR.h"
#include <SPI.h>

#define ISR_PREFIX IRAM_ATTR
#define MSEC_THRESHOLD 3
#ifndef SPI_TOUCH_FREQUENCY
#define SPI_TOUCH_FREQUENCY 2000000
#endif
#define SPI_SETTING SPISettings(SPI_TOUCH_FREQUENCY, MSBFIRST, SPI_MODE0)

static CYD28_TouchR *isrPinptr;
void isrPin(void);
// ------------------------------------------------------------
bool CYD28_TouchR::configurePins(bool hardwareSpi) {
    if (CYD28_TouchR_CS < 0 || CYD28_TouchR_MISO < 0 || CYD28_TouchR_MOSI < 0 || CYD28_TouchR_CLK < 0)
        return false;

    pinMode(CYD28_TouchR_CS, OUTPUT);
    digitalWrite(CYD28_TouchR_CS, HIGH);

    if (!hardwareSpi) {
        pinMode(CYD28_TouchR_MOSI, OUTPUT);
        pinMode(CYD28_TouchR_MISO, INPUT);
        pinMode(CYD28_TouchR_CLK, OUTPUT);
        digitalWrite(CYD28_TouchR_CLK, LOW);
    }

    return true;
}

bool CYD28_TouchR::setupInterrupt() {
    isrPinptr = this;
#if CYD28_TouchR_IRQ >= 0
    pinMode(CYD28_TouchR_IRQ, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CYD28_TouchR_IRQ), isrPin, FALLING);
#else
    isrWake = true;
#endif
    return true;
}

bool CYD28_TouchR::probe() {
    uint16_t samples[3];

    if (_pspi != nullptr) _pspi->beginTransaction(SPI_SETTING);
    digitalWrite(CYD28_TouchR_CS, LOW);
    delayMicroseconds(10);

    transfer(0xB1);
    samples[0] = transfer16(0xC1) >> 3;
    samples[1] = transfer16(0x91) >> 3;
    samples[2] = transfer16(0) >> 3;

    digitalWrite(CYD28_TouchR_CS, HIGH);
    if (_pspi != nullptr) _pspi->endTransaction();

    return samples[0] != 0x1FFF || samples[1] != 0x1FFF || samples[2] != 0x1FFF;
}

bool CYD28_TouchR::begin() {
    _started = false;
    _pspi = nullptr;

    if (!configurePins(false)) return false;
    setupInterrupt();
    _started = true;

    if (!probe()) {
        _started = false;
        return false;
    }
    return true;
}

bool CYD28_TouchR::begin(SPIClass *wspi) {
    if (wspi == nullptr) return false;
    _started = false;
    _pspi = wspi;

    if (!configurePins(true)) return false;
    setupInterrupt();
    _started = true;

    if (!probe()) {
        _started = false;
        return false;
    }
    return true;
}
// ------------------------------------------------------------
ISR_PREFIX
void isrPin(void) {
    CYD28_TouchR *o = isrPinptr;
    if (o != nullptr) o->isrWake = true;
}
// ------------------------------------------------------------
uint8_t CYD28_TouchR::transfer(uint8_t val) {
    if (_pspi == nullptr) {
        uint8_t out = 0;
        uint8_t del = _delay >> 1;
        uint8_t bval = 0;
        int sck = LOW;

        int8_t bit = 8;
        while (bit) {
            bit--;
            digitalWrite(CYD28_TouchR_MOSI, ((val & (1 << bit)) ? HIGH : LOW)); // Write bit
            wait(del);
            sck ^= 1u;
            digitalWrite(CYD28_TouchR_CLK, sck);
            /* ... Read bit */
            bval = digitalRead(CYD28_TouchR_MISO);
            out <<= 1;
            out |= bval;
            wait(del);
            sck ^= 1u;
            digitalWrite(CYD28_TouchR_CLK, sck);
        }
        return out;
    } else {
        uint8_t out = _pspi->transfer(val);
        return out;
    }
}
// ------------------------------------------------------------
uint16_t CYD28_TouchR::transfer16(uint16_t data) {
    union {
        uint16_t val;
        struct {
            uint8_t lsb;
            uint8_t msb;
        };
    } in, out;
    in.val = data;

    if (_pspi == nullptr) {
        out.msb = transfer(in.msb);
        out.lsb = transfer(in.lsb);
        return out.val;
    } else {
        // out.msb =_pspi->transfer(in.msb);
        // out.lsb =_pspi->transfer(in.lsb);
        return _pspi->transfer16(data);
    }
}
// ------------------------------------------------------------
void CYD28_TouchR::wait(uint_fast8_t del) {
    for (uint_fast8_t i = 0; i < del; i++) { asm volatile("nop"); }
}
// ------------------------------------------------------------
CYD28_TS_Point CYD28_TouchR::getPointScaled() {
    update();
    uint16_t x = xraw, y = yraw;
    convertRawXY(&x, &y);
    return CYD28_TS_Point(x, y, zraw);
}
// ------------------------------------------------------------
CYD28_TS_Point CYD28_TouchR::getPointRaw() {
    update();
    return CYD28_TS_Point(xraw, yraw, zraw);
}
// ------------------------------------------------------------
bool CYD28_TouchR::touched() {
    update();
    return ((zraw >= threshold) && isrWake);
}
// ------------------------------------------------------------
void CYD28_TouchR::readData(uint16_t *x, uint16_t *y, uint8_t *z) {
    update();
    *x = xraw;
    *y = yraw;
    *z = zraw;
}
// ------------------------------------------------------------
static int16_t besttwoavg(int16_t x, int16_t y, int16_t z) {
    int16_t da, db, dc;
    int16_t reta = 0;
    if (x > y) da = x - y;
    else da = y - x;
    if (x > z) db = x - z;
    else db = z - x;
    if (z > y) dc = z - y;
    else dc = y - z;

    if (da <= db && da <= dc) reta = (x + y) >> 1;
    else if (db <= da && db <= dc) reta = (x + z) >> 1;
    else reta = (y + z) >> 1;

    return (reta);
}
// ------------------------------------------------------------
void CYD28_TouchR::update() {
    int16_t data[6];
    int z;
    if (!isrWake) return;
    if (!_started) return;
    uint32_t now = millis();
    if (now - msraw < MSEC_THRESHOLD) return;

    if (_pspi != nullptr) _pspi->beginTransaction(SPI_SETTING);

    digitalWrite(CYD28_TouchR_CS, LOW);

    transfer(0xB1 /* Z1 */);
    int16_t z1 = transfer16(0xC1 /* Z2 */) >> 3;
    z = z1 + 4095;
    int16_t z2 = transfer16(0x91 /* X */) >> 3;
    z -= z2;
    if (z >= threshold) {
        transfer16(0x91 /* X */); // dummy X measure, 1st is always noisy
        data[0] = transfer16(0xD1 /* Y */) >> 3;
        data[1] = transfer16(0x91 /* X */) >> 3; // make 3 x-y measurements
        data[2] = transfer16(0xD1 /* Y */) >> 3;
        data[3] = transfer16(0x91 /* X */) >> 3;
    } else data[0] = data[1] = data[2] = data[3] = 0;
    data[4] = transfer16(0xD0 /* Y */) >> 3;
    data[5] = transfer16(0) >> 3;

    digitalWrite(CYD28_TouchR_CS, HIGH);

    if (_pspi != nullptr) _pspi->endTransaction();

    if (z < 0) z = 0;
    if (z < threshold) {
        zraw = 0;
        if (z < CYD28_TouchR_Z_THRES_INT) {
#if CYD28_TouchR_IRQ >= 0
            isrWake = false;
#endif
        }
        return;
    }
    zraw = z;

    int16_t x = besttwoavg(data[0], data[2], data[4]);
    int16_t y = besttwoavg(data[1], data[3], data[5]);

    if (z >= threshold) {
        msraw = now; // good read completed, set wait
        xraw = x;
        yraw = y;
        // log_i("xraw= %d, yraw= %d", xraw, yraw);
    }
}
// ------------------------------------------------------------
void CYD28_TouchR::convertRawXY(uint16_t *x, uint16_t *y) {
    uint16_t x_tmp = *x, y_tmp = *y, xx, yy;

    if (!touchCalibration_rotate) {
        xx = (x_tmp - touchCalibration_x0) * sizeX_px / (touchCalibration_x1 - touchCalibration_x0);
        yy = (y_tmp - touchCalibration_y0) * sizeY_px / (touchCalibration_y1 - touchCalibration_y0);
        if (touchCalibration_invert_x) xx = sizeX_px - xx;
        if (touchCalibration_invert_y) yy = sizeY_px - yy;
    } else {
        xx = (y_tmp - touchCalibration_x0) * sizeX_px / (touchCalibration_x1 - touchCalibration_x0);
        yy = (x_tmp - touchCalibration_y0) * sizeY_px / (touchCalibration_y1 - touchCalibration_y0);
        if (touchCalibration_invert_x) xx = sizeX_px - xx;
        if (touchCalibration_invert_y) yy = sizeY_px - yy;
    }
    if (xx > sizeX_px) xx = sizeX_px - 1;
    if (yy > sizeY_px) yy = sizeY_px - 1;
    *x = xx;
    *y = yy;
}

/***************************************************************************************
** Function name:           setTouch
** Description:             imports calibration parameters for touchscreen.
***************************************************************************************/
void CYD28_TouchR::setTouch(uint16_t *parameters) {
    touchCalibration_x0 = parameters[0];
    touchCalibration_x1 = parameters[1];
    touchCalibration_y0 = parameters[2];
    touchCalibration_y1 = parameters[3];

    if (touchCalibration_x0 == 0) touchCalibration_x0 = 1;
    if (touchCalibration_x1 == 0) touchCalibration_x1 = 1;
    if (touchCalibration_y0 == 0) touchCalibration_y0 = 1;
    if (touchCalibration_y1 == 0) touchCalibration_y1 = 1;

    touchCalibration_rotate = parameters[4] & 0x01;
    touchCalibration_invert_x = parameters[4] & 0x02;
    touchCalibration_invert_y = parameters[4] & 0x04;
}
