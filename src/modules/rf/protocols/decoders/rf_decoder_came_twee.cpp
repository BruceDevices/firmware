// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_came_twee.h"
#include "../rf_config.h"

#define CT_TE_SHORT 500
#define CT_TE_LONG 1000
#define CT_TE_DELTA 250
#define CT_MIN_BITS 54

static const uint32_t ct_magic_xor[15] = {
    0x0E0E0E00, 0x1D1D1D11, 0x2C2C2C22, 0x3B3B3B33,
    0x4A4A4A44, 0x59595955, 0x68686866, 0x77777777,
    0x86868688, 0x95959599, 0xA4A4A4AA, 0xB3B3B3BB,
    0xC2C2C2CC, 0xD1D1D1DD, 0xE0E0E0EE,
};

static inline unsigned int ct_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

static uint16_t reverse_key_16(uint16_t data) {
    uint16_t rev = 0;
    for (int i = 0; i < 16; i++) {
        rev = (rev << 1) | ((data >> i) & 1U);
    }
    return rev;
}

typedef enum { ME_RESET, ME_LOW, ME_HIGH } manchester_state;

static bool manchester_advance(manchester_state& state, bool high_pulse, bool& bit) {
    switch (state) {
    case ME_RESET:
        state = high_pulse ? ME_HIGH : ME_LOW;
        return false;
    case ME_LOW:
        if (high_pulse) { state = ME_HIGH; return false; }
        bit = 1; state = ME_LOW; return true;
    case ME_HIGH:
        if (!high_pulse) { state = ME_LOW; return false; }
        bit = 0; state = ME_HIGH; return true;
    }
    return false;
}

static void manchester_reset(manchester_state& state) {
    state = ME_RESET;
}

bool rf_decode_came_twee(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 4) return false;

    enum { ST_RESET, ST_DATA } step = ST_RESET;
    uint64_t data = 0;
    int bits = 0;
    manchester_state man_state;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (!level && ct_diff(dur, CT_TE_LONG * 51) < CT_TE_DELTA * 20) {
                data = 0; bits = 0;
                manchester_reset(man_state);
                bool dummy = false;
                manchester_advance(man_state, false, dummy);
                manchester_advance(man_state, true, dummy);
                manchester_advance(man_state, false, dummy);
                step = ST_DATA;
            }
            break;

        case ST_DATA: {
            if (!level) {
                if (ct_diff(dur, CT_TE_SHORT) < CT_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, false, bit_out)) {
                        data = (data << 1) | (bit_out ? 0ULL : 1ULL);
                        bits++;
                    }
                } else if (ct_diff(dur, CT_TE_LONG) < CT_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, false, bit_out)) {
                        data = (data << 1) | (bit_out ? 0ULL : 1ULL);
                        bits++;
                    }
                } else if (dur >= (uint32_t)(CT_TE_LONG * 2 + CT_TE_DELTA)) {
                    if (bits == CT_MIN_BITS) {
                        uint8_t cnt_parcel = (uint8_t)(data & 0xF);
                        uint32_t d = (uint32_t)(data & 0x0FFFFFFFF);
                        d = d ^ ct_magic_xor[cnt_parcel];
                        uint32_t serial = d;
                        d /= 4;
                        uint8_t btn = (d >> 4) & 0x0F;
                        d >>= 16;
                        uint16_t dip = reverse_key_16((uint16_t)d);
                        uint16_t cnt = dip >> 6;

                        out.key = data;
                        out.Bit = CT_MIN_BITS;
                        out.te = CT_TE_SHORT;
                        out.protocol = "CAME_Twee";
                        out.preset = "Ook270Async";
                        out.serial = serial;
                        out.cnt = cnt;
                        out.btn = btn;
                        return true;
                    }
                    data = 0; bits = 0;
                    manchester_reset(man_state);
                    bool dummy = false;
                    manchester_advance(man_state, false, dummy);
                    manchester_advance(man_state, true, dummy);
                    manchester_advance(man_state, false, dummy);
                } else {
                    step = ST_RESET;
                }
            } else {
                if (ct_diff(dur, CT_TE_SHORT) < CT_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, true, bit_out)) {
                        data = (data << 1) | (bit_out ? 0ULL : 1ULL);
                        bits++;
                    }
                } else if (ct_diff(dur, CT_TE_LONG) < CT_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, true, bit_out)) {
                        data = (data << 1) | (bit_out ? 0ULL : 1ULL);
                        bits++;
                    }
                } else {
                    step = ST_RESET;
                }
            }
            break;
        }
        }
    }

    return false;
}

bool rf_encode_came_twee(const RfCodes& in, std::vector<int>& out) {
    if (in.Bit != CT_MIN_BITS) return false;

    for (int parcel = 14; parcel >= 0; parcel--) {
        uint64_t temp = 0x003FFF7200000000ULL |
                        ((uint64_t)(in.serial ^ ct_magic_xor[parcel]) & 0xFFFFFFFF);

        manchester_state enc_state;
        manchester_reset(enc_state);

        // Manchester encode
        uint8_t man_bits[200];
        int man_idx = 0;
        bool mid_bit = false;
        bool need_mid = false;

        for (int i = CT_MIN_BITS - 1; i >= 0; i--) {
            bool bit_val = !((temp >> i) & 1ULL);
            if (!need_mid) {
                if (bit_val) {
                    man_bits[man_idx++] = 0;
                    man_bits[man_idx++] = 1;
                } else {
                    man_bits[man_idx++] = 1;
                    man_bits[man_idx++] = 0;
                }
            }
            need_mid = !need_mid;
        }

        for (int i = 0; i < man_idx; i++) {
            if (man_bits[i]) {
                out.push_back(CT_TE_LONG);
            } else {
                out.push_back(CT_TE_SHORT);
            }
            // Add low duration
            if (i + 1 < man_idx) {
                out.push_back(-CT_TE_SHORT);
            } else {
                out.push_back(-CT_TE_LONG * 51);
            }
        }
    }
    return true;
}
