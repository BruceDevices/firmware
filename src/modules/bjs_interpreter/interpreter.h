#ifndef __BJS_INTERPRETER_H__
#define __BJS_INTERPRETER_H__
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "core/display.h"
#include "helpers_js.h"
#include "stdio.h"
#include <duktape.h>
#include <string.h>

extern char *script;
extern char *scriptDirpath;
extern char *scriptName;

#include "audio_js.h"
#include "badusb_js.h"
#include "device_js.h"
#include "dialog_js.h"
#include "display_js.h"
#include "globals_js.h"
#include "gpio_js.h"
#include "helpers_js.h"
#include "i2c_js.h"
#include "ir_js.h"
#include "keyboard_js.h"
#include "math_js.h"
#include "mic_js.h"
#include "notification_js.h"
#include "serial_js.h"
#include "storage_js.h"
#include "subghz_js.h"
#include "wifi_js.h"

// Credits to https://github.com/justinknight93/Doolittle
// This functionality is dedicated to @justinknight93 for providing such a nice example! Consider yourself a
// part of the team!

void interpreterHandler(void *pvParameters);
void run_bjs_script();
bool run_bjs_script_headless(char *code);
bool run_bjs_script_headless(FS fs, String filename);

duk_ret_t native_print(duk_context *ctx);
duk_ret_t native_console_log(duk_context *ctx);

FileParamsJS js_get_path_from_params(duk_context *ctx, bool require_exists);

const char *nth_strchr(const char *s, char c, int8_t n);
void *ps_alloc_function(void *udata, duk_size_t size);
void *ps_realloc_function(void *udata, void *ptr, duk_size_t newsize);
void ps_free_function(void *udata, void *ptr);
void js_fatal_error_handler(void *udata, const char *msg);

duk_ret_t native_exit(duk_context *ctx);
duk_ret_t native_require(duk_context *ctx);
duk_ret_t native_assert(duk_context *ctx);
duk_ret_t native_load(duk_context *ctx);

#endif
#endif
