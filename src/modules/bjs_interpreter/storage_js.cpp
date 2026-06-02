#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "storage_js.h"

#include "core/sd_functions.h"

#include "helpers_js.h"

JSValue native_storageReaddir(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    (void)argc;
    // usage: storageReaddir(path: string | Path, options?: { withFileTypes?: false }): string[]

    FileParamsJS fileParams = js_get_path_from_params(ctx, argv, true);
    if (!fileParams.exist) {
        return JS_ThrowTypeError(
            ctx, "%s: Directory does not exist: %s", "storageReaddir", fileParams.path.c_str()
        );
    }

    bool withFileTypes = false;
    if (argc > 1 && JS_IsObject(ctx, argv[1])) {
        JSValue v = JS_GetPropertyStr(ctx, argv[1], "withFileTypes");
        if (!JS_IsUndefined(v)) withFileTypes = JS_ToBool(ctx, v);
    }

    File root = (fileParams.fs)->open(fileParams.path);
    if (!root || !root.isDirectory()) {
        return JS_ThrowTypeError(ctx, "%s: Not a directory: %s", "storageReaddir", fileParams.path.c_str());
    }

    JSValue arr = JS_NewArray(ctx, 0);
    uint32_t index = 0;

    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        String nameOnly = fullPath.substring(fullPath.lastIndexOf("/") + 1);
        if (fullPath == "") { break; }

        if (withFileTypes) {
            JSValue obj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, obj, "name", JS_NewString(ctx, nameOnly.c_str()));

            if (isDir) {
                JS_SetPropertyStr(ctx, obj, "size", JS_NewInt32(ctx, 0));
            } else {
                File file = (fileParams.fs)->open(fullPath);
                JS_SetPropertyStr(ctx, obj, "size", JS_NewInt32(ctx, (int)file.size()));
                file.close();
            }

            JS_SetPropertyStr(ctx, obj, "isDirectory", JS_NewBool(isDir));
            JS_SetPropertyUint32(ctx, arr, index++, obj);
        } else {
            JS_SetPropertyUint32(ctx, arr, index++, JS_NewString(ctx, nameOnly.c_str()));
        }
    }
    root.close();

    return arr;
}

JSValue native_storageRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    bool binary = false;
    if (argc > 1 && JS_IsBool(argv[1])) binary = JS_ToBool(ctx, argv[1]);

    size_t fileSize = 0;
    char *fileContent = NULL;
    FileParamsJS fileParams = js_get_path_from_params(ctx, argv, true);
    if (!fileParams.exist) {
        return JS_ThrowTypeError(ctx, "%s: File: %s does not exist", "storageRead", fileParams.path.c_str());
    }
    if (!fileParams.path.startsWith("/")) fileParams.path = "/" + fileParams.path;

    fileContent = readBigFile(fileParams.fs, fileParams.path, binary, &fileSize);
    if (fileContent == NULL) {
        return JS_ThrowTypeError(ctx, "%s: Could not read file: %s", "storageRead", fileParams.path.c_str());
    }

    JSValue ret;
    if (binary) {
        ret = JS_NewUint8ArrayCopy(ctx, (const uint8_t *)fileContent, fileSize);
    } else {
        ret = JS_NewStringLen(ctx, fileContent, fileSize);
    }
    free(fileContent);
    return ret;
}

JSValue native_storageWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    // usage: storageWrite(path: string | Path, data: string, mode: "write" | "append", position: number |
    // string)

    size_t dataSize = 0;
    const char *dataPtr = NULL;
    JSCStringBuf sb;
    if (argc > 0 && JS_IsString(ctx, argv[1])) { dataPtr = JS_ToCStringLen(ctx, &dataSize, argv[1], &sb); }

    FileParamsJS fileParams = js_get_path_from_params(ctx, argv, true);
    if (!fileParams.path.startsWith("/")) fileParams.path = "/" + fileParams.path;

    const char *mode = FILE_APPEND; // default append
    if (argc > 2 && JS_IsString(ctx, argv[2])) {
        JSCStringBuf mb;
        const char *modeString = JS_ToCString(ctx, argv[2], &mb);
        if (modeString && modeString[0] == 'w') mode = FILE_WRITE;
    }

    File file = (fileParams.fs)->open(fileParams.path, mode, true);
    if (!file) { return JS_NewBool(false); }

    // Check if position is provided
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) {
        int64_t pos;
        int tmp;
        JS_ToInt32(ctx, &tmp, argv[3]);
        pos = tmp;
        if (pos < 0) {
            file.seek(file.size() + pos, SeekSet);
        } else {
            file.seek(pos, SeekSet);
        }
    } else if (argc > 3 && JS_IsString(ctx, argv[3])) {
        size_t tmpSize = 0;
        char *fileContent = readBigFile(fileParams.fs, fileParams.path, false, &tmpSize);
        if (fileContent == NULL) {
            file.close();
            return JS_ThrowTypeError(
                ctx, "%s: Could not read file: %s", "storageWrite", fileParams.path.c_str()
            );
        }
        JSCStringBuf sb2;
        const char *needle = JS_ToCString(ctx, argv[3], &sb2);
        char *foundPos = strstr(fileContent, needle);
        if (foundPos) {
            file.seek(foundPos - fileContent, SeekSet);
        } else {
            file.seek(0, SeekEnd);
        }
        free(fileContent);
    }

    if (dataPtr != NULL && dataSize > 0) { file.write((const uint8_t *)dataPtr, dataSize); }
    file.close();

    return JS_NewBool(true);
}

JSValue native_storageRename(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    (void)argc;
    FileParamsJS oldFileParams = js_get_path_from_params(ctx, argv, true);
    JSCStringBuf nb;
    const char *newPath = JS_ToCString(ctx, argv[1], &nb);

    if (!oldFileParams.exist) {
        return JS_ThrowTypeError(
            ctx, "%s: File: %s does not exist", "storageRename", oldFileParams.path.c_str()
        );
    }

    String oldPath = oldFileParams.path;
    String nPath = newPath ? String(newPath) : String("");

    if (!oldPath.startsWith("/")) oldPath = "/" + oldPath;
    if (!nPath.startsWith("/")) nPath = "/" + nPath;

    bool success = (oldFileParams.fs)->rename(oldPath, nPath);
    return JS_NewBool(success);
}

JSValue native_storageRemove(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    (void)argc;
    FileParamsJS fileParams = js_get_path_from_params(ctx, argv, true);
    if (!fileParams.exist) {
        return JS_ThrowTypeError(
            ctx, "%s: File: %s does not exist", "storageRemove", fileParams.path.c_str()
        );
    }

    if (!fileParams.path.startsWith("/")) { fileParams.path = "/" + fileParams.path; }

    bool success = (fileParams.fs)->remove(fileParams.path);
    return JS_NewBool(success);
}

JSValue native_storageMkdir(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    (void)argc;
    (void)argv;
    FileParamsJS fileParams = js_get_path_from_params(ctx, argv, true);

    if (!fileParams.path.startsWith("/")) fileParams.path = "/" + fileParams.path;

    bool success = true;
    if (success && !(fileParams.fs)->exists(fileParams.path)) {
        success = (fileParams.fs)->mkdir(fileParams.path);
    }

    return JS_NewBool(success);
}

JSValue native_storageRmdir(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    (void)argc;
    (void)argv;
    FileParamsJS fileParams = js_get_path_from_params(ctx, argv, true);

    if (!fileParams.path.startsWith("/")) fileParams.path = "/" + fileParams.path;

    if (!(fileParams.fs)->exists(fileParams.path)) { return JS_NewBool(false); }

    bool success = (fileParams.fs)->rmdir(fileParams.path);
    return JS_NewBool(success);
}

JSValue native_storageSpaceLittleFS(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    (void)argc;
    (void)argv;
    uint32_t totalKiloBytes = (uint32_t)(LittleFS.totalBytes() / 1024);
    uint32_t usedKiloBytes = (uint32_t)(LittleFS.usedBytes() / 1024);

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "total", JS_NewUint32(ctx, totalKiloBytes));
    JS_SetPropertyStr(ctx, obj, "used", JS_NewUint32(ctx, usedKiloBytes));
    JS_SetPropertyStr(ctx, obj, "free", JS_NewUint32(ctx, (totalKiloBytes - usedKiloBytes)));

    return obj;
}

JSValue native_storageSpaceSDCard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    (void)argc;
    (void)argv;
    uint32_t totalKiloBytes = (uint32_t)(SD.totalBytes() / 1024);
    uint32_t usedKiloBytes = (uint32_t)(SD.usedBytes() / 1024);

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "total", JS_NewUint32(ctx, totalKiloBytes));
    JS_SetPropertyStr(ctx, obj, "used", JS_NewUint32(ctx, usedKiloBytes));
    JS_SetPropertyStr(ctx, obj, "free", JS_NewUint32(ctx, (totalKiloBytes - usedKiloBytes)));

    return obj;
}
#endif
