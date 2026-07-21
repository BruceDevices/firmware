// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_came_atomo.h"
#include "../rf_config.h"

#define CA_TE_SHORT 600
#define CA_TE_LONG 1200
#define CA_TE_DELTA 250
#define CA_MIN_BITS 62

static inline unsigned int ca_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
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

static const uint64_t ca_default_xor[32] = {
    0x1fafef3ed0f7d9efULL, 0x185fcc1531ee86e7ULL, 0x184fa96912c567ffULL, 0x187f8a42f3dc38f7ULL,
    0x186f63915492a5cdULL, 0x181f40bab58bfac5ULL, 0x180f25c696a01bddULL, 0x183f06ed77b944d5ULL,
    0x182ef661d83d21a9ULL, 0x18ded54a39247ea1ULL, 0x18ceb0361a0f9fb9ULL, 0x18fe931dfb16c0b1ULL,
    0x18ee7ace5c585d8bULL, 0x181e59e5bd410283ULL, 0x180e3c999e6ae39bULL, 0x183e1fb27f73bc93ULL,
    0x184fcc1531ee86e7ULL, 0x18bfef3ed0f7d9efULL, 0x18af8a42f3dc38f7ULL, 0x189fa96912c567ffULL,
    0x188f63915492a5cdULL, 0x187f40bab58bfac5ULL, 0x186f25c696a01bddULL, 0x185f06ed77b944d5ULL,
    0x182ef661d83d21a9ULL, 0x18ded54a39247ea1ULL, 0x18ceb0361a0f9fb9ULL, 0x18fe931dfb16c0b1ULL,
    0x18ee7ace5c585d8bULL, 0x181e59e5bd410283ULL, 0x180e3c999e6ae39bULL, 0x183e1fb27f73bc93ULL,
};

bool rf_decode_came_atomo(const std::vector<int>& durations, RfCodes& out) {
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
        if (!level && ca_diff(dur, CA_TE_LONG * 60) < CA_TE_DELTA * 40) {
            data = 0; bits = 1;
            manchester_reset(man_state);
            bool dummy = false;
            manchester_advance(man_state, false, dummy);
            step = ST_DATA;
        }
        break;

    case ST_DATA: {
        if (!level) {
            if (ca_diff(dur, CA_TE_SHORT) < CA_TE_DELTA) {
                bool bit_out = false;
                if (manchester_advance(man_state, false, bit_out)) {
                    data = (data << 1) | (bit_out ? 0ULL : 1ULL);
                    bits++;
                }
            } else if (ca_diff(dur, CA_TE_LONG) < CA_TE_DELTA) {
                bool bit_out = false;
                if (manchester_advance(man_state, false, bit_out)) {
                    data = (data << 1) | (bit_out ? 0ULL : 1ULL);
                    bits++;
                }
            } else if (dur >= (uint32_t)(CA_TE_LONG * 2 + CA_TE_DELTA)) {
                if (bits == CA_MIN_BITS) {
                        uint16_t parcel_counter = (uint16_t)(data >> 48);
                        parcel_counter = parcel_counter ^ 0x185F;
                        parcel_counter >>= 4;
                        uint8_t ind = (parcel_counter + 1) % 32;
                        uint64_t temp_data = data & 0x0000FFFFFFFFFFFFULL;
                        uint64_t magic = ca_default_xor[ind];

                        temp_data = temp_data ^ magic;
                        uint32_t cnt = (uint32_t)(temp_data >> 36);
                        uint32_t serial = (uint32_t)((temp_data >> 4) & 0x000FFFFFFFFULL);
                        uint8_t btn = (uint8_t)(temp_data & 0xF);

                        out.key = data;
                        out.Bit = CA_MIN_BITS;
                        out.te = CA_TE_SHORT;
                        out.protocol = "CAME_Atomo";
                        out.preset = "Ook270Async";
                        out.serial = serial;
                        out.cnt = cnt;
                        out.btn = btn;
                        return true;
                    }
                    data = 0; bits = 1;
                    manchester_reset(man_state);
                    bool dummy = false;
                    manchester_advance(man_state, false, dummy);
                } else {
                    step = ST_RESET;
                }
            } else {
                if (ca_diff(dur, CA_TE_SHORT) < CA_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, true, bit_out)) {
                        data = (data << 1) | (bit_out ? 0ULL : 1ULL);
                        bits++;
                    }
                } else if (ca_diff(dur, CA_TE_LONG) < CA_TE_DELTA) {
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

bool rf_encode_came_atomo(const RfCodes& in, std::vector<int>& out) {
    (void)in; (void)out;
    return false;
}
