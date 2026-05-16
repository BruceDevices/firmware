/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#ifndef ESP_PLATFORM

#include <assert.h>
#include <stdlib.h>
#include "nimble/nimble/include/nimble/ble.h"
#include "nimble/nimble/controller/include/controller/ble_ll.h"
#include "nimble/nimble/controller/include/controller/ble_ll_tmr.h"
#include "nimble/nimble/controller/include/controller/ble_ll_utils.h"

/* 37 bits require 5 bytes */
#define BLE_LL_CHMAP_LEN (5)

/* Sleep clock accuracy table (in ppm) */
static const uint16_t g_ble_sca_ppm_tbl[8] = {
    500, 250, 150, 100, 75, 50, 30, 20
};

int
ble_ll_utils_verify_aa(uint32_t aa)
{
    uint16_t aa_low;
    uint16_t aa_high;
    uint32_t temp;
    uint32_t mask;
    uint32_t prev_bit;
    uint8_t bits_diff;
    uint8_t consecutive;
    uint8_t transitions;
    uint8_t ones;
    int tmp;

    aa_low = aa & 0xffff;
    aa_high = aa >> 16;

    /* All four bytes cannot be equal */
    if (aa_low == aa_high) {
        return 0;
    }

    /* Upper 6 bits must have 2 transitions */
    tmp = (int16_t)aa_high >> 10;
    if (__builtin_popcount(tmp ^ (tmp >> 1)) < 2) {
        return 0;
    }

    /* Cannot be access address or be 1 bit different */
    aa = aa_high;
    aa = (aa << 16) | aa_low;
    bits_diff = 0;
    temp = aa ^ BLE_ACCESS_ADDR_ADV;
    for (mask = 0x00000001; mask != 0; mask <<= 1) {
        if (mask & temp) {
            ++bits_diff;
            if (bits_diff > 1) {
                break;
            }
        }
    }
    if (bits_diff <= 1) {
        return 0;
    }

    /* Cannot have more than 24 transitions */
    transitions = 0;
    consecutive = 1;
    ones = 0;
    mask = 0x00000001;
    while (mask < 0x80000000) {
        prev_bit = aa & mask;
        mask <<= 1;
        if (mask & aa) {
            if (prev_bit == 0) {
                ++transitions;
                consecutive = 1;
            } else {
                ++consecutive;
            }
        } else {
            if (prev_bit == 0) {
                ++consecutive;
            } else {
                ++transitions;
                consecutive = 1;
            }
        }

        if (prev_bit) {
            ones++;
        }

        /* 8 lsb should have at least three 1 */
        if (mask == 0x00000100 && ones < 3) {
            break;
        }

        /* 16 lsb should have no more than 11 transitions */
        if (mask == 0x00010000 && transitions > 11) {
            break;
        }

        /* This is invalid! */
        if (consecutive > 6) {
            /* Make sure we always detect invalid sequence below */
            mask = 0;
            break;
        }
    }

    /* Invalid sequence found */
    if (mask != 0x80000000) {
        return 0;
    }

    /* Cannot be more than 24 transitions */
    if (transitions > 24) {
        return 0;
    }

    return 1;
}

uint32_t
ble_ll_utils_calc_aa(void)
{
    uint32_t aa;

    do {
        aa = ble_ll_rand();
    } while (!ble_ll_utils_verify_aa(aa));

    return aa;
}

uint32_t
ble_ll_utils_calc_seed_aa(void)
{
    uint32_t seed_aa;

    while (1) {
        seed_aa = ble_ll_rand();

        /* saa(19) == saa(15) */
        if (!!(seed_aa & (1 << 19)) != !!(seed_aa & (1 << 15))) {
            continue;
        }

        /* saa(22) = saa(16) */
        if (!!(seed_aa & (1 << 22)) != !!(seed_aa & (1 << 16))) {
            continue;
        }

        /* saa(22) != saa(15) */
        if (!!(seed_aa & (1 << 22)) == !!(seed_aa & (1 << 15))) {
            continue;
        }

        /* saa(25) == 0 */
        if (seed_aa & (1 << 25)) {
            continue;
        }

        /* saa(23) == 1 */
        if (!(seed_aa & (1 << 23))) {
            continue;
        }

        break;
    }

    return seed_aa;
}

uint32_t
ble_ll_utils_calc_big_aa(uint32_t seed_aa, uint32_t n)
{
    uint32_t d;
    uint32_t dw;

    /* Core 5.3, Vol 6, Part B, 2.1.2 */
    /* TODO simplify? */
    d = ((35 * n) + 42) % 128;
    dw = (!!(d & (1 << 0)) << 31) |
         (!!(d & (1 << 0)) << 30) |
         (!!(d & (1 << 0)) << 29) |
         (!!(d & (1 << 0)) << 28) |
         (!!(d & (1 << 0)) << 27) |
         (!!(d & (1 << 0)) << 26) |
         (!!(d & (1 << 1)) << 25) |
         (!!(d & (1 << 6)) << 24) |
         (!!(d & (1 << 1)) << 23) |
         (!!(d & (1 << 5)) << 21) |
         (!!(d & (1 << 4)) << 20) |
         (!!(d & (1 << 3)) << 18) |
         (!!(d & (1 << 2)) << 17);

    return seed_aa ^ dw;
}

uint8_t
ble_ll_utils_chan_map_remap(const uint8_t *chan_map, uint8_t remap_index)
{
    uint8_t cntr;
    uint8_t mask;
    uint8_t usable_chans;
    uint8_t chan;
    int i, j;

    /* NOTE: possible to build a map but this would use memory. For now,
     * we just calculate
     * Iterate through channel map to find this channel
     */
    chan = 0;
    cntr = 0;
    for (i = 0; i < BLE_LL_CHMAP_LEN; i++) {
        usable_chans = chan_map[i];
        if (usable_chans != 0) {
            mask = 0x01;
            for (j = 0; j < 8; j++) {
                if (usable_chans & mask) {
                    if (cntr == remap_index) {
                        return (chan + j);
                    }
                    ++cntr;
                }
                mask <<= 1;
            }
        }
        chan += 8;
    }

    /* we should never reach here */
    BLE_LL_ASSERT(0);
    return 0;
}

uint8_t
ble_ll_utils_chan_map_used_get(const uint8_t *chan_map)
{
    return __builtin_popcountll(((uint64_t)(chan_map[4] & 0x1f) << 32) |
                                get_le32(chan_map));
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CSA2)
#if __thumb2__
static inline uint32_t
ble_ll_utils_csa2_perm(uint32_t val)
{
    __asm__ volatile (".syntax unified      \n"
                      "rbit %[val], %[val]  \n"
                      "rev %[val], %[val]   \n"
                      : [val] "+r" (val));

    return val;
}
#else
static uint32_t
ble_ll_utils_csa2_perm(uint32_t in)
{
    uint32_t out = 0;
    int i;

    for (i = 0; i < 8; i++) {
        out |= ((in >> i) & 0x00000001) << (7 - i);
    }

    for (i = 8; i < 16; i++) {
        out |= ((in >> i) & 0x00000001) << (15 + 8 - i);
    }

    return out;
}
#endif

static inline uint32_t
ble_ll_utils_csa2_mam(uint32_t a, uint32_t b)
{
    return (17 * a + b) % 65536;
}

static uint16_t
ble_ll_utils_csa2_prn_s(uint16_t counter, uint16_t ch_id)
{
    uint32_t prn_s;

    prn_s = counter ^ ch_id;

    prn_s = ble_ll_utils_csa2_perm(prn_s);
    prn_s = ble_ll_utils_csa2_mam(prn_s, ch_id);

    prn_s = ble_ll_utils_csa2_perm(prn_s);
    prn_s = ble_ll_utils_csa2_mam(prn_s, ch_id);

    prn_s = ble_ll_utils_csa2_perm(prn_s);
    prn_s = ble_ll_utils_csa2_mam(prn_s, ch_id);

    return prn_s;
}

static uint16_t
ble_ll_utils_csa2_prng(uint16_t counter, uint16_t ch_id)
{
    uint16_t prn_s;
    uint16_t prn_e;

    prn_s = ble_ll_utils_csa2_prn_s(counter, ch_id);
    prn_e = prn_s ^ ch_id;

    return prn_e;
}

/* Find remap_idx for given chan_idx */
static uint16_t
ble_ll_utils_csa2_chan2remap(uint16_t chan_idx, const uint8_t *chan_map)
{
    uint16_t remap_idx = 0;
    uint32_t u32 = 0;
    unsigned idx;

    for (idx = 0; idx < 37; idx++) {
        if ((idx % 8) == 0) {
            u32 = chan_map[idx / 8];
        }
        if (u32 & 1) {
            if (idx == chan_idx) {
                return remap_idx;
            }
            remap_idx++;
        }
        u32 >>= 1;
    }

    BLE_LL_ASSERT(0);

    return 0;
}

/* Find chan_idx at given remap_idx */
static uint16_t
ble_ll_utils_csa2_remap2chan(uint16_t remap_idx, const uint8_t *chan_map)
{
    uint32_t u32 = 0;
    unsigned idx;

    for (idx = 0; idx < 37; idx++) {
        if ((idx % 8) == 0) {
            u32 = chan_map[idx / 8];
        }
        if (u32 & 1) {
            if (!remap_idx) {
                return idx;
            }
            remap_idx--;
        }
        u32 >>= 1;
    }

    BLE_LL_ASSERT(0);

    return 0;
}

static uint16_t
ble_ll_utils_csa2_calc_chan_idx(uint16_t prn_e, uint8_t num_used_chans,
                                const uint8_t *chanm_map, uint16_t *remap_idx)
{
    uint16_t chan_idx;

    chan_idx = prn_e % 37;
    if (chanm_map[chan_idx / 8] & (1 << (chan_idx % 8))) {
        *remap_idx = ble_ll_utils_csa2_chan2remap(chan_idx, chanm_map);
        return chan_idx;
    }

    *remap_idx = (num_used_chans * prn_e) / 65536;
    chan_idx = ble_ll_utils_csa2_remap2chan(*remap_idx, chanm_map);

    return chan_idx;
}

uint8_t
ble_ll_utils_dci_csa2(uint16_t counter, uint16_t chan_id,
                      uint8_t num_used_chans, const uint8_t *chan_map)
{
    uint16_t prn_e;
    uint16_t chan_idx;
    uint16_t remap_idx;

    prn_e = ble_ll_utils_csa2_prng(counter, chan_id);

    chan_idx = ble_ll_utils_csa2_calc_chan_idx(prn_e, num_used_chans, chan_map,
                                               &remap_idx);

    return chan_idx;
}

uint16_t
ble_ll_utils_dci_iso_event(uint16_t counter, uint16_t chan_id,
                           uint16_t *prn_sub_lu, uint8_t chan_map_used,
                           const uint8_t *chan_map, uint16_t *remap_idx)
{
    uint16_t prn_s;
    uint16_t prn_e;
    uint16_t chan_idx;

    prn_s = ble_ll_utils_csa2_prn_s(counter, chan_id);
    prn_e = prn_s ^ chan_id;

    *prn_sub_lu = prn_s;

    chan_idx = ble_ll_utils_csa2_calc_chan_idx(prn_e, chan_map_used, chan_map,
                                               remap_idx);

    return chan_idx;
}

uint16_t
ble_ll_utils_dci_iso_subevent(uint16_t chan_id, uint16_t *prn_sub_lu,
                              uint8_t chan_map_used, const uint8_t *chan_map,
                              uint16_t *remap_idx)
{
    uint16_t prn_sub_se;
    uint16_t chan_idx;
    uint16_t d;

    *prn_sub_lu = ble_ll_utils_csa2_perm(*prn_sub_lu);
    *prn_sub_lu = ble_ll_utils_csa2_mam(*prn_sub_lu, chan_id);
    prn_sub_se = *prn_sub_lu ^ chan_id;

    /* Core 5.3, Vol 6, Part B, 4.5.8.3.6 (enjoy!) */
    /* TODO optimize this somehow */
    d = MAX(1, MAX(MIN(3, chan_map_used - 5),
                   MIN(11, (chan_map_used - 10) / 2)));
    *remap_idx = (*remap_idx + d + prn_sub_se *
                  (chan_map_used - 2 * d + 1) / 65536) % chan_map_used;

    chan_idx = ble_ll_utils_csa2_remap2chan(*remap_idx, chan_map);

    return chan_idx;
}
#endif

uint32_t
ble_ll_utils_calc_window_widening(uint32_t anchor_point,
                                  uint32_t last_anchor_point,
                                  uint8_t central_sca)
{
    uint32_t total_sca_ppm;
    uint32_t window_widening;
    int32_t time_since_last_anchor;
    uint32_t delta_msec;

    window_widening = 0;

    time_since_last_anchor = (int32_t)(anchor_point - last_anchor_point);
    if (time_since_last_anchor > 0) {
        delta_msec = ble_ll_tmr_t2u(time_since_last_anchor) / 1000;
        total_sca_ppm = g_ble_sca_ppm_tbl[central_sca] + MYNEWT_VAL(BLE_LL_SCA);
        window_widening = (total_sca_ppm * delta_msec) / 1000;
    }

    return window_widening;
}
#endif
