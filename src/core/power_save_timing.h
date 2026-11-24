#ifndef BRUCE_POWER_SAVE_TIMING_H
#define BRUCE_POWER_SAVE_TIMING_H

#include <stdint.h>

// Absolute idle time avoids accumulating drift when polling or drawing is slow.
// Caller checks the screen-off threshold before invoking this function.
inline uint8_t idleFadeBrightness(uint8_t start, uint32_t fadingMs) {
    const uint32_t steps = fadingMs / 5;
    return steps >= start ? 0 : uint8_t(start - steps);
}

#endif
