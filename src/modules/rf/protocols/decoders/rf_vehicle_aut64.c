// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "rf_vehicle_support.h"

#include <string.h>

// https://www.usenix.org/system/files/conference/usenixsecurity16/sec16_paper_garcia.pdf

/*
 * AUT64 algorithm, 12 rounds
 * 8 bytes block size, 8 bytes key size
 * 8 bytes pbox size, 16 bytes sbox size
 *
 * Based on: Reference AUT64 implementation in JavaScript (aut64.js)
 * Vencislav Atanasov, 2025-09-13
 *
 * Based on: Reference AUT64 implementation in python
 * C Hicks, hkscy.org, 03-01-19
 */

static const uint8_t table_ln[AUT64_NUM_ROUNDS][AUT64_BLOCK_SIZE] = {
    {0x4, 0x5, 0x6, 0x7, 0x0, 0x1, 0x2, 0x3}, // Round 0
    {0x5, 0x4, 0x7, 0x6, 0x1, 0x0, 0x3, 0x2}, // Round 1
    {0x6, 0x7, 0x4, 0x5, 0x2, 0x3, 0x0, 0x1}, // Round 2
    {0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0}, // Round 3
    {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7}, // Round 4
    {0x1, 0x0, 0x3, 0x2, 0x5, 0x4, 0x7, 0x6}, // Round 5
    {0x2, 0x3, 0x0, 0x1, 0x6, 0x7, 0x4, 0x5}, // Round 6
    {0x3, 0x2, 0x1, 0x0, 0x7, 0x6, 0x5, 0x4}, // Round 7
    {0x5, 0x4, 0x7, 0x6, 0x1, 0x0, 0x3, 0x2}, // Round 8
    {0x4, 0x5, 0x6, 0x7, 0x0, 0x1, 0x2, 0x3}, // Round 9
    {0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0}, // Round 10
    {0x6, 0x7, 0x4, 0x5, 0x2, 0x3, 0x0, 0x1}, // Round 11
};

static const uint8_t table_un[AUT64_NUM_ROUNDS][AUT64_BLOCK_SIZE] = {
    {0x1, 0x0, 0x3, 0x2, 0x5, 0x4, 0x7, 0x6}, // Round 0
    {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7}, // Round 1
    {0x3, 0x2, 0x1, 0x0, 0x7, 0x6, 0x5, 0x4}, // Round 2
    {0x2, 0x3, 0x0, 0x1, 0x6, 0x7, 0x4, 0x5}, // Round 3
    {0x5, 0x4, 0x7, 0x6, 0x1, 0x0, 0x3, 0x2}, // Round 4
    {0x4, 0x5, 0x6, 0x7, 0x0, 0x1, 0x2, 0x3}, // Round 5
    {0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0}, // Round 6
    {0x6, 0x7, 0x4, 0x5, 0x2, 0x3, 0x0, 0x1}, // Round 7
    {0x3, 0x2, 0x1, 0x0, 0x7, 0x6, 0x5, 0x4}, // Round 8
    {0x2, 0x3, 0x0, 0x1, 0x6, 0x7, 0x4, 0x5}, // Round 9
    {0x1, 0x0, 0x3, 0x2, 0x5, 0x4, 0x7, 0x6}, // Round 10
    {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7}, // Round 11
};

static const uint8_t table_offset[AUT64_OFFSET_TABLE_SIZE] = {
    // 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, // 1
    0x0, 0x2, 0x4, 0x6, 0x8, 0xA, 0xC, 0xE, 0x3, 0x1, 0x7, 0x5, 0xB, 0x9, 0xF, 0xD, // 2
    0x0, 0x3, 0x6, 0x5, 0xC, 0xF, 0xA, 0x9, 0xB, 0x8, 0xD, 0xE, 0x7, 0x4, 0x1, 0x2, // 3
    0x0, 0x4, 0x8, 0xC, 0x3, 0x7, 0xB, 0xF, 0x6, 0x2, 0xE, 0xA, 0x5, 0x1, 0xD, 0x9, // 4
    0x0, 0x5, 0xA, 0xF, 0x7, 0x2, 0xD, 0x8, 0xE, 0xB, 0x4, 0x1, 0x9, 0xC, 0x3, 0x6, // 5
    0x0, 0x6, 0xC, 0xA, 0xB, 0xD, 0x7, 0x1, 0x5, 0x3, 0x9, 0xF, 0xE, 0x8, 0x2, 0x4, // 6
    0x0, 0x7, 0xE, 0x9, 0xF, 0x8, 0x1, 0x6, 0xD, 0xA, 0x3, 0x4, 0x2, 0x5, 0xC, 0xB, // 7
    0x0, 0x8, 0x3, 0xB, 0x6, 0xE, 0x5, 0xD, 0xC, 0x4, 0xF, 0x7, 0xA, 0x2, 0x9, 0x1, // 8
    0x0, 0x9, 0x1, 0x8, 0x2, 0xB, 0x3, 0xA, 0x4, 0xD, 0x5, 0xC, 0x6, 0xF, 0x7, 0xE, // 9
    0x0, 0xA, 0x7, 0xD, 0xE, 0x4, 0x9, 0x3, 0xF, 0x5, 0x8, 0x2, 0x1, 0xB, 0x6, 0xC, // A
    0x0, 0xB, 0x5, 0xE, 0xA, 0x1, 0xF, 0x4, 0x7, 0xC, 0x2, 0x9, 0xD, 0x6, 0x8, 0x3, // B
    0x0, 0xC, 0xB, 0x7, 0x5, 0x9, 0xE, 0x2, 0xA, 0x6, 0x1, 0xD, 0xF, 0x3, 0x4, 0x8, // C
    0x0, 0xD, 0x9, 0x4, 0x1, 0xC, 0x8, 0x5, 0x2, 0xF, 0xB, 0x6, 0x3, 0xE, 0xA, 0x7, // D
    0x0, 0xE, 0xF, 0x1, 0xD, 0x3, 0x2, 0xC, 0x9, 0x7, 0x6, 0x8, 0x4, 0xA, 0xB, 0x5, // E
    0x0, 0xF, 0xD, 0x2, 0x9, 0x6, 0x4, 0xB, 0x1, 0xE, 0xC, 0x3, 0x8, 0x7, 0x5, 0xA // F
};

static const uint8_t table_sub[AUT64_SBOX_SIZE] = {
    0x0,
    0x1,
    0x9,
    0xE,
    0xD,
    0xB,
    0x7,
    0x6,
    0xF,
    0x2,
    0xC,
    0x5,
    0xA,
    0x4,
    0x3,
    0x8,
};

// Build an inverse/permutation table.
// Sentinel 0xFF is used to detect missing entries and duplicates.
// Returns AUT64_OK on success, otherwise AUT64_ERR_INVALID_KEY.
static int reverse_box(uint8_t* reversed, const uint8_t* box, size_t len) {
    size_t i;

    for(i = 0; i < len; i++) {
        reversed[i] = 0xFF;
    }

    for(i = 0; i < len; i++) {
        const uint8_t v = box[i];
#ifdef AUT64_ENABLE_VALIDATIONS
        if(v >= len) {
            return AUT64_ERR_INVALID_KEY;
        }
        if(reversed[v] != 0xFF) {
            // Duplicate value means it is not a permutation.
            return AUT64_ERR_INVALID_KEY;
        }
#endif
        reversed[v] = (uint8_t)i;
    }

#ifdef AUT64_ENABLE_VALIDATIONS
    for(i = 0; i < len; i++) {
        if(reversed[i] == 0xFF) {
            // Missing mapping.
            return AUT64_ERR_INVALID_KEY;
        }
    }
#endif

    return AUT64_OK;
}

#ifdef AUT64_ENABLE_VALIDATIONS

// Validate that 'box' is a permutation of 0..len-1.
// Uses 0xFF sentinel logic to detect duplicates/missing values.
static int validate_box_is_permutation(const uint8_t* box, size_t len) {
    uint8_t inv[32]; // enough for pbox (8) and sbox (16)
    size_t i;

    if(len > sizeof(inv)) {
        return AUT64_ERR_INVALID_KEY;
    }

    for(i = 0; i < len; i++) {
        inv[i] = 0xFF;
    }

    for(i = 0; i < len; i++) {
        const uint8_t v = box[i];
        if(v >= len) {
            return AUT64_ERR_INVALID_KEY;
        }
        if(inv[v] != 0xFF) {
            return AUT64_ERR_INVALID_KEY;
        }
        inv[v] = (uint8_t)i;
    }

    for(i = 0; i < len; i++) {
        if(inv[i] == 0xFF) {
            return AUT64_ERR_INVALID_KEY;
        }
    }

    return AUT64_OK;
}

// Validate that a key is structurally correct:
// - key nibbles are in range 0..15
// - pbox is a permutation of 0..7
// - sbox is a permutation of 0..15
// return AUT64_OK or AUT64_ERR_INVALID_KEY/AUT64_ERR_NULL_POINTER
int aut64_validate_key(const struct aut64_key* key) {
    uint8_t i;
    int rc;

    if(!key) {
        return AUT64_ERR_NULL_POINTER;
    }

    // key->key[] is treated as nibbles in multiple places (table_sub indexing, offset building).
    for(i = 0; i < AUT64_KEY_SIZE; i++) {
        if(key->key[i] >= AUT64_SBOX_SIZE) {
            return AUT64_ERR_INVALID_KEY;
        }
    }

    rc = validate_box_is_permutation(key->pbox, AUT64_PBOX_SIZE);
    if(rc != AUT64_OK) {
        return rc;
    }

    rc = validate_box_is_permutation(key->sbox, AUT64_SBOX_SIZE);
    if(rc != AUT64_OK) {
        return rc;
    }

    return AUT64_OK;
}

#endif // AUT64_ENABLE_VALIDATIONS

// Compute one 4-bit contribution to the round key
static uint8_t key_nibble(
    const struct aut64_key* key,
    uint8_t nibble,
    const uint8_t table[AUT64_BLOCK_SIZE],
    uint8_t iteration) {
    const uint8_t keyValue = key->key[table[iteration]];
    const uint8_t offset = (uint8_t)((keyValue << 4) | nibble);
    return table_offset[offset];
}

// Compute the round compression byte derived from the current state and the key for a given round.
static uint8_t round_key(const struct aut64_key* key, const uint8_t* state, uint8_t roundN) {
    uint8_t result_hi = 0, result_lo = 0;

    for(int i = 0; i < AUT64_BLOCK_SIZE - 1; i++) {
        result_hi ^= key_nibble(key, (uint8_t)(state[i] >> 4), table_un[roundN], (uint8_t)i);
        result_lo ^= key_nibble(key, (uint8_t)(state[i] & 0x0F), table_ln[roundN], (uint8_t)i);
    }

    return (uint8_t)((result_hi << 4) | result_lo);
}

// Compute the transformed key nibble used as an offset for final-byte processing in a round.
static uint8_t
    final_byte_nibble(const struct aut64_key* key, const uint8_t table[AUT64_BLOCK_SIZE]) {
    const uint8_t keyValue = key->key[table[AUT64_BLOCK_SIZE - 1]];
    return (uint8_t)(table_sub[keyValue] << 4);
}

// Compute the inverse lookup for a final-byte nibble during encryption.
static uint8_t encrypt_final_byte_nibble(
    const struct aut64_key* key,
    uint8_t nibble,
    const uint8_t table[AUT64_BLOCK_SIZE]) {
    const uint8_t offset = final_byte_nibble(key, table);

    for(int i = 0; i < 16; i++) {
        if(table_offset[(uint8_t)(offset + i)] == nibble) {
            return (uint8_t)i;
        }
    }
    // Should never happen for valid inputs; return 0 as a defined value.
    return 0;
}

// Perform the compression step for one encryption round, producing the new last byte.
static uint8_t
    encrypt_compress(const struct aut64_key* key, const uint8_t* state, uint8_t roundN) {
    const uint8_t roundKey = round_key(key, state, roundN);
    uint8_t result_hi = (uint8_t)(roundKey >> 4), result_lo = (uint8_t)(roundKey & 0x0F);

    result_hi ^= encrypt_final_byte_nibble(
        key, (uint8_t)(state[AUT64_BLOCK_SIZE - 1] >> 4), table_un[roundN]);
    result_lo ^= encrypt_final_byte_nibble(
        key, (uint8_t)(state[AUT64_BLOCK_SIZE - 1] & 0x0F), table_ln[roundN]);

    return (uint8_t)((result_hi << 4) | result_lo);
}

// Reverse the final-byte nibble transformation during decryption.
static uint8_t decrypt_final_byte_nibble(
    const struct aut64_key* key,
    uint8_t nibble,
    const uint8_t table[AUT64_BLOCK_SIZE],
    uint8_t result) {
    const uint8_t offset = final_byte_nibble(key, table);
    return table_offset[(uint8_t)((result ^ nibble) + offset)];
}

// Perform the compression step for one decryption round, restoring the previous last byte.
static uint8_t
    decrypt_compress(const struct aut64_key* key, const uint8_t* state, uint8_t roundN) {
    const uint8_t roundKey = round_key(key, state, roundN);
    uint8_t result_hi = (uint8_t)(roundKey >> 4), result_lo = (uint8_t)(roundKey & 0x0F);

    result_hi = decrypt_final_byte_nibble(
        key, (uint8_t)(state[AUT64_BLOCK_SIZE - 1] >> 4), table_un[roundN], result_hi);
    result_lo = decrypt_final_byte_nibble(
        key, (uint8_t)(state[AUT64_BLOCK_SIZE - 1] & 0x0F), table_ln[roundN], result_lo);

    return (uint8_t)((result_hi << 4) | result_lo);
}

// Apply the S-box substitution to a single byte.
static uint8_t substitute(const struct aut64_key* key, uint8_t byte) {
    return (uint8_t)((key->sbox[byte >> 4] << 4) | key->sbox[byte & 0x0F]);
}

// Apply the byte-level permutation (pbox) to the 8-byte state block.
static void permute_bytes(const struct aut64_key* key, uint8_t* state) {
    // Key is validated up-front, so pbox[] is a correct permutation of 0..7.
    uint8_t result[AUT64_PBOX_SIZE];

    for(int i = 0; i < AUT64_PBOX_SIZE; i++) {
        result[key->pbox[i]] = state[i];
    }

    memcpy(state, result, AUT64_PBOX_SIZE);
}

// Apply bit-level permutation to a single byte using the pbox mapping.
static uint8_t permute_bits(const struct aut64_key* key, uint8_t byte) {
    // Key is validated up-front, so pbox[] is a correct permutation of 0..7.
    uint8_t result = 0;

    for(int i = 0; i < 8; i++) {
        if(byte & (1 << i)) {
            result |= (uint8_t)(1 << key->pbox[i]);
        }
    }

    return result;
}

// Encrypt one 8-byte block in place using the provided validated key.
int aut64_encrypt(const struct aut64_key* key, uint8_t* message) {
    int rc;

#ifdef AUT64_ENABLE_VALIDATIONS
    if(!key || !message) {
        return AUT64_ERR_NULL_POINTER;
    }
    // Validate key before doing anything. This prevents silent, unsafe behavior.
    rc = aut64_validate_key(key);
    if(rc != AUT64_OK) {
        return rc;
    }
#endif

    // Build a reversed key (inverse pbox and sbox) ...
    // Fully initialize to avoid any uninitialized fields/padding.
    struct aut64_key reverse_key = (struct aut64_key){0};
    reverse_key.index = key->index;
    memcpy(reverse_key.key, key->key, AUT64_KEY_SIZE);

    rc = reverse_box(reverse_key.pbox, key->pbox, AUT64_PBOX_SIZE);
    if(rc != AUT64_OK) {
        return rc;
    }

    rc = reverse_box(reverse_key.sbox, key->sbox, AUT64_SBOX_SIZE);
    if(rc != AUT64_OK) {
        return rc;
    }

    for(int i = 0; i < AUT64_NUM_ROUNDS; i++) {
        permute_bytes(&reverse_key, message);
        message[AUT64_BLOCK_SIZE - 1] = encrypt_compress(&reverse_key, message, (uint8_t)i);
        message[AUT64_BLOCK_SIZE - 1] = substitute(&reverse_key, message[AUT64_BLOCK_SIZE - 1]);
        message[AUT64_BLOCK_SIZE - 1] = permute_bits(&reverse_key, message[AUT64_BLOCK_SIZE - 1]);
        message[AUT64_BLOCK_SIZE - 1] = substitute(&reverse_key, message[AUT64_BLOCK_SIZE - 1]);
    }

    return AUT64_OK;
}

// Decrypt one 8-byte block in place using the provided validated key.
int aut64_decrypt(const struct aut64_key* key, uint8_t* message) {
#ifdef AUT64_ENABLE_VALIDATIONS
    if(!key || !message) {
        return AUT64_ERR_NULL_POINTER;
    }
    int rc = aut64_validate_key(key);
    if(rc != AUT64_OK) {
        return rc;
    }
#endif

    for(int i = AUT64_NUM_ROUNDS - 1; i >= 0; i--) {
        message[AUT64_BLOCK_SIZE - 1] = substitute(key, message[AUT64_BLOCK_SIZE - 1]);
        message[AUT64_BLOCK_SIZE - 1] = permute_bits(key, message[AUT64_BLOCK_SIZE - 1]);
        message[AUT64_BLOCK_SIZE - 1] = substitute(key, message[AUT64_BLOCK_SIZE - 1]);
        message[AUT64_BLOCK_SIZE - 1] = decrypt_compress(key, message, (uint8_t)i);
        permute_bytes(key, message);
    }

    return AUT64_OK;
}

#ifdef AUT64_PACK_SUPPORT
// Serialize a validated key structure into its 16-byte packed format.
int aut64_pack(uint8_t* dest, const struct aut64_key* src) {
#ifdef AUT64_ENABLE_VALIDATIONS
    if(!dest || !src) {
        return AUT64_ERR_NULL_POINTER;
    }
    // Validate the key we are about to pack. This prevents producing garbage packed keys.
    int rc = aut64_validate_key(src);
    if(rc != AUT64_OK) {
        return rc;
    }
#endif

    // Initialize the output so callers never observe stale bytes.
    memset(dest, 0, AUT64_PACKED_KEY_SIZE);

    dest[0] = src->index;

    for(uint8_t i = 0; i < AUT64_KEY_SIZE / 2; i++) {
        dest[i + 1] = (uint8_t)((src->key[i * 2] << 4) | src->key[i * 2 + 1]);
    }

    uint32_t pbox = 0;
    for(uint8_t i = 0; i < AUT64_PBOX_SIZE; i++) {
        pbox = (pbox << 3) | src->pbox[i];
    }

    dest[5] = (uint8_t)(pbox >> 16);
    dest[6] = (uint8_t)((pbox >> 8) & 0xFF);
    dest[7] = (uint8_t)(pbox & 0xFF);

    for(uint8_t i = 0; i < AUT64_SBOX_SIZE / 2; i++) {
        dest[i + 8] = (uint8_t)((src->sbox[i * 2] << 4) | src->sbox[i * 2 + 1]);
    }

    return AUT64_OK;
}
#endif // AUT64_PACK_SUPPORT

// Deserialize a 16-byte packed key into a key structure and validate it.
int aut64_unpack(struct aut64_key* dest, const uint8_t* src) {
#ifdef AUT64_ENABLE_VALIDATIONS
    if(!dest || !src) {
        return AUT64_ERR_NULL_POINTER;
    }
#endif

    // Clear the whole struct first, so all fields are in a defined state.
    *dest = (struct aut64_key){0};

    dest->index = src[0];

    for(uint8_t i = 0; i < AUT64_KEY_SIZE / 2; i++) {
        dest->key[i * 2] = (uint8_t)(src[i + 1] >> 4);
        dest->key[i * 2 + 1] = (uint8_t)(src[i + 1] & 0xF);
    }

    uint32_t pbox = ((uint32_t)src[5] << 16) | ((uint32_t)src[6] << 8) | (uint32_t)src[7];

    for(int8_t i = AUT64_PBOX_SIZE - 1; i >= 0; i--) {
        dest->pbox[i] = (uint8_t)(pbox & 0x7);
        pbox >>= 3;
    }

    for(uint8_t i = 0; i < AUT64_SBOX_SIZE / 2; i++) {
        dest->sbox[i * 2] = (uint8_t)(src[i + 8] >> 4);
        dest->sbox[i * 2 + 1] = (uint8_t)(src[i + 8] & 0xF);
    }

#ifdef AUT64_ENABLE_VALIDATIONS
    // Validate what we just unpacked. If invalid, return error.
    // We do not fix up broken keys silently.
    int rc = aut64_validate_key(dest);
    if(rc != AUT64_OK) {
        return AUT64_ERR_INVALID_PACKED;
    }
#endif

    return AUT64_OK;
}

#endif
