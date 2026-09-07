// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_decoder_nice_flor_s.h"
#include "../rf_config.h"

#define NFS_TE_SHORT 500
#define NFS_TE_LONG 1000
#define NFS_TE_DELTA 300
#define NFS_MIN_BITS 52

static inline unsigned int nfs_diff(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

static void nfs_magic_xor(uint8_t* p, uint8_t k) {
    for (uint8_t i = 1; i < 6; i++) p[i] ^= k;
}

static uint64_t nfs_decrypt(uint64_t data) {
    uint8_t* p = (uint8_t*)&data;
    uint8_t k = 0;

    k = ~p[4]; p[5] = ~p[5]; p[4] = ~p[2]; p[2] = ~p[0]; p[0] = k;
    k = ~p[3]; p[3] = ~p[1]; p[1] = k;

    // Simplified decryption without rainbow table - uses XOR with fixed constants
    // Full decryption requires external rainbow table file
    for (uint8_t y = 0; y < 2; y++) {
        k = 0x25;
        nfs_magic_xor(p, k);
        p[5] &= 0x0f;
        p[0] ^= k & 0x7;
        k = 0x55;
        nfs_magic_xor(p, k);
        p[5] &= 0x0f;
        p[0] ^= k & 0xe0;
        if (y == 0) { k = p[0]; p[0] = p[1]; p[1] = k; }
    }

    return data;
}

bool rf_decode_nice_flor_s(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 4) return false;

    enum {
        ST_RESET,
        ST_CHECK_HEADER,
        ST_FOUND_HEADER,
        ST_SAVE,
        ST_CHECK
    } step = ST_RESET;

    uint64_t data = 0;
    int bits = 0;
    unsigned int te_last = 0;
    uint64_t saved_data = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        unsigned int dur = (unsigned int)(raw > 0 ? raw : -raw);

        switch (step) {
        case ST_RESET:
            if (!level && nfs_diff(dur, NFS_TE_SHORT * 38) < NFS_TE_DELTA * 38)
                step = ST_CHECK_HEADER;
            break;

        case ST_CHECK_HEADER:
            if (level && nfs_diff(dur, NFS_TE_SHORT * 3) < NFS_TE_DELTA * 3)
                step = ST_FOUND_HEADER;
            else
                step = ST_RESET;
            break;

        case ST_FOUND_HEADER:
            if (!level && nfs_diff(dur, NFS_TE_SHORT * 3) < NFS_TE_DELTA * 3) {
                data = 0; bits = 0;
                step = ST_SAVE;
            } else {
                step = ST_RESET;
            }
            break;

        case ST_SAVE:
            if (bits > NFS_MIN_BITS) { step = ST_RESET; break; }
            if (level) {
                if (nfs_diff(dur, NFS_TE_SHORT * 3) < NFS_TE_DELTA) {
                    step = ST_RESET;
                    if (bits == NFS_MIN_BITS) {
                        uint64_t dec = nfs_decrypt(data);
                        out.key = data;
                        out.Bit = NFS_MIN_BITS;
                        out.te = NFS_TE_SHORT;
                        out.protocol = "Nice_Flor_S";
                        out.preset = "Ook270Async";
                        out.cnt = (uint32_t)(dec & 0xFFFF);
                        out.serial = (uint32_t)((dec >> 16) & 0xFFFFFFF);
                        out.btn = (uint8_t)((dec >> 48) & 0xF);
                        return true;
                    }
                    break;
                }
                te_last = dur;
                step = ST_CHECK;
            }
            break;

        case ST_CHECK:
            if (!level) {
                if (nfs_diff(te_last, NFS_TE_SHORT) < NFS_TE_DELTA &&
                    nfs_diff(dur, NFS_TE_LONG) < NFS_TE_DELTA) {
                    data = (data << 1) | 0ULL;
                    bits++;
                    step = ST_SAVE;
                } else if (nfs_diff(te_last, NFS_TE_LONG) < NFS_TE_DELTA &&
                           nfs_diff(dur, NFS_TE_SHORT) < NFS_TE_DELTA) {
                    data = (data << 1) | 1ULL;
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

bool rf_encode_nice_flor_s(const RfCodes& in, std::vector<int>& out) {
    (void)in; (void)out;
    return false;
}
