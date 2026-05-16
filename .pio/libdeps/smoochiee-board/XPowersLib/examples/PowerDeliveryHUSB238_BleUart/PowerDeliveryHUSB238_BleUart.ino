/**
 * @file      PowerDeliveryHUSB238_BleUart.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-07-24
 *
 */

#include <XPowersLib.h>

#ifdef ARDUINO_ARCH_NRF52

#include <bluefruit.h>
#include <Adafruit_LittleFS.h>

PowerDeliveryHUSB238 pd;


#ifndef CONFIG_PMU_SDA
#define CONFIG_PMU_SDA 0
#endif

#ifndef CONFIG_PMU_SCL
#define CONFIG_PMU_SCL 1
#endif

const uint8_t i2c_sda = CONFIG_PMU_SDA;
const uint8_t i2c_scl = CONFIG_PMU_SCL;


BLEUart bleuart; // uart over ble


void connect_callback(uint16_t conn_handle)
{
    BLEConnection *conn = Bluefruit.Connection(conn_handle);
    Serial.println("Connected");

    // request PHY changed to 2MB
    Serial.println("Request to change PHY");
    conn->requestPHY();

    // request to update data length
    Serial.println("Request to change Data Length");
    conn->requestDataLengthUpdate();

    // request mtu exchange
    Serial.println("Request to change MTU");
    conn->requestMtuExchange(247);

    // request connection interval of 7.5 ms
    //conn->requestConnectionParameter(6); // in unit of 1.25

    // delay a bit for all the request to complete
    delay(1000);
}

void startAdv(void)
{
    // Advertising packet

    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();

    // Include bleuart 128-bit uuid
    Bluefruit.Advertising.addService(bleuart);

    // Secondary Scan Response packet (optional)
    // Since there is no room for 'Name' in Advertising packet
    Bluefruit.ScanResponse.addName();

    /* Start Advertising
     * - Enable auto advertising if disconnected
     * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     * - Timeout for fast mode is 30 seconds
     * - Start(timeout) with timeout = 0 will advertise forever (until connected)
     *
     * For recommended advertising interval
     * https://developer.apple.com/library/content/qa/qa1931/_index.html
     */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
    Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void setup()
{
    Serial.begin(115200);
    // while (!Serial);

    bool result =  pd.init(Wire, i2c_sda, i2c_scl, HUSB238_SLAVE_ADDRESS);
    if (result == false) {
        while (1) {
            Serial.println("USB Power Delivery controller not online...");
            delay(1000);
        }
    }

    // Config the peripheral connection with maximum bandwidth
    // more SRAM required by SoftDevice
    // Note: All config***() function must be called before begin()
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
    
    Bluefruit.begin();
    Bluefruit.setName("PowerDelivery");
    Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setConnInterval(6, 12); // 7.5 - 15 ms

    // Configure and Start BLE Uart Service
    bleuart.begin();

    startAdv();

    Serial.println("Please use Adafruit's Bluefruit LE app to connect in UART mode");
}


String buffer;

void loop()
{

    Serial.print("PD Voltage:"); Serial.print(pd.getPdVoltage()); Serial.print(" V");
    Serial.print(" Current: "); Serial.print(pd.getPdCurrent()); Serial.println(" A");

    buffer = "Vol:";
    buffer.concat(pd.getPdVoltage());
    buffer.concat("V ");
    buffer.concat("Cur:");
    buffer.concat(pd.getPdCurrent());
    buffer.concat("A");
    buffer.concat("Status:");


    PowerDeliveryHUSB238::PD_Status status =  pd.status();
    Serial.print("USB Power Delivery Status : ");

    switch (status) {
    case PowerDeliveryHUSB238::NO_RESPONSE:
        Serial.println("no response");
        buffer.concat("no response");
        break;
    case PowerDeliveryHUSB238::SUCCESS:
        Serial.println("success");
        buffer.concat("success");
        break;
    case PowerDeliveryHUSB238::INVALID_CMD:
        Serial.println("invalid command");
        buffer.concat("invalid command");
        break;
    case PowerDeliveryHUSB238::NOT_SUPPORT:
        Serial.println("not support");
        buffer.concat("not support");
        break;
    case PowerDeliveryHUSB238::TRANSACTION_FAIL:
        Serial.println("transaction failed");
        buffer.concat("transaction failed");
        break;
    default:
        break;
    }
    bleuart.write(buffer.c_str());

    delay(5000);
}

#else
void setup()
{
    Serial.begin(115200);
}
void loop()
{
    Serial.println("ble examples only support nrf platform"); delay(1000);
}
#endif



