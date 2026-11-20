// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define SUBGHZ_PROTOCOL_STAR_LINE_NAME "Star Line"

typedef struct RfStreamProtocolDecoderStarLine RfStreamProtocolDecoderStarLine;
typedef struct RfStreamProtocolEncoderStarLine RfStreamProtocolEncoderStarLine;

extern const RfStreamProtocolDecoder rf_stream_protocol_star_line_decoder;
extern const RfStreamProtocolEncoder rf_stream_protocol_star_line_encoder;
extern const RfStreamCodec rf_stream_protocol_star_line;

/**
 * Allocate RfStreamProtocolEncoderStarLine.
 * @param environment Pointer to a RfStreamEnvironment instance
 * @return RfStreamProtocolEncoderStarLine* pointer to a RfStreamProtocolEncoderStarLine instance
 */
void* rf_stream_protocol_encoder_star_line_alloc(RfStreamEnvironment* environment);

/**
 * Free RfStreamProtocolEncoderStarLine.
 * @param context Pointer to a RfStreamProtocolEncoderStarLine instance
 */
void rf_stream_protocol_encoder_star_line_free(void* context);

/**
 * Deserialize and generating an upload to send.
 * @param context Pointer to a RfStreamProtocolEncoderStarLine instance
 * @param flipper_format Pointer to a RfRecord instance
 * @return true On success
 */
RfStreamProtocolStatus
    rf_stream_protocol_encoder_star_line_deserialize(void* context, RfRecord* flipper_format);

/**
 * Allocate RfStreamProtocolDecoderStarLine.
 * @param environment Pointer to a RfStreamEnvironment instance
 * @return RfStreamProtocolDecoderStarLine* pointer to a RfStreamProtocolDecoderStarLine instance
 */
void* rf_stream_protocol_decoder_star_line_alloc(RfStreamEnvironment* environment);

/**
 * Free RfStreamProtocolDecoderStarLine.
 * @param context Pointer to a RfStreamProtocolDecoderStarLine instance
 */
void rf_stream_protocol_decoder_star_line_free(void* context);

/**
 * Reset decoder RfStreamProtocolDecoderStarLine.
 * @param context Pointer to a RfStreamProtocolDecoderStarLine instance
 */
void rf_stream_protocol_decoder_star_line_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a RfStreamProtocolDecoderStarLine instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void rf_stream_protocol_decoder_star_line_feed(void* context, bool level, uint32_t duration);

/**
 * Serialize data RfStreamProtocolDecoderStarLine.
 * @param context Pointer to a RfStreamProtocolDecoderStarLine instance
 * @param flipper_format Pointer to a RfRecord instance
 * @param preset The modulation on which the signal was received, RfStreamRadioPreset
 * @return status
 */
RfStreamProtocolStatus rf_stream_protocol_decoder_star_line_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);

/**
 * Deserialize data RfStreamProtocolDecoderStarLine.
 * @param context Pointer to a RfStreamProtocolDecoderStarLine instance
 * @param flipper_format Pointer to a RfRecord instance
 * @return status
 */
RfStreamProtocolStatus
    rf_stream_protocol_decoder_star_line_deserialize(void* context, RfRecord* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a RfStreamProtocolDecoderStarLine instance
 * @param output Resulting text
 */
void rf_stream_protocol_decoder_star_line_get_string(void* context, RfText* output);






#define TAG "SubGhzProtocolStarLine"

static const RfStreamBlockConst rf_stream_protocol_star_line_const = {
    .te_short = 250,
    .te_long = 500,
    .te_delta = 120,
    .min_count_bit_for_found = 64,
};

#define STAR_LINE_MIN_COUNT_BIT   64U
#define STAR_LINE_UPLOAD_CAPACITY ((6U * 2U) + (STAR_LINE_MIN_COUNT_BIT * 2U))
_Static_assert(
    STAR_LINE_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "STAR_LINE_UPLOAD_CAPACITY exceeds shared upload slab");

struct RfStreamProtocolDecoderStarLine {
    RfStreamProtocolDecoderBase base;

    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    uint16_t header_count;
    RfStreamKeystore* keystore;
    const char* manufacture_name;

    RfText* manufacture_from_file;
};

struct RfStreamProtocolEncoderStarLine {
    RfStreamProtocolEncoderBase base;

    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;

    RfStreamKeystore* keystore;
    const char* manufacture_name;

    RfText* manufacture_from_file;
};

typedef enum {
    StarLineDecoderStepReset = 0,
    StarLineDecoderStepCheckPreambula,
    StarLineDecoderStepSaveDuration,
    StarLineDecoderStepCheckDuration,
} StarLineDecoderStep;

const RfStreamProtocolDecoder rf_stream_protocol_star_line_decoder = {
    .alloc = rf_stream_protocol_decoder_star_line_alloc,
    .free = rf_stream_protocol_decoder_star_line_free,

    .feed = rf_stream_protocol_decoder_star_line_feed,
    .reset = rf_stream_protocol_decoder_star_line_reset,

    .get_hash_data = rf_stream_decoder_hash_blocks,
    .serialize = rf_stream_protocol_decoder_star_line_serialize,
    .deserialize = rf_stream_protocol_decoder_star_line_deserialize,
    .get_string = rf_stream_protocol_decoder_star_line_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_star_line_encoder = {
    .alloc = rf_stream_protocol_encoder_star_line_alloc,
    .free = rf_stream_protocol_encoder_star_line_free,

    .deserialize = rf_stream_protocol_encoder_star_line_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_star_line_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec rf_stream_protocol_star_line = {
    .name = SUBGHZ_PROTOCOL_STAR_LINE_NAME,

#if RF_STREAM_WITH_DECODER

    .decoder = &rf_stream_protocol_star_line_decoder,

#else

    .decoder = NULL,

#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_star_line_encoder,
#else
    .encoder = NULL,
#endif
};

/**
 * Analysis of received data
 * @param instance Pointer to a RfStreamBlockGeneric* instance
 * @param keystore Pointer to a RfStreamKeystore* instance
 * @param manufacture_name
 */
static void rf_stream_protocol_star_line_check_remote_controller(
    RfStreamBlockGeneric* instance,
    RfStreamKeystore* keystore,
    const char** manufacture_name);
#if RF_STREAM_WITH_ENCODER

void* rf_stream_protocol_encoder_star_line_alloc(RfStreamEnvironment* environment) {
    RfStreamProtocolEncoderStarLine* instance = calloc(1, sizeof(RfStreamProtocolEncoderStarLine));

    instance->base.protocol = &rf_stream_protocol_star_line;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->keystore = rf_stream_environment_get_keystore(environment);

    instance->manufacture_from_file = rf_text_alloc();

    instance->encoder.repeat = 40;
    rf_stream_encoder_buffer_ensure(instance, STAR_LINE_UPLOAD_CAPACITY);
    instance->encoder.is_running = false;

    return instance;
}

#endif
#if RF_STREAM_WITH_ENCODER

void rf_stream_protocol_encoder_star_line_free(void* context) {
    RfStreamProtocolEncoderStarLine* instance = context;
    rf_text_free(instance->manufacture_from_file);
    free(instance);
}

#endif

/**
 * Key generation from simple data
 * @param instance Pointer to a RfStreamProtocolEncoderKeeloq* instance
 * @param btn Button number, 4 bit
 */
static bool
    rf_stream_protocol_star_line_gen_data(RfStreamProtocolEncoderStarLine* instance, uint8_t btn) {
    // Increase counter
    if((instance->generic.cnt + 1) > 0xFFFF) {
        instance->generic.cnt = 0;
    } else {
        instance->generic.cnt += 1;
    }

    uint32_t fix = (uint32_t)btn << 24 | instance->generic.serial;
    uint32_t decrypt = (uint32_t)btn << 24 | (instance->generic.serial & 0xFF) << 16 | instance->generic.cnt;
    uint32_t hop = 0;
    uint64_t man = 0;
    uint64_t code_found_reverse;
    int res = 0;

    if(instance->manufacture_name == 0x0) {
        instance->manufacture_name = "";
    }

    if(strcmp(instance->manufacture_name, "Unknown") == 0) {
        code_found_reverse = rf_stream_protocol_blocks_reverse_key(
            instance->generic.data, instance->generic.data_count_bit);
        hop = code_found_reverse & 0x00000000ffffffff;
    } else {
        uint8_t kl_type_en = instance->keystore->kl_type;
        for
            M_EACH(
                manufacture_code,
                *rf_stream_keystore_get_data(instance->keystore),
                RfStreamKeyArray_t) {
                res = strcmp(
                    rf_text_get_cstr(manufacture_code->name), instance->manufacture_name);
                if(res == 0) {
                    switch(manufacture_code->type) {
                    case KEELOQ_SIMPLE_LEARNING:
                        //Simple Learning
                        hop =
                            keeloq_encrypt(decrypt, manufacture_code->key);
                        break;
                    case KEELOQ_NORMAL_LEARNING:
                        //Normal Learning
                        man = keeloq_normal_learning(
                            fix, manufacture_code->key);
                        hop = keeloq_encrypt(decrypt, man);
                        break;
                    case KEELOQ_UNKNOWN_LEARNING:
                        if(kl_type_en == 1) {
                            hop = keeloq_encrypt(
                                decrypt, manufacture_code->key);
                        }
                        if(kl_type_en == 2) {
                            man = keeloq_normal_learning(
                                fix, manufacture_code->key);
                            hop = keeloq_encrypt(decrypt, man);
                        }
                        break;
                    }
                    break;
                }
            }
    }
    if(hop) {
        uint64_t yek = (uint64_t)fix << 32 | hop;
        instance->generic.data =
            rf_stream_protocol_blocks_reverse_key(yek, instance->generic.data_count_bit);
        return true;
    } else {
        instance->manufacture_name = "Unknown";
        return false;
    }
}

bool rf_stream_protocol_star_line_create_data(
    void* context,
    RfRecord* flipper_format,
    uint32_t serial,
    uint8_t btn,
    uint16_t cnt,
    const char* manufacture_name,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolEncoderStarLine* instance = context;
    instance->generic.serial = serial;
    instance->generic.cnt = cnt;
    instance->manufacture_name = manufacture_name;
    instance->generic.data_count_bit = 64;
    bool res = rf_stream_protocol_star_line_gen_data(instance, btn);
    if(res) {
        return RfStreamProtocolStatusOk ==
               rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
    }
    return res;
}

/**
 * Generating an upload from data.
 * @param instance Pointer to a RfStreamProtocolEncoderKeeloq instance
 * @return true On success
 */
#if RF_STREAM_WITH_ENCODER
static bool rf_stream_protocol_encoder_star_line_get_upload(
    RfStreamProtocolEncoderStarLine* instance,
    uint8_t btn) {
    // Gen new key
    if(!rf_stream_protocol_star_line_gen_data(instance, btn)) {
        return false;
    }

    size_t index = 0;
    size_t size_upload = 6 * 2 + (instance->generic.data_count_bit * 2);
    if(size_upload > instance->encoder.size_upload) {
        RF_STREAM_LOG_E(TAG, "Size upload exceeds allocated encoder buffer.");
        return false;
    } else {
        instance->encoder.size_upload = size_upload;
    }

    LevelDuration* up = instance->encoder.upload;
    const size_t cap = STAR_LINE_UPLOAD_CAPACITY;
    const uint32_t te_short = (uint32_t)rf_stream_protocol_star_line_const.te_short;
    const uint32_t te_long = (uint32_t)rf_stream_protocol_star_line_const.te_long;

    index = rf_stream_emit_short_pairs(up, index, cap, te_long * 2U, 6);

    for(uint8_t i = instance->generic.data_count_bit; i > 0; i--) {
        const uint32_t te = bit_read(instance->generic.data, i - 1) ? te_long : te_short;
        index = rf_stream_emit(up, index, cap, true, te);
        index = rf_stream_emit(up, index, cap, false, te);
    }

    return true;
}

#endif
#if RF_STREAM_WITH_ENCODER

static RfStreamProtocolStatus rf_stream_protocol_encoder_star_line_serialize(
    RfStreamProtocolEncoderStarLine* instance,
    RfRecord* flipper_format) {
    rf_stream_protocol_star_line_check_remote_controller(
        &instance->generic, instance->keystore, &instance->manufacture_name);

    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    do {
        if(!rf_record_insert_or_update_string_cstr(
               flipper_format, FF_PROTOCOL, instance->generic.protocol_name)) {
            break;
        }

        uint32_t bits = instance->generic.data_count_bit;
        if(!rf_record_insert_or_update_uint32(flipper_format, FF_BIT, &bits, 1)) {
            break;
        }

        char key_str[20];
        snprintf(key_str, sizeof(key_str), "%016llX", instance->generic.data);
        if(!rf_record_insert_or_update_string_cstr(flipper_format, FF_KEY, key_str)) {
            break;
        }

        if(!rf_record_insert_or_update_uint32(
               flipper_format, FF_SERIAL, &instance->generic.serial, 1)) {
            break;
        }

        uint32_t temp = instance->generic.btn;
        if(!rf_record_insert_or_update_uint32(flipper_format, FF_BTN, &temp, 1)) {
            break;
        }

        if(!rf_record_insert_or_update_uint32(
               flipper_format, FF_CNT, &instance->generic.cnt, 1)) {
            break;
        }

        if(!rf_record_insert_or_update_string_cstr(
               flipper_format, FF_MANUFACTURE, instance->manufacture_name)) {
            break;
        }

        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
    //RfStreamProtocolStatus ret = rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
}

#endif
#if RF_STREAM_WITH_ENCODER

RfStreamProtocolStatus
    rf_stream_protocol_encoder_star_line_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderStarLine* instance = context;

    if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
       RfStreamProtocolStatusOk) {
        return RfStreamProtocolStatusError;
    }

    uint32_t bits = 0;
    if(rf_stream_encoder_read_bit(flipper_format, NULL, 0, &bits) != RfStreamProtocolStatusOk) {
        return RfStreamProtocolStatusErrorValueBitCount;
    }
    instance->generic.data_count_bit = rf_stream_protocol_star_line_const.min_count_bit_for_found;

    uint64_t key = 0;
    if(!rf_stream_flipper_read_hex_u64(flipper_format, FF_KEY, &key)) {
        return RfStreamProtocolStatusError;
    }
    instance->generic.data = key;
    if(instance->generic.data == 0) return RfStreamProtocolStatusError;

    uint32_t serial = UINT32_MAX;
    uint32_t btn = UINT32_MAX;
    uint32_t cnt = UINT32_MAX;
    rf_stream_encoder_read_fields(flipper_format, &serial, &btn, &cnt, NULL);
    if(serial == UINT32_MAX || btn == UINT32_MAX || cnt == UINT32_MAX) {
        return RfStreamProtocolStatusError;
    }
    instance->generic.serial = serial;
    instance->generic.btn = (uint8_t)btn;
    instance->generic.cnt = (uint16_t)cnt;

    instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, 40);

    rf_record_rewind(flipper_format);
    if(rf_record_read_string(
           flipper_format, FF_MANUFACTURE, instance->manufacture_from_file)) {
        instance->manufacture_name = rf_text_get_cstr(instance->manufacture_from_file);
        instance->keystore->mfname = instance->manufacture_name;
    }

    rf_stream_protocol_star_line_check_remote_controller(
        &instance->generic, instance->keystore, &instance->manufacture_name);

    rf_stream_protocol_encoder_star_line_get_upload(instance, instance->generic.btn);
    rf_stream_protocol_encoder_star_line_serialize(instance, flipper_format);

    instance->encoder.is_running = true;
    return RfStreamProtocolStatusOk;
}

#endif
void* rf_stream_protocol_decoder_star_line_alloc(RfStreamEnvironment* environment) {
    RfStreamProtocolDecoderStarLine* instance = malloc(sizeof(RfStreamProtocolDecoderStarLine));
    instance->base.protocol = &rf_stream_protocol_star_line;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->manufacture_from_file = rf_text_alloc();

    instance->keystore = rf_stream_environment_get_keystore(environment);

    return instance;
}

void rf_stream_protocol_decoder_star_line_free(void* context) {
    RfStreamProtocolDecoderStarLine* instance = context;
    rf_text_free(instance->manufacture_from_file);

    free(instance);
}

void rf_stream_protocol_decoder_star_line_reset(void* context) {
    RfStreamProtocolDecoderStarLine* instance = context;
    instance->decoder.parser_step = StarLineDecoderStepReset;
    // TODO
    instance->keystore->mfname = "";
    instance->keystore->kl_type = 0;
}

void rf_stream_protocol_decoder_star_line_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderStarLine* instance = context;

    switch(instance->decoder.parser_step) {
    case StarLineDecoderStepReset:
        if(level) {
            if(DURATION_DIFF(duration, rf_stream_protocol_star_line_const.te_long * 2) <
               rf_stream_protocol_star_line_const.te_delta * 2) {
                instance->decoder.parser_step = StarLineDecoderStepCheckPreambula;
                instance->header_count++;
            } else if(instance->header_count > 4) {
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->decoder.te_last = duration;
                instance->decoder.parser_step = StarLineDecoderStepCheckDuration;
            }
        } else {
            instance->header_count = 0;
        }
        break;
    case StarLineDecoderStepCheckPreambula:
        if((!level) && (DURATION_DIFF(duration, rf_stream_protocol_star_line_const.te_long * 2) <
                        rf_stream_protocol_star_line_const.te_delta * 2)) {
            //Found Preambula
            instance->decoder.parser_step = StarLineDecoderStepReset;
        } else {
            instance->header_count = 0;
            instance->decoder.parser_step = StarLineDecoderStepReset;
        }
        break;
    case StarLineDecoderStepSaveDuration:
        if(level) {
            if(duration >= (rf_stream_protocol_star_line_const.te_long +
                            rf_stream_protocol_star_line_const.te_delta)) {
                instance->decoder.parser_step = StarLineDecoderStepReset;
                if((instance->decoder.decode_count_bit >=
                    rf_stream_protocol_star_line_const.min_count_bit_for_found) &&
                   (instance->decoder.decode_count_bit <=
                    rf_stream_protocol_star_line_const.min_count_bit_for_found + 2)) {
                    if(instance->generic.data != instance->decoder.decode_data) {
                        instance->generic.data = instance->decoder.decode_data;
                        instance->generic.data_count_bit =
                            rf_stream_protocol_star_line_const.min_count_bit_for_found;
                        if(instance->base.callback)
                            instance->base.callback(&instance->base, instance->base.context);
                    }
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->header_count = 0;
                break;
            } else {
                instance->decoder.te_last = duration;
                instance->decoder.parser_step = StarLineDecoderStepCheckDuration;
            }

        } else {
            instance->decoder.parser_step = StarLineDecoderStepReset;
        }
        break;
    case StarLineDecoderStepCheckDuration:
        if(!level) {
            if((DURATION_DIFF(instance->decoder.te_last, rf_stream_protocol_star_line_const.te_short) <
                rf_stream_protocol_star_line_const.te_delta) &&
               (DURATION_DIFF(duration, rf_stream_protocol_star_line_const.te_short) <
                rf_stream_protocol_star_line_const.te_delta)) {
                if(instance->decoder.decode_count_bit <
                   rf_stream_protocol_star_line_const.min_count_bit_for_found) {
                    rf_stream_protocol_blocks_add_bit(&instance->decoder, 0);
                } else {
                    instance->decoder.decode_count_bit++;
                }
                instance->decoder.parser_step = StarLineDecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, rf_stream_protocol_star_line_const.te_long) <
                 rf_stream_protocol_star_line_const.te_delta) &&
                (DURATION_DIFF(duration, rf_stream_protocol_star_line_const.te_long) <
                 rf_stream_protocol_star_line_const.te_delta)) {
                if(instance->decoder.decode_count_bit <
                   rf_stream_protocol_star_line_const.min_count_bit_for_found) {
                    rf_stream_protocol_blocks_add_bit(&instance->decoder, 1);
                } else {
                    instance->decoder.decode_count_bit++;
                }
                instance->decoder.parser_step = StarLineDecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = StarLineDecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = StarLineDecoderStepReset;
        }
        break;
    }
}

/**
 * Validation of decrypt data.
 * @param instance Pointer to a RfStreamBlockGeneric instance
 * @param decrypt Decrypd data
 * @param btn Button number, 4 bit
 * @param end_serial decrement the last 10 bits of the serial number
 * @return true On success
 */
static inline bool rf_stream_protocol_star_line_check_decrypt(
    RfStreamBlockGeneric* instance,
    uint32_t decrypt,
    uint8_t btn,
    uint32_t end_serial) {
    if((decrypt >> 24 == btn) && ((((uint16_t)(decrypt >> 16)) & 0x00FF) == end_serial)) {
        instance->cnt = decrypt & 0x0000FFFF;
        return true;
    }
    return false;
}

/**
 * Checking the accepted code against the database manafacture key
 * @param instance Pointer to a RfStreamBlockGeneric* instance
 * @param fix Fix part of the parcel
 * @param hop Hop encrypted part of the parcel
 * @param keystore Pointer to a RfStreamKeystore* instance
 * @param manufacture_name
 * @return true on successful search
 */
static uint8_t rf_stream_protocol_star_line_check_remote_controller_selector(
    RfStreamBlockGeneric* instance,
    uint32_t fix,
    uint32_t hop,
    RfStreamKeystore* keystore,
    const char** manufacture_name) {
    uint16_t end_serial = (uint16_t)(fix & 0xFF);
    uint8_t btn = (uint8_t)(fix >> 24);
    uint32_t decrypt = 0;
    uint64_t man_normal_learning;
    bool mf_not_set = false;
    // TODO:
    // if(mfname == 0x0) {
    //     mfname = "";
    // }

    const char* mfname = keystore->mfname;

    if(strcmp(mfname, "Unknown") == 0) {
        return 1;
    } else if(strcmp(mfname, "") == 0) {
        mf_not_set = true;
    }
    for
        M_EACH(manufacture_code, *rf_stream_keystore_get_data(keystore), RfStreamKeyArray_t) {
            if(mf_not_set || (strcmp(rf_text_get_cstr(manufacture_code->name), mfname) == 0)) {
                switch(manufacture_code->type) {
                case KEELOQ_SIMPLE_LEARNING:
                    // Simple Learning
                    decrypt = keeloq_decrypt(hop, manufacture_code->key);
                    if(rf_stream_protocol_star_line_check_decrypt(
                           instance, decrypt, btn, end_serial)) {
                        *manufacture_name = rf_text_get_cstr(manufacture_code->name);
                        keystore->mfname = *manufacture_name;
                        return 1;
                    }
                    break;
                case KEELOQ_NORMAL_LEARNING:
                    // Normal Learning
                    // https://phreakerclub.com/forum/showpost.php?p=43557&postcount=37
                    man_normal_learning =
                        keeloq_normal_learning(fix, manufacture_code->key);
                    decrypt = keeloq_decrypt(hop, man_normal_learning);
                    if(rf_stream_protocol_star_line_check_decrypt(
                           instance, decrypt, btn, end_serial)) {
                        *manufacture_name = rf_text_get_cstr(manufacture_code->name);
                        keystore->mfname = *manufacture_name;
                        return 1;
                    }
                    break;
                case KEELOQ_UNKNOWN_LEARNING:
                    // Simple Learning
                    decrypt = keeloq_decrypt(hop, manufacture_code->key);
                    if(rf_stream_protocol_star_line_check_decrypt(
                           instance, decrypt, btn, end_serial)) {
                        *manufacture_name = rf_text_get_cstr(manufacture_code->name);
                        keystore->mfname = *manufacture_name;
                        keystore->kl_type = 1;
                        return 1;
                    }
                    // Check for mirrored man
                    uint64_t man_rev = 0;
                    uint64_t man_rev_byte = 0;
                    for(uint8_t i = 0; i < 64; i += 8) {
                        man_rev_byte = (uint8_t)(manufacture_code->key >> i);
                        man_rev = man_rev | man_rev_byte << (56 - i);
                    }
                    decrypt = keeloq_decrypt(hop, man_rev);
                    if(rf_stream_protocol_star_line_check_decrypt(
                           instance, decrypt, btn, end_serial)) {
                        *manufacture_name = rf_text_get_cstr(manufacture_code->name);
                        keystore->mfname = *manufacture_name;
                        keystore->kl_type = 1;
                        return 1;
                    }
                    //###########################
                    // Normal Learning
                    // https://phreakerclub.com/forum/showpost.php?p=43557&postcount=37
                    man_normal_learning =
                        keeloq_normal_learning(fix, manufacture_code->key);
                    decrypt = keeloq_decrypt(hop, man_normal_learning);
                    if(rf_stream_protocol_star_line_check_decrypt(
                           instance, decrypt, btn, end_serial)) {
                        *manufacture_name = rf_text_get_cstr(manufacture_code->name);
                        keystore->mfname = *manufacture_name;
                        keystore->kl_type = 2;
                        return 1;
                    }
                    // Check for mirrored man
                    man_normal_learning =
                        keeloq_normal_learning(fix, man_rev);
                    decrypt = keeloq_decrypt(hop, man_normal_learning);
                    if(rf_stream_protocol_star_line_check_decrypt(
                           instance, decrypt, btn, end_serial)) {
                        *manufacture_name = rf_text_get_cstr(manufacture_code->name);
                        keystore->mfname = *manufacture_name;
                        keystore->kl_type = 2;
                        return 1;
                    }
                    break;
                }
            }
        }

    *manufacture_name = "Unknown";
    keystore->mfname = "Unknown";
    instance->cnt = 0;

    return 0;
}

/**
 * Analysis of received data
 * @param instance Pointer to a RfStreamBlockGeneric* instance
 * @param keystore Pointer to a RfStreamKeystore* instance
 * @param manufacture_name
 */
static void rf_stream_protocol_star_line_check_remote_controller(
    RfStreamBlockGeneric* instance,
    RfStreamKeystore* keystore,
    const char** manufacture_name) {
    uint64_t key = rf_stream_protocol_blocks_reverse_key(instance->data, instance->data_count_bit);
    uint32_t key_fix = key >> 32;
    uint32_t key_hop = key & 0x00000000ffffffff;

    rf_stream_protocol_star_line_check_remote_controller_selector(
        instance, key_fix, key_hop, keystore, manufacture_name);

    instance->serial = key_fix & 0x00FFFFFF;
    instance->btn = key_fix >> 24;
}

RfStreamProtocolStatus rf_stream_protocol_decoder_star_line_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderStarLine* instance = context;
    rf_stream_protocol_star_line_check_remote_controller(
        &instance->generic, instance->keystore, &instance->manufacture_name);

    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    do {
        if(preset != NULL) {
            if(!rf_record_insert_or_update_uint32(
                   flipper_format, FF_FREQUENCY, &preset->frequency, 1)) {
                break;
            }

            const char* preset_name = rf_text_get_cstr(preset->name);
            const char* short_preset = rf_stream_get_short_preset_name(preset_name);
            if(!rf_record_insert_or_update_string_cstr(
                   flipper_format, FF_PRESET, short_preset)) {
                break;
            }
        }

        if(!rf_record_insert_or_update_string_cstr(
               flipper_format, FF_PROTOCOL, instance->generic.protocol_name)) {
            break;
        }

        uint32_t bits = instance->generic.data_count_bit;
        if(!rf_record_insert_or_update_uint32(flipper_format, FF_BIT, &bits, 1)) {
            break;
        }

        char key_str[20];
        snprintf(key_str, sizeof(key_str), "%016llX", instance->generic.data);
        if(!rf_record_insert_or_update_string_cstr(flipper_format, FF_KEY, key_str)) {
            break;
        }

        if(!rf_record_insert_or_update_uint32(
               flipper_format, FF_SERIAL, &instance->generic.serial, 1)) {
            break;
        }

        uint32_t temp = instance->generic.btn;
        if(!rf_record_insert_or_update_uint32(flipper_format, FF_BTN, &temp, 1)) {
            break;
        }

        if(!rf_record_insert_or_update_uint32(
               flipper_format, FF_CNT, &instance->generic.cnt, 1)) {
            break;
        }

        if(!rf_record_insert_or_update_string_cstr(
               flipper_format, FF_MANUFACTURE, instance->manufacture_name)) {
            break;
        }

        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
    //RfStreamProtocolStatus ret = rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_star_line_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderStarLine* instance = context;
    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    rf_record_rewind(flipper_format);

    do {
        if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
           RfStreamProtocolStatusOk) {
            RF_STREAM_LOG_E(TAG, "Missing or wrong Protocol");
            break;
        }

        uint32_t bit_count_temp;
        if(!rf_record_read_uint32(flipper_format, FF_BIT, &bit_count_temp, 1)) {
            RF_STREAM_LOG_E(TAG, "Missing Bit");
            break;
        }

        instance->generic.data_count_bit = rf_stream_protocol_star_line_const.min_count_bit_for_found;

        uint64_t key = 0;
        if(!rf_stream_flipper_read_hex_u64(flipper_format, FF_KEY, &key)) {
            RF_STREAM_LOG_E(TAG, "Missing Key");
            break;
        }

        instance->generic.data = key;
        RF_STREAM_LOG_I(TAG, "Parsed key: 0x%016llX", instance->generic.data);

        if(instance->generic.data == 0) {
            RF_STREAM_LOG_E(TAG, "Key is zero after parsing!");
            break;
        }

        if(!rf_record_read_uint32(flipper_format, FF_SERIAL, &instance->generic.serial, 1)) {
            RF_STREAM_LOG_E(TAG, "Missing Serial");
            break;
        }

        uint32_t btn_temp;
        if(!rf_record_read_uint32(flipper_format, FF_BTN, &btn_temp, 1)) {
            RF_STREAM_LOG_E(TAG, "Missing Btn");
            break;
        }
        instance->generic.btn = (uint8_t)btn_temp;

        uint32_t cnt_temp;
        if(!rf_record_read_uint32(flipper_format, FF_CNT, &cnt_temp, 1)) {
            RF_STREAM_LOG_E(TAG, "Missing Cnt");
            break;
        }
        instance->generic.cnt = (uint16_t)cnt_temp;

        if(!rf_record_rewind(flipper_format)) {
            RF_STREAM_LOG_E(TAG, "Rewind error");
            break;
        }
        // Read manufacturer from file
        if(rf_record_read_string(
               flipper_format, FF_MANUFACTURE, instance->manufacture_from_file)) {
            instance->manufacture_name = rf_text_get_cstr(instance->manufacture_from_file);
            instance->keystore->mfname = instance->manufacture_name;
        } else {
            RF_STREAM_LOG_D(TAG, "DECODER: Missing Manufacture");
        }

        RF_STREAM_LOG_I(TAG, "Decoder deserialized");

        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

void rf_stream_protocol_decoder_star_line_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderStarLine* instance = context;

    rf_stream_protocol_star_line_check_remote_controller(
        &instance->generic, instance->keystore, &instance->manufacture_name);

    uint32_t code_found_hi = instance->generic.data >> 32;
    uint32_t code_found_lo = instance->generic.data & 0x00000000ffffffff;

    uint64_t code_found_reverse = rf_stream_protocol_blocks_reverse_key(
        instance->generic.data, instance->generic.data_count_bit);
    uint32_t code_found_reverse_hi = code_found_reverse >> 32;
    uint32_t code_found_reverse_lo = code_found_reverse & 0x00000000ffffffff;

    rf_text_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%08lX%08lX\r\n"
        "Fix:0x%08lX    Cnt:%04lX\r\n"
        "Hop:0x%08lX    Btn:%02X\r\n"
        "MF:%s\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        code_found_hi,
        code_found_lo,
        code_found_reverse_hi,
        instance->generic.cnt,
        code_found_reverse_lo,
        instance->generic.btn,
        instance->manufacture_name);
}

#endif
