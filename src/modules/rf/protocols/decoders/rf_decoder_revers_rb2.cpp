// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_revers_rb2.h"
#include "../rf_config.h"
#include "manchester_helpers.h"

#define TE_SHORT 250
#define TE_LONG 500
#define TE_DELTA 160
#define MIN_BITS 64

static inline int DURATION_DIFF(int a, int b) { return (a > b) ? (a - b) : (b - a); }

bool rf_decode_revers_rb2(const std::vector<int>& durations, RfCodes& out) {
    enum { ST_RESET, ST_HEADER, ST_DATA } step = ST_RESET;
    uint64_t data = 0xF;
    int bits = 4;
    int header_count = 0;
    unsigned int te_last = 0;
    ManchesterState manchester;
    manchester_reset(manchester);

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? raw : -raw;

        switch (step) {
        case ST_RESET:
            if (!level && DURATION_DIFF(dur, 600) < TE_DELTA) {
                manchester_reset(manchester);
                step = ST_HEADER;
                header_count = 0;
                te_last = 0;
            }
            break;

        case ST_HEADER:
            if (!level) {
                if (DURATION_DIFF(dur, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA) {
                    if (te_last == 1) header_count++;
                    te_last = 0;
                } else {
                    header_count = 0; te_last = 0;
                    step = ST_RESET;
                }
            } else {
                if (DURATION_DIFF(dur, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA) {
                    if (te_last == 0) header_count++;
                    te_last = 1;
                } else {
                    header_count = 0; te_last = 0;
                    step = ST_RESET;
                }
            }
            if (header_count == 4) {
                header_count = 0;
                data = 0xF;
                bits = 4;
                step = ST_DATA;
            }
            break;

        case ST_DATA: {
            ManchesterEvent event = manchester_event_for(level, dur, TE_SHORT, TE_LONG, TE_DELTA);
            if (event == ManchesterEventReset) {
                step = ST_RESET;
                break;
            }
            bool d;
            if (manchester_advance(manchester, event, &d)) {
                data = (data << 1) | (d ? 1ULL : 0ULL);
                bits++;
                if (bits > 65) { data = 0; bits = 0; step = ST_RESET; break; }
                if (bits >= MIN_BITS) {
                    uint16_t preamble = (data >> 48) & 0xFF;
                    uint16_t stop_code = data & 0x3FF;
                    if (preamble == 0xFF && stop_code == 0x200) {
                        out.key = data;
                        out.Bit = bits;
                        out.te = TE_SHORT;
                        out.protocol = "Revers_RB2";
                        out.preset = "Ook270Async";
                        return true;
                    }
                }
            }
            break;
        }
        }
    }
    return false;
}

bool rf_encode_revers_rb2(const RfCodes& in, std::vector<int>& out) {
    (void)in; (void)out;
    return false;
}
