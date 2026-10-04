#include "core/powerSave.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include <driver/gpio.h>
#include <interface.h>

#define SEL_BTN 35

#define BTN_ACT LOW
#define DW_BTN 26
#define L_BTN 33
#define R_BTN 27
#define UP_BTN 34

// GPIO 34/35 are input-only (no internal pull-up) -- external pull-ups are present on this board.
static DeviceButtons buttonsCfg() { return DeviceButtons{L_BTN, R_BTN, UP_BTN, DW_BTN, SEL_BTN}; }

// Deselect NRF24/CC1101 before any Arduino/TFT init runs (runs before setup())
static void __attribute__((constructor)) _early_spi_deselect() {
    gpio_reset_pin(GPIO_NUM_15); // CC1101_SS / NRF24_SS
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_15, 1); // CS HIGH = deselected

    gpio_reset_pin(GPIO_NUM_4); // NRF24_CE
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_4, 0); // CE LOW = standby
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.rotation = 1;
    bruceConfigPins.irTx = 2;
    bruceConfigPins.i2c_bus = {(gpio_num_t)21, (gpio_num_t)22}; // sda, scl (Grove)
    bruceConfigPins.rfTx = 21;
    bruceConfigPins.rfRx = 22;
    bruceConfigPins.uart_bus = {(gpio_num_t)3, (gpio_num_t)1};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)3, (gpio_num_t)1};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)3, (gpio_num_t)1}; // rx, tx
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)15};
    // No dedicated PN532 pins on this board; PN532_bus shares the same slot as outer_bus
    bruceConfigPins.PN532_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)15};
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)15, (gpio_num_t)2
    }; // sck,miso,mosi,cs,gdo0
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)15, (gpio_num_t)4
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)12};

    // 5-way tactile switch — GPIO 34/35 are input-only (no pull-up on chip)
    // Ensure external pull-up resistors are present on these pins
    hal_buttons_init(buttonsCfg(), 5);

    // Deselect CC1101 and NRF24 on shared SPI bus so they don't interfere with TFT
    pinMode(bruceConfigPins.CC1101_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.CC1101_bus.cs, HIGH);
    pinMode(bruceConfigPins.NRF24_bus.cs, OUTPUT);
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
    pinMode(bruceConfigPins.NRF24_bus.io0, OUTPUT);
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup
***************************************************************************************/
void _post_setup_gpio() {
    // Backlight PWM
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
** On 5-way joystick: hold UP for ~400ms = EscPress (back/exit)
**********************************************************************/
void InputHandler(void) { hal_buttons_poll_5(buttonsCfg()); }

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, BTN_ACT);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device
**********************************************************************/
void checkReboot() {
    int countDown = 0;
    /* Long press Left+Right to power off */
    if (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
        uint32_t time_count = millis();
        while (digitalRead(L_BTN) == BTN_ACT && digitalRead(R_BTN) == BTN_ACT) {
            if (millis() - time_count > 500) {
                if (countDown == 0) {
                    int textWidth = tft.textWidth("PWR OFF IN 3/3", 1);
                    tft.fillRect(tftWidth / 2 - textWidth / 2, 7, textWidth, 18, bruceConfig.bgColor);
                }
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                if (countDown < 4)
                    tft.drawCentreString("PWR OFF IN " + String(countDown) + "/3", tftWidth / 2, 12, 1);
                else {
                    tft.fillScreen(bruceConfig.bgColor);
                    while (digitalRead(L_BTN) == BTN_ACT || digitalRead(R_BTN) == BTN_ACT);
                    delay(200);
                    powerOff();
                }
                delay(10);
            }
        }
        delay(30);
        if (millis() - time_count > 500) {
            tft.fillRect(60, 12, tftWidth - 60, tft.fontHeight(1), bruceConfig.bgColor);
            drawStatusBar();
        }
    }
}

int getBattery() { return 0; }

bool isCharging() { return false; }
