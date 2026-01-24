#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __GPIO_JS_H__
#define __GPIO_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_digitalWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_analogWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_analogWriteResolution(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_analogWriteFrequency(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_digitalRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_analogRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_touchRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dacWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

JSValue native_ledcAttach(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_ledcWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_ledcWriteTone(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_ledcFade(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_ledcChangeFrequency(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_ledcDetach(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

JSValue native_pinMode(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_pins(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
