#ifndef NRF_JAMMER_API_H
#define NRF_JAMMER_API_H

#include "modules/NRF24/nrf_common.h"
#include <NimBLEDevice.h>

enum BLEJamMode {
    BLE_JAM_ADV_CHANNELS = 0,
    BLE_JAM_ALL_CHANNELS,
    BLE_JAM_TARGET_CHANNEL,
    BLE_JAM_HOP_ADV,
    BLE_JAM_HOP_ALL,
    BLE_JAM_CONNECT_ATTACK
};

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