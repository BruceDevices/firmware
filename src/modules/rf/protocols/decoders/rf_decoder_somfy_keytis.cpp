// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_somfy_keytis.h"
#include "../rf_config.h"

#define KEYTIS_TE_SHORT 640
#define KEYTIS_TE_LONG 1280
#define KEYTIS_TE_DELTA 250
#define KEYTIS_MIN_BITS 80

static inline unsigned int keytis_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

static uint8_t keytis_crc(uint64_t data) {
    uint8_t crc = 0;
    data &= 0xFFF0FFFFFFFFFFULL;
    for (uint8_t i = 0; i < 56; i += 8) {
        crc = crc ^ (uint8_t)(data >> i) ^ (uint8_t)(data >> (i + 4));
    }
    return crc & 0xf;
}

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

bool rf_decode_somfy_keytis(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 4) return false;

    enum {
        ST_RESET,
        ST_PREAMBLE_H,
        ST_PREAMBLE_L,
        ST_CHECK_PREAMBLE,
        ST_DATA
    } step = ST_RESET;

    uint64_t data = 0;
    int bits = 0;
    int header_cnt = 0;
    uint32_t pdc = 0;
    manchester_state man_state;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (level && keytis_diff(dur, KEYTIS_TE_SHORT * 4) < KEYTIS_TE_DELTA * 4) {
                header_cnt++;
                step = ST_PREAMBLE_H;
            }
            break;

        case ST_PREAMBLE_H:
            if (!level && keytis_diff(dur, KEYTIS_TE_SHORT * 4) < KEYTIS_TE_DELTA * 4) {
                step = ST_CHECK_PREAMBLE;
            } else {
                header_cnt = 0;
                step = ST_RESET;
            }
            break;

        case ST_CHECK_PREAMBLE:
            if (level) {
                if (keytis_diff(dur, KEYTIS_TE_SHORT * 4) < KEYTIS_TE_DELTA * 4) {
                    header_cnt++;
                    step = ST_PREAMBLE_H;
                } else if (header_cnt > 1 &&
                           keytis_diff(dur, KEYTIS_TE_SHORT * 7) < KEYTIS_TE_DELTA * 4) {
                    data = 0; bits = 0; pdc = 0;
                    manchester_reset(man_state);
                    bool dummy = false;
                    manchester_advance(man_state, true, dummy);
                    step = ST_DATA;
                } else {
                    header_cnt = 0;
                    step = ST_RESET;
                }
            }
            break;

        case ST_DATA: {
            if (!level) {
                if (keytis_diff(dur, KEYTIS_TE_SHORT) < KEYTIS_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, false, bit_out)) {
                        if (bits < 56) {
                            data = (data << 1) | (bit_out ? 1ULL : 0ULL);
                        } else {
                            pdc = (pdc << 1) | (bit_out ? 1U : 0U);
                        }
                        bits++;
                    }
                } else if (keytis_diff(dur, KEYTIS_TE_LONG) < KEYTIS_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, false, bit_out)) {
                        if (bits < 56) {
                            data = (data << 1) | (bit_out ? 1ULL : 0ULL);
                        } else {
                            pdc = (pdc << 1) | (bit_out ? 1U : 0U);
                        }
                        bits++;
                    }
                } else if (dur >= (uint32_t)(KEYTIS_TE_LONG + KEYTIS_TE_DELTA)) {
                    if (bits == KEYTIS_MIN_BITS) {
                        uint64_t tmp = data ^ (data >> 8);
                        uint8_t crc_calc = ((tmp >> 40) & 0xF);
                        uint8_t crc_exp = keytis_crc(tmp);
                        if (crc_calc == crc_exp) {
                            uint64_t dec = data ^ (data >> 8);
                            out.key = data;
                            out.Bit = KEYTIS_MIN_BITS;
                            out.te = KEYTIS_TE_SHORT;
                            out.protocol = "Somfy_Keytis";
                            out.preset = "Ook270Async";
                            out.btn = (dec >> 48) & 0xF;
                            out.cnt = (dec >> 24) & 0xFFFF;
                            out.serial = dec & 0xFFFFFF;
                            return true;
                        }
                    }
                    data = 0; bits = 0; pdc = 0;
                    manchester_reset(man_state);
                    bool dummy = false;
                    manchester_advance(man_state, true, dummy);
                    step = ST_RESET;
                } else {
                    step = ST_RESET;
                }
            } else {
                if (keytis_diff(dur, KEYTIS_TE_SHORT) < KEYTIS_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, true, bit_out)) {
                        if (bits < 56) {
                            data = (data << 1) | (bit_out ? 1ULL : 0ULL);
                        } else {
                            pdc = (pdc << 1) | (bit_out ? 1U : 0U);
                        }
                        bits++;
                    }
                } else if (keytis_diff(dur, KEYTIS_TE_LONG) < KEYTIS_TE_DELTA) {
                    bool bit_out = false;
                    if (manchester_advance(man_state, true, bit_out)) {
                        if (bits < 56) {
                            data = (data << 1) | (bit_out ? 1ULL : 0ULL);
                        } else {
                            pdc = (pdc << 1) | (bit_out ? 1U : 0U);
                        }
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

bool rf_encode_somfy_keytis(const RfCodes& in, std::vector<int>& out) {
    (void)in; (void)out;
    return false;
}
