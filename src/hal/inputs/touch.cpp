#include "touch.h"

#include "DisplayConfig.h" // displayConfig.width/height -- see panelSize() below
#include "globals.h"

#include "core/powerSave.h"
#include "core/utils.h"

// The touch controller's coordinate range is the physical panel, full stop
// -- it has no notion of the footer bar main.cpp/settings.cpp carve out of
// tftHeight for the nav-hint strip (HAS_TOUCH && !HAS_TOUCH_NO_BORDER).
// Using tftWidth/tftHeight here (as this file used to, via caller-supplied
// parameters) silently shrank the touch-side height by that footer on every
// board with one, throwing off any MirrorX/MirrorY math by exactly that
// many pixels. displayConfig.width/height ("visible panel size, unrotated")
// is the board-fundamental constant that doesn't have this problem;
// swapping the two per rotation replicates the same swap main.cpp does for
// tftWidth/tftHeight, just without the footer subtraction.
static void panelSize(int16_t &w, int16_t &h) {
    if (bruceConfigPins.rotation & 1) {
        w = displayConfig.height;
        h = displayConfig.width;
    } else {
        w = displayConfig.width;
        h = displayConfig.height;
    }
}

#if defined(TOUCH_CTRL_XPT2046)
#include "CYD28_TouchscreenR.h"
#include <SPI.h>
#ifndef CYD28_DISPLAY_HOR_RES_MAX
#define CYD28_DISPLAY_HOR_RES_MAX 320
#endif
#ifndef CYD28_DISPLAY_VER_RES_MAX
#define CYD28_DISPLAY_VER_RES_MAX 240
#endif
// Named `touch` (external linkage, not `static`) on purpose: settings.cpp's
// calibrateTouch()/loadTouchCalibration() (HAS_RESISTIVE_TOUCH) still do
// `extern CYD28_TouchR touch;` to reach the raw setTouch()/touched()/
// getPointRaw() calibration API, same as every un-migrated XPT2046 board's
// own interface.cpp used to provide. Keep this name until every
// TOUCH_CTRL_XPT2046 board is migrated and that calibration code is updated
// to go through a HAL entry point instead.
CYD28_TouchR touch(CYD28_DISPLAY_HOR_RES_MAX, CYD28_DISPLAY_VER_RES_MAX);

#elif defined(TOUCH_CTRL_GT911)
#include <TouchDrvGT911.hpp>
#include <Wire.h>
static TouchDrvGT911 touch;
static uint8_t touchLastRot = 0xFF;

// Drives RST through cfg.reset_cb when set (touch reset not a raw ESP32
// GPIO, e.g. behind an IO expander -- see DeviceTouch.reset_cb), else the
// raw pin_rst GPIO as before.
static void gt911WriteRst(const DeviceTouch &cfg, bool level) {
    if (cfg.reset_cb) cfg.reset_cb(level);
    else digitalWrite(cfg.pin_rst, level);
}

static bool gt911ResetAndSync(const DeviceTouch &cfg, uint8_t i2c_addr) {
    if ((cfg.pin_rst < 0 && !cfg.reset_cb) || cfg.pin_irq < 0) return false;

    // Select the address during reset, then synchronize INT before the controller starts scanning.
    // 在复位期间选择地址，再同步 INT，使控制器开始触摸扫描。
    if (cfg.pin_rst >= 0) {
        gpio_hold_dis(static_cast<gpio_num_t>(cfg.pin_rst));
        pinMode(cfg.pin_rst, OUTPUT);
    }
    gpio_hold_dis(static_cast<gpio_num_t>(cfg.pin_irq));
    pinMode(cfg.pin_irq, OUTPUT);
    gt911WriteRst(cfg, LOW);
    delay(20);
    digitalWrite(cfg.pin_irq, i2c_addr == 0x14 ? HIGH : LOW);
    delay(1);
    gt911WriteRst(cfg, HIGH);
    delay(10);
    digitalWrite(cfg.pin_irq, LOW);
    delay(50);
    pinMode(cfg.pin_irq, INPUT);
    delay(10);
    return true;
}

#elif defined(TOUCH_CTRL_CST8XX)
#include <TouchDrvCSTXXX.hpp>
#include <Wire.h>
static TouchDrvCSTXXX touch;
static uint8_t touchLastRot = 0xFF;

#elif defined(TOUCH_CTRL_FT6X36)
#include <TouchDrvFT6X36.hpp>
#include <Wire.h>
static TouchDrvFT6X36 touch;

#elif defined(TOUCH_CTRL_GT9895)
#include <TouchDrvGT9895.hpp>
#include <Wire.h>
static TouchDrvGT9895 touch;
static uint8_t touchLastRot = 0xFF;

#elif defined(TOUCH_CTRL_HI8561)
#include <TouchDrvHI8561.hpp>
#include <Wire.h>
static TouchDrvHI8561 touch;
static uint8_t touchLastRot = 0xFF;
#endif

#if defined(TOUCH_CTRL_GT911) || defined(TOUCH_CTRL_CST8XX) || defined(TOUCH_CTRL_FT6X36) ||                 \
    defined(TOUCH_CTRL_GT9895) || defined(TOUCH_CTRL_HI8561)
#include <Wire.h>
static TwoWire &wireFor(const DeviceTouch &cfg) {
    return cfg.i2c_bus ? *static_cast<TwoWire *>(cfg.i2c_bus) : Wire;
}

// Pulses RST through cfg.reset_cb when the touch controller's reset line
// isn't a raw ESP32 GPIO (see DeviceTouch.reset_cb). No-op if unset -- the
// chip driver's own setPins()-based reset (raw pin_rst) handles that case.
static void touchResetPulse(const DeviceTouch &cfg) {
    if (!cfg.reset_cb) return;
    cfg.reset_cb(LOW);
    delay(10);
    cfg.reset_cb(HIGH);
    delay(50);
}
#endif

bool hal_touch_init(const DeviceTouch &cfg, uint8_t i2c_addr, bool xpt_shared_spi) {
#if defined(TOUCH_CTRL_XPT2046)
    (void)i2c_addr;
    // Shared with the display: reuse its SPI instance (both sides take the SPI transaction lock)
#if !defined(TFT_PARALLEL_8_BIT) // parallel displays have no SPI instance to share
    if (xpt_shared_spi) return touch.begin(&tft.getSPIinstance());
#else
    (void)xpt_shared_spi;
#endif
    if (cfg.spi_bus) return touch.begin(static_cast<SPIClass *>(cfg.spi_bus));
    return touch.begin(); // bit-banged on its own pins
#elif defined(TOUCH_CTRL_GT911)
    (void)xpt_shared_spi;
    if (cfg.pin_sda >= 0 && cfg.pin_scl >= 0) wireFor(cfg).begin(cfg.pin_sda, cfg.pin_scl);
    if (cfg.gt911_int_sync) {
        if (!gt911ResetAndSync(cfg, i2c_addr)) return false;

        // Preserve the synchronized state while SensorLib probes and reads the native resolution.
        // 在 SensorLib 探测芯片并读取原生分辨率期间保持已同步状态。
        touch.setPins(-1, -1);
        const bool ready = touch.begin(wireFor(cfg), i2c_addr);
        touch.setPins(cfg.reset_cb ? -1 : cfg.pin_rst, cfg.pin_irq);
        return ready;
    }
    // Not the int-sync path (no address-select dance needed): a reset_cb
    // still needs its own pulse here since SensorLib's begin() only knows
    // how to drive a raw pin_rst GPIO.
    if (cfg.reset_cb) {
        gt911WriteRst(cfg, LOW);
        delay(10);
        gt911WriteRst(cfg, HIGH);
        delay(50);
    }
    touch.setPins(cfg.reset_cb ? -1 : cfg.pin_rst, cfg.pin_irq);
    return touch.begin(wireFor(cfg), i2c_addr);
#elif defined(TOUCH_CTRL_CST8XX)
    (void)xpt_shared_spi;
    if (cfg.pin_sda >= 0 && cfg.pin_scl >= 0) wireFor(cfg).begin(cfg.pin_sda, cfg.pin_scl);
    touchResetPulse(cfg);
    touch.setPins(cfg.reset_cb ? -1 : cfg.pin_rst, cfg.pin_irq);
    if (cfg.cst8xx_model >= 0) touch.setTouchDrvModel(static_cast<TouchDrvType>(cfg.cst8xx_model));
    if (touch.begin(wireFor(cfg), i2c_addr, cfg.pin_sda, cfg.pin_scl)) return true;
    return touch.begin(wireFor(cfg), CST816_SLAVE_ADDRESS, cfg.pin_sda, cfg.pin_scl);
#elif defined(TOUCH_CTRL_FT6X36)
    (void)xpt_shared_spi;
    if (cfg.pin_sda >= 0 && cfg.pin_scl >= 0) wireFor(cfg).begin(cfg.pin_sda, cfg.pin_scl);
    touchResetPulse(cfg);
    touch.setPins(cfg.reset_cb ? -1 : cfg.pin_rst, cfg.pin_irq);
    // Most FT6X36-family boards answer on SensorLib's own default (0x38);
    // a few (e.g. the FT6336U on seeedstudio-sensecap) ship at 0x48 instead
    // -- try whatever the board passed in first, then fall back to the
    // family default so existing boards that never override it keep working.
    if (!touch.begin(wireFor(cfg), i2c_addr, cfg.pin_sda, cfg.pin_scl) &&
        !touch.begin(wireFor(cfg), FT6X36_SLAVE_ADDRESS, cfg.pin_sda, cfg.pin_scl))
        return false;
    Serial.printf("[FT6X36] model: %s\n", touch.getModelName());
    // hal_touch_read() polls (no IRQ line wired into the read path) -- every
    // SensorLib example for this chip calls this right after begin() for
    // that reason.
    touch.interruptPolling();
    return true;
#elif defined(TOUCH_CTRL_GT9895)
    (void)xpt_shared_spi;
    if (cfg.pin_sda >= 0 && cfg.pin_scl >= 0) wireFor(cfg).begin(cfg.pin_sda, cfg.pin_scl);
    touchResetPulse(cfg);
    touch.setPins(cfg.reset_cb ? -1 : cfg.pin_rst, cfg.pin_irq);
    if (!touch.begin(wireFor(cfg), i2c_addr, cfg.pin_sda, cfg.pin_scl)) return false;
    // Chip doesn't report its own native resolution -- only scale if the
    // board supplied one (see DeviceTouch.raw_width/raw_height).
    if (cfg.raw_width != 0 && cfg.raw_height != 0) touch.setResolution(cfg.raw_width, cfg.raw_height);
    return true;
#elif defined(TOUCH_CTRL_HI8561)
    (void)xpt_shared_spi;
    (void)i2c_addr; // fixed HI8561_SLAVE_ADDRESS, like FT6X36's fixed address
    if (cfg.pin_sda >= 0 && cfg.pin_scl >= 0) wireFor(cfg).begin(cfg.pin_sda, cfg.pin_scl);
    touchResetPulse(cfg);
    touch.setPins(cfg.reset_cb ? -1 : cfg.pin_rst, cfg.pin_irq);
    return touch.begin(wireFor(cfg), HI8561_SLAVE_ADDRESS, cfg.pin_sda, cfg.pin_scl);
#else
    (void)cfg;
    (void)i2c_addr;
    (void)xpt_shared_spi;
    return false;
#endif
}

bool hal_touch_read(const DeviceTouch &cfg, BruceTouchPoint &out) {
#if defined(TOUCH_CTRL_XPT2046) || defined(TOUCH_CTRL_GT911) || defined(TOUCH_CTRL_CST8XX) ||                \
    defined(TOUCH_CTRL_FT6X36) || defined(TOUCH_CTRL_GT9895) || defined(TOUCH_CTRL_HI8561)
    uint8_t r = bruceConfigPins.rotation & 0x03;
    int16_t screenW, screenH;
    panelSize(screenW, screenH);
#endif

    // XPT2046/FT6X36 give a raw, unrotated point -- the mirror/swap math
    // below is applied on the host side. GT911/CST8xx hand rotation to the
    // driver itself instead (setSwapXY/setMirrorXY), see the other half of
    // this function.
#if defined(TOUCH_CTRL_XPT2046)
    if (!touch.touched()) return false;
    auto p = touch.getPointScaled();
    int16_t x = p.x, y = p.y;
#elif defined(TOUCH_CTRL_FT6X36)
    int16_t x = 0, y = 0;
    if (!touch.getPoint(&x, &y, 1)) return false;
#endif
#if defined(TOUCH_CTRL_XPT2046) || defined(TOUCH_CTRL_FT6X36)
    if (cfg.SwapXY[r]) {
        int16_t t = x;
        x = y;
        y = t;
    }
    if (cfg.MirrorX[r]) x = screenW - x;
    if (cfg.MirrorY[r]) y = screenH - y;
    out.x = x;
    out.y = y;
    out.pressed = true;
#if !defined(TOUCH_CTRL_XPT2046)
    touch.reset();
#endif
    return true;
#elif defined(TOUCH_CTRL_GT911) || defined(TOUCH_CTRL_CST8XX) || defined(TOUCH_CTRL_GT9895) ||               \
    defined(TOUCH_CTRL_HI8561)
    if (touchLastRot != r) {
#if defined(TOUCH_CTRL_GT9895)
        // GT9895's digitizer grid is bigger than the panel -- if the board
        // gave us its native resolution, use it to scale instead of just
        // clamping (see DeviceTouch.raw_width/raw_height).
        if (cfg.raw_width != 0 && cfg.raw_height != 0) touch.setTargetResolution(screenW, screenH);
        else touch.setMaxCoordinates(screenW, screenH);
#else
        touch.setMaxCoordinates(screenW, screenH);
#endif
        touch.setSwapXY(cfg.SwapXY[r]);
        touch.setMirrorXY(cfg.MirrorX[r], cfg.MirrorY[r]);
        touchLastRot = r;
    }
    int16_t x = 0, y = 0;
    if (!touch.getPoint(&x, &y, 1)) return false;
    out.x = x;
    out.y = y;
    out.pressed = true;
    touch.reset();
    return true;
#else
    (void)cfg;
    (void)out;
    return false;
#endif
}

bool hal_touch_read_raw(BruceTouchPoint &out) {
#if defined(TOUCH_CTRL_XPT2046)
    if (!touch.touched()) return false;
    auto p = touch.getPointScaled();
    out.x = p.x;
    out.y = p.y;
    out.pressed = true;
    return true;
#elif defined(TOUCH_CTRL_GT911) || defined(TOUCH_CTRL_CST8XX) || defined(TOUCH_CTRL_FT6X36) ||               \
    defined(TOUCH_CTRL_GT9895) || defined(TOUCH_CTRL_HI8561)
    int16_t x = 0, y = 0;
    if (!touch.getPoint(&x, &y, 1)) return false;
    out.x = x;
    out.y = y;
    out.pressed = true;
    touch.reset();
    return true;
#else
    (void)out;
    return false;
#endif
}

bool hal_touch_get_resolution(uint16_t &width, uint16_t &height) {
#if defined(TOUCH_CTRL_GT911) || defined(TOUCH_CTRL_CST8XX) || defined(TOUCH_CTRL_GT9895) ||                 \
    defined(TOUCH_CTRL_HI8561)
    width = touch.getResolutionX();
    height = touch.getResolutionY();
    return width != 0 && height != 0;
#else
    (void)width;
    (void)height;
    return false;
#endif
}

bool hal_touch_apply(const BruceTouchPoint &t, bool wakeUp) {
    if (wakeUp) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return false;
    } else {
        AnyKeyPress = true;
    }

    touchPoint.x = t.x;
    touchPoint.y = t.y;
    touchPoint.pressed = true;
    touchHeatMap(touchPoint);
    return true;
}

void hal_touch_set_home_button(int16_t x, int16_t y, void (*cb)(void *user_data), void *user_data) {
#if defined(TOUCH_CTRL_CST8XX) || defined(TOUCH_CTRL_GT911)
    touch.setCenterButtonCoordinate(x, y);
    touch.setHomeButtonCallback(cb, user_data);
#else
    (void)x;
    (void)y;
    (void)cb;
    (void)user_data;
#endif
}

void hal_touch_disable_auto_sleep() {
#if defined(TOUCH_CTRL_CST8XX)
    touch.disableAutoSleep();
#endif
}

void hal_touch_set_threshold(uint8_t value) {
#if defined(TOUCH_CTRL_FT6X36)
    touch.setThreshold(value);
#else
    (void)value;
#endif
}
