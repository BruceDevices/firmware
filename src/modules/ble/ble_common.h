#ifndef __BLE_COMMON_H__
#define __BLE_COMMON_H__

// #include "BLE2902.h"
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>

#include <NimBLEAdvertisedDevice.h>
#include <NimBLEBeacon.h>
#include <NimBLEScan.h>

#include "core/display.h"
#include <globals.h>

#define SCANTIME 5
#define SCANTYPE ACTIVE
#define SCAN_INT 100
#define SCAN_WINDOW 99

// Maximum number of BLE devices to display to prevent memory issues
// In dense environments (subway, airport, etc.) there can be hundreds of devices
// Limiting to 100 prevents out-of-memory crashes while still showing plenty
#define MAX_DISPLAY_DEVICES 100

extern BLEScan *pBLEScan;
extern int scanTime;

void ble_test();
#if 0 // keep it out for now
#ifdef BOARD_HAS_PSRAM
constexpr bool FORCE_RADIO_TEARDOWN_ON_SWITCH = false;
#else
constexpr bool FORCE_RADIO_TEARDOWN_ON_SWITCH = true;
#endif
#else
constexpr bool FORCE_RADIO_TEARDOWN_ON_SWITCH = false;
#endif

// Initialize BLE scan with proper memory management
// Returns: true on success, false if aborted (e.g. not enough contiguous RAM)
bool ble_scan_setup();

// Perform BLE scan and display results
// Automatically handles device limits to prevent memory issues
void ble_scan();

// Safely stop BLE stack and clean up resources
// Does not aggressively deinit if BLE is still needed
void stopBLEStack();

// GATT notification tolerant to temporary MSYS-pool exhaustion.
// NimBLECharacteristic::notify() returns false when the os_mbuf could not be
// allocated (MSYS pool full, made worse by reducing
// CONFIG_BT_NIMBLE_MSYS_*_BLOCK_COUNT). If untreated, the notification is silently
// dropped (lost HID key / BLE-serial chunk). Retry while yielding 1 tick for the
// host to drain the pool. Mirrors wifiRawTx() on the Wi-Fi side.
bool bleNotifyRetry(NimBLECharacteristic *chr, const uint8_t *value, size_t length, uint8_t retries = 8);
bool bleNotifyRetry(NimBLECharacteristic *chr, uint8_t retries = 8);

// Display BLE send interface
void disPlayBLESend();

#endif
