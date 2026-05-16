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

bool  pmu_flag = 0;
XPowersPMU power;

const uint8_t i2c_sda = CONFIG_PMU_SDA;
const uint8_t i2c_scl = CONFIG_PMU_SCL;
const uint8_t pmu_irq_pin = CONFIG_PMU_IRQ;
static uint32_t interval = 0;

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

    // Set VSYS off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
    power.setSysPowerDownVoltage(2600);

    vol = power.getSysPowerDownVoltage();
    Serial.printf("->  getSysPowerDownVoltage:%u\n", vol);


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



    // Enable internal ADC detection
    power.enableBattDetection();
    power.enableVbusVoltageMeasure();
    power.enableBattVoltageMeasure();
    power.enableSystemVoltageMeasure();
    power.enableTemperatureMeasure();


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
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ    |   //CHARGE
        XPOWERS_AXP2101_WARNING_LEVEL1_IRQ | XPOWERS_AXP2101_WARNING_LEVEL2_IRQ  // LOW BATTER 
    );


    // Get the default low pressure warning percentage setting
    uint8_t low_warn_per = power.getLowBatWarnThreshold();
    Serial.printf("Default low battery warning threshold is %d percentage\n", low_warn_per);

    //
    // setLowBatWarnThreshold Range:  5% ~ 20%
    // The following data is obtained from actual testing , Please see the description below for the test method.
    // 20% ~= 3.7v
    // 15% ~= 3.6v
    // 10% ~= 3.55V
    // 5%  ~= 3.5V
    // 1%  ~= 3.4V
    power.setLowBatWarnThreshold(5); // Set to trigger interrupt when reaching 5%

    // Get the low voltage warning percentage setting
    low_warn_per = power.getLowBatWarnThreshold();
    Serial.printf("Set low battery warning threshold is %d percentage\n", low_warn_per);

    // Get the default low voltage shutdown percentage setting
    uint8_t low_shutdown_per = power.getLowBatShutdownThreshold();
    Serial.printf("Default low battery shutdown threshold is %d percentage\n", low_shutdown_per);

    // setLowBatShutdownThreshold Range:  0% ~ 15%
    // The following data is obtained from actual testing , Please see the description below for the test method.
    // 15% ~= 3.6v
    // 10% ~= 3.55V
    // 5%  ~= 3.5V
    // 1%  ~= 3.4V
    power.setLowBatShutdownThreshold(1);  // Set to trigger interrupt when reaching 1%

    // Get the low voltage shutdown percentage setting
    low_shutdown_per = power.getLowBatShutdownThreshold();
    Serial.printf("Set low battery shutdown threshold is %d percentage\n", low_shutdown_per);



    /*
    * 
    *    Measurement methods:
    *    1. Connect the battery terminal to a voltage stabilizing source
    *    2. Set voltage test voltage
    *    3. Press PWR to boot
    *    4. Read the serial output voltage percentage
    * 
    *   If a voltage regulator is connected during testing and the voltage is slowly reduced, 
    *   the voltage percentage will not change immediately. It will take a while to slowly decrease. 
    *   In actual production, it needs to be adjusted according to the actual situation.
    * * * */
}


void loop()
{

    if (millis() > interval) {

        interval = millis() + 3000;

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

    if (pmu_flag) {

        pmu_flag = false;

        // Get PMU Interrupt Status Register
        uint32_t status = power.getIrqStatus();
        Serial.print("STATUS => HEX:");
        Serial.print(status, HEX);
        Serial.print(" BIN:");
        Serial.println(status, BIN);

        // When the set low-voltage battery percentage warning threshold is reached,
        // set the threshold through getLowBatWarnThreshold( 5% ~ 20% )
        if (power.isDropWarningLevel2Irq()) {
            Serial.println("The voltage percentage has reached the low voltage warning threshold!!!");
        }

        // When the set low-voltage battery percentage shutdown threshold is reached
        // set the threshold through setLowBatShutdownThreshold()
        if (power.isDropWarningLevel1Irq()) {
            int i = 4;
            while (i--) {
                Serial.printf("The voltage percentage has reached the low voltage shutdown threshold and will shut down in %d seconds.\n", i);
            }
            // Turn off all power supplies, leaving only the RTC power supply. The RTC power supply cannot be turned off.
            power.shutdown();
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
        // Clear PMU Interrupt Status Register
        power.clearIrqStatus();

    }
    delay(10);
}

