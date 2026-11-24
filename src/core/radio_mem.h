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

    Teardown timing: WiFi.mode(WIFI_OFF) releases the driver's DMA buffers
    asynchronously. A single fixed delay after wifiDisconnect() either
    over-waits (slowing boot) or under-waits (false "Low RAM" popups that
    resolve a moment later). radioHasMemForBle() therefore polls the largest
    block for up to ~1.5s after triggering teardown, returning as soon as the
    threshold is met, and only failing (popup) if the block genuinely never
    recovers in that window.
*/
#include <esp_heap_caps.h>
#include <stddef.h>
#include "core/iq_record_gate.h"
#include "core/wifi/wifi_common.h"
#include <WiFi.h>

// Largest contiguous DMA-capable internal block, in bytes. This is the number
// that gates Wi-Fi/BLE controller init.
static inline size_t radioLargestDmaBlock() { return heap_caps_get_largest_free_block(MALLOC_CAP_DMA); }

// Minimum contiguous DMA block required BEFORE bringing Wi-Fi up (checked at the
// scan/menu entry, i.e. before esp_wifi is touched at all).
constexpr size_t RADIO_WIFI_MIN_DMA_BLOCK = 8 * 1024;

// Minimum contiguous DMA block required before bringing the BLE stack up.
// Lowered to 8 KB: the BLE suite only need a small NimBLE
// controller init block. The WiFi-sized gate (8 KB) remains
// as a fallback check in radioHasMemForBle() for the "pre-1.17 layout" case.
#if !defined(BRUCE_IQ_RECORD_UNAVAILABLE)
constexpr size_t RADIO_BLE_MIN_DMA_BLOCK = 8 * 1024;
#else
constexpr size_t RADIO_BLE_MIN_DMA_BLOCK = RADIO_WIFI_MIN_DMA_BLOCK;
#endif

// SDR bank hand-off (implemented in modules/wifi/sdr.cpp): the IQ record's
// 64 KiB SRAM bank is kept out of the heap behind a fence allocation so the
// capture can always start; when a radio bring-up finds the heap too
// small, releasing the fence returns the bank to the heap. After that the SDR
// needs a reboot (it refuses cleanly instead of corrupting the heap).
#if !defined(BRUCE_IQ_RECORD_UNAVAILABLE)
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

    // Fast path: enough DMA already.
    if (radioLargestDmaBlock() >= RADIO_BLE_MIN_DMA_BLOCK) return true;

    // Hand the SDR bank back to the heap (if it was reserved).
    sdrBankHeapInit();
    sdrBankReleaseForRadio();
    if (radioLargestDmaBlock() >= RADIO_BLE_MIN_DMA_BLOCK) return true;

    // Pre-1.17 layout fallback: if the block meets the WiFi-sized gate, the
    // heap is in a state where BLE ran fine before 1.17, so accept it.
    if (radioLargestDmaBlock() >= RADIO_WIFI_MIN_DMA_BLOCK) return true;

    // Try to free WiFi. Teardown is asynchronous, so poll for up to ~1.5s.
    Serial.println("[RAM] Low contiguous DMA memory for BLE, attempting to free WiFi...");

    if (WiFi.getMode() != WIFI_MODE_NULL || wifiConnected) {
        wifiDisconnect();
#ifdef WIFI_DEINIT_ON_DISCONNECT
        WiFi.mode(WIFI_OFF);
#endif
        for (int i = 0; i < 6; i++) {
            delay(250);
            size_t block = radioLargestDmaBlock();
            if (block >= RADIO_WIFI_MIN_DMA_BLOCK) {
                Serial.printf("[RAM] WiFi freed after %d ms, DMA block: %u bytes\n",
                              (i + 1) * 250, (unsigned)block);
                return true;
            }
        }
    }

    Serial.printf("[RAM] Still only %u bytes DMA block, minimum %u needed\n",
                  (unsigned)radioLargestDmaBlock(), (unsigned)RADIO_BLE_MIN_DMA_BLOCK);
    return false;
}

#endif // __RADIO_MEM_H__
