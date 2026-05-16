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

// Defined using AXP192
#define XPOWERS_CHIP_AXP192

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

    bool result = power.begin(Wire, AXP192_SLAVE_ADDRESS, i2c_sda, i2c_scl);

    if (result == false) {
        Serial.println("power is not online..."); while (1)delay(50);
    }


    Serial.printf("getID:0x%x\n", power.getChipID());

    // Set the minimum system operating voltage inside the PMU,
    // below this value will shut down the PMU
    // Range: 2600~3300mV
    power.setSysPowerDownVoltage(2700);

    // Set the minimum common working voltage of the PMU VBUS input,
    // below this value will turn off the PMU
    power.setVbusVoltageLimit(XPOWERS_AXP192_VBUS_VOL_LIM_4V5);

    // Turn off USB input current limit
    power.setVbusCurrentLimit(XPOWERS_AXP192_VBUS_CUR_LIM_OFF);

    // DC1 700~3500mV, IMAX=1.2A
    power.setDC1Voltage(3300);
    Serial.printf("DC1  :%s   Voltage:%u mV \n",  power.isEnableDC1()  ? "+" : "-", power.getDC1Voltage());

    // DC2 700~2750 mV, IMAX=1.6A;
    power.setDC2Voltage(700);
    Serial.printf("DC2  :%s   Voltage:%u mV \n",  power.isEnableDC2()  ? "+" : "-", power.getDC2Voltage());

    // DC3 700~3500 mV,IMAX=0.7A;
    power.setDC3Voltage(3300);
    Serial.printf("DC3  :%s   Voltage:%u mV \n",  power.isEnableDC3()  ? "+" : "-", power.getDC3Voltage());


    //LDO2 1800~3300 mV, 100mV/step, IMAX=200mA
    power.setLDO2Voltage(1800);

    //LDO3 1800~3300 mV, 100mV/step, IMAX=200mA
    power.setLDO3Voltage(1800);

    //LDOio 1800~3300 mV, 100mV/step, IMAX=50mA
    power.setLDOioVoltage(3300);


    // Enable PMU output channel
    // power.enableDC1();
    power.enableDC2();
    power.enableDC3();
    power.enableLDO2();
    power.enableLDO3();
    power.enableLDOio();

    Serial.println("DCDC=======================================================================");
    Serial.printf("DC1  :%s   Voltage:%u mV \n",  power.isEnableDC1()  ? "ENABLE" : "DISABLE", power.getDC1Voltage());
    Serial.printf("DC2  :%s   Voltage:%u mV \n",  power.isEnableDC2()  ? "ENABLE" : "DISABLE", power.getDC2Voltage());
    Serial.printf("DC3  :%s   Voltage:%u mV \n",  power.isEnableDC3()  ? "ENABLE" : "DISABLE", power.getDC3Voltage());
    Serial.println("LDO=======================================================================");
    Serial.printf("LDO2: %s   Voltage:%u mV\n",  power.isEnableLDO2()  ? "ENABLE" : "DISABLE", power.getLDO2Voltage());
    Serial.printf("LDO3: %s   Voltage:%u mV\n",  power.isEnableLDO3()  ? "ENABLE" : "DISABLE", power.getLDO3Voltage());
    Serial.printf("LDOio: %s   Voltage:%u mV\n",  power.isEnableLDOio()  ? "ENABLE" : "DISABLE", power.getLDOioVoltage());
    Serial.println("==========================================================================");

    // Set the time of pressing the button to turn off
    power.setPowerKeyPressOffTime(XPOWERS_AXP192_POWEROFF_4S);
    uint8_t opt = power.getPowerKeyPressOffTime();
    Serial.print("PowerKeyPressOffTime:");
    switch (opt) {
    case XPOWERS_AXP192_POWEROFF_4S: Serial.println("4 Second");
        break;
    case XPOWERS_AXP192_POWEROFF_65: Serial.println("6 Second");
        break;
    case XPOWERS_AXP192_POWEROFF_8S: Serial.println("8 Second");
        break;
    case XPOWERS_AXP192_POWEROFF_10S: Serial.println("10 Second");
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

    // It is necessary to disable the detection function of the TS pin on the board
    // without the battery temperature detection function, otherwise it will cause abnormal charging
    power.disableTSPinMeasure();

    // power.enableTemperatureMeasure();
    // power.disableTemperatureMeasure();

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


    pinMode(pmu_irq_pin, INPUT);
    attachInterrupt(pmu_irq_pin, setFlag, FALLING);

    // Disable all interrupts
    power.disableIRQ(XPOWERS_AXP192_ALL_IRQ);
    // Clear all interrupt flags
    power.clearIrqStatus();
    // Enable the required interrupt function
    power.enableIRQ(
        XPOWERS_AXP192_BAT_INSERT_IRQ    | XPOWERS_AXP192_BAT_REMOVE_IRQ      |   //BATTERY
        XPOWERS_AXP192_VBUS_INSERT_IRQ   | XPOWERS_AXP192_VBUS_REMOVE_IRQ     |   //VBUS
        XPOWERS_AXP192_PKEY_SHORT_IRQ    | XPOWERS_AXP192_PKEY_LONG_IRQ       |   //POWER KEY
        XPOWERS_AXP192_BAT_CHG_DONE_IRQ  | XPOWERS_AXP192_BAT_CHG_START_IRQ   |    //CHARGE
        // XPOWERS_AXP192_PKEY_NEGATIVE_IRQ | XPOWERS_AXP192_PKEY_POSITIVE_IRQ   |   //POWER KEY
        XPOWERS_AXP192_TIMER_TIMEOUT_IRQ               //Timer
    );

    // Set constant current charge current limit
    power.setChargerConstantCurr(XPOWERS_AXP192_CHG_CUR_280MA);
    // Set stop charging termination current
    power.setChargerTerminationCurr(XPOWERS_AXP192_CHG_ITERM_LESS_10_PERCENT);

    // Set charge cut-off voltage
    power.setChargeTargetVoltage(XPOWERS_AXP192_CHG_VOL_4V2);

    // Cache writes and reads, as long as the PMU remains powered, the data will always be stored inside the PMU
    Serial.println("Write pmu data buffer .");
    uint8_t data[XPOWERS_AXP192_DATA_BUFFER_SIZE] = {1, 2, 3, 4, 5, 6};
    power.writeDataBuffer(data, XPOWERS_AXP192_DATA_BUFFER_SIZE);
    memset(data, 0, XPOWERS_AXP192_DATA_BUFFER_SIZE);

    Serial.print("Read pmu data buffer :");
    power.readDataBuffer(data, XPOWERS_AXP192_DATA_BUFFER_SIZE);
    for (int i = 0; i < XPOWERS_AXP192_DATA_BUFFER_SIZE; ++i) {
        Serial.print(data[i]);
        Serial.print(",");
    }
    Serial.println();

    // Set the timing after one minute, the isWdtExpireIrq will be triggered in the loop interrupt function
    power.setTimerout(1);
}

void printPMU()
{
    Serial.print("isCharging:"); Serial.println(power.isCharging() ? "YES" : "NO");
    Serial.print("isDischarge:"); Serial.println(power.isDischarge() ? "YES" : "NO");
    Serial.print("isVbusIn:"); Serial.println(power.isVbusIn() ? "YES" : "NO");
    Serial.print("getBattVoltage:"); Serial.print(power.getBattVoltage()); Serial.println("mV");
    Serial.print("getVbusVoltage:"); Serial.print(power.getVbusVoltage()); Serial.println("mV");
    Serial.print("getSystemVoltage:"); Serial.print(power.getSystemVoltage()); Serial.println("mV");
    Serial.print("getTemperature:"); Serial.print(power.getTemperature()); Serial.println("*C");

    if (power.isBatteryConnect()) {
        Serial.print("getBatteryPercent:"); Serial.print(power.getBatteryPercent()); Serial.println("%");
    }

    Serial.println();
}



void enterPmuSleep(void)
{
    // Set sleep flag
    power.enableSleep();

    power.disableDC2();
    power.disableDC3();

    power.disableLDO2();
    power.disableLDO3();

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

        if (power.isAcinOverVoltageIrq()) {
            Serial.println("isAcinOverVoltageIrq");
        }
        if (power.isAcinInserIrq()) {
            Serial.println("isAcinInserIrq");
        }
        if (power.isAcinRemoveIrq()) {
            Serial.println("isAcinRemoveIrq");
        }
        if (power.isVbusOverVoltageIrq()) {
            Serial.println("isVbusOverVoltageIrq");
        }
        if (power.isVbusInsertIrq()) {
            Serial.println("isVbusInsertIrq");
        }
        if (power.isVbusRemoveIrq()) {
            Serial.println("isVbusRemoveIrq");
        }
        if (power.isVbusLowVholdIrq()) {
            Serial.println("isVbusLowVholdIrq");
        }
        if (power.isBatInsertIrq()) {
            Serial.println("isBatInsertIrq");
        }
        if (power.isBatRemoveIrq()) {
            Serial.println("isBatRemoveIrq");
        }
        if (power.isBattEnterActivateIrq()) {
            Serial.println("isBattEnterActivateIrq");
        }
        if (power.isBattExitActivateIrq()) {
            Serial.println("isBattExitActivateIrq");
        }
        if (power.isBatChargeStartIrq()) {
            Serial.println("isBatChargeStartIrq");
        }
        if (power.isBatChargeDoneIrq()) {
            Serial.println("isBatChargeDoneIrq");
        }
        if (power.isBattTempHighIrq()) {
            Serial.println("isBattTempHighIrq");
        }
        if (power.isBattTempLowIrq()) {
            Serial.println("isBattTempLowIrq");
        }
        if (power.isChipOverTemperatureIrq()) {
            Serial.println("isChipOverTemperatureIrq");
        }
        if (power.isChargingCurrentLessIrq()) {
            Serial.println("isChargingCurrentLessIrq");
        }
        if (power.isDC1VoltageLessIrq()) {
            Serial.println("isDC1VoltageLessIrq");
        }
        if (power.isDC2VoltageLessIrq()) {
            Serial.println("isDC2VoltageLessIrq");
        }
        if (power.isDC3VoltageLessIrq()) {
            Serial.println("isDC3VoltageLessIrq");
        }
        if (power.isPekeyShortPressIrq()) {
            Serial.println("isPekeyShortPress");

            // enterPmuSleep();

            //CHG LED mode test
            uint8_t m =  power.getChargingLedMode();
            Serial.print("getChargingLedMode:");
            Serial.println(m++);
            m %= XPOWERS_CHG_LED_CTRL_CHG;
            Serial.printf("setChargingLedMode:%u", m);
            power.setChargingLedMode(m);

        }
        if (power.isPekeyLongPressIrq()) {
            Serial.println("isPekeyLongPress");

        }
        if (power.isNOEPowerOnIrq()) {
            Serial.println("isNOEPowerOnIrq");
        }
        if (power.isNOEPowerDownIrq()) {
            Serial.println("isNOEPowerDownIrq");
        }
        if (power.isVbusEffectiveIrq()) {
            Serial.println("isVbusEffectiveIrq");
        }
        if (power.isVbusInvalidIrq()) {
            Serial.println("isVbusInvalidIrq");
        }
        if (power.isVbusSessionIrq()) {
            Serial.println("isVbusSessionIrq");
        }
        if (power.isVbusSessionEndIrq()) {
            Serial.println("isVbusSessionEndIrq");
        }
        if (power.isLowVoltageLevel2Irq()) {
            Serial.println("isLowVoltageLevel2Irq");
        }
        if (power.isWdtExpireIrq()) {
            Serial.println("isWdtExpire");

            printPMU();
            // Clear the timer state and continue to the next timer
            power.clearTimerFlag();
        }
        if (power.isGpio2EdgeTriggerIrq()) {
            Serial.println("isGpio2EdgeTriggerIrq");
        }
        if (power.isGpio1EdgeTriggerIrq()) {
            Serial.println("isGpio1EdgeTriggerIrq");
        }
        if (power.isGpio0EdgeTriggerIrq()) {
            Serial.println("isGpio0EdgeTriggerIrq");
        }
        // Clear PMU Interrupt Status Register
        power.clearIrqStatus();

    }
    delay(10);
}

