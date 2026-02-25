#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __SUBGHZ_JS_H__
#define __SUBGHZ_JS_H__

#include "helpers_js.h"

#ifdef __cplusplus
extern "C" {
#endif

JSValue native_subghzTransmitFile(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_subghzTransmit(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_subghzRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_subghzReadRaw(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_subghzSetFrequency(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

// Raw pulse TX API for bruteforce and custom protocol transmission
JSValue native_subghzTxSetup(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_subghzTxPulses(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_subghzTxEnd(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

#ifdef __cplusplus
}
#endif

#endif
#endif
