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
// NimBLE Version Detection - Must be consistent across all files
//=============================================================================

// Detect NimBLE 2.x by checking for features only available in v2+
#if defined(NIMBLE_VERSION)
    #if NIMBLE_VERSION >= 20000
        #define NIMBLE_V2_PLUS 1
    #endif
#elif defined(NIMBLE_CPP_VERSION) && NIMBLE_CPP_VERSION >= 2
    #define NIMBLE_V2_PLUS 1
#elif defined(NIMBLE_VERSION_MAJOR) && NIMBLE_VERSION_MAJOR >= 2
    #define NIMBLE_V2_PLUS 1
#elif defined(NIMBLE_VERSION_MAJOR) && NIMBLE_VERSION_MAJOR == 1 && NIMBLE_VERSION_MINOR >= 5
    #define NIMBLE_V2_PLUS 1
#elif __has_include(<NimBLEExtAdvertising.h>)
    #define NIMBLE_V2_PLUS 1
#endif

// If none of the above matched, check if NimBLEScanResults is a type
#ifndef NIMBLE_V2_PLUS
    #ifdef __has_include
        #if __has_include(<NimBLEScan.h>)
            #if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
                #define NIMBLE_V2_PLUS 1
            #endif
        #endif
    #endif
#endif

// If we still don't know, default to v1 behavior (safe fallback)
#ifndef NIMBLE_V2_PLUS
    #define NIMBLE_V2_PLUS 0
#endif

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
