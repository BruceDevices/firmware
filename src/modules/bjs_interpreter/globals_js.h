#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __GLOBALS_JS_H__
#define __GLOBALS_JS_H__

#include "helpers_js.h"

extern "C" {

JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_setInterval(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_clearInterval(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
void run_timers(JSContext *ctx);

int js_add_main_timer(JSContext *ctx, JSValue func);

void native_timers_state_finalizer(JSContext *ctx, void *opaque);

void js_timers_init(JSContext *ctx);
void js_timers_deinit(JSContext *ctx);

JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

JSValue native_require(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

JSValue native_assert(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_delay(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_random(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_parse_int(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_to_string(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_to_hex_string(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_to_lower_case(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_to_upper_case(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_atob(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_btoa(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_atob_bin(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_btoa_bin(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_exit(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
