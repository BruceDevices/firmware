#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include <M5Unified.h>
#include <interface.h>

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.buzzer = 25;
    bruceConfigPins.i2c_bus = {(gpio_num_t)21, (gpio_num_t)22};   // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {(gpio_num_t)21, (gpio_num_t)22}; // sda, scl
    bruceConfigPins.rfTx = 21;
    bruceConfigPins.rfRx = 22;
    bruceConfigPins.irTx = 21;
    bruceConfigPins.irRx = 22;
    bruceConfigPins.rotation = 1;
    bruceConfigPins.badusb_bus = {(gpio_num_t)22, (gpio_num_t)21}; // rx, tx (CH9329)
    // Board's default/generic SPI bus (used by drivers without their own bus, e.g. RC522-SPI)
    bruceConfigPins.outer_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)26};
    // No dedicated PN532 pins on this board; RFID2 (RC522-SPI) shares the outer/default SPI bus
    bruceConfigPins.PN532_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)26};
    // CC1101/NRF24/W5500/SDCard share the SD Card's SPI bus (sck=18, miso=19, mosi=23)
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)0, (gpio_num_t)35, GPIO_NUM_NC
    }; // sck,miso,mosi,cs,gdo0,gdo2
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)12, (gpio_num_t)13
    }; // sck,miso,mosi,cs(ss),ce
    bruceConfigPins.SDCARD_bus = {(gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)4
    }; // sck,miso,mosi,cs
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)18, (gpio_num_t)19, (gpio_num_t)23, (gpio_num_t)15, (gpio_num_t)34, (gpio_num_t)13
    }; // sck,miso,mosi,cs,int,rst
#endif

    M5.begin(); // Need to test if SDCard inits with the new setup
    setSysI2CBus(M5.In_I2C.getPort() == I2C_NUM_1 ? &Wire1 : &Wire);
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    uint8_t percent = 0;
    percent = M5.Power.getBatteryLevel();
    return (percent < 0) ? 1 : (percent >= 100) ? 100 : percent;
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    uint8_t _tmp = (255 * brightval) / 100;
    M5.Lcd.setBrightness(_tmp);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, check(SelPress), AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    // RFID driver mid-transaction - skip this cycle's M5.update(), buttons below are plain GPIO
    // so they're unaffected; just retry next tick.
    if (trylockSysI2CBus()) {
        M5.update();
        unlockSysI2CBus();
    }
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool aPressed = (M5.BtnA.isPressed());
    bool bPressed = (M5.BtnB.isPressed());
    bool cPressed = (M5.BtnC.isPressed());

    bool anyPressed = aPressed || bPressed || cPressed;
    if (anyPressed) tm = millis();
    if (anyPressed && wakeUpScreen()) return;

    AnyKeyPress = anyPressed;
    if (aPressed && cPressed) {
        EscPress = true;
        return;
    }
    PrevPress = aPressed;
    NextPress = cPressed;
    SelPress = bPressed;
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
