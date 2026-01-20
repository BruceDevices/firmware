#ifndef __MIFARE_KEYS_MANAGER_H__
#define __MIFARE_KEYS_MANAGER_H__

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <set>

/**
 * @brief Internal helper for managing MIFARE keys
 * This class is used internally by BruceConfig
 * External code should use BruceConfig.mifareKeys directly
 */
class MifareKeysManager {
public:
    // Constants
    static constexpr const char *KEYS_PATH = "/BruceRFID/keys.conf";
    static constexpr const char *KEYS_DIR = "/BruceRFID";

    // Core operations (work directly on provided std::set reference)
    static void ensureLoaded(std::set<String> &keys);
    static void addKey(std::set<String> &keys, String key);
    static void removeKey(std::set<String> &keys, const String &key);
    static void save(const std::set<String> &keys);
    static void reload(std::set<String> &keys);
    static void clear(std::set<String> &keys);

    // Validation
    static bool isValidHexKey(const String &key);
    static void validateKeys(std::set<String> &keys);

private:
    // File I/O
    static void loadFromFile(std::set<String> &keys);
    static void saveToFile(const std::set<String> &keys);
    static void appendToFile(const String &key);
    static void createDefaultFile(std::set<String> &keys);

    // Filesystem helpers
    static bool writeToFS(FS *fs, const char *fsName, const std::set<String> &keys);
    static bool appendToFS(FS *fs, const char *fsName, const String &key);
    static bool copyFileToFS(FS *sourceFS, FS *destFS, const char *destFsName);
};

#endif
