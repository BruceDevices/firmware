#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "keyboard_js.h"

#include "core/mykeyboard.h"

#include "helpers_js.h"

// Enum for keyboard types
enum KeyboardType { KEYBOARD_NORMAL, KEYBOARD_HEX, KEYBOARD_NUM };

// Helper function that handles the common argument parsing logic for keyboard functions
static JSValue keyboard_wrapper(JSContext *ctx, JSValue *argv, int argc, KeyboardType type) {
    String result = "";

    if (argc == 0) {
        switch (type) {
            case KEYBOARD_NORMAL: result = keyboard(""); break;
            case KEYBOARD_HEX: result = hex_keyboard(""); break;
            case KEYBOARD_NUM: result = num_keyboard(""); break;
        }
    } else if (JS_IsNumber(ctx, argv[0])) {
        int max_size = 0;
        if (JS_ToInt32(ctx, &max_size, argv[0])) return JS_EXCEPTION;
        const char *msg = "";
        if (argc > 1 && JS_IsString(ctx, argv[1])) {
            JSCStringBuf buf;
            msg = JS_ToCString(ctx, argv[1], &buf);
        }
        bool mask_input = JS_IsBool(argv[argc - 1]) ? JS_ToBool(ctx, argv[argc - 1]) : false;
        switch (type) {
            case KEYBOARD_NORMAL: result = keyboard("", max_size, msg, mask_input); break;
            case KEYBOARD_HEX: result = hex_keyboard("", max_size, msg, mask_input); break;
            case KEYBOARD_NUM: result = num_keyboard("", max_size, msg, mask_input); break;
        }
    } else if (JS_IsString(ctx, argv[0])) {
        JSCStringBuf buf0;
        const char *title = JS_ToCString(ctx, argv[0], &buf0);
        if (argc == 1 || !JS_IsNumber(ctx, argv[1])) {
            switch (type) {
                case KEYBOARD_NORMAL: result = keyboard(title ? title : ""); break;
                case KEYBOARD_HEX: result = hex_keyboard(title ? title : ""); break;
                case KEYBOARD_NUM: result = num_keyboard(title ? title : ""); break;
            }
        } else {
            int max_size = 0;
            if (JS_ToInt32(ctx, &max_size, argv[1])) return JS_EXCEPTION;
            if (argc < 3 || !JS_IsString(ctx, argv[2])) {
                switch (type) {
                    case KEYBOARD_NORMAL: result = keyboard(title ? title : "", max_size); break;
                    case KEYBOARD_HEX: result = hex_keyboard(title ? title : "", max_size); break;
                    case KEYBOARD_NUM: result = num_keyboard(title ? title : "", max_size); break;
                }
            } else {
                JSCStringBuf buf2;
                const char *msg = JS_ToCString(ctx, argv[2], &buf2);
                bool mask_input = JS_IsBool(argv[argc - 1]) ? JS_ToBool(ctx, argv[argc - 1]) : false;
                switch (type) {
                    case KEYBOARD_NORMAL:
                        result = keyboard(title ? title : "", max_size, msg ? msg : "", mask_input);
                        break;
                    case KEYBOARD_HEX:
                        result = hex_keyboard(title ? title : "", max_size, msg ? msg : "", mask_input);
                        break;
                    case KEYBOARD_NUM:
                        result = num_keyboard(title ? title : "", max_size, msg ? msg : "", mask_input);
                        break;
                }
            }
        }
    } else {
        switch (type) {
            case KEYBOARD_NORMAL: result = keyboard(""); break;
            case KEYBOARD_HEX: result = hex_keyboard(""); break;
            case KEYBOARD_NUM: result = num_keyboard(""); break;
        }
    }

    return JS_NewString(ctx, result.c_str());
}

JSValue native_keyboard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return keyboard_wrapper(ctx, argv, argc, KEYBOARD_NORMAL);
}

JSValue native_hex_keyboard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return keyboard_wrapper(ctx, argv, argc, KEYBOARD_HEX);
}

JSValue native_num_keyboard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return keyboard_wrapper(ctx, argv, argc, KEYBOARD_NUM);
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
