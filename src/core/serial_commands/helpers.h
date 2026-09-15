#ifndef __SERIAL_HELPERS_H__
#define __SERIAL_HELPERS_H__
#ifndef LITE_VERSION
#include <precompiler_flags.h>

#include <Arduino.h>
#include <PSRamFS.h>

bool _setupPsramFs();
char *_readFileFromSerial(size_t fileSizeChar = SAFE_STACK_BUFFER_SIZE);

#endif
#endif
