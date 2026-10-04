#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <Wire.h>
#include <globals.h>
#include <interface.h>

#define SEL_BTN 5

#define BTN_ACT LOW
#define DW_BTN 41
#define ESC_BTN 6
#define L_BTN 39
#define R_BTN 40
#define UP_BTN 38

static DeviceButtons buttonsCfg() { return DeviceButtons{L_BTN, R_BTN, UP_BTN, DW_BTN, SEL_BTN, ESC_BTN}; }

// Keep this app "unconfirmed" so it can be launched as a temporary/guest app
// from the Lilka keira launcher (on reboot the device rolls back to keira).
// Harmless for plain USB flashing (rollback isn't armed there).
extern "C" bool verifyRollbackLater() { return true; }

/***************************************************************************************
** Function: _setup_gpio()  — initial device setup (called from main.cpp)
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.buzzer = 11;
    bruceConfigPins.i2c_bus = {(gpio_num_t)13, (gpio_num_t)14}; // sda, scl (extension header)
    bruceConfigPins.rfTx = 13;
    bruceConfigPins.rfRx = 14;
    bruceConfigPins.irTx = 21;
    bruceConfigPins.irRx = 14;
    bruceConfigPins.rotation = 3;
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};    // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};     // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)44, (gpio_num_t)43};  // rx, tx
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)14, (gpio_num_t)47};
    bruceConfigPins.PN532_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)14, (gpio_num_t)47};
    // CC1101/NRF24 share the extension-header module bus (sck=12, miso=13, mosi=14)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)14, (gpio_num_t)47, (gpio_num_t)21, (gpio_num_t)48
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)14, (gpio_num_t)47, (gpio_num_t)21
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)18, (gpio_num_t)8, (gpio_num_t)17, (gpio_num_t)16
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)14, (gpio_num_t)47, (gpio_num_t)21, (gpio_num_t)13, (gpio_num_t)48, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,rst,dio0
#endif

    // Buttons — Lilka has NO external pull-ups (verified on schematic + working ESPHome),
    // so use INPUT_PULLUP. All buttons are active LOW (wired straight to GND).
    hal_buttons_init(buttonsCfg(), 6);

    // Keep radio module chip-selects idle (HIGH) at boot so they don't talk on the bus.
    // CC1101 and NRF24 share SS = 47 on Lilka.
    pinMode(bruceConfigPins.CC1101_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.CC1101_bus.cs, HIGH);
    pinMode(bruceConfigPins.NRF24_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);

    // Default external modules
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;

    // Default I2C bus (extension header)
    Wire.setPins(bruceConfigPins.i2c_bus.sda, bruceConfigPins.i2c_bus.scl);

    // TFT_BL = GPIO46 is the display power/SLEEP line, so PWM acts mostly as
    // on/off rather than smooth dimming.
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
}

/***************************************************************************************
** Function: isCharging()  — Lilka uses a bare TP4056, no charge-status GPIO
***************************************************************************************/
bool isCharging() { return false; }

/*********************************************************************
** Function: _setBrightness  — display backlight / power (TFT_BL = GPIO46)
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Sets PrevPress / NextPress / UpPress / DownPress / SelPress / EscPress
**********************************************************************/
void InputHandler(void) { hal_buttons_poll_6(buttonsCfg()); }

/*********************************************************************
** Function: powerOff  — deep sleep, wake on Select (GPIO0)
**********************************************************************/
void powerOff() {
    hal_bright_set(TFT_BL, 0); // backlight/display off
    esp_sleep_enable_ext0_wakeup((gpio_num_t)DEEPSLEEP_WAKEUP_PIN, DEEPSLEEP_PIN_ACT);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot  — hold Left+Right ~2s to power off
**********************************************************************/
void checkReboot() {
    if (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
        uint32_t t0 = millis();
        while (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
            if (millis() - t0 > 2000) { powerOff(); }
            delay(10);
        }
    }
}
