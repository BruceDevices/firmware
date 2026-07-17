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
// NimBLE Version Detection - Simplified for reliability
//=============================================================================

// Define NIMBLE_V2_PLUS based on available features
// We check for NimBLEScanCallbacks which is v2-specific
#ifndef NIMBLE_V2_PLUS
    // Check if NimBLEScanCallbacks is defined (v2 feature)
    #ifdef __has_include
        #if __has_include(<NimBLEScan.h>)
            // Try to detect v2 by checking for a v2-only type
            // We'll use a simple approach: check if NimBLEScanCallbacks exists
            namespace __nimble_detect {
                template<typename...> struct void_type { typedef void type; };
                template<typename T, typename = void> struct has_scan_callbacks : std::false_type {};
                template<typename T> struct has_scan_callbacks<T, 
                    typename void_type<typename T::ScanCallbacks>::type> : std::true_type {};
            }
            // If the compiler can see NimBLEScanCallbacks, we're on v2+
            #if __nimble_detect::has_scan_callbacks<NimBLEDevice>::value
                #define NIMBLE_V2_PLUS 1
            #endif
        #endif
    #endif
#endif

// Check specific NimBLE version macros
#ifndef NIMBLE_V2_PLUS
    #if defined(NIMBLE_VERSION)
        #if NIMBLE_VERSION >= 20000
            #define NIMBLE_V2_PLUS 1
        #endif
    #endif
#endif

#ifndef NIMBLE_V2_PLUS
    #if defined(NIMBLE_CPP_VERSION) && NIMBLE_CPP_VERSION >= 2
        #define NIMBLE_V2_PLUS 1
    #endif
#endif

#ifndef NIMBLE_V2_PLUS
    #if defined(NIMBLE_VERSION_MAJOR) && NIMBLE_VERSION_MAJOR >= 2
        #define NIMBLE_V2_PLUS 1
    #endif
#endif

// If we're on ESP-IDF 5.0+, likely using NimBLE 2.x
#ifndef NIMBLE_V2_PLUS
    #if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        #define NIMBLE_V2_PLUS 1
    #endif
#endif

// Check for NimBLEExtAdvertising.h which is v2-specific
#ifndef NIMBLE_V2_PLUS
    #if __has_include(<NimBLEExtAdvertising.h>)
        #define NIMBLE_V2_PLUS 1
    #endif
#endif

// If we still don't know, default to v1 behavior (safe fallback)
#ifndef NIMBLE_V2_PLUS
    #define NIMBLE_V2_PLUS 0
#endif

// Debug output to help with troubleshooting
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
class AdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
public:
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override;
    void onScanEnd(NimBLEScanResults results, int reason) override;
};
#else
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override;
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
