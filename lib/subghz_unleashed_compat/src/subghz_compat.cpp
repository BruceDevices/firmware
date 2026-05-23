#include <Arduino.h>
#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_rtc.h>
#include <lib/flipper_format/flipper_format.h>

#include <ctype.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

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

struct FlipperFormatCompatEntry {
    std::string key;
    std::string value;
};

struct FlipperFormatCompatData {
    std::vector<FlipperFormatCompatEntry> entries;
    std::unordered_map<std::string, size_t> cursors;
    std::string filetype;
    uint32_t version = 0;
    bool has_header = false;
};

static FlipperFormatCompatData* ff_data(FlipperFormat* ff) {
    if(!ff) return nullptr;
    return reinterpret_cast<FlipperFormatCompatData*>(ff->compat_ctx);
}

static std::string trim_copy(const std::string& input) {
    size_t start = 0;
    size_t end = input.size();
    while(start < end && isspace((unsigned char)input[start])) start++;
    while(end > start && isspace((unsigned char)input[end - 1])) end--;
    return input.substr(start, end - start);
}

static std::string lower_copy(const std::string& input) {
    std::string out = input;
    for(char& c : out) c = (char)tolower((unsigned char)c);
    return out;
}

static bool parse_uint_token(const std::string& token, uint32_t& out) {
    std::string t = trim_copy(token);
    if(t.empty()) return false;

    bool hex = false;
    if(t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) {
        hex = true;
        t = t.substr(2);
    } else {
        for(char c : t) {
            if((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                hex = true;
                break;
            }
        }
    }

    char* end = nullptr;
    unsigned long parsed = strtoul(t.c_str(), &end, hex ? 16 : 10);
    if(end == t.c_str()) return false;
    out = (uint32_t)parsed;
    return true;
}

static bool parse_int_token(const std::string& token, int32_t& out) {
    std::string t = trim_copy(token);
    if(t.empty()) return false;

    bool hex = false;
    size_t i = (t[0] == '-' || t[0] == '+') ? 1 : 0;
    if(i + 2 <= t.size() && t[i] == '0' && (t[i + 1] == 'x' || t[i + 1] == 'X')) hex = true;
    if(!hex) {
        for(; i < t.size(); i++) {
            char c = t[i];
            if((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                hex = true;
                break;
            }
        }
    }

    char* end = nullptr;
    long parsed = strtol(t.c_str(), &end, hex ? 16 : 10);
    if(end == t.c_str()) return false;
    out = (int32_t)parsed;
    return true;
}

static void split_tokens(const std::string& input, std::vector<std::string>& out) {
    out.clear();
    std::string token;
    for(char c : input) {
        if(isspace((unsigned char)c) || c == ',' || c == ';') {
            if(!token.empty()) {
                out.push_back(token);
                token.clear();
            }
        } else {
            token.push_back(c);
        }
    }
    if(!token.empty()) out.push_back(token);
}

static bool ff_load_from_text(FlipperFormat* ff, const char* text) {
    FlipperFormatCompatData* data = ff_data(ff);
    if(!data) return false;

    data->entries.clear();
    data->cursors.clear();
    data->filetype.clear();
    data->version = 0;
    data->has_header = false;

    if(!text) return false;
    String src = String(text);
    src.replace("\r", "");

    int line_start = 0;
    while(line_start <= (int)src.length()) {
        int line_end = src.indexOf('\n', line_start);
        if(line_end < 0) line_end = src.length();
        String line = src.substring(line_start, line_end);
        line.trim();

        if(line.length() > 0) {
            int colon = line.indexOf(':');
            if(colon > 0) {
                String k = line.substring(0, colon);
                String v = line.substring(colon + 1);
                k.trim();
                v.trim();
                std::string key = k.c_str();
                std::string value = v.c_str();
                data->entries.push_back({key, value});
                if(lower_copy(key) == "filetype") {
                    data->filetype = value;
                    data->has_header = true;
                } else if(lower_copy(key) == "version") {
                    uint32_t version = 0;
                    if(parse_uint_token(value, version)) data->version = version;
                }
            } else if(line.startsWith("Version ")) {
                String v = line.substring(strlen("Version "));
                v.trim();
                uint32_t version = 0;
                if(parse_uint_token(v.c_str(), version)) {
                    data->version = version;
                    data->has_header = true;
                }
            }
        }

        if(line_end == (int)src.length()) break;
        line_start = line_end + 1;
    }

    return !data->entries.empty() || data->has_header;
}

static bool ff_find_next_value(
    FlipperFormat* ff,
    const char* key,
    std::string& value_out) {
    FlipperFormatCompatData* data = ff_data(ff);
    if(!data || !key) return false;

    std::string needle = key;
    size_t start = 0;
    auto cursor_it = data->cursors.find(needle);
    if(cursor_it != data->cursors.end()) start = cursor_it->second;

    for(size_t i = start; i < data->entries.size(); i++) {
        if(data->entries[i].key == needle) {
            data->cursors[needle] = i + 1;
            value_out = data->entries[i].value;
            return true;
        }
    }

    // Fallback: case-insensitive lookup.
    const std::string needle_lower = lower_copy(needle);
    for(size_t i = start; i < data->entries.size(); i++) {
        if(lower_copy(data->entries[i].key) == needle_lower) {
            data->cursors[needle] = i + 1;
            value_out = data->entries[i].value;
            return true;
        }
    }

    return false;
}

FlipperFormat* flipper_format_file_alloc(Storage* storage) {
    UNUSED(storage);
    FlipperFormat* ff = (FlipperFormat*)malloc(sizeof(FlipperFormat));
    if(ff) {
        ff->raw = NULL;
        ff->compat_ctx = new FlipperFormatCompatData();
    }
    return ff;
}

void flipper_format_free(FlipperFormat* ff) {
    if(!ff) return;
    delete ff_data(ff);
    ff->compat_ctx = nullptr;
    free(ff);
}

bool flipper_format_file_open_existing(FlipperFormat* ff, const char* path) {
    UNUSED(ff);
    UNUSED(path);
    return false;
}

bool flipper_format_file_open_always(FlipperFormat* ff, const char* path) {
    UNUSED(ff);
    UNUSED(path);
    FlipperFormatCompatData* data = ff_data(ff);
    if(!data) return false;
    data->entries.clear();
    data->cursors.clear();
    data->filetype.clear();
    data->version = 0;
    data->has_header = false;
    return true;
}

bool flipper_format_file_close(FlipperFormat* ff) {
    FlipperFormatCompatData* data = ff_data(ff);
    if(!data) return false;
    data->entries.clear();
    data->cursors.clear();
    data->filetype.clear();
    data->version = 0;
    data->has_header = false;
    return true;
}

bool flipper_format_read_header(FlipperFormat* ff, FuriString* filetype, uint32_t* version) {
    FlipperFormatCompatData* data = ff_data(ff);
    if(!data || !data->has_header) {
        if(filetype) furi_string_set(filetype, "");
        if(version) *version = 0;
        return false;
    }
    if(filetype) furi_string_set(filetype, data->filetype.c_str());
    if(version) *version = data->version;
    return true;
}

bool flipper_format_write_header_cstr(FlipperFormat* ff, const char* type, uint32_t version) {
    FlipperFormatCompatData* data = ff_data(ff);
    if(!data) return false;
    data->filetype = type ? type : "";
    data->version = version;
    data->has_header = true;
    return true;
}

bool flipper_format_rewind(FlipperFormat* ff) {
    FlipperFormatCompatData* data = ff_data(ff);
    if(!data) return false;
    data->cursors.clear();
    return true;
}

bool flipper_format_read_uint32(FlipperFormat* ff, const char* key, uint32_t* out, size_t cnt) {
    if(!out || cnt == 0) return false;
    std::string value;
    if(!ff_find_next_value(ff, key, value)) return false;

    std::vector<std::string> tokens;
    split_tokens(value, tokens);
    if(tokens.size() < cnt) return false;

    for(size_t i = 0; i < cnt; i++) {
        if(!parse_uint_token(tokens[i], out[i])) return false;
    }
    return true;
}

bool flipper_format_read_int32(FlipperFormat* ff, const char* key, int32_t* out, size_t cnt) {
    if(!out || cnt == 0) return false;
    std::string value;
    if(!ff_find_next_value(ff, key, value)) return false;

    std::vector<std::string> tokens;
    split_tokens(value, tokens);
    if(tokens.size() < cnt) return false;

    for(size_t i = 0; i < cnt; i++) {
        if(!parse_int_token(tokens[i], out[i])) return false;
    }
    return true;
}

bool flipper_format_read_bool(FlipperFormat* ff, const char* key, bool* out, size_t cnt) {
    if(!out || cnt == 0) return false;
    std::string value;
    if(!ff_find_next_value(ff, key, value)) return false;

    std::vector<std::string> tokens;
    split_tokens(value, tokens);
    if(tokens.size() < cnt) return false;

    for(size_t i = 0; i < cnt; i++) {
        std::string v = lower_copy(tokens[i]);
        if(v == "1" || v == "true" || v == "yes" || v == "on")
            out[i] = true;
        else if(v == "0" || v == "false" || v == "no" || v == "off")
            out[i] = false;
        else
            return false;
    }
    return true;
}

bool flipper_format_read_hex(FlipperFormat* ff, const char* key, uint8_t* out, size_t cnt) {
    if(!out || cnt == 0) return false;
    std::string value;
    if(!ff_find_next_value(ff, key, value)) return false;

    std::vector<std::string> tokens;
    split_tokens(value, tokens);

    if(tokens.size() >= cnt) {
        for(size_t i = 0; i < cnt; i++) {
            std::string t = tokens[i];
            if(t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) t = t.substr(2);
            char* end = nullptr;
            unsigned long parsed = strtoul(t.c_str(), &end, 16);
            if(end == t.c_str()) return false;
            out[i] = (uint8_t)parsed;
        }
        return true;
    }

    // Compact representation: "AABBCC..." (without separators).
    std::string compact;
    compact.reserve(value.size());
    for(char c : value) {
        if(isxdigit((unsigned char)c)) compact.push_back(c);
    }
    if(compact.size() < cnt * 2) return false;
    for(size_t i = 0; i < cnt; i++) {
        std::string byte = compact.substr(i * 2, 2);
        char* end = nullptr;
        unsigned long parsed = strtoul(byte.c_str(), &end, 16);
        if(end == byte.c_str()) return false;
        out[i] = (uint8_t)parsed;
    }
    return true;
}

bool flipper_format_read_string(FlipperFormat* ff, const char* key, FuriString* out) {
    if(!out) return false;
    std::string value;
    if(!ff_find_next_value(ff, key, value)) {
        furi_string_set(out, "");
        return false;
    }
    furi_string_set(out, value.c_str());
    return true;
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

bool flipper_format_load_from_string(FlipperFormat* ff, const char* text) {
    return ff_load_from_text(ff, text);
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
