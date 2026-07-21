// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_secplus_v2.h"
#include "../rf_config.h"

#define SP2_TE_SHORT 250
#define SP2_TE_LONG 500
#define SP2_TE_DELTA 110
#define SP2_MIN_BITS 62
#define SP2_HEADER_MASK 0xFFFF3C0000000000ULL
#define SP2_HEADER_VAL  0x00003C0000000000ULL
#define SP2_PACKET_MASK 0x30000000000ULL
#define SP2_PACKET_1    0x00000000000ULL
#define SP2_PACKET_2    0x10000000000ULL

static inline unsigned int sp2_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

static bool sp2_mix_invert(uint8_t invert, uint16_t p[3]) {
    switch (invert) {
    case 0x00: p[0] = ~p[0] & 0x03FF; p[1] = ~p[1] & 0x03FF; break;
    case 0x01: p[1] = ~p[1] & 0x03FF; break;
    case 0x02: p[2] = ~p[2] & 0x03FF; break;
    case 0x04: p[0] = ~p[0] & 0x03FF; p[1] = ~p[1] & 0x03FF; p[2] = ~p[2] & 0x03FF; break;
    case 0x05: case 0x0a: p[0] = ~p[0] & 0x03FF; p[2] = ~p[2] & 0x03FF; break;
    case 0x06: p[1] = ~p[1] & 0x03FF; p[2] = ~p[2] & 0x03FF; break;
    case 0x08: p[0] = ~p[0] & 0x03FF; break;
    case 0x09: break;
    default: return false;
    }
    return true;
}

static void sp2_mix_order_decode(uint8_t order, uint16_t p[3]) {
    uint16_t a = p[0], b = p[1], c = p[2];
    switch (order) {
    case 0x06: case 0x09: p[2] = a; p[0] = c; break;
    case 0x08: case 0x04: p[1] = a; p[2] = b; p[0] = c; break;
    case 0x01: p[2] = a; p[0] = b; p[1] = c; break;
    case 0x00: p[2] = b; p[1] = c; break;
    case 0x05: p[1] = a; p[0] = b; break;
    case 0x02: case 0x0A: break;
    }
}

static void sp2_mix_order_encode(uint8_t order, uint16_t p[3]) {
    uint16_t a = p[0], b = p[1], c = p[2];
    switch (order) {
    case 0x06: case 0x09: a = p[2]; b = p[1]; c = p[0]; break;
    case 0x08: case 0x04: a = p[1]; b = p[2]; c = p[0]; break;
    case 0x01: a = p[2]; b = p[0]; c = p[1]; break;
    case 0x00: a = p[0]; b = p[2]; c = p[1]; break;
    case 0x05: a = p[1]; b = p[0]; c = p[2]; break;
    case 0x02: case 0x0A: break;
    }
    p[0] = a; p[1] = b; p[2] = c;
}

static bool sp2_decode_half(uint64_t data, uint8_t roll_array[9], uint32_t& fixed) {
    uint8_t order = (data >> 34) & 0x0f;
    uint8_t invert = (data >> 30) & 0x0f;
    uint16_t p[3] = {0};

    for (int i = 29; i >= 0; i -= 3) {
        p[0] = (p[0] << 1) | ((data >> i) & 1ULL);
        p[1] = (p[1] << 1) | ((data >> (i - 1)) & 1ULL);
        p[2] = (p[2] << 1) | ((data >> (i - 2)) & 1ULL);
    }

    if (!sp2_mix_invert(invert, p)) return false;
    sp2_mix_order_decode(order, p);

    data = (uint64_t)order << 4 | invert;
    int k = 0;
    for (int i = 6; i >= 0; i -= 2) {
        roll_array[k] = (data >> i) & 0x03;
        if (roll_array[k++] == 3) return false;
    }
    for (int i = 8; i >= 0; i -= 2) {
        roll_array[k] = (p[2] >> i) & 0x03;
        if (roll_array[k++] == 3) return false;
    }

    fixed = (p[0] << 10) | p[1];
    return true;
}

static void sp2_remote_controller(uint64_t packet_1, uint64_t packet_2,
                                   uint32_t& serial, uint32_t& cnt, uint8_t& btn) {
    uint32_t fixed_1 = 0, fixed_2 = 0;
    uint8_t roll_1[9] = {0}, roll_2[9] = {0};
    uint8_t rolling_digits[18] = {0};

    if (!sp2_decode_half(packet_1, roll_1, fixed_1) ||
        !sp2_decode_half(packet_2, roll_2, fixed_2)) {
        cnt = 0; btn = 0; serial = 0;
        return;
    }

    rolling_digits[0] = roll_2[8];
    rolling_digits[1] = roll_1[8];
    rolling_digits[2] = roll_2[4];
    rolling_digits[3] = roll_2[5];
    rolling_digits[4] = roll_2[6];
    rolling_digits[5] = roll_2[7];
    rolling_digits[6] = roll_1[4];
    rolling_digits[7] = roll_1[5];
    rolling_digits[8] = roll_1[6];
    rolling_digits[9] = roll_1[7];
    rolling_digits[10] = roll_2[0];
    rolling_digits[11] = roll_2[1];
    rolling_digits[12] = roll_2[2];
    rolling_digits[13] = roll_2[3];
    rolling_digits[14] = roll_1[0];
    rolling_digits[15] = roll_1[1];
    rolling_digits[16] = roll_1[2];
    rolling_digits[17] = roll_1[3];

    uint32_t rolling = 0;
    for (int i = 0; i < 18; i++) {
        rolling = (rolling * 3) + rolling_digits[i];
    }

    if (rolling >= 0x10000000) {
        cnt = 0; btn = 0; serial = 0;
    } else {
        uint32_t rev = 0;
        for (int i = 0; i < 28; i++) {
            rev = (rev << 1) | ((rolling >> i) & 1U);
        }
        cnt = rev;
        btn = (fixed_1 >> 12) & 0xF;
        serial = (fixed_1 << 20) | fixed_2;
    }
}

static bool sp2_check_packet(uint64_t data, uint64_t& packet_1) {
    if ((data & SP2_HEADER_MASK) == SP2_HEADER_VAL) {
        if ((data & SP2_PACKET_MASK) == SP2_PACKET_1) {
            packet_1 = data;
        } else if (((data & SP2_PACKET_MASK) == SP2_PACKET_2) && packet_1) {
            return true;
        }
    }
    return false;
}

// Manchester helpers for decoding
typedef enum { ME_RESET, ME_LOW, ME_HIGH } manchester_state;

static bool manchester_advance(manchester_state& state, bool high_pulse, bool& bit) {
    switch (state) {
    case ME_RESET:
        state = high_pulse ? ME_HIGH : ME_LOW;
        return false;
    case ME_LOW:
        if (high_pulse) {
            state = ME_HIGH;
            return false;
        }
        bit = 1; state = ME_LOW;
        return true;
    case ME_HIGH:
        if (!high_pulse) {
            state = ME_LOW;
            return false;
        }
        bit = 0; state = ME_HIGH;
        return true;
    }
    return false;
}

static void manchester_reset(manchester_state& state) {
    state = ME_RESET;
}

bool rf_decode_secplus_v2(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 4) return false;

    enum { ST_RESET, ST_DATA } step = ST_RESET;

    uint64_t data = 0;
    int bits = 0;
    uint64_t packet_1 = 0;
    manchester_state man_state;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (!level && sp2_diff(dur, SP2_TE_LONG * 130) < SP2_TE_DELTA * 100) {
                data = 0; bits = 0; packet_1 = 0;
                manchester_reset(man_state);
                bool dummy = false;
                manchester_advance(man_state, true, dummy);  // prime with long high
                manchester_advance(man_state, false, dummy); // then short low
                step = ST_DATA;
            }
            break;

        case ST_DATA: {
            if (!level) {
                if (sp2_diff(dur, SP2_TE_SHORT) < SP2_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, false, bit_out)) {
                        data = (data << 1) | (bit_out ? 1ULL : 0ULL);
                        bits++;
                    }
                } else if (sp2_diff(dur, SP2_TE_LONG) < SP2_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, false, bit_out)) {
                        data = (data << 1) | (bit_out ? 1ULL : 0ULL);
                        bits++;
                    }
                } else if (dur >= (uint32_t)(SP2_TE_LONG * 2 + SP2_TE_DELTA)) {
                    if (bits == SP2_MIN_BITS) {
                        if (sp2_check_packet(data, packet_1)) {
                            uint32_t serial = 0, cnt = 0;
                            uint8_t btn = 0;
                            sp2_remote_controller(packet_1, data, serial, cnt, btn);
                            out.key = data;
                            out.Bit = SP2_MIN_BITS;
                            out.te = SP2_TE_SHORT;
                            out.protocol = "SecPlus_v2";
                            out.preset = "Ook270Async";
                            out.serial = serial;
                            out.cnt = cnt;
                            out.btn = btn;
                            return true;
                        }
                    }
                    data = 0; bits = 0;
                    manchester_reset(man_state);
                    bool dummy = false;
                    manchester_advance(man_state, true, dummy);
                    manchester_advance(man_state, false, dummy);
                } else {
                    step = ST_RESET;
                }
            } else {
                if (sp2_diff(dur, SP2_TE_SHORT) < SP2_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, true, bit_out)) {
                        data = (data << 1) | (bit_out ? 1ULL : 0ULL);
                        bits++;
                    }
                } else if (sp2_diff(dur, SP2_TE_LONG) < SP2_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, true, bit_out)) {
                        data = (data << 1) | (bit_out ? 1ULL : 0ULL);
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

bool rf_encode_secplus_v2(const RfCodes& in, std::vector<int>& out) {
    (void)in; (void)out;
    // Encoder requires two interdependent half-messages; use the saved .sub file
    return false;
}
