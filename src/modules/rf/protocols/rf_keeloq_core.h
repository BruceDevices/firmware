// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#pragma once
#include <stdint.h>
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
#define KEELOQ_FAAC_LEARNING 5 // separate protocol (FAAC SLH) — not handled here
#define KEELOQ_MAGIC_SERIAL_TYPE1_LEARNING 6
#define KEELOQ_MAGIC_SERIAL_TYPE2_LEARNING 7
#define KEELOQ_MAGIC_SERIAL_TYPE3_LEARNING 8
#define KEELOQ_SIMPLE_KINGGATES_LEARNING 10 // separate protocol — not handled here
#define KEELOQ_NORMAL_JAROLIFT_LEARNING 11  // separate protocol — not handled here
#define KEELOQ_ERREKA_LEARNING 12
#define KEELOQ_PUJOL_LEARNING 13
#define KEELOQ_AERF_LEARNING 14
#define KEELOQ_SIMPLE_JCM_LEARNING 15

#ifdef __cplusplus
extern "C" {
#endif
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

#ifdef __cplusplus
}
#endif
