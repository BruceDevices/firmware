// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). This file is DERIVED FROM and modified
// after the Flipper Zero firmware (KeeLoq cipher + keeloq_common learning
// schemes + SubGhz keystore format), Copyright (C) Flipper Devices Inc. and
// the flipperzero-firmware contributors, licensed GPL-3.0-or-later. The
// encrypted built-in keystore mirrors Momentum firmware's keeloq_mfcodes
// (GPL-3.0-or-later). See THIRD_PARTY.md for full attribution.
#pragma once

#include "../structs.h" // KeeloqKey
#include <FS.h>
#include <stdint.h>
#include <vector>

// KeeLoq block cipher + manufacturer keystore. Centralized here so the
// rolling-code primitives live under protocols/ alongside the static protocol
// tables. The per-frame framing (button/serial/counter -> hop, manufacturer
// quirks) stays on the RfCodes struct in rf_utils, since it operates on a
// captured code; only the cipher core and the `/mfcodes` keystore reader moved.
//
// References / credits — the cipher core, the manufacturer "learning" schemes
// and the keystore format are ports of the Flipper Zero ecosystem. The
// encrypted built-in keystore (decrypted at runtime as a fallback) mirrors how
// Momentum ships its `keeloq_mfcodes` asset:
//   - Flipper Zero firmware (KeeLoq + keeloq_common, SubGhz keystore):
//       https://github.com/flipperdevices/flipperzero-firmware
//       (lib/subghz/protocols/keeloq.c, lib/subghz/blocks/*)
//   - Momentum firmware (extended manufacturer list / encrypted keystore):
//       https://github.com/Next-Flip/Momentum-Firmware
//       (applications/main/subghz/.../assets/keeloq_mfcodes)
// The learning-type ids below match that keystore's `type` column so a
// `/mfcodes` exported from there works verbatim.

#include "rf_keeloq_core.h"
class KeeloqKeystore {
public:
    KeeloqKeystore(FS *fs);

    const std::vector<KeeloqKey> &get_keys();

private:
    std::vector<KeeloqKey> keys{};
};

// Pick the filesystem that holds `/mfcodes`: the active storage (SD when
// mounted) if it has the file, otherwise LittleFS (so a keystore written to
// LittleFS works even with an SD card inserted). Returns nullptr if neither has
// it and no storage is available.
FS *keeloq_mfcodes_fs();

// Build the KeeLoq "hop" plaintext (button | manufacturer-masked serial |
// counter) for a manufacturer. Pure function so every framing route is testable
// without the SD keystore. Centurion/Merlin/Monarch use a fixed serial field;
// Aprimatic/Dea_Mio derive it from the serial; the rest mask it. Mirrors the
// branch table consumed by RfCodes::keeloq_step.
uint32_t keeloq_build_hop(const String &mf_name, uint8_t btn, uint32_t serial, uint16_t cnt);

// Golden self-test of the KeeLoq routes: for each manufacturer family encrypt a
// frame and decrypt it back through the real check, for both learning types, and
// round-trip every supported learning type. Diagnostic; defined under RF_DEBUG.
bool rf_keeloq_selftest();

// Same round-trip, but driven by the real `/mfcodes` keystore: every entry is
// encoded and decoded with its actual key + learning type. Diagnostic; defined
// under RF_DEBUG.
bool rf_keeloq_filetest();
