// Mic bindings for the JS interpreter

#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __MIC_JS_H__
#define __MIC_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_micRecordWav(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
