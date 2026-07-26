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

#define bitAt(x, n) (((x) >> (n)) & 1)
#define g5(x, a, b, c, d, e)                                                                                 \
    (bitAt(x, a) + bitAt(x, b) * 2 + bitAt(x, c) * 4 + bitAt(x, d) * 8 + bitAt(x, e) * 16)

#define KEELOQ_NLF 0x3A5C742E

// Learning-type ids. Values match the Flipper/SubGhz keystore `type` column so a
// `/mfcodes` exported from there can be used verbatim. SIMPLE/NORMAL are the
// historical ones; the rest were ported from the reference keeloq_common.
#define KEELOQ_UNKNOWN_LEARNING 0
#define KEELOQ_SIMPLE_LEARNING 1
#define KEELOQ_NORMAL_LEARNING 2
#define KEELOQ_SECURE_LEARNING 3
#define KEELOQ_MAGIC_XOR_TYPE1_LEARNING 4
#define KEELOQ_FAAC_LEARNING 5             // separate protocol (FAAC SLH) — not handled here
#define KEELOQ_MAGIC_SERIAL_TYPE1_LEARNING 6
#define KEELOQ_MAGIC_SERIAL_TYPE2_LEARNING 7
#define KEELOQ_MAGIC_SERIAL_TYPE3_LEARNING 8
#define KEELOQ_SIMPLE_KINGGATES_LEARNING 10 // separate protocol — not handled here
#define KEELOQ_NORMAL_JAROLIFT_LEARNING 11  // separate protocol — not handled here
#define KEELOQ_ERREKA_LEARNING 12
#define KEELOQ_PUJOL_LEARNING 13
#define KEELOQ_AERF_LEARNING 14
#define KEELOQ_SIMPLE_JCM_LEARNING 15

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

uint32_t keeloq_encrypt(const uint32_t data, const uint64_t key);
uint32_t keeloq_decrypt(const uint32_t data, const uint64_t key);
uint64_t keeloq_normal_learning(uint32_t data, const uint64_t key);

// Manufacturer-specific key-derivation schemes ("learning"), ported from the
// reference keeloq_common. Each turns the fixed part (and sometimes a seed) into
// the 64-bit manufacturer key used to encrypt/decrypt the hopping code.
uint64_t keeloq_secure_learning(uint32_t data, uint32_t seed, const uint64_t key);
uint64_t keeloq_magic_xor_type1_learning(uint32_t data, uint64_t xorv);
uint64_t keeloq_magic_serial_type1_learning(uint32_t data, uint64_t man);
uint64_t keeloq_magic_serial_type2_learning(uint32_t data, uint64_t man);
uint64_t keeloq_magic_serial_type3_learning(uint32_t data, uint64_t man);
uint64_t keeloq_learning_aerf(uint32_t data, const uint64_t key);
uint64_t keeloq_learning_erreka(uint32_t data, uint32_t mix, const uint64_t key);
uint64_t keeloq_learning_pujol(uint32_t data, const uint64_t key);

// Single dispatch used by both the encoder (keeloq_step) and the decoder
// (keeloq_identify): derive the manufacturer key for `type`. `fix` is
// button|serial; `seed` is only consulted by secure/erreka. Unknown/simple
// learning returns `key` unchanged.
uint64_t keeloq_derive_man(uint32_t type, uint32_t fix, uint32_t seed, uint64_t key);

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
