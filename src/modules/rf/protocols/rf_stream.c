// SPDX-License-Identifier: GPL-3.0-or-later
// Stateful protocol lifecycle; derived from ProtoPirate. See THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "rf_stream.h"
#include "rf_decode_limits.h"
#include "rf_stream_internal.h"
#include <ctype.h>

#define RF_STREAM_MAX_PULSES 16384U
#define RF_STREAM_MAX_FRAMES 16U
static RfStreamKeystore empty_keystore;
static RfStreamEnvironment environment = {&empty_keystore};

static RfText *dump(const RfRecord *record) {
    RfText *text = rf_text_alloc();
    for (size_t i = 0; i < record->count; ++i)
        rf_text_cat_printf(text, "%s: %s\n", record->fields[i].key, record->fields[i].value);
    return text;
}

bool rf_stream_has_encoder(const RfStreamCodec *codec) {
    return codec && codec->encoder && codec->encoder->alloc && codec->encoder->deserialize &&
           codec->encoder->yield && codec->encoder->free && codec->encoder->stop;
}

static bool preset_read(RfRecord *record, RfStreamRadioPreset *preset) {
    const char *name = rf_stream_ff_get(record, "Preset");
    if (!name || !rf_record_read_uint32(record, "Frequency", &preset->frequency, 1)) return false;
    uint32_t f = preset->frequency;
    if (!((f >= 300000000 && f <= 348000000) || (f >= 387000000 && f <= 464000000) ||
          (f >= 779000000 && f <= 928000000)))
        return false;
    const char *custom = rf_stream_ff_get(record, "Custom_preset_data");
    if (custom) {
        size_t digits = 0;
        for (const char *p = custom; *p; ++p) {
            if (isxdigit((unsigned char)*p)) ++digits;
            else if (!isspace((unsigned char)*p)) return false;
        }
        if (!digits || digits % 2 || digits > 512) return false;
        preset->data_size = digits / 2;
        preset->data = calloc(preset->data_size, 1);
        if (!rf_record_read_hex(record, "Custom_preset_data", preset->data, preset->data_size)) return false;
        const char *module = rf_stream_ff_get(record, "Custom_preset_module");
        if (module && strcmp(module, "CC1101")) return false;
    } else if (!strcmp(name, "FuriHalSubGhzPresetCustom")) return false;
    preset->name = rf_text_alloc();
    rf_text_set(preset->name, name);
    return true;
}

typedef struct {
    const RfStreamCodec *codec;
    RfStreamRadioPreset preset;
    const char *original;
    RfStreamFrame frame;
    void *context;
    unsigned count;
    uint64_t signatures[RF_STREAM_MAX_FRAMES];
    bool failed;
} Decode;

static void decoded(RfStreamProtocolDecoderBase *base, void *opaque) {
    Decode *d = opaque;
    if (d->count >= RF_STREAM_MAX_FRAMES) return;
    // Record storage belongs to the bounded stream allocation scope, not loopTask.
    RfRecord *record = calloc(1, sizeof(*record));
    if (d->codec->decoder->serialize(base, record, &d->preset) != RfStreamProtocolStatusOk) {
        d->failed = true;
        rf_stream_ff_clear(record);
        free(record);
        return;
    }
    RfText *description = rf_text_alloc();
    d->codec->decoder->get_string(base, description);
    RfText *data = dump(record);
    if (d->original) rf_text_set(data, d->original);
    const char *text = rf_text_get_cstr(description);
    bool invalid =
        strstr(text, "CRC:") && (strstr(text, " WRONG") || strstr(text, "CRC:BAD") ||
                                 strstr(text, " - BAD") || strstr(text, "[BAD]") ||
                                 strstr(text, "CRC:FAIL") || strstr(text, "[FAIL]") || strstr(text, "[ERR]"));
    uint64_t signature = 1469598103934665603ULL;
    for (const unsigned char *p = (const unsigned char *)rf_text_get_cstr(data); *p; ++p)
        signature = (signature ^ *p) * 1099511628211ULL;
    bool duplicate = false;
    for (unsigned i = 0; i < d->count; ++i)
        if (d->signatures[i] == signature) duplicate = true;
    if (!duplicate) {
        d->signatures[d->count++] = signature;
        d->frame(
            d->context, rf_text_get_cstr(data), text, rf_stream_has_encoder(d->codec) && !invalid, invalid
        );
    }
    rf_text_free(data);
    rf_text_free(description);
    rf_stream_ff_clear(record);
    free(record);
}

static const char *read_impl(
    const RfStreamCodec *codec, const char *input, const int32_t *pulses, size_t count, RfStreamFrame frame,
    void *context
) {
    if (!codec || !input || strnlen(input, RF_DECODE_MAX_FILE_BYTES + 1) > RF_DECODE_MAX_FILE_BYTES ||
        count > RF_DECODE_MAX_PULSES)
        return "Invalid protocol input";
    // Record storage belongs to the bounded stream allocation scope, not loopTask.
    RfRecord *record = calloc(1, sizeof(*record));
    if (!rf_stream_ff_parse(record, input)) return "Malformed or duplicate fields";
    Decode d = {.codec = codec, .frame = frame, .context = context};
    if (!preset_read(record, &d.preset)) return "Missing or invalid frequency/preset";
    const RfStreamProtocolDecoder *ops = codec->decoder;
    if (!ops || !ops->alloc || !ops->reset || !ops->free || !ops->serialize || !ops->get_string)
        return "Decoder unavailable";
    void *decoder = ops->alloc(&environment);
    if (!decoder) return "Decoder unavailable: required key material is not loaded";
    ops->reset(decoder);
    if (pulses) {
        if (!ops->feed) return "Pulse decoder unavailable";
        RfStreamProtocolDecoderBase *base = decoder;
        base->callback = decoded;
        base->context = &d;
        for (size_t i = 0; i < count && d.count < RF_STREAM_MAX_FRAMES; ++i)
            ops->feed(decoder, pulses[i] > 0, (uint32_t)(pulses[i] > 0 ? pulses[i] : -pulses[i]));
    } else if (ops->deserialize && ops->deserialize(decoder, record) == RfStreamProtocolStatusOk) {
        d.original = input;
        decoded(decoder, &d);
    }
    (ops->free)(decoder);
    return d.failed ? "Protocol serialization failed" : NULL;
}

static const char *
write_impl(const RfStreamCodec *codec, const char *input, RfStreamEncoded encoded, void *context) {
    if (!input || strlen(input) > 16384 || !rf_stream_has_encoder(codec))
        return "Encoder unavailable or invalid file size";
    // Record storage belongs to the bounded stream allocation scope, not loopTask.
    RfRecord *record = calloc(1, sizeof(*record));
    if (!rf_stream_ff_parse(record, input)) return "Malformed or duplicate fields";
    if (!rf_stream_ff_get(record, "Repeat")) {
        uint32_t repeat = 3;
        rf_record_write_uint32(record, "Repeat", &repeat, 1);
    }
    RfStreamRadioPreset preset = {0};
    if (!preset_read(record, &preset)) return "Missing or invalid frequency/preset";
    const RfStreamProtocolEncoder *ops = codec->encoder;
    void *encoder = ops->alloc(&environment);
    if (!encoder) return "Encoder unavailable: required key material is not loaded";
    if (ops->deserialize(encoder, record) != RfStreamProtocolStatusOk)
        return "Encoder rejected fields, framing or unavailable key material";
    int32_t *pulses = calloc(RF_STREAM_MAX_PULSES, sizeof(int32_t));
    size_t count = 0;
    uint64_t duration = 0;
    bool complete = false;
    while (count < RF_STREAM_MAX_PULSES) {
        LevelDuration ld = ops->yield(encoder);
        if (level_duration_is_reset(ld)) {
            complete = true;
            break;
        }
        uint32_t us = level_duration_get_duration(ld);
        if (!us || us > 1000000 || (duration += us) > 10000000) break;
        pulses[count++] = level_duration_get_level(ld) ? (int32_t)us : -(int32_t)us;
    }
    ops->stop(encoder);
    (ops->free)(encoder);
    if (!complete || !count) return "Encoder output exceeds duration/pulse limits or is empty";
    RfText *updated = dump(record);
    encoded(context, pulses, count, rf_text_get_cstr(updated));
    return NULL;
}

// longjmp is confined to C frames; C++ callers own their result vectors.
#define RF_STREAM_RUN(expression)                                                                            \
    rf_stream_memory_begin();                                                                                \
    empty_keystore = (RfStreamKeystore){.mfname = ""};                                                       \
    if (setjmp(*rf_stream_memory_jump())) {                                                                  \
        rf_stream_shared_upload_release();                                                                   \
        rf_stream_memory_finish(NULL);                                                                       \
        return "Protocol input rejected or memory limit reached";                                            \
    }                                                                                                        \
    const char *error = (expression);                                                                        \
    rf_stream_shared_upload_release();                                                                       \
    rf_stream_memory_finish(NULL);                                                                           \
    return error

const char *rf_stream_read(
    const RfStreamCodec *codec, const char *record, const int32_t *pulses, size_t count, RfStreamFrame frame,
    void *context
) {
    RF_STREAM_RUN(read_impl(codec, record, pulses, count, frame, context));
}
const char *
rf_stream_write(const RfStreamCodec *codec, const char *record, RfStreamEncoded encoded, void *context) {
    RF_STREAM_RUN(write_impl(codec, record, encoded, context));
}
#endif
