#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "mic_js.h"

#include "core/sd_functions.h"
#include "helpers_js.h"
#include "modules/others/mic.h"

// mic.recordWav(pathOrPathObj?, options?)
// - pathOrPathObj: string ("/BruceMIC/rec.wav") OR { fs: "SD"|"LittleFS", path: "/..." } (same as storage)
// - options:
//    - maxMs: number (0 = unlimited)
//    - stopOnSel: boolean (default true)
// Returns: { ok: boolean, path: string, bytes: number, sampleRateHz: number, channels: number }
JSValue native_micRecordWav(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;

    FileParamsJS fileParams;
    if (argc < 1 || JS_IsUndefined(argv[0]) || JS_IsNull(argv[0])) {
        fileParams.fs = nullptr;
        fileParams.path = "/BruceMIC/recording.wav";
        fileParams.exist = false;
        fileParams.paramOffset = 0;
    } else {
        fileParams = js_get_path_from_params(ctx, argv, false);
    }

    uint32_t maxMs = 8000;
    bool stopOnSel = true;

    if (argc > 1 && JS_IsObject(ctx, argv[1])) {
        JSValue maxMsVal = JS_GetPropertyStr(ctx, argv[1], "maxMs");
        if (JS_IsNumber(ctx, maxMsVal)) {
            int tmp = 0;
            JS_ToInt32(ctx, &tmp, maxMsVal);
            if (tmp >= 0) maxMs = (uint32_t)tmp;
        }

        JSValue stopVal = JS_GetPropertyStr(ctx, argv[1], "stopOnSel");
        if (JS_IsBool(stopVal)) { stopOnSel = JS_ToBool(ctx, stopVal); }
    }

    // Choose FS if not specified
    FS *fs = fileParams.fs;
    if (fs == nullptr) {
        if (!getFsStorage(fs) || fs == nullptr) {
            JSValue obj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, obj, "ok", JS_NewBool(0));
            JS_SetPropertyStr(ctx, obj, "path", JS_NewString(ctx, ""));
            JS_SetPropertyStr(ctx, obj, "bytes", JS_NewInt32(ctx, 0));
            JS_SetPropertyStr(ctx, obj, "sampleRateHz", JS_NewInt32(ctx, 48000));
            JS_SetPropertyStr(ctx, obj, "channels", JS_NewInt32(ctx, 1));
            return obj;
        }
    }

    String path = fileParams.path;
    if (!path.startsWith("/")) path = "/" + path;

    uint32_t outBytes = 0;
    bool ok = mic_record_wav_to_path(fs, path, maxMs, stopOnSel, &outBytes);

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "ok", JS_NewBool(ok ? 1 : 0));
    JS_SetPropertyStr(ctx, obj, "path", JS_NewString(ctx, path.c_str()));
    JS_SetPropertyStr(ctx, obj, "bytes", JS_NewInt32(ctx, (int32_t)outBytes));
    JS_SetPropertyStr(ctx, obj, "sampleRateHz", JS_NewInt32(ctx, 48000));
    JS_SetPropertyStr(ctx, obj, "channels", JS_NewInt32(ctx, 1));
    return obj;
}

#endif
