#include "CYD28_TouchscreenR.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <interface.h>

#define BTN_ACT LOW
#define BTN_PIN 0

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

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.rotation = 1;
    bruceConfigPins.irTx = 40;
    bruceConfigPins.i2c_bus = {(gpio_num_t)8, (gpio_num_t)9}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 8;
    bruceConfigPins.rfRx = 9;
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43}; // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};  // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)9, (gpio_num_t)8}; // rx, tx (Grove, shares I2C pins)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10};
    // No dedicated PN532 pins on this board; PN532_bus shares the same slot as outer_bus
    bruceConfigPins.PN532_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10};
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9
    }; // sck,miso,mosi,cs,gdo0
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)14, (gpio_num_t)16
    }; // sck,miso,mosi,cs(ss),ce
    // No SD card by default on this variant
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)4};

    // Keep XPT2046 CS high until needed
    pinMode(XPT2046_SPI_CONFIG_CS_GPIO_NUM, OUTPUT);
    digitalWrite(XPT2046_SPI_CONFIG_CS_GPIO_NUM, HIGH);

    bruceConfig.colorInverted = 1;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup — runs AFTER tft.init()
***************************************************************************************/
void _post_setup_gpio() {
    // Initialize XPT2046 touch using hardware SPI (shares bus with display)
    // Initialize XPT2046 touch using hardware SPI (shares bus with display)
    if (!hal_touch_init(touchCfg(), 0, TFT_MOSI == CYD28_TouchR_MOSI)) {
        Serial.println("[TOUCH] XPT2046 not started");
    } else {
        Serial.println("[TOUCH] XPT2046 started OK");
    }

    // Backlight on
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { return 0; }

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
bool isCharging() { return false; }

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
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            d_tmp = millis();
            hal_touch_apply(t);
        }
    }

    checkPowerSaveTime();
    if (digitalRead(BTN_PIN) == BTN_ACT) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        SelPress = true;
        long tmp = millis();
        while ((millis() - tmp) < 200 && digitalRead(BTN_PIN) == BTN_ACT);
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { esp_deep_sleep_start(); }

/*********************************************************************
** Function: goToDeepSleep
** location: mykeyboard.cpp
** Puts the device into DeepSleep
**********************************************************************/
void goToDeepSleep() { esp_deep_sleep_start(); }

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device
**********************************************************************/
void checkReboot() {}
