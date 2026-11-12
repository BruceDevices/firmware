#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include <M5Unified.h>
#include <interface.h>

// Rotary encoder
#include "hal/device.h"
#include "hal/inputs/encoder.h"

#define ENCODER_INA 41
#define ENCODER_INB 40
#define ENCODER_KEY 42

static DeviceEncoder encoderCfg() {
    DeviceEncoder cfg;
    cfg.pin_a = ENCODER_INA;
    cfg.pin_b = ENCODER_INB;
    cfg.pin_sel = ENCODER_KEY;
    return cfg;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.buzzer = 3;
    bruceConfigPins.i2c_bus = {(gpio_num_t)1, (gpio_num_t)2};   // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {(gpio_num_t)11, (gpio_num_t)12}; // sda, scl
    bruceConfigPins.rfTx = 1;
    bruceConfigPins.rfRx = 2;
    bruceConfigPins.irTx = 1;
    bruceConfigPins.irRx = 2;
    bruceConfigPins.rotation = 3;
    bruceConfigPins.uart_bus = {(gpio_num_t)15, (gpio_num_t)13};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)15, (gpio_num_t)13};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)2, (gpio_num_t)1};   // rx, tx (CH9329, via GROVE fallback)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)15, (gpio_num_t)2, (gpio_num_t)1, (gpio_num_t)13};
    // No dedicated PN532 pins on this board -> PN532_bus reuses outer_bus (shared SPI slot)
    bruceConfigPins.PN532_bus = {(gpio_num_t)15, (gpio_num_t)2, (gpio_num_t)1, (gpio_num_t)13};
    // CC1101/NRF24/SDCARD/LoRa/W5500 share the same SPI bus (sck=15, miso=2, mosi=1)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)15, (gpio_num_t)2, (gpio_num_t)1, GPIO_NUM_NC, (gpio_num_t)13, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)15, (gpio_num_t)2, (gpio_num_t)1, (gpio_num_t)13, GPIO_NUM_NC
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)15, (gpio_num_t)2, (gpio_num_t)1, (gpio_num_t)13
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)15, (gpio_num_t)2, (gpio_num_t)1, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)15, (gpio_num_t)2, (gpio_num_t)1, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,rst,dio0
#endif

    M5.begin();
    setSysI2CBus(M5.In_I2C.getPort() == I2C_NUM_1 ? &Wire1 : &Wire);
    bruceConfig.colorInverted = 0;
    hal_encoder_init(encoderCfg());
}
/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { M5.Display.setBrightness(brightval); }

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int level = M5.Power.getBatteryLevel();
    return (level < 0) ? 0 : (level >= 100) ? 100 : level;
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    hal_encoder_poll(encoderCfg());

    // Prev + Select together -> Esc
    if (PrevPress && SelPress) {
        EscPress = true;
        SelPress = false;
        PrevPress = false;
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { M5.Power.powerOff(); }
