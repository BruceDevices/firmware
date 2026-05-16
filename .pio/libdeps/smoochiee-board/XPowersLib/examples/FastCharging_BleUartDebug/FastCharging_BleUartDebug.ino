/**
 * @file      FastCharging_BleUartDebug.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-07-30
 *
 */

// Use SY6970
// #define XPOWERS_CHIP_SY6970

// Use BQ25896
#define XPOWERS_CHIP_BQ25896
 
#include <XPowersLib.h>

#ifdef ARDUINO_ARCH_NRF52

#include <bluefruit.h>
#include <Adafruit_LittleFS.h>

PowerDeliveryHUSB238 pd;
XPowersPPM ppm;

#ifndef CONFIG_PMU_SDA
#define CONFIG_PMU_SDA 0
#endif

#ifndef CONFIG_PMU_SCL
#define CONFIG_PMU_SCL 1
#endif

const uint8_t i2c_sda = CONFIG_PMU_SDA;
const uint8_t i2c_scl = CONFIG_PMU_SCL;


BLEUart bleuart; // uart over ble
bool find_ppm = false;
String buffer;


void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    Serial.println();
    Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}


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
    // Print out the current connection info
    Serial.printf("Connection Info: PHY = %d Mbps, Conn Interval = %.2f ms, Data Length = %d, MTU = %d\n",
                  conn->getPHY(), conn->getConnectionInterval() * 1.25f, conn->getDataLength(), conn->getMtu());

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

    find_ppm = ppm.init(Wire);
    if (!find_ppm) {
        Serial.println("Can't find XPowers PPM Chip !");
    } else {
        Serial.println("Find XPowers ppm Chip .Reset PPM default config");

        ppm.resetDefault();

        ppm.disableWatchdog();

        ppm.enableCharge();

        ppm.disableOTG();

        // SY6970 Range:0~5056 mA / step:64mA
        // BQ25896 Range:0~3008 mA / step:64mA
        ppm.setChargerConstantCurr(2048);
        // Disable current limit pin
        ppm.disableCurrentLimitPin();

        // SY6970 : Range : 100 mA ~ 3250 mA
        ppm.setInputCurrentLimit(3250);

        // ppm.disableInputCurrentLimit();

        // Set the charging target voltage, Range:3840 ~ 4608mV ,step:16 mV
        ppm.setChargeTargetVoltage(4208);

        ppm.enableMeasure();

        // ppm.setHighVoltageRequestedRange(RequestRange::REQUEST_9V);
        // ppm.setSysPowerDownVoltage(3500);
        // ppm.setVinDpmThreshold(POWERS_SY6970_VINDPM_VOL_MAX);

    }
    // Config the peripheral connection with maximum bandwidth
    // more SRAM required by SoftDevice
    // Note: All config***() function must be called before begin()
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

    Bluefruit.begin();
    Bluefruit.setName("PowerDelivery");
    Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
    Bluefruit.Periph.setConnInterval(6, 12); // 7.5 - 15 ms

    // Configure and Start BLE Uart Service
    bleuart.begin();

    startAdv();

    Serial.println("Please use Adafruit's Bluefruit LE app to connect in UART mode");
}



void loop()
{
    buffer = "PD Vol:";
    buffer.concat(pd.getPdVoltage());
    buffer.concat("V ");
    buffer.concat("Cur:");
    buffer.concat(pd.getPdCurrent());
    buffer.concat("A\n");

    buffer.concat("Status:");
    PowerDeliveryHUSB238::PD_Status status =  pd.status();
    switch (status) {
    case PowerDeliveryHUSB238::NO_RESPONSE:
        buffer.concat("no response");
        break;
    case PowerDeliveryHUSB238::SUCCESS:
        buffer.concat("success");
        break;
    case PowerDeliveryHUSB238::INVALID_CMD:
        buffer.concat("invalid command");
        break;
    case PowerDeliveryHUSB238::NOT_SUPPORT:
        buffer.concat("not support");
        break;
    case PowerDeliveryHUSB238::TRANSACTION_FAIL:
        buffer.concat("transaction failed");
        break;
    default:
        break;
    }

    if (Bluefruit.connected() && bleuart.notifyEnabled()) {

        buffer.concat("\n");

        if (find_ppm) {
            ppm.getFaultStatus();
            buffer.concat("NTC:"); buffer.concat(ppm.getNTCStatusString()); buffer.concat("\n");
            buffer.concat("VBUS:"); buffer.concat(ppm.getVbusVoltage()); buffer.concat("\n");
            buffer.concat("VBAT:"); buffer.concat(ppm.getBattVoltage()); buffer.concat("\n");
            buffer.concat("VSYS:"); buffer.concat(ppm.getSystemVoltage()); buffer.concat("\n");
            buffer.concat("BUS:"); buffer.concat(ppm.getBusStatusString()); buffer.concat("\n");
            buffer.concat("CHG:"); buffer.concat(ppm.getChargeStatusString()); buffer.concat("\n");
            buffer.concat("CUR:"); buffer.concat(ppm.getChargeCurrent()); buffer.concat("\n");
        }
        bleuart.write(buffer.c_str());

    } else {

        Serial.println(buffer);
        if (find_ppm) {

            ppm.getFaultStatus();
            Serial.print("NTC STR:"); Serial.println(ppm.getNTCStatusString());
            Serial.printf("CHG TARGET VOLTAGE :%04dmV CURRENT:%04dmA PER_CHARGE_CUR %04dmA\n",
                          ppm.getChargeTargetVoltage(), ppm.getChargerConstantCurr(), ppm.getPrechargeCurr());
            Serial.printf("VBUS:%s %04dmV VBAT:%04dmV VSYS:%04dmV\n", ppm.isVbusIn() ? "Connected" : "Disconnect",
                          ppm.getVbusVoltage(),
                          ppm.getBattVoltage(),
                          ppm.getSystemVoltage());
            Serial.printf("BUS STATE:%d  --  STR:%s\n", ppm.getBusStatus(), ppm.getBusStatusString());
            Serial.printf("CHG STATE:%d  --  STR:%s CURRENT:%04dmA\n", ppm.chargeStatus(), ppm.getChargeStatusString(), ppm.getChargeCurrent());
            Serial.printf("[%lu]", millis() / 1000);
            Serial.println("----------------------------------------------------------------------------------");
        }
    }

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



