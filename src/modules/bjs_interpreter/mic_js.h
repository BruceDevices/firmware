// Mic bindings for the JS interpreter

// More info: https://github.com/Senape3000/firmware/blob/main/docs_custom/MIC/MIC_API_README.md

#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

#ifndef __MIC_JS_H__
#define __MIC_JS_H__

#include "helpers_js.h"

extern "C" {

// WAV recording
JSValue native_micRecordWav(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

// Raw sample capture
JSValue native_micCaptureSamples(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}
#endif

#endif
