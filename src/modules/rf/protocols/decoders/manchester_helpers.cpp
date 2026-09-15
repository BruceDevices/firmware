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

bool manchester_advance(ManchesterState &s, ManchesterEvent event, bool *data) {
    bool result = false;
    *data = false;

    switch (s.state) {
    case 0: // Start
        switch (event) {
        case ManchesterEventShortHigh:
            s.state = 1; // MidBit
            break;
        case ManchesterEventShortLow:
            s.state = 1; // MidBit
            break;
        case ManchesterEventLongHigh:
            *data = true;
            result = true;
            break;
        case ManchesterEventLongLow:
            *data = true;
            result = true;
            break;
        default:
            break;
        }
        break;

    case 1: // MidBit
        switch (event) {
        case ManchesterEventShortLow:
            s.state = 2; // Done
            break;
        case ManchesterEventShortHigh:
            s.state = 2; // Done
            break;
        case ManchesterEventLongLow:
            *data = false;
            result = true;
            break;
        case ManchesterEventLongHigh:
            *data = false;
            result = true;
            break;
        default:
            break;
        }
        break;

    case 2: // Done
        switch (event) {
        case ManchesterEventShortLow:
            *data = true;
            result = true;
            s.state = 0;
            break;
        case ManchesterEventShortHigh:
            s.state = 0;
            break;
        case ManchesterEventLongLow:
            *data = false;
            result = true;
            s.state = 0;
            break;
        case ManchesterEventLongHigh:
            s.state = 0;
            break;
        default:
            break;
        }
        break;
    }

    return result;
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
