// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Port of Flipper Zero SMC5326/AX5326 protocol decoder (GPL-3.0-or-later),
// lib/subghz/protocols/smc5326.c — PWM 25-bit rolling/static code with
// auto-calibrated TE and DIP-switch encoding (2 bits per switch: +/o/-).
#include "rf_decoder_smc5326.h"
#include "../rf_config.h"

#define SM_TE_SHORT 300
#define SM_TE_LONG 900
#define SM_TE_DELTA 200
#define SM_MIN_BITS 25

static inline unsigned int sm_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_smc5326(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 10) return false;

    enum { ST_RESET, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;
    uint32_t te_accum = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (!level && sm_diff(dur, SM_TE_SHORT * 24) < SM_TE_DELTA * 12) {
                data = 0; bits = 0; te_accum = 0;
                step = ST_SAVE;
            }
            break;

        case ST_SAVE:
            if (level) {
                te_last = dur;
                te_accum += dur;
                step = ST_CHECK;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_CHECK:
            if (!level) {
                if (dur >= (unsigned int)(SM_TE_LONG * 2)) {
                    te_accum += dur;
                    if (bits == SM_MIN_BITS) {
                        te_accum /= (SM_MIN_BITS * 4 + 1);
                        out.key = data;
                        out.Bit = SM_MIN_BITS;
                        out.te = (int)te_accum;
                        out.protocol = "SMC5326";
                        out.preset = "Ook270Async";
                        return true;
                    }
                    data = 0; bits = 0; te_accum = 0;
                    step = ST_SAVE;
                    break;
                }
                te_accum += dur;
                if (sm_diff(te_last, SM_TE_SHORT) < SM_TE_DELTA &&
                    sm_diff(dur, SM_TE_LONG) < SM_TE_DELTA * 3) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (sm_diff(te_last, SM_TE_LONG) < SM_TE_DELTA * 3 &&
                           sm_diff(dur, SM_TE_SHORT) < SM_TE_DELTA) {
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

bool rf_encode_smc5326(const RfCodes& in, std::vector<int>& out) {
    out.clear();
    out.push_back(-(SM_TE_SHORT * 24));
    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(SM_TE_LONG);
            out.push_back(-SM_TE_SHORT);
        } else {
            out.push_back(SM_TE_SHORT);
            out.push_back(-SM_TE_LONG);
        }
    }
    out.push_back(SM_TE_SHORT);
    out.push_back(-(SM_TE_SHORT * 25));
    return true;
}
