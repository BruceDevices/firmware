#ifndef __BLE_COMMON_H__
#define __BLE_COMMON_H__

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEBeacon.h>
#include <NimBLEScan.h>

#include "core/display.h"
#include <globals.h>

//=============================================================================
// NimBLE Version Detection - SUPER SIMPLE
//=============================================================================

// Try to detect NimBLE 2.x using simple macro checks
// NIMBLE_V2_PLUS is defined as 1 for v2, 0 for v1 (default)

// First check: NIMBLE_VERSION macro
#ifdef NIMBLE_VERSION
    #if NIMBLE_VERSION >= 20000
        #ifndef NIMBLE_V2_PLUS
            #define NIMBLE_V2_PLUS 1
        #endif
    #endif
#endif

// Second check: NIMBLE_CPP_VERSION
#ifdef NIMBLE_CPP_VERSION
    #if NIMBLE_CPP_VERSION >= 2
        #ifndef NIMBLE_V2_PLUS
            #define NIMBLE_V2_PLUS 1
        #endif
    #endif
#endif

// Third check: NIMBLE_VERSION_MAJOR
#ifdef NIMBLE_VERSION_MAJOR
    #if NIMBLE_VERSION_MAJOR >= 2
        #ifndef NIMBLE_V2_PLUS
            #define NIMBLE_V2_PLUS 1
        #endif
    #endif
#endif

// Fourth check: ESP-IDF version
#ifdef ESP_IDF_VERSION
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        #ifndef NIMBLE_V2_PLUS
            #define NIMBLE_V2_PLUS 1
        #endif
    #endif
#endif

// Fifth check: Check for v2-specific header
#ifdef __has_include
    #if __has_include(<NimBLEExtAdvertising.h>)
        #ifndef NIMBLE_V2_PLUS
            #define NIMBLE_V2_PLUS 1
        #endif
    #endif
#endif

// If no detection succeeded, default to v1 (safe fallback)
#ifndef NIMBLE_V2_PLUS
    #define NIMBLE_V2_PLUS 0
#endif

// Print the detected version for debugging
#pragma message("NIMBLE_V2_PLUS = " __STRINGIFY(NIMBLE_V2_PLUS))

//=============================================================================
// BLE Constants
//=============================================================================

#define SCANTIME 5
#define SCANTYPE ACTIVE
#define SCAN_INT 100
#define SCAN_WINDOW 99

// Maximum number of BLE devices to display to prevent memory issues
#define MAX_DISPLAY_DEVICES 100

// Memory protection: Reduce scan time in low-memory situations
#define SCAN_TIME_REDUCED 3

//=============================================================================
// Forward Declaration - SINGLE SOURCE OF TRUTH
//=============================================================================

// Forward declaration of AdvertisedDeviceCallbacks
// Only declare onScanEnd for NimBLE 2.x
#if NIMBLE_V2_PLUS
// NimBLE 2.x version
class AdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
public:
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override;
    void onScanEnd(NimBLEScanResults results, int reason) override;
};
#else
// NimBLE 1.x version - NO onScanEnd!
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override;
    // No onScanEnd - this doesn't exist in NimBLE 1.x
};
#endif

// External reference to g_scanCallbacks
extern AdvertisedDeviceCallbacks* g_scanCallbacks;

extern BLEScan *pBLEScan;
extern int scanTime;

void ble_test();
#if 0
#ifdef BOARD_HAS_PSRAM
constexpr bool FORCE_RADIO_TEARDOWN_ON_SWITCH = false;
#else
constexpr bool FORCE_RADIO_TEARDOWN_ON_SWITCH = true;
#endif
#else
constexpr bool FORCE_RADIO_TEARDOWN_ON_SWITCH = false;
#endif

bool ble_scan_setup();
void ble_scan();
void stopBLEStack();

bool bleNotifyRetry(NimBLECharacteristic *chr, const uint8_t *value, size_t length, uint8_t retries = 8);
bool bleNotifyRetry(NimBLECharacteristic *chr, uint8_t retries = 8);

void disPlayBLESend();

#endif
