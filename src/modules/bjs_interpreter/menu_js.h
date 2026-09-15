#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __MENU_JS_H__
#define __MENU_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue native_menuShow(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_menuShowMainBorder(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_menuShowMainBorderWithTitle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_menuPrintTitle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_menuPrintSubtitle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_menuDisplayMessage(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
