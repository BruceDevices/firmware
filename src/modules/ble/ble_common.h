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
// AdvertisedDeviceCallbacks - SINGLE DEFINITION (Header Only)
//=============================================================================

// Forward declaration of pBLEScan for the callback
extern BLEScan *pBLEScan;

// This is the ONLY definition of AdvertisedDeviceCallbacks
// It's defined inline in the header so it's visible everywhere
class AdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
public:
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override {
        if (!advertisedDevice) return;
        
        if (options.size() >= MAX_DISPLAY_DEVICES) {
            if (pBLEScan) {
                pBLEScan->stop();
                Serial.println("Reached max devices, stopping scan");
            }
            return;
        }
        
        String bt_title;
        String bt_name;
        String bt_address;
        String bt_signal;

        bt_name = advertisedDevice->getName().c_str();
        bt_address = advertisedDevice->getAddress().toString().c_str();
        bt_signal = String(advertisedDevice->getRSSI());
        
        if (bt_name.isEmpty()) bt_name = "<no name>";
        bt_title = bt_name;
        if (bt_title.isEmpty()) bt_title = bt_address;
        
        if (options.size() < MAX_DISPLAY_DEVICES) {
            options.emplace_back(bt_title.c_str(), [=]() { 
                ble_info(bt_name, bt_address, bt_signal); 
            });
        }
    }
};

// External reference to g_scanCallbacks
extern AdvertisedDeviceCallbacks* g_scanCallbacks;

extern BLEScan *pBLEScan;
extern int scanTime;

// Function declarations
void ble_info(const String &name, const String &address, const String &signal);
void ble_test();
constexpr bool FORCE_RADIO_TEARDOWN_ON_SWITCH = false;

bool ble_scan_setup();
void ble_scan();
void stopBLEStack();

bool bleNotifyRetry(NimBLECharacteristic *chr, const uint8_t *value, size_t length, uint8_t retries = 8);
bool bleNotifyRetry(NimBLECharacteristic *chr, uint8_t retries = 8);

void disPlayBLESend();

#endif
