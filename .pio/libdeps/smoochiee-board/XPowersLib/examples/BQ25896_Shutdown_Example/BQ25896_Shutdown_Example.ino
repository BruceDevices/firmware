/**
 * @file      BQ25896_Shutdown_Example.ino
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
uint32_t countdown = 10;


void setup()
{
    Serial.begin(115200);
    while (!Serial);

    bool result =  PPM.init(Wire, i2c_sda, i2c_scl, BQ25896_SLAVE_ADDRESS);
    if (result == false) {
        while (1) {
            Serial.println("PPM is not online...");
            delay(1000);
        }
    }
}


void loop()
{
    if (millis() > cycleInterval) {
        Serial.printf("%d\n", countdown);
        if (!(countdown--)) {
            Serial.println("Shutdown .....");
            // The shutdown function can only be used when the battery is connected alone,
            // and cannot be shut down when connected to USB.
            // It can only be powered on in the following two ways:
            // 1. Press the PPM/QON button
            // 2. Connect to USB
            PPM.shutdown();
            countdown = 10000;
        }
        cycleInterval = millis() + 1000;
    }
}





