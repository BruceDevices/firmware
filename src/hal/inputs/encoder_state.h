#ifndef BRUCE_ENCODER_STATE_H
#define BRUCE_ENCODER_STATE_H

#include <stdint.h>

// One owner samples GPIO. Consumers receive a copy under the input mutex; no ISR
// touches this state. Keep the last press after release so a short click survives
// a slow menu redraw or the legacy input-event throttle.
struct EncoderButtonState {
    bool down = false;
    bool suppressed = false;
    uint32_t generation = 0;
    uint32_t pressedAt = 0;
    uint32_t releasedAt = 0;
};

class EncoderButtonDebounce {
public:
    void init(bool pressed, uint32_t now) {
        raw = pressed;
        rawSince = now;
        state.down = pressed;
        state.suppressed = pressed; // A button held at startup is not a fresh click.
        state.pressedAt = now;
    }

    bool sample(bool pressed, uint32_t now) {
        if (pressed != raw) {
            raw = pressed;
            rawSince = now;
        }
        if (raw == state.down || uint32_t(now - rawSince) < 20) return false;
        state.down = raw;
        if (raw) {
            ++state.generation;
            state.pressedAt = rawSince;
            state.suppressed = false;
        } else {
            state.releasedAt = rawSince;
        }
        return true;
    }

    EncoderButtonState state;

private:
    bool raw = false;
    uint32_t rawSince = 0;
};

// Per-invocation menu state. Entry consumes the current generation, so a held
// button cannot enter a child menu. Movement cancels a pending hold until the
// next distinct press; synthetic selectors can cancel it explicitly.
class EncoderMenuSelect {
public:
    explicit EncoderMenuSelect(uint32_t entryGeneration) : seen(entryGeneration) {}

    void cancel() { pending = false; }

    bool update(
        const EncoderButtonState &button, uint32_t now, int index, bool moved,
        bool detectHold, bool &longPress
    ) {
        longPress = false;
        if (moved || (pending && index != pendingIndex)) {
            cancel();
        }
        if (button.generation != seen) {
            seen = button.generation;
            cancel();
            if (!button.suppressed) {
                if (!detectHold) return true;
                pending = true;
                pendingIndex = index;
                pressedAt = button.pressedAt;
            }
        }
        if (pending) {
            const uint32_t heldMs = (button.down ? now : button.releasedAt) - pressedAt;
            if (!button.down || heldMs >= 500) {
                longPress = heldMs >= 500;
                cancel();
                return true;
            }
        }
        return false;
    }

private:
    uint32_t seen;
    uint32_t pressedAt = 0;
    int pendingIndex = -1;
    bool pending = false;
};

#endif
