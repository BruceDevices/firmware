#include "core/bus_HAL.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include <M5Unified.h>
#include <globals.h>
#include <interface.h>

// Single front button under the LCD. M5Unified owns it as BtnA, but we never call
// M5.update(), so the shared 1-button HAL can just read the pin itself.
#define BTN_PIN 41

static DeviceButtons buttonsCfg() {
    DeviceButtons cfg;
    cfg.btn1 = BTN_PIN;
    return cfg;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.i2c_bus = {(gpio_num_t)2, (gpio_num_t)1};   // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {(gpio_num_t)38, (gpio_num_t)39}; // sda, scl (internal)
    bruceConfigPins.irTx = 4;                                   // internal IR LED
    bruceConfigPins.irRx = 2;
    bruceConfigPins.rfTx = 2;
    bruceConfigPins.rfRx = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)2, (gpio_num_t)1}; // rx, tx (Grove)
    bruceConfigPins.gps_bus = {(gpio_num_t)2, (gpio_num_t)1};  // rx, tx (Grove)
    // No on-board SD slot or radios: every SPI module shares the free bottom-header pins
    // (sck=5, miso=6, mosi=7, cs=8).
    bruceConfigPins.outer_bus = {(gpio_num_t)5, (gpio_num_t)6, (gpio_num_t)7, (gpio_num_t)8};
    bruceConfigPins.PN532_bus = bruceConfigPins.outer_bus;
    bruceConfigPins.SDCARD_bus = bruceConfigPins.outer_bus;
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)5, (gpio_num_t)6, (gpio_num_t)7, (gpio_num_t)8, (gpio_num_t)1, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)5, (gpio_num_t)6, (gpio_num_t)7, (gpio_num_t)8, (gpio_num_t)1
    }; // sck,miso,mosi,cs(ss),ce
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {(gpio_num_t)5, (gpio_num_t)6, (gpio_num_t)7, (gpio_num_t)8};
    bruceConfigPins.LoRa_bus = {(gpio_num_t)5, (gpio_num_t)6, (gpio_num_t)7, (gpio_num_t)8};
#endif

    M5.begin(); // auto-detects the AtomS3 panel and power
    setSysI2CBus(M5.In_I2C.getPort() == I2C_NUM_1 ? &Wire1 : &Wire);
    bruceConfig.colorInverted = 0;
    hal_buttons_init(buttonsCfg(), 1);
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { M5.Display.setBrightness((255 * brightval) / 100); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) { hal_buttons_poll_1(buttonsCfg()); }
