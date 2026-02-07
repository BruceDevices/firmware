// More info: https://github.com/Senape3000/firmware/blob/main/docs_custom/MIC/MIC_API_README.md
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "mic_js.h"

#include "core/sd_functions.h"
#include "helpers_js.h"
#include "modules/others/mic.h"

/**
 * JavaScript binding for microphone WAV recording
 *
 * @function mic.recordWav(pathOrPathObj?, options?)
 *
 * @param pathOrPathObj {string|object} - Optional path or path object
 *   - string: "/BruceMIC/rec.wav"
 *   - object: { fs: "SD"|"LittleFS", path: "/..." }
 *   - If omitted, defaults to auto-detected storage
 *
 * @param options {object} - Optional recording configuration
 *   - maxMs: {number} - Recording duration in milliseconds (0 = unlimited, default: 8000)
 *   - stopOnSel: {boolean} - Stop recording when SEL button is pressed (default: true)
 *   - gain: {number} - Audio gain multiplier, range 0.5-4.0 (default: 1.0)
 *
 * @returns {object} - Recording result
 *   - ok: {boolean} - Success status
 *   - path: {string} - Actual file path used
 *   - bytes: {number} - File size in bytes
 *   - sampleRateHz: {number} - Sample rate (always 48000)
 *   - channels: {number} - Channel count (always 1 for mono)
 *
 * @example
 *   // Simple 8-second recording with default settings
 *   let result = mic.recordWav("/test.wav");
 *
 *   // Custom duration and gain
 *   let result = mic.recordWav("/test.wav", { maxMs: 15000, gain: 2.5 });
 *
 *   // Unlimited recording, no stop on button
 *   let result = mic.recordWav("/test.wav", { maxMs: 0, stopOnSel: false });
 */
JSValue native_micRecordWav(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;

    // ===== PARSE PATH PARAMETER =====
    FileParamsJS fileParams;
    if (argc < 1 || JS_IsUndefined(argv[0]) || JS_IsNull(argv[0])) {
        // No path provided - use defaults
        fileParams.fs = nullptr;
        fileParams.path = "/BruceMIC/recording.wav";
        fileParams.exist = false;
        fileParams.paramOffset = 0;
    } else {
        // Path provided as string or object
        fileParams = js_get_path_from_params(ctx, argv, false);
    }

    // ===== PARSE OPTIONS PARAMETER =====
    uint32_t maxMs = 8000; // Default: 8 seconds
    bool stopOnSel = true; // Default: stop on button press
    float gain = 1.0f;     // Default: no gain adjustment

    if (argc > 1 && JS_IsObject(ctx, argv[1])) {
        // Parse maxMs (recording duration)
        JSValue maxMsVal = JS_GetPropertyStr(ctx, argv[1], "maxMs");
        if (JS_IsNumber(ctx, maxMsVal)) {
            int tmp = 0;
            JS_ToInt32(ctx, &tmp, maxMsVal);
            if (tmp >= 0) { maxMs = (uint32_t)tmp; }
        }

        // Parse stopOnSel (button stop control)
        JSValue stopVal = JS_GetPropertyStr(ctx, argv[1], "stopOnSel");
        if (JS_IsBool(stopVal)) { stopOnSel = JS_ToBool(ctx, stopVal); }

        // Parse gain (audio amplification)
        // Note: gain is passed as integer (e.g., 25 for 2.5x) to avoid float parsing issues
        JSValue gainVal = JS_GetPropertyStr(ctx, argv[1], "gain");
        if (JS_IsNumber(ctx, gainVal)) {
            int tmpGain = 0;
            JS_ToInt32(ctx, &tmpGain, gainVal);
            // Convert from int to float: 10 = 1.0x, 25 = 2.5x
            float parsedGain = (float)tmpGain / 10.0f;
            // Validate gain range (0.5x to 4.0x)
            if (parsedGain >= 0.5f && parsedGain <= 4.0f) { gain = parsedGain; }
        }
    }

    // ===== RESOLVE FILESYSTEM =====
    FS *fs = fileParams.fs;
    if (fs == nullptr) {
        // No filesystem specified - auto-detect (SD or LittleFS)
        if (!getFsStorage(fs) || fs == nullptr) {
            // No storage available - return failure object
            JSValue obj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, obj, "ok", JS_NewBool(0));
            JS_SetPropertyStr(ctx, obj, "path", JS_NewString(ctx, ""));
            JS_SetPropertyStr(ctx, obj, "bytes", JS_NewInt32(ctx, 0));
            JS_SetPropertyStr(ctx, obj, "sampleRateHz", JS_NewInt32(ctx, 48000));
            JS_SetPropertyStr(ctx, obj, "channels", JS_NewInt32(ctx, 1));
            return obj;
        }
    }

    // ===== NORMALIZE PATH =====
    String path = fileParams.path;
    if (!path.startsWith("/")) { path = "/" + path; }

    // ===== SETUP RECORDING CALLBACK =====
    uint32_t outBytes = 0;
    auto jsCallback = [&]() -> bool {
        // Check if user wants to stop via button press
        if (stopOnSel) {
            InputHandler(); // Update button states
            if (check(SelPress)) {
                return false; // Stop recording
            }
        }
        return true; // Continue recording
    };

    // ===== EXECUTE RECORDING =====
    bool ok = mic_record_wav_to_path(
        fs,        // Filesystem pointer
        path,      // Output file path
        maxMs,     // Recording duration (0 = unlimited)
        &outBytes, // [OUT] Bytes written
        gain,      // Audio gain multiplier
        jsCallback // Progress callback with stop control
    );

    // ===== BUILD RESULT OBJECT =====
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "ok", JS_NewBool(ok ? 1 : 0));
    JS_SetPropertyStr(ctx, obj, "path", JS_NewString(ctx, path.c_str()));
    JS_SetPropertyStr(ctx, obj, "bytes", JS_NewInt32(ctx, (int32_t)outBytes));
    JS_SetPropertyStr(ctx, obj, "sampleRateHz", JS_NewInt32(ctx, 48000));
    JS_SetPropertyStr(ctx, obj, "channels", JS_NewInt32(ctx, 1));

    return obj;
}

#endif
