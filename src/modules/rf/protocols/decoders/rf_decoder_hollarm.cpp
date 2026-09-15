// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_hollarm.h"
#include "../rf_config.h"

#define TE_SHORT 200
#define TE_LONG 1000
#define TE_DELTA 200
#define MIN_BITS 42

static inline int DURATION_DIFF(int a, int b) { return (a > b) ? (a - b) : (b - a); }

bool rf_decode_hollarm(const std::vector<int>& durations, RfCodes& out) {
    enum { ST_RESET, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? raw : -raw;

        switch (step) {
        case ST_RESET:
            if (!level && DURATION_DIFF(dur, TE_SHORT * 12) < TE_DELTA * 2) {
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
                if (DURATION_DIFF(te_last, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA &&
                    DURATION_DIFF(dur, (unsigned int)TE_LONG) < (unsigned int)TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (DURATION_DIFF(te_last, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA &&
                           DURATION_DIFF(dur, TE_SHORT * 8) < (unsigned int)TE_DELTA) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (DURATION_DIFF(dur, TE_SHORT * 12) < TE_DELTA * 2) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    if (bits == MIN_BITS) {
                        uint64_t k = data >> 2;
                        uint8_t sum = ((k >> 32) & 0xFF) + ((k >> 24) & 0xFF) +
                                      ((k >> 16) & 0xFF) + ((k >> 8) & 0xFF);
                        if (sum == (k & 0xFF)) {
                            out.key = k;
                            out.Bit = MIN_BITS;
                            out.te = TE_SHORT;
                            out.protocol = "Hollarm";
                            out.preset = "Ook270Async";
                            return true;
                        }
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

bool rf_encode_hollarm(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit != MIN_BITS) return false;
    uint64_t k = in.key;
    uint8_t sum = ((k >> 32) & 0xFF) + ((k >> 24) & 0xFF) +
                  ((k >> 16) & 0xFF) + ((k >> 8) & 0xFF);
    k = (k & 0xFFFFFFFFFF00ULL) | sum;
    for (int i = MIN_BITS - 1; i >= 0; i--) {
        out.push_back(TE_SHORT);
        if ((k >> i) & 1ULL)
            out.push_back(i == 0 ? -(TE_SHORT * 12) : -(TE_SHORT * 8));
        else
            out.push_back(i == 0 ? -(TE_SHORT * 12) : -(TE_LONG));
    }
    return true;
}
