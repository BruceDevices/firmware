// Author: Senape3000
// More info: https://github.com/Senape3000/firmware/blob/main/docs_custom/JS_RFID/RFID_API_README.md
// More info: https://github.com/Senape3000/firmware/blob/main/docs_custom/JS_RFID/RFID_SRIX_API_README.md

#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

#ifndef __RFID_JS_H__
#define __RFID_JS_H__

#include <duktape.h>

duk_ret_t putPropRFIDFunctions(duk_context *ctx, duk_idx_t obj_idx, uint8_t magic);
duk_ret_t registerRFID(duk_context *ctx);

duk_ret_t native_rfidRead(duk_context *ctx);
duk_ret_t native_rfidReadUID(duk_context *ctx);
duk_ret_t native_rfidWrite(duk_context *ctx);
duk_ret_t native_rfidSave(duk_context *ctx);
duk_ret_t native_rfidLoad(duk_context *ctx);
duk_ret_t native_rfidClear(duk_context *ctx);
// MIFARE key management
duk_ret_t native_rfid_AddMifareKey(duk_context *ctx);
// SRIX functions
duk_ret_t native_srixRead(duk_context *ctx);
duk_ret_t native_srixWrite(duk_context *ctx);
duk_ret_t native_srixSave(duk_context *ctx);
duk_ret_t native_srixLoad(duk_context *ctx);
duk_ret_t native_srixClear(duk_context *ctx);
duk_ret_t native_srixWriteBlock(duk_context *ctx);

#endif

#endif
