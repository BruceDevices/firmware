#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __BLE_JS_H__
#define __BLE_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_bleScan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_bleAdvertise(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_bleStopAdvertise(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
