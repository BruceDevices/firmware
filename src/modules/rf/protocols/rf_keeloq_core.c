// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#include "rf_keeloq_core.h"
uint32_t keeloq_encrypt(const uint32_t data, const uint64_t key) {
    uint32_t x = data, r;

    for (r = 0; r < 528; r++)
        x = (x >> 1) ^ ((bitAt(x, 0) ^ bitAt(x, 16) ^ (uint32_t)bitAt(key, r & 63) ^
                         bitAt(KEELOQ_NLF, g5(x, 1, 9, 20, 26, 31)))
                        << 31);

    return x;
}

uint32_t keeloq_decrypt(const uint32_t data, const uint64_t key) {
    uint32_t x = data, r;

    for (r = 0; r < 528; r++)
        x = (x << 1) ^ bitAt(x, 31) ^ bitAt(x, 15) ^ (uint32_t)bitAt(key, (15 - r) & 63) ^
            bitAt(KEELOQ_NLF, g5(x, 0, 8, 19, 25, 30));

    return x;
}

uint64_t keeloq_normal_learning(uint32_t data, const uint64_t key) {
    uint32_t k1, k2;

    data &= 0x0FFFFFFF;
    data |= 0x20000000;
    k1 = keeloq_decrypt(data, key);

    data &= 0x0FFFFFFF;
    data |= 0x60000000;
    k2 = keeloq_decrypt(data, key);

    return ((uint64_t)k2 << 32) | k1;
}

// --- Manufacturer-specific learning schemes (ported from keeloq_common) ------

uint64_t keeloq_secure_learning(uint32_t data, uint32_t seed, const uint64_t key) {
    data &= 0x0FFFFFFF;
    uint32_t k1 = keeloq_decrypt(data, key);
    uint32_t k2 = keeloq_decrypt(seed, key);
    return ((uint64_t)k1 << 32) | k2;
}

uint64_t keeloq_magic_xor_type1_learning(uint32_t data, uint64_t xorv) {
    data &= 0x0FFFFFFF;
    return (((uint64_t)data << 32) | data) ^ xorv;
}

uint64_t keeloq_magic_serial_type1_learning(uint32_t data, uint64_t man) {
    return (man & 0xFFFFFFFF) | ((uint64_t)data << 40) |
           ((uint64_t)(((data & 0xff) + ((data >> 8) & 0xFF)) & 0xFF) << 32);
}

uint64_t keeloq_magic_serial_type2_learning(uint32_t data, uint64_t man) {
    uint8_t *p = (uint8_t *)&data;
    uint8_t *m = (uint8_t *)&man;
    m[7] = p[0];
    m[6] = p[1];
    m[5] = p[2];
    m[4] = p[3];
    return man;
}

uint64_t keeloq_magic_serial_type3_learning(uint32_t data, uint64_t man) {
    return (man & 0xFFFFFFFFFF000000) | (data & 0xFFFFFF);
}

uint64_t keeloq_learning_aerf(uint32_t data, const uint64_t key) {
    uint32_t d = data & 0x0FFFFFFFu;
    uint32_t k1 = keeloq_decrypt(d | 0x20000000u, key);
    uint32_t k2 = keeloq_decrypt(d | 0x60000000u, key);
    return ((uint64_t)k2 << 32) | k1;
}

uint64_t keeloq_learning_erreka(uint32_t data, uint32_t mix, const uint64_t key) {
    uint32_t d = data & 0x0FFFFFFFu;
    uint32_t k1 = keeloq_decrypt(d | 0x20000000u, key);
    uint32_t r4 = mix >> 4;
    uint32_t r1 = (mix << 4) & 0xF000F000u;
    r4 = (r4 & 0x0F000F00u) | r1;
    uint32_t r5 = mix & 0x00FF00FFu;
    uint32_t x = r4 | r5;
    x |= 0x60000000u;
    uint32_t k2 = keeloq_decrypt(x, key);
    return ((uint64_t)k2 << 32) | k1;
}

uint64_t keeloq_learning_pujol(uint32_t data, const uint64_t key) {
    uint32_t d = data & 0x0FFFFFFFu;
    uint32_t w1 = keeloq_decrypt(d | 0x20000000u, key);
    uint32_t w2 = keeloq_decrypt(d | 0x60000000u, key);
    uint32_t k1 = (w1 >> 16) | (w1 << 16);
    uint32_t k2 = (w2 >> 16) | (w2 << 16);
    return ((uint64_t)k2 << 32) | k1;
}

uint64_t keeloq_derive_man(uint32_t type, uint32_t fix, uint32_t seed, uint64_t key) {
    switch (type) {
        case KEELOQ_NORMAL_LEARNING: return keeloq_normal_learning(fix, key);
        case KEELOQ_SECURE_LEARNING: return keeloq_secure_learning(fix, seed, key);
        case KEELOQ_MAGIC_XOR_TYPE1_LEARNING: return keeloq_magic_xor_type1_learning(fix, key);
        case KEELOQ_MAGIC_SERIAL_TYPE1_LEARNING: return keeloq_magic_serial_type1_learning(fix, key);
        case KEELOQ_MAGIC_SERIAL_TYPE2_LEARNING: return keeloq_magic_serial_type2_learning(fix, key);
        case KEELOQ_MAGIC_SERIAL_TYPE3_LEARNING: return keeloq_magic_serial_type3_learning(fix, key);
        case KEELOQ_AERF_LEARNING: return keeloq_learning_aerf(fix, key);
        case KEELOQ_ERREKA_LEARNING: return keeloq_learning_erreka(fix, seed, key);
        case KEELOQ_PUJOL_LEARNING: return keeloq_learning_pujol(fix, key);
        // SIMPLE, SIMPLE_JCM, UNKNOWN and any unhandled type encrypt with the
        // manufacturer key directly.
        default: return key;
    }
}
