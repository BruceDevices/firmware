#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include <interface.h>

#define SEL_BTN 37

#define DW_BTN 39
#define UP_BTN 35

static DeviceButtons buttonsCfg() {
    DeviceButtons cfg{UP_BTN, DW_BTN, SEL_BTN};
    cfg.pullup = false; // no internal/external pull-ups on these pins
    return cfg;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.mic_bus = {(gpio_num_t)0, (gpio_num_t)34, GPIO_NUM_NC, MIC_TYPE_PDM}; // clk,data,ws,type
    bruceConfigPins.buzzer = 2;
    bruceConfigPins.i2c_bus = {(gpio_num_t)32, (gpio_num_t)33};  // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {(gpio_num_t)21, (gpio_num_t)22};  // sda, scl
    bruceConfigPins.rfTx = 32;
    bruceConfigPins.rfRx = 33;
    bruceConfigPins.irTx = 19;
    bruceConfigPins.irRx = 33;
    bruceConfigPins.rotation = 3;
    bruceConfigPins.badusb_bus = {(gpio_num_t)33, (gpio_num_t)32}; // rx, tx (Grove)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)0, (gpio_num_t)33, (gpio_num_t)32, (gpio_num_t)26};
    // No dedicated PN532 pins on this board; reuse the shared/outer SPI slot (same convention
    // used for RC522-SPI on other boards).
    bruceConfigPins.PN532_bus = {(gpio_num_t)0, (gpio_num_t)33, (gpio_num_t)32, (gpio_num_t)26};
    // CC1101/NRF24/W5500/SDCARD share the same physical SPI bus (sck=0, miso=33, mosi=32),
    // switching between SD/RF via shared CS logic below.
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)0, (gpio_num_t)33, (gpio_num_t)32, (gpio_num_t)26, (gpio_num_t)25, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)0, (gpio_num_t)33, (gpio_num_t)32, (gpio_num_t)26, (gpio_num_t)25
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)0, (gpio_num_t)25, (gpio_num_t)26, (gpio_num_t)14
    }; // sck,miso,mosi,cs
    // Alt wiring: CC1101/NRF24 dongle sharing the SD card's SPI bus instead of the Grove module
    bruceConfigPins.CC1101_presets = {
        {"Shared SPI",
         {bruceConfigPins.SDCARD_bus.sck, bruceConfigPins.SDCARD_bus.miso, bruceConfigPins.SDCARD_bus.mosi,
          GPIO_NUM_33, GPIO_NUM_32, GPIO_NUM_NC},
         "https://github.com/pr3y/Bruce/blob/main/media/connections/cc1101_stick_SDCard.jpg"}
    };
    bruceConfigPins.NRF24_presets = {
        {"Shared SPI",
         {bruceConfigPins.SDCARD_bus.sck, bruceConfigPins.SDCARD_bus.miso, bruceConfigPins.SDCARD_bus.mosi,
          GPIO_NUM_33, GPIO_NUM_32, GPIO_NUM_NC}}
    };
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)0, (gpio_num_t)33, (gpio_num_t)32, (gpio_num_t)26, (gpio_num_t)25, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)0, (gpio_num_t)33, (gpio_num_t)32, (gpio_num_t)26, (gpio_num_t)-1, (gpio_num_t)25
    }; // sck,miso,mosi,cs,rst,dio0
#endif

    setSysI2CBus(&Wire1); // BM8563 RTC lives on Wire1
#if defined(HAS_RTC)
    _rtc.setWire(getSysI2CBus());
#endif
    hal_buttons_init(buttonsCfg(), 3);
    pinMode(4, OUTPUT);    // Keeps the Stick alive after take off the USB cable
    digitalWrite(4, HIGH); // Keeps the Stick alive after take off the USB cable
    gpio_pulldown_dis(GPIO_NUM_36);
    gpio_pullup_dis(GPIO_NUM_36);
    pinMode(32, OUTPUT);
    pinMode(33, OUTPUT);
    digitalWrite(32, LOW);
    digitalWrite(33, HIGH);
    //=========================================================================
    // Issue: During startup, the SD card might keep the MISO line at a high level continuously, causing RF
    // initialization to fail. Solution：Forcing switch to SD card and sending dummy clocks
    //=========================================================================
    int pin_shared_ctrl = 33; // Controls CS: HIGH=SD_Select, LOW=RF_Select
    int pin_sck = 0;          // SCK Pin for M5StickC Plus 2
    pinMode(pin_shared_ctrl, OUTPUT);
    pinMode(pin_sck, OUTPUT);
    digitalWrite(pin_shared_ctrl, HIGH); // Force Select SD Card
    delay(10);
    for (int i = 0; i < 80; i++) {
        digitalWrite(pin_sck, HIGH);
        delayMicroseconds(10);
        digitalWrite(pin_sck, LOW);
        delayMicroseconds(10);
    } // send dummy clocks
    digitalWrite(pin_shared_ctrl, HIGH); // Keep the SD card selected.

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
**********************************************************************/
void InputHandler(void) { hal_buttons_poll_3(buttonsCfg()); }

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    digitalWrite(4, LOW);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)UP_BTN, LOW);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {
    int countDown = 0;
    /* Long press power off */
    if (digitalRead(UP_BTN) == LOW) {
        uint32_t time_count = millis();
        while (digitalRead(UP_BTN) == LOW) {
            // Display poweroff bar only if holding button
            if (millis() - time_count > 500) {
                if (countDown == 0) {
                    int textWidth = tft.textWidth("PWR OFF IN 3/3", 1);
                    tft.fillRect(60, 7, textWidth, 18, bruceConfig.bgColor);
                }
                tft.setCursor(60, 12);
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                tft.printf(" PWR OFF IN %d/3\n", countDown);
                vTaskDelay(10 / portTICK_RATE_MS);
            }
        }

        // Clear text after releasing the button
        if (millis() - time_count > 500) {
            tft.fillRect(60, 12, 16 * LW, tft.fontHeight(1), bruceConfig.bgColor);
            drawStatusBar();
        }
        PrevPress = true;
    }
}

bool isCharging() { return false; }
