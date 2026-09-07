// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_dooya.h"
#include "../rf_config.h"

#define TE_SHORT 366
#define TE_LONG 733
#define TE_DELTA 120
#define MIN_BITS 40

static inline int DURATION_DIFF(int a, int b) { return (a > b) ? (a - b) : (b - a); }

bool rf_decode_dooya(const std::vector<int>& durations, RfCodes& out) {
    enum { ST_RESET, ST_START, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? raw : -raw;

        switch (step) {
        case ST_RESET:
            if (!level && DURATION_DIFF(dur, TE_LONG * 12) < TE_DELTA * 20) {
                step = ST_START;
            }
            break;

        case ST_START:
            if (!level) {
                if (DURATION_DIFF(dur, TE_LONG * 2) < TE_DELTA * 3) {
                    data = 0; bits = 0;
                    step = ST_SAVE;
                } else {
                    step = ST_RESET;
                }
            } else if (DURATION_DIFF(dur, TE_SHORT * 13) < TE_DELTA * 5) {
                break;
            } else {
                step = ST_RESET;
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
                if (dur >= (unsigned int)(TE_LONG * 4)) {
                    if (DURATION_DIFF(te_last, TE_SHORT) < TE_DELTA) {
                        data = (data << 1) | 0ULL;
                        bits++;
                    } else if (DURATION_DIFF(te_last, TE_LONG) < TE_DELTA * 2) {
                        data = (data << 1) | 1ULL;
                        bits++;
                    } else {
                        step = ST_RESET;
                        break;
                    }
                    if (bits == MIN_BITS) {
                        out.key = data;
                        out.Bit = MIN_BITS;
                        out.te = TE_SHORT;
                        out.protocol = "Dooya";
                        out.preset = "Ook270Async";
                        return true;
                    }
                    data = 0; bits = 0;
                    step = ST_START;
                } else if (DURATION_DIFF(te_last, TE_SHORT) < TE_DELTA &&
                           DURATION_DIFF(dur, TE_LONG) < TE_DELTA * 2) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (DURATION_DIFF(te_last, TE_LONG) < TE_DELTA * 2 &&
                           DURATION_DIFF(dur, TE_SHORT) < TE_DELTA) {
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

bool rf_encode_dooya(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit != MIN_BITS) return false;
    if ((in.key >> 0) & 1ULL) {
        out.push_back(-(TE_LONG * 12 + TE_LONG));
    } else {
        out.push_back(-(TE_LONG * 12 + TE_SHORT));
    }
    out.push_back(TE_SHORT * 13);
    out.push_back(-(TE_LONG * 2));
    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(TE_LONG);
            out.push_back(-TE_SHORT);
        } else {
            out.push_back(TE_SHORT);
            out.push_back(-TE_LONG);
        }
    }
    return true;
}
