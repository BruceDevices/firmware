// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "rf_stream_internal.h"
#include "rf_decode_limits.h"

#include <ctype.h>
#include <errno.h>

static char *rf_stream_copy(const char *s, size_t n) {
    char *p = calloc(n + 1, 1);
    if (p) memcpy(p, s, n);
    return p;
}
RfText *rf_text_alloc(void) { return calloc(1, sizeof(RfText)); }
void rf_text_free(RfText *s) {
    if (s) {
        free(s->data);
        free(s);
    }
}
void rf_text_reset(RfText *s) {
    s->len = 0;
    if (s->data) s->data[0] = 0;
}
const char *rf_text_get_cstr(const RfText *s) { return s && s->data ? s->data : ""; }
static bool rf_stream_reserve(RfText *s, size_t n) {
    if (n > 1048576) rf_stream_memory_fail();
    if (n <= s->cap) return true;
    size_t capacity = s->cap ? s->cap : 64;
    while (capacity < n) capacity *= 2;
    char *p = realloc(s->data, capacity);
    if (!p) return false;
    s->data = p;
    s->cap = capacity;
    return true;
}
void rf_text_set(RfText *s, const char *v) {
    size_t n = strlen(v);
    if (rf_stream_reserve(s, n + 1)) {
        memcpy(s->data, v, n + 1);
        s->len = n;
    }
}
static void rf_stream_vprintf(RfText *s, bool append, const char *fmt, va_list ap) {
    va_list aq;
    va_copy(aq, ap);
    int n = vsnprintf(NULL, 0, fmt, aq);
    va_end(aq);
    if (n < 0) return;
    size_t off = append ? s->len : 0;
    if (rf_stream_reserve(s, off + (size_t)n + 1)) {
        vsnprintf(s->data + off, n + 1, fmt, ap);
        s->len = off + n;
    }
}
void rf_text_printf(RfText *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    rf_stream_vprintf(s, false, fmt, ap);
    va_end(ap);
}
void rf_text_cat_printf(RfText *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    rf_stream_vprintf(s, true, fmt, ap);
    va_end(ap);
}
int rf_text_cmp_str(const RfText *s, const char *v) { return strcmp(rf_text_get_cstr(s), v); }
bool rf_text_equal(const RfText *s, const char *v) { return !strcmp(rf_text_get_cstr(s), v); }

void rf_stream_ff_clear(RfRecord *ff) {
    for (size_t i = 0; i < ff->count; i++) {
        free(ff->fields[i].key);
        free(ff->fields[i].value);
    }
    memset(ff, 0, sizeof(*ff));
}
const char *rf_stream_ff_get(const RfRecord *ff, const char *key) {
    for (size_t i = 0; i < ff->count; i++)
        if (!strcmp(key, ff->fields[i].key)) return ff->fields[i].value;
    return NULL;
}
bool rf_record_write_string_cstr(RfRecord *ff, const char *key, const char *v) {
    if (!ff || !key || !v || strlen(key) > 64 || strlen(v) > 4096) return false;
    size_t i = 0;
    while (i < ff->count && strcmp(key, ff->fields[i].key)) i++;
    if (i == 64) return false;
    char *copy = rf_stream_copy(v, strlen(v));
    if (!copy) return false;
    if (i == ff->count) {
        ff->fields[i].key = rf_stream_copy(key, strlen(key));
        if (!ff->fields[i].key) {
            free(copy);
            return false;
        }
        ff->count++;
    }
    free(ff->fields[i].value);
    ff->fields[i].value = copy;
    return true;
}
bool rf_record_rewind(RfRecord *ff) { return ff != NULL; }
bool rf_record_delete_key(RfRecord *ff, const char *key) {
    for (size_t i = 0; i < ff->count; i++)
        if (!strcmp(ff->fields[i].key, key)) {
            free(ff->fields[i].key);
            free(ff->fields[i].value);
            memmove(ff->fields + i, ff->fields + i + 1, (ff->count - i - 1) * sizeof(RfRecordField));
            ff->count--;
            return true;
        }
    return false;
}
bool rf_record_read_string(RfRecord *ff, const char *key, RfText *out) {
    const char *s = rf_stream_ff_get(ff, key);
    if (!s) return false;
    rf_text_set(out, s);
    return true;
}
bool rf_record_read_uint32(RfRecord *ff, const char *key, uint32_t *out, size_t count) {
    const char *s = rf_stream_ff_get(ff, key);
    if (!s) return false;
    for (size_t i = 0; i < count; i++) {
        while (isspace((unsigned char)*s)) s++;
        if (!isdigit((unsigned char)*s)) return false;
        char *end;
        errno = 0;
        unsigned long n = strtoul(s, &end, 10);
        if (errno || n > UINT32_MAX || end == s) return false;
        out[i] = (uint32_t)n;
        s = end;
    }
    while (isspace((unsigned char)*s)) s++;
    return *s == 0;
}
bool rf_record_write_uint32(RfRecord *ff, const char *key, const uint32_t *v, size_t n) {
    if (n > 32) return false;
    char buf[384];
    size_t used = 0;
    for (size_t i = 0; i < n; i++)
        used += snprintf(buf + used, sizeof(buf) - used, "%s%lu", i ? " " : "", (unsigned long)v[i]);
    return rf_record_write_string_cstr(ff, key, buf);
}
bool rf_record_write_hex(RfRecord *ff, const char *key, const uint8_t *v, size_t n) {
    if (n > 256) return false;
    char buf[769];
    size_t used = 0;
    for (size_t i = 0; i < n; i++)
        used += snprintf(buf + used, sizeof(buf) - used, "%s%02X", i ? " " : "", v[i]);
    return rf_record_write_string_cstr(ff, key, buf);
}
static int rf_stream_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)tolower((unsigned char)c);
    return c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
}
bool rf_record_read_hex(RfRecord *ff, const char *key, uint8_t *out, size_t n) {
    const char *s = rf_stream_ff_get(ff, key);
    if (!s) return false;
    for (size_t i = 0; i < n; i++) {
        while (isspace((unsigned char)*s)) s++;
        if (!s[0] || !s[1]) return false;
        int a = rf_stream_nibble(s[0]), b = rf_stream_nibble(s[1]);
        if (a < 0 || b < 0) return false;
        out[i] = (a << 4) | b;
        s += 2;
    }
    while (isspace((unsigned char)*s)) s++;
    return !*s;
}
bool rf_stream_ff_parse(RfRecord *ff, const char *text) {
    rf_stream_ff_clear(ff);
    if (!text || strnlen(text, RF_DECODE_MAX_FILE_BYTES + 1) > RF_DECODE_MAX_FILE_BYTES) return false;
    const char *p = text;
    while (*p) {
        const char *e = strchr(p, '\n');
        if (!e) e = p + strlen(p);
        const char *colon = memchr(p, ':', e - p);
        if (colon && *p != '#') {
            size_t kn = colon - p;
            while (kn && isspace((unsigned char)p[kn - 1])) kn--;
            const char *v = colon + 1;
            while (v < e && isspace((unsigned char)*v)) v++;
            const char *ve = e;
            while (ve > v && isspace((unsigned char)ve[-1])) ve--;
            if (kn > 64) return false;
            char key[65];
            memcpy(key, p, kn);
            key[kn] = 0;
            if (strcmp(key, "RAW_Data") && strcmp(key, "Data_RAW")) {
                if (rf_stream_ff_get(ff, key)) return false;
                char *value = rf_stream_copy(v, ve - v);
                if (!value) return false;
                bool ok = rf_record_write_string_cstr(ff, key, value);
                free(value);
                if (!ok) return false;
            }
        }
        p = *e ? e + 1 : e;
    }
    return true;
}
RfStreamProtocolStatus
rf_stream_block_generic_serialize(RfStreamBlockGeneric *g, RfRecord *ff, RfStreamRadioPreset *p) {
    rf_stream_ff_clear(ff);
    uint32_t bits = g->data_count_bit, version = 1;
    uint8_t key[8];
    for (int i = 0; i < 8; i++) key[7 - i] = (uint8_t)(g->data >> (8 * i));
    if (!rf_record_write_string_cstr(ff, "Filetype", "Flipper SubGhz Key File") ||
        !rf_record_write_uint32(ff, "Version", &version, 1) ||
        !rf_record_write_uint32(ff, "Frequency", &p->frequency, 1) ||
        !rf_record_write_string_cstr(ff, "Preset", rf_text_get_cstr(p->name)) ||
        !rf_record_write_string_cstr(ff, "Protocol", g->protocol_name) ||
        !rf_record_write_uint32(ff, "Bit", &bits, 1) || !rf_record_write_hex(ff, "Key", key, 8))
        return RfStreamProtocolStatusError;
    if (p->data && p->data_size) {
        rf_record_write_string_cstr(ff, "Custom_preset_module", "CC1101");
        rf_record_write_hex(ff, "Custom_preset_data", p->data, p->data_size);
    }
    return RfStreamProtocolStatusOk;
}
RfStreamProtocolStatus rf_stream_block_generic_deserialize(RfStreamBlockGeneric *g, RfRecord *ff) {
    uint32_t bits;
    uint8_t key[8];
    if (!rf_record_read_uint32(ff, "Bit", &bits, 1) || bits > UINT16_MAX)
        return RfStreamProtocolStatusErrorParserBitCount;
    if (!rf_record_read_hex(ff, "Key", key, 8)) return RfStreamProtocolStatusErrorParserKey;
    g->data = 0;
    for (int i = 0; i < 8; i++) g->data = (g->data << 8) | key[i];
    g->data_count_bit = bits;
    return RfStreamProtocolStatusOk;
}
RfStreamProtocolStatus
rf_stream_block_generic_deserialize_check_count_bit(RfStreamBlockGeneric *g, RfRecord *ff, uint16_t bits) {
    RfStreamProtocolStatus r = rf_stream_block_generic_deserialize(g, ff);
    return r                           ? r
           : g->data_count_bit == bits ? RfStreamProtocolStatusOk
                                       : RfStreamProtocolStatusErrorValueBitCount;
}
RfStreamKeystore *rf_stream_environment_get_keystore(RfStreamEnvironment *e) {
    return e ? e->keystore : NULL;
}
RfStreamKeyArray_t *rf_stream_keystore_get_data(RfStreamKeystore *k) { return k ? &k->data : NULL; }
/* Device-encrypted Flipper keystores cannot be treated as plaintext on ESP32. */
bool rf_stream_keystore_raw_get_data(const char *name, size_t off, uint8_t *out, size_t n) {
    (void)name;
    (void)off;
    (void)out;
    (void)n;
    return false;
}

#include <string.h>

uint8_t rf_stream_reverse_bits8(uint8_t value) {
    value = (uint8_t)(((value >> 4U) | (value << 4U)) & 0xFFU);
    value = (uint8_t)(((value & 0x33U) << 2U) | ((value >> 2U) & 0x33U));
    value = (uint8_t)(((value & 0x55U) << 1U) | ((value >> 1U) & 0x55U));
    return value;
}

void rf_stream_u64_to_bytes_be(uint64_t data, uint8_t bytes[8]) {
    for (size_t i = 0; i < 8; i++) { bytes[i] = (uint8_t)((data >> ((7U - i) * 8U)) & 0xFFU); }
}

uint64_t rf_stream_bytes_to_u64_be(const uint8_t bytes[8]) {
    uint64_t data = 0;
    for (size_t i = 0; i < 8; i++) { data = (data << 8U) | bytes[i]; }
    return data;
}

static bool rf_stream_hex_nibble(char c, uint8_t *nibble) {
    if (c >= '0' && c <= '9') {
        *nibble = (uint8_t)(c - '0');
    } else if (c >= 'A' && c <= 'F') {
        *nibble = (uint8_t)(c - 'A' + 10);
    } else if (c >= 'a' && c <= 'f') {
        *nibble = (uint8_t)(c - 'a' + 10);
    } else {
        return false;
    }
    return true;
}

bool rf_stream_parse_hex_u64_strict(const char *str, uint64_t *out_key) {
    if (!str || !out_key) { return false; }

    uint64_t key = 0;
    uint8_t hex_pos = 0;
    for (size_t i = 0; str[i] != '\0' && hex_pos < 16; i++) {
        if (str[i] == ' ') { continue; }

        uint8_t nibble = 0;
        if (!rf_stream_hex_nibble(str[i], &nibble)) { return false; }
        key = (key << 4) | nibble;
        hex_pos++;
    }

    if (hex_pos != 16) { return false; }
    *out_key = key;
    return true;
}

bool rf_stream_flipper_read_hex_u64(RfRecord *flipper_format, const char *key, uint64_t *out_key) {
    RfText *value = rf_text_alloc();
    if (!value) { return false; }

    bool ok = false;
    if (rf_record_rewind(flipper_format) && rf_record_read_string(flipper_format, key, value)) {
        ok = rf_stream_parse_hex_u64_strict(rf_text_get_cstr(value), out_key);
    }
    rf_text_free(value);
    return ok;
}

void rf_stream_flipper_update_or_insert_u32(RfRecord *flipper_format, const char *key, uint32_t value) {
    rf_record_rewind(flipper_format);
    if (!rf_record_update_uint32(flipper_format, key, &value, 1)) {
        rf_record_rewind(flipper_format);
        rf_record_insert_or_update_uint32(flipper_format, key, &value, 1);
    }
}

RfStreamProtocolStatus rf_stream_verify_protocol_name(RfRecord *ff, const char *expected_name) {
    if (!ff || !expected_name) { return RfStreamProtocolStatusError; }
    RfText *tmp = rf_text_alloc();
    if (!tmp) { return RfStreamProtocolStatusError; }
    RfStreamProtocolStatus result = RfStreamProtocolStatusError;
    if (!rf_record_read_string(ff, FF_PROTOCOL, tmp)) {
        result = RfStreamProtocolStatusErrorParserOthers;
    } else if (rf_text_equal(tmp, expected_name)) {
        result = RfStreamProtocolStatusOk;
    }
    rf_text_free(tmp);
    return result;
}

RfStreamProtocolStatus rf_stream_encoder_read_bit(
    RfRecord *ff, const uint16_t *allowed_bits, size_t allowed_bits_count, uint32_t *out_bit
) {
    if (!ff || !out_bit) return RfStreamProtocolStatusError;
    rf_record_rewind(ff);
    uint32_t bit = 0;
    if (!rf_record_read_uint32(ff, FF_BIT, &bit, 1)) { return RfStreamProtocolStatusErrorValueBitCount; }
    if (allowed_bits && allowed_bits_count) {
        bool ok = false;
        for (size_t i = 0; i < allowed_bits_count; i++) {
            if ((uint32_t)allowed_bits[i] == bit) {
                ok = true;
                break;
            }
        }
        if (!ok) return RfStreamProtocolStatusError;
    }
    *out_bit = bit;
    return RfStreamProtocolStatusOk;
}

void rf_stream_encoder_read_fields(
    RfRecord *ff, uint32_t *serial_out, uint32_t *btn_out, uint32_t *cnt_out, uint32_t *type_out
) {
    if (!ff) return;
    uint32_t tmp = 0;
    if (serial_out) {
        rf_record_rewind(ff);
        if (rf_record_read_uint32(ff, FF_SERIAL, &tmp, 1)) *serial_out = tmp;
    }
    if (btn_out) {
        rf_record_rewind(ff);
        if (rf_record_read_uint32(ff, FF_BTN, &tmp, 1)) *btn_out = tmp;
    }
    if (cnt_out) {
        rf_record_rewind(ff);
        if (rf_record_read_uint32(ff, FF_CNT, &tmp, 1)) *cnt_out = tmp;
    }
    if (type_out) {
        rf_record_rewind(ff);
        if (rf_record_read_uint32(ff, FF_TYPE, &tmp, 1)) *type_out = tmp;
    }
}

uint32_t rf_stream_encoder_read_repeat(RfRecord *ff, uint32_t default_repeat) {
    if (!ff) return default_repeat;
    rf_record_rewind(ff);
    uint32_t tmp = 0;
    if (!rf_record_read_uint32(ff, FF_REPEAT, &tmp, 1)) { return default_repeat; }

    if (tmp == 0) { return default_repeat; }
    if (tmp > RF_ENCODER_REPEAT_MAX) { tmp = RF_ENCODER_REPEAT_MAX; }
    return tmp;
}

RfStreamProtocolStatus rf_stream_serialize_fields(
    RfRecord *ff, uint32_t field_mask, uint32_t serial, uint32_t btn, uint32_t cnt, uint32_t type
) {
    if (!ff) return RfStreamProtocolStatusError;

    if (field_mask & RF_FIELD_SERIAL) { rf_stream_flipper_update_or_insert_u32(ff, FF_SERIAL, serial); }
    if (field_mask & RF_FIELD_BTN) { rf_stream_flipper_update_or_insert_u32(ff, FF_BTN, btn); }
    if (field_mask & RF_FIELD_CNT) { rf_stream_flipper_update_or_insert_u32(ff, FF_CNT, cnt); }
    if (field_mask & RF_FIELD_TYPE) { rf_stream_flipper_update_or_insert_u32(ff, FF_TYPE, type); }
    return RfStreamProtocolStatusOk;
}

RfStreamProtocolStatus rf_stream_write_display(RfRecord *ff, const char *protocol_name, const char *suffix) {
    if (!ff || !protocol_name || !suffix) { return RfStreamProtocolStatusError; }
    RfText *display = rf_text_alloc();
    if (!display) { return RfStreamProtocolStatusError; }
    rf_text_printf(display, "%s - %s", protocol_name, suffix);
    rf_record_rewind(ff);
    RfStreamProtocolStatus status =
        rf_record_insert_or_update_string_cstr(ff, "Disp", rf_text_get_cstr(display))
            ? RfStreamProtocolStatusOk
            : RfStreamProtocolStatusErrorParserOthers;
    rf_text_free(display);
    return status;
}

size_t rf_stream_emit_merge(LevelDuration *up, size_t i, size_t cap, bool level, uint32_t us) {
    if (i > 0 && level_duration_get_level(up[i - 1]) == level) {
        uint32_t prev = level_duration_get_duration(up[i - 1]);
        up[i - 1] = level_duration_make(level, prev + us);
        return i;
    }
    if (i < cap) up[i++] = level_duration_make(level, us);
    return i;
}

size_t rf_stream_emit_byte_manchester(LevelDuration *up, size_t i, size_t cap, uint8_t value, uint32_t te) {
    for (int8_t bit = 7; bit >= 0; bit--) {
        bool bit_value = ((value >> bit) & 1) != 0;
        i = rf_stream_emit_manchester_bit(up, i, cap, bit_value, te);
    }
    return i;
}

size_t rf_stream_emit_short_pairs(LevelDuration *up, size_t i, size_t cap, uint32_t te, size_t pair_count) {
    for (size_t p = 0; p < pair_count; p++) {
        i = rf_stream_emit(up, i, cap, true, te);
        i = rf_stream_emit(up, i, cap, false, te);
    }
    return i;
}

#if RF_STREAM_WITH_ENCODER

void rf_stream_encoder_free(void *context) {
    RfStreamEncoderHeader *hdr = context;
    hdr->encoder.upload = NULL;
    hdr->encoder.size_upload = 0;
    free(hdr);
}

void rf_stream_encoder_stop(void *context) {
    RfStreamEncoderHeader *hdr = context;
    hdr->encoder.is_running = false;
    hdr->encoder.front = 0;
}

LevelDuration rf_stream_encoder_yield(void *context) {
    RfStreamEncoderHeader *hdr = context;
    if (hdr->encoder.repeat == 0 || !hdr->encoder.is_running || hdr->encoder.size_upload == 0) {
        hdr->encoder.is_running = false;
        return level_duration_reset();
    }
    LevelDuration ret = hdr->encoder.upload[hdr->encoder.front];
    if (++hdr->encoder.front == hdr->encoder.size_upload) {
        hdr->encoder.repeat--;
        hdr->encoder.front = 0;
    }
    return ret;
}

static LevelDuration *rf_stream_shared_upload_buf = NULL;

LevelDuration *rf_stream_shared_upload_buffer(void) {
    if (rf_stream_shared_upload_buf == NULL) {
        rf_stream_shared_upload_buf = malloc(RF_SHARED_UPLOAD_CAPACITY * sizeof(LevelDuration));
    }
    return rf_stream_shared_upload_buf;
}

size_t rf_stream_shared_upload_capacity(void) { return RF_SHARED_UPLOAD_CAPACITY; }

void rf_stream_shared_upload_release(void) {
    free(rf_stream_shared_upload_buf);
    rf_stream_shared_upload_buf = NULL;
}

void rf_stream_encoder_buffer_ensure(void *context, size_t capacity) {
    RfStreamEncoderHeader *hdr = context;
    hdr->encoder.upload = rf_stream_shared_upload_buffer();
    hdr->encoder.size_upload = capacity;
}

#endif

uint8_t rf_stream_decoder_hash_blocks(void *context) {
    RfStreamDecoderHeader *hdr = context;
    return rf_stream_protocol_blocks_get_hash_data(&hdr->decoder, (hdr->decoder.decode_count_bit / 8U) + 1U);
}

void rf_stream_decoder_free_default(void *context) { free(context); }

bool rf_stream_preset_name_is_custom_marker(const char *preset_name) {
    return preset_name &&
           (!strcmp(preset_name, "Custom") || !strcmp(preset_name, "CUSTOM") ||
            !strcmp(preset_name, "FuriHalSubGhzPresetCustom") || strstr(preset_name, "PresetCustom"));
}

const char *rf_stream_get_short_preset_name(const char *preset_name) {
    if (!preset_name || preset_name[0] == '\0') return "AM650";

    if (strstr(preset_name, "Ook650") || strstr(preset_name, "OOK650")) return "AM650";
    if (strstr(preset_name, "Ook270") || strstr(preset_name, "OOK270")) return "AM270";
    if (strstr(preset_name, "2FSKDev238") || strstr(preset_name, "Dev238")) return "FM238";
    if (strstr(preset_name, "2FSKDev12K") || strstr(preset_name, "Dev12K")) return "FM12K";
    if (strstr(preset_name, "2FSKDev476") || strstr(preset_name, "Dev476")) return "FM476";
    if (rf_stream_preset_name_is_custom_marker(preset_name)) return "Custom";

    if (!strcmp(preset_name, "AM650")) return "AM650";
    if (!strcmp(preset_name, "AM270")) return "AM270";
    if (!strcmp(preset_name, "FM238")) return "FM238";
    if (!strcmp(preset_name, "FM12K")) return "FM12K";
    if (!strcmp(preset_name, "FM476")) return "FM476";

    return preset_name;
}

#define TAG "SubGhzBlockDecoder"

void rf_stream_protocol_blocks_add_bit(RfStreamBlockDecoder *decoder, uint8_t bit) {
    decoder->decode_data = decoder->decode_data << 1 | bit;
    decoder->decode_count_bit++;
}

void rf_stream_protocol_blocks_add_to_128_bit(
    RfStreamBlockDecoder *decoder, uint8_t bit, uint64_t *head_64_bit
) {
    if (++decoder->decode_count_bit > 64) {
        (*head_64_bit) = ((*head_64_bit) << 1) | (decoder->decode_data >> 63);
    }
    decoder->decode_data = decoder->decode_data << 1 | bit;
}

uint8_t rf_stream_protocol_blocks_get_hash_data(RfStreamBlockDecoder *decoder, size_t len) {
    uint8_t hash = 0;
    uint8_t *p = (uint8_t *)&decoder->decode_data;
    for (size_t i = 0; i < len; i++) { hash ^= p[i]; }
    return hash;
}

uint64_t rf_stream_protocol_blocks_reverse_key(uint64_t key, uint8_t bit_count) {
    uint64_t reverse_key = 0;
    for (uint8_t i = 0; i < bit_count; i++) { reverse_key = reverse_key << 1 | bit_read(key, i); }
    return reverse_key;
}

uint8_t rf_stream_protocol_blocks_get_parity(uint64_t key, uint8_t bit_count) {
    uint8_t parity = 0;
    for (uint8_t i = 0; i < bit_count; i++) { parity += bit_read(key, i); }
    return parity & 0x01;
}

uint8_t
rf_stream_protocol_blocks_crc4(uint8_t const message[], size_t size, uint8_t polynomial, uint8_t init) {
    uint8_t remainder = init << 4; // LSBs are unused
    uint8_t poly = polynomial << 4;
    uint8_t bit;

    while (size--) {
        remainder ^= *message++;
        for (bit = 0; bit < 8; bit++) {
            if (remainder & 0x80) {
                remainder = (remainder << 1) ^ poly;
            } else {
                remainder = (remainder << 1);
            }
        }
    }
    return remainder >> 4 & 0x0f; // discard the LSBs
}

uint8_t
rf_stream_protocol_blocks_crc7(uint8_t const message[], size_t size, uint8_t polynomial, uint8_t init) {
    uint8_t remainder = init << 1; // LSB is unused
    uint8_t poly = polynomial << 1;

    for (size_t byte = 0; byte < size; ++byte) {
        remainder ^= message[byte];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (remainder & 0x80) {
                remainder = (remainder << 1) ^ poly;
            } else {
                remainder = (remainder << 1);
            }
        }
    }
    return remainder >> 1 & 0x7f; // discard the LSB
}

uint8_t
rf_stream_protocol_blocks_crc8(uint8_t const message[], size_t size, uint8_t polynomial, uint8_t init) {
    uint8_t remainder = init;

    for (size_t byte = 0; byte < size; ++byte) {
        remainder ^= message[byte];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (remainder & 0x80) {
                remainder = (remainder << 1) ^ polynomial;
            } else {
                remainder = (remainder << 1);
            }
        }
    }
    return remainder;
}

uint8_t
rf_stream_protocol_blocks_crc8le(uint8_t const message[], size_t size, uint8_t polynomial, uint8_t init) {
    uint8_t remainder = rf_stream_protocol_blocks_reverse_key(init, 8);
    polynomial = rf_stream_protocol_blocks_reverse_key(polynomial, 8);

    for (size_t byte = 0; byte < size; ++byte) {
        remainder ^= message[byte];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (remainder & 1) {
                remainder = (remainder >> 1) ^ polynomial;
            } else {
                remainder = (remainder >> 1);
            }
        }
    }
    return remainder;
}

uint16_t
rf_stream_protocol_blocks_crc16lsb(uint8_t const message[], size_t size, uint16_t polynomial, uint16_t init) {
    uint16_t remainder = init;

    for (size_t byte = 0; byte < size; ++byte) {
        remainder ^= message[byte];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (remainder & 1) {
                remainder = (remainder >> 1) ^ polynomial;
            } else {
                remainder = (remainder >> 1);
            }
        }
    }
    return remainder;
}

uint16_t
rf_stream_protocol_blocks_crc16(uint8_t const message[], size_t size, uint16_t polynomial, uint16_t init) {
    uint16_t remainder = init;

    for (size_t byte = 0; byte < size; ++byte) {
        remainder ^= message[byte] << 8;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (remainder & 0x8000) {
                remainder = (remainder << 1) ^ polynomial;
            } else {
                remainder = (remainder << 1);
            }
        }
    }
    return remainder;
}

uint8_t
rf_stream_protocol_blocks_lfsr_digest8(uint8_t const message[], size_t size, uint8_t gen, uint8_t key) {
    uint8_t sum = 0;
    for (size_t byte = 0; byte < size; ++byte) {
        uint8_t data = message[byte];
        for (int i = 7; i >= 0; --i) {
            // XOR key into sum if data bit is set
            if ((data >> i) & 1) sum ^= key;

            // roll the key right (actually the LSB is dropped here)
            // and apply the gen (needs to include the dropped LSB as MSB)
            if (key & 1) key = (key >> 1) ^ gen;
            else key = (key >> 1);
        }
    }
    return sum;
}

uint8_t rf_stream_protocol_blocks_lfsr_digest8_reflect(
    uint8_t const message[], size_t size, uint8_t gen, uint8_t key
) {
    uint8_t sum = 0;
    // Process message from last byte to first byte (reflected)
    for (int byte = size - 1; byte >= 0; --byte) {
        uint8_t data = message[byte];
        // Process individual bits of each byte (reflected)
        for (uint8_t i = 0; i < 8; ++i) {
            // XOR key into sum if data bit is set
            if ((data >> i) & 1) { sum ^= key; }

            // roll the key left (actually the LSB is dropped here)
            // and apply the gen (needs to include the dropped lsb as MSB)
            if (key & 0x80) key = (key << 1) ^ gen;
            else key = (key << 1);
        }
    }
    return sum;
}

uint16_t
rf_stream_protocol_blocks_lfsr_digest16(uint8_t const message[], size_t size, uint16_t gen, uint16_t key) {
    uint16_t sum = 0;
    for (size_t byte = 0; byte < size; ++byte) {
        uint8_t data = message[byte];
        for (int8_t i = 7; i >= 0; --i) {
            // if data bit is set then xor with key
            if ((data >> i) & 1) sum ^= key;

            // roll the key right (actually the LSB is dropped here)
            // and apply the gen (needs to include the dropped LSB as MSB)
            if (key & 1) key = (key >> 1) ^ gen;
            else key = (key >> 1);
        }
    }
    return sum;
}

uint8_t rf_stream_protocol_blocks_add_bytes(uint8_t const message[], size_t size) {
    uint32_t result = 0;
    for (size_t i = 0; i < size; ++i) { result += message[i]; }
    return (uint8_t)result;
}

uint8_t rf_stream_protocol_blocks_parity8(uint8_t byte) {
    byte ^= byte >> 4;
    byte &= 0xf;
    return (0x6996 >> byte) & 1;
}

uint8_t rf_stream_protocol_blocks_parity_bytes(uint8_t const message[], size_t size) {
    uint8_t result = 0;
    for (size_t i = 0; i < size; ++i) { result ^= rf_stream_protocol_blocks_parity8(message[i]); }
    return result;
}

uint8_t rf_stream_protocol_blocks_xor_bytes(uint8_t const message[], size_t size) {
    uint8_t result = 0;
    for (size_t i = 0; i < size; ++i) { result ^= message[i]; }
    return result;
}

bool rf_stream_keystore_add_key(const char *name, uint64_t key, uint16_t type) {
    // Add a key to the keystore
    // This is a stub implementation - actual keystore management is handled by the RF protocol engine
    (void)name; (void)key; (void)type;
    return true;
}

void rf_stream_keystore_raw_blob_set(const uint8_t *data, size_t size) {
    // Set a raw blob of key data for the keystore
    (void)data; (void)size;
}

void rf_stream_keystore_clear_keys(void) {
    // Clear all keys from the keystore
}

#endif
