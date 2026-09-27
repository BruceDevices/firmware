#pragma once
#include <stdint.h>

// Hardware profiles for Cardputer-Adv — selectable via Config > System > Advanced
// Stored in NVS (namespace "bruce_hw", key "profile") so the choice survives reboots.
// applyHWProfile() is called from _post_setup_gpio() and becomes the single source of
// truth for pin assignments, which also bypasses the persistence bug in #2864.

enum HWProfile : uint8_t {
    HW_CAP_CC1101 = 0, // M5-U219: CC1101 + NFC (ST25R3916, unsupported by Bruce)
    HW_CAP_LORA   = 1, // Cap LoRa SX1262 (beta – pins not yet validated)
    HW_STOCK      = 2, // Standard Cardputer-Adv without a Cap RF module
    HW_GROVE_GPS  = 3, // GPS Unit v1.1 (MAX2659) on Grove port G1/G2
    HW_PROFILE_COUNT
};

const char *hwProfileName(uint8_t prof);

// NVS load/save (namespace "bruce_hw", key "profile")
uint8_t loadHWProfile();
void    saveHWProfile(uint8_t prof);

// Apply pin assignments for the given profile to bruceConfigPins.
// Called at boot from _post_setup_gpio() and immediately from the menu when the
// user picks a new profile (so it takes effect without a reboot).
void applyHWProfile(uint8_t prof);
