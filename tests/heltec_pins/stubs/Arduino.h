#pragma once
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>

using gpio_num_t = int;
constexpr int GPIO_NUM_NC = -1;
constexpr int GPIO_NUM_MAX = 49;
constexpr int GPIO_PIN_COUNT = 49;
constexpr int HEX = 16;

class String : public std::string {
public:
    using std::string::string;
    String() = default;
    String(const std::string &value) : std::string(value) {}
    String(int value, int radix = 10) {
        std::ostringstream out;
        if (radix == HEX) out << std::hex;
        out << value;
        assign(out.str());
    }
    String substring(size_t start) const { return substr(start); }
};

struct EspStub {
    uint64_t getEfuseMac() const { return 0x123456789ABCULL; }
};
static EspStub ESP;
#define log_e(...) ((void)0)
#define log_i(...) ((void)0)
#define log_w(...) ((void)0)
