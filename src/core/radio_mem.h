#ifndef __RADIO_MEM_H__
#define __RADIO_MEM_H__
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
#include "core/wifi/wifi_common.h"
#include <WiFi.h>

// Largest contiguous DMA-capable internal block, in bytes. This is the number
// that gates Wi-Fi/BLE controller init.
static inline size_t radioLargestDmaBlock() { return heap_caps_get_largest_free_block(MALLOC_CAP_DMA); }

// Minimum contiguous DMA block required BEFORE bringing Wi-Fi up (checked at the
// scan/menu entry, i.e. before esp_wifi is touched at all).
constexpr size_t RADIO_WIFI_MIN_DMA_BLOCK = 15 * 1024;

// Minimum contiguous DMA block required before bringing the BLE stack up.
constexpr size_t RADIO_BLE_MIN_DMA_BLOCK = 15 * 1024;

static inline bool radioHasMemForWifi() {
    // return true; // uncomment to disable it
    return radioLargestDmaBlock() >= RADIO_WIFI_MIN_DMA_BLOCK;
}

static inline bool radioHasMemForBle() {
    // return true; // uncomment to disable it

    // Already enough DMA memory — nothing to do.
    if (radioLargestDmaBlock() >= RADIO_BLE_MIN_DMA_BLOCK) {
        return true;
    }

    // Not enough DMA memory. Suspend WiFi through its owner (wifi_common),
    // which calls esp_wifi_stop() and actually frees the driver's DMA buffers
    // (unlike wifiDisconnect(), which only disconnects and leaves the driver
    // holding its DMA allocations).
    Serial.println("[RAM] Low contiguous DMA memory for BLE, suspending WiFi...");
    wifiSuspend();
    delay(200);

    if (radioLargestDmaBlock() >= RADIO_BLE_MIN_DMA_BLOCK) {
        Serial.printf("[RAM] WiFi suspended, DMA block: %u bytes\n", (unsigned)radioLargestDmaBlock());
        return true;
    }

    Serial.printf("[RAM] Still only %u bytes DMA block, minimum %u needed\n",
                  (unsigned)radioLargestDmaBlock(), (unsigned)RADIO_BLE_MIN_DMA_BLOCK);
    return false;
}

#endif // __RADIO_MEM_H__
