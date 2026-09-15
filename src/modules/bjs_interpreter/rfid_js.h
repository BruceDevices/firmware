// Author: Senape3000
// More info: https://github.com/Senape3000/firmware/blob/main/docs_custom/JS_RFID/RFID_API_README.md
// More info: https://github.com/Senape3000/firmware/blob/main/docs_custom/JS_RFID/RFID_SRIX_API_README.md

#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

#ifndef __RFID_JS_H__
#define __RFID_JS_H__

#include "helpers_js.h"

#ifdef __cplusplus
extern "C" {
#endif

// Standard RFID functions
JSValue native_rfidRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_rfidReadUID(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_rfidWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_rfidSave(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_rfidLoad(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_rfidClear(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

// MIFARE key management
JSValue native_rfid_AddMifareKey(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

// SRIX functions
JSValue native_srixRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_srixWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_srixSave(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_srixLoad(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_srixClear(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_srixWriteBlock(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

#ifdef __cplusplus
}
#endif

#endif

#endif
