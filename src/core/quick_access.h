/**
 * @file quick_access.h
 * @brief Quick Access Manager — pin/unpin favorite files (IR, SubGHz, BadUSB)
 * @version 0.1
 *
 * Persists pinned file references to /quick_access.json in LittleFS.
 * Max 15 pinned items.
 */

#ifndef __QUICK_ACCESS_H__
#define __QUICK_ACCESS_H__

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

struct PinnedItem {
    String filepath;  // full path, e.g. "/BruceIR/kipas.ir"
    String label;     // display name (filename without extension)
    String type;      // "ir", "sub", or "txt"
    uint8_t fsType;   // 0 = LittleFS, 1 = SD

    JsonDocument toJson() const {
        JsonDocument doc;
        doc["filepath"] = filepath;
        doc["label"]    = label;
        doc["type"]     = type;
        doc["fsType"]   = fsType;
        return doc;
    }

    static PinnedItem fromJson(const JsonObjectConst &obj) {
        PinnedItem item;
        item.filepath = obj["filepath"].as<String>();
        item.label    = obj["label"].as<String>();
        item.type     = obj["type"].as<String>();
        item.fsType   = obj["fsType"].as<uint8_t>();
        return item;
    }
};

class QuickAccessManager {
public:
    static constexpr size_t MAX_ITEMS = 15;
    static constexpr const char *CONFIG_PATH = "/quick_access.json";

    QuickAccessManager() { load(); }

    // Load from LittleFS
    bool load();

    // Save to LittleFS
    bool save();

    // Pin a file (returns false if already pinned or max reached)
    bool pin(const PinnedItem &item);

    // Unpin by index
    bool unpin(size_t index);

    // Unpin by filepath
    bool unpin(const String &filepath);

    // Check if a filepath is already pinned
    bool isPinned(const String &filepath) const;

    // Get all pinned items
    const std::vector<PinnedItem> &items() const { return _items; }

    // Number of pinned items
    size_t count() const { return _items.size(); }

    // Clear all pins
    void clear();

private:
    std::vector<PinnedItem> _items;
};

// Global singleton accessor
QuickAccessManager &getQuickAccessManager();

#endif // __QUICK_ACCESS_H__
