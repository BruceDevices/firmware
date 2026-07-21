// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_nero_sketch.h"
#include "../rf_config.h"

#define NS_TE_SHORT 330
#define NS_TE_LONG 660
#define NS_TE_DELTA 150
#define NS_MIN_BITS 40
#define NS_HEADER_COUNT 47

static inline unsigned int ns_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

bool rf_decode_nero_sketch(const std::vector<int>& durations, RfCodes& out) {
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
            if (level && ns_diff(dur, NS_TE_SHORT) < NS_TE_DELTA) {
                te_last = dur;
                header_cnt = 0;
                step = ST_PREAMBLE;
            }
            break;

        case ST_PREAMBLE:
            if (level) {
                if (ns_diff(dur, NS_TE_SHORT) < NS_TE_DELTA ||
                    ns_diff(dur, NS_TE_SHORT * 4) < NS_TE_DELTA) {
                    te_last = dur;
                } else {
                    step = ST_RESET;
                }
            } else if (ns_diff(dur, NS_TE_SHORT) < NS_TE_DELTA) {
                if (ns_diff(te_last, NS_TE_SHORT) < NS_TE_DELTA) {
                    header_cnt++;
                } else if (ns_diff(te_last, NS_TE_SHORT * 4) < NS_TE_DELTA) {
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
            if (bits > NS_MIN_BITS) { step = ST_RESET; break; }
            if (level) {
                if (dur >= (unsigned int)(NS_TE_SHORT * 2 + NS_TE_DELTA * 2)) {
                    if (bits == NS_MIN_BITS) {
                        out.key = data;
                        out.Bit = NS_MIN_BITS;
                        out.te = NS_TE_SHORT;
                        out.protocol = "Nero_Sketch";
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
                if (ns_diff(te_last, NS_TE_SHORT) < NS_TE_DELTA &&
                    ns_diff(dur, NS_TE_LONG) < NS_TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (ns_diff(te_last, NS_TE_LONG) < NS_TE_DELTA &&
                           ns_diff(dur, NS_TE_SHORT) < NS_TE_DELTA) {
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

bool rf_encode_nero_sketch(const RfCodes& in, std::vector<int>& out) {
    for (int i = 0; i < NS_HEADER_COUNT; i++) {
        out.push_back(NS_TE_SHORT);
        out.push_back(-NS_TE_SHORT);
    }
    out.push_back(NS_TE_SHORT * 4);
    out.push_back(-NS_TE_SHORT);

    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(NS_TE_LONG);
            out.push_back(-NS_TE_SHORT);
        } else {
            out.push_back(NS_TE_SHORT);
            out.push_back(-NS_TE_LONG);
        }
    }

    out.push_back(NS_TE_SHORT * 3);
    out.push_back(-NS_TE_SHORT);
    return true;
}
