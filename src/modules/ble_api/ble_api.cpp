#if !defined(LITE_VERSION)
#include "ble_api.hpp"
#include <NimBLEDevice.h>
#include <core/USBSerial/USBSerial.h>
#include <globals.h>

BLE_API::BLE_API() = default;

class BLEAPICallback : public NimBLEServerCallbacks {
    BLE_API *api;

    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
        pServer->updateConnParams(connInfo.getConnHandle(), 6, 24, 0, 400); // Improve latency
    };

    void onMTUChange(uint16_t MTU, NimBLEConnInfo &connInfo) override { api->update_mtu(MTU); };

public:
    explicit BLEAPICallback(BLE_API *api) : api(api) {}
};

void BLE_API::setup() {
    NimBLEDevice::init("Bruce");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // 9 dBm, tweak if you want

    pServer = NimBLEDevice::createServer();
    pServer->advertiseOnDisconnect(true);
    pServer->setCallbacks(new BLEAPICallback(this));

    battery_service.setup(pServer);
    serial_service.setup(pServer);
    serialDevice = &serial_service;

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    // The 128-bit NUS UUID + the 16-bit battery UUID + the name overflow the
    // 31-byte advertising packet, so use the scan response for the extra data.
    // This keeps the NUS service UUID discoverable, letting the companion app
    // filter on it during scanning.
    pAdvertising->enableScanResponse(true);
    pAdvertising->setName("Bruce");
    pAdvertising->start();
}

void BLE_API::update_mtu(uint16_t mtu) {
    battery_service.setMTU(mtu);
    serial_service.setMTU(mtu);
}

void BLE_API::end() {
    battery_service.end();
    serial_service.end();
    BLEDevice::deinit();
    serialDevice = &USBserial;
}
#endif
