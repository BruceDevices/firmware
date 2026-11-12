#include "CYD28_TouchscreenR.h"
#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <Arduino.h>
#include <interface.h>

#define PWR_EN_PIN 10
#define PWR_ON_PIN 14

#define PIN_SD_CMD 11
#define PIN_SD_CLK 12
#define PIN_SD_D0 13

extern CYD28_TouchR touch; // defined by hal/inputs/touch.cpp

static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    // The XPT2046 reports rotation 2 coordinates
    // rotation:        0      1      2      3
    const bool swapXY[4] = {true, false, true, false};
    const bool mirrorX[4] = {true, false, false, true};
    const bool mirrorY[4] = {false, false, true, true};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    static SPIClass *bus = acquireSPIBus( // acquired once, on the first call (hal_touch_init)
        (gpio_num_t)XPT2046_SPI_BUS_SCLK_IO_NUM, (gpio_num_t)XPT2046_SPI_BUS_MISO_IO_NUM,
        (gpio_num_t)XPT2046_SPI_BUS_MOSI_IO_NUM
    );
    cfg.spi_bus = bus;
    return cfg;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.i2c_bus = {(gpio_num_t)17, (gpio_num_t)18};    // sda, scl (Grove)
    bruceConfigPins.uart_bus = {(gpio_num_t)43, (gpio_num_t)44};   // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)43, (gpio_num_t)44};    // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)18, (gpio_num_t)17}; // rx, tx (CH9329)
    bruceConfigPins.irTx = 17;
    bruceConfigPins.irRx = 18;
    bruceConfigPins.rotation = 3;
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)1, (gpio_num_t)4, (gpio_num_t)3, (gpio_num_t)15};
    // No dedicated PN532 pins on this board; RC522-SPI/PN532-SPI share the same SPI slot
    bruceConfigPins.PN532_bus = {(gpio_num_t)1, (gpio_num_t)4, (gpio_num_t)3, (gpio_num_t)15};
    // CC1101/NRF24/W5500 share the same SPI bus (sck=1, miso=4, mosi=3)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)1, (gpio_num_t)4, (gpio_num_t)3, (gpio_num_t)15, (gpio_num_t)16, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)1, (gpio_num_t)4, (gpio_num_t)3, (gpio_num_t)15, (gpio_num_t)16
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)1, (gpio_num_t)4, (gpio_num_t)3, (gpio_num_t)15, (gpio_num_t)16, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
#endif

    // Using SD_MMC
    SD.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);

    pinMode(XPT2046_SPI_CONFIG_CS_GPIO_NUM, OUTPUT);
    digitalWrite(XPT2046_SPI_CONFIG_CS_GPIO_NUM, HIGH);
    pinMode(PWR_ON_PIN, OUTPUT);
    digitalWrite(PWR_ON_PIN, HIGH);
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);
    bruceConfig.colorInverted = 0;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    if (!hal_touch_init(touchCfg(), 0, true)) { // own SPI bus (touchCfg().spi_bus)
        Serial.println("Touchscreen initialization failed!");
    }
    // Brightness control must be initialized after tft in this case @Pirata
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
void InputHandler(void) {
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            d_tmp = millis();
            hal_touch_apply(t);
        }
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
