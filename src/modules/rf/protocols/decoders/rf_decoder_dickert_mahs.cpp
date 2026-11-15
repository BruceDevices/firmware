// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_dickert_mahs.h"
#include "../rf_config.h"

#define TE_SHORT 400
#define TE_LONG 800
#define TE_DELTA 100
#define MIN_BITS 36

static inline int DURATION_DIFF(int a, int b) { return (a > b) ? (a - b) : (b - a); }

bool rf_decode_dickert_mahs(const std::vector<int>& durations, RfCodes& out) {
    enum { ST_RESET, ST_INIT, ST_REC } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    unsigned int tmp[2] = {0, 0};
    int tmp_cnt = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? raw : -raw;

        switch (step) {
        case ST_RESET:
            if (bits >= MIN_BITS) {
                out.key = data;
                out.Bit = bits;
                out.te = TE_SHORT;
                out.protocol = "Dickert_MAHS";
                out.preset = "Ook270Async";
                return true;
            }
            if (!level && DURATION_DIFF(dur, TE_LONG * 50) < TE_DELTA * 70) {
                step = ST_INIT;
            }
            break;

        case ST_INIT:
            if (!level) {
                break;
            } else if (DURATION_DIFF(dur, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA) {
                data = 0; bits = 0; tmp_cnt = 0;
                step = ST_REC;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_REC:
            if ((!level && tmp_cnt == 0) || (level && tmp_cnt == 1)) {
                tmp[tmp_cnt] = dur;
                tmp_cnt++;
                if (tmp_cnt == 2) {
                    if (DURATION_DIFF((int)(tmp[0] + tmp[1]), 1200) < TE_DELTA) {
                        if (DURATION_DIFF((int)tmp[0], (int)TE_LONG) < TE_DELTA)
                            { data = (data << 1) | 1ULL; bits++; }
                        else if (DURATION_DIFF((int)tmp[0], (int)TE_SHORT) < TE_DELTA)
                            { data = (data << 1) | 0ULL; bits++; }
                        tmp_cnt = 0;
                    } else {
                        tmp_cnt = 0;
                        step = ST_RESET;
                    }
                }
            } else {
                tmp_cnt = 0;
                step = ST_RESET;
            }
            break;
        }
    }
    return false;
}

bool rf_encode_dickert_mahs(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit < MIN_BITS) return false;
    out.push_back(-(TE_LONG * 50));
    out.push_back(TE_SHORT);
    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(-TE_LONG);
            out.push_back(TE_SHORT);
        } else {
            out.push_back(-TE_SHORT);
            out.push_back(TE_LONG);
        }
    }
    return true;
}
