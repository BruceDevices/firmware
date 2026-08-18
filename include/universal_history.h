#pragma once

#include <Arduino.h>
#include <FS.h>
#include <vector>

// Shared persistence for the Universal IR/RF "Preferiti/Recenti" lists.
// Every entry references a signal that can be replayed later:
//   - RF:  path = full .sub file path, isDir = false
//   - IR:  path = category folder (isDir = true, signals indexed recursively)
//          or a single .ir file (isDir = false); name = the button/signal name
struct HistEntry {
    String path;
    String name;
    bool isDir = false;
};

// Read a history file ("D|F<TAB>path<TAB>name" per line). Missing file -> empty.
std::vector<HistEntry> hist_load(FS &fs, const String &file);

// Persist the whole list. Returns false when the FS is not writable.
bool hist_save(FS &fs, const String &file, const std::vector<HistEntry> &list);

// True when an entry with the same path+name is present.
bool hist_has(const std::vector<HistEntry> &list, const String &path, const String &name);

// Add to the front, deduplicating path+name, capped at `cap` entries.
void hist_add(std::vector<HistEntry> &list, const String &path, const String &name, bool isDir, size_t cap);

// Remove every entry matching path+name.
void hist_remove(std::vector<HistEntry> &list, const String &path, const String &name);
