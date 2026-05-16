/**
 * @file      PowerDeliveryHUSB238.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-07-24
 *
 */
#include <XPowersLib.h>

PowerDeliveryHUSB238 pd;


#ifndef CONFIG_PMU_SDA
#define CONFIG_PMU_SDA 0
#endif

#ifndef CONFIG_PMU_SCL
#define CONFIG_PMU_SCL 1
#endif

const uint8_t i2c_sda = CONFIG_PMU_SDA;
const uint8_t i2c_scl = CONFIG_PMU_SCL;


void setup()
{
    Serial.begin(115200);
    while (!Serial);
    bool result =  pd.init(Wire, i2c_sda, i2c_scl, HUSB238_SLAVE_ADDRESS);
    if (result == false) {
        while (1) {
            Serial.println("USB Power Delivery controller not online...");
            delay(1000);
        }
    }
}


void loop()
{
    Serial.print("PD Voltage:"); Serial.print(pd.getPdVoltage()); Serial.print(" V");
    Serial.print(" Current: "); Serial.print(pd.getPdCurrent()); Serial.println(" A");

    PowerDeliveryHUSB238::PD_Status status =  pd.status();
    Serial.print("USB Power Delivery Status : ");
    switch (status) {
    case PowerDeliveryHUSB238::NO_RESPONSE:
        Serial.println("no response");
        break;
    case PowerDeliveryHUSB238::SUCCESS:
        Serial.println("success");
        break;
    case PowerDeliveryHUSB238::INVALID_CMD:
        Serial.println("invalid command");
        break;
    case PowerDeliveryHUSB238::NOT_SUPPORT:
        Serial.println("not support");
        break;
    case PowerDeliveryHUSB238::TRANSACTION_FAIL:
        Serial.println("transaction failed");
        break;
    default:
        break;
    }
    delay(1000);
}





