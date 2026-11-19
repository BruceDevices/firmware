// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_ido.h"
#include "../rf_config.h"

#define IDO_TE_SHORT 450
#define IDO_TE_LONG 1450
#define IDO_TE_DELTA 150
#define IDO_MIN_BITS 48

static inline unsigned int ido_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

static uint64_t reverse_key(uint64_t data, int bits) {
    uint64_t rev = 0;
    for (int i = 0; i < bits; i++) {
        rev = (rev << 1) | ((data >> i) & 1ULL);
    }
    return rev;
}

bool rf_decode_ido(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 4) return false;

    enum {
        ST_RESET,
        ST_PREAMBLE_H,
        ST_SAVE,
        ST_CHECK
    } step = ST_RESET;

    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (level && ido_diff(dur, IDO_TE_SHORT * 10) < IDO_TE_DELTA * 5)
                step = ST_PREAMBLE_H;
            break;

        case ST_PREAMBLE_H:
            if (!level && ido_diff(dur, IDO_TE_SHORT * 10) < IDO_TE_DELTA * 5) {
                data = 0; bits = 0;
                step = ST_SAVE;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_SAVE:
            if (bits > IDO_MIN_BITS) { step = ST_RESET; break; }
            if (level) {
                if (dur >= (unsigned int)(IDO_TE_SHORT * 5 + IDO_TE_DELTA)) {
                    if (bits >= IDO_MIN_BITS) {
                        uint64_t rev = reverse_key(data, bits);
                        uint32_t fix = (uint32_t)(rev & 0xFFFFFF);
                        out.key = data;
                        out.Bit = bits;
                        out.te = IDO_TE_SHORT;
                        out.protocol = "IDO";
                        out.preset = "Ook270Async";
                        out.serial = fix & 0xFFFFF;
                        out.btn = (fix >> 20) & 0x0F;
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
                if (ido_diff(te_last, IDO_TE_SHORT) < IDO_TE_DELTA &&
                    ido_diff(dur, IDO_TE_LONG) < IDO_TE_DELTA * 3) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (ido_diff(te_last, IDO_TE_SHORT) < IDO_TE_DELTA * 3 &&
                           ido_diff(dur, IDO_TE_SHORT) < IDO_TE_DELTA) {
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

bool rf_encode_ido(const RfCodes& in, std::vector<int>& out) {
    out.push_back(IDO_TE_SHORT * 10);
    out.push_back(-(IDO_TE_SHORT * 10));

    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(IDO_TE_SHORT);
            out.push_back(-IDO_TE_SHORT);
        } else {
            out.push_back(IDO_TE_SHORT);
            out.push_back(-IDO_TE_LONG);
        }
    }
    return true;
}
