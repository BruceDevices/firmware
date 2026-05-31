#ifndef __APP_LAUNCHER_H__
#define __APP_LAUNCHER_H__
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

#include <Arduino.h>
#include <FS.h>
#include <vector>

struct AppManifest {
    String name;
    String description;
    String entryPoint;   // e.g. "main.js"
    String category;     // e.g. "RF", "BLE", "Tools"
    String version;
    String basePath;     // absolute path to app directory
    FS *fs;              // SD or LittleFS
};

// Scan /apps/ on SD and LittleFS for apps with manifest.json
std::vector<AppManifest> discoverApps();

// Launch an app by running its entry point script
bool launchApp(const AppManifest &app);

// Filter apps by category
std::vector<AppManifest> filterAppsByCategory(const std::vector<AppManifest> &apps, const String &category);

#endif
#endif
