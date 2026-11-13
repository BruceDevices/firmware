// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). Car-model database and custom radio
// profiles, ported from ProtoPirate (GPL-3.0-or-later), (C) RocketGod and the
// ProtoPirate contributors. See PROTOPIRATE_LICENSE and THIRD_PARTY.md.
//
// The "models database" is a small named list of receiver configurations:
// each entry binds a human name to a frequency and a radio preset. Entries
// using a *custom* preset carry a raw CC1101 register dump (Flipper
// "CustomPresetData" format, registers 0x00..0x2D) instead of a named preset.
#pragma once

#include <stdint.h>

struct CarModel {
    const char *name;
    uint32_t freqHz;
    const char *preset;          // Bruce preset name, or "Custom"
    const uint8_t *registers;    // raw CC1101 registers (nullptr unless Custom)
    uint8_t registerCount;
};

// Total number of models in the database.
int car_model_count();

// Model at index (0..count-1). Returns nullptr for out-of-range.
const CarModel *car_model_at(int index);

// Apply a model's frequency + preset (or raw registers) to the CC1101.
// The radio must already be initialised (initRfModule). Returns false on
// failure.
bool car_model_apply(const CarModel *model);
