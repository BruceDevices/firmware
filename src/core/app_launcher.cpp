#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "app_launcher.h"
#include "sd_functions.h"
#include "modules/bjs_interpreter/interpreter.h"
#include <ArduinoJson.h>
#include <SD.h>
#include <LittleFS.h>

static bool parseManifest(FS &fs, const String &dirPath, AppManifest &out) {
    String manifestPath = dirPath + "/manifest.json";
    if (!fs.exists(manifestPath)) return false;

    File f = fs.open(manifestPath, "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;

    out.name        = doc["name"] | dirPath.substring(dirPath.lastIndexOf('/') + 1);
    out.description = doc["description"] | "";
    out.entryPoint  = doc["entry"] | "main.js";
    out.category    = doc["category"] | "Other";
    out.version     = doc["version"] | "1.0";
    out.basePath    = dirPath;
    out.fs          = &fs;
    return true;
}

static void scanDirectory(FS &fs, const String &root, std::vector<AppManifest> &apps) {
    File dir = fs.open(root);
    if (!dir || !dir.isDirectory()) return;

    while (true) {
        bool isDir;
        String path = dir.getNextFileName(&isDir);
        if (path.isEmpty()) break;
        if (!isDir) continue;
        // Skip hidden directories
        String name = path.substring(path.lastIndexOf('/') + 1);
        if (name.startsWith(".")) continue;

        AppManifest app;
        if (parseManifest(fs, path, app)) {
            apps.push_back(app);
        }
    }
    dir.close();
}

std::vector<AppManifest> discoverApps() {
    std::vector<AppManifest> apps;

    // Scan SD card first
    setupSdCard();
    if (sdcardMounted) {
        if (SD.exists("/apps")) scanDirectory(SD, "/apps", apps);
    }

    // Then LittleFS
    if (LittleFS.exists("/apps")) {
        scanDirectory(LittleFS, "/apps", apps);
    }

    return apps;
}

bool launchApp(const AppManifest &app) {
    String scriptPath = app.basePath + "/" + app.entryPoint;
    if (!app.fs->exists(scriptPath)) {
        log_e("App entry point not found: %s", scriptPath.c_str());
        return false;
    }
    return run_bjs_script_headless(*app.fs, scriptPath);
}

std::vector<AppManifest> filterAppsByCategory(const std::vector<AppManifest> &apps, const String &category) {
    std::vector<AppManifest> filtered;
    for (const auto &app : apps) {
        if (app.category.equalsIgnoreCase(category)) {
            filtered.push_back(app);
        }
    }
    return filtered;
}

#endif
