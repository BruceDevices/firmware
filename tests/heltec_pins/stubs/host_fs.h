#pragma once
#include "Arduino.h"
#include <algorithm>
#include <cstring>
#include <functional>
#include <map>
#include <memory>

// Skip the firmware's unrelated SD API declarations; retain the actual pin
// configuration implementation and ArduinoJson parser/serializer unchanged.
#define __SD_FUNCTIONS_H__
#define FILE_READ "r"
#define FILE_WRITE "w"

extern std::function<void()> storageAccess;

class File {
    std::shared_ptr<std::string> data;
    size_t pos = 0;

public:
    File() = default;
    explicit File(std::shared_ptr<std::string> value) : data(value) {}
    explicit operator bool() const { return data != nullptr; }
    void close() {}
    int read() { return data && pos < data->size() ? static_cast<unsigned char>((*data)[pos++]) : -1; }
    size_t readBytes(char *dst, size_t length) {
        if (!data) return 0;
        const size_t count = std::min(length, data->size() - pos);
        std::memcpy(dst, data->data() + pos, count);
        pos += count;
        return count;
    }
    size_t write(uint8_t value) { return write(&value, 1); }
    size_t write(const uint8_t *src, size_t length) {
        if (!data) return 0;
        data->append(reinterpret_cast<const char *>(src), length);
        return length;
    }
};

class FS {
public:
    std::map<std::string, std::shared_ptr<std::string>> files;
    bool exists(const char *path) {
        if (storageAccess) storageAccess();
        return files.count(path) != 0;
    }
    File open(const char *path, const char *mode) {
        if (storageAccess) storageAccess();
        if (*mode == 'w') files[path] = std::make_shared<std::string>();
        auto it = files.find(path);
        return it == files.end() ? File() : File(it->second);
    }
    bool rename(const String &from, const String &to) {
        auto it = files.find(from);
        if (it == files.end()) return false;
        files[to] = it->second;
        files.erase(it);
        return true;
    }
};

struct SerialStub {
    size_t write(uint8_t) { return 1; }
    size_t write(const uint8_t *, size_t length) { return length; }
};
extern SerialStub Serial;
extern FS LittleFS;
extern FS SD;
extern bool sdcardMounted;
inline bool copyToFs(FS, FS, String, bool) { return true; }
inline bool checkLittleFsSize() { return true; }
inline bool getFsStorage(FS *&fs) {
    if (storageAccess) storageAccess();
    fs = &LittleFS;
    return true;
}
