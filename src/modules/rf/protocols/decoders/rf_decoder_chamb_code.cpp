// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_chamb_code.h"
#include "../rf_config.h"

#define TE_SHORT 1000
#define TE_LONG 3000
#define TE_DELTA 200
#define MIN_BITS 10

#define BIT_STOP 0b0001
#define BIT_1    0b0011
#define BIT_0    0b0111

#define MASK_7   0xF000000FF0FULL
#define MASK_8   0xF00000F00FULL
#define MASK_9   0xF000000000FULL
#define MASK7_CHK 0x10000001101ULL
#define MASK8_CHK 0x1000001001ULL
#define MASK9_CHK 0x10000000001ULL

static inline int DURATION_DIFF(int a, int b) { return (a > b) ? (a - b) : (b - a); }

static bool chamb_code_to_bit(uint64_t& data, int size) {
    uint64_t tmp = data;
    data = 0;
    for (int i = 0; i < size; i++) {
        if ((tmp & 0xF) == BIT_0) {
        } else if ((tmp & 0xF) == BIT_1) {
            data |= (1ULL << i);
        } else {
            return false;
        }
        tmp >>= 4;
    }
    return true;
}

static bool chamb_check_mask(uint64_t& data, int& nbits) {
    if ((data & MASK_7) == MASK7_CHK) {
        nbits = 7;
        data &= ~MASK_7;
        data = ((data >> 12) & 0xF) | ((data >> 4) & 0xF0);
    } else if ((data & MASK_8) == MASK8_CHK) {
        nbits = 8;
        data &= ~MASK_8;
        data = (data >> 4) | (BIT_0 << 8);
    } else if ((data & MASK_9) == MASK9_CHK) {
        nbits = 9;
        data &= ~MASK_9;
        data >>= 4;
    } else {
        return false;
    }
    return chamb_code_to_bit(data, nbits);
}

bool rf_decode_chamb_code(const std::vector<int>& durations, RfCodes& out) {
    enum { ST_RESET, ST_START, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? raw : -raw;

        switch (step) {
        case ST_RESET:
            if (!level && DURATION_DIFF(dur, TE_SHORT * 39) < TE_DELTA * 20) {
                step = ST_START;
            }
            break;

        case ST_START:
            if (level && DURATION_DIFF(dur, (unsigned int)TE_SHORT) < (unsigned int)TE_DELTA) {
                data = 0; bits = 0;
                data = (data << 4) | BIT_STOP;
                bits++;
                step = ST_SAVE;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_SAVE:
            if (!level) {
                if (dur > (unsigned int)(TE_SHORT * 5)) {
                    int nbits = bits;
                    if (bits >= MIN_BITS && chamb_check_mask(data, nbits)) {
                        out.key = data;
                        out.Bit = nbits;
                        out.te = TE_SHORT;
                        out.protocol = "Chamb_Code";
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
            if (level) {
                if (DURATION_DIFF((int)te_last, TE_SHORT * 3) < TE_DELTA &&
                    DURATION_DIFF((int)dur, (int)TE_SHORT) < TE_DELTA) {
                    data = (data << 4) | BIT_STOP;
                    bits++;
                    step = ST_SAVE;
                } else if (DURATION_DIFF((int)te_last, TE_SHORT * 2) < TE_DELTA &&
                           DURATION_DIFF((int)dur, TE_SHORT * 2) < TE_DELTA) {
                    data = (data << 4) | BIT_1;
                    bits++;
                    step = ST_SAVE;
                } else if (DURATION_DIFF((int)te_last, (int)TE_SHORT) < TE_DELTA &&
                           DURATION_DIFF((int)dur, TE_SHORT * 3) < TE_DELTA) {
                    data = (data << 4) | BIT_0;
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
