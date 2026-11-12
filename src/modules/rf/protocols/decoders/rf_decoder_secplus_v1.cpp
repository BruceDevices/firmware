// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_secplus_v1.h"
#include "../rf_config.h"

#define SP1_TE_SHORT 500
#define SP1_TE_LONG 1500
#define SP1_TE_DELTA 100
#define SP1_MIN_BITS 21

#define SP1_BIT_ERR (-1)
#define SP1_BIT_0 0
#define SP1_BIT_1 1
#define SP1_BIT_2 2

#define SP1_PACKET_1_BASE 0
#define SP1_PACKET_2_BASE 21

static inline unsigned int sp1_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

static uint32_t reverse_key_32(uint32_t data, int bits) {
    uint32_t rev = 0;
    for (int i = 0; i < bits; i++) {
        rev = (rev << 1) | ((data >> i) & 1U);
    }
    return rev;
}

static bool sp1_decode_payload(uint8_t data_array[44], uint32_t& fixed, uint32_t& rolling) {
    uint32_t acc = 0;
    uint8_t digit = 0;

    for (uint8_t i = 1; i < 21; i += 2) {
        digit = data_array[i];
        rolling = (rolling * 3) + digit;
        acc += digit;
        digit = (60 + data_array[i + 1] - acc) % 3;
        fixed = (fixed * 3) + digit;
        acc += digit;
    }

    acc = 0;
    for (uint8_t i = 22; i < 42; i += 2) {
        digit = data_array[i];
        rolling = (rolling * 3) + digit;
        acc += digit;
        digit = (60 + data_array[i + 1] - acc) % 3;
        fixed = (fixed * 3) + digit;
        acc += digit;
    }

    rolling = reverse_key_32(rolling, 32);
    return true;
}

bool rf_decode_secplus_v1(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 4) return false;

    enum {
        ST_RESET,
        ST_SEARCH_START,
        ST_SAVE,
        ST_DATA
    } step = ST_RESET;

    uint8_t data_array[44] = {0};
    uint8_t base_index = 0;
    uint8_t packet_accepted = 0;
    int bits = 0;
    unsigned int te_last = 0;
    int raw_idx = 0;
    int total = (int)durations.size();

    while (raw_idx < total) {
        int raw = durations[raw_idx];
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (!level && sp1_diff(dur, SP1_TE_SHORT * 120) < SP1_TE_DELTA * 120) {
                bits = 0;
                packet_accepted = 0;
                memset(data_array, 0, sizeof(data_array));
                step = ST_SEARCH_START;
            }
            break;

        case ST_SEARCH_START:
            if (level) {
                if (sp1_diff(dur, SP1_TE_SHORT) < SP1_TE_DELTA) {
                    base_index = SP1_PACKET_1_BASE;
                    data_array[bits + base_index] = SP1_BIT_0;
                    bits++;
                    step = ST_SAVE;
                } else if (sp1_diff(dur, SP1_TE_LONG) < SP1_TE_DELTA) {
                    base_index = SP1_PACKET_2_BASE;
                    data_array[bits + base_index] = SP1_BIT_2;
                    bits++;
                    step = ST_SAVE;
                } else {
                    step = ST_RESET;
                }
            } else {
                step = ST_RESET;
            }
            break;

        case ST_SAVE:
            if (!level) {
                if (sp1_diff(dur, SP1_TE_SHORT * 120) < SP1_TE_DELTA * 120) {
                    if (bits == SP1_MIN_BITS) {
                        if (base_index == SP1_PACKET_1_BASE) packet_accepted |= 1;
                        if (base_index == SP1_PACKET_2_BASE) packet_accepted |= 2;

                        if (packet_accepted == 3) {
                            uint32_t fixed = 0, rolling = 0;
                            sp1_decode_payload(data_array, fixed, rolling);
                            out.key = ((uint64_t)fixed << 32) | rolling;
                            out.Bit = 42;
                            out.te = SP1_TE_SHORT;
                            out.protocol = "SecPlus_v1";
                            out.preset = "Ook270Async";
                            out.serial = (fixed >= 27) ? (fixed / 27) : 0;
                            out.cnt = rolling;
                            out.btn = fixed % 3;
                            return true;
                        }
                    }
                    bits = 0;
                    step = ST_SEARCH_START;
                } else {
                    te_last = dur;
                    step = ST_DATA;
                }
            } else {
                step = ST_RESET;
            }
            break;

        case ST_DATA:
            if (level && bits <= SP1_MIN_BITS) {
                if (sp1_diff(te_last, SP1_TE_SHORT * 3) < SP1_TE_DELTA * 3 &&
                    sp1_diff(dur, SP1_TE_SHORT) < SP1_TE_DELTA) {
                    data_array[bits + base_index] = SP1_BIT_0;
                    bits++;
                    step = ST_SAVE;
                } else if (sp1_diff(te_last, SP1_TE_SHORT * 2) < SP1_TE_DELTA * 2 &&
                           sp1_diff(dur, SP1_TE_SHORT * 2) < SP1_TE_DELTA * 2) {
                    data_array[bits + base_index] = SP1_BIT_1;
                    bits++;
                    step = ST_SAVE;
                } else if (sp1_diff(te_last, SP1_TE_SHORT) < SP1_TE_DELTA &&
                           sp1_diff(dur, SP1_TE_SHORT * 3) < SP1_TE_DELTA * 3) {
                    data_array[bits + base_index] = SP1_BIT_2;
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
        raw_idx++;
    }

    return false;
}

bool rf_encode_secplus_v1(const RfCodes& in, std::vector<int>& out) {
    uint32_t fixed = (uint32_t)(in.key >> 32);
    uint32_t rolling = (uint32_t)(in.key & 0xFFFFFFFF);

    rolling += 2;
    if (rolling == 0xFFFFFFFF) rolling = 0xE6000000;

    rolling = reverse_key_32(rolling, 32);

    uint8_t rolling_array[20] = {0};
    uint8_t fixed_array[20] = {0};
    uint32_t tmp_r = rolling, tmp_f = fixed;

    for (int i = 19; i >= 0; i--) {
        rolling_array[i] = tmp_r % 3;
        tmp_r /= 3;
        fixed_array[i] = tmp_f % 3;
        tmp_f /= 3;
    }

    uint8_t data_array[44] = {0};
    data_array[0] = 0;
    data_array[21] = 2;

    uint32_t acc = 0;
    for (uint8_t i = 1; i < 11; i++) {
        acc += rolling_array[i - 1];
        data_array[i * 2 - 1] = rolling_array[i - 1];
        acc += fixed_array[i - 1];
        data_array[i * 2] = acc % 3;
    }

    acc = 0;
    for (uint8_t i = 11; i < 21; i++) {
        acc += rolling_array[i - 1];
        data_array[i * 2] = rolling_array[i - 1];
        acc += fixed_array[i - 1];
        data_array[i * 2 + 1] = acc % 3;
    }

    // Packet 1 header
    out.push_back(-(SP1_TE_SHORT * (116 + 3)));
    out.push_back(SP1_TE_SHORT);

    for (uint8_t i = 1; i < 21; i++) {
        switch (data_array[i]) {
        case 0:
            out.push_back(-(SP1_TE_SHORT * 3)); out.push_back(SP1_TE_SHORT); break;
        case 1:
            out.push_back(-(SP1_TE_SHORT * 2)); out.push_back(SP1_TE_SHORT * 2); break;
        case 2:
            out.push_back(-SP1_TE_SHORT); out.push_back(SP1_TE_SHORT * 3); break;
        }
    }

    // Packet 2 header
    out.push_back(-(SP1_TE_SHORT * 116));
    out.push_back(SP1_TE_SHORT * 3);

    for (uint8_t i = 22; i < 42; i++) {
        switch (data_array[i]) {
        case 0:
            out.push_back(-(SP1_TE_SHORT * 3)); out.push_back(SP1_TE_SHORT); break;
        case 1:
            out.push_back(-(SP1_TE_SHORT * 2)); out.push_back(SP1_TE_SHORT * 2); break;
        case 2:
            out.push_back(-SP1_TE_SHORT); out.push_back(SP1_TE_SHORT * 3); break;
        }
    }
    return true;
}
