// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_nero_radio.h"
#include "../rf_config.h"

#define NR_TE_SHORT 200
#define NR_TE_LONG 400
#define NR_TE_DELTA 80
#define NR_MIN_BITS 56
#define NR_HEADER_COUNT 49

static inline unsigned int nr_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_nero_radio(const std::vector<int>& durations, RfCodes& out) {
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
    int header_cnt = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (level && nr_diff(dur, NR_TE_SHORT) < NR_TE_DELTA) {
                te_last = dur;
                header_cnt = 0;
                step = ST_PREAMBLE;
            }
            break;

        case ST_PREAMBLE:
            if (level) {
                if (nr_diff(dur, NR_TE_SHORT) < NR_TE_DELTA ||
                    nr_diff(dur, NR_TE_SHORT * 4) < NR_TE_DELTA) {
                    te_last = dur;
                } else {
                    step = ST_RESET;
                }
            } else if (nr_diff(dur, NR_TE_SHORT) < NR_TE_DELTA) {
                if (nr_diff(te_last, NR_TE_SHORT) < NR_TE_DELTA) {
                    header_cnt++;
                } else if (nr_diff(te_last, NR_TE_SHORT * 4) < NR_TE_DELTA) {
                    if (header_cnt > 40) {
                        data = 0; bits = 0;
                        step = ST_SAVE;
                    } else {
                        step = ST_RESET;
                    }
                } else {
                    step = ST_RESET;
                }
            } else {
                step = ST_RESET;
            }
            break;

        case ST_SAVE:
            if (bits > NR_MIN_BITS) { step = ST_RESET; break; }
            if (level) {
                te_last = dur;
                step = ST_CHECK;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_CHECK:
            if (!level) {
                if (dur >= (unsigned int)(NR_TE_SHORT * 10 + NR_TE_DELTA * 2)) {
                    if (nr_diff(te_last, NR_TE_SHORT) < NR_TE_DELTA) {
                        data = (data << 1) | 0ULL;
                        bits++;
                    } else if (nr_diff(te_last, NR_TE_LONG) < NR_TE_DELTA) {
                        data = (data << 1) | 1ULL;
                        bits++;
                    }
                    if (bits == NR_MIN_BITS) {
                        out.key = data;
                        out.Bit = NR_MIN_BITS;
                        out.te = NR_TE_SHORT;
                        out.protocol = "Nero_Radio";
                        out.preset = "Ook270Async";
                        return true;
                    }
                    data = 0; bits = 0;
                    step = ST_RESET;
                } else if (nr_diff(te_last, NR_TE_SHORT) < NR_TE_DELTA &&
                           nr_diff(dur, NR_TE_LONG) < NR_TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (nr_diff(te_last, NR_TE_LONG) < NR_TE_DELTA &&
                           nr_diff(dur, NR_TE_SHORT) < NR_TE_DELTA) {
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

bool rf_encode_nero_radio(const RfCodes& in, std::vector<int>& out) {
    for (int i = 0; i < NR_HEADER_COUNT; i++) {
        out.push_back(NR_TE_SHORT);
        out.push_back(-NR_TE_SHORT);
    }
    out.push_back(NR_TE_SHORT * 4);
    out.push_back(-NR_TE_SHORT);

    for (int i = in.Bit - 1; i > 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(NR_TE_LONG);
            out.push_back(-NR_TE_SHORT);
        } else {
            out.push_back(NR_TE_SHORT);
            out.push_back(-NR_TE_LONG);
        }
    }
    if ((in.key >> 0) & 1ULL) {
        out.push_back(NR_TE_LONG);
        out.push_back(-(NR_TE_SHORT * 37));
    } else {
        out.push_back(NR_TE_SHORT);
        out.push_back(-(NR_TE_SHORT * 37));
    }
    return true;
}
