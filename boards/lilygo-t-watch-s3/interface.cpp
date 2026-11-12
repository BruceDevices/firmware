#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <Wire.h>
#include <XPowersLib.h>
#include <interface.h>

XPowersAXP2101 axp192;

// Haptic
#include "HapticDrivers.hpp"
HapticDriver_DRV2605 drv;

// FT6X36 over Wire1 (39, 40). Derived algebraically from the pre-HAL per-rotation remap
// InputHandler used to do -- see src/hal/README.md for the swap/mirror table method.
static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    cfg.pin_sda = 39;
    cfg.pin_scl = 40;
    cfg.pin_irq = 16;
    cfg.i2c_bus = &Wire1;
    // rotation:        0      1      2      3
    const bool swapXY[4] = {true, false, true, false};
    const bool mirrorX[4] = {true, true, false, false};
    const bool mirrorY[4] = {true, false, false, true};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // bclk,ws,dout,mclk
    bruceConfigPins.speaker_bus = {(gpio_num_t)48, (gpio_num_t)15, (gpio_num_t)46, (gpio_num_t)44};
    bruceConfigPins.mic_bus = {(gpio_num_t)44, (gpio_num_t)47, GPIO_NUM_NC, MIC_TYPE_PDM}; // clk,data,ws,type
    bruceConfigPins.i2c_bus = {(gpio_num_t)10, (gpio_num_t)11};  // sda, scl (Grove)
    bruceConfigPins.sys_i2c = {(gpio_num_t)10, (gpio_num_t)11};  // sda, scl
    bruceConfigPins.rfTx = 10;
    bruceConfigPins.rfRx = 11;
    bruceConfigPins.irTx = 2;
    bruceConfigPins.irRx = 11;
    bruceConfigPins.rotation = 2;
    bruceConfigPins.uart_bus = {(gpio_num_t)41, (gpio_num_t)42};    // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)41, (gpio_num_t)42};     // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)11, (gpio_num_t)10};  // rx, tx (CH9329)
    // Disabled/not present on this board (module not populated) - kept explicit for documentation
    bruceConfigPins.CC1101_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
    bruceConfigPins.NRF24_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
    bruceConfigPins.SDCARD_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)3, (gpio_num_t)4, (gpio_num_t)1, (gpio_num_t)5, (gpio_num_t)8, (gpio_num_t)9
    }; // sck,miso,mosi,cs,rst,dio0
#endif

    // NOTE: this board permanently reserves BOTH hardware I2C controllers for system peripherals
    // (sensors/PMU/RTC on Wire, touch on Wire1) — bus_HAL only tracks one "sys" bus, so
    // bruceConfigPins.i2c_bus only has a real hardware bus free if its pins match one of these two.
    setSysI2CBus(&Wire);
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

    // Disable RF and NRF Menus for default
    bruceConfig.disabledMenus.push_back("RF");
    bruceConfig.disabledMenus.push_back("NRF24");

    // Haptic driver
    if (!drv.begin(Wire, 10, 11)) {
        Serial.println("Failed to find DRV2605.");
    } else {
        Serial.println("Init DRV2605 Sensor success!");
        drv.selectLibrary(1);
        drv.setMode(HapticMode::INTERNAL_TRIGGER);
        drv.setActuatorType(HapticActuatorType::ERM);

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
    if (!hal_touch_init(touchCfg(), 0)) { Serial.println("Touch IC not Started"); }
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
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
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm > 200 || LongPress) {
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            tm = millis();
            if (!hal_touch_apply(t)) return;
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
