// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Port of Flipper Zero Linear Delta-3 protocol decoder (GPL-3.0-or-later),
// lib/subghz/protocols/linear_delta3.c — Simple PWM 8-bit DIP switch protocol.
// te_short=500, bit1=short+7*short, bit0=long+long.
#include "rf_decoder_linear_delta3.h"
#include "../rf_config.h"

#define LD_TE_SHORT 500
#define LD_TE_LONG 2000
#define LD_TE_DELTA 150
#define LD_MIN_BITS 8

static inline unsigned int ld_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_linear_delta3(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 6) return false;

    enum { ST_RESET, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;
    uint64_t last_data = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (!level && ld_diff(dur, LD_TE_SHORT * 70) < LD_TE_DELTA * 24) {
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
                if (dur >= (unsigned int)(LD_TE_SHORT * 10)) {
                    if (ld_diff(te_last, LD_TE_SHORT) < LD_TE_DELTA)
                        data = (data << 1) | 1ULL;
                    else if (ld_diff(te_last, LD_TE_LONG) < LD_TE_DELTA)
                        data = (data << 1) | 0ULL;
                    else { step = ST_RESET; break; }
                    bits++;

                    if (bits == LD_MIN_BITS) {
                        if (last_data == data && last_data) {
                            out.key = data;
                            out.Bit = LD_MIN_BITS;
                            out.te = LD_TE_SHORT;
                            out.protocol = "Linear_Delta3";
                            out.preset = "Ook270Async";
                            return true;
                        }
                        last_data = data;
                        data = 0; bits = 0;
                    }
                    step = ST_SAVE;
                    break;
                }

                if ((ld_diff(te_last, LD_TE_SHORT) < LD_TE_DELTA &&
                     ld_diff(dur, LD_TE_SHORT * 7) < LD_TE_DELTA)) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    step = ST_SAVE;
                } else if ((ld_diff(te_last, LD_TE_LONG) < LD_TE_DELTA &&
                            ld_diff(dur, LD_TE_LONG) < LD_TE_DELTA)) {
                    data = (data << 1) | 0ULL;
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

bool rf_encode_linear_delta3(const RfCodes& in, std::vector<int>& out) {
    out.clear();
    out.push_back(-(LD_TE_SHORT * 70));
    for (int i = in.Bit - 1; i > 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(LD_TE_SHORT);
            out.push_back(-(LD_TE_SHORT * 7));
        } else {
            out.push_back(LD_TE_LONG);
            out.push_back(-LD_TE_LONG);
        }
    }
    if ((in.key >> 0) & 1ULL) {
        out.push_back(LD_TE_SHORT);
        out.push_back(-(LD_TE_SHORT * 73));
    } else {
        out.push_back(LD_TE_LONG);
        out.push_back(-(LD_TE_SHORT * 70));
    }
    return true;
}
