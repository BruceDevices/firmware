#include "core/powerSave.h"
#include <interface.h>

// Seeed Studio XIAO ESP32S3 (bare module), headless.
//
// Pin defaults below are the XIAO footprint's own labelled functions, so an
// off-the-shelf XIAO expansion/Grove wiring works without touching anything:
//
//   D4  GPIO5  SDA      D8  GPIO7  SCK     D0 GPIO1 free
//   D5  GPIO6  SCL      D9  GPIO8  MISO    D1 GPIO2 free
//   D6  GPIO43 TX       D10 GPIO9  MOSI    D2 GPIO3 free
//   D7  GPIO44 RX                          D3 GPIO4 free
//
// The four free pads carry the chip-selects and the IR/one-pin-RF lines. The
// SD card, CC1101 and NRF24 all share the one SPI bus (only one of them can be
// plugged at a time); every one of these is remappable at runtime from
// Config > Set Device pins, which is where a different wiring belongs -- see
// connections.md.

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.sys_i2c = {(gpio_num_t)5, (gpio_num_t)6};      // sda, scl (D4/D5)
    bruceConfigPins.i2c_bus = {(gpio_num_t)5, (gpio_num_t)6};      // sda, scl (D4/D5)
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};   // rx, tx (D7/D6)
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};    // rx, tx (D7/D6)
    bruceConfigPins.badusb_bus = {(gpio_num_t)44, (gpio_num_t)43}; // rx, tx (CH9329)

    bruceConfigPins.irTx = 1; // D0
    bruceConfigPins.irRx = 2; // D1
    bruceConfigPins.rfTx = 1; // D0
    bruceConfigPins.rfRx = 2; // D1

    // One shared SPI bus on the XIAO's SCK/MISO/MOSI pads, chip-selects on the
    // free ones: sck, miso, mosi, cs[, gdo0, gdo2]
    bruceConfigPins.outer_bus = {(gpio_num_t)7, (gpio_num_t)8, (gpio_num_t)9, (gpio_num_t)3};
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)7, (gpio_num_t)8, (gpio_num_t)9, (gpio_num_t)3};
    bruceConfigPins.PN532_bus = {(gpio_num_t)7, (gpio_num_t)8, (gpio_num_t)9, (gpio_num_t)4};
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)7, (gpio_num_t)8, (gpio_num_t)9, (gpio_num_t)4, (gpio_num_t)2, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)7, (gpio_num_t)8, (gpio_num_t)9, (gpio_num_t)4, (gpio_num_t)2, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,ce,-

    // No screen on this module: the WebUI is the only usable interface.
    bruceConfig.startupApp = "WebUI";
}

/***************************************************************************************
** Function name: getBattery()
** location: utils.cpp
** Description:   No battery gauge and no divider wired on the module itself
***************************************************************************************/
int getBattery() { return 0; }

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** No panel, no backlight
**********************************************************************/
void _setBrightness(uint8_t brightval) {}

/*********************************************************************
** Function: InputHandler
** No buttons on the module (BOOT is left alone so it stays usable for
** flashing); navigation happens through the WebUI / serial commands
**********************************************************************/
void InputHandler(void) {}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {}
