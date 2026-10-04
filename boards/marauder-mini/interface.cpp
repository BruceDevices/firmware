#include "core/powerSave.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include <interface.h>

#define SEL_BTN 34

#define DW_BTN 35
#define L_BTN 13
#define R_BTN 39
#define UP_BTN 36

static DeviceButtons buttonsCfg() { return DeviceButtons{L_BTN, R_BTN, UP_BTN, DW_BTN, SEL_BTN}; }

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.i2c_bus = {(gpio_num_t)33, (gpio_num_t)26}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 33;
    bruceConfigPins.rfRx = 26;
    bruceConfigPins.irTx = -1;
    bruceConfigPins.irRx = 26;
    bruceConfigPins.rotation = 0;
    bruceConfigPins.uart_bus = {(gpio_num_t)22, (gpio_num_t)21};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)22, (gpio_num_t)21};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)22, (gpio_num_t)21}; // rx, tx
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)1};
    bruceConfigPins.PN532_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)1};
    // CC1101/NRF24/SDCARD share the main SPI bus (sck=18, miso=19, mosi=23)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, GPIO_NUM_NC, GPIO_NUM_NC
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)4
    }; // sck,miso,mosi,cs

    hal_buttons_init(buttonsCfg(), 5);

    bruceConfig.colorInverted = 0;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() { pinMode(TFT_BL, OUTPUT); }

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { return 0; }

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    pinMode(TFT_BL, OUTPUT);
    if (brightval > 5) {
        digitalWrite(TFT_BL, LOW);
        digitalWrite(TFT_BL, HIGH);
    } else {
        digitalWrite(TFT_BL, HIGH);
        digitalWrite(TFT_BL, LOW);
    }
}

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
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
