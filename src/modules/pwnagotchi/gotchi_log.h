#if !defined(LITE_VERSION)
#pragma once

#include <Arduino.h>
#include <FS.h>

// Brucegotchi file logging — appends timestamped events to
// /BrucePCAP/brucegotchi.log on the active filesystem (SD or LittleFS).
void gotchiLogSetFs(FS *fs);
void gotchiLogInit();
void gotchiLogClose();
void gotchiLog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif