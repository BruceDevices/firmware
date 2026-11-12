// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define SUBGHZ_PROTOCOL_SCHER_KHAN_NAME "Scher-Khan"

typedef struct RfStreamProtocolDecoderScherKhan RfStreamProtocolDecoderScherKhan;

extern const RfStreamProtocolDecoder rf_stream_protocol_scher_khan_decoder;
extern const RfStreamProtocolEncoder rf_stream_protocol_scher_khan_encoder;
extern const RfStreamCodec rf_stream_protocol_scher_khan;

/**
 * Allocate RfStreamProtocolDecoderScherKhan.
 * @param environment Pointer to a RfStreamEnvironment instance
 * @return RfStreamProtocolDecoderScherKhan* pointer to a RfStreamProtocolDecoderScherKhan instance
 */
void* rf_stream_protocol_decoder_scher_khan_alloc(RfStreamEnvironment* environment);

/**
 * Free RfStreamProtocolDecoderScherKhan.
 * @param context Pointer to a RfStreamProtocolDecoderScherKhan instance
 */
void rf_stream_protocol_decoder_scher_khan_free(void* context);

/**
 * Reset decoder RfStreamProtocolDecoderScherKhan.
 * @param context Pointer to a RfStreamProtocolDecoderScherKhan instance
 */
void rf_stream_protocol_decoder_scher_khan_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a RfStreamProtocolDecoderScherKhan instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void rf_stream_protocol_decoder_scher_khan_feed(void* context, bool level, uint32_t duration);

/**
 * Serialize data RfStreamProtocolDecoderScherKhan.
 * @param context Pointer to a RfStreamProtocolDecoderScherKhan instance
 * @param flipper_format Pointer to a RfRecord instance
 * @param preset The modulation on which the signal was received, RfStreamRadioPreset
 * @return status
 */
RfStreamProtocolStatus rf_stream_protocol_decoder_scher_khan_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);

/**
 * Deserialize data RfStreamProtocolDecoderScherKhan.
 * @param context Pointer to a RfStreamProtocolDecoderScherKhan instance
 * @param flipper_format Pointer to a RfRecord instance
 * @return status
 */
RfStreamProtocolStatus
    rf_stream_protocol_decoder_scher_khan_deserialize(void* context, RfRecord* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a RfStreamProtocolDecoderScherKhan instance
 * @param output Resulting text
 */
void rf_stream_protocol_decoder_scher_khan_get_string(void* context, RfText* output);




//https://phreakerclub.com/72
//https://phreakerclub.com/forum/showthread.php?t=7&page=2
//https://phreakerclub.com/forum/showthread.php?t=274&highlight=magicar
//!!!  https://phreakerclub.com/forum/showthread.php?t=489&highlight=magicar&page=5

#define TAG "SubGhzProtocolScherKhan"

static const RfStreamBlockConst rf_stream_protocol_scher_khan_const = {
    .te_short = 750,
    .te_long = 1100,
    .te_delta = 160,
    .min_count_bit_for_found = 35,
};

struct RfStreamProtocolDecoderScherKhan {
    RfStreamProtocolDecoderBase base;

    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    uint16_t header_count;
    const char* protocol_name;
};

typedef enum {
    ScherKhanDecoderStepReset = 0,
    ScherKhanDecoderStepCheckPreambula,
    ScherKhanDecoderStepSaveDuration,
    ScherKhanDecoderStepCheckDuration,
} ScherKhanDecoderStep;

const RfStreamProtocolDecoder rf_stream_protocol_scher_khan_decoder = {
    .alloc = rf_stream_protocol_decoder_scher_khan_alloc,
    .free = rf_stream_decoder_free_default,

    .feed = rf_stream_protocol_decoder_scher_khan_feed,
    .reset = rf_stream_protocol_decoder_scher_khan_reset,

    .get_hash_data = rf_stream_decoder_hash_blocks,
    .serialize = rf_stream_protocol_decoder_scher_khan_serialize,
    .deserialize = rf_stream_protocol_decoder_scher_khan_deserialize,
    .get_string = rf_stream_protocol_decoder_scher_khan_get_string,
};

const RfStreamProtocolEncoder rf_stream_protocol_scher_khan_encoder = {
    .alloc = NULL,
    .free = NULL,

    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};

const RfStreamCodec rf_stream_protocol_scher_khan = {
    .name = SUBGHZ_PROTOCOL_SCHER_KHAN_NAME,

#if RF_STREAM_WITH_DECODER

    .decoder = &rf_stream_protocol_scher_khan_decoder,

#else

    .decoder = NULL,

#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_scher_khan_encoder,
#else
    .encoder = NULL,
#endif
};

void* rf_stream_protocol_decoder_scher_khan_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderScherKhan* instance = calloc(1, sizeof(RfStreamProtocolDecoderScherKhan));
    if(!instance) {
        return NULL;
    }
    instance->base.protocol = &rf_stream_protocol_scher_khan;
    instance->generic.protocol_name = instance->base.protocol->name;

    return instance;
}

void rf_stream_protocol_decoder_scher_khan_reset(void* context) {
    RfStreamProtocolDecoderScherKhan* instance = context;
    instance->decoder.parser_step = ScherKhanDecoderStepReset;
}

void rf_stream_protocol_decoder_scher_khan_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderScherKhan* instance = context;

    switch(instance->decoder.parser_step) {
    case ScherKhanDecoderStepReset:
        if((level) && (DURATION_DIFF(duration, rf_stream_protocol_scher_khan_const.te_short * 2) <
                       rf_stream_protocol_scher_khan_const.te_delta)) {
            instance->decoder.parser_step = ScherKhanDecoderStepCheckPreambula;
            instance->decoder.te_last = duration;
            instance->header_count = 0;
        }
        break;
    case ScherKhanDecoderStepCheckPreambula:
        if(level) {
            if((DURATION_DIFF(duration, rf_stream_protocol_scher_khan_const.te_short * 2) <
                rf_stream_protocol_scher_khan_const.te_delta) ||
               (DURATION_DIFF(duration, rf_stream_protocol_scher_khan_const.te_short) <
                rf_stream_protocol_scher_khan_const.te_delta)) {
                instance->decoder.te_last = duration;
            } else {
                instance->decoder.parser_step = ScherKhanDecoderStepReset;
            }
        } else if(
            (DURATION_DIFF(duration, rf_stream_protocol_scher_khan_const.te_short * 2) <
             rf_stream_protocol_scher_khan_const.te_delta) ||
            (DURATION_DIFF(duration, rf_stream_protocol_scher_khan_const.te_short) <
             rf_stream_protocol_scher_khan_const.te_delta)) {
            if(DURATION_DIFF(
                   instance->decoder.te_last, rf_stream_protocol_scher_khan_const.te_short * 2) <
               rf_stream_protocol_scher_khan_const.te_delta) {
                // Found header
                instance->header_count++;
                break;
            } else if(
                DURATION_DIFF(
                    instance->decoder.te_last, rf_stream_protocol_scher_khan_const.te_short) <
                rf_stream_protocol_scher_khan_const.te_delta) {
                // Found start bit
                if(instance->header_count >= 2) {
                    instance->decoder.parser_step = ScherKhanDecoderStepSaveDuration;
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 1;
                } else {
                    instance->decoder.parser_step = ScherKhanDecoderStepReset;
                }
            } else {
                instance->decoder.parser_step = ScherKhanDecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = ScherKhanDecoderStepReset;
        }
        break;
    case ScherKhanDecoderStepSaveDuration:
        if(level) {
            if(duration >= (rf_stream_protocol_scher_khan_const.te_delta * 2UL +
                            rf_stream_protocol_scher_khan_const.te_long)) {
                //Found stop bit
                instance->decoder.parser_step = ScherKhanDecoderStepReset;
                if(instance->decoder.decode_count_bit >=
                   rf_stream_protocol_scher_khan_const.min_count_bit_for_found) {
                    instance->generic.data = instance->decoder.decode_data;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                break;
            } else {
                instance->decoder.te_last = duration;
                instance->decoder.parser_step = ScherKhanDecoderStepCheckDuration;
            }

        } else {
            instance->decoder.parser_step = ScherKhanDecoderStepReset;
        }
        break;
    case ScherKhanDecoderStepCheckDuration:
        if(!level) {
            if((DURATION_DIFF(
                    instance->decoder.te_last, rf_stream_protocol_scher_khan_const.te_short) <
                rf_stream_protocol_scher_khan_const.te_delta) &&
               (DURATION_DIFF(duration, rf_stream_protocol_scher_khan_const.te_short) <
                rf_stream_protocol_scher_khan_const.te_delta)) {
                rf_stream_protocol_blocks_add_bit(&instance->decoder, 0);
                instance->decoder.parser_step = ScherKhanDecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(
                     instance->decoder.te_last, rf_stream_protocol_scher_khan_const.te_long) <
                 rf_stream_protocol_scher_khan_const.te_delta) &&
                (DURATION_DIFF(duration, rf_stream_protocol_scher_khan_const.te_long) <
                 rf_stream_protocol_scher_khan_const.te_delta)) {
                rf_stream_protocol_blocks_add_bit(&instance->decoder, 1);
                instance->decoder.parser_step = ScherKhanDecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = ScherKhanDecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = ScherKhanDecoderStepReset;
        }
        break;
    }
}

/**
 * Analysis of received data
 * @param instance Pointer to a RfStreamBlockGeneric* instance
 * @param protocol_name
 */
static void rf_stream_protocol_scher_khan_check_remote_controller(
    RfStreamBlockGeneric* instance,
    const char** protocol_name) {
    /*
    * MAGICAR 51 bit 00000001A99121DE83C3 MAGIC CODE, Dynamic
    * 0E8C1619E830C -> 000011101000110000010110 0001 1001 1110 1000001100001100
    * 0E8C1629D830D -> 000011101000110000010110 0010 1001 1101 1000001100001101
    * 0E8C1649B830E -> 000011101000110000010110 0100 1001 1011 1000001100001110
    * 0E8C16897830F -> 000011101000110000010110 1000 1001 0111 1000001100001111
    *                             Serial         Key  Ser ~Key   CNT
    */

    switch(instance->data_count_bit) {
    case 35: //MAGIC CODE, Static
        *protocol_name = "MAGIC CODE, Static";
        instance->serial = 0;
        instance->btn = 0;
        instance->cnt = 0;
        break;
    case 51: //MAGIC CODE, Dynamic
        *protocol_name = "MAGIC CODE, Dynamic";
        instance->serial = ((instance->data >> 24) & 0xFFFFFF0) | ((instance->data >> 20) & 0x0F);
        instance->btn = (instance->data >> 24) & 0x0F;
        instance->cnt = instance->data & 0xFFFF;
        break;
    case 57: //MAGIC CODE PRO / PRO2
        *protocol_name = "MAGIC CODE PRO/PRO2";
        instance->serial = 0;
        instance->btn = 0;
        instance->cnt = 0;
        break;
    case 63: //MAGIC CODE, Dynamic Response
        *protocol_name = "MAGIC CODE, Response";
        instance->serial = 0;
        instance->btn = 0;
        instance->cnt = 0;
        break;
    case 64: //MAGICAR, Response ???
        *protocol_name = "MAGICAR, Response";
        instance->serial = 0;
        instance->btn = 0;
        instance->cnt = 0;
        break;
    case 81: // MAGIC CODE PRO / PRO2 Response ???
    case 82: // MAGIC CODE PRO / PRO2 Response ???
        *protocol_name = "MAGIC CODE PRO,\n Response";
        instance->serial = 0;
        instance->btn = 0;
        instance->cnt = 0;
        break;

    default:
        *protocol_name = "Unknown";
        instance->serial = 0;
        instance->btn = 0;
        instance->cnt = 0;
        break;
    }
}

RfStreamProtocolStatus rf_stream_protocol_decoder_scher_khan_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderScherKhan* instance = context;
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

        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_scher_khan_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderScherKhan* instance = context;
    RfStreamProtocolStatus status =
        rf_stream_block_generic_deserialize(&instance->generic, flipper_format);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }
    if(instance->generic.data_count_bit <
           rf_stream_protocol_scher_khan_const.min_count_bit_for_found ||
       instance->generic.data_count_bit > 64U) {
        return RfStreamProtocolStatusErrorValueBitCount;
    }
    return status;
}

void rf_stream_protocol_decoder_scher_khan_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderScherKhan* instance = context;

    rf_stream_protocol_scher_khan_check_remote_controller(
        &instance->generic, &instance->protocol_name);

    rf_text_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:0x%lX%08lX\r\n"
        "Sn:%07lX Btn:%X\r\n"
        "Cntr:%04lX\r\n"
        "Pt: %s\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (uint32_t)(instance->generic.data >> 32),
        (uint32_t)instance->generic.data,
        instance->generic.serial,
        instance->generic.btn,
        instance->generic.cnt,
        instance->protocol_name);
}

#endif
