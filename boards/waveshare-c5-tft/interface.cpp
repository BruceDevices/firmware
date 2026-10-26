#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <interface.h>

#define SEL_BTN 28

#define DW_BTN 1
#define L_BTN 14
#define R_BTN 13
#define UP_BTN 0

static DeviceButtons buttonsCfg() { return DeviceButtons{L_BTN, R_BTN, UP_BTN, DW_BTN, SEL_BTN}; }

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.i2c_bus = {(gpio_num_t)4, (gpio_num_t)5}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 4;
    bruceConfigPins.rfRx = 5;
    bruceConfigPins.irTx = 7;
    bruceConfigPins.irRx = 26;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)12, (gpio_num_t)11}; // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)4, (gpio_num_t)5};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)4, (gpio_num_t)5}; // rx, tx
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)8, (gpio_num_t)3};
    bruceConfigPins.PN532_bus = {(gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)8, (gpio_num_t)3};
    // CC1101/NRF24/SDCARD/W5500 share the main SPI bus (sck=10, miso=9, mosi=8)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)8, (gpio_num_t)3, (gpio_num_t)2, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)8, (gpio_num_t)3, (gpio_num_t)2
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)8, (gpio_num_t)6
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)10, (gpio_num_t)9, (gpio_num_t)8, (gpio_num_t)3, (gpio_num_t)2, GPIO_NUM_NC
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

#ifdef HAS_5_BUTTONS
    hal_buttons_init(buttonsCfg(), 5);
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
void _post_setup_gpio() {}

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
void InputHandler(void) { hal_buttons_poll_5(buttonsCfg()); }

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
