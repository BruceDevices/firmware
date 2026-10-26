#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __UART_JS_H__
#define __UART_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_uart_begin(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_uart_write(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_uart_read(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_uart_available(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_uart_flush(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_uart_end(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
