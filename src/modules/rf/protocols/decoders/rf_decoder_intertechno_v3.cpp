// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). Intertechno V3 decoder ported from
// Flipper Zero firmware (GPL-3.0-or-later),
// lib/subghz/protocols/intertechno_v3.c.
#include "rf_decoder_intertechno_v3.h"
#include "../rf_config.h"

#define ITV3_TE_SHORT 275
#define ITV3_TE_LONG 1375
#define ITV3_TE_DELTA 150
#define ITV3_MIN_BITS 32
#define ITV3_DIMMING_BITS 36

static inline unsigned int itv3_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_intertechno_v3(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 8) return false;

    enum {
        ST_RESET,
        ST_HEADER, ST_SYNC_H,
        ST_SAVE, ST_CHECK, ST_END
    } step = ST_RESET;

    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (!level && itv3_diff(dur, ITV3_TE_SHORT * 38) < ITV3_TE_DELTA * 15)
                step = ST_HEADER;
            break;

        case ST_HEADER:
            if (level && itv3_diff(dur, ITV3_TE_SHORT) < ITV3_TE_DELTA)
                step = ST_SYNC_H;
            else
                step = ST_RESET;
            break;

        case ST_SYNC_H:
            if (!level && itv3_diff(dur, ITV3_TE_SHORT * 10) < ITV3_TE_DELTA * 3) {
                data = 0;
                bits = 0;
                step = ST_SAVE;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_SAVE:
            if (bits > ITV3_DIMMING_BITS) { step = ST_RESET; break; }
            if (!level) {
                if (dur >= ITV3_TE_SHORT * 11) {
                    if (bits == ITV3_MIN_BITS || bits == ITV3_DIMMING_BITS) {
                        out.key = data;
                        out.Bit = bits;
                        out.te = ITV3_TE_SHORT;
                        out.protocol = "Intertechno_V3";
                        out.preset = "Ook270Async";
                        return true;
                    }
                    step = ST_RESET;
                } else {
                    te_last = dur;
                    step = ST_CHECK;
                }
            } else {
                step = ST_RESET;
            }
            break;

        case ST_CHECK:
            if (level) {
                bool short_low  = itv3_diff(te_last, ITV3_TE_SHORT) < ITV3_TE_DELTA;
                bool long_low   = itv3_diff(te_last, ITV3_TE_LONG) < ITV3_TE_DELTA * 2;
                bool short_high = itv3_diff(dur, ITV3_TE_SHORT) < ITV3_TE_DELTA;

                if (short_low && short_high) {
                    // bit 0: previous LOW was SHORT, current HIGH is SHORT
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_END;
                } else if (long_low && short_high) {
                    // bit 1: previous LOW was LONG, current HIGH is SHORT
                    data = (data << 1) | 1ULL;
                    bits++;
                    step = ST_END;
                } else {
                    step = ST_RESET;
                }
            } else {
                step = ST_RESET;
            }
            break;

        case ST_END:
            if (!level) {
                if (itv3_diff(dur, ITV3_TE_SHORT) < ITV3_TE_DELTA ||
                    itv3_diff(dur, ITV3_TE_LONG) < ITV3_TE_DELTA * 2) {
                    if (bits <= ITV3_DIMMING_BITS) step = ST_SAVE;
                    else step = ST_RESET;
                } else if (dur >= ITV3_TE_SHORT * 11) {
                    if (bits == ITV3_MIN_BITS || bits == ITV3_DIMMING_BITS) {
                        out.key = data;
                        out.Bit = bits;
                        out.te = ITV3_TE_SHORT;
                        out.protocol = "Intertechno_V3";
                        out.preset = "Ook270Async";
                        return true;
                    }
                    step = ST_RESET;
                } else {
                    step = ST_RESET;
                }
            } else {
                step = ST_RESET;
            }
            break;
        }
    }
    return false;
}

bool rf_encode_intertechno_v3(const RfCodes& in, std::vector<int>& out) {
    out.clear();
    out.push_back(ITV3_TE_SHORT);
    out.push_back(-(ITV3_TE_SHORT * 38));
    out.push_back(ITV3_TE_SHORT);
    out.push_back(-(ITV3_TE_SHORT * 10));

    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(ITV3_TE_SHORT);
            out.push_back(-ITV3_TE_LONG);
            out.push_back(ITV3_TE_SHORT);
            out.push_back(-ITV3_TE_SHORT);
        } else {
            out.push_back(ITV3_TE_SHORT);
            out.push_back(-ITV3_TE_SHORT);
            out.push_back(ITV3_TE_SHORT);
            out.push_back(-ITV3_TE_LONG);
        }
    }
    return true;
}
