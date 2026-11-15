#pragma once
#include <stdint.h>

namespace MangaBoot {
constexpr uint32_t requested = 0x4d414e47;
constexpr uint32_t active = 0x4d414e41;
enum class Mode { Normal, Manga, Menu };

inline Mode consume(uint32_t &intent, bool softwareReset, bool powerOn) {
    const uint32_t previous = intent;
    intent = 0;
    if (previous == requested && softwareReset) {
        intent = active;
        return Mode::Manga;
    }
    if (previous == active && !powerOn) return Mode::Menu;
    return Mode::Normal;
}
}
