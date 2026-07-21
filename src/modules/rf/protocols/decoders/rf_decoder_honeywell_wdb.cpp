// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Port of Flipper Zero Honeywell Wireless Doorbell protocol decoder
// (GPL-3.0-or-later), lib/subghz/protocols/honeywell_wdb.c — PWM 48-bit
// protocol with parity LSB. te_short=160, te_long=320.
// https://github.com/klohner/honeywell-wireless-doorbell
#include "rf_decoder_honeywell_wdb.h"
#include "../rf_config.h"

#define HW_TE_SHORT 160
#define HW_TE_LONG 320
#define HW_TE_DELTA 60
#define HW_MIN_BITS 48

static inline unsigned int hw_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

static unsigned int hw_parity(uint64_t data, int bits) {
    unsigned int p = 0;
    for (int i = 0; i < bits; i++) {
        if ((data >> i) & 1ULL) p++;
    }
    return p & 1;
}

bool rf_decode_honeywell_wdb(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 10) return false;

    enum { ST_RESET, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (!level && hw_diff(dur, HW_TE_SHORT * 3) < HW_TE_DELTA) {
                data = 0; bits = 0;
                step = ST_SAVE;
            }
            break;

        case ST_SAVE:
            if (level) {
                if (hw_diff(dur, HW_TE_SHORT * 3) < HW_TE_DELTA) {
                    if (bits == HW_MIN_BITS && (data & 1) == hw_parity(data >> 1, HW_MIN_BITS - 1)) {
                        out.key = data;
                        out.Bit = HW_MIN_BITS;
                        out.te = HW_TE_SHORT;
                        out.protocol = "Honeywell_WDB";
                        out.preset = "Ook270Async";
                        return true;
                    }
                    step = ST_RESET;
                    break;
                }
                te_last = dur;
                step = ST_CHECK;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_CHECK:
            if (!level) {
                if ((hw_diff(te_last, HW_TE_SHORT) < HW_TE_DELTA &&
                     hw_diff(dur, HW_TE_LONG) < HW_TE_DELTA)) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if ((hw_diff(te_last, HW_TE_LONG) < HW_TE_DELTA &&
                            hw_diff(dur, HW_TE_SHORT) < HW_TE_DELTA)) {
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

bool rf_encode_honeywell_wdb(const RfCodes& in, std::vector<int>& out) {
    out.clear();
    out.push_back(-(HW_TE_SHORT * 3));
    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(HW_TE_LONG);
            out.push_back(-HW_TE_SHORT);
        } else {
            out.push_back(HW_TE_SHORT);
            out.push_back(-HW_TE_LONG);
        }
    }
    out.push_back(HW_TE_SHORT * 3);
    return true;
}
