#pragma once

#include <stdint.h>

struct AppleProximityDevice {
    const char *name;
    uint16_t device_id;
    uint8_t new_device_prefix;
};

struct BleSpamAppleDevice {
    const char *ui_name;
    uint8_t action_code;
};

struct WatchModel {
    const char *name;
    uint8_t value;
};

struct DeviceType {
    const char *name;
    uint32_t value;
};

struct BudsModel {
    const char *name;
    uint32_t value;
};

extern const AppleProximityDevice APPLE_PROXIMITY_DEVICES[];
extern const int APPLE_PROXIMITY_DEVICE_COUNT;

extern const BleSpamAppleDevice BLE_SPAM_APPLE_ACTION_DEVICES[];
extern const int BLE_SPAM_APPLE_ACTION_DEVICE_COUNT;

extern const WatchModel watch_models[];
extern const int watch_models_count;

extern const BudsModel samsung_buds_models[];
extern const int samsung_buds_count;

extern const DeviceType android_models[];
extern const int android_models_count;

extern const char *const BLE_SPAM_WINDOWS_PRESETS[];
extern const int BLE_SPAM_WINDOWS_PRESET_COUNT;

extern const char *const BLE_SPAM_BEACON_PRESETS[];
extern const int BLE_SPAM_BEACON_PRESET_COUNT;
