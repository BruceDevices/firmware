#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __STORAGE_JS_H__
#define __STORAGE_JS_H__

#include "helpers_js.h"
#include <SD.h>

extern "C" {
JSValue putPropStorageFunctions(JSContext *ctx, JSValue obj_idx, int magic);
JSValue registerStorage(JSContext *ctx);

JSValue native_storageReaddir(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_storageRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_storageWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_storageRename(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_storageRemove(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_storageMkdir(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_storageRmdir(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_storageSpaceLittleFS(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_storageSpaceSDCard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
