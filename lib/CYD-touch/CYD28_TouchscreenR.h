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

#include <Arduino.h>
#include <SPI.h>

#define CYD28_TouchR_Z_THRESH       300
#define CYD28_TouchR_Z_THRES_INT  75

// These definitions come from https://github.com/rzeldent/platformio-espressif32-sunton board definitions
#if defined(TOUCH_XPT2046_SPI)
  #define CYD28_TouchR_IRQ    XPT2046_TOUCH_CONFIG_INT_GPIO_NUM
  #define CYD28_TouchR_MOSI   XPT2046_SPI_BUS_MOSI_IO_NUM
  #define CYD28_TouchR_MISO   XPT2046_SPI_BUS_MISO_IO_NUM
  #define CYD28_TouchR_CLK    XPT2046_SPI_BUS_SCLK_IO_NUM
  #define CYD28_TouchR_CS     XPT2046_SPI_CONFIG_CS_GPIO_NUM
#else
  #define CYD28_TouchR_IRQ  36
  #define CYD28_TouchR_MOSI   32
  #define CYD28_TouchR_MISO   39
  #define CYD28_TouchR_CLK  25
  #define CYD28_TouchR_CS   33

#endif
// CALIBRAION VALUES
#define CYD28_TouchR_CAL_XMIN 185
#define CYD28_TouchR_CAL_XMAX 3700
#define CYD28_TouchR_CAL_YMIN 280
#define CYD28_TouchR_CAL_YMAX 3850


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
    : _delay(2), sizeX_px(w), sizeY_px(h){ }
  bool begin();
  bool begin(SPIClass *tspi);

  CYD28_TS_Point getPointScaled();
  CYD28_TS_Point getPointRaw();
  bool touched();
  void readData(uint16_t *x, uint16_t *y, uint8_t *z);
  void setRotation(uint8_t n) { rotation = n % 4; }
    void setThreshold(uint16_t th) { threshold = th;}

  // Runtime calibration (overrides the compile-time CYD28_TouchR_CAL_* defaults).
  // swap = true when the panel's raw X axis runs along the screen's vertical axis.
  void setCalibration(int16_t xmin, int16_t xmax, int16_t ymin, int16_t ymax, bool swap = false) {
    if (xmax == xmin || ymax == ymin) return;
    calXmin = xmin; calXmax = xmax; calYmin = ymin; calYmax = ymax; swapXY = swap;
  }

  volatile bool isrWake=true;

private:
  void update();
  uint8_t transfer(uint8_t);
  uint16_t transfer16(uint16_t data);
  void wait(uint_fast8_t del);
  void convertRawXY(int16_t *x, int16_t *y);
  uint8_t rotation=1;
  int16_t calXmin = CYD28_TouchR_CAL_XMIN, calXmax = CYD28_TouchR_CAL_XMAX;
  int16_t calYmin = CYD28_TouchR_CAL_YMIN, calYmax = CYD28_TouchR_CAL_YMAX;
  bool swapXY = false;
  int16_t xraw=0, yraw=0, zraw=0;
    uint16_t threshold = CYD28_TouchR_Z_THRESH;
  uint32_t msraw=0x80000000;
  uint8_t _delay;
  const  int32_t sizeX_px;
  const int32_t sizeY_px;
  SPIClass *_pspi = nullptr;

};

#endif 
