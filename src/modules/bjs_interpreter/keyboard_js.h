#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __KEYBOARD_JS_H__
#define __KEYBOARD_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_keyboard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_hex_keyboard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_num_keyboard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getPrevPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getSelPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getEscPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getNextPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getAnyPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getKeysPressed(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_setLongPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
