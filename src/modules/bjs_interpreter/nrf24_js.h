#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __NRF24_JS_H__
#define __NRF24_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_nrf24Begin(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_nrf24Send(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_nrf24Receive(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_nrf24SetChannel(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_nrf24IsConnected(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
