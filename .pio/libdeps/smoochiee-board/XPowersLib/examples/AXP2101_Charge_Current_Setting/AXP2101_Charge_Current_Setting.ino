/*
MIT License

Copyright (c) 2024 lewis he

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
! WARN:
Please do not run the example without knowing the external load voltage of the PMU,
it may burn your external load, please check the voltage setting before running the example,
if there is any loss, please bear it by yourself
*/
#ifndef XPOWERS_NO_ERROR
#error "Running this example is known to not damage the device! Please go and uncomment this!"
#endif
// Defined using AXP2102
#define XPOWERS_CHIP_AXP2101

#include <Wire.h>
#include <Arduino.h>
#include "XPowersLib.h"

#ifndef CONFIG_PMU_SDA
#define CONFIG_PMU_SDA 21
#endif

#ifndef CONFIG_PMU_SCL
#define CONFIG_PMU_SCL 22
#endif

#ifndef CONFIG_PMU_IRQ
#define CONFIG_PMU_IRQ 35
#endif

XPowersPMU power;

bool  pmu_flag = false;
uint8_t curIndex = 4;

void setFlag(void)
{
    pmu_flag = true;
}


void printChargerConstantCurr()
{
    const uint16_t currTable[] = {
        0, 0, 0, 0, 100, 125, 150, 175, 200, 300, 400, 500, 600, 700, 800, 900, 1000
    };
    uint8_t val = power.getChargerConstantCurr();
    Serial.printf("Setting Charge Target Current - VAL:%u CURRENT:%u\n", val, currTable[val]);
}

void setup()
{

    Serial.begin(115200);

    //Start while waiting for Serial monitoring
    while (!Serial);

    delay(3000);

    Serial.println();

    bool res = power.begin(Wire, AXP2101_SLAVE_ADDRESS, CONFIG_PMU_SDA, CONFIG_PMU_SCL);
    if (!res) {
        Serial.println("Failed to initialize power.....");
        while (1) {
            delay(5000);
        }
    }

    // Set power interrupt
    pinMode(CONFIG_PMU_IRQ, INPUT);
    attachInterrupt(CONFIG_PMU_IRQ, setFlag, FALLING);

    // Disable all interrupts
    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

    // Enable the required interrupt function
    power.enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
        XPOWERS_AXP2101_VBUS_INSERT_IRQ   | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS
        XPOWERS_AXP2101_PKEY_SHORT_IRQ    | XPOWERS_AXP2101_PKEY_LONG_IRQ       |   //POWER KEY
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ       //CHARGE
    );
    // Clear all interrupt flags
    power.clearIrqStatus();

    // Set the minimum common working voltage of the PMU VBUS input,
    // below this value will turn off the power
    power.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);
    // Set the maximum current of the PMU VBUS input,
    // higher than this value will turn off the PMU
    power.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);
    // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
    power.setSysPowerDownVoltage(2600);
    // Set the precharge charging current
    power.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    // Set constant current charge current limit
    power.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_200MA);
    // Set stop charging termination current
    power.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
    // Set charge cut-off voltage
    power.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V1);

    /*
    The default setting is CHGLED is automatically controlled by the PMU.
    - XPOWERS_CHG_LED_OFF,
    - XPOWERS_CHG_LED_BLINK_1HZ,
    - XPOWERS_CHG_LED_BLINK_4HZ,
    - XPOWERS_CHG_LED_ON,
    - XPOWERS_CHG_LED_CTRL_CHG,
    * */
    power.setChargingLedMode(XPOWERS_CHG_LED_OFF);
    // Set cell battery voltage to 3.3V
    power.setButtonBatteryChargeVoltage(3300);
    // Enable cell battery charge function
    power.enableButtonBatteryCharge();
    // Print default setting current
    printChargerConstantCurr();
}

void loop()
{
    if (pmu_flag) {
        pmu_flag = false;
        // Get PMU Interrupt Status Register
        power.getIrqStatus();
        if (power.isPekeyShortPressIrq()) {

            // Set constant current charge current limit
            // For setting a current greater than 500mA, the VBUS power supply must be sufficient. If the input is lower than 5V, the charging current will be below 500mA.
            if (!power.setChargerConstantCurr(curIndex)) {
                Serial.println("Setting Charger Constant Current Failed!");
            }
            printChargerConstantCurr();
            Serial.println("===========================");
            curIndex++;
            curIndex %= (XPOWERS_AXP2101_CHG_CUR_1000MA + 1);
            if (curIndex == 0) {
                curIndex = 3;
            }

            // Toggle CHG led
            power.setChargingLedMode(power.getChargingLedMode() != XPOWERS_CHG_LED_OFF ? XPOWERS_CHG_LED_OFF : XPOWERS_CHG_LED_ON);
        }
        // Clear PMU Interrupt Status Register
        power.clearIrqStatus();
    }

    delay(200);
}

