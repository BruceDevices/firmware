#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "interpreter.h"

#include <duktape.h>

char *script = NULL;
char *scriptDirpath = NULL;
char *scriptName = NULL;

// #define DUK_USE_DEBUG
// #define DUK_USE_DEBUG_LEVEL 2
// #define DUK_USE_DEBUG_WRITE

// Code interpreter, must be called in the loop() function to work
void interpreterHandler(void *pvParameters) {
    Serial.printf(
        "init interpreter:\nPSRAM: [Free: %d, max alloc: %d],\nRAM: [Free: %d, "
        "max alloc: %d]\n",
        ESP.getFreePsram(),
        ESP.getMaxAllocPsram(),
        ESP.getFreeHeap(),
        ESP.getMaxAllocHeap()
    );
    if (script == NULL) { return; }
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(bruceConfigPins.rotation);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_WHITE);
    // Create context.
    Serial.println("Create context");
    auto alloc_function = &ps_alloc_function;
    auto realloc_function = &ps_realloc_function;
    auto free_function = &ps_free_function;
    if (!psramFound()) {
        alloc_function = NULL;
        realloc_function = NULL;
        free_function = NULL;
    }

    /// TODO: Add DUK_USE_NATIVE_STACK_CHECK check with
    /// uxTaskGetStackHighWaterMark
    duk_context *ctx =
        duk_create_heap(alloc_function, realloc_function, free_function, NULL, js_fatal_error_handler);

    // Init containers
    clearDisplayModuleData();

    registerConsole(ctx);

    // Typescript emits: Object.defineProperty(exports, "__esModule", { value:
    // true }); In every file, this is polyfill so typescript project can run on
    // Bruce
    duk_push_object(ctx);
    duk_put_global_string(ctx, "exports");

    bduk_register_c_lightfunc(ctx, "require", native_require, 1);
    bduk_register_c_lightfunc(ctx, "assert", native_assert, 2);
    // Deprecated
    bduk_register_c_lightfunc(ctx, "load", native_load, 1);
    registerGlobals(ctx);
    registerMath(ctx);

    // registerAudio(ctx);
    // registerBadUSB(ctx);
    // TODO: BLE UART API js wrapper https://github.com/pr3y/Bruce/pull/1133
    // registerDevice(ctx);
    // registerDialog(ctx);
    // registerDisplay(ctx);
    // registerGPIO(ctx);
    // registerI2C(ctx);
    // registerIR(ctx);
    // registerKeyboard(ctx);
    // registerNotification(ctx);
    // registerSerial(ctx);
    // registerStorage(ctx);
    // registerSubGHz(ctx);
    // registerWiFi(ctx);

    Serial.printf(
        "global populated:\nPSRAM: [Free: %d, max alloc: %d],\nRAM: [Free: %d, "
        "max alloc: %d]\n",
        ESP.getFreePsram(),
        ESP.getMaxAllocPsram(),
        ESP.getFreeHeap(),
        ESP.getMaxAllocHeap()
    );

    // TODO: match flipper syntax
    // https://github.com/jamisonderek/flipper-zero-tutorials/wiki/JavaScript
    // MEMO: API https://duktape.org/api.html
    // https://github.com/joeqread/arduino-duktape/blob/main/src/duktape.h

    Serial.printf("Script length: %d\n", strlen(script));

    if (duk_peval_string(ctx, script) != DUK_EXEC_SUCCESS) {
        tft.fillScreen(bruceConfig.bgColor);
        tft.setTextSize(FM);
        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        tft.drawCentreString("Error", tftWidth / 2, 10, 1);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.setTextSize(FP);
        tft.setCursor(0, 33);

        String errorMessage = "";
        if (duk_is_error(ctx, -1)) {
            errorMessage = duk_safe_to_stacktrace(ctx, -1);
        } else {
            errorMessage = duk_safe_to_string(ctx, -1);
        }
        Serial.printf("eval failed: %s\n", errorMessage.c_str());
        tft.printf("%s\n\n", errorMessage.c_str());

        int lineIndexOf = errorMessage.indexOf("line ");
        int evalIndexOf = errorMessage.indexOf("(eval:");
        Serial.printf("lineIndexOf: %d\n", lineIndexOf);
        Serial.printf("evalIndexOf: %d\n", evalIndexOf);
        String errorLine = "";
        if (lineIndexOf != -1) {
            lineIndexOf += 5;
            errorLine = errorMessage.substring(lineIndexOf, errorMessage.indexOf("\n", lineIndexOf));
        } else if (evalIndexOf != -1) {
            evalIndexOf += 6;
            errorLine = errorMessage.substring(evalIndexOf, errorMessage.indexOf(")", evalIndexOf));
        }
        Serial.printf("errorLine: [%s]\n", errorLine.c_str());

        if (errorLine != "") {
            uint8_t errorLineNumber = errorLine.toInt();
            const char *errorScript = nth_strchr(script, '\n', errorLineNumber - 1);
            Serial.printf("%.80s\n\n", errorScript);
            tft.printf("%.80s\n\n", errorScript);

            if (strstr(errorScript, "let ")) {
                Serial.println("let is not supported, change it to var");
                tft.println("let is not supported, change it to var");
            }
        }

        delay(500);
        while (!check(AnyKeyPress)) { vTaskDelay(50 / portTICK_PERIOD_MS); }
    } else {
        duk_uint_t resultType = duk_get_type_mask(ctx, -1);
        if (resultType & (DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER)) {
            printf("Script ran succesfully, result is: %s\n", duk_safe_to_string(ctx, -1));
        } else {
            printf("Script ran succesfully");
        }
    }
    free((char *)script);
    script = NULL;
    free((char *)scriptDirpath);
    scriptDirpath = NULL;
    free((char *)scriptName);
    scriptName = NULL;
    duk_pop(ctx);

    // Clean up.
    duk_destroy_heap(ctx);

    clearDisplayModuleData();

    // delay(1000);
    interpreter_start = false;
    vTaskDelete(NULL);
    return;
}

// function to start the JS Interpreterm choosinng the file, processing and
// start
void run_bjs_script() {
    String filename;
    FS *fs = &LittleFS;
    setupSdCard();
    if (sdcardMounted) {
        options = {
            {"SD Card",  [&]() { fs = &SD; }      },
            {"LittleFS", [&]() { fs = &LittleFS; }},
        };
        loopOptions(options);
    }
    filename = loopSD(*fs, true, "BJS|JS");
    script = readBigFile(*fs, filename);
    if (script == NULL) { return; }

    returnToMenu = true;
    interpreter_start = true;

    // To stop the script, press Prev and Next together for a few seconds
}

bool run_bjs_script_headless(char *code) {
    script = code;
    if (script == NULL) { return false; }
    scriptDirpath = NULL;
    scriptName = NULL;
    returnToMenu = true;
    interpreter_start = true;
    return true;
}

bool run_bjs_script_headless(FS fs, String filename) {
    script = readBigFile(fs, filename);
    if (script == NULL) { return false; }
    const char *sName = filename.substring(0, filename.lastIndexOf('/')).c_str();
    const char *sDirpath = filename.substring(filename.lastIndexOf('/') + 1).c_str();
    scriptDirpath = strdup(sDirpath);
    scriptName = strdup(sName);
    returnToMenu = true;
    interpreter_start = true;
    return true;
}

const char *nth_strchr(const char *s, char c, int8_t n) {
    const char *nth = s;
    if (c == '\0' || n < 1) return NULL;

    for (int i = 0; i < n; i++) {
        if ((nth = strchr(nth, c)) == 0) break;
        nth++;
    }

    return nth;
}

void *ps_alloc_function(void *udata, duk_size_t size) {
    void *res;
    DUK_UNREF(udata);
    res = ps_malloc(size);
    return res;
}

void *ps_realloc_function(void *udata, void *ptr, duk_size_t newsize) {
    void *res;
    DUK_UNREF(udata);
    res = ps_realloc(ptr, newsize);
    return res;
}

void ps_free_function(void *udata, void *ptr) {
    DUK_UNREF(udata);
    DUK_ANSI_FREE(ptr);
}

void js_fatal_error_handler(void *udata, const char *msg) {
    (void)udata;
    tft.setTextSize(FM);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("Error", tftWidth / 2, 10, 1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(0, 33);

    tft.printf("JS FATAL ERROR: %s\n", (msg != NULL ? msg : "no message"));
    Serial.printf("JS FATAL ERROR: %s\n", (msg != NULL ? msg : "no message"));
    Serial.flush();

    delay(500);
    while (!check(AnyKeyPress)) vTaskDelay(50 / portTICK_PERIOD_MS);
    // We need to restart esp32 after fatal error
    abort();
}

/* 2FIX: not working
// terminate the script
duk_ret_t native_exit(duk_context *ctx) {
  duk_error(ctx, DUK_ERR_ERROR, "Script exited");
  interpreter_start=false;
  return 0;
}
*/

duk_ret_t native_require(duk_context *ctx) {
    duk_idx_t obj_idx = duk_push_object(ctx);

    if (!duk_is_string(ctx, 0)) { return 1; }
    String filepath = duk_to_string(ctx, 0);

    if (filepath == "audio") {
        putPropAudioFunctions(ctx, obj_idx, 0);
    } else if (filepath == "badusb") {
        putPropBadUSBFunctions(ctx, obj_idx, 0);
    } else if (filepath == "blebeacon") {

    } else if (filepath == "dialog" || filepath == "gui") {
        putPropDialogFunctions(ctx, obj_idx, 0);
    } else if (filepath == "display") {
        putPropDisplayFunctions(ctx, obj_idx, 0);

    } else if (filepath == "device" || filepath == "flipper") {
        putPropDeviceFunctions(ctx, obj_idx, 0);
    } else if (filepath == "gpio") {
        putPropGPIOFunctions(ctx, obj_idx, 0);
    } else if (filepath == "i2c") {
        putPropI2CFunctions(ctx, obj_idx, 0);
    } else if (filepath == "http") {
        // TODO: Make the WebServer API compatible with the Node.js API
        // The more compatible we are, the more Node.js scripts can run on Bruce
        // MEMO: We need to implement an event loop so the WebServer can run:
        // https://github.com/svaarala/duktape/tree/master/examples/eventloop

    } else if (filepath == "ir") {
        putPropIRFunctions(ctx, obj_idx, 0);
    } else if (filepath == "keyboard" || filepath == "input") {
        putPropKeyboardFunctions(ctx, obj_idx, 0);
    } else if (filepath == "math") {
        putPropMathFunctions(ctx, obj_idx, 0);
    } else if (filepath == "notification") {
        putPropNotificationFunctions(ctx, obj_idx, 0);
    } else if (filepath == "serial") {
        putPropSerialFunctions(ctx, obj_idx, 0);
    } else if (filepath == "storage") {
        putPropStorageFunctions(ctx, obj_idx, 0);
    } else if (filepath == "subghz") {
        putPropSubGHzFunctions(ctx, obj_idx, 0);
    } else if (filepath == "wifi") {
        putPropWiFiFunctions(ctx, obj_idx, 0);
    } else {
        FS *fs = NULL;
        if (SD.exists(filepath)) fs = &SD;
        else if (LittleFS.exists(filepath)) fs = &LittleFS;
        if (fs == NULL) { return 1; }

        const char *requiredScript = readBigFile(*fs, filepath);
        if (requiredScript == NULL) { return 1; }

        duk_push_string(ctx, "(function(){exports={};module={exports:exports};\n");
        duk_push_string(ctx, requiredScript);
        duk_push_string(ctx, "\n})");
        duk_concat(ctx, 3);

        duk_int_t pcall_rc = duk_pcompile(ctx, DUK_COMPILE_EVAL);
        if (pcall_rc != DUK_EXEC_SUCCESS) { return 1; }

        pcall_rc = duk_pcall(ctx, 1);
        if (pcall_rc == DUK_EXEC_SUCCESS) {
            duk_get_prop_string(ctx, -1, "exports");
            duk_compact(ctx, -1);
        }
    }

    return 1;
}

duk_ret_t native_assert(duk_context *ctx) {
    if (duk_get_boolean_default(ctx, 0, false)) {
        duk_push_boolean(ctx, true);
        return 1;
    }
    return duk_error(ctx, DUK_ERR_ERROR, "Assertion failed: %s", duk_get_string_default(ctx, 1, "assert"));
}

// Deprecated
duk_ret_t native_load(duk_context *ctx) {
    free((char *)script);
    free((char *)scriptDirpath);
    free((char *)scriptName);
    script = strdup(duk_to_string(ctx, 0));
    scriptDirpath = NULL;
    scriptName = NULL;
    return 0;
}

#endif
