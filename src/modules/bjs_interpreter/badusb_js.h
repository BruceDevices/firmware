#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __BADUSB_JS_H__
#define __BADUSB_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_badusbRunFile(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_badusbSetup(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_badusbPrint(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_badusbPrintln(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_badusbPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_badusbHold(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_badusbRelease(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_badusbReleaseAll(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_badusbPressRaw(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
