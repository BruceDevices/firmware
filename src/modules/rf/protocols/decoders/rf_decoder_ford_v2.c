// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define FORD_PROTOCOL_V2_NAME "Ford V2"

extern const RfStreamCodec ford_protocol_v2;

void* rf_stream_protocol_decoder_ford_v2_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_ford_v2_free(void* context);
void rf_stream_protocol_decoder_ford_v2_reset(void* context);
void rf_stream_protocol_decoder_ford_v2_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_ford_v2_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_ford_v2_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_ford_v2_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_ford_v2_get_string(void* context, RfText* output);

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_ford_v2_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_ford_v2_deserialize(void* context, RfRecord* flipper_format);
extern const RfStreamProtocolEncoder rf_stream_protocol_ford_v2_encoder;
#endif




#include <string.h>

#define FORD_V2_TE_SHORT               200U
#define FORD_V2_TE_LONG                400U
#define FORD_V2_TE_DELTA               260U
#define FORD_V2_INTER_BURST_GAP_US     15000U
#define FORD_V2_PREAMBLE_MIN           64U
#define FORD_V2_DATA_BITS              104U
#define FORD_V2_DATA_BYTES             13U
#define FORD_V2_SYNC_0                 0x7FU
#define FORD_V2_SYNC_1                 0xA7U
#define FORD_V2_ENC_TE_SHORT           240U
#define FORD_V2_ENC_PREAMBLE_PAIRS     70U
#define FORD_V2_ENC_BURST_COUNT        6U
#define FORD_V2_ENC_INTER_BURST_GAP_US 16000U
#define FORD_V2_ENC_SEPARATOR_ELEMS    2U
#define FORD_V2_ENC_PREAMBLE_ELEMS     (FORD_V2_ENC_PREAMBLE_PAIRS * 2U)
#define FORD_V2_ENC_DATA_ELEMS         ((FORD_V2_DATA_BITS - 1U) * 2U)
#define FORD_V2_ENC_BURST_ELEMS \
    (FORD_V2_ENC_PREAMBLE_ELEMS + FORD_V2_ENC_SEPARATOR_ELEMS + FORD_V2_ENC_DATA_ELEMS)
#define FORD_V2_ENC_UPLOAD_ELEMS \
    (FORD_V2_ENC_BURST_COUNT * FORD_V2_ENC_BURST_ELEMS + (FORD_V2_ENC_BURST_COUNT - 1U))
#define FORD_V2_ENC_SYNC_LO_US 476U

#define FORD_V2_SYNC_BITS                  16U
#define FORD_V2_POST_SYNC_DECODE_COUNT_BIT 16U
#define FORD_V2_KEY_BYTE_COUNT             8U
#define FORD_V2_TAIL_RAW_BYTE_COUNT        5U
#define FORD_V2_PREAMBLE_COUNT_MAX         0xFFFFU
#define FORD_V2_ENCODER_DEFAULT_REPEAT     10U

static const uint16_t ford_v2_sync_shift16_inv =
    (uint16_t)(~(((uint16_t)FORD_V2_SYNC_0 << 8) | (uint16_t)FORD_V2_SYNC_1));

static const RfStreamBlockConst rf_stream_protocol_ford_v2_const = {
    .te_short = FORD_V2_TE_SHORT,
    .te_long = FORD_V2_TE_LONG,
    .te_delta = FORD_V2_TE_DELTA,
    .min_count_bit_for_found = FORD_V2_DATA_BITS,
};

typedef enum {
    FordV2DecoderStepReset = 0,
    FordV2DecoderStepPreamble = 1,
    FordV2DecoderStepSync = 2,
    FordV2DecoderStepData = 3,
} FordV2DecoderStep;

typedef struct RfStreamProtocolDecoderFordV2 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    RfManchesterState manchester_state;
    uint16_t preamble_count;

    uint8_t raw_bytes[FORD_V2_DATA_BYTES];
    uint8_t byte_count;

    uint16_t sync_shift;
    uint8_t sync_bit_count;

    uint64_t extra_data;
    uint16_t counter16;
    uint32_t tail31;
    bool structure_ok;
} RfStreamProtocolDecoderFordV2;

#if RF_STREAM_WITH_ENCODER
typedef struct RfStreamProtocolEncoderFordV2 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;
    uint64_t extra_data;
    uint8_t raw_bytes[FORD_V2_DATA_BYTES];
} RfStreamProtocolEncoderFordV2;
#endif

static void ford_v2_decoder_manchester_feed_event(
    RfStreamProtocolDecoderFordV2* instance,
    RfManchesterEvent event);

static void ford_v2_decoder_reset_state(RfStreamProtocolDecoderFordV2* instance) {
    instance->decoder.parser_step = FordV2DecoderStepReset;
    instance->decoder.decode_data = 0;
    instance->decoder.decode_count_bit = 0;
    instance->decoder.te_last = 0;

    instance->byte_count = 0;
    instance->sync_shift = 0;
    instance->sync_bit_count = 0;
    instance->preamble_count = 0;
    instance->counter16 = 0;
    instance->tail31 = 0;
    instance->structure_ok = false;

    memset(instance->raw_bytes, 0, sizeof(instance->raw_bytes));

    rf_manchester_advance(
        instance->manchester_state, RfManchesterEventReset, &instance->manchester_state, NULL);
}

static bool ford_v2_duration_is_short(uint32_t duration) {
    return DURATION_DIFF(duration, FORD_V2_TE_SHORT) < (int32_t)FORD_V2_TE_DELTA;
}

static bool ford_v2_duration_is_long(uint32_t duration) {
    return DURATION_DIFF(duration, FORD_V2_TE_LONG) < (int32_t)FORD_V2_TE_DELTA;
}

static bool ford_v2_button_is_valid(uint8_t btn) {
    switch(btn) {
    case 0x10:
    case 0x11:
    case 0x13:
    case 0x14:
    case 0x15:
        return true;
    default:
        return false;
    }
}

#if RF_STREAM_WITH_ENCODER
static uint8_t ford_v2_uint8_parity(uint8_t value) {
    uint8_t parity = 0U;
    while(value) {
        parity ^= (value & 1U);
        value >>= 1U;
    }
    return parity;
}
#endif

static const char* ford_v2_button_name(uint8_t btn) {
    switch(btn) {
    case 0x10:
        return "Lock";
    case 0x11:
        return "Unlock";
    case 0x13:
        return "Trunk";
    case 0x14:
        return "Panic";
    case 0x15:
        return "RemoteStart";
    default:
        return "Unknown";
    }
}

static void ford_v2_decoder_extract_from_raw(RfStreamProtocolDecoderFordV2* instance) {
    const uint8_t* k = instance->raw_bytes;

    instance->generic.serial = ((uint32_t)k[2] << 24) | ((uint32_t)k[3] << 16) |
                               ((uint32_t)k[4] << 8) | (uint32_t)k[5];

    instance->generic.btn = k[6];

    instance->counter16 = (uint16_t)((((uint16_t)(k[7] & 0x7FU)) << 9) | (((uint16_t)k[8]) << 1) |
                                     ((uint16_t)(k[9] >> 7)));

    instance->generic.cnt = instance->counter16;

    instance->tail31 = (((uint32_t)(k[9] & 0x7FU)) << 24) | ((uint32_t)k[10] << 16) |
                       ((uint32_t)k[11] << 8) | (uint32_t)k[12];

    instance->structure_ok = true;

    if(k[0] != FORD_V2_SYNC_0) instance->structure_ok = false;
    if(k[1] != FORD_V2_SYNC_1) instance->structure_ok = false;
    if(!ford_v2_button_is_valid(k[6])) instance->structure_ok = false;

    if((k[7] & 0x7FU) != (uint8_t)((instance->counter16 >> 9) & 0x7FU)) {
        instance->structure_ok = false;
    }

    if(k[8] != (uint8_t)((instance->counter16 >> 1) & 0xFFU)) {
        instance->structure_ok = false;
    }

    if(((k[9] >> 7) & 1U) != (uint8_t)(instance->counter16 & 1U)) {
        instance->structure_ok = false;
    }

    instance->generic.data = 0;
    for(uint8_t i = 0; i < FORD_V2_KEY_BYTE_COUNT; i++) {
        instance->generic.data = (instance->generic.data << 8) | (uint64_t)k[i];
    }

    instance->generic.data_count_bit = FORD_V2_DATA_BITS;

    instance->extra_data = 0;
    for(uint8_t i = 0; i < FORD_V2_TAIL_RAW_BYTE_COUNT; i++) {
        instance->extra_data = (instance->extra_data << 8) | (uint64_t)k[8U + i];
    }
}

static bool ford_v2_decoder_commit_frame(RfStreamProtocolDecoderFordV2* instance) {
    if(instance->raw_bytes[0] != FORD_V2_SYNC_0 || instance->raw_bytes[1] != FORD_V2_SYNC_1) {
        return false;
    }

    ford_v2_decoder_extract_from_raw(instance);

    if(!instance->structure_ok) {
        return false;
    }

    if(instance->base.callback) {
        instance->base.callback(&instance->base, instance->base.context);
    }

    return true;
}

static void ford_v2_decoder_sync_enter_data(RfStreamProtocolDecoderFordV2* instance) {
    memset(instance->raw_bytes, 0, sizeof(instance->raw_bytes));
    instance->raw_bytes[0] = FORD_V2_SYNC_0;
    instance->raw_bytes[1] = FORD_V2_SYNC_1;
    instance->byte_count = 2U;
    instance->decoder.parser_step = FordV2DecoderStepData;
    instance->decoder.decode_data = 0;
    instance->decoder.decode_count_bit = FORD_V2_POST_SYNC_DECODE_COUNT_BIT;
}

static bool
    ford_v2_decoder_sync_feed_event(RfStreamProtocolDecoderFordV2* instance, RfManchesterEvent event) {
    bool data_bit;

    if(!rf_manchester_advance(
           instance->manchester_state, event, &instance->manchester_state, &data_bit)) {
        return false;
    }

    instance->sync_shift = (uint16_t)((instance->sync_shift << 1) | (data_bit ? 1U : 0U));
    if(instance->sync_bit_count < FORD_V2_SYNC_BITS) {
        instance->sync_bit_count++;
    }

    return instance->sync_bit_count >= FORD_V2_SYNC_BITS &&
           instance->sync_shift == ford_v2_sync_shift16_inv;
}

static void ford_v2_decoder_manchester_feed_event(
    RfStreamProtocolDecoderFordV2* instance,
    RfManchesterEvent event) {
    bool data_bit;

    if(instance->decoder.parser_step == FordV2DecoderStepSync) {
        if(ford_v2_decoder_sync_feed_event(instance, event)) {
            ford_v2_decoder_sync_enter_data(instance);
        }
        return;
    }

    if(!rf_manchester_advance(
           instance->manchester_state, event, &instance->manchester_state, &data_bit)) {
        return;
    }

    if(instance->decoder.parser_step != FordV2DecoderStepData) {
        return;
    }

    data_bit = !data_bit;

    instance->decoder.decode_data = (instance->decoder.decode_data << 1) | (data_bit ? 1U : 0U);
    instance->decoder.decode_count_bit++;

    if((instance->decoder.decode_count_bit & 7U) == 0U) {
        uint8_t byte_val = (uint8_t)(instance->decoder.decode_data & 0xFFU);

        if(instance->byte_count < FORD_V2_DATA_BYTES) {
            instance->raw_bytes[instance->byte_count] = byte_val;
            instance->byte_count++;
        }

        instance->decoder.decode_data = 0;

        if(instance->byte_count == FORD_V2_DATA_BYTES) {
            (void)ford_v2_decoder_commit_frame(instance);
            ford_v2_decoder_reset_state(instance);
        }
    }
}

static bool ford_v2_decoder_manchester_feed_pulse(
    RfStreamProtocolDecoderFordV2* instance,
    bool level,
    uint32_t duration) {
    if(ford_v2_duration_is_short(duration)) {
        RfManchesterEvent ev = level ? RfManchesterEventShortHigh : RfManchesterEventShortLow;
        ford_v2_decoder_manchester_feed_event(instance, ev);
        return true;
    }

    if(ford_v2_duration_is_long(duration)) {
        RfManchesterEvent ev = level ? RfManchesterEventLongHigh : RfManchesterEventLongLow;
        ford_v2_decoder_manchester_feed_event(instance, ev);
        return true;
    }

    return false;
}

static void ford_v2_decoder_enter_sync_from_preamble(
    RfStreamProtocolDecoderFordV2* instance,
    bool level,
    uint32_t duration) {
    instance->decoder.parser_step = FordV2DecoderStepSync;
    instance->decoder.decode_data = 0;
    instance->decoder.decode_count_bit = 0;
    instance->byte_count = 0;
    instance->sync_shift = 0;
    instance->sync_bit_count = 0;
    memset(instance->raw_bytes, 0, sizeof(instance->raw_bytes));

    rf_manchester_advance(
        instance->manchester_state, RfManchesterEventReset, &instance->manchester_state, NULL);

    if(ford_v2_duration_is_short(duration)) {
        RfManchesterEvent ev = level ? RfManchesterEventShortHigh : RfManchesterEventShortLow;
        if(ev == RfManchesterEventShortLow || ev == RfManchesterEventLongLow) {
            instance->manchester_state = RfManchesterStateMid0;
        }
        ford_v2_decoder_manchester_feed_event(instance, ev);
    } else if(ford_v2_duration_is_long(duration)) {
        RfManchesterEvent ev = level ? RfManchesterEventLongHigh : RfManchesterEventLongLow;
        if(ev == RfManchesterEventShortLow || ev == RfManchesterEventLongLow) {
            instance->manchester_state = RfManchesterStateMid0;
        }
        ford_v2_decoder_manchester_feed_event(instance, ev);
    } else {
        ford_v2_decoder_reset_state(instance);
    }
}

static void ford_v2_decoder_rebuild_raw_buffer(RfStreamProtocolDecoderFordV2* instance) {
    for(uint8_t i = 0; i < FORD_V2_KEY_BYTE_COUNT; i++) {
        instance->raw_bytes[i] = (uint8_t)(instance->generic.data >> (56U - i * 8U));
    }

    for(uint8_t i = 0; i < FORD_V2_TAIL_RAW_BYTE_COUNT; i++) {
        instance->raw_bytes[8U + i] = (uint8_t)(instance->extra_data >> (32U - i * 8U));
    }
}

#if RF_STREAM_WITH_ENCODER
static inline void ford_v2_encoder_add_level(
    RfStreamProtocolEncoderFordV2* instance,
    bool level,
    uint32_t duration) {
    size_t idx = instance->encoder.size_upload;
    if(idx > 0 && level_duration_get_level(instance->encoder.upload[idx - 1]) == level) {
        uint32_t prev = level_duration_get_duration(instance->encoder.upload[idx - 1]);
        instance->encoder.upload[idx - 1] = level_duration_make(level, prev + duration);
    } else {
        instance->encoder.upload[idx] = level_duration_make(level, duration);
        instance->encoder.size_upload++;
    }
}

static void ford_v2_encoder_rebuild_raw_from_payload(RfStreamProtocolEncoderFordV2* instance) {
    for(uint8_t i = 0; i < FORD_V2_KEY_BYTE_COUNT; i++) {
        instance->raw_bytes[i] = (uint8_t)(instance->generic.data >> (56U - i * 8U));
    }

    for(uint8_t i = 0; i < FORD_V2_TAIL_RAW_BYTE_COUNT; i++) {
        instance->raw_bytes[8U + i] = (uint8_t)(instance->extra_data >> (32U - i * 8U));
    }

    const uint8_t btn = instance->raw_bytes[6];
    const uint8_t k7_msb = (uint8_t)(ford_v2_uint8_parity(btn) << 7);
    instance->raw_bytes[7] = (instance->raw_bytes[7] & 0x7FU) | k7_msb;
}

static void ford_v2_encoder_refresh_data_from_raw(RfStreamProtocolEncoderFordV2* instance) {
    instance->generic.data = 0;
    for(uint8_t i = 0; i < FORD_V2_KEY_BYTE_COUNT; i++) {
        instance->generic.data = (instance->generic.data << 8) | (uint64_t)instance->raw_bytes[i];
    }
}

static inline void
    ford_v2_encoder_emit_manchester_bit(RfStreamProtocolEncoderFordV2* instance, bool bit) {
    if(bit) {
        ford_v2_encoder_add_level(instance, true, FORD_V2_ENC_TE_SHORT);
        ford_v2_encoder_add_level(instance, false, FORD_V2_ENC_TE_SHORT);
    } else {
        ford_v2_encoder_add_level(instance, false, FORD_V2_ENC_TE_SHORT);
        ford_v2_encoder_add_level(instance, true, FORD_V2_ENC_TE_SHORT);
    }
}

static void ford_v2_encoder_emit_burst(RfStreamProtocolEncoderFordV2* instance) {
    for(uint8_t i = 0; i < FORD_V2_ENC_PREAMBLE_PAIRS; i++) {
        ford_v2_encoder_add_level(instance, false, FORD_V2_ENC_TE_SHORT);
        ford_v2_encoder_add_level(instance, true, FORD_V2_ENC_TE_SHORT);
    }

    ford_v2_encoder_add_level(instance, false, FORD_V2_ENC_SYNC_LO_US);
    ford_v2_encoder_add_level(instance, true, FORD_V2_ENC_TE_SHORT);

    for(uint16_t bit_pos = 1U; bit_pos < FORD_V2_DATA_BITS; bit_pos++) {
        const uint8_t byte_idx = (uint8_t)(bit_pos / 8U);
        const uint8_t bit_idx = (uint8_t)(7U - (bit_pos % 8U));
        ford_v2_encoder_emit_manchester_bit(
            instance, ((instance->raw_bytes[byte_idx] >> bit_idx) & 1U) != 0U);
    }
}

static void ford_v2_encoder_build_upload(RfStreamProtocolEncoderFordV2* instance) {
    instance->encoder.size_upload = 0;
    instance->encoder.front = 0;

    for(uint8_t burst = 0; burst < FORD_V2_ENC_BURST_COUNT; burst++) {
        ford_v2_encoder_emit_burst(instance);

        if(burst + 1U < FORD_V2_ENC_BURST_COUNT) {
            ford_v2_encoder_add_level(instance, true, FORD_V2_ENC_INTER_BURST_GAP_US);
        }
    }
}

static void ford_v2_encoder_read_optional_tail_raw(
    RfStreamProtocolEncoderFordV2* instance,
    RfRecord* flipper_format) {
    instance->extra_data = 0U;
    uint8_t tail_raw[FORD_V2_TAIL_RAW_BYTE_COUNT] = {0};
    rf_record_rewind(flipper_format);
    if(rf_record_read_hex(flipper_format, "TailRaw", tail_raw, sizeof(tail_raw))) {
        for(uint8_t i = 0; i < FORD_V2_TAIL_RAW_BYTE_COUNT; i++) {
            instance->extra_data = (instance->extra_data << 8) | (uint64_t)tail_raw[i];
        }
    }
}

static RfStreamProtocolStatus ford_v2_encoder_deserialize_read_header(
    RfStreamProtocolEncoderFordV2* instance,
    RfRecord* flipper_format,
    RfText* temp_str) {
    rf_record_rewind(flipper_format);
    if(!rf_record_read_string(flipper_format, "Protocol", temp_str)) {
        return RfStreamProtocolStatusError;
    }
    if(!rf_text_equal(temp_str, instance->base.protocol->name)) {
        return RfStreamProtocolStatusError;
    }

    RfStreamProtocolStatus g = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, FORD_V2_DATA_BITS);
    if(g != RfStreamProtocolStatusOk) {
        return g;
    }

    ford_v2_encoder_read_optional_tail_raw(instance, flipper_format);
    return RfStreamProtocolStatusOk;
}

static RfStreamProtocolStatus
    ford_v2_encoder_deserialize_validate_and_pack(RfStreamProtocolEncoderFordV2* instance) {
    ford_v2_encoder_rebuild_raw_from_payload(instance);

    if(!ford_v2_button_is_valid(instance->raw_bytes[6])) {
        return RfStreamProtocolStatusErrorParserOthers;
    }

    ford_v2_encoder_refresh_data_from_raw(instance);
    instance->generic.btn = instance->raw_bytes[6];
    instance->generic.serial =
        ((uint32_t)instance->raw_bytes[2] << 24) | ((uint32_t)instance->raw_bytes[3] << 16) |
        ((uint32_t)instance->raw_bytes[4] << 8) | (uint32_t)instance->raw_bytes[5];
    instance->generic.cnt = (uint16_t)((((uint16_t)(instance->raw_bytes[7] & 0x7FU)) << 9) |
                                       (((uint16_t)instance->raw_bytes[8]) << 1) |
                                       ((uint16_t)(instance->raw_bytes[9] >> 7)));

    return RfStreamProtocolStatusOk;
}

static void ford_v2_encoder_deserialize_apply_repeat(
    RfStreamProtocolEncoderFordV2* instance,
    RfRecord* flipper_format) {
    instance->encoder.repeat =
        (int32_t)rf_stream_encoder_read_repeat(flipper_format, FORD_V2_ENCODER_DEFAULT_REPEAT);
}

void* rf_stream_protocol_encoder_ford_v2_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolEncoderFordV2* instance = calloc(1, sizeof(RfStreamProtocolEncoderFordV2));

    instance->base.protocol = &ford_protocol_v2;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->encoder.repeat = FORD_V2_ENCODER_DEFAULT_REPEAT;
    instance->encoder.upload = calloc(FORD_V2_ENC_UPLOAD_ELEMS, sizeof(LevelDuration));

    return instance;
}

void rf_stream_protocol_encoder_ford_v2_free(void* context) {
    RfStreamProtocolEncoderFordV2* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_ford_v2_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderFordV2* instance = context;

    instance->encoder.is_running = false;
    instance->encoder.front = 0;
    instance->encoder.repeat = FORD_V2_ENCODER_DEFAULT_REPEAT;
    instance->generic.data_count_bit = FORD_V2_DATA_BITS;

    RfText* temp_str = rf_text_alloc();

    RfStreamProtocolStatus ret =
        ford_v2_encoder_deserialize_read_header(instance, flipper_format, temp_str);

    if(ret == RfStreamProtocolStatusOk) {
        ret = ford_v2_encoder_deserialize_validate_and_pack(instance);
    }

    if(ret == RfStreamProtocolStatusOk) {
        ford_v2_encoder_deserialize_apply_repeat(instance, flipper_format);
        ford_v2_encoder_build_upload(instance);
        instance->encoder.is_running = true;
    }

    rf_text_free(temp_str);
    return ret;
}

void rf_stream_protocol_encoder_ford_v2_stop(void* context) {
    RfStreamProtocolEncoderFordV2* instance = context;
    instance->encoder.is_running = false;
}

LevelDuration rf_stream_protocol_encoder_ford_v2_yield(void* context) {
    RfStreamProtocolEncoderFordV2* instance = context;

    if(!instance->encoder.is_running || instance->encoder.repeat == 0U) {
        instance->encoder.is_running = false;
        return level_duration_reset();
    }

    LevelDuration ret = instance->encoder.upload[instance->encoder.front];

    if(++instance->encoder.front == instance->encoder.size_upload) {
        instance->encoder.front = 0U;
        instance->encoder.repeat--;
    }

    return ret;
}
#endif

void* rf_stream_protocol_decoder_ford_v2_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);

    RfStreamProtocolDecoderFordV2* instance = calloc(1, sizeof(RfStreamProtocolDecoderFordV2));

    instance->base.protocol = &ford_protocol_v2;
    instance->generic.protocol_name = instance->base.protocol->name;

    return instance;
}

void rf_stream_protocol_decoder_ford_v2_free(void* context) {
    free(context);
}

void rf_stream_protocol_decoder_ford_v2_reset(void* context) {
    ford_v2_decoder_reset_state((RfStreamProtocolDecoderFordV2*)context);
}

void rf_stream_protocol_decoder_ford_v2_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderFordV2* instance = context;

    switch(instance->decoder.parser_step) {
    case FordV2DecoderStepReset:
        if(ford_v2_duration_is_short(duration)) {
            instance->preamble_count = 1U;
            instance->decoder.parser_step = FordV2DecoderStepPreamble;
        }
        break;

    case FordV2DecoderStepPreamble:
        if(ford_v2_duration_is_short(duration)) {
            if(instance->preamble_count < FORD_V2_PREAMBLE_COUNT_MAX) {
                instance->preamble_count++;
            }
        } else if(!level && ford_v2_duration_is_long(duration)) {
            if(instance->preamble_count >= FORD_V2_PREAMBLE_MIN) {
                ford_v2_decoder_enter_sync_from_preamble(instance, level, duration);
            } else {
                ford_v2_decoder_reset_state(instance);
            }
        } else {
            ford_v2_decoder_reset_state(instance);
        }
        break;

    case FordV2DecoderStepSync:
    case FordV2DecoderStepData:
        if(ford_v2_decoder_manchester_feed_pulse(instance, level, duration)) {
        } else {
            if(instance->decoder.parser_step == FordV2DecoderStepSync &&
               duration >= FORD_V2_INTER_BURST_GAP_US) {
                ford_v2_decoder_reset_state(instance);
                break;
            }
            if(instance->decoder.parser_step == FordV2DecoderStepSync) {
                ford_v2_decoder_reset_state(instance);
                break;
            }
            if(instance->decoder.parser_step == FordV2DecoderStepData) {
                if(duration >= FORD_V2_INTER_BURST_GAP_US) {
                    ford_v2_decoder_reset_state(instance);
                    break;
                }
            }
            ford_v2_decoder_reset_state(instance);
        }

        instance->decoder.te_last = duration;
        break;
    }
}

uint8_t rf_stream_protocol_decoder_ford_v2_get_hash_data(void* context) {
    RfStreamProtocolDecoderFordV2* instance = context;
    const uint8_t* k = instance->raw_bytes;

    const uint16_t cnt = (uint16_t)((((uint16_t)(k[7] & 0x7FU)) << 9) | (((uint16_t)k[8]) << 1) |
                                    ((uint16_t)(k[9] >> 7)));
    const uint32_t tail = (((uint32_t)(k[9] & 0x7FU)) << 24) | ((uint32_t)k[10] << 16) |
                          ((uint32_t)k[11] << 8) | (uint32_t)k[12];

    uint32_t mix = ((uint32_t)k[2] << 24) | ((uint32_t)k[3] << 16) | ((uint32_t)k[4] << 8) |
                   (uint32_t)k[5];
    mix ^= (uint32_t)k[6] << 16;
    mix ^= (uint32_t)cnt << 8;
    mix ^= tail;

    return (uint8_t)((mix >> 0) ^ (mix >> 8) ^ (mix >> 16) ^ (mix >> 24) ^ (uint8_t)(cnt >> 8) ^
                     (uint8_t)(tail >> 16));
}

RfStreamProtocolStatus rf_stream_protocol_decoder_ford_v2_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderFordV2* instance = context;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);

    if(ret == RfStreamProtocolStatusOk) {
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_uint32(
            flipper_format, "Serial", &instance->generic.serial, 1);

        uint32_t btn = instance->generic.btn;
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_uint32(flipper_format, "Btn", &btn, 1);

        uint32_t cnt = instance->generic.cnt;
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_uint32(flipper_format, "Cnt", &cnt, 1);

        uint32_t tail31 = instance->tail31;
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_uint32(flipper_format, "Tail31", &tail31, 1);

        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_hex(flipper_format, "TailRaw", &instance->raw_bytes[8], 5);
    }

    return ret;
}

static void ford_v2_decoder_read_tail_raw_if_present(
    RfStreamProtocolDecoderFordV2* instance,
    RfRecord* flipper_format) {
    uint8_t tail_raw[FORD_V2_TAIL_RAW_BYTE_COUNT] = {0};
    if(rf_record_read_hex(flipper_format, "TailRaw", tail_raw, sizeof(tail_raw))) {
        instance->extra_data = 0;
        for(uint8_t i = 0; i < FORD_V2_TAIL_RAW_BYTE_COUNT; i++) {
            instance->extra_data = (instance->extra_data << 8) | (uint64_t)tail_raw[i];
        }
    }
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_ford_v2_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderFordV2* instance = context;

    RfStreamProtocolStatus ret = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, rf_stream_protocol_ford_v2_const.min_count_bit_for_found);

    if(ret != RfStreamProtocolStatusOk) {
        return ret;
    }

    if(instance->generic.data_count_bit != FORD_V2_DATA_BITS) {
        return RfStreamProtocolStatusErrorValueBitCount;
    }

    rf_record_rewind(flipper_format);
    ford_v2_decoder_read_tail_raw_if_present(instance, flipper_format);

    ford_v2_decoder_rebuild_raw_buffer(instance);
    ford_v2_decoder_extract_from_raw(instance);

    if(!instance->structure_ok) {
        return RfStreamProtocolStatusErrorParserOthers;
    }

    return ret;
}

void rf_stream_protocol_decoder_ford_v2_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderFordV2* instance = context;
    const uint8_t* k = instance->raw_bytes;

    rf_text_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n"
        "Sn:%08lX Btn:%02X [%s]\r\n"
        "Cnt:%u Struct:%s\r\n"
        "Tail31:%08lX\r\n"
        "TailRaw:%02X%02X%02X%02X%02X\r\n",
        instance->generic.protocol_name,
        (int)instance->generic.data_count_bit,
        k[2],
        k[3],
        k[4],
        k[5],
        k[6],
        k[7],
        k[8],
        k[9],
        k[10],
        k[11],
        k[12],
        (unsigned long)instance->generic.serial,
        instance->generic.btn,
        ford_v2_button_name(instance->generic.btn),
        (unsigned)instance->counter16,
        instance->structure_ok ? "OK" : "BAD",
        (unsigned long)instance->tail31,
        k[8],
        k[9],
        k[10],
        k[11],
        k[12]);
}

const RfStreamProtocolDecoder rf_stream_protocol_ford_v2_decoder = {
    .alloc = rf_stream_protocol_decoder_ford_v2_alloc,
    .free = rf_stream_protocol_decoder_ford_v2_free,
    .feed = rf_stream_protocol_decoder_ford_v2_feed,
    .reset = rf_stream_protocol_decoder_ford_v2_reset,
    .get_hash_data = rf_stream_protocol_decoder_ford_v2_get_hash_data,
    .serialize = rf_stream_protocol_decoder_ford_v2_serialize,
    .deserialize = rf_stream_protocol_decoder_ford_v2_deserialize,
    .get_string = rf_stream_protocol_decoder_ford_v2_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_ford_v2_encoder = {
    .alloc = rf_stream_protocol_encoder_ford_v2_alloc,
    .free = rf_stream_protocol_encoder_ford_v2_free,
    .deserialize = rf_stream_protocol_encoder_ford_v2_deserialize,
    .stop = rf_stream_protocol_encoder_ford_v2_stop,
    .yield = rf_stream_protocol_encoder_ford_v2_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_ford_v2_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec ford_protocol_v2 = {
    .name = FORD_PROTOCOL_V2_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_ford_v2_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_ford_v2_encoder,
#else
    .encoder = NULL,
#endif
};

#endif
