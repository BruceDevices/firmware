#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "helpers_js.h"
#include "core/sd_functions.h"
#include <globals.h>

#include <math.h>
#include <string.h>

void print_errorMessage(const char *msg, const char *stackTrace) {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("Error", tftWidth / 2, 10, 1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(0, 33);

    tft.printf("%s\n%s\n", msg, stackTrace);
    Serial.printf("%s\n%s\n", msg, stackTrace);
    Serial.flush();

    delay(500);
    while (!check(AnyKeyPress)) delay(50);
}

void js_fatal_error_handler(JSContext *ctx) {
    JSValue obj;
    JSCStringBuf sb;
    obj = JS_GetException(ctx);

    JSValue jsvMessage = JS_GetPropertyStr(ctx, obj, "message");
    if (strcmp(JS_ToCString(ctx, jsvMessage, &sb), "Script exited") == 0) { return; }

    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("Error", tftWidth / 2, 10, 1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(0, 33);

    JSValue jsvStack = JS_GetPropertyStr(ctx, obj, "stack");
    const char *stackTrace = NULL;
    if (!JS_IsUndefined(jsvStack) && JS_IsString(ctx, jsvStack)) {
        stackTrace = JS_ToCString(ctx, jsvStack, &sb);
    } else {
        /* fallback to exception's string representation */
        stackTrace = JS_ToCString(ctx, obj, &sb);
    }

    JS_PrintValueF(ctx, obj, JS_DUMP_LONG);
    const char *msg = JS_ToCString(ctx, obj, &sb);

    print_errorMessage(msg != NULL ? msg : "JS Error", stackTrace);
}

bool JS_IsTypedArray(JSContext *ctx, JSValue val) {
    int classId = JS_GetClassID(ctx, val);
    return (classId >= JS_CLASS_ARRAY_BUFFER && classId <= JS_CLASS_UINT32_ARRAY);
}

static int32_t js_to_integer_clamped(double v, int32_t min_value, int32_t max_value, int32_t default_value) {
    if (isnan(v)) return default_value;
    if (v < (double)min_value) return min_value;
    if (v > (double)max_value) return max_value;
    return (int32_t)v;
}

JSValue js_fill(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (!JS_IsObject(ctx, *this_val)) { return JS_ThrowTypeError(ctx, "fill() called on non-object"); }

    int classId = JS_GetClassID(ctx, *this_val);
    /* Non-standard but useful: ArrayBuffer.prototype.fill(value, start?, end?) as byte fill. */
    if (classId == JS_CLASS_ARRAY_BUFFER) {
        double len_d = 0;
        JSValue len_val = JS_GetPropertyStr(ctx, *this_val, "byteLength");
        if (!JS_IsUndefined(len_val)) { JS_ToNumber(ctx, &len_d, len_val); }
        if (len_d < 0) len_d = 0;
        if (len_d > 0x7fffffff) len_d = 0x7fffffff;
        int32_t len = (int32_t)len_d;

        double start_d = 0;
        if (argc > 1 && !JS_IsUndefined(argv[1])) { JS_ToNumber(ctx, &start_d, argv[1]); }
        double end_d = (double)len;
        if (argc > 2 && !JS_IsUndefined(argv[2])) { JS_ToNumber(ctx, &end_d, argv[2]); }

        int32_t start = js_to_integer_clamped(start_d, -len, len, 0);
        int32_t end = js_to_integer_clamped(end_d, -len, len, len);
        if (start < 0) start = len + start;
        if (end < 0) end = len + end;
        if (start < 0) start = 0;
        if (end < 0) end = 0;
        if (start > len) start = len;
        if (end > len) end = len;
        if (end <= start) return *this_val;

        double v_d = 0;
        JSValue fill_value = (argc > 0) ? argv[0] : JS_UNDEFINED;
        if (!JS_IsUndefined(fill_value) && !JS_IsNull(fill_value) && JS_IsNumber(ctx, fill_value)) {
            JS_ToNumber(ctx, &v_d, fill_value);
        } else if (!JS_IsUndefined(fill_value) && !JS_IsNull(fill_value) && JS_IsBool(fill_value)) {
            v_d = JS_ToBool(ctx, fill_value) ? 1.0 : 0.0;
        } else if (JS_IsUndefined(fill_value) || JS_IsNull(fill_value)) {
            v_d = 0;
        } else {
            JS_ToNumber(ctx, &v_d, fill_value);
        }

        int iv = 0;
        if (v_d != v_d) iv = 0; /* NaN */
        else iv = (int)v_d;
        uint8_t b = (uint8_t)(iv & 0xff);

        JSValue global = JS_GetGlobalObject(ctx);
        JSValue ctor = JS_GetPropertyStr(ctx, global, "Uint8Array");
        if (!JS_IsFunction(ctx, ctor)) {
            return JS_ThrowTypeError(ctx, "ArrayBuffer.fill: Uint8Array not available");
        }

        if (JS_StackCheck(ctx, 3)) { return JS_ThrowOutOfMemory(ctx); }
        JS_PushArg(ctx, *this_val); /* arg0: buffer */
        JS_PushArg(ctx, ctor);      /* constructor */
        JS_PushArg(ctx, JS_NULL);   /* this */
        JSValue view = JS_Call(ctx, FRAME_CF_CTOR | 1);
        if (JS_IsException(view)) { return view; }

        size_t byte_len = 0;
        char *raw = (char *)JS_GetTypedArrayBuffer(ctx, &byte_len, view);
        if (!raw) return JS_ThrowTypeError(ctx, "ArrayBuffer.fill: invalid backing store");
        if ((int32_t)byte_len < len) len = (int32_t)byte_len;
        if (start > len) start = len;
        if (end > len) end = len;

        memset((uint8_t *)raw + (size_t)start, b, (size_t)(end - start));
        return *this_val;
    }

    /* Fast path for TypedArray.fill(): operate on the backing store directly.
       This avoids creating JS values per element (much faster on ESP32). */
    if (JS_IsTypedArray(ctx, *this_val)) {
        size_t byte_len = 0;
        char *raw = (char *)JS_GetTypedArrayBuffer(ctx, &byte_len, *this_val);
        if (!raw) return JS_ThrowTypeError(ctx, "fill() invalid TypedArray backing store");

        /* length is in elements (not bytes) */
        double len_d = 0;
        JSValue len_val = JS_GetPropertyStr(ctx, *this_val, "length");
        if (!JS_IsUndefined(len_val)) { JS_ToNumber(ctx, &len_d, len_val); }
        if (len_d < 0) len_d = 0;
        if (len_d > 0x7fffffff) len_d = 0x7fffffff;
        int32_t len = (int32_t)len_d;

        double start_d = 0;
        if (argc > 1 && !JS_IsUndefined(argv[1])) { JS_ToNumber(ctx, &start_d, argv[1]); }
        double end_d = (double)len;
        if (argc > 2 && !JS_IsUndefined(argv[2])) { JS_ToNumber(ctx, &end_d, argv[2]); }

        int32_t start = js_to_integer_clamped(start_d, -len, len, 0);
        int32_t end = js_to_integer_clamped(end_d, -len, len, len);
        if (start < 0) start = len + start;
        if (end < 0) end = len + end;
        if (start < 0) start = 0;
        if (end < 0) end = 0;
        if (start > len) start = len;
        if (end > len) end = len;
        if (end <= start) return *this_val;

        double v_d = 0;
        JSValue fill_value = (argc > 0) ? argv[0] : JS_UNDEFINED;
        if (!JS_IsUndefined(fill_value) && !JS_IsNull(fill_value) && JS_IsNumber(ctx, fill_value)) {
            JS_ToNumber(ctx, &v_d, fill_value);
        } else if (!JS_IsUndefined(fill_value) && !JS_IsNull(fill_value) && JS_IsBool(fill_value)) {
            v_d = JS_ToBool(ctx, fill_value) ? 1.0 : 0.0;
        } else if (JS_IsUndefined(fill_value) || JS_IsNull(fill_value)) {
            v_d = 0;
        } else {
            /* best effort numeric conversion */
            JS_ToNumber(ctx, &v_d, fill_value);
        }

        uint32_t count = (uint32_t)(end - start);

        switch (classId) {
            case JS_CLASS_UINT8C_ARRAY:
            case JS_CLASS_UINT8_ARRAY:
            case JS_CLASS_INT8_ARRAY: {
                int iv = 0;
                if (v_d != v_d) iv = 0; /* NaN */
                else iv = (int)v_d;
                uint8_t b = (uint8_t)(iv & 0xff);
                memset((uint8_t *)raw + (size_t)start, b, (size_t)count);
                break;
            }
            case JS_CLASS_INT16_ARRAY:
            case JS_CLASS_UINT16_ARRAY: {
                int iv = 0;
                if (v_d != v_d) iv = 0;
                else iv = (int)v_d;
                uint16_t x = (uint16_t)(iv & 0xffff);
                uint16_t *p = (uint16_t *)raw + start;
                for (uint32_t i = 0; i < count; ++i) p[i] = x;
                break;
            }
            case JS_CLASS_INT32_ARRAY:
            case JS_CLASS_UINT32_ARRAY: {
                int64_t iv = 0;
                if (v_d != v_d) iv = 0;
                else iv = (int64_t)v_d;
                uint32_t x = (uint32_t)iv;
                uint32_t *p = (uint32_t *)raw + start;
                for (uint32_t i = 0; i < count; ++i) p[i] = x;
                break;
            }
            case JS_CLASS_FLOAT32_ARRAY: {
                float x = (float)v_d;
                float *p = (float *)raw + start;
                for (uint32_t i = 0; i < count; ++i) p[i] = x;
                break;
            }
            case JS_CLASS_FLOAT64_ARRAY: {
                double x = v_d;
                double *p = (double *)raw + start;
                for (uint32_t i = 0; i < count; ++i) p[i] = x;
                break;
            }
            default: {
                /* Fallback: still faster than property ops because this only hits TypedArrays. */
                for (int32_t i = start; i < end; ++i) {
                    JS_SetPropertyUint32(ctx, *this_val, (uint32_t)i, fill_value);
                }
                break;
            }
        }

        return *this_val;
    }

    double len_d = 0;
    JSValue len_val = JS_GetPropertyStr(ctx, *this_val, "length");
    if (!JS_IsUndefined(len_val)) { JS_ToNumber(ctx, &len_d, len_val); }

    if (len_d < 0) len_d = 0;
    if (len_d > 0x7fffffff) len_d = 0x7fffffff;
    int32_t len = (int32_t)len_d;

    double start_d = 0;
    if (argc > 1 && !JS_IsUndefined(argv[1])) { JS_ToNumber(ctx, &start_d, argv[1]); }

    double end_d = (double)len;
    if (argc > 2 && !JS_IsUndefined(argv[2])) { JS_ToNumber(ctx, &end_d, argv[2]); }

    int32_t start = js_to_integer_clamped(start_d, -len, len, 0);
    int32_t end = js_to_integer_clamped(end_d, -len, len, len);

    if (start < 0) start = len + start;
    if (end < 0) end = len + end;
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if (start > len) start = len;
    if (end > len) end = len;

    if (end <= start) { return *this_val; }

    JSValue fill_value = (argc > 0) ? argv[0] : JS_UNDEFINED;
    for (int32_t i = start; i < end; ++i) { JS_SetPropertyUint32(ctx, *this_val, (uint32_t)i, fill_value); }

    return *this_val;
}

FileParamsJS js_get_path_from_params(JSContext *ctx, JSValue *argv, bool checkIfexist, bool legacy) {
    FileParamsJS filePath;
    filePath.fs = &LittleFS;
    filePath.path = "";
    filePath.exist = false;
    filePath.paramOffset = 1;

    String fsParam = "";

    /* legacy: first arg is fs string */
    if (legacy && !JS_IsUndefined(argv[0])) {
        JSCStringBuf buf;
        const char *s = JS_ToCString(ctx, argv[0], &buf);
        if (s) { fsParam = s; }
        fsParam.toLowerCase();
    }

    /* if function({ fs, path }) */
    if (JS_IsObject(ctx, argv[0])) {
        JSValue fsVal = JS_GetPropertyStr(ctx, argv[0], "fs");
        JSValue pathVal = JS_GetPropertyStr(ctx, argv[0], "path");

        if (!JS_IsUndefined(fsVal)) {
            JSCStringBuf buf;
            const char *s = JS_ToCString(ctx, fsVal, &buf);
            if (s) { fsParam = s; }
        }

        if (!JS_IsUndefined(pathVal)) {
            JSCStringBuf buf;
            const char *s = JS_ToCString(ctx, pathVal, &buf);
            if (s) { filePath.path = s; }
        }

        filePath.paramOffset = 0;
    }

    /* filesystem selection */
    if (fsParam == "sd") {
        filePath.fs = &SD;
    } else if (fsParam == "littlefs") {
        filePath.fs = &LittleFS;
    } else {
        /* function(path: string) */
        filePath.paramOffset = 0;

        if (!JS_IsUndefined(argv[0])) {
            JSCStringBuf buf;
            const char *s = JS_ToCString(ctx, argv[0], &buf);
            if (s) { filePath.path = s; }
        }

        if (sdcardMounted && SD.exists(filePath.path)) {
            filePath.fs = &SD;
        } else {
            filePath.fs = &LittleFS;
        }
    }

    /* function(fs: string, path: string) */
    if (filePath.paramOffset == 1 && !JS_IsUndefined(argv[1])) {
        JSCStringBuf buf;
        const char *s = JS_ToCString(ctx, argv[1], &buf);
        if (s) { filePath.path = s; }
    }

    /* existence check */
    if (checkIfexist) { filePath.exist = filePath.fs->exists(filePath.path); }

    return filePath;
}

JSValue js_value_from_json_variant(JSContext *ctx, JsonVariantConst value) {
    if (value.isNull()) return JS_NULL;
    if (value.is<bool>()) return JS_NewBool(value.as<bool>());
    if (value.is<const char *>()) {
        const char *s = value.as<const char *>();
        return s ? JS_NewString(ctx, s) : JS_NULL;
    }
    if (value.is<JsonArrayConst>()) {
        JsonArrayConst arr = value.as<JsonArrayConst>();
        JSValue jsArr = JS_NewArray(ctx, arr.size());
        uint32_t idx = 0;
        for (JsonVariantConst item : arr) {
            JS_SetPropertyUint32(ctx, jsArr, idx++, js_value_from_json_variant(ctx, item));
        }
        return jsArr;
    }
    if (value.is<JsonObjectConst>()) {
        JsonObjectConst obj = value.as<JsonObjectConst>();
        JSValue jsObj = JS_NewObject(ctx);
        for (JsonPairConst kv : obj) {
            const char *key = kv.key().c_str();
            JS_SetPropertyStr(ctx, jsObj, key ? key : "", js_value_from_json_variant(ctx, kv.value()));
        }
        return jsObj;
    }
    if (value.is<int64_t>()) return JS_NewInt64(ctx, value.as<int64_t>());
    if (value.is<uint64_t>()) {
        uint64_t u = value.as<uint64_t>();
        return (u <= UINT32_MAX) ? JS_NewUint32(ctx, (uint32_t)u) : JS_NewFloat64(ctx, (double)u);
    }
    if (value.is<double>()) return JS_NewFloat64(ctx, value.as<double>());
    return JS_UNDEFINED;
}

void internal_print(
    JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, uint8_t printTft, uint8_t newLine
) {
    int maxArgs = argc;
    if (maxArgs > 20) maxArgs = 20;
    for (int i = 0; i < maxArgs; ++i) {
        JSValue jsvValue = argv[i];
        if (JS_IsUndefined(jsvValue)) break;
        if (i > 0) {
            if (printTft) tft.print(" ");
            Serial.print(" ");
        }

        if (JS_IsUndefined(jsvValue)) {
            if (printTft) tft.print("undefined");
            Serial.print("undefined");

        } else if (JS_IsNull(jsvValue)) {
            if (printTft) tft.print("null");
            Serial.print("null");

        } else if (JS_IsNumber(ctx, jsvValue)) {
            double numberValue = 0.0;
            JS_ToNumber(ctx, &numberValue, jsvValue);
            if (printTft) tft.printf("%g", numberValue);
            Serial.printf("%g", numberValue);

        } else if (JS_IsBool(jsvValue)) {
            bool b = JS_ToBool(ctx, jsvValue);
            const char *boolValue = b ? "true" : "false";
            if (printTft) tft.print(boolValue);
            Serial.print(boolValue);

        } else {
            JSCStringBuf sb;
            const char *s = JS_ToCString(ctx, jsvValue, &sb);
            if (s) {
                if (printTft) tft.print(s);
                Serial.print(s);
            } else {
                /* fallback */
                JS_PrintValueF(ctx, jsvValue, JS_DUMP_LONG);
            }
        }
    }

    if (newLine) {
        if (printTft) tft.println();
        Serial.println();
    }
}

JSValue buffer_latin1_to_string(JSContext *ctx, const uint8_t *buf, size_t len) {
    /* JS strings are UTF-8. For Node's 'binary'/'latin1', map each byte 0..255
       to Unicode codepoint U+0000..U+00FF and UTF-8 encode it. */
    if (!buf || len == 0) { return JS_NewStringLen(ctx, "", 0); }

    size_t out_cap = len * 2;
    uint8_t *out = (uint8_t *)malloc(out_cap);
    if (!out) { return JS_ThrowOutOfMemory(ctx); }

    size_t out_len = 0;
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = buf[i];
        if (b < 0x80) {
            out[out_len++] = b;
        } else if (b < 0xC0) {
            out[out_len++] = 0xC2;
            out[out_len++] = b;
        } else {
            out[out_len++] = 0xC3;
            out[out_len++] = (uint8_t)(b - 0x40);
        }
    }

    JSValue s = JS_NewStringLen(ctx, (const char *)out, out_len);
    free(out);
    return s;
}

bool buffer_latin1_string_to_bytes(const uint8_t *s, size_t len, uint8_t **out_bytes, size_t *out_len) {
    if (!out_bytes || !out_len) return false;
    *out_bytes = NULL;
    *out_len = 0;

    if (!s || len == 0) return true;

    // Worst-case output is <= input length.
    uint8_t *out = (uint8_t *)malloc(len);
    if (!out) return false;

    size_t oi = 0;
    for (size_t i = 0; i < len;) {
        uint8_t b = s[i];
        if (b < 0x80) {
            out[oi++] = b;
            i += 1;
            continue;
        }

        if (b == 0xC2) {
            if (i + 1 >= len) {
                free(out);
                return false;
            }
            uint8_t b2 = s[i + 1];
            if (b2 < 0x80 || b2 > 0xBF) {
                free(out);
                return false;
            }
            out[oi++] = b2;
            i += 2;
            continue;
        }

        if (b == 0xC3) {
            if (i + 1 >= len) {
                free(out);
                return false;
            }
            uint8_t b2 = s[i + 1];
            if (b2 < 0x80 || b2 > 0xBF) {
                free(out);
                return false;
            }
            out[oi++] = (uint8_t)(b2 + 0x40);
            i += 2;
            continue;
        }

        // Not latin1 (or invalid UTF-8 for the subset we support).
        free(out);
        return false;
    }

    *out_bytes = out;
    *out_len = oi;
    return true;
}

#endif
