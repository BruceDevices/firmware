// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define SUBARU_PROTOCOL_NAME "Subaru"

extern const RfStreamCodec subaru_protocol;

// Decoder functions
void* rf_stream_protocol_decoder_subaru_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_subaru_free(void* context);
void rf_stream_protocol_decoder_subaru_reset(void* context);
void rf_stream_protocol_decoder_subaru_feed(void* context, bool level, uint32_t duration);
RfStreamProtocolStatus rf_stream_protocol_decoder_subaru_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_subaru_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_subaru_get_string(void* context, RfText* output);

// Encoder functions
void* rf_stream_protocol_encoder_subaru_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_subaru_deserialize(void* context, RfRecord* flipper_format);




#define TAG "SubaruProtocol"

static const RfStreamBlockConst rf_stream_protocol_subaru_const = {
    .te_short = 800,
    .te_long = 1600,
    .te_delta = 200,
    .min_count_bit_for_found = 64,
};

#define SUBARU_PREAMBLE_PAIRS  75
#define SUBARU_GAP_US          2800
#define SUBARU_SYNC_US         2800
#define SUBARU_UPLOAD_CAPACITY 320U
_Static_assert(
    SUBARU_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "SUBARU_UPLOAD_CAPACITY exceeds shared upload slab");

typedef struct RfStreamProtocolDecoderSubaru {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    uint16_t header_count;

    uint64_t key;
    uint32_t serial;
    uint8_t btn;
    uint16_t cnt;
} RfStreamProtocolDecoderSubaru;

typedef struct RfStreamProtocolEncoderSubaru {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;

    uint64_t key;
    uint32_t serial;
    uint8_t btn;
    uint16_t cnt;
} RfStreamProtocolEncoderSubaru;

typedef enum {
    SubaruDecoderStepReset = 0,
    SubaruDecoderStepCheckPreamble,
    SubaruDecoderStepFoundGap,
    SubaruDecoderStepFoundSync,
    SubaruDecoderStepSaveDuration,
} SubaruDecoderStep;

static const char* subaru_get_button_name(uint8_t btn) {
    static const char* const names[4] = {"Lock", "Unlock", "Trunk", "Panic"};
    return names[btn & 0x03];
}

static uint8_t rf_stream_protocol_decoder_subaru_get_hash_data(void* context);

const RfStreamProtocolDecoder rf_stream_protocol_subaru_decoder = {
    .alloc = rf_stream_protocol_decoder_subaru_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_subaru_feed,
    .reset = rf_stream_protocol_decoder_subaru_reset,
    .get_hash_data = rf_stream_protocol_decoder_subaru_get_hash_data,
    .serialize = rf_stream_protocol_decoder_subaru_serialize,
    .deserialize = rf_stream_protocol_decoder_subaru_deserialize,
    .get_string = rf_stream_protocol_decoder_subaru_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_subaru_encoder = {
    .alloc = rf_stream_protocol_encoder_subaru_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = rf_stream_protocol_encoder_subaru_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_subaru_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec subaru_protocol = {
    .name = SUBARU_PROTOCOL_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_subaru_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_subaru_encoder,
#else
    .encoder = NULL,
#endif
};

// ============================================================================
// DECODER HELPER FUNCTIONS
// ============================================================================

static void subaru_rotate_left_3bytes(uint8_t* b0, uint8_t* b1, uint8_t* b2, uint8_t count) {
    for(uint8_t i = 0; i < count; i++) {
        uint8_t t = *b0;
        *b0 = (uint8_t)((*b0 << 1) | (*b1 >> 7));
        *b1 = (uint8_t)((*b1 << 1) | (*b2 >> 7));
        *b2 = (uint8_t)((*b2 << 1) | (t >> 7));
    }
}

static bool subaru_decode_fields_exact(
    const uint8_t* kb,
    uint32_t* out_serial,
    uint8_t* out_btn,
    uint16_t* out_cnt) {
    const uint8_t b0 = kb[0];
    const uint8_t b1 = kb[1];
    const uint8_t b2 = kb[2];
    const uint8_t b3 = kb[3];
    const uint8_t b4 = kb[4];
    const uint8_t b5 = kb[5];
    const uint8_t b6 = kb[6];
    const uint8_t b7 = kb[7];

    *out_btn = b0 & 0x0F;
    *out_serial = ((uint32_t)b1 << 16) | ((uint32_t)b2 << 8) | b3;

    uint8_t lo = 0;
    if((b4 & 0x40) == 0) lo |= 0x01;
    if((b4 & 0x80) == 0) lo |= 0x02;
    if((b5 & 0x01) == 0) lo |= 0x04;
    if((b5 & 0x02) == 0) lo |= 0x08;
    if((b6 & 0x01) == 0) lo |= 0x10;
    if((b6 & 0x02) == 0) lo |= 0x20;
    if((b5 & 0x40) == 0) lo |= 0x40;
    if((b5 & 0x80) == 0) lo |= 0x80;
    uint8_t reg_sh1 = (uint8_t)(((b7 & 0x0F) << 4) | (b5 & 0x0C) | ((b6 >> 6) & 0x03));
    uint8_t reg_sh2 = (uint8_t)(((b6 & 0x3C) << 2) | ((b7 >> 4) & 0x0F));

    uint8_t ser0 = b3;
    uint8_t ser1 = b1;
    uint8_t ser2 = b2;
    subaru_rotate_left_3bytes(&ser0, &ser1, &ser2, (uint8_t)(4U + lo));

    uint8_t t1 = (uint8_t)(ser1 ^ reg_sh1);
    uint8_t t2 = (uint8_t)(ser2 ^ reg_sh2);

    uint8_t hi =
        (uint8_t)((((t1 & 0x10) == 0) ? 0x04 : 0x00) | (((t1 & 0x20) == 0) ? 0x08 : 0x00) |
                  (((t2 & 0x80) == 0) ? 0x02 : 0x00) | (((t2 & 0x40) == 0) ? 0x01 : 0x00) |
                  (((t1 & 0x01) == 0) ? 0x40 : 0x00) | (((t1 & 0x02) == 0) ? 0x80 : 0x00) |
                  (((t2 & 0x08) == 0) ? 0x20 : 0x00) | (((t2 & 0x04) == 0) ? 0x10 : 0x00));

    const uint8_t local34 = ser0;

    const uint8_t x1 = (uint8_t)(b1 ^ reg_sh1);
    const uint8_t x2 = (uint8_t)(b2 ^ reg_sh2);
    const uint8_t reg_hi_inv = (uint8_t)(~hi);
    const uint8_t expect1 = (uint8_t)(((x1 ^ (uint8_t)(reg_hi_inv << 2)) & 0x30) |
                                      ((x1 ^ (uint8_t)(reg_hi_inv >> 6)) & 0x03) |
                                      ((x1 ^ (uint8_t)((uint8_t)~lo << 2)) & 0xCC));
    const uint8_t expect2 = (uint8_t)(((x2 ^ (uint8_t)(reg_hi_inv >> 2)) & 0x0C) |
                                      ((x2 ^ (uint8_t)(reg_hi_inv << 6)) & 0xC0) |
                                      ((x2 ^ (uint8_t)(~lo >> 2)) & 0x33));

    const bool valid = (((uint8_t)(b1 ^ expect1) | (uint8_t)(b2 ^ expect2)) == 0) &&
                       ((uint8_t)(b0 ^ local34) == lo);

    *out_cnt = (uint16_t)((uint16_t)hi << 8 | lo);

    return valid;
}

static bool subaru_process_data(RfStreamProtocolDecoderSubaru* instance) {
    if(instance->decoder.decode_count_bit < 64) {
        return false;
    }

    uint8_t b[8];
    rf_stream_u64_to_bytes_be(instance->decoder.decode_data, b);

    instance->key = ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48) | ((uint64_t)b[2] << 40) |
                    ((uint64_t)b[3] << 32) | ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16) |
                    ((uint64_t)b[6] << 8) | ((uint64_t)b[7]);

    (void)subaru_decode_fields_exact(b, &instance->serial, &instance->btn, &instance->cnt);

    return true;
}

static uint8_t rf_stream_protocol_decoder_subaru_get_hash_data(void* context) {
    RfStreamProtocolDecoderSubaru* instance = context;
    const uint8_t* p = (const uint8_t*)&instance->decoder.decode_data;
    uint8_t hash = 0;
    size_t bytes = (size_t)(instance->decoder.decode_count_bit >> 3) + 1U;
    if(bytes > sizeof(instance->decoder.decode_data)) {
        bytes = sizeof(instance->decoder.decode_data);
    }
    for(size_t i = 0; i < bytes; i++) {
        hash ^= p[i];
    }
    return hash;
}

// ============================================================================
// ENCODER IMPLEMENTATION
// ============================================================================
#if RF_STREAM_WITH_ENCODER

static void subaru_encode_count(uint8_t* KB, uint16_t count) {
    uint8_t lo = count & 0xFF;
    uint8_t hi = (count >> 8) & 0xFF;

    KB[4] &= ~0xC0;
    KB[5] &= ~0xC3;
    KB[6] &= ~0x03;

    if((lo & 0x01) == 0) KB[4] |= 0x40;
    if((lo & 0x02) == 0) KB[4] |= 0x80;
    if((lo & 0x04) == 0) KB[5] |= 0x01;
    if((lo & 0x08) == 0) KB[5] |= 0x02;
    if((lo & 0x10) == 0) KB[6] |= 0x01;
    if((lo & 0x20) == 0) KB[6] |= 0x02;
    if((lo & 0x40) == 0) KB[5] |= 0x40;
    if((lo & 0x80) == 0) KB[5] |= 0x80;

    uint8_t SER0 = KB[3];
    uint8_t SER1 = KB[1];
    uint8_t SER2 = KB[2];

    uint8_t total_rot = 4 + lo;
    for(uint8_t i = 0; i < total_rot; ++i) {
        uint8_t t_bit = (SER0 >> 7) & 1;
        SER0 = ((SER0 << 1) & 0xFE) | ((SER1 >> 7) & 1);
        SER1 = ((SER1 << 1) & 0xFE) | ((SER2 >> 7) & 1);
        SER2 = ((SER2 << 1) & 0xFE) | t_bit;
    }

    const uint8_t rel = (uint8_t)(SER0 ^ KB[0]);
    KB[4] = (uint8_t)((KB[4] & 0xC0) | ((rel >> 2) & 0x3F));
    KB[5] = (uint8_t)((KB[5] & 0xCF) | ((rel << 4) & 0x30));

    uint8_t T1 = 0xFF;
    uint8_t T2 = 0xFF;

    if(hi & 0x04) T1 &= ~0x10;
    if(hi & 0x08) T1 &= ~0x20;
    if(hi & 0x02) T2 &= ~0x80;
    if(hi & 0x01) T2 &= ~0x40;
    if(hi & 0x40) T1 &= ~0x01;
    if(hi & 0x80) T1 &= ~0x02;
    if(hi & 0x20) T2 &= ~0x08;
    if(hi & 0x10) T2 &= ~0x04;

    uint8_t new_REG_SH1 = T1 ^ SER1;
    uint8_t new_REG_SH2 = T2 ^ SER2;

    KB[5] &= ~0x0C;
    KB[6] &= ~0xC0;

    KB[7] = (KB[7] & 0xF0) | ((new_REG_SH1 >> 4) & 0x0F);

    if(new_REG_SH1 & 0x04) KB[5] |= 0x04;
    if(new_REG_SH1 & 0x08) KB[5] |= 0x08;
    if(new_REG_SH1 & 0x02) KB[6] |= 0x80;
    if(new_REG_SH1 & 0x01) KB[6] |= 0x40;

    KB[6] = (KB[6] & 0xC3) | ((new_REG_SH2 >> 2) & 0x3C);

    KB[7] = (KB[7] & 0x0F) | ((new_REG_SH2 << 4) & 0xF0);
}

void* rf_stream_protocol_encoder_subaru_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolEncoderSubaru* instance = calloc(1, sizeof(RfStreamProtocolEncoderSubaru));
    if(!instance) {
        return NULL;
    }

    instance->base.protocol = &subaru_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    rf_stream_encoder_buffer_ensure(instance, SUBARU_UPLOAD_CAPACITY);
    instance->encoder.is_running = false;
    instance->encoder.front = 0;

    instance->key = 0;
    instance->serial = 0;
    instance->btn = 0;
    instance->cnt = 0;

    return instance;
}

#endif
#if RF_STREAM_WITH_ENCODER

static void rf_stream_protocol_encoder_subaru_get_upload(RfStreamProtocolEncoderSubaru* instance) {
    size_t index = 0;

    const uint32_t te_short = rf_stream_protocol_subaru_const.te_short;
    const uint32_t te_long = rf_stream_protocol_subaru_const.te_long;
    const uint32_t gap_duration = SUBARU_GAP_US;
    const uint32_t sync_duration = SUBARU_SYNC_US;

    RF_STREAM_LOG_I(
        TAG,
        "Building upload: key=0x%016llX, serial=0x%06lX, btn=0x%X, cnt=0x%04X",
        instance->key,
        instance->serial,
        instance->btn,
        instance->cnt);

    LevelDuration* up = instance->encoder.upload;
    const size_t cap = SUBARU_UPLOAD_CAPACITY;

    for(int i = 0; i < SUBARU_PREAMBLE_PAIRS; i++) {
        index = rf_stream_emit(up, index, cap, false, te_long);
        index = rf_stream_emit(up, index, cap, true, te_long);
    }

    index = rf_stream_emit(up, index, cap, false, gap_duration);
    index = rf_stream_emit(up, index, cap, true, sync_duration);

    for(int i = 63; i >= 0; i--) {
        bool bit = (instance->key >> i) & 1;
        if(bit) {
            index = rf_stream_emit(up, index, cap, false, te_long);
            index = rf_stream_emit(up, index, cap, true, te_short);
        } else {
            index = rf_stream_emit(up, index, cap, false, te_short);
            index = rf_stream_emit(up, index, cap, true, te_long);
        }
    }

    index = rf_stream_emit(up, index, cap, false, gap_duration);
    index = rf_stream_emit(up, index, cap, true, sync_duration);

    instance->encoder.size_upload = index;
    instance->encoder.front = 0;

    RF_STREAM_LOG_I(TAG, "Upload built: %zu elements", instance->encoder.size_upload);
}

#endif
#if RF_STREAM_WITH_ENCODER

RfStreamProtocolStatus
    rf_stream_protocol_encoder_subaru_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderSubaru* instance = context;

    instance->encoder.is_running = false;
    instance->encoder.front = 0;

    if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
       RfStreamProtocolStatusOk) {
        return RfStreamProtocolStatusError;
    }

    rf_record_rewind(flipper_format);
    if(rf_stream_block_generic_deserialize_check_count_bit(
           &instance->generic,
           flipper_format,
           rf_stream_protocol_subaru_const.min_count_bit_for_found) != RfStreamProtocolStatusOk) {
        return RfStreamProtocolStatusError;
    }

    instance->key = instance->generic.data;

    uint8_t b[8];
    rf_stream_u64_to_bytes_be(instance->key, b);

    uint32_t serial = ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
    uint8_t btn = b[0] & 0x0F;
    uint16_t cnt = 0;
    (void)subaru_decode_fields_exact(b, &serial, &btn, &cnt);

    uint32_t serial_ff = serial;
    uint32_t btn_ff = btn;
    uint32_t cnt_ff = cnt;
    rf_stream_encoder_read_fields(flipper_format, &serial_ff, &btn_ff, &cnt_ff, NULL);

    instance->serial = serial_ff & 0xFFFFFF;
    instance->btn = (uint8_t)(btn_ff & 0x0F);
    instance->cnt = (uint16_t)(cnt_ff & 0xFFFF);

    b[0] = (b[0] & 0xF0) | instance->btn;
    b[1] = (uint8_t)(instance->serial >> 16);
    b[2] = (uint8_t)(instance->serial >> 8);
    b[3] = (uint8_t)(instance->serial);
    subaru_encode_count(b, instance->cnt);

    instance->key = rf_stream_bytes_to_u64_be(b);
    instance->generic.data = instance->key;
    instance->generic.data_count_bit = rf_stream_protocol_subaru_const.min_count_bit_for_found;
    instance->generic.serial = instance->serial;
    instance->generic.btn = instance->btn;
    instance->generic.cnt = instance->cnt;

    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_SERIAL, instance->serial);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_BTN, instance->btn);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_CNT, instance->cnt);
    {
        uint8_t key_bytes[8];
        rf_stream_u64_to_bytes_be(instance->key, key_bytes);
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_hex(flipper_format, FF_KEY, key_bytes, sizeof(key_bytes));
    }

    instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, 10);

    rf_stream_encoder_buffer_ensure(instance, SUBARU_UPLOAD_CAPACITY);
    rf_stream_protocol_encoder_subaru_get_upload(instance);
    instance->encoder.is_running = true;

    return RfStreamProtocolStatusOk;
}

#endif
// ============================================================================
// DECODER IMPLEMENTATION
// ============================================================================

void* rf_stream_protocol_decoder_subaru_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderSubaru* instance = calloc(1, sizeof(RfStreamProtocolDecoderSubaru));
    if(!instance) {
        return NULL;
    }
    instance->base.protocol = &subaru_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void rf_stream_protocol_decoder_subaru_reset(void* context) {
    RfStreamProtocolDecoderSubaru* instance = context;
    instance->decoder.parser_step = SubaruDecoderStepReset;
}

void rf_stream_protocol_decoder_subaru_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderSubaru* instance = context;

    const uint32_t te_short = rf_stream_protocol_subaru_const.te_short;
    const uint32_t te_long = rf_stream_protocol_subaru_const.te_long;
    const uint32_t te_delta = rf_stream_protocol_subaru_const.te_delta;

    switch(instance->decoder.parser_step) {
    case SubaruDecoderStepReset:

        if(level && (DURATION_DIFF(duration, te_long) < te_delta)) {
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
            instance->header_count = 0;
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = SubaruDecoderStepCheckPreamble;
        }
        break;

    case SubaruDecoderStepCheckPreamble:
        if(level) {
            break;
        }

        if((DURATION_DIFF(duration, te_long) < te_delta) &&
           (DURATION_DIFF(instance->decoder.te_last, te_long) < te_delta)) {
            instance->header_count++;
            break;
        }

        if((instance->header_count >= 0x15) && (DURATION_DIFF(duration, SUBARU_GAP_US) < 800)) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = SubaruDecoderStepFoundGap;
        } else {
            instance->decoder.parser_step = SubaruDecoderStepReset;
        }
        break;

    case SubaruDecoderStepFoundGap:
        if(level && (DURATION_DIFF(duration, SUBARU_SYNC_US) < 800) &&
           (DURATION_DIFF(instance->decoder.te_last, SUBARU_GAP_US) < 800)) {
            instance->decoder.parser_step = SubaruDecoderStepFoundSync;
        } else {
            instance->decoder.parser_step = SubaruDecoderStepReset;
        }
        break;

    case SubaruDecoderStepFoundSync:
        if(!level && ((DURATION_DIFF(duration, te_long) < te_delta) ||
                      (DURATION_DIFF(duration, te_short) < te_delta))) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = SubaruDecoderStepSaveDuration;
        } else {
            instance->decoder.parser_step = SubaruDecoderStepReset;
        }
        break;

    case SubaruDecoderStepSaveDuration: {
        uint8_t next_step = SubaruDecoderStepReset;
        bool bit = false;
        bool valid = false;

        if(level) {
            if(DURATION_DIFF(duration, te_long) < te_delta) {
                if(DURATION_DIFF(instance->decoder.te_last, te_short) < te_delta) {
                    bit = false;
                    valid = true;
                }
            } else if(DURATION_DIFF(duration, te_short) < te_delta) {
                if(DURATION_DIFF(instance->decoder.te_last, te_long) < te_delta) {
                    bit = true;
                    valid = true;
                }
            }

            if(valid) {
                rf_stream_protocol_blocks_add_bit(&instance->decoder, bit);
                next_step = SubaruDecoderStepFoundSync;
            }
        }

        instance->decoder.parser_step = next_step;

        if((instance->decoder.decode_count_bit >= 64) &&
           (instance->decoder.decode_count_bit <= 80)) {
            instance->generic.data_count_bit = instance->decoder.decode_count_bit;
            instance->generic.data = instance->decoder.decode_data;
            if(subaru_process_data(instance)) {
                instance->generic.serial = instance->serial;
                instance->generic.btn = instance->btn;
                instance->generic.cnt = instance->cnt;
            }

            if(instance->base.callback) {
                instance->base.callback(&instance->base, instance->base.context);
            }

            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
            instance->decoder.parser_step = SubaruDecoderStepReset;
        }
        break;
    }
    }
}

RfStreamProtocolStatus rf_stream_protocol_decoder_subaru_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderSubaru* instance = context;

    uint8_t b[8];
    rf_stream_u64_to_bytes_be(instance->generic.data, b);
    (void)subaru_decode_fields_exact(b, &instance->serial, &instance->btn, &instance->cnt);
    instance->key = instance->generic.data;
    instance->generic.serial = instance->serial;
    instance->generic.btn = instance->btn;
    instance->generic.cnt = instance->cnt;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
    if(ret != RfStreamProtocolStatusOk) {
        return ret;
    }

    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_SERIAL, instance->serial);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_BTN, instance->btn);
    rf_stream_flipper_update_or_insert_u32(flipper_format, FF_CNT, instance->cnt);

    return rf_stream_write_display(
        flipper_format, instance->generic.protocol_name, subaru_get_button_name(instance->btn));
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_subaru_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderSubaru* instance = context;
    return rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, rf_stream_protocol_subaru_const.min_count_bit_for_found);
}

void rf_stream_protocol_decoder_subaru_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderSubaru* instance = context;

    uint8_t b[8];
    rf_stream_u64_to_bytes_be(instance->generic.data, b);
    (void)subaru_decode_fields_exact(b, &instance->serial, &instance->btn, &instance->cnt);
    instance->key = instance->generic.data;
    instance->generic.serial = instance->serial;
    instance->generic.btn = instance->btn;
    instance->generic.cnt = instance->cnt;

    rf_text_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%016llX\r\n"
        "Sn:%06lX Btn:%01X [%s]\r\n"
        "Cnt:%04lX",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        instance->generic.data,
        instance->serial,
        instance->btn,
        subaru_get_button_name(instance->btn),
        (uint32_t)instance->cnt);
}

#endif
