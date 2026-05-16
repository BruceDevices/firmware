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

    vol = power.getSysPowerDownVoltage();
    Serial.printf("->  getSysPowerDownVoltage:%u\n", vol);


    // DC1 IMAX=2A
    // 1500~3400mV,100mV/step,20steps
    power.setDC1Voltage(3300);
    Serial.printf("DC1  : %s   Voltage:%u mV \n",  power.isEnableDC1()  ? "+" : "-", power.getDC1Voltage());

    // DC2 IMAX=2A
    // 500~1200mV  10mV/step,71steps
    // 1220~1540mV 20mV/step,17steps
    power.setDC2Voltage(1000);
    Serial.printf("DC2  : %s   Voltage:%u mV \n",  power.isEnableDC2()  ? "+" : "-", power.getDC2Voltage());

    // DC3 IMAX = 2A
    // 500~1200mV,10mV/step,71steps
    // 1220~1540mV,20mV/step,17steps
    // 1600~3400mV,100mV/step,19steps
    power.setDC3Voltage(3300);
    Serial.printf("DC3  : %s   Voltage:%u mV \n",  power.isEnableDC3()  ? "+" : "-", power.getDC3Voltage());

    // DCDC4 IMAX=1.5A
    // 500~1200mV,10mV/step,71steps
    // 1220~1840mV,20mV/step,32steps
    power.setDC4Voltage(1000);
    Serial.printf("DC4  : %s   Voltage:%u mV \n",  power.isEnableDC4()  ? "+" : "-", power.getDC4Voltage());

    // DC5 IMAX=2A
    // 1200mV
    // 1400~3700mV,100mV/step,24steps
    power.setDC5Voltage(3300);
    Serial.printf("DC5  : %s   Voltage:%u mV \n",  power.isEnableDC5()  ? "+" : "-", power.getDC5Voltage());

    //ALDO1 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    power.setALDO1Voltage(3300);

    //ALDO2 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    power.setALDO2Voltage(3300);

    //ALDO3 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    power.setALDO3Voltage(3300);

    //ALDO4 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    power.setALDO4Voltage(3300);

    //BLDO1 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    power.setBLDO1Voltage(3300);

    //BLDO2 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    power.setBLDO2Voltage(3300);

    //CPUSLDO IMAX=30mA
    //500~1400mV,50mV/step,19steps
    power.setCPUSLDOVoltage(1000);

    //DLDO1 IMAX=300mA
    //500~3400mV, 100mV/step,29steps
    power.setDLDO1Voltage(3300);

    //DLDO2 IMAX=300mA
    //500~1400mV, 50mV/step,2steps
    power.setDLDO2Voltage(3300);


    // power.enableDC1();
    power.enableDC2();
    power.enableDC3();
    power.enableDC4();
    power.enableDC5();
    power.enableALDO1();
    power.enableALDO2();
    power.enableALDO3();
    power.enableALDO4();
    power.enableBLDO1();
    power.enableBLDO2();
    power.enableCPUSLDO();
    power.enableDLDO1();
    power.enableDLDO2();


    Serial.println("DCDC=======================================================================");
    Serial.printf("DC1  : %s   Voltage:%u mV \n",  power.isEnableDC1()  ? "+" : "-", power.getDC1Voltage());
    Serial.printf("DC2  : %s   Voltage:%u mV \n",  power.isEnableDC2()  ? "+" : "-", power.getDC2Voltage());
    Serial.printf("DC3  : %s   Voltage:%u mV \n",  power.isEnableDC3()  ? "+" : "-", power.getDC3Voltage());
    Serial.printf("DC4  : %s   Voltage:%u mV \n",  power.isEnableDC4()  ? "+" : "-", power.getDC4Voltage());
    Serial.printf("DC5  : %s   Voltage:%u mV \n",  power.isEnableDC5()  ? "+" : "-", power.getDC5Voltage());
    Serial.println("ALDO=======================================================================");
    Serial.printf("ALDO1: %s   Voltage:%u mV\n",  power.isEnableALDO1()  ? "+" : "-", power.getALDO1Voltage());
    Serial.printf("ALDO2: %s   Voltage:%u mV\n",  power.isEnableALDO2()  ? "+" : "-", power.getALDO2Voltage());
    Serial.printf("ALDO3: %s   Voltage:%u mV\n",  power.isEnableALDO3()  ? "+" : "-", power.getALDO3Voltage());
    Serial.printf("ALDO4: %s   Voltage:%u mV\n",  power.isEnableALDO4()  ? "+" : "-", power.getALDO4Voltage());
    Serial.println("BLDO=======================================================================");
    Serial.printf("BLDO1: %s   Voltage:%u mV\n",  power.isEnableBLDO1()  ? "+" : "-", power.getBLDO1Voltage());
    Serial.printf("BLDO2: %s   Voltage:%u mV\n",  power.isEnableBLDO2()  ? "+" : "-", power.getBLDO2Voltage());
    Serial.println("CPUSLDO====================================================================");
    Serial.printf("CPUSLDO: %s Voltage:%u mV\n",  power.isEnableCPUSLDO() ? "+" : "-", power.getCPUSLDOVoltage());
    Serial.println("DLDO=======================================================================");
    Serial.printf("DLDO1: %s   Voltage:%u mV\n",  power.isEnableDLDO1()  ? "+" : "-", power.getDLDO1Voltage());
    Serial.printf("DLDO2: %s   Voltage:%u mV\n",  power.isEnableDLDO2()  ? "+" : "-", power.getDLDO2Voltage());
    Serial.println("===========================================================================");

    // Set the time of pressing the button to turn off
    power.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    uint8_t opt = power.getPowerKeyPressOffTime();
    Serial.print("PowerKeyPressOffTime:");
    switch (opt) {
    case XPOWERS_POWEROFF_4S: Serial.println("4 Second");
        break;
    case XPOWERS_POWEROFF_6S: Serial.println("6 Second");
        break;
    case XPOWERS_POWEROFF_8S: Serial.println("8 Second");
        break;
    case XPOWERS_POWEROFF_10S: Serial.println("10 Second");
        break;
    default:
        break;
    }
    // Set the button power-on press time
    power.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
    opt = power.getPowerKeyPressOnTime();
    Serial.print("PowerKeyPressOnTime:");
    switch (opt) {
    case XPOWERS_POWERON_128MS: Serial.println("128 Ms");
        break;
    case XPOWERS_POWERON_512MS: Serial.println("512 Ms");
        break;
    case XPOWERS_POWERON_1S: Serial.println("1 Second");
        break;
    case XPOWERS_POWERON_2S: Serial.println("2 Second");
        break;
    default:
        break;
    }

    Serial.println("===========================================================================");

    bool en;

    // DCDC 120%(130%) high voltage turn off PMIC function
    en = power.getDCHighVoltagePowerDownEn();
    Serial.print("getDCHighVoltagePowerDownEn:");
    Serial.println(en ? "ENABLE" : "DISABLE");
    // DCDC1 85% low voltage turn off PMIC function
    en = power.getDC1LowVoltagePowerDownEn();
    Serial.print("getDC1LowVoltagePowerDownEn:");
    Serial.println(en ? "ENABLE" : "DISABLE");
    // DCDC2 85% low voltage turn off PMIC function
    en = power.getDC2LowVoltagePowerDownEn();
    Serial.print("getDC2LowVoltagePowerDownEn:");
    Serial.println(en ? "ENABLE" : "DISABLE");
    // DCDC3 85% low voltage turn off PMIC function
    en = power.getDC3LowVoltagePowerDownEn();
    Serial.print("getDC3LowVoltagePowerDownEn:");
    Serial.println(en ? "ENABLE" : "DISABLE");
    // DCDC4 85% low voltage turn off PMIC function
    en = power.getDC4LowVoltagePowerDownEn();
    Serial.print("getDC4LowVoltagePowerDownEn:");
    Serial.println(en ? "ENABLE" : "DISABLE");
    // DCDC5 85% low voltage turn off PMIC function
    en = power.getDC5LowVoltagePowerDownEn();
    Serial.print("getDC5LowVoltagePowerDownEn:");
    Serial.println(en ? "ENABLE" : "DISABLE");

    // power.setDCHighVoltagePowerDown(true);
    // power.setDC1LowVoltagePowerDown(true);
    // power.setDC2LowVoltagePowerDown(true);
    // power.setDC3LowVoltagePowerDown(true);
    // power.setDC4LowVoltagePowerDown(true);
    // power.setDC5LowVoltagePowerDown(true);

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
    power.setChargingLedMode(XPOWERS_CHG_LED_OFF);


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
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ       //CHARGE
        // XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
    );

    // Set the precharge charging current
    power.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    // Set constant current charge current limit
    power.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_200MA);
    // Set stop charging termination current
    power.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);

    // Set charge cut-off voltage
    power.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V1);

    // Set the watchdog trigger event type
    power.setWatchdogConfig(XPOWERS_AXP2101_WDT_IRQ_TO_PIN);
    // Set watchdog timeout
    power.setWatchdogTimeout(XPOWERS_AXP2101_WDT_TIMEOUT_4S);
    // Enable watchdog to trigger interrupt event
    power.enableWatchdog();

    // power.disableWatchdog();

    // Enable Button Battery charge
    power.enableButtonBatteryCharge();

    // Set Button Battery charge voltage
    power.setButtonBatteryChargeVoltage(3300);

}

void printPMU()
{
    Serial.print("isCharging:"); Serial.println(power.isCharging() ? "YES" : "NO");
    Serial.print("isDischarge:"); Serial.println(power.isDischarge() ? "YES" : "NO");
    Serial.print("isStandby:"); Serial.println(power.isStandby() ? "YES" : "NO");
    Serial.print("isVbusIn:"); Serial.println(power.isVbusIn() ? "YES" : "NO");
    Serial.print("isVbusGood:"); Serial.println(power.isVbusGood() ? "YES" : "NO");
    Serial.print("getChargerStatus:");
    uint8_t charge_status = power.getChargerStatus();
    if (charge_status == XPOWERS_AXP2101_CHG_TRI_STATE) {
        Serial.println("tri_charge");
    } else if (charge_status == XPOWERS_AXP2101_CHG_PRE_STATE) {
        Serial.println("pre_charge");
    } else if (charge_status == XPOWERS_AXP2101_CHG_CC_STATE) {
        Serial.println("constant charge");
    } else if (charge_status == XPOWERS_AXP2101_CHG_CV_STATE) {
        Serial.println("constant voltage");
    } else if (charge_status == XPOWERS_AXP2101_CHG_DONE_STATE) {
        Serial.println("charge done");
    } else if (charge_status == XPOWERS_AXP2101_CHG_STOP_STATE) {
        Serial.println("not charge");
    }

    Serial.print("getBattVoltage:"); Serial.print(power.getBattVoltage()); Serial.println("mV");
    Serial.print("getVbusVoltage:"); Serial.print(power.getVbusVoltage()); Serial.println("mV");
    Serial.print("getSystemVoltage:"); Serial.print(power.getSystemVoltage()); Serial.println("mV");

    // The battery percentage may be inaccurate at first use, the PMU will automatically
    // learn the battery curve and will automatically calibrate the battery percentage
    // after a charge and discharge cycle
    if (power.isBatteryConnect()) {
        Serial.print("getBatteryPercent:"); Serial.print(power.getBatteryPercent()); Serial.println("%");
    }

    Serial.println();
}



void enterPmuSleep(void)
{
    // Set the wake-up source to PWRKEY
    power.wakeupControl(XPOWERS_AXP2101_WAKEUP_IRQ_PIN_TO_LOW, true);

    // Set sleep flag
    power.enableSleep();

    power.disableDC2();
    power.disableDC3();
    power.disableDC4();
    power.disableDC5();

    power.disableALDO1();
    power.disableALDO2();
    power.disableALDO3();
    power.disableALDO4();

    power.disableBLDO1();
    power.disableBLDO2();

    power.disableCPUSLDO();
    power.disableDLDO1();
    power.disableDLDO2();

    // Finally, turn off the power of the control chip
    power.disableDC1();
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
            // enterPmuSleep();

            Serial.print("Read pmu data buffer .");
            uint8_t data[4] = {0};
            power.readDataBuffer(data, XPOWERS_AXP2101_DATA_BUFFER_SIZE);
            for (int i = 0; i < 4; ++i) {
                Serial.print(data[i]);
                Serial.print(",");
            }
            Serial.println();
        }

        if (power.isPekeyLongPressIrq()) {
            Serial.println("isPekeyLongPress");
            Serial.println("write pmu data buffer .");
            uint8_t data[4] = {1, 2, 3, 4};
            power.writeDataBuffer(data, XPOWERS_AXP2101_DATA_BUFFER_SIZE);
        }

        if (power.isPekeyNegativeIrq()) {
            Serial.println("isPekeyNegative");
        }
        if (power.isPekeyPositiveIrq()) {
            Serial.println("isPekeyPositive");
        }
        if (power.isWdtExpireIrq()) {
            Serial.println("isWdtExpire");
            printPMU();
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

