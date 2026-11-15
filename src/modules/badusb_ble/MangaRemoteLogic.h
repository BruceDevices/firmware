#pragma once

#include <stdint.h>

// Hardware-neutral button state machine. Call every 5-10 ms with raw GPIO level.
class MangaRemoteLogic {
public:
    enum class Action : uint8_t { None, NextPage, PreviousPage, LongHold };
    static constexpr uint32_t DebounceMs = 25;
    static constexpr uint32_t DoubleClickMs = 280;
    static constexpr uint32_t LongHoldMs = 900;

    Action update(bool pressed, uint32_t now) {
        if (pressed != rawPressed_) {
            rawPressed_ = pressed;
            rawChangeMs_ = now;
        }
        if (pressed != stablePressed_ && now - rawChangeMs_ >= DebounceMs) {
            stablePressed_ = pressed;
            if (stablePressed_) {
                pressStartMs_ = now;
                holdHandled_ = false;
                if (firstClickPending_) {
                    // Use the start of the RAW press: a valid second click can
                    // finish debouncing AFTER the double-click deadline.
                    if (rawChangeMs_ - firstReleaseMs_ <= DoubleClickMs) {
                        secondPress_ = true;
                    } else {
                        firstClickPending_ = false;
                        return Action::NextPage;
                    }
                }
            } else if (!holdHandled_) {
                if (secondPress_) {
                    firstClickPending_ = false;
                    secondPress_ = false;
                    return Action::PreviousPage;
                }
                firstClickPending_ = true;
                firstReleaseMs_ = now;
            }
        }
        if (stablePressed_ && !holdHandled_ && now - pressStartMs_ >= LongHoldMs) {
            holdHandled_ = true;
            firstClickPending_ = false;
            secondPress_ = false;
            return Action::LongHold;
        }
        // Do not fire the first click while a potential second press is
        // physically down but still within its debounce interval.
        if (firstClickPending_ && !stablePressed_ && !rawPressed_ && !secondPress_ &&
            now - firstReleaseMs_ >= DoubleClickMs) {
            firstClickPending_ = false;
            return Action::NextPage;
        }
        return Action::None;
    }

private:
    bool rawPressed_ = false;
    bool stablePressed_ = false;
    bool firstClickPending_ = false;
    bool secondPress_ = false;
    bool holdHandled_ = false;
    uint32_t rawChangeMs_ = 0;
    uint32_t pressStartMs_ = 0;
    uint32_t firstReleaseMs_ = 0;
};

namespace MangaWheel {
// At 16 ms per unit, a full queue takes 192 ms while notifications succeed.
constexpr int32_t MaxPending = 12;
constexpr uint32_t IntervalMs = 16;

inline void add(int32_t &pending, int32_t steps, uint8_t sensitivity) {
    if ((pending > 0 && steps < 0) || (pending < 0 && steps > 0)) pending = 0;
    if (steps > MaxPending) steps = MaxPending;
    if (steps < -MaxPending) steps = -MaxPending;
    pending += steps * sensitivity;
    if (pending > MaxPending) pending = MaxPending;
    if (pending < -MaxPending) pending = -MaxPending;
}

inline int8_t ready(int32_t pending, uint32_t now, uint32_t lastSent) {
    if (!pending || now - lastSent < IntervalMs) return 0;
    return pending > 0 ? 1 : -1;
}

inline void sent(int32_t &pending, uint32_t &lastSent, int8_t wheel, uint32_t now) {
    pending -= wheel;
    lastSent = now;
}
}
