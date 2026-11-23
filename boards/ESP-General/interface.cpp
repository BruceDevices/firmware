#include "core/powerSave.h"
#include "core/bus_HAL.h"
#include <interface.h>
#include <Wire.h>

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // I2C bus setup: SDA = GPIO 7, SCL = GPIO 8 (SH1106 OLED & external Grove/I2C modules)
    bruceConfigPins.i2c_bus = {(gpio_num_t)7, (gpio_num_t)8};
    bruceConfigPins.sys_i2c = {(gpio_num_t)7, (gpio_num_t)8};
    setSysI2CBus(&Wire);

    // Reassigned non-colliding pins for ESP32-S3-Zero (no conflicts with GPIO 7, 8, 21, 26-37, 45, 46)
    bruceConfigPins.rfTx = 4;
    bruceConfigPins.rfRx = 5;
    bruceConfigPins.irTx = 6;
    bruceConfigPins.irRx = 14;
    bruceConfigPins.rotation = 0;
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};   // rx=44, tx=43 (native UART0)
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};    // rx=44, tx=43
    bruceConfigPins.badusb_bus = {(gpio_num_t)1, (gpio_num_t)2};   // rx=1, tx=2 (CH9329)

    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10};
    bruceConfigPins.PN532_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10};
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2

    // SDCARD pins unconfigured (NC) so setupSdCard() exits immediately without touching SPI or blocking
    bruceConfigPins.SDCARD_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};

    // Ensure OLED monochrome polarity: black background (0), white text
    bruceConfig.colorInverted = 0;
    bruceConfig.mainMenuStyle = 1; // Grid/compact style
    bruceConfig.wifiAtStartup = 1;
    bruceConfig.startupApp = "";
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Description:   post-storage hook to ensure SDCARD pins remain NC, Wi-Fi credentials
**                are registered, and OLED polarity is set even if old configs exist
***************************************************************************************/
void _post_setup_gpio() {
    bruceConfigPins.SDCARD_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
    bruceConfig.colorInverted = 0;
    bruceConfig.mainMenuStyle = 1;
    bruceConfig.wifiAtStartup = 1;
    bruceConfig.startupApp = "";

    // Register WiFi network Compass1 / Compass2026 if not already saved
    if (!bruceConfig.hasWifiCredential("Compass1") || bruceConfig.getWifiPassword("Compass1") != "Compass2026") {
        bruceConfig.addWifiCredential("Compass1", "Compass2026");
    }
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {}

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
