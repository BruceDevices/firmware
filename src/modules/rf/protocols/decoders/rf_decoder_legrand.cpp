// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_legrand.h"
#include "../rf_config.h"

#define TE_SHORT 375
#define TE_LONG 1125
#define TE_DELTA 150
#define MIN_BITS 18

static inline int DURATION_DIFF(int a, int b) { return (a > b) ? (a - b) : (b - a); }

bool rf_decode_legrand(const std::vector<int>& durations, RfCodes& out) {
    enum { ST_RESET, ST_FIRST, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    int te_sum = 0;
    uint64_t last_data = 0;
    unsigned int te_last_low = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? raw : -raw;

        switch (step) {
        case ST_RESET:
            if (!level && DURATION_DIFF(dur, (unsigned int)(TE_SHORT * 16)) < TE_DELTA * 8) {
                data = 0; bits = 0; te_sum = 0; te_last_low = 0;
                step = ST_FIRST;
            }
            break;

        case ST_FIRST:
            if (level) {
                if (DURATION_DIFF(dur, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    te_sum += dur * 4;
                }
                if (DURATION_DIFF(dur, (unsigned int)TE_LONG) < TE_DELTA * 3) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    te_sum += (dur / 3) * 4;
                }
                if (bits > 0) {
                    step = ST_SAVE;
                    break;
                }
            }
            step = ST_RESET;
            break;

        case ST_SAVE:
            if (!level) {
                te_last_low = dur;
                te_sum += dur;
                step = ST_CHECK;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_CHECK:
            if (level) {
                int found = 0;
                if (DURATION_DIFF(te_last_low, (unsigned int)TE_LONG) < TE_DELTA * 3 &&
                    DURATION_DIFF(dur, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA) {
                    found = 1;
                    data = (data << 1) | 0ULL;
                    bits++;
                }
                if (DURATION_DIFF(te_last_low, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA &&
                    DURATION_DIFF(dur, (unsigned int)TE_LONG) < TE_DELTA * 3) {
                    found = 1;
                    data = (data << 1) | 1ULL;
                    bits++;
                }
                if (found) {
                    te_sum += dur;
                    if (bits >= MIN_BITS && last_data && last_data == data) {
                        te_sum /= bits * 4;
                        out.key = data;
                        out.Bit = MIN_BITS;
                        out.te = te_sum;
                        out.protocol = "Legrand";
                        out.preset = "Ook270Async";
                        return true;
                    }
                    last_data = data;
                    step = ST_SAVE;
                    break;
                }
            }
            step = ST_RESET;
            break;
        }
    }
    return false;
}
