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

extern BLEScan *pBLEScan;
extern int scanTime;

void ble_test();

void ble_scan_setup();
void ble_scan();

void disPlayBLESend();

#endif
