#include "mifare_keys_manager.h"
#include "sd_functions.h"

/**
 * @brief Ensures keys are loaded (lazy loading)
 */
void MifareKeysManager::ensureLoaded(std::set<String> &keys) {
    if (!keys.empty()) return; // Already loaded

    // Try loading from file (SD priority)
    if ((setupSdCard() && SD.exists(KEYS_PATH)) || LittleFS.exists(KEYS_PATH)) {
        loadFromFile(keys);
    } else {
        createDefaultFile(keys);
    }
}

/**
 * @brief Adds a new key
 */
void MifareKeysManager::addKey(std::set<String> &keys, String key) {
    key.toUpperCase();

    if (!isValidHexKey(key)) {
        log_e("Invalid MIFARE key format");
        return;
    }

    ensureLoaded(keys);

    if (keys.find(key) != keys.end()) {
        log_w("Key already exists");
        return;
    }

    keys.insert(key);
    appendToFile(key);

    log_i("Key added");
}

/**
 * @brief Removes a key
 */
void MifareKeysManager::removeKey(std::set<String> &keys, const String &key) {
    ensureLoaded(keys);

    auto it = keys.find(key);
    if (it == keys.end()) {
        log_w("Key not found");
        return;
    }

    keys.erase(it);
    saveToFile(keys);

    log_i("Key removed");
}

/**
 * @brief Saves all keys to file
 */
void MifareKeysManager::save(const std::set<String> &keys) { saveToFile(keys); }

/**
 * @brief Reloads keys from file
 */
void MifareKeysManager::reload(std::set<String> &keys) {
    keys.clear();
    loadFromFile(keys);
}

/**
 * @brief Clears all keys and deletes files
 */
void MifareKeysManager::clear(std::set<String> &keys) {
    keys.clear();

    if (LittleFS.exists(KEYS_PATH)) LittleFS.remove(KEYS_PATH);
    if (setupSdCard() && SD.exists(KEYS_PATH)) SD.remove(KEYS_PATH);

    log_i("All keys cleared");
}

/**
 * @brief Validates key format
 */
bool MifareKeysManager::isValidHexKey(const String &key) {
    if (key.length() != 12) return false;

    const char *str = key.c_str();
    for (int i = 0; i < 12; i++) {
        if (!isxdigit(static_cast<unsigned char>(str[i]))) { return false; }
    }
    return true;
}

/**
 * @brief Validates and removes invalid keys
 */
void MifareKeysManager::validateKeys(std::set<String> &keys) {
    for (auto it = keys.begin(); it != keys.end();) {
        if (!isValidHexKey(*it)) {
            log_w("Removing invalid key: %s", it->c_str());
            it = keys.erase(it);
        } else {
            ++it;
        }
    }
}

// ========== PRIVATE METHODS ==========

void MifareKeysManager::loadFromFile(std::set<String> &keys) {
    FS *sourceFS = nullptr;
    bool fromSD = false;

    if (setupSdCard() && SD.exists(KEYS_PATH)) {
        sourceFS = &SD;
        fromSD = true;
        log_i("Loading keys from SD");
    } else if (LittleFS.exists(KEYS_PATH)) {
        sourceFS = &LittleFS;
        log_i("Loading keys from LittleFS");
    } else {
        log_w("No keys file found");
        return;
    }

    File file = sourceFS->open(KEYS_PATH, FILE_READ);
    if (!file) {
        log_e("Failed to open keys file");
        return;
    }

    keys.clear();
    int loaded = 0, skipped = 0;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.length() == 0 || line.startsWith("//")) continue;

        line.toUpperCase();
        if (isValidHexKey(line)) {
            keys.insert(line);
            loaded++;
        } else {
            log_w("Invalid key skipped: %s", line.c_str());
            skipped++;
        }
    }
    file.close();

    // Sync to other filesystem - PRESERVING ORIGINAL ORDER
    if (fromSD) {
        // SD → LittleFS (copy byte-a-byte)
        copyFileToFS(&SD, &LittleFS, "LittleFS");
    } else if (setupSdCard()) {
        // LittleFS → SD (copy byte-a-byte)
        copyFileToFS(&LittleFS, &SD, "SD");
    }

    log_i("Loaded %d keys%s", loaded, (skipped > 0 ? " (" + String(skipped) + " skipped)" : "").c_str());
}

bool MifareKeysManager::copyFileToFS(FS *sourceFS, FS *destFS, const char *destFsName) {
    // Ensure destination directory exists
    if (!destFS->exists(KEYS_DIR)) {
        if (!destFS->mkdir(KEYS_DIR)) {
            log_e("Failed to create dir on %s", destFsName);
            return false;
        }
    }

    // Open source file for reading
    File sourceFile = sourceFS->open(KEYS_PATH, FILE_READ);
    if (!sourceFile) {
        log_e("Failed to open source file for copying");
        return false;
    }

    // Open destination file for writing
    File destFile = destFS->open(KEYS_PATH, FILE_WRITE);
    if (!destFile) {
        log_e("Failed to open destination file on %s", destFsName);
        sourceFile.close();
        return false;
    }

    // Copy byte-a-byte (preserves EXACT content and order)
    const size_t bufferSize = 512;
    uint8_t buffer[bufferSize];
    size_t totalCopied = 0;

    while (sourceFile.available()) {
        size_t bytesRead = sourceFile.read(buffer, bufferSize);
        if (bytesRead > 0) {
            destFile.write(buffer, bytesRead);
            totalCopied += bytesRead;
        }
    }

    sourceFile.close();
    destFile.close();

    log_i("File copied to %s (%d bytes)", destFsName, totalCopied);
    return true;
}

void MifareKeysManager::saveToFile(const std::set<String> &keys) {
    bool sdSuccess = false;

    if (setupSdCard()) { sdSuccess = writeToFS(&SD, "SD", keys); }

    if (sdSuccess) {
        writeToFS(&LittleFS, "LittleFS", keys);
    } else {
        log_w("SD not available, using LittleFS only");
        writeToFS(&LittleFS, "LittleFS", keys);
    }
}

void MifareKeysManager::appendToFile(const String &key) {
    bool sdSuccess = false;

    if (setupSdCard()) { sdSuccess = appendToFS(&SD, "SD", key); }

    if (sdSuccess) {
        appendToFS(&LittleFS, "LittleFS", key);
    } else {
        log_w("SD not available, appending to LittleFS only");
        appendToFS(&LittleFS, "LittleFS", key);
    }
}

void MifareKeysManager::createDefaultFile(std::set<String> &keys) {
    log_i("Creating default keys file");

    keys.insert("FFFFFFFFFFFF");
    keys.insert("A0A1A2A3A4A5");
    keys.insert("D3F7D3F7D3F7");

    saveToFile(keys);
}

bool MifareKeysManager::writeToFS(FS *fs, const char *fsName, const std::set<String> &keys) {
    if (!fs->exists(KEYS_DIR)) {
        if (!fs->mkdir(KEYS_DIR)) {
            log_e("Failed to create dir on %s", fsName);
            return false;
        }
    }

    File file = fs->open(KEYS_PATH, FILE_WRITE);
    if (!file) {
        log_e("Failed to open file on %s", fsName);
        return false;
    }

    file.println("//BRUCE MIFARE KEYS FILE");
    file.println("//ADD YOUR KEYS ONE PER LINE");
    file.println("//");
    file.println("//STANDARD MIFARE KEYS");

    for (const auto &key : keys) { file.println(key); }

    file.println("//CUSTOM KEYS");
    file.close();

    log_i("%d keys saved to %s", keys.size(), fsName);
    return true;
}

bool MifareKeysManager::appendToFS(FS *fs, const char *fsName, const String &key) {
    if (!fs->exists(KEYS_DIR)) { fs->mkdir(KEYS_DIR); }

    if (!fs->exists(KEYS_PATH)) {
        log_i("File missing on %s, creating", fsName);
        // Need to create full file - will be done by caller
        return false;
    }

    File file = fs->open(KEYS_PATH, FILE_APPEND);
    if (!file) {
        log_w("Failed to append to %s", fsName);
        return false;
    }

    file.println(key);
    file.close();
    return true;
}
