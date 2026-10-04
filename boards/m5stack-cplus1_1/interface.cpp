#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include <AXP192.h>
#include <interface.h>

#define SEL_BTN 37

#define DW_BTN 39
AXP192 axp192;

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.mic_bus = {(gpio_num_t)0, (gpio_num_t)34, GPIO_NUM_NC, MIC_TYPE_PDM}; // clk,data,ws,type
    bruceConfigPins.buzzer = 2;
    bruceConfigPins.i2c_bus = {(gpio_num_t)32, (gpio_num_t)33};   // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {(gpio_num_t)21, (gpio_num_t)22};   // sda, scl
    bruceConfigPins.rfTx = 32;
    bruceConfigPins.rfRx = 33;
    bruceConfigPins.irTx = 9;
    bruceConfigPins.irRx = 33;
    bruceConfigPins.rotation = 3;
    bruceConfigPins.uart_bus = {(gpio_num_t)33, (gpio_num_t)32};    // rx, tx (was SERIAL_TX/RX, via GROVE fallback)
    bruceConfigPins.gps_bus = {(gpio_num_t)33, (gpio_num_t)32};     // rx, tx (was GPS_SERIAL_TX/RX, via GROVE fallback)
    bruceConfigPins.badusb_bus = {(gpio_num_t)33, (gpio_num_t)32};  // rx, tx (CH9329, Grove)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)0, (gpio_num_t)33, (gpio_num_t)32, (gpio_num_t)26};
    // No dedicated PN532 pins on this board; PN532_bus shares the same slot as outer_bus
    bruceConfigPins.PN532_bus = {(gpio_num_t)0, (gpio_num_t)33, (gpio_num_t)32, (gpio_num_t)26};
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)0, (gpio_num_t)33, (gpio_num_t)32, (gpio_num_t)26, (gpio_num_t)25, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)0, (gpio_num_t)33, (gpio_num_t)32, (gpio_num_t)26, (gpio_num_t)25
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)0, (gpio_num_t)36, (gpio_num_t)26, (gpio_num_t)14
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
        (gpio_num_t)0, (gpio_num_t)36, (gpio_num_t)26, (gpio_num_t)33, (gpio_num_t)25, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,int,rst
#endif

    pinMode(SEL_BTN, INPUT);
    pinMode(DW_BTN, INPUT);
    setSysI2CBus(&Wire1); // AXP192 (BM8563 RTC included) lives on Wire1
#if defined(HAS_RTC)
    _rtc.setWire(getSysI2CBus());
#endif
    axp192.begin(); // Start the energy management of AXP192
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/

int getBattery() {
    int percent = 0;
#ifndef LITE_VERSION
    float b = axp192.GetBatVoltage();
    percent = ((b - 3.0) / 1.2) * 100;
#endif
    return (percent < 0) ? 1 : (percent >= 100) ? 100 : percent;
}

/*********************************************************************
**  Function: setBrightness
**  set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval > 100) brightval = 100;
    axp192.ScreenBreath(brightval);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool upPressed = (axp192.GetBtnPress());
    bool selPressed = (digitalRead(SEL_BTN) == LOW);
    bool dwPressed = (digitalRead(DW_BTN) == LOW);

    bool anyPressed = upPressed || selPressed || dwPressed;
    if (anyPressed) tm = millis();
    if (anyPressed && wakeUpScreen()) return;

    AnyKeyPress = anyPressed;
    if (upPressed && dwPressed) {
        EscPress = true;
        return;
    }
    PrevPress = upPressed;
    NextPress = dwPressed;
    SelPress = selPressed;
}

void powerOff() { axp192.PowerOff(); }
#ifndef LITE_VERSION
/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {
    int countDown = 0;
    /* Long press power off */
    if (axp192.GetBtnPress()) {
        uint32_t time_count = millis();
        while (axp192.GetBtnPress()) {
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

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
bool isCharging() {
    return axp192.GetBatCurrent() > 20; // need testing
}
#endif
