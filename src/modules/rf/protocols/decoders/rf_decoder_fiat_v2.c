// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define FIAT_V2_PROTOCOL_NAME "Fiat V2"

typedef struct RfStreamProtocolDecoderFiatV2 RfStreamProtocolDecoderFiatV2;

extern const RfStreamCodec fiat_v2_protocol;

void* rf_stream_protocol_decoder_fiat_v2_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_fiat_v2_reset(void* context);
void rf_stream_protocol_decoder_fiat_v2_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_fiat_v2_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_fiat_v2_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_fiat_v2_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_fiat_v2_get_string(void* context, RfText* output);



#include <string.h>

#define TAG "FiatProtocolV2"

#define FIAT_V2_TE_SHORT        210U
#define FIAT_V2_TE_LONG         420U
#define FIAT_V2_TE_DELTA        100U
#define FIAT_V2_BOUNDARY_MIN_US 900U
#define FIAT_V2_WIRE_BITS       112U
#define FIAT_V2_WIRE_BYTES      14U
#define FIAT_V2_WIRE_CELLS      (FIAT_V2_WIRE_BITS * 2U)
#define FIAT_V2_LOGICAL_BITS    112U
#define FIAT_V2_MARKER0         0x00U
#define FIAT_V2_MARKER1         0x01U
#define FIAT_V2_BTN_SHIFT       6U
#define FIAT_V2_BUTTON_LOCK     0x2U
#define FIAT_V2_BUTTON_UNLOCK   0x3U
#define FIAT_V2_BUTTON_TRUNK    0x1U
#define FIAT_V2_CNT_SHIFT       3U
#define FIAT_V2_FCA_TYPE_NIBBLE 0xD0U
#define FIAT_V2_RAW_FIELD       "Raw"
#define FIAT_V2_HOP_FIELD       "Hop"
#define FIAT_V2_BTN_FIELD       "Btn"

static const RfStreamBlockConst rf_stream_protocol_fiat_v2_const = {
    .te_short = FIAT_V2_TE_SHORT,
    .te_long = FIAT_V2_TE_LONG,
    .te_delta = FIAT_V2_TE_DELTA,
    .min_count_bit_for_found = FIAT_V2_LOGICAL_BITS,
};

typedef enum {
    FiatV2DecoderStepReset = 0,
    FiatV2DecoderStepData = 1,
} FiatV2DecoderStep;

struct RfStreamProtocolDecoderFiatV2 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    uint8_t cells[FIAT_V2_WIRE_CELLS];
    uint16_t cell_count;

    uint8_t raw_data[FIAT_V2_WIRE_BYTES];
    uint8_t last_raw_data[FIAT_V2_WIRE_BYTES];
    bool last_raw_valid;

    uint32_t uid;
    uint32_t hop;
    uint8_t button;
};

static bool
    fiat_v2_feed_data_pulse(RfStreamProtocolDecoderFiatV2* instance, bool level, uint32_t duration);
static bool fiat_v2_frame_valid(const uint8_t raw[FIAT_V2_WIRE_BYTES]);

const RfStreamProtocolDecoder rf_stream_protocol_fiat_v2_decoder = {
    .alloc = rf_stream_protocol_decoder_fiat_v2_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_fiat_v2_feed,
    .reset = rf_stream_protocol_decoder_fiat_v2_reset,
    .get_hash_data = rf_stream_protocol_decoder_fiat_v2_get_hash_data,
    .serialize = rf_stream_protocol_decoder_fiat_v2_serialize,
    .deserialize = rf_stream_protocol_decoder_fiat_v2_deserialize,
    .get_string = rf_stream_protocol_decoder_fiat_v2_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_fiat_v2_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec fiat_v2_protocol = {
    .name = FIAT_V2_PROTOCOL_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_fiat_v2_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_fiat_v2_encoder,
#else
    .encoder = NULL,
#endif
};

static bool fiat_v2_duration_is_short(uint32_t duration) {
    return rf_stream_is_short(duration, &rf_stream_protocol_fiat_v2_const);
}

static bool fiat_v2_duration_is_long(uint32_t duration) {
    return rf_stream_is_long(duration, &rf_stream_protocol_fiat_v2_const);
}

static bool fiat_v2_button_valid(uint8_t button) {
    const uint8_t sel = button >> FIAT_V2_BTN_SHIFT;
    return sel == FIAT_V2_BUTTON_LOCK || sel == FIAT_V2_BUTTON_UNLOCK ||
           sel == FIAT_V2_BUTTON_TRUNK;
}

static const char* fiat_v2_button_name(uint8_t button) {
    switch(button >> FIAT_V2_BTN_SHIFT) {
    case FIAT_V2_BUTTON_LOCK:
        return "Lock";
    case FIAT_V2_BUTTON_UNLOCK:
        return "Unlock";
    case FIAT_V2_BUTTON_TRUNK:
        return "Trunk";
    default:
        return "Unknown";
    }
}

static uint32_t fiat_v2_uid(const uint8_t raw[FIAT_V2_WIRE_BYTES]) {
    return ((uint32_t)raw[2] << 24U) | ((uint32_t)raw[3] << 16U) | ((uint32_t)raw[4] << 8U) |
           raw[5];
}

static bool fiat_v2_is_fca(const uint8_t raw[FIAT_V2_WIRE_BYTES]) {
    return (raw[6] & 0xF0U) == FIAT_V2_FCA_TYPE_NIBBLE;
}
static uint32_t fiat_v2_hop(const uint8_t raw[FIAT_V2_WIRE_BYTES]) {
    if(fiat_v2_is_fca(raw)) {
        return ((uint32_t)raw[10] << 24U) | ((uint32_t)raw[11] << 16U) |
               ((uint32_t)raw[12] << 8U) | raw[13];
    }
    return ((uint32_t)raw[9] << 24U) | ((uint32_t)raw[10] << 16U) | ((uint32_t)raw[11] << 8U) |
           raw[12];
}

static uint32_t fiat_v2_counter(const uint8_t raw[FIAT_V2_WIRE_BYTES]) {
    if(fiat_v2_is_fca(raw)) {
        const uint32_t raw_cnt = ((uint32_t)raw[8] << 6U) | (uint32_t)(raw[9] >> 2U);
        return (~raw_cnt) & 0x3FFFU;
    }
    const uint32_t raw_cnt = ((uint32_t)(raw[7] & 0x3FU) << 5U) |
                             (uint32_t)(raw[8] >> FIAT_V2_CNT_SHIFT);
    return (~raw_cnt) & 0x7FFU;
}

static bool fiat_v2_frame_valid(const uint8_t raw[FIAT_V2_WIRE_BYTES]) {
    if(raw[0] != FIAT_V2_MARKER0 || raw[1] != FIAT_V2_MARKER1) {
        return false;
    }
    if(!fiat_v2_button_valid(raw[7])) {
        return false;
    }

    const uint32_t uid = fiat_v2_uid(raw);
    return uid != 0U && uid != UINT32_MAX;
}

static void fiat_v2_clear_cells(RfStreamProtocolDecoderFiatV2* instance) {
    instance->cell_count = 0U;
    memset(instance->cells, 0, sizeof(instance->cells));
}

static void fiat_v2_decode_fields(RfStreamProtocolDecoderFiatV2* instance) {
    instance->uid = fiat_v2_uid(instance->raw_data);
    instance->button = instance->raw_data[7];
    instance->hop = fiat_v2_hop(instance->raw_data);
    instance->generic.serial = instance->uid;
    instance->generic.btn = instance->button;
    instance->generic.cnt = fiat_v2_counter(instance->raw_data);
    instance->generic.data = ((uint64_t)instance->generic.serial << 32U) | instance->hop;
    instance->generic.data_count_bit = FIAT_V2_LOGICAL_BITS;
    instance->decoder.decode_data = instance->generic.data;
    instance->decoder.decode_count_bit = instance->generic.data_count_bit;
}

static bool
    fiat_v2_commit(RfStreamProtocolDecoderFiatV2* instance, const uint8_t raw[FIAT_V2_WIRE_BYTES]) {
    if(!fiat_v2_frame_valid(raw)) {
        return false;
    }

    if(instance->last_raw_valid && memcmp(instance->last_raw_data, raw, FIAT_V2_WIRE_BYTES) == 0) {
        return true;
    }

    memcpy(instance->raw_data, raw, FIAT_V2_WIRE_BYTES);
    memcpy(instance->last_raw_data, raw, FIAT_V2_WIRE_BYTES);
    instance->last_raw_valid = true;
    fiat_v2_decode_fields(instance);

    RF_STREAM_LOG_D(
        TAG,
        "Accepted UID:%08lX Btn:%02X Cnt:%02lX Hop:%08lX",
        (unsigned long)instance->uid,
        instance->button,
        (unsigned long)instance->generic.cnt,
        (unsigned long)instance->hop);

    if(instance->base.callback) {
        instance->base.callback(&instance->base, instance->base.context);
    }
    return true;
}

static bool fiat_v2_try_decode_window(RfStreamProtocolDecoderFiatV2* instance, bool invert) {
    if(instance->cell_count != FIAT_V2_WIRE_CELLS) {
        return false;
    }

    uint8_t raw[FIAT_V2_WIRE_BYTES] = {0};
    for(uint8_t bit_index = 0U; bit_index < FIAT_V2_WIRE_BITS; bit_index++) {
        const uint8_t first = instance->cells[bit_index * 2U];
        const uint8_t second = instance->cells[bit_index * 2U + 1U];
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

    return fiat_v2_commit(instance, raw);
}

static void fiat_v2_try_decode(RfStreamProtocolDecoderFiatV2* instance) {
    if(fiat_v2_try_decode_window(instance, false)) {
        return;
    }
    (void)fiat_v2_try_decode_window(instance, true);
}

static void fiat_v2_push_cell(RfStreamProtocolDecoderFiatV2* instance, bool level) {
    if(instance->cell_count < FIAT_V2_WIRE_CELLS) {
        instance->cells[instance->cell_count++] = level ? 1U : 0U;
    } else {
        memmove(instance->cells, &instance->cells[1], FIAT_V2_WIRE_CELLS - 1U);
        instance->cells[FIAT_V2_WIRE_CELLS - 1U] = level ? 1U : 0U;
    }
    fiat_v2_try_decode(instance);
}

static bool
    fiat_v2_feed_data_pulse(RfStreamProtocolDecoderFiatV2* instance, bool level, uint32_t duration) {
    if(fiat_v2_duration_is_short(duration)) {
        fiat_v2_push_cell(instance, level);
        return true;
    }

    if(fiat_v2_duration_is_long(duration)) {
        fiat_v2_push_cell(instance, level);
        fiat_v2_push_cell(instance, level);
        return true;
    }

    if(!level && duration >= FIAT_V2_BOUNDARY_MIN_US) {
        fiat_v2_push_cell(instance, false);
    }
    fiat_v2_clear_cells(instance);
    return false;
}

void* rf_stream_protocol_decoder_fiat_v2_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderFiatV2* instance = calloc(1, sizeof(RfStreamProtocolDecoderFiatV2));
    instance->base.protocol = &fiat_v2_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    rf_stream_protocol_decoder_fiat_v2_reset(instance);
    return instance;
}

void rf_stream_protocol_decoder_fiat_v2_reset(void* context) {
    RfStreamProtocolDecoderFiatV2* instance = context;

    memset(instance->raw_data, 0, sizeof(instance->raw_data));
    memset(instance->last_raw_data, 0, sizeof(instance->last_raw_data));
    instance->decoder.parser_step = FiatV2DecoderStepReset;
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
    instance->button = 0U;
    fiat_v2_clear_cells(instance);
}

void rf_stream_protocol_decoder_fiat_v2_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderFiatV2* instance = context;

    switch(instance->decoder.parser_step) {
    case FiatV2DecoderStepReset:
        if(fiat_v2_duration_is_short(duration) || fiat_v2_duration_is_long(duration)) {
            fiat_v2_clear_cells(instance);
            instance->decoder.parser_step = FiatV2DecoderStepData;
            (void)fiat_v2_feed_data_pulse(instance, level, duration);
        }
        break;

    case FiatV2DecoderStepData:
        if(!fiat_v2_feed_data_pulse(instance, level, duration)) {
            instance->decoder.parser_step = FiatV2DecoderStepReset;
        }
        break;
    }
}

uint8_t rf_stream_protocol_decoder_fiat_v2_get_hash_data(void* context) {
    RfStreamProtocolDecoderFiatV2* instance = context;
    RfStreamBlockDecoder decoder = {
        .decode_data = instance->generic.data,
        .decode_count_bit = 64U,
    };
    return rf_stream_protocol_blocks_get_hash_data(&decoder, 8U) ^ instance->generic.cnt ^
           instance->button;
}

RfStreamProtocolStatus rf_stream_protocol_decoder_fiat_v2_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderFiatV2* instance = context;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
    if(ret != RfStreamProtocolStatusOk) {
        return ret;
    }

    rf_record_rewind(flipper_format);
    rf_record_delete_key(flipper_format, FIAT_V2_RAW_FIELD);

    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_SERIAL, instance->generic.serial);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_BTN, instance->generic.btn);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_CNT, instance->generic.cnt);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FIAT_V2_HOP_FIELD, instance->hop);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FIAT_V2_BTN_FIELD, instance->button);

    if(!rf_record_insert_or_update_hex(
           flipper_format, FIAT_V2_RAW_FIELD, instance->raw_data, FIAT_V2_WIRE_BYTES)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }
    return RfStreamProtocolStatusOk;
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_fiat_v2_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderFiatV2* instance = context;

    RfStreamProtocolStatus ret = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, rf_stream_protocol_fiat_v2_const.min_count_bit_for_found);
    if(ret != RfStreamProtocolStatusOk) {
        return ret;
    }
    if(instance->generic.data_count_bit != FIAT_V2_LOGICAL_BITS) {
        return RfStreamProtocolStatusErrorValueBitCount;
    }

    rf_record_rewind(flipper_format);
    if(rf_record_read_hex(
           flipper_format, FIAT_V2_RAW_FIELD, instance->raw_data, FIAT_V2_WIRE_BYTES)) {
        if(!fiat_v2_frame_valid(instance->raw_data)) {
            return RfStreamProtocolStatusErrorParserOthers;
        }
        fiat_v2_decode_fields(instance);
        return RfStreamProtocolStatusOk;
    }

    return RfStreamProtocolStatusErrorParserOthers;
}

void rf_stream_protocol_decoder_fiat_v2_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderFiatV2* instance = context;

    rf_text_cat_printf(
        output,
        "%s %ubit\r\n"
        "UID:%08lX\r\n"
        "Hop:%08lX Type:%01X\r\n"
        "Btn:%02X [%s] Cnt:%02lX\r\n",
        instance->generic.protocol_name,
        FIAT_V2_LOGICAL_BITS,
        (unsigned long)instance->uid,
        (unsigned long)instance->hop,
        (unsigned)(instance->raw_data[6] >> 4),
        instance->button,
        fiat_v2_button_name(instance->button),
        (unsigned long)instance->generic.cnt);
}

#endif
