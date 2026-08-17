#ifndef NRF_JAMMER_API_H
#define NRF_JAMMER_API_H
#if !defined(LITE_VERSION)
#include "modules/NRF24/nrf_common.h"
#include <NimBLEDevice.h>

enum BLEJamMode {
    BLE_JAM_ADV_CHANNELS = 0, // hop the 3 advertising channels (37/38/39)
    BLE_JAM_ALL_CHANNELS,     // sweep all 40 BLE channels (0-36 data + 37/38/39 adv)
    BLE_JAM_TARGET_CHANNEL,   // CW on one channel (param: 0-39)
    BLE_JAM_HOP_ADV,          // alias of BLE_JAM_ADV_CHANNELS
    BLE_JAM_HOP_ALL,          // alias of BLE_JAM_ALL_CHANNELS
    BLE_JAM_CONNECT_ATTACK    // same as BLE_JAM_ADV_CHANNELS
};

// Hopping modes run in a background FreeRTOS task started by startBLEJammer()
// and torn down by stopBLEJammer(), so they keep hopping even while the
// caller blocks in UI code. updateBLEJammer() is kept public for manual
// callers; it is idempotent and safe to call alongside the task.

bool isNRF24Available();
bool startBLEJammer(BLEJamMode mode, int param = 0);
void updateBLEJammer();
void stopBLEJammer();
bool isBLEJammingActive();
int getCurrentBLEChannel();
void setBLEJammingPower(int powerLevel);
bool jamBLEChannel(int channel);
bool jamBLEAdvertisingChannels();
bool jamBLEConnectionChannel(NimBLEAddress target);
bool jamDuringConnect(NimBLEAddress target);

#endif
#endif