// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define HONDA_V1_PROTOCOL_NAME "Honda V1"

typedef struct RfStreamProtocolDecoderHondaV1 RfStreamProtocolDecoderHondaV1;
typedef struct RfStreamProtocolEncoderHondaV1 RfStreamProtocolEncoderHondaV1;

extern const RfStreamCodec honda_v1_protocol;

void* rf_stream_protocol_decoder_honda_v1_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_honda_v1_reset(void* context);
void rf_stream_protocol_decoder_honda_v1_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_honda_v1_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_honda_v1_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_honda_v1_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_honda_v1_get_string(void* context, RfText* output);

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_honda_v1_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_honda_v1_deserialize(void* context, RfRecord* flipper_format);
#endif



#include <string.h>

#define HONDA_V1_BIT_COUNT             68
#define HONDA_V1_TE_SHORT              1000
#define HONDA_V1_TE_LONG               2000
#define HONDA_V1_TE_DELTA              400
#define HONDA_V1_TE_SHORT_MIN          600
#define HONDA_V1_TE_END                3500
#define HONDA_V1_VALID_MAX             0x4B
#define HONDA_V1_NIBBLE_MASK           0x0FU
#define HONDA_V1_SERIAL_MASK           0x0FFFFFFFU
#define HONDA_V1_COUNTER_MASK          0xFFFFU
#define HONDA_V1_LOW32_MASK            0xFFFFFFFFULL
#define HONDA_V1_BUTTON_MAX            10U
#define HONDA_V1_BUTTON_VALID_MASK     0x701U
#define HONDA_V1_BUTTON_FALLBACK_CODE  0x00088888U
#define HONDA_V1_UPLOAD_CAPACITY       2048U
#define HONDA_V1_PREAMBLE_UPLOAD_COUNT 180U
#define HONDA_V1_FRAME_SYMBOLS         80U
#define HONDA_V1_FRAME_START           12U
#define HONDA_V1_FRAME_SYNC_DROP       2U
#define HONDA_V1_FRAME_REPEAT_PER_CRC  2U
#define HONDA_V1_FRAME_BYTES           9U
#define HONDA_V1_FRAME_CRC_INDEX       8U
#define HONDA_V1_FRAME_GAP_US          5000U
#define HONDA_V1_FRAME_GENERATED_MAX   (HONDA_V1_FRAME_SYMBOLS * 2U)
#define HONDA_V1_FRAME_TAIL_MAX        3U
#define HONDA_V1_DECODE_BUFFER_BYTES   12U
#define HONDA_V1_KEY_BYTES             8U
#define HONDA_V1_CRC_FIELD             "Crc"
#define HONDA_V1_KEY_2_FIELD           "Key_2"

_Static_assert(
    HONDA_V1_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "HONDA_V1_UPLOAD_CAPACITY exceeds shared upload slab");
_Static_assert(
    HONDA_V1_PREAMBLE_UPLOAD_COUNT < HONDA_V1_UPLOAD_CAPACITY,
    "HONDA_V1 preamble exceeds upload slab");
_Static_assert(
    HONDA_V1_FRAME_SYNC_DROP < HONDA_V1_FRAME_GENERATED_MAX,
    "HONDA_V1 frame sync drop exceeds generated frame");

typedef enum {
    HondaV1DecoderStepReset = 0,
    HondaV1DecoderStepPreamble,
    HondaV1DecoderStepData,
} HondaV1DecoderStep;

typedef enum {
    HondaV1ButtonUnlock = 0,
    HondaV1ButtonLock = 8,
    HondaV1ButtonTrunk = 9,
    HondaV1ButtonPanic = 10,
} HondaV1Button;

static const char* const honda_v1_button_names[HONDA_V1_BUTTON_MAX + 1U] = {
    [HondaV1ButtonUnlock] = "Unlock",
    [HondaV1ButtonLock] = "Lock",
    [HondaV1ButtonTrunk] = "Trunk",
    [HondaV1ButtonPanic] = "Panic",
};

#if RF_STREAM_WITH_ENCODER
static const uint32_t honda_v1_button_codes[HONDA_V1_BUTTON_MAX + 1U] = {
    [HondaV1ButtonUnlock] = 0x00080808,
    [HondaV1ButtonLock] = 0x00088888,
    [HondaV1ButtonTrunk] = 0x00099190,
    [HondaV1ButtonPanic] = 0x000FA7A0,
};
#endif

struct RfStreamProtocolDecoderHondaV1 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockGeneric generic;

    uint8_t step;
    uint8_t preamble_count;
    bool preamble_has_long;
    bool data_pending;
    bool last_level;
    uint8_t bits[HONDA_V1_DECODE_BUFFER_BYTES];
    uint8_t bit_count;
    uint32_t pending;
    bool pending_valid;
    uint8_t k2;
};

#if RF_STREAM_WITH_ENCODER
struct RfStreamProtocolEncoderHondaV1 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;

    uint8_t k2;
};
#endif

static bool honda_v1_button_valid(uint8_t b) {
    if(b > HONDA_V1_BUTTON_MAX) return false;
    return ((HONDA_V1_BUTTON_VALID_MASK >> b) & 1U) != 0U;
}

static const char* honda_v1_button_name(uint8_t b) {
    if((b < COUNT_OF(honda_v1_button_names)) && (honda_v1_button_names[b] != NULL)) {
        return honda_v1_button_names[b];
    }
    return "Unknown";
}

#if RF_STREAM_WITH_ENCODER
static uint32_t honda_v1_button_code(uint8_t button) {
    if(!honda_v1_button_valid(button)) {
        return HONDA_V1_BUTTON_FALLBACK_CODE;
    }
    return honda_v1_button_codes[button];
}
#endif

static bool honda_v1_duration_is(uint32_t d, uint32_t t) {
    return (d >= t) ? ((d - t) <= HONDA_V1_TE_DELTA) : ((t - d) <= HONDA_V1_TE_DELTA);
}

static uint8_t honda_v1_crc_fold(uint16_t v) {
    const uint8_t lo = (uint8_t)(v & HONDA_V1_NIBBLE_MASK);
    const uint16_t hi = (uint16_t)(v >> 4U);
    int32_t s = (hi & 1U) ? (int32_t)lo : -(int32_t)lo;
    uint8_t out = (uint8_t)((s - (int32_t)hi) & 7);
    out |= (uint8_t)(((v >> 3U) & 1U) << 3U);
    if(((v >> 1U) & 1U) && (((v >> 4U) ^ (v >> 5U)) & 1U)) {
        out ^= 0x04U;
    }
    return (uint8_t)(out & HONDA_V1_NIBBLE_MASK);
}

static uint8_t honda_v1_checksum_base(uint64_t data) {
    const uint8_t a = honda_v1_crc_fold((uint16_t)(data & HONDA_V1_COUNTER_MASK));
    const uint8_t b = honda_v1_crc_fold((uint8_t)((data >> 40U) & 0xFFU));
    return (uint8_t)((a ^ b ^ 1U) & HONDA_V1_NIBBLE_MASK);
}

static uint8_t honda_v1_checksum_alternate(uint8_t checksum) {
    uint8_t mask = 0x09U;
    if((checksum & 1U) == 0U) {
        mask = (checksum & 2U) ? 0x0BU : HONDA_V1_NIBBLE_MASK;
    }
    return (uint8_t)((checksum ^ mask) & HONDA_V1_NIBBLE_MASK);
}

static void honda_v1_checksum_wire_order(uint64_t data, uint8_t* first, uint8_t* second) {
    const uint8_t checksum = honda_v1_checksum_base(data);
    const uint8_t other = honda_v1_checksum_alternate(checksum);
    if((checksum & 0x08U) != 0U) {
        *first = other;
        *second = checksum;
    } else {
        *first = checksum;
        *second = other;
    }
}

static bool honda_v1_crc_valid(uint64_t data, uint8_t crc) {
    uint8_t first = 0U;
    uint8_t second = 0U;
    honda_v1_checksum_wire_order(data, &first, &second);
    crc &= HONDA_V1_NIBBLE_MASK;
    return (crc == first) || (crc == second);
}

static void honda_v1_decode_fields(RfStreamBlockGeneric* generic) {
    const uint32_t low = (uint32_t)(generic->data & HONDA_V1_LOW32_MASK);

    generic->serial = (uint32_t)((generic->data >> 36U) & HONDA_V1_SERIAL_MASK);
    generic->btn = (uint8_t)((low >> 28U) & HONDA_V1_NIBBLE_MASK);
    generic->cnt = low & HONDA_V1_COUNTER_MASK;
    generic->data_count_bit = HONDA_V1_BIT_COUNT;
}

#if RF_STREAM_WITH_ENCODER
static uint64_t honda_v1_build_key(uint32_t serial, uint8_t button, uint16_t counter) {
    const uint32_t table = honda_v1_button_code(button);
    const uint32_t low = ((table & HONDA_V1_COUNTER_MASK) << 16U) | counter;
    const uint32_t high = ((serial & HONDA_V1_SERIAL_MASK) << 4U) | (table >> 16U);

    return ((uint64_t)high << 32U) | low;
}
#endif

static void honda_v1_state_reset(RfStreamProtocolDecoderHondaV1* instance) {
    instance->step = HondaV1DecoderStepReset;
    instance->preamble_count = 0U;
    instance->preamble_has_long = false;
    instance->data_pending = false;
    instance->last_level = false;
    instance->bit_count = 0U;
    memset(instance->bits, 0, sizeof(instance->bits));
}

static void honda_v1_add_bit(RfStreamProtocolDecoderHondaV1* instance, bool bit) {
    if(instance->bit_count > HONDA_V1_VALID_MAX) return;
    if(bit) {
        instance->bits[instance->bit_count >> 3U] |=
            (uint8_t)(1U << (((uint8_t)~instance->bit_count) & 0x07U));
    }
    instance->bit_count++;
}

static bool honda_v1_commit(RfStreamProtocolDecoderHondaV1* instance) {
    if(instance->bit_count < HONDA_V1_BIT_COUNT) return false;

    uint8_t aligned[sizeof(instance->bits)];
    memcpy(aligned, instance->bits, sizeof(aligned));

    uint8_t shift_count = instance->bit_count - HONDA_V1_BIT_COUNT;
    if(shift_count < 1U) shift_count = 1U;

    for(uint8_t shift = 0U; shift < shift_count; shift++) {
        for(size_t i = 0; i < sizeof(aligned) - 1U; i++) {
            aligned[i] = (uint8_t)((aligned[i] << 1U) | (aligned[i + 1U] >> 7U));
        }
        aligned[sizeof(aligned) - 1U] <<= 1U;
    }

    const uint8_t button = (uint8_t)(aligned[4] >> 4U);
    if(!honda_v1_button_valid(button)) return false;

    instance->generic.data = rf_stream_bytes_to_u64_be(aligned);
    instance->k2 = (uint8_t)(aligned[8] >> 4U);
    honda_v1_decode_fields(&instance->generic);

    if(instance->base.callback) {
        instance->base.callback(&instance->base, instance->base.context);
    }

    return true;
}

static void
    honda_v1_symbol(RfStreamProtocolDecoderHondaV1* instance, bool level, uint32_t duration) {
    const bool sh = honda_v1_duration_is(duration, HONDA_V1_TE_SHORT);
    const bool lg = honda_v1_duration_is(duration, HONDA_V1_TE_LONG);

    if(!sh && !lg) {
        if(!level && (duration > HONDA_V1_TE_END) && (instance->step == HondaV1DecoderStepData)) {
            honda_v1_commit(instance);
        }
        honda_v1_state_reset(instance);
        return;
    }

    if(instance->step == HondaV1DecoderStepReset) {
        if(level) {
            instance->step = HondaV1DecoderStepPreamble;
            instance->preamble_count = 1U;
            instance->last_level = level;
        }
        return;
    }

    if(instance->step == HondaV1DecoderStepPreamble) {
        if(lg) {
            if(instance->preamble_count < 0xFFU) instance->preamble_count++;
            instance->preamble_has_long = true;
            instance->last_level = level;
            return;
        }

        if(sh) {
            if(instance->preamble_has_long && (instance->preamble_count > 5U)) {
                instance->step = HondaV1DecoderStepData;
                instance->bit_count = 0U;
                memset(instance->bits, 0, sizeof(instance->bits));
                instance->data_pending = true;
                instance->last_level = level;
                return;
            }

            if(instance->preamble_count < 0xFFU) instance->preamble_count++;
            instance->last_level = level;
            return;
        }

        honda_v1_state_reset(instance);
        return;
    }

    if(sh) {
        if(instance->data_pending) {
            honda_v1_add_bit(instance, level);
            instance->data_pending = false;
            instance->last_level = level;
            return;
        }

        instance->data_pending = true;
        instance->last_level = level;
    } else {
        if(instance->data_pending) {
            honda_v1_add_bit(instance, level);
        } else {
            honda_v1_add_bit(instance, instance->last_level);
        }

        instance->last_level = level;
    }
}

#if RF_STREAM_WITH_ENCODER
static bool honda_v1_append_frame(
    RfStreamProtocolEncoderHondaV1* instance,
    size_t* index,
    const uint8_t frame[HONDA_V1_FRAME_BYTES]) {
    LevelDuration* upload = instance->encoder.upload;
    LevelDuration generated[HONDA_V1_FRAME_GENERATED_MAX];
    size_t generated_count = 0U;

    for(uint32_t bit_index = 0U; bit_index < HONDA_V1_FRAME_SYMBOLS; bit_index++) {
        uint32_t bit;

        if(bit_index >= HONDA_V1_FRAME_START) {
            const uint32_t data_index = (bit_index - HONDA_V1_FRAME_START) >> 3U;
            const uint8_t shift = (uint8_t)((11U - bit_index) & 0x07U);
            bit = (frame[data_index] >> shift) & 0x01U;
        } else {
            bit = ((uint32_t)~bit_index) & 0x01U;
        }

        generated_count = rf_stream_emit_merge(
            generated, generated_count, COUNT_OF(generated), bit != 0U, HONDA_V1_TE_SHORT);
        generated_count = rf_stream_emit_merge(
            generated, generated_count, COUNT_OF(generated), bit == 0U, HONDA_V1_TE_SHORT);
    }

    if(generated_count <= HONDA_V1_FRAME_SYNC_DROP) {
        return false;
    }

    const size_t copy_count = generated_count - HONDA_V1_FRAME_SYNC_DROP;
    if((*index + copy_count + HONDA_V1_FRAME_TAIL_MAX) > HONDA_V1_UPLOAD_CAPACITY) {
        return false;
    }

    memcpy(
        &upload[*index], &generated[HONDA_V1_FRAME_SYNC_DROP], copy_count * sizeof(LevelDuration));
    *index += copy_count;

    const bool tail_level = !level_duration_get_level(upload[*index - 1U]);
    *index = rf_stream_emit(upload, *index, HONDA_V1_UPLOAD_CAPACITY, tail_level, HONDA_V1_TE_SHORT);
    if(!tail_level) {
        *index = rf_stream_emit(upload, *index, HONDA_V1_UPLOAD_CAPACITY, true, HONDA_V1_TE_SHORT);
    }
    *index = rf_stream_emit(upload, *index, HONDA_V1_UPLOAD_CAPACITY, false, HONDA_V1_FRAME_GAP_US);
    return true;
}

static bool honda_v1_build_upload(RfStreamProtocolEncoderHondaV1* instance) {
    LevelDuration* upload = instance->encoder.upload;
    if(upload == NULL) return false;

    uint8_t frame[HONDA_V1_FRAME_BYTES] = {0};
    uint8_t first = 0U;
    uint8_t second = 0U;
    size_t index = 0U;

    index = rf_stream_emit_short_pairs(
        upload,
        index,
        HONDA_V1_UPLOAD_CAPACITY,
        HONDA_V1_TE_SHORT,
        HONDA_V1_PREAMBLE_UPLOAD_COUNT / 2U);
    if(index != HONDA_V1_PREAMBLE_UPLOAD_COUNT) {
        return false;
    }
    upload[index - 1U] = level_duration_make(false, HONDA_V1_FRAME_GAP_US);

    rf_stream_u64_to_bytes_be(instance->generic.data, frame);
    honda_v1_checksum_wire_order(instance->generic.data, &first, &second);

    const uint8_t crc_order[] = {first, second};
    for(size_t crc_index = 0U; crc_index < COUNT_OF(crc_order); crc_index++) {
        frame[HONDA_V1_FRAME_CRC_INDEX] = (uint8_t)(crc_order[crc_index] << 4U);
        for(size_t repeat = 0U; repeat < HONDA_V1_FRAME_REPEAT_PER_CRC; repeat++) {
            if(!honda_v1_append_frame(instance, &index, frame)) {
                return false;
            }
        }
    }

    instance->k2 = second;
    instance->encoder.front = 0U;
    instance->encoder.size_upload = index;
    return true;
}
#endif

const RfStreamProtocolDecoder rf_stream_protocol_honda_v1_decoder = {
    .alloc = rf_stream_protocol_decoder_honda_v1_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_honda_v1_feed,
    .reset = rf_stream_protocol_decoder_honda_v1_reset,
    .get_hash_data = rf_stream_protocol_decoder_honda_v1_get_hash_data,
    .serialize = rf_stream_protocol_decoder_honda_v1_serialize,
    .deserialize = rf_stream_protocol_decoder_honda_v1_deserialize,
    .get_string = rf_stream_protocol_decoder_honda_v1_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_honda_v1_encoder = {
    .alloc = rf_stream_protocol_encoder_honda_v1_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = rf_stream_protocol_encoder_honda_v1_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_honda_v1_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec honda_v1_protocol = {
    .name = HONDA_V1_PROTOCOL_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_honda_v1_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_honda_v1_encoder,
#else
    .encoder = NULL,
#endif
};

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_honda_v1_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);

    RfStreamProtocolEncoderHondaV1* instance = malloc(sizeof(RfStreamProtocolEncoderHondaV1));
    memset(instance, 0, sizeof(*instance));

    instance->base.protocol = &honda_v1_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->encoder.repeat = 1U;
    rf_stream_encoder_buffer_ensure(instance, HONDA_V1_UPLOAD_CAPACITY);

    return instance;
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_honda_v1_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderHondaV1* instance = context;
    instance->encoder.is_running = false;
    instance->encoder.front = 0U;

    if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
       RfStreamProtocolStatusOk) {
        return RfStreamProtocolStatusError;
    }

    RfStreamProtocolStatus status = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, HONDA_V1_BIT_COUNT);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }

    instance->generic.protocol_name = instance->base.protocol->name;
    honda_v1_decode_fields(&instance->generic);

    uint32_t serial = instance->generic.serial;
    uint32_t btn = instance->generic.btn;
    uint32_t cnt = instance->generic.cnt;
    rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);

    serial &= HONDA_V1_SERIAL_MASK;
    uint8_t button = (uint8_t)(btn & HONDA_V1_NIBBLE_MASK);
    if(!honda_v1_button_valid(button)) {
        button = (uint8_t)instance->generic.btn;
    }
    if(!honda_v1_button_valid(button)) {
        button = HondaV1ButtonUnlock;
    }

    instance->generic.serial = serial;
    instance->generic.btn = button;
    instance->generic.cnt = cnt & HONDA_V1_COUNTER_MASK;
    instance->generic.data_count_bit = HONDA_V1_BIT_COUNT;
    instance->generic.data =
        honda_v1_build_key(instance->generic.serial, instance->generic.btn, instance->generic.cnt);

    uint8_t first = 0U;
    uint8_t second = 0U;
    honda_v1_checksum_wire_order(instance->generic.data, &first, &second);
    instance->k2 = second;

    uint8_t key_data[HONDA_V1_KEY_BYTES];
    rf_stream_u64_to_bytes_be(instance->generic.data, key_data);
    rf_record_rewind(flipper_format);
    bool key_written =
        rf_record_update_hex(flipper_format, FF_KEY, key_data, sizeof(key_data));
    if(!key_written) {
        rf_record_rewind(flipper_format);
        key_written = rf_record_insert_or_update_hex(
            flipper_format, FF_KEY, key_data, sizeof(key_data));
    }
    if(!key_written) {
        return RfStreamProtocolStatusErrorParserKey;
    }

    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_SERIAL, instance->generic.serial);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_BTN, instance->generic.btn);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_CNT, instance->generic.cnt);
    rf_stream_flipper_update_or_insert_u32(flipper_format, HONDA_V1_CRC_FIELD, instance->k2);
    rf_stream_flipper_update_or_insert_u32(flipper_format, HONDA_V1_KEY_2_FIELD, instance->k2);

    instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, 1U);
    if(!honda_v1_build_upload(instance)) {
        return RfStreamProtocolStatusErrorEncoderGetUpload;
    }

    instance->encoder.is_running = true;
    return RfStreamProtocolStatusOk;
}
#endif

void* rf_stream_protocol_decoder_honda_v1_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);

    RfStreamProtocolDecoderHondaV1* instance = malloc(sizeof(RfStreamProtocolDecoderHondaV1));
    memset(instance, 0, sizeof(*instance));

    instance->base.protocol = &honda_v1_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;

    return instance;
}

void rf_stream_protocol_decoder_honda_v1_reset(void* context) {
    RfStreamProtocolDecoderHondaV1* instance = context;
    instance->pending = 0U;
    instance->pending_valid = false;
    honda_v1_state_reset(instance);
}

void rf_stream_protocol_decoder_honda_v1_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderHondaV1* instance = context;

    if(duration < HONDA_V1_TE_DELTA) {
        instance->pending += duration;
        instance->pending_valid = true;
        return;
    }

    if(instance->pending_valid) {
        const uint32_t p = instance->pending;
        if(level) {
            instance->pending = p + duration;
            instance->pending_valid = true;
            return;
        }
        if(p >= HONDA_V1_TE_SHORT_MIN) honda_v1_symbol(instance, true, p);
        instance->pending = 0U;
        instance->pending_valid = false;
    }

    if(level) {
        instance->pending = duration;
        instance->pending_valid = true;
        return;
    }

    honda_v1_symbol(instance, false, duration);
}

uint8_t rf_stream_protocol_decoder_honda_v1_get_hash_data(void* context) {
    RfStreamProtocolDecoderHondaV1* instance = context;
    const uint64_t data = instance->generic.data;

    return (uint8_t)(data ^ (data >> 8U) ^ (data >> 16U) ^ (data >> 24U) ^ (data >> 32U) ^
                     (data >> 40U) ^ (data >> 48U) ^ (data >> 56U));
}

RfStreamProtocolStatus rf_stream_protocol_decoder_honda_v1_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderHondaV1* instance = context;
    honda_v1_decode_fields(&instance->generic);

    RfStreamProtocolStatus status =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }

    status = rf_stream_serialize_fields(
        flipper_format,
        RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT,
        instance->generic.serial,
        instance->generic.btn,
        instance->generic.cnt,
        0);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }

    uint32_t crc = instance->k2 & HONDA_V1_NIBBLE_MASK;
    if(!rf_record_write_uint32(flipper_format, HONDA_V1_CRC_FIELD, &crc, 1)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }
    if(!rf_record_write_uint32(flipper_format, HONDA_V1_KEY_2_FIELD, &crc, 1)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }

    return rf_stream_write_display(
        flipper_format,
        instance->generic.protocol_name,
        honda_v1_button_name(instance->generic.btn));
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_honda_v1_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderHondaV1* instance = context;
    RfStreamProtocolStatus status = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, HONDA_V1_BIT_COUNT);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }

    rf_record_rewind(flipper_format);
    uint32_t crc = 0U;
    bool crc_found = rf_record_read_uint32(flipper_format, HONDA_V1_KEY_2_FIELD, &crc, 1);
    if(!crc_found) {
        rf_record_rewind(flipper_format);
        crc_found = rf_record_read_uint32(flipper_format, HONDA_V1_CRC_FIELD, &crc, 1);
    }
    if(crc_found) {
        instance->k2 = (uint8_t)(crc & HONDA_V1_NIBBLE_MASK);
    } else {
        uint8_t first = 0U;
        uint8_t second = 0U;
        honda_v1_checksum_wire_order(instance->generic.data, &first, &second);
        instance->k2 = first;
    }

    instance->generic.protocol_name = instance->base.protocol->name;
    honda_v1_decode_fields(&instance->generic);

    return status;
}

void rf_stream_protocol_decoder_honda_v1_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderHondaV1* instance = context;
    honda_v1_decode_fields(&instance->generic);

    const uint8_t k2 = instance->k2 & HONDA_V1_NIBBLE_MASK;
    const bool crc_ok = honda_v1_crc_valid(instance->generic.data, k2);

    rf_text_printf(
        output,
        "%s %dbit\r\n"
        "Key:%016llX\r\n"
        "Btn:%s\r\n"
        "Sn:%07lX Cnt:%04lX\r\n"
        "Crc:%X [%s]",
        instance->generic.protocol_name,
        (int)instance->generic.data_count_bit,
        (unsigned long long)instance->generic.data,
        honda_v1_button_name((uint8_t)instance->generic.btn),
        (unsigned long)instance->generic.serial,
        (unsigned long)instance->generic.cnt,
        k2,
        crc_ok ? "OK" : "ERR");
}

#endif
