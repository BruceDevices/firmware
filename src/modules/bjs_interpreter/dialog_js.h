#ifndef __DIALOG_JS_H__
#define __DIALOG_JS_H__
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

#include "helpers_js.h"

extern "C" {
JSValue native_dialogMessage(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogInfo(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogSuccess(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogWarning(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogError(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogPickFile(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogChoice(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogViewFile(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogViewText(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

// TextViewer class
JSValue native_dialogCreateTextViewer(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogCreateTextViewerDraw(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogCreateTextViewerScrollUp(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogCreateTextViewerScrollDown(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogCreateTextViewerScrollToLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogCreateTextViewerGetLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogCreateTextViewerGetMaxLines(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue
native_dialogCreateTextViewerGetVisibleText(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogCreateTextViewerClear(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogCreateTextViewerFromString(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_dialogCreateTextViewerClose(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

// finalizer used by the runtime when TextViewer instances are freed
void native_textviewer_finalizer(JSContext *ctx, void *opaque);

JSValue native_drawStatusBar(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
