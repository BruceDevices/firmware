#ifndef __MATH_JS_H__
#define __MATH_JS_H__
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

#include "helpers_js.h"

extern "C" {
JSValue native_math_acosh(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_math_asinh(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_math_atanh(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_math_is_equal(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
