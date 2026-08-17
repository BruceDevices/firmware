#if !defined(LITE_VERSION)
#include "nrf_jammer_api.h"
#include "core/display.h"
#include "modules/ble/BLE_Suite.h"
#include "nrf_jammer.h"
#include <RF24.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//=============================================================================
// BLE jammer implementation
//
// Hopping modes (HOP_ADV / HOP_ALL / ADV_CHANNELS / ALL_CHANNELS) are driven by
// a background FreeRTOS task so they keep hopping even while the caller blocks
// in UI code (e.g. BLE_Suite's showDeviceInfoScreen()). stopBLEJammer() tears
// the task down. updateBLEJammer() remains public for manual/custom callers;
// it is idempotent and safe to call alongside the task.
//=============================================================================

// BLE uses nRF24 channels 0-39: 0-36 are data channels, 37/38/39 are the
// advertising channels (2402/2426/2480 MHz). The nRF channel number == the
// BLE channel number, so a single 0..39 sweep covers the whole BLE band.
static byte bleAllChannels[40];
static byte bleAdvertisingChannels[] = {37, 38, 39};

#define BLE_HOP_INTERVAL_MS 100 // dwell time per channel while hopping
#define JAM_TASK_PRIORITY 1
#define JAM_TASK_STACK 2048

static bool nrf24Initialized = false;
static bool bleJammingActive = false;
static BLEJamMode currentMode = BLE_JAM_ADV_CHANNELS;
static rf24_pa_dbm_e currentPowerLevel = RF24_PA_MAX;
static unsigned long lastChannelHop = 0;
static int currentChannelIndex = 0;
static int targetChannel = 0;
static bool isHopping = false;
static byte *hopTable = NULL;     // active hop set (single source of truth)
static int hopTableCount = 0;     // number of channels in hopTable
static TaskHandle_t jammerTaskHandle = NULL;

// Full power cycle for reliable channel changes. Bare setChannel() during
// active CW leaves the PLL in an undefined state on many PA+LNA modules —
// the carrier freezes or stops after the first hop. powerUp() also needs the
// ~1.5ms crystal oscillator settle time, hence the delay(2).
static void setRadioChannel(uint8_t channel) {
    NRFradio.stopConstCarrier();
    delayMicroseconds(500);
    NRFradio.powerDown();
    delayMicroseconds(500);
    NRFradio.powerUp();
    delay(2);
    NRFradio.setChannel(channel);
    // Re-apply settings — the power cycle clears radio registers
    NRFradio.setPALevel(currentPowerLevel);
    NRFradio.setDataRate(RF24_2MBPS);
    NRFradio.setAddressWidth(3);
    NRFradio.setPayloadSize(2);
    NRFradio.startConstCarrier(currentPowerLevel, channel);
}

static void bleJammerTask(void *arg);

bool isNRF24Available() {
    if (!nrf24Initialized) {
        NRF24_MODE mode = nrf_setMode();
        if (nrf_start(mode) && CHECK_NRF_SPI(mode)) {
            NRFradio.setPALevel(RF24_PA_MAX);
            NRFradio.setAddressWidth(3);
            NRFradio.setPayloadSize(2);
            if (!NRFradio.setDataRate(RF24_2MBPS)) {
                Serial.println("Failed to set data rate to 2Mbps, trying 1Mbps");
                if (!NRFradio.setDataRate(RF24_1MBPS)) {
                    Serial.println("Failed to set data rate to 1Mbps, trying 250kbps");
                    NRFradio.setDataRate(RF24_250KBPS);
                }
            }
            for (int i = 0; i < 40; i++) bleAllChannels[i] = i;
            nrf24Initialized = true;
        }
    }
    return nrf24Initialized;
}

bool startBLEJammer(BLEJamMode mode, int param) {
    if (!isNRF24Available()) return false;
    NRF24_MODE nrfMode = nrf_setMode();
    if (!CHECK_NRF_SPI(nrfMode)) return false;

    // A previous session may still be running (task + CW carrier)
    stopBLEJammer();

    currentMode = mode;
    isHopping = false;
    hopTable = NULL;
    hopTableCount = 0;

    switch (mode) {
        case BLE_JAM_ADV_CHANNELS:
        case BLE_JAM_HOP_ADV:
        case BLE_JAM_CONNECT_ATTACK:
            hopTable = bleAdvertisingChannels;
            hopTableCount = 3;
            isHopping = true;
            break;
        case BLE_JAM_ALL_CHANNELS:
        case BLE_JAM_HOP_ALL:
            hopTable = bleAllChannels;
            hopTableCount = 40;
            isHopping = true;
            break;
        case BLE_JAM_TARGET_CHANNEL:
            if (param < 0 || param > 39) return false;
            targetChannel = param;
            break;
        default:
            return false;
    }

    currentChannelIndex = 0;
    if (isHopping) {
        setRadioChannel(hopTable[0]);
    } else {
        setRadioChannel((uint8_t)targetChannel);
    }

    bleJammingActive = true;
    lastChannelHop = millis();

    if (xTaskCreate(bleJammerTask, "bleJammer", JAM_TASK_STACK, NULL, JAM_TASK_PRIORITY, &jammerTaskHandle) !=
        pdPASS) {
        jammerTaskHandle = NULL;
    }
    return true;
}

static void bleJammerTask(void *arg) {
    while (true) {
        updateBLEJammer();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void updateBLEJammer() {
    if (!bleJammingActive || !isHopping) return;
    if (!hopTable || hopTableCount <= 0) return;
    unsigned long now = millis();
    if (now - lastChannelHop < BLE_HOP_INTERVAL_MS) return;
    currentChannelIndex = (currentChannelIndex + 1) % hopTableCount;
    setRadioChannel(hopTable[currentChannelIndex]);
    lastChannelHop = now;
}

void stopBLEJammer() {
    if (jammerTaskHandle) {
        vTaskDelete(jammerTaskHandle);
        jammerTaskHandle = NULL;
    }
    if (!bleJammingActive) return;
    NRF24_MODE mode = nrf_setMode();
    if (CHECK_NRF_SPI(mode) && nrf24Initialized) {
        NRFradio.stopConstCarrier();
        NRFradio.powerDown(); // explicit power-down for clean shutdown
    }
    bleJammingActive = false;
    isHopping = false;
    currentChannelIndex = 0;
    hopTable = NULL;
    hopTableCount = 0;
}

bool isBLEJammingActive() { return bleJammingActive; }

int getCurrentBLEChannel() {
    if (!bleJammingActive) return -1;
    if (currentMode == BLE_JAM_TARGET_CHANNEL) return targetChannel;
    if (isHopping && hopTable) return hopTable[currentChannelIndex];
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
    if (!bleJammingActive || !nrf24Initialized) return;
    // Re-apply the power level to the channel currently being jammed
    // without tearing down the hopping session.
    uint8_t ch = (currentMode == BLE_JAM_TARGET_CHANNEL)
                     ? (uint8_t)targetChannel
                     : (isHopping && hopTable) ? hopTable[currentChannelIndex] : 0;
    setRadioChannel(ch);
}

bool jamBLEChannel(int channel) {
    if (channel < 0 || channel > 39) return false;
    return startBLEJammer(BLE_JAM_TARGET_CHANNEL, channel);
}

bool jamBLEAdvertisingChannels() { return startBLEJammer(BLE_JAM_ADV_CHANNELS); }

bool jamBLEConnectionChannel(NimBLEAddress target) { return startBLEJammer(BLE_JAM_ADV_CHANNELS); }

bool jamDuringConnect(NimBLEAddress target) {
    if (!isNRF24Available()) return false;
    startBLEJammer(BLE_JAM_ADV_CHANNELS);
    String connectionMethod = "";
    NimBLEClient *pClient = attemptConnectionWithStrategies(target, connectionMethod);
    stopBLEJammer();
    if (pClient) {
        pClient->disconnect();
        NimBLEDevice::deleteClient(pClient);
        return true;
    }
    return false;
}
#endif