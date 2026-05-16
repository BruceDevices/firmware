/**
 * @file      BQ25896_Example.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-06-04
 *
 */

#define XPOWERS_CHIP_BQ25896

#include <XPowersLib.h>

XPowersPPM PPM;


#ifndef CONFIG_PMU_SDA
#define CONFIG_PMU_SDA 0
#endif

#ifndef CONFIG_PMU_SCL
#define CONFIG_PMU_SCL 1
#endif

#ifndef CONFIG_PMU_IRQ
#define CONFIG_PMU_IRQ 28
#endif

const uint8_t i2c_sda = CONFIG_PMU_SDA;
const uint8_t i2c_scl = CONFIG_PMU_SCL;
const uint8_t pmu_irq_pin = CONFIG_PMU_IRQ;
uint32_t cycleInterval;
bool pmu_irq = false;

void setup()
{
    Serial.begin(115200);
    while (!Serial);


    bool result =  PPM.init(Wire, i2c_sda, i2c_scl, BQ25896_SLAVE_ADDRESS);

    if (result == false) {
        while (1) {
            Serial.println("PPM is not online...");
            delay(50);
        }
    }

    // Set the minimum operating voltage. Below this voltage, the PPM will protect
    PPM.setSysPowerDownVoltage(3300);

    // Set input current limit, default is 500mA
    PPM.setInputCurrentLimit(3250);

    Serial.printf("getInputCurrentLimit: %d mA\n", PPM.getInputCurrentLimit());

    // Disable current limit pin
    PPM.disableCurrentLimitPin();

    // Set the charging target voltage, Range:3840 ~ 4608mV ,step:16 mV
    PPM.setChargeTargetVoltage(4208);

    // Set the precharge current , Range: 64mA ~ 1024mA ,step:64mA
    PPM.setPrechargeCurr(64);

    // The premise is that Limit Pin is disabled, or it will only follow the maximum charging current set by Limi tPin.
    // Set the charging current , Range:0~5056mA ,step:64mA
    PPM.setChargerConstantCurr(1024);

    // Get the set charging current
    PPM.getChargerConstantCurr();
    Serial.printf("getChargerConstantCurr: %d mA\n", PPM.getChargerConstantCurr());


    // To obtain voltage data, the ADC must be enabled first
    PPM.enableMeasure();

    // Turn on charging function
    // If there is no battery connected, do not turn on the charging function
    PPM.enableCharge();

    // Turn off charging function
    // If USB is used as the only power input, it is best to turn off the charging function,
    // otherwise the VSYS power supply will have a sawtooth wave, affecting the discharge output capability.
    // PPM.disableCharge();


    // The OTG function needs to enable OTG, and set the OTG control pin to HIGH
    // After OTG is enabled, if an external power supply is plugged in, OTG will be turned off

    // PPM.enableOTG();
    // PPM.disableOTG();
    // pinMode(OTG_ENABLE_PIN, OUTPUT);
    // digitalWrite(OTG_ENABLE_PIN, HIGH);

    pinMode(pmu_irq_pin, INPUT_PULLUP);
    attachInterrupt(pmu_irq_pin, []() {
        pmu_irq = true;
    }, FALLING);

}


void loop()
{
    if (pmu_irq) {
        pmu_irq = false;

        // Get PPM interrupt status
        PPM.getFaultStatus();

        Serial.print("-> [");
        Serial.print(millis() / 1000);
        Serial.print("] ");

        if (PPM.isWatchdogFault()) {

            Serial.println("Watchdog Fault");

        } else if (PPM.isBoostFault()) {

            Serial.println("Boost Fault");

        } else if (PPM.isChargeFault()) {

            Serial.println("Charge Fault");

        } else if (PPM.isBatteryFault()) {

            Serial.println("Batter Fault");

        } else if (PPM.isNTCFault()) {

            Serial.print("NTC Fault:");
            Serial.print(PPM.getNTCStatusString());
            Serial.print(" Percentage:");
            Serial.print(PPM.getNTCPercentage()); Serial.println("%");
        } else {
            /*
            * When the battery is removed, INT will send an interrupt every 100ms. If the battery is not connected,
            * you can use PPM.disableCharge() to turn off the charging function.
            * */
            // PPM.disableCharge();

            Serial.println("Battery remove");
        }
    }

    /*
    * Obtaining the battery voltage and battery charging status does not directly read the register,
    * but determines whether the charging current register is normal.
    * If read directly, the reading will be inaccurate.
    * The premise for obtaining these two states is that the NTC temperature measurement circuit is normal.
    * If the NTC detection is abnormal, it will return 0
    * * */
    if (millis() > cycleInterval) {
        Serial.printf("CHG TARGET VOLTAGE :%04dmV CURRENT:%04dmA PER_CHARGE_CUR %04dmA\n",
                      PPM.getChargeTargetVoltage(), PPM.getChargerConstantCurr(), PPM.getPrechargeCurr());
        Serial.printf("VBUS:%s %04dmV VBAT:%04dmV VSYS:%04dmV\n", PPM.isVbusIn() ? "Connected" : "Disconnect",
                      PPM.getVbusVoltage(),
                      PPM.getBattVoltage(),
                      PPM.getSystemVoltage());
        Serial.printf("BUS STATE:%d STR:%s\n", PPM.getBusStatus(), PPM.getBusStatusString());
        Serial.printf("CHG STATE:%d STR:%s CURRENT:%04dmA\n", PPM.chargeStatus(), PPM.getChargeStatusString(), PPM.getChargeCurrent());
        Serial.printf("[%lu]", millis() / 1000);
        Serial.println("----------------------------------------------------------------------------------");
        cycleInterval = millis() + 1000;
    }

}





