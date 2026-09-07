// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_faac_slh.h"
#include "../rf_config.h"

#define FAAC_TE_SHORT 255
#define FAAC_TE_LONG 595
#define FAAC_TE_DELTA 100
#define FAAC_MIN_BITS 64

static inline unsigned int faac_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_faac_slh(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 4) return false;

    enum {
        ST_RESET,
        ST_PREAMBLE,
        ST_SAVE,
        ST_CHECK
    } step = ST_RESET;

    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (level && faac_diff(dur, FAAC_TE_LONG * 2) < FAAC_TE_DELTA * 3)
                step = ST_PREAMBLE;
            break;

        case ST_PREAMBLE:
            if (!level && faac_diff(dur, FAAC_TE_LONG * 2) < FAAC_TE_DELTA * 3) {
                data = 0;
                bits = 0;
                step = ST_SAVE;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_SAVE:
            if (bits > 64) { step = ST_RESET; break; }
            if (level) {
                if (dur >= (unsigned int)(FAAC_TE_SHORT * 3 + FAAC_TE_DELTA)) {
                    if (bits == FAAC_MIN_BITS) {
                        out.key = data;
                        out.Bit = FAAC_MIN_BITS;
                        out.te = FAAC_TE_SHORT;
                        out.protocol = "FAAC_SLH";
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
            if (!level) {
                if (faac_diff(te_last, FAAC_TE_SHORT) < FAAC_TE_DELTA &&
                    faac_diff(dur, FAAC_TE_LONG) < FAAC_TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (faac_diff(te_last, FAAC_TE_LONG) < FAAC_TE_DELTA &&
                           faac_diff(dur, FAAC_TE_SHORT) < FAAC_TE_DELTA) {
                    data = (data << 1) | 1ULL;
                    bits++;
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

    if (bits == FAAC_MIN_BITS) {
        out.key = data;
        out.Bit = FAAC_MIN_BITS;
        out.te = FAAC_TE_SHORT;
        out.protocol = "FAAC_SLH";
        out.preset = "Ook270Async";
        return true;
    }

    return false;
}

bool rf_encode_faac_slh(const RfCodes& in, std::vector<int>& out) {
    out.push_back(FAAC_TE_LONG * 2);
    out.push_back(-(FAAC_TE_LONG * 2));

    for (int i = 63; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(FAAC_TE_LONG);
            out.push_back(-FAAC_TE_SHORT);
        } else {
            out.push_back(FAAC_TE_SHORT);
            out.push_back(-FAAC_TE_LONG);
        }
    }
    return true;
}
