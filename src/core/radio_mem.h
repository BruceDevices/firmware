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
// On boards where the SDR bank is reserved out of the heap (see
// modules/wifi/sdr.cpp) the menu heap only has ~15.8 kB contiguous (measured on
// Cardputer ADV) and the controller still fails ("Malloc failed"), so the
// threshold must sit comfortably above that; the SDR bank hand-off below gives
// the heap ~64 kB more when the check fails, which is what makes this higher
// gate reachable there.
// On every other board the bank is NOT reserved (BRUCE_NO_SDR_JAMMER), so there
// is no hand-off that could ever satisfy a bigger gate: those builds keep the
// pre-1.17 gate (same as Wi-Fi), otherwise BLE fails right after boot on
// T-Embed/T-Deck/etc. even with WiFi off.
#if !defined(LITE_VERSION) && !defined(BRUCE_NO_SDR_JAMMER)
constexpr size_t RADIO_BLE_MIN_DMA_BLOCK = 48 * 1024;
#else
constexpr size_t RADIO_BLE_MIN_DMA_BLOCK = RADIO_WIFI_MIN_DMA_BLOCK;
#endif

// SDR bank hand-off (implemented in modules/wifi/sdr.cpp): the SDR engine's
// 64 KiB SRAM bank is kept out of the heap behind a fence allocation so the IQ
// record/jammer can always start; when a radio bring-up finds the heap too
// small, releasing the fence returns the bank to the heap. After that the SDR
// needs a reboot (it refuses cleanly instead of corrupting the heap).
#if !defined(LITE_VERSION) && !defined(BRUCE_NO_SDR_JAMMER)
void sdrBankReleaseForRadio();
void sdrBankHeapInit();
#else
static inline void sdrBankReleaseForRadio() {}
static inline void sdrBankHeapInit() {}
#endif

static inline bool radioHasMemForWifi() {
    if (radioLargestDmaBlock() >= RADIO_WIFI_MIN_DMA_BLOCK) return true;
    // Heap looks tight: hand the SDR bank back to the heap before Wi-Fi init
    sdrBankHeapInit();
    sdrBankReleaseForRadio();
    return radioLargestDmaBlock() >= RADIO_WIFI_MIN_DMA_BLOCK;
}

static inline bool radioHasMemForBle() {
    // return true; // uncomment to disable it
    if (radioLargestDmaBlock() >= RADIO_BLE_MIN_DMA_BLOCK) return true;

    // Not enough DMA memory - first hand the SDR bank back to the heap
    sdrBankHeapInit();
    sdrBankReleaseForRadio();
    if (radioLargestDmaBlock() >= RADIO_BLE_MIN_DMA_BLOCK) {
        return true;
    }

    // Still not enough - try to free WiFi
    Serial.println("[RAM] Low contiguous DMA memory for BLE, attempting to free WiFi...");

    // Disconnect WiFi if active
    if (WiFi.getMode() != WIFI_MODE_NULL || wifiConnected) {
        wifiDisconnect();
        delay(200);
        #ifdef WIFI_DEINIT_ON_DISCONNECT
        WiFi.mode(WIFI_OFF);
        #endif
        delay(300);
    }

    // Recheck after freeing WiFi
    if (radioLargestDmaBlock() >= RADIO_BLE_MIN_DMA_BLOCK) {
        Serial.printf("[RAM] WiFi freed, DMA block: %d bytes\n", radioLargestDmaBlock());
        return true;
    }

    // Still not enough - return false, caller shows error
    Serial.printf("[RAM] Still only %d bytes DMA block, minimum %d needed\n",
                  radioLargestDmaBlock(), RADIO_BLE_MIN_DMA_BLOCK);
    return false;
}

#endif // __RADIO_MEM_H__
