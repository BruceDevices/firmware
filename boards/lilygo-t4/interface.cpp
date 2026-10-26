#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "core/powerSave.h"
#include "core/utils.h"

#include <globals.h>
#include <interface.h>

#define SEL_BTN 37

#define BTN_ACT LOW
#define DW_BTN 39
#define UP_BTN 38

// LEFT (UP_BTN) -> Previous, RIGHT (DW_BTN) -> Next, CENTER (SEL_BTN) -> Select
// LEFT + RIGHT together -> Escape
// The three front buttons are on ESP32 input-only GPIOs (37/38/39), which
// cannot use the internal pull-ups - the T4 board provides external ones.
static DeviceButtons buttonsCfg() {
    DeviceButtons cfg{UP_BTN, DW_BTN, SEL_BTN};
    cfg.pullup = false;
    return cfg;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.i2c_bus = {(gpio_num_t)21, (gpio_num_t)22}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 21;
    bruceConfigPins.rfRx = 22;
    bruceConfigPins.irTx = 26; // TXLED
    bruceConfigPins.irRx = 25; // RXLED
    bruceConfigPins.rotation = 1;
    bruceConfigPins.uart_bus = {(gpio_num_t)22, (gpio_num_t)21};    // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)22, (gpio_num_t)21};     // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)22, (gpio_num_t)21};  // rx, tx (CH9329)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI).
    // No dedicated PN532 module on this board, so PN532_bus shares the same slot.
    bruceConfigPins.outer_bus = {(gpio_num_t)25, (gpio_num_t)33, (gpio_num_t)26, (gpio_num_t)17};
    bruceConfigPins.PN532_bus = {(gpio_num_t)25, (gpio_num_t)33, (gpio_num_t)26, (gpio_num_t)17};
    // No RF module fitted on the T4; these are sane defaults on free GPIOs so CC1101/NRF24
    // modules can be wired manually (ALLOW_ALL_GPIO_FOR_IR_RF lets the user re-map at runtime).
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)25, (gpio_num_t)33, (gpio_num_t)26, (gpio_num_t)17, (gpio_num_t)34, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)25, (gpio_num_t)33, (gpio_num_t)26, (gpio_num_t)17, (gpio_num_t)35
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)14, (gpio_num_t)2, (gpio_num_t)15, (gpio_num_t)13
    }; // sck,miso,mosi,cs

    hal_buttons_init(buttonsCfg(), 3);

    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);

    Serial.begin(115200);
}

/*********************************************************************
**  Function: setBrightness
**  set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) { hal_buttons_poll_3(buttonsCfg()); }

/*********************************************************************
** Function: powerOff
** Turns off the device (deep sleep, wake on the center button)
**********************************************************************/
void powerOff() {
    tft.fillScreen(bruceConfig.bgColor);
    digitalWrite(TFT_BL, LOW);
    tft.writecommand(0x10); // ILI9341 enter sleep
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, BTN_ACT);
    esp_deep_sleep_start();
}
