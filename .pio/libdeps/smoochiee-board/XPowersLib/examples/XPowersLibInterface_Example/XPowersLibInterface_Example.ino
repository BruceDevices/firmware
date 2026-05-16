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


#include <Arduino.h>
#include <Wire.h>
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

// Use the XPowersLibInterface standard to use the xpowers API
XPowersLibInterface *power = NULL;

bool  pmu_flag = 0;

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


    if (!power) {
        power = new XPowersAXP2101(Wire, i2c_sda, i2c_scl);
        if (!power->init()) {
            Serial.printf("Warning: Failed to find AXP2101 power management\n");
            delete power;
            power = NULL;
        } else {
            Serial.printf("AXP2101 PMU init succeeded, using AXP2101 PMU\n");
        }
    }

    if (!power) {
        power = new XPowersAXP192(Wire, i2c_sda, i2c_scl);
        if (!power->init()) {
            Serial.printf("Warning: Failed to find AXP192 power management\n");
            delete power;
            power = NULL;
        } else {
            Serial.printf("AXP192 PMU init succeeded, using AXP192 PMU\n");
        }
    }

    if (!power) {
        power = new XPowersAXP202(Wire, i2c_sda, i2c_scl);
        if (!power->init()) {
            Serial.printf("Warning: Failed to find AXP202 power management\n");
            delete power;
            power = NULL;
        } else {
            Serial.printf("AXP202 PMU init succeeded, using AXP202 PMU\n");
        }
    }


    if (!power) {
        Serial.println("PMU not detected, please check.."); while (1)delay(50);
    }



    //The following AXP192 power supply setting voltage is based on esp32 T-beam
    if (power->getChipModel() == XPOWERS_AXP192) {

        // lora radio power channel
        power->setPowerChannelVoltage(XPOWERS_LDO2, 3300);
        power->enablePowerOutput(XPOWERS_LDO2);


        // oled module power channel,
        // disable it will cause abnormal communication between boot and AXP power supply,
        // do not turn it off
        power->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
        // enable oled power
        power->enablePowerOutput(XPOWERS_DCDC1);

        // gnss module power channel
        power->setPowerChannelVoltage(XPOWERS_LDO3, 3300);
        // power->enablePowerOutput(XPOWERS_LDO3);


        //protected oled power source
        power->setProtectedChannel(XPOWERS_DCDC1);
        //protected esp32 power source
        power->setProtectedChannel(XPOWERS_DCDC3);

        //disable not use channel
        power->disablePowerOutput(XPOWERS_DCDC2);

        //disable all axp chip interrupt
        power->disableIRQ(XPOWERS_AXP192_ALL_IRQ);


        //
        /*  Set the constant current charging current of AXP192
            opt:
            XPOWERS_AXP192_CHG_CUR_100MA,
            XPOWERS_AXP192_CHG_CUR_190MA,
            XPOWERS_AXP192_CHG_CUR_280MA,
            XPOWERS_AXP192_CHG_CUR_360MA,
            XPOWERS_AXP192_CHG_CUR_450MA,
            XPOWERS_AXP192_CHG_CUR_550MA,
            XPOWERS_AXP192_CHG_CUR_630MA,
            XPOWERS_AXP192_CHG_CUR_700MA,
            XPOWERS_AXP192_CHG_CUR_780MA,
            XPOWERS_AXP192_CHG_CUR_880MA,
            XPOWERS_AXP192_CHG_CUR_960MA,
            XPOWERS_AXP192_CHG_CUR_1000MA,
            XPOWERS_AXP192_CHG_CUR_1080MA,
            XPOWERS_AXP192_CHG_CUR_1160MA,
            XPOWERS_AXP192_CHG_CUR_1240MA,
            XPOWERS_AXP192_CHG_CUR_1320MA,
        */
        power->setChargerConstantCurr(XPOWERS_AXP192_CHG_CUR_550MA);


    }
    // The following AXP202 power supply voltage setting is based on esp32 T-Watch
    else if (power->getChipModel() == XPOWERS_AXP202) {

        power->disablePowerOutput(XPOWERS_DCDC2); //not elicited

        //Display backlight
        power->setPowerChannelVoltage(XPOWERS_LDO2, 3300);
        power->enablePowerOutput(XPOWERS_LDO2);

        // Shiled Vdd
        power->setPowerChannelVoltage(XPOWERS_LDO3, 3300);
        power->enablePowerOutput(XPOWERS_LDO3);

        // S7xG GNSS Vdd
        power->setPowerChannelVoltage(XPOWERS_LDO4, 1800);
        power->enablePowerOutput(XPOWERS_LDO4);


        //
        /*  Set the constant current charging current of AXP202
            opt:
            XPOWERS_AXP202_CHG_CUR_100MA,
            XPOWERS_AXP202_CHG_CUR_190MA,
            XPOWERS_AXP202_CHG_CUR_280MA,
            XPOWERS_AXP202_CHG_CUR_360MA,
            XPOWERS_AXP202_CHG_CUR_450MA,
            XPOWERS_AXP202_CHG_CUR_550MA,
            XPOWERS_AXP202_CHG_CUR_630MA,
            XPOWERS_AXP202_CHG_CUR_700MA,
            XPOWERS_AXP202_CHG_CUR_780MA,
            XPOWERS_AXP202_CHG_CUR_880MA,
            XPOWERS_AXP202_CHG_CUR_960MA,
            XPOWERS_AXP202_CHG_CUR_1000MA,
            XPOWERS_AXP202_CHG_CUR_1080MA,
            XPOWERS_AXP202_CHG_CUR_1160MA,
            XPOWERS_AXP202_CHG_CUR_1240MA,
            XPOWERS_AXP202_CHG_CUR_1320MA,
        */
        power->setChargerConstantCurr(XPOWERS_AXP202_CHG_CUR_550MA);

    }
    // The following AXP192 power supply voltage setting is based on esp32s3 T-beam
    else if (power->getChipModel() == XPOWERS_AXP2101) {

        // gnss module power channel
        power->setPowerChannelVoltage(XPOWERS_ALDO4, 3300);
        power->enablePowerOutput(XPOWERS_ALDO4);

        // lora radio power channel
        power->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
        power->enablePowerOutput(XPOWERS_ALDO3);

        // m.2 interface
        power->setPowerChannelVoltage(XPOWERS_DCDC3, 3300);
        power->enablePowerOutput(XPOWERS_DCDC3);

        // power->setPowerChannelVoltage(XPOWERS_DCDC4, 3300);
        // power->enablePowerOutput(XPOWERS_DCDC4);

        //not use channel
        power->disablePowerOutput(XPOWERS_DCDC2); //not elicited
        power->disablePowerOutput(XPOWERS_DCDC5); //not elicited
        power->disablePowerOutput(XPOWERS_DLDO1); //Invalid power channel, it does not exist
        power->disablePowerOutput(XPOWERS_DLDO2); //Invalid power channel, it does not exist
        power->disablePowerOutput(XPOWERS_VBACKUP);

        //disable all axp chip interrupt
        power->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

        /*  Set the constant current charging current of AXP2101
            opt:
            XPOWERS_AXP2101_CHG_CUR_100MA,
            XPOWERS_AXP2101_CHG_CUR_125MA,
            XPOWERS_AXP2101_CHG_CUR_150MA,
            XPOWERS_AXP2101_CHG_CUR_175MA,
            XPOWERS_AXP2101_CHG_CUR_200MA,
            XPOWERS_AXP2101_CHG_CUR_300MA,
            XPOWERS_AXP2101_CHG_CUR_400MA,
            XPOWERS_AXP2101_CHG_CUR_500MA,
            XPOWERS_AXP2101_CHG_CUR_600MA,
            XPOWERS_AXP2101_CHG_CUR_700MA,
            XPOWERS_AXP2101_CHG_CUR_800MA,
            XPOWERS_AXP2101_CHG_CUR_900MA,
            XPOWERS_AXP2101_CHG_CUR_1000MA,
        */
        power->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);

    }

    Serial.println("=======================================================================\n");
    if (power->isChannelAvailable(XPOWERS_DCDC1)) {
        Serial.printf("DC1  : %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_DCDC1)  ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_DCDC1));
    }
    if (power->isChannelAvailable(XPOWERS_DCDC2)) {
        Serial.printf("DC2  : %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_DCDC2)  ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_DCDC2));
    }
    if (power->isChannelAvailable(XPOWERS_DCDC3)) {
        Serial.printf("DC3  : %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_DCDC3)  ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_DCDC3));
    }
    if (power->isChannelAvailable(XPOWERS_DCDC4)) {
        Serial.printf("DC4  : %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_DCDC4)  ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_DCDC4));
    }
    if (power->isChannelAvailable(XPOWERS_LDO2)) {
        Serial.printf("LDO2 : %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_LDO2)   ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_LDO2));
    }
    if (power->isChannelAvailable(XPOWERS_LDO3)) {
        Serial.printf("LDO3 : %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_LDO3)   ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_LDO3));
    }
    if (power->isChannelAvailable(XPOWERS_LDO4)) {
        Serial.printf("LDO4 : %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_LDO4)   ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_LDO4));
    }
    if (power->isChannelAvailable(XPOWERS_LDO5)) {
        Serial.printf("LDO5 : %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_LDO5)   ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_LDO5));
    }
    if (power->isChannelAvailable(XPOWERS_ALDO1)) {
        Serial.printf("ALDO1: %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_ALDO1)  ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_ALDO1));
    }
    if (power->isChannelAvailable(XPOWERS_ALDO2)) {
        Serial.printf("ALDO2: %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_ALDO2)  ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_ALDO2));
    }
    if (power->isChannelAvailable(XPOWERS_ALDO3)) {
        Serial.printf("ALDO3: %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_ALDO3)  ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_ALDO3));
    }
    if (power->isChannelAvailable(XPOWERS_ALDO4)) {
        Serial.printf("ALDO4: %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_ALDO4)  ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_ALDO4));
    }
    if (power->isChannelAvailable(XPOWERS_BLDO1)) {
        Serial.printf("BLDO1: %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_BLDO1)  ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_BLDO1));
    }
    if (power->isChannelAvailable(XPOWERS_BLDO2)) {
        Serial.printf("BLDO2: %s   Voltage:%u mV \n",  power->isPowerChannelEnable(XPOWERS_BLDO2)  ? "+" : "-",  power->getPowerChannelVoltage(XPOWERS_BLDO2));
    }
    Serial.println("=======================================================================\n");


    //Set up the charging voltage, AXP2101/AXP192 4.2V gear is the same
    // XPOWERS_AXP192_CHG_VOL_4V2 = XPOWERS_AXP2101_CHG_VOL_4V2
    power->setChargeTargetVoltage(XPOWERS_AXP192_CHG_VOL_4V2);

    // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
    power->setSysPowerDownVoltage(2600);

    // Get the VSYS shutdown voltage
    uint16_t vol = power->getSysPowerDownVoltage();
    Serial.printf("->  getSysPowerDownVoltage:%u\n", vol);



    // Set the time of pressing the button to turn off
    power->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    uint8_t opt = power->getPowerKeyPressOffTime();
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
    power->setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
    opt = power->getPowerKeyPressOnTime();
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
    power->disableTSPinMeasure();

    // Enable internal ADC detection
    power->enableBattDetection();
    power->enableVbusVoltageMeasure();
    power->enableBattVoltageMeasure();
    power->enableSystemVoltageMeasure();


    /*
      The default setting is CHGLED is automatically controlled by the PMU.
    - XPOWERS_CHG_LED_OFF,
    - XPOWERS_CHG_LED_BLINK_1HZ,
    - XPOWERS_CHG_LED_BLINK_4HZ,
    - XPOWERS_CHG_LED_ON,
    - XPOWERS_CHG_LED_CTRL_CHG,
    * */
    power->setChargingLedMode(XPOWERS_CHG_LED_OFF);


    pinMode(pmu_irq_pin, INPUT);
    attachInterrupt(pmu_irq_pin, setFlag, FALLING);

    // Clear all interrupt flags
    power->clearIrqStatus();


    /*
    // call specific interrupt request

    uint64_t pmuIrqMask = 0;

    if (power->getChipModel() == XPOWERS_AXP192) {

        pmuIrqMask = XPOWERS_AXP192_VBUS_INSERT_IRQ     | XPOWERS_AXP192_VBUS_REMOVE_IRQ |      //BATTERY
                     XPOWERS_AXP192_BAT_INSERT_IRQ      | XPOWERS_AXP192_BAT_REMOVE_IRQ  |      //VBUS
                     XPOWERS_AXP192_PKEY_SHORT_IRQ      | XPOWERS_AXP192_PKEY_LONG_IRQ   |      //POWER KEY
                     XPOWERS_AXP192_BAT_CHG_START_IRQ   | XPOWERS_AXP192_BAT_CHG_DONE_IRQ ;     //CHARGE
    } else if (power->getChipModel() == XPOWERS_AXP2101) {

        pmuIrqMask = XPOWERS_AXP2101_BAT_INSERT_IRQ     | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
                     XPOWERS_AXP2101_VBUS_INSERT_IRQ    | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS
                     XPOWERS_AXP2101_PKEY_SHORT_IRQ     | XPOWERS_AXP2101_PKEY_LONG_IRQ       |   //POWER KEY
                     XPOWERS_AXP2101_BAT_CHG_DONE_IRQ   | XPOWERS_AXP2101_BAT_CHG_START_IRQ;      //CHARGE
    }
    // Enable the required interrupt function
    power->enableIRQ(pmuIrqMask);

    */

    // Call the interrupt request through the interface class
    power->disableInterrupt(XPOWERS_ALL_INT);

    power->enableInterrupt(XPOWERS_USB_INSERT_INT |
                           XPOWERS_USB_REMOVE_INT |
                           XPOWERS_BATTERY_INSERT_INT |
                           XPOWERS_BATTERY_REMOVE_INT |
                           XPOWERS_PWR_BTN_CLICK_INT |
                           XPOWERS_CHARGE_START_INT |
                           XPOWERS_CHARGE_DONE_INT);


    // Access the subclass method by getting the model
    uint8_t chipType = power->getChipModel();
    switch (chipType) {
    case XPOWERS_AXP192: {
        XPowersAXP192 *axp192 = static_cast<XPowersAXP192 *>(power);
        axp192->enableCoulomb();
        uint32_t data = axp192->getBattChargeCoulomb();
        float chargeCurrent =  axp192->getBatteryChargeCurrent();
        float dischargeCurrent = axp192->getBattDischargeCurrent();
        Serial.printf("AXP192 GetBattChargeCoulomb : %X\n", data);
        Serial.printf("AXP192 GetBatteryChargeCurrent:%.2f mA\n", chargeCurrent);
        Serial.printf("AXP192 GetBattDischargeCurrent:%.2f mA\n", dischargeCurrent);
    }
    break;
    case XPOWERS_AXP202: {
        XPowersAXP202 *axp202 = static_cast<XPowersAXP202 *>(power);
        axp202->enableCoulomb();
        uint32_t data = axp202->getBattChargeCoulomb();
        Serial.printf("AX202 GetBattChargeCoulomb : %X\n", data);
    }
    break;
    case XPOWERS_AXP2101: {
        XPowersAXP2101 *axp2101 = static_cast<XPowersAXP2101 *>(power);
        axp2101->fuelGaugeControl(true, true);
        axp2101->setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
        axp2101->setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
    }
    break;
    default:
        break;
    }

}

void printPMU()
{
    Serial.print("isCharging:"); Serial.println(power->isCharging() ? "YES" : "NO");
    Serial.print("isDischarge:"); Serial.println(power->isDischarge() ? "YES" : "NO");
    Serial.print("isVbusIn:"); Serial.println(power->isVbusIn() ? "YES" : "NO");
    Serial.print("getBattVoltage:"); Serial.print(power->getBattVoltage()); Serial.println("mV");
    Serial.print("getVbusVoltage:"); Serial.print(power->getVbusVoltage()); Serial.println("mV");
    Serial.print("getSystemVoltage:"); Serial.print(power->getSystemVoltage()); Serial.println("mV");

    // The battery percentage may be inaccurate at first use, the PMU will automatically
    // learn the battery curve and will automatically calibrate the battery percentage
    // after a charge and discharge cycle
    if (power->isBatteryConnect()) {
        Serial.print("getBatteryPercent:"); Serial.print(power->getBatteryPercent()); Serial.println("%");
    }

    Serial.println();
}



void loop()
{

    if (pmu_flag) {

        pmu_flag = false;

        // Get PMU Interrupt Status Register
        uint32_t status = power->getIrqStatus();
        Serial.print("STATUS => HEX:");
        Serial.print(status, HEX);
        Serial.print(" BIN:");
        Serial.println(status, BIN);

        if (power->isVbusInsertIrq()) {
            Serial.println("isVbusInsert");
        }
        if (power->isVbusRemoveIrq()) {
            Serial.println("isVbusRemove");
        }
        if (power->isBatInsertIrq()) {
            Serial.println("isBatInsert");
        }
        if (power->isBatRemoveIrq()) {
            Serial.println("isBatRemove");
        }
        if (power->isPekeyShortPressIrq()) {
            Serial.println("isPekeyShortPress");
        }
        if (power->isPekeyLongPressIrq()) {
            Serial.println("isPekeyLongPress");
        }
        if (power->isBatChargeDoneIrq()) {
            Serial.println("isBatChargeDone");
        }
        if (power->isBatChargeStartIrq()) {
            Serial.println("isBatChargeStart");
        }
        // Clear PMU Interrupt Status Register
        power->clearIrqStatus();

    }
    delay(10);
}

