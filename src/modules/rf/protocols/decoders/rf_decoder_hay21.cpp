// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_hay21.h"
#include "../rf_config.h"

#define TE_SHORT 300
#define TE_LONG 700
#define TE_DELTA 150
#define MIN_BITS 21

static inline int DURATION_DIFF(int a, int b) { return (a > b) ? (a - b) : (b - a); }

bool rf_decode_hay21(const std::vector<int>& durations, RfCodes& out) {
    enum { ST_RESET, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? raw : -raw;

        switch (step) {
        case ST_RESET:
            if (!level && DURATION_DIFF(dur, TE_LONG * 6) < TE_DELTA * 3) {
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
                    DURATION_DIFF(dur, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (DURATION_DIFF(te_last, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA &&
                           DURATION_DIFF(dur, (unsigned int)TE_LONG) < (unsigned int)TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (DURATION_DIFF(dur, TE_LONG * 6) < TE_DELTA * 2) {
                    if (DURATION_DIFF(te_last, (unsigned int)TE_LONG) < (unsigned int)TE_DELTA)
                        { data = (data << 1) | 1ULL; bits++; }
                    if (DURATION_DIFF(te_last, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA)
                        { data = (data << 1) | 0ULL; bits++; }
                    if (bits == MIN_BITS) {
                        out.key = data;
                        out.Bit = MIN_BITS;
                        out.te = TE_SHORT;
                        out.protocol = "Hay21";
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
