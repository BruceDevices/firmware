#ifndef __BJS_INTERPRETER_H__
#define __BJS_INTERPRETER_H__
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "core/display.h"
#include "stdio.h"

#include <string.h>

extern TaskHandle_t interpreterTaskHandler;

// Credits to https://github.com/justinknight93/Doolittle
// This functionality is dedicated to @justinknight93 for providing such a nice example! Consider yourself a
// part of the team!

void interpreterHandler(void *pvParameters);
void run_bjs_script();
bool run_bjs_script_headless(char *code);
bool run_bjs_script_headless(FS fs, String filename);

String getScriptsFolder(FS *&fs);
std::vector<Option>
getScriptsOptionsList(String currentPath = "", bool saveStartupScript = false, int rememberedIndex = 0);

#endif
#endif
