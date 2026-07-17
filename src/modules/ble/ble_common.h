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

#define MAX_DISPLAY_DEVICES 100

//=============================================================================
// AdvertisedDeviceCallbacks - DECLARATION ONLY
//=============================================================================

// Forward declaration - the class is defined in ble_common.cpp
class AdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
public:
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override;
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
