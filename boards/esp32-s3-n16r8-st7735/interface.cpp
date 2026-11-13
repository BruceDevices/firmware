// ESP32-S3 N16R8 (DevKitC-1 style) + 1.8" ST7735 128x160 SPI screen + 3 buttons.
// Pins are in esp32-s3-n16r8-st7735.ini, wiring in connections.md.
#include "core/powerSave.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include <interface.h>

// Each button goes to GND (internal pull-ups, active LOW). Previous + next together = back.
#define PREV_BTN 39
#define SEL_BTN 41
#define NEXT_BTN 42
static DeviceButtons buttonsCfg() { return DeviceButtons{PREV_BTN, NEXT_BTN, SEL_BTN}; }

// Minimum backlight duty 160 (of 255), the same floor this board's earlier pre-HAL build used.
static HalBrightCurve backlightCurve() {
    HalBrightCurve curve;
    curve.pwm_min = 160;
    return curve;
}

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
    // I2C and serial use GPIO47 instead of the ESP-General GPIO46 (a strapping pin: a PN532
    // pull-up there can stop the board entering download mode).
    bruceConfigPins.i2c_bus = {(gpio_num_t)8, (gpio_num_t)47}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 8;
    bruceConfigPins.rfRx = 47;
    bruceConfigPins.irTx = 40;
    bruceConfigPins.irRx = 47;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)47, (gpio_num_t)8};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)47, (gpio_num_t)8};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)47, (gpio_num_t)8}; // rx, tx (CH9329)
    // Modules share one SPI bus (SCK 12, MISO 13, MOSI 11), separate from the screen's HSPI bus
    bruceConfigPins.outer_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10};
    bruceConfigPins.PN532_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10};
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)9, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)14, (gpio_num_t)17
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)21
    }; // sck,miso,mosi,cs

    // Deselect every module on the shared bus so none drives MISO while another is talking
    deselect(bruceConfigPins.CC1101_bus.cs);
    deselect(bruceConfigPins.NRF24_bus.cs);
    deselect(bruceConfigPins.SDCARD_bus.cs);

    hal_buttons_init(buttonsCfg(), 3);
    bruceConfig.colorInverted = 0; // plain (non-IPS) ST7735
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100, backlightCurve());
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval, backlightCurve()); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) { hal_buttons_poll_3(buttonsCfg()); }
