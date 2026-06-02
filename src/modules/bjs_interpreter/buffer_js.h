#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __BUFFER_JS_H__
#define __BUFFER_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_buffer_from(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_buffer_toString(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
