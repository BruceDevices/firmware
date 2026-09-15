#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "audio_js.h"

JSValue native_playAudioFile(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1) { return JS_NewBool(false); }

    JSCStringBuf buf;
    const char *filename = JS_ToCString(ctx, argv[0], &buf);
    if (!filename) { return JS_EXCEPTION; }

    bool r = serialCli.parse("music_player " + String(filename));

    return JS_NewBool(r);
}

JSValue native_tone(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (!bruceConfig.soundEnabled) { return JS_UNDEFINED; }

    uint32_t freq = 500;
    uint32_t duration = 1000;
    int nonBlocking = false;

    if (argc > 0) { JS_ToUint32(ctx, &freq, argv[0]); }
    if (argc > 1) { JS_ToUint32(ctx, &duration, argv[1]); }
    if (argc > 2) { nonBlocking = JS_ToInt32(ctx, &nonBlocking, argv[2]); }

#if defined(BUZZ_PIN)
    tone(BUZZ_PIN, freq, duration);

#elif defined(HAS_NS4168_SPKR)
    if (!nonBlocking) { serialCli.parse("tone " + String(freq) + " " + String(duration)); }
#endif

    return JS_UNDEFINED;
}

#endif
