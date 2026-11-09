// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#include "rf_vehicle_support.h"
#define KIA_PROTOCOL_V6_NAME "Kia V6"

typedef struct RfStreamProtocolDecoderKiaV6 RfStreamProtocolDecoderKiaV6;
typedef struct RfStreamProtocolEncoderKiaV6 RfStreamProtocolEncoderKiaV6;

extern const RfStreamProtocolDecoder kia_protocol_v6_decoder;
extern const RfStreamProtocolEncoder kia_protocol_v6_encoder;
extern const RfStreamCodec kia_protocol_v6;

// Decoder functions
void* kia_protocol_decoder_v6_alloc(RfStreamEnvironment* environment);
void kia_protocol_decoder_v6_free(void* context);
void kia_protocol_decoder_v6_reset(void* context);
void kia_protocol_decoder_v6_feed(void* context, bool level, uint32_t duration);
uint8_t kia_protocol_decoder_v6_get_hash_data(void* context);
RfStreamProtocolStatus kia_protocol_decoder_v6_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    kia_protocol_decoder_v6_deserialize(void* context, RfRecord* flipper_format);
void kia_protocol_decoder_v6_get_string(void* context, RfText* output);

#if RF_STREAM_WITH_ENCODER
void* kia_protocol_encoder_v6_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    kia_protocol_encoder_v6_deserialize(void* context, RfRecord* flipper_format);
#endif






#include <string.h>

#define TAG "KiaV6"

#define KIA_V6_XOR_MASK_LOW  0x84AF25FB
#define KIA_V6_XOR_MASK_HIGH 0x638766AB

static const RfStreamBlockConst kia_protocol_v6_const = {
    .te_short = 200,
    .te_long = 400,
    .te_delta = 100,
    .min_count_bit_for_found = 144,
};

static const uint8_t aes_sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab,
    0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4,
    0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71,
    0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6,
    0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb,
    0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45,
    0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44,
    0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a,
    0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49,
    0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, 0xba, 0x78, 0x25,
    0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e,
    0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1,
    0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb,
    0x16};

static const uint8_t aes_sbox_inv[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7,
    0xfb, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde,
    0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42,
    0xfa, 0xc3, 0x4e, 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49,
    0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c,
    0xcc, 0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15,
    0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84, 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7,
    0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
    0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc,
    0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73, 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad,
    0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 0x71, 0x1d,
    0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b,
    0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4, 0x1f, 0xdd, 0xa8,
    0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f, 0x60, 0x51,
    0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0,
    0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c,
    0x7d};

static const uint8_t aes_rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

struct RfStreamProtocolDecoderKiaV6 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;
    uint16_t header_count;

    RfManchesterState manchester_state;

    uint32_t data_part1_low;
    uint32_t data_part1_high;

    uint32_t stored_part1_low;
    uint32_t stored_part1_high;
    uint32_t stored_part2_low;
    uint32_t stored_part2_high;
    uint16_t data_part3;

    uint8_t bit_count;
    uint8_t fx_field;
    uint8_t crc1_field;
    uint8_t crc2_field;
    bool keys_loaded;
};

struct RfStreamProtocolEncoderKiaV6 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;
    uint32_t stored_part1_low;
    uint32_t stored_part1_high;
    uint32_t stored_part2_low;
    uint32_t stored_part2_high;
    uint16_t data_part3;
    uint8_t fx_field;
};

typedef enum {
    KiaV6DecoderStepReset = 0,
    KiaV6DecoderStepWaitFirstHigh,
    KiaV6DecoderStepCountPreamble,
    KiaV6DecoderStepWaitLongHigh,
    KiaV6DecoderStepData,
} KiaV6DecoderStep;

const RfStreamProtocolDecoder kia_protocol_v6_decoder = {
    .alloc = kia_protocol_decoder_v6_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = kia_protocol_decoder_v6_feed,
    .reset = kia_protocol_decoder_v6_reset,
    .get_hash_data = kia_protocol_decoder_v6_get_hash_data,
    .serialize = kia_protocol_decoder_v6_serialize,
    .deserialize = kia_protocol_decoder_v6_deserialize,
    .get_string = kia_protocol_decoder_v6_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder kia_protocol_v6_encoder = {
    .alloc = kia_protocol_encoder_v6_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = kia_protocol_encoder_v6_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder kia_protocol_v6_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec kia_protocol_v6 = {
    .name = KIA_PROTOCOL_V6_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &kia_protocol_v6_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &kia_protocol_v6_encoder,
#else
    .encoder = NULL,
#endif
};

#define kia_v6_crc8(data, len) rf_stream_protocol_blocks_crc8((data), (len), 0x07, 0xFF)

static uint8_t gf_mul2(uint8_t x) {
    return ((x >> 7) * 0x1b) ^ (x << 1);
}

static void aes_subbytes_inv(uint8_t* state) {
    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 4; col++) {
            state[row + col * 4] = aes_sbox_inv[state[row + col * 4]];
        }
    }
}

static void aes_shiftrows_inv(uint8_t* state) {
    uint8_t temp;

    temp = state[13];
    state[13] = state[9];
    state[9] = state[5];
    state[5] = state[1];
    state[1] = temp;

    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;

    temp = state[3];
    state[3] = state[7];
    state[7] = state[11];
    state[11] = state[15];
    state[15] = temp;
}

static void aes_mixcolumns_inv(uint8_t* state) {
    uint8_t a, b, c, d;
    for(int i = 0; i < 4; i++) {
        a = state[i * 4];
        b = state[i * 4 + 1];
        c = state[i * 4 + 2];
        d = state[i * 4 + 3];

        uint8_t a2 = gf_mul2(a);
        uint8_t a4 = gf_mul2(a2);
        uint8_t a8 = gf_mul2(a4);
        uint8_t b2 = gf_mul2(b);
        uint8_t b4 = gf_mul2(b2);
        uint8_t b8 = gf_mul2(b4);
        uint8_t c2 = gf_mul2(c);
        uint8_t c4 = gf_mul2(c2);
        uint8_t c8 = gf_mul2(c4);
        uint8_t d2 = gf_mul2(d);
        uint8_t d4 = gf_mul2(d2);
        uint8_t d8 = gf_mul2(d4);

        state[i * 4] = (a8 ^ a4 ^ a2) ^ (b8 ^ b2 ^ b) ^ (c8 ^ c4 ^ c) ^ (d8 ^ d);
        state[i * 4 + 1] = (a8 ^ a) ^ (b8 ^ b4 ^ b2) ^ (c8 ^ c2 ^ c) ^ (d8 ^ d4 ^ d);
        state[i * 4 + 2] = (a8 ^ a4 ^ a) ^ (b8 ^ b) ^ (c8 ^ c4 ^ c2) ^ (d8 ^ d2 ^ d);
        state[i * 4 + 3] = (a8 ^ a2 ^ a) ^ (b8 ^ b4 ^ b) ^ (c8 ^ c) ^ (d8 ^ d4 ^ d2);
    }
}

static void aes_addroundkey(uint8_t* state, const uint8_t* round_key) {
    for(int col = 0; col < 4; col++) {
        state[col * 4] ^= round_key[col * 4];
        state[col * 4 + 1] ^= round_key[col * 4 + 1];
        state[col * 4 + 2] ^= round_key[col * 4 + 2];
        state[col * 4 + 3] ^= round_key[col * 4 + 3];
    }
}

#if RF_STREAM_WITH_ENCODER
static void aes_subbytes(uint8_t* state) {
    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 4; col++) {
            state[row + col * 4] = aes_sbox[state[row + col * 4]];
        }
    }
}

static void aes_shiftrows(uint8_t* state) {
    uint8_t temp;
    temp = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = temp;
    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;
    temp = state[3];
    state[3] = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = temp;
}

static void aes_mixcolumns(uint8_t* state) {
    uint8_t a, b, c, d;
    for(int i = 0; i < 4; i++) {
        a = state[i * 4];
        b = state[i * 4 + 1];
        c = state[i * 4 + 2];
        d = state[i * 4 + 3];
        state[i * 4] = gf_mul2(a) ^ gf_mul2(b) ^ b ^ c ^ d;
        state[i * 4 + 1] = a ^ gf_mul2(b) ^ gf_mul2(c) ^ c ^ d;
        state[i * 4 + 2] = a ^ b ^ gf_mul2(c) ^ gf_mul2(d) ^ d;
        state[i * 4 + 3] = gf_mul2(a) ^ a ^ b ^ c ^ gf_mul2(d);
    }
}

static void aes128_encrypt(const uint8_t* expanded_key, uint8_t* data) {
    uint8_t state[16];
    memcpy(state, data, 16);
    aes_addroundkey(state, &expanded_key[0]);
    for(int round = 1; round < 10; round++) {
        aes_subbytes(state);
        aes_shiftrows(state);
        aes_mixcolumns(state);
        aes_addroundkey(state, &expanded_key[round * 16]);
    }
    aes_subbytes(state);
    aes_shiftrows(state);
    aes_addroundkey(state, &expanded_key[160]);
    memcpy(data, state, 16);
}
#endif

static void aes_key_expansion(const uint8_t* key, uint8_t* round_keys) {
    for(int i = 0; i < 16; i++) {
        round_keys[i] = key[i];
    }

    for(int i = 4; i < 44; i++) {
        int prev_word_idx = (i - 1) * 4;
        uint8_t b0 = round_keys[prev_word_idx];
        uint8_t b1 = round_keys[prev_word_idx + 1];
        uint8_t b2 = round_keys[prev_word_idx + 2];
        uint8_t b3 = round_keys[prev_word_idx + 3];

        if((i % 4) == 0) {
            uint8_t new_b0 = aes_sbox[b1] ^ aes_rcon[(i / 4) - 1];
            uint8_t new_b1 = aes_sbox[b2];
            uint8_t new_b2 = aes_sbox[b3];
            uint8_t new_b3 = aes_sbox[b0];
            b0 = new_b0;
            b1 = new_b1;
            b2 = new_b2;
            b3 = new_b3;
        }

        int back_word_idx = (i - 4) * 4;
        b0 ^= round_keys[back_word_idx];
        b1 ^= round_keys[back_word_idx + 1];
        b2 ^= round_keys[back_word_idx + 2];
        b3 ^= round_keys[back_word_idx + 3];

        int curr_word_idx = i * 4;
        round_keys[curr_word_idx] = b0;
        round_keys[curr_word_idx + 1] = b1;
        round_keys[curr_word_idx + 2] = b2;
        round_keys[curr_word_idx + 3] = b3;
    }
}

static void aes128_decrypt(const uint8_t* expanded_key, uint8_t* data) {
    uint8_t state[16];
    memcpy(state, data, 16);

    aes_addroundkey(state, &expanded_key[160]);

    for(int round = 9; round > 0; round--) {
        aes_shiftrows_inv(state);
        aes_subbytes_inv(state);
        aes_addroundkey(state, &expanded_key[round * 16]);
        aes_mixcolumns_inv(state);
    }

    aes_shiftrows_inv(state);
    aes_subbytes_inv(state);
    aes_addroundkey(state, &expanded_key[0]);

    memcpy(data, state, 16);
}

static void get_kia_v6_aes_key(uint8_t* aes_key) {
    uint64_t keystore_a = get_kia_v6_keystore_a();
    uint32_t keystore_a_hi = (keystore_a >> 32) & 0xFFFFFFFF;
    uint32_t keystore_a_lo = keystore_a & 0xFFFFFFFF;

    uint32_t uVar15_a = keystore_a_lo ^ KIA_V6_XOR_MASK_LOW; // low part
    uint32_t uVar5_a = KIA_V6_XOR_MASK_HIGH ^ keystore_a_hi; // high part

    uint64_t val64_a = ((uint64_t)uVar5_a << 32) | uVar15_a;
    for(int i = 0; i < 8; i++) {
        aes_key[i] = (val64_a >> (56 - i * 8)) & 0xFF;
    }

    uint64_t keystore_b = get_kia_v6_keystore_b();
    uint32_t keystore_b_hi = (keystore_b >> 32) & 0xFFFFFFFF;
    uint32_t keystore_b_lo = keystore_b & 0xFFFFFFFF;

    uint32_t uVar15_b = keystore_b_lo ^ KIA_V6_XOR_MASK_LOW;
    uint32_t uVar5_b = KIA_V6_XOR_MASK_HIGH ^ keystore_b_hi;

    uint64_t val64_b = ((uint64_t)uVar5_b << 32) | uVar15_b;
    for(int i = 0; i < 8; i++) {
        aes_key[i + 8] = (val64_b >> (56 - i * 8)) & 0xFF;
    }
}

#if RF_STREAM_WITH_ENCODER
static void kia_v6_encrypt_payload(
    uint8_t fx_field,
    uint32_t serial,
    uint8_t btn,
    uint32_t cnt,
    uint32_t* out_part1_low,
    uint32_t* out_part1_high,
    uint32_t* out_part2_low,
    uint32_t* out_part2_high,
    uint16_t* out_part3) {
    uint8_t plain[16];
    memset(plain, 0, 16);
    plain[0] = fx_field;
    plain[4] = (serial >> 16) & 0xFF;
    plain[5] = (serial >> 8) & 0xFF;
    plain[6] = serial & 0xFF;
    plain[7] = btn & 0x0F;
    plain[8] = (cnt >> 24) & 0xFF;
    plain[9] = (cnt >> 16) & 0xFF;
    plain[10] = (cnt >> 8) & 0xFF;
    plain[11] = cnt & 0xFF;
    plain[12] = aes_sbox[cnt & 0xFF];
    plain[15] = kia_v6_crc8(plain, 15);

    uint8_t aes_key[16];
    get_kia_v6_aes_key(aes_key);
    uint8_t expanded_key[176];
    aes_key_expansion(aes_key, expanded_key);
    aes128_encrypt(expanded_key, plain);

    uint8_t fx_hi = 0x20 | (fx_field >> 4);
    uint8_t fx_lo = fx_field & 0x0F;
    *out_part1_high = ((uint32_t)fx_hi << 24) | ((uint32_t)fx_lo << 16) |
                      ((uint32_t)plain[0] << 8) | plain[1];
    *out_part1_low = ((uint32_t)plain[2] << 24) | ((uint32_t)plain[3] << 16) |
                     ((uint32_t)plain[4] << 8) | plain[5];
    *out_part2_high = ((uint32_t)plain[6] << 24) | ((uint32_t)plain[7] << 16) |
                      ((uint32_t)plain[8] << 8) | plain[9];
    *out_part2_low = ((uint32_t)plain[10] << 24) | ((uint32_t)plain[11] << 16) |
                     ((uint32_t)plain[12] << 8) | plain[13];
    *out_part3 = ((uint16_t)plain[14] << 8) | plain[15];
}
#endif

static bool kia_v6_decrypt(RfStreamProtocolDecoderKiaV6* instance) {
    uint8_t encrypted_data[16];

    encrypted_data[0] = (instance->stored_part1_high >> 8) & 0xFF;
    encrypted_data[1] = instance->stored_part1_high & 0xFF;

    encrypted_data[2] = (instance->stored_part1_low >> 24) & 0xFF;
    encrypted_data[3] = (instance->stored_part1_low >> 16) & 0xFF;
    encrypted_data[4] = (instance->stored_part1_low >> 8) & 0xFF;
    encrypted_data[5] = instance->stored_part1_low & 0xFF;

    encrypted_data[6] = (instance->stored_part2_high >> 24) & 0xFF;
    encrypted_data[7] = (instance->stored_part2_high >> 16) & 0xFF;
    encrypted_data[8] = (instance->stored_part2_high >> 8) & 0xFF;
    encrypted_data[9] = instance->stored_part2_high & 0xFF;

    encrypted_data[10] = (instance->stored_part2_low >> 24) & 0xFF;
    encrypted_data[11] = (instance->stored_part2_low >> 16) & 0xFF;
    encrypted_data[12] = (instance->stored_part2_low >> 8) & 0xFF;
    encrypted_data[13] = instance->stored_part2_low & 0xFF;

    encrypted_data[14] = (instance->data_part3 >> 8) & 0xFF;
    encrypted_data[15] = instance->data_part3 & 0xFF;

    uint8_t fx_byte0 = (instance->stored_part1_high >> 24) & 0xFF;
    uint8_t fx_byte1 = (instance->stored_part1_high >> 16) & 0xFF;
    instance->fx_field = ((fx_byte0 & 0xF) << 4) | (fx_byte1 & 0xF);

    uint8_t aes_key[16];
    get_kia_v6_aes_key(aes_key);
    uint8_t expanded_key[176];
    aes_key_expansion(aes_key, expanded_key);
    aes128_decrypt(expanded_key, encrypted_data);

    uint8_t* decrypted = encrypted_data;

    uint8_t calculated_crc = kia_v6_crc8(decrypted, 15);
    uint8_t stored_crc = decrypted[15];

    // Serial: bytes 4-6 as 24-bit big-endian
    instance->generic.serial = ((uint32_t)decrypted[4] << 16) | ((uint32_t)decrypted[5] << 8) |
                               decrypted[6];

    instance->generic.btn = decrypted[7];

    instance->generic.cnt = ((uint32_t)decrypted[8] << 24) | ((uint32_t)decrypted[9] << 16) |
                            ((uint32_t)decrypted[10] << 8) | decrypted[11];

    instance->crc1_field = decrypted[12];

    instance->crc2_field = decrypted[15];

    return (calculated_crc ^ stored_crc) < 2;
}

void* kia_protocol_decoder_v6_alloc(RfStreamEnvironment* environment) {
    RfStreamProtocolDecoderKiaV6* instance = malloc(sizeof(RfStreamProtocolDecoderKiaV6));
    memset(instance, 0, sizeof(RfStreamProtocolDecoderKiaV6));

    if(environment) {
        rf_vehicle_keys_load(environment);
    }

    instance->base.protocol = &kia_protocol_v6;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void kia_protocol_decoder_v6_reset(void* context) {
    RfStreamProtocolDecoderKiaV6* instance = context;
    instance->decoder.parser_step = KiaV6DecoderStepReset;
    rf_manchester_advance(
        instance->manchester_state, RfManchesterEventReset, &instance->manchester_state, NULL);
}

void kia_protocol_decoder_v6_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderKiaV6* instance = context;

    uint32_t uVar4, uVar5;
    RfManchesterEvent event;
    bool data_bit;
    uint8_t bit_count_inc;
    uint32_t step_value;

    switch(instance->decoder.parser_step) {
    case KiaV6DecoderStepReset: // case 0
        if(level == 0) {
            return;
        }
        if(DURATION_DIFF(duration, kia_protocol_v6_const.te_short) <
           kia_protocol_v6_const.te_delta) {
            instance->decoder.parser_step = KiaV6DecoderStepWaitFirstHigh;
            instance->decoder.te_last = duration;
            instance->header_count = 0;
            rf_manchester_advance(
                instance->manchester_state,
                RfManchesterEventReset,
                &instance->manchester_state,
                NULL);
        }
        return;

    case KiaV6DecoderStepWaitFirstHigh: // case 1
        if(level != 0) {
            return;
        }
        uint32_t diff_short = DURATION_DIFF(duration, kia_protocol_v6_const.te_short);
        uint32_t diff_long = DURATION_DIFF(duration, kia_protocol_v6_const.te_long);

        uint32_t diff = (diff_long < diff_short) ? diff_long : diff_short;

        if(diff_long < kia_protocol_v6_const.te_delta && diff_long < diff_short) {
            if(instance->header_count >= 0x259) { // 601 decimal
                instance->header_count = 0;
                instance->decoder.te_last = duration;
                instance->decoder.parser_step = KiaV6DecoderStepWaitLongHigh;
                return;
            }
        }

        if(diff >= kia_protocol_v6_const.te_delta) {
            step_value = KiaV6DecoderStepReset;
            goto LAB_reset;
        }

        if(DURATION_DIFF(instance->decoder.te_last, kia_protocol_v6_const.te_short) <
           kia_protocol_v6_const.te_delta) {
            instance->decoder.te_last = duration;
            instance->header_count++;
            return;
        } else {
            step_value = KiaV6DecoderStepReset;
            goto LAB_reset;
        }

    case KiaV6DecoderStepWaitLongHigh: // case 2
        if(level == 0) {
            step_value = KiaV6DecoderStepReset;
            goto LAB_reset;
        }
        uint32_t diff_long_check = DURATION_DIFF(duration, kia_protocol_v6_const.te_long);
        uint32_t diff_short_check = DURATION_DIFF(duration, kia_protocol_v6_const.te_short);

        if(diff_long_check >= kia_protocol_v6_const.te_delta) {
            if(diff_short_check >= kia_protocol_v6_const.te_delta) {
                step_value = KiaV6DecoderStepReset;
                goto LAB_reset;
            }
        }

        if(DURATION_DIFF(instance->decoder.te_last, kia_protocol_v6_const.te_long) >=
           kia_protocol_v6_const.te_delta) {
            step_value = KiaV6DecoderStepReset;
            goto LAB_reset;
        }
        instance->decoder.decode_data = 0;
        instance->decoder.decode_count_bit = 0;

        rf_stream_protocol_blocks_add_bit(&instance->decoder, 1);
        rf_stream_protocol_blocks_add_bit(&instance->decoder, 1);
        rf_stream_protocol_blocks_add_bit(&instance->decoder, 0);
        rf_stream_protocol_blocks_add_bit(&instance->decoder, 1);

        instance->data_part1_low = (uint32_t)(instance->decoder.decode_data & 0xFFFFFFFF);
        instance->data_part1_high = (uint32_t)((instance->decoder.decode_data >> 32) & 0xFFFFFFFF);
        instance->bit_count = instance->decoder.decode_count_bit;

        instance->decoder.parser_step = KiaV6DecoderStepData;
        return;

    case KiaV6DecoderStepData: // case 3
        if(DURATION_DIFF(duration, kia_protocol_v6_const.te_short) <
           kia_protocol_v6_const.te_delta) {
            event = (level & 0x7F) << 1;
            goto manchester_process;
        } else if(
            DURATION_DIFF(duration, kia_protocol_v6_const.te_long) <
            kia_protocol_v6_const.te_delta) {
            event = level ? 6 : 4;
            goto manchester_process;
        }
        step_value = KiaV6DecoderStepReset;
        goto LAB_reset;

    manchester_process:
        if(rf_manchester_advance(
               instance->manchester_state, event, &instance->manchester_state, &data_bit)) {
            uVar4 = instance->data_part1_low;
            uVar5 = (uVar4 << 1) | (data_bit ? 1 : 0);

            uint32_t carry = (uVar4 >> 31) & 1;
            uVar4 = (instance->data_part1_high << 1) | carry;

            instance->data_part1_low = uVar5;
            instance->data_part1_high = uVar4;

            instance->decoder.decode_data = ((uint64_t)uVar4 << 32) | uVar5;

            bit_count_inc = instance->bit_count + 1;
            instance->bit_count = bit_count_inc;

            if(bit_count_inc == 0x40) {
                instance->stored_part1_low = ~uVar5;
                instance->stored_part1_high = ~uVar4;
                instance->data_part1_low = 0;
                instance->data_part1_high = 0;
            } else if(bit_count_inc == 0x80) {
                instance->stored_part2_low = ~uVar5;
                instance->stored_part2_high = ~uVar4;
                instance->data_part1_low = 0;
                instance->data_part1_high = 0;
            }
        }

        instance->decoder.te_last = duration;

        if(instance->bit_count != kia_protocol_v6_const.min_count_bit_for_found) {
            return;
        }
        instance->generic.data_count_bit = kia_protocol_v6_const.min_count_bit_for_found;
        instance->data_part3 = ~((uint16_t)instance->data_part1_low);

        kia_v6_decrypt(instance);

        if(instance->base.callback) {
            instance->base.callback(&instance->base, instance->base.context);
        }

        instance->data_part1_low = 0;
        instance->data_part1_high = 0;
        instance->bit_count = 0;
        step_value = KiaV6DecoderStepReset;
        goto LAB_reset;

    default:
        return;
    }

LAB_reset:
    instance->decoder.parser_step = step_value;
    return;
}

uint8_t kia_protocol_decoder_v6_get_hash_data(void* context) {
    RfStreamProtocolDecoderKiaV6* instance = context;

    uint8_t hash = 0;
    uint8_t* data = (uint8_t*)&instance->data_part1_low;
    size_t data_size = (instance->bit_count + 7) / 8;
    size_t max_size = (kia_protocol_v6_const.min_count_bit_for_found + 7) / 8;
    if(data_size > max_size) {
        data_size = max_size;
    }

    for(size_t i = 0; i < data_size; i++) {
        hash ^= data[i];
    }

    return hash;
}

RfStreamProtocolStatus kia_protocol_decoder_v6_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderKiaV6* instance = context;

    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;
    do {
        if(!rf_record_write_uint32(flipper_format, FF_FREQUENCY, &preset->frequency, 1))
            break;

        if(!rf_record_write_string_cstr(
               flipper_format, FF_PRESET, rf_text_get_cstr(preset->name)))
            break;

        if(!rf_record_write_string_cstr(
               flipper_format, FF_PROTOCOL, instance->generic.protocol_name))
            break;

        uint32_t bits = kia_protocol_v6_const.min_count_bit_for_found;
        if(!rf_record_write_uint32(flipper_format, FF_BIT, &bits, 1)) break;

        uint64_t key_data = ((uint64_t)instance->stored_part1_high << 32) |
                            instance->stored_part1_low;
        char key_str[20];
        snprintf(key_str, sizeof(key_str), "%016llX", key_data);
        if(!rf_record_write_string_cstr(flipper_format, FF_KEY, key_str)) break;

        if(rf_stream_serialize_fields(
               flipper_format,
               RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT,
               instance->generic.serial,
               instance->generic.btn,
               instance->generic.cnt,
               0) != RfStreamProtocolStatusOk)
            break;

        uint32_t key2_low = instance->stored_part2_low;
        if(!rf_record_write_uint32(flipper_format, "Key_2", &key2_low, 1)) break;

        uint32_t key2_high = instance->stored_part2_high;
        if(!rf_record_write_uint32(flipper_format, "Key_3", &key2_high, 1)) break;

        uint32_t key3 = instance->data_part3;
        if(!rf_record_write_uint32(flipper_format, "Key_4", &key3, 1)) break;

        uint32_t fx = instance->fx_field;
        if(!rf_record_write_uint32(flipper_format, "Fx", &fx, 1)) break;

        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

RfStreamProtocolStatus
    kia_protocol_decoder_v6_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderKiaV6* instance = context;

    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;
    do {
        if(!rf_stream_block_generic_deserialize_check_count_bit(
               &instance->generic, flipper_format, kia_protocol_v6_const.min_count_bit_for_found)) {
            break;
        }

        uint64_t key = instance->generic.data;

        instance->stored_part1_low = (uint32_t)(key & 0xFFFFFFFF);
        instance->stored_part1_high = (uint32_t)((key >> 32) & 0xFFFFFFFF);

        uint32_t temp;
        if(rf_record_read_uint32(flipper_format, "Key_2", &temp, 1)) {
            instance->stored_part2_low = temp;
        }
        if(rf_record_read_uint32(flipper_format, "Key_3", &temp, 1)) {
            instance->stored_part2_high = temp;
        }
        if(rf_record_read_uint32(flipper_format, "Key_4", &temp, 1)) {
            instance->data_part3 = (uint16_t)temp;
        }

        uint32_t serial = instance->generic.serial;
        uint32_t btn = instance->generic.btn;
        uint32_t cnt = instance->generic.cnt;
        rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);
        instance->generic.serial = serial;
        instance->generic.btn = (uint8_t)btn;
        instance->generic.cnt = (uint16_t)cnt;
        if(rf_record_read_uint32(flipper_format, "Fx", &temp, 1)) {
            instance->fx_field = (uint8_t)temp;
        }

        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

void kia_protocol_decoder_v6_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderKiaV6* instance = context;

    kia_v6_decrypt(instance);

    uint32_t key1_hi = instance->stored_part1_high;
    uint32_t key1_lo = instance->stored_part1_low;
    uint32_t key2_hi = instance->stored_part2_high;
    uint32_t key2_lo = instance->stored_part2_low;

    uint32_t key2_uVar4 = key2_hi << 16;
    uint32_t key2_uVar2 = key2_lo >> 16;
    uint32_t key2_uVar1 = key2_hi >> 16;
    uint32_t key2_combined = key2_uVar4 | key2_uVar2;

    uint32_t key2_uVar3 = key2_lo << 16;
    uint32_t key2_second = (instance->data_part3 & 0xFFFF) | key2_uVar3;

    uint32_t serial_6 = instance->generic.serial & 0xFFFFFF;

    rf_text_printf(
        output,
        "%s %dbit\r\n"
        "%08lX%08lX%04lX\r\n"
        "%08lX%08lX Fx:%02X\r\n"
        "Ser:%06lX Btn:%01X CRC1:%02X\r\n"
        "Cnt:%08lX CRC2:%02X",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        key1_hi,
        key1_lo,
        key2_uVar1,
        key2_combined,
        key2_second,
        instance->fx_field,
        serial_6,
        instance->generic.btn & 0x0F,
        instance->crc1_field,
        instance->generic.cnt,
        instance->crc2_field);
}

#if RF_STREAM_WITH_ENCODER

#define KIA_V6_PREAMBLE_PAIRS_1 640
#define KIA_V6_PREAMBLE_PAIRS_2 38
#define KIA_V6_TE_SHORT         200
#define KIA_V6_TE_LONG          400
#define KIA_V6_UPLOAD_SIZE      1928
_Static_assert(
    KIA_V6_UPLOAD_SIZE <= RF_SHARED_UPLOAD_CAPACITY,
    "KIA_V6_UPLOAD_SIZE exceeds shared upload slab");

static inline void
    kia_v6_encode_manchester_bit(LevelDuration* upload, size_t* index, bool bit, uint32_t te) {
    if(bit) {
        upload[(*index)++] = level_duration_make(false, te);
        upload[(*index)++] = level_duration_make(true, te);
    } else {
        upload[(*index)++] = level_duration_make(true, te);
        upload[(*index)++] = level_duration_make(false, te);
    }
}

static void kia_v6_encode_message(
    LevelDuration* upload,
    size_t* index,
    int preamble_pairs,
    uint32_t p1_lo,
    uint32_t p1_hi,
    uint32_t p2_lo,
    uint32_t p2_hi,
    uint16_t p3) {
    const uint32_t te_short = KIA_V6_TE_SHORT;
    const uint32_t te_long = KIA_V6_TE_LONG;

    for(int i = 0; i < preamble_pairs; i++) {
        upload[(*index)++] = level_duration_make(true, te_short);
        upload[(*index)++] = level_duration_make(false, te_short);
    }

    upload[(*index)++] = level_duration_make(false, te_short);
    upload[(*index)++] = level_duration_make(true, te_long);
    upload[(*index)++] = level_duration_make(false, te_short);

    for(int b = 60; b >= 0; b--) {
        uint32_t word = (b >= 32) ? p1_hi : p1_lo;
        int shift = (b >= 32) ? (b - 32) : b;
        kia_v6_encode_manchester_bit(upload, index, ((~word) >> shift) & 1, te_short);
    }

    for(int b = 63; b >= 0; b--) {
        uint32_t word = (b >= 32) ? p2_hi : p2_lo;
        int shift = (b >= 32) ? (b - 32) : b;
        kia_v6_encode_manchester_bit(upload, index, ((~word) >> shift) & 1, te_short);
    }

    for(int b = 15; b >= 0; b--) {
        kia_v6_encode_manchester_bit(upload, index, ((~p3) >> b) & 1, te_short);
    }
}

static void kia_protocol_encoder_v6_build_upload(RfStreamProtocolEncoderKiaV6* instance) {
    kia_v6_encrypt_payload(
        instance->fx_field,
        instance->generic.serial,
        instance->generic.btn & 0x0F,
        instance->generic.cnt,
        &instance->stored_part1_low,
        &instance->stored_part1_high,
        &instance->stored_part2_low,
        &instance->stored_part2_high,
        &instance->data_part3);

    uint32_t p1_lo = instance->stored_part1_low;
    uint32_t p1_hi = instance->stored_part1_high;
    uint32_t p2_lo = instance->stored_part2_low;
    uint32_t p2_hi = instance->stored_part2_high;
    uint16_t p3 = instance->data_part3;

    size_t index = 0;

    kia_v6_encode_message(
        instance->encoder.upload, &index, KIA_V6_PREAMBLE_PAIRS_1, p1_lo, p1_hi, p2_lo, p2_hi, p3);

    instance->encoder.upload[index++] = level_duration_make(false, KIA_V6_TE_LONG);

    kia_v6_encode_message(
        instance->encoder.upload, &index, KIA_V6_PREAMBLE_PAIRS_2, p1_lo, p1_hi, p2_lo, p2_hi, p3);

    instance->encoder.upload[index++] = level_duration_make(false, KIA_V6_TE_LONG);

    instance->encoder.size_upload = index;

#ifndef REMOVE_LOGS
    RF_STREAM_LOG_I(
        TAG,
        "Encrypted payload (TX): P1=%08lX%08lX P2=%08lX%08lX P3=%04X",
        (unsigned long)p1_hi,
        (unsigned long)p1_lo,
        (unsigned long)p2_hi,
        (unsigned long)p2_lo,
        (unsigned int)p3);
    RF_STREAM_LOG_I(
        TAG,
        "Encoder input: Ser=0x%06lX Btn=%u Cnt=0x%08lX -> upload %u entries, repeat %ld",
        (unsigned long)instance->generic.serial,
        (unsigned int)(instance->generic.btn & 0x0F),
        (unsigned long)instance->generic.cnt,
        (unsigned)index,
        (long)instance->encoder.repeat);
#endif
    instance->encoder.front = 0;
}

void* kia_protocol_encoder_v6_alloc(RfStreamEnvironment* environment) {
    if(environment) {
        rf_vehicle_keys_load(environment);
    }
    if(!rf_vehicle_keys_has_kia_v6_keystore_a() || !rf_vehicle_keys_has_kia_v6_keystore_b()) {
        RF_STREAM_LOG_E(TAG, "Kia V6 encoder missing KIA_KEY2/KIA_KEY3 keystore entries");
        return NULL;
    }

    RfStreamProtocolEncoderKiaV6* instance = malloc(sizeof(RfStreamProtocolEncoderKiaV6));
    if(!instance) return NULL;
    memset(instance, 0, sizeof(RfStreamProtocolEncoderKiaV6));

    instance->base.protocol = &kia_protocol_v6;
    instance->generic.protocol_name = instance->base.protocol->name;
    rf_stream_encoder_buffer_ensure(instance, KIA_V6_UPLOAD_SIZE);
    instance->encoder.repeat = 10;
    instance->encoder.front = 0;
    instance->encoder.is_running = false;
    return instance;
}

RfStreamProtocolStatus
    kia_protocol_encoder_v6_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderKiaV6* instance = context;

    instance->encoder.is_running = false;
    instance->encoder.front = 0;
    instance->generic.data_count_bit = kia_protocol_v6_const.min_count_bit_for_found;

    if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
       RfStreamProtocolStatusOk) {
        return RfStreamProtocolStatusError;
    }

    uint32_t serial = 0;
    uint32_t btn = UINT32_MAX;
    uint32_t cnt = UINT32_MAX;
    rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);
    if(btn == UINT32_MAX || cnt == UINT32_MAX) {
        return RfStreamProtocolStatusError;
    }
    instance->generic.serial = serial;
    instance->generic.btn = (uint8_t)(btn & 0x0F);
    instance->generic.cnt = cnt;

    uint32_t fx_temp = 0;
    rf_record_rewind(flipper_format);
    if(rf_record_read_uint32(flipper_format, "Fx", &fx_temp, 1)) {
        instance->fx_field = (uint8_t)(fx_temp & 0xFF);
    } else {
        instance->fx_field = 0;
    }

    instance->encoder.repeat = (int32_t)(rf_stream_encoder_read_repeat(flipper_format, 10) & 0x7FFFFFFF);

    kia_protocol_encoder_v6_build_upload(instance);
    instance->encoder.is_running = true;
    return RfStreamProtocolStatusOk;
}

#endif // RF_STREAM_WITH_ENCODER

#endif
