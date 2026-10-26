// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_bett.h"
#include "../rf_config.h"

#define BETT_TE_SHORT 340
#define BETT_TE_LONG 2000
#define BETT_TE_DELTA 150
#define BETT_MIN_BITS 18

static inline unsigned int bett_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_bett(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 4) return false;

    enum { ST_RESET, ST_CHECK, ST_DATA } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (!level && bett_diff(dur, BETT_TE_SHORT * 44) < BETT_TE_DELTA * 15) {
                data = 0; bits = 0;
                step = ST_CHECK;
            }
            break;

        case ST_CHECK:
            if (level) {
                if (bett_diff(dur, BETT_TE_LONG) < BETT_TE_DELTA * 3) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    step = ST_DATA;
                } else if (bett_diff(dur, BETT_TE_SHORT) < BETT_TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_DATA;
                } else {
                    step = ST_RESET;
                }
            } else {
                step = ST_RESET;
            }
            break;

        case ST_DATA:
            if (!level) {
                if (bett_diff(dur, BETT_TE_SHORT * 44) < BETT_TE_DELTA * 15) {
                    if (bits == BETT_MIN_BITS) {
                        out.key = data;
                        out.Bit = BETT_MIN_BITS;
                        out.te = BETT_TE_SHORT;
                        out.protocol = "Bett";
                        out.preset = "Ook270Async";
                        return true;
                    }
                    data = 0; bits = 0;
                    step = ST_CHECK;
                } else if (bett_diff(dur, BETT_TE_SHORT) < BETT_TE_DELTA ||
                           bett_diff(dur, BETT_TE_LONG) < BETT_TE_DELTA * 3) {
                    step = ST_CHECK;
                } else {
                    step = ST_RESET;
                }
            }
            break;
        }
    }

    return false;
}

bool rf_encode_bett(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit != BETT_MIN_BITS) return false;
    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(BETT_TE_LONG);
            out.push_back(-BETT_TE_SHORT);
        } else {
            out.push_back(BETT_TE_SHORT);
            out.push_back(-BETT_TE_LONG);
        }
    }
    return true;
}
