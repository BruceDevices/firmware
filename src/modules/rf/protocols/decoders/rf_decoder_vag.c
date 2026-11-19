// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#include "rf_vehicle_support.h"
#define VAG_PROTOCOL_NAME "VAG"

extern const RfStreamCodec vag_protocol;

// Decoder functions
void* rf_stream_protocol_decoder_vag_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_vag_free(void* context);
void rf_stream_protocol_decoder_vag_reset(void* context);
void rf_stream_protocol_decoder_vag_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_vag_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_vag_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_vag_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_vag_get_string(void* context, RfText* output);

// Encoder functions
void* rf_stream_protocol_encoder_vag_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_encoder_vag_free(void* context);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_vag_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_encoder_vag_stop(void* context);
LevelDuration rf_stream_protocol_encoder_vag_yield(void* context);





#include <string.h>

#ifdef ENABLE_MODELS_DATABASE
#define PROTOPIRATE_VAG_KEYS_FILE_NAME APP_ASSETS_PATH("keystore/vag")
#else
#define PROTOPIRATE_VAG_KEYS_FILE_NAME APP_ASSETS_PATH("vag")
#endif

#define TAG "VAGProtocol"

#define VAG_ENCODER_UPLOAD_TYPE12_SIZE 635
#define VAG_ENCODER_UPLOAD_TYPE34_SIZE 518
#define VAG_ENCODER_UPLOAD_MAX_SIZE    VAG_ENCODER_UPLOAD_TYPE12_SIZE
_Static_assert(
    VAG_ENCODER_UPLOAD_MAX_SIZE <= RF_SHARED_UPLOAD_CAPACITY,
    "VAG_ENCODER_UPLOAD_MAX_SIZE exceeds shared upload slab");

static const RfStreamBlockConst rf_stream_protocol_vag_const = {
    .te_short = 500,
    .te_long = 1000,
    .te_delta = 80,
    .min_count_bit_for_found = 80,
};

#define VAG_T12_TE_SHORT     300u
#define VAG_T12_TE_LONG      600u
#define VAG_T12_TE_DELTA     100u
#define VAG_T12_GAP_DELTA    200u
#define VAG_T12_PREAMBLE_MIN 151u

#define VAG_T34_TE_SHORT     500u
#define VAG_T34_TE_LONG      1000u
#define VAG_T34_TE_DELTA     100u
#define VAG_T34_LONG_DELTA   200u
#define VAG_T34_SYNC         750u
#define VAG_T34_SYNC_DELTA   150u
#define VAG_T34_PREAMBLE_MIN 31u
#define VAG_T34_SYNC_PAIRS   3u

#define VAG_DATA_GAP_MIN    4001u
#define VAG_TOTAL_BITS      80u
#define VAG_KEY1_BITS       64u
#define VAG_PREFIX_BITS     15u
#define VAG_BIT_LIMIT       96u
#define VAG_FRAME_PREFIX_T1 0x2F3Fu
#define VAG_FRAME_PREFIX_T2 0x2F1Cu

#define VAG_KEYS_COUNT 3

static int8_t protocol_vag_keys_loaded = -1;
static struct aut64_key protocol_vag_keys[VAG_KEYS_COUNT];

static void protocol_vag_load_keys(const char* file_name) {
    if(protocol_vag_keys_loaded >= 0) {
        RF_STREAM_LOG_I(
            TAG,
            "Already loaded %u keys from %s, skipping load",
            protocol_vag_keys_loaded,
            file_name);
        return;
    }

    RF_STREAM_LOG_I(TAG, "Loading keys from %s", file_name);

    protocol_vag_keys_loaded = 0;

    for(uint8_t i = 0; i < VAG_KEYS_COUNT; i++) {
        uint8_t key_packed[AUT64_PACKED_KEY_SIZE];

        if(rf_stream_keystore_raw_get_data(
               file_name, i * AUT64_PACKED_KEY_SIZE, key_packed, AUT64_PACKED_KEY_SIZE)) {
            int rc = aut64_unpack(&protocol_vag_keys[i], key_packed);
#ifdef AUT64_ENABLE_VALIDATIONS
            if(rc == AUT64_ERR_INVALID_PACKED) {
                RF_STREAM_LOG_E(TAG, "Invalid key: %u", i);
            } else if(rc == AUT64_ERR_NULL_POINTER) {
                RF_STREAM_LOG_E(TAG, "Key is NULL: %d", i);
            }
            if(rc == AUT64_OK) {
                protocol_vag_keys_loaded++;
            } else {
                break;
            }
#else
            (void)rc;
            protocol_vag_keys_loaded++;
#endif
        } else {
            RF_STREAM_LOG_E(TAG, "Unable to load key %u", i);
            break;
        }
    }

    RF_STREAM_LOG_I(TAG, "Loaded %u keys", protocol_vag_keys_loaded);
}

static struct aut64_key* protocol_vag_get_key(uint8_t index) {
    for(uint8_t i = 0; i < MIN(protocol_vag_keys_loaded, VAG_KEYS_COUNT); i++) {
        if(protocol_vag_keys[i].index == index) {
            return &protocol_vag_keys[i];
        }
    }

    return NULL;
}

#define VAG_TEA_DELTA  0x9E3779B9U
#define VAG_TEA_ROUNDS 32

static const uint32_t vag_tea_key_schedule[] = {0x0B46502D, 0x5E253718, 0x2BF93A19, 0x622C1206};

static const char* vag_button_name(uint8_t btn) {
    switch(btn) {
    case 0x1:
        return "Unlock";
    case 0x2:
        return "Lock";
    case 0x4:
        return "Boot";
    case 0x10:
        return "Unlock";
    case 0x20:
        return "Lock";
    case 0x40:
        return "Boot";
    default:
        return "Unkn";
    }
}

typedef struct RfStreamProtocolDecoderVAG {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    uint32_t data_low;
    uint32_t data_high;
    uint8_t bit_count;
    uint32_t key1_low;
    uint32_t key1_high;
    uint32_t key2_low;
    uint32_t key2_high;
    uint16_t data_count_bit;
    uint8_t vag_type;
    uint16_t header_count;
    uint8_t mid_count;
    RfManchesterState manchester_state;

    uint32_t serial;
    uint32_t cnt;
    uint8_t btn;
    uint8_t check_byte;
    uint8_t key_idx;
    bool decrypted;
} RfStreamProtocolDecoderVAG;

typedef enum {
    VAGDecoderStepReset = 0,
    VAGDecoderStepPreamble1 = 1,
    VAGDecoderStepData1 = 2,
    VAGDecoderStepPreamble2 = 3,
    VAGDecoderStepSync2A = 4,
    VAGDecoderStepSync2B = 5,
    VAGDecoderStepSync2C = 6,
    VAGDecoderStepData2 = 7,
} VAGDecoderStep;

static void vag_tea_decrypt(uint32_t* v0, uint32_t* v1, const uint32_t* key_schedule) {
    uint32_t sum = VAG_TEA_DELTA * VAG_TEA_ROUNDS;
    for(int i = 0; i < VAG_TEA_ROUNDS; i++) {
        *v1 -= (((*v0 << 4) ^ (*v0 >> 5)) + *v0) ^ (sum + key_schedule[(sum >> 11) & 3]);
        sum -= VAG_TEA_DELTA;
        *v0 -= (((*v1 << 4) ^ (*v1 >> 5)) + *v1) ^ (sum + key_schedule[sum & 3]);
    }
}
#if RF_STREAM_WITH_ENCODER
static void vag_tea_encrypt(uint32_t* v0, uint32_t* v1, const uint32_t* key_schedule) {
    uint32_t sum = 0;
    for(int i = 0; i < VAG_TEA_ROUNDS; i++) {
        *v0 += (((*v1 << 4) ^ (*v1 >> 5)) + *v1) ^ (sum + key_schedule[sum & 3]);
        sum += VAG_TEA_DELTA;
        *v1 += (((*v0 << 4) ^ (*v0 >> 5)) + *v0) ^ (sum + key_schedule[(sum >> 11) & 3]);
    }
}
#endif
static bool vag_dispatch_type_1_2(uint8_t dispatch) {
    return (dispatch == 0x2A || dispatch == 0x1C || dispatch == 0x46);
}

static bool vag_dispatch_type_3_4(uint8_t dispatch) {
    return (dispatch == 0x2B || dispatch == 0x1D || dispatch == 0x47);
}

static bool vag_button_valid(const uint8_t* dec) {
    uint8_t dec_byte = dec[7];
    uint8_t dec_btn = (dec_byte >> 4) & 0xF;

    if(dec_btn == 1 || dec_btn == 2 || dec_btn == 4) {
        return true;
    }
    if(dec_byte == 0) {
        return true;
    }
    return false;
}

static bool vag_button_matches(const uint8_t* dec, uint8_t dispatch_byte) {
    uint8_t expected_btn = (dispatch_byte >> 4) & 0xF;
    uint8_t dec_btn = (dec[7] >> 4) & 0xF;

    if(dec_btn == expected_btn) {
        return true;
    }
    if(dec[7] == 0 && expected_btn == 2) {
        return true;
    }
    return false;
}

static void vag_fill_from_decrypted(
    RfStreamProtocolDecoderVAG* instance,
    const uint8_t* dec,
    uint8_t dispatch_byte) {
    uint32_t serial_raw = (uint32_t)dec[0] | ((uint32_t)dec[1] << 8) | ((uint32_t)dec[2] << 16) |
                          ((uint32_t)dec[3] << 24);
    instance->serial = (serial_raw << 24) | ((serial_raw & 0xFF00) << 8) |
                       ((serial_raw >> 8) & 0xFF00) | (serial_raw >> 24);

    instance->cnt = (uint32_t)dec[4] | ((uint32_t)dec[5] << 8) | ((uint32_t)dec[6] << 16);

    instance->btn = (dec[7] >> 4) & 0xF;
    instance->check_byte = dispatch_byte;
    instance->decrypted = true;
}

static bool vag_aut64_decrypt(uint8_t* block, int key_index) {
    struct aut64_key* key = protocol_vag_get_key(key_index + 1);
    if(!key) {
        RF_STREAM_LOG_E(TAG, "Key not found: %d", key_index + 1);
        return false;
    }
    int rc = aut64_decrypt(key, block);
#ifdef AUT64_ENABLE_VALIDATIONS
    if(rc == AUT64_ERR_INVALID_KEY) {
        RF_STREAM_LOG_E(TAG, "Invalid key: %d", key_index + 1);
    } else if(rc == AUT64_ERR_NULL_POINTER) {
        RF_STREAM_LOG_E(TAG, "key is NULL: %d", key_index + 1);
    }
#endif

    return (rc == AUT64_OK) ? true : false;
}

static void vag_parse_data(RfStreamProtocolDecoderVAG* instance) {
    instance->decrypted = false;
    instance->serial = 0;
    instance->cnt = 0;
    instance->btn = 0;

    uint8_t dispatch_byte = (uint8_t)(instance->key2_low & 0xFF);
    uint8_t key2_high = (uint8_t)((instance->key2_low >> 8) & 0xFF);

    RF_STREAM_LOG_I(
        TAG,
        "Parsing VAG type=%d dispatch=0x%02X expected_btn=%d",
        instance->vag_type,
        dispatch_byte,
        (dispatch_byte >> 4) & 0xF);

    uint8_t key1_bytes[8];
    uint32_t key1_low = instance->key1_low;
    uint32_t key1_high = instance->key1_high;

    key1_bytes[0] = (uint8_t)(key1_high >> 24);
    key1_bytes[1] = (uint8_t)(key1_high >> 16);
    key1_bytes[2] = (uint8_t)(key1_high >> 8);
    key1_bytes[3] = (uint8_t)(key1_high);
    key1_bytes[4] = (uint8_t)(key1_low >> 24);
    key1_bytes[5] = (uint8_t)(key1_low >> 16);
    key1_bytes[6] = (uint8_t)(key1_low >> 8);
    key1_bytes[7] = (uint8_t)(key1_low);

#ifndef REMOVE_LOGS
    uint8_t type_byte = key1_bytes[0];
#endif
    uint8_t block[8];
    block[0] = key1_bytes[1];
    block[1] = key1_bytes[2];
    block[2] = key1_bytes[3];
    block[3] = key1_bytes[4];
    block[4] = key1_bytes[5];
    block[5] = key1_bytes[6];
    block[6] = key1_bytes[7];
    block[7] = key2_high;

    RF_STREAM_LOG_D(
        TAG,
        "Type byte: 0x%02X, Encrypted block: %02X %02X %02X %02X %02X %02X %02X %02X",
        type_byte,
        block[0],
        block[1],
        block[2],
        block[3],
        block[4],
        block[5],
        block[6],
        block[7]);

    switch(instance->vag_type) {
    case 1:
        if(!vag_dispatch_type_1_2(dispatch_byte)) {
            RF_STREAM_LOG_W(TAG, "Type 1: dispatch mismatch 0x%02X", dispatch_byte);
            break;
        }
        {
            uint8_t block_copy[8];

            for(int key_idx = 0; key_idx < 3; key_idx++) {
                memcpy(block_copy, block, 8);
                if(!vag_aut64_decrypt(block_copy, key_idx)) {
                    continue;
                }

                if(vag_button_valid(block_copy)) {
                    instance->serial = ((uint32_t)block_copy[0] << 24) |
                                       ((uint32_t)block_copy[1] << 16) |
                                       ((uint32_t)block_copy[2] << 8) | (uint32_t)block_copy[3];
                    instance->cnt = (uint32_t)block_copy[4] | ((uint32_t)block_copy[5] << 8) |
                                    ((uint32_t)block_copy[6] << 16);
                    instance->btn = block_copy[7];
                    instance->check_byte = dispatch_byte;
                    instance->key_idx = key_idx;
                    instance->decrypted = true;
                    RF_STREAM_LOG_I(
                        TAG,
                        "Type 1 key%d decoded: Ser=%08lX Cnt=%06lX Btn=%02X",
                        key_idx,
                        (unsigned long)instance->serial,
                        (unsigned long)instance->cnt,
                        instance->btn);
                    return;
                }
            }
            RF_STREAM_LOG_W(
                TAG,
                "Type 1: all keys failed, dec[7]=0x%02X dispatch=0x%02X",
                block_copy[7],
                dispatch_byte);
        }
        break;

    case 2:
        if(!vag_dispatch_type_1_2(dispatch_byte)) {
            RF_STREAM_LOG_W(TAG, "Type 2: dispatch mismatch 0x%02X", dispatch_byte);
            break;
        }
        {
            uint32_t v0_orig = ((uint32_t)block[0] << 24) | ((uint32_t)block[1] << 16) |
                               ((uint32_t)block[2] << 8) | (uint32_t)block[3];
            uint32_t v1_orig = ((uint32_t)block[4] << 24) | ((uint32_t)block[5] << 16) |
                               ((uint32_t)block[6] << 8) | (uint32_t)block[7];

            {
                uint32_t v0 = v0_orig;
                uint32_t v1 = v1_orig;

                vag_tea_decrypt(&v0, &v1, vag_tea_key_schedule);

                uint8_t tea_dec[8];
                tea_dec[0] = (uint8_t)(v0 >> 24);
                tea_dec[1] = (uint8_t)(v0 >> 16);
                tea_dec[2] = (uint8_t)(v0 >> 8);
                tea_dec[3] = (uint8_t)(v0);
                tea_dec[4] = (uint8_t)(v1 >> 24);
                tea_dec[5] = (uint8_t)(v1 >> 16);
                tea_dec[6] = (uint8_t)(v1 >> 8);
                tea_dec[7] = (uint8_t)(v1);

                if(!vag_button_matches(tea_dec, dispatch_byte)) {
                    RF_STREAM_LOG_W(
                        TAG,
                        "Type 2: XTEA button mismatch dec[7]=0x%02X dispatch=0x%02X",
                        tea_dec[7],
                        dispatch_byte);
                    break;
                }

                vag_fill_from_decrypted(instance, tea_dec, dispatch_byte);
                instance->key_idx = 0xFF;

                RF_STREAM_LOG_I(
                    TAG,
                    "Type 2 XTEA decoded: Ser=%08lX Cnt=%06lX Btn=%d",
                    (unsigned long)instance->serial,
                    (unsigned long)instance->cnt,
                    instance->btn);
                return;
            }
        }
        break;

    case 3: {
        uint8_t block_copy[8];

        memcpy(block_copy, block, 8);
        if(vag_aut64_decrypt(block_copy, 2) && vag_button_valid(block_copy)) {
            instance->vag_type = 4;
            instance->key_idx = 2;
            vag_fill_from_decrypted(instance, block_copy, dispatch_byte);
            RF_STREAM_LOG_I(
                TAG,
                "Type 3->4 key2 decoded: Ser=%08lX Cnt=%06lX Btn=%d",
                (unsigned long)instance->serial,
                (unsigned long)instance->cnt,
                instance->btn);
            return;
        }

        memcpy(block_copy, block, 8);
        if(vag_aut64_decrypt(block_copy, 1) && vag_button_valid(block_copy)) {
            instance->key_idx = 1;
            vag_fill_from_decrypted(instance, block_copy, dispatch_byte);
            RF_STREAM_LOG_I(
                TAG,
                "Type 3 key1 decoded: Ser=%08lX Cnt=%06lX Btn=%d",
                (unsigned long)instance->serial,
                (unsigned long)instance->cnt,
                instance->btn);
            return;
        }

        memcpy(block_copy, block, 8);
        if(vag_aut64_decrypt(block_copy, 0) && vag_button_valid(block_copy)) {
            instance->key_idx = 0;
            vag_fill_from_decrypted(instance, block_copy, dispatch_byte);
            RF_STREAM_LOG_I(
                TAG,
                "Type 3 key0 decoded: Ser=%08lX Cnt=%06lX Btn=%d",
                (unsigned long)instance->serial,
                (unsigned long)instance->cnt,
                instance->btn);
            return;
        }

        RF_STREAM_LOG_W(
            TAG,
            "Type 3: all keys failed, dec[7]=0x%02X dispatch=0x%02X",
            block_copy[7],
            dispatch_byte);
    } break;

    case 4:
        if(!vag_dispatch_type_3_4(dispatch_byte)) {
            RF_STREAM_LOG_W(TAG, "Type 4: dispatch mismatch 0x%02X", dispatch_byte);
            break;
        }
        {
            uint8_t block_copy[8];
            memcpy(block_copy, block, 8);

            if(!vag_aut64_decrypt(block_copy, 2)) {
                RF_STREAM_LOG_E(TAG, "Type 4: key 2 not loaded");
                break;
            }
            if(!vag_button_matches(block_copy, dispatch_byte)) {
                RF_STREAM_LOG_W(
                    TAG,
                    "Type 4: button mismatch dec[7]=0x%02X dispatch=0x%02X",
                    block_copy[7],
                    dispatch_byte);
                break;
            }
            instance->key_idx = 2;
            vag_fill_from_decrypted(instance, block_copy, dispatch_byte);
            RF_STREAM_LOG_I(
                TAG,
                "Type 4 decoded: Ser=%08lX Cnt=%06lX Btn=%d",
                (unsigned long)instance->serial,
                (unsigned long)instance->cnt,
                instance->btn);
        }
        return;

    default:
        RF_STREAM_LOG_W(TAG, "Unknown VAG type %d", instance->vag_type);
        break;
    }

    instance->decrypted = false;
    instance->serial = 0;
    instance->cnt = 0;
    instance->btn = 0;
    instance->check_byte = 0;
    RF_STREAM_LOG_W(TAG, "VAG decryption failed for type %d", instance->vag_type);
}

const RfStreamProtocolDecoder rf_stream_protocol_vag_decoder = {
    .alloc = rf_stream_protocol_decoder_vag_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_vag_feed,
    .reset = rf_stream_protocol_decoder_vag_reset,
    .get_hash_data = rf_stream_protocol_decoder_vag_get_hash_data,
    .serialize = rf_stream_protocol_decoder_vag_serialize,
    .deserialize = rf_stream_protocol_decoder_vag_deserialize,
    .get_string = rf_stream_protocol_decoder_vag_get_string,
};
#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_vag_encoder = {
    .alloc = rf_stream_protocol_encoder_vag_alloc,
    .free = rf_stream_protocol_encoder_vag_free,
    .deserialize = rf_stream_protocol_encoder_vag_deserialize,
    .stop = rf_stream_protocol_encoder_vag_stop,
    .yield = rf_stream_protocol_encoder_vag_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_vag_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec vag_protocol = {
    .name = VAG_PROTOCOL_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_vag_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_vag_encoder,
#else
    .encoder = NULL,
#endif
};

void* rf_stream_protocol_decoder_vag_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderVAG* instance = calloc(1, sizeof(RfStreamProtocolDecoderVAG));
    instance->base.protocol = &vag_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->decrypted = false;
    instance->key_idx = 0xFF;

    protocol_vag_load_keys(PROTOPIRATE_VAG_KEYS_FILE_NAME);

    return instance;
}

void rf_stream_protocol_decoder_vag_reset(void* context) {
    RfStreamProtocolDecoderVAG* instance = context;
    instance->decoder.parser_step = VAGDecoderStepReset;
    instance->decrypted = false;

    /* Reset decoder state to avoid stale parsing after external resets */
    instance->data_low = 0;
    instance->data_high = 0;
    instance->bit_count = 0;
    instance->data_count_bit = 0;
    instance->vag_type = 0;
    instance->header_count = 0;
    instance->mid_count = 0;
    rf_manchester_advance(
        instance->manchester_state, RfManchesterEventReset, &instance->manchester_state, NULL);

    instance->serial = 0;
    instance->cnt = 0;
    instance->btn = 0;
    instance->check_byte = 0;
    instance->key_idx = 0xFF;
}

void rf_stream_protocol_decoder_vag_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderVAG* instance = context;

    switch(instance->decoder.parser_step) {
    case VAGDecoderStepReset:
        if(!level) break;
        if(DURATION_DIFF(duration, VAG_T12_TE_SHORT) < VAG_T12_TE_DELTA) {
            instance->decoder.parser_step = VAGDecoderStepPreamble1;
        } else if(DURATION_DIFF(duration, VAG_T34_TE_SHORT) < VAG_T34_TE_DELTA) {
            instance->decoder.parser_step = VAGDecoderStepPreamble2;
        } else {
            break;
        }
        instance->data_low = 0;
        instance->data_high = 0;
        instance->header_count = 0;
        instance->mid_count = 0;
        instance->bit_count = 0;
        instance->vag_type = 0;
        instance->decoder.te_last = duration;
        rf_manchester_advance(
            instance->manchester_state, RfManchesterEventReset, &instance->manchester_state, NULL);
        break;

    case VAGDecoderStepPreamble1:
        if(level) break;
        if(DURATION_DIFF(duration, VAG_T12_TE_SHORT) < VAG_T12_TE_DELTA) {
            if(DURATION_DIFF(instance->decoder.te_last, VAG_T12_TE_SHORT) < VAG_T12_TE_DELTA) {
                instance->decoder.te_last = duration;
                instance->header_count++;
            } else {
                instance->decoder.parser_step = VAGDecoderStepReset;
            }
            break;
        }
        instance->decoder.parser_step = VAGDecoderStepReset;
        if(instance->header_count < VAG_T12_PREAMBLE_MIN) break;
        if(DURATION_DIFF(duration, VAG_T12_TE_LONG) >= VAG_T12_GAP_DELTA) break;
        if(DURATION_DIFF(instance->decoder.te_last, VAG_T12_TE_SHORT) >= VAG_T12_TE_DELTA) break;
        instance->decoder.parser_step = VAGDecoderStepData1;
        break;

    case VAGDecoderStepData1: {
        if(instance->bit_count >= VAG_BIT_LIMIT) {
            instance->decoder.parser_step = VAGDecoderStepReset;
            break;
        }

        bool bit_value = false;
        RfManchesterEvent event = RfManchesterEventReset;
        bool got_pulse = false;

        if(DURATION_DIFF(duration, VAG_T12_TE_SHORT) < VAG_T12_TE_DELTA) {
            event = level ? RfManchesterEventShortLow : RfManchesterEventShortHigh;
            got_pulse = true;
        } else if(
            duration > VAG_T12_TE_SHORT + VAG_T12_TE_DELTA &&
            DURATION_DIFF(duration, VAG_T12_TE_LONG) < VAG_T12_GAP_DELTA) {
            event = level ? RfManchesterEventLongLow : RfManchesterEventLongHigh;
            got_pulse = true;
        }

        if(got_pulse && instance->bit_count < VAG_BIT_LIMIT) {
            if(rf_manchester_advance(
                   instance->manchester_state, event, &instance->manchester_state, &bit_value)) {
                uint32_t carry = (instance->data_low >> 31) & 1;
                instance->data_low = (instance->data_low << 1) | (bit_value ? 1 : 0);
                instance->data_high = (instance->data_high << 1) | carry;
                instance->bit_count++;

                if(instance->bit_count == VAG_PREFIX_BITS) {
                    if(instance->data_low == VAG_FRAME_PREFIX_T1 && instance->data_high == 0) {
                        instance->data_low = 0;
                        instance->data_high = 0;
                        instance->bit_count = 0;
                        instance->vag_type = 1;
                    } else if(instance->data_low == VAG_FRAME_PREFIX_T2 && instance->data_high == 0) {
                        instance->data_low = 0;
                        instance->data_high = 0;
                        instance->bit_count = 0;
                        instance->vag_type = 2;
                    }
                } else if(instance->bit_count == VAG_KEY1_BITS) {
                    instance->key1_low = ~instance->data_low;
                    instance->key1_high = ~instance->data_high;
                    instance->data_low = 0;
                    instance->data_high = 0;
                }
            }
            break;
        }

        if(level) break;
        if(duration < VAG_DATA_GAP_MIN) break;
        if(instance->bit_count == VAG_TOTAL_BITS) {
            instance->key2_low = (~instance->data_low) & 0xFFFF;
            instance->key2_high = 0;
            instance->data_count_bit = VAG_TOTAL_BITS;
            RF_STREAM_LOG_I(
                TAG,
                "VAG decoded: Key1:%08lX%08lX Key2:%04X Type:%d",
                (unsigned long)instance->key1_high,
                (unsigned long)instance->key1_low,
                (unsigned int)(instance->key2_low & 0xFFFF),
                instance->vag_type);
            vag_parse_data(instance);
            if(instance->base.callback) {
                instance->base.callback(&instance->base, instance->base.context);
            }
        }
        instance->data_low = 0;
        instance->data_high = 0;
        instance->bit_count = 0;
        instance->decoder.parser_step = VAGDecoderStepReset;
        break;
    }

    case VAGDecoderStepPreamble2:
        if(!level) {
            if(DURATION_DIFF(duration, VAG_T34_TE_SHORT) < VAG_T34_TE_DELTA &&
               DURATION_DIFF(instance->decoder.te_last, VAG_T34_TE_SHORT) < VAG_T34_TE_DELTA) {
                instance->decoder.te_last = duration;
                instance->header_count++;
            } else {
                instance->decoder.parser_step = VAGDecoderStepReset;
            }
            break;
        }
        if(instance->header_count < VAG_T34_PREAMBLE_MIN) break;
        if(DURATION_DIFF(duration, VAG_T34_TE_LONG) >= VAG_T34_LONG_DELTA) break;
        if(DURATION_DIFF(instance->decoder.te_last, VAG_T34_TE_SHORT) >= VAG_T34_TE_DELTA) break;
        instance->decoder.te_last = duration;
        instance->decoder.parser_step = VAGDecoderStepSync2A;
        break;

    case VAGDecoderStepSync2A:
        if(!level && DURATION_DIFF(duration, VAG_T34_TE_SHORT) < VAG_T34_TE_DELTA &&
           DURATION_DIFF(instance->decoder.te_last, VAG_T34_TE_LONG) < VAG_T34_LONG_DELTA) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = VAGDecoderStepSync2B;
        } else {
            instance->decoder.parser_step = VAGDecoderStepReset;
        }
        break;

    case VAGDecoderStepSync2B:
        if(level && DURATION_DIFF(duration, VAG_T34_SYNC) < VAG_T34_SYNC_DELTA) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = VAGDecoderStepSync2C;
        } else {
            instance->decoder.parser_step = VAGDecoderStepReset;
        }
        break;

    case VAGDecoderStepSync2C:
        if(!level && DURATION_DIFF(duration, VAG_T34_SYNC) < VAG_T34_SYNC_DELTA &&
           DURATION_DIFF(instance->decoder.te_last, VAG_T34_SYNC) < VAG_T34_SYNC_DELTA) {
            instance->mid_count++;
            instance->decoder.parser_step = VAGDecoderStepSync2B;
            if(instance->mid_count == VAG_T34_SYNC_PAIRS) {
                instance->data_low = 1;
                instance->data_high = 0;
                instance->bit_count = 1;
                rf_manchester_advance(
                    instance->manchester_state,
                    RfManchesterEventReset,
                    &instance->manchester_state,
                    NULL);
                instance->decoder.parser_step = VAGDecoderStepData2;
            }
        } else {
            instance->decoder.parser_step = VAGDecoderStepReset;
        }
        break;

    case VAGDecoderStepData2: {
        bool bit_value = false;
        RfManchesterEvent event = RfManchesterEventReset;
        bool got_pulse = false;

        if(DURATION_DIFF(duration, VAG_T34_TE_SHORT) < VAG_T34_TE_DELTA) {
            event = level ? RfManchesterEventShortLow : RfManchesterEventShortHigh;
            got_pulse = true;
        } else if(DURATION_DIFF(duration, VAG_T34_TE_LONG) < VAG_T34_LONG_DELTA) {
            event = level ? RfManchesterEventLongLow : RfManchesterEventLongHigh;
            got_pulse = true;
        }

        if(got_pulse) {
            if(rf_manchester_advance(
                   instance->manchester_state, event, &instance->manchester_state, &bit_value)) {
                uint32_t carry = (instance->data_low >> 31) & 1;
                instance->data_low = (instance->data_low << 1) | (bit_value ? 1 : 0);
                instance->data_high = (instance->data_high << 1) | carry;
                instance->bit_count++;

                if(instance->bit_count == VAG_KEY1_BITS) {
                    instance->key1_low = instance->data_low;
                    instance->key1_high = instance->data_high;
                    instance->data_low = 0;
                    instance->data_high = 0;
                }
            }
        }

        if(instance->bit_count != VAG_TOTAL_BITS) break;
        instance->key2_low = instance->data_low & 0xFFFF;
        instance->key2_high = 0;
        instance->data_count_bit = VAG_TOTAL_BITS;
        instance->vag_type = 3;
        RF_STREAM_LOG_I(
            TAG,
            "VAG decoded: Key1:%08lX%08lX Key2:%04X Type:%d",
            (unsigned long)instance->key1_high,
            (unsigned long)instance->key1_low,
            (unsigned int)(instance->key2_low & 0xFFFF),
            instance->vag_type);
        vag_parse_data(instance);
        if(instance->base.callback) {
            instance->base.callback(&instance->base, instance->base.context);
        }
        instance->data_low = 0;
        instance->data_high = 0;
        instance->bit_count = 0;
        instance->decoder.parser_step = VAGDecoderStepReset;
        break;
    }

    default:
        instance->decoder.parser_step = VAGDecoderStepReset;
        break;
    }
}

uint8_t rf_stream_protocol_decoder_vag_get_hash_data(void* context) {
    RfStreamProtocolDecoderVAG* instance = context;
    uint8_t hash = 0;
    hash ^= (instance->key1_low & 0xFF);
    hash ^= ((instance->key1_low >> 8) & 0xFF);
    hash ^= ((instance->key1_low >> 16) & 0xFF);
    hash ^= ((instance->key1_low >> 24) & 0xFF);
    hash ^= (instance->key1_high & 0xFF);
    hash ^= ((instance->key1_high >> 8) & 0xFF);
    hash ^= ((instance->key1_high >> 16) & 0xFF);
    hash ^= ((instance->key1_high >> 24) & 0xFF);
    return hash;
}

RfStreamProtocolStatus rf_stream_protocol_decoder_vag_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderVAG* instance = context;

    RF_STREAM_LOG_I(TAG, "=== VAG SERIALIZE START ===");
    RF_STREAM_LOG_I(
        TAG,
        "Before parse: decrypted=%d serial=%08lX cnt=%06lX btn=%02X type=%d",
        instance->decrypted,
        (unsigned long)instance->serial,
        (unsigned long)instance->cnt,
        instance->btn,
        instance->vag_type);

    if(!instance->decrypted && instance->data_count_bit >= 80) {
        RF_STREAM_LOG_I(TAG, "Not decrypted yet, calling vag_parse_data...");
        vag_parse_data(instance);
    }

    RF_STREAM_LOG_I(
        TAG,
        "After parse: decrypted=%d serial=%08lX cnt=%06lX btn=%02X type=%d",
        instance->decrypted,
        (unsigned long)instance->serial,
        (unsigned long)instance->cnt,
        instance->btn,
        instance->vag_type);

    uint64_t key1 = ((uint64_t)instance->key1_high << 32) | instance->key1_low;
    uint16_t key2_16bit = (uint16_t)(instance->key2_low & 0xFFFF);

    RF_STREAM_LOG_I(
        TAG,
        "Keys: Key1=%08lX%08lX Key2=%08lX%08lX",
        (unsigned long)instance->key1_high,
        (unsigned long)instance->key1_low,
        (unsigned long)instance->key2_high,
        (unsigned long)instance->key2_low);

    instance->generic.data = key1;
    instance->generic.data_count_bit = instance->data_count_bit;

    if(instance->decrypted) {
        instance->generic.serial = instance->serial;
        instance->generic.cnt = instance->cnt;
        instance->generic.btn = instance->btn;
        RF_STREAM_LOG_I(TAG, "Decrypted - setting generic fields for serialize");
    } else {
        RF_STREAM_LOG_W(TAG, "NOT decrypted - Serial/Cnt/Btn will be 0 in saved file!");
    }

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);

    RF_STREAM_LOG_I(TAG, "Generic serialize returned: %d", ret);

    if(ret == RfStreamProtocolStatusOk) {
        uint8_t key2_bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        key2_bytes[6] = (uint8_t)((key2_16bit >> 8) & 0xFF);
        key2_bytes[7] = (uint8_t)(key2_16bit & 0xFF);
        rf_record_write_hex(flipper_format, "Key2", key2_bytes, 8);
        RF_STREAM_LOG_I(TAG, "Wrote Key2");

        uint32_t type_tmp = instance->vag_type;
        rf_record_write_uint32(flipper_format, FF_TYPE, &type_tmp, 1);
        RF_STREAM_LOG_I(TAG, "Wrote Type: %d", instance->vag_type);

        if(instance->decrypted) {
            rf_stream_serialize_fields(
                flipper_format,
                RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT,
                instance->serial,
                instance->btn,
                instance->cnt,
                0);

            uint32_t key_idx_temp = instance->key_idx;
            rf_record_write_uint32(flipper_format, "KeyIdx", &key_idx_temp, 1);
            RF_STREAM_LOG_I(TAG, "Wrote KeyIdx: %d", instance->key_idx);
        }
    }

    RF_STREAM_LOG_I(TAG, "=== VAG SERIALIZE END (ret=%d) ===", ret);
    return ret;
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_vag_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderVAG* instance = context;

    RfStreamProtocolStatus ret = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, rf_stream_protocol_vag_const.min_count_bit_for_found);

    if(ret == RfStreamProtocolStatusOk) {
        uint64_t key1 = instance->generic.data;
        instance->key1_low = (uint32_t)key1;
        instance->key1_high = (uint32_t)(key1 >> 32);

        uint8_t key2_bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        rf_record_rewind(flipper_format);
        if(rf_record_read_hex(flipper_format, "Key2", key2_bytes, 8)) {
            uint16_t key2_16bit = ((uint16_t)key2_bytes[6] << 8) | (uint16_t)key2_bytes[7];
            instance->key2_low = (uint32_t)key2_16bit & 0xFFFF;
            instance->key2_high = 0;
            RF_STREAM_LOG_D(
                TAG,
                "Read Key2 from file: bytes[6]=0x%02X bytes[7]=0x%02X normalized=0x%04X",
                key2_bytes[6],
                key2_bytes[7],
                (unsigned int)key2_16bit);
        }

        uint32_t type = 0;
        rf_record_rewind(flipper_format);
        if(rf_record_read_uint32(flipper_format, FF_TYPE, &type, 1)) {
            instance->vag_type = (uint8_t)type;
        }

        instance->data_count_bit = instance->generic.data_count_bit;

        instance->decrypted = false;
        vag_parse_data(instance);
    }

    return ret;
}

#if RF_STREAM_WITH_ENCODER

typedef struct RfStreamProtocolEncoderVAG {
    RfStreamProtocolEncoderBase base;
    RfStreamBlockGeneric generic;

    uint32_t key1_low;
    uint32_t key1_high;
    uint32_t key2_low;
    uint32_t key2_high;
    uint32_t serial;
    uint32_t cnt;
    uint8_t vag_type;
    uint8_t btn;
    uint8_t dispatch_byte;
    uint8_t key_idx;

    size_t repeat;
    size_t front;
    size_t size_upload;
    LevelDuration* upload;
    bool is_running;
} RfStreamProtocolEncoderVAG;

static bool vag_aut64_encrypt(uint8_t* block, int key_index) {
    struct aut64_key* key = protocol_vag_get_key(key_index + 1);
    if(!key) {
        RF_STREAM_LOG_E(TAG, "Key not found for encryption: %d", key_index + 1);
        return false;
    }
    int rc = aut64_encrypt(key, block);
#ifdef AUT64_ENABLE_VALIDATIONS
    if(rc == AUT64_ERR_INVALID_KEY) {
        RF_STREAM_LOG_E(TAG, "Invalid key: %d", key_index + 1);
    } else if(rc == AUT64_ERR_NULL_POINTER) {
        RF_STREAM_LOG_E(TAG, "key is NULL");
    }
#endif

    return (rc == AUT64_OK) ? true : false;
}

static uint8_t vag_get_dispatch_byte(uint8_t btn, uint8_t vag_type) {
    if(vag_type == 1 || vag_type == 2) {
        switch(btn) {
        case 0x20:
        case 2:
            return 0x2A;
        case 0x40:
        case 4:
            return 0x46;
        case 0x10:
        case 1:
            return 0x1C;
        default:
            return 0x2A;
        }
    } else {
        switch(btn) {
        case 0x20:
        case 2:
            return 0x2B;
        case 0x40:
        case 4:
            return 0x47;
        case 0x10:
        case 1:
            return 0x1D;
        default:
            return 0x2B;
        }
    }
}

static uint8_t vag_btn_to_byte(uint8_t btn, uint8_t vag_type) {
    if(vag_type == 1) {
        return btn;
    } else {
        switch(btn) {
        case 0x1:
            return 0x10;
        case 0x2:
            return 0x20;
        case 0x4:
            return 0x40;
        default:
            return btn;
        }
    }
}

static void vag_encoder_build_type1(RfStreamProtocolEncoderVAG* instance) {
    RF_STREAM_LOG_I(TAG, "=== Building Type 1 upload (300us, AUT64) ===");

    size_t index = 0;
    LevelDuration* upload = instance->upload;
    const size_t cap = VAG_ENCODER_UPLOAD_MAX_SIZE;

    uint8_t btn_byte = vag_btn_to_byte(instance->btn, 1);
    uint8_t dispatch = vag_get_dispatch_byte(btn_byte, 1);
    instance->dispatch_byte = dispatch;

    RF_STREAM_LOG_D(TAG, "btn=%02X -> btn_byte=%02X, dispatch=%02X", instance->btn, btn_byte, dispatch);

    uint8_t block[8];
    uint8_t type_byte = (uint8_t)(instance->key1_high >> 24);

    block[0] = (uint8_t)(instance->serial >> 24);
    block[1] = (uint8_t)(instance->serial >> 16);
    block[2] = (uint8_t)(instance->serial >> 8);
    block[3] = (uint8_t)(instance->serial);

    block[4] = (uint8_t)(instance->cnt);
    block[5] = (uint8_t)(instance->cnt >> 8);
    block[6] = (uint8_t)(instance->cnt >> 16);

    block[7] = btn_byte;

    RF_STREAM_LOG_I(
        TAG,
        "Plaintext: %02X %02X %02X %02X %02X %02X %02X %02X (ser=%08lX cnt=%06lX btn=%02X)",
        block[0],
        block[1],
        block[2],
        block[3],
        block[4],
        block[5],
        block[6],
        block[7],
        (unsigned long)instance->serial,
        (unsigned long)instance->cnt,
        btn_byte);

    int key_idx = (instance->key_idx != 0xFF) ? instance->key_idx : 0;
    RF_STREAM_LOG_D(TAG, "Encrypting with AUT64 key index %d (saved=%d)", key_idx, instance->key_idx);

    if(!vag_aut64_encrypt(block, key_idx)) {
        RF_STREAM_LOG_E(TAG, "Type1 AUT64 encryption failed! Key not loaded?");
        instance->size_upload = 0;
        return;
    }

    RF_STREAM_LOG_I(
        TAG,
        "Encrypted: %02X %02X %02X %02X %02X %02X %02X %02X",
        block[0],
        block[1],
        block[2],
        block[3],
        block[4],
        block[5],
        block[6],
        block[7]);

    instance->key1_high = ((uint32_t)type_byte << 24) | ((uint32_t)block[0] << 16) |
                          ((uint32_t)block[1] << 8) | (uint32_t)block[2];
    instance->key1_low = ((uint32_t)block[3] << 24) | ((uint32_t)block[4] << 16) |
                         ((uint32_t)block[5] << 8) | (uint32_t)block[6];
    uint32_t key2_upper = ((uint32_t)(block[7] & 0xFF) << 8);
    uint32_t key2_lower = (uint32_t)(dispatch & 0xFF);
    instance->key2_low = (key2_upper | key2_lower) & 0xFFFF;
    instance->key2_high = 0;

    index = rf_stream_emit_short_pairs(upload, index, cap, 300, 220);
    index = rf_stream_emit(upload, index, cap, false, 300);
    index = rf_stream_emit(upload, index, cap, true, 300);

    RF_STREAM_LOG_D(TAG, "Preamble: %zu pulses (220 cycles + 2 sync)", index);

    uint16_t prefix = 0xAF3F;
#ifndef REMOVE_LOGS
    size_t prefix_start = index;
#endif
    for(int i = 15; i >= 0; i--) {
        bool bit = (prefix >> i) & 1;
        index = rf_stream_emit_manchester_bit(upload, index, cap, bit, 300);
    }
    RF_STREAM_LOG_D(TAG, "Prefix 0x%04X: %zu pulses", prefix, index - prefix_start);

    uint64_t key1 = ((uint64_t)instance->key1_high << 32) | instance->key1_low;
    uint64_t key1_inv = ~key1;
    RF_STREAM_LOG_D(
        TAG,
        "Key1: %08lX%08lX -> inverted: %08lX%08lX",
        (unsigned long)(key1 >> 32),
        (unsigned long)(key1 & 0xFFFFFFFF),
        (unsigned long)(key1_inv >> 32),
        (unsigned long)(key1_inv & 0xFFFFFFFF));

#ifndef REMOVE_LOGS
    size_t key1_start = index;
#endif
    for(int i = 63; i >= 0; i--) {
        bool bit = (key1_inv >> i) & 1;
        index = rf_stream_emit_manchester_bit(upload, index, cap, bit, 300);
    }
    RF_STREAM_LOG_D(TAG, "Key1: %zu pulses (64 bits)", index - key1_start);

    uint16_t key2 = (uint16_t)(instance->key2_low & 0xFFFF);
    uint16_t key2_inv = ~key2;
    RF_STREAM_LOG_D(TAG, "Key2: %04X -> inverted: %04X", key2, key2_inv);
#ifndef REMOVE_LOGS
    size_t key2_start = index;
#endif
    for(int i = 15; i >= 0; i--) {
        bool bit = (key2_inv >> i) & 1;
        index = rf_stream_emit_manchester_bit(upload, index, cap, bit, 300);
    }
    RF_STREAM_LOG_D(TAG, "Key2: %zu pulses (16 bits)", index - key2_start);

    index = rf_stream_emit(upload, index, cap, false, 6000);

    instance->size_upload = index;
    RF_STREAM_LOG_I(TAG, "Type1 upload built: %zu pulses (expected: 635)", index);

    if(index != 635) {
        RF_STREAM_LOG_W(TAG, "WARNING: Pulse count %zu != expected 635!", index);
    }
}

static void vag_encoder_build_type2(RfStreamProtocolEncoderVAG* instance) {
    RF_STREAM_LOG_I(TAG, "=== Building Type 2 upload (300us, TEA) ===");

    size_t index = 0;
    LevelDuration* upload = instance->upload;
    const size_t cap = VAG_ENCODER_UPLOAD_MAX_SIZE;

    uint8_t btn_byte = vag_btn_to_byte(instance->btn, 2);
    uint8_t dispatch = vag_get_dispatch_byte(btn_byte, 2);
    instance->dispatch_byte = dispatch;

    RF_STREAM_LOG_D(TAG, "btn=%02X -> btn_byte=%02X, dispatch=%02X", instance->btn, btn_byte, dispatch);

    uint8_t type_byte = (uint8_t)(instance->key1_high >> 24);

    uint8_t block[8];
    block[0] = (uint8_t)(instance->serial >> 24);
    block[1] = (uint8_t)(instance->serial >> 16);
    block[2] = (uint8_t)(instance->serial >> 8);
    block[3] = (uint8_t)(instance->serial);

    block[4] = (uint8_t)(instance->cnt);
    block[5] = (uint8_t)(instance->cnt >> 8);
    block[6] = (uint8_t)(instance->cnt >> 16);

    block[7] = btn_byte;

    RF_STREAM_LOG_I(
        TAG,
        "Plaintext: %02X %02X %02X %02X %02X %02X %02X %02X (ser=%08lX cnt=%06lX btn=%02X)",
        block[0],
        block[1],
        block[2],
        block[3],
        block[4],
        block[5],
        block[6],
        block[7],
        (unsigned long)instance->serial,
        (unsigned long)instance->cnt,
        btn_byte);

    uint32_t v0 = ((uint32_t)block[0] << 24) | ((uint32_t)block[1] << 16) |
                  ((uint32_t)block[2] << 8) | (uint32_t)block[3];
    uint32_t v1 = ((uint32_t)block[4] << 24) | ((uint32_t)block[5] << 16) |
                  ((uint32_t)block[6] << 8) | (uint32_t)block[7];

    RF_STREAM_LOG_D(TAG, "TEA input: v0=%08lX v1=%08lX", (unsigned long)v0, (unsigned long)v1);

    vag_tea_encrypt(&v0, &v1, vag_tea_key_schedule);

    RF_STREAM_LOG_D(TAG, "TEA output: v0=%08lX v1=%08lX", (unsigned long)v0, (unsigned long)v1);

    block[0] = (uint8_t)(v0 >> 24);
    block[1] = (uint8_t)(v0 >> 16);
    block[2] = (uint8_t)(v0 >> 8);
    block[3] = (uint8_t)(v0);
    block[4] = (uint8_t)(v1 >> 24);
    block[5] = (uint8_t)(v1 >> 16);
    block[6] = (uint8_t)(v1 >> 8);
    block[7] = (uint8_t)(v1);

    RF_STREAM_LOG_I(
        TAG,
        "Encrypted: %02X %02X %02X %02X %02X %02X %02X %02X",
        block[0],
        block[1],
        block[2],
        block[3],
        block[4],
        block[5],
        block[6],
        block[7]);

    instance->key1_high = ((uint32_t)type_byte << 24) | ((uint32_t)block[0] << 16) |
                          ((uint32_t)block[1] << 8) | (uint32_t)block[2];
    instance->key1_low = ((uint32_t)block[3] << 24) | ((uint32_t)block[4] << 16) |
                         ((uint32_t)block[5] << 8) | (uint32_t)block[6];
    uint32_t key2_upper = ((uint32_t)(block[7] & 0xFF) << 8);
    uint32_t key2_lower = (uint32_t)(dispatch & 0xFF);
    instance->key2_low = (key2_upper | key2_lower) & 0xFFFF;
    instance->key2_high = 0;

    index = rf_stream_emit_short_pairs(upload, index, cap, 300, 220);
    index = rf_stream_emit(upload, index, cap, false, 300);
    index = rf_stream_emit(upload, index, cap, true, 300);

    RF_STREAM_LOG_D(TAG, "Preamble: %zu pulses (220 cycles + 2 sync)", index);

    uint16_t prefix = 0xAF1C;
#ifndef REMOVE_LOGS
    size_t prefix_start = index;
#endif
    for(int i = 15; i >= 0; i--) {
        bool bit = (prefix >> i) & 1;
        index = rf_stream_emit_manchester_bit(upload, index, cap, bit, 300);
    }
    RF_STREAM_LOG_D(TAG, "Prefix 0x%04X: %zu pulses", prefix, index - prefix_start);

    uint64_t key1 = ((uint64_t)instance->key1_high << 32) | instance->key1_low;
    uint64_t key1_inv = ~key1;
    RF_STREAM_LOG_D(
        TAG,
        "Key1: %08lX%08lX -> inverted: %08lX%08lX",
        (unsigned long)(key1 >> 32),
        (unsigned long)(key1 & 0xFFFFFFFF),
        (unsigned long)(key1_inv >> 32),
        (unsigned long)(key1_inv & 0xFFFFFFFF));
#ifndef REMOVE_LOGS
    size_t key1_start = index;
#endif
    for(int i = 63; i >= 0; i--) {
        bool bit = (key1_inv >> i) & 1;
        index = rf_stream_emit_manchester_bit(upload, index, cap, bit, 300);
    }
    RF_STREAM_LOG_D(TAG, "Key1: %zu pulses", index - key1_start);

    uint16_t key2 = (uint16_t)(instance->key2_low & 0xFFFF);
    uint16_t key2_inv = ~key2;
    RF_STREAM_LOG_D(TAG, "Key2: %04X -> inverted: %04X", key2, key2_inv);
#ifndef REMOVE_LOGS
    size_t key2_start = index;
#endif
    for(int i = 15; i >= 0; i--) {
        bool bit = (key2_inv >> i) & 1;
        index = rf_stream_emit_manchester_bit(upload, index, cap, bit, 300);
    }
    RF_STREAM_LOG_D(TAG, "Key2: %zu pulses", index - key2_start);

    index = rf_stream_emit(upload, index, cap, false, 6000);

    instance->size_upload = index;
    RF_STREAM_LOG_I(TAG, "Type2 upload built: %zu pulses (expected: 635)", index);
}

static void vag_encoder_build_type3_4(RfStreamProtocolEncoderVAG* instance) {
    RF_STREAM_LOG_I(TAG, "=== Building Type %d upload (500us, AUT64) ===", instance->vag_type);

    size_t index = 0;
    LevelDuration* upload = instance->upload;
    const size_t cap = VAG_ENCODER_UPLOAD_MAX_SIZE;

    uint8_t btn_byte = vag_btn_to_byte(instance->btn, instance->vag_type);
    uint8_t dispatch = vag_get_dispatch_byte(btn_byte, instance->vag_type);
    instance->dispatch_byte = dispatch;

    RF_STREAM_LOG_D(TAG, "btn=%02X -> btn_byte=%02X, dispatch=%02X", instance->btn, btn_byte, dispatch);

    uint8_t type_byte = (uint8_t)(instance->key1_high >> 24);

    uint8_t block[8];
    block[0] = (uint8_t)(instance->serial >> 24);
    block[1] = (uint8_t)(instance->serial >> 16);
    block[2] = (uint8_t)(instance->serial >> 8);
    block[3] = (uint8_t)(instance->serial);
    block[4] = (uint8_t)(instance->cnt);
    block[5] = (uint8_t)(instance->cnt >> 8);
    block[6] = (uint8_t)(instance->cnt >> 16);
    block[7] = btn_byte;

    RF_STREAM_LOG_I(
        TAG,
        "Plaintext: %02X %02X %02X %02X %02X %02X %02X %02X (ser=%08lX cnt=%06lX btn=%02X)",
        block[0],
        block[1],
        block[2],
        block[3],
        block[4],
        block[5],
        block[6],
        block[7],
        (unsigned long)instance->serial,
        (unsigned long)instance->cnt,
        btn_byte);

    int key_idx;
    if(instance->key_idx != 0xFF) {
        key_idx = instance->key_idx;
    } else {
        key_idx = (instance->vag_type == 4) ? 2 : 1;
    }
    RF_STREAM_LOG_D(TAG, "Encrypting with AUT64 key index %d (saved=%d)", key_idx, instance->key_idx);

    if(!vag_aut64_encrypt(block, key_idx)) {
        RF_STREAM_LOG_E(TAG, "Type%d AUT64 encryption failed! Key not loaded?", instance->vag_type);
        instance->size_upload = 0;
        return;
    }

    RF_STREAM_LOG_I(
        TAG,
        "Encrypted: %02X %02X %02X %02X %02X %02X %02X %02X",
        block[0],
        block[1],
        block[2],
        block[3],
        block[4],
        block[5],
        block[6],
        block[7]);

    instance->key1_high = ((uint32_t)type_byte << 24) | ((uint32_t)block[0] << 16) |
                          ((uint32_t)block[1] << 8) | (uint32_t)block[2];
    instance->key1_low = ((uint32_t)block[3] << 24) | ((uint32_t)block[4] << 16) |
                         ((uint32_t)block[5] << 8) | (uint32_t)block[6];
    uint32_t key2_upper = ((uint32_t)(block[7] & 0xFF) << 8);
    uint32_t key2_lower = (uint32_t)(dispatch & 0xFF);
    instance->key2_low = (key2_upper | key2_lower) & 0xFFFF;
    instance->key2_high = 0;

    uint64_t key1 = ((uint64_t)instance->key1_high << 32) | instance->key1_low;
    uint16_t key2 = (uint16_t)(instance->key2_low & 0xFFFF);
    RF_STREAM_LOG_D(
        TAG,
        "Key1: %08lX%08lX (NOT inverted for Type 3/4)",
        (unsigned long)(key1 >> 32),
        (unsigned long)(key1 & 0xFFFFFFFF));
    RF_STREAM_LOG_D(TAG, "Key2: %04X (NOT inverted for Type 3/4)", key2);
#ifndef REMOVE_LOGS
    uint8_t key1_byte6 = (key1 >> 8) & 0xFF;
    uint8_t key1_byte7 = key1 & 0xFF;
    RF_STREAM_LOG_D(
        TAG,
        "Key1 last 2 bytes: %02X %02X (bits: %d%d%d%d%d%d%d%d %d%d%d%d%d%d%d%d)",
        key1_byte6,
        key1_byte7,
        (key1_byte6 >> 7) & 1,
        (key1_byte6 >> 6) & 1,
        (key1_byte6 >> 5) & 1,
        (key1_byte6 >> 4) & 1,
        (key1_byte6 >> 3) & 1,
        (key1_byte6 >> 2) & 1,
        (key1_byte6 >> 1) & 1,
        (key1_byte6 >> 0) & 1,
        (key1_byte7 >> 7) & 1,
        (key1_byte7 >> 6) & 1,
        (key1_byte7 >> 5) & 1,
        (key1_byte7 >> 4) & 1,
        (key1_byte7 >> 3) & 1,
        (key1_byte7 >> 2) & 1,
        (key1_byte7 >> 1) & 1,
        (key1_byte7 >> 0) & 1);
#endif
    for(int repeat = 0; repeat < 2; repeat++) {
#ifndef REMOVE_LOGS
        size_t repeat_start = index;
#endif
        index = rf_stream_emit_short_pairs(upload, index, cap, 500, 45);
        RF_STREAM_LOG_D(
            TAG, "Repeat %d: Preamble %zu pulses (45 cycles)", repeat + 1, index - repeat_start);

        index = rf_stream_emit(upload, index, cap, true, 1000);
        index = rf_stream_emit(upload, index, cap, false, 500);
        index = rf_stream_emit_short_pairs(upload, index, cap, 750, 3);

#ifndef REMOVE_LOGS
        size_t key1_start = index;
#endif
        for(int i = 63; i >= 0; i--) {
            bool bit = (key1 >> i) & 1;

            index = rf_stream_emit_manchester_bit(upload, index, cap, bit, 500);
        }
        RF_STREAM_LOG_D(TAG, "Repeat %d: Key1 %zu pulses (64 bits)", repeat + 1, index - key1_start);

#ifndef REMOVE_LOGS
        size_t key2_start = index;
#endif
        for(int i = 15; i >= 0; i--) {
            bool bit = (key2 >> i) & 1;
            index = rf_stream_emit_manchester_bit(upload, index, cap, bit, 500);
        }
        RF_STREAM_LOG_D(TAG, "Repeat %d: Key2 %zu pulses (16 bits)", repeat + 1, index - key2_start);

        index = rf_stream_emit(upload, index, cap, false, 10000);
        RF_STREAM_LOG_D(TAG, "Repeat %d: Total %zu pulses", repeat + 1, index - repeat_start);
    }

    instance->size_upload = index;
    RF_STREAM_LOG_I(TAG, "Type%d upload built: %zu pulses (expected: 518)", instance->vag_type, index);

    if(index != 518) {
        RF_STREAM_LOG_W(TAG, "WARNING: Pulse count %zu != expected 518!", index);
    }
}
#endif

void rf_stream_protocol_decoder_vag_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderVAG* instance = context;

    if(!instance->decrypted && instance->data_count_bit >= 80) {
        vag_parse_data(instance);
    }

    uint64_t key1 = ((uint64_t)instance->key1_high << 32) | instance->key1_low;
    uint16_t key2 = (uint16_t)(instance->key2_low & 0xFFFF);

    uint8_t type_byte = (uint8_t)(instance->key1_high >> 24);
    const char* vehicle_name;
    switch(type_byte) {
    case 0x00:
        vehicle_name = "VW Passat";
        break;
    case 0xC0:
        vehicle_name = "VW";
        break;
    case 0xC1:
        vehicle_name = "Audi";
        break;
    case 0xC2:
        vehicle_name = "Seat";
        break;
    case 0xC3:
        vehicle_name = "Skoda";
        break;
    default:
        vehicle_name = "VAG";
        break;
    }

    if(instance->decrypted) {
        rf_text_cat_printf(
            output,
            "%s %dbit\r\n"
            "Key1:%08lX%08lX\r\n"
            "Key2:%04X Btn:%0X - %s\r\n"
            "Ser:%08lX Cnt:%06lX\r\n",
            vehicle_name,
            instance->data_count_bit,
            (unsigned long)(key1 >> 32),
            (unsigned long)(key1 & 0xFFFFFFFF),
            key2,
            instance->btn,
            vag_button_name(instance->btn),
            (unsigned long)instance->serial,
            (unsigned long)instance->cnt);
    } else {
        rf_text_cat_printf(
            output,
            "%s %dbit\r\n"
            "Key1:%08lX%08lX\r\n"
            "Key2:%04X (corrupted)\r\n",
            vehicle_name,
            instance->data_count_bit,
            (unsigned long)(key1 >> 32),
            (unsigned long)(key1 & 0xFFFFFFFF),
            key2);
    }
}

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_vag_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RF_STREAM_LOG_I(TAG, "VAG encoder alloc");

    RfStreamProtocolEncoderVAG* instance = calloc(1, sizeof(RfStreamProtocolEncoderVAG));
    instance->base.protocol = &vag_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->upload = rf_stream_shared_upload_buffer();
    instance->repeat = 10;
    instance->is_running = false;
    instance->key_idx = 0xFF;

    protocol_vag_load_keys(APP_ASSETS_PATH("keystore/vag"));

    RF_STREAM_LOG_I(TAG, "VAG encoder alloc complete, keys loaded: %d", protocol_vag_keys_loaded);

    return instance;
}

void rf_stream_protocol_encoder_vag_free(void* context) {
    RfStreamProtocolEncoderVAG* instance = context;
    instance->upload = NULL;
    free(instance);
}

void rf_stream_protocol_encoder_vag_stop(void* context) {
    RfStreamProtocolEncoderVAG* instance = context;
    RF_STREAM_LOG_I(TAG, "VAG encoder stop (was_running=%d)", instance->is_running);
    instance->is_running = false;
}

LevelDuration rf_stream_protocol_encoder_vag_yield(void* context) {
    RfStreamProtocolEncoderVAG* instance = context;

    if(!instance->is_running || instance->repeat == 0) {
        if(instance->is_running) {
            RF_STREAM_LOG_I(TAG, "VAG encoder transmission complete");
        }
        instance->is_running = false;
        return level_duration_reset();
    }

    LevelDuration ret = instance->upload[instance->front];
    instance->front++;

    if(instance->front >= instance->size_upload) {
        instance->front = 0;
        instance->repeat--;
        RF_STREAM_LOG_D(TAG, "VAG encoder repeat cycle, %zu remaining", instance->repeat);
    }

    return ret;
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_vag_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderVAG* instance = context;

    RF_STREAM_LOG_I(TAG, "=== VAG ENCODER DESERIALIZE START ===");

    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    do {
        ret = rf_stream_block_generic_deserialize(&instance->generic, flipper_format);
        if(ret != RfStreamProtocolStatusOk) {
            RF_STREAM_LOG_E(TAG, "Encoder deserialize: generic failed (ret=%d)", ret);
            break;
        }

        uint64_t key1 = instance->generic.data;
        instance->key1_low = (uint32_t)key1;
        instance->key1_high = (uint32_t)(key1 >> 32);
        RF_STREAM_LOG_I(
            TAG,
            "Loaded Key1: %08lX%08lX",
            (unsigned long)instance->key1_high,
            (unsigned long)instance->key1_low);

        uint8_t key2_bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        rf_record_rewind(flipper_format);
        if(!rf_record_read_hex(flipper_format, "Key2", key2_bytes, 8)) {
            RF_STREAM_LOG_E(TAG, "Encoder deserialize: Key2 not found in file");
            ret = RfStreamProtocolStatusErrorParserOthers;
            break;
        }
        uint16_t key2_16bit = ((uint16_t)key2_bytes[6] << 8) | (uint16_t)key2_bytes[7];
        instance->key2_low = (uint32_t)key2_16bit & 0xFFFF;
        instance->key2_high = 0;
        RF_STREAM_LOG_I(
            TAG,
            "Loaded Key2: bytes[6]=0x%02X bytes[7]=0x%02X normalized=0x%04X (stored as %08lX%08lX)",
            key2_bytes[6],
            key2_bytes[7],
            (unsigned int)key2_16bit,
            (unsigned long)instance->key2_high,
            (unsigned long)instance->key2_low);

        uint32_t serial_val = instance->generic.serial;
        uint32_t btn_val = instance->generic.btn;
        uint32_t cnt_val = instance->generic.cnt;
        uint32_t type_val = 0;
        rf_stream_encoder_read_fields(flipper_format, &serial_val, &btn_val, &cnt_val, &type_val);
        instance->vag_type = (uint8_t)type_val;

        uint32_t file_key_idx = 0xFF;
        rf_record_rewind(flipper_format);
        rf_record_read_uint32(flipper_format, "KeyIdx", &file_key_idx, 1);

        instance->serial = serial_val;
        instance->cnt = cnt_val;
        instance->btn = (uint8_t)btn_val;
        instance->key_idx = (uint8_t)file_key_idx;

        RF_STREAM_LOG_I(
            TAG,
            "Final values: Ser=%08lX Cnt=%06lX Btn=%02X KeyIdx=%d",
            (unsigned long)instance->serial,
            (unsigned long)instance->cnt,
            instance->btn,
            instance->key_idx);

        if(instance->key_idx == 0xFF) {
            RF_STREAM_LOG_I(
                TAG, "KeyIdx not found in file, decoding original signal to find correct key...");

            RfStreamProtocolDecoderVAG decoder;
            memset(&decoder, 0, sizeof(decoder));
            decoder.key1_low = instance->key1_low;
            decoder.key1_high = instance->key1_high;
            decoder.key2_low = instance->key2_low;
            decoder.key2_high = instance->key2_high;
            decoder.vag_type = instance->vag_type;
            decoder.data_count_bit = 80;
            decoder.key_idx = 0xFF;
            vag_parse_data(&decoder);

            if(decoder.decrypted) {
                instance->key_idx = decoder.key_idx;
                RF_STREAM_LOG_I(TAG, "Decoded key_idx=%d from original signal", instance->key_idx);

                if(instance->serial == 0 && instance->cnt == 0) {
                    instance->serial = decoder.serial;
                    instance->cnt = decoder.cnt;
                    instance->btn = decoder.btn;
                    RF_STREAM_LOG_I(
                        TAG,
                        "Also decoded: Ser=%08lX Cnt=%06lX Btn=%02X",
                        (unsigned long)instance->serial,
                        (unsigned long)instance->cnt,
                        instance->btn);
                }
            } else {
                RF_STREAM_LOG_W(
                    TAG,
                    "Could not decode original signal - check if keys are loaded and Type is correct");
            }
        }
#ifndef REMOVE_LOGS
        uint32_t old_cnt = instance->cnt;
#endif
        instance->cnt = (instance->cnt + 1) & 0xFFFFFF;
        RF_STREAM_LOG_I(
            TAG,
            "Counter incremented: %06lX -> %06lX",
            (unsigned long)old_cnt,
            (unsigned long)instance->cnt);

        uint8_t type_byte = (uint8_t)(instance->key1_high >> 24);
        if(instance->vag_type == 1 && type_byte == 0x00) {
            RF_STREAM_LOG_I(
                TAG,
                "Detected Passat signal (type_byte 0x00), converting type 1 -> type 2 to fix key2 issue");
            instance->vag_type = 2;
        }

        RF_STREAM_LOG_I(TAG, "Building upload for type %d...", instance->vag_type);
        switch(instance->vag_type) {
        case 1:
            vag_encoder_build_type1(instance);
            break;
        case 2:
            vag_encoder_build_type2(instance);
            break;
        case 3:
        case 4:
            vag_encoder_build_type3_4(instance);
            break;
        default:
            RF_STREAM_LOG_W(TAG, "Unknown type %d, defaulting to type 1", instance->vag_type);
            instance->vag_type = 1;
            vag_encoder_build_type1(instance);
            break;
        }

        if(instance->size_upload == 0) {
            RF_STREAM_LOG_E(TAG, "Failed to build upload buffer!");
            ret = RfStreamProtocolStatusErrorEncoderGetUpload;
            break;
        }

        RF_STREAM_LOG_I(
            TAG,
            "Upload built: %zu pulses, Key1=%08lX%08lX Key2=%04lX",
            instance->size_upload,
            (unsigned long)instance->key1_high,
            (unsigned long)instance->key1_low,
            (unsigned long)(instance->key2_low & 0xFFFF));

        rf_record_rewind(flipper_format);
        uint8_t key1_bytes[8];
        key1_bytes[0] = (uint8_t)(instance->key1_high >> 24);
        key1_bytes[1] = (uint8_t)(instance->key1_high >> 16);
        key1_bytes[2] = (uint8_t)(instance->key1_high >> 8);
        key1_bytes[3] = (uint8_t)(instance->key1_high);
        key1_bytes[4] = (uint8_t)(instance->key1_low >> 24);
        key1_bytes[5] = (uint8_t)(instance->key1_low >> 16);
        key1_bytes[6] = (uint8_t)(instance->key1_low >> 8);
        key1_bytes[7] = (uint8_t)(instance->key1_low);
        if(!rf_record_update_hex(flipper_format, FF_KEY, key1_bytes, 8)) {
            RF_STREAM_LOG_W(TAG, "Failed to update Key in file (non-fatal)");
        }

        rf_record_rewind(flipper_format);
        instance->key2_high = 0;
        uint16_t key2_write = (uint16_t)(instance->key2_low & 0xFFFF);
        instance->key2_low = (uint32_t)key2_write;
        uint8_t key2_write_bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        key2_write_bytes[6] = (uint8_t)((key2_write >> 8) & 0xFF);
        key2_write_bytes[7] = (uint8_t)(key2_write & 0xFF);
        if(!rf_record_update_hex(flipper_format, "Key2", key2_write_bytes, 8)) {
            RF_STREAM_LOG_W(TAG, "Failed to update Key2 in file (non-fatal)");
        }

        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_SERIAL, instance->serial);
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_CNT, instance->cnt);
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_BTN, instance->btn);
        rf_stream_flipper_update_or_insert_u32(flipper_format, "KeyIdx", instance->key_idx);
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_TYPE, instance->vag_type);

        RF_STREAM_LOG_I(
            TAG,
            "Updated file: Serial=%08lX Cnt=%06lX Btn=%02X KeyIdx=%d Type=%d",
            (unsigned long)instance->serial,
            (unsigned long)instance->cnt,
            instance->btn,
            instance->key_idx,
            instance->vag_type);

        instance->repeat = 10;
        instance->front = 0;
        instance->is_running = true;

        RF_STREAM_LOG_I(TAG, "=== VAG ENCODER READY (repeat=%zu) ===", instance->repeat);
        ret = RfStreamProtocolStatusOk;
    } while(false);

    if(ret != RfStreamProtocolStatusOk) {
        RF_STREAM_LOG_E(TAG, "=== VAG ENCODER DESERIALIZE FAILED (ret=%d) ===", ret);
    }

    return ret;
}
#endif

#endif
