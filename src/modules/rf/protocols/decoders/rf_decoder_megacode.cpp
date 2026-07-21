// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_megacode.h"
#include "../rf_config.h"

#define TE_SHORT 1000
#define TE_DELTA 200
#define MIN_BITS 24

static inline int DURATION_DIFF(int a, int b) { return (a > b) ? (a - b) : (b - a); }

bool rf_decode_megacode(const std::vector<int>& durations, RfCodes& out) {
    enum { ST_RESET, ST_START, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    int last_bit = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? raw : -raw;

        switch (step) {
        case ST_RESET:
            if (!level && DURATION_DIFF(dur, TE_SHORT * 13) < TE_DELTA * 17) {
                step = ST_START;
            }
            break;

        case ST_START:
            if (level && DURATION_DIFF(dur, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA) {
                data = 0; bits = 0;
                data = (data << 1) | 1ULL;
                bits++;
                last_bit = 1;
                step = ST_SAVE;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_SAVE:
            if (!level) {
                if (dur >= (unsigned int)(TE_SHORT * 10)) {
                    if (bits == MIN_BITS) {
                        out.key = data;
                        out.Bit = MIN_BITS;
                        out.te = TE_SHORT;
                        out.protocol = "MegaCode";
                        out.preset = "Ook270Async";
                        return true;
                    }
                    step = ST_RESET;
                } else {
                    if (!last_bit)
                        te_last = dur - TE_SHORT * 3;
                    else
                        te_last = dur;
                    step = ST_CHECK;
                }
            } else {
                step = ST_RESET;
            }
            break;

        case ST_CHECK:
            if (level) {
                if (DURATION_DIFF((int)te_last, TE_SHORT * 5) < TE_DELTA * 5 &&
                    DURATION_DIFF((int)dur, (int)TE_SHORT) < TE_DELTA) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    last_bit = 1;
                    step = ST_SAVE;
                } else if (DURATION_DIFF((int)te_last, TE_SHORT * 2) < TE_DELTA * 2 &&
                           DURATION_DIFF((int)dur, (int)TE_SHORT) < TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    last_bit = 0;
                    step = ST_SAVE;
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

bool rf_encode_megacode(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit != MIN_BITS) return false;
    int last_bit = (in.key >> 0) & 1ULL;
    out.resize(in.Bit * 2);
    int idx = in.Bit * 2 - 1;
    out[idx--] = TE_SHORT;
    for (int i = 1; i < in.Bit; i++) {
        if ((in.key >> i) & 1ULL) {
            out[idx--] = -(last_bit ? TE_SHORT * 5 : TE_SHORT * 2);
            last_bit = 1;
        } else {
            out[idx--] = -(last_bit ? TE_SHORT * 8 : TE_SHORT * 5);
            last_bit = 0;
        }
        out[idx--] = TE_SHORT;
    }
    out.insert(out.begin(), last_bit ? -(TE_SHORT * 11) : -(TE_SHORT * 14));
    return true;
}
