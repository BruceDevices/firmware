/**
 * @file quick_access.cpp
 * @brief Quick Access Manager implementation
 */

#include "quick_access.h"
#include "sd_functions.h"
#include <LittleFS.h>

// ── Global singleton ──────────────────────────────────────────────────────
QuickAccessManager &getQuickAccessManager() {
    static QuickAccessManager instance;
    return instance;
}

// ── Load from LittleFS ────────────────────────────────────────────────────
bool QuickAccessManager::load() {
    _items.clear();

    if (!LittleFS.begin()) {
        log_e("LittleFS mount failed");
        return false;
    }

    if (!LittleFS.exists(CONFIG_PATH)) {
        log_i("No quick_access.json yet");
        return true; // empty is valid
    }

    File file = LittleFS.open(CONFIG_PATH, FILE_READ);
    if (!file) {
        log_e("Failed to open quick_access.json");
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        log_e("JSON parse error: %s", err.c_str());
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    for (const auto &v : arr) {
        _items.push_back(PinnedItem::fromJson(v.as<JsonObjectConst>()));
    }

    // Clamp to MAX_ITEMS on load (defensive)
    if (_items.size() > MAX_ITEMS) _items.resize(MAX_ITEMS);

    log_i("Loaded %zu pinned items", _items.size());
    return true;
}

// ── Save to LittleFS ──────────────────────────────────────────────────────
bool QuickAccessManager::save() {
    if (!LittleFS.begin()) {
        log_e("LittleFS mount failed");
        return false;
    }

    // Remove stale file first (JsonDocument limitation with File)
    if (LittleFS.exists(CONFIG_PATH)) LittleFS.remove(CONFIG_PATH);

    File file = LittleFS.open(CONFIG_PATH, FILE_WRITE);
    if (!file) {
        log_e("Failed to write quick_access.json");
        return false;
    }

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const auto &item : _items) {
        arr.add(item.toJson().as<JsonObjectConst>());
    }

    size_t bytes = serializeJson(doc, file);
    file.close();

    log_i("Saved %zu pinned items (%zu bytes)", _items.size(), bytes);
    return true;
}

// ── Pin ──────────────────────────────────────────────────────────────────
bool QuickAccessManager::pin(const PinnedItem &item) {
    if (isPinned(item.filepath)) return false;
    if (_items.size() >= MAX_ITEMS) return false;

    _items.push_back(item);
    save();
    return true;
}

// ── Unpin by index ───────────────────────────────────────────────────────
bool QuickAccessManager::unpin(size_t index) {
    if (index >= _items.size()) return false;

    _items.erase(_items.begin() + index);
    save();
    return true;
}

// ── Unpin by filepath ────────────────────────────────────────────────────
bool QuickAccessManager::unpin(const String &filepath) {
    for (size_t i = 0; i < _items.size(); i++) {
        if (_items[i].filepath == filepath) {
            _items.erase(_items.begin() + i);
            save();
            return true;
        }
    }
    return false;
}

// ── Check pinned ─────────────────────────────────────────────────────────
bool QuickAccessManager::isPinned(const String &filepath) const {
    for (const auto &item : _items) {
        if (item.filepath == filepath) return true;
    }
    return false;
}

// ── Clear all ────────────────────────────────────────────────────────────
void QuickAccessManager::clear() {
    _items.clear();
    save();
}
