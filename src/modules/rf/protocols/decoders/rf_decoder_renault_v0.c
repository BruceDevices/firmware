// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define RENAULT_PROTOCOL_V0_NAME "Renault V0"

extern const RfStreamCodec renault_v0_protocol;

void* rf_stream_protocol_decoder_renault_v0_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_renault_v0_reset(void* context);
void rf_stream_protocol_decoder_renault_v0_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_renault_v0_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_renault_v0_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_renault_v0_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_renault_v0_get_string(void* context, RfText* output);

void* rf_stream_protocol_encoder_renault_v0_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_renault_v0_deserialize(void* context, RfRecord* flipper_format);



#define RENAULT_V0_MIN_BITS          0x52U
#define RENAULT_V0_DECODER_BIT_LIMIT 0x6DU
#define RENAULT_V0_SYNC_MIN_US       0x320U
#define RENAULT_V0_DECODED_BITS_MAX  0x70U
#define RENAULT_V0_UPLOAD_CAPACITY   0x258U
_Static_assert(
    RENAULT_V0_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "RENAULT_V0_UPLOAD_CAPACITY exceeds shared upload slab");
#define RENAULT_V0_TE_SHORT_US    0x7DU
#define RENAULT_V0_TE_LONG_US     0xFAU
#define RENAULT_V0_TE_DELTA_US    0x45U
#define RENAULT_V0_PREAMBLE_PAIRS 16U
#define RENAULT_V0_BURST_COUNT    3U
#define RENAULT_V0_INTER_BURST_US 0x61A8U
#define RENAULT_V0_FINAL_LOW_US   250U
#define RENAULT_V0_SYNC_HIGH_US   1000U
#define RENAULT_V0_REPEAT         1U
#define RENAULT_V0_KEY2_FIELD     "Key2"

typedef enum {
    RenaultV0DecoderStepReset = 0,
    RenaultV0DecoderStepData = 1,
} RenaultV0DecoderStep;

typedef struct {
    uint32_t low;
    uint32_t high;
} RenaultV0MatrixRow;

static const RenaultV0MatrixRow renault_v0_matrix[42] = {
    {0x00000001, 0x00000000}, {0x04000029, 0x00000000}, {0x0000001B, 0x00000000},
    {0x00000000, 0x00000000}, {0x00000001, 0x00000000}, {0x05220124, 0x00000000},
    {0x00000001, 0x00000000}, {0x00088410, 0x00000000}, {0x60132D1D, 0x00000000},
    {0x60170F87, 0x00001004}, {0x00000000, 0x00000000}, {0x002000A9, 0x00000000},
    {0x20863E01, 0x0000100C}, {0x24BB3755, 0x00000004}, {0x640199A4, 0x00000004},
    {0x24225C43, 0x00001004}, {0x607886F1, 0x0000100C}, {0x6007A101, 0x0000000C},
    {0x66672A10, 0x00000004}, {0x4651623F, 0x00001008}, {0x43380BBF, 0x00001008},
    {0x20237F84, 0x00001000}, {0x4245755E, 0x00001008}, {0x60AAF581, 0x00000004},
    {0x22722DAD, 0x0000000C}, {0x27C617F7, 0x00000000}, {0x46DE8F1B, 0x0000000C},
    {0x231DEC51, 0x00000000}, {0x03ACAA0B, 0x00000008}, {0x22D2BF81, 0x00000004},
    {0x626EF6AE, 0x0000100C}, {0x40441F95, 0x0000000C}, {0x00000001, 0x00000000},
    {0x00000000, 0x00000000}, {0x20B9A590, 0x00000008}, {0x656C8E86, 0x00001008},
    {0x60129F96, 0x0000000C}, {0x2368F667, 0x00001000}, {0x442A1A5C, 0x00000000},
    {0x04C43242, 0x0000100C}, {0x22198640, 0x00001000}, {0x23D6B958, 0x00001008},
};

static const uint8_t renault_v0_decoder_state_table[4] = {0x01, 0x91, 0x9B, 0xFB};

typedef struct RfStreamProtocolDecoderRenaultV0 {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;

    uint16_t packet_bit_count;
    uint8_t check_c1;
    uint8_t check_c2;
    uint8_t check_ic;
    uint32_t key2;
    uint8_t manchester_state;
    uint8_t decoded_bits[RENAULT_V0_DECODED_BITS_MAX];
    uint8_t decoded_bit_count;
} RfStreamProtocolDecoderRenaultV0;

#if RF_STREAM_WITH_ENCODER
typedef struct RfStreamProtocolEncoderRenaultV0 {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;
    uint32_t key2;
} RfStreamProtocolEncoderRenaultV0;
#endif

static void renault_v0_u64_to_bytes_be(uint64_t data, uint8_t bytes[8]) {
    for(size_t j = 0; j < 8; j++) {
        bytes[j] = (uint8_t)((data >> ((7U - j) * 8U)) & 0xFFU);
    }
}

static void
    renault_v0_parse_fields(uint64_t data, uint32_t* serial, uint8_t* button, uint8_t* counter) {
    if(serial) {
        *serial = (uint32_t)(data >> 40U);
    }
    if(button) {
        *button = (uint8_t)(data >> 32U);
    }
    if(counter) {
        *counter = (uint8_t)(((uint32_t)data >> 24U) & 0xFFU);
    }
}

static bool renault_v0_button_valid(uint8_t button) {
    return (button == 0x05U) || (button == 0x06U) || (button == 0x0AU);
}

static const char* renault_v0_get_button_name(uint8_t button) {
    switch(button) {
    case 0x05:
        return "Trunk";
    case 0x06:
        return "Lock";
    case 0x0A:
        return "Unlock";
    default:
        return "?";
    }
}

static uint8_t renault_v0_checksum(uint64_t data, uint32_t key2) {
    uint8_t bytes[10];
    renault_v0_u64_to_bytes_be(data, bytes);
    bytes[8] = (uint8_t)((key2 >> 10U) & 0xFFU);
    bytes[9] = (uint8_t)((key2 >> 2U) & 0xFFU);

    uint8_t checksum = 0U;
    for(size_t i = 0; i < COUNT_OF(bytes); i++) {
        checksum ^= bytes[i];
    }
    return checksum;
}

static void renault_v0_set_split_bit(uint32_t* low, uint32_t* high, uint8_t bit) {
    if(bit < 32U) {
        *low |= (1UL << bit);
    } else {
        *high |= (1UL << (bit - 32U));
    }
}

static uint8_t renault_v0_parity32(uint32_t value) {
    value ^= value >> 16U;
    value ^= value >> 8U;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (uint8_t)(value & 1U);
}

static void renault_v0_build_key(
    uint32_t serial,
    uint8_t button,
    uint8_t counter,
    uint64_t* out_data,
    uint32_t* out_key2) {
    uint8_t vars[7];
    uint8_t parity_bits[42];

    vars[0] = (button == 0x0AU) ? 1U : 0U;
    for(uint8_t bit = 0; bit < 6U; bit++) {
        vars[bit + 1U] = (counter >> bit) & 1U;
    }

    uint32_t mask_low = 1U;
    uint32_t mask_high = 0U;
    uint8_t mask_bit = 1U;

    for(uint8_t i = 0; i < 7U; i++, mask_bit++) {
        if(vars[i]) {
            renault_v0_set_split_bit(&mask_low, &mask_high, mask_bit);
        }
    }

    for(uint8_t i = 0; i < 6U; i++) {
        for(uint8_t j = i + 1U; j < 7U; j++, mask_bit++) {
            if(vars[i] & vars[j]) {
                renault_v0_set_split_bit(&mask_low, &mask_high, mask_bit);
            }
        }
    }

    for(uint8_t i = 0; i < 6U; i++) {
        for(uint8_t j = i + 1U; j < 7U; j++) {
            for(uint8_t k = j + 1U; k < 7U; k++, mask_bit++) {
                if(vars[i] & vars[j] & vars[k]) {
                    renault_v0_set_split_bit(&mask_low, &mask_high, mask_bit);
                }
            }
        }
    }

    for(size_t row = 0; row < COUNT_OF(renault_v0_matrix); row++) {
        const uint32_t mixed = (renault_v0_matrix[row].low & mask_low) ^
                               (renault_v0_matrix[row].high & mask_high);
        parity_bits[row] = renault_v0_parity32(mixed);
    }

    if(counter & 0x40U) {
        parity_bits[41] ^= 1U;
    }
    if((counter >> 7U) != 0U) {
        parity_bits[40] ^= 1U;
    }

    uint32_t data_low = ((uint32_t)counter) << 24U;
    uint32_t data_high = (serial << 8U) | (uint32_t)button;

    for(uint8_t i = 0; i < 24U; i++) {
        if(parity_bits[i]) {
            data_low |= 1UL << (23U - i);
        }
    }

    uint32_t key2 = 0U;
    for(uint8_t i = 24U; i < 42U; i++) {
        if(parity_bits[i]) {
            key2 |= 1UL << (41U - i);
        }
    }

    if(out_data) {
        *out_data = ((uint64_t)data_high << 32U) | data_low;
    }
    if(out_key2) {
        *out_key2 = key2;
    }
}

static bool renault_v0_model_matches(
    uint64_t data,
    uint32_t key2,
    uint32_t serial,
    uint8_t button,
    uint8_t counter) {
    uint64_t rebuilt_data = 0ULL;
    uint32_t rebuilt_key2 = 0U;
    renault_v0_build_key(serial, button, counter, &rebuilt_data, &rebuilt_key2);
    return (rebuilt_data == data) && (rebuilt_key2 == key2);
}

static void
    renault_v0_update_checks(uint64_t data, uint32_t key2, uint8_t* c1, uint8_t* c2, uint8_t* ic) {
    uint32_t serial = 0U;
    uint8_t button = 0U;
    uint8_t counter = 0U;
    renault_v0_parse_fields(data, &serial, &button, &counter);

    const uint8_t checksum = renault_v0_checksum(data, key2);
    if(c1) {
        *c1 = ((checksum & 0x3FU) == 0x13U) ? 0U : 1U;
    }
    if(c2) {
        *c2 = (((key2 & 0x03U) == (uint32_t)(checksum >> 6U))) ? 0U : 1U;
    }
    if(ic) {
        *ic = renault_v0_model_matches(data, key2, serial, button, counter) ? 0U : 1U;
    }
}

static bool renault_v0_type13_valid(uint64_t data, uint32_t key2) {
    uint8_t button = (uint8_t)(data >> 32U);
    const uint8_t checksum = renault_v0_checksum(data, key2);
    if((checksum & 0x3FU) != 0x13U) {
        return false;
    }
    if((key2 & 0x03U) != (uint32_t)(checksum >> 6U)) {
        return false;
    }
    return renault_v0_button_valid(button);
}

#if RF_STREAM_WITH_ENCODER
static bool renault_v0_emit(
    RfStreamProtocolEncoderRenaultV0* instance,
    size_t* index,
    bool level,
    uint32_t duration) {
    const size_t prev = *index;
    *index =
        rf_stream_emit_merge(instance->encoder.upload, prev, RENAULT_V0_UPLOAD_CAPACITY, level, duration);
    if(*index > prev) {
        return true;
    }
    return (prev > 0U) && (level_duration_get_level(instance->encoder.upload[prev - 1U]) == level);
}

static bool renault_v0_emit_decoded_bit(
    RfStreamProtocolEncoderRenaultV0* instance,
    size_t* index,
    uint8_t* state,
    bool bit) {
    if(*state == 1U) {
        if(bit) {
            return renault_v0_emit(instance, index, false, RENAULT_V0_TE_SHORT_US) &&
                   renault_v0_emit(instance, index, true, RENAULT_V0_TE_SHORT_US);
        }
        *state = 2U;
        return renault_v0_emit(instance, index, false, RENAULT_V0_TE_LONG_US);
    }

    if(bit) {
        *state = 1U;
        return renault_v0_emit(instance, index, true, RENAULT_V0_TE_LONG_US);
    }

    return renault_v0_emit(instance, index, true, RENAULT_V0_TE_SHORT_US) &&
           renault_v0_emit(instance, index, false, RENAULT_V0_TE_SHORT_US);
}

static bool renault_v0_get_bit_msb82(uint64_t data, uint32_t key2, uint8_t bit_index) {
    if(bit_index <= 0x3FU) {
        return ((data >> (63U - bit_index)) & 1ULL) != 0ULL;
    }
    return ((key2 >> (0x51U - bit_index)) & 1U) != 0U;
}

static bool renault_v0_build_upload(RfStreamProtocolEncoderRenaultV0* instance) {
    size_t write_index = 0U;
    for(uint8_t burst = 0U; burst < RENAULT_V0_BURST_COUNT; burst++) {
        if(!renault_v0_emit(instance, &write_index, true, RENAULT_V0_SYNC_HIGH_US)) {
            return false;
        }

        uint8_t state = 1U;
        for(uint8_t pair = 0U; pair < RENAULT_V0_PREAMBLE_PAIRS; pair++) {
            if(!renault_v0_emit_decoded_bit(instance, &write_index, &state, true)) {
                return false;
            }
        }

        for(uint8_t bit_index = 0U; bit_index < RENAULT_V0_MIN_BITS; bit_index++) {
            const bool bit =
                renault_v0_get_bit_msb82(instance->generic.data, instance->key2, bit_index);
            if(!renault_v0_emit_decoded_bit(instance, &write_index, &state, bit)) {
                return false;
            }
        }

        if(state == 2U) {
            if(!renault_v0_emit(instance, &write_index, true, RENAULT_V0_TE_SHORT_US)) {
                return false;
            }
        }

        const uint32_t trailing_low = (burst + 1U < RENAULT_V0_BURST_COUNT) ?
                                          RENAULT_V0_INTER_BURST_US :
                                          RENAULT_V0_FINAL_LOW_US;
        if(!renault_v0_emit(instance, &write_index, false, trailing_low)) {
            return false;
        }
    }

    instance->encoder.size_upload = write_index;
    instance->encoder.front = 0U;
    return write_index > 0U;
}
#endif

static bool renault_v0_classify_event(uint32_t duration, bool level, uint8_t* event_code) {
    if(duration <= (RENAULT_V0_TE_SHORT_US - 1U)) {
        if((RENAULT_V0_TE_SHORT_US - duration) > RENAULT_V0_TE_DELTA_US) {
            return false;
        }
        *event_code = (uint8_t)(((level ? 1U : 0U) ^ 1U) << 1U);
        return true;
    }

    if(duration <= (RENAULT_V0_TE_LONG_US - 1U)) {
        const uint32_t short_delta = duration - RENAULT_V0_TE_SHORT_US;
        const uint32_t long_inv_delta = RENAULT_V0_TE_LONG_US - duration;
        if(short_delta <= RENAULT_V0_TE_DELTA_US) {
            if(long_inv_delta > RENAULT_V0_TE_DELTA_US) {
                *event_code = (uint8_t)(((level ? 1U : 0U) ^ 1U) << 1U);
            } else {
                *event_code = level ? 4U : 6U;
            }
            return true;
        }
        if(long_inv_delta <= RENAULT_V0_TE_DELTA_US) {
            *event_code = level ? 4U : 6U;
            return true;
        }
        return false;
    }

    if((duration - RENAULT_V0_TE_LONG_US) > RENAULT_V0_TE_DELTA_US) {
        return false;
    }
    *event_code = level ? 4U : 6U;
    return true;
}

static void renault_v0_decode_candidate(RfStreamProtocolDecoderRenaultV0* instance) {
    const uint8_t bit_count = instance->decoded_bit_count;
    if(bit_count <= 0x51U) {
        return;
    }

    uint8_t preamble = 0U;
    while((preamble < bit_count) && (instance->decoded_bits[preamble] == 1U)) {
        preamble++;
    }
    if(preamble <= 9U) {
        return;
    }
    if((uint8_t)(bit_count - preamble) <= 0x51U) {
        return;
    }

    uint64_t data = 0ULL;
    for(uint8_t i = 0; i < 64U; i++) {
        data = (data << 1U) | (uint64_t)(instance->decoded_bits[preamble + i] & 1U);
    }

    uint32_t key2 = 0U;
    for(uint8_t i = 0; i < 18U; i++) {
        key2 = (key2 << 1U) | (uint32_t)(instance->decoded_bits[preamble + 64U + i] & 1U);
    }

    if(!renault_v0_type13_valid(data, key2)) {
        instance->packet_bit_count = 0U;
        instance->generic.data_count_bit = 0U;
        return;
    }

    uint32_t serial = 0U;
    uint8_t button = 0U;
    uint8_t counter = 0U;
    renault_v0_parse_fields(data, &serial, &button, &counter);

    instance->generic.data = data;
    instance->decoder.decode_data = data;
    instance->decoder.decode_count_bit = RENAULT_V0_MIN_BITS;
    instance->packet_bit_count = RENAULT_V0_MIN_BITS;
    instance->generic.data_count_bit = RENAULT_V0_MIN_BITS;
    instance->key2 = key2;
    instance->generic.serial = serial;
    instance->generic.btn = button;
    instance->generic.cnt = counter;
    renault_v0_update_checks(
        data, key2, &instance->check_c1, &instance->check_c2, &instance->check_ic);
    if(instance->base.callback) {
        instance->base.callback(&instance->base, instance->base.context);
    }
}

static uint32_t renault_v0_arm_lsl(uint32_t value, uint32_t shift) {
    shift &= 0xFFU;
    if(shift >= 32U) {
        return 0U;
    }
    return value << shift;
}

static uint32_t renault_v0_arm_lsr(uint32_t value, uint32_t shift) {
    shift &= 0xFFU;
    if(shift >= 32U) {
        return 0U;
    }
    return value >> shift;
}

const RfStreamProtocolDecoder rf_stream_protocol_renault_v0_decoder = {
    .alloc = rf_stream_protocol_decoder_renault_v0_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_renault_v0_feed,
    .reset = rf_stream_protocol_decoder_renault_v0_reset,
    .get_hash_data = rf_stream_protocol_decoder_renault_v0_get_hash_data,
    .get_string = rf_stream_protocol_decoder_renault_v0_get_string,
    .serialize = rf_stream_protocol_decoder_renault_v0_serialize,
    .deserialize = rf_stream_protocol_decoder_renault_v0_deserialize,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_renault_v0_encoder = {
    .alloc = rf_stream_protocol_encoder_renault_v0_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = rf_stream_protocol_encoder_renault_v0_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_renault_v0_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec renault_v0_protocol = {
    .name = RENAULT_PROTOCOL_V0_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_renault_v0_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_renault_v0_encoder,
#else
    .encoder = NULL,
#endif
};

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_renault_v0_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolEncoderRenaultV0* instance = calloc(1, sizeof(RfStreamProtocolEncoderRenaultV0));
    instance->base.protocol = &renault_v0_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->encoder.repeat = RENAULT_V0_REPEAT;
    instance->encoder.is_running = false;
    return instance;
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_renault_v0_deserialize(void* context, RfRecord* flipper_format) {
    rf_stream_assert(context);
    RfStreamProtocolEncoderRenaultV0* instance = context;
    RfStreamProtocolStatus ret = RfStreamProtocolStatusError;

    instance->encoder.is_running = false;
    instance->encoder.front = 0;

    do {
        rf_record_rewind(flipper_format);
        if(rf_stream_block_generic_deserialize_check_count_bit(
               &instance->generic, flipper_format, RENAULT_V0_MIN_BITS) !=
           RfStreamProtocolStatusOk) {
            break;
        }

        uint32_t key2 = 0U;
        rf_record_rewind(flipper_format);
        if(!rf_record_read_uint32(flipper_format, RENAULT_V0_KEY2_FIELD, &key2, 1)) {
            break;
        }
        instance->key2 = key2;
        if(!renault_v0_type13_valid(instance->generic.data, instance->key2)) {
            break;
        }

        uint32_t serial = 0U;
        uint8_t button = 0U;
        uint8_t counter = 0U;
        renault_v0_parse_fields(instance->generic.data, &serial, &button, &counter);

        const bool rolling = renault_v0_model_matches(
            instance->generic.data, instance->key2, serial, button, counter);

        uint32_t serial_u32 = serial;
        uint32_t btn_u32 = button;
        uint32_t cnt_u32 = counter;
        rf_stream_encoder_read_fields(flipper_format, &serial_u32, &btn_u32, &cnt_u32, NULL);
        serial = serial_u32;
        button = (uint8_t)btn_u32;
        counter = (uint8_t)(cnt_u32 & 0xFFU);

        if(rolling) {
            if(!renault_v0_button_valid(button)) {
                break;
            }
            renault_v0_build_key(
                serial, button, counter, &instance->generic.data, &instance->key2);
            if(!renault_v0_type13_valid(instance->generic.data, instance->key2)) {
                break;
            }
        }

        instance->generic.serial = serial;
        instance->generic.btn = button;
        instance->generic.cnt = counter;
        instance->generic.data_count_bit = RENAULT_V0_MIN_BITS;
        instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, RENAULT_V0_REPEAT);
        if(instance->encoder.repeat == 0U) {
            instance->encoder.repeat = RENAULT_V0_REPEAT;
        }

        rf_stream_encoder_buffer_ensure(instance, RENAULT_V0_UPLOAD_CAPACITY);
        if(!renault_v0_build_upload(instance)) {
            break;
        }

        if(rolling) {
            uint8_t key_data[8];
            renault_v0_u64_to_bytes_be(instance->generic.data, key_data);
            rf_record_rewind(flipper_format);
            if(!rf_record_update_hex(flipper_format, FF_KEY, key_data, sizeof(key_data))) {
                rf_record_rewind(flipper_format);
                rf_record_insert_or_update_hex(
                    flipper_format, FF_KEY, key_data, sizeof(key_data));
            }
            rf_record_rewind(flipper_format);
            if(!rf_record_update_uint32(
                   flipper_format, RENAULT_V0_KEY2_FIELD, &instance->key2, 1)) {
                rf_record_rewind(flipper_format);
                rf_record_insert_or_update_uint32(
                    flipper_format, RENAULT_V0_KEY2_FIELD, &instance->key2, 1);
            }
            rf_stream_serialize_fields(
                flipper_format,
                RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT,
                instance->generic.serial,
                instance->generic.btn,
                instance->generic.cnt,
                0);
        }

        instance->encoder.is_running = true;
        ret = RfStreamProtocolStatusOk;
    } while(false);

    return ret;
}
#endif

void* rf_stream_protocol_decoder_renault_v0_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);
    RfStreamProtocolDecoderRenaultV0* instance = calloc(1, sizeof(RfStreamProtocolDecoderRenaultV0));
    instance->base.protocol = &renault_v0_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->manchester_state = 1U;
    return instance;
}

void rf_stream_protocol_decoder_renault_v0_reset(void* context) {
    rf_stream_assert(context);
    RfStreamProtocolDecoderRenaultV0* instance = context;
    instance->decoder.parser_step = RenaultV0DecoderStepReset;
    instance->manchester_state = 1U;
    instance->decoded_bit_count = 0U;
    instance->key2 = 0U;
}

void rf_stream_protocol_decoder_renault_v0_feed(void* context, bool level, uint32_t duration) {
    rf_stream_assert(context);
    RfStreamProtocolDecoderRenaultV0* instance = context;
    uint8_t event_code = 0U;

    if(instance->decoder.parser_step == RenaultV0DecoderStepReset) {
        if(level && (duration >= RENAULT_V0_SYNC_MIN_US)) {
            instance->decoder.parser_step = RenaultV0DecoderStepData;
            instance->decoded_bit_count = 0U;
            instance->manchester_state = 1U;
        }
        return;
    }

    if(instance->decoded_bit_count > RENAULT_V0_DECODER_BIT_LIMIT) {
        renault_v0_decode_candidate(instance);
        instance->decoder.parser_step = RenaultV0DecoderStepReset;
        return;
    }

    if(!renault_v0_classify_event(duration, level, &event_code)) {
        renault_v0_decode_candidate(instance);
        if(level && (duration >= RENAULT_V0_SYNC_MIN_US)) {
            instance->decoder.parser_step = RenaultV0DecoderStepData;
            instance->decoded_bit_count = 0U;
            instance->manchester_state = 1U;
        } else {
            instance->decoder.parser_step = RenaultV0DecoderStepReset;
        }
        return;
    }

    const uint8_t state = instance->manchester_state & 0x03U;
    uint8_t next_state = (renault_v0_decoder_state_table[state] >> event_code) & 0x03U;
    if(next_state == state) {
        return;
    }

    instance->manchester_state = next_state;
    if((next_state == 1U) || (next_state == 2U)) {
        const uint8_t bit = (next_state == 1U) ? 1U : 0U;
        const uint8_t bit_offset = instance->decoded_bit_count;
        instance->decoded_bit_count = bit_offset + 1U;
        instance->decoded_bits[bit_offset] = bit;
    }
}

uint8_t rf_stream_protocol_decoder_renault_v0_get_hash_data(void* context) {
    rf_stream_assert(context);
    RfStreamProtocolDecoderRenaultV0* instance = context;
    const uint32_t low = (uint32_t)instance->decoder.decode_data;
    const uint32_t high = (uint32_t)(instance->decoder.decode_data >> 32U);

    uint32_t hash = 0U;
    for(uint32_t shift = 0U; shift < 0x38U; shift += 8U) {
        uint32_t mixed = renault_v0_arm_lsr(low, shift);
        mixed |= renault_v0_arm_lsl(high, 32U - shift);
        mixed |= renault_v0_arm_lsr(high, shift - 32U);
        hash ^= mixed;
        hash = ((hash << 1U) & 0xFEU) | ((hash >> 7U) & 1U);
    }

    const uint32_t key2_mix = instance->key2 ^ (instance->key2 >> 2U) ^ (instance->key2 >> 10U);
    return (uint8_t)(hash ^ key2_mix);
}

void rf_stream_protocol_decoder_renault_v0_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderRenaultV0* instance = context;
    rf_text_printf(
        output, "%s %ubit\r\n", instance->generic.protocol_name, instance->packet_bit_count);
    rf_text_cat_printf(
        output,
        "Key:%016llX\r\nKey2:%05lX Sn:%06lX\r\nBtn:%01X [%s] Cnt:%02lX\r\nC1:[%s] C2:[%s] IC:[%s]",
        (unsigned long long)instance->generic.data,
        (unsigned long)instance->key2,
        (unsigned long)instance->generic.serial,
        instance->generic.btn,
        renault_v0_get_button_name(instance->generic.btn),
        (unsigned long)instance->generic.cnt,
        instance->check_c1 ? "ERR" : "OK",
        instance->check_c2 ? "ERR" : "OK",
        instance->check_ic ? "ERR" : "OK");
}

RfStreamProtocolStatus rf_stream_protocol_decoder_renault_v0_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    rf_stream_assert(context);
    RfStreamProtocolDecoderRenaultV0* instance = context;
    instance->generic.data_count_bit = instance->packet_bit_count;

    RfStreamProtocolStatus status =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }

    RfStreamProtocolStatus fields = rf_stream_serialize_fields(
        flipper_format,
        RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT,
        instance->generic.serial,
        instance->generic.btn,
        instance->generic.cnt,
        0);
    if(fields != RfStreamProtocolStatusOk) {
        return fields;
    }

    if(!rf_record_write_uint32(flipper_format, RENAULT_V0_KEY2_FIELD, &instance->key2, 1)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }
    return rf_stream_write_display(
        flipper_format,
        instance->generic.protocol_name,
        renault_v0_get_button_name(instance->generic.btn));
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_renault_v0_deserialize(void* context, RfRecord* flipper_format) {
    rf_stream_assert(context);
    RfStreamProtocolDecoderRenaultV0* instance = context;
    RfStreamProtocolStatus status = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, RENAULT_V0_MIN_BITS);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }

    uint32_t key2 = 0U;
    if(!rf_record_read_uint32(flipper_format, RENAULT_V0_KEY2_FIELD, &key2, 1)) {
        return RfStreamProtocolStatusError;
    }

    instance->key2 = key2;
    if(!renault_v0_type13_valid(instance->generic.data, instance->key2)) {
        return RfStreamProtocolStatusError;
    }
    instance->packet_bit_count = RENAULT_V0_MIN_BITS;
    instance->generic.data_count_bit = RENAULT_V0_MIN_BITS;
    instance->decoder.decode_data = instance->generic.data;
    instance->decoder.decode_count_bit = RENAULT_V0_MIN_BITS;

    uint8_t button = 0;
    uint32_t serial = 0;
    uint8_t counter = 0;
    renault_v0_parse_fields(instance->generic.data, &serial, &button, &counter);
    instance->generic.serial = serial;
    instance->generic.btn = button;
    instance->generic.cnt = counter;
    renault_v0_update_checks(
        instance->generic.data,
        instance->key2,
        &instance->check_c1,
        &instance->check_c2,
        &instance->check_ic);
    return status;
}

#endif
