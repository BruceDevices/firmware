#include "config_storage.h"
#include "sd_functions.h"
#include <cstdio>
#include <mutex>
#include <unistd.h>

namespace {
std::mutex configMutex;

bool readJson(FS &fs, const String &path, JsonDocument &document) {
    File file = fs.open(path, FILE_READ);
    if (file && !deserializeJson(document, file) && document.is<JsonObject>()) return true;
    document.clear();
    return false;
}

ConfigReadResult loadFrom(FS &fs, const char *path, const String &previous, JsonDocument &document) {
    if (!fs.mountpoint()) return ConfigReadResult::Missing;
    bool found = false;
    for (const char *candidate : {path, previous.c_str()}) {
        if (!fs.exists(candidate)) continue;
        found = true;
        if (readJson(fs, candidate, document)) return ConfigReadResult::Loaded;
    }
    return found ? ConfigReadResult::Unreadable : ConfigReadResult::Missing;
}

struct StdioWriter {
    FILE *file;
    size_t write(uint8_t byte) { return fputc(byte, file) == EOF ? 0 : 1; }
    size_t write(const uint8_t *data, size_t size) { return fwrite(data, 1, size, file); }
};

bool writeStaged(FS &fs, const String &temporary, const JsonDocument &document) {
    const char *mount = fs.mountpoint();
    if (!mount) return false;
    const String fullPath = String(mount) + temporary;
    FILE *file = fopen(fullPath.c_str(), "wb");
    if (!file) return false;
    StdioWriter writer{file};
    const bool saved = serializeJsonPretty(document, writer) == measureJsonPretty(document) &&
                       fflush(file) == 0 && fsync(fileno(file)) == 0;
    const bool closed = fclose(file) == 0;
    return saved && closed;
}

bool publish(FS &fs, const String &path, const String &temporary, const String &previous) {
    bool keepLive = fs.exists(path);
    if (keepLive && fs.exists(previous)) {
        // A restart can leave both copies. Retain a readable one while publishing the new file.
        JsonDocument current;
        if (readJson(fs, path, current)) {
            if (!fs.remove(previous)) return false;
        } else {
            if (!readJson(fs, previous, current) || !fs.remove(path)) return false;
            keepLive = false;
        }
    }
    if (keepLive && !fs.rename(path, previous)) return false;
    if (!fs.rename(temporary, path)) {
        if (fs.exists(previous)) fs.rename(previous, path);
        return false;
    }
    if (fs.exists(previous)) fs.remove(previous);
    return true;
}

bool saveTo(FS &fs, const String &path, const JsonDocument &document) {
    const String temporary = path + ".tmp";
    const String previous = path + ".prev";
    if (writeStaged(fs, temporary, document) && publish(fs, path, temporary, previous)) return true;
    if (fs.mountpoint() && fs.exists(temporary)) fs.remove(temporary);
    return false;
}

void backupOn(FS &fs, const String &path) {
    if (!fs.mountpoint()) return;
    const String previous = path + ".prev";
    const bool hasLive = fs.exists(path);
    const String &source = hasLive ? path : previous;
    if (!hasLive && !fs.exists(previous)) return;

    const String backup = "/bak." + path.substring(1);
    if ((fs.exists(backup) && !fs.remove(backup)) ||
        (hasLive && fs.exists(previous) && !fs.remove(previous)) || !fs.rename(source, backup)) {
        log_e("Failed to back up config %s", path.c_str());
    }
}
} // namespace

ConfigReadResult loadConfigFile(const char *path, JsonDocument &document, bool useSd) {
    const std::lock_guard<std::mutex> lock(configMutex);
    document.clear();
    const String previous = String(path) + ".prev";
    const auto sd = useSd ? loadFrom(SD, path, previous, document) : ConfigReadResult::Missing;
    if (sd == ConfigReadResult::Loaded) return sd;
    const auto onboard = loadFrom(LittleFS, path, previous, document);
    return onboard == ConfigReadResult::Missing ? sd : onboard;
}

void saveConfigFile(const char *path, const JsonDocument &document, bool useSd) {
    if (document.overflowed() || !document.is<JsonObjectConst>()) {
        log_e("Invalid config document for %s", path);
        return;
    }
    const std::lock_guard<std::mutex> lock(configMutex);
    // Boot prefers the SD copy, so commit it before updating the onboard mirror.
    if (useSd && !saveTo(SD, path, document)) {
        log_e("Failed to save config %s to SD", path);
        return;
    }
    if (!saveTo(LittleFS, path, document)) log_e("Failed to save config %s to LittleFS", path);
}

void backupConfigFile(const char *path, bool useSd) {
    const std::lock_guard<std::mutex> lock(configMutex);
    backupOn(LittleFS, path);
    if (useSd) backupOn(SD, path);
}
