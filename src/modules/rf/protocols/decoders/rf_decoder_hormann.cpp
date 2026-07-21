// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_hormann.h"
#include "../rf_config.h"

#define HORMANN_TE_SHORT 500
#define HORMANN_TE_LONG 1000
#define HORMANN_TE_DELTA 200
#define HORMANN_MIN_BITS 44
#define HORMANN_PATTERN 0xFF000000003ULL

static inline unsigned int hormann_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_hormann(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 4) return false;

    enum {
        ST_RESET,
        ST_START_H,
        ST_START_L,
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
            if (level && hormann_diff(dur, HORMANN_TE_SHORT * 24) < HORMANN_TE_DELTA * 24)
                step = ST_START_H;
            break;

        case ST_START_H:
            if (!level && hormann_diff(dur, HORMANN_TE_SHORT) < HORMANN_TE_DELTA) {
                data = 0; bits = 0;
                step = ST_SAVE;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_SAVE:
            if (bits > HORMANN_MIN_BITS) { step = ST_RESET; break; }
            if (level) {
                if (dur >= (unsigned int)(HORMANN_TE_SHORT * 5) &&
                    (data & HORMANN_PATTERN) == HORMANN_PATTERN) {
                    if (bits >= HORMANN_MIN_BITS) {
                        out.key = data;
                        out.Bit = bits;
                        out.te = HORMANN_TE_SHORT;
                        out.protocol = "Hormann_HSM";
                        out.preset = "Ook270Async";
                        out.btn = (data >> 8) & 0xF;
                        return true;
                    }
                    step = ST_START_L;
                    break;
                }
                te_last = dur;
                step = ST_CHECK;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_START_L:
            if (!level && hormann_diff(dur, HORMANN_TE_SHORT) < HORMANN_TE_DELTA) {
                data = 0; bits = 0;
                step = ST_SAVE;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_CHECK:
            if (!level) {
                if (hormann_diff(te_last, HORMANN_TE_SHORT) < HORMANN_TE_DELTA &&
                    hormann_diff(dur, HORMANN_TE_LONG) < HORMANN_TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (hormann_diff(te_last, HORMANN_TE_LONG) < HORMANN_TE_DELTA &&
                           hormann_diff(dur, HORMANN_TE_SHORT) < HORMANN_TE_DELTA) {
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

    return false;
}

bool rf_encode_hormann(const RfCodes& in, std::vector<int>& out) {
    for (int r = 0; r < 20; r++) {
        out.push_back(HORMANN_TE_SHORT * 24);
        out.push_back(-HORMANN_TE_SHORT);

        for (int i = in.Bit - 1; i >= 0; i--) {
            if ((in.key >> i) & 1ULL) {
                out.push_back(HORMANN_TE_LONG);
                out.push_back(-HORMANN_TE_SHORT);
            } else {
                out.push_back(HORMANN_TE_SHORT);
                out.push_back(-HORMANN_TE_LONG);
            }
        }
    }
    out.push_back(HORMANN_TE_SHORT * 24);
    return true;
}
