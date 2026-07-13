// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). This file is DERIVED FROM and modified
// after the Flipper Zero firmware (KeeLoq cipher + keeloq_common learning
// schemes), Copyright (C) Flipper Devices Inc. and the flipperzero-firmware
// contributors, licensed GPL-3.0-or-later. The encrypted built-in keystore
// mirrors Momentum firmware's keeloq_mfcodes (GPL-3.0-or-later).
// See THIRD_PARTY.md for full attribution.
#include "rf_keeloq.h"
#include "core/sd_functions.h" // getFsStorage
#include "rf_config.h"         // RF_DEBUG, RF_DBG
#include "rf_keeloq_mfcodes_data.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <cstdlib>
#include <cstring>
#include <mbedtls/aes.h>
#include <vector>

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

static std::vector<String> split_string(String str, char c) {
    std::vector<String> cols{};
    size_t start = 0;

    while (start < str.length()) {
        auto it = str.indexOf(c, start);

        if (it == -1) break;

        cols.emplace_back(&str[start], it - start);
        start = it + 1;
    }

    if (start <= str.length() && !str.isEmpty()) cols.emplace_back(&str[start], str.length() - start);

    return cols;
}

static void parse_keystore(const String &content, std::vector<KeeloqKey> &keys) {
    int start = 0;
    const int len = content.length();

    while (start < len) {
        int nl = content.indexOf('\n', start);
        String line = (nl < 0) ? content.substring(start) : content.substring(start, nl);
        start = (nl < 0) ? len : nl + 1;

        line.trim(); // also drops a trailing '\r'
        if (line.isEmpty()) continue;

        auto cols = split_string(line, ';');
        if (cols.size() != 3) continue; // skip malformed lines, keep the rest

        keys.push_back({cols[0], std::strtoull(cols[1].c_str(), NULL, 16), (uint8_t)cols[2].toInt()});
    }
}

static String keeloq_embedded_plaintext() {
    if (KEELOQ_MFCODES_ENC_LEN == 0 || (KEELOQ_MFCODES_ENC_LEN % 16) != 0) return "";

    std::vector<uint8_t> out(KEELOQ_MFCODES_ENC_LEN + 1, 0);
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    if (mbedtls_aes_setkey_dec(&ctx, KEELOQ_MFCODES_KEY, 256) != 0) {
        mbedtls_aes_free(&ctx);
        return "";
    }

    uint8_t iv[16];
    memcpy(iv, KEELOQ_MFCODES_IV, sizeof(iv)); // CBC mutates the IV in place
    int rc = mbedtls_aes_crypt_cbc(
        &ctx, MBEDTLS_AES_DECRYPT, KEELOQ_MFCODES_ENC_LEN, iv, KEELOQ_MFCODES_ENC, out.data()
    );
    mbedtls_aes_free(&ctx);
    if (rc != 0) return "";

    size_t plen = KEELOQ_MFCODES_ENC_LEN;
    uint8_t pad = out[plen - 1];
    if (pad >= 1 && pad <= 16 && pad <= plen) plen -= pad;
    out[plen] = 0;

    return String((const char *)out.data());
}

KeeloqKeystore::KeeloqKeystore(FS *fs) {
    if (fs) {
        File keystore = fs->open("/mfcodes");
        if (keystore) {
            parse_keystore(keystore.readString(), keys);
            keystore.close();
        }
    }
    if (keys.empty()) parse_keystore(keeloq_embedded_plaintext(), keys);
}

const std::vector<KeeloqKey> &KeeloqKeystore::get_keys() { return keys; }

FS *keeloq_mfcodes_fs() {
    FS *fs = nullptr;
    // Active storage first (SD when mounted) — but only if it actually has the
    // keystore; otherwise fall back to a LittleFS copy.
    if (getFsStorage(fs) && fs && fs->exists("/mfcodes")) return fs;
    if (LittleFS.exists("/mfcodes")) return &LittleFS;
    return fs; // may be nullptr
}

uint32_t keeloq_build_hop(const String &mf_name, uint8_t btn, uint32_t serial, uint16_t cnt) {
    const uint32_t b = (uint32_t)btn << 28;

    if (mf_name == "Aprimatic") {
        uint32_t apri_serial = serial;
        uint8_t apr1 = 0;
        for (uint16_t i = 1; i != 0b10000000000; i <<= 1) {
            if (apri_serial & i) apr1++;
        }
        apri_serial &= 0b00001111111111;
        if (apr1 % 2 == 0) { apri_serial |= 0b110000000000; }
        return b | (apri_serial & 0xFFF) << 16 | cnt;
    } else if (
        mf_name == "DTM_Neo" || mf_name == "FAAC_RC,XT" || mf_name == "Mutanco_Mutancode" ||
        mf_name == "Came_Space" || mf_name == "Genius_Bravo" || mf_name == "GSN" || mf_name == "Rosh" ||
        mf_name == "Rossi" || mf_name == "Pecinin" || mf_name == "Peccinin" || mf_name == "Steelmate" ||
        mf_name == "Cardin_S449"
    ) {
        return b | (serial & 0xFFF) << 16 | cnt;
    } else if (mf_name == "NICE_Smilo" || mf_name == "NICE_MHOUSE" || mf_name == "JCM_Tech") {
        return b | (serial & 0xFF) << 16 | cnt;
    } else if (mf_name == "Merlin") {
        return b | (0x000) << 16 | cnt;
    } else if (mf_name == "Centurion") {
        return b | (0x1CE) << 16 | cnt;
    } else if (mf_name == "Monarch") {
        return b | (0x100) << 16 | cnt;
    } else if (mf_name == "Dea_Mio") {
        uint8_t first_disc_num = (serial >> 8) & 0xF;
        uint8_t result_disc = (0xC + (first_disc_num % 4));
        uint32_t dea_serial = (serial & 0xFF) | (((uint32_t)result_disc) << 8);
        return b | (dea_serial & 0xFFF) << 16 | cnt;
    }

    // Default route (Unknown and the plain manufacturers): 10-bit serial.
    return b | (serial & 0x3FF) << 16 | cnt;
}

#if RF_DEBUG
// ---------------------------------------------------------------------------
// KeeLoq golden self-test (`subghz keeloqtest`).
//
// Cipher fidelity is already pinned by code-equivalence to the reference
// (identical NLF 0x3A5C742E, 528 rounds, taps), so this validates the *routes*:
// for every manufacturer family, build the hop, encrypt it (simple AND normal
// learning) exactly as the TX side does, then decrypt it back through the real
// `keeloq_check_decrypt[_centurion]` exactly as `keeloq_identify` does, and
// confirm the button/serial/counter survive the round-trip.
//
// Normal learning derives the manufacturer key from `fix` (button|serial) on
// BOTH sides — matching the reference encoder (keeloq.c). The test also probes
// the legacy `man = normal_learning(hop)` derivation to show it does NOT survive
// the round-trip (the bug this milestone fixes in keeloq_step).
// ---------------------------------------------------------------------------

// An arbitrary manufacturer key (value is irrelevant to a round-trip; we only
// require encrypt/decrypt to use the same derived key).
#define RF_KL_TESTKEY 0x5cec6701b79fd949ULL

static bool kl_check(const String &mf, uint32_t dec, uint8_t btn, uint32_t serial, uint16_t cnt) {
    RfCodes rf;
    rf.mf_name = mf;
    rf.btn = btn;
    rf.serial = serial;
    rf.cnt = 0;
    bool ok = (mf == "Centurion") ? rf.keeloq_check_decrypt_centurion(dec) : rf.keeloq_check_decrypt(dec);
    return ok && (rf.cnt == cnt);
}

bool rf_keeloq_selftest() {
    struct KlCase {
        const char *mf;
        uint32_t serial;
    }; // one per distinct hop route + the user-named families
    static const KlCase cases[] = {
        {"Unknown",    0x4D5E6}, // default 10-bit serial
        {"Rossi",      0x4D5E6}, // 12-bit serial
        {"Pecinin",    0x4D5E6}, // 12-bit serial
        {"Peccinin",   0x4D5E6}, // 12-bit serial
        {"FAAC_RC,XT", 0x4D5E6}, // 12-bit serial
        {"Aprimatic",  0x4D5E6}, // parity-derived serial
        {"NICE_Smilo", 0x4D5E6}, // 8-bit serial
        {"Merlin",     0x4D5E6}, // fixed 0x000
        {"Monarch",    0x4D5E6}, // fixed 0x100
        {"Dea_Mio",    0x4D5E6}, // derived serial
        {"Centurion",  0x4D5E6}, // fixed 0x1CE + dedicated check
    };

    const uint8_t btn = 0x5;
    const uint16_t cnt = 0x1234;
    const uint64_t key = RF_KL_TESTKEY;
    bool allok = true;

    for (const auto &c : cases) {
        String mf = c.mf;
        uint32_t fix = ((uint32_t)btn << 28) | c.serial;
        uint32_t hop = keeloq_build_hop(mf, btn, c.serial, cnt);

        // Simple learning: key used directly on both sides.
        uint32_t enc_s = keeloq_encrypt(hop, key);
        bool simple_ok = kl_check(mf, keeloq_decrypt(enc_s, key), btn, c.serial, cnt);

        // Normal learning, reference-correct: man derived from fix on both sides.
        uint64_t man = keeloq_normal_learning(fix, key);
        uint32_t enc_n = keeloq_encrypt(hop, man);
        bool normal_ok = kl_check(mf, keeloq_decrypt(enc_n, man), btn, c.serial, cnt);

        // Legacy buggy derivation (man from hop on TX): proves why it failed.
        uint64_t man_bug = keeloq_normal_learning(hop, key);
        uint32_t enc_b = keeloq_encrypt(hop, man_bug);
        bool legacy_ok = kl_check(mf, keeloq_decrypt(enc_b, man), btn, c.serial, cnt);

        bool ok = simple_ok && normal_ok;
        allok = allok && ok;
        RF_DBG(
            "keeloq %-12s simple=%s normal=%s (legacy_hop=%s)",
            c.mf,
            simple_ok ? "PASS" : "FAIL",
            normal_ok ? "PASS" : "FAIL",
            legacy_ok ? "PASS" : "FAIL"
        );
    }
    // --- Learning-type round-trip --------------------------------------------
    // For every supported learning type, encrypt the hop with the derived
    // manufacturer key (encode side) and decrypt it back with the same
    // derivation (decode side), exactly as keeloq_step / keeloq_identify do.
    struct LtCase {
        const char *name;
        uint32_t type;
    };
    static const LtCase lts[] = {
        {"Simple",    KEELOQ_SIMPLE_LEARNING            },
        {"Normal",    KEELOQ_NORMAL_LEARNING            },
        {"Secure",    KEELOQ_SECURE_LEARNING            },
        {"MagicXor1", KEELOQ_MAGIC_XOR_TYPE1_LEARNING   },
        {"MagicSer1", KEELOQ_MAGIC_SERIAL_TYPE1_LEARNING},
        {"MagicSer2", KEELOQ_MAGIC_SERIAL_TYPE2_LEARNING},
        {"MagicSer3", KEELOQ_MAGIC_SERIAL_TYPE3_LEARNING},
        {"AERF",      KEELOQ_AERF_LEARNING              },
        {"Erreka",    KEELOQ_ERREKA_LEARNING            },
        {"Pujol",     KEELOQ_PUJOL_LEARNING             },
        {"SimpleJCM", KEELOQ_SIMPLE_JCM_LEARNING        },
    };

    const uint32_t serial = 0x4D5E6;
    const uint32_t fix = ((uint32_t)btn << 28) | serial;
    const uint32_t seed = fix & 0x0FFFFFFF; // serial-derived fallback
    const uint32_t hop = keeloq_build_hop("Unknown", btn, serial, cnt);

    for (const auto &lt : lts) {
        uint64_t man = keeloq_derive_man(lt.type, fix, seed, key);
        uint32_t enc = keeloq_encrypt(hop, man);
        uint32_t dec = keeloq_decrypt(enc, man);
        bool ok = kl_check("Unknown", dec, btn, serial, cnt);
        allok = allok && ok;
        RF_DBG("keeloq learning %-10s round-trip=%s", lt.name, ok ? "PASS" : "FAIL");
    }

    RF_DBG("keeloq selftest result: %s", allok ? "ALL PASS" : "FAILURES");
    return allok;
}

// A learning type whose algorithm we actually implement (round-trip is
// meaningful). FAAC/KingGates/Jarolift live in separate protocols and would
// only trivially "pass" as plain simple learning, so we flag them instead.
static bool kl_type_supported(uint32_t type) {
    switch (type) {
        case KEELOQ_UNKNOWN_LEARNING:
        case KEELOQ_SIMPLE_LEARNING:
        case KEELOQ_NORMAL_LEARNING:
        case KEELOQ_SECURE_LEARNING:
        case KEELOQ_MAGIC_XOR_TYPE1_LEARNING:
        case KEELOQ_MAGIC_SERIAL_TYPE1_LEARNING:
        case KEELOQ_MAGIC_SERIAL_TYPE2_LEARNING:
        case KEELOQ_MAGIC_SERIAL_TYPE3_LEARNING:
        case KEELOQ_ERREKA_LEARNING:
        case KEELOQ_PUJOL_LEARNING:
        case KEELOQ_AERF_LEARNING:
        case KEELOQ_SIMPLE_JCM_LEARNING: return true;
        default: return false; // 5 FAAC, 10 KingGates, 11 Jarolift, ...
    }
}

bool rf_keeloq_filetest() {
    FS *fs = keeloq_mfcodes_fs();
    if (!fs) {
        RF_DBG("keeloq filetest: no storage");
        return false;
    }
    KeeloqKeystore ks{fs};
    const std::vector<KeeloqKey> &keys = ks.get_keys();
    if (keys.empty()) {
        RF_DBG("keeloq filetest: /mfcodes missing or empty");
        return false;
    }

    const uint8_t btn = 0x5;
    const uint16_t cnt = 0x1234;
    const uint32_t serial = 0x4D5E6;
    const uint32_t fix = ((uint32_t)btn << 28) | serial;
    const uint32_t seed = fix & 0x0FFFFFFF;

    int pass = 0, fail = 0, unsup = 0;
    for (const auto &k : keys) {
        if (!kl_type_supported(k.type)) {
            unsup++;
            RF_DBG("keeloq file %-18s type=%2u UNSUPPORTED", k.mf_name.c_str(), (unsigned)k.type);
            continue;
        }
        uint32_t hop = keeloq_build_hop(k.mf_name, btn, serial, cnt);
        uint64_t man = keeloq_derive_man(k.type, fix, seed, k.key);
        uint32_t enc = keeloq_encrypt(hop, man);
        uint32_t dec = keeloq_decrypt(enc, man);
        bool ok = kl_check(k.mf_name, dec, btn, serial, cnt);
        if (ok) pass++;
        else fail++;
        RF_DBG("keeloq file %-18s type=%2u %s", k.mf_name.c_str(), (unsigned)k.type, ok ? "PASS" : "FAIL");
    }
    RF_DBG(
        "keeloq filetest: %d pass, %d fail, %d unsupported (of %u)", pass, fail, unsup, (unsigned)keys.size()
    );
    return fail == 0;
}
#endif // RF_DEBUG
