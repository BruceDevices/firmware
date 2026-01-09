#pragma once

#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include <duktape.h>

duk_ret_t putPropMicFunctions(duk_context *ctx, duk_idx_t obj_idx, uint8_t magic);
duk_ret_t registerMic(duk_context *ctx);

duk_ret_t native_micRecordWav(duk_context *ctx);

#endif


