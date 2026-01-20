#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "keyboard_js.h"

#include "core/mykeyboard.h"

#include "helpers_js.h"

JSValue native_keyboard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    String result = "";
    if (argc == 0) {
        result = keyboard("");
    } else if (JS_IsNumber(ctx, argv[0])) {
        int m = 0;
        if (JS_ToInt32(ctx, &m, argv[0])) return JS_EXCEPTION;
        const char *init = "";
        if (argc > 1 && JS_IsString(ctx, argv[1])) {
            JSCStringBuf buf;
            init = JS_ToCString(ctx, argv[1], &buf);
        }
        result = keyboard("", m, init);
    } else if (JS_IsString(ctx, argv[0])) {
        JSCStringBuf buf0;
        const char *title = JS_ToCString(ctx, argv[0], &buf0);
        if (argc == 1 || !JS_IsNumber(ctx, argv[1])) {
            result = keyboard(title ? title : "");
        } else {
            int m = 0;
            if (JS_ToInt32(ctx, &m, argv[1])) return JS_EXCEPTION;
            if (argc < 3 || !JS_IsString(ctx, argv[2])) {
                result = keyboard(title ? title : "", m);
            } else {
                JSCStringBuf buf2;
                const char *init = JS_ToCString(ctx, argv[2], &buf2);
                result = keyboard(title ? title : "", m, init ? init : "");
            }
        }
    } else {
        result = keyboard("");
    }

    return JS_NewString(ctx, result.c_str());
}

JSValue native_getPrevPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewBool(check(PrevPress));
}

JSValue native_getSelPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewBool(check(SelPress));
}

JSValue native_getEscPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewBool(check(EscPress));
}

JSValue native_getNextPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewBool(check(NextPress));
}

JSValue native_getAnyPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewBool(check(AnyKeyPress));
}

JSValue native_setLongPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // TODO: if backgroud app implemented, store in ctx and set if on foreground/background
    LongPress = (argc > 0) ? JS_ToBool(ctx, argv[0]) : false;
    return JS_UNDEFINED;
}

JSValue native_getKeysPressed(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSValue arr = JS_NewArray(ctx, 0);
#ifdef HAS_KEYBOARD
    keyStroke key = _getKeyPress();
    if (!key.pressed) return arr;
    uint32_t arrayIndex = 0;
    for (auto i : key.word) {
        char str[2] = {(char)i, '\0'};
        JSValue s = JS_NewString(ctx, str);
        JS_SetPropertyUint32(ctx, arr, arrayIndex++, s);
    }
    if (key.del) {
        JSValue s = JS_NewString(ctx, "Delete");
        JS_SetPropertyUint32(ctx, arr, arrayIndex++, s);
    }
    if (key.enter) {
        JSValue s = JS_NewString(ctx, "Enter");
        JS_SetPropertyUint32(ctx, arr, arrayIndex++, s);
    }
    if (key.fn) {
        JSValue s = JS_NewString(ctx, "Function");
        JS_SetPropertyUint32(ctx, arr, arrayIndex++, s);
    }
    for (auto i : key.modifier_keys) {
        if (i == 0x82) {
            JSValue s = JS_NewString(ctx, "Alt");
            JS_SetPropertyUint32(ctx, arr, arrayIndex++, s);
        } else if (i == 0x2B) {
            JSValue s = JS_NewString(ctx, "Tab");
            JS_SetPropertyUint32(ctx, arr, arrayIndex++, s);
        } else if (i == 0x00) {
            JSValue s = JS_NewString(ctx, "Option");
            JS_SetPropertyUint32(ctx, arr, arrayIndex++, s);
        }
    }
#endif
    return arr;
}

#endif
