// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_magellan.h"
#include "../rf_config.h"

#define TE_SHORT 200
#define TE_LONG 400
#define TE_DELTA 100
#define MIN_BITS 32

static inline int DURATION_DIFF(int a, int b) { return (a > b) ? (a - b) : (b - a); }

static uint8_t magellan_crc8(uint8_t* data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}

bool rf_decode_magellan(const std::vector<int>& durations, RfCodes& out) {
    enum { ST_RESET, ST_HEADER, ST_PREAMBLE, ST_SAVE, ST_CHECK } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    int header_count = 0;
    unsigned int te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = raw > 0 ? raw : -raw;

        switch (step) {
        case ST_RESET:
            if (level && DURATION_DIFF(dur, TE_SHORT) < TE_DELTA) {
                te_last = dur;
                header_count = 0;
                step = ST_HEADER;
            }
            break;

        case ST_HEADER:
            if (level) {
                te_last = dur;
            } else {
                if (DURATION_DIFF(te_last, TE_SHORT) < TE_DELTA &&
                    DURATION_DIFF(dur, TE_SHORT) < TE_DELTA) {
                    header_count++;
                } else if (DURATION_DIFF(te_last, TE_SHORT) < TE_DELTA &&
                           DURATION_DIFF(dur, TE_LONG) < TE_DELTA * 2 &&
                           header_count > 10) {
                    step = ST_PREAMBLE;
                } else {
                    step = ST_RESET;
                }
            }
            break;

        case ST_PREAMBLE:
            if (level) {
                te_last = dur;
            } else {
                if (DURATION_DIFF(te_last, TE_SHORT * 6) < TE_DELTA * 3 &&
                    DURATION_DIFF(dur, TE_LONG) < TE_DELTA * 2) {
                    data = 0; bits = 0;
                    step = ST_SAVE;
                } else {
                    step = ST_RESET;
                }
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
                if (DURATION_DIFF(te_last, TE_SHORT) < TE_DELTA &&
                    DURATION_DIFF(dur, TE_LONG) < TE_DELTA) {
                    data = (data << 1) | 1ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (DURATION_DIFF(te_last, TE_LONG) < TE_DELTA &&
                           DURATION_DIFF(dur, TE_SHORT) < TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (dur >= (unsigned int)(TE_LONG * 3)) {
                    if (bits == MIN_BITS) {
                        uint8_t cdata[3] = {
                            (uint8_t)(data >> 24),
                            (uint8_t)(data >> 16),
                            (uint8_t)(data >> 8)};
                        if ((data & 0xFF) == magellan_crc8(cdata, 3)) {
                            out.key = data;
                            out.Bit = MIN_BITS;
                            out.te = TE_SHORT;
                            out.protocol = "Magellan";
                            out.preset = "Ook270Async";
                            return true;
                        }
                    }
                    data = 0; bits = 0;
                    step = ST_RESET;
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

bool rf_encode_magellan(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit != MIN_BITS) return false;
    out.push_back(TE_SHORT * 4);
    out.push_back(-TE_SHORT);
    for (int i = 0; i < 12; i++) {
        out.push_back(TE_SHORT);
        out.push_back(-TE_SHORT);
    }
    out.push_back(TE_SHORT);
    out.push_back(-TE_LONG);
    out.push_back(TE_LONG * 3);
    out.push_back(-TE_LONG);
    for (int i = in.Bit - 1; i >= 0; i--) {
        if ((in.key >> i) & 1ULL) {
            out.push_back(TE_SHORT);
            out.push_back(-TE_LONG);
        } else {
            out.push_back(TE_LONG);
            out.push_back(-TE_SHORT);
        }
    }
    out.push_back(TE_SHORT);
    out.push_back(-(TE_LONG * 100));
    return true;
}
