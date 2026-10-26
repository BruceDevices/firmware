// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#pragma once
#include "decoders/manchester_helpers.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define EXT_PATH(x) x
#define APP_ASSETS_PATH(x) x
#define COUNT_OF(x) (sizeof(x) / sizeof((x)[0]))
#define UNUSED(x) (void)(x)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#include <setjmp.h>
void rf_stream_memory_begin(void);
jmp_buf *rf_stream_memory_jump(void);
void rf_stream_memory_finish(void *keep);
void rf_stream_memory_fail(void);
void *rf_stream_malloc(size_t n);
void *rf_stream_calloc(size_t n, size_t size);
void *rf_stream_realloc(void *p, size_t n);
void rf_stream_free(void *p);

#define rf_stream_crash(...) rf_stream_memory_fail()
#define RF_STREAM_LOG_E(...)
#define RF_STREAM_LOG_W(...)
#define RF_STREAM_LOG_I(...)
#define RF_STREAM_LOG_D(...)
#define RF_STREAM_LOG_T(...)
#define rf_stream_assert(x)                                                                                  \
    do {                                                                                                     \
        if (!(x)) rf_stream_memory_fail();                                                                   \
    } while (0)
#define rf_stream_check(x) rf_stream_assert(x)
typedef struct {
    char *data;
    size_t len, cap;
} RfText;
RfText *rf_text_alloc(void);
void rf_text_free(RfText *s);
void rf_text_reset(RfText *s);
const char *rf_text_get_cstr(const RfText *s);
void rf_text_set(RfText *s, const char *v);
void rf_text_printf(RfText *s, const char *fmt, ...);
void rf_text_cat_printf(RfText *s, const char *fmt, ...);
int rf_text_cmp_str(const RfText *s, const char *v);
bool rf_text_equal(const RfText *s, const char *v);
/* Flipper allocations are zero initialized. Keep this behavior in the port. */
#ifndef __cplusplus
#define malloc(n) rf_stream_malloc(n)
#endif
#ifndef __cplusplus
#define calloc(n, s) rf_stream_calloc(n, s)
#endif
#ifndef __cplusplus
#define realloc(p, n) rf_stream_realloc(p, n)
#endif
#ifndef __cplusplus
#define free(p) rf_stream_free(p)
#endif

typedef struct {
    char *key, *value;
} RfRecordField;
typedef struct {
    RfRecordField fields[64];
    size_t count;
} RfRecord;
void rf_stream_ff_clear(RfRecord *ff);
bool rf_stream_ff_parse(RfRecord *ff, const char *text);
const char *rf_stream_ff_get(const RfRecord *ff, const char *key);
bool rf_record_rewind(RfRecord *ff);
bool rf_record_read_string(RfRecord *ff, const char *key, RfText *out);
bool rf_record_read_uint32(RfRecord *ff, const char *key, uint32_t *out, size_t count);
bool rf_record_read_hex(RfRecord *ff, const char *key, uint8_t *out, size_t count);
bool rf_record_write_string_cstr(RfRecord *ff, const char *key, const char *value);
bool rf_record_write_uint32(RfRecord *ff, const char *key, const uint32_t *value, size_t count);
bool rf_record_write_hex(RfRecord *ff, const char *key, const uint8_t *value, size_t count);
bool rf_record_delete_key(RfRecord *ff, const char *key);
#define rf_record_insert_or_update_string_cstr rf_record_write_string_cstr
#define rf_record_insert_or_update_uint32 rf_record_write_uint32
#define rf_record_insert_or_update_hex rf_record_write_hex
#define rf_record_update_uint32 rf_record_write_uint32
#define rf_record_update_hex rf_record_write_hex

#define LEVEL_DURATION_BIG

#ifdef LEVEL_DURATION_BIG

#define LEVEL_DURATION_RESET 0U
#define LEVEL_DURATION_LEVEL_LOW 1U
#define LEVEL_DURATION_LEVEL_HIGH 2U
#define LEVEL_DURATION_WAIT 3U
#define LEVEL_DURATION_RESERVED 0x800000U

typedef struct {
    uint32_t duration : 30;
    uint8_t level : 2;
} LevelDuration;

static inline LevelDuration level_duration_make(bool level, uint32_t duration) {
    LevelDuration level_duration;
    level_duration.level = level ? LEVEL_DURATION_LEVEL_HIGH : LEVEL_DURATION_LEVEL_LOW;
    level_duration.duration = duration;
    return level_duration;
}

static inline LevelDuration level_duration_reset() {
    LevelDuration level_duration;
    level_duration.level = LEVEL_DURATION_RESET;
    return level_duration;
}

static inline LevelDuration level_duration_wait() {
    LevelDuration level_duration;
    level_duration.level = LEVEL_DURATION_WAIT;
    return level_duration;
}

static inline bool level_duration_is_reset(LevelDuration level_duration) {
    return level_duration.level == LEVEL_DURATION_RESET;
}

static inline bool level_duration_is_wait(LevelDuration level_duration) {
    return level_duration.level == LEVEL_DURATION_WAIT;
}

static inline bool level_duration_get_level(LevelDuration level_duration) {
    return level_duration.level == LEVEL_DURATION_LEVEL_HIGH;
}

static inline uint32_t level_duration_get_duration(LevelDuration level_duration) {
    return level_duration.duration;
}

#else

#define LEVEL_DURATION_RESET 0U
#define LEVEL_DURATION_RESERVED 0x800000U

typedef int32_t LevelDuration;

static inline LevelDuration level_duration(bool level, uint32_t duration) {
    return level ? duration : -(int32_t)duration;
}

static inline LevelDuration level_duration_reset() { return LEVEL_DURATION_RESET; }

static inline bool level_duration_is_reset(LevelDuration level_duration) {
    return level_duration == LEVEL_DURATION_RESET;
}

static inline bool level_duration_get_level(LevelDuration level_duration) { return level_duration > 0; }

static inline uint32_t level_duration_get_duration(LevelDuration level_duration) {
    return (level_duration >= 0) ? level_duration : -level_duration;
}

#endif

typedef struct RfStreamCodec RfStreamCodec;
typedef struct RfStreamKeystore RfStreamKeystore;
typedef struct RfStreamEnvironment {
    RfStreamKeystore *keystore;
} RfStreamEnvironment;
RfStreamKeystore *rf_stream_environment_get_keystore(RfStreamEnvironment *e);

typedef struct RfStreamEnvironment RfStreamEnvironment;

// Radio Preset
typedef struct {
    RfText *name;
    uint32_t frequency;
    uint8_t *data;
    size_t data_size;
} RfStreamRadioPreset;

typedef enum {
    RfStreamProtocolStatusOk = 0,
    // Errors
    RfStreamProtocolStatusError = (-1), ///< General unclassified error
    // Serialize/De-serialize
    RfStreamProtocolStatusErrorParserHeader = (-2),       ///< Missing or invalid file header
    RfStreamProtocolStatusErrorParserFrequency = (-3),    ///< Missing `Frequency`
    RfStreamProtocolStatusErrorParserPreset = (-4),       ///< Missing `Preset`
    RfStreamProtocolStatusErrorParserCustomPreset = (-5), ///< Missing `Custom_preset_module`
    RfStreamProtocolStatusErrorParserProtocolName = (-6), ///< Missing `Protocol` name
    RfStreamProtocolStatusErrorParserBitCount = (-7),     ///< Missing `Bit`
    RfStreamProtocolStatusErrorParserKey = (-8),          ///< Missing `Key`
    RfStreamProtocolStatusErrorParserTe = (-9),           ///< Missing `Te`
    RfStreamProtocolStatusErrorParserOthers = (-10),      ///< Missing some other mandatory keys
    // Invalid data
    RfStreamProtocolStatusErrorValueBitCount = (-11), ///< Invalid bit count value
    // Encoder issue
    RfStreamProtocolStatusErrorEncoderGetUpload = (-12), ///< Payload encoder failure
    // Special Values
    RfStreamProtocolStatusErrorProtocolNotFound = (-13), ///< Protocol not found
    RfStreamProtocolStatusReserved = 0x7FFFFFFF,         ///< Prevents enum down-size compiler optimization.
} RfStreamProtocolStatus;

// Allocator and Deallocator
typedef void *(*RfStreamAlloc)(RfStreamEnvironment *environment);
typedef void (*RfStreamFree)(void *context);

// Serialize and Deserialize
typedef RfStreamProtocolStatus (*RfStreamSerialize)(
    void *context, RfRecord *flipper_format, RfStreamRadioPreset *preset
);
typedef RfStreamProtocolStatus (*RfStreamDeserialize)(void *context, RfRecord *flipper_format);

// Decoder specific
typedef void (*RfStreamDecoderFeed)(void *decoder, bool level, uint32_t duration);
typedef void (*RfStreamDecoderReset)(void *decoder);
typedef uint8_t (*RfStreamGetHashData)(void *decoder);
typedef void (*RfStreamGetString)(void *decoder, RfText *output);

// Encoder specific
typedef void (*RfStreamEncoderStop)(void *encoder);
typedef LevelDuration (*RfStreamEncoderYield)(void *context);

typedef struct {
    RfStreamAlloc alloc;
    RfStreamFree free;

    RfStreamDecoderFeed feed;
    RfStreamDecoderReset reset;

    RfStreamGetHashData get_hash_data;
    RfStreamGetString get_string;
    RfStreamSerialize serialize;
    RfStreamDeserialize deserialize;
} RfStreamProtocolDecoder;

typedef struct {
    RfStreamAlloc alloc;
    RfStreamFree free;

    RfStreamDeserialize deserialize;
    RfStreamEncoderStop stop;
    RfStreamEncoderYield yield;
} RfStreamProtocolEncoder;

struct RfStreamCodec {
    const char *name;

    const RfStreamProtocolEncoder *encoder;
    const RfStreamProtocolDecoder *decoder;
};

typedef struct {
    const uint16_t te_long;
    const uint16_t te_short;
    const uint16_t te_delta;
    const uint8_t min_count_bit_for_found;
} RfStreamBlockConst;

typedef struct RfStreamBlockDecoder RfStreamBlockDecoder;

struct RfStreamBlockDecoder {
    uint32_t parser_step;
    uint32_t te_last;
    uint64_t decode_data;
    uint8_t decode_count_bit;
};

void rf_stream_protocol_blocks_add_bit(RfStreamBlockDecoder *decoder, uint8_t bit);

void rf_stream_protocol_blocks_add_to_128_bit(
    RfStreamBlockDecoder *decoder, uint8_t bit, uint64_t *head_64_bit
);

uint8_t rf_stream_protocol_blocks_get_hash_data(RfStreamBlockDecoder *decoder, size_t len);

typedef struct RfStreamBlockGeneric RfStreamBlockGeneric;

struct RfStreamBlockGeneric {
    const char *protocol_name;
    uint64_t data;
    uint32_t serial;
    uint16_t data_count_bit;
    uint8_t btn;
    uint32_t cnt;
};

void rf_stream_block_generic_get_preset_name(const char *preset_name, RfText *preset_str);

RfStreamProtocolStatus rf_stream_block_generic_serialize(
    RfStreamBlockGeneric *instance, RfRecord *flipper_format, RfStreamRadioPreset *preset
);

RfStreamProtocolStatus
rf_stream_block_generic_deserialize(RfStreamBlockGeneric *instance, RfRecord *flipper_format);

RfStreamProtocolStatus rf_stream_block_generic_deserialize_check_count_bit(
    RfStreamBlockGeneric *instance, RfRecord *flipper_format, uint16_t count_bit
);

#define bit_read(value, bit) (((value) >> (bit)) & 0x01)
#define bit_set(value, bit)                                                                                  \
    ({                                                                                                       \
        __typeof__(value) _one = (1);                                                                        \
        (value) |= (_one << (bit));                                                                          \
    })
#define bit_clear(value, bit)                                                                                \
    ({                                                                                                       \
        __typeof__(value) _one = (1);                                                                        \
        (value) &= ~(_one << (bit));                                                                         \
    })
#define bit_write(value, bit, bitvalue) (bitvalue ? bit_set(value, bit) : bit_clear(value, bit))
#define DURATION_DIFF(x, y) (((x) < (y)) ? ((y) - (x)) : ((x) - (y)))

uint64_t rf_stream_protocol_blocks_reverse_key(uint64_t key, uint8_t bit_count);

uint8_t rf_stream_protocol_blocks_get_parity(uint64_t key, uint8_t bit_count);

uint8_t
rf_stream_protocol_blocks_crc4(uint8_t const message[], size_t size, uint8_t polynomial, uint8_t init);

uint8_t
rf_stream_protocol_blocks_crc7(uint8_t const message[], size_t size, uint8_t polynomial, uint8_t init);

uint8_t
rf_stream_protocol_blocks_crc8(uint8_t const message[], size_t size, uint8_t polynomial, uint8_t init);

uint8_t
rf_stream_protocol_blocks_crc8le(uint8_t const message[], size_t size, uint8_t polynomial, uint8_t init);

uint16_t
rf_stream_protocol_blocks_crc16lsb(uint8_t const message[], size_t size, uint16_t polynomial, uint16_t init);

uint16_t
rf_stream_protocol_blocks_crc16(uint8_t const message[], size_t size, uint16_t polynomial, uint16_t init);

uint8_t
rf_stream_protocol_blocks_lfsr_digest8(uint8_t const message[], size_t size, uint8_t gen, uint8_t key);

uint8_t rf_stream_protocol_blocks_lfsr_digest8_reflect(
    uint8_t const message[], size_t size, uint8_t gen, uint8_t key
);

uint16_t
rf_stream_protocol_blocks_lfsr_digest16(uint8_t const message[], size_t size, uint16_t gen, uint16_t key);

uint8_t rf_stream_protocol_blocks_add_bytes(uint8_t const message[], size_t size);

uint8_t rf_stream_protocol_blocks_parity8(uint8_t byte);

uint8_t rf_stream_protocol_blocks_parity_bytes(uint8_t const message[], size_t size);

uint8_t rf_stream_protocol_blocks_xor_bytes(uint8_t const message[], size_t size);

typedef struct RfStreamProtocolDecoderBase RfStreamProtocolDecoderBase;

typedef void (*RfStreamProtocolDecoderBaseRxCallback)(RfStreamProtocolDecoderBase *instance, void *context);

typedef void (*RfStreamProtocolDecoderBaseSerialize)(
    RfStreamProtocolDecoderBase *decoder_base, RfText *output
);

struct RfStreamProtocolDecoderBase {
    // Decoder general section
    const RfStreamCodec *protocol;

    // Callback section
    RfStreamProtocolDecoderBaseRxCallback callback;
    void *context;
};

void rf_stream_protocol_decoder_base_set_decoder_callback(
    RfStreamProtocolDecoderBase *decoder_base, RfStreamProtocolDecoderBaseRxCallback callback, void *context
);

bool rf_stream_protocol_decoder_base_get_string(RfStreamProtocolDecoderBase *decoder_base, RfText *output);

RfStreamProtocolStatus rf_stream_protocol_decoder_base_serialize(
    RfStreamProtocolDecoderBase *decoder_base, RfRecord *flipper_format, RfStreamRadioPreset *preset
);

RfStreamProtocolStatus rf_stream_protocol_decoder_base_deserialize(
    RfStreamProtocolDecoderBase *decoder_base, RfRecord *flipper_format
);

uint8_t rf_stream_protocol_decoder_base_get_hash_data(RfStreamProtocolDecoderBase *decoder_base);

// Encoder Base
typedef struct RfStreamProtocolEncoderBase RfStreamProtocolEncoderBase;

struct RfStreamProtocolEncoderBase {
    // Decoder general section
    const RfStreamCodec *protocol;

    // Callback section
};

#define RF_STREAM_WITH_ENCODER 1
#define RF_STREAM_WITH_DECODER 1

// On-disk field names shared by the stateful vehicle codecs.
#define FF_BIT "Bit"
#define FF_KEY "Key"
#define FF_SERIAL "Serial"
#define FF_BTN "Btn"
#define FF_CNT "Cnt"
#define FF_REPEAT "Repeat"
#define FF_PROTOCOL "Protocol"
#define FF_PRESET "Preset"
#define FF_FREQUENCY "Frequency"
#define FF_MANUFACTURE "Manufacture"
#define FF_TYPE "Type"

bool rf_stream_preset_name_is_custom_marker(const char *preset_name);

const char *rf_stream_get_short_preset_name(const char *preset_name);
bool rf_stream_parse_hex_u64_strict(const char *str, uint64_t *out_key);
bool rf_stream_flipper_read_hex_u64(RfRecord *flipper_format, const char *key, uint64_t *out_key);
void rf_stream_flipper_update_or_insert_u32(RfRecord *flipper_format, const char *key, uint32_t value);

RfStreamProtocolStatus rf_stream_verify_protocol_name(RfRecord *ff, const char *expected_name);

#define RF_FIELD_SERIAL 0x01U
#define RF_FIELD_BTN 0x02U
#define RF_FIELD_CNT 0x04U
#define RF_FIELD_TYPE 0x08U

RfStreamProtocolStatus rf_stream_encoder_read_bit(
    RfRecord *ff, const uint16_t *allowed_bits, size_t allowed_bits_count, uint32_t *out_bit
);

void rf_stream_encoder_read_fields(
    RfRecord *ff, uint32_t *serial_out, uint32_t *btn_out, uint32_t *cnt_out, uint32_t *type_out
);

#define RF_ENCODER_REPEAT_MAX 50U

uint32_t rf_stream_encoder_read_repeat(RfRecord *ff, uint32_t default_repeat);

RfStreamProtocolStatus rf_stream_serialize_fields(
    RfRecord *ff, uint32_t field_mask, uint32_t serial, uint32_t btn, uint32_t cnt, uint32_t type
);

RfStreamProtocolStatus rf_stream_write_display(RfRecord *ff, const char *protocol_name, const char *suffix);

static inline size_t rf_stream_emit(LevelDuration *up, size_t i, size_t cap, bool level, uint32_t us) {
    if (i < cap) up[i++] = level_duration_make(level, us);
    return i;
}

size_t rf_stream_emit_merge(LevelDuration *up, size_t i, size_t cap, bool level, uint32_t us);

static inline size_t
rf_stream_emit_manchester_bit(LevelDuration *up, size_t i, size_t cap, bool bit_value, uint32_t te) {
    i = rf_stream_emit(up, i, cap, bit_value, te);
    i = rf_stream_emit(up, i, cap, !bit_value, te);
    return i;
}

size_t rf_stream_emit_byte_manchester(LevelDuration *up, size_t i, size_t cap, uint8_t value, uint32_t te);

size_t rf_stream_emit_short_pairs(LevelDuration *up, size_t i, size_t cap, uint32_t te, size_t pair_count);

uint8_t rf_stream_reverse_bits8(uint8_t value);
void rf_stream_u64_to_bytes_be(uint64_t data, uint8_t bytes[8]);
uint64_t rf_stream_bytes_to_u64_be(const uint8_t bytes[8]);

static inline bool rf_stream_is_short(uint32_t duration, const RfStreamBlockConst *t) {
    return DURATION_DIFF(duration, t->te_short) < t->te_delta;
}

static inline bool rf_stream_is_long(uint32_t duration, const RfStreamBlockConst *t) {
    return DURATION_DIFF(duration, t->te_long) < t->te_delta;
}

static inline RfManchesterEvent
rf_stream_manchester_event(uint32_t duration, bool level, const RfStreamBlockConst *t) {
    if (DURATION_DIFF(duration, t->te_short) < t->te_delta) {
        return level ? RfManchesterEventShortLow : RfManchesterEventShortHigh;
    }
    if (DURATION_DIFF(duration, t->te_long) < t->te_delta) {
        return level ? RfManchesterEventLongLow : RfManchesterEventLongHigh;
    }
    return RfManchesterEventReset;
}

typedef struct {
    RfStreamProtocolDecoderBase base;
    RfStreamBlockDecoder decoder;
    RfStreamBlockGeneric generic;
} RfStreamDecoderHeader;

uint8_t rf_stream_decoder_hash_blocks(void *context);

void rf_stream_decoder_free_default(void *context);

#define RF_SHARED_UPLOAD_CAPACITY 2048U

#if RF_STREAM_WITH_ENCODER

typedef struct {
    bool is_running;
    size_t repeat;
    size_t front;
    size_t size_upload;
    LevelDuration *upload;

} RfStreamProtocolBlockEncoder;

typedef enum {
    RfStreamProtocolBlockAlignBitLeft,
    RfStreamProtocolBlockAlignBitRight,
} RfStreamProtocolBlockAlignBit;

void rf_stream_protocol_blocks_set_bit_array(
    bool bit_value, uint8_t data_array[], size_t set_index_bit, size_t max_size_array
);

bool rf_stream_protocol_blocks_get_bit_array(uint8_t data_array[], size_t read_index_bit);

size_t rf_stream_protocol_blocks_get_upload_from_bit_array(
    uint8_t data_array[], size_t count_bit_data_array, LevelDuration *upload, size_t max_size_upload,
    uint32_t duration_bit, RfStreamProtocolBlockAlignBit align_bit
);

typedef struct {
    RfStreamProtocolEncoderBase base;
    RfStreamProtocolBlockEncoder encoder;
} RfStreamEncoderHeader;

void rf_stream_encoder_free(void *context);
void rf_stream_encoder_stop(void *context);
LevelDuration rf_stream_encoder_yield(void *context);

void rf_stream_encoder_buffer_ensure(void *context, size_t capacity);

LevelDuration *rf_stream_shared_upload_buffer(void);
size_t rf_stream_shared_upload_capacity(void);
void rf_stream_shared_upload_release(void);

#endif

typedef struct {
    RfText *name;
    uint64_t key;
    uint16_t type;
} RfStreamKey;
typedef struct {
    RfStreamKey *items;
    size_t count;
} RfStreamKeyArray_t;
#define M_EACH(it, arr, type)                                                                                \
    (RfStreamKey *it = (arr).items; (arr).items && it < (arr).items + (arr).count; ++it)
RfStreamKeyArray_t *rf_stream_keystore_get_data(RfStreamKeystore *k);
bool rf_stream_keystore_raw_get_data(const char *name, size_t offset, uint8_t *out, size_t length);
#define M_EACH(it, arr, type)                                                                                \
    (RfStreamKey *it = (arr).items; (arr).items && it < (arr).items + (arr).count; ++it)

struct RfStreamKeystore {
    RfStreamKeyArray_t data;
    const char *mfname;
    uint8_t kl_type;
};

#ifdef __cplusplus
}
#endif
