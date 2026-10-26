#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "core/powerSave.h"
#include <interface.h>

#define SEL_BTN 1

#define DW_BTN 2
#define UP_BTN 3

static DeviceButtons buttonsCfg() { return DeviceButtons{UP_BTN, DW_BTN, SEL_BTN}; }

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/

void _setup_gpio() {
    bruceConfigPins.buzzer = 47;
    bruceConfigPins.i2c_bus = {(gpio_num_t)4, (gpio_num_t)5}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 4;
    bruceConfigPins.rfRx = 5;
    bruceConfigPins.irTx = 5;
    bruceConfigPins.irRx = 4;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)5, (gpio_num_t)4};    // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)5, (gpio_num_t)4};     // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)5, (gpio_num_t)4};  // rx, tx (CH9329)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)10, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)6};
    bruceConfigPins.PN532_bus = {(gpio_num_t)10, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)6};
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)10, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)17, (gpio_num_t)16, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)10, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)18, (gpio_num_t)21
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)10, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)7
    }; // sck,miso,mosi,cs

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

    hal_buttons_init(buttonsCfg(), 3);

    pinMode(bruceConfigPins.NRF24_bus.cs, OUTPUT);
    pinMode(bruceConfigPins.CC1101_bus.cs, OUTPUT);
    pinMode(bruceConfigPins.SDCARD_bus.cs, OUTPUT);
    pinMode(TFT_CS, OUTPUT);

    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
    digitalWrite(bruceConfigPins.CC1101_bus.cs, HIGH);
    digitalWrite(bruceConfigPins.SDCARD_bus.cs, HIGH);
    digitalWrite(TFT_CS, HIGH);
}

bool isCharging() { return false; }

int getBattery() { return 0; }
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
void InputHandler(void) { hal_buttons_poll_3(buttonsCfg()); }

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
