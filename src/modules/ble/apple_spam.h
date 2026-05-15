#pragma once
#if !defined(LITE_VERSION)
#include <Arduino.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>

bool buildAppleSpamAdvertisement(int payloadIndex, BLEAdvertisementData &advertisementData);

struct ApplePayload {
    const char* name;
    const uint8_t* data;
    uint8_t length;
};

void appleSubMenu();
void applePairingSubMenu();
void appleActionSubMenu();
void startAppleSpam(int payloadIndex);
void startAppleSpamAll();
void stopAppleSpam();
void quickAppleSpam(int payloadIndex);
bool isAppleSpamRunning();
const char* getApplePayloadName(int index);
int getApplePayloadCount();
#endif
