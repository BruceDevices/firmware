#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEUtils.h>

enum EBLEPayloadType { 
    AppleIOS, Microsoft, SamsungWatch, SamsungBuds, 
    SamsungRaw, GoogleFastPair, CustomName, NameFlood 
};

void generateRandomMac(uint8_t *mac);
const char *generateRandomName();
void hexStringToBytes(const char* hexString, uint8_t* output, size_t outputLength);
const char* getRandomBudsId();
uint8_t* createApplePacket(uint8_t deviceType, bool isContinuity = false);
bool setRandomBLEAddress();
BLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type, int specific_index = -1);
void executeSpam(EBLEPayloadType type, int delayMs = 20, int specific_index = -1);
void executeCustomSpam(String spamName, bool isFloodMode = false);
void aj_adv(int ble_choice);

extern BLEAdvertising *pAdvertising;
extern const char* flood_names[];
extern const int FLOOD_NAME_COUNT;
