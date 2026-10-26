#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include "CYD28_TouchscreenR.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/inputs/buttons.h"
#include <interface.h>

#ifdef HAS_3_BUTTONS
static DeviceButtons buttonsCfg() { return DeviceButtons{UP_BTN, DW_BTN, SEL_BTN}; }
#endif

#ifdef HAS_TOUCH
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
#endif

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.i2c_bus = {(gpio_num_t)9, (gpio_num_t)8}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 8;
    bruceConfigPins.rfRx = 9;
    bruceConfigPins.irTx = 8;
    bruceConfigPins.irRx = 9;
    bruceConfigPins.rotation = 3;
    bruceConfigPins.uart_bus = {(gpio_num_t)12, (gpio_num_t)11}; // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)4, (gpio_num_t)5};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)4, (gpio_num_t)5}; // rx, tx
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)6, (gpio_num_t)2, (gpio_num_t)7, (gpio_num_t)9};
    bruceConfigPins.PN532_bus = {(gpio_num_t)6, (gpio_num_t)2, (gpio_num_t)7, (gpio_num_t)9};
    // CC1101/NRF24/SDCARD/W5500 share the main SPI bus (sck=6, miso=2, mosi=7)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)6, (gpio_num_t)2, (gpio_num_t)7, (gpio_num_t)9, (gpio_num_t)8, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)6, (gpio_num_t)2, (gpio_num_t)7, (gpio_num_t)9, (gpio_num_t)8
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)6, (gpio_num_t)2, (gpio_num_t)7, (gpio_num_t)10
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)6, (gpio_num_t)2, (gpio_num_t)7, (gpio_num_t)9, (gpio_num_t)8, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
#endif

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(TFT_MOSI, OUTPUT);
    digitalWrite(TFT_MOSI, HIGH);
    pinMode(TFT_SCLK, OUTPUT);

    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
    pinMode(TFT_RST, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);

#ifdef HAS_3_BUTTONS
    hal_buttons_init(buttonsCfg(), 3);
#endif
    pinMode(bruceConfigPins.NRF24_bus.cs, OUTPUT);
    pinMode(bruceConfigPins.CC1101_bus.cs, OUTPUT);
    pinMode(bruceConfigPins.SDCARD_bus.cs, OUTPUT);
    pinMode(TFT_CS, OUTPUT);

    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
    digitalWrite(bruceConfigPins.CC1101_bus.cs, HIGH);
    digitalWrite(bruceConfigPins.SDCARD_bus.cs, HIGH);
    digitalWrite(TFT_CS, HIGH);
#if !defined(LITE_VERSION)
    pinMode(bruceConfigPins.W5500_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.W5500_bus.cs, HIGH);
#endif
}
/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
#ifdef HAS_TOUCH
    hal_touch_init(touchCfg(), 0, true); // shares the display SPI bus
#endif
    bruceConfigPins.gps_bus.rx = (gpio_num_t)4;
    bruceConfigPins.gps_bus.tx = (gpio_num_t)5;
    bruceConfigPins.gpsBaudrate = 9600;
    bruceConfigPins.rfTx = 8;
    bruceConfigPins.rfRx = 9;

    // Force disable color inversion for ST7789/ILI9341 on nm-cyd-c5.
    // Must be done here because begin_storage() loads bruceConf.json which may
    // override the value set in _setup_gpio().
#ifdef ST7789_DRIVER
    bruceConfig.colorInverted = 0;
    tft.invertDisplay(0);
#endif
#ifdef ILI9341_DRIVER
    bruceConfig.colorInverted = 0;
    tft.invertDisplay(0);
#endif

    // Force set I2C bus pins for nm-cyd-c5.
    // brucePins.conf may have stale/wrong i2c_bus values, so override them
    // to ensure PN532 and other I2C devices use the correct GPIO9(SDA)/GPIO8(SCL).
    bruceConfigPins.i2c_bus.sda = (gpio_num_t)9;
    bruceConfigPins.i2c_bus.scl = (gpio_num_t)8;
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { return 0; }

/***************************************************************************************
** Function name: isCharging()
** Description:   Default implementation that returns false
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
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;
#ifdef HAS_TOUCH
    checkPowerSaveTime();
    {
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            tm = millis();
            if (!hal_touch_apply(t)) return;
        }
    }
#endif
#ifdef HAS_3_BUTTONS
    hal_buttons_poll_3(buttonsCfg());
#endif
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turnoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
