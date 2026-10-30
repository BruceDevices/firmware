// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#pragma once
#include "../rf_stream_internal.h"
#define BRUTEFORCE_STATUS_IDLE      0
#define BRUTEFORCE_STATUS_RUNNING   1
#define BRUTEFORCE_STATUS_FOUND     2
#define BRUTEFORCE_STATUS_NOT_FOUND 3
#define BRUTEFORCE_STATUS_CANCELLED 4
typedef struct BruteForceState {
    void (*on_done)(void* context);
    void* on_done_ctx;
    volatile uint32_t progress_current;
    volatile uint32_t progress_total;
    uint32_t decrypted_serial;
    uint32_t decrypted_seed;
    uint32_t decrypted_counter;
    uint32_t key1_low;
    uint32_t key1_high;
    uint16_t key2_low;
    uint16_t decrypted_crc;
    volatile uint8_t cancel;
    volatile uint8_t status;
    uint8_t decrypted_button;
    uint8_t decrypted_type;
} BruteForceState;

#include <stdbool.h>
#include <stdint.h>



extern uint64_t kia_mf_key;
extern uint64_t kia_v6_a_key;
extern uint64_t kia_v6_b_key;
extern uint64_t kia_v5_key;

uint64_t get_kia_mf_key();

uint64_t get_kia_v6_keystore_a();

uint64_t get_kia_v6_keystore_b();

uint64_t get_kia_v5_key();

bool rf_vehicle_keys_has_kia_mf_key(void);
bool rf_vehicle_keys_has_kia_v6_keystore_a(void);
bool rf_vehicle_keys_has_kia_v6_keystore_b(void);
bool rf_vehicle_keys_has_kia_v5_key(void);

bool rf_vehicle_keys_load(RfStreamEnvironment* environment);

#include <stdint.h>
#include <stdbool.h>

// uncomment to activate key validation, index boundary verifications, ...
//#define AUT64_ENABLE_VALIDATIONS

// uncomment to add compilation of aut64_pack (currently unused in the code)
//#define AUT64_PACK_SUPPORT

#define AUT64_NUM_ROUNDS      12
#define AUT64_BLOCK_SIZE      8
#define AUT64_KEY_SIZE        8
#define AUT64_PBOX_SIZE       8
#define AUT64_SBOX_SIZE       16
#define AUT64_PACKED_KEY_SIZE 16

// Internal helper table size (offset lookup table).
#define AUT64_OFFSET_TABLE_SIZE 256

// Status codes. Keep it simple and C-friendly.
#define AUT64_OK                 0
#define AUT64_ERR_INVALID_KEY    (-1)
#define AUT64_ERR_INVALID_PACKED (-2)
#define AUT64_ERR_NULL_POINTER   (-3)

struct aut64_key {
    uint8_t index;
    uint8_t key[AUT64_KEY_SIZE];
    uint8_t pbox[AUT64_PBOX_SIZE];
    uint8_t sbox[AUT64_SBOX_SIZE];
};

#ifdef AUT64_ENABLE_VALIDATIONS
// Optional helper if callers want to check keys up-front.
int aut64_validate_key(const struct aut64_key* key);
#endif

// Pointers are used for both the key and the message to avoid implicit copies.
// The message buffer must be at least AUT64_BLOCK_SIZE bytes.
int aut64_encrypt(const struct aut64_key* key, uint8_t* message);
int aut64_decrypt(const struct aut64_key* key, uint8_t* message);

// Packed key buffer must be at least AUT64_PACKED_KEY_SIZE bytes.
#ifdef AUT64_PACK_SUPPORT
int aut64_pack(uint8_t* dest, const struct aut64_key* src);
#endif
int aut64_unpack(struct aut64_key* dest, const uint8_t* src);

#include <stdint.h>

#define PSA_CRYPTO_BF1_CONST_U4 0x0E0F5C41U
#define PSA_CRYPTO_BF1_CONST_U5 0x0F5C4123U

#define PSA_CRYPTO_BF1_START 0x23000000U
#define PSA_CRYPTO_BF1_END   0x24000000U
#define PSA_CRYPTO_BF2_START 0xF3000000U
#define PSA_CRYPTO_BF2_END   0xF4000000U

extern const uint32_t psa_crypto_bf1_key_schedule[4];
extern const uint32_t psa_crypto_bf2_key_schedule[4];

void psa_crypto_setup_byte_buffer(
    uint8_t* buffer,
    uint32_t key1_low,
    uint32_t key1_high,
    uint32_t key2_low);
void psa_crypto_prepare_tea_data(const uint8_t* buffer, uint32_t* w0, uint32_t* w1);
uint8_t psa_crypto_tea_crc(uint32_t v0, uint32_t v1);
uint16_t psa_crypto_crc16_bf2(uint8_t* buffer, int length);
void psa_crypto_unpack_tea_result_to_buffer(uint8_t* buffer, uint32_t v0, uint32_t v1);

#if RF_STREAM_WITH_ENCODER
void psa_crypto_tea_encrypt(uint32_t* v0, uint32_t* v1, const uint32_t* key);
#endif
