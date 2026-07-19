#include "ble_common.h"
#include "core/mykeyboard.h"
#include "core/radio_mem.h"
#include "core/ram_profile.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "esp_mac.h"
#include "modules/badusb_ble/ducky_typer.h"
#if !defined(LITE_VERSION)
#include "BLE_Suite.h"
#endif

#define SERVICE_UUID "1bc68b2a-f3e3-11e9-81b4-2a2ae2dbcce4"
#define CHARACTERISTIC_RX_UUID "1bc68da0-f3e3-11e9-81b4-2a2ae2dbcce4"
#define CHARACTERISTIC_TX_UUID "1bc68efe-f3e3-11e9-81b4-2a2ae2dbcce4"

BLEScan *pBLEScan = nullptr;
int scanTime = SCANTIME; // In seconds

bool bleNotifyRetry(NimBLECharacteristic *chr, const uint8_t *value, size_t length, uint8_t retries) {
    if (chr == nullptr) return false;
    if (chr->notify(value, length)) return true;
    for (uint8_t i = 0; i < retries; i++) {
        vTaskDelay(1);
        if (chr->notify(value, length)) return true;
    }
    return false;
}

bool bleNotifyRetry(NimBLECharacteristic *chr, uint8_t retries) {
    if (chr == nullptr) return false;
    if (chr->notify()) return true;
    for (uint8_t i = 0; i < retries; i++) {
        vTaskDelay(1);
        if (chr->notify()) return true;
    }
    return false;
}

#define ENDIAN_CHANGE_U16(x) ((((x) & 0xFF00) >> 8) + (((x) & 0xFF) << 8))

BLEServer *pServer = NULL;
BLEService *pService = NULL;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;
bool bleDataTransferEnabled = false;

bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) { deviceConnected = true; };

    void onDisconnect(BLEServer *pServer) { deviceConnected = false; }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    NimBLEAttValue data;
    void onWrite(NimBLECharacteristic *pCharacteristic) { data = pCharacteristic->getValue(); }
};

uint8_t sta_mac[6];
char strID[18];
char strAddl[200];

void ble_info(const String &name, const String &address, const String &signal) {
    drawMainBorder();
    tft.setTextColor(bruceConfig.priColor);
    tft.drawCentreString("-=Information=-", tftWidth / 2, 28, SMOOTH_FONT);
    tft.drawString("Name: " + name, 10, 48);
    tft.drawString("Adresse: " + address, 10, 66);
    tft.drawString("Signal: " + String(signal) + " dBm", 10, 84);
    tft.drawCentreString("   Press " + String(BTN_ALIAS) + " to act", tftWidth / 2, tftHeight - 20, 1);

    delay(300);
    while (!check(SelPress)) {
        while (!check(SelPress)) { yield(); }
        returnToMenu = true;
        break;
    }
}

//=============================================================================
// NimBLE Callbacks - Version-specific with proper lifetime management
//=============================================================================

#if NIMBLE_V2_PLUS
class AdvertisedDeviceCallbacks : public NimBLEScanCallbacks {};
#else
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override {}
};
#endif

static AdvertisedDeviceCallbacks g_scanCallbacks;

static bool is_ble_inited = false;

void stopBLEStack() {
    if (pBLEScan) {
        pBLEScan->stop();
        pBLEScan->clearResults();
        // Don't delete pBLEScan - it's owned by BLEDevice
        pBLEScan = nullptr;
    }

    if (is_ble_inited) {
#if !defined(LITE_VERSION)
        if (BLEStateManager::isBLEActive() || BLEStateManager::getActiveClientCount() > 0) {
            BLEStateManager::deinitBLE(true);
        } else
#endif
            if (BLEDevice::getScan() != nullptr || BLEDevice::getAdvertising() != nullptr ||
                BLEDevice::getServer() != nullptr || BLEConnected) {
            BLEDevice::deinit();
        }
    }

    pServer = nullptr;
    pService = nullptr;
    pTxCharacteristic = nullptr;
    pRxCharacteristic = nullptr;
    deviceConnected = false;
    oldDeviceConnected = false;
    bleDataTransferEnabled = false;
    is_ble_inited = false;
    BLEConnected = false;
#if !defined(LITE_VERSION)
    if (hid_ble) {
        delete hid_ble;
        hid_ble = nullptr;
    }
#endif
}

bool ble_scan_setup() {
    if (FORCE_RADIO_TEARDOWN_ON_SWITCH) {
        if (WiFi.getMode() != WIFI_MODE_NULL || wifiConnected) {
            wifiDisconnect();
            delay(200);
        }

        stopBLEStack();
        delay(100);
    }

    RAM_LOG("ble-scan pre-init");

    if (!is_ble_inited) {
        if (!radioHasMemForBle()) {
            displayError("Low RAM: free WiFi/SD first", true);
            returnToMenu = true;
            return false;
        }
        // Use a minimal name to save RAM
        BLEDevice::init("");
        is_ble_inited = true;
    }

    RAM_LOG("ble-scan post-init");
    pBLEScan = BLEDevice::getScan();
    if (!pBLEScan) {
        displayError("Failed to get scan object", true);
        return false;
    }

#if NIMBLE_V2_PLUS
    pBLEScan->setScanCallbacks(&g_scanCallbacks);
#else
    pBLEScan->setAdvertisedDeviceCallbacks(&g_scanCallbacks);
#endif

    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(SCAN_INT);
    pBLEScan->setWindow(SCAN_WINDOW);
    pBLEScan->setDuplicateFilter(false);

    esp_read_mac(sta_mac, ESP_MAC_BT);

    sprintf(
        strID,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        sta_mac[0],
        sta_mac[1],
        sta_mac[2],
        sta_mac[3],
        sta_mac[4],
        sta_mac[5]
    );
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return true;
}

void ble_scan() {
    displayTextLine("Scanning..");

    options = {};
    options.reserve(MAX_DISPLAY_DEVICES);
    
    bool bleWasActiveBefore = BLEConnected || (BLEDevice::getServer() != nullptr);
#if !defined(LITE_VERSION)
    bleWasActiveBefore = bleWasActiveBefore || BLEStateManager::isBLEActive() || BLEStateManager::getActiveClientCount() > 0;
#endif

    if (!ble_scan_setup() || pBLEScan == nullptr) {
        displayError("Failed to init BLE scan");
        return;
    }

    // Clear previous results before scanning
    pBLEScan->clearResults();

    // Use a try-catch block to handle potential exceptions
    try {
#if NIMBLE_V2_PLUS
        BLEScanResults foundDevices = pBLEScan->getResults(scanTime * 1000, false);
#else
        // NimBLE 1.x: start() returns results directly, time in seconds
        BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
#endif

        int deviceCount = foundDevices.getCount();
        int processedCount = 0;
        
        // Cap the number of devices to prevent memory issues
        int maxToProcess = min(deviceCount, MAX_DISPLAY_DEVICES);
        
        for (int i = 0; i < maxToProcess && processedCount < MAX_DISPLAY_DEVICES; i++) {
#if NIMBLE_V2_PLUS
            const NimBLEAdvertisedDevice *advertisedDevice = foundDevices.getDevice(i);
#else
            NimBLEAdvertisedDevice *advertisedDevice = foundDevices.getDevice(i);
#endif
            if (!advertisedDevice) continue;
            
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
                processedCount++;
            }
        }

        // Show "and more" if we hit the limit
        if (options.size() >= MAX_DISPLAY_DEVICES) {
            options.emplace_back("... and more devices", nullptr);
        }
    } catch (...) {
        displayError("BLE scan error");
        pBLEScan->clearResults();
        return;
    }
    
    // Stop scan
    if (pBLEScan) {
        pBLEScan->stop();
        // Don't clear results here - we need them for display
    }
    
    // Only stop BLE if it wasn't active before and we're done with it
    if (!bleWasActiveBefore) {
#if !defined(LITE_VERSION)
        if (!BLEStateManager::isBLEActive()) {
            stopBLEStack();
        }
#else
        stopBLEStack();
#endif
    }
    
    if (!options.empty()) {
        addOptionToMainMenu();
        loopOptions(options);
        options.clear();
    } else {
        displayError("No devices found");
        delay(1000);
    }
}

bool initBLEServer() {
    uint64_t chipid = ESP.getEfuseMac();
    String blename = "Bruce-" + String((uint8_t)(chipid >> 32), HEX);

    if (!is_ble_inited) {
        BLEDevice::init(blename.c_str());
        is_ble_inited = true;
    }
    
    pServer = BLEDevice::createServer();
    if (!pServer) {
        displayError("Failed to create BLE server");
        return false;
    }

    pServer->setCallbacks(new MyServerCallbacks());
    pService = pServer->createService(SERVICE_UUID);
    if (!pService) {
        displayError("Failed to create BLE service");
        return false;
    }
    
    pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_RX_UUID, NIMBLE_PROPERTY::NOTIFY);
    if (!pTxCharacteristic) {
        displayError("Failed to create TX characteristic");
        return false;
    }

    pTxCharacteristic->addDescriptor(new NimBLE2904());
    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_TX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    if (!pRxCharacteristic) {
        displayError("Failed to create RX characteristic");
        return false;
    }
    pRxCharacteristic->setCallbacks(new MyCallbacks());

#if NIMBLE_V2_PLUS
    // NimBLE 2.x: Services start automatically when server starts
    // No need to call pService->start()
#else
    // NimBLE 1.x: Need to call pService->start()
    pService->start();
#endif

    return true;
}

void disPlayBLESend() {
    uint8_t senddata[2] = {0};
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();
    tft.setTextSize(1);

    if (!pServer) {
        if (!initBLEServer()) {
            displayError("Failed to init BLE server");
            return;
        }
    }

    pServer->getAdvertising()->start();

    uint64_t chipid = ESP.getEfuseMac();
    String blename = "Bruce-" + String((uint8_t)(chipid >> 32), HEX);

    BLEConnected = true;

    bool wasConnected = false;
    bool first_run = true;
    while (!check(EscPress)) {
        if (deviceConnected) {
            if (!wasConnected) {
                tft.fillRect(10, 26, tftWidth - 20, tftHeight - 36, TFT_BLACK);
                drawBLE_beacon(180, 28, TFT_BLUE);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.setTextSize(FM);
                tft.setCursor(12, 50);
                tft.printf("BLE Send\n");
                tft.setTextSize(FM);
            }
            tft.fillRect(10, 100, tftWidth - 20, 28, TFT_BLACK);
            tft.setCursor(12, 100);
            if (senddata[0] % 4 == 0) {
                tft.printf("0x%02X>    ", senddata[0]);
            } else if (senddata[0] % 4 == 1) {
                tft.printf("0x%02X>>   ", senddata[0]);
            } else if (senddata[0] % 4 == 2) {
                tft.printf("0x%02X >>  ", senddata[0]);
            } else if (senddata[0] % 4 == 3) {
                tft.printf("0x%02X  >  ", senddata[0]);
            }

            senddata[1]++;
            if (senddata[1] > 3) {
                senddata[1] = 0;
                senddata[0]++;
                pTxCharacteristic->setValue(senddata, 1);
                pTxCharacteristic->notify();
            }
            wasConnected = true;
        } else {
            if (wasConnected or first_run) {
                first_run = false;
                tft.fillRect(10, 26, tftWidth - 20, tftHeight - 36, TFT_BLACK);
                tft.setTextSize(FM);
                tft.setCursor(12, 50);
                tft.setTextColor(TFT_RED);
                tft.printf("BLE disconnect\n");
                tft.setCursor(12, 75);
                tft.setTextColor(tft.color565(18, 150, 219));

                tft.printf(String("Name:" + blename + "\n").c_str());
                tft.setCursor(12, 100);
                tft.printf("UUID:1bc68b2a\n");
                drawBLE_beacon(180, 40, TFT_DARKGREY);
            }
            wasConnected = false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    tft.setTextColor(TFT_WHITE);
    pServer->getAdvertising()->stop();
    BLEConnected = false;
}

void ble_test() {
    printf("ble test\n");

    if (!is_ble_inited) {
        printf("Init ble server\n");
        if (!initBLEServer()) {
            displayError("Failed to init BLE server");
            return;
        }
        delay(100);
    }

    disPlayBLESend();

    if (pServer) {
        pServer->getAdvertising()->stop();
    }
    stopBLEStack();

    printf("Quit ble test\n");
}
