#pragma once
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

using String = std::string;
constexpr int DEC = 10;
// ESP32's Arduino overloads distinguish size_t from uint32_t. Preserve that
// distinction when running this fixture with 32-bit MinGW's different typedefs.
#if UINTPTR_MAX == UINT32_MAX
#define uint32_t unsigned long
#endif

class Stream {
public:
    std::string output;
    unsigned printfCalls = 0;
    unsigned vprintfCalls = 0;

    size_t vprintf(const char *format, va_list args) {
        ++vprintfCalls;
        va_list copy;
        va_copy(copy, args);
        const int count = std::vsnprintf(nullptr, 0, format, copy);
        va_end(copy);
        if (count < 0) return 0;
        std::vector<char> buffer(static_cast<size_t>(count) + 1);
        std::vsnprintf(buffer.data(), buffer.size(), format, args);
        output.append(buffer.data(), count);
        return static_cast<size_t>(count);
    }
    size_t printf(const char *format, ...) {
        ++printfCalls;
        va_list args;
        va_start(args, format);
        const size_t count = vprintf(format, args);
        va_end(args);
        return count;
    }
    size_t print(const String &text) {
        output += text;
        return text.size();
    }
    size_t print(int value, int) { return print(std::to_string(value)); }
    size_t println() { return print("\n"); }
    size_t println(const String &text) { return print(text) + println(); }
    size_t println(size_t value) { return println(std::to_string(value)); }
    size_t println(uint32_t value) { return println(std::to_string(value)); }
    size_t println(int value, int radix) { return print(value, radix) + println(); }
    String readStringUntil(char) { return {}; }
    void flush() {}
    int available() { return 0; }
    int read() { return -1; }
    size_t write(uint8_t *bytes, size_t count) {
        output.append(reinterpret_cast<const char *>(bytes), count);
        return count;
    }
};

extern Stream Serial;
