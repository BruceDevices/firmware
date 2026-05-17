#pragma once

#include <Arduino.h>
void aj_adv(int ble_choice);
void spamMenu();
void legacySubMenu();

void ibeacon(
    const char *DeviceName = "Bruce iBeacon", const char *BEACON_UUID = "8ec76ea3-6668-48da-9866-75be8bc86f4d",
    int ManufacturerId = 0x4C00
);

// Public API for BLE spam (used by serial CLI & BLE terminal)
void bleSpamStart(int protocol);
void bleSpamStop(void);
