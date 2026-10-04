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
 * ----------------------- CYD28 (2.8" cheap yellow display) version --------------------------
 *   using software bit banged SPI and fixed GPIOs  + manual calibration
 * 08.2023 - Piotr Zapart www.hexefx.com
 *
 *
 */

#ifndef _CYD28_TOUCHSCREENR_H_
#define _CYD28_TOUCHSCREENR_H_

#include "Arduino.h"
#include <SPI.h>

// Bruce boards describe the XPT2046 pins with the esp-idf style TOUCH_XPT2046_SPI macros
// (see boards/*/*.ini); map them onto the CYD28_TouchR_* names used below.
#if defined(TOUCH_XPT2046_SPI)
#ifndef CYD28_TouchR_IRQ
#define CYD28_TouchR_IRQ XPT2046_TOUCH_CONFIG_INT_GPIO_NUM
#endif
#ifndef CYD28_TouchR_MOSI
#define CYD28_TouchR_MOSI XPT2046_SPI_BUS_MOSI_IO_NUM
#endif
#ifndef CYD28_TouchR_MISO
#define CYD28_TouchR_MISO XPT2046_SPI_BUS_MISO_IO_NUM
#endif
#ifndef CYD28_TouchR_CLK
#define CYD28_TouchR_CLK XPT2046_SPI_BUS_SCLK_IO_NUM
#endif
#ifndef CYD28_TouchR_CS
#define CYD28_TouchR_CS XPT2046_SPI_CONFIG_CS_GPIO_NUM
#endif
#endif

#ifndef CYD28_TouchR_Z_THRESH
#define CYD28_TouchR_Z_THRESH 300
#endif
#ifndef CYD28_TouchR_Z_THRES_INT
#define CYD28_TouchR_Z_THRES_INT 75
#endif

#ifndef CYD28_TouchR_IRQ
#define CYD28_TouchR_IRQ 36
#endif
#ifndef CYD28_TouchR_MOSI
#define CYD28_TouchR_MOSI 32
#endif
#ifndef CYD28_TouchR_MISO
#define CYD28_TouchR_MISO 39
#endif
#ifndef CYD28_TouchR_CLK
#define CYD28_TouchR_CLK 25
#endif
#ifndef CYD28_TouchR_CS
#define CYD28_TouchR_CS 33
#endif

// CALIBRAION VALUES
#ifndef CYD28_TouchR_CAL_XMIN
#define CYD28_TouchR_CAL_XMIN 185
#endif
#ifndef CYD28_TouchR_CAL_XMAX
#define CYD28_TouchR_CAL_XMAX 3700
#endif
#ifndef CYD28_TouchR_CAL_YMIN
#define CYD28_TouchR_CAL_YMIN 280
#endif
#ifndef CYD28_TouchR_CAL_YMAX
#define CYD28_TouchR_CAL_YMAX 3850
#endif

#ifndef CYD28_TouchR_ROT
#define CYD28_TouchR_ROT 0
#endif

class CYD28_TS_Point {
public:
    CYD28_TS_Point(void) : x(0), y(0), z(0) {}
    CYD28_TS_Point(int16_t x, int16_t y, int16_t z) : x(x), y(y), z(z) {}
    bool operator==(CYD28_TS_Point p) { return ((p.x == x) && (p.y == y) && (p.z == z)); }
    bool operator!=(CYD28_TS_Point p) { return ((p.x != x) || (p.y != y) || (p.z != z)); }
    int16_t x, y, z;
};

class CYD28_TouchR {
public:
    constexpr CYD28_TouchR(int32_t w, int32_t h)
        : _delay(2), sizeX_px(w), sizeY_px(h), touchCalibration_rotate(CYD28_TouchR_ROT & 0x01),
          touchCalibration_invert_x(CYD28_TouchR_ROT & 0x02),
          touchCalibration_invert_y(CYD28_TouchR_ROT & 0x04) {}
    bool begin();
    bool begin(SPIClass *tspi);

    CYD28_TS_Point getPointScaled();
    CYD28_TS_Point getPointRaw();
    bool touched();
    void readData(uint16_t *x, uint16_t *y, uint8_t *z);
    void setRotation(uint8_t n) { rotation = n % 4; }
    void setThreshold(uint16_t th) { threshold = th; }
    void setTouch(uint16_t *parameters);
    volatile bool isrWake = true;

private:
    bool configurePins(bool hardwareSpi);
    bool setupInterrupt();
    bool probe();
    void update();
    uint8_t transfer(uint8_t);
    uint16_t transfer16(uint16_t data);
    void wait(uint_fast8_t del);
    void convertRawXY(uint16_t *x, uint16_t *y);
    uint8_t rotation = CYD28_TouchR_ROT;
    uint16_t xraw = 0, yraw = 0, zraw = 0;
    uint16_t threshold = CYD28_TouchR_Z_THRESH;
    uint16_t touchCalibration_x0 = CYD28_TouchR_CAL_XMIN, touchCalibration_x1 = CYD28_TouchR_CAL_XMAX,
             touchCalibration_y0 = CYD28_TouchR_CAL_YMIN, touchCalibration_y1 = CYD28_TouchR_CAL_YMAX;
    bool touchCalibration_rotate, touchCalibration_invert_x, touchCalibration_invert_y;
    uint32_t msraw = 0x80000000;
    uint8_t _delay;
    const int32_t sizeX_px;
    const int32_t sizeY_px;
    SPIClass *_pspi = nullptr;
    bool _started = false;
};

#endif
