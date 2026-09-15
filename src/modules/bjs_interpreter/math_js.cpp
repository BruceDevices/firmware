#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "math_js.h"

JSValue native_math_acosh(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    double x = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToNumber(ctx, &x, argv[0]);
    return JS_NewFloat64(ctx, acosh(x));
}

JSValue native_math_asinh(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    double x = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToNumber(ctx, &x, argv[0]);
    return JS_NewFloat64(ctx, asinh(x));
}

JSValue native_math_atanh(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    double x = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToNumber(ctx, &x, argv[0]);
    return JS_NewFloat64(ctx, atanh(x));
}

JSValue native_math_is_equal(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    double a = 0, b = 0, epsilon = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToNumber(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToNumber(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToNumber(ctx, &epsilon, argv[2]);
    return JS_NewBool(fabs(a - b) < epsilon);
}

#endif
