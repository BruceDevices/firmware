// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_elplast.h"
#include "../rf_config.h"

#define ELP_TE_SHORT 230
#define ELP_TE_LONG 1550
#define ELP_TE_DELTA 160
#define ELP_MIN_BITS 18
#define ELP_GAP (ELP_TE_LONG * 8)

static inline unsigned int elp_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_elplast(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 4) return false;

    enum { ST_RESET, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (!level && elp_diff(dur, ELP_GAP) < ELP_TE_DELTA * 13) {
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
                if (elp_diff(te_last, ELP_TE_LONG) < ELP_TE_DELTA &&
                    elp_diff(dur, ELP_TE_SHORT) < ELP_TE_DELTA) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (elp_diff(te_last, ELP_TE_SHORT) < ELP_TE_DELTA &&
                           elp_diff(dur, ELP_TE_LONG) < ELP_TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (elp_diff(dur, ELP_GAP) < ELP_TE_DELTA * 13) {
                    if (elp_diff(te_last, ELP_TE_LONG) < ELP_TE_DELTA) {
                        data = (data << 1) | 1ULL;
                        bits++;
                    } else if (elp_diff(te_last, ELP_TE_SHORT) < ELP_TE_DELTA) {
                        data = (data << 1) | 0ULL;
                        bits++;
                    }
                    if (bits == ELP_MIN_BITS) {
                        out.key = data;
                        out.Bit = ELP_MIN_BITS;
                        out.te = ELP_TE_SHORT;
                        out.protocol = "Elplast";
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

bool rf_encode_elplast(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit != ELP_MIN_BITS) return false;
    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(ELP_TE_LONG);
            out.push_back(-ELP_TE_SHORT);
        } else {
            out.push_back(ELP_TE_SHORT);
            out.push_back(-ELP_TE_LONG);
        }
    }
    out.push_back(-ELP_GAP);
    return true;
}
