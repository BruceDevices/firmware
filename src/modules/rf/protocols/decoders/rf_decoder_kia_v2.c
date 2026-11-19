// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define KIA_PROTOCOL_V2_NAME "Kia V2"

typedef struct RfStreamProtocolDecoderKiaV2 RfStreamProtocolDecoderKiaV2;
typedef struct RfStreamProtocolEncoderKiaV2 RfStreamProtocolEncoderKiaV2;

extern const RfStreamProtocolDecoder kia_protocol_v2_decoder;
extern const RfStreamProtocolEncoder kia_protocol_v2_encoder;
extern const RfStreamCodec kia_protocol_v2;

void* kia_protocol_decoder_v2_alloc(RfStreamEnvironment* environment);
void kia_protocol_decoder_v2_free(void* context);
void kia_protocol_decoder_v2_reset(void* context);
void kia_protocol_decoder_v2_feed(void* context, bool level, uint32_t duration);
RfStreamProtocolStatus kia_protocol_decoder_v2_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    kia_protocol_decoder_v2_deserialize(void* context, RfRecord* flipper_format);
void kia_protocol_decoder_v2_get_string(void* context, RfText* output);

void* kia_protocol_encoder_v2_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    kia_protocol_encoder_v2_deserialize(void* context, RfRecord* flipper_format);





#define TAG "KiaV2"

#define KIA_V2_HEADER_PAIRS 252
#define KIA_V2_TOTAL_BURSTS 2
#define KIA_V2_UPLOAD_CAPACITY \
    (KIA_V2_TOTAL_BURSTS * ((KIA_V2_HEADER_PAIRS * 2) + 1 + (53U * 2)))
_Static_assert(
    KIA_V2_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "KIA_V2_UPLOAD_CAPACITY exceeds shared upload slab");

static const RfStreamBlockConst kia_protocol_v2_const = {
    .te_short = 500,
    .te_long = 1000,
    .te_delta = 150,
    .min_count_bit_for_found = 53,
};

struct RfStreamProtocolDecoderKiaV2 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;
    uint16_t header_count;

    RfManchesterState manchester_state;
};

struct RfStreamProtocolEncoderKiaV2 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;
};

typedef enum {
    KiaV2DecoderStepReset = 0,
    KiaV2DecoderStepCheckPreamble,
    KiaV2DecoderStepCollectRawBits,
} KiaV2DecoderStep;

const RfStreamProtocolDecoder kia_protocol_v2_decoder = {
    .alloc = kia_protocol_decoder_v2_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = kia_protocol_decoder_v2_feed,
    .reset = kia_protocol_decoder_v2_reset,
    .get_hash_data = rf_stream_decoder_hash_blocks,
    .serialize = kia_protocol_decoder_v2_serialize,
    .deserialize = kia_protocol_decoder_v2_deserialize,
    .get_string = kia_protocol_decoder_v2_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder kia_protocol_v2_encoder = {
    .alloc = kia_protocol_encoder_v2_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = kia_protocol_encoder_v2_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder kia_protocol_v2_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec kia_protocol_v2 = {
    .name = KIA_PROTOCOL_V2_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &kia_protocol_v2_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &kia_protocol_v2_encoder,
#else
    .encoder = NULL,
#endif
};

static uint8_t kia_v2_calculate_crc(uint64_t data) {
    // Remove the CRC nibble (last 4 bits) to get the actual data
    uint64_t data_without_crc = data >> 4;

    // Extract 6 bytes from the data
    uint8_t bytes[6];
    bytes[0] = (uint8_t)(data_without_crc);
    bytes[1] = (uint8_t)(data_without_crc >> 8);
    bytes[2] = (uint8_t)(data_without_crc >> 16);
    bytes[3] = (uint8_t)(data_without_crc >> 24);
    bytes[4] = (uint8_t)(data_without_crc >> 32);
    bytes[5] = (uint8_t)(data_without_crc >> 40);

    uint8_t crc = 0;
    for(int i = 0; i < 6; i++) {
        crc ^= (bytes[i] & 0x0F) ^ (bytes[i] >> 4);
    }

    return (crc + 1) & 0x0F;
}
#if RF_STREAM_WITH_ENCODER

static void kia_protocol_encoder_v2_get_upload(RfStreamProtocolEncoderKiaV2* instance) {
    if(instance->encoder.upload == NULL) return;
    size_t index = 0;
    LevelDuration* up = instance->encoder.upload;
    const size_t cap = KIA_V2_UPLOAD_CAPACITY;

    const uint32_t te_short = (uint32_t)kia_protocol_v2_const.te_short;
    const uint32_t te_long = (uint32_t)kia_protocol_v2_const.te_long;

    uint8_t crc = kia_v2_calculate_crc(instance->generic.data);
    instance->generic.data = (instance->generic.data & ~0x0FULL) | crc;

    for(uint8_t burst = 0; burst < KIA_V2_TOTAL_BURSTS; burst++) {
        for(int i = 0; i < KIA_V2_HEADER_PAIRS; i++) {
            index = rf_stream_emit(up, index, cap, false, te_long);
            index = rf_stream_emit(up, index, cap, true, te_long);
        }

        index = rf_stream_emit(up, index, cap, false, te_short);

        // Include the leading Manchester 1 before the 52 payload bits. Its
        // short HIGH is the decoder's preamble-to-data boundary.
        index = rf_stream_emit(up, index, cap, true, te_short);
        index = rf_stream_emit(up, index, cap, false, te_short);

        for(uint8_t i = instance->generic.data_count_bit; i > 1; i--) {
            bool bit = bit_read(instance->generic.data, i - 2);
            index = rf_stream_emit(up, index, cap, bit, te_short);
            index = rf_stream_emit(up, index, cap, !bit, te_short);
        }
    }

    instance->encoder.size_upload = index;
    instance->encoder.front = 0;

    RF_STREAM_LOG_I(
        TAG,
        "Upload built: %d bursts, size_upload=%zu, data_count_bit=%u, data=0x%016llX",
        KIA_V2_TOTAL_BURSTS,
        instance->encoder.size_upload,
        instance->generic.data_count_bit,
        instance->generic.data);
}

#endif
#if RF_STREAM_WITH_ENCODER

void* kia_protocol_encoder_v2_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolEncoderKiaV2* instance = calloc(1, sizeof(RfStreamProtocolEncoderKiaV2));
    if(!instance) {
        return NULL;
    }

    instance->base.protocol = &kia_protocol_v2;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 0;
    instance->encoder.upload = NULL;
    instance->encoder.is_running = false;
    instance->encoder.front = 0;

    return instance;
}

#endif
#if RF_STREAM_WITH_ENCODER

RfStreamProtocolStatus
    kia_protocol_encoder_v2_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderKiaV2* instance = context;
    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    rf_record_rewind(flipper_format);

    do {
        if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
           RfStreamProtocolStatusOk) {
            RF_STREAM_LOG_E(TAG, "Missing or wrong Protocol");
            break;
        }

        uint32_t bits = 0;
        if(rf_stream_encoder_read_bit(flipper_format, NULL, 0, &bits) != RfStreamProtocolStatusOk) break;

        instance->generic.data_count_bit = kia_protocol_v2_const.min_count_bit_for_found;

        uint64_t key = 0;
        if(!rf_stream_flipper_read_hex_u64(flipper_format, FF_KEY, &key)) break;

        instance->generic.data = key;
        if(instance->generic.data == 0) break;

        uint32_t serial = UINT32_MAX;
        uint32_t btn = UINT32_MAX;
        uint32_t cnt = UINT32_MAX;
        // A full-width serial (0xFFFFFFFF) is data, not a missing-field sentinel.
        rf_record_rewind(flipper_format);
        if(!rf_record_read_uint32(flipper_format, FF_SERIAL, &serial, 1)) break;
        rf_record_rewind(flipper_format);
        if(!rf_record_read_uint32(flipper_format, FF_BTN, &btn, 1)) break;
        rf_record_rewind(flipper_format);
        if(!rf_record_read_uint32(flipper_format, FF_CNT, &cnt, 1)) break;
        if(btn > 0xFU || cnt > 0xFFFU) break;
        instance->generic.serial = serial;
        instance->generic.btn = (uint8_t)btn;
        instance->generic.cnt = (uint16_t)cnt;

        uint64_t new_data = 0;

        new_data |= 1ULL << 52;

        new_data |= ((uint64_t)instance->generic.serial << 20) & 0xFFFFFFFFF00000ULL;

        uint32_t uVar6 = ((uint32_t)(instance->generic.cnt & 0xFF) << 8) |
                         ((uint32_t)(instance->generic.btn & 0x0F) << 16) |
                         ((uint32_t)(instance->generic.cnt >> 4) & 0xF0);

        new_data |= (uint64_t)uVar6;

        instance->generic.data = new_data;
        instance->generic.data_count_bit = 53;

        RF_STREAM_LOG_I(
            TAG,
            "Encoder reconstruct: serial=0x%08lX, btn=0x%X, cnt=0x%03lX, uVar6=0x%05lX, data=0x%016llX",
            (unsigned long)instance->generic.serial,
            (unsigned int)instance->generic.btn,
            (unsigned long)instance->generic.cnt,
            (unsigned long)uVar6,
            (unsigned long long)instance->generic.data);

        instance->encoder.repeat = (int32_t)rf_stream_encoder_read_repeat(flipper_format, 10);

        rf_stream_encoder_buffer_ensure(instance, KIA_V2_UPLOAD_CAPACITY);
        kia_protocol_encoder_v2_get_upload(instance);

        uint8_t updated_key[8];
        rf_stream_u64_to_bytes_be(instance->generic.data, updated_key);
        if(!rf_record_insert_or_update_hex(flipper_format, FF_KEY, updated_key, 8)) break;
        rf_stream_flipper_update_or_insert_u32(flipper_format, "CRC", instance->generic.data & 0x0F);
        rf_stream_flipper_update_or_insert_u32(flipper_format, "RawCnt", (instance->generic.data >> 4) & 0xFFF);

        instance->encoder.is_running = true;

        RF_STREAM_LOG_I(
            TAG,
            "Encoder deserialized: repeat=%u, size_upload=%zu, is_running=%d, front=%zu",
            instance->encoder.repeat,
            instance->encoder.size_upload,
            instance->encoder.is_running,
            instance->encoder.front);

        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

#endif

void* kia_protocol_decoder_v2_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderKiaV2* instance = calloc(1, sizeof(RfStreamProtocolDecoderKiaV2));
    if(!instance) {
        return NULL;
    }
    instance->base.protocol = &kia_protocol_v2;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void kia_protocol_decoder_v2_reset(void* context) {
    RfStreamProtocolDecoderKiaV2* instance = context;
    instance->decoder.parser_step = KiaV2DecoderStepReset;
    instance->header_count = 0;
    instance->manchester_state = RfManchesterStateMid1;
    instance->decoder.decode_data = 0;
    instance->decoder.decode_count_bit = 0;
}

void kia_protocol_decoder_v2_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderKiaV2* instance = context;

    switch(instance->decoder.parser_step) {
    case KiaV2DecoderStepReset:
        if((level) && (DURATION_DIFF(duration, kia_protocol_v2_const.te_long) <
                       kia_protocol_v2_const.te_delta)) {
            instance->decoder.parser_step = KiaV2DecoderStepCheckPreamble;
            instance->decoder.te_last = duration;
            instance->header_count = 0;
            rf_manchester_advance(
                instance->manchester_state,
                RfManchesterEventReset,
                &instance->manchester_state,
                NULL);
        }
        break;

    case KiaV2DecoderStepCheckPreamble:
        if(level) // HIGH pulse
        {
            if(DURATION_DIFF(duration, kia_protocol_v2_const.te_long) <
               kia_protocol_v2_const.te_delta) {
                instance->decoder.te_last = duration;
                instance->header_count++;
            } else if(
                DURATION_DIFF(duration, kia_protocol_v2_const.te_short) <
                kia_protocol_v2_const.te_delta) {
                if(instance->header_count >= 100) {
                    instance->header_count = 0;
                    instance->decoder.decode_data = 0;
                    // add_bit already counts the seeded leading bit.
                    instance->decoder.decode_count_bit = 0;
                    instance->decoder.parser_step = KiaV2DecoderStepCollectRawBits;
                    rf_stream_protocol_blocks_add_bit(&instance->decoder, 1);
                } else {
                    instance->decoder.te_last = duration;
                }
            } else {
                instance->decoder.parser_step = KiaV2DecoderStepReset;
            }
        } else {
            if(DURATION_DIFF(duration, kia_protocol_v2_const.te_long) <
               kia_protocol_v2_const.te_delta) {
                instance->header_count++;
                instance->decoder.te_last = duration;
            } else if(
                DURATION_DIFF(duration, kia_protocol_v2_const.te_short) <
                kia_protocol_v2_const.te_delta) {
                instance->decoder.te_last = duration;
            } else {
                instance->decoder.parser_step = KiaV2DecoderStepReset;
            }
        }
        break;

    case KiaV2DecoderStepCollectRawBits: {
        RfManchesterEvent event = rf_stream_manchester_event(duration, level, &kia_protocol_v2_const);
        if(event == RfManchesterEventReset) {
            instance->decoder.parser_step = KiaV2DecoderStepReset;
            break;
        }

        bool data_bit;
        if(rf_manchester_advance(
               instance->manchester_state, event, &instance->manchester_state, &data_bit)) {
            instance->decoder.decode_data = (instance->decoder.decode_data << 1) | data_bit;
            instance->decoder.decode_count_bit++;

            if(instance->decoder.decode_count_bit == 53) {
                instance->generic.data = instance->decoder.decode_data;
                instance->generic.data_count_bit = instance->decoder.decode_count_bit;

                instance->generic.serial = (uint32_t)((instance->generic.data >> 20) & 0xFFFFFFFF);
                instance->generic.btn = (uint8_t)((instance->generic.data >> 16) & 0x0F);

                uint16_t raw_count = (uint16_t)((instance->generic.data >> 4) & 0xFFF);
                instance->generic.cnt = ((raw_count >> 4) | (raw_count << 8)) & 0xFFF;

                if(instance->base.callback)
                    instance->base.callback(&instance->base, instance->base.context);

                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->header_count = 0;
                instance->decoder.parser_step = KiaV2DecoderStepReset;
            }
        }
        break;
    }
    }
}

RfStreamProtocolStatus kia_protocol_decoder_v2_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderKiaV2* instance = context;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
    if(ret != RfStreamProtocolStatusOk) return ret;

    ret = rf_stream_serialize_fields(
        flipper_format,
        RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT,
        instance->generic.serial,
        instance->generic.btn,
        instance->generic.cnt,
        0);
    if(ret != RfStreamProtocolStatusOk) return ret;

    uint32_t crc = instance->generic.data & 0x0F;
    if(!rf_record_write_uint32(flipper_format, "CRC", &crc, 1)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }
    uint32_t raw_count = (uint16_t)((instance->generic.data >> 4) & 0xFFF);
    if(!rf_record_write_uint32(flipper_format, "RawCnt", &raw_count, 1)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }
    return RfStreamProtocolStatusOk;
}

RfStreamProtocolStatus
    kia_protocol_decoder_v2_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderKiaV2* instance = context;
    RfStreamProtocolStatus status = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, kia_protocol_v2_const.min_count_bit_for_found);
    if(status == RfStreamProtocolStatusOk) {
        instance->generic.serial = (uint32_t)(instance->generic.data >> 20);
        instance->generic.btn = (uint8_t)((instance->generic.data >> 16) & 0x0F);
        uint16_t raw_count = (uint16_t)((instance->generic.data >> 4) & 0xFFF);
        instance->generic.cnt = ((raw_count >> 4) | (raw_count << 8)) & 0xFFF;
    }
    return status;
}

void kia_protocol_decoder_v2_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderKiaV2* instance = context;

    uint8_t crc = instance->generic.data & 0x0F;

    bool crc_valid = crc == kia_v2_calculate_crc(instance->generic.data);

    rf_text_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%013llX\r\n"
        "Sn:%08lX Btn:%X\r\n"
        "Cnt:%03lX CRC:%X - %s\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        instance->generic.data,
        instance->generic.serial,
        instance->generic.btn,
        instance->generic.cnt,
        crc,
        crc_valid ? "OK" : "BAD");
}

#endif
