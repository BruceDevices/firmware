// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "../rf_stream_internal.h"
#include "../rf_keeloq_core.h"
#define HONDA_STATIC_PROTOCOL_NAME "Honda Static"

typedef struct RfStreamProtocolDecoderHondaStatic RfStreamProtocolDecoderHondaStatic;
typedef struct RfStreamProtocolEncoderHondaStatic RfStreamProtocolEncoderHondaStatic;

extern const RfStreamCodec honda_static_protocol;

void* rf_stream_protocol_decoder_honda_static_alloc(RfStreamEnvironment* environment);
void rf_stream_protocol_decoder_honda_static_free(void* context);
void rf_stream_protocol_decoder_honda_static_reset(void* context);
void rf_stream_protocol_decoder_honda_static_feed(void* context, bool level, uint32_t duration);
uint8_t rf_stream_protocol_decoder_honda_static_get_hash_data(void* context);
RfStreamProtocolStatus rf_stream_protocol_decoder_honda_static_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset);
RfStreamProtocolStatus
    rf_stream_protocol_decoder_honda_static_deserialize(void* context, RfRecord* flipper_format);
void rf_stream_protocol_decoder_honda_static_get_string(void* context, RfText* output);

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_honda_static_alloc(RfStreamEnvironment* environment);
RfStreamProtocolStatus
    rf_stream_protocol_encoder_honda_static_deserialize(void* context, RfRecord* flipper_format);
#endif




#define HONDA_STATIC_BIT_COUNT       64
#define HONDA_STATIC_MIN_SYMBOLS     36
#define HONDA_STATIC_SHORT_BASE_US   28
#define HONDA_STATIC_SHORT_SPAN_US   70
#define HONDA_STATIC_LONG_BASE_US    61
#define HONDA_STATIC_LONG_SPAN_US    130
#define HONDA_STATIC_SYNC_TIME_US    700
#define HONDA_STATIC_ELEMENT_TIME_US 63
#define HONDA_STATIC_UPLOAD_CAPACITY \
    (1U + HONDA_STATIC_PREAMBLE_ALTERNATING_COUNT + (2U * HONDA_STATIC_BIT_COUNT) + 1U)
#define HONDA_STATIC_SYMBOL_CAPACITY            512
#define HONDA_STATIC_PREAMBLE_ALTERNATING_COUNT 160
#define HONDA_STATIC_PREAMBLE_MAX_TRANSITIONS   19
#define HONDA_STATIC_SYMBOL_BYTE_COUNT          ((HONDA_STATIC_SYMBOL_CAPACITY + 7U) / 8U)
_Static_assert(
    HONDA_STATIC_UPLOAD_CAPACITY <= RF_SHARED_UPLOAD_CAPACITY,
    "HONDA_STATIC_UPLOAD_CAPACITY exceeds shared upload slab");

#if RF_STREAM_WITH_ENCODER
static const uint8_t honda_static_encoder_button_map[4] = {0x02, 0x04, 0x08, 0x05};
#endif
static const char* const honda_static_button_names[9] = {
    "Lock",
    "Unlock",
    "Unknown",
    "Trunk",
    "Remote Start",
    "Unknown",
    "Unknown",
    "Panic",
    "Lock x2",
};

typedef struct {
    uint32_t serial;
    uint32_t counter;
    uint8_t button;
    uint8_t checksum;
} HondaStaticFields;

struct RfStreamProtocolDecoderHondaStatic {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockGeneric generic;

    uint16_t symbols_count;
    uint8_t symbols[HONDA_STATIC_SYMBOL_BYTE_COUNT];
};

#if RF_STREAM_WITH_ENCODER
struct RfStreamProtocolEncoderHondaStatic {
    RfStreamProtocolEncoderBase base;

    RfStreamProtocolBlockEncoder encoder;
    RfStreamBlockGeneric generic;

    HondaStaticFields decoded;
    uint8_t tx_button;
};
#endif

static void honda_static_decoder_commit(
    RfStreamProtocolDecoderHondaStatic* instance,
    const HondaStaticFields* decoded);

static uint8_t honda_static_get_bits(const uint8_t* data, uint8_t start, uint8_t count) {
    uint32_t value = 0;

    for(uint8_t i = 0; i < count; i++) {
        const uint8_t bit_index = start + i;
        const uint8_t byte = data[bit_index >> 3U];
        const uint8_t shift = (uint8_t)(~bit_index) & 0x07U;
        value = (value << 1U) | ((byte >> shift) & 1U);
    }

    return (uint8_t)value;
}

static uint32_t honda_static_get_bits_u32(const uint8_t* data, uint8_t start, uint8_t count) {
    uint32_t value = 0;

    for(uint8_t i = 0; i < count; i++) {
        const uint8_t bit_index = start + i;
        const uint8_t byte = data[bit_index >> 3U];
        const uint8_t shift = (uint8_t)(~bit_index) & 0x07U;
        value = (value << 1U) | ((byte >> shift) & 1U);
    }

    return value;
}

#if RF_STREAM_WITH_ENCODER
static void honda_static_set_bits(uint8_t* data, uint8_t start, uint8_t count, uint32_t value) {
    for(uint8_t i = 0; i < count; i++) {
        const uint8_t bit_index = start + i;
        const uint8_t byte_index = bit_index >> 3U;
        const uint8_t shift = ((uint8_t)~bit_index) & 0x07U;
        const uint8_t mask = (uint8_t)(1U << shift);
        const bool bit = ((value >> (count - 1U - i)) & 1U) != 0U;

        if(bit) {
            data[byte_index] |= mask;
        } else {
            data[byte_index] &= (uint8_t)~mask;
        }
    }
}
#endif

static uint8_t honda_static_level_u8(bool level) {
    return level ? 1U : 0U;
}

static void honda_static_symbol_set(uint8_t* buf, uint16_t index, uint8_t v) {
    const uint8_t byte_index = (uint8_t)(index >> 3U);
    const uint8_t shift = (uint8_t)(~index) & 0x07U;
    const uint8_t mask = (uint8_t)(1U << shift);
    if(v) {
        buf[byte_index] |= mask;
    } else {
        buf[byte_index] &= (uint8_t)~mask;
    }
}

static uint8_t honda_static_symbol_get(const uint8_t* buf, uint16_t index) {
    const uint8_t byte_index = (uint8_t)(index >> 3U);
    const uint8_t shift = (uint8_t)(~index) & 0x07U;
    return (uint8_t)((buf[byte_index] >> shift) & 1U);
}

static bool honda_static_is_valid_button(uint8_t button) {
    if(button > 9U) {
        return false;
    }

    return ((0x336U >> button) & 1U) != 0U;
}

static bool honda_static_is_valid_serial(uint32_t serial) {
    return (serial != 0U) && (serial != 0x0FFFFFFFU);
}

#if RF_STREAM_WITH_ENCODER
static uint8_t honda_static_encoder_remap_button(uint8_t button) {
    if(button < 2U) {
        return 1U;
    }
    button -= 2U;
    if(button <= 3U) {
        return honda_static_encoder_button_map[button];
    }

    return 1U;
}
#endif

static const char* honda_static_button_name(uint8_t button) {
    if((button >= 1U) && (button <= COUNT_OF(honda_static_button_names))) {
        return honda_static_button_names[button - 1U];
    }

    return "Unknown";
}

static uint8_t honda_static_compact_bytes_checksum(const uint8_t compact[8]) {
    const uint8_t canonical[7] = {
        (uint8_t)((compact[0] << 4U) | (compact[1] >> 4U)),
        (uint8_t)((compact[1] << 4U) | (compact[2] >> 4U)),
        (uint8_t)((compact[2] << 4U) | (compact[3] >> 4U)),
        (uint8_t)((compact[3] << 4U) | (compact[4] >> 4U)),
        compact[5],
        compact[6],
        compact[7],
    };

    uint8_t checksum = 0U;
    for(size_t i = 0; i < COUNT_OF(canonical); i++) {
        checksum ^= canonical[i];
    }

    return checksum;
}

static void honda_static_unpack_compact(uint64_t key, HondaStaticFields* fields) {
    uint8_t compact[8];
    rf_stream_u64_to_bytes_be(key, compact);

    memset(fields, 0, sizeof(*fields));
    fields->button = compact[0] & 0x0FU;
    fields->serial = ((uint32_t)compact[1] << 20U) | ((uint32_t)compact[2] << 12U) |
                     ((uint32_t)compact[3] << 4U) | ((uint32_t)compact[4] >> 4U);
    fields->counter = ((uint32_t)compact[5] << 16U) | ((uint32_t)compact[6] << 8U) |
                      (uint32_t)compact[7];
    fields->checksum = honda_static_compact_bytes_checksum(compact);
}

static uint64_t honda_static_pack_compact(const HondaStaticFields* fields) {
    uint8_t compact[8];

    compact[0] = fields->button & 0x0FU;
    compact[1] = (uint8_t)(fields->serial >> 20U);
    compact[2] = (uint8_t)(fields->serial >> 12U);
    compact[3] = (uint8_t)(fields->serial >> 4U);
    compact[4] = (uint8_t)(fields->serial << 4U);
    compact[5] = (uint8_t)(fields->counter >> 16U);
    compact[6] = (uint8_t)(fields->counter >> 8U);
    compact[7] = (uint8_t)fields->counter;

    return rf_stream_bytes_to_u64_be(compact);
}

#if RF_STREAM_WITH_ENCODER
static void honda_static_build_packet_bytes(const HondaStaticFields* fields, uint8_t packet[8]) {
    memset(packet, 0, 8);

    honda_static_set_bits(packet, 0, 4, fields->button & 0x0FU);
    honda_static_set_bits(packet, 4, 28, fields->serial);
    honda_static_set_bits(packet, 32, 24, fields->counter);

    uint8_t checksum = 0U;
    for(size_t i = 0; i < 7; i++) {
        checksum ^= packet[i];
    }

    honda_static_set_bits(packet, 56, 8, checksum);
}
#endif

static bool
    honda_static_validate_forward_packet(const uint8_t packet[9], HondaStaticFields* fields) {
    const uint8_t button = honda_static_get_bits(packet, 0, 4);
    const uint32_t serial = honda_static_get_bits_u32(packet, 4, 28);
    const uint32_t counter = honda_static_get_bits_u32(packet, 32, 24);
    const uint8_t checksum = honda_static_get_bits(packet, 56, 8);

    uint8_t checksum_calc = 0U;
    for(size_t i = 0; i < 7; i++) {
        checksum_calc ^= packet[i];
    }

    if(checksum != checksum_calc) {
        return false;
    }
    if(!honda_static_is_valid_button(button)) {
        return false;
    }
    if(!honda_static_is_valid_serial(serial)) {
        return false;
    }

    fields->button = button;
    fields->serial = serial;
    fields->counter = counter;
    fields->checksum = checksum;

    return true;
}

static bool
    honda_static_validate_reverse_packet(const uint8_t packet[9], HondaStaticFields* fields) {
    uint8_t reversed[9];
    for(size_t i = 0; i < COUNT_OF(reversed); i++) {
        reversed[i] = rf_stream_reverse_bits8(packet[i]);
    }

    const uint8_t button = honda_static_get_bits(reversed, 0, 4);
    const uint32_t serial = honda_static_get_bits_u32(reversed, 4, 28);
    const uint32_t counter = honda_static_get_bits_u32(reversed, 32, 24);

    uint8_t checksum = 0U;
    for(size_t i = 0; i < 7; i++) {
        checksum ^= reversed[i];
    }

    if(!honda_static_is_valid_button(button)) {
        return false;
    }
    if(!honda_static_is_valid_serial(serial)) {
        return false;
    }

    fields->button = button;
    fields->serial = serial;
    fields->counter = counter;
    fields->checksum = checksum;

    return true;
}

static bool honda_static_manchester_pack_64(
    const uint8_t* symbol_bits,
    uint16_t count,
    uint16_t start_pos,
    bool inverted,
    uint8_t packet[9],
    uint16_t* out_bit_count) {
    memset(packet, 0, 9);

    uint16_t pos = start_pos;
    uint16_t bit_count = 0U;

    while((uint16_t)(pos + 1U) < count) {
        if(bit_count >= HONDA_STATIC_BIT_COUNT) {
            break;
        }

        const uint8_t a = honda_static_symbol_get(symbol_bits, pos);
        const uint8_t b = honda_static_symbol_get(symbol_bits, pos + 1U);

        if(a == b) {
            pos++;
            continue;
        }

        bool bit = false;
        if(inverted) {
            bit = (a == 0U) && (b == 1U);
        } else {
            bit = (a == 1U) && (b == 0U);
        }

        if(bit) {
            packet[bit_count >> 3U] |= (uint8_t)(1U << (((uint8_t)~bit_count) & 0x07U));
        }

        bit_count++;
        pos += 2U;
    }

    if(out_bit_count) {
        *out_bit_count = bit_count;
    }

    return bit_count >= HONDA_STATIC_BIT_COUNT;
}

static bool honda_static_parse_symbols(RfStreamProtocolDecoderHondaStatic* instance, bool inverted) {
    const uint16_t count = instance->symbols_count;
    const uint8_t* symbol_bits = instance->symbols;
    HondaStaticFields decoded;

    uint16_t index = 1U;
    uint16_t transitions = 0U;

    while(index < count) {
        if(honda_static_symbol_get(symbol_bits, index) !=
           honda_static_symbol_get(symbol_bits, index - 1U)) {
            transitions++;
        } else {
            if(transitions > HONDA_STATIC_PREAMBLE_MAX_TRANSITIONS) {
                break;
            }
            transitions = 0U;
        }
        index++;
    }

    if(index >= count) {
        return false;
    }

    while(((uint16_t)(index + 1U) < count) && (honda_static_symbol_get(symbol_bits, index) ==
                                               honda_static_symbol_get(symbol_bits, index + 1U))) {
        index++;
    }

    const uint16_t data_start = index;

    uint8_t packet[9] = {0};
    uint16_t bit_count = 0U;

    if(!honda_static_manchester_pack_64(
           symbol_bits, count, data_start, inverted, packet, &bit_count)) {
        return false;
    }

    if(honda_static_validate_forward_packet(packet, &decoded)) {
        honda_static_decoder_commit(instance, &decoded);
        return true;
    }

    if(inverted) {
        return false;
    }

    if(honda_static_validate_reverse_packet(packet, &decoded)) {
        honda_static_decoder_commit(instance, &decoded);
        return true;
    }

    return false;
}

static void honda_static_decoder_commit(
    RfStreamProtocolDecoderHondaStatic* instance,
    const HondaStaticFields* decoded) {
    instance->generic.data_count_bit = HONDA_STATIC_BIT_COUNT;
    instance->generic.data = honda_static_pack_compact(decoded);
    instance->generic.serial = decoded->serial;
    instance->generic.cnt = decoded->counter;
    instance->generic.btn = decoded->button;

    if(instance->base.callback) {
        instance->base.callback(&instance->base, instance->base.context);
    }
}

#if RF_STREAM_WITH_ENCODER
static void honda_static_build_upload(RfStreamProtocolEncoderHondaStatic* instance) {
    uint8_t packet[8];
    honda_static_build_packet_bytes(&instance->decoded, packet);

    size_t index = 0U;
    LevelDuration* up = instance->encoder.upload;
    const size_t cap = HONDA_STATIC_UPLOAD_CAPACITY;

    index = rf_stream_emit(up, index, cap, true, HONDA_STATIC_SYNC_TIME_US);

    for(size_t i = 0; i < HONDA_STATIC_PREAMBLE_ALTERNATING_COUNT; i++) {
        index = rf_stream_emit(up, index, cap, (i & 1U) != 0U, HONDA_STATIC_ELEMENT_TIME_US);
    }

    for(uint8_t bit = 0U; bit < HONDA_STATIC_BIT_COUNT; bit++) {
        const bool value = ((packet[bit >> 3U] >> (((uint8_t)~bit) & 0x07U)) & 1U) != 0U;
        index = rf_stream_emit(up, index, cap, !value, HONDA_STATIC_ELEMENT_TIME_US);
        index = rf_stream_emit(up, index, cap, value, HONDA_STATIC_ELEMENT_TIME_US);
    }

    const bool last_bit = (packet[7] & 1U) != 0U;
    index = rf_stream_emit(up, index, cap, !last_bit, HONDA_STATIC_SYNC_TIME_US);

    instance->encoder.front = 0U;
    instance->encoder.size_upload = index;
}

#endif

const RfStreamProtocolDecoder rf_stream_protocol_honda_static_decoder = {
    .alloc = rf_stream_protocol_decoder_honda_static_alloc,
    .free = rf_stream_decoder_free_default,
    .feed = rf_stream_protocol_decoder_honda_static_feed,
    .reset = rf_stream_protocol_decoder_honda_static_reset,
    .get_hash_data = rf_stream_protocol_decoder_honda_static_get_hash_data,
    .serialize = rf_stream_protocol_decoder_honda_static_serialize,
    .deserialize = rf_stream_protocol_decoder_honda_static_deserialize,
    .get_string = rf_stream_protocol_decoder_honda_static_get_string,
};

#if RF_STREAM_WITH_ENCODER
const RfStreamProtocolEncoder rf_stream_protocol_honda_static_encoder = {
    .alloc = rf_stream_protocol_encoder_honda_static_alloc,
    .free = rf_stream_encoder_free,
    .deserialize = rf_stream_protocol_encoder_honda_static_deserialize,
    .stop = rf_stream_encoder_stop,
    .yield = rf_stream_encoder_yield,
};
#else
const RfStreamProtocolEncoder rf_stream_protocol_honda_static_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};
#endif

const RfStreamCodec honda_static_protocol = {
    .name = HONDA_STATIC_PROTOCOL_NAME,
#if RF_STREAM_WITH_DECODER
    .decoder = &rf_stream_protocol_honda_static_decoder,
#else
    .decoder = NULL,
#endif
#if RF_STREAM_WITH_ENCODER
    .encoder = &rf_stream_protocol_honda_static_encoder,
#else
    .encoder = NULL,
#endif
};

#if RF_STREAM_WITH_ENCODER
void* rf_stream_protocol_encoder_honda_static_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);

    RfStreamProtocolEncoderHondaStatic* instance = malloc(sizeof(RfStreamProtocolEncoderHondaStatic));
    memset(instance, 0, sizeof(*instance));

    instance->base.protocol = &honda_static_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->encoder.repeat = 3U;
    rf_stream_encoder_buffer_ensure(instance, HONDA_STATIC_UPLOAD_CAPACITY);

    return instance;
}

RfStreamProtocolStatus
    rf_stream_protocol_encoder_honda_static_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolEncoderHondaStatic* instance = context;

    instance->encoder.is_running = false;
    instance->encoder.front = 0U;

    if(rf_stream_verify_protocol_name(flipper_format, instance->base.protocol->name) !=
       RfStreamProtocolStatusOk) {
        return RfStreamProtocolStatusError;
    }

    uint64_t key = 0;
    if(!rf_stream_flipper_read_hex_u64(flipper_format, FF_KEY, &key)) {
        return RfStreamProtocolStatusError;
    }
    honda_static_unpack_compact(key, &instance->decoded);

    uint32_t serial = instance->decoded.serial;
    uint32_t btn_u32 = instance->decoded.button;
    uint32_t cnt = instance->decoded.counter & 0x00FFFFFFU;
    rf_stream_encoder_read_fields(flipper_format, &serial, &btn_u32, &cnt, NULL);

    instance->decoded.serial = serial;
    uint8_t b = (uint8_t)btn_u32;
    if(honda_static_is_valid_button(b)) {
        instance->decoded.button = b;
    } else if(b >= 2U && b <= 5U) {
        instance->decoded.button = honda_static_encoder_remap_button(b);
    }
    instance->decoded.counter = cnt & 0x00FFFFFFU;

    instance->generic.serial = instance->decoded.serial;
    instance->generic.cnt = instance->decoded.counter;
    instance->generic.btn = instance->decoded.button;
    instance->generic.data_count_bit = HONDA_STATIC_BIT_COUNT;
    instance->generic.data = honda_static_pack_compact(&instance->decoded);

    uint8_t key_data[8];
    rf_stream_u64_to_bytes_be(instance->generic.data, key_data);
    rf_record_rewind(flipper_format);
    if(!rf_record_update_hex(flipper_format, FF_KEY, key_data, sizeof(key_data))) {
        return RfStreamProtocolStatusErrorParserKey;
    }

    instance->encoder.repeat = rf_stream_encoder_read_repeat(flipper_format, 3U);

    honda_static_build_upload(instance);
    instance->encoder.is_running = true;
    return RfStreamProtocolStatusOk;
}

#endif

void* rf_stream_protocol_decoder_honda_static_alloc(RfStreamEnvironment* environment) {
    UNUSED(environment);

    RfStreamProtocolDecoderHondaStatic* instance = malloc(sizeof(RfStreamProtocolDecoderHondaStatic));
    memset(instance, 0, sizeof(*instance));

    instance->base.protocol = &honda_static_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;

    return instance;
}

void rf_stream_protocol_decoder_honda_static_reset(void* context) {
    RfStreamProtocolDecoderHondaStatic* instance = context;
    instance->symbols_count = 0U;
}

void rf_stream_protocol_decoder_honda_static_feed(void* context, bool level, uint32_t duration) {
    RfStreamProtocolDecoderHondaStatic* instance = context;

    const uint8_t sym = honda_static_level_u8(level);

    if((duration >= HONDA_STATIC_SHORT_BASE_US) &&
       ((duration - HONDA_STATIC_SHORT_BASE_US) <= HONDA_STATIC_SHORT_SPAN_US)) {
        if(instance->symbols_count < HONDA_STATIC_SYMBOL_CAPACITY) {
            honda_static_symbol_set(instance->symbols, instance->symbols_count, sym);
            instance->symbols_count++;
        }
        return;
    }

    if((duration >= HONDA_STATIC_LONG_BASE_US) &&
       ((duration - HONDA_STATIC_LONG_BASE_US) <= HONDA_STATIC_LONG_SPAN_US)) {
        if((uint16_t)(instance->symbols_count + 2U) <= HONDA_STATIC_SYMBOL_CAPACITY) {
            honda_static_symbol_set(instance->symbols, instance->symbols_count, sym);
            instance->symbols_count++;
            honda_static_symbol_set(instance->symbols, instance->symbols_count, sym);
            instance->symbols_count++;
        }
        return;
    }

    const uint16_t sc = instance->symbols_count;

    if(sc >= HONDA_STATIC_MIN_SYMBOLS) {
        if(!honda_static_parse_symbols(instance, true)) {
            honda_static_parse_symbols(instance, false);
        }
    }

    instance->symbols_count = 0U;
}

uint8_t rf_stream_protocol_decoder_honda_static_get_hash_data(void* context) {
    RfStreamProtocolDecoderHondaStatic* instance = context;
    const uint64_t data = instance->generic.data;

    return (uint8_t)(data ^ (data >> 8U) ^ (data >> 16U) ^ (data >> 24U) ^ (data >> 32U) ^
                     (data >> 40U) ^ (data >> 48U) ^ (data >> 56U));
}

void rf_stream_protocol_decoder_honda_static_get_string(void* context, RfText* output) {
    RfStreamProtocolDecoderHondaStatic* instance = context;
    HondaStaticFields decoded;
    honda_static_unpack_compact(instance->generic.data, &decoded);

    rf_text_printf(
        output,
        "%s\r\n"
        "Key:%016llX\r\n"
        "Btn:%s\r\n"
        "Ser:%07lX Cnt:%06lX",
        instance->generic.protocol_name,
        (unsigned long long)instance->generic.data,
        honda_static_button_name(decoded.button),
        (unsigned long)decoded.serial,
        (unsigned long)decoded.counter);
}

RfStreamProtocolStatus rf_stream_protocol_decoder_honda_static_serialize(
    void* context,
    RfRecord* flipper_format,
    RfStreamRadioPreset* preset) {
    RfStreamProtocolDecoderHondaStatic* instance = context;
    instance->generic.data_count_bit = HONDA_STATIC_BIT_COUNT;
    HondaStaticFields decoded;
    honda_static_unpack_compact(instance->generic.data, &decoded);

    RfStreamProtocolStatus status =
        rf_stream_block_generic_serialize(&instance->generic, flipper_format, preset);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }

    status = rf_stream_serialize_fields(
        flipper_format,
        RF_FIELD_SERIAL | RF_FIELD_BTN | RF_FIELD_CNT,
        decoded.serial,
        decoded.button,
        decoded.counter,
        0);
    if(status != RfStreamProtocolStatusOk) return status;

    uint32_t temp = decoded.checksum;
    if(!rf_record_write_uint32(flipper_format, "Checksum", &temp, 1)) {
        return RfStreamProtocolStatusErrorParserOthers;
    }

    return status;
}

RfStreamProtocolStatus
    rf_stream_protocol_decoder_honda_static_deserialize(void* context, RfRecord* flipper_format) {
    RfStreamProtocolDecoderHondaStatic* instance = context;
    RfStreamProtocolStatus status = rf_stream_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, HONDA_STATIC_BIT_COUNT);
    if(status != RfStreamProtocolStatusOk) {
        return status;
    }

    HondaStaticFields decoded;
    honda_static_unpack_compact(instance->generic.data, &decoded);
    uint32_t s = decoded.serial;
    uint32_t b = decoded.button;
    uint32_t c = decoded.counter;
    rf_stream_encoder_read_fields(flipper_format, &s, &b, &c, NULL);
    decoded.serial = s;
    decoded.button = (uint8_t)b;
    decoded.counter = c & 0x00FFFFFFU;

    instance->generic.data = honda_static_pack_compact(&decoded);
    instance->generic.serial = decoded.serial;
    instance->generic.cnt = decoded.counter;
    instance->generic.btn = decoded.button;

    return status;
}

#endif
