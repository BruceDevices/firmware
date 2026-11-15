#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <M5Unified.h>
#include <interface.h>

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // Wiring kept for reference; this board has no HAS_SPEAKER gate
    // bclk,ws,dout,mclk
    bruceConfigPins.speaker_bus = {(gpio_num_t)12, (gpio_num_t)0, (gpio_num_t)2, GPIO_NUM_NC};
    bruceConfigPins.mic_bus = {(gpio_num_t)0, (gpio_num_t)34, GPIO_NUM_NC, MIC_TYPE_PDM}; // clk,data,ws,type
    bruceConfigPins.i2c_bus = {(gpio_num_t)32, (gpio_num_t)33};  // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {(gpio_num_t)21, (gpio_num_t)22};  // sda, scl
    bruceConfigPins.rfTx = 32;
    bruceConfigPins.rfRx = 33;
    bruceConfigPins.irTx = 32;
    bruceConfigPins.irRx = 33;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.badusb_bus = {(gpio_num_t)33, (gpio_num_t)32}; // rx, tx (CH9329)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)18, (gpio_num_t)38, (gpio_num_t)23, (gpio_num_t)26};
    bruceConfigPins.PN532_bus = {(gpio_num_t)18, (gpio_num_t)38, (gpio_num_t)23, (gpio_num_t)26};
    // CC1101/NRF24/W5500/SDCard share the SD Card's SPI bus (sck, miso, mosi)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)18, (gpio_num_t)38, (gpio_num_t)23, (gpio_num_t)0, (gpio_num_t)35, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)18, (gpio_num_t)38, (gpio_num_t)23, (gpio_num_t)27, (gpio_num_t)19
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)18, (gpio_num_t)38, (gpio_num_t)23, (gpio_num_t)4
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)18, (gpio_num_t)38, (gpio_num_t)23, (gpio_num_t)2, (gpio_num_t)34, (gpio_num_t)19
    }; // sck,miso,mosi,cs,int,rst
#endif

    M5.begin(); // Need to test if SDCard inits with the new setup
    setSysI2CBus(M5.In_I2C.getPort() == I2C_NUM_1 ? &Wire1 : &Wire);
#if defined(HAS_RTC)
    _rtc.setWire(getSysI2CBus());
#endif
    pinMode(GPIO_NUM_0, OUTPUT);
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int percent = 0;
    percent = M5.Power.getBatteryLevel();
    return (percent < 0) ? 1 : (percent >= 100) ? 100 : percent;
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { M5.Display.setBrightness(brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;
    if (!trylockSysI2CBus()) return; // RFID driver mid-transaction - retry next tick
    M5.update();
    unlockSysI2CBus();
    auto t = M5.Touch.getDetail();
    if (t.isPressed() || t.isHolding()) {
        tm = millis();
        if (bruceConfigPins.rotation == 3) {
            t.y = (tftHeight + TOUCH_FOOTER_HEIGHT) - t.y;
            t.x = tftWidth - t.x;
        }
        if (bruceConfigPins.rotation == 0) {
            int tmp = t.x;
            t.x = tftWidth - t.y;
            t.y = tmp;
        }
        if (bruceConfigPins.rotation == 2) {
            int tmp = t.x;
            t.x = t.y;
            t.y = (tftHeight + TOUCH_FOOTER_HEIGHT) - tmp;
        }
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;

        // Touch point global variable
        Serial.printf("Touch at x: %d  y: %d\n", t.x, t.y);
        touchPoint.x = t.x;
        touchPoint.y = t.y;
        touchPoint.pressed = true;
        touchHeatMap(touchPoint);
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { M5.Power.powerOff(); }
void goToDeepSleep() { M5.Power.deepSleep(); }

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {}

/***************************************************************************************
** Function name: isCharging()
** Description:   Determines if the device is charging
***************************************************************************************/
bool isCharging() {
    if (M5.Power.getBatteryCurrent() > 0 || M5.Power.getBatteryCurrent()) return true;
    else return false;
}
