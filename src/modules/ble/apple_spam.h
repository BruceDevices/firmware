#pragma once

#include <Arduino.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>

struct ApplePayload {
    const char* name;
    const uint8_t* data;
    uint8_t length;
};

void appleSubMenu();
void startAppleSpam(int payloadIndex);
void startAppleSpamAll();
void stopAppleSpam();
void quickAppleSpam(int payloadIndex);
bool isAppleSpamRunning();
const char* getApplePayloadName(int index);
int getApplePayloadCount();
