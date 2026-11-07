#pragma once

// CHSC report decoding adapted from LORA Tunnel. See NOTICE.md for attribution.

#include <stddef.h>
#include <stdint.h>

namespace heltec_v4 {

// CHSC6xxx normal mapped report: point count, event flags, 12-bit coordinates.
inline bool decodeTouch(const uint8_t *report, size_t size, uint16_t &x, uint16_t &y) {
    if (size != 7) return false;
    const uint8_t count = report[2] & 7;
    const uint8_t event = report[3] >> 6;
    if (count == 0 || count > 2 || event == 1 || event == 3) return false;
    const uint16_t rawX = ((report[3] & 15) << 8) | report[4];
    const uint16_t rawY = ((report[5] & 15) << 8) | report[6];
    if (rawX >= 240 || rawY >= 320) return false;
    x = rawX;
    y = rawY;
    return true;
}

// Raw touch coordinates match ST7789 rotation 0. Use physical panel dimensions,
// including Bruce's bottom 20-pixel navigation strip, when rotating them.
inline void rotateTouch(uint8_t rotation, uint16_t &x, uint16_t &y) {
    const uint16_t rawX = x;
    const uint16_t rawY = y;
    switch (rotation & 3) {
        case 1: x = rawY; y = 239 - rawX; break;
        case 2: x = 239 - rawX; y = 319 - rawY; break;
        case 3: x = 319 - rawY; y = rawX; break;
        default: break;
    }
}

enum class ButtonEvent { None, Click, Hold };

enum InputAction : uint8_t { Select = 1, Back = 2, Next = 4, Previous = 8, Touch = 16 };

struct InputEvent {
    uint8_t actions = 0;
    uint16_t x = 0;
    uint16_t y = 0;
};

// Sampling continues while the UI consumes an earlier event. Keep short taps
// in a bounded FIFO instead of losing their release during the UI's 75 ms gate.
class InputQueue {
    static constexpr uint8_t Capacity = 8;
    InputEvent events[Capacity]{};
    uint8_t head = 0;
    uint8_t count = 0;

public:
    bool push(const InputEvent &event) {
        if (count == Capacity) return false;
        events[(head + count) % Capacity] = event;
        ++count;
        return true;
    }

    bool pop(InputEvent &event) {
        if (count == 0) return false;
        event = events[head];
        head = (head + 1) % Capacity;
        --count;
        return true;
    }
};

class Button {
    bool sampled = false;
    bool stable = false;
    bool held = false;
    uint32_t changedAt = 0;
    uint32_t pressedAt = 0;

public:
    void begin(bool pressed, uint32_t now) {
        sampled = stable = pressed;
        held = pressed; // Ignore a button already held during boot/wake.
        changedAt = pressedAt = now;
    }

    ButtonEvent update(bool pressed, uint32_t now) {
        if (pressed != sampled) {
            sampled = pressed;
            changedAt = now;
        }
        if (sampled != stable && uint32_t(now - changedAt) >= 25) {
            stable = sampled;
            if (stable) {
                pressedAt = now;
                held = false;
            } else if (!held) {
                return ButtonEvent::Click;
            }
        }
        if (stable && sampled && !held && uint32_t(now - pressedAt) >= 650) {
            held = true;
            return ButtonEvent::Hold;
        }
        return ButtonEvent::None;
    }
};

} // namespace heltec_v4
