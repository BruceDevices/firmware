#if !defined(LITE_VERSION)
#include "gotchi_log.h"

#include <stdarg.h>

static FS *logFs = nullptr;
static bool logActive = false;

void gotchiLogSetFs(FS *fs) { logFs = fs; }

void gotchiLogInit() {
    logActive = false;
    if (logFs == nullptr) return;
    if (!logFs->exists("/BrucePCAP")) logFs->mkdir("/BrucePCAP");
    logActive = true;
    gotchiLog("=== Brucegotchi session started ===");
}

void gotchiLogClose() {
    if (!logActive) return;
    gotchiLog("=== Brucegotchi session ended ===");
    logActive = false;
}

void gotchiLog(const char *fmt, ...) {
    if (!logActive || logFs == nullptr) return;

    // Uptime HH:MM:SS (same convention as drawTime)
    unsigned long elapsed = millis() / 1000;
    int h = elapsed / 3600;
    int m = (elapsed % 3600) / 60;
    int s = elapsed % 60;

    char msg[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    File f = logFs->open("/BrucePCAP/brucegotchi.log", FILE_APPEND);
    if (!f) return;
    f.printf("[%02d:%02d:%02d] %s\n", h, m, s, msg);
    f.flush();
    f.close();
}
#endif