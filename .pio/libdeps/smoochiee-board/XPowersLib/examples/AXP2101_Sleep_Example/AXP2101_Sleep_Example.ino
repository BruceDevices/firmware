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



//! @note  In the experiment on T-SIM7080-ESP32, the power consumption is about 900uA after setting the PMU to sleep,
//!  and 1mA if it is not set to sleep. After setting the PMU to sleep, it can save about 100uA consumption current

#ifndef XPOWERS_NO_ERROR
#error "Running this example is known to not damage the device! Please go and uncomment this!"
#endif
// Defined using AXP2102
#define XPOWERS_CHIP_AXP2101

#include <Wire.h>
#include <Arduino.h>
#include "XPowersLib.h"

#ifndef CONFIG_PMU_SDA
#define CONFIG_PMU_SDA 15
#endif

#ifndef CONFIG_PMU_SCL
#define CONFIG_PMU_SCL 7
#endif

#ifndef CONFIG_PMU_IRQ
#define CONFIG_PMU_IRQ 6
#endif

XPowersPMU power;

const uint8_t i2c_sda = CONFIG_PMU_SDA;
const uint8_t i2c_scl = CONFIG_PMU_SCL;
const uint8_t pmu_irq_pin = CONFIG_PMU_IRQ;
bool  pmu_flag = false;
bool  adc_switch = false;

void setFlag(void)
{
    pmu_flag = true;
}


void setup()
{
    Serial.begin(115200);

    bool result = power.begin(Wire, AXP2101_SLAVE_ADDRESS, i2c_sda, i2c_scl);

    if (result == false) {
        Serial.println("PMU is not online..."); while (1)delay(50);
    }

    // Force add pull-up
    pinMode(pmu_irq_pin, INPUT_PULLUP);
    attachInterrupt(pmu_irq_pin, setFlag, FALLING);

    // Close other IRQs
    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    // Clear all interrupt flags
    power.clearIrqStatus();
    // Enable the required interrupt function
    power.enableIRQ(
        XPOWERS_AXP2101_PKEY_SHORT_IRQ    //POWER KEY
    );

    // Turn on the charging indicator light as a power-on indicator
    power.setChargingLedMode(XPOWERS_CHG_LED_ON);

    // Enable chip temperature detection
    power.enableTemperatureMeasure();
}



void loop()
{
    if (pmu_flag) {
        pmu_flag = false;
        // Get PMU Interrupt Status Register
        power.getIrqStatus();
        if (power.isPekeyShortPressIrq()) {

            // Turn off the charging indicator to save power
            power.setChargingLedMode(XPOWERS_CHG_LED_OFF);

            // Turn off ADC data monitoring to save power
            power.disableTemperatureMeasure();
            // Enable internal ADC detection
            power.disableBattDetection();
            power.disableVbusVoltageMeasure();
            power.disableBattVoltageMeasure();
            power.disableSystemVoltageMeasure();


            // Enable PMU sleep
            power.enableSleep();

            // Reserve the MCU chip power supply, LilyGo AXP2101 usually uses DC as ESP power supply
            // power.enableDC1();

            // Turn off the power output of other channels
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

            // Clear PMU Interrupt Status Register
            power.clearIrqStatus();

            // Send IRQ wakeup command
            power.enableWakeup();

#if !CONFIG_IDF_TARGET_ESP32S3
            Serial.println("Please implement the MCU sleep method");
#else
            // Set ESP32 to wake up externally
            esp_sleep_enable_ext0_wakeup((gpio_num_t )pmu_irq_pin, LOW);

            // Enable ESP32 sleep
            esp_deep_sleep_start();
#endif

            Serial.println("Here never prints . ");
        }
    }

    // When not sleeping, print PMU temperature
    Serial.print("power Temperature:"); Serial.print(power.getTemperature()); Serial.println("*C");
    delay(1000);
}

