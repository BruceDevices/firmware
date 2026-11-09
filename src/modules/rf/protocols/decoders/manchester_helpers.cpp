// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Manchester decode helpers ported from Flipper Zero firmware
// (GPL-3.0-or-later), lib/subghz/blocks/decoder.c / manchester_advance.
#include "manchester_helpers.h"

static inline unsigned int mh_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

void manchester_reset(ManchesterState &s) {
    s.state = 0;
}

// Transition byte: low two bits = next state, bit 4 = emit, bit 5 = value.
// One interpreter serves both phase conventions; tables preserve legacy RX.
static bool advance_table(const uint8_t *table, uint8_t &state, unsigned event, bool *data) {
    const uint8_t transition = table[state * 5 + event];
    state = transition & 3;
    if(!(transition & 16)) return false;
    if(data) *data = (transition & 32) != 0;
    return true;
}

bool manchester_advance(ManchesterState &s, ManchesterEvent event, bool *data) {
    static const uint8_t transitions[] = {
        0, 1, 1, 48, 48,
        1, 2, 2, 17, 17,
        2, 48, 0, 16, 0
    };
    *data = false;
    if(s.state > 2 || event > ManchesterEventLongHigh) return false;
    return advance_table(transitions, s.state, event, data);
}

ManchesterEvent manchester_event_for(bool level, unsigned int dur,
                                     unsigned int te_short, unsigned int te_long,
                                     unsigned int te_delta) {
    if (!level) {
        if (mh_diff(dur, te_short) < te_delta)
            return ManchesterEventShortLow;
        if (mh_diff(dur, te_long) < te_delta)
            return ManchesterEventLongLow;
    } else {
        if (mh_diff(dur, te_short) < te_delta)
            return ManchesterEventShortHigh;
        if (mh_diff(dur, te_long) < te_delta)
            return ManchesterEventLongHigh;
    }
    return ManchesterEventReset;
}

#if !defined(LITE_VERSION)
extern "C" void rf_stream_memory_fail(void);
bool rf_manchester_advance(RfManchesterState state, RfManchesterEvent event,
                           RfManchesterState *next_state, bool *data) {
    static const uint8_t transitions[] = {49, 1, 1, 1, 1, 1, 0, 1, 18, 1, 3, 1, 49, 1, 1, 1, 18, 1, 1, 1};
    if((unsigned)state > 3 || (unsigned)event > 8 || ((unsigned)event & 1)) {
        *next_state = RfManchesterStateMid1;
        return false;
    }
    uint8_t current = (uint8_t)state;
    bool result = advance_table(transitions, current, (unsigned)event / 2, data);
    *next_state = (RfManchesterState)current;
    return result;
}


void rf_manchester_encoder_reset(RfManchesterEncoderState* state) {
    if(!state) rf_stream_memory_fail();
    state->step = 0;
}

bool rf_manchester_encoder_advance(
    RfManchesterEncoderState* state,
    const bool curr_bit,
    RfManchesterEncoderResult* result) {
    if(!state) rf_stream_memory_fail();
    if(!result) rf_stream_memory_fail();

    bool advance = false;
    switch(state->step) {
    case 0:
        state->prev_bit = curr_bit;
        if(state->prev_bit) {
            *result = RfManchesterEncoderResultShortLow;
        } else {
            *result = RfManchesterEncoderResultShortHigh;
        }
        state->step = 1;
        advance = true;
        break;
    case 1:
        *result = (RfManchesterEncoderResult)((state->prev_bit << 1) + curr_bit);
        if(curr_bit == state->prev_bit) {
            state->step = 2;
        } else {
            state->prev_bit = curr_bit;
            advance = true;
        }
        break;
    case 2:
        if(curr_bit) {
            *result = RfManchesterEncoderResultShortLow;
        } else {
            *result = RfManchesterEncoderResultShortHigh;
        }
        state->prev_bit = curr_bit;
        state->step = 1;
        advance = true;
        break;
    default:
        rf_stream_memory_fail();
        break;
    }
    return advance;
}

RfManchesterEncoderResult rf_manchester_encoder_finish(RfManchesterEncoderState* state) {
    if(!state) rf_stream_memory_fail();

    state->step = 0;

    return (RfManchesterEncoderResult)((state->prev_bit << 1) + state->prev_bit);
}

#endif

bool manchester_half_advance(ManchesterHalfState &state, bool high, bool &bit) {
    switch(state) {
    case ME_RESET: state = high ? ME_HIGH : ME_LOW; return false;
    case ME_LOW:
        if(high) { state = ME_HIGH; return false; }
        bit = true; return true;
    case ME_HIGH:
        if(!high) { state = ME_LOW; return false; }
        bit = false; return true;
    }
    return false;
}
void manchester_half_reset(ManchesterHalfState &state) { state = ME_RESET; }
