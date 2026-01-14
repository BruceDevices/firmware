#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __DEVICE_JS_H__
#define __DEVICE_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_getDeviceName(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getBoard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getBruceVersion(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getBattery(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getBatteryDetailed(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getFreeHeapSize(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getEEPROMSize(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
