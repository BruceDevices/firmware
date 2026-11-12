// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define MAZDA_PROTOCOL_V0_NAME "Mazda V0"

extern const RfStreamCodec mazda_v0_protocol;

void* rf_stream_protocol_decoder_mazda_v0_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_mazda_v0_free(void* context);
void rf_stream_protocol_decoder_mazda_v0_reset(void* context);
void rf_stream_protocol_decoder_mazda_v0_feed(void* context, bool level, uint32_t duration);
RfStreamProtocolStatus rf_stream_protocol_decoder_mazda_v0_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_mazda_v0_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_mazda_v0_get_string(void* context, RfText* output);

void* rf_stream_protocol_encoder_mazda_v0_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_mazda_v0_deserialize(void* context, RfRecord* flipper_format);



#include <string.h>

// =============================================================================
// PROTOCOL CONSTANTS
// =============================================================================

static const RfStreamBlockConst rf_stream_protocol_mazda_v0_const = {
    .te_short = 250,
    .te_long = 500,
    .te_delta = 100,
    .min_count_bit_for_found = 64,
};

#define MAZDA_V0_UPLOAD_CAPACITY (((12U + 3U + 8U + 1U) * 16U) + 2U)
_Static_assert(
    MAZDA_V0_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "MAZDA_V0_UPLOAD_CAPACITY exceeds shared upload slab");
#define MAZDA_V0_GAP_US        0xCB20
#define MAZDA_V0_SYNC_BYTE     0xD7
#define MAZDA_V0_TAIL_BYTE     0x5A
#define MAZDA_V0_PREAMBLE_ONES 16

// =============================================================================
// STRUCT DEFINITIONS
// =============================================================================

typedef struct RfStreamProtocolDecoderMazdaV0 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    RfManchesterState manchester_state;
    uint16_t preamble_count;
    uint8_t preamble_pattern;

    uint32_t serial;
    uint8_t button;
    uint32_t count;
} RfStreamProtocolDecoderMazdaV0;

#if RF_STREAM_WITH_ENCODER
typedef struct RfStreamProtocolEncoderMazdaV0 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;

    uint32_t serial;
    uint8_t button;
    uint32_t count;
} RfStreamProtocolEncoderMazdaV0;
#endif

typedef enum {
    MazdaV0DecoderStepReset = 0,
    MazdaV0DecoderStepPreamble = 5,
    MazdaV0DecoderStepData = 6,
} MazdaV0DecoderStep;

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

static bool mazda_v0_get_event(uint32_t duration, bool level, RfManchesterEvent* event);
static void mazda_v0_decode_key(RfStreamBlockGeneric* generic);
#if RF_STREAM_WITH_ENCODER
static uint64_t mazda_v0_encode_key(uint32_t serial, uint8_t button, uint32_t counter);
static bool mazda_v0_encoder_add_level(
    RfStreamProtocolEncoderMazdaV0* instance,
    size_t* index,
    bool level,
    uint32_t duration);
static bool
    mazda_v0_append_byte(RfStreamProtocolEncoderMazdaV0* instance, size_t* index, uint8_t value);
static bool mazda_v0_build_upload(RfStreamProtocolEncoderMazdaV0* instance);
#endif
static RfStreamProtocolStatus mazda_v0_write_display(
    RfRecord* flipper_format,
    const char* protocol_name,
    uint8_t button);

// =============================================================================
// PROTOCOL INTERFACE DEFINITIONS
// =============================================================================

const RfStreamProtocolDecoder rf_stream_protocol_mazda_v0_decoder = {
    .alloc = rf_stream_protocol_decoder_mazda_v0_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_mazda_v0_feed,
    .reset = rf_stream_protocol_decoder_mazda_v0_reset,
    .get_hash_data = rf_stream_decoder_hash_blocks,
    .serialize = rf_stream_protocol_decoder_mazda_v0_serialize,
    .deserialize = rf_stream_protocol_decoder_mazda_v0_deserialize,
    .get_string = rf_stream_protocol_decoder_mazda_v0_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_mazda_v0_encoder = {
    .alloc = rf_stream_protocol_encoder_mazda_v0_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = rf_stream_protocol_encoder_mazda_v0_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_mazda_v0_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec mazda_v0_protocol = {
    .name = MAZDA_PROTOCOL_V0_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_mazda_v0_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_mazda_v0_encoder,
#else
    .encoder = NULL,
#endif
};

// =============================================================================
// HELPERS
// =============================================================================

static uint8_t mazda_v0_calculate_checksum(uint32_t serial, uint8_t button, uint32_t counter) {
    counter &= 0xFFFFFU;
    return (uint8_t)(((serial >> 24) & 0xFF) + ((serial >> 16) & 0xFF) + ((serial >> 8) & 0xFF) +
                     (serial & 0xFF) + ((counter >> 8) & 0xFF) + (counter & 0xFF) +
                     ((((counter >> 16) & 0x0F) | ((button & 0x0F) << 4)) & 0xFF));
}

static const char* mazda_v0_get_button_name(uint8_t button) {
    switch(button) {
    case 0x01:
        return "Lock";
    case 0x02:
        return "Unlock";
    case 0x04:
        return "Trunk";
    case 0x08:
        return "Remote";
    default:
        return "??";
    }
}

static bool mazda_v0_get_event(uint32_t duration, bool level, RfManchesterEvent* event) {
    const uint32_t tol = (uint32_t)rf_stream_protocol_mazda_v0_const.te_delta + 20U;

    if((uint32_t)DURATION_DIFF(duration, rf_stream_protocol_mazda_v0_const.te_short) < tol) {
        *event = level ? RfManchesterEventShortLow : RfManchesterEventShortHigh;
        return true;
    }

    if((uint32_t)DURATION_DIFF(duration, rf_stream_protocol_mazda_v0_const.te_long) < tol) {
        *event = level ? RfManchesterEventLongLow : RfManchesterEventLongHigh;
        return true;
    }

    return false;
}

static void mazda_v0_decode_key(RfStreamBlockGeneric* generic) {
    uint8_t data[8];
    rf_stream_u64_to_bytes_be(generic->data, data);

    const bool parity = rf_stream_protocol_blocks_parity8(data[7]) != 0;
    const uint8_t limit = parity ? 6 : 5;
    const uint8_t mask = data[limit];

    for(uint8_t i = 0; i < limit; i++) {
        data[i] ^= mask;
    }

    if(!parity) {
        data[6] ^= mask;
    }

    const uint8_t counter_lo = (data[5] & 0x55) | (data[6] & 0xAA);
    const uint8_t counter_mid = (data[6] & 0x55) | (data[5] & 0xAA);

    generic->serial = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                      ((uint32_t)data[2] << 8) | (uint32_t)data[3];
    generic->btn = (data[4] >> 4) & 0x0F;
    generic->cnt = (((uint32_t)data[4] & 0x0F) << 16) | ((uint32_t)counter_mid << 8) |
                   (uint32_t)counter_lo;
    generic->data_count_bit = rf_stream_protocol_mazda_v0_const.min_count_bit_for_found;
}

#if RF_STREAM_WITH_ENCODER
static uint64_t mazda_v0_encode_key(uint32_t serial, uint8_t button, uint32_t counter) {
    uint8_t data[8];

    counter &= 0xFFFFFU;
    button &= 0x0F;

    data[0] = (serial >> 24) & 0xFF;
    data[1] = (serial >> 16) & 0xFF;
    data[2] = (serial >> 8) & 0xFF;
    data[3] = serial & 0xFF;
    data[4] = (button << 4) | ((counter >> 16) & 0x0F);
    data[5] = (counter >> 8) & 0xFF;
    data[6] = counter & 0xFF;
    data[7] = mazda_v0_calculate_checksum(serial, button, counter);

    const uint8_t stored_5 = (data[6] & 0x55) | (data[5] & 0xAA);
    const uint8_t stored_6 = (data[6] & 0xAA) | (data[5] & 0x55);
    const uint8_t xor_mask = stored_5 ^ stored_6;
    const bool replace_second = rf_stream_protocol_blocks_parity8(data[7]) == 0;
    const uint8_t forward_mask = replace_second ? stored_5 : stored_6;

    data[5] = replace_second ? stored_5 : xor_mask;
    data[6] = replace_second ? xor_mask : stored_6;

    for(size_t i = 0; i < 5; i++) {
        data[i] ^= forward_mask;
    }

    return rf_stream_bytes_to_u64_be(data);
}

static bool mazda_v0_encoder_add_level(
    RfStreamProtocolEncoderMazdaV0* instance,
    size_t* index,
    bool level,
    uint32_t duration) {
    size_t before = *index;
    *index = rf_stream_emit(instance->encoder.upload, before, MAZDA_V0_UPLOAD_CAPACITY, level, duration);
    return *index > before;
}

static bool
    mazda_v0_append_byte(RfStreamProtocolEncoderMazdaV0* instance, size_t* index, uint8_t value) {
    if(*index + 16 > MAZDA_V0_UPLOAD_CAPACITY) {
        return false;
    }
    *index = rf_stream_emit_byte_manchester(
        instance->encoder.upload,
        *index,
        MAZDA_V0_UPLOAD_CAPACITY,
        value,
        rf_stream_protocol_mazda_v0_const.te_short);
    return true;
}

static bool mazda_v0_build_upload(RfStreamProtocolEncoderMazdaV0* instance) {
    size_t index = 0;
    const uint64_t key64 = instance->generic.data;

    for(size_t r = 0; r < 12; r++) {
        if(!mazda_v0_append_byte(instance, &index, 0xFF)) {
            return false;
        }
    }

    if(!mazda_v0_encoder_add_level(instance, &index, false, MAZDA_V0_GAP_US)) {
        return false;
    }

    if(!mazda_v0_append_byte(instance, &index, 0xFF) ||
       !mazda_v0_append_byte(instance, &index, 0xFF) ||
       !mazda_v0_append_byte(instance, &index, MAZDA_V0_SYNC_BYTE)) {
        return false;
    }

    for(int bi = 0; bi < 8; bi++) {
        const uint8_t raw = (uint8_t)((key64 >> (56 - bi * 8)) & 0xFF);
        const uint8_t air = (uint8_t)~raw;
        if(!mazda_v0_append_byte(instance, &index, air)) {
            return false;
        }
    }

    if(!mazda_v0_append_byte(instance, &index, MAZDA_V0_TAIL_BYTE)) {
        return false;
    }

    if(!mazda_v0_encoder_add_level(instance, &index, false, MAZDA_V0_GAP_US)) {
        return false;
    }

    instance->encoder.front = 0;
    instance->encoder.size_upload = index;

    return true;
}
#endif

static RfStreamProtocolStatus mazda_v0_write_display(
    RfRecord* flipper_format,
    const char* protocol_name,
    uint8_t button) {
    return rf_stream_write_display(flipper_format, protocol_name, mazda_v0_get_button_name(button));
}

// =============================================================================
// ENCODER
// =============================================================================

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_mazda_v0_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);

    RfStreamProtocolEncoderMazdaV0* instance = calloc(1, sizeof(RfStreamProtocolEncoderMazdaV0));

    instance->base.protocol = &mazda_v0_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->encoder.repeat = 10;
    instance->encoder.front = 0;
    instance->encoder.is_running = false;
    rf_stream_encoder_buffer_ensure(instance, MAZDA_V0_UPLOAD_CAPACITY);

    return instance;
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_mazda_v0_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderMazdaV0* instance = context;
    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    instance->encoder.is_running = false;
    instance->encoder.front = 0;

    do {
        if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
           RfStreamProtocolStatusOk) {
            break;
        }

        rf_record_rewind(flipper_format);
        RfStreamProtocolStatus load_st = rf_stream_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            rf_stream_protocol_mazda_v0_const.min_count_bit_for_found);
        if(load_st != RfStreamProtocolStatusOk) {
            break;
        }

        mazda_v0_decode_key(&instance->generic);

        uint32_t serial = instance->generic.serial;
        uint32_t btn = instance->generic.btn;
        uint32_t cnt = instance->generic.cnt;
        rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);
        instance->generic.serial = serial;
        instance->generic.btn = (uint8_t)btn & 0x0FU;
        instance->generic.cnt = cnt & 0xFFFFFU;

        instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, 10);

        instance->generic.data = mazda_v0_encode_key(
            instance->generic.serial, instance->generic.btn, instance->generic.cnt);
        instance->generic.data_count_bit = rf_stream_protocol_mazda_v0_const.min_count_bit_for_found;

        instance->serial = instance->generic.serial;
        instance->button = instance->generic.btn;
        instance->count = instance->generic.cnt;

        if(!mazda_v0_build_upload(instance)) {
            break;
        }
        if(instance->encoder.size_upload == 0) {
            break;
        }

        rf_record_rewind(flipper_format);
        uint8_t key_data[sizeof(uint64_t)];
        rf_stream_u64_to_bytes_be(instance->generic.data, key_data);
        if(!rf_record_update_hex(flipper_format, FF_KEY, key_data, sizeof(key_data))) {
            break;
        }

        uint32_t chk =
            mazda_v0_calculate_checksum(instance->serial, instance->button, instance->count);
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_uint32(flipper_format, "Checksum", &chk, 1);

        instance->encoder.is_running = true;

        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

#endif

// =============================================================================
// DECODER
// =============================================================================

void* rf_stream_protocol_decoder_mazda_v0_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);

    RfStreamProtocolDecoderMazdaV0* instance = calloc(1, sizeof(RfStreamProtocolDecoderMazdaV0));

    instance->base.protocol = &mazda_v0_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;

    return instance;
}

void rf_stream_protocol_decoder_mazda_v0_reset(void* context) {
    RfStreamProtocolDecoderMazdaV0* instance = context;

    instance->decoder.parser_step = MazdaV0DecoderStepReset;
    instance->decoder.te_last = 0;
    instance->decoder.decode_data = 0;
    instance->decoder.decode_count_bit = 0;
    instance->manchester_state = RfManchesterStateStart1;
    instance->preamble_count = 0;
    instance->preamble_pattern = 0;
}

void rf_stream_protocol_decoder_mazda_v0_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderMazdaV0* instance = context;
    RfManchesterEvent event = RfManchesterEventReset;
    bool data = false;

    switch(instance->decoder.parser_step) {
    case MazdaV0DecoderStepReset:
        if(level && ((uint32_t)DURATION_DIFF(duration, rf_stream_protocol_mazda_v0_const.te_short) <
                     (uint32_t)rf_stream_protocol_mazda_v0_const.te_delta + 20U)) {
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
            instance->decoder.parser_step = MazdaV0DecoderStepPreamble;
            instance->manchester_state = RfManchesterStateMid1;
            instance->preamble_count = 0;
            instance->preamble_pattern = 0;
        }
        break;

    case MazdaV0DecoderStepPreamble:
        if(!mazda_v0_get_event(duration, level, &event)) {
            instance->decoder.parser_step = MazdaV0DecoderStepReset;
            break;
        }

        if(rf_manchester_advance(
               instance->manchester_state, event, &instance->manchester_state, &data)) {
            instance->preamble_pattern = (instance->preamble_pattern << 1) | (data ? 1 : 0);

            if(data) {
                instance->preamble_count++;
            } else if(instance->preamble_count <= MAZDA_V0_PREAMBLE_ONES - 1U) {
                instance->preamble_count = 0;
                instance->preamble_pattern = 0;
                break;
            }

            if((instance->preamble_pattern == MAZDA_V0_SYNC_BYTE) &&
               (instance->preamble_count > MAZDA_V0_PREAMBLE_ONES - 1U)) {
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->decoder.parser_step = MazdaV0DecoderStepData;
            }
        }
        break;

    case MazdaV0DecoderStepData:
        if(!mazda_v0_get_event(duration, level, &event)) {
            instance->decoder.parser_step = MazdaV0DecoderStepReset;
            break;
        }

        if(rf_manchester_advance(
               instance->manchester_state, event, &instance->manchester_state, &data)) {
            rf_stream_protocol_blocks_add_bit(&instance->decoder, data);

            if(instance->decoder.decode_count_bit ==
               rf_stream_protocol_mazda_v0_const.min_count_bit_for_found) {
                instance->generic.data = ~instance->decoder.decode_data;
                mazda_v0_decode_key(&instance->generic);

                if(mazda_v0_calculate_checksum(
                       instance->generic.serial, instance->generic.btn, instance->generic.cnt) ==
                   (uint8_t)instance->generic.data) {
                    instance->serial = instance->generic.serial;
                    instance->button = instance->generic.btn;
                    instance->count = instance->generic.cnt;

                    if(instance->base.callback) {
                        instance->base.callback(&instance->base, instance->base.context);
                    }
                }

                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->preamble_count = 0;
                instance->preamble_pattern = 0;
                instance->manchester_state = RfManchesterStateStart1;
                instance->decoder.te_last = 0;
                instance->decoder.parser_step = MazdaV0DecoderStepReset;
            }
        }
        break;
    }
}

RfStreamProtocolStatus rf_stream_protocol_decoder_mazda_v0_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderMazdaV0* instance = context;

    mazda_v0_decode_key(&instance->generic);
    instance->serial = instance->generic.serial;
    instance->button = instance->generic.btn;
    instance->count = instance->generic.cnt;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);

    if(ret == RfStreamProtocolStatusOk) {
        uint32_t chk =
            mazda_v0_calculate_checksum(instance->serial, instance->button, instance->count);
        rf_record_write_uint32(flipper_format, "Checksum", &chk, 1);

        rf_stream_serialize_fields(
            flipper_format,
            RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT,
            instance->serial,
            instance->button,
            instance->count,
            0);

        ret = mazda_v0_write_display(
            flipper_format, instance->generic.protocol_name, instance->button);
    }

    return ret;
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_mazda_v0_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderMazdaV0* instance = context;

    RfStreamProtocolStatus ret = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic,
        flipper_format,
        rf_stream_protocol_mazda_v0_const.min_count_bit_for_found);

    if(ret == RfStreamProtocolStatusOk) {
        uint32_t serial = instance->serial;
        uint32_t btn = instance->button;
        uint32_t cnt = instance->count;
        rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);
        instance->serial = serial;
        instance->button = (uint8_t)btn;
        instance->count = cnt;
        instance->generic.serial = instance->serial;
        instance->generic.btn = instance->button;
        instance->generic.cnt = instance->count;
    }

    return ret;
}

void rf_stream_protocol_decoder_mazda_v0_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderMazdaV0* instance = context;

    mazda_v0_decode_key(&instance->generic);

    const uint8_t raw_crc = instance->generic.data & 0xFF;
    const uint8_t calc_crc = mazda_v0_calculate_checksum(
        instance->generic.serial, instance->generic.btn, instance->generic.cnt);

    rf_text_cat_printf(
        output,
        "%s %dbit CRC:%s\r\n"
        "Key: %016llX\r\n"
        "Sn: %08lX  Btn: %02X - %s\r\n"
        "Cnt: %05lX  Chk: %02X\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (raw_crc == calc_crc) ? "OK" : "BAD",
        (unsigned long long)instance->generic.data,
        (unsigned long)instance->generic.serial,
        instance->generic.btn,
        mazda_v0_get_button_name(instance->generic.btn),
        (unsigned long)(instance->generic.cnt & 0xFFFFFU),
        raw_crc);
}

#endif
