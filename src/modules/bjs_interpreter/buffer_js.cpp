#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "buffer_js.h"

#include <vector>

#include "helpers_js.h"
#include "user_classes_js.h"

#include "mbedtls/base64.h"

static JSValue buffer_decode_base64(JSContext *ctx, const uint8_t *input, size_t input_len, size_t *out_len) {
    if (!input) { return JS_ThrowTypeError(ctx, "Buffer.from: invalid input"); }

    size_t out_max = (input_len / 4) * 3 + 3;
    uint8_t *out = (uint8_t *)malloc(out_max);

    if (!out) { return JS_ThrowOutOfMemory(ctx); }

    int rc = mbedtls_base64_decode(out, out_max, out_len, input, input_len);

    if (rc != 0) {
        free(out);
        return JS_ThrowTypeError(ctx, "Buffer.from: invalid base64");
    }

    JSValue bytes = JS_NewUint8ArrayCopy(ctx, out, *out_len);
    free(out);
    return bytes;
}

JSValue native_buffer_from(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "Buffer.from: arg0 must be string");
    }

    const char *enc = NULL;
    if (argc > 1 && !JS_IsUndefined(argv[1])) {
        if (!JS_IsString(ctx, argv[1])) {
            return JS_ThrowTypeError(ctx, "Buffer.from: arg1 must be string encoding");
        }
        JSCStringBuf enc_sb;
        enc = JS_ToCString(ctx, argv[1], &enc_sb);
    }

    size_t input_len = 0;
    JSCStringBuf input_sb;
    const char *input = JS_ToCStringLen(ctx, &input_len, argv[0], &input_sb);
    if (!input) { return JS_ThrowTypeError(ctx, "Buffer.from: invalid string"); }

    JSValue bytes;
    size_t out_len = 0;
    if (!enc || strcmp(enc, "utf8") == 0 || strcmp(enc, "utf-8") == 0) {
        bytes = JS_NewUint8ArrayCopy(ctx, (const uint8_t *)input, input_len);
        out_len = input_len;
    } else if (strcmp(enc, "base64") == 0) {
        bytes = buffer_decode_base64(ctx, (const uint8_t *)input, input_len, &out_len);
    } else {
        return JS_ThrowTypeError(ctx, "Buffer.from: unsupported encoding");
    }

    if (JS_IsException(bytes)) { return bytes; }

    JSValue obj = JS_NewObjectClassUser(ctx, JS_CLASS_BUFFER);
    if (JS_IsException(obj)) { return obj; }

    JS_SetPropertyStr(ctx, obj, "_data", bytes);
    JS_SetPropertyStr(ctx, obj, "length", JS_NewUint32(ctx, out_len));

    return obj;
}

JSValue native_buffer_toString(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    const char *enc = NULL;
    if (argc > 0 && !JS_IsUndefined(argv[0])) {
        if (!JS_IsString(ctx, argv[0])) {
            return JS_ThrowTypeError(ctx, "Buffer.toString: encoding must be string");
        }
        JSCStringBuf enc_sb;
        enc = JS_ToCString(ctx, argv[0], &enc_sb);
    }

    /* Node default is utf8. We support utf8/utf-8 plus binary/latin1. */
    bool is_binary = false;
    if (enc) {
        if (strcmp(enc, "binary") == 0 || strcmp(enc, "latin1") == 0) {
            is_binary = true;
        } else if (strcmp(enc, "utf8") == 0 || strcmp(enc, "utf-8") == 0) {
            is_binary = false;
        } else {
            return JS_ThrowTypeError(ctx, "Buffer.toString: unsupported encoding");
        }
    }

    JSValue data = JS_GetPropertyStr(ctx, *this_val, "_data");
    if (JS_IsUndefined(data) || !JS_IsTypedArray(ctx, data)) {
        return JS_ThrowTypeError(ctx, "Buffer.toString: invalid Buffer");
    }

    size_t len = 0;
    const char *buf = JS_GetTypedArrayBuffer(ctx, &len, data);
    if (!buf) { return JS_ThrowTypeError(ctx, "Buffer.toString: invalid backing store"); }

    if (is_binary) { return buffer_latin1_to_string(ctx, (const uint8_t *)buf, len); }

    return JS_NewStringLen(ctx, buf, len);
}

#endif
