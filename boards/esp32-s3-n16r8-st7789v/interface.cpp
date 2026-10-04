#include "CYD28_TouchscreenR.h"
#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <Arduino.h>
#include <interface.h>

extern CYD28_TouchR touch; // defined by hal/inputs/touch.cpp

static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    // The XPT2046 reports rotation 2 coordinates
    // rotation:        0      1      2      3
    const bool swapXY[4] = {true, false, true, false};
    const bool mirrorX[4] = {false, false, true, true};
    const bool mirrorY[4] = {false, true, true, false};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

void _setup_gpio() {
    bruceConfigPins.irTx = -1;
    bruceConfigPins.irRx = -1;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.badusb_bus = {(gpio_num_t)-1, (gpio_num_t)-1}; // rx, tx (none available)
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)21, (gpio_num_t)38, (gpio_num_t)18, (gpio_num_t)-1
    }; // sck,miso,mosi,cs
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)21, (gpio_num_t)38, (gpio_num_t)18, (gpio_num_t)15};
    bruceConfigPins.PN532_bus = {(gpio_num_t)21, (gpio_num_t)38, (gpio_num_t)18, (gpio_num_t)15};

    pinMode(XPT2046_SPI_CONFIG_CS_GPIO_NUM, OUTPUT);
    digitalWrite(XPT2046_SPI_CONFIG_CS_GPIO_NUM, HIGH);
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
}

void _post_setup_gpio() {
    // Use software SPI (GPIO bit-banging) for touch to avoid conflicts with AUX_SPI
    // CYD28_TouchR::begin() with no arguments uses software SPI mode on the defined GPIO pins
    if (!hal_touch_init(touchCfg(), 0, TFT_MOSI == CYD28_TouchR_MOSI)) {
        Serial.println("Touchscreen initialization failed!");
    }
}

int getBattery() { return 0; }

bool isCharging() { return false; }

void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

void InputHandler(void) {
    static unsigned long lastTouch = 0;
    if (millis() - lastTouch > 200 || LongPress) {
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            lastTouch = millis();
            hal_touch_apply(t);
        }
    }
}

void powerOff() {}

void checkReboot() {}
