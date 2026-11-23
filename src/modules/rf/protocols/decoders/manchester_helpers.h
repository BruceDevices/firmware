#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus

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

// Shared half-level decoder used by Somfy, CAME, Security+ and related codecs.
enum ManchesterHalfState { ME_RESET, ME_LOW, ME_HIGH };
bool manchester_half_advance(ManchesterHalfState &state, bool high, bool &bit);
void manchester_half_reset(ManchesterHalfState &state);

#endif // __cplusplus

// Four-phase stream codec convention (Flipper-compatible event encoding).
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    RfManchesterEventShortLow = 0,
    RfManchesterEventShortHigh = 2,
    RfManchesterEventLongLow = 4,
    RfManchesterEventLongHigh = 6,
    RfManchesterEventReset = 8
} RfManchesterEvent;

typedef enum {
    RfManchesterStateStart1 = 0,
    RfManchesterStateMid1 = 1,
    RfManchesterStateMid0 = 2,
    RfManchesterStateStart0 = 3
} RfManchesterState;

bool rf_manchester_advance(
    RfManchesterState state,
    RfManchesterEvent event,
    RfManchesterState* next_state,
    bool* data);
typedef struct {
    bool prev_bit;
    uint8_t step;
} RfManchesterEncoderState;

typedef enum {
    RfManchesterEncoderResultShortLow = 0b00,
    RfManchesterEncoderResultLongLow = 0b01,
    RfManchesterEncoderResultLongHigh = 0b10,
    RfManchesterEncoderResultShortHigh = 0b11,
} RfManchesterEncoderResult;

void rf_manchester_encoder_reset(RfManchesterEncoderState* state);

bool rf_manchester_encoder_advance(
    RfManchesterEncoderState* state,
    const bool curr_bit,
    RfManchesterEncoderResult* result);

RfManchesterEncoderResult rf_manchester_encoder_finish(RfManchesterEncoderState* state);
#ifdef __cplusplus
}
#endif
