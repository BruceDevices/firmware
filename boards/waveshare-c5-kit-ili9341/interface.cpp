// Waveshare ESP32-C5-WIFI6-KIT + 2.8" ILI9341 SPI screen with XPT2046 touch.
// Based on boards/ESP32-C5-tft; pins are in waveshare-c5-kit-ili9341.ini, wiring in connections.md.
#include "CYD28_TouchscreenR.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <interface.h>

extern CYD28_TouchR touch; // defined by hal/inputs/touch.cpp
static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    // The XPT2046 reports landscape (rotation 1) coordinates
    // rotation:        0      1      2      3
    const bool swapXY[4] = {true, false, true, false};
    const bool mirrorX[4] = {true, false, false, true};
    const bool mirrorY[4] = {false, false, true, true};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

// The screen, touch, SD card and radio modules all share one SPI bus (SCK 1, MISO 7, MOSI 2).
static void deselect(int pin) {
    if (pin < 0) return;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // Defaults only: the user can re-map these in Config, and /brucePins.conf wins from the second boot.
    // I2C, GPS and CH9329 use GPIO24/25: GPIO4/5 (the stock C5 choice) are TFT_RST / TFT_CS here.
    bruceConfigPins.i2c_bus = {(gpio_num_t)24, (gpio_num_t)25}; // sda, scl
    bruceConfigPins.rfTx = 24;
    bruceConfigPins.rfRx = 25;
    bruceConfigPins.irTx = 0; // GPIO3 (the stock C5 choice) is TFT_DC here
    bruceConfigPins.irRx = 26;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)12, (gpio_num_t)11};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)24, (gpio_num_t)25};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)24, (gpio_num_t)25}; // rx, tx (CH9329)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)1, (gpio_num_t)7, (gpio_num_t)2, (gpio_num_t)9};
    bruceConfigPins.PN532_bus = {(gpio_num_t)1, (gpio_num_t)7, (gpio_num_t)2, (gpio_num_t)9};
    // CC1101/NRF24/W5500 share one chip select (GPIO9) and one signal pin (GPIO8): fit one at a time
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)1, (gpio_num_t)7, (gpio_num_t)2, (gpio_num_t)9, (gpio_num_t)8, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)1, (gpio_num_t)7, (gpio_num_t)2, (gpio_num_t)9, (gpio_num_t)8
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)1, (gpio_num_t)7, (gpio_num_t)2, (gpio_num_t)10
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)1, (gpio_num_t)7, (gpio_num_t)2, (gpio_num_t)9, (gpio_num_t)8, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
#endif

    // Deselect everything on the shared bus before the screen starts
    deselect(TFT_CS);
    deselect(TOUCH_CS);
    deselect(bruceConfigPins.SDCARD_bus.cs);
    deselect(bruceConfigPins.CC1101_bus.cs);
    deselect(bruceConfigPins.NRF24_bus.cs);
#if !defined(LITE_VERSION)
    deselect(bruceConfigPins.W5500_bus.cs);
#endif

    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, HIGH); // TFT_eSPI pulses it low when the display starts

    bruceConfig.colorInverted = 0; // plain (non-IPS) ILI9341
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
    hal_touch_init(touchCfg(), 0, true); // shares the display SPI bus
    // calibration: loadTouchCalibration()/calibrateTouch() in main.cpp (NVS "touch_cal")
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;
    checkPowerSaveTime();
    BruceTouchPoint t;
    if (hal_touch_read(touchCfg(), t)) {
        tm = millis();
        hal_touch_apply(t);
    }
}
