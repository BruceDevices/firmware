#ifndef __HELPERS_JS_H__
#define __HELPERS_JS_H__
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "core/serialcmds.h"
#include <FS.h>

extern "C" {
#include <mquickjs.h>
}

#include <globals.h>
#include <string.h>

extern "C" {
void print_errorMessage(const char *msg, const char *stackTrace = NULL);
void js_fatal_error_handler(JSContext *ctx);
bool JS_IsTypedArray(JSContext *ctx, JSValue val);
JSValue js_fill(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

struct FileParamsJS {
    FS *fs;
    String path;
    bool exist;
    u_int8_t paramOffset;
};
FileParamsJS
js_get_path_from_params(JSContext *ctx, JSValue *argv, bool checkIfexist = true, bool legacy = false);

JSValue js_value_from_json_variant(JSContext *ctx, JsonVariantConst value);

void internal_print(
    JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, uint8_t printTft, uint8_t newLine
);

JSValue buffer_latin1_to_string(JSContext *ctx, const uint8_t *buf, size_t len);
bool buffer_latin1_string_to_bytes(const uint8_t *s, size_t len, uint8_t **out_bytes, size_t *out_len);

#endif
#endif
