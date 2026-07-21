// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). PowerSmart decoder ported from
// Flipper Zero firmware (GPL-3.0-or-later), lib/subghz/protocols/power_smart.c.
#include "rf_decoder_power_smart.h"
#include "manchester_helpers.h"
#include "../rf_config.h"

#define PS_TE_SHORT 225
#define PS_TE_LONG 450
#define PS_TE_DELTA 100
#define PS_MIN_BITS 64

// Sync word: 0xFDxxxxxxxxAAxxxxxxxx
#define PS_HEADER 0xFD000000AA000000ULL
#define PS_HEADER_MASK 0xFF000000FF000000ULL

static bool ps_check_valid(uint64_t packet) {
    uint32_t data_1 = (uint32_t)((packet >> 40) & 0xFFFF);
    uint32_t data_2 = (uint32_t)((~packet >> 8) & 0xFFFF);
    uint8_t data_3 = (uint8_t)(packet >> 32) & 0xFF;
    uint8_t data_4 = (uint8_t)(((~packet) & 0xFF) - 1);
    return (data_1 == data_2) && (data_3 == data_4);
}

bool rf_decode_power_smart(const std::vector<int>& durations, RfCodes& out) {
    ManchesterState ms;
    manchester_reset(ms);

    uint64_t data = 0;
    int bits = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        ManchesterEvent ev = manchester_event_for(level, dur, PS_TE_SHORT, PS_TE_LONG, PS_TE_DELTA);
        if (ev != ManchesterEventReset) {
            bool bit_val = false;
            if (manchester_advance(ms, ev, &bit_val)) {
                data = (data << 1) | !bit_val;
                bits++;
            }
        } else {
            manchester_reset(ms);
            data = 0;
            bits = 0;
        }

        if (bits >= PS_MIN_BITS) {
            if ((data & PS_HEADER_MASK) == PS_HEADER && ps_check_valid(data)) {
                out.key = data;
                out.Bit = PS_MIN_BITS;
                out.te = PS_TE_SHORT;
                out.protocol = "PowerSmart";
                out.preset = "Ook270Async";
                return true;
            }
            data = 0;
            bits = 0;
        }
    }
    return false;
}

bool rf_encode_power_smart(const RfCodes& in, std::vector<int>& out) {
    out.clear();
    out.reserve(in.Bit * 2 + 4);

    // Use Flipper-style Manchester encoder: each data bit produces a
    // (short,long) or (long,short) HIGH/LOW pair. The data bits are
    // inverted before Manchester encoding.
    for (int i = in.Bit - 1; i >= 0; i--) {
        bool bit_val = !((in.key >> i) & 1ULL);
        if (!bit_val) { // Manchester 0: ShortHigh + LongLow
            out.push_back(PS_TE_SHORT);
            out.push_back(-PS_TE_LONG);
        } else { // Manchester 1: LongHigh + ShortLow
            out.push_back(PS_TE_LONG);
            out.push_back(-PS_TE_SHORT);
        }
    }
    // Trailing gap
    out.push_back(-(PS_TE_LONG * 1111));
    return true;
}
