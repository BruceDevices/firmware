// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Port of Flipper Zero Marantec protocol decoder (GPL-3.0-or-later),
// lib/subghz/protocols/marantec.c — Manchester-encoded 49-bit garage door
// protocol with CRC-8 (poly 0x1D).
#include "rf_decoder_marantec.h"
#include "manchester_helpers.h"
#include "../rf_config.h"

#define MT_TE_SHORT 1000
#define MT_TE_LONG 2000
#define MT_TE_DELTA 200
#define MT_MIN_BITS 49

static inline unsigned int mt_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

static uint8_t mt_crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0x01;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ 0x1D);
            else
                crc <<= 1;
        }
    }
    return crc;
}

bool rf_decode_marantec(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 20) return false;

    for (size_t start = 0; start + 1 < durations.size(); start++) {
        int raw = durations[start];
        if (raw > 0) continue;
        unsigned int dur = (unsigned int)(-raw);
        if (mt_diff(dur, MT_TE_LONG * 5) >= MT_TE_DELTA * 8) continue;

        ManchesterState ms;
        manchester_reset(ms);
        uint64_t data = 1;
        int bits = 1;

        for (size_t i = start + 1; i < durations.size(); i++) {
            bool level = durations[i] > 0;
            unsigned int d = (unsigned int)(durations[i] > 0 ? durations[i] : -durations[i]);

            if (!level && d >= (unsigned int)(MT_TE_LONG * 2 + MT_TE_DELTA)) {
                if (bits == MT_MIN_BITS) {
                    uint8_t tdata[6] = {
                        (uint8_t)(data >> 48), (uint8_t)(data >> 40),
                        (uint8_t)(data >> 32), (uint8_t)(data >> 24),
                        (uint8_t)(data >> 16), (uint8_t)(data >> 8)};
                    if (mt_crc8(tdata, 6) == (data & 0xFF)) {
                        out.key = data;
                        out.Bit = MT_MIN_BITS;
                        out.te = MT_TE_SHORT;
                        out.protocol = "Marantec";
                        out.preset = "Ook270Async";
                        return true;
                    }
                }
                break;
            }

            ManchesterEvent ev;
            if (level) {
                if (mt_diff(d, MT_TE_SHORT) < MT_TE_DELTA) ev = ManchesterEventShortHigh;
                else if (mt_diff(d, MT_TE_LONG) < MT_TE_DELTA) ev = ManchesterEventLongHigh;
                else { manchester_reset(ms); continue; }
            } else {
                if (mt_diff(d, MT_TE_SHORT) < MT_TE_DELTA) ev = ManchesterEventShortLow;
                else if (mt_diff(d, MT_TE_LONG) < MT_TE_DELTA) ev = ManchesterEventLongLow;
                else { manchester_reset(ms); continue; }
            }

            bool bit_val = false;
            if (manchester_advance(ms, ev, &bit_val)) {
                if (bits >= MT_MIN_BITS) break;
                data = (data << 1) | (bit_val ? 1ULL : 0ULL);
                bits++;
            }
        }
    }
    return false;
}

bool rf_encode_marantec(const RfCodes& in, std::vector<int>& out) {
    out.clear();
    out.push_back(-(MT_TE_LONG * 5));
    ManchesterState ms;
    manchester_reset(ms);
    bool bit_val = false;
    if (manchester_advance(ms, ManchesterEventShortHigh, &bit_val)) {
        out.push_back(MT_TE_SHORT);
        out.push_back(-MT_TE_LONG);
    } else {
        out.push_back(MT_TE_SHORT);
    }
    for (int i = in.Bit - 2; i >= 0; i--) {
        bool bit = (in.key >> (i + 1)) & 1ULL;
        if (manchester_advance(ms, bit ? ManchesterEventLongHigh : ManchesterEventShortHigh, &bit_val)) {
            if (bit_val) { out.push_back(MT_TE_LONG); out.push_back(-MT_TE_SHORT); }
            else { out.push_back(MT_TE_SHORT); out.push_back(-MT_TE_LONG); }
        } else {
            if (bit) { out.push_back(MT_TE_LONG); }
            else { out.push_back(MT_TE_SHORT); }
        }
    }
    out.push_back(-(MT_TE_LONG * 5));
    return true;
}
