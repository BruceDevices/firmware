// Author: Senape3000
// More info: https://github.com/Senape3000/firmware/blob/main/docs_custom/JS_RFID/RFID_API_README.md
// More info: https://github.com/Senape3000/firmware/blob/main/docs_custom/JS_RFID/RFID_SRIX_API_README.md

#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

#include "rfid_js.h"
#include "globals.h"
#include "helpers_js.h"
#include "modules/rfid/srix_tool.h"
#include "modules/rfid/tag_o_matic.h"

// Singleton instances for RFID readers
static TagOMatic *g_tagReader = nullptr;

static TagOMatic *getTagReader() {
    if (!g_tagReader) {
        g_tagReader = new TagOMatic(true); // headless mode
    }
    return g_tagReader;
}

static void clearTagReader() {
    if (g_tagReader) {
        delete g_tagReader;
        g_tagReader = nullptr;
    }
}

// SRIX singleton
static SRIXTool *g_srixReader = nullptr;

static SRIXTool *getSRIXReader() {
    if (!g_srixReader) {
        g_srixReader = new SRIXTool(true); // headless mode
    }
    return g_srixReader;
}

static void clearSRIXReader() {
    if (g_srixReader) {
        delete g_srixReader;
        g_srixReader = nullptr;
    }
}

// ============================================================================
// RFID FUNCTIONS
// ============================================================================

JSValue native_rfidRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: rfidRead(timeout_in_seconds : number = 10);
    // returns: object with complete data or null on timeout

    int timeout = 10;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) { JS_ToInt32(ctx, &timeout, argv[0]); }

    TagOMatic *tagReader = getTagReader();

    // Use existing headless functionality
    String jsonResult = tagReader->read_tag_headless(timeout);

    if (jsonResult.isEmpty()) { return JS_NULL; }

    // Create a JS object from the fields
    JSValue obj = JS_NewObject(ctx);

    // Extract fields from the interface
    RFIDInterface *rfid = tagReader->getRFIDInterface();
    if (rfid) {
        JS_SetPropertyStr(ctx, obj, "uid", JS_NewString(ctx, rfid->printableUID.uid.c_str()));
        JS_SetPropertyStr(ctx, obj, "type", JS_NewString(ctx, rfid->printableUID.picc_type.c_str()));
        JS_SetPropertyStr(ctx, obj, "sak", JS_NewString(ctx, rfid->printableUID.sak.c_str()));
        JS_SetPropertyStr(ctx, obj, "atqa", JS_NewString(ctx, rfid->printableUID.atqa.c_str()));
        JS_SetPropertyStr(ctx, obj, "bcc", JS_NewString(ctx, rfid->printableUID.bcc.c_str()));
        JS_SetPropertyStr(ctx, obj, "pages", JS_NewString(ctx, rfid->strAllPages.c_str()));
        JS_SetPropertyStr(ctx, obj, "totalPages", JS_NewInt32(ctx, rfid->totalPages));
    }

    return obj;
}

JSValue native_rfidReadUID(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: rfidReadUID(timeout_in_seconds : number = 5);
    // returns: string (UID) or empty string on timeout

    int timeout = 5;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) { JS_ToInt32(ctx, &timeout, argv[0]); }

    TagOMatic tagReader(true);
    String uid = tagReader.read_uid_headless(timeout);

    return JS_NewString(ctx, uid.c_str());
}

JSValue native_rfidWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: rfidWrite(timeout_in_seconds : number = 10);
    // returns: { success: boolean, message: string }

    int timeout = 10;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) { JS_ToInt32(ctx, &timeout, argv[0]); }

    TagOMatic *tagReader = getTagReader();

    // Use headless write function
    int result = tagReader->write_tag_headless(timeout);

    // Create return object
    JSValue obj = JS_NewObject(ctx);

    switch (result) {
        case RFIDInterface::SUCCESS:
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(true));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Tag written successfully"));
            break;
        case RFIDInterface::TAG_NOT_PRESENT:
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "No tag present"));
            break;
        case RFIDInterface::TAG_NOT_MATCH:
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Tag types do not match"));
            break;
        default:
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Error writing data to tag"));
            break;
    }

    return obj;
}

JSValue native_rfidSave(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: rfidSave(filename : string);
    // returns: { success: boolean, message: string, filepath: string }

    if (argc < 1 || !JS_IsString(ctx, argv[0])) { return JS_NULL; }

    JSCStringBuf buf;
    const char *filename = JS_ToCString(ctx, argv[0], &buf);
    if (!filename) { return JS_NULL; }

    TagOMatic *tagReader = getTagReader();

    // Save file
    String result = tagReader->save_file_headless(String(filename));

    // Create return object
    JSValue obj = JS_NewObject(ctx);

    if (!result.isEmpty()) {
        JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(true));
        JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "File saved successfully"));
        JS_SetPropertyStr(ctx, obj, "filepath", JS_NewString(ctx, result.c_str()));
    } else {
        JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
        JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Error saving file"));
        JS_SetPropertyStr(ctx, obj, "filepath", JS_NewString(ctx, ""));
    }

    return obj;
}

JSValue native_rfidLoad(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: rfidLoad(filename : string);
    // returns: object with tag data or null on error

    if (argc < 1 || !JS_IsString(ctx, argv[0])) { return JS_NULL; }

    JSCStringBuf buf;
    const char *filename = JS_ToCString(ctx, argv[0], &buf);
    if (!filename) { return JS_NULL; }

    TagOMatic *tagReader = getTagReader();

    // Load file
    bool success = tagReader->load_file_headless(String(filename));

    if (!success) { return JS_NULL; }

    // Create a JS object from the loaded data
    JSValue obj = JS_NewObject(ctx);

    // Extract fields from the interface
    RFIDInterface *rfid = tagReader->getRFIDInterface();
    if (rfid) {
        JS_SetPropertyStr(ctx, obj, "uid", JS_NewString(ctx, rfid->printableUID.uid.c_str()));
        JS_SetPropertyStr(ctx, obj, "type", JS_NewString(ctx, rfid->printableUID.picc_type.c_str()));
        JS_SetPropertyStr(ctx, obj, "sak", JS_NewString(ctx, rfid->printableUID.sak.c_str()));
        JS_SetPropertyStr(ctx, obj, "atqa", JS_NewString(ctx, rfid->printableUID.atqa.c_str()));
        JS_SetPropertyStr(ctx, obj, "bcc", JS_NewString(ctx, rfid->printableUID.bcc.c_str()));
        JS_SetPropertyStr(ctx, obj, "pages", JS_NewString(ctx, rfid->strAllPages.c_str()));
        JS_SetPropertyStr(ctx, obj, "totalPages", JS_NewInt32(ctx, rfid->totalPages));
    }

    return obj;
}

JSValue native_rfidClear(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: rfidClear();
    // returns: undefined
    clearTagReader();
    return JS_UNDEFINED;
}

JSValue native_rfid_AddMifareKey(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: rfidAddMifareKey(key : string);
    // returns: { success: boolean, message: string, key: string }

    if (argc < 1 || !JS_IsString(ctx, argv[0])) {
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
        JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Invalid parameter: key must be a string"));
        return obj;
    }

    JSCStringBuf buf;
    const char *key_str = JS_ToCString(ctx, argv[0], &buf);
    if (!key_str) {
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
        JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Failed to read key string"));
        return obj;
    }

    // Use bruceConfig instead of tagReader
    String keyStr = String(key_str);
    bruceConfig.addMifareKey(keyStr);

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(true));
    JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Key processed"));
    JS_SetPropertyStr(ctx, obj, "key", JS_NewString(ctx, keyStr.c_str()));

    return obj;
}

// ============================================================================
// SRIX FUNCTIONS
// ============================================================================

JSValue native_srixRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: srixRead(timeout_in_seconds : number = 10);
    // returns: { uid: string, blocks: number, size: number, data: string } or null

    int timeout = 10;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) { JS_ToInt32(ctx, &timeout, argv[0]); }

    SRIXTool *srixReader = getSRIXReader();

    // Use existing headless functionality
    String jsonResult = srixReader->read_tag_headless(timeout);

    if (jsonResult.isEmpty()) { return JS_NULL; }

    // Create a JS object from the fields
    JSValue obj = JS_NewObject(ctx);

    // Get UID using getUID() method
    String uid_str = "";
    uint8_t *uid = srixReader->getUID();
    for (uint8_t i = 0; i < 8; i++) {
        if (uid[i] < 0x10) uid_str += "0";
        uid_str += String(uid[i], HEX);
        if (i < 7) uid_str += " ";
    }
    uid_str.toUpperCase();
    JS_SetPropertyStr(ctx, obj, "uid", JS_NewString(ctx, uid_str.c_str()));

    // Blocks count
    JS_SetPropertyStr(ctx, obj, "blocks", JS_NewInt32(ctx, 128));

    // Size in bytes
    JS_SetPropertyStr(ctx, obj, "size", JS_NewInt32(ctx, 512));

    // Data as hex string using getDump() method
    String dump_str = "";
    uint8_t *dump = srixReader->getDump();
    for (uint16_t i = 0; i < 512; i++) {
        if (dump[i] < 0x10) dump_str += "0";
        dump_str += String(dump[i], HEX);
    }
    dump_str.toUpperCase();
    JS_SetPropertyStr(ctx, obj, "data", JS_NewString(ctx, dump_str.c_str()));

    return obj;
}

JSValue native_srixWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: srixWrite(timeout_in_seconds : number = 10);
    // returns: { success: boolean, message: string }

    int timeout = 10;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) { JS_ToInt32(ctx, &timeout, argv[0]); }

    SRIXTool *srixReader = getSRIXReader();

    // Use headless write function
    int result = srixReader->write_tag_headless(timeout);

    // Create return object
    JSValue obj = JS_NewObject(ctx);

    switch (result) {
        case 0:
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(true));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Tag written successfully"));
            break;
        case -1:
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Timeout: no tag present"));
            break;
        case -2:
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Tag types do not match"));
            break;
        default:
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Error writing data to tag"));
            break;
    }

    return obj;
}

JSValue native_srixSave(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: srixSave(filename : string);
    // returns: { success: boolean, message: string, filepath: string }

    if (argc < 1 || !JS_IsString(ctx, argv[0])) { return JS_NULL; }

    JSCStringBuf buf;
    const char *filename = JS_ToCString(ctx, argv[0], &buf);
    if (!filename) { return JS_NULL; }

    SRIXTool *srixReader = getSRIXReader();

    // Save file
    String result = srixReader->save_file_headless(String(filename));

    // Create return object
    JSValue obj = JS_NewObject(ctx);

    if (!result.isEmpty()) {
        JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(true));
        JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "File saved successfully"));
        JS_SetPropertyStr(ctx, obj, "filepath", JS_NewString(ctx, result.c_str()));
    } else {
        JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
        JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Error saving file"));
        JS_SetPropertyStr(ctx, obj, "filepath", JS_NewString(ctx, ""));
    }

    return obj;
}

JSValue native_srixLoad(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: srixLoad(filename : string);
    // returns: { uid: string, blocks: number, size: number, data: string } or null

    if (argc < 1 || !JS_IsString(ctx, argv[0])) { return JS_NULL; }

    JSCStringBuf buf;
    const char *filename = JS_ToCString(ctx, argv[0], &buf);
    if (!filename) { return JS_NULL; }

    SRIXTool *srixReader = getSRIXReader();

    // Load file
    int result = srixReader->load_file_headless(String(filename));

    if (result != 0) { return JS_NULL; }

    // Create a JS object from the loaded data
    JSValue obj = JS_NewObject(ctx);

    // UID using getUID()
    String uid_str = "";
    uint8_t *uid = srixReader->getUID();
    for (uint8_t i = 0; i < 8; i++) {
        if (uid[i] < 0x10) uid_str += "0";
        uid_str += String(uid[i], HEX);
        if (i < 7) uid_str += " ";
    }
    uid_str.toUpperCase();
    JS_SetPropertyStr(ctx, obj, "uid", JS_NewString(ctx, uid_str.c_str()));

    JS_SetPropertyStr(ctx, obj, "blocks", JS_NewInt32(ctx, 128));
    JS_SetPropertyStr(ctx, obj, "size", JS_NewInt32(ctx, 512));

    // Data as hex string using getDump()
    String dump_str = "";
    uint8_t *dump = srixReader->getDump();
    for (uint16_t i = 0; i < 512; i++) {
        if (dump[i] < 0x10) dump_str += "0";
        dump_str += String(dump[i], HEX);
    }
    dump_str.toUpperCase();
    JS_SetPropertyStr(ctx, obj, "data", JS_NewString(ctx, dump_str.c_str()));

    return obj;
}

JSValue native_srixClear(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: srixClear();
    // returns: undefined
    clearSRIXReader();
    return JS_UNDEFINED;
}

JSValue native_srixWriteBlock(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: srixWriteBlock(blockNum : number, blockData : string);
    // blockData must be 8 hex characters (4 bytes)
    // returns: { success: boolean, message: string }

    // Validate parameters
    if (argc < 2 || !JS_IsNumber(ctx, argv[0]) || !JS_IsString(ctx, argv[1])) {
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
        JS_SetPropertyStr(
            ctx,
            obj,
            "message",
            JS_NewString(ctx, "Invalid parameters: blockNum (number) and blockData (string) required")
        );
        return obj;
    }

    int block_num;
    JS_ToInt32(ctx, &block_num, argv[0]);

    JSCStringBuf buf;
    const char *hex_data = JS_ToCString(ctx, argv[1], &buf);
    if (!hex_data) {
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
        JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Failed to read blockData string"));
        return obj;
    }

    // Validate block number
    if (block_num < 0 || block_num > 127) {
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
        JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Block number must be 0-127"));
        return obj;
    }

    // Validate hex data length (must be 8 characters = 4 bytes)
    if (strlen(hex_data) != 8) {
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
        JS_SetPropertyStr(
            ctx, obj, "message", JS_NewString(ctx, "Block data must be 8 hex characters (4 bytes)")
        );
        return obj;
    }

    // Convert hex string to bytes
    uint8_t block_data[4];
    for (int i = 0; i < 4; i++) {
        char byte_str[3] = {hex_data[i * 2], hex_data[i * 2 + 1], '\0'};
        block_data[i] = (uint8_t)strtol(byte_str, NULL, 16);
    }

    // Write block
    SRIXTool *srixReader = getSRIXReader();
    int result = srixReader->write_single_block_headless((uint8_t)block_num, block_data);

    // Create return object
    JSValue obj = JS_NewObject(ctx);

    switch (result) {
        case 0:
            // Success
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(true));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Write + Verify SUCCESS!"));
            break;
        case 1:
            // Success with verify mismatch
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(true));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "VERIFY MISMATCH (write assumed OK)"));
            break;
        case 2:
            // Success but verify skipped
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(true));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "VERIFY SKIPPED (no RST / RF dirty)"));
            break;
        case -1:
            // Timeout
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Timeout: no tag present"));
            break;
        case -3:
            // Invalid data pointer
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Invalid data pointer"));
            break;
        case -4:
            // Invalid block number
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Invalid block number"));
            break;
        case -5:
            // Write failed
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Write operation failed"));
            break;
        default:
            // Unknown error
            JS_SetPropertyStr(ctx, obj, "success", JS_NewBool(false));
            JS_SetPropertyStr(ctx, obj, "message", JS_NewString(ctx, "Unknown error"));
            break;
    }

    return obj;
}

#endif
