#pragma once

#include "rf_protocol.h"

// Radio preset registry. Single place where the `.sub` "Preset:" names map
// to concrete transceiver parameters.

// Resolve a preset by its canonical/alias name. Returns nullptr if unknown
// (caller then tries the numeric legacy-protocol path).
const RfPreset *rf_find_preset(const String &name);
