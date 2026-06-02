#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __I2C_JS_H__
#define __I2C_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_i2c_begin(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_i2c_scan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_i2c_write(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_i2c_read(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_i2c_write_read(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
