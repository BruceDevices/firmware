/**
 *  iBeacon example
 *
 *  This example demonstrates how to publish an Apple-compatible iBeacon
 *
 *  Created: on May 26 2025
 *      Author: lazd
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEBeacon.h>

// According to Apple, it's important to have a 100ms advertising time
#define BEACON_ADVERTISING_TIME	160   // 100ms

// Hey, you! Replace this with your own unique UUID with something like https://www.uuidgenerator.net/
const char* iBeaconUUID = "26D0814C-F81C-4B2D-AC57-032E2AFF8642";

void setup() {
	NimBLEDevice::init("NimBLEiBeacon");

	// Create beacon object
	NimBLEBeacon beacon;
	beacon.setMajor(1);
	beacon.setMinor(1);
	beacon.setSignalPower(0xC5); // Optional
	beacon.setProximityUUID(BLEUUID(iBeaconUUID)); // Unlike Bluedroid, you do not need to reverse endianness here

	// Create advertisement data
 	NimBLEAdvertisementData beaconAdvertisementData;
	beaconAdvertisementData.setFlags(0x04); // BR_EDR_NOT_SUPPORTED
	beaconAdvertisementData.setManufacturerData(beacon.getData());

	// Start advertising
	NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
	advertising->setAdvertisingInterval(BEACON_ADVERTISING_TIME);
	advertising->setAdvertisementData(beaconAdvertisementData);
	advertising->start();
}

void loop() {}
