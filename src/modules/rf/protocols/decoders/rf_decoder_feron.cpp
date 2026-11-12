// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_feron.h"
#include "../rf_config.h"

#define FER_TE_SHORT 350
#define FER_TE_LONG 750
#define FER_TE_DELTA 150
#define FER_MIN_BITS 32

static inline unsigned int fer_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_feron(const std::vector<int>& durations, RfCodes& out) {
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
            if (!level && fer_diff(dur, FER_TE_LONG * 6) < FER_TE_DELTA * 4) {
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
                if (fer_diff(te_last, FER_TE_SHORT) < FER_TE_DELTA &&
                    fer_diff(dur, FER_TE_LONG) < FER_TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (fer_diff(te_last, FER_TE_LONG) < FER_TE_DELTA &&
                           fer_diff(dur, FER_TE_SHORT) < FER_TE_DELTA) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (fer_diff(dur, FER_TE_SHORT + 150) < FER_TE_DELTA) {
                    if (fer_diff(te_last, FER_TE_SHORT) < FER_TE_DELTA) {
                        data = (data << 1) | 0ULL;
                        bits++;
                    } else if (fer_diff(te_last, FER_TE_LONG) < FER_TE_DELTA) {
                        data = (data << 1) | 1ULL;
                        bits++;
                    }
                    if (bits == FER_MIN_BITS) {
                        out.key = data;
                        out.Bit = FER_MIN_BITS;
                        out.te = FER_TE_SHORT;
                        out.protocol = "Feron";
                        out.preset = "Ook270Async";
                        out.serial = (uint32_t)(data >> 16);
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

bool rf_encode_feron(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit != FER_MIN_BITS) return false;
    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(FER_TE_LONG);
            out.push_back(-(FER_TE_SHORT));
        } else {
            out.push_back(FER_TE_SHORT);
            out.push_back(-(FER_TE_LONG));
        }
    }
    out.push_back(FER_TE_SHORT + 150);
    out.push_back(-(FER_TE_LONG * 6));
    return true;
}
