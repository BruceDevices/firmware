#if !defined(LITE_VERSION)
#include "BatteryService.hpp"
#include "ArduinoJson.h"
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <WiFi.h>
#include <globals.h>

BatteryService::BatteryService() : BruceBLEService() {}

BatteryService::~BatteryService() {}

void battery_handler_task(void *params) {
    NimBLECharacteristic *battery_char = static_cast<NimBLECharacteristic *>(params);
    while (true) {
        uint8_t val = getBattery();
        battery_char->setValue(&val, 1);

        delay(60000); // Update battery every minute
    }
}

void BatteryService::setup(BLEServer *pServer) {

    pService = pServer->createService(BLEUUID((uint16_t)0x180F));

    battery_char = pService->createCharacteristic(
        (uint16_t)0x2A19, // Battery Level
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    const uint8_t batLevel = getBattery();
    battery_char->setValue(&batLevel, 1); // initial value

    pService->start();
    pServer->getAdvertising()->addServiceUUID(pService->getUUID());

    xTaskCreate(
        battery_handler_task,
        "battery_ble_handler",
        2048,
        battery_char,
        tskIDLE_PRIORITY,
        &battery_task_handle
    );
}

void BatteryService::end() { vTaskDelete(battery_task_handle); }
#endif
