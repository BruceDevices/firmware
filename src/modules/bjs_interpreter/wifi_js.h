#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __WIFI_JS_H__
#define __WIFI_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_wifiConnected(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_wifiConnectDialog(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_wifiConnect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_wifiScan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_wifiDisconnect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_httpFetch(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_wifiMACAddress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_ipAddress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
