#pragma once
#if !defined(LITE_VERSION)
#include <Arduino.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>

struct ApplePayload {
    const char *name;
    const uint8_t *data;
    uint8_t length;
};

// ── Main Menu ──────────────────────────────────────────────────
void appleSubMenu();

// ── Standard Spam Functions ──────────────────────────────────
void startAppleSpam(int payloadIndex);
void startAppleSpamAll();
void stopAppleSpam();
void quickAppleSpam(int payloadIndex);
bool isAppleSpamRunning();

// ── Enhanced Spam with iCloud Spoofing ──────────────────────
/// Start spam with optional iCloud binding spoofing
/// @param payloadIndex - Index of the Apple payload to use (0-21)
/// @param useICloudSpoof - If true, uses dynamic iCloud-bound packets
///                         (better for iOS 17+ devices)
void startAppleSpamEnhanced(int payloadIndex, bool useICloudSpoof = true);

// ── Payload Information ──────────────────────────────────────
const char *getApplePayloadName(int index);
int getApplePayloadCount();

// ── Advertisement Builder ────────────────────────────────────
bool buildAppleSpamAdvertisement(int payloadIndex, BLEAdvertisementData &advertisementData);

#endif
