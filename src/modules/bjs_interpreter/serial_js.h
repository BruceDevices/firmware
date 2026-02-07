#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __SERIAL_JS_H__
#define __SERIAL_JS_H__

#include "helpers_js.h"

#ifdef __cplusplus
extern "C" {
#endif

JSValue native_serialPrint(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_serialPrintln(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_serialReadln(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_serialCmd(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

#ifdef __cplusplus
}
#endif

#endif
#endif
