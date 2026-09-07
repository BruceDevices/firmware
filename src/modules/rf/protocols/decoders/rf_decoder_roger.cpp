// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_roger.h"
#include "../rf_config.h"

#define ROG_TE_SHORT 500
#define ROG_TE_LONG 1000
#define ROG_TE_DELTA 270
#define ROG_MIN_BITS 28
#define ROG_GAP (ROG_TE_SHORT * 19)

static inline unsigned int rog_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_roger(const std::vector<int>& durations, RfCodes& out) {
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
            if (!level && rog_diff(dur, ROG_GAP) < ROG_TE_DELTA * 5) {
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
                if (rog_diff(te_last, ROG_TE_LONG) < ROG_TE_DELTA &&
                    rog_diff(dur, ROG_TE_SHORT) < ROG_TE_DELTA) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (rog_diff(te_last, ROG_TE_SHORT) < ROG_TE_DELTA &&
                           rog_diff(dur, ROG_TE_LONG) < ROG_TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (rog_diff(dur, ROG_GAP) < ROG_TE_DELTA * 5) {
                    if (rog_diff(te_last, ROG_TE_LONG) < ROG_TE_DELTA) {
                        data = (data << 1) | 1ULL;
                        bits++;
                    } else if (rog_diff(te_last, ROG_TE_SHORT) < ROG_TE_DELTA) {
                        data = (data << 1) | 0ULL;
                        bits++;
                    }
                    if (bits == ROG_MIN_BITS) {
                        out.key = data;
                        out.Bit = ROG_MIN_BITS;
                        out.te = ROG_TE_SHORT;
                        out.protocol = "Roger";
                        out.preset = "Ook270Async";
                        out.serial = (uint32_t)(data >> 12);
                        out.btn = (uint8_t)((data >> 8) & 0xF);
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

bool rf_encode_roger(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit != ROG_MIN_BITS) return false;
    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(ROG_TE_LONG);
            out.push_back(-ROG_TE_SHORT);
        } else {
            out.push_back(ROG_TE_SHORT);
            out.push_back(-ROG_TE_LONG);
        }
    }
    out.push_back(-ROG_GAP);
    return true;
}
