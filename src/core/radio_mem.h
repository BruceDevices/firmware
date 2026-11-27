#ifndef __RADIO_MEM_H__
#define __RADIO_MEM_H__
#define RADIO_MEM_TWO_TIER 1
/*
    Internal-DRAM contiguous-block guard for radio bring-up on no-PSRAM boards.

    On boards without PSRAM (e.g. m5stack-cardputer) the Wi-Fi and BLE stacks —
    plus the SD/SPI DMA buffers — all compete for the SAME internal DRAM. What
    actually gates them is not total free heap but the LARGEST CONTIGUOUS
    DMA-capable block. When that block is exhausted, esp_wifi_init /
    esp_bt_controller_init fail deep inside the SDK and leave the driver in a
    half-initialized state, which then crashes (LoadProhibited / abort()) on the
    next operation.

    These helpers let callers refuse to start a radio *before* touching it when
    there isn't enough contiguous DMA memory, turning an unavoidable crash into a
    clean, user-visible error.
*/
#include <esp_heap_caps.h>
#include <stddef.h>
#include <sdkconfig.h>
#ifndef CONFIG_IDF_TARGET_ESP32P4
#include <esp_bt.h>
#include <esp_wifi.h>
#endif
#if __has_include(<NimBLEDevice.h>)
#include <NimBLEDevice.h>
#define RADIO_MEM_HAS_NIMBLE 1
#endif
#include "core/iq_record_gate.h"
#include "core/sd_functions.h"
#include "core/wifi/wifi_common.h"
#include <WiFi.h>
#include <globals.h>

// Forward declarations to avoid pulling heavy/unguarded headers into radio_mem.h
void stopBLEStack();
void cleanlyStopWebUiForWiFiFeature();

// Largest contiguous DMA-capable internal block and total free DMA bytes.
static inline size_t radioLargestDmaBlock() { return heap_caps_get_largest_free_block(MALLOC_CAP_DMA); }
static inline size_t radioFreeDmaBytes() { return heap_caps_get_free_size(MALLOC_CAP_DMA); }

static inline bool radioIsBleInitialized() {
#if defined(RADIO_MEM_HAS_NIMBLE)
    return NimBLEDevice::isInitialized();
#else
    return false;
#endif
}

// Minimum contiguous DMA block required BEFORE bringing Wi-Fi up (checked at the
// scan/menu entry, i.e. before esp_wifi is touched at all).
constexpr size_t RADIO_WIFI_MIN_DMA_BLOCK = 15 * 1024;

// BLE memory thresholds (two-tier):
// esp_bt_controller_init() + NimBLE on ESP32-S3 allocates ~28-40 KiB TOTAL across
// multiple smaller blocks (largest single allocation ~12-16 KiB), NOT one 48 KiB
// contiguous block. Requiring a single 48 KiB contiguous block caused false-positive
// "Low RAM: free WiFi/SD first" on ESP32-S3 PSRAM boards (like LilyGO T-Deck) whose
// ~119 KiB post-static DIRAM gets fragmented into 24-44 KiB blocks by small <=4 KiB
// allocations (CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=4096).
//
// Tier 1 (target): proactively release the SDR bank (if present) / WebUI / Wi-Fi
//                  when free DMA or largest block looks tight.
// Tier 2 (hard minimum): only refuse BLE bring-up if DMA is still below what the
//                        controller actually needs after reclamation.
constexpr size_t RADIO_BLE_TARGET_DMA_BLOCK = 36 * 1024;
constexpr size_t RADIO_BLE_TARGET_DMA_FREE = 48 * 1024;
constexpr size_t RADIO_BLE_MIN_DMA_BLOCK_PSRAM = 16 * 1024;
constexpr size_t RADIO_BLE_MIN_DMA_FREE_PSRAM = 28 * 1024;
constexpr size_t RADIO_BLE_MIN_DMA_BLOCK_NOPSRAM = 18 * 1024;
constexpr size_t RADIO_BLE_MIN_DMA_FREE_NOPSRAM = 32 * 1024;

// SDR bank hand-off (implemented in modules/wifi/sdr.cpp): the SDR engine's
// 64 KiB SRAM bank is kept out of the heap behind a fence allocation so the IQ
// record/jammer can always start; when a radio bring-up finds the heap too
// small, releasing the fence returns the bank to the heap. After that the SDR
// needs a reboot (it refuses cleanly instead of corrupting the heap).
#if !defined(BRUCE_IQ_RECORD_UNAVAILABLE)
void sdrBankReleaseForRadio();
void sdrBankHeapInit();
#else
static inline void sdrBankReleaseForRadio() {}
static inline void sdrBankHeapInit() {}
#endif

static inline void radioBleMinThresholds(size_t &minBlock, size_t &minFree) {
    if (psramFound()) {
        minBlock = RADIO_BLE_MIN_DMA_BLOCK_PSRAM;
        minFree = RADIO_BLE_MIN_DMA_FREE_PSRAM;
    } else {
        minBlock = RADIO_BLE_MIN_DMA_BLOCK_NOPSRAM;
        minFree = RADIO_BLE_MIN_DMA_FREE_NOPSRAM;
    }
}

static inline bool radioDmaAtLeast(size_t minBlock, size_t minFree) {
    return radioLargestDmaBlock() >= minBlock && radioFreeDmaBytes() >= minFree;
}

static inline String radioLowRamError(const char *prefix) {
    char buf[64];
    snprintf(
        buf,
        sizeof(buf),
        "%s (%uK/%uK)",
        prefix,
        (unsigned)(radioLargestDmaBlock() / 1024),
        (unsigned)(radioFreeDmaBytes() / 1024)
    );
    return String(buf);
}

static inline void radioReclaimWifiMemory(size_t targetBlock, size_t targetFree = 0) {
    // Stop WebUI (AsyncWebServer + TFT screen logger buffers) if running
    if (isWebUIActive) {
        cleanlyStopWebUiForWiFiFeature();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Clear any cached Wi-Fi scan records
    WiFi.scanDelete();

    // Disconnect Arduino WiFi stack if active
    if (WiFi.getMode() != WIFI_MODE_NULL || wifiConnected) {
        wifiDisconnect();
        vTaskDelay(pdMS_TO_TICKS(150));
    }

#ifndef CONFIG_IDF_TARGET_ESP32P4
    // Force full ESP-IDF Wi-Fi driver teardown if a raw esp_wifi_init() caller
    // (sniffer/deauth/channel analyzer/pwngrid) left DMA buffers allocated while
    // Arduino WiFi.getMode() reported WIFI_MODE_NULL.
    if (!radioDmaAtLeast(targetBlock, targetFree)) {
        WiFi.mode(WIFI_OFF);
        esp_wifi_stop();
        esp_wifi_deinit();
        vTaskDelay(pdMS_TO_TICKS(150));
    }
#endif
}

static inline void radioCleanupFailedBleController() {
#if !defined(CONFIG_IDF_TARGET_ESP32P4)
    esp_bt_controller_status_t st = esp_bt_controller_get_status();
    if (st == ESP_BT_CONTROLLER_STATUS_ENABLED) { esp_bt_controller_disable(); }
    if (st != ESP_BT_CONTROLLER_STATUS_IDLE) { esp_bt_controller_deinit(); }
#endif
}

static inline bool radioHasMemForWifi(bool allowSdUnmount = true) {
    // If Wi-Fi driver is already up, only verify minimal scan/buffer headroom
    if (WiFi.getMode() != WIFI_MODE_NULL && radioLargestDmaBlock() >= 10 * 1024) return true;
    if (radioLargestDmaBlock() >= RADIO_WIFI_MIN_DMA_BLOCK) return true;

    // Heap looks tight: hand the SDR bank back to the heap before Wi-Fi init
    sdrBankHeapInit();
    sdrBankReleaseForRadio();
    if (radioLargestDmaBlock() >= RADIO_WIFI_MIN_DMA_BLOCK) return true;

    // Still tight: tear down BLE stack to reclaim internal DMA memory
    if (radioIsBleInitialized() || BLEConnected) {
        Serial.println("[RAM] Low contiguous DMA memory for WiFi, freeing BLE stack...");
        stopBLEStack();
        vTaskDelay(pdMS_TO_TICKS(150));
        if (radioLargestDmaBlock() >= RADIO_WIFI_MIN_DMA_BLOCK) {
            Serial.printf("[RAM] BLE freed, DMA block: %u bytes\n", (unsigned)radioLargestDmaBlock());
            return true;
        }
    }

    // Last resort: unmount SD card to free SPI/FATFS internal DMA buffers
    if (allowSdUnmount && sdcardMounted) {
        Serial.println("[RAM] Low contiguous DMA memory for WiFi, unmounting SD card...");
        closeSdCard();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    return radioLargestDmaBlock() >= RADIO_WIFI_MIN_DMA_BLOCK;
}

static inline bool radioHasMemForBle(bool allowSdUnmount = true) {
    // If NimBLE is already initialized, BLEDevice::init() is a no-op and the
    // controller already holds its DMA memory.
    if (radioIsBleInitialized()) return true;

    size_t minBlock = 0, minFree = 0;
    radioBleMinThresholds(minBlock, minFree);

    // Plenty of DMA headroom: proceed directly without touching Wi-Fi or SDR bank.
    if (radioDmaAtLeast(RADIO_BLE_TARGET_DMA_BLOCK, RADIO_BLE_TARGET_DMA_FREE)) return true;

    // Headroom is getting tight — first hand any reserved SDR bank back to the heap
    // (on Cardputer ADV this frees ~64 KiB immediately; on other boards it is a no-op).
    sdrBankHeapInit();
    sdrBankReleaseForRadio();
    if (radioDmaAtLeast(minBlock, minFree)) return true;

    // Only tear down Wi-Fi/WebUI when DMA is actually below the safe minimum for BLE,
    // allowing Wi-Fi + BLE to coexist whenever internal DMA headroom permits.
    Serial.printf(
        "[RAM] Tight DMA for BLE (largest=%u, free=%u, psram=%d), reclaiming WiFi...\n",
        (unsigned)radioLargestDmaBlock(),
        (unsigned)radioFreeDmaBytes(),
        psramFound() ? 1 : 0
    );
    radioReclaimWifiMemory(RADIO_BLE_TARGET_DMA_BLOCK, RADIO_BLE_TARGET_DMA_FREE);

    if (radioDmaAtLeast(minBlock, minFree)) {
        Serial.printf(
            "[RAM] BLE ready (largest=%u, free=%u, psram=%u)\n",
            (unsigned)radioLargestDmaBlock(),
            (unsigned)radioFreeDmaBytes(),
            (unsigned)ESP.getFreePsram()
        );
        return true;
    }

    // Still below hard minimum - unmount SD card (SPI/FATFS DMA buffers) if allowed
    if (allowSdUnmount && sdcardMounted) {
        Serial.println("[RAM] Low DMA for BLE, unmounting SD card...");
        closeSdCard();
        vTaskDelay(pdMS_TO_TICKS(50));
        if (radioDmaAtLeast(minBlock, minFree)) {
            Serial.printf(
                "[RAM] SD freed, BLE ready (largest=%u, free=%u)\n",
                (unsigned)radioLargestDmaBlock(),
                (unsigned)radioFreeDmaBytes()
            );
            return true;
        }
    }

    Serial.printf(
        "[RAM] Insufficient DMA for BLE: largest=%u (min %u), free=%u (min %u), psram=%u\n",
        (unsigned)radioLargestDmaBlock(),
        (unsigned)minBlock,
        (unsigned)radioFreeDmaBytes(),
        (unsigned)minFree,
        (unsigned)ESP.getFreePsram()
    );
    return false;
}

#if defined(RADIO_MEM_HAS_NIMBLE)
// Safely initializes NimBLE, checking the boolean return value and cleaning up
// any half-initialized BT controller on failure so callers never crash.
static inline bool radioSafeBleInit(const std::string &deviceName, bool allowSdUnmount = true) {
    if (NimBLEDevice::isInitialized()) return true;

    if (NimBLEDevice::init(deviceName)) return true;

    // First attempt failed — clean up half-initialized controller, aggressively
    // reclaim Wi-Fi and (if allowed) SD DMA buffers, and retry once.
    Serial.printf(
        "[RAM] NimBLEDevice::init failed (largest=%u, free=%u), cleaning up & retrying...\n",
        (unsigned)radioLargestDmaBlock(),
        (unsigned)radioFreeDmaBytes()
    );
    radioCleanupFailedBleController();
    sdrBankHeapInit();
    sdrBankReleaseForRadio();
    radioReclaimWifiMemory(RADIO_BLE_TARGET_DMA_BLOCK, RADIO_BLE_TARGET_DMA_FREE);
    if (allowSdUnmount && sdcardMounted) {
        closeSdCard();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (NimBLEDevice::init(deviceName)) return true;

    radioCleanupFailedBleController();
    Serial.printf(
        "[RAM] NimBLEDevice::init retry failed (largest=%u, free=%u, psram=%u)\n",
        (unsigned)radioLargestDmaBlock(),
        (unsigned)radioFreeDmaBytes(),
        (unsigned)ESP.getFreePsram()
    );
    return false;
}
#endif

#endif // __RADIO_MEM_H__
