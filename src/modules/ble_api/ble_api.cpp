#if !defined(LITE_VERSION)
#include "ble_api.hpp"
#include <NimBLEDevice.h>
#include "modules/ble/ble_p4_compat.h"
#include <core/USBSerial/USBSerial.h>
#include <globals.h>

BLE_API::BLE_API() = default;

class BLEAPICallback : public NimBLEServerCallbacks {
    BLE_API *api;

    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
        // 12..24 units = 15..30 ms. Apple rejects the whole request if the minimum
        // is under 15 ms or if max < min + 15 ms, and a rejected request left us on
        // whatever interval iOS picked by itself.
        pServer->updateConnParams(connInfo.getConnHandle(), 12, 24, 0, 400);
    };

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override {
        api->on_disconnect();
    };

    void onMTUChange(uint16_t MTU, NimBLEConnInfo &connInfo) override { api->update_mtu(MTU); };

public:
    explicit BLEAPICallback(BLE_API *api) : api(api) {}
};

void BLE_API::setup() {
    NimBLEDevice::init("Bruce");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // 9 dBm, tweak if you want

    // Ask for the largest standard ATT MTU. The central still decides the final
    // value (iOS settles around 185); without this we advertised the NimBLE
    // default and every notification was capped much lower than it had to be.
    NimBLEDevice::setMTU(517);

#if !defined(CONFIG_IDF_TARGET_ESP32)
    // LE 2M PHY roughly doubles the raw throughput. The original ESP32 is BLE 4.2
    // and has no 2M radio, so only offer it where the controller supports it; the
    // peer keeps 1M if it does not.
    NimBLEDevice::setDefaultPhy(
        BLE_GAP_LE_PHY_1M_MASK | BLE_GAP_LE_PHY_2M_MASK,
        BLE_GAP_LE_PHY_1M_MASK | BLE_GAP_LE_PHY_2M_MASK
    );
#endif

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

void BLE_API::on_disconnect() { serial_service.onDisconnected(); }

void BLE_API::end() {
    battery_service.end();
    serial_service.end();
    BLEDevice::deinit();
    serialDevice = &USBserial;
}
#endif
