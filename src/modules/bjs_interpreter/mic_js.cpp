#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "mic_js.h"

#include "helpers_js.h"
#include "modules/others/mic.h"

duk_ret_t putPropMicFunctions(duk_context *ctx, duk_idx_t obj_idx, uint8_t magic) {
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "recordWav", native_micRecordWav, 2, magic);
    return 0;
}

duk_ret_t registerMic(duk_context *ctx) {
    bduk_register_c_lightfunc(ctx, "micRecordWav", native_micRecordWav, 2);
    return 0;
}

// mic.recordWav(pathOrPathObj?, options?)
// - pathOrPathObj: string ("/BruceMIC/rec.wav") OR { fs: "SD"|"LittleFS", path: "/..." } (same as storage)
// - options:
//    - maxMs: number (0 = unlimited)
//    - stopOnSel: boolean (default true)
// Returns: { ok: boolean, path: string, bytes: number, sampleRateHz: number, channels: number }
duk_ret_t native_micRecordWav(duk_context *ctx) {
    FileParamsJS fileParams;
    if (duk_is_undefined(ctx, 0) || duk_is_null(ctx, 0)) {
        // default path
        fileParams.fs = nullptr;
        fileParams.path = "/BruceMIC/recording.wav";
        fileParams.exist = false;
        fileParams.paramOffset = 0;
    } else {
        fileParams = js_get_path_from_params(ctx, false);
    }

    uint32_t maxMs = 8000;
    bool stopOnSel = true;

    if (duk_is_object(ctx, 1)) {
        if (duk_get_prop_string(ctx, 1, "maxMs")) {
            if (duk_is_number(ctx, -1)) maxMs = (uint32_t)duk_to_uint(ctx, -1);
        }
        duk_pop(ctx);

        if (duk_get_prop_string(ctx, 1, "stopOnSel")) {
            stopOnSel = duk_get_boolean_default(ctx, -1, true);
        }
        duk_pop(ctx);
    }

    // Choose FS if not specified
    FS *fs = fileParams.fs;
    if (fs == nullptr) {
        if (!getFsStorage(fs) || fs == nullptr) {
            duk_idx_t obj = duk_push_object(ctx);
            bduk_put_prop(ctx, obj, "ok", duk_push_boolean, false);
            bduk_put_prop(ctx, obj, "path", duk_push_string, "");
            bduk_put_prop(ctx, obj, "bytes", duk_push_uint, 0);
            bduk_put_prop(ctx, obj, "sampleRateHz", duk_push_uint, 48000);
            bduk_put_prop(ctx, obj, "channels", duk_push_uint, 1);
            return 1;
        }
    }

    String path = fileParams.path;
    if (!path.startsWith("/")) path = "/" + path;

    uint32_t outBytes = 0;
    bool ok = mic_record_wav_to_path(fs, path, maxMs, stopOnSel, &outBytes);

    duk_idx_t obj = duk_push_object(ctx);
    bduk_put_prop(ctx, obj, "ok", duk_push_boolean, ok);
    bduk_put_prop(ctx, obj, "path", duk_push_string, path.c_str());
    bduk_put_prop(ctx, obj, "bytes", duk_push_uint, outBytes);
    bduk_put_prop(ctx, obj, "sampleRateHz", duk_push_uint, 48000);
    bduk_put_prop(ctx, obj, "channels", duk_push_uint, 1);
    return 1;
}

#endif


