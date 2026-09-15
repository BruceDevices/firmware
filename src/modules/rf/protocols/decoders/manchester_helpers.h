#pragma once

#include <stdint.h>
#include <vector>

// Manchester event types (mirrors Flipper Zero lib/subghz/blocks/const.h).
enum ManchesterEvent : uint8_t {
    ManchesterEventReset = 0,
    ManchesterEventShortLow,
    ManchesterEventShortHigh,
    ManchesterEventLongLow,
    ManchesterEventLongHigh,
};

// Manchester decoder state (mirrors Flipper manchester_advance).
struct ManchesterState {
    uint8_t state = 0; // 0=start, 1=mid_bit, 2=done
};

// Reset the Manchester state machine.
void manchester_reset(ManchesterState &s);

// Feed one event to the Manchester decoder. When a complete bit is decoded,
// sets `*data` to the bit value and returns true. Returns false when more
// events are needed.
bool manchester_advance(ManchesterState &s, ManchesterEvent event, bool *data);

// Map a (level, unsigned duration, te_short, te_long, te_delta) quad to a
// ManchesterEvent. Returns ManchesterEventReset if the duration doesn't match.
ManchesterEvent manchester_event_for(bool level, unsigned int dur,
                                     unsigned int te_short, unsigned int te_long,
                                     unsigned int te_delta);
