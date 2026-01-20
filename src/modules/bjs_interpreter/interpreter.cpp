#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "interpreter.h"
#include "core/utils.h"

static void js_log_func(void *opaque, const void *buf, size_t buf_len) { fwrite(buf, 1, buf_len, stdout); }

extern "C" {
#include "mqjs_stdlib.h"
}

#include "display_js.h"
#include "globals_js.h"

char *script = NULL;
char *scriptDirpath = NULL;
char *scriptName = NULL;

TaskHandle_t interpreterTaskHandler = NULL;

void interpreterHandler(void *pvParameters) {
    printMemoryUsage("init interpreter");
    if (script == NULL) { return; }

    while (interpreter_state != 2) { vTaskDelay(pdMS_TO_TICKS(500)); }

    tft.fillScreen(TFT_BLACK);
    tft.setRotation(bruceConfigPins.rotation);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_WHITE);

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
    registerRFID(ctx);
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
    bool psramAvailable = psramFound();

    size_t mem_size = psramAvailable ? 65536 : 32768;
    uint8_t *mem_buf = psramAvailable ? (uint8_t *)ps_malloc(mem_size) : (uint8_t *)malloc(mem_size);
    JSContext *ctx = JS_NewContext(mem_buf, mem_size, &js_stdlib);
    JS_SetLogFunc(ctx, js_log_func);

    js_timers_init(ctx);

    // Set global variables
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(
        ctx, global, "__filepath", JS_NewString(ctx, (String(scriptDirpath) + String(scriptName)).c_str())
    );
    JS_SetPropertyStr(ctx, global, "__dirpath", JS_NewString(ctx, scriptDirpath));
    JS_SetPropertyStr(ctx, global, "BRUCE_VERSION", JS_NewString(ctx, BRUCE_VERSION));
    JS_SetPropertyStr(ctx, global, "BRUCE_PRICOLOR", JS_NewInt32(ctx, bruceConfig.priColor));
    JS_SetPropertyStr(ctx, global, "BRUCE_SECCOLOR", JS_NewInt32(ctx, bruceConfig.secColor));
    JS_SetPropertyStr(ctx, global, "BRUCE_BGCOLOR", JS_NewInt32(ctx, bruceConfig.bgColor));

    JS_SetPropertyStr(ctx, global, "HIGH", JS_NewInt32(ctx, HIGH));
    JS_SetPropertyStr(ctx, global, "LOW", JS_NewInt32(ctx, LOW));
    JS_SetPropertyStr(ctx, global, "INPUT", JS_NewInt32(ctx, INPUT));
    JS_SetPropertyStr(ctx, global, "OUTPUT", JS_NewInt32(ctx, OUTPUT));
    JS_SetPropertyStr(ctx, global, "PULLUP", JS_NewInt32(ctx, PULLUP));
    JS_SetPropertyStr(ctx, global, "INPUT_PULLUP", JS_NewInt32(ctx, INPUT_PULLUP));
    JS_SetPropertyStr(ctx, global, "PULLDOWN", JS_NewInt32(ctx, PULLDOWN));
    JS_SetPropertyStr(ctx, global, "INPUT_PULLDOWN", JS_NewInt32(ctx, INPUT_PULLDOWN));

    printMemoryUsage("context created");

    size_t scriptSize = strlen(script);
    log_d("Script length: %zu\n", scriptSize);

    JSValue val = JS_Eval(ctx, (const char *)script, scriptSize, scriptName, 0);

    run_timers(ctx);

    LongPress = false;
    if (JS_IsException(val)) { js_fatal_error_handler(ctx); }

    // Clean up.
    free((char *)script);
    script = NULL;
    free((char *)scriptDirpath);
    scriptDirpath = NULL;
    free((char *)scriptName);
    scriptName = NULL;

    js_timers_deinit(ctx);
    JS_FreeContext(ctx);
    free(mem_buf);

    printMemoryUsage("deinit interpreter");

    // TODO: if backgroud app implemented, store in ctx and set if on foreground/background

    interpreter_state = -1;
    vTaskDelete(NULL);
    return;
}

void startInterpreterTask() {
    if (interpreterTaskHandler != NULL) {
        log_w("Interpreter task already running");
        interpreter_state = 1;
        return;
    }

    xTaskCreateUniversal(
        interpreterHandler,          // Task function
        "interpreterHandler",        // Task Name
        INTERPRETER_TASK_STACK_SIZE, // Stack size
        NULL,                        // Task parameters
        2,                           // Task priority (0 to 3), loopTask has priority 2.
        &interpreterTaskHandler,     // Task handle
        ARDUINO_RUNNING_CORE         // run on core the same core as loop task
    );
}

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
    script = readBigFile(fs, filename);
    if (script == NULL) { return; }

    returnToMenu = true;
    interpreter_state = 1;
    startInterpreterTask();
}

bool run_bjs_script_headless(char *code) {
    script = code;
    if (script == NULL) { return false; }
    scriptDirpath = strdup("/scripts");
    scriptName = strdup("index.js");

    returnToMenu = true;
    interpreter_state = 1;
    startInterpreterTask();
    return true;
}

bool run_bjs_script_headless(FS fs, String filename) {
    script = readBigFile(&fs, filename);
    if (script == NULL) { return false; }

    int slash = filename.lastIndexOf('/');
    scriptName = strdup(filename.c_str() + slash + 1);
    scriptDirpath = strndup(filename.c_str(), slash);

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
    } else if (filepath == "rfid") {
        putPropRFIDFunctions(ctx, obj_idx, 0);
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
    returnToMenu = true;
    interpreter_state = 1;
    startInterpreterTask();
    return true;
}

#endif
