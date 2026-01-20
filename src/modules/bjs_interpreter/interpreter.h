#ifndef __BJS_INTERPRETER_H__
#define __BJS_INTERPRETER_H__
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "core/display.h"
#include "stdio.h"

#include <string.h>

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
#include "notification_js.h"
#include "rfid_js.h"
#include "serial_js.h"
#include "storage_js.h"
#include "subghz_js.h"
#include "wifi_js.h"
extern TaskHandle_t interpreterTaskHandler;

// Credits to https://github.com/justinknight93/Doolittle
// This functionality is dedicated to @justinknight93 for providing such a nice example! Consider yourself a
// part of the team!

void interpreterHandler(void *pvParameters);
void run_bjs_script();
bool run_bjs_script_headless(char *code);
bool run_bjs_script_headless(FS fs, String filename);

#endif
#endif
