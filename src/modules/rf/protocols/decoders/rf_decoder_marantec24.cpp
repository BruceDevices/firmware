// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_marantec24.h"
#include "../rf_config.h"

#define TE_SHORT 800
#define TE_LONG 1600
#define TE_DELTA 200
#define MIN_BITS 24

static inline int DURATION_DIFF(int a, int b) { return (a > b) ? (a - b) : (b - a); }

bool rf_decode_marantec24(const std::vector<int>& durations, RfCodes& out) {
    enum { ST_RESET, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? raw : -raw;

        switch (step) {
        case ST_RESET:
            if (!level && DURATION_DIFF(dur, TE_LONG * 9) < TE_DELTA * 6) {
                data = 0; bits = 0;
                step = ST_SAVE;
            }
            break;

        case ST_SAVE:
            if (level) {
                te_last = dur;
                step = ST_CHECK;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_CHECK:
            if (!level) {
                if (DURATION_DIFF(te_last, (unsigned int)TE_LONG) < (unsigned int)TE_DELTA &&
                    DURATION_DIFF(dur, TE_SHORT * 3) < (unsigned int)TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (DURATION_DIFF(te_last, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA &&
                           DURATION_DIFF(dur, TE_LONG * 2) < (unsigned int)TE_DELTA) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (DURATION_DIFF(dur, TE_LONG * 9) < TE_DELTA * 6) {
                    if (DURATION_DIFF(te_last, (unsigned int)TE_LONG) < (unsigned int)TE_DELTA)
                        { data = (data << 1) | 0ULL; bits++; }
                    if (DURATION_DIFF(te_last, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA)
                        { data = (data << 1) | 1ULL; bits++; }
                    if (bits == MIN_BITS) {
                        out.key = data;
                        out.Bit = MIN_BITS;
                        out.te = TE_SHORT;
                        out.protocol = "Marantec24";
                        out.preset = "Ook270Async";
                        return true;
                    }
                    data = 0; bits = 0;
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

bool rf_encode_marantec24(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit != MIN_BITS) return false;
    out.push_back(-(TE_LONG * 9));
    for (int r = 0; r < 4; r++) {
        for (int i = in.Bit - 1; i >= 0; i--) {
            if ((in.key >> i) & 1ULL) {
                out.push_back(TE_SHORT);
                out.push_back(i == 0 ? -(TE_LONG * 9 + TE_SHORT) : -(TE_LONG * 2));
            } else {
                out.push_back(TE_LONG);
                out.push_back(i == 0 ? -(TE_LONG * 9 + TE_SHORT) : -(TE_SHORT * 3));
            }
        }
    }
    return true;
}
