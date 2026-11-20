// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define RENAULT_PROTOCOL_V1_NAME "Renault V1"

typedef struct RfStreamProtocolDecoderRenaultV1 RfStreamProtocolDecoderRenaultV1;
typedef struct RfStreamProtocolEncoderRenaultV1 RfStreamProtocolEncoderRenaultV1;

#define HITAG2_BF_STATUS_IDLE      0
#define HITAG2_BF_STATUS_RUNNING   1
#define HITAG2_BF_STATUS_FOUND     2
#define HITAG2_BF_STATUS_NOT_FOUND 3
#define HITAG2_BF_STATUS_CANCELLED 4

typedef struct Hitag2BfState Hitag2BfState;
struct Hitag2BfState {
    volatile uint8_t cancel;
    volatile uint32_t progress_current;
    volatile uint8_t status;
    volatile uint32_t progress_total;
    uint8_t frame[11];
    uint8_t iv[4];
    void (*on_done)(void* context);
    void* on_done_ctx;
};

int32_t hitag2_brute_force_thread_entry(void* arg);
bool hitag2_bf_state_from_flipper_format(Hitag2BfState* state, RfRecord* ff);
bool hitag2_bf_needs_bruteforce(RfRecord* ff);
bool hitag2_bf_patch_flipper_format_on_success(RfRecord* ff, const Hitag2BfState* state);
bool hitag2_bf_patch_flipper_format_on_miss(RfRecord* ff);
bool hitag2_flipper_format_get_string(RfRecord* ff, RfText* output);

extern const RfStreamCodec renault_v1_protocol;

void* rf_stream_protocol_decoder_renault_v1_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_renault_v1_reset(void* context);
void rf_stream_protocol_decoder_renault_v1_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_renault_v1_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_renault_v1_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_renault_v1_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_renault_v1_get_string(void* context, RfText* output);

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_renault_v1_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_renault_v1_deserialize(void* context, RfRecord* flipper_format);
#endif




#include <stdlib.h>
#include <string.h>

#define TAG "RenaultV1Protocol"

#define HITAG2_TE_US           125U
#define HITAG2_HEADER_LOW_US   1500U
#define HITAG2_HEADER_HIGH_US  1000U
#define HITAG2_SHORT_GAP_US    21500U
#define HITAG2_PREAMBLE_PAIRS  250U
#define HITAG2_LONG_FRAMES     3U
#define HITAG2_SHORT_FRAMES    3U
#define HITAG2_UPLOAD_CAPACITY 1295U
_Static_assert(
    HITAG2_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "HITAG2_UPLOAD_CAPACITY exceeds shared upload slab");
#define HITAG2_MIN_COUNT_BIT     88U
#define HITAG2_HEADER_BITS       16U
#define HITAG2_KEY_BITS          64U
#define HITAG2_KEY2_BITS         24U
#define HITAG2_SHORT_KEY_BITS    10U
#define HITAG2_KEY_END_BITS      (HITAG2_HEADER_BITS + HITAG2_KEY_BITS)
#define HITAG2_LONG_FRAME_BITS   (HITAG2_KEY_END_BITS + HITAG2_KEY2_BITS)
#define HITAG2_RECOVERED_YES     1U
#define HITAG2_RECOVERED_BF_MISS 2U
#define HITAG2_KEY_FIELD         "Hitag2 Key"
#define HITAG2_EPOCH_FIELD       "Hitag2 Epoch"

#define HITAG2_HEADER_LOW_MIN_US  1150U
#define HITAG2_HEADER_LOW_MAX_US  2200U
#define HITAG2_HEADER_HIGH_MIN_US 800U
#define HITAG2_HEADER_HIGH_MAX_US 1150U
#define HITAG2_DATA_IGNORE_US     49U
#define HITAG2_DATA_RESET_US      520U
#define HITAG2_TE_HIGH_INIT_US    120U
#define HITAG2_TE_LOW_INIT_US     150U
#define HITAG2_HOP_FIELD          "Hop"

static const RfStreamBlockConst renault_v1_const = {
    .te_short = HITAG2_TE_US,
    .te_long = HITAG2_TE_US * 2U,
    .te_delta = 50,
    .min_count_bit_for_found = HITAG2_MIN_COUNT_BIT,
};

struct RfStreamProtocolDecoderRenaultV1 {
    RfStreamProtocolDecoderBase base;

    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    RfManchesterState manchester_state;
    uint16_t te_high;
    uint16_t te_low;
    uint16_t header;
    uint8_t recovered;
    uint8_t hitag2_key[6];
    uint32_t hop;
    uint8_t tail_bits;
    bool hitag2_key_valid;
    uint64_t last_data;
    uint64_t last_data_2;
    bool last_frame_valid;
    uint32_t seed;
    uint64_t data_2;
};

#if RF_STREAM_WITH_ENCODER
struct RfStreamProtocolEncoderRenaultV1 {
    RfStreamProtocolEncoderBase base;

    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;

    uint32_t seed;
    uint64_t data_2;
    uint8_t recovered;
    uint8_t hitag2_key[6];
    uint8_t tail_bits;
    bool hitag2_key_valid;
};
#endif

typedef enum {
    RenaultV1DecoderStepReset = 0,
    RenaultV1DecoderStepCheckSync = 2,
    RenaultV1DecoderStepData = 3,
} RenaultV1DecoderStep;

const RfStreamProtocolDecoder renault_v1_decoder = {
    .alloc = rf_stream_protocol_decoder_renault_v1_alloc,
    .free = rf_stream_decoder_free_default,

    .feed = rf_stream_protocol_decoder_renault_v1_feed,
    .reset = rf_stream_protocol_decoder_renault_v1_reset,

    .get_hash_data = rf_stream_protocol_decoder_renault_v1_get_hash_data,
    .serialize = rf_stream_protocol_decoder_renault_v1_serialize,
    .deserialize = rf_stream_protocol_decoder_renault_v1_deserialize,
    .get_string = rf_stream_protocol_decoder_renault_v1_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder renault_v1_encoder = {
    .alloc = rf_stream_protocol_encoder_renault_v1_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = rf_stream_protocol_encoder_renault_v1_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder renault_v1_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec renault_v1_protocol = {
    .name = RENAULT_PROTOCOL_V1_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &renault_v1_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &renault_v1_encoder,
#else
    .encoder = NULL,
#endif
};

static const char* hitag2_get_button_name(uint8_t btn) {
    static const char* const names[] = {
        "Sync",
        "Lock",
        "Unlock",
        "??",
        "Trunk",
        "??",
        "??",
        "??",
        "Panic",
    };
    return (btn < COUNT_OF(names)) ? names[btn] : "??";
}

static void hitag2_u64_to_bytes_be(uint64_t value, uint8_t* out, size_t nbytes) {
    for(size_t i = 0; i < nbytes; i++) {
        out[i] = (uint8_t)(value >> (8U * (nbytes - 1U - i)));
    }
}

static uint64_t hitag2_bytes_to_u64_be(const uint8_t* data, size_t nbytes) {
    uint64_t value = 0;
    for(size_t i = 0; i < nbytes; i++) {
        value = (value << 8U) | data[i];
    }
    return value;
}

static void hitag2_pack_key_bytes(uint64_t key, uint64_t key_2, uint8_t raw[11]) {
    hitag2_u64_to_bytes_be(key, raw, 8);
    raw[8] = (uint8_t)(key_2 >> 16U);
    raw[9] = (uint8_t)(key_2 >> 8U);
    raw[10] = (uint8_t)key_2;
}

static uint8_t hitag2_frame_xor(const uint8_t raw[11]) {
    uint8_t value = 0;
    for(size_t i = 0; i < 10; i++) {
        value ^= raw[i];
    }
    return value;
}

static uint8_t hitag2_i4(uint64_t x, uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return (uint8_t)((((x >> a) & 1U) << 3U) | (((x >> b) & 1U) << 2U) | (((x >> c) & 1U) << 1U) |
                     ((x >> d) & 1U));
}

static uint8_t hitag2_f20(uint64_t state) {
    const uint8_t s0 = (uint8_t)((0x3C65U >> hitag2_i4(state, 2U, 3U, 5U, 6U)) & 1U);
    const uint8_t s1 = (uint8_t)((0x0EE5U >> hitag2_i4(state, 8U, 12U, 14U, 15U)) & 1U);
    const uint8_t s2 = (uint8_t)((0x0EE5U >> hitag2_i4(state, 17U, 21U, 23U, 26U)) & 1U);
    const uint8_t s3 = (uint8_t)((0x0EE5U >> hitag2_i4(state, 28U, 29U, 31U, 33U)) & 1U);
    const uint8_t s4 = (uint8_t)((0x3C65U >> hitag2_i4(state, 34U, 43U, 44U, 46U)) & 1U);
    return (uint8_t)((0x0DD3929BUL >> ((s0 << 4U) | (s1 << 3U) | (s2 << 2U) | (s3 << 1U) | s4)) &
                     1U);
}

static uint64_t hitag2_lfsr(uint64_t state) {
    const uint64_t fb =
        (state ^ (state >> 2U) ^ (state >> 3U) ^ (state >> 6U) ^ (state >> 7U) ^ (state >> 8U) ^
         (state >> 16U) ^ (state >> 22U) ^ (state >> 23U) ^ (state >> 26U) ^ (state >> 30U) ^
         (state >> 41U) ^ (state >> 42U) ^ (state >> 43U) ^ (state >> 46U) ^ (state >> 47U)) &
        1ULL;
    return (state >> 1U) | (fb << 47U);
}

static uint64_t hitag2_key_to_u64(const uint8_t key[6]) {
    uint64_t key64 = 0ULL;
    for(size_t i = 0; i < 6U; i++) {
        key64 = (key64 << 8U) | key[i];
    }
    return key64;
}

static uint32_t
    hitag2_authenticator(uint32_t uid, uint8_t button, uint32_t counter, const uint8_t key[6]) {
    const uint64_t key64 = hitag2_key_to_u64(key);
    const uint32_t nonce = (counter << 4U) | ((uint32_t)button & 0x0FU);
    uint64_t state = 0ULL;
    for(uint8_t i = 32U; i < 48U; i++) {
        state = (state << 1U) | ((key64 >> i) & 1ULL);
    }
    for(uint8_t i = 0U; i < 32U; i++) {
        state = (state << 1U) | ((uint64_t)((uid >> i) & 1U));
    }
    for(uint8_t i = 0U; i < 32U; i++) {
        const uint64_t nonce_bit = (uint64_t)hitag2_f20(state) ^ ((nonce >> (31U - i)) & 1U);
        state = (state >> 1U) | (((nonce_bit ^ ((key64 >> (31U - i)) & 1ULL)) & 1ULL) << 47U);
    }
    uint32_t hop = 0U;
    for(uint8_t i = 0U; i < 32U; i++) {
        hop = (hop << 1U) | hitag2_f20(state);
        state = hitag2_lfsr(state);
    }
    return hop;
}

static uint16_t hitag2_frame_cnt10(const uint8_t raw[11]) {
    return (uint16_t)(((uint16_t)(raw[4] & 0x0FU) << 6U) | (raw[5] >> 2U));
}

static uint32_t hitag2_frame_hop(const uint8_t raw[11]) {
    return ((uint32_t)(raw[5] & 3U) << 30U) | ((uint32_t)raw[6] << 22U) |
           ((uint32_t)raw[7] << 14U) | ((uint32_t)raw[8] << 6U) | (raw[9] >> 2U);
}

static uint8_t hitag2_frame_tail(const uint8_t raw[11]) {
    return (uint8_t)(raw[9] & 3U);
}

#if RF_STREAM_WITH_ENCODER
static void hitag2_pack_auth_frame(
    uint32_t uid,
    uint8_t btn,
    uint16_t cnt10,
    uint32_t hop,
    uint8_t tail,
    uint8_t raw[11]) {
    raw[0] = (uint8_t)(uid >> 24U);
    raw[1] = (uint8_t)(uid >> 16U);
    raw[2] = (uint8_t)(uid >> 8U);
    raw[3] = (uint8_t)uid;
    raw[4] = (uint8_t)(((btn & 0x0FU) << 4U) | ((cnt10 >> 6U) & 0x0FU));
    raw[5] = (uint8_t)(((cnt10 & 0x3FU) << 2U) | ((hop >> 30U) & 3U));
    raw[6] = (uint8_t)(hop >> 22U);
    raw[7] = (uint8_t)(hop >> 14U);
    raw[8] = (uint8_t)(hop >> 6U);
    raw[9] = (uint8_t)(((hop << 2U) & 0xFCU) | (tail & 3U));
    raw[10] = hitag2_frame_xor(raw);
}

static void hitag2_apply_raw(uint8_t raw[11], uint64_t* data, uint64_t* data_2) {
    *data = hitag2_bytes_to_u64_be(raw, 8);
    *data_2 = ((uint64_t)raw[8] << 16U) | ((uint64_t)raw[9] << 8U) | raw[10];
}
#endif

static bool hitag2_key_nonzero(const uint8_t key[6]) {
    for(size_t i = 0; i < 6U; i++) {
        if(key[i]) return true;
    }
    return false;
}

static void hitag2_flipper_u32(RfRecord* ff, const char* key, uint32_t value) {
    rf_record_rewind(ff);
    if(!rf_record_update_uint32(ff, key, &value, 1)) {
        rf_record_rewind(ff);
        rf_record_insert_or_update_uint32(ff, key, &value, 1);
    }
}

static bool hitag2_read_hex_be(
    RfRecord* flipper_format,
    const char* name,
    uint8_t* data,
    size_t nbytes);

static bool hitag2_read_key(RfRecord* ff, uint8_t key[6]) {
    memset(key, 0, 6U);
    if(!hitag2_read_hex_be(ff, HITAG2_KEY_FIELD, key, 6U)) {
        return false;
    }
    return hitag2_key_nonzero(key);
}

static bool hitag2_key_matches_hop(
    const uint8_t key[6],
    uint32_t uid,
    uint8_t btn,
    uint16_t cnt10,
    uint32_t hop) {
    return hitag2_authenticator(uid, btn, cnt10 & 0x3FFU, key) == hop;
}

static void hitag2_unpack_frame(
    uint64_t data,
    uint64_t data_2,
    uint32_t* serial,
    uint8_t* btn,
    uint16_t* cnt10,
    uint32_t* hop,
    uint8_t* tail) {
    uint8_t raw[11];
    hitag2_pack_key_bytes(data, data_2, raw);
    if(serial) {
        *serial = (uint32_t)(data >> 32U);
    }
    if(btn) {
        *btn = (uint8_t)((raw[4] >> 4U) & 0x0FU);
    }
    if(cnt10) {
        *cnt10 = hitag2_frame_cnt10(raw);
    }
    if(hop) {
        *hop = hitag2_frame_hop(raw);
    }
    if(tail) {
        *tail = hitag2_frame_tail(raw);
    }
}

static bool hitag2_read_hex_be(
    RfRecord* flipper_format,
    const char* name,
    uint8_t* data,
    size_t nbytes) {
    if(!rf_record_rewind(flipper_format)) {
        return false;
    }
    return rf_record_read_hex(flipper_format, name, data, nbytes);
}

static bool hitag2_write_hex_be(
    RfRecord* flipper_format,
    const char* name,
    uint64_t value,
    size_t nbytes) {
    uint8_t data[8] = {0};
    hitag2_u64_to_bytes_be(value, data, nbytes);
    return rf_record_insert_or_update_hex(flipper_format, name, data, nbytes);
}

static uint8_t hitag2_extract_bits(uint32_t value, uint8_t lsb, uint8_t width) {
    return (uint8_t)((value >> lsb) & ((1U << width) - 1U));
}

static void hitag2_serial_permute(const uint8_t serial[4], uint8_t perm[6]) {
    const uint8_t sn0 = serial[0];
    const uint8_t sn1 = serial[1];
    const uint8_t sn2 = serial[2];
    const uint8_t sn3 = serial[3];

    uint8_t acc =
        (uint8_t)(((sn0 >> 6) & 2U) | ((sn1 >> 4) & 8U) | hitag2_extract_bits(sn0 ^ 0x10U, 4, 1));
    acc |= (uint8_t)((~(uint32_t)(sn0 << 2)) & 0x20U);
    acc |= (uint8_t)((sn0 << 5) & 0x40U);
    acc |= (uint8_t)((sn2 << 1) & 0x80U);
    acc |= (uint8_t)((~(uint32_t)(sn2 >> 5)) & 4U);
    acc |= (uint8_t)((~(uint32_t)(sn0 >> 2)) & 0x10U);
    perm[0] = acc;

    const uint8_t sn1_inv_shr3 = (uint8_t) ~(sn1 >> 3);
    const uint8_t sn3_inv_shl3 = (uint8_t) ~(sn3 << 3);
    acc = (uint8_t)(hitag2_extract_bits(sn1, 5, 1) | (sn1_inv_shr3 & 4U) | (sn0 & 0x20U));
    acc |= (uint8_t)(sn3_inv_shl3 & 8U);
    acc |= (uint8_t)((~(uint32_t)(sn2 << 2)) & 0x10U);
    acc |= (uint8_t)((~(uint32_t)(sn2 << 3)) & 0x40U);
    acc |= 0x80U;
    perm[1] = acc;

    const uint8_t sn0_shr3 = (uint8_t)(sn0 >> 3);
    acc = (uint8_t)(hitag2_extract_bits(sn0, 2, 1) | (sn0_shr3 & 2U) |
                    ((~(uint32_t)(sn3 >> 2)) & 4U) | (sn1 & 0x10U));
    acc |= (uint8_t)((~(uint32_t)(sn1 << 4)) & 0x20U);
    acc |= (uint8_t)(sn3_inv_shl3 & 0x40U);
    acc |= 0x80U;
    perm[2] = acc;

    const uint8_t sn2_inv_shl6 = (uint8_t) ~(sn2 << 6);
    acc =
        (uint8_t)(((sn0 >> 2) & 2U) | ((sn1 >> 3) & 8U) | hitag2_extract_bits(sn2 ^ 0x20U, 5, 1));
    acc |= (uint8_t)((sn1 << 5) & 0x20U);
    acc |= (uint8_t)((sn3 << 1) & 0x40U);
    acc |= (uint8_t)(((uint8_t)~sn0_shr3) & 4U);
    acc |= (uint8_t)(sn1_inv_shr3 & 0x10U);
    acc |= (uint8_t)(sn2_inv_shl6 & 0x80U);
    perm[3] = acc;

    uint8_t perm4_lo =
        (uint8_t)(((sn3 << 2) & 8U) | ((sn0 << 4) & 0x10U) | hitag2_extract_bits(sn0 ^ 4U, 2, 1));
    perm4_lo |= (uint8_t)((~(uint32_t)(sn3 >> 1)) & 2U);
    perm4_lo |= (uint8_t)((~(uint32_t)(sn1 >> 4)) & 4U);
    uint8_t perm4_hi = (uint8_t)((~(uint32_t)(sn0 << 4)) & 0x20U);
    perm4_hi |= (uint8_t)(sn2_inv_shl6 & 0x40U);
    const uint8_t sn1_inv_shl3 = (uint8_t) ~(sn1 << 3);
    perm4_hi |= (uint8_t)(sn1_inv_shl3 & 0x80U);
    perm[4] = (uint8_t)(perm4_lo | perm4_hi);

    uint8_t perm5 =
        (uint8_t)(((sn3 >> 2) & 0x10U) | ((sn2 >> 3) & 2U) | (((uint8_t)~sn0) & 0x80U));
    perm5 |= (uint8_t)((~(uint32_t)(sn0 << 3)) & 8U);
    perm5 |= (uint8_t)((sn0 >> 2) & 0x10U);
    perm5 |= (uint8_t)(sn1_inv_shl3 & 0x20U);
    perm5 |= (uint8_t)((sn3 >> 1) & 0x40U);
    perm5 |= (uint8_t)((~(uint32_t)(sn1 >> 1)) & 4U);
    perm[5] = perm5;
}

static uint8_t hitag2_truth(uint32_t table, uint8_t index) {
    return (uint8_t)((table >> index) & 1U);
}

static uint8_t hitag2_filter_index(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return (uint8_t)((a << 3U) | (b << 2U) | (c << 1U) | d);
}

static uint8_t hitag2_byte_bit(uint8_t byte, uint8_t bit) {
    return (uint8_t)((byte >> bit) & 1U);
}

static uint8_t hitag2_filter(const uint8_t state[6]) {
    uint8_t group = 0;
    group |= hitag2_truth(
        0x2C79U,
        hitag2_filter_index(
            hitag2_byte_bit(state[0], 1),
            hitag2_byte_bit(state[0], 2),
            hitag2_byte_bit(state[0], 4),
            hitag2_byte_bit(state[0], 5)));
    group |= (uint8_t)(hitag2_truth(
                           0x6671U,
                           hitag2_filter_index(
                               hitag2_byte_bit(state[1], 0),
                               hitag2_byte_bit(state[1], 1),
                               hitag2_byte_bit(state[1], 3),
                               hitag2_byte_bit(state[1], 7)))
                       << 1U);
    group |= (uint8_t)(hitag2_truth(
                           0x6671U,
                           hitag2_filter_index(
                               hitag2_byte_bit(state[3], 5),
                               hitag2_byte_bit(state[2], 0),
                               hitag2_byte_bit(state[2], 2),
                               hitag2_byte_bit(state[2], 6)))
                       << 2U);
    group |= (uint8_t)(hitag2_truth(
                           0x6671U,
                           hitag2_filter_index(
                               hitag2_byte_bit(state[4], 6),
                               hitag2_byte_bit(state[3], 0),
                               hitag2_byte_bit(state[3], 2),
                               hitag2_byte_bit(state[3], 3)))
                       << 3U);
    group |= (uint8_t)(hitag2_truth(
                           0x2C79U,
                           hitag2_filter_index(
                               hitag2_byte_bit(state[5], 1),
                               hitag2_byte_bit(state[5], 3),
                               hitag2_byte_bit(state[5], 4),
                               hitag2_byte_bit(state[4], 5)))
                       << 4U);
    return hitag2_truth(0x7907287BUL, group);
}

static uint8_t hitag2_parity8(uint8_t value) {
    value ^= (uint8_t)(value >> 4U);
    value ^= (uint8_t)(value >> 2U);
    value ^= (uint8_t)(value >> 1U);
    return (uint8_t)(value & 1U);
}

static uint8_t hitag2_feedback(const uint8_t state[6]) {
    static const uint8_t masks[6] = {0xB3U, 0x80U, 0x83U, 0x22U, 0x00U, 0x73U};
    uint8_t feedback = 0;
    for(uint8_t i = 0; i < 6; i++) {
        feedback ^= hitag2_parity8((uint8_t)(state[i] & masks[i]));
    }
    return (uint8_t)(feedback & 1U);
}

static void hitag2_shift_state(uint8_t state[6], uint8_t input) {
    for(uint8_t i = 0; i < 5; i++) {
        state[i] = (uint8_t)((state[i] << 1U) | (state[i + 1U] >> 7U));
    }
    state[5] = (uint8_t)((state[5] << 1U) | (input & 1U));
}

static void hitag2_shift_u32(uint8_t buf[4], uint8_t inject) {
    const uint8_t b0 = buf[0];
    const uint8_t b1 = buf[1];
    const uint8_t b2 = buf[2];
    const uint8_t b3 = buf[3];
    buf[3] = (uint8_t)((b2 >> 7U) | (b3 << 1U));
    buf[2] = (uint8_t)((b1 >> 7U) | (b2 << 1U));
    buf[1] = (uint8_t)((b0 >> 7U) | (b1 << 1U));
    buf[0] = (uint8_t)((b0 << 1U) | (inject & 1U));
}

static void hitag2_clock_cipher(uint8_t state[6], uint8_t iv_work[4], uint8_t iv_orig[4]) {
    for(uint8_t i = 0; i < 32; i++) {
        const uint8_t filter = hitag2_filter(state);
        uint8_t mix = (iv_work[3] & 0x80U) ? (filter ? 0U : 1U) : (filter ? 1U : 0U);
        if(iv_orig[3] & 0x80U) {
            mix ^= 5U;
        }
        hitag2_shift_state(state, (uint8_t)(mix & 1U));
        hitag2_shift_u32(iv_work, 0);
        hitag2_shift_u32(iv_orig, (uint8_t)((mix >> 2U) & 1U));
    }

    for(uint8_t i = 0; i < 32; i++) {
        hitag2_shift_u32(iv_work, 0);
        if(hitag2_filter(state)) {
            iv_work[0] |= 1U;
        }
        hitag2_shift_state(state, hitag2_feedback(state));
    }
}

#if RF_STREAM_WITH_ENCODER
static void hitag2_build_iv(uint32_t cnt, uint8_t btn, uint32_t seed, uint8_t iv[4]) {
    iv[0] = (uint8_t)(((cnt << 4U) & 0xF0U) | (btn & 0x0FU));
    iv[1] = (uint8_t)((cnt >> 4U) & 0xFFU);
    iv[2] = (uint8_t)(((seed >> 8U) & 0xF0U) | ((cnt >> 12U) & 0x0FU));
    iv[3] = (uint8_t)(seed & 0xFFU);
}

static void
    hitag2_encrypt_from_iv(const uint8_t serial_be[4], const uint8_t iv[4], uint8_t out[11]) {
    uint8_t perm[6];
    uint8_t state[6];
    uint8_t iv_work[4];
    uint8_t iv_orig[4];

    hitag2_serial_permute(serial_be, perm);
    state[0] = serial_be[0];
    state[1] = serial_be[1];
    state[2] = serial_be[2];
    state[3] = serial_be[3];
    state[4] = perm[4];
    state[5] = perm[5];
    iv_work[0] = perm[0];
    iv_work[1] = perm[1];
    iv_work[2] = perm[2];
    iv_work[3] = perm[3];
    iv_orig[0] = iv[0];
    iv_orig[1] = iv[1];
    iv_orig[2] = iv[2];
    iv_orig[3] = iv[3];
    hitag2_clock_cipher(state, iv_work, iv_orig);

    const uint8_t hop0 = iv_work[0];
    uint8_t hop1 = (uint8_t)((hop0 >> 7U) | (iv_work[1] << 1U));
    uint8_t hop2 = (uint8_t)((iv_work[1] >> 7U) | (iv_work[2] << 1U));
    const uint8_t hop_ext = (uint8_t)(((hop0 >> 6U) & 1U) | (hop1 << 1U));
    uint8_t hop3 = (uint8_t)((iv_work[2] >> 7U) | (iv_work[3] << 1U));
    uint8_t hop4 = (uint8_t)((iv_work[3] >> 7U) | (state[5] << 1U));
    hop1 = (uint8_t)((hop1 >> 7U) | (hop2 << 1U));
    hop2 = (uint8_t)((hop2 >> 7U) | (hop3 << 1U));
    hop3 = (uint8_t)((hop3 >> 7U) | (hop4 << 1U));

    uint32_t mix = ((uint32_t)iv[1] << 4U) | ((uint32_t)iv[0] >> 4U);
    mix = (mix | (((uint32_t)iv[2] << 12U) & 0xFFFFU)) & 0xFFFFU;

    out[0] = serial_be[0];
    out[1] = serial_be[1];
    out[2] = serial_be[2];
    out[3] = serial_be[3];
    out[4] = (uint8_t)(((mix >> 6U) & 0x0FU) | ((iv[0] << 4U) & 0xF0U));
    out[5] = (uint8_t)((hop3 & 3U) | ((mix << 2U) & 0xFCU));
    out[6] = hop2;
    out[7] = hop1;
    out[8] = hop_ext;
    out[9] = (uint8_t)((hop0 << 2U) | 2U);
    out[10] = hitag2_frame_xor(out);
}

static void hitag2_encrypt_frame(
    uint32_t serial,
    uint32_t cnt,
    uint8_t btn,
    uint32_t seed,
    uint8_t out[11],
    uint8_t iv[4]) {
    const uint8_t serial_be[4] = {
        (uint8_t)(serial >> 24U),
        (uint8_t)(serial >> 16U),
        (uint8_t)(serial >> 8U),
        (uint8_t)serial,
    };
    hitag2_build_iv(cnt, btn, seed, iv);
    hitag2_encrypt_from_iv(serial_be, iv, out);
}
#endif

static void hitag2_apply_check_remote(
    uint64_t data,
    uint64_t data_2,
    uint8_t recovered,
    uint32_t seed,
    uint32_t* serial,
    uint8_t* btn,
    uint32_t* cnt,
    uint32_t* hop,
    uint8_t* tail) {
    uint16_t cnt10 = 0;
    hitag2_unpack_frame(data, data_2, serial, btn, &cnt10, hop, tail);
    if(recovered == HITAG2_RECOVERED_YES) {
        *cnt = ((seed >> 12U) & 0xFF0U) | ((seed << 4U) & 0xF000U) | (seed >> 28U);
    } else {
        *cnt = cnt10;
    }
}

static void renault_v1_check_remote_controller(RfStreamProtocolDecoderRenaultV1* instance) {
    hitag2_apply_check_remote(
        instance->generic.data,
        instance->data_2,
        instance->recovered,
        instance->seed,
        &instance->generic.serial,
        &instance->generic.btn,
        &instance->generic.cnt,
        &instance->hop,
        &instance->tail_bits);
}

#if RF_STREAM_WITH_ENCODER
static bool hitag2_encoder_next_frame(
    RfStreamProtocolEncoderRenaultV1* instance,
    uint32_t orig_uid,
    uint8_t orig_btn,
    uint16_t orig_cnt10,
    uint32_t orig_hop,
    uint8_t tail) {
    uint8_t raw[11];
    if(!hitag2_key_matches_hop(instance->hitag2_key, orig_uid, orig_btn, orig_cnt10, orig_hop)) {
        return false;
    }

    uint8_t tx_btn = instance->generic.btn;
    if(tx_btn == 0U) {
        tx_btn = orig_btn;
    }
    uint32_t uid = instance->generic.serial ? instance->generic.serial : orig_uid;
    uint16_t cnt10 = (uint16_t)(instance->generic.cnt & 0x3FFU);
    uint32_t hop = hitag2_authenticator(uid, tx_btn, cnt10, instance->hitag2_key);

    hitag2_pack_auth_frame(uid, tx_btn, cnt10, hop, tail, raw);
    hitag2_apply_raw(raw, &instance->generic.data, &instance->data_2);
    instance->generic.serial = uid;
    instance->generic.btn = tx_btn;
    instance->generic.cnt = cnt10;
    instance->tail_bits = tail;
    instance->hitag2_key_valid = true;
    return true;
}
#endif

static void hitag2_write_named_fields(
    RfRecord* ff,
    uint32_t serial,
    uint8_t btn,
    uint32_t cnt,
    uint32_t hop) {
    hitag2_flipper_u32(ff, FF_SERIAL, serial);
    hitag2_flipper_u32(ff, FF_BTN, btn);
    hitag2_flipper_u32(ff, FF_CNT, cnt);
    hitag2_flipper_u32(ff, HITAG2_HOP_FIELD, hop);
}

static void hitag2_read_recovered_and_seed(
    RfRecord* flipper_format,
    uint8_t* recovered,
    uint32_t* seed) {
    *recovered = 0;
    *seed = 0;

    uint8_t recovered_hex = 0;
    if(hitag2_read_hex_be(flipper_format, "Recovered", &recovered_hex, 1)) {
        *recovered = recovered_hex;
    } else {
        uint32_t recovered_u32 = 0;
        if(rf_record_rewind(flipper_format) &&
           rf_record_read_uint32(flipper_format, "Recovered", &recovered_u32, 1)) {
            *recovered = (uint8_t)recovered_u32;
        }
    }

    uint8_t seed_be[4] = {0};
    if(hitag2_read_hex_be(flipper_format, "Seed", seed_be, 4)) {
        *seed = (uint32_t)hitag2_bytes_to_u64_be(seed_be, 4);
        return;
    }

    if(rf_record_rewind(flipper_format)) {
        rf_record_read_uint32(flipper_format, "Seed", seed, 1);
    }
}

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_renault_v1_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolEncoderRenaultV1* instance = calloc(1, sizeof(RfStreamProtocolEncoderRenaultV1));

    instance->base.protocol = &renault_v1_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->encoder.repeat = 1;
    instance->encoder.is_running = false;

    return instance;
}

static bool
    hitag2_encoder_add_level(LevelDuration* upload, size_t* index, bool level, uint32_t duration) {
    if(*index >= HITAG2_UPLOAD_CAPACITY) {
        return false;
    }
    upload[(*index)++] = level_duration_make(level, duration);
    return true;
}

static bool hitag2_encoder_add_bits(
    LevelDuration* upload,
    size_t* index,
    uint64_t value,
    uint8_t bit_count) {
    for(uint8_t i = bit_count; i > 0; i--) {
        const bool one = ((value >> (i - 1U)) & 1ULL) != 0;
        if(one) {
            if(!hitag2_encoder_add_level(upload, index, true, HITAG2_TE_US)) return false;
            if(!hitag2_encoder_add_level(upload, index, false, HITAG2_TE_US)) return false;
        } else {
            if(!hitag2_encoder_add_level(upload, index, false, HITAG2_TE_US)) return false;
            if(!hitag2_encoder_add_level(upload, index, true, HITAG2_TE_US)) return false;
        }
    }
    return true;
}

static bool renault_v1_encoder_get_upload(RfStreamProtocolEncoderRenaultV1* instance) {
    size_t index = 0;
    LevelDuration* upload = instance->encoder.upload;
    const uint64_t key = instance->generic.data;
    const uint64_t key_2 = instance->data_2 & 0xFFFFFFULL;

    for(size_t i = 0; i < HITAG2_PREAMBLE_PAIRS; i++) {
        if(!hitag2_encoder_add_level(upload, &index, true, HITAG2_TE_US)) return false;
        if(!hitag2_encoder_add_level(upload, &index, false, HITAG2_TE_US)) return false;
    }

    for(uint8_t frame = 0; frame < HITAG2_LONG_FRAMES; frame++) {
        if(!hitag2_encoder_add_level(upload, &index, false, HITAG2_HEADER_LOW_US)) return false;
        if(!hitag2_encoder_add_level(upload, &index, true, HITAG2_HEADER_HIGH_US)) return false;
        if(!hitag2_encoder_add_bits(upload, &index, 1, HITAG2_HEADER_BITS)) return false;
        if(!hitag2_encoder_add_bits(upload, &index, key, HITAG2_KEY_BITS)) return false;
        if(!hitag2_encoder_add_bits(upload, &index, key_2, HITAG2_KEY2_BITS)) return false;
    }

    const uint64_t short_key = (key >> 18U) & 0x3FFULL;
    for(uint8_t frame = 0; frame < HITAG2_SHORT_FRAMES; frame++) {
        if(!hitag2_encoder_add_level(upload, &index, false, HITAG2_HEADER_LOW_US)) return false;
        if(!hitag2_encoder_add_level(upload, &index, true, HITAG2_HEADER_HIGH_US)) return false;
        if(!hitag2_encoder_add_bits(upload, &index, 1, HITAG2_HEADER_BITS)) return false;
        if(!hitag2_encoder_add_bits(upload, &index, short_key, HITAG2_SHORT_KEY_BITS))
            return false;
        if(!hitag2_encoder_add_level(upload, &index, false, HITAG2_SHORT_GAP_US)) return false;
    }

    instance->encoder.size_upload = index;
    return true;
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_renault_v1_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderRenaultV1* instance = context;
    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    do {
        ret = rf_stream_block_generic_deserialize_check_count_bit(
            &instance->generic, flipper_format, renault_v1_const.min_count_bit_for_found);
        if(ret != RfStreamProtocolStatusOk) {
            break;
        }

        uint8_t key2[8] = {0};
        if(!hitag2_read_hex_be(flipper_format, "Key_2", key2, sizeof(key2))) {
            ret = RfStreamProtocolStatusErrorParserOthers;
            break;
        }
        instance->data_2 = hitag2_bytes_to_u64_be(key2, sizeof(key2));

        hitag2_read_recovered_and_seed(flipper_format, &instance->recovered, &instance->seed);

        if(!rf_record_rewind(flipper_format)) {
            ret = RfStreamProtocolStatusErrorParserOthers;
            break;
        }

        uint32_t hop = 0;
        uint8_t tail = 0;
        uint32_t wire_serial = 0;
        uint8_t wire_btn = 0;
        uint16_t wire_cnt10 = 0;
        hitag2_unpack_frame(
            instance->generic.data,
            instance->data_2,
            &wire_serial,
            &wire_btn,
            &wire_cnt10,
            &hop,
            &tail);
        hitag2_apply_check_remote(
            instance->generic.data,
            instance->data_2,
            instance->recovered,
            instance->seed,
            &instance->generic.serial,
            &instance->generic.btn,
            &instance->generic.cnt,
            &hop,
            &tail);
        instance->tail_bits = tail;

        const uint32_t orig_uid = wire_serial;
        const uint8_t orig_btn = wire_btn;
        const uint16_t orig_cnt10 = wire_cnt10;
        const uint32_t orig_hop = hop;

        uint32_t serial = instance->generic.serial;
        uint32_t btn = instance->generic.btn;
        uint32_t cnt = instance->generic.cnt;
        rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);
        instance->generic.serial = serial;
        instance->generic.btn = (uint8_t)btn;
        instance->generic.cnt = cnt;

        instance->hitag2_key_valid = false;
        memset(instance->hitag2_key, 0, 6U);
        const bool have_hitag2_key = hitag2_read_key(flipper_format, instance->hitag2_key);
        if(have_hitag2_key) {
            if(!hitag2_encoder_next_frame(
                   instance, orig_uid, orig_btn, orig_cnt10, orig_hop, tail) &&
               instance->recovered != HITAG2_RECOVERED_YES) {
                ret = RfStreamProtocolStatusErrorParserOthers;
                break;
            }
        }

        instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, 1U);
        if(instance->encoder.repeat == 0U) {
            instance->encoder.repeat = 1U;
        }

        if(!instance->hitag2_key_valid && instance->recovered == HITAG2_RECOVERED_YES) {
            uint8_t out[11];
            uint8_t iv[4];
            hitag2_encrypt_frame(
                instance->generic.serial,
                instance->generic.cnt,
                instance->generic.btn,
                instance->seed,
                out,
                iv);
            instance->generic.data = hitag2_bytes_to_u64_be(out, 8);
            instance->data_2 = ((uint64_t)out[8] << 16U) | ((uint64_t)out[9] << 8U) | out[10];
            instance->seed = ((uint32_t)iv[0] << 24U) | ((uint32_t)iv[1] << 16U) |
                             ((uint32_t)iv[2] << 8U) | iv[3];
        } else if(!instance->hitag2_key_valid && instance->recovered != HITAG2_RECOVERED_YES) {
            ret = RfStreamProtocolStatusErrorParserOthers;
            break;
        }

        rf_stream_encoder_buffer_ensure(instance, HITAG2_UPLOAD_CAPACITY);

        if(!renault_v1_encoder_get_upload(instance)) {
            ret = RfStreamProtocolStatusErrorEncoderGetUpload;
            break;
        }

        if(!rf_record_rewind(flipper_format)) {
            ret = RfStreamProtocolStatusErrorParserOthers;
            break;
        }
        uint8_t key_data[8];
        hitag2_u64_to_bytes_be(instance->generic.data, key_data, 8);
        if(!rf_record_insert_or_update_hex(
               flipper_format, FF_KEY, key_data, sizeof(key_data))) {
            ret = RfStreamProtocolStatusErrorParserOthers;
            break;
        }
        hitag2_u64_to_bytes_be(instance->data_2, key_data, 8);
        if(!rf_record_insert_or_update_hex(
               flipper_format, "Key_2", key_data, sizeof(key_data))) {
            ret = RfStreamProtocolStatusErrorParserOthers;
            break;
        }
        if(instance->recovered == HITAG2_RECOVERED_YES) {
            const uint8_t recovered_hex = HITAG2_RECOVERED_YES;
            if(!rf_record_insert_or_update_hex(
                   flipper_format, "Recovered", &recovered_hex, 1)) {
                ret = RfStreamProtocolStatusErrorParserOthers;
                break;
            }
            if(!hitag2_write_hex_be(flipper_format, "Seed", instance->seed, 4)) {
                ret = RfStreamProtocolStatusErrorParserOthers;
                break;
            }
        }

        {
            uint32_t hop = 0;
            hitag2_unpack_frame(
                instance->generic.data, instance->data_2, NULL, NULL, NULL, &hop, NULL);
            hitag2_write_named_fields(
                flipper_format,
                instance->generic.serial,
                instance->generic.btn,
                instance->generic.cnt,
                hop);
        }
        if(instance->hitag2_key_valid) {
            rf_record_rewind(flipper_format);
            rf_record_insert_or_update_hex(
                flipper_format, HITAG2_KEY_FIELD, instance->hitag2_key, 6U);
            hitag2_flipper_u32(flipper_format, HITAG2_EPOCH_FIELD, 0U);
        }

        instance->encoder.front = 0;
        instance->encoder.is_running = true;
        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

#endif

void* rf_stream_protocol_decoder_renault_v1_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderRenaultV1* instance = calloc(1, sizeof(RfStreamProtocolDecoderRenaultV1));
    instance->base.protocol = &renault_v1_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;

    return instance;
}

void rf_stream_protocol_decoder_renault_v1_reset(void* context) {
    RfStreamProtocolDecoderRenaultV1* instance = context;
    instance->decoder.parser_step = RenaultV1DecoderStepReset;
    rf_manchester_advance(
        instance->manchester_state, RfManchesterEventReset, &instance->manchester_state, NULL);
}

static bool hitag2_duration_is_header_low(uint32_t duration) {
    return (duration >= HITAG2_HEADER_LOW_MIN_US) && (duration <= HITAG2_HEADER_LOW_MAX_US);
}

static uint32_t hitag2_data_threshold(uint16_t te) {
    const uint32_t triple = (uint32_t)te * 3U;
    if(triple < 300U) {
        return 150U;
    }
    if(triple >= 422U) {
        return 210U;
    }
    return triple / 2U;
}

static uint16_t hitag2_adapt_te(uint16_t te, uint32_t duration) {
    const uint32_t mixed = ((uint32_t)te * 7U) + duration;
    if(mixed < 560U) {
        return 70;
    }
    if(mixed >= 1488U) {
        return 185;
    }
    return (uint16_t)(mixed / 8U);
}

static bool hitag2_accept_frame(RfStreamProtocolDecoderRenaultV1* instance, uint64_t key_2) {
    if(instance->header != 1U) {
        return false;
    }

    uint8_t raw[11];
    hitag2_pack_key_bytes(instance->generic.data, key_2, raw);
    if(hitag2_frame_xor(raw) != raw[10]) {
        return false;
    }

    if(instance->last_frame_valid && instance->last_data == instance->generic.data &&
       instance->last_data_2 == key_2) {
        return false;
    }

    instance->data_2 = key_2;
    instance->generic.data_count_bit = HITAG2_MIN_COUNT_BIT;
    instance->recovered = 0;
    instance->seed = 0;
    uint16_t cnt10 = 0;
    hitag2_unpack_frame(
        instance->generic.data,
        instance->data_2,
        &instance->generic.serial,
        &instance->generic.btn,
        &cnt10,
        &instance->hop,
        &instance->tail_bits);
    instance->generic.cnt = cnt10;
    instance->last_data = instance->generic.data;
    instance->last_data_2 = key_2;
    instance->last_frame_valid = true;
    return true;
}

void rf_stream_protocol_decoder_renault_v1_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderRenaultV1* instance = context;

    while(true) {
        switch(instance->decoder.parser_step) {
        case RenaultV1DecoderStepReset:
            if((!level) && hitag2_duration_is_header_low(duration)) {
                instance->decoder.te_last = duration;
                instance->decoder.parser_step = RenaultV1DecoderStepCheckSync;
            }
            return;

        case RenaultV1DecoderStepCheckSync:
            if(level) {
                if((duration < HITAG2_HEADER_HIGH_MIN_US) ||
                   (duration > HITAG2_HEADER_HIGH_MAX_US) ||
                   (instance->decoder.te_last < HITAG2_HEADER_LOW_MIN_US) ||
                   (instance->decoder.te_last > HITAG2_HEADER_LOW_MAX_US)) {
                    instance->decoder.parser_step = RenaultV1DecoderStepReset;
                    return;
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->manchester_state = RfManchesterStateStart1;
                instance->te_high = HITAG2_TE_HIGH_INIT_US;
                instance->te_low = HITAG2_TE_LOW_INIT_US;
                instance->decoder.parser_step = RenaultV1DecoderStepData;
                return;
            }
            instance->decoder.parser_step = RenaultV1DecoderStepReset;
            if(hitag2_duration_is_header_low(duration)) {
                instance->decoder.te_last = duration;
                instance->decoder.parser_step = RenaultV1DecoderStepCheckSync;
            }
            return;

        case RenaultV1DecoderStepData:
            if(duration <= HITAG2_DATA_IGNORE_US) {
                return;
            }
            if(duration > HITAG2_DATA_RESET_US) {
                instance->decoder.parser_step = RenaultV1DecoderStepReset;
                continue;
            }

            uint16_t* te = level ? &instance->te_high : &instance->te_low;
            RfManchesterEvent event;
            if(duration > hitag2_data_threshold(*te)) {
                event = level ? RfManchesterEventLongHigh : RfManchesterEventLongLow;
            } else {
                event = level ? RfManchesterEventShortHigh : RfManchesterEventShortLow;
                *te = hitag2_adapt_te(*te, duration);
            }

            bool bit = false;
            if(!rf_manchester_advance(
                   instance->manchester_state, event, &instance->manchester_state, &bit)) {
                return;
            }

            instance->decoder.decode_data = (instance->decoder.decode_data << 1U) |
                                            (bit ? 1ULL : 0ULL);
            instance->decoder.decode_count_bit++;

            if(instance->decoder.decode_count_bit == HITAG2_HEADER_BITS) {
                instance->header = (uint16_t)~instance->decoder.decode_data;
                instance->decoder.decode_data = 0;
                return;
            }
            if(instance->decoder.decode_count_bit == HITAG2_KEY_END_BITS) {
                instance->generic.data = ~instance->decoder.decode_data;
                instance->decoder.decode_data = 0;
                return;
            }
            if(instance->decoder.decode_count_bit == HITAG2_LONG_FRAME_BITS) {
                const uint64_t key_2 = (~instance->decoder.decode_data) & 0xFFFFFFULL;
                if(hitag2_accept_frame(instance, key_2) && instance->base.callback) {
                    instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->decoder.parser_step = RenaultV1DecoderStepData;
            }
            return;

        default:
            instance->decoder.parser_step = RenaultV1DecoderStepReset;
            return;
        }
    }
}

uint8_t rf_stream_protocol_decoder_renault_v1_get_hash_data(void* context) {
    RfStreamProtocolDecoderRenaultV1* instance = context;
    return (uint8_t)instance->generic.serial ^ (uint8_t)(instance->generic.serial >> 8) ^
           (uint8_t)(instance->generic.serial >> 16) ^ (uint8_t)(instance->generic.serial >> 24) ^
           instance->generic.btn ^ (uint8_t)instance->generic.cnt ^
           (uint8_t)(instance->generic.cnt >> 8);
}

static RfStreamProtocolStatus hitag2_write_extra_fields(
    RfRecord* flipper_format,
    uint64_t key_2,
    uint8_t recovered,
    uint32_t seed) {
    if(!hitag2_write_hex_be(flipper_format, "Key_2", key_2, 8)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }
    uint8_t recovered_hex = 0;
    if(recovered == HITAG2_RECOVERED_YES) {
        recovered_hex = HITAG2_RECOVERED_YES;
    } else if(recovered == HITAG2_RECOVERED_BF_MISS) {
        recovered_hex = HITAG2_RECOVERED_BF_MISS;
    }
    if(!rf_record_insert_or_update_hex(flipper_format, "Recovered", &recovered_hex, 1)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }
    if(!hitag2_write_hex_be(flipper_format, "Seed", seed, 4)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }
    return RfStreamProtocolStatusOk;
}

RfStreamProtocolStatus rf_stream_protocol_decoder_renault_v1_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderRenaultV1* instance = context;
    renault_v1_check_remote_controller(instance);

    const uint32_t serial = instance->generic.serial;
    const uint8_t btn = instance->generic.btn;
    const uint32_t cnt = instance->generic.cnt;
    const uint32_t seed = instance->seed;
    const uint64_t data_2 = instance->data_2;
    instance->generic.serial = 0;
    instance->generic.btn = 0;
    instance->generic.cnt = 0;
    instance->seed = 0;
    instance->data_2 = 0;
    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
    instance->generic.serial = serial;
    instance->generic.btn = btn;
    instance->generic.cnt = cnt;
    instance->seed = seed;
    instance->data_2 = data_2;
    if(ret != RfStreamProtocolStatusOk) {
        return ret;
    }

    ret = hitag2_write_extra_fields(
        flipper_format, instance->data_2, instance->recovered, instance->seed);
    if(ret != RfStreamProtocolStatusOk) {
        return ret;
    }
    hitag2_write_named_fields(flipper_format, serial, btn, cnt, instance->hop);
    if(instance->hitag2_key_valid) {
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_hex(
            flipper_format, HITAG2_KEY_FIELD, instance->hitag2_key, 6U);
        hitag2_flipper_u32(flipper_format, HITAG2_EPOCH_FIELD, 0U);
    }
    return RfStreamProtocolStatusOk;
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_renault_v1_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderRenaultV1* instance = context;
    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    do {
        ret = rf_stream_block_generic_deserialize_check_count_bit(
            &instance->generic, flipper_format, renault_v1_const.min_count_bit_for_found);
        if(ret != RfStreamProtocolStatusOk) {
            break;
        }

        uint8_t key2[8] = {0};
        if(!hitag2_read_hex_be(flipper_format, "Key_2", key2, sizeof(key2))) {
            ret = RfStreamProtocolStatusErrorParserOthers;
            break;
        }
        instance->data_2 = hitag2_bytes_to_u64_be(key2, sizeof(key2));

        hitag2_read_recovered_and_seed(flipper_format, &instance->recovered, &instance->seed);

        renault_v1_check_remote_controller(instance);
        instance->hitag2_key_valid = false;
        memset(instance->hitag2_key, 0, 6U);
        if(hitag2_read_key(flipper_format, instance->hitag2_key)) {
            instance->hitag2_key_valid = hitag2_key_matches_hop(
                instance->hitag2_key,
                instance->generic.serial,
                instance->generic.btn,
                (uint16_t)(instance->generic.cnt & 0x3FFU),
                instance->hop);
        }
        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

void rf_stream_protocol_decoder_renault_v1_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderRenaultV1* instance = context;

    renault_v1_check_remote_controller(instance);

    rf_text_printf(
        output,
        "%s\r\nK1:%016llX\r\nK2:%06llX Sn:%08lX\r\nBtn:%02X [%s] %db",
        instance->generic.protocol_name,
        (unsigned long long)instance->generic.data,
        (unsigned long long)(instance->data_2 & 0xFFFFFFULL),
        (unsigned long)instance->generic.serial,
        instance->generic.btn,
        hitag2_get_button_name(instance->generic.btn),
        instance->generic.data_count_bit);

    if(instance->recovered == HITAG2_RECOVERED_YES) {
        rf_text_cat_printf(
            output,
            "\r\nIV:%08lX Cnt:%04lX",
            (unsigned long)instance->seed,
            (unsigned long)(instance->generic.cnt & 0xFFFFU));
    } else if(instance->recovered == HITAG2_RECOVERED_BF_MISS) {
        rf_text_cat_printf(output, "\r\nBF not found");
    } else {
        rf_text_cat_printf(
            output,
            "\r\nCnt:%03lX Hop:%08lX",
            (unsigned long)(instance->generic.cnt & 0x3FFU),
            (unsigned long)instance->hop);
        if(instance->hitag2_key_valid) {
            rf_text_cat_printf(output, "\r\nKEY:OK");
        } else {
            rf_text_cat_printf(output, "\r\nKEY:??");
        }
    }
}

#define HITAG2_BF_PROGRESS_INTERVAL 0x400U
#define HITAG2_BF_CANDIDATES        0x40000U

static uint32_t hitag2_seed_from_iv(const uint8_t iv[4]) {
    return ((uint32_t)iv[0] << 24U) | ((uint32_t)iv[1] << 16U) | ((uint32_t)iv[2] << 8U) | iv[3];
}

static void hitag2_bf_rearrange_dest(uint8_t dest[11]) {
    const uint8_t frame4 = dest[6];
    uint8_t frame5 = dest[5];
    uint8_t frame6 = dest[4];
    uint8_t frame7 = dest[3];
    const uint8_t frame8 = dest[2];
    const uint8_t frame9 = dest[1];

    uint8_t hop0 = (uint8_t)(((frame4 & 1U) << 7U) | (frame5 >> 1U));
    frame5 = (uint8_t)(((frame5 & 1U) << 7U) | (frame6 >> 1U));
    const uint8_t hop0_hi = (uint8_t)(hop0 >> 1U);
    hop0 = (uint8_t)(hop0 & 1U);
    hop0 = (uint8_t)((hop0 << 7U) | (frame5 >> 1U));
    frame6 = (uint8_t)(((frame6 & 1U) << 7U) | (frame7 >> 1U));
    frame5 = (uint8_t)(((frame5 & 1U) << 7U) | (frame6 >> 1U));
    dest[3] = frame5;
    frame7 = (uint8_t)(((frame7 & 1U) << 7U) | (frame8 >> 1U));
    const uint8_t hop2 = (uint8_t)(((frame6 & 1U) << 7U) | (frame7 >> 1U));
    frame6 = (uint8_t)(((frame8 & 1U) << 7U) | (frame9 >> 1U));
    dest[6] = (uint8_t)(((frame9 & 1U) << 7U) | (frame4 >> 2U));
    dest[1] = (uint8_t)(((frame7 & 1U) << 7U) | (frame6 >> 1U));
    dest[5] = (uint8_t)(hop0_hi | (((frame4 >> 1U) & 1U) << 7U));
    dest[2] = hop2;
    dest[4] = hop0;
}

static void hitag2_bf_prepare(
    const uint8_t frame[11],
    uint8_t serial_be[4],
    uint8_t perm[6],
    uint8_t hop_target[4],
    uint8_t* iv0,
    uint8_t* fp) {
    uint8_t dest[11];
    for(size_t i = 0; i < 11; i++) {
        dest[i] = frame[10U - i];
    }
    hitag2_bf_rearrange_dest(dest);

    hop_target[0] = dest[1];
    hop_target[1] = dest[2];
    hop_target[2] = dest[3];
    hop_target[3] = dest[4];
    *iv0 = (uint8_t)((frame[4] >> 4U) | (dest[5] << 4U));
    *fp = (uint8_t)((dest[5] >> 4U) | ((dest[6] & 3U) << 4U));

    serial_be[0] = frame[0];
    serial_be[1] = frame[1];
    serial_be[2] = frame[2];
    serial_be[3] = frame[3];
    hitag2_serial_permute(serial_be, perm);
}

static bool hitag2_bf_hop_matches(
    const uint8_t serial_be[4],
    const uint8_t perm[6],
    const uint8_t iv[4],
    const uint8_t hop_target[4]) {
    uint8_t state[6];
    uint8_t iv_work[4];
    uint8_t iv_orig[4];

    state[0] = serial_be[0];
    state[1] = serial_be[1];
    state[2] = serial_be[2];
    state[3] = serial_be[3];
    state[4] = perm[4];
    state[5] = perm[5];
    iv_work[0] = perm[0];
    iv_work[1] = perm[1];
    iv_work[2] = perm[2];
    iv_work[3] = perm[3];
    iv_orig[0] = iv[0];
    iv_orig[1] = iv[1];
    iv_orig[2] = iv[2];
    iv_orig[3] = iv[3];
    hitag2_clock_cipher(state, iv_work, iv_orig);

    return (iv_work[0] == hop_target[0]) && (iv_work[1] == hop_target[1]) &&
           (iv_work[2] == hop_target[2]) && (iv_work[3] == hop_target[3]);
}

static void hitag2_brute_force_run(Hitag2BfState* state) {
    uint8_t serial_be[4];
    uint8_t perm[6];
    uint8_t hop_target[4];
    uint8_t iv0 = 0;
    uint8_t fp = 0;
    hitag2_bf_prepare(state->frame, serial_be, perm, hop_target, &iv0, &fp);

    state->progress_current = 0;
    state->progress_total = HITAG2_BF_CANDIDATES;
    state->status = HITAG2_BF_STATUS_RUNNING;

    uint8_t iv[4];
    for(uint32_t cand = 0; cand < HITAG2_BF_CANDIDATES; cand++) {
        if((cand & (HITAG2_BF_PROGRESS_INTERVAL - 1U)) == 0U) {
            state->progress_current = cand;
            if(state->cancel) {
                state->status = HITAG2_BF_STATUS_CANCELLED;
                return;
            }
        }

        iv[0] = iv0;
        iv[1] = (uint8_t)(fp | ((cand & 3U) << 6U));
        iv[2] = (uint8_t)(cand >> 2U);
        iv[3] = (uint8_t)(cand >> 10U);
        if(hitag2_bf_hop_matches(serial_be, perm, iv, hop_target)) {
            state->iv[0] = iv[0];
            state->iv[1] = iv[1];
            state->iv[2] = iv[2];
            state->iv[3] = iv[3];
            state->progress_current = HITAG2_BF_CANDIDATES;
            state->status = HITAG2_BF_STATUS_FOUND;
            return;
        }
    }

    state->progress_current = HITAG2_BF_CANDIDATES;
    state->status = state->cancel ? HITAG2_BF_STATUS_CANCELLED : HITAG2_BF_STATUS_NOT_FOUND;
}

int32_t hitag2_brute_force_thread_entry(void* arg) {
    Hitag2BfState* state = arg;
    hitag2_brute_force_run(state);
    if(state->on_done) {
        state->on_done(state->on_done_ctx);
    }
    return 0;
}

bool hitag2_bf_state_from_flipper_format(Hitag2BfState* state, RfRecord* ff) {
    uint8_t key1[8] = {0};
    uint8_t key2[8] = {0};
    if(!hitag2_read_hex_be(ff, FF_KEY, key1, sizeof(key1))) {
        return false;
    }
    if(!hitag2_read_hex_be(ff, "Key_2", key2, sizeof(key2))) {
        return false;
    }

    for(size_t i = 0; i < 8; i++) {
        state->frame[i] = key1[i];
    }
    state->frame[8] = key2[5];
    state->frame[9] = key2[6];
    state->frame[10] = key2[7];

    state->cancel = 0;
    state->progress_current = 0;
    state->progress_total = HITAG2_BF_CANDIDATES;
    state->status = HITAG2_BF_STATUS_IDLE;
    state->on_done = NULL;
    state->on_done_ctx = NULL;
    state->iv[0] = 0;
    state->iv[1] = 0;
    state->iv[2] = 0;
    state->iv[3] = 0;
    return true;
}

bool hitag2_bf_needs_bruteforce(RfRecord* ff) {
    if(!ff) {
        return false;
    }

    RfText* s = rf_text_alloc();
    rf_record_rewind(ff);
    if(!rf_record_read_string(ff, FF_PROTOCOL, s) ||
       rf_text_cmp_str(s, RENAULT_PROTOCOL_V1_NAME) != 0) {
        rf_text_free(s);
        return false;
    }
    rf_record_rewind(ff);

    bool has_key = rf_record_read_string(ff, FF_KEY, s);
    rf_text_free(s);
    if(!has_key) {
        return false;
    }

    uint8_t recovered = 0;
    uint32_t seed = 0;
    hitag2_read_recovered_and_seed(ff, &recovered, &seed);
    UNUSED(seed);
    if(recovered != 0) {
        return false;
    }
    uint8_t stored_key[6];
    if(hitag2_read_key(ff, stored_key)) {
        return false;
    }
    return true;
}

bool hitag2_bf_patch_flipper_format_on_success(RfRecord* ff, const Hitag2BfState* state) {
    if(!ff || !state) {
        return false;
    }

    const uint32_t seed = hitag2_seed_from_iv(state->iv);
    const uint64_t data = hitag2_bytes_to_u64_be(state->frame, 8);
    const uint64_t data_2 = ((uint64_t)state->frame[8] << 16U) |
                            ((uint64_t)state->frame[9] << 8U) | state->frame[10];
    uint32_t serial = 0;
    uint32_t cnt = 0;
    uint32_t hop = 0;
    uint8_t btn = 0;
    uint8_t tail = 0;
    hitag2_apply_check_remote(
        data, data_2, HITAG2_RECOVERED_YES, seed, &serial, &btn, &cnt, &hop, &tail);

    rf_record_rewind(ff);
    const uint8_t recovered_hex = HITAG2_RECOVERED_YES;
    if(!rf_record_insert_or_update_hex(ff, "Recovered", &recovered_hex, 1)) {
        return false;
    }
    if(!hitag2_write_hex_be(ff, "Seed", seed, 4)) {
        return false;
    }
    hitag2_write_named_fields(ff, serial, btn, cnt, hop);
    return true;
}

bool hitag2_bf_patch_flipper_format_on_miss(RfRecord* ff) {
    if(!ff) {
        return false;
    }

    rf_record_rewind(ff);
    const uint8_t recovered_hex = HITAG2_RECOVERED_BF_MISS;
    return rf_record_insert_or_update_hex(ff, "Recovered", &recovered_hex, 1);
}

bool hitag2_flipper_format_get_string(RfRecord* ff, RfText* output) {
    if(!ff || !output) {
        return false;
    }

    RfStreamProtocolDecoderRenaultV1* decoder = rf_stream_protocol_decoder_renault_v1_alloc(NULL);
    if(!decoder) {
        return false;
    }

    const bool ok = rf_stream_protocol_decoder_renault_v1_deserialize(decoder, ff) ==
                    RfStreamProtocolStatusOk;
    if(ok) {
        rf_text_reset(output);
        rf_stream_protocol_decoder_renault_v1_get_string(decoder, output);
    }
    rf_stream_decoder_free_default(decoder);
    return ok;
}

#endif
