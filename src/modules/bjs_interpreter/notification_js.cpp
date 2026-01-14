#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "notification_js.h"

#include "helpers_js.h"

JSValue native_notifyBlink(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    uint32_t delayMs = 500;

    if (argc > 0) {
        if (JS_IsNumber(ctx, argv[0])) {
            int d = 0;
            if (JS_ToInt32(ctx, &d, argv[0])) return JS_EXCEPTION;
            delayMs = (uint32_t)d;
        } else if (JS_IsString(ctx, argv[0])) {
            JSCStringBuf buf;
            const char *s = JS_ToCString(ctx, argv[0], &buf);
            if (s) {
                if (strcmp(s, "long") == 0) delayMs = 1000;
            }
        }
    }

    digitalWrite(19, HIGH);
    delay(delayMs);
    digitalWrite(19, LOW);

    return JS_UNDEFINED;
}

#endif
