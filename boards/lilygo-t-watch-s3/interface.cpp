#include "core/powerSave.h"
#include "core/utils.h"
#include <Wire.h>
#include <XPowersLib.h>
#include <interface.h>

XPowersAXP2101 axp192;
#include <TouchDrvFT6X36.hpp>
TouchDrvFT6X36 touch;

// Haptic
#include "SensorDRV2605.hpp"
SensorDRV2605 drv;

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    pinMode(16, INPUT); // Touch IRQ
    Wire.begin(10, 11); // sensors
    delay(10);
    Wire1.begin(39, 40); // touchscreen
    delay(10);
    _rtc.setWire(&Wire); // Cplus uses Wire1 default, the lib had been changed to accept setting I2C bus
                         // StickCPlus uses BM8563 that is the same as PCF8536
    axp192.init(Wire, 10, 11);
    axp192.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);
    axp192.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_900MA);
    axp192.setSysPowerDownVoltage(2600);
    axp192.setALDO1Voltage(3300);
    axp192.setALDO2Voltage(3300);
    axp192.setALDO3Voltage(3300);
    axp192.setALDO4Voltage(3300);
    axp192.setBLDO2Voltage(3300);
    axp192.setDC3Voltage(3300);
    axp192.enableDC3(); // gps
    axp192.disableDC2();
    axp192.disableDC4();
    axp192.disableDC5();
    axp192.disableBLDO1();
    axp192.disableCPUSLDO();
    axp192.disableDLDO1();
    axp192.disableDLDO2();
    axp192.enableALDO1(); //! RTC VBAT
    axp192.enableALDO2(); //! TFT BACKLIGHT   VDD
    axp192.enableALDO3(); //! Screen touch VDD
    axp192.enableALDO4(); //! Radio VDD
    axp192.enableBLDO2(); //! drv2605 enable
    //  Set the time of pressing the button to turn off
    axp192.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    // Set the button power-on press time
    axp192.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
    // It is necessary to disable the detection function of the TS pin on the board
    // without the battery temperature detection function, otherwise it will cause abnormal charging
    axp192.disableTSPinMeasure();
    // Enable internal ADC detection
    axp192.enableBattDetection();
    axp192.enableVbusVoltageMeasure();
    axp192.enableBattVoltageMeasure();
    axp192.enableSystemVoltageMeasure();
    // t-watch no chg led
    axp192.setChargingLedMode(XPOWERS_CHG_LED_OFF);
    axp192.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    // Enable the required interrupt function
    axp192.enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |    // BATTERY
        XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |  // VBUS
        XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |     // POWER KEY
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ // CHARGE
    );

    // Clear all interrupt flags
    axp192.clearIrqStatus();
    // Set the precharge charging current
    axp192.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    // Set constant current charge current limit
    axp192.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_300MA);
    // Set stop charging termination current
    axp192.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
    // Set charge cut-off voltage
    axp192.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V35);
    // Set RTC Battery voltage to 3.3V
    axp192.setButtonBatteryChargeVoltage(3300);
    axp192.enableButtonBatteryCharge();

    touch.begin(Wire1, FT6X36_SLAVE_ADDRESS, 39, 40);
    touch.setSwapXY(true);
    touch.interruptPolling();

    // Disable RF and NRF Menus for default
    bruceConfig.disabledMenus.push_back("RF");
    bruceConfig.disabledMenus.push_back("NRF24");

    // Haptic driver
    if (!drv.begin(Wire, 10, 11)) {
        Serial.println("Failed to find DRV2605.");
    } else {
        Serial.println("Init DRV2605 Sensor success!");
        drv.selectLibrary(1);
        drv.setMode(SensorDRV2605::MODE_INTTRIG);
        drv.useERM();

        // Startup buzz
        drv.setWaveform(0, 70);
        drv.setWaveform(1, 0);
        drv.run();
    }
    bruceConfigPins.gpsBaudrate = 38400;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 255);
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int percent = axp192.getBatteryPercent();
    return percent;
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 255;
    else if (brightval == 75) dutyCycle = 130;
    else if (brightval == 50) dutyCycle = 70;
    else if (brightval == 25) dutyCycle = 20;
    else if (brightval == 0) dutyCycle = 0;
    else dutyCycle = ((brightval * 255) / 100);

    // log_i("dutyCycle for bright 0-255: %d", dutyCycle);
    ledcWrite(TFT_BL, dutyCycle);
}

bool getTouched() { return digitalRead(16) == LOW; }
struct TP {
    int16_t x[1], y[1];
};
/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    TP t;
    static unsigned long tm = 0;
    if (millis() - tm > 200 || LongPress) {
        // I know R3CK.. I Should NOT nest if statements..
        // but it is needed to not keep SPI bus used without need, it save resources
        if (getTouched()) {
            touch.getPoint(t.x, t.y, 1);
            // Serial.printf("\nRAW: Touch Pressed on x=%d, y=%d",t.x, t.y);
            if (bruceConfigPins.rotation == 3) {
                t.y[0] = (tftHeight + 20) - t.y[0];
                t.x[0] = t.x[0];
            }
            if (bruceConfigPins.rotation == 0) {
                int tmp = t.x[0];
                t.x[0] = tftWidth - t.y[0];
                t.y[0] = tftHeight - tmp;
            }
            if (bruceConfigPins.rotation == 2) {
                int tmp = t.x[0];
                t.x[0] = t.y[0];
                t.y[0] = tmp;
            }
            if (bruceConfigPins.rotation == 1) { t.x[0] = tftWidth - t.x[0]; }
            // Serial.printf("\nROT: Touch Pressed on x=%d, y=%d\n",t.x[0], t.y[0]);

            if (!wakeUpScreen()) AnyKeyPress = true;
            else return;

            // Touch point global variable
            touchPoint.x = t.x[0];
            touchPoint.y = t.y[0];
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);

            tm = millis();
            drv.setWaveform(0, 75);
            drv.setWaveform(1, 0); // end waveform
            drv.run();
        }
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {}

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
    return axp192.isCharging(); // Return the charging status from AXP
}
