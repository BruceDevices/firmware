#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "runtime_js.h"

#include "globals_js.h"

JSValue native_runtimeToBackground(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    interpreter_state = 0;
    return JS_UNDEFINED;
}

JSValue native_runtimeToForeground(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    interpreter_state = 1;
    return JS_UNDEFINED;
}

JSValue native_runtimeIsForeground(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewBool(interpreter_state > 0);
}

JSValue native_runtimeMain(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) return JS_ThrowTypeError(ctx, "not a function");

    int id = js_add_main_timer(ctx, argv[0]);
    if (id < 0) return JS_ThrowInternalError(ctx, "too many timers");
    return JS_NewInt32(ctx, id);
}

#endif
