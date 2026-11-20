// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define FORD_PROTOCOL_V3_NAME "Ford V3"

extern const RfStreamCodec ford_protocol_v3;

void* rf_stream_protocol_decoder_ford_v3_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_ford_v3_reset(void* context);
void rf_stream_protocol_decoder_ford_v3_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_ford_v3_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_ford_v3_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_ford_v3_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_ford_v3_get_string(void* context, RfText* output);




#include <string.h>

#define FORD_V3_TE_SHORT      240U
#define FORD_V3_TE_LONG       480U
#define FORD_V3_TE_DELTA      60U
#define FORD_V3_CELL_TE_DELTA 120U
#define FORD_V3_DATA_BITS     104U
#define FORD_V3_DATA_BYTES    13U
#define FORD_V3_PREAMBLE_MIN  30U
#define FORD_V3_CELL_CAP      320U
#define FORD_V3_CELL_MIN      200U
#define FORD_V3_CELL_MIN_BITS 100U
#define FORD_V3_FF_VARIANT    "Variant"

#define FORD_V3_BTN_LOCK   0x01U
#define FORD_V3_BTN_UNLOCK 0x02U

#define FORD_V3_VARIANT_EU 0U
#define FORD_V3_VARIANT_US 1U

static const RfStreamBlockConst rf_stream_protocol_ford_v3_const = {
    .te_short = FORD_V3_TE_SHORT,
    .te_long = FORD_V3_TE_LONG,
    .te_delta = FORD_V3_TE_DELTA,
    .min_count_bit_for_found = FORD_V3_DATA_BITS,
};

static const RfStreamBlockConst rf_stream_protocol_ford_v3_cell_const = {
    .te_short = FORD_V3_TE_SHORT,
    .te_long = FORD_V3_TE_LONG,
    .te_delta = FORD_V3_CELL_TE_DELTA,
    .min_count_bit_for_found = FORD_V3_DATA_BITS,
};

typedef enum {
    FordV3DecoderStepReset = 0,
    FordV3DecoderStepPreamble = 1,
    FordV3DecoderStepData = 2,
} FordV3DecoderStep;

typedef struct RfStreamProtocolDecoderFordV3 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    RfManchesterState manchester_state;
    uint8_t manchester_raw[FORD_V3_DATA_BYTES];
    uint8_t manchester_bit_count;
    uint16_t preamble_count;

    uint8_t cells[FORD_V3_CELL_CAP];
    uint16_t cell_count;

    uint8_t raw_bytes[FORD_V3_DATA_BYTES];
    uint8_t last_raw_bytes[FORD_V3_DATA_BYTES];
    bool last_raw_valid;

    uint8_t variant;
    uint8_t flag;
    uint32_t serial;
    uint16_t counter;
} RfStreamProtocolDecoderFordV3;

static void ford_v3_reset_manchester(RfStreamProtocolDecoderFordV3* instance);
static void ford_v3_reset_cells(RfStreamProtocolDecoderFordV3* instance);
static void ford_v3_add_manchester_bit(RfStreamProtocolDecoderFordV3* instance, bool bit);
static bool ford_v3_cell_frame_valid(const uint8_t* raw);
static uint8_t ford_v3_variant_from_saved_or_raw(const uint8_t* raw, uint32_t saved_variant);
static void ford_v3_parse_fields(RfStreamProtocolDecoderFordV3* instance);
static bool ford_v3_commit_frame(
    RfStreamProtocolDecoderFordV3* instance,
    const uint8_t* raw,
    uint8_t variant);
static void ford_v3_manchester_emit_if_ready(RfStreamProtocolDecoderFordV3* instance);
static void ford_v3_cell_process(RfStreamProtocolDecoderFordV3* instance);
static void
    ford_v3_cell_feed(RfStreamProtocolDecoderFordV3* instance, bool level, uint32_t duration);
static void
    ford_v3_manchester_feed(RfStreamProtocolDecoderFordV3* instance, bool level, uint32_t duration);
static const char* ford_v3_button_name(uint8_t btn, uint8_t variant);

static const char* ford_v3_button_name(uint8_t btn, uint8_t variant) {
    if(variant == FORD_V3_VARIANT_US) {
        switch(btn) {
        case FORD_V3_BTN_LOCK:
            return "Lock";
        case FORD_V3_BTN_UNLOCK:
            return "Unlock";
        default:
            return "?";
        }
    }

    switch(btn) {
    case FORD_V3_BTN_LOCK:
        return "Lock";
    case FORD_V3_BTN_UNLOCK:
        return "Unlock";
    default:
        return "?";
    }
}

static bool ford_v3_cell_frame_valid(const uint8_t* raw) {
    if(raw[0] != 0xFFU) {
        return false;
    }

    const uint32_t serial = ((uint32_t)raw[1] << 24) | ((uint32_t)raw[2] << 16) |
                            ((uint32_t)raw[3] << 8) | (uint32_t)raw[4];
    if(serial == 0U || serial == 0xFFFFFFFFU) {
        return false;
    }

    if(raw[6] != FORD_V3_BTN_LOCK && raw[6] != FORD_V3_BTN_UNLOCK) {
        return false;
    }

    if((raw[5] & 0x80U) == 0U) {
        return false;
    }

    return true;
}

static uint8_t ford_v3_variant_from_saved_or_raw(const uint8_t* raw, uint32_t saved_variant) {
    if(saved_variant == FORD_V3_VARIANT_US) {
        return FORD_V3_VARIANT_US;
    }
    if(saved_variant == FORD_V3_VARIANT_EU) {
        return FORD_V3_VARIANT_EU;
    }

    return ford_v3_cell_frame_valid(raw) ? FORD_V3_VARIANT_US : FORD_V3_VARIANT_EU;
}

static void ford_v3_reset_manchester(RfStreamProtocolDecoderFordV3* instance) {
    memset(instance->manchester_raw, 0, sizeof(instance->manchester_raw));
    instance->manchester_bit_count = 0;
    instance->preamble_count = 0;
    rf_manchester_advance(
        instance->manchester_state, RfManchesterEventReset, &instance->manchester_state, NULL);
}

static void ford_v3_reset_cells(RfStreamProtocolDecoderFordV3* instance) {
    instance->cell_count = 0;
}

static void ford_v3_add_manchester_bit(RfStreamProtocolDecoderFordV3* instance, bool bit) {
    if(instance->manchester_bit_count >= FORD_V3_DATA_BITS) {
        return;
    }

    const uint8_t byte_index = instance->manchester_bit_count / 8U;
    const uint8_t bit_in_byte = 7U - (instance->manchester_bit_count % 8U);
    if(bit) {
        instance->manchester_raw[byte_index] |= (uint8_t)(1U << bit_in_byte);
    }
    instance->manchester_bit_count++;
}

static void ford_v3_parse_fields(RfStreamProtocolDecoderFordV3* instance) {
    const uint8_t* b = instance->raw_bytes;

    instance->serial = ((uint32_t)b[1] << 24) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 8) |
                       (uint32_t)b[4];
    instance->generic.serial = instance->serial;

    if(instance->variant == FORD_V3_VARIANT_US) {
        instance->flag = b[5];
        instance->counter = (uint16_t)(((uint16_t)b[7] << 8) | (uint16_t)b[8]);
        instance->generic.btn = b[6];
    } else {
        instance->flag = 0;
        instance->counter = (uint16_t)((((uint16_t)(uint8_t)~b[7]) << 8) | (uint8_t)~b[8]);
        instance->generic.btn = (b[6] & 0x01U) ? FORD_V3_BTN_UNLOCK : FORD_V3_BTN_LOCK;
    }

    instance->generic.cnt = instance->counter;
}

static bool ford_v3_commit_frame(
    RfStreamProtocolDecoderFordV3* instance,
    const uint8_t* raw,
    uint8_t variant) {
    if(instance->last_raw_valid &&
       memcmp(instance->last_raw_bytes, raw, FORD_V3_DATA_BYTES) == 0) {
        return true;
    }

    memcpy(instance->raw_bytes, raw, FORD_V3_DATA_BYTES);
    memcpy(instance->last_raw_bytes, raw, FORD_V3_DATA_BYTES);
    instance->last_raw_valid = true;

    instance->variant = variant;
    instance->generic.data_count_bit = FORD_V3_DATA_BITS;
    ford_v3_parse_fields(instance);

    if(instance->base.callback) {
        instance->base.callback(&instance->base, instance->base.context);
    }

    return true;
}

static void ford_v3_manchester_emit_if_ready(RfStreamProtocolDecoderFordV3* instance) {
    if(instance->manchester_bit_count < FORD_V3_DATA_BITS) {
        return;
    }

    (void)ford_v3_commit_frame(instance, instance->manchester_raw, FORD_V3_VARIANT_EU);
}

static bool ford_v3_cell_decode(const uint8_t* cells, uint16_t cell_count, uint8_t* raw_out) {
    for(int phase = 0; phase < 2; phase++) {
        uint8_t frame[FORD_V3_DATA_BYTES];
        memset(frame, 0, sizeof(frame));

        int bit_count = 0;
        bool ok = true;
        for(int i = phase; (i + 1) < (int)cell_count && bit_count < (int)FORD_V3_DATA_BITS;
            i += 2) {
            const uint8_t first = cells[i];
            const uint8_t second = cells[i + 1];
            if(first == second) {
                ok = false;
                break;
            }
            if(first) {
                frame[bit_count >> 3] |= (uint8_t)(1U << (7 - (bit_count & 7)));
            }
            bit_count++;
        }

        if(!ok || bit_count < (int)FORD_V3_CELL_MIN_BITS) {
            continue;
        }
        if(!ford_v3_cell_frame_valid(frame)) {
            continue;
        }

        memcpy(raw_out, frame, FORD_V3_DATA_BYTES);
        return true;
    }

    return false;
}

static void ford_v3_cell_process(RfStreamProtocolDecoderFordV3* instance) {
    if(instance->cell_count < FORD_V3_CELL_MIN) {
        return;
    }

    uint8_t raw[FORD_V3_DATA_BYTES];
    if(!ford_v3_cell_decode(instance->cells, instance->cell_count, raw)) {
        return;
    }

    (void)ford_v3_commit_frame(instance, raw, FORD_V3_VARIANT_US);
}

static void
    ford_v3_cell_feed(RfStreamProtocolDecoderFordV3* instance, bool level, uint32_t duration) {
    if(rf_stream_is_short(duration, &rf_stream_protocol_ford_v3_cell_const)) {
        if(instance->cell_count < FORD_V3_CELL_CAP) {
            instance->cells[instance->cell_count++] = level ? 1U : 0U;
        }
    } else if(rf_stream_is_long(duration, &rf_stream_protocol_ford_v3_cell_const)) {
        if(instance->cell_count + 2U <= FORD_V3_CELL_CAP) {
            instance->cells[instance->cell_count++] = level ? 1U : 0U;
            instance->cells[instance->cell_count++] = level ? 1U : 0U;
        }
    } else {
        ford_v3_cell_process(instance);
        instance->cell_count = 0;
    }
}

static void
    ford_v3_manchester_feed(RfStreamProtocolDecoderFordV3* instance, bool level, uint32_t duration) {
    switch(instance->decoder.parser_step) {
    case FordV3DecoderStepReset:
        if(rf_stream_is_short(duration, &rf_stream_protocol_ford_v3_const)) {
            ford_v3_reset_manchester(instance);
            instance->preamble_count = 1U;
            instance->decoder.parser_step = FordV3DecoderStepPreamble;
        }
        break;

    case FordV3DecoderStepPreamble:
        if(rf_stream_is_short(duration, &rf_stream_protocol_ford_v3_const)) {
            instance->preamble_count++;
        } else if(
            instance->preamble_count >= FORD_V3_PREAMBLE_MIN &&
            rf_stream_is_long(duration, &rf_stream_protocol_ford_v3_const)) {
            instance->manchester_state = RfManchesterStateMid1;

            const RfManchesterEvent event = level ? RfManchesterEventLongHigh : RfManchesterEventLongLow;

            bool data_bit = false;
            const bool valid = rf_manchester_advance(
                instance->manchester_state, event, &instance->manchester_state, &data_bit);
            if(valid) {
                ford_v3_add_manchester_bit(instance, data_bit);
            }
            instance->decoder.parser_step = FordV3DecoderStepData;
        } else {
            instance->decoder.parser_step = FordV3DecoderStepReset;
        }
        break;

    case FordV3DecoderStepData:
        if(!rf_stream_is_short(duration, &rf_stream_protocol_ford_v3_const) &&
           !rf_stream_is_long(duration, &rf_stream_protocol_ford_v3_const)) {
            ford_v3_manchester_emit_if_ready(instance);
            instance->decoder.parser_step = FordV3DecoderStepReset;

            if(rf_stream_is_short(duration, &rf_stream_protocol_ford_v3_const)) {
                ford_v3_reset_manchester(instance);
                instance->preamble_count = 1U;
                instance->decoder.parser_step = FordV3DecoderStepPreamble;
            }
            break;
        }

        RfManchesterEvent event;
        if(level) {
            event = rf_stream_is_short(duration, &rf_stream_protocol_ford_v3_const) ?
                        RfManchesterEventShortHigh :
                        RfManchesterEventLongHigh;
        } else {
            event = rf_stream_is_short(duration, &rf_stream_protocol_ford_v3_const) ?
                        RfManchesterEventShortLow :
                        RfManchesterEventLongLow;
        }

        bool data_bit = false;
        const bool valid = rf_manchester_advance(
            instance->manchester_state, event, &instance->manchester_state, &data_bit);

        if(valid) {
            ford_v3_add_manchester_bit(instance, data_bit);
            if(instance->manchester_bit_count >= FORD_V3_DATA_BITS) {
                ford_v3_manchester_emit_if_ready(instance);
                instance->decoder.parser_step = FordV3DecoderStepReset;
            }
        }
        break;
    }
}

void* rf_stream_protocol_decoder_ford_v3_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);

    RfStreamProtocolDecoderFordV3* instance = calloc(1, sizeof(RfStreamProtocolDecoderFordV3));

    instance->base.protocol = &ford_protocol_v3;
    instance->generic.protocol_name = instance->base.protocol->name;

    return instance;
}

void rf_stream_protocol_decoder_ford_v3_reset(void* context) {
    RfStreamProtocolDecoderFordV3* instance = context;
    instance->decoder.parser_step = FordV3DecoderStepReset;
    ford_v3_reset_manchester(instance);
    ford_v3_reset_cells(instance);
    instance->last_raw_valid = false;
}

void rf_stream_protocol_decoder_ford_v3_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderFordV3* instance = context;
    ford_v3_cell_feed(instance, level, duration);
    ford_v3_manchester_feed(instance, level, duration);
}

uint8_t rf_stream_protocol_decoder_ford_v3_get_hash_data(void* context) {
    RfStreamProtocolDecoderFordV3* instance = context;
    uint8_t hash = 0;

    for(size_t i = 0; i < FORD_V3_DATA_BYTES; i++) {
        hash ^= instance->raw_bytes[i];
    }

    return hash;
}

RfStreamProtocolStatus rf_stream_protocol_decoder_ford_v3_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderFordV3* instance = context;

    instance->generic.data =
        ((uint64_t)instance->raw_bytes[0] << 56) | ((uint64_t)instance->raw_bytes[1] << 48) |
        ((uint64_t)instance->raw_bytes[2] << 40) | ((uint64_t)instance->raw_bytes[3] << 32) |
        ((uint64_t)instance->raw_bytes[4] << 24) | ((uint64_t)instance->raw_bytes[5] << 16) |
        ((uint64_t)instance->raw_bytes[6] << 8) | (uint64_t)instance->raw_bytes[7];
    instance->generic.data_count_bit = FORD_V3_DATA_BITS;

    RfStreamProtocolStatus ret =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);

    if(ret == RfStreamProtocolStatusOk) {
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_hex(
            flipper_format, "Raw", instance->raw_bytes, FORD_V3_DATA_BYTES);

        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_SERIAL, instance->generic.serial);
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_BTN, instance->generic.btn);
        rf_stream_flipper_update_or_insert_u32(flipper_format, FF_CNT, instance->counter);
        rf_stream_flipper_update_or_insert_u32(flipper_format, FORD_V3_FF_VARIANT, instance->variant);
    }

    return ret;
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_ford_v3_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderFordV3* instance = context;

    RfStreamProtocolStatus ret = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, rf_stream_protocol_ford_v3_const.min_count_bit_for_found);

    if(ret != RfStreamProtocolStatusOk) {
        return ret;
    }

    const uint64_t d = instance->generic.data;
    for(uint8_t i = 0; i < 8U; i++) {
        instance->raw_bytes[i] = (uint8_t)(d >> (56 - i * 8));
    }
    memset(&instance->raw_bytes[8], 0, FORD_V3_DATA_BYTES - 8U);

    rf_record_rewind(flipper_format);
    rf_record_read_hex(flipper_format, "Raw", instance->raw_bytes, FORD_V3_DATA_BYTES);

    uint32_t variant = UINT32_MAX;
    if(!rf_record_read_uint32(flipper_format, FORD_V3_FF_VARIANT, &variant, 1)) {
        variant = UINT32_MAX;
    }
    instance->variant = ford_v3_variant_from_saved_or_raw(instance->raw_bytes, variant);

    instance->manchester_bit_count = FORD_V3_DATA_BITS;
    ford_v3_parse_fields(instance);

    return ret;
}

void rf_stream_protocol_decoder_ford_v3_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderFordV3* instance = context;
    const uint8_t* k = instance->raw_bytes;

    if(instance->variant == FORD_V3_VARIANT_US) {
        rf_text_cat_printf(
            output,
            "%s US %dbit\r\n"
            "Key:%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n"
            "Sn:%08lX Btn:%02X %s\r\n"
            "Cnt:%04X Hop:%02X%02X%02X%02X\r\n",
            instance->generic.protocol_name,
            (int)instance->generic.data_count_bit,
            k[0],
            k[1],
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
            ford_v3_button_name(instance->generic.btn, FORD_V3_VARIANT_US),
            (unsigned)instance->counter,
            k[9],
            k[10],
            k[11],
            k[12]);
        return;
    }

    rf_text_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n"
        "Sn:%08lX Btn:%02X %s\r\n"
        "Cnt:%04X Hop:%02X%02X%02X%02X\r\n",
        instance->generic.protocol_name,
        (int)instance->generic.data_count_bit,
        k[0],
        k[1],
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
        ford_v3_button_name(instance->generic.btn, FORD_V3_VARIANT_EU),
        (unsigned)instance->counter,
        k[9],
        k[10],
        k[11],
        k[12]);
}

const RfStreamProtocolDecoder rf_stream_protocol_ford_v3_decoder = {
    .alloc = rf_stream_protocol_decoder_ford_v3_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_ford_v3_feed,
    .reset = rf_stream_protocol_decoder_ford_v3_reset,
    .get_hash_data = rf_stream_protocol_decoder_ford_v3_get_hash_data,
    .serialize = rf_stream_protocol_decoder_ford_v3_serialize,
    .deserialize = rf_stream_protocol_decoder_ford_v3_deserialize,
    .get_string = rf_stream_protocol_decoder_ford_v3_get_string,
};

const RfStreamProtocolEncoder rf_stream_protocol_ford_v3_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};

const RfStreamCodec ford_protocol_v3 = {
    .name = FORD_PROTOCOL_V3_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_ford_v3_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_ford_v3_encoder,
#else
    .encoder = NULL,
#endif
};

#endif
