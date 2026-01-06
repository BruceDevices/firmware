// Author: Senape3000
// More info: https://github.com/Senape3000/firmware/blob/main/docs_custom/JS_RFID/RFID_API_README.md
// More info: https://github.com/Senape3000/firmware/blob/main/docs_custom/JS_RFID/RFID_SRIX_API_README.md

#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

#include "rfid_js.h"
#include "globals.h"
#include "helpers_js.h"
#include "modules/rfid/srix_tool.h"
#include "modules/rfid/tag_o_matic.h"

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

// SRIX
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

duk_ret_t putPropRFIDFunctions(duk_context *ctx, duk_idx_t obj_idx, uint8_t magic) {
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "read", native_rfidRead, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "readUID", native_rfidReadUID, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "write", native_rfidWrite, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "save", native_rfidSave, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "load", native_rfidLoad, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "clear", native_rfidClear, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "addMifareKey", native_rfid_AddMifareKey, 1, magic);
    // SRIX functions
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "srixRead", native_srixRead, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "srixWrite", native_srixWrite, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "srixSave", native_srixSave, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "srixLoad", native_srixLoad, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "srixClear", native_srixClear, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "srixWriteBlock", native_srixWriteBlock, 2, magic);
    return 0;
}

duk_ret_t registerRFID(duk_context *ctx) {
    bduk_register_c_lightfunc(ctx, "rfidRead", native_rfidRead, 1);
    bduk_register_c_lightfunc(ctx, "rfidReadUID", native_rfidReadUID, 1);
    bduk_register_c_lightfunc(ctx, "rfidWrite", native_rfidWrite, 1);
    bduk_register_c_lightfunc(ctx, "rfidSave", native_rfidSave, 1);
    bduk_register_c_lightfunc(ctx, "rfidLoad", native_rfidLoad, 1);
    bduk_register_c_lightfunc(ctx, "rfidClear", native_rfidClear, 0);
    bduk_register_c_lightfunc(ctx, "rfidAddMifareKey", native_rfid_AddMifareKey, 1);
    // SRIX functions
    bduk_register_c_lightfunc(ctx, "srixRead", native_srixRead, 1);
    bduk_register_c_lightfunc(ctx, "srixWrite", native_srixWrite, 1);
    bduk_register_c_lightfunc(ctx, "srixSave", native_srixSave, 1);
    bduk_register_c_lightfunc(ctx, "srixLoad", native_srixLoad, 1);
    bduk_register_c_lightfunc(ctx, "srixClear", native_srixClear, 0);
    bduk_register_c_lightfunc(ctx, "srixWriteBlock", native_srixWriteBlock, 2);
    return 0;
}

duk_ret_t native_rfidRead(duk_context *ctx) {
    // usage: rfidRead(timeout_in_seconds : number = 10);
    // returns: object with complete or null data on timeout

    duk_int_t timeout = duk_get_int_default(ctx, 0, 10);

    TagOMatic *tagReader = getTagReader();

    // Use existing headless functionality
    String jsonResult = tagReader->read_tag_headless(timeout);

    if (jsonResult.isEmpty()) {
        duk_push_null(ctx);
        return 1;
    }

    // Create a JS object from the fields
    duk_idx_t obj_idx = duk_push_object(ctx);

    // Extract fields from the interface
    RFIDInterface *rfid = tagReader->getRFIDInterface();

    if (rfid) {
        duk_push_string(ctx, rfid->printableUID.uid.c_str());
        duk_put_prop_string(ctx, obj_idx, "uid");

        duk_push_string(ctx, rfid->printableUID.picc_type.c_str());
        duk_put_prop_string(ctx, obj_idx, "type");

        duk_push_string(ctx, rfid->printableUID.sak.c_str());
        duk_put_prop_string(ctx, obj_idx, "sak");

        duk_push_string(ctx, rfid->printableUID.atqa.c_str());
        duk_put_prop_string(ctx, obj_idx, "atqa");

        duk_push_string(ctx, rfid->printableUID.bcc.c_str());
        duk_put_prop_string(ctx, obj_idx, "bcc");

        duk_push_string(ctx, rfid->strAllPages.c_str());
        duk_put_prop_string(ctx, obj_idx, "pages");

        duk_push_int(ctx, rfid->totalPages);
        duk_put_prop_string(ctx, obj_idx, "totalPages");
    }

    return 1;
}

duk_ret_t native_rfidReadUID(duk_context *ctx) {
    // usage: rfidReadUID(timeout_in_seconds : number = 5);
    // returns: string (UID) or empty string on timeout

    duk_int_t timeout = duk_get_int_default(ctx, 0, 5);

    TagOMatic tagReader(true);

    String uid = tagReader.read_uid_headless(timeout);
    duk_push_string(ctx, uid.c_str());

    return 1;
}

duk_ret_t native_rfidWrite(duk_context *ctx) {
    // usage: rfidWrite(timeout_in_seconds : number = 10);
    // returns: { success: boolean, message: string }

    duk_int_t timeout = duk_get_int_default(ctx, 0, 10);

    TagOMatic *tagReader = getTagReader();

    // Use headless write function
    int result = tagReader->write_tag_headless(timeout);

    // Create return object
    duk_idx_t obj_idx = duk_push_object(ctx);

    switch (result) {
        case RFIDInterface::SUCCESS:
            duk_push_boolean(ctx, true);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "Tag written successfully");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;

        case RFIDInterface::TAG_NOT_PRESENT:
            duk_push_boolean(ctx, false);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "No tag present");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;

        case RFIDInterface::TAG_NOT_MATCH:
            duk_push_boolean(ctx, false);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "Tag types do not match");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;

        default:
            duk_push_boolean(ctx, false);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "Error writing data to tag");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;
    }

    return 1;
}

duk_ret_t native_rfidSave(duk_context *ctx) {
    // usage: rfidSave(filename : string);
    // returns: { success: boolean, message: string, filepath: string }

    if (!duk_is_string(ctx, 0)) {
        duk_push_null(ctx);
        return 1;
    }

    const char *filename = duk_get_string(ctx, 0);

    TagOMatic *tagReader = getTagReader();

    // Save file
    String result = tagReader->save_file_headless(String(filename));

    // Create return object
    duk_idx_t obj_idx = duk_push_object(ctx);

    if (!result.isEmpty()) {
        duk_push_boolean(ctx, true);
        duk_put_prop_string(ctx, obj_idx, "success");

        duk_push_string(ctx, "File saved successfully");
        duk_put_prop_string(ctx, obj_idx, "message");

        duk_push_string(ctx, result.c_str());
        duk_put_prop_string(ctx, obj_idx, "filepath");
    } else {
        duk_push_boolean(ctx, false);
        duk_put_prop_string(ctx, obj_idx, "success");

        duk_push_string(ctx, "Error saving file");
        duk_put_prop_string(ctx, obj_idx, "message");

        duk_push_string(ctx, "");
        duk_put_prop_string(ctx, obj_idx, "filepath");
    }

    return 1;
}

duk_ret_t native_rfidLoad(duk_context *ctx) {
    // usage: rfidLoad(filename : string);
    // returns: object with tag data or null on error

    if (!duk_is_string(ctx, 0)) {
        duk_push_null(ctx);
        return 1;
    }

    const char *filename = duk_get_string(ctx, 0);

    TagOMatic *tagReader = getTagReader();

    // Load file
    int result = tagReader->load_file_headless(String(filename));

    if (result != RFIDInterface::SUCCESS) {
        duk_push_null(ctx);
        return 1;
    }

    // Get the loaded data
    RFIDInterface *rfid = tagReader->getRFIDInterface();

    if (!rfid) {
        duk_push_null(ctx);
        return 1;
    }

    // Create return object with loaded data
    duk_idx_t obj_idx = duk_push_object(ctx);

    duk_push_string(ctx, rfid->printableUID.uid.c_str());
    duk_put_prop_string(ctx, obj_idx, "uid");

    duk_push_string(ctx, rfid->printableUID.picc_type.c_str());
    duk_put_prop_string(ctx, obj_idx, "type");

    duk_push_string(ctx, rfid->printableUID.sak.c_str());
    duk_put_prop_string(ctx, obj_idx, "sak");

    duk_push_string(ctx, rfid->printableUID.atqa.c_str());
    duk_put_prop_string(ctx, obj_idx, "atqa");

    duk_push_string(ctx, rfid->printableUID.bcc.c_str());
    duk_put_prop_string(ctx, obj_idx, "bcc");

    duk_push_string(ctx, rfid->strAllPages.c_str());
    duk_put_prop_string(ctx, obj_idx, "pages");

    duk_push_int(ctx, rfid->totalPages);
    duk_put_prop_string(ctx, obj_idx, "totalPages");

    duk_push_int(ctx, rfid->dataPages);
    duk_put_prop_string(ctx, obj_idx, "dataPages");

    return 1;
}

duk_ret_t native_rfidClear(duk_context *ctx) {
    // usage: rfidClear();
    // returns: undefined

    clearTagReader();
    return 0;
}

duk_ret_t native_rfid_AddMifareKey(duk_context *ctx) {
    if (!duk_is_string(ctx, 0)) {
        duk_idx_t obj_idx = duk_push_object(ctx);
        duk_push_boolean(ctx, false);
        duk_put_prop_string(ctx, obj_idx, "success");
        duk_push_string(ctx, "Invalid parameter: key must be a string");
        duk_put_prop_string(ctx, obj_idx, "message");
        return 1;
    }

    const char *key = duk_get_string(ctx, 0);
    String keyStr = String(key);

    bruceConfig.addMifareKey(keyStr);

    duk_idx_t obj_idx = duk_push_object(ctx);
    duk_push_boolean(ctx, true);
    duk_put_prop_string(ctx, obj_idx, "success");
    duk_push_string(ctx, "Key processed");
    duk_put_prop_string(ctx, obj_idx, "message");
    duk_push_string(ctx, keyStr.c_str());
    duk_put_prop_string(ctx, obj_idx, "key");

    return 1;
}

// ========== SRIX FUNCTIONS ==========

duk_ret_t native_srixRead(duk_context *ctx) {
    // usage: srixRead(timeout_in_seconds : number = 10);
    // returns: { uid: string, blocks: number, size: number, data: string } or null

    duk_int_t timeout = duk_get_int_default(ctx, 0, 10);
    SRIXTool *srixReader = getSRIXReader();

    String jsonResult = srixReader->read_tag_headless(timeout);

    if (jsonResult.isEmpty()) {
        duk_push_null(ctx);
        return 1;
    }

    // Build JS object from data
    duk_idx_t obj_idx = duk_push_object(ctx);

    // Get UID
    String uid_str = "";
    uint8_t *uid = srixReader->getUID();
    for (uint8_t i = 0; i < 8; i++) {
        if (uid[i] < 0x10) uid_str += "0";
        uid_str += String(uid[i], HEX);
        if (i < 7) uid_str += " ";
    }
    uid_str.toUpperCase();
    duk_push_string(ctx, uid_str.c_str());
    duk_put_prop_string(ctx, obj_idx, "uid");

    // Blocks count
    duk_push_int(ctx, 128);
    duk_put_prop_string(ctx, obj_idx, "blocks");

    // Size in bytes
    duk_push_int(ctx, 512);
    duk_put_prop_string(ctx, obj_idx, "size");

    // Data as hex string
    String dump_str = "";
    uint8_t *dump = srixReader->getDump();
    for (uint16_t i = 0; i < 512; i++) {
        if (dump[i] < 0x10) dump_str += "0";
        dump_str += String(dump[i], HEX);
    }
    dump_str.toUpperCase();
    duk_push_string(ctx, dump_str.c_str());
    duk_put_prop_string(ctx, obj_idx, "data");

    return 1;
}

duk_ret_t native_srixWrite(duk_context *ctx) {
    // usage: srixWrite(timeout_in_seconds : number = 10);
    // returns: { success, message, blocksVerified, code }

    duk_int_t timeout = duk_get_int_default(ctx, 0, 10);
    SRIXTool *srixReader = getSRIXReader();

    int result = srixReader->write_tag_headless(timeout);

    duk_idx_t obj_idx = duk_push_object(ctx);

    // expose numeric result (IMPORTANT)
    duk_push_int(ctx, result);
    duk_put_prop_string(ctx, obj_idx, "code");

    if (result == 0) {
        // WRITE + VERIFY OK
        duk_push_boolean(ctx, true);
        duk_put_prop_string(ctx, obj_idx, "success");

        duk_push_string(ctx, "Tag written and fully verified");
        duk_put_prop_string(ctx, obj_idx, "message");

        duk_push_int(ctx, 128);
        duk_put_prop_string(ctx, obj_idx, "blocksVerified");

    } else if (result > 0) {
        // WRITE OK, VERIFY PARTIAL / SKIPPED
        duk_push_boolean(ctx, true);
        duk_put_prop_string(ctx, obj_idx, "success");

        duk_push_string(ctx, "Tag written successfully (partial or skipped verification)");
        duk_put_prop_string(ctx, obj_idx, "message");

        duk_push_int(ctx, result);
        duk_put_prop_string(ctx, obj_idx, "blocksVerified");

    } else if (result == -1) {
        // Timeout
        duk_push_boolean(ctx, false);
        duk_put_prop_string(ctx, obj_idx, "success");

        duk_push_string(ctx, "Timeout: no tag present");
        duk_put_prop_string(ctx, obj_idx, "message");

    } else if (result == -2) {
        // No data
        duk_push_boolean(ctx, false);
        duk_put_prop_string(ctx, obj_idx, "success");

        duk_push_string(ctx, "No data in memory. Read or load a tag first");
        duk_put_prop_string(ctx, obj_idx, "message");

    } else if (result == -6) {
        // NFC not initialized
        duk_push_boolean(ctx, false);
        duk_put_prop_string(ctx, obj_idx, "success");

        duk_push_string(ctx, "NFC not initialized");
        duk_put_prop_string(ctx, obj_idx, "message");

    } else {
        // Real write failure
        duk_push_boolean(ctx, false);
        duk_put_prop_string(ctx, obj_idx, "success");

        duk_push_string(ctx, "Write operation failed");
        duk_put_prop_string(ctx, obj_idx, "message");
    }

    return 1;
}

duk_ret_t native_srixSave(duk_context *ctx) {
    // usage: srixSave(filename : string);
    // returns: { success: boolean, message: string, filepath: string }

    if (!duk_is_string(ctx, 0)) {
        duk_push_null(ctx);
        return 1;
    }

    const char *filename = duk_get_string(ctx, 0);
    SRIXTool *srixReader = getSRIXReader();

    String result = srixReader->save_file_headless(String(filename));

    // Create return object
    duk_idx_t obj_idx = duk_push_object(ctx);

    if (!result.isEmpty()) {
        duk_push_boolean(ctx, true);
        duk_put_prop_string(ctx, obj_idx, "success");
        duk_push_string(ctx, "File saved successfully");
        duk_put_prop_string(ctx, obj_idx, "message");
        duk_push_string(ctx, result.c_str());
        duk_put_prop_string(ctx, obj_idx, "filepath");
    } else {
        duk_push_boolean(ctx, false);
        duk_put_prop_string(ctx, obj_idx, "success");
        duk_push_string(ctx, "Error saving file. Check if tag data is loaded");
        duk_put_prop_string(ctx, obj_idx, "message");
        duk_push_string(ctx, "");
        duk_put_prop_string(ctx, obj_idx, "filepath");
    }

    return 1;
}

duk_ret_t native_srixLoad(duk_context *ctx) {
    // usage: srixLoad(filename : string);
    // returns: { uid: string, blocks: number, size: number, data: string } or null

    if (!duk_is_string(ctx, 0)) {
        duk_push_null(ctx);
        return 1;
    }

    const char *filename = duk_get_string(ctx, 0);
    SRIXTool *srixReader = getSRIXReader();

    int result = srixReader->load_file_headless(String(filename));

    if (result != 0) {
        duk_push_null(ctx);
        return 1;
    }

    // Build JS object with loaded data
    duk_idx_t obj_idx = duk_push_object(ctx);

    // UID
    String uid_str = "";
    uint8_t *uid = srixReader->getUID();
    for (uint8_t i = 0; i < 8; i++) {
        if (uid[i] < 0x10) uid_str += "0";
        uid_str += String(uid[i], HEX);
        if (i < 7) uid_str += " ";
    }
    uid_str.toUpperCase();
    duk_push_string(ctx, uid_str.c_str());
    duk_put_prop_string(ctx, obj_idx, "uid");

    duk_push_int(ctx, 128);
    duk_put_prop_string(ctx, obj_idx, "blocks");

    duk_push_int(ctx, 512);
    duk_put_prop_string(ctx, obj_idx, "size");

    // Data as hex string
    String dump_str = "";
    uint8_t *dump = srixReader->getDump();
    for (uint16_t i = 0; i < 512; i++) {
        if (dump[i] < 0x10) dump_str += "0";
        dump_str += String(dump[i], HEX);
    }
    dump_str.toUpperCase();
    duk_push_string(ctx, dump_str.c_str());
    duk_put_prop_string(ctx, obj_idx, "data");

    return 1;
}

duk_ret_t native_srixClear(duk_context *ctx) {
    // usage: srixClear();
    // returns: undefined

    clearSRIXReader();
    return 0;
}

duk_ret_t native_srixWriteBlock(duk_context *ctx) {
    // usage: srixWriteBlock(blockNum : number, blockData : string);
    // blockData must be 8 hex characters (4 bytes)
    // returns: { success: boolean, message: string }

    // Validate parameters
    if (!duk_is_number(ctx, 0) || !duk_is_string(ctx, 1)) {
        duk_idx_t obj_idx = duk_push_object(ctx);
        duk_push_boolean(ctx, false);
        duk_put_prop_string(ctx, obj_idx, "success");
        duk_push_string(ctx, "Invalid parameters: blockNum (number) and blockData (string) required");
        duk_put_prop_string(ctx, obj_idx, "message");
        return 1;
    }

    duk_int_t block_num = duk_get_int(ctx, 0);
    const char *hex_data = duk_get_string(ctx, 1);

    // Validate block number
    if (block_num < 0 || block_num > 127) {
        duk_idx_t obj_idx = duk_push_object(ctx);
        duk_push_boolean(ctx, false);
        duk_put_prop_string(ctx, obj_idx, "success");
        duk_push_string(ctx, "Block number must be 0-127");
        duk_put_prop_string(ctx, obj_idx, "message");
        return 1;
    }

    // Validate hex data length (must be 8 characters = 4 bytes)
    if (strlen(hex_data) != 8) {
        duk_idx_t obj_idx = duk_push_object(ctx);
        duk_push_boolean(ctx, false);
        duk_put_prop_string(ctx, obj_idx, "success");
        duk_push_string(ctx, "Block data must be 8 hex characters (4 bytes)");
        duk_put_prop_string(ctx, obj_idx, "message");
        return 1;
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
    duk_idx_t obj_idx = duk_push_object(ctx);

    switch (result) {
        case 0:
            // Success
            duk_push_boolean(ctx, true);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "Write + Verify SUCCESS!");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;
        case 1:
            // Success
            duk_push_boolean(ctx, true);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "VERIFY MISMATCH (write assumed OK)");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;
        case 2:
            // Success
            duk_push_boolean(ctx, true);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "VERIFY SKIPPED (no RST / RF dirty)");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;

        case -1:
            // Timeout
            duk_push_boolean(ctx, false);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "Timeout: no tag present");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;

        case -3:
            // Invalid data pointer
            duk_push_boolean(ctx, false);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "Invalid data pointer");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;

        case -4:
            // Invalid block number
            duk_push_boolean(ctx, false);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "Invalid block number");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;

        case -5:
            // Write failed
            duk_push_boolean(ctx, false);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "Write operation failed");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;

        default:
            // Unknown error
            duk_push_boolean(ctx, false);
            duk_put_prop_string(ctx, obj_idx, "success");
            duk_push_string(ctx, "Unknown error");
            duk_put_prop_string(ctx, obj_idx, "message");
            break;
    }

    return 1;
}

#endif
