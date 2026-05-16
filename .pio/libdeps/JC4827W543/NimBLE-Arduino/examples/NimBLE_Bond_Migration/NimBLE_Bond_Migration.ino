/**
 * NimBLE_Bond_Migration Demo:
 *
 * This example demonstrates running one-time bond-store migration before
 * NimBLE initialization. Set NIMBLE_BOND_MIGRATION_MODE to select direction:
 * 0 = off, 1 = migrate to current, 2 = migrate to v1.
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEBondMigration.h>

#ifndef NIMBLE_BOND_MIGRATION_MODE
#define NIMBLE_BOND_MIGRATION_MODE 1
#endif

#if (NIMBLE_BOND_MIGRATION_MODE < 0) || (NIMBLE_BOND_MIGRATION_MODE > 2)
#error "NIMBLE_BOND_MIGRATION_MODE must be 0 (off), 1 (to current), or 2 (to v1)"
#endif

void setup() {
    using namespace NimBLEBondMigration;

    Serial.begin(115200);
    Serial.println("Starting NimBLE_Bond_Migration Demo");

    bool success = false;
#if NIMBLE_BOND_MIGRATION_MODE == 1
    success = migrateBondStoreToCurrent();
    Serial.printf("NimBLE_Bond_Migration to current %s\n", success ? "success" : "failed");
#elif NIMBLE_BOND_MIGRATION_MODE == 2
    success = migrateBondStoreToV1();
    Serial.printf("NimBLE_Bond_Migration to v1 %s\n", success ? "success" : "failed");
#endif

    if (success) {
        Serial.println("NimBLE_Bond_Migration completed, upload target firmware to continue");
        while (true) {
            delay(1000);
        }
    }

    NimBLEDevice::init("NimBLE");
    NimBLEDevice::setSecurityAuth(true, true, false); /** bonding, MITM, don't need BLE secure connections as we are using passkey pairing */
    NimBLEDevice::setSecurityPasskey(123456);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); /** Display only passkey */
    NimBLEServer*         pServer                  = NimBLEDevice::createServer();
    NimBLEService*        pService                 = pServer->createService("ABCD");
    NimBLECharacteristic* pSecureCharacteristic =
        pService->createCharacteristic("1235",
                                       NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);

    pSecureCharacteristic->setValue("Hello Secure BLE");

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("ABCD");
    pAdvertising->start();
}

void loop() {}
