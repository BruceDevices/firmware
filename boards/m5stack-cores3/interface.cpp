#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <M5Unified.h>
#include <interface.h>
#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>

static void setupCoreS3SharedSpiPins() {
    pinMode(TFT_CS, OUTPUT);
    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    digitalWrite(SDCARD_CS, HIGH);

    // CoreS3 shares the display D/C pin with SPI MISO. M5.begin() and the first
    // TFT_eSPI draw happen before storage is mounted, so release GPIO35 back to
    // MISO at the pre-storage hook. TFT_eSPI switches it to D/C only while TFT CS is active.
#if defined(USE_HSPI_PORT)
    pinMatrixInAttach(TFT_MISO, SPI3_Q_IN_IDX, false);
    *(volatile uint32_t *)GPIO_FUNC35_OUT_SEL_CFG_REG = SPI3_Q_OUT_IDX;
#else
    pinMatrixInAttach(TFT_MISO, FSPIQ_IN_IDX, false);
    *(volatile uint32_t *)GPIO_FUNC35_OUT_SEL_CFG_REG = FSPIQ_OUT_IDX;
#endif
    *(volatile uint32_t *)GPIO_ENABLE1_W1TC_REG = 1u << (TFT_MISO - 32);
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    M5.begin();
    M5.Power.setUsbOutput(false);
    M5.Power.setExtOutput(true);
    setSysI2CBus(M5.In_I2C.getPort() == I2C_NUM_1 ? &Wire1 : &Wire);
#if defined(HAS_RTC)
    _rtc.setWire(getSysI2CBus());
#endif
}

void _pre_storage_gpio() { setupCoreS3SharedSpiPins(); }

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int percent;
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
            t.y = (tftHeight + 20) - t.y;
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
            t.y = (tftHeight + 20) - tmp;
        }

        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;

        // Touch point global variable
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
