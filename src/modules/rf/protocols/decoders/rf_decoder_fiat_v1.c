// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define FIAT_V1_PROTOCOL_NAME "Fiat V1"

typedef struct RfStreamProtocolDecoderFiatV1 RfStreamProtocolDecoderFiatV1;
typedef struct RfStreamProtocolEncoderFiatV1 RfStreamProtocolEncoderFiatV1;

extern const RfStreamCodec fiat_v1_protocol;

void* rf_stream_protocol_decoder_fiat_v1_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_fiat_v1_reset(void* context);
void rf_stream_protocol_decoder_fiat_v1_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_fiat_v1_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_fiat_v1_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_fiat_v1_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_fiat_v1_get_string(void* context, RfText* output);

void* rf_stream_protocol_encoder_fiat_v1_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_fiat_v1_deserialize(void* context, RfRecord* flipper_format);



#include <string.h>

#define TAG "FiatProtocolV1"

#define FIAT_V1_TE_SHORT           250U
#define FIAT_V1_TE_LONG            500U
#define FIAT_V1_TE_DELTA           100U
#define FIAT_V1_TE_B_SHORT         100U
#define FIAT_V1_TE_B_LONG          200U
#define FIAT_V1_TE_B_DELTA         50U
#define FIAT_V1_WIRE_BITS          104U
#define FIAT_V1_WIRE_BYTES         13U
#define FIAT_V1_WIRE_CELLS         (FIAT_V1_WIRE_BITS * 2U)
#define FIAT_V1_LOGICAL_BITS       102U
#define FIAT_V1_VARIANT_COUNT      2U
#define FIAT_V1_BOUNDARY_MIN_US    800U
#define FIAT_V1_DEFAULT_TAIL_BITS  2U
#define FIAT_V1_RAW_FIELD          "Raw"
#define FIAT_V1_HOP_FIELD          "Hop"
#define FIAT_V1_TAIL_BITS_FIELD    "Tail Bits"
#define FIAT_V1_XOR_FIELD          "XOR"
#define FIAT_V1_HITAG2_KEY_FIELD   "Hitag2 Key"
#define FIAT_V1_HITAG2_EPOCH_FIELD "Hitag2 Epoch"
#define FIAT_V1_KNOWN_KEY_COUNT    9U

#define FIAT_V1_TE_VARIANT_A       0U
#define FIAT_V1_TE_VARIANT_B       1U
#define FIAT_V1_DEFAULT_TE_VARIANT FIAT_V1_TE_VARIANT_B
#define FIAT_V1_ENC_LEAD_A_US      2033U
#define FIAT_V1_ENC_GAP_A_US       3252U
#define FIAT_V1_ENC_LEAD_B_US      850U
#define FIAT_V1_ENC_GAP_B_US       1230U
#define FIAT_V1_ENC_DEFAULT_REPEAT 6U
#define FIAT_V1_UPLOAD_CAPACITY    240U
_Static_assert(
    FIAT_V1_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "FIAT_V1_UPLOAD_CAPACITY exceeds shared upload slab");

static const RfStreamBlockConst rf_stream_protocol_fiat_v1_const = {
    .te_short = FIAT_V1_TE_SHORT,
    .te_long = FIAT_V1_TE_LONG,
    .te_delta = FIAT_V1_TE_DELTA,
    .min_count_bit_for_found = FIAT_V1_LOGICAL_BITS,
};

static const RfStreamBlockConst rf_stream_protocol_fiat_v1_const_b = {
    .te_short = FIAT_V1_TE_B_SHORT,
    .te_long = FIAT_V1_TE_B_LONG,
    .te_delta = FIAT_V1_TE_B_DELTA,
    .min_count_bit_for_found = FIAT_V1_LOGICAL_BITS,
};

static const RfStreamBlockConst* fiat_v1_variant_const(uint8_t variant) {
    return (variant == FIAT_V1_TE_VARIANT_A) ? &rf_stream_protocol_fiat_v1_const :
                                               &rf_stream_protocol_fiat_v1_const_b;
}

#if RF_STREAM_WITH_ENCODER
static uint32_t fiat_v1_enc_lead_us(uint8_t variant) {
    return (variant == FIAT_V1_TE_VARIANT_A) ? FIAT_V1_ENC_LEAD_A_US : FIAT_V1_ENC_LEAD_B_US;
}

static uint32_t fiat_v1_enc_gap_us(uint8_t variant) {
    return (variant == FIAT_V1_TE_VARIANT_A) ? FIAT_V1_ENC_GAP_A_US : FIAT_V1_ENC_GAP_B_US;
}

static uint32_t fiat_v1_enc_te_us(uint8_t variant) {
    return fiat_v1_variant_const(variant)->te_short;
}
#endif

typedef enum {
    FiatV1DecoderStepReset = 0,
    FiatV1DecoderStepData = 1,
} FiatV1DecoderStep;

struct RfStreamProtocolDecoderFiatV1 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    uint8_t cells[FIAT_V1_VARIANT_COUNT][FIAT_V1_WIRE_CELLS];
    uint16_t cell_count[FIAT_V1_VARIANT_COUNT];

    uint8_t raw_data[FIAT_V1_WIRE_BYTES];
    uint8_t last_raw_data[FIAT_V1_WIRE_BYTES];
    bool last_raw_valid;

    uint32_t uid;
    uint32_t hop;
    uint8_t family;
    uint8_t tail_bits;
    uint8_t frame_xor;

    uint8_t hitag2_key[6];
    uint32_t hitag2_epoch;
    bool hitag2_key_valid;
    uint8_t te_variant;
};

#if RF_STREAM_WITH_ENCODER
struct RfStreamProtocolEncoderFiatV1 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;

    uint8_t raw_data[FIAT_V1_WIRE_BYTES];
    uint8_t hitag2_key[6];
    uint32_t epoch;
    uint32_t hop;
    uint8_t tail_bits;
    uint8_t frame_xor;
    uint8_t te_variant;
};
#endif

static bool
    fiat_v1_feed_data_pulse(RfStreamProtocolDecoderFiatV1* instance, bool level, uint32_t duration);
static bool fiat_v1_frame_valid(const uint8_t raw[FIAT_V1_WIRE_BYTES]);
static void fiat_v1_build_raw(
    uint8_t raw[FIAT_V1_WIRE_BYTES],
    uint32_t uid,
    uint8_t button,
    uint16_t control,
    uint32_t auth,
    uint8_t tail_bits);
static void fiat_v1_verify_hitag2_key(RfStreamProtocolDecoderFiatV1* instance);

const RfStreamProtocolDecoder rf_stream_protocol_fiat_v1_decoder = {
    .alloc = rf_stream_protocol_decoder_fiat_v1_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_fiat_v1_feed,
    .reset = rf_stream_protocol_decoder_fiat_v1_reset,
    .get_hash_data = rf_stream_protocol_decoder_fiat_v1_get_hash_data,
    .serialize = rf_stream_protocol_decoder_fiat_v1_serialize,
    .deserialize = rf_stream_protocol_decoder_fiat_v1_deserialize,
    .get_string = rf_stream_protocol_decoder_fiat_v1_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_fiat_v1_encoder = {
    .alloc = rf_stream_protocol_encoder_fiat_v1_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = rf_stream_protocol_encoder_fiat_v1_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_fiat_v1_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec fiat_v1_protocol = {
    .name = FIAT_V1_PROTOCOL_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_fiat_v1_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_fiat_v1_encoder,
#else
    .encoder = NULL,
#endif
};

static bool fiat_v1_duration_is_short(uint8_t variant, uint32_t duration) {
    return rf_stream_is_short(duration, fiat_v1_variant_const(variant));
}

static bool fiat_v1_duration_is_long(uint8_t variant, uint32_t duration) {
    return rf_stream_is_long(duration, fiat_v1_variant_const(variant));
}

static bool fiat_v1_duration_is_pulse(uint32_t duration) {
    for(uint8_t variant = 0U; variant < FIAT_V1_VARIANT_COUNT; variant++) {
        if(fiat_v1_duration_is_short(variant, duration) ||
           fiat_v1_duration_is_long(variant, duration)) {
            return true;
        }
    }
    return false;
}

static bool fiat_v1_button_valid(uint8_t button) {
    return button == 0x1U || button == 0x2U || button == 0x4U || button == 0x8U;
}

static const char* fiat_v1_button_name(uint8_t button) {
    switch(button) {
    case 0x8U:
        return "Unlock";
    case 0x4U:
        return "Lock";
    case 0x2U:
        return "Trunk";
    case 0x1U:
        return "Close";
    default:
        return "Unknown";
    }
}

static uint8_t fiat_v1_frame_xor(const uint8_t raw[FIAT_V1_WIRE_BYTES]) {
    uint8_t value = 0x01U;
    for(uint8_t i = 0U; i < FIAT_V1_WIRE_BYTES - 1U; i++) {
        value ^= raw[i];
    }
    return value;
}

static void fiat_v1_build_raw(
    uint8_t raw[FIAT_V1_WIRE_BYTES],
    uint32_t uid,
    uint8_t button,
    uint16_t control,
    uint32_t auth,
    uint8_t tail_bits) {
    memset(raw, 0, FIAT_V1_WIRE_BYTES);
    raw[1] = 0x01U;
    raw[2] = (uint8_t)(uid >> 24U);
    raw[3] = (uint8_t)(uid >> 16U);
    raw[4] = (uint8_t)(uid >> 8U);
    raw[5] = (uint8_t)uid;
    raw[6] = (uint8_t)(((button & 0x0FU) << 4U) | ((control >> 6U) & 0x0FU));
    raw[7] = (uint8_t)(((control & 0x3FU) << 2U) | ((auth >> 30U) & 0x03U));
    raw[8] = (uint8_t)(auth >> 22U);
    raw[9] = (uint8_t)(auth >> 14U);
    raw[10] = (uint8_t)(auth >> 6U);
    raw[11] = (uint8_t)((auth << 2U) | (tail_bits & 0x03U));
    raw[12] = fiat_v1_frame_xor(raw);
}

static uint32_t fiat_v1_uid(const uint8_t raw[FIAT_V1_WIRE_BYTES]) {
    return ((uint32_t)raw[2] << 24U) | ((uint32_t)raw[3] << 16U) | ((uint32_t)raw[4] << 8U) |
           raw[5];
}

static uint32_t fiat_v1_counter(const uint8_t raw[FIAT_V1_WIRE_BYTES]) {
    return ((uint32_t)(raw[6] & 0x0FU) << 6U) | (raw[7] >> 2U);
}

static uint32_t fiat_v1_hop(const uint8_t raw[FIAT_V1_WIRE_BYTES]) {
    return ((uint32_t)(raw[7] & 0x03U) << 30U) | ((uint32_t)raw[8] << 22U) |
           ((uint32_t)raw[9] << 14U) | ((uint32_t)raw[10] << 6U) | (raw[11] >> 2U);
}

static bool fiat_v1_frame_valid(const uint8_t raw[FIAT_V1_WIRE_BYTES]) {
    if(raw[0] != 0x00U || raw[1] != 0x01U) {
        return false;
    }
    if(fiat_v1_frame_xor(raw) != raw[12]) {
        return false;
    }
    if(!fiat_v1_button_valid(raw[6] >> 4U)) {
        return false;
    }

    const uint32_t uid = fiat_v1_uid(raw);
    return uid != 0U && uid != UINT32_MAX;
}

static void fiat_v1_clear_cells(RfStreamProtocolDecoderFiatV1* instance, uint8_t variant) {
    instance->cell_count[variant] = 0U;
    memset(instance->cells[variant], 0, FIAT_V1_WIRE_CELLS);
}

static void fiat_v1_clear_all_cells(RfStreamProtocolDecoderFiatV1* instance) {
    for(uint8_t variant = 0U; variant < FIAT_V1_VARIANT_COUNT; variant++) {
        fiat_v1_clear_cells(instance, variant);
    }
}

static void fiat_v1_decode_fields(RfStreamProtocolDecoderFiatV1* instance) {
    instance->family = instance->raw_data[1];
    instance->uid = fiat_v1_uid(instance->raw_data);
    instance->generic.serial = instance->uid;
    instance->generic.btn = instance->raw_data[6] >> 4U;
    instance->generic.cnt = fiat_v1_counter(instance->raw_data);
    instance->hop = fiat_v1_hop(instance->raw_data);
    instance->tail_bits = instance->raw_data[11] & 0x03U;
    instance->frame_xor = instance->raw_data[12];
    instance->generic.data = ((uint64_t)instance->generic.serial << 32U) | instance->hop;
    instance->generic.data_count_bit = FIAT_V1_LOGICAL_BITS;
    instance->decoder.decode_data = instance->generic.data;
    instance->decoder.decode_count_bit = instance->generic.data_count_bit;
    fiat_v1_verify_hitag2_key(instance);
}

static bool fiat_v1_commit(
    RfStreamProtocolDecoderFiatV1* instance,
    const uint8_t raw[FIAT_V1_WIRE_BYTES],
    uint8_t te_variant) {
    if(!fiat_v1_frame_valid(raw)) {
        return false;
    }

    if(instance->last_raw_valid && memcmp(instance->last_raw_data, raw, FIAT_V1_WIRE_BYTES) == 0) {
        return true;
    }

    memcpy(instance->raw_data, raw, FIAT_V1_WIRE_BYTES);
    memcpy(instance->last_raw_data, raw, FIAT_V1_WIRE_BYTES);
    instance->last_raw_valid = true;
    instance->te_variant = (te_variant == FIAT_V1_TE_VARIANT_A) ? FIAT_V1_TE_VARIANT_A :
                                                                  FIAT_V1_TE_VARIANT_B;
    fiat_v1_decode_fields(instance);

    RF_STREAM_LOG_D(
        TAG,
        "Accepted UID:%08lX Btn:%02X Cnt:%03lX Auth:%08lX XOR:%02X",
        (unsigned long)instance->uid,
        instance->generic.btn,
        (unsigned long)instance->generic.cnt,
        (unsigned long)instance->hop,
        instance->frame_xor);

    if(instance->base.callback) {
        instance->base.callback(&instance->base, instance->base.context);
    }
    return true;
}

static bool
    fiat_v1_try_decode_window(RfStreamProtocolDecoderFiatV1* instance, uint8_t variant, bool invert) {
    if(instance->cell_count[variant] != FIAT_V1_WIRE_CELLS) {
        return false;
    }

    const uint8_t* cells = instance->cells[variant];
    uint8_t raw[FIAT_V1_WIRE_BYTES] = {0};
    for(uint8_t bit_index = 0U; bit_index < FIAT_V1_WIRE_BITS; bit_index++) {
        const uint8_t first = cells[bit_index * 2U];
        const uint8_t second = cells[bit_index * 2U + 1U];
        if(first == second) {
            return false;
        }

        bool bit = first != 0U;
        if(invert) {
            bit = !bit;
        }
        if(bit) {
            raw[bit_index >> 3U] |= (uint8_t)(1U << (7U - (bit_index & 7U)));
        }
    }

    return fiat_v1_commit(instance, raw, variant);
}

static void fiat_v1_try_decode(RfStreamProtocolDecoderFiatV1* instance, uint8_t variant) {
    if(fiat_v1_try_decode_window(instance, variant, false)) {
        return;
    }
    (void)fiat_v1_try_decode_window(instance, variant, true);
}

static void fiat_v1_push_cell(RfStreamProtocolDecoderFiatV1* instance, uint8_t variant, bool level) {
    uint8_t* cells = instance->cells[variant];
    if(instance->cell_count[variant] < FIAT_V1_WIRE_CELLS) {
        cells[instance->cell_count[variant]++] = level ? 1U : 0U;
    } else {
        memmove(cells, &cells[1], FIAT_V1_WIRE_CELLS - 1U);
        cells[FIAT_V1_WIRE_CELLS - 1U] = level ? 1U : 0U;
    }
    fiat_v1_try_decode(instance, variant);
}

static bool
    fiat_v1_feed_data_pulse(RfStreamProtocolDecoderFiatV1* instance, bool level, uint32_t duration) {
    bool matched = false;

    for(uint8_t variant = 0U; variant < FIAT_V1_VARIANT_COUNT; variant++) {
        if(fiat_v1_duration_is_short(variant, duration)) {
            fiat_v1_push_cell(instance, variant, level);
            matched = true;
        } else if(fiat_v1_duration_is_long(variant, duration)) {
            fiat_v1_push_cell(instance, variant, level);
            fiat_v1_push_cell(instance, variant, level);
            matched = true;
        } else {
            if(!level && duration >= FIAT_V1_BOUNDARY_MIN_US) {
                fiat_v1_push_cell(instance, variant, false);
            }
            fiat_v1_clear_cells(instance, variant);
        }
    }

    return matched;
}

static void fiat_v1_rebuild_raw(RfStreamProtocolDecoderFiatV1* instance) {
    fiat_v1_build_raw(
        instance->raw_data,
        instance->generic.serial,
        instance->generic.btn,
        (uint16_t)(instance->generic.cnt & 0x03FFU),
        instance->hop,
        instance->tail_bits);
    if(instance->family && instance->family != 0x01U) {
        instance->raw_data[1] = instance->family;
        instance->raw_data[12] = fiat_v1_frame_xor(instance->raw_data);
    }
    if(instance->frame_xor) {
        instance->raw_data[12] = instance->frame_xor;
    }
}

#if RF_STREAM_WITH_ENCODER || RF_STREAM_WITH_DECODER
static uint8_t fiat_v1_truth(uint32_t table, uint8_t index) {
    return (uint8_t)((table >> index) & 1U);
}

static uint8_t fiat_v1_filter_index(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return (uint8_t)((a << 3U) | (b << 2U) | (c << 1U) | d);
}

static uint8_t fiat_v1_byte_bit(uint8_t byte, uint8_t bit) {
    return (uint8_t)((byte >> bit) & 1U);
}

static uint8_t fiat_v1_bcm_hitag2_filter(const uint8_t state[6]) {
    uint8_t group = 0U;
    group |= fiat_v1_truth(
        0x2c79U,
        fiat_v1_filter_index(
            fiat_v1_byte_bit(state[0], 1U),
            fiat_v1_byte_bit(state[0], 2U),
            fiat_v1_byte_bit(state[0], 4U),
            fiat_v1_byte_bit(state[0], 5U)));
    group |= (uint8_t)(fiat_v1_truth(
                           0x6671U,
                           fiat_v1_filter_index(
                               fiat_v1_byte_bit(state[1], 0U),
                               fiat_v1_byte_bit(state[1], 1U),
                               fiat_v1_byte_bit(state[1], 3U),
                               fiat_v1_byte_bit(state[1], 7U)))
                       << 1U);
    group |= (uint8_t)(fiat_v1_truth(
                           0x6671U,
                           fiat_v1_filter_index(
                               fiat_v1_byte_bit(state[3], 5U),
                               fiat_v1_byte_bit(state[2], 0U),
                               fiat_v1_byte_bit(state[2], 2U),
                               fiat_v1_byte_bit(state[2], 6U)))
                       << 2U);
    group |= (uint8_t)(fiat_v1_truth(
                           0x6671U,
                           fiat_v1_filter_index(
                               fiat_v1_byte_bit(state[4], 6U),
                               fiat_v1_byte_bit(state[3], 0U),
                               fiat_v1_byte_bit(state[3], 2U),
                               fiat_v1_byte_bit(state[3], 3U)))
                       << 3U);
    group |= (uint8_t)(fiat_v1_truth(
                           0x2c79U,
                           fiat_v1_filter_index(
                               fiat_v1_byte_bit(state[5], 1U),
                               fiat_v1_byte_bit(state[5], 3U),
                               fiat_v1_byte_bit(state[5], 4U),
                               fiat_v1_byte_bit(state[4], 5U)))
                       << 4U);
    return fiat_v1_truth(0x7907287bUL, group);
}

static uint8_t fiat_v1_parity8(uint8_t value) {
    value ^= (uint8_t)(value >> 4U);
    value ^= (uint8_t)(value >> 2U);
    value ^= (uint8_t)(value >> 1U);
    return value & 1U;
}

static uint8_t fiat_v1_bcm_hitag2_feedback(const uint8_t state[6]) {
    static const uint8_t masks[6] = {0xb3U, 0x80U, 0x83U, 0x22U, 0x00U, 0x73U};
    uint8_t feedback = 0U;
    for(uint8_t i = 0U; i < 6U; i++) {
        feedback ^= fiat_v1_parity8((uint8_t)(state[i] & masks[i]));
    }
    return feedback & 1U;
}

static void fiat_v1_bcm_hitag2_shift(uint8_t state[6], uint8_t input) {
    for(uint8_t i = 0U; i < 5U; i++) {
        state[i] = (uint8_t)((state[i] << 1U) | (state[i + 1U] >> 7U));
    }
    state[5] = (uint8_t)((state[5] << 1U) | (input & 1U));
}

static uint8_t fiat_v1_input_bit_u32_be(uint32_t value, uint8_t index) {
    return (uint8_t)((value >> (31U - index)) & 1U);
}

static uint8_t fiat_v1_input_bit_bytes_be(const uint8_t* bytes, uint8_t index) {
    return (uint8_t)((bytes[index >> 3U] >> (7U - (index & 7U))) & 1U);
}

static uint32_t fiat_v1_bcm_generate_authenticator(
    uint32_t uid,
    uint8_t button,
    uint16_t control,
    const uint8_t key[6],
    uint32_t epoch) {
    uint8_t state[6] = {
        (uint8_t)(uid >> 24U),
        (uint8_t)(uid >> 16U),
        (uint8_t)(uid >> 8U),
        (uint8_t)uid,
        key[4],
        key[5],
    };

    const uint32_t iv = ((epoch & 0x3FFFFUL) << 14U) | (((uint32_t)control & 0x03FFUL) << 4U) |
                        ((uint32_t)button & 0x0FUL);

    for(uint8_t i = 0U; i < 32U; i++) {
        const uint8_t input = fiat_v1_input_bit_u32_be(iv, i) ^
                              fiat_v1_input_bit_bytes_be(key, i) ^
                              fiat_v1_bcm_hitag2_filter(state);
        fiat_v1_bcm_hitag2_shift(state, input);
    }

    uint32_t authenticator = 0U;
    for(uint8_t i = 0U; i < 32U; i++) {
        authenticator = (authenticator << 1U) | fiat_v1_bcm_hitag2_filter(state);
        fiat_v1_bcm_hitag2_shift(state, fiat_v1_bcm_hitag2_feedback(state));
    }
    return authenticator;
}
#endif

static const uint8_t fiat_v1_known_keys[FIAT_V1_KNOWN_KEY_COUNT][6] = {
    {0xB7U, 0x92U, 0x80U, 0xAEU, 0xCCU, 0x37U},
    {0xD4U, 0x24U, 0x28U, 0xF7U, 0xD9U, 0x66U},
    {0x4DU, 0x34U, 0x3FU, 0xD4U, 0xE7U, 0xB6U},
    {0x6DU, 0x6BU, 0xF2U, 0x1DU, 0x3AU, 0x1AU},
    {0xA3U, 0xF3U, 0xACU, 0xF7U, 0xB9U, 0x10U},
    {0x4DU, 0x49U, 0x4BU, 0x52U, 0x4FU, 0x4EU},
    {0xCDU, 0x49U, 0x4BU, 0x52U, 0x4FU, 0x4EU},
    {0x33U, 0xFAU, 0x2FU, 0xCDU, 0xC3U, 0x3BU},
    {0xF6U, 0x1AU, 0xEFU, 0x9CU, 0xD0U, 0x1BU},
};

static bool fiat_v1_key_matches(
    uint32_t uid,
    uint8_t button,
    uint16_t control,
    uint32_t hop,
    const uint8_t key[6],
    uint32_t epoch) {
    return fiat_v1_bcm_generate_authenticator(uid, button, control, key, epoch) == hop;
}

static bool fiat_v1_key_matches_any_button(
    uint32_t uid,
    uint16_t control,
    uint32_t hop,
    const uint8_t key[6],
    uint32_t epoch) {
    static const uint8_t buttons[] = {0x1U, 0x2U, 0x4U, 0x8U};
    for(size_t i = 0; i < COUNT_OF(buttons); i++) {
        if(fiat_v1_key_matches(uid, buttons[i], control, hop, key, epoch)) {
            return true;
        }
    }
    return false;
}

static bool fiat_v1_resolve_hitag2_key(
    RfRecord* ff,
    uint32_t uid,
    uint8_t button,
    uint16_t control,
    uint32_t hop,
    uint8_t key_out[6],
    uint32_t* epoch_out) {
    if(!key_out) return false;

    uint8_t key[6] = {0};
    uint32_t epoch = 0U;
    if(ff) {
        rf_record_rewind(ff);
        if(rf_record_read_hex(ff, FIAT_V1_HITAG2_KEY_FIELD, key, sizeof(key))) {
            rf_record_rewind(ff);
            if(rf_record_read_uint32(ff, FIAT_V1_HITAG2_EPOCH_FIELD, &epoch, 1U)) {
                epoch &= 0x3FFFFUL;
            }
            if(fiat_v1_key_matches(uid, button, control, hop, key, epoch) ||
               fiat_v1_key_matches_any_button(uid, control, hop, key, epoch)) {
                memcpy(key_out, key, sizeof(key));
                if(epoch_out) *epoch_out = epoch;
                return true;
            }
        }
    }

    for(uint8_t i = 0U; i < FIAT_V1_KNOWN_KEY_COUNT; i++) {
        if(fiat_v1_key_matches(uid, button, control, hop, fiat_v1_known_keys[i], 0U) ||
           fiat_v1_key_matches_any_button(uid, control, hop, fiat_v1_known_keys[i], 0U)) {
            memcpy(key_out, fiat_v1_known_keys[i], sizeof(key));
            if(epoch_out) *epoch_out = 0U;
            return true;
        }
    }
    return false;
}

static void fiat_v1_verify_hitag2_key(RfStreamProtocolDecoderFiatV1* instance) {
    instance->hitag2_key_valid = false;
    instance->hitag2_epoch = 0U;
    memset(instance->hitag2_key, 0, sizeof(instance->hitag2_key));

    instance->hitag2_key_valid = fiat_v1_resolve_hitag2_key(
        NULL,
        instance->uid,
        instance->generic.btn,
        (uint16_t)(instance->generic.cnt & 0x03FFU),
        instance->hop,
        instance->hitag2_key,
        &instance->hitag2_epoch);
}

#if RF_STREAM_WITH_ENCODER
static bool fiat_v1_encoder_build_upload(RfStreamProtocolEncoderFiatV1* instance) {
    LevelDuration* upload = instance->encoder.upload;
    if(!upload) {
        return false;
    }

    const uint8_t variant = instance->te_variant;
    const uint32_t te = fiat_v1_enc_te_us(variant);
    size_t index = 0U;
    const size_t cap = FIAT_V1_UPLOAD_CAPACITY;
    index = rf_stream_emit_merge(upload, index, cap, true, fiat_v1_enc_lead_us(variant));

    for(uint8_t bit_index = 0U; bit_index < FIAT_V1_WIRE_BITS; bit_index++) {
        const bool bit = ((instance->raw_data[bit_index >> 3U] >> (7U - (bit_index & 7U))) & 1U) !=
                         0U;
        index = rf_stream_emit_merge(upload, index, cap, bit, te);
        index = rf_stream_emit_merge(upload, index, cap, !bit, te);
    }

    index = rf_stream_emit_merge(upload, index, cap, false, fiat_v1_enc_gap_us(variant));
    instance->encoder.size_upload = index;
    instance->encoder.front = 0U;
    return true;
}

void* rf_stream_protocol_encoder_fiat_v1_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolEncoderFiatV1* instance = calloc(1, sizeof(RfStreamProtocolEncoderFiatV1));

    instance->base.protocol = &fiat_v1_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->tail_bits = FIAT_V1_DEFAULT_TAIL_BITS;
    instance->te_variant = FIAT_V1_DEFAULT_TE_VARIANT;
    instance->encoder.repeat = FIAT_V1_ENC_DEFAULT_REPEAT;
    return instance;
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_fiat_v1_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderFiatV1* instance = context;

    instance->encoder.is_running = false;
    instance->encoder.front = 0U;

    rf_record_rewind(flipper_format);
    if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
       RfStreamProtocolStatusOk) {
        return RfStreamProtocolStatusErrorParserOthers;
    }

    uint32_t bit_count = 0U;
    static const uint16_t allowed_bits[] = {FIAT_V1_LOGICAL_BITS};
    if(rf_stream_encoder_read_bit(flipper_format, allowed_bits, 1U, &bit_count) !=
       RfStreamProtocolStatusOk) {
        return RfStreamProtocolStatusErrorValueBitCount;
    }
    instance->generic.data_count_bit = bit_count;

    uint32_t key_serial = 0U;
    uint32_t key_button = 0U;
    uint32_t key_control = 0U;
    uint32_t captured_hop = 0U;
    uint8_t raw_from_file[FIAT_V1_WIRE_BYTES] = {0};
    bool have_raw = false;

    rf_record_rewind(flipper_format);
    if(rf_record_read_hex(
           flipper_format, FIAT_V1_RAW_FIELD, raw_from_file, sizeof(raw_from_file)) &&
       fiat_v1_frame_valid(raw_from_file)) {
        have_raw = true;
        key_serial = fiat_v1_uid(raw_from_file);
        key_button = raw_from_file[6] >> 4U;
        key_control = fiat_v1_counter(raw_from_file);
        captured_hop = fiat_v1_hop(raw_from_file);
        instance->tail_bits = raw_from_file[11] & 0x03U;
    } else {
        RfStreamBlockGeneric generic = {0};
        rf_record_rewind(flipper_format);
        if(rf_stream_block_generic_deserialize_check_count_bit(
               &generic, flipper_format, rf_stream_protocol_fiat_v1_const.min_count_bit_for_found) ==
           RfStreamProtocolStatusOk) {
            key_serial = (uint32_t)(generic.data >> 32U);
            key_button = generic.btn;
            key_control = generic.cnt;
            captured_hop = (uint32_t)generic.data;
        }
        rf_record_rewind(flipper_format);
        uint32_t hop_field = 0U;
        if(rf_record_read_uint32(flipper_format, FIAT_V1_HOP_FIELD, &hop_field, 1U)) {
            captured_hop = hop_field;
        }
    }

    uint32_t serial = key_serial;
    uint32_t button = key_button;
    uint32_t control = key_control;
    uint32_t type = FIAT_V1_DEFAULT_TE_VARIANT;
    rf_stream_encoder_read_fields(flipper_format, &serial, &button, &control, &type);
    if(!have_raw) {
        key_serial = serial;
        key_control = control;
        if(fiat_v1_button_valid((uint8_t)key_button) == false) {
            key_button = button;
        }
    }

    if(serial == 0U || serial == UINT32_MAX || !fiat_v1_button_valid((uint8_t)button)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }

    rf_record_rewind(flipper_format);
    uint32_t tail_bits = instance->tail_bits;
    if(rf_record_read_uint32(flipper_format, FIAT_V1_TAIL_BITS_FIELD, &tail_bits, 1U)) {
        instance->tail_bits = (uint8_t)(tail_bits & 0x03U);
    }

    instance->te_variant = (type == FIAT_V1_TE_VARIANT_A) ? FIAT_V1_TE_VARIANT_A :
                                                            FIAT_V1_TE_VARIANT_B;

    if(!fiat_v1_resolve_hitag2_key(
           flipper_format,
           key_serial ? key_serial : serial,
           (uint8_t)(key_button ? key_button : button),
           (uint16_t)(key_control & 0x03FFU),
           captured_hop,
           instance->hitag2_key,
           &instance->epoch)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }

    control &= 0x03FFU;
    button &= 0x0FU;
    instance->generic.serial = serial;
    instance->generic.btn = (uint8_t)button;
    instance->generic.cnt = control;
    instance->hop = fiat_v1_bcm_generate_authenticator(
        serial, (uint8_t)button, (uint16_t)control, instance->hitag2_key, instance->epoch);
    instance->generic.data = ((uint64_t)serial << 32U) | instance->hop;
    instance->generic.data_count_bit = FIAT_V1_LOGICAL_BITS;

    fiat_v1_build_raw(
        instance->raw_data,
        serial,
        (uint8_t)button,
        (uint16_t)control,
        instance->hop,
        instance->tail_bits);
    instance->frame_xor = instance->raw_data[12];

    instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, FIAT_V1_ENC_DEFAULT_REPEAT);
    if(instance->encoder.repeat == 0U) {
        instance->encoder.repeat = FIAT_V1_ENC_DEFAULT_REPEAT;
    }

    rf_stream_encoder_buffer_ensure(instance, FIAT_V1_UPLOAD_CAPACITY);
    if(!fiat_v1_encoder_build_upload(instance)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }
    instance->encoder.is_running = true;

    RF_STREAM_LOG_I(
        TAG,
        "TX UID:%08lX Btn:%02lX Cnt:%03lX Auth:%08lX Epoch:%05lX TE:%u XOR:%02X",
        (unsigned long)serial,
        (unsigned long)button,
        (unsigned long)control,
        (unsigned long)instance->hop,
        (unsigned long)instance->epoch,
        instance->te_variant,
        instance->frame_xor);

    return RfStreamProtocolStatusOk;
}
#endif

void* rf_stream_protocol_decoder_fiat_v1_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderFiatV1* instance = calloc(1, sizeof(RfStreamProtocolDecoderFiatV1));
    instance->base.protocol = &fiat_v1_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    rf_stream_protocol_decoder_fiat_v1_reset(instance);
    return instance;
}

void rf_stream_protocol_decoder_fiat_v1_reset(void* context) {
    RfStreamProtocolDecoderFiatV1* instance = context;

    memset(instance->raw_data, 0, sizeof(instance->raw_data));
    memset(instance->last_raw_data, 0, sizeof(instance->last_raw_data));
    instance->decoder.parser_step = FiatV1DecoderStepReset;
    instance->decoder.decode_data = 0U;
    instance->decoder.decode_count_bit = 0U;
    instance->last_raw_valid = false;
    instance->generic.data = 0U;
    instance->generic.data_count_bit = 0U;
    instance->generic.serial = 0U;
    instance->generic.btn = 0U;
    instance->generic.cnt = 0U;
    instance->uid = 0U;
    instance->hop = 0U;
    instance->family = 0U;
    instance->tail_bits = FIAT_V1_DEFAULT_TAIL_BITS;
    instance->frame_xor = 0U;
    instance->hitag2_key_valid = false;
    instance->hitag2_epoch = 0U;
    instance->te_variant = FIAT_V1_DEFAULT_TE_VARIANT;
    memset(instance->hitag2_key, 0, sizeof(instance->hitag2_key));
    fiat_v1_clear_all_cells(instance);
}

void rf_stream_protocol_decoder_fiat_v1_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderFiatV1* instance = context;

    switch(instance->decoder.parser_step) {
    case FiatV1DecoderStepReset:
        if(fiat_v1_duration_is_pulse(duration)) {
            fiat_v1_clear_all_cells(instance);
            instance->decoder.parser_step = FiatV1DecoderStepData;
            (void)fiat_v1_feed_data_pulse(instance, level, duration);
        }
        break;

    case FiatV1DecoderStepData:
        if(!fiat_v1_feed_data_pulse(instance, level, duration)) {
            instance->decoder.parser_step = FiatV1DecoderStepReset;
        }
        break;
    }
}

uint8_t rf_stream_protocol_decoder_fiat_v1_get_hash_data(void* context) {
    RfStreamProtocolDecoderFiatV1* instance = context;
    RfStreamBlockDecoder decoder = {
        .decode_data = instance->generic.data,
        .decode_count_bit = 64U,
    };
    return rf_stream_protocol_blocks_get_hash_data(&decoder, 8U) ^
           (uint8_t)(instance->generic.cnt >> 8U) ^ (uint8_t)instance->generic.cnt ^
           instance->generic.btn ^ instance->frame_xor;
}

RfStreamProtocolStatus rf_stream_protocol_decoder_fiat_v1_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderFiatV1* instance = context;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
    if(ret != RfStreamProtocolStatusOk) {
        return ret;
    }

    rf_record_rewind(flipper_format);
    rf_record_delete_key(flipper_format, FIAT_V1_RAW_FIELD);

    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_SERIAL, instance->generic.serial);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_BTN, instance->generic.btn);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_CNT, instance->generic.cnt);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_TYPE, instance->te_variant);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FIAT_V1_HOP_FIELD, instance->hop);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FIAT_V1_XOR_FIELD, instance->frame_xor);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FIAT_V1_TAIL_BITS_FIELD, instance->tail_bits);

    if(!rf_record_insert_or_update_hex(
           flipper_format, FIAT_V1_RAW_FIELD, instance->raw_data, FIAT_V1_WIRE_BYTES)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }

    if(instance->hitag2_key_valid) {
        uint32_t epoch = instance->hitag2_epoch & 0x3FFFFUL;
        if(!rf_record_insert_or_update_hex(
               flipper_format, FIAT_V1_HITAG2_KEY_FIELD, instance->hitag2_key, 6U)) {
            return RfStreamProtocolStatusErrorParserOthers;
        }
        rf_stream_flipper_update_or_insert_u32(flipper_format, FIAT_V1_HITAG2_EPOCH_FIELD, epoch);
    }

    return rf_stream_write_display(
        flipper_format,
        instance->generic.protocol_name,
        fiat_v1_button_name(instance->generic.btn));
}

static void
    fiat_v1_load_hitag2_key(RfStreamProtocolDecoderFiatV1* instance, RfRecord* flipper_format) {
    instance->hitag2_key_valid = fiat_v1_resolve_hitag2_key(
        flipper_format,
        instance->uid,
        instance->generic.btn,
        (uint16_t)(instance->generic.cnt & 0x03FFU),
        instance->hop,
        instance->hitag2_key,
        &instance->hitag2_epoch);
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_fiat_v1_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderFiatV1* instance = context;

    RfStreamProtocolStatus ret = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, rf_stream_protocol_fiat_v1_const.min_count_bit_for_found);
    if(ret != RfStreamProtocolStatusOk) {
        return ret;
    }
    if(instance->generic.data_count_bit != FIAT_V1_LOGICAL_BITS) {
        return RfStreamProtocolStatusErrorValueBitCount;
    }

    rf_record_rewind(flipper_format);
    uint32_t value = 0U;
    instance->te_variant = FIAT_V1_DEFAULT_TE_VARIANT;
    if(rf_record_read_uint32(flipper_format, FF_TYPE, &value, 1)) {
        instance->te_variant = (value == FIAT_V1_TE_VARIANT_A) ? FIAT_V1_TE_VARIANT_A :
                                                                 FIAT_V1_TE_VARIANT_B;
    }

    rf_record_rewind(flipper_format);
    if(rf_record_read_hex(
           flipper_format, FIAT_V1_RAW_FIELD, instance->raw_data, FIAT_V1_WIRE_BYTES)) {
        if(!fiat_v1_frame_valid(instance->raw_data)) {
            return RfStreamProtocolStatusErrorParserOthers;
        }
        fiat_v1_decode_fields(instance);
        fiat_v1_load_hitag2_key(instance, flipper_format);
        return RfStreamProtocolStatusOk;
    }

    instance->generic.serial = (uint32_t)(instance->generic.data >> 32U);
    instance->hop = (uint32_t)instance->generic.data;
    instance->family = 0x01U;
    instance->uid = instance->generic.serial;
    instance->tail_bits = FIAT_V1_DEFAULT_TAIL_BITS;

    if(rf_record_read_uint32(flipper_format, FF_SERIAL, &value, 1)) {
        instance->generic.serial = value;
    }
    if(rf_record_read_uint32(flipper_format, FF_BTN, &value, 1)) {
        instance->generic.btn = (uint8_t)value;
    }
    if(rf_record_read_uint32(flipper_format, FF_CNT, &value, 1)) {
        instance->generic.cnt = value;
    }
    if(rf_record_read_uint32(flipper_format, FIAT_V1_HOP_FIELD, &value, 1)) {
        instance->hop = value;
    }
    if(rf_record_read_uint32(flipper_format, FIAT_V1_XOR_FIELD, &value, 1)) {
        instance->frame_xor = (uint8_t)value;
    }
    if(rf_record_read_uint32(flipper_format, FIAT_V1_TAIL_BITS_FIELD, &value, 1)) {
        instance->tail_bits = (uint8_t)(value & 0x03U);
    }

    fiat_v1_rebuild_raw(instance);
    if(!fiat_v1_frame_valid(instance->raw_data)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }
    fiat_v1_decode_fields(instance);
    fiat_v1_load_hitag2_key(instance, flipper_format);
    return RfStreamProtocolStatusOk;
}

void rf_stream_protocol_decoder_fiat_v1_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderFiatV1* instance = context;

    rf_text_cat_printf(
        output,
        "%s %ubit %s\r\n"
        "%08lX %03lX%01X %08lX\r\n"
        "Sync:%02X UID:%08lX Auth:%08lX\r\n"
        "Btn:%02X [%s] Ctrl:%03lX\r\n"
        "TE:%u Tail:%u XOR:%02X\r\n",
        instance->generic.protocol_name,
        FIAT_V1_LOGICAL_BITS,
        instance->hitag2_key_valid ? "KEY:OK" : "KEY:??",
        (unsigned long)instance->generic.serial,
        (unsigned long)instance->generic.cnt,
        instance->generic.btn,
        (unsigned long)instance->hop,
        instance->family,
        (unsigned long)instance->uid,
        (unsigned long)instance->hop,
        instance->generic.btn,
        fiat_v1_button_name(instance->generic.btn),
        (unsigned long)instance->generic.cnt,
        instance->te_variant,
        instance->tail_bits,
        instance->frame_xor);
}

#endif
