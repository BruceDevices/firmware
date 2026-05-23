#include <Arduino.h>
#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_rtc.h>
#include <lib/flipper_format/flipper_format.h>

#include <stdio.h>

struct FuriString {
    String s;
};

extern "C" {

FuriString* furi_string_alloc(void) {
    FuriString* s = (FuriString*)malloc(sizeof(FuriString));
    if(s) s->s = "";
    return s;
}

FuriString* furi_string_alloc_set(const char* cstr) {
    FuriString* s = furi_string_alloc();
    if(s) s->s = cstr ? cstr : "";
    return s;
}

void furi_string_free(FuriString* str) {
    if(str) free(str);
}

void furi_string_set(FuriString* str, const char* cstr) {
    if(!str) return;
    str->s = cstr ? cstr : "";
}

void furi_string_set_n(FuriString* out, const FuriString* in, size_t pos, size_t len) {
    if(!out || !in) return;
    out->s = in->s.substring(pos, pos + len);
}

void furi_string_cat(FuriString* str, const char* cstr) {
    if(!str || !cstr) return;
    str->s += cstr;
}

void furi_string_cat_printf(FuriString* str, const char* fmt, ...) {
    if(!str || !fmt) return;
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    str->s += buf;
}

void furi_string_printf(FuriString* str, const char* fmt, ...) {
    if(!str || !fmt) return;
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    str->s = buf;
}

const char* furi_string_get_cstr(const FuriString* str) {
    if(!str) return "";
    return str->s.c_str();
}

size_t furi_string_size(const FuriString* str) {
    if(!str) return 0;
    return str->s.length();
}

void furi_string_trim(FuriString* str) {
    if(!str) return;
    str->s.trim();
}

void furi_delay_ms(uint32_t ms) { delay(ms); }
void furi_delay_tick(uint32_t ticks) { delay(ticks); }
void furi_crash_message(const char* message) {
    UNUSED(message);
    abort();
}

int32_t furi_hal_subghz_get_rolling_counter_mult(void) {
    // Default "disabled" value used by upstream code.
    return -0x7FFFFFFF;
}

uint32_t furi_hal_rtc_get_timestamp(void) { return (uint32_t)millis(); }

void* furi_record_open(const char* record) {
    UNUSED(record);
    return NULL;
}

void furi_record_close(const char* record) { UNUSED(record); }

FlipperFormat* flipper_format_file_alloc(Storage* storage) {
    UNUSED(storage);
    FlipperFormat* ff = (FlipperFormat*)malloc(sizeof(FlipperFormat));
    if(ff) ff->raw = NULL;
    return ff;
}

void flipper_format_free(FlipperFormat* ff) {
    if(ff) free(ff);
}

bool flipper_format_file_open_existing(FlipperFormat* ff, const char* path) {
    UNUSED(ff);
    UNUSED(path);
    return false;
}

bool flipper_format_file_open_always(FlipperFormat* ff, const char* path) {
    UNUSED(ff);
    UNUSED(path);
    return false;
}

bool flipper_format_file_close(FlipperFormat* ff) {
    UNUSED(ff);
    return true;
}

bool flipper_format_read_header(FlipperFormat* ff, FuriString* filetype, uint32_t* version) {
    UNUSED(ff);
    if(filetype) furi_string_set(filetype, "");
    if(version) *version = 0;
    return false;
}

bool flipper_format_write_header_cstr(FlipperFormat* ff, const char* type, uint32_t version) {
    UNUSED(ff);
    UNUSED(type);
    UNUSED(version);
    return true;
}

bool flipper_format_rewind(FlipperFormat* ff) {
    UNUSED(ff);
    return true;
}

bool flipper_format_read_uint32(FlipperFormat* ff, const char* key, uint32_t* out, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(out);
    UNUSED(cnt);
    return false;
}

bool flipper_format_read_int32(FlipperFormat* ff, const char* key, int32_t* out, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(out);
    UNUSED(cnt);
    return false;
}

bool flipper_format_read_bool(FlipperFormat* ff, const char* key, bool* out, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(out);
    UNUSED(cnt);
    return false;
}

bool flipper_format_read_hex(FlipperFormat* ff, const char* key, uint8_t* out, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(out);
    UNUSED(cnt);
    return false;
}

bool flipper_format_read_string(FlipperFormat* ff, const char* key, FuriString* out) {
    UNUSED(ff);
    UNUSED(key);
    if(out) furi_string_set(out, "");
    return false;
}

bool flipper_format_write_uint32(FlipperFormat* ff, const char* key, const uint32_t* in, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(in);
    UNUSED(cnt);
    return true;
}

bool flipper_format_write_int32(FlipperFormat* ff, const char* key, const int32_t* in, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(in);
    UNUSED(cnt);
    return true;
}

bool flipper_format_write_hex(FlipperFormat* ff, const char* key, const uint8_t* in, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(in);
    UNUSED(cnt);
    return true;
}

bool flipper_format_write_string_cstr(FlipperFormat* ff, const char* key, const char* in) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(in);
    return true;
}

bool flipper_format_update_uint32(FlipperFormat* ff, const char* key, const uint32_t* in, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(in);
    UNUSED(cnt);
    return true;
}

bool flipper_format_insert_or_update_uint32(FlipperFormat* ff, const char* key, const uint32_t* in, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(in);
    UNUSED(cnt);
    return true;
}

bool flipper_format_update_hex(FlipperFormat* ff, const char* key, const uint8_t* in, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(in);
    UNUSED(cnt);
    return true;
}

bool flipper_format_insert_or_update_hex(FlipperFormat* ff, const char* key, const uint8_t* in, size_t cnt) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(in);
    UNUSED(cnt);
    return true;
}

bool flipper_format_update_string_cstr(FlipperFormat* ff, const char* key, const char* in) {
    UNUSED(ff);
    UNUSED(key);
    UNUSED(in);
    return true;
}

Stream* flipper_format_get_raw_stream(FlipperFormat* ff) {
    if(!ff) return NULL;
    return ff->raw;
}

size_t stream_size(Stream* s) {
    UNUSED(s);
    return 0;
}

size_t stream_tell(Stream* s) {
    UNUSED(s);
    return 0;
}

bool stream_seek(Stream* s, size_t offset, int origin) {
    UNUSED(s);
    UNUSED(offset);
    UNUSED(origin);
    return false;
}

bool stream_read_line(Stream* s, FuriString* out) {
    UNUSED(s);
    if(out) furi_string_set(out, "");
    return false;
}

void stream_clean(Stream* s) { UNUSED(s); }
void stream_write_cstring(Stream* s, const char* str) {
    UNUSED(s);
    UNUSED(str);
}
void stream_write_char(Stream* s, char c) {
    UNUSED(s);
    UNUSED(c);
}

} // extern "C"
