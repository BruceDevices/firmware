// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#include "rf_vehicle_support.h"
#define KIA_PROTOCOL_V3_V4_NAME "Kia V3/V4"

extern const RfStreamCodec kia_protocol_v3_v4;

// Decoder functions
void* kia_protocol_decoder_v3_v4_alloc(RfStreamEnvironment* environment);
void kia_protocol_decoder_v3_v4_free(void* context);
void kia_protocol_decoder_v3_v4_reset(void* context);
void kia_protocol_decoder_v3_v4_feed(void* context, bool level, uint32_t duration);
RfStreamProtocolStatus kia_protocol_decoder_v3_v4_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    kia_protocol_decoder_v3_v4_deserialize(void* context, RfRecord* flipper_format);
void kia_protocol_decoder_v3_v4_get_string(void* context, RfText* output);

// Encoder functions
void* kia_protocol_encoder_v3_v4_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    kia_protocol_encoder_v3_v4_deserialize(void* context, RfRecord* flipper_format);
void kia_protocol_encoder_v3_v4_stop(void* context);
LevelDuration kia_protocol_encoder_v3_v4_yield(void* context);

// Encoder helper functions for UI
void kia_protocol_encoder_v3_v4_set_button(void* context, uint8_t button);
void kia_protocol_encoder_v3_v4_set_counter(void* context, uint16_t counter);
void kia_protocol_encoder_v3_v4_increment_counter(void* context);
uint16_t kia_protocol_encoder_v3_v4_get_counter(void* context);
uint8_t kia_protocol_encoder_v3_v4_get_button(void* context);







#define TAG "KiaV3V4"

static const char* kia_version_names[] = {"Kia V4", "Kia V3"};

#define KIA_V3_V4_PREAMBLE_PAIRS  12U
#define KIA_V3_V4_BIT_COUNT       64U
#define KIA_V3_V4_CRC_BIT_COUNT   4U
#define KIA_V3_V4_CRC_SWEEP_COUNT 16U
#define KIA_V3_V4_SYNC_DURATION   1200U
#define KIA_V3_V4_END_MARKER_US   800U
#define KIA_V3_V4_DEFAULT_REPEAT  KIA_V3_V4_CRC_SWEEP_COUNT

#define KIA_V3_V4_DATA_OFFSET   ((KIA_V3_V4_PREAMBLE_PAIRS * 2U) + 2U) // 26
#define KIA_V3_V4_CRC_OFFSET    (KIA_V3_V4_DATA_OFFSET + (KIA_V3_V4_BIT_COUNT * 2U))
#define KIA_V3_V4_END_OFFSET    (KIA_V3_V4_CRC_OFFSET + (KIA_V3_V4_CRC_BIT_COUNT * 2U))
#define KIA_V3_V4_BURST_ENTRIES (KIA_V3_V4_END_OFFSET + 2U)

#define KIA_V3_V4_UPLOAD_CAPACITY KIA_V3_V4_BURST_ENTRIES

_Static_assert(
    KIA_V3_V4_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "KIA_V3_V4_UPLOAD_CAPACITY exceeds shared upload slab");

static const RfStreamBlockConst kia_protocol_v3_v4_const = {
    .te_short = 400,
    .te_long = 800,
    .te_delta = 150,
    .min_count_bit_for_found = 68,
};

typedef struct RfStreamProtocolDecoderKiaV3V4 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;
    uint16_t header_count;

    uint8_t raw_bits[32];
    uint16_t raw_bit_count;
    bool is_v3_sync;

    uint32_t encrypted;
    uint32_t decrypted;
    uint8_t crc;
    uint8_t version;
} RfStreamProtocolDecoderKiaV3V4;

typedef struct RfStreamProtocolEncoderKiaV3V4 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;

    uint32_t serial;
    uint8_t btn;
    uint16_t cnt;
    uint8_t version;

    uint32_t encrypted;
    uint32_t decrypted;

    uint8_t crc_iter;
    uint8_t bursts_sent;
} RfStreamProtocolEncoderKiaV3V4;

typedef enum {
    KiaV3V4DecoderStepReset = 0,
    KiaV3V4DecoderStepCheckPreamble,
    KiaV3V4DecoderStepCollectRawBits,
} KiaV3V4DecoderStep;

static void kia_v3_v4_add_raw_bit(RfStreamProtocolDecoderKiaV3V4* instance, bool bit) {
    if(instance->raw_bit_count < 256) {
        uint16_t byte_idx = instance->raw_bit_count / 8;
        uint8_t bit_idx = 7 - (instance->raw_bit_count % 8);
        if(bit) {
            instance->raw_bits[byte_idx] |= (1 << bit_idx);
        } else {
            instance->raw_bits[byte_idx] &= ~(1 << bit_idx);
        }
        instance->raw_bit_count++;
    }
}

#if RF_STREAM_WITH_ENCODER
static inline void kia_v3_v4_emit_bit_pwm(LevelDuration* upload, size_t* idx, bool bit, bool v4) {
    const uint32_t te_short = kia_protocol_v3_v4_const.te_short;
    const uint32_t te_long = kia_protocol_v3_v4_const.te_long;
    const uint32_t first_us = bit ? te_short : te_long;
    const uint32_t second_us = bit ? te_long : te_short;

    if(v4) {
        upload[(*idx)++] = level_duration_make(false, (int32_t)first_us);
        upload[(*idx)++] = level_duration_make(true, (int32_t)second_us);
    } else {
        upload[(*idx)++] = level_duration_make(true, (int32_t)first_us);
        upload[(*idx)++] = level_duration_make(false, (int32_t)second_us);
    }
}
static uint64_t kia_v3_v4_build_tx_bitstream(RfStreamProtocolEncoderKiaV3V4* instance) {
    const uint32_t serial_btn = (instance->serial & 0x0FFFFFFFU) |
                                ((uint32_t)(instance->btn & 0x0FU) << 28);
    const uint64_t key = ((uint64_t)serial_btn << 32) | (uint64_t)instance->encrypted;
    return rf_stream_protocol_blocks_reverse_key(key, 64);
}
#endif

static bool kia_v3_v4_process_buffer(RfStreamProtocolDecoderKiaV3V4* instance) {
    if(instance->raw_bit_count < 68) {
        return false;
    }

    uint8_t* b = instance->raw_bits;

    if(instance->is_v3_sync) {
        uint16_t num_bytes = (instance->raw_bit_count + 7) / 8;
        for(uint16_t i = 0; i < num_bytes; i++) {
            b[i] = ~b[i];
        }
    }

    uint8_t crc = (b[8] >> 4) & 0x0F;

    uint32_t encrypted =
        ((uint32_t)rf_stream_reverse_bits8(b[3]) << 24) | ((uint32_t)rf_stream_reverse_bits8(b[2]) << 16) |
        ((uint32_t)rf_stream_reverse_bits8(b[1]) << 8) | (uint32_t)rf_stream_reverse_bits8(b[0]);

    uint32_t serial = ((uint32_t)rf_stream_reverse_bits8(b[7] & 0xF0) << 24) |
                      ((uint32_t)rf_stream_reverse_bits8(b[6]) << 16) |
                      ((uint32_t)rf_stream_reverse_bits8(b[5]) << 8) | (uint32_t)rf_stream_reverse_bits8(b[4]);

    uint8_t btn = (rf_stream_reverse_bits8(b[7]) & 0xF0) >> 4;
    uint8_t our_serial_lsb = serial & 0xFF;

    uint32_t decrypted = keeloq_decrypt(encrypted, get_kia_mf_key());
    uint8_t dec_btn = (decrypted >> 28) & 0x0F;
    uint8_t dec_serial_lsb = (decrypted >> 16) & 0xFF;

    if(dec_btn != btn || dec_serial_lsb != our_serial_lsb) {
        return false;
    }

    instance->encrypted = encrypted;
    instance->decrypted = decrypted;
    instance->crc = crc;
    instance->generic.serial = serial;
    instance->generic.btn = btn;
    instance->generic.cnt = decrypted & 0xFFFF;
    instance->version = instance->is_v3_sync ? 1 : 0;

    uint64_t key_data = ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48) | ((uint64_t)b[2] << 40) |
                        ((uint64_t)b[3] << 32) | ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16) |
                        ((uint64_t)b[6] << 8) | (uint64_t)b[7];
    instance->generic.data = key_data;
    instance->generic.data_count_bit = 68;

    return true;
}

const RfStreamProtocolDecoder kia_protocol_v3_v4_decoder = {
    .alloc = kia_protocol_decoder_v3_v4_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = kia_protocol_decoder_v3_v4_feed,
    .reset = kia_protocol_decoder_v3_v4_reset,
    .get_hash_data = rf_stream_decoder_hash_blocks,
    .serialize = kia_protocol_decoder_v3_v4_serialize,
    .deserialize = kia_protocol_decoder_v3_v4_deserialize,
    .get_string = kia_protocol_decoder_v3_v4_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder kia_protocol_v3_v4_encoder = {
    .alloc = kia_protocol_encoder_v3_v4_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = kia_protocol_encoder_v3_v4_deserialize,
    .stop = kia_protocol_encoder_v3_v4_stop,
    .yield = kia_protocol_encoder_v3_v4_yield,
};
#else
const RfStreamProtocolEncoder kia_protocol_v3_v4_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec kia_protocol_v3_v4 = {
    .name = KIA_PROTOCOL_V3_V4_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &kia_protocol_v3_v4_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &kia_protocol_v3_v4_encoder,
#else
    .encoder = NULL,
#endif
};

// ============================================================================
// ENCODER IMPLEMENTATION
// ============================================================================
#if RF_STREAM_WITH_ENCODER

void* kia_protocol_encoder_v3_v4_alloc(RfStreamEnvironment* environment) {
    if(environment) {
        rf_vehicle_keys_load(environment);
    }
    if(!rf_vehicle_keys_has_kia_mf_key()) {
        RF_STREAM_LOG_E(TAG, "Kia V3/V4 encoder missing KIA_KEY1 keystore entry");
        return NULL;
    }

    RfStreamProtocolEncoderKiaV3V4* instance = malloc(sizeof(RfStreamProtocolEncoderKiaV3V4));

    instance->base.protocol = &kia_protocol_v3_v4;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->serial = 0;
    instance->btn = 0;
    instance->cnt = 0;
    instance->version = 0;
    instance->crc_iter = 0;
    instance->bursts_sent = 0;

    rf_stream_encoder_buffer_ensure(instance, KIA_V3_V4_UPLOAD_CAPACITY);
    instance->encoder.repeat = (int32_t)KIA_V3_V4_DEFAULT_REPEAT;
    instance->encoder.front = 0;
    instance->encoder.is_running = false;

    RF_STREAM_LOG_I(TAG, "Encoder allocated at %p", instance);
    return instance;
}

#endif
#if RF_STREAM_WITH_ENCODER

static void kia_protocol_encoder_v3_v4_build_packet(
    RfStreamProtocolEncoderKiaV3V4* instance,
    uint8_t* raw_bytes) {
    uint32_t plaintext = (uint32_t)(instance->cnt & 0xFFFFU) |
                         ((uint32_t)(instance->serial & 0x3FFU) << 16) |
                         ((uint32_t)(instance->btn & 0x0FU) << 28);

    instance->decrypted = plaintext;

    uint32_t encrypted = keeloq_encrypt(plaintext, get_kia_mf_key());
    instance->encrypted = encrypted;

    RF_STREAM_LOG_I(
        TAG,
        "Encrypt: plain=0x%08lX -> enc=0x%08lX",
        (unsigned long)plaintext,
        (unsigned long)encrypted);

    raw_bytes[0] = rf_stream_reverse_bits8((encrypted >> 0) & 0xFF);
    raw_bytes[1] = rf_stream_reverse_bits8((encrypted >> 8) & 0xFF);
    raw_bytes[2] = rf_stream_reverse_bits8((encrypted >> 16) & 0xFF);
    raw_bytes[3] = rf_stream_reverse_bits8((encrypted >> 24) & 0xFF);

    uint32_t serial_btn = (instance->serial & 0x0FFFFFFFU) |
                          ((uint32_t)(instance->btn & 0x0F) << 28);
    raw_bytes[4] = rf_stream_reverse_bits8((serial_btn >> 0) & 0xFF);
    raw_bytes[5] = rf_stream_reverse_bits8((serial_btn >> 8) & 0xFF);
    raw_bytes[6] = rf_stream_reverse_bits8((serial_btn >> 16) & 0xFF);
    raw_bytes[7] = rf_stream_reverse_bits8((serial_btn >> 24) & 0xFF);

    RF_STREAM_LOG_I(
        TAG,
        "TX raw: %02X %02X %02X %02X %02X %02X %02X %02X",
        raw_bytes[0],
        raw_bytes[1],
        raw_bytes[2],
        raw_bytes[3],
        raw_bytes[4],
        raw_bytes[5],
        raw_bytes[6],
        raw_bytes[7]);

    instance->generic.data = ((uint64_t)raw_bytes[0] << 56) | ((uint64_t)raw_bytes[1] << 48) |
                             ((uint64_t)raw_bytes[2] << 40) | ((uint64_t)raw_bytes[3] << 32) |
                             ((uint64_t)raw_bytes[4] << 24) | ((uint64_t)raw_bytes[5] << 16) |
                             ((uint64_t)raw_bytes[6] << 8) | (uint64_t)raw_bytes[7];
    instance->generic.data_count_bit = 68;

    RF_STREAM_LOG_I(
        TAG,
        "Packet built: Serial=0x%07lX, Btn=0x%X, Cnt=0x%04X",
        (unsigned long)instance->serial,
        instance->btn,
        instance->cnt);
}
static void kia_protocol_encoder_v3_v4_patch_crc(RfStreamProtocolEncoderKiaV3V4* instance) {
    if(!instance || !instance->encoder.upload) return;
    const bool v4 = (instance->version == 0);
    const uint8_t crc = instance->crc_iter & 0x0FU;
    size_t idx = KIA_V3_V4_CRC_OFFSET;
    for(int b = 3; b >= 0; b--) {
        const bool bit = (crc >> b) & 1U;
        kia_v3_v4_emit_bit_pwm(instance->encoder.upload, &idx, bit, v4);
    }
}

#endif
#if RF_STREAM_WITH_ENCODER

static void kia_protocol_encoder_v3_v4_get_upload(RfStreamProtocolEncoderKiaV3V4* instance) {
    uint8_t raw_bytes[8];
    kia_protocol_encoder_v3_v4_build_packet(instance, raw_bytes);

    const bool v4 = (instance->version == 0);
    const uint64_t tx_key = kia_v3_v4_build_tx_bitstream(instance);

    size_t idx = 0;
    LevelDuration* upload = instance->encoder.upload;
    const uint32_t te_short = kia_protocol_v3_v4_const.te_short;

    for(uint32_t i = 0; i < KIA_V3_V4_PREAMBLE_PAIRS; i++) {
        if(v4) {
            upload[idx++] = level_duration_make(false, (int32_t)te_short);
            upload[idx++] = level_duration_make(true, (int32_t)te_short);
        } else {
            upload[idx++] = level_duration_make(true, (int32_t)te_short);
            upload[idx++] = level_duration_make(false, (int32_t)te_short);
        }
    }

    if(v4) {
        upload[idx++] = level_duration_make(false, (int32_t)te_short);
        upload[idx++] = level_duration_make(true, (int32_t)KIA_V3_V4_SYNC_DURATION);
    } else {
        upload[idx++] = level_duration_make(true, (int32_t)te_short);
        upload[idx++] = level_duration_make(false, (int32_t)KIA_V3_V4_SYNC_DURATION);
    }

    for(int i = 63; i >= 0; i--) {
        const bool bit = (tx_key >> i) & 1ULL;
        kia_v3_v4_emit_bit_pwm(upload, &idx, bit, v4);
    }

    const uint8_t crc = instance->crc_iter & 0x0FU;
    for(int b = 3; b >= 0; b--) {
        const bool bit = (crc >> b) & 1U;
        kia_v3_v4_emit_bit_pwm(upload, &idx, bit, v4);
    }

    if(v4) {
        upload[idx++] = level_duration_make(false, (int32_t)KIA_V3_V4_END_MARKER_US);
        upload[idx++] = level_duration_make(true, (int32_t)KIA_V3_V4_END_MARKER_US);
    } else {
        upload[idx++] = level_duration_make(true, (int32_t)KIA_V3_V4_END_MARKER_US);
        upload[idx++] = level_duration_make(false, (int32_t)KIA_V3_V4_END_MARKER_US);
    }

    instance->encoder.size_upload = idx;
    instance->encoder.front = 0;

    RF_STREAM_LOG_I(
        TAG,
        "Upload built: size=%zu %s enc=0x%08lX tx=0x%016llX crc=0x%X",
        instance->encoder.size_upload,
        v4 ? "V4" : "V3",
        (unsigned long)instance->encrypted,
        (unsigned long long)tx_key,
        crc);
}

#endif
#if RF_STREAM_WITH_ENCODER

RfStreamProtocolStatus
    kia_protocol_encoder_v3_v4_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderKiaV3V4* instance = context;

    instance->encoder.is_running = false;
    instance->encoder.front = 0;
    //instance->encoder.repeat = 40;

    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    rf_record_rewind(flipper_format);

    do {
        RfText* temp_str = rf_text_alloc();
        if(!rf_record_read_string(flipper_format, FF_PROTOCOL, temp_str)) {
            RF_STREAM_LOG_E(TAG, "Missing Protocol");
            rf_text_free(temp_str);
            break;
        }

        // Accept "Kia V3/V4", "Kia V3", or "Kia V4"
        const char* proto_str = rf_text_get_cstr(temp_str);
        if(!rf_text_equal(temp_str, instance->base.protocol->name) &&
           strcmp(proto_str, "Kia V3") != 0 && strcmp(proto_str, "Kia V4") != 0) {
            RF_STREAM_LOG_E(TAG, "Wrong protocol %s", proto_str);
            rf_text_free(temp_str);
            break;
        }

        // Set version based on protocol name if specific
        bool version_from_protocol_name = false;

        if(strcmp(proto_str, "Kia V3") == 0) {
            instance->version = 1;
            version_from_protocol_name = true;
            RF_STREAM_LOG_I(TAG, "Protocol name indicates V3");
        } else if(strcmp(proto_str, "Kia V4") == 0) {
            instance->version = 0;
            version_from_protocol_name = true;
            RF_STREAM_LOG_I(TAG, "Protocol name indicates V4");
        }

        rf_text_free(temp_str);

        uint32_t bits = 0;
        if(rf_stream_encoder_read_bit(flipper_format, NULL, 0, &bits) != RfStreamProtocolStatusOk) break;
        instance->generic.data_count_bit = 68;

        if(!rf_stream_flipper_read_hex_u64(flipper_format, FF_KEY, &instance->generic.data)) {
            break;
        }

        uint32_t serial = UINT32_MAX;
        uint32_t btn = UINT32_MAX;
        uint32_t cnt = UINT32_MAX;
        rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);
        if(serial == UINT32_MAX || btn == UINT32_MAX || cnt == UINT32_MAX) break;
        instance->serial = serial;
        instance->btn = (uint8_t)btn;
        instance->cnt = (uint16_t)cnt;
        instance->generic.serial = instance->serial;
        instance->generic.btn = instance->btn;
        instance->generic.cnt = instance->cnt;

        // Read version - ONLY use file version if protocol name didn't specify one
        rf_record_rewind(flipper_format);
        uint32_t version_temp;
        if(rf_record_read_uint32(flipper_format, "KIAVersion", &version_temp, 1)) {
            if(!version_from_protocol_name) {
                instance->version = (uint8_t)version_temp;
            }
        } else if(!version_from_protocol_name) {
            instance->version = 0;
        }

        instance->encoder.repeat =
            (int32_t)rf_stream_encoder_read_repeat(flipper_format, KIA_V3_V4_DEFAULT_REPEAT);

        instance->crc_iter = 0;
        instance->bursts_sent = 0;

        kia_protocol_encoder_v3_v4_get_upload(instance);

        instance->encoder.is_running = true;
        instance->encoder.front = 0;

        RF_STREAM_LOG_I(
            TAG,
            "Encoder initialized: Serial=0x%07lX, Btn=0x%X, Cnt=0x%04X, Version=%s",
            (unsigned long)instance->serial,
            instance->btn,
            instance->cnt,
            instance->version == 0 ? "V4" : "V3");

        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

#endif
#if RF_STREAM_WITH_ENCODER

void kia_protocol_encoder_v3_v4_stop(void* context) {
    if(!context) return;
    RfStreamProtocolEncoderKiaV3V4* instance = context;
    instance->encoder.is_running = false;
    instance->encoder.front = 0;
}

#endif
#if RF_STREAM_WITH_ENCODER

LevelDuration kia_protocol_encoder_v3_v4_yield(void* context) {
    RfStreamProtocolEncoderKiaV3V4* instance = context;

    if(!instance || !instance->encoder.upload || instance->encoder.repeat == 0 ||
       !instance->encoder.is_running) {
        if(instance) {
            RF_STREAM_LOG_D(
                TAG,
                "Encoder yield stopped: repeat=%u, is_running=%d",
                instance->encoder.repeat,
                instance->encoder.is_running);
            instance->encoder.is_running = false;
        }
        return level_duration_reset();
    }

    if(instance->encoder.front >= instance->encoder.size_upload) {
        RF_STREAM_LOG_E(
            TAG,
            "Encoder front out of bounds: %zu >= %zu",
            instance->encoder.front,
            instance->encoder.size_upload);
        instance->encoder.is_running = false;
        instance->encoder.front = 0;
        return level_duration_reset();
    }

    LevelDuration ret = instance->encoder.upload[instance->encoder.front];

    if(++instance->encoder.front == instance->encoder.size_upload) {
        instance->crc_iter = (uint8_t)((instance->crc_iter + 1U) & 0x0FU);
        kia_protocol_encoder_v3_v4_patch_crc(instance);
        instance->encoder.front = 0;
        instance->encoder.repeat--;
        if(instance->bursts_sent < KIA_V3_V4_CRC_SWEEP_COUNT) {
            instance->bursts_sent++;
        }
        if(instance->encoder.repeat == 0) {
            RF_STREAM_LOG_I(
                TAG,
                "CRC BF: %u/%u bursts transmitted (~%u ms)",
                instance->bursts_sent,
                KIA_V3_V4_CRC_SWEEP_COUNT,
                (unsigned)(instance->bursts_sent * 94U));
        }
    }

    return ret;
}

#endif
#if RF_STREAM_WITH_ENCODER

void kia_protocol_encoder_v3_v4_set_button(void* context, uint8_t button) {
    RfStreamProtocolEncoderKiaV3V4* instance = context;
    instance->btn = button & 0x0F;
    instance->generic.btn = instance->btn;
    kia_protocol_encoder_v3_v4_get_upload(instance);
    RF_STREAM_LOG_I(TAG, "Button set to 0x%X", instance->btn);
}

#endif
#if RF_STREAM_WITH_ENCODER

void kia_protocol_encoder_v3_v4_set_counter(void* context, uint16_t counter) {
    RfStreamProtocolEncoderKiaV3V4* instance = context;
    instance->cnt = counter;
    instance->generic.cnt = instance->cnt;
    kia_protocol_encoder_v3_v4_get_upload(instance);
    RF_STREAM_LOG_I(TAG, "Counter set to 0x%04X", instance->cnt);
}

#endif
#if RF_STREAM_WITH_ENCODER

void kia_protocol_encoder_v3_v4_increment_counter(void* context) {
    RfStreamProtocolEncoderKiaV3V4* instance = context;
    instance->cnt++;
    instance->generic.cnt = instance->cnt;
    kia_protocol_encoder_v3_v4_get_upload(instance);
    RF_STREAM_LOG_I(TAG, "Counter incremented to 0x%04X", instance->cnt);
}

#endif
#if RF_STREAM_WITH_ENCODER

uint16_t kia_protocol_encoder_v3_v4_get_counter(void* context) {
    RfStreamProtocolEncoderKiaV3V4* instance = context;
    return instance->cnt;
}

#endif
#if RF_STREAM_WITH_ENCODER

uint8_t kia_protocol_encoder_v3_v4_get_button(void* context) {
    RfStreamProtocolEncoderKiaV3V4* instance = context;
    return instance->btn;
}

#endif

// ============================================================================
// DECODER IMPLEMENTATION
// ============================================================================

void* kia_protocol_decoder_v3_v4_alloc(RfStreamEnvironment* environment) {
    RfStreamProtocolDecoderKiaV3V4* instance = malloc(sizeof(RfStreamProtocolDecoderKiaV3V4));

    if(environment) {
        rf_vehicle_keys_load(environment);
    }

    instance->base.protocol = &kia_protocol_v3_v4;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void kia_protocol_decoder_v3_v4_reset(void* context) {
    RfStreamProtocolDecoderKiaV3V4* instance = context;
    instance->decoder.parser_step = KiaV3V4DecoderStepReset;
    instance->header_count = 0;
    instance->raw_bit_count = 0;
    instance->crc = 0;
    memset(instance->raw_bits, 0, sizeof(instance->raw_bits));
}

void kia_protocol_decoder_v3_v4_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderKiaV3V4* instance = context;

    switch(instance->decoder.parser_step) {
    case KiaV3V4DecoderStepReset:
        if(level && (DURATION_DIFF(duration, kia_protocol_v3_v4_const.te_short) <
                     kia_protocol_v3_v4_const.te_delta)) {
            instance->decoder.parser_step = KiaV3V4DecoderStepCheckPreamble;
            instance->decoder.te_last = duration;
            instance->header_count = 1;
        }
        break;

    case KiaV3V4DecoderStepCheckPreamble:
        if(level) {
            if(DURATION_DIFF(duration, kia_protocol_v3_v4_const.te_short) <
               kia_protocol_v3_v4_const.te_delta) {
                instance->decoder.te_last = duration;
            } else if(duration > 1000 && duration < 1500) {
                if(instance->header_count >= 8) {
                    instance->decoder.parser_step = KiaV3V4DecoderStepCollectRawBits;
                    instance->raw_bit_count = 0;
                    instance->is_v3_sync = false;
                    memset(instance->raw_bits, 0, sizeof(instance->raw_bits));
                } else {
                    instance->decoder.parser_step = KiaV3V4DecoderStepReset;
                }
            } else {
                instance->decoder.parser_step = KiaV3V4DecoderStepReset;
            }
        } else {
            if(duration > 1000 && duration < 1500) {
                if(instance->header_count >= 8) {
                    instance->decoder.parser_step = KiaV3V4DecoderStepCollectRawBits;
                    instance->raw_bit_count = 0;
                    instance->is_v3_sync = true;
                    memset(instance->raw_bits, 0, sizeof(instance->raw_bits));
                } else {
                    instance->decoder.parser_step = KiaV3V4DecoderStepReset;
                }
            } else if(
                (DURATION_DIFF(duration, kia_protocol_v3_v4_const.te_short) <
                 kia_protocol_v3_v4_const.te_delta) &&
                (DURATION_DIFF(instance->decoder.te_last, kia_protocol_v3_v4_const.te_short) <
                 kia_protocol_v3_v4_const.te_delta)) {
                instance->header_count++;
            } else if(duration > 1500) {
                instance->decoder.parser_step = KiaV3V4DecoderStepReset;
            }
        }
        break;

    case KiaV3V4DecoderStepCollectRawBits:
        if(level) {
            if(duration > 1000 && duration < 1500) {
                if(kia_v3_v4_process_buffer(instance)) {
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.parser_step = KiaV3V4DecoderStepReset;
            } else if(
                DURATION_DIFF(duration, kia_protocol_v3_v4_const.te_short) <
                kia_protocol_v3_v4_const.te_delta) {
                kia_v3_v4_add_raw_bit(instance, false);
            } else if(
                DURATION_DIFF(duration, kia_protocol_v3_v4_const.te_long) <
                kia_protocol_v3_v4_const.te_delta) {
                kia_v3_v4_add_raw_bit(instance, true);
            } else {
                instance->decoder.parser_step = KiaV3V4DecoderStepReset;
            }
        } else {
            if(duration > 1000 && duration < 1500) {
                if(kia_v3_v4_process_buffer(instance)) {
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.parser_step = KiaV3V4DecoderStepReset;
            } else if(duration > 1500) {
                if(kia_v3_v4_process_buffer(instance)) {
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.parser_step = KiaV3V4DecoderStepReset;
            }
        }
        break;
    }
}

RfStreamProtocolStatus kia_protocol_decoder_v3_v4_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderKiaV3V4* instance = context;

    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    do {
        // Write frequency
        if(!rf_record_write_uint32(flipper_format, FF_FREQUENCY, &preset->frequency, 1)) {
            break;
        }

        // Write preset
        if(!rf_record_write_string_cstr(
               flipper_format, FF_PRESET, rf_text_get_cstr(preset->name))) {
            break;
        }

        // Write version-specific protocol name instead of generic "Kia V3/V4"
        const char* version_name = (instance->version == 0) ? "Kia V4" : "Kia V3";
        if(!rf_record_write_string_cstr(flipper_format, FF_PROTOCOL, version_name)) {
            break;
        }

        // Write bit count
        uint32_t bits = instance->generic.data_count_bit;
        if(!rf_record_write_uint32(flipper_format, FF_BIT, &bits, 1)) {
            break;
        }

        // Write key
        char key_str[20];
        snprintf(key_str, sizeof(key_str), "%016llX", (unsigned long long)instance->generic.data);
        if(!rf_record_write_string_cstr(flipper_format, FF_KEY, key_str)) {
            break;
        }

        // Write all fields needed by encoder
        if(rf_stream_serialize_fields(
               flipper_format,
               RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT,
               instance->generic.serial,
               instance->generic.btn,
               instance->generic.cnt,
               0) != RfStreamProtocolStatusOk) {
            break;
        }

        // Write protocol-specific fields
        if(!rf_record_write_uint32(flipper_format, "Encrypted", &instance->encrypted, 1)) {
            break;
        }

        if(!rf_record_write_uint32(flipper_format, "Decrypted", &instance->decrypted, 1)) {
            break;
        }

        uint32_t temp = instance->version;
        if(!rf_record_write_uint32(flipper_format, "KIAVersion", &temp, 1)) {
            break;
        }

        temp = instance->crc;
        if(!rf_record_write_uint32(flipper_format, "CRC", &temp, 1)) {
            break;
        }

        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

RfStreamProtocolStatus
    kia_protocol_decoder_v3_v4_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderKiaV3V4* instance = context;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_deserialize_check_count_bit(&instance->generic, flipper_format, 64);

    if(ret == RfStreamProtocolStatusOk) {
        uint32_t temp = 0;

        if(rf_record_read_uint32(flipper_format, "Encrypted", &temp, 1)) {
            instance->encrypted = temp;
        }
        if(rf_record_read_uint32(flipper_format, "Decrypted", &temp, 1)) {
            instance->decrypted = temp;
        }
        if(rf_record_read_uint32(flipper_format, "KIAVersion", &temp, 1)) {
            instance->version = (uint8_t)temp;
        }
        if(rf_record_read_uint32(flipper_format, "CRC", &temp, 1)) {
            instance->crc = (uint8_t)temp;
        }
    }

    return ret;
}

static uint64_t compute_yek(uint64_t key) {
    uint64_t yek = 0;
    for(int i = 0; i < 64; i++) {
        yek |= ((key >> i) & 1) << (63 - i);
    }
    return yek;
}

void kia_protocol_decoder_v3_v4_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderKiaV3V4* instance = context;

    uint64_t yek = compute_yek(instance->generic.data);
    uint32_t key_hi = (uint32_t)(instance->generic.data >> 32);
    uint32_t key_lo = (uint32_t)(instance->generic.data & 0xFFFFFFFF);
    uint32_t yek_hi = (uint32_t)(yek >> 32);
    uint32_t yek_lo = (uint32_t)(yek & 0xFFFFFFFF);

    rf_text_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%08lX%08lX\r\n"
        "Yek:%08lX%08lX\r\n"
        "Serial:%07lX Btn:%01X\r\n"
        "Cnt:%04lX CRC:%01X\r\n",
        kia_version_names[instance->version],
        instance->generic.data_count_bit,
        key_hi,
        key_lo,
        yek_hi,
        yek_lo,
        instance->generic.serial,
        instance->generic.btn,
        instance->generic.cnt,
        instance->crc);
}

#endif
