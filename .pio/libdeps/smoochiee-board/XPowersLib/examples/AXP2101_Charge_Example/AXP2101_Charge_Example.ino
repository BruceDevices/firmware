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

    // Set the minimum common working voltage of the PMU VBUS input,
    // below this value will turn off the PMU
    power.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);

    // Set the maximum current of the PMU VBUS input,
    // higher than this value will turn off the PMU
    power.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);


    // Get the VSYS shutdown voltage
    uint16_t vol = power.getSysPowerDownVoltage();
    Serial.printf("->  getSysPowerDownVoltage:%u\n", vol);

    // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
    power.setSysPowerDownVoltage(2600);

    // It is necessary to disable the detection function of the TS pin on the board
    // without the battery temperature detection function, otherwise it will cause abnormal charging
    power.disableTSPinMeasure();

    // power.enableTemperatureMeasure();

    // Enable internal ADC detection
    power.enableBattDetection();
    power.enableVbusVoltageMeasure();
    power.enableBattVoltageMeasure();
    power.enableSystemVoltageMeasure();

    /*
      The default setting is CHGLED is automatically controlled by the PMU.
    - XPOWERS_CHG_LED_OFF,
    - XPOWERS_CHG_LED_BLINK_1HZ,
    - XPOWERS_CHG_LED_BLINK_4HZ,
    - XPOWERS_CHG_LED_ON,
    - XPOWERS_CHG_LED_CTRL_CHG,
    * */
    power.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);


    // Force add pull-up
    pinMode(pmu_irq_pin, INPUT_PULLUP);
    attachInterrupt(pmu_irq_pin, setFlag, FALLING);


    // Disable all interrupts
    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    // Clear all interrupt flags
    power.clearIrqStatus();
    // Enable the required interrupt function
    power.enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
        XPOWERS_AXP2101_VBUS_INSERT_IRQ   | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS
        XPOWERS_AXP2101_PKEY_SHORT_IRQ    | XPOWERS_AXP2101_PKEY_LONG_IRQ       |   //POWER KEY
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ   |    //CHARGE
        XPOWERS_AXP2101_WARNING_LEVEL1_IRQ | XPOWERS_AXP2101_WARNING_LEVEL2_IRQ     //Low battery warning
        // XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
    );

    // Set the precharge charging current
    power.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_200MA);


    // Set stop charging termination current
    power.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);


    // Set constant current charge current limit
    if (!power.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_1000MA)) {
        Serial.println("Setting Charger Constant Current Failed!");
    }

    const uint16_t currTable[] = {
        0, 0, 0, 0, 100, 125, 150, 175, 200, 300, 400, 500, 600, 700, 800, 900, 1000
    };
    uint8_t val = power.getChargerConstantCurr();
    Serial.print("Setting Charge Target Current : ");
    Serial.println(currTable[val]);

    // Set charge cut-off voltage
    power.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

    const uint16_t tableVoltage[] = {
        0, 4000, 4100, 4200, 4350, 4400, 255
    };
    val = power.getChargeTargetVoltage();
    Serial.print("Setting Charge Target Voltage : ");
    Serial.println(tableVoltage[val]);


    // Set the power level to be lower than 15% and send an interrupt to the host`
    power.setLowBatWarnThreshold(10);

    // Set the power level to be lower than 5% and turn off the power supply
    power.setLowBatShutdownThreshold(5);

    /*
    Turn on the learning battery curve,
    And write the learned battery curve into the ROM
    */
    power.fuelGaugeControl(true, true);

    Serial.println();

    delay(5000);
}

void printPMU()
{
    Serial.println("---------------------------------------------------------------------------------------------------------");
    Serial.println("Satus1  Satus2  CHARG   DISC   STBY    VBUSIN    VGOOD    VBAT   VBUS   VSYS   Percentage    CHG_STATUS");
    Serial.println("(Bin)   (Bin)   (bool)  (bool) (bool)  (bool)    (bool)   (mV)   (mV)   (mV)      (%)           (str)  ");
    Serial.println("---------------------------------------------------------------------------------------------------------");
    uint16_t statusVal =  power.status();
    Serial.print("0b"); Serial.print(statusVal >> 8, BIN); Serial.print("\t");
    Serial.print("0b"); Serial.print(statusVal & 0xFF, BIN); Serial.print("\t");
    Serial.print(power.isCharging() ? "YES" : "NO "); Serial.print("\t");
    Serial.print(power.isDischarge() ? "YES" : "NO "); Serial.print("\t");
    Serial.print(power.isStandby() ? "YES" : "NO "); Serial.print("\t");
    Serial.print(power.isVbusIn() ? "YES" : "NO "); Serial.print("\t");
    Serial.print(power.isVbusGood() ? "YES" : "NO "); Serial.print("\t");
    Serial.print(power.getBattVoltage());     Serial.print("\t");
    Serial.print(power.getVbusVoltage());     Serial.print("\t");
    Serial.print(power.getSystemVoltage());   Serial.print("\t");

    // The battery percentage may be inaccurate at first use, the PMU will automatically
    // learn the battery curve and will automatically calibrate the battery percentage
    // after a charge and discharge cycle
    Serial.print(power.getBatteryPercent()); Serial.print("\t");

    uint8_t charge_status = power.getChargerStatus();
    if (charge_status == XPOWERS_AXP2101_CHG_TRI_STATE) {
        Serial.println("tri_charge");
    } else if (charge_status == XPOWERS_AXP2101_CHG_PRE_STATE) {
        Serial.println("pre_charge");
    } else if (charge_status == XPOWERS_AXP2101_CHG_CC_STATE) {
        Serial.println("constant charge(CC)");
    } else if (charge_status == XPOWERS_AXP2101_CHG_CV_STATE) {
        Serial.println("constant voltage(CV)");
    } else if (charge_status == XPOWERS_AXP2101_CHG_DONE_STATE) {
        Serial.println("charge done");
    } else if (charge_status == XPOWERS_AXP2101_CHG_STOP_STATE) {
        Serial.println("not charge");
    }
    Serial.println();

}

uint32_t printTime = 0;
void loop()
{

    if (millis() > printTime) {
        printTime = millis() + 2000;
        printPMU();
    }


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
            Serial.println(" >>>>   isDropWarningLevel1 <<<<");
            power.shutdown();
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
            uint8_t val = power.getVbusCurrentLimit();
            Serial.print("Get Vbus Current Limit = "); Serial.println(val);
        }
        if (power.isVbusRemoveIrq()) {
            Serial.println("isVbusRemove");
            uint8_t val = power.getVbusCurrentLimit();
            Serial.print("Get Vbus Current Limit = "); Serial.println(val);
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

    }
    delay(10);
}

