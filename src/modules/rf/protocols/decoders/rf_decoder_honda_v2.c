// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define HONDA_V2_PROTOCOL_NAME "Honda V2"

extern const RfStreamCodec honda_v2_protocol;

void* rf_stream_protocol_decoder_honda_v2_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_honda_v2_free(void* context);
void rf_stream_protocol_decoder_honda_v2_reset(void* context);
void rf_stream_protocol_decoder_honda_v2_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_honda_v2_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_honda_v2_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_honda_v2_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_honda_v2_get_string(void* context, RfText* output);

void* rf_stream_protocol_encoder_honda_v2_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_encoder_honda_v2_free(void* context);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_honda_v2_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_encoder_honda_v2_stop(void* context);
LevelDuration rf_stream_protocol_encoder_honda_v2_yield(void* context);



#include <string.h>

#define TAG "HondaV2"

static const RfStreamBlockConst rf_stream_protocol_honda_v2_const = {
    .te_short = 250,
    .te_long = 500,
    .te_delta = 100,
    .min_count_bit_for_found = 81,
};

#define HONDA_V2_PREAMBLE_PAIRS     319U
#define HONDA_V2_MIN_PREAMBLE_PAIRS 64U
#define HONDA_V2_SYNC_US            750U
#define HONDA_V2_SYNC_DELTA_US      120U
#define HONDA_V2_UPLOAD_CAPACITY    1024U
#define HONDA_V2_GAP_US             50000U
_Static_assert(
    HONDA_V2_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "HONDA_V2_UPLOAD_CAPACITY exceeds shared upload slab");

#define HONDA_V2_BTN_UNKNOWN 0x00U
#define HONDA_V2_BTN_LOCK    0x02U
#define HONDA_V2_BTN_UNLOCK  0x04U

#define HONDA_V2_SIG_UNLOCK 0xA285E3UL
#define HONDA_V2_SIG_LOCK   0xC20363UL

#define HONDA_V2_FF_BTNSIG    "BtnSig"
#define HONDA_V2_FF_CHECK     "Check"
#define HONDA_V2_FF_TAIL      "Tail"
#define HONDA_V2_FF_EXTRA_BIT "ExtraBit"

typedef struct RfStreamProtocolDecoderHondaV2 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    uint16_t preamble_count;
    uint8_t raw[10];
    uint8_t bit_count;
    bool extra_bit;
    bool previous_bit;
    bool boundary_pad_skipped;
    bool pending_short;

    uint64_t key;
    uint16_t tail;
    uint32_t command_signature;
    uint32_t serial;
    uint32_t count;
    uint8_t button;
    uint8_t check;
    bool check_ok;
    bool tail_ok;
} RfStreamProtocolDecoderHondaV2;

#if RF_STREAM_WITH_ENCODER
typedef struct RfStreamProtocolEncoderHondaV2 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;

    uint64_t key;
    uint16_t tail;
    uint32_t command_signature;
    uint32_t serial;
    uint32_t count;
    uint8_t button;
    uint8_t check;
} RfStreamProtocolEncoderHondaV2;
#endif

typedef enum {
    HondaV2DecoderStepReset = 0,
    HondaV2DecoderStepPreambleLow,
    HondaV2DecoderStepPreambleHigh,
    HondaV2DecoderStepSyncLow,
    HondaV2DecoderStepData,
} HondaV2DecoderStep;

static uint8_t honda_v2_button_from_signature(uint32_t signature);
static const char* honda_v2_button_name(uint8_t button);
static uint8_t honda_v2_calculate_check(uint32_t count);
static bool honda_v2_calculate_tail_msb(uint32_t count);
static uint16_t honda_v2_calculate_tail(uint32_t count);
static void honda_v2_parse_key_fields(
    uint64_t key,
    uint32_t* signature,
    uint32_t* serial,
    uint32_t* count,
    uint8_t* button,
    uint8_t* check);
static bool honda_v2_validate_frame(
    uint64_t key,
    uint16_t tail,
    bool extra_bit,
    bool* check_ok,
    bool* tail_ok);
static bool honda_v2_add_decoded_bit(RfStreamProtocolDecoderHondaV2* instance, bool bit);
static bool honda_v2_process_transition(
    RfStreamProtocolDecoderHondaV2* instance,
    bool level,
    uint32_t duration);
static bool honda_v2_finish_frame(RfStreamProtocolDecoderHondaV2* instance);

#if RF_STREAM_WITH_ENCODER
static bool honda_v2_encoder_add_level(
    RfStreamProtocolEncoderHondaV2* instance,
    size_t* index,
    bool level,
    uint32_t duration);
static bool honda_v2_encoder_add_bit(
    RfStreamProtocolEncoderHondaV2* instance,
    size_t* index,
    bool* previous_bit,
    bool bit);
static bool honda_v2_build_upload(RfStreamProtocolEncoderHondaV2* instance);
#endif

const RfStreamProtocolDecoder rf_stream_protocol_honda_v2_decoder = {
    .alloc = rf_stream_protocol_decoder_honda_v2_alloc,
    .free = rf_stream_protocol_decoder_honda_v2_free,
    .feed = rf_stream_protocol_decoder_honda_v2_feed,
    .reset = rf_stream_protocol_decoder_honda_v2_reset,
    .get_hash_data = rf_stream_protocol_decoder_honda_v2_get_hash_data,
    .serialize = rf_stream_protocol_decoder_honda_v2_serialize,
    .deserialize = rf_stream_protocol_decoder_honda_v2_deserialize,
    .get_string = rf_stream_protocol_decoder_honda_v2_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_honda_v2_encoder = {
    .alloc = rf_stream_protocol_encoder_honda_v2_alloc,
    .free = rf_stream_protocol_encoder_honda_v2_free,
    .deserialize = rf_stream_protocol_encoder_honda_v2_deserialize,
    .stop = rf_stream_protocol_encoder_honda_v2_stop,
    .yield = rf_stream_protocol_encoder_honda_v2_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_honda_v2_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec honda_v2_protocol = {
    .name = HONDA_V2_PROTOCOL_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_honda_v2_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_honda_v2_encoder,
#else
    .encoder = NULL,
#endif
};

static bool honda_v2_is_short(uint32_t duration) {
    return rf_stream_is_short(duration, &rf_stream_protocol_honda_v2_const);
}

static bool honda_v2_is_long(uint32_t duration) {
    return rf_stream_is_long(duration, &rf_stream_protocol_honda_v2_const);
}

static bool honda_v2_is_sync(uint32_t duration) {
    return DURATION_DIFF(duration, HONDA_V2_SYNC_US) < HONDA_V2_SYNC_DELTA_US;
}

static uint8_t honda_v2_button_from_signature(uint32_t signature) {
    if(signature == HONDA_V2_SIG_UNLOCK) {
        return HONDA_V2_BTN_UNLOCK;
    } else if(signature == HONDA_V2_SIG_LOCK) {
        return HONDA_V2_BTN_LOCK;
    }
    return HONDA_V2_BTN_UNKNOWN;
}

static const char* honda_v2_button_name(uint8_t button) {
    switch(button) {
    case HONDA_V2_BTN_LOCK:
        return "Lock";
    case HONDA_V2_BTN_UNLOCK:
        return "Unlock";
    default:
        return "Unknown";
    }
}

static uint8_t honda_v2_calculate_check(uint32_t count) {
    const uint8_t c0 = ((count >> 1) ^ (count >> 2) ^ (count >> 3) ^ (count >> 4) ^ (count >> 6)) &
                       1U;
    const uint8_t c1 = ((count >> 0) ^ (count >> 2) ^ (count >> 3) ^ (count >> 4) ^ (count >> 5) ^
                        (count >> 6) ^ 1U) &
                       1U;
    const uint8_t c2 = ((count >> 1) ^ (count >> 3) ^ (count >> 4) ^ (count >> 5) ^ (count >> 6)) &
                       1U;

    return (uint8_t)(c0 | (c1 << 1) | (c2 << 2));
}

static bool honda_v2_calculate_tail_msb(uint32_t count) {
    const uint8_t tail = ((count >> 0) ^ (count >> 2) ^ (count >> 4) ^ (count >> 5)) & 1U;
    return tail != 0U;
}

static uint16_t honda_v2_calculate_tail(uint32_t count) {
    return honda_v2_calculate_tail_msb(count) ? 0xFFFFU : 0x7FFFU;
}

static void honda_v2_parse_key_fields(
    uint64_t key,
    uint32_t* signature,
    uint32_t* serial,
    uint32_t* count,
    uint8_t* button,
    uint8_t* check) {
    uint8_t key_bytes[8];
    rf_stream_u64_to_bytes_be(key, key_bytes);

    const uint32_t sig = ((uint32_t)key_bytes[0] << 16) | ((uint32_t)key_bytes[1] << 8) |
                         key_bytes[2];
    const uint32_t sn = ((uint32_t)key_bytes[3] << 16) | ((uint32_t)key_bytes[4] << 8) |
                        key_bytes[5];
    const uint32_t cnt = ((uint32_t)key_bytes[6] << 1) | ((key_bytes[7] >> 7) & 1U);

    if(signature) *signature = sig;
    if(serial) *serial = sn;
    if(count) *count = cnt;
    if(button) *button = honda_v2_button_from_signature(sig);
    if(check) *check = key_bytes[7] & 0x07U;
}

static bool honda_v2_validate_frame(
    uint64_t key,
    uint16_t tail,
    bool extra_bit,
    bool* check_ok,
    bool* tail_ok) {
    uint8_t key_bytes[8];
    rf_stream_u64_to_bytes_be(key, key_bytes);

    const uint32_t count = ((uint32_t)key_bytes[6] << 1) | ((key_bytes[7] >> 7) & 1U);
    const uint8_t expected_check = honda_v2_calculate_check(count);
    const uint16_t expected_tail = honda_v2_calculate_tail(count);

    const bool local_check_ok = ((key_bytes[7] & 0x78U) == 0U) &&
                                ((key_bytes[7] & 0x07U) == expected_check);
    const bool local_tail_ok = (tail == expected_tail) && extra_bit;

    if(check_ok) *check_ok = local_check_ok;
    if(tail_ok) *tail_ok = local_tail_ok;

    return local_check_ok && local_tail_ok;
}

static bool honda_v2_add_decoded_bit(RfStreamProtocolDecoderHondaV2* instance, bool bit) {
    if(instance->bit_count < 80U) {
        const uint8_t byte_index = instance->bit_count / 8U;
        const uint8_t bit_index = 7U - (instance->bit_count % 8U);
        if(bit) {
            instance->raw[byte_index] |= (uint8_t)(1U << bit_index);
        }
    } else if(instance->bit_count == 80U) {
        instance->extra_bit = bit;
    } else {
        return false;
    }

    instance->bit_count++;
    return true;
}

static bool honda_v2_finish_frame(RfStreamProtocolDecoderHondaV2* instance) {
    const uint64_t key = rf_stream_bytes_to_u64_be(instance->raw);
    const uint16_t tail = ((uint16_t)instance->raw[8] << 8) | instance->raw[9];

    if(!honda_v2_validate_frame(
           key, tail, instance->extra_bit, &instance->check_ok, &instance->tail_ok)) {
        return false;
    }

    instance->key = key;
    instance->tail = tail;

    honda_v2_parse_key_fields(
        key,
        &instance->command_signature,
        &instance->serial,
        &instance->count,
        &instance->button,
        &instance->check);

    instance->generic.data = instance->key;
    instance->generic.data_count_bit = rf_stream_protocol_honda_v2_const.min_count_bit_for_found;
    instance->generic.serial = instance->serial;
    instance->generic.btn = instance->button;
    instance->generic.cnt = instance->count;

    return true;
}

static bool honda_v2_process_transition(
    RfStreamProtocolDecoderHondaV2* instance,
    bool level,
    uint32_t duration) {
    if(!instance->boundary_pad_skipped) {
        if(level && honda_v2_is_short(duration)) {
            instance->boundary_pad_skipped = true;
            return true;
        }
        instance->boundary_pad_skipped = true;
    }

    if(instance->pending_short) {
        if(!instance->previous_bit && !level && honda_v2_is_short(duration)) {
            instance->pending_short = false;
            return honda_v2_add_decoded_bit(instance, false);
        } else if(instance->previous_bit && level && honda_v2_is_short(duration)) {
            instance->pending_short = false;
            return honda_v2_add_decoded_bit(instance, true);
        }
        return false;
    }

    if(!instance->previous_bit) {
        if(level && honda_v2_is_long(duration)) {
            instance->previous_bit = true;
            return honda_v2_add_decoded_bit(instance, true);
        } else if(level && honda_v2_is_short(duration)) {
            instance->pending_short = true;
            return true;
        }
        return false;
    }

    if(!level && honda_v2_is_long(duration)) {
        instance->previous_bit = false;
        return honda_v2_add_decoded_bit(instance, false);
    } else if(!level && honda_v2_is_short(duration)) {
        instance->pending_short = true;
        return true;
    }

    return false;
}

#if RF_STREAM_WITH_ENCODER
static bool honda_v2_encoder_add_level(
    RfStreamProtocolEncoderHondaV2* instance,
    size_t* index,
    bool level,
    uint32_t duration) {
    if(*index >= HONDA_V2_UPLOAD_CAPACITY) {
        return false;
    }
    instance->encoder.upload[(*index)++] = level_duration_make(level, duration);
    return true;
}

static bool honda_v2_encoder_add_bit(
    RfStreamProtocolEncoderHondaV2* instance,
    size_t* index,
    bool* previous_bit,
    bool bit) {
    const uint32_t te_short = rf_stream_protocol_honda_v2_const.te_short;
    const uint32_t te_long = rf_stream_protocol_honda_v2_const.te_long;

    if(!*previous_bit && !bit) {
        if(!honda_v2_encoder_add_level(instance, index, true, te_short) ||
           !honda_v2_encoder_add_level(instance, index, false, te_short)) {
            return false;
        }
    } else if(!*previous_bit && bit) {
        if(!honda_v2_encoder_add_level(instance, index, true, te_long)) {
            return false;
        }
    } else if(*previous_bit && !bit) {
        if(!honda_v2_encoder_add_level(instance, index, false, te_long)) {
            return false;
        }
    } else {
        if(!honda_v2_encoder_add_level(instance, index, false, te_short) ||
           !honda_v2_encoder_add_level(instance, index, true, te_short)) {
            return false;
        }
    }

    *previous_bit = bit;
    return true;
}

static bool honda_v2_build_upload(RfStreamProtocolEncoderHondaV2* instance) {
    size_t index = 0;
    const uint32_t te_short = rf_stream_protocol_honda_v2_const.te_short;

    uint8_t key_bytes[8];
    rf_stream_u64_to_bytes_be(instance->key, key_bytes);

    for(uint16_t i = 0; i < HONDA_V2_PREAMBLE_PAIRS; i++) {
        if(!honda_v2_encoder_add_level(instance, &index, true, te_short) ||
           !honda_v2_encoder_add_level(instance, &index, false, te_short)) {
            return false;
        }
    }

    if(!honda_v2_encoder_add_level(instance, &index, true, HONDA_V2_SYNC_US) ||
       !honda_v2_encoder_add_level(instance, &index, false, HONDA_V2_SYNC_US) ||
       !honda_v2_encoder_add_level(instance, &index, true, te_short)) {
        return false;
    }

    bool previous_bit = true;
    if(!honda_v2_encoder_add_bit(instance, &index, &previous_bit, false)) {
        return false;
    }

    for(uint8_t bit_index = 2; bit_index < 64; bit_index++) {
        const uint8_t byte_index = bit_index / 8U;
        const uint8_t bit_in_byte = 7U - (bit_index % 8U);
        const bool bit = (key_bytes[byte_index] >> bit_in_byte) & 1U;
        if(!honda_v2_encoder_add_bit(instance, &index, &previous_bit, bit)) {
            return false;
        }
    }

    instance->tail = honda_v2_calculate_tail(instance->count);
    for(uint8_t bit_index = 0; bit_index < 16; bit_index++) {
        const bool bit = (instance->tail >> (15U - bit_index)) & 1U;
        if(!honda_v2_encoder_add_bit(instance, &index, &previous_bit, bit)) {
            return false;
        }
    }

    if(!honda_v2_encoder_add_bit(instance, &index, &previous_bit, true)) {
        return false;
    }

    if(!honda_v2_encoder_add_level(instance, &index, false, HONDA_V2_GAP_US)) {
        return false;
    }

    instance->encoder.front = 0;
    instance->encoder.size_upload = index;
    return true;
}
#endif

void* rf_stream_protocol_decoder_honda_v2_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderHondaV2* instance = calloc(1, sizeof(RfStreamProtocolDecoderHondaV2));

    instance->base.protocol = &honda_v2_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;

    return instance;
}

void rf_stream_protocol_decoder_honda_v2_free(void* context) {
    RfStreamProtocolDecoderHondaV2* instance = context;
    free(instance);
}

void rf_stream_protocol_decoder_honda_v2_reset(void* context) {
    RfStreamProtocolDecoderHondaV2* instance = context;

    instance->decoder.parser_step = HondaV2DecoderStepReset;
    instance->decoder.te_last = 0;
    instance->preamble_count = 0;
    memset(instance->raw, 0, sizeof(instance->raw));
    instance->bit_count = 0;
    instance->extra_bit = false;
    instance->previous_bit = true;
    instance->boundary_pad_skipped = false;
    instance->pending_short = false;
}

void rf_stream_protocol_decoder_honda_v2_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderHondaV2* instance = context;

    switch(instance->decoder.parser_step) {
    case HondaV2DecoderStepReset:
        if(level && honda_v2_is_short(duration)) {
            instance->preamble_count = 0;
            instance->decoder.parser_step = HondaV2DecoderStepPreambleLow;
        }
        break;

    case HondaV2DecoderStepPreambleLow:
        if(!level && honda_v2_is_short(duration)) {
            instance->preamble_count++;
            instance->decoder.parser_step = HondaV2DecoderStepPreambleHigh;
        } else {
            instance->decoder.parser_step = HondaV2DecoderStepReset;
        }
        break;

    case HondaV2DecoderStepPreambleHigh:
        if(level && honda_v2_is_short(duration)) {
            instance->decoder.parser_step = HondaV2DecoderStepPreambleLow;
        } else if(
            level && honda_v2_is_sync(duration) &&
            instance->preamble_count >= HONDA_V2_MIN_PREAMBLE_PAIRS) {
            instance->decoder.parser_step = HondaV2DecoderStepSyncLow;
        } else {
            instance->decoder.parser_step = HondaV2DecoderStepReset;
        }
        break;

    case HondaV2DecoderStepSyncLow:
        if(!level && honda_v2_is_sync(duration)) {
            memset(instance->raw, 0, sizeof(instance->raw));
            instance->bit_count = 0;
            instance->extra_bit = false;
            instance->previous_bit = true;
            instance->boundary_pad_skipped = false;
            instance->pending_short = false;
            honda_v2_add_decoded_bit(instance, true);
            instance->decoder.parser_step = HondaV2DecoderStepData;
        } else {
            instance->decoder.parser_step = HondaV2DecoderStepReset;
        }
        break;

    case HondaV2DecoderStepData:
        if(!honda_v2_process_transition(instance, level, duration)) {
            instance->decoder.parser_step = HondaV2DecoderStepReset;
            break;
        }

        if(instance->bit_count == rf_stream_protocol_honda_v2_const.min_count_bit_for_found) {
            if(honda_v2_finish_frame(instance) && instance->base.callback) {
                instance->base.callback(&instance->base, instance->base.context);
            }
            instance->decoder.parser_step = HondaV2DecoderStepReset;
        }
        break;
    }

    instance->decoder.te_last = duration;
}

uint8_t rf_stream_protocol_decoder_honda_v2_get_hash_data(void* context) {
    RfStreamProtocolDecoderHondaV2* instance = context;

    RfStreamBlockDecoder decoder = {
        .decode_data = instance->key,
        .decode_count_bit = 64,
    };
    uint8_t hash = rf_stream_protocol_blocks_get_hash_data(&decoder, 9);
    hash ^= (uint8_t)(instance->tail >> 8);
    hash ^= (uint8_t)instance->tail;
    hash ^= instance->extra_bit ? 1U : 0U;
    return hash;
}

RfStreamProtocolStatus rf_stream_protocol_decoder_honda_v2_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderHondaV2* instance = context;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);

    if(ret == RfStreamProtocolStatusOk) {
        uint8_t key_bytes[8];
        rf_stream_u64_to_bytes_be(instance->key, key_bytes);
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_hex(flipper_format, FF_KEY, key_bytes, sizeof(key_bytes));
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_SERIAL, instance->serial);
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_BTN, instance->button);
        rf_stream_flipper_update_or_insert_u32(
            flipper_format, HONDA_V2_FF_BTNSIG, instance->command_signature);
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_CNT, instance->count);
        rf_stream_flipper_update_or_insert_u32(flipper_format, HONDA_V2_FF_CHECK, instance->check);
        rf_stream_flipper_update_or_insert_u32(flipper_format, HONDA_V2_FF_TAIL, instance->tail);
        rf_stream_flipper_update_or_insert_u32(
            flipper_format, HONDA_V2_FF_EXTRA_BIT, instance->extra_bit ? 1U : 0U);
    }

    return ret;
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_honda_v2_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderHondaV2* instance = context;

    RfStreamProtocolStatus ret = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic,
        flipper_format,
        rf_stream_protocol_honda_v2_const.min_count_bit_for_found);

    if(ret == RfStreamProtocolStatusOk) {
        uint8_t key_bytes[8] = {0};
        bool have_key = false;

        rf_record_rewind(flipper_format);
        if(rf_record_read_hex(flipper_format, FF_KEY, key_bytes, sizeof(key_bytes))) {
            instance->key = rf_stream_bytes_to_u64_be(key_bytes);
            have_key = true;
        }

        if(!have_key) {
            instance->key = instance->generic.data;
            rf_stream_u64_to_bytes_be(instance->key, key_bytes);
        }

        uint32_t temp = 0;
        rf_record_rewind(flipper_format);
        if(rf_record_read_uint32(flipper_format, HONDA_V2_FF_TAIL, &temp, 1)) {
            instance->tail = temp & 0xFFFFU;
        } else {
            const uint32_t count = ((uint32_t)key_bytes[6] << 1) | ((key_bytes[7] >> 7) & 1U);
            instance->tail = honda_v2_calculate_tail(count);
        }

        rf_record_rewind(flipper_format);
        if(rf_record_read_uint32(flipper_format, HONDA_V2_FF_EXTRA_BIT, &temp, 1)) {
            instance->extra_bit = (temp & 1U) != 0;
        } else {
            instance->extra_bit = true;
        }

        honda_v2_validate_frame(
            instance->key,
            instance->tail,
            instance->extra_bit,
            &instance->check_ok,
            &instance->tail_ok);
        honda_v2_parse_key_fields(
            instance->key,
            &instance->command_signature,
            &instance->serial,
            &instance->count,
            &instance->button,
            &instance->check);

        instance->generic.data = instance->key;
        instance->generic.data_count_bit = rf_stream_protocol_honda_v2_const.min_count_bit_for_found;
        instance->generic.serial = instance->serial;
        instance->generic.btn = instance->button;
        instance->generic.cnt = instance->count;
    }

    return ret;
}

void rf_stream_protocol_decoder_honda_v2_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderHondaV2* instance = context;

    rf_text_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%016llX\r\n"
        "Sn:%06lX  Btn:%02X - %s\r\n"
        "BtnSig:%06lX\r\n"
        "Cnt:%05lX  Chk:%02X [%s]  Tail:%05lX [%s]\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (unsigned long long)instance->key,
        (unsigned long)instance->serial,
        instance->button,
        honda_v2_button_name(instance->button),
        (unsigned long)instance->command_signature,
        (unsigned long)instance->count,
        instance->check,
        instance->check_ok ? "OK" : "BAD",
        (unsigned long)(((instance->tail >> 15) & 1U) ? 0x1FFFFUL : 0x0FFFFUL),
        instance->tail_ok ? "OK" : "BAD");
}

#if RF_STREAM_WITH_ENCODER

static uint32_t honda_v2_signature_from_button(uint8_t button) {
    switch(button) {
    case HONDA_V2_BTN_LOCK:
        return HONDA_V2_SIG_LOCK;
    case HONDA_V2_BTN_UNLOCK:
        return HONDA_V2_SIG_UNLOCK;
    default:
        return 0;
    }
}

static uint64_t honda_v2_build_key(uint32_t signature, uint32_t serial, uint32_t count) {
    uint8_t key_bytes[8] = {0};
    key_bytes[0] = (uint8_t)((signature >> 16) & 0xFFU);
    key_bytes[1] = (uint8_t)((signature >> 8) & 0xFFU);
    key_bytes[2] = (uint8_t)(signature & 0xFFU);
    key_bytes[3] = (uint8_t)((serial >> 16) & 0xFFU);
    key_bytes[4] = (uint8_t)((serial >> 8) & 0xFFU);
    key_bytes[5] = (uint8_t)(serial & 0xFFU);
    key_bytes[6] = (uint8_t)((count >> 1) & 0xFFU);

    const bool counter_lsb = (count & 1U) != 0;
    const uint8_t check = honda_v2_calculate_check(count);
    key_bytes[7] = (counter_lsb ? 0x80U : 0x00U) | check;

    return rf_stream_bytes_to_u64_be(key_bytes);
}

void* rf_stream_protocol_encoder_honda_v2_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolEncoderHondaV2* instance = calloc(1, sizeof(RfStreamProtocolEncoderHondaV2));

    instance->base.protocol = &honda_v2_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 0;
    instance->encoder.front = 0;
    instance->encoder.is_running = false;
    rf_stream_encoder_buffer_ensure(instance, HONDA_V2_UPLOAD_CAPACITY);

    return instance;
}

void rf_stream_protocol_encoder_honda_v2_free(void* context) {
    rf_stream_encoder_free(context);
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_honda_v2_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderHondaV2* instance = context;
    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    instance->encoder.is_running = false;
    instance->encoder.front = 0;
    instance->encoder.repeat = 10;

    do {
        if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
           RfStreamProtocolStatusOk) {
            break;
        }

        RfStreamProtocolStatus load_status = rf_stream_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            rf_stream_protocol_honda_v2_const.min_count_bit_for_found);
        if(load_status != RfStreamProtocolStatusOk) {
            break;
        }

        instance->serial = instance->generic.serial & 0xFFFFFFU;
        instance->button = instance->generic.btn;
        instance->count = instance->generic.cnt & 0x1FFU;

        uint8_t key_bytes[8] = {0};
        bool have_key = false;
        rf_record_rewind(flipper_format);
        if(rf_record_read_hex(flipper_format, FF_KEY, key_bytes, sizeof(key_bytes))) {
            instance->key = rf_stream_bytes_to_u64_be(key_bytes);
            have_key = true;
        }

        if(have_key) {
            honda_v2_parse_key_fields(
                instance->key,
                &instance->command_signature,
                &instance->serial,
                &instance->count,
                &instance->button,
                &instance->check);
        }

        uint32_t u32 = 0;
        bool have_button = false;

        rf_record_rewind(flipper_format);
        if(rf_record_read_uint32(flipper_format, FF_SERIAL, &u32, 1)) {
            instance->serial = u32 & 0xFFFFFFU;
        }

        rf_record_rewind(flipper_format);
        if(rf_record_read_uint32(flipper_format, FF_CNT, &u32, 1)) {
            instance->count = u32 & 0x1FFU;
        }

        rf_record_rewind(flipper_format);
        if(rf_record_read_uint32(flipper_format, FF_BTN, &u32, 1)) {
            instance->button = (uint8_t)u32;
            have_button = true;
        }

        rf_record_rewind(flipper_format);
        if(rf_record_read_uint32(flipper_format, HONDA_V2_FF_BTNSIG, &u32, 1)) {
            instance->command_signature = u32 & 0xFFFFFFU;
        }

        if(have_button) {
            const uint32_t signature = honda_v2_signature_from_button(instance->button);
            if(signature != 0U) {
                instance->command_signature = signature;
            }
        }

        if(instance->command_signature == 0U) {
            break;
        }

        instance->key =
            honda_v2_build_key(instance->command_signature, instance->serial, instance->count);

        rf_stream_u64_to_bytes_be(instance->key, key_bytes);
        instance->tail = honda_v2_calculate_tail(instance->count);

        honda_v2_parse_key_fields(
            instance->key,
            &instance->command_signature,
            &instance->serial,
            &instance->count,
            &instance->button,
            &instance->check);

        instance->generic.data = instance->key;
        instance->generic.data_count_bit = rf_stream_protocol_honda_v2_const.min_count_bit_for_found;
        instance->generic.serial = instance->serial;
        instance->generic.btn = instance->button;
        instance->generic.cnt = instance->count;

        rf_record_rewind(flipper_format);
        instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, 10);

        if(!honda_v2_build_upload(instance) || instance->encoder.size_upload == 0U) {
            break;
        }

        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_hex(flipper_format, FF_KEY, key_bytes, sizeof(key_bytes));
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_SERIAL, instance->serial);
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_BTN, instance->button);
        rf_stream_flipper_update_or_insert_u32(
            flipper_format, HONDA_V2_FF_BTNSIG, instance->command_signature);
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_CNT, instance->count);
        rf_stream_flipper_update_or_insert_u32(flipper_format, HONDA_V2_FF_CHECK, instance->check);
        rf_stream_flipper_update_or_insert_u32(flipper_format, HONDA_V2_FF_TAIL, instance->tail);
        rf_stream_flipper_update_or_insert_u32(flipper_format, HONDA_V2_FF_EXTRA_BIT, 1U);

        instance->encoder.is_running = true;
        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

void rf_stream_protocol_encoder_honda_v2_stop(void* context) {
    rf_stream_encoder_stop(context);
}

LevelDuration rf_stream_protocol_encoder_honda_v2_yield(void* context) {
    return rf_stream_encoder_yield(context);
}
#endif

#endif
