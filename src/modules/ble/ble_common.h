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
// NimBLE Version - MANUAL FORCE FOR NIMBLE 2.x
//=============================================================================

// Since you're using NimBLE 2.3.7+, set this to 1
#ifndef NIMBLE_V2_PLUS
    #define NIMBLE_V2_PLUS 1
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

//=============================================================================
// AdvertisedDeviceCallbacks - NO onScanEnd (doesn't exist in NimBLE)
//=============================================================================

// For NimBLE 2.x - uses NimBLEScanCallbacks with ONLY onResult
class AdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
public:
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override;
    // onScanEnd does NOT exist in NimBLEScanCallbacks - DO NOT DECLARE IT!
};

// External reference to g_scanCallbacks
extern AdvertisedDeviceCallbacks* g_scanCallbacks;

extern BLEScan *pBLEScan;
extern int scanTime;

void ble_test();
constexpr bool FORCE_RADIO_TEARDOWN_ON_SWITCH = false;

bool ble_scan_setup();
void ble_scan();
void stopBLEStack();

bool bleNotifyRetry(NimBLECharacteristic *chr, const uint8_t *value, size_t length, uint8_t retries = 8);
bool bleNotifyRetry(NimBLECharacteristic *chr, uint8_t retries = 8);

void disPlayBLESend();

#endif
