// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define KIA_PROTOCOL_V0_NAME "Kia V0"

typedef struct RfStreamProtocolDecoderKIA RfStreamProtocolDecoderKIA;
typedef struct RfStreamProtocolEncoderKIA RfStreamProtocolEncoderKIA;

extern const RfStreamProtocolDecoder rf_stream_protocol_kia_decoder;
extern const RfStreamProtocolEncoder rf_stream_protocol_kia_encoder;
extern const RfStreamCodec kia_protocol_v0;

// Decoder functions
void* rf_stream_protocol_decoder_kia_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_kia_free(void* context);
void rf_stream_protocol_decoder_kia_reset(void* context);
void rf_stream_protocol_decoder_kia_feed(void* context, bool level, uint32_t duration);
RfStreamProtocolStatus rf_stream_protocol_decoder_kia_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_kia_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_kia_get_string(void* context, RfText* output);

// Encoder helper functions
void rf_stream_protocol_encoder_kia_set_button(void* context, uint8_t button);
void rf_stream_protocol_encoder_kia_set_counter(void* context, uint16_t counter);
void rf_stream_protocol_encoder_kia_increment_counter(void* context);
uint16_t rf_stream_protocol_encoder_kia_get_counter(void* context);
uint8_t rf_stream_protocol_encoder_kia_get_button(void* context);




#include <string.h>

#define TAG "KiaV0"

static const RfStreamBlockConst kia_protocol_v0_const = {
    .te_short = 250,
    .te_long = 500,
    .te_delta = 100,
    .min_count_bit_for_found = 61,
};

#define KIA_V0_TYPE_KIA    1U
#define KIA_V0_TYPE_SUZUKI 2U
#define KIA_V0_TYPE_HONDA  3U
#define KIA_V0_TYPE_MITSU  4U

#define KIA_V0_BIT_COUNT_KIA    61U
#define KIA_V0_BIT_COUNT_SUZUKI 64U
#define KIA_V0_BIT_COUNT_HONDA  72U

#define KIA_V0_KIA_GAP      1000U
#define KIA_V0_KIA_GAP_BASE 700U
#define KIA_V0_KIA_GAP_SPAN 1000U

#define KIA_V0_SUZUKI_GAP      2000U
#define KIA_V0_SUZUKI_GAP_SPAN 500U

#define KIA_V0_TYPE1_SYNC                750U
#define KIA_V0_TYPE1_PREAMBLE_PAIRS      0x13FU
#define KIA_V0_TYPE2_PREAMBLE_PAIRS      0x140U
#define KIA_V0_TAIL_PREAMBLE_PAIRS       0x0FU
#define KIA_V0_MITSU_TAIL_PREAMBLE_PAIRS 0x50U
#define KIA_V0_MITSU_PREAMBLE_MIN        72U
#define KIA_V0_MITSU_PREAMBLE_MAX        88U

#define KIA_V0_UPLOAD_CAPACITY                                                       \
    ((KIA_V0_TYPE2_PREAMBLE_PAIRS * 2U) + 2U + (KIA_V0_BIT_COUNT_SUZUKI * 2U) + 3U + \
     (KIA_V0_MITSU_TAIL_PREAMBLE_PAIRS * 2U) + (KIA_V0_BIT_COUNT_SUZUKI * 2U) + 4U)
_Static_assert(
    KIA_V0_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "KIA_V0_UPLOAD_CAPACITY exceeds shared upload slab");
#define KIA_V0_ENCODER_DEFAULT_REPEAT 3U

typedef enum {
    KiaV0DecoderStepReset = 0,
    KiaV0DecoderStepPreamble = 1,
    KiaV0DecoderStepSaveDuration = 2,
    KiaV0DecoderStepCheckDuration = 3,
} KiaV0DecoderStep;

typedef struct {
    uint32_t serial;
    uint16_t counter;
    uint8_t button;
    uint8_t crc;
    uint8_t type;
    bool crc_valid;
} KiaV0Fields;

struct RfStreamProtocolDecoderKIA {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;
    uint16_t packet_bit_count;
    uint16_t preamble_pairs;
    uint8_t type;
    uint64_t last_kia_data;
    bool have_last_kia;
};

struct RfStreamProtocolEncoderKIA {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;
    uint8_t type;
    KiaV0Fields fields;
};

static const uint8_t kia_v0_honda_crc_table[16] = {
    0x4A,
    0x25,
    0x96,
    0x4B,
    0xA1,
    0xD4,
    0x6A,
    0x35,
    0x9E,
    0x4F,
    0xA3,
    0xD5,
    0xEE,
    0x77,
    0xBF,
    0xDB,
};

static const char* const kia_v0_honda_button_names[7] = {
    "Unlock",
    "Trunk",
    "Lock2",
    "Unlock2",
    "Trunk2",
    "Unlock3",
    "Trunk3",
};

static bool kia_v0_is_kia_gap(uint32_t duration) {
    return (duration >= KIA_V0_KIA_GAP_BASE) &&
           ((duration - KIA_V0_KIA_GAP_BASE) <= KIA_V0_KIA_GAP_SPAN);
}

static bool kia_v0_is_suzuki_gap_strict(uint32_t duration) {
    if(duration < KIA_V0_SUZUKI_GAP) {
        return false;
    }
    return (duration - KIA_V0_SUZUKI_GAP) <= KIA_V0_SUZUKI_GAP_SPAN;
}

#define kia_v0_crc8_poly(data, len) rf_stream_protocol_blocks_crc8((data), (len), 0x7F, 0x00)

static bool kia_v0_suzuki_verify_shifted_crc_words(uint32_t lo, uint32_t hi) {
    uint32_t r3 = ((lo >> 16) | (hi << 16)) & (uint32_t)~0xF0000000u;
    uint32_t r2 = (lo >> 12) & 0x0FU;
    uint32_t r1 = (hi >> 12) & 0xFFFFU;
    const uint8_t r4 = (uint8_t)((lo >> 4) & 0xFFU);

    r1 = ((r1 & 0xFFU) << 8) | ((r1 >> 8) & 0xFFU);

    uint8_t buf[6];
    buf[0] = (uint8_t)(r1 & 0xFFu);
    buf[1] = (uint8_t)((r1 >> 8) & 0xFFu);
    buf[2] = (uint8_t)((r3 >> 20) & 0xFFu);

    const uint8_t mid = (uint8_t)((r3 >> 12) & 0xFFu);
    buf[3] = mid;
    buf[4] = (uint8_t)((r3 >> 4) & 0xFFu);
    buf[5] = (uint8_t)(((r3 << 4) | (r2 & 0x0FU)) & 0xFFU);

    return (kia_v0_crc8_poly(buf, 6) == r4);
}

static bool kia_v0_suzuki_shifted_crc_valid(uint64_t shifted_key) {
    const uint32_t lo = (uint32_t)(shifted_key & 0xFFFFFFFFULL);
    const uint32_t hi = (uint32_t)((shifted_key >> 32) & 0xFFFFFFFFULL);
    return kia_v0_suzuki_verify_shifted_crc_words(lo, hi);
}

static uint8_t kia_v0_suzuki_crc8_from_fields(uint32_t serial, uint8_t button, uint32_t counter) {
    uint8_t buf[6];
    const uint16_t cnt_u16 = (uint16_t)(counter & 0xFFFFU);
    const uint16_t c_sw = (uint16_t)((cnt_u16 << 8) | (cnt_u16 >> 8));
    buf[0] = (uint8_t)(c_sw & 0xFFU);
    buf[1] = (uint8_t)((c_sw >> 8) & 0xFFU);
    buf[2] = (uint8_t)((serial >> 20) & 0xFFU);
    buf[3] = (uint8_t)((serial >> 12) & 0xFFU);
    buf[4] = (uint8_t)((serial >> 4) & 0xFFU);
    buf[5] = (uint8_t)((button & 0xFFU) | ((uint8_t)((uint32_t)serial << 4)));
    return kia_v0_crc8_poly(buf, 6);
}

static uint64_t kia_v0_suzuki_shifted_key_from_fields(
    uint32_t serial,
    uint8_t button,
    uint32_t counter,
    uint8_t crc_byte) {
    const uint32_t r8 = ((uint32_t)serial << 16) | ((uint32_t)crc_byte << 4) |
                        (((uint32_t)button & 0x0FU) << 12);
    const uint32_t r5 =
        (uint32_t)(((serial >> 16) & 0xFFFU) | (((uint32_t)(counter & 0xFFFFU)) << 12)) |
        0xF0000000U;
    return ((uint64_t)r5 << 32) | (uint64_t)r8;
}

static bool kia_v0_suzuki_resolve_shifted(uint64_t decode_data, uint64_t* out_shifted) {
    if(kia_v0_suzuki_shifted_crc_valid(decode_data)) {
        *out_shifted = decode_data;
        return true;
    }
    const uint64_t from_wire = decode_data << 1U;
    if(kia_v0_suzuki_shifted_crc_valid(from_wire)) {
        *out_shifted = from_wire;
        return true;
    }
    return false;
}

static uint8_t kia_v0_calculate_crc_poly(uint64_t data) {
    uint8_t crc_data[6];
    crc_data[0] = (data >> 48) & 0xFF;
    crc_data[1] = (data >> 40) & 0xFF;
    crc_data[2] = (data >> 32) & 0xFF;
    crc_data[3] = (data >> 24) & 0xFF;
    crc_data[4] = (data >> 16) & 0xFF;
    crc_data[5] = (data >> 8) & 0xFF;
    return kia_v0_crc8_poly(crc_data, 6);
}

static bool kia_v0_verify_crc_poly(uint64_t data) {
    uint8_t received_crc = data & 0xFF;
    return (kia_v0_calculate_crc_poly(data) == received_crc);
}

static uint64_t kia_v0_honda_transform(uint64_t data) {
    uint8_t bytes[8];
    rf_stream_u64_to_bytes_be(data, bytes);
    for(size_t index = 0; index < 8; index++) {
        bytes[index] = rf_stream_reverse_bits8(bytes[index]);
    }
    return rf_stream_bytes_to_u64_be(bytes);
}

static uint8_t kia_v0_family_crc(uint16_t counter, uint32_t serial, uint8_t button) {
    const uint8_t bytes[6] = {
        (uint8_t)(counter >> 8U),
        (uint8_t)counter,
        (uint8_t)(serial >> 20U),
        (uint8_t)(serial >> 12U),
        (uint8_t)(serial >> 4U),
        (uint8_t)(((serial & 0x0FU) << 4U) | (button & 0x0FU)),
    };
    uint8_t crc = 0;
    for(size_t index = 0; index < sizeof(bytes); index++) {
        crc ^= bytes[index];
    }
    return crc;
}

static uint8_t kia_v0_honda_fold_counter(uint16_t counter) {
    uint8_t value = 0;
    for(size_t index = 0; index < sizeof(kia_v0_honda_crc_table); index++) {
        if((counter >> index) & 1U) {
            value ^= kia_v0_honda_crc_table[index];
        }
    }
    return value;
}

static uint8_t kia_v0_honda_crc(uint8_t header, uint16_t counter) {
    uint8_t value = kia_v0_honda_fold_counter(counter);
    switch(header) {
    case 0xAA:
        value ^= 0xA5;
        break;
    case 0x2A:
        value ^= 0x21;
        break;
    case 0x6A:
        value ^= 0x15;
        break;
    case 0xFA:
        value ^= 0x73;
        break;
    default:
        value ^= 0xC6;
        break;
    }
    return value;
}

static uint8_t kia_v0_honda_header(uint8_t button) {
    const uint8_t button_3bit = button & 0x07U;
    const uint8_t base = (button_3bit == 0x07U) ? 0x1AU : 0x0AU;
    return (uint8_t)(((button_3bit << 5U) & 0xE0U) | base);
}

static uint64_t
    kia_v0_build_kia_raw(uint32_t serial, uint8_t button, uint16_t counter, uint8_t crc) {
    const uint32_t high = 0x0F000000UL | (((uint32_t)counter & 0xFFFFUL) << 8U) |
                          ((serial >> 20U) & 0xFFUL);
    const uint32_t low = (((uint32_t)serial & 0x000FFFFFUL) << 12U) |
                         (((uint32_t)button & 0x0FUL) << 8U) | crc;
    return ((uint64_t)high << 32U) | low;
}

static uint64_t kia_v0_build_honda_key(uint32_t serial, uint8_t button, uint16_t counter) {
    const uint8_t header = kia_v0_honda_header(button);
    const uint8_t crc = kia_v0_honda_crc(header, counter);
    const uint8_t bytes[8] = {
        0xF0,
        rf_stream_reverse_bits8((uint8_t)(counter >> 8U)),
        rf_stream_reverse_bits8((uint8_t)counter),
        (uint8_t)(serial >> 16U),
        (uint8_t)(serial >> 8U),
        (uint8_t)serial,
        header,
        crc,
    };
    return rf_stream_bytes_to_u64_be(bytes);
}

static bool kia_v0_honda_key_valid(uint64_t key) {
    return ((key >> 60U) == 0x0FULL) && (((key >> 56U) & 0x0FULL) == 0x00ULL) &&
           (((key >> 8U) & 0x0FULL) == 0x0AULL);
}

static void kia_v0_parse_family_raw(uint64_t raw, uint8_t type, KiaV0Fields* fields) {
    fields->type = type;
    fields->serial = (type == KIA_V0_TYPE_SUZUKI) ? (uint32_t)((raw >> 16U) & 0x0FFFFFFFULL) :
                                                    (uint32_t)((raw >> 12U) & 0x0FFFFFFFULL);
    fields->button = (type == KIA_V0_TYPE_SUZUKI) ? (uint8_t)((raw >> 12U) & 0x0FU) :
                                                    (uint8_t)((raw >> 8U) & 0x0FU);
    fields->counter = (type == KIA_V0_TYPE_SUZUKI) ? (uint16_t)((raw >> 44U) & 0xFFFFU) :
                                                     (uint16_t)((raw >> 40U) & 0xFFFFU);
    fields->crc = (type == KIA_V0_TYPE_SUZUKI) ? (uint8_t)((raw >> 4U) & 0xFFU) :
                                                 (uint8_t)(raw & 0xFFU);
    if(type == KIA_V0_TYPE_SUZUKI) {
        fields->crc_valid = kia_v0_suzuki_shifted_crc_valid(raw);
    } else {
        fields->crc_valid =
            (kia_v0_family_crc(fields->counter, fields->serial, fields->button) == fields->crc);
    }
}

static void kia_v0_parse_honda_key(uint64_t key, KiaV0Fields* fields) {
    uint8_t bytes[8];
    rf_stream_u64_to_bytes_be(key, bytes);
    fields->type = KIA_V0_TYPE_HONDA;
    fields->serial = ((uint32_t)bytes[3] << 16U) | ((uint32_t)bytes[4] << 8U) | (uint32_t)bytes[5];
    fields->counter = ((uint16_t)rf_stream_reverse_bits8(bytes[1]) << 8U) |
                      (uint16_t)rf_stream_reverse_bits8(bytes[2]);
    fields->button = bytes[6] >> 5U;
    fields->crc = bytes[7];
    fields->crc_valid = (kia_v0_honda_crc(bytes[6], fields->counter) == fields->crc);
}

static const char* kia_v0_protocol_name(uint8_t type) {
    switch(type) {
    case KIA_V0_TYPE_SUZUKI:
        return "Suzuki V0";
    case KIA_V0_TYPE_HONDA:
        return "Honda V0";
    case KIA_V0_TYPE_MITSU:
        return "Mitsubishi V0";
    default:
        return KIA_PROTOCOL_V0_NAME;
    }
}

static const char* kia_v0_button_name(uint8_t button, uint8_t type) {
    if(type == KIA_V0_TYPE_HONDA) {
        if((button >= 1U) && (button <= COUNT_OF(kia_v0_honda_button_names))) {
            return kia_v0_honda_button_names[button - 1U];
        }
        return "??";
    }
    if(type == KIA_V0_TYPE_SUZUKI) {
        switch(button) {
        case 0x03:
            return "Lock";
        case 0x04:
            return "Unlock";
        case 0x02:
            return "Trunk";
        default:
            return "??";
        }
    }
    switch(button) {
    case 0x01:
        return "Lock";
    case 0x02:
        return "Unlock";
    case 0x03:
        return "Trunk";
    default:
        return "??";
    }
}

static void kia_v0_parse_data(
    RfStreamBlockGeneric* generic,
    uint8_t type,
    KiaV0Fields* fields,
    uint16_t* packet_bit_count) {
    memset(fields, 0, sizeof(*fields));

    if(type == KIA_V0_TYPE_HONDA) {
        kia_v0_parse_honda_key(generic->data, fields);
        generic->data_count_bit = KIA_V0_BIT_COUNT_HONDA;
    } else {
        kia_v0_parse_family_raw(generic->data, type, fields);
        generic->data_count_bit = (type == KIA_V0_TYPE_SUZUKI) ? KIA_V0_BIT_COUNT_SUZUKI :
                                                                 KIA_V0_BIT_COUNT_KIA;
        if((type == KIA_V0_TYPE_KIA) || (type == KIA_V0_TYPE_MITSU)) {
            fields->crc_valid = kia_v0_verify_crc_poly(generic->data);
        }
    }

    generic->serial = fields->serial;
    generic->cnt = fields->counter;
    generic->btn = fields->button;
    if(packet_bit_count) {
        *packet_bit_count = generic->data_count_bit;
    }
}

static void kia_v0_decoder_state_clear(RfStreamProtocolDecoderKIA* instance) {
    instance->decoder.parser_step = KiaV0DecoderStepReset;
    instance->decoder.te_last = 0;
    instance->decoder.decode_data = 0;
    instance->decoder.decode_count_bit = 0;
    instance->preamble_pairs = 0;
}

static void kia_v0_decoder_commit(
    RfStreamProtocolDecoderKIA* instance,
    uint64_t data,
    uint8_t type,
    uint16_t bit_count) {
    instance->type = type;
    instance->packet_bit_count = bit_count;
    instance->generic.data = data;
    instance->generic.data_count_bit = bit_count;

    KiaV0Fields scratch;
    kia_v0_parse_data(&instance->generic, type, &scratch, &instance->packet_bit_count);

    if(instance->base.callback) {
        instance->base.callback(&instance->base, instance->base.context);
    }

    kia_v0_decoder_state_clear(instance);
}

static void kia_v0_decoder_finish_kia_or_honda_at_gap(RfStreamProtocolDecoderKIA* instance) {
    if(instance->decoder.decode_count_bit != KIA_V0_BIT_COUNT_KIA) {
        kia_v0_decoder_state_clear(instance);
        return;
    }

    const uint64_t data = instance->decoder.decode_data;
    if(kia_v0_verify_crc_poly(data)) {
        uint8_t type = KIA_V0_TYPE_KIA;
        if(instance->have_last_kia && (instance->last_kia_data == data) &&
           (instance->preamble_pairs >= KIA_V0_MITSU_PREAMBLE_MIN) &&
           (instance->preamble_pairs <= KIA_V0_MITSU_PREAMBLE_MAX)) {
            type = KIA_V0_TYPE_MITSU;
        }
        instance->last_kia_data = data;
        instance->have_last_kia = true;
        kia_v0_decoder_commit(instance, data, type, KIA_V0_BIT_COUNT_KIA);
        return;
    }

    const uint64_t raw = data & 0x0FFFFFFFFFFFFFFFULL;
    const uint64_t key = kia_v0_honda_transform(raw);
    if(kia_v0_honda_key_valid(key)) {
        kia_v0_decoder_commit(instance, key, KIA_V0_TYPE_HONDA, KIA_V0_BIT_COUNT_HONDA);
        return;
    }

    kia_v0_decoder_state_clear(instance);
}

static bool kia_v0_decoder_try_honda(RfStreamProtocolDecoderKIA* instance) {
    if(instance->decoder.decode_count_bit != KIA_V0_BIT_COUNT_KIA) {
        return false;
    }
    if(kia_v0_verify_crc_poly(instance->decoder.decode_data)) {
        return false;
    }

    const uint64_t raw = instance->decoder.decode_data & 0x0FFFFFFFFFFFFFFFULL;
    const uint64_t key = kia_v0_honda_transform(raw);
    if(!kia_v0_honda_key_valid(key)) {
        return false;
    }

    kia_v0_decoder_commit(instance, key, KIA_V0_TYPE_HONDA, KIA_V0_BIT_COUNT_HONDA);
    return true;
}
#if RF_STREAM_WITH_ENCODER

static size_t kia_v0_append_short_pairs(LevelDuration* upload, size_t index, size_t count) {
    return rf_stream_emit_short_pairs(
        upload, index, KIA_V0_UPLOAD_CAPACITY, kia_protocol_v0_const.te_short, count);
}

#endif
#if RF_STREAM_WITH_ENCODER

static size_t kia_v0_append_data_pairs(
    LevelDuration* upload,
    size_t index,
    uint64_t data,
    uint8_t bit_count) {
    for(int bit = bit_count - 1; bit >= 0; bit--) {
        const uint32_t duration = ((data >> bit) & 1ULL) ?
                                      (uint32_t)kia_protocol_v0_const.te_long :
                                      (uint32_t)kia_protocol_v0_const.te_short;
        index = rf_stream_emit(upload, index, KIA_V0_UPLOAD_CAPACITY, true, duration);
        index = rf_stream_emit(upload, index, KIA_V0_UPLOAD_CAPACITY, false, duration);
    }
    return index;
}

#endif
#if RF_STREAM_WITH_ENCODER

static void kia_v0_build_honda_upload(RfStreamProtocolEncoderKIA* instance, uint64_t raw) {
    size_t index = 0;
    const uint64_t transformed = kia_v0_honda_transform(raw);

    index = kia_v0_append_short_pairs(instance->encoder.upload, index, 40);

    for(size_t pair = 0; pair < 4; pair++) {
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)kia_protocol_v0_const.te_long);
        instance->encoder.upload[index++] =
            level_duration_make(false, (uint32_t)kia_protocol_v0_const.te_long);
    }

    index = kia_v0_append_data_pairs(instance->encoder.upload, index, transformed, 56);
    instance->encoder.upload[index++] = level_duration_make(true, KIA_V0_KIA_GAP);

    instance->encoder.front = 0;
    instance->encoder.size_upload = index;
}

#endif
#if RF_STREAM_WITH_ENCODER

static void kia_v0_build_kia_upload(RfStreamProtocolEncoderKIA* instance, uint64_t raw) {
    size_t index = 0;

    instance->encoder.upload[index++] = level_duration_make(true, KIA_V0_TYPE1_SYNC);
    instance->encoder.upload[index++] = level_duration_make(false, KIA_V0_TYPE1_SYNC);
    index =
        kia_v0_append_short_pairs(instance->encoder.upload, index, KIA_V0_TYPE1_PREAMBLE_PAIRS);
    index = kia_v0_append_data_pairs(instance->encoder.upload, index, raw, KIA_V0_BIT_COUNT_KIA);
    instance->encoder.upload[index++] = level_duration_make(true, 1500);
    instance->encoder.upload[index++] = level_duration_make(false, 1500);
    index = kia_v0_append_short_pairs(instance->encoder.upload, index, KIA_V0_TAIL_PREAMBLE_PAIRS);
    index = kia_v0_append_data_pairs(instance->encoder.upload, index, raw, KIA_V0_BIT_COUNT_KIA);
    instance->encoder.upload[index++] = level_duration_make(true, KIA_V0_KIA_GAP);

    instance->encoder.front = 0;
    instance->encoder.size_upload = index;
}

static void kia_v0_build_mitsu_upload(RfStreamProtocolEncoderKIA* instance, uint64_t raw) {
    size_t index = 0;

    instance->encoder.upload[index++] = level_duration_make(true, KIA_V0_TYPE1_SYNC);
    instance->encoder.upload[index++] = level_duration_make(false, KIA_V0_TYPE1_SYNC);
    index =
        kia_v0_append_short_pairs(instance->encoder.upload, index, KIA_V0_TYPE1_PREAMBLE_PAIRS);
    index = kia_v0_append_data_pairs(instance->encoder.upload, index, raw, KIA_V0_BIT_COUNT_KIA);
    instance->encoder.upload[index++] = level_duration_make(true, 1500);
    instance->encoder.upload[index++] = level_duration_make(false, 1500);
    index = kia_v0_append_short_pairs(
        instance->encoder.upload, index, KIA_V0_MITSU_TAIL_PREAMBLE_PAIRS);
    index = kia_v0_append_data_pairs(instance->encoder.upload, index, raw, KIA_V0_BIT_COUNT_KIA);
    instance->encoder.upload[index++] = level_duration_make(true, 1500);
    instance->encoder.upload[index++] = level_duration_make(false, 1500);

    instance->encoder.front = 0;
    instance->encoder.size_upload = index;
}

#endif
#if RF_STREAM_WITH_ENCODER

static void kia_v0_build_suzuki_upload(RfStreamProtocolEncoderKIA* instance, uint64_t shifted) {
    size_t index = 0;

    index =
        kia_v0_append_short_pairs(instance->encoder.upload, index, KIA_V0_TYPE2_PREAMBLE_PAIRS);
    index = kia_v0_append_data_pairs(
        instance->encoder.upload, index, shifted, KIA_V0_BIT_COUNT_SUZUKI);
    instance->encoder.upload[index++] = level_duration_make(false, KIA_V0_SUZUKI_GAP);
    instance->encoder.upload[index++] = level_duration_make(true, KIA_V0_SUZUKI_GAP);
    instance->encoder.upload[index++] =
        level_duration_make(false, (uint32_t)kia_protocol_v0_const.te_short);
    index = kia_v0_append_short_pairs(instance->encoder.upload, index, KIA_V0_TAIL_PREAMBLE_PAIRS);
    index = kia_v0_append_data_pairs(
        instance->encoder.upload, index, shifted, KIA_V0_BIT_COUNT_SUZUKI);

    instance->encoder.front = 0;
    instance->encoder.size_upload = index;
}

#endif

static uint8_t kia_v0_infer_type_from_bits(uint32_t bits) {
    if(bits == KIA_V0_BIT_COUNT_KIA) return KIA_V0_TYPE_KIA;
    if(bits == KIA_V0_BIT_COUNT_SUZUKI) return KIA_V0_TYPE_SUZUKI;
    if(bits == KIA_V0_BIT_COUNT_HONDA) return KIA_V0_TYPE_HONDA;
    return KIA_V0_TYPE_KIA;
}

static uint8_t kia_v0_type_from_protocol_name(const char* name) {
    if(!name) {
        return 0;
    }
    if((strcmp(name, "Mitsu v0") == 0) || (strcmp(name, "Mitsu V0") == 0) ||
       (strcmp(name, "Mitsubishi v0") == 0) || (strcmp(name, "Mitsubishi V0") == 0)) {
        return KIA_V0_TYPE_MITSU;
    }
    if((strcmp(name, "Suzuki") == 0) || (strcmp(name, "Suzuki V0") == 0)) {
        return KIA_V0_TYPE_SUZUKI;
    }
    if((strcmp(name, "Honda v0") == 0) || (strcmp(name, "Honda V0") == 0)) {
        return KIA_V0_TYPE_HONDA;
    }
    return 0;
}
#if RF_STREAM_WITH_ENCODER

static void kia_v0_encoder_apply_fields(RfStreamProtocolEncoderKIA* instance) {
    instance->generic.serial = instance->fields.serial;
    instance->generic.cnt = instance->fields.counter;
    if(instance->type == KIA_V0_TYPE_HONDA) {
        instance->generic.btn = instance->fields.button & 0x07U;
    } else {
        instance->generic.btn = instance->fields.button & 0x0FU;
    }

    if(instance->type == KIA_V0_TYPE_HONDA) {
        instance->generic.data = kia_v0_build_honda_key(
            instance->fields.serial, instance->fields.button & 0x07U, instance->fields.counter);
        instance->generic.data_count_bit = KIA_V0_BIT_COUNT_HONDA;
        kia_v0_parse_data(&instance->generic, instance->type, &instance->fields, NULL);
        kia_v0_build_honda_upload(instance, instance->generic.data);
    } else {
        if(instance->type == KIA_V0_TYPE_SUZUKI) {
            instance->fields.crc = kia_v0_suzuki_crc8_from_fields(
                instance->fields.serial,
                instance->fields.button & 0x0FU,
                (uint32_t)instance->fields.counter);
            instance->fields.crc_valid = true;
            const uint64_t shifted = kia_v0_suzuki_shifted_key_from_fields(
                instance->fields.serial,
                instance->fields.button & 0x0FU,
                (uint32_t)instance->fields.counter,
                instance->fields.crc);
            instance->generic.data = shifted;
            instance->generic.data_count_bit = KIA_V0_BIT_COUNT_SUZUKI;
            kia_v0_parse_data(&instance->generic, instance->type, &instance->fields, NULL);
            kia_v0_build_suzuki_upload(instance, shifted);
        } else {
            uint64_t partial = kia_v0_build_kia_raw(
                instance->fields.serial,
                instance->fields.button & 0x0FU,
                instance->fields.counter,
                0);
            instance->fields.crc = kia_v0_calculate_crc_poly(partial);
            instance->fields.crc_valid = true;
            instance->generic.data = kia_v0_build_kia_raw(
                instance->fields.serial,
                instance->fields.button & 0x0FU,
                instance->fields.counter,
                instance->fields.crc);
            instance->generic.data_count_bit = KIA_V0_BIT_COUNT_KIA;
            kia_v0_parse_data(&instance->generic, instance->type, &instance->fields, NULL);
            if(instance->type == KIA_V0_TYPE_MITSU) {
                kia_v0_build_mitsu_upload(instance, instance->generic.data);
            } else {
                kia_v0_build_kia_upload(instance, instance->generic.data);
            }
        }
    }
}

#endif
#if RF_STREAM_WITH_ENCODER

static void kia_v0_encoder_sync_from_generic(RfStreamProtocolEncoderKIA* instance) {
    instance->fields.serial = instance->generic.serial;
    instance->fields.counter = (uint16_t)(instance->generic.cnt & 0xFFFFU);
    if(instance->type == KIA_V0_TYPE_HONDA) {
        instance->fields.button = (uint8_t)(instance->generic.btn & 0x07U);
    } else {
        instance->fields.button = (uint8_t)(instance->generic.btn & 0x0FU);
    }
    kia_v0_encoder_apply_fields(instance);
}

#endif
#if RF_STREAM_WITH_ENCODER

static void kia_v0_encoder_apply_flipper_fields(
    RfStreamProtocolEncoderKIA* instance,
    bool got_serial,
    uint32_t ser_u32,
    bool got_btn,
    uint32_t btn_u32,
    bool got_cnt,
    uint32_t cnt_u32) {
    if(got_serial) instance->generic.serial = ser_u32;
    if(got_btn) instance->generic.btn = (uint8_t)btn_u32;
    if(got_cnt) instance->generic.cnt = cnt_u32;

    if(instance->type == KIA_V0_TYPE_HONDA) {
        instance->generic.data = kia_v0_build_honda_key(
            instance->generic.serial,
            instance->generic.btn & 0x07U,
            (uint16_t)(instance->generic.cnt & 0xFFFFU));
        instance->generic.data_count_bit = KIA_V0_BIT_COUNT_HONDA;
    } else if(instance->type == KIA_V0_TYPE_SUZUKI) {
        const uint8_t crc = kia_v0_suzuki_crc8_from_fields(
            instance->generic.serial, instance->generic.btn & 0x0FU, instance->generic.cnt);
        instance->generic.data = kia_v0_suzuki_shifted_key_from_fields(
            instance->generic.serial, instance->generic.btn & 0x0FU, instance->generic.cnt, crc);
        instance->generic.data_count_bit = KIA_V0_BIT_COUNT_SUZUKI;
    } else {
        uint64_t partial = kia_v0_build_kia_raw(
            instance->generic.serial,
            instance->generic.btn & 0x0FU,
            (uint16_t)(instance->generic.cnt & 0xFFFFU),
            0);
        uint8_t crc = kia_v0_calculate_crc_poly(partial);
        instance->generic.data = kia_v0_build_kia_raw(
            instance->generic.serial,
            instance->generic.btn & 0x0FU,
            (uint16_t)(instance->generic.cnt & 0xFFFFU),
            crc);
        instance->generic.data_count_bit = KIA_V0_BIT_COUNT_KIA;
    }
}

#endif
#if RF_STREAM_WITH_ENCODER

void* rf_stream_protocol_encoder_kia_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);

    RfStreamProtocolEncoderKIA* instance = malloc(sizeof(RfStreamProtocolEncoderKIA));
    memset(instance, 0, sizeof(*instance));

    instance->base.protocol = &kia_protocol_v0;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->encoder.repeat = KIA_V0_ENCODER_DEFAULT_REPEAT;
    rf_stream_encoder_buffer_ensure(instance, KIA_V0_UPLOAD_CAPACITY);

    return instance;
}

#endif
#if RF_STREAM_WITH_ENCODER

RfStreamProtocolStatus
    rf_stream_protocol_encoder_kia_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderKIA* instance = context;

    instance->encoder.is_running = false;
    instance->encoder.front = 0U;

    if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
       RfStreamProtocolStatusOk) {
        RfText* proto = rf_text_alloc();
        bool named_ok = false;
        rf_record_rewind(flipper_format);
        if(rf_record_read_string(flipper_format, FF_PROTOCOL, proto)) {
            named_ok = kia_v0_type_from_protocol_name(rf_text_get_cstr(proto)) != 0;
        }
        rf_text_free(proto);
        if(!named_ok) {
            return RfStreamProtocolStatusErrorParserProtocolName;
        }
    }

    static const uint16_t allowed_bits[] = {
        KIA_V0_BIT_COUNT_KIA, KIA_V0_BIT_COUNT_SUZUKI, KIA_V0_BIT_COUNT_HONDA};
    uint32_t bits = 0;
    RfStreamProtocolStatus bit_st = rf_stream_encoder_read_bit(flipper_format, allowed_bits, 3, &bits);
    if(bit_st != RfStreamProtocolStatusOk) return bit_st;

    uint32_t type_u32 = kia_v0_infer_type_from_bits(bits);
    uint32_t type_from_file = 0;
    rf_stream_encoder_read_fields(flipper_format, NULL, NULL, NULL, &type_from_file);
    if(type_from_file >= KIA_V0_TYPE_KIA && type_from_file <= KIA_V0_TYPE_MITSU) {
        type_u32 = type_from_file;
    } else {
        RfText* proto = rf_text_alloc();
        rf_record_rewind(flipper_format);
        if(rf_record_read_string(flipper_format, FF_PROTOCOL, proto)) {
            const uint8_t named = kia_v0_type_from_protocol_name(rf_text_get_cstr(proto));
            if(named) type_u32 = named;
        }
        rf_text_free(proto);
    }
    if(type_u32 < KIA_V0_TYPE_KIA || type_u32 > KIA_V0_TYPE_MITSU) {
        return RfStreamProtocolStatusErrorValueBitCount;
    }
    instance->type = (uint8_t)type_u32;

    rf_record_rewind(flipper_format);
    uint64_t key_from_hex = 0;
    bool have_key = rf_stream_flipper_read_hex_u64(flipper_format, FF_KEY, &key_from_hex);

    // Need to know which fields were actually present because the encoder
    // switches between "apply SBC overrides" and "use raw key" based on that.
    uint32_t ser_u32 = 0;
    uint32_t btn_u32 = 0;
    uint32_t cnt_u32 = 0;
    rf_record_rewind(flipper_format);
    const bool got_serial = rf_record_read_uint32(flipper_format, FF_SERIAL, &ser_u32, 1);
    rf_record_rewind(flipper_format);
    const bool got_btn = rf_record_read_uint32(flipper_format, FF_BTN, &btn_u32, 1);
    rf_record_rewind(flipper_format);
    const bool got_cnt = rf_record_read_uint32(flipper_format, FF_CNT, &cnt_u32, 1);

    if(got_serial || got_btn || got_cnt) {
        kia_v0_encoder_apply_flipper_fields(
            instance, got_serial, ser_u32, got_btn, btn_u32, got_cnt, cnt_u32);
    } else if(have_key) {
        instance->generic.data = key_from_hex;
        instance->generic.data_count_bit = bits;
        kia_v0_parse_data(&instance->generic, instance->type, &instance->fields, NULL);
    } else {
        return RfStreamProtocolStatusErrorParserOthers;
    }

    uint32_t repeat = rf_stream_encoder_read_repeat(flipper_format, KIA_V0_ENCODER_DEFAULT_REPEAT);
    if(repeat == 0U) repeat = KIA_V0_ENCODER_DEFAULT_REPEAT;
    instance->encoder.repeat = repeat;

    kia_v0_encoder_sync_from_generic(instance);

    char key_out[24];
    snprintf(key_out, sizeof(key_out), "%016llX", (unsigned long long)instance->generic.data);
    rf_record_rewind(flipper_format);
    rf_record_insert_or_update_string_cstr(flipper_format, FF_KEY, key_out);
    uint32_t bit_u32 = instance->generic.data_count_bit;
    rf_record_insert_or_update_uint32(flipper_format, FF_BIT, &bit_u32, 1);
    uint32_t type_w = instance->type;
    rf_record_insert_or_update_uint32(flipper_format, FF_TYPE, &type_w, 1);

    instance->encoder.front = 0;
    instance->encoder.is_running = true;

    return RfStreamProtocolStatusOk;
}

#endif
#if RF_STREAM_WITH_ENCODER

void rf_stream_protocol_encoder_kia_set_button(void* context, uint8_t button) {
    RfStreamProtocolEncoderKIA* instance = context;
    if(instance->type == KIA_V0_TYPE_HONDA) {
        instance->generic.btn = button & 0x07U;
    } else {
        instance->generic.btn = button & 0x0FU;
    }
    kia_v0_encoder_sync_from_generic(instance);
}

#endif
#if RF_STREAM_WITH_ENCODER

void rf_stream_protocol_encoder_kia_set_counter(void* context, uint16_t counter) {
    RfStreamProtocolEncoderKIA* instance = context;
    instance->generic.cnt = counter;
    kia_v0_encoder_sync_from_generic(instance);
}

#endif
#if RF_STREAM_WITH_ENCODER

void rf_stream_protocol_encoder_kia_increment_counter(void* context) {
    RfStreamProtocolEncoderKIA* instance = context;
    instance->generic.cnt = (uint16_t)(instance->generic.cnt + 1U);
    kia_v0_encoder_sync_from_generic(instance);
}

#endif
#if RF_STREAM_WITH_ENCODER

uint16_t rf_stream_protocol_encoder_kia_get_counter(void* context) {
    RfStreamProtocolEncoderKIA* instance = context;
    return (uint16_t)(instance->generic.cnt & 0xFFFFU);
}

#endif
#if RF_STREAM_WITH_ENCODER

uint8_t rf_stream_protocol_encoder_kia_get_button(void* context) {
    RfStreamProtocolEncoderKIA* instance = context;
    return (uint8_t)instance->generic.btn;
}

#endif

void* rf_stream_protocol_decoder_kia_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);

    RfStreamProtocolDecoderKIA* instance = malloc(sizeof(RfStreamProtocolDecoderKIA));
    memset(instance, 0, sizeof(*instance));

    instance->base.protocol = &kia_protocol_v0;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void rf_stream_protocol_decoder_kia_reset(void* context) {
    RfStreamProtocolDecoderKIA* instance = context;
    kia_v0_decoder_state_clear(instance);
    instance->type = 0;
    instance->have_last_kia = false;
    instance->last_kia_data = 0;
}

void rf_stream_protocol_decoder_kia_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderKIA* instance = context;

    switch(instance->decoder.parser_step) {
    case KiaV0DecoderStepReset:
        kia_v0_decoder_state_clear(instance);
        if(!level) {
            break;
        }
        if(!rf_stream_is_short(duration, &kia_protocol_v0_const)) {
            break;
        }
        instance->decoder.parser_step = KiaV0DecoderStepPreamble;
        instance->decoder.te_last = duration;
        instance->preamble_pairs = 0;
        break;

    case KiaV0DecoderStepPreamble:
        if(level) {
            if(rf_stream_is_short(duration, &kia_protocol_v0_const) ||
               rf_stream_is_long(duration, &kia_protocol_v0_const)) {
                instance->decoder.te_last = duration;
            } else {
                kia_v0_decoder_state_clear(instance);
            }
        } else if(
            rf_stream_is_short(duration, &kia_protocol_v0_const) &&
            rf_stream_is_short(instance->decoder.te_last, &kia_protocol_v0_const)) {
            instance->preamble_pairs++;
        } else if(
            rf_stream_is_long(duration, &kia_protocol_v0_const) &&
            rf_stream_is_long(instance->decoder.te_last, &kia_protocol_v0_const)) {
            if(instance->preamble_pairs > 14U) {
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                rf_stream_protocol_blocks_add_bit(&instance->decoder, 1U);
                rf_stream_protocol_blocks_add_bit(&instance->decoder, 1U);
                instance->decoder.parser_step = KiaV0DecoderStepSaveDuration;
            } else {
                kia_v0_decoder_state_clear(instance);
            }
        } else {
            kia_v0_decoder_state_clear(instance);
        }
        break;

    case KiaV0DecoderStepSaveDuration:
        if(!level) {
            kia_v0_decoder_state_clear(instance);
            break;
        }

        if(kia_v0_is_kia_gap(duration)) {
            kia_v0_decoder_finish_kia_or_honda_at_gap(instance);
            break;
        }

        if(kia_v0_is_suzuki_gap_strict(duration) &&
           kia_v0_is_suzuki_gap_strict(instance->decoder.te_last)) {
            if(instance->decoder.decode_count_bit == KIA_V0_BIT_COUNT_SUZUKI) {
                uint64_t shifted = 0;
                if(kia_v0_suzuki_resolve_shifted(instance->decoder.decode_data, &shifted)) {
                    kia_v0_decoder_commit(
                        instance, shifted, KIA_V0_TYPE_SUZUKI, KIA_V0_BIT_COUNT_SUZUKI);
                } else {
                    kia_v0_decoder_state_clear(instance);
                }
            } else {
                kia_v0_decoder_state_clear(instance);
            }
            break;
        }

        if(rf_stream_is_short(duration, &kia_protocol_v0_const) ||
           rf_stream_is_long(duration, &kia_protocol_v0_const)) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = KiaV0DecoderStepCheckDuration;
        } else {
            kia_v0_decoder_state_clear(instance);
        }
        break;

    case KiaV0DecoderStepCheckDuration:
        if(level) {
            kia_v0_decoder_state_clear(instance);
            break;
        }

        if(rf_stream_is_short(instance->decoder.te_last, &kia_protocol_v0_const) &&
           rf_stream_is_short(duration, &kia_protocol_v0_const)) {
            rf_stream_protocol_blocks_add_bit(&instance->decoder, 0U);
            if(!kia_v0_decoder_try_honda(instance)) {
                instance->decoder.parser_step = KiaV0DecoderStepSaveDuration;
            }
            break;
        }

        if(rf_stream_is_long(instance->decoder.te_last, &kia_protocol_v0_const) &&
           rf_stream_is_long(duration, &kia_protocol_v0_const)) {
            rf_stream_protocol_blocks_add_bit(&instance->decoder, 1U);
            if(!kia_v0_decoder_try_honda(instance)) {
                instance->decoder.parser_step = KiaV0DecoderStepSaveDuration;
            }
            break;
        }

        if(kia_v0_is_suzuki_gap_strict(duration)) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = KiaV0DecoderStepSaveDuration;
            break;
        }

        if(!kia_v0_decoder_try_honda(instance)) {
            kia_v0_decoder_state_clear(instance);
        }
        break;

    default:
        kia_v0_decoder_state_clear(instance);
        break;
    }
}

void rf_stream_protocol_decoder_kia_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderKIA* instance = context;
    KiaV0Fields fields;
    kia_v0_parse_data(&instance->generic, instance->type, &fields, &instance->packet_bit_count);

    const char* sn_fmt =
        (instance->type == KIA_V0_TYPE_HONDA) ?
            "%s %dbit\r\nKey:%016llX\r\nSn:%06lX Btn:%01X [%s]\r\nCnt:%04X CRC:%02X [%s]\r\n" :
            "%s %dbit\r\nKey:%016llX\r\nSn:%07lX Btn:%01X [%s]\r\nCnt:%04X CRC:%02X [%s]\r\n";
    rf_text_cat_printf(
        output,
        sn_fmt,
        kia_v0_protocol_name(instance->type),
        instance->packet_bit_count,
        (unsigned long long)instance->generic.data,
        fields.serial,
        fields.button,
        kia_v0_button_name(fields.button, instance->type),
        fields.counter,
        fields.crc,
        fields.crc_valid ? "OK" : "ERR");
}

RfStreamProtocolStatus rf_stream_protocol_decoder_kia_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderKIA* instance = context;
    KiaV0Fields scratch;
    kia_v0_parse_data(&instance->generic, instance->type, &scratch, &instance->packet_bit_count);

    RfStreamProtocolStatus status =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }

    return rf_stream_serialize_fields(
        flipper_format,
        RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT | RF_FIELD_TYPE,
        instance->generic.serial,
        instance->generic.btn,
        instance->generic.cnt,
        instance->type);
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_kia_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderKIA* instance = context;
    RfStreamProtocolStatus status =
        rf_stream_block_generic_deserialize(&instance->generic, flipper_format);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }

    uint32_t bits = instance->generic.data_count_bit;
    if((bits != KIA_V0_BIT_COUNT_KIA) && (bits != KIA_V0_BIT_COUNT_SUZUKI) &&
       (bits != KIA_V0_BIT_COUNT_HONDA)) {
        return RfStreamProtocolStatusErrorValueBitCount;
    }

    uint32_t type_u32 = kia_v0_infer_type_from_bits(bits);
    rf_record_rewind(flipper_format);
    if(rf_record_read_uint32(flipper_format, FF_TYPE, &type_u32, 1) &&
       (type_u32 >= KIA_V0_TYPE_KIA) && (type_u32 <= KIA_V0_TYPE_MITSU)) {
        instance->type = (uint8_t)type_u32;
    } else {
        RfText* proto = rf_text_alloc();
        uint8_t named = 0;
        rf_record_rewind(flipper_format);
        if(rf_record_read_string(flipper_format, FF_PROTOCOL, proto)) {
            named = kia_v0_type_from_protocol_name(rf_text_get_cstr(proto));
        }
        rf_text_free(proto);
        instance->type = named ? named : (uint8_t)kia_v0_infer_type_from_bits(bits);
    }

    KiaV0Fields scratch;
    kia_v0_parse_data(&instance->generic, instance->type, &scratch, &instance->packet_bit_count);

    uint32_t ser_u32 = 0;
    uint32_t btn_u32 = 0;
    uint32_t cnt_u32 = 0;
    bool got_serial = false;
    bool got_btn = false;
    bool got_cnt = false;

    rf_record_rewind(flipper_format);
    got_serial = rf_record_read_uint32(flipper_format, FF_SERIAL, &ser_u32, 1);
    rf_record_rewind(flipper_format);
    got_btn = rf_record_read_uint32(flipper_format, FF_BTN, &btn_u32, 1);
    rf_record_rewind(flipper_format);
    got_cnt = rf_record_read_uint32(flipper_format, FF_CNT, &cnt_u32, 1);

    if(got_serial || got_btn || got_cnt) {
        if(got_serial) instance->generic.serial = ser_u32;
        if(got_btn) instance->generic.btn = (uint8_t)btn_u32;
        if(got_cnt) instance->generic.cnt = (uint16_t)cnt_u32;

        if(instance->type == KIA_V0_TYPE_HONDA) {
            instance->generic.data = kia_v0_build_honda_key(
                instance->generic.serial,
                instance->generic.btn & 0x07U,
                (uint16_t)(instance->generic.cnt & 0xFFFFU));
        } else if(instance->type == KIA_V0_TYPE_SUZUKI) {
            const uint8_t crc = kia_v0_suzuki_crc8_from_fields(
                instance->generic.serial, instance->generic.btn & 0x0FU, instance->generic.cnt);
            instance->generic.data = kia_v0_suzuki_shifted_key_from_fields(
                instance->generic.serial,
                instance->generic.btn & 0x0FU,
                instance->generic.cnt,
                crc);
        } else {
            uint64_t partial = kia_v0_build_kia_raw(
                instance->generic.serial,
                instance->generic.btn & 0x0FU,
                (uint16_t)(instance->generic.cnt & 0xFFFFU),
                0);
            uint8_t crc = kia_v0_calculate_crc_poly(partial);
            instance->generic.data = kia_v0_build_kia_raw(
                instance->generic.serial,
                instance->generic.btn & 0x0FU,
                (uint16_t)(instance->generic.cnt & 0xFFFFU),
                crc);
        }
        instance->generic.data_count_bit =
            (instance->type == KIA_V0_TYPE_SUZUKI) ? KIA_V0_BIT_COUNT_SUZUKI :
            (instance->type == KIA_V0_TYPE_HONDA)  ? KIA_V0_BIT_COUNT_HONDA :
                                                     KIA_V0_BIT_COUNT_KIA;
        kia_v0_parse_data(
            &instance->generic, instance->type, &scratch, &instance->packet_bit_count);
    }

    return RfStreamProtocolStatusOk;
}

const RfStreamProtocolDecoder rf_stream_protocol_kia_decoder = {
    .alloc = rf_stream_protocol_decoder_kia_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_kia_feed,
    .reset = rf_stream_protocol_decoder_kia_reset,
    .get_hash_data = rf_stream_decoder_hash_blocks,
    .serialize = rf_stream_protocol_decoder_kia_serialize,
    .deserialize = rf_stream_protocol_decoder_kia_deserialize,
    .get_string = rf_stream_protocol_decoder_kia_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_kia_encoder = {
    .alloc = rf_stream_protocol_encoder_kia_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = rf_stream_protocol_encoder_kia_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_kia_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec kia_protocol_v0 = {
    .name = KIA_PROTOCOL_V0_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_kia_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_kia_encoder,
#else
    .encoder = NULL,
#endif
};

#endif
