/*
MIT License

Copyright (c) 2022 lewis he

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

bool  pmu_flag = 0;
XPowersPMU power;


const uint8_t i2c_sda = CONFIG_PMU_SDA;
const uint8_t i2c_scl = CONFIG_PMU_SCL;
const uint8_t pmu_irq_pin = CONFIG_PMU_IRQ;

void setFlag(void)
{
    pmu_flag = true;
}

void setup()
{
    Serial.begin(115200);

    bool result = power.begin(Wire, AXP2101_SLAVE_ADDRESS, i2c_sda, i2c_scl);

    if (result == false) {
        Serial.println("power is not online..."); while (1)delay(50);
    }

    Serial.printf("getID:0x%x\n", power.getChipID());

    // Force add pull-up
    pinMode(pmu_irq_pin, INPUT_PULLUP);
    attachInterrupt(pmu_irq_pin, setFlag, FALLING);


    // Disable all interrupts
    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

    // Print interrupt register
    power.printIntRegister(&Serial);

    // Clear all interrupt flags
    power.clearIrqStatus();

    // Enable the required interrupt function
    // power.enableIRQ(
    //     XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
    //     XPOWERS_AXP2101_VBUS_INSERT_IRQ   | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS
    //     XPOWERS_AXP2101_PKEY_SHORT_IRQ    | XPOWERS_AXP2101_PKEY_LONG_IRQ       |   //POWER KEY
    //     XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ       //CHARGE
    //     // XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
    // );

    power.enableIRQ(XPOWERS_AXP2101_BAT_NOR_UNDER_TEMP_IRQ);
    // Print AXP2101 interrupt control register
    power.printIntRegister(&Serial);

    power.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ);
    // Print AXP2101 interrupt control register
    power.printIntRegister(&Serial);

    power.enableIRQ(XPOWERS_AXP2101_BAT_OVER_VOL_IRQ);
    // Print AXP2101 interrupt control register
    power.printIntRegister(&Serial);

    // delay(30000);
}

void loop()
{

    if (pmu_flag) {

        pmu_flag = false;

        // Get PMU Interrupt Status Register
        uint32_t status = power.getIrqStatus();
        Serial.print("STATUS => HEX:");
        Serial.print(status, HEX);
        Serial.print(" BIN:");
        Serial.println(status, BIN);

        if (power.isDropWarningLevel2Irq()) {
            Serial.println("isDropWarningLevel2");
        }
        if (power.isDropWarningLevel1Irq()) {
            Serial.println("isDropWarningLevel1");
        }
        if (power.isGaugeWdtTimeoutIrq()) {
            Serial.println("isWdtTimeout");
        }
        if (power.isBatChargerOverTemperatureIrq()) {
            Serial.println("isBatChargeOverTemperature");
        }
        if (power.isBatWorkOverTemperatureIrq()) {
            Serial.println("isBatWorkOverTemperature");
        }
        if (power.isBatWorkUnderTemperatureIrq()) {
            Serial.println("isBatWorkUnderTemperature");
        }
        if (power.isVbusInsertIrq()) {
            Serial.println("isVbusInsert");
        }
        if (power.isVbusRemoveIrq()) {
            Serial.println("isVbusRemove");
        }
        if (power.isBatInsertIrq()) {
            Serial.println("isBatInsert");
        }
        if (power.isBatRemoveIrq()) {
            Serial.println("isBatRemove");
        }
        if (power.isPekeyShortPressIrq()) {
            Serial.println("isPekeyShortPress");
        }
        if (power.isPekeyLongPressIrq()) {
            Serial.println("isPekeyLongPress");
        }
        if (power.isPekeyNegativeIrq()) {
            Serial.println("isPekeyNegative");
        }
        if (power.isPekeyPositiveIrq()) {
            Serial.println("isPekeyPositive");
        }
        if (power.isWdtExpireIrq()) {
            Serial.println("isWdtExpire");
        }
        if (power.isLdoOverCurrentIrq()) {
            Serial.println("isLdoOverCurrentIrq");
        }
        if (power.isBatfetOverCurrentIrq()) {
            Serial.println("isBatfetOverCurrentIrq");
        }
        if (power.isBatChargeDoneIrq()) {
            Serial.println("isBatChargeDone");
        }
        if (power.isBatChargeStartIrq()) {
            Serial.println("isBatChargeStart");
        }
        if (power.isBatDieOverTemperatureIrq()) {
            Serial.println("isBatDieOverTemperature");
        }
        if (power.isChargeOverTimeoutIrq()) {
            Serial.println("isChargeOverTimeout");
        }
        if (power.isBatOverVoltageIrq()) {
            Serial.println("isBatOverVoltage");
        }

        // Clear PMU Interrupt Status Register
        power.clearIrqStatus();

        // Print AXP2101 interrupt control register
        power.printIntRegister(&Serial);

    }
    delay(10);
}

