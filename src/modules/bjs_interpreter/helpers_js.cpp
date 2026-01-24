#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "helpers_js.h"
#include "core/sd_functions.h"
#include <globals.h>

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

    tft.printf("%s\n%s\n", (msg != NULL ? msg : "JS Error"), stackTrace);
    Serial.printf("%s\n%s\n", (msg != NULL ? msg : "JS Error"), stackTrace);
    Serial.flush();

    delay(500);
    while (!check(AnyKeyPress)) delay(50);
}

bool JS_IsTypedArray(JSContext *ctx, JSValue val) {
    int classId = JS_GetClassID(ctx, val);
    return (classId >= JS_CLASS_ARRAY_BUFFER && classId <= JS_CLASS_UINT32_ARRAY);
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

        if (sdcardMounted && checkIfexist && SD.exists(filePath.path)) {
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

#endif
