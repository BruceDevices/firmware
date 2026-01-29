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
    returnToMenu = true;
    interpreter_state = 1;
    startInterpreterTask();
    return true;
}

String getScriptsFolder(FS *&fs) {
    String folder;
    String possibleFolders[] = {"/scripts", "/BruceScripts", "/BruceJS"};
    int listSize = sizeof(possibleFolders) / sizeof(possibleFolders[0]);

    for (int i = 0; i < listSize; i++) {
        if (SD.exists(possibleFolders[i])) {
            fs = &SD;
            return possibleFolders[i];
        }
        if (LittleFS.exists(possibleFolders[i])) {
            fs = &LittleFS;
            return possibleFolders[i];
        }
    }
    return "";
}

std::vector<Option> getScriptsOptionsList(bool saveStartupScript) {
    std::vector<Option> opt = {};
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    FS *fs;
    String folder = getScriptsFolder(fs);
    if (folder == "") return opt; // did not find

    File root = fs->open(folder);
    if (!root || !root.isDirectory()) return opt; // not a dir

    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        String nameOnly = fullPath.substring(fullPath.lastIndexOf("/") + 1);
        if (fullPath == "") { break; }
        // Serial.printf("Path: %s (isDir: %d)\n", fullPath.c_str(), isDir);

        if (isDir) continue;

        int dotIndex = nameOnly.lastIndexOf(".");
        String ext = dotIndex >= 0 ? nameOnly.substring(dotIndex + 1) : "";
        ext.toUpperCase();
        if (ext != "JS" && ext != "BJS") continue;

        String entry_title = nameOnly.substring(0, nameOnly.lastIndexOf(".")); // remove the extension
        opt.push_back({entry_title.c_str(), [=]() {
                           if (saveStartupScript) {
                               bruceConfig.startupAppJSInterpreterFile = fullPath;
                               bruceConfig.saveFile();
                           } else {
                               run_bjs_script_headless(*fs, fullPath);
                           }
                       }});
    }

    root.close();

    std::sort(opt.begin(), opt.end(), [](const Option &a, const Option &b) {
        String fa = String(a.label);
        fa.toUpperCase();
        String fb = String(b.label);
        fb.toUpperCase();
        return fa < fb;
    });

#endif
    return opt;
}

#endif
