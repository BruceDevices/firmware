#include "rolling_code_proto.h"
#include "rf_utils.h"
#include "rolling_aes.h"
#include <esp_random.h>
#include <string.h>

// ===========================================================================
// Low-level pulse helpers. Signed-int convention: +HIGH µs, -LOW µs.
// All per-protocol builders below are direct ports of Momentum's *_get_upload()
// functions (index arithmetic only; no HAL).
// ===========================================================================

// Standard KeeLoq-family PWM bit stream, MSB first. Matches Momentum's
// faac_slh / keeloq get_upload: header = 2*te_long high + 2*te_long low, then
// each bit: '1' = te_long high + te_short low, '0' = te_short high + te_long low.
static void pwm_append(std::vector<int> &t, uint64_t data, int nbits, int te_short, int te_long) {
    t.push_back(te_long * 2);
    t.push_back(-(te_long * 2));
    for (int i = nbits - 1; i >= 0; i--) {
        if ((data >> i) & 1) {
            t.push_back(te_long);
            t.push_back(-te_short);
        } else {
            t.push_back(te_short);
            t.push_back(-te_long);
        }
    }
}

std::vector<int> encode_keeloq(uint64_t data64, int te_short, int te_long) {
    std::vector<int> t;
    pwm_append(t, data64, 64, te_short, te_long);
    return t;
}

std::vector<int> encode_faac_slh(uint64_t data64) {
    std::vector<int> t;
    pwm_append(t, data64, 64, 255, 595);
    return t;
}

std::vector<int> encode_jarolift(uint64_t data64, uint8_t tail8) {
    // 72-bit frame: 64-bit data then 8-bit tail. Long LOW gap + header (te 400/800).
    const int te_short = 400, te_long = 800;
    std::vector<int> t;
    t.push_back(-(te_long * 18)); // ~14k µs start gap
    t.push_back(1500);            // first header bit high
    t.push_back(-te_short);
    for (uint8_t i = 0; i < 8; i++) { // finish header
        t.push_back(te_short);
        t.push_back(-te_short);
    }
    for (int i = 63; i >= 0; i--) {
        if ((data64 >> i) & 1) { t.push_back(te_short); t.push_back(-te_long); }
        else { t.push_back(te_long); t.push_back(-te_short); }
    }
    for (int i = 7; i >= 0; i--) {
        if ((tail8 >> i) & 1) { t.push_back(te_short); t.push_back(-te_long); }
        else { t.push_back(te_long); t.push_back(-te_short); }
    }
    return t;
}

// ---------------------------------------------------------------------------
// Nice FloR-S / Nice One (ported from nice_flor_s.c).
// Table cipher over the 32-byte rainbow table; with the placeholder zeros it
// yields a deterministic-but-invalid result until the real table is supplied.
// ---------------------------------------------------------------------------
static uint64_t nice_flor_s_encrypt(uint64_t data) {
    const uint8_t *buffer = nice_flor_s_rainbow;
    uint8_t *p = (uint8_t *)&data; // ESP32 + Flipper are both little-endian
    uint8_t k = 0;
    for (uint8_t y = 0; y < 2; y++) {
        k = buffer[p[0] & 0x1f];
        for (uint8_t i = 1; i < 6; i++) p[i] ^= k;
        p[5] &= 0x0f;
        p[0] ^= k & 0xe0;
        k = buffer[p[0] >> 3] + 0x25;
        for (uint8_t i = 1; i < 6; i++) p[i] ^= k;
        p[5] &= 0x0f;
        p[0] ^= k & 0x7;
        if (y == 0) { k = p[0]; p[0] = p[1]; p[1] = k; }
    }
    p[5] = ~p[5] & 0x0f;
    k = ~p[4];
    p[4] = ~p[0];
    p[0] = ~p[2];
    p[2] = k;
    k = ~p[3];
    p[3] = ~p[1];
    p[1] = k;
    return data;
}

// Inverse of nice_flor_s_encrypt (ported from nice_flor_s_decrypt).
static uint64_t nice_flor_s_decrypt(uint64_t data) {
    const uint8_t *buffer = nice_flor_s_rainbow;
    uint8_t *p = (uint8_t *)&data;
    uint8_t k = 0;
    k = ~p[4];
    p[5] = ~p[5];
    p[4] = ~p[2];
    p[2] = ~p[0];
    p[0] = k;
    k = ~p[3];
    p[3] = ~p[1];
    p[1] = k;
    for (uint8_t y = 0; y < 2; y++) {
        k = buffer[p[0] >> 3] + 0x25;
        for (uint8_t i = 1; i < 6; i++) p[i] ^= k;
        p[5] &= 0x0f;
        p[0] ^= k & 0x7;
        k = buffer[p[0] & 0x1f];
        for (uint8_t i = 1; i < 6; i++) p[i] ^= k;
        p[5] &= 0x0f;
        p[0] ^= k & 0xe0;
        if (y == 0) { k = p[0]; p[0] = p[1]; p[1] = k; }
    }
    return data;
}

static uint8_t nice_one_crc(uint8_t *p) {
    uint8_t crc = 0;
    uint8_t crc_data = 0xff;
    for (uint8_t i = 4; i < 68; i++) {
        if ((p[i / 8] >> (7 - (i % 8))) & 1) crc = crc_data ^ 1;
        else crc = crc_data;
        crc_data >>= 1;
        if (crc & 0x01) crc_data ^= 0x97;
    }
    crc = 0;
    for (uint8_t i = 0; i < 8; i++) {
        crc <<= 1;
        if ((crc_data >> i) & 0x01) crc |= 1;
    }
    return crc;
}

static void nice_one_get_data(uint8_t *p, uint8_t num_parcel, uint8_t hold_bit) {
    uint8_t k = 0, crc = 0;
    p[1] = (p[1] & 0x0f) | ((0x0f ^ (p[0] & 0x0F) ^ num_parcel) << 4);
    k = (num_parcel < 4) ? 0x8f : 0x80;
    hold_bit = hold_bit ? 0x10 : 0;
    k = num_parcel ^ k;
    p[7] = k;
    p[8] = hold_bit ^ (k << 4);
    crc = nice_one_crc(p);
    p[8] |= crc >> 4;
    p[9] = crc << 4;
}

// Build the full Nice upload (16 parcels). nice_one = true → 72-bit frame.
static std::vector<int> build_nice_upload(uint32_t serial, uint16_t cnt, uint8_t btn, bool nice_one) {
    const int te_short = 500, te_long = 1000;
    std::vector<int> t;
    uint64_t decrypt = ((uint64_t)serial << 16) | cnt;
    uint64_t enc_part = nice_flor_s_encrypt(decrypt);

    for (int parcel = 0; parcel < 16; parcel++) {
        uint8_t byte = (btn << 4) | (0xF ^ btn ^ parcel);
        uint64_t data = (uint64_t)byte << 44 | enc_part;

        // Header + start bit
        t.push_back(-(te_short * 37));
        t.push_back(te_short * 3);
        t.push_back(-(te_short * 3));

        for (int j = 52; j > 0; j--) {
            if ((data >> (j - 1)) & 1) { t.push_back(te_long); t.push_back(-te_short); }
            else { t.push_back(te_short); t.push_back(-te_long); }
        }

        if (nice_one) {
            uint8_t add[10] = {0};
            for (int i = 0; i < 7; i++) add[i] = (data >> (48 - i * 8)) & 0xFF;
            nice_one_get_data(add, parcel, parcel);
            uint32_t data2 = 0;
            for (int j = 7; j < 10; j++) { data2 <<= 8; data2 += add[j]; }
            for (int j = 24; j > 4; j--) {
                if ((data2 >> (j - 1)) & 1) { t.push_back(te_long); t.push_back(-te_short); }
                else { t.push_back(te_short); t.push_back(-te_long); }
            }
        }
        t.push_back(te_short * 3); // stop bit
    }
    return t;
}

std::vector<int> encode_nice_flor_s(uint64_t data52) { return build_nice_upload((uint32_t)(data52 >> 16), (uint16_t)data52, 1, false); }
std::vector<int> encode_nice_one(uint64_t data72) { return build_nice_upload((uint32_t)(data72 >> 16), (uint16_t)data72, 1, true); }

// ---------------------------------------------------------------------------
// Alutech AT-4N (ported from alutech_at_4n.c). LFSR cipher over the rainbow
// table (placeholder zeros until supplied).
// ---------------------------------------------------------------------------
static uint32_t alutech_magic(const uint8_t *buffer, uint8_t n) {
    uint32_t addr = n * 4, v = 0;
    for (uint32_t i = addr; i < addr + 4; i++) v = (v << 8) | buffer[i];
    return v;
}

static uint8_t alutech_crc(uint64_t data) {
    uint8_t *p = (uint8_t *)&data;
    uint8_t crc = 0xff;
    for (uint8_t y = 0; y < 8; y++) {
        crc ^= p[y];
        for (uint8_t i = 0; i < 8; i++) crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
    }
    return crc;
}

static uint8_t alutech_data_crc(uint8_t data) {
    uint8_t crc = data;
    for (uint8_t i = 0; i < 8; i++) crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
    return ~crc;
}

static uint64_t alutech_encrypt(uint64_t data) {
    const uint8_t *buffer = alutech_at4n_rainbow;
    uint8_t *p = (uint8_t *)&data;
    uint32_t data1 = 0;
    uint32_t data2 = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
    uint32_t data3 = p[4] << 24 | p[5] << 16 | p[6] << 8 | p[7];
    uint32_t md[] = {alutech_magic(buffer, 6), alutech_magic(buffer, 4), alutech_magic(buffer, 5),
                     alutech_magic(buffer, 1), alutech_magic(buffer, 2), alutech_magic(buffer, 0)};
    do {
        data1 += md[0];
        data2 += ((md[1] + (data3 << 4)) ^ ((md[2] + (data3 >> 5)) ^ (data1 + data3)));
        data3 += ((md[3] + (data2 << 4)) ^ ((md[4] + (data2 >> 5)) ^ (data1 + data2)));
    } while (data1 != md[5]);
    p[0] = data2 >> 24; p[1] = data2 >> 16; p[3] = data2; p[2] = data2 >> 8;
    p[4] = data3 >> 24; p[5] = data3 >> 16; p[6] = data3 >> 8; p[7] = data3;
    return data;
}

// Inverse of alutech_encrypt (ported from alutech_at_4n_decrypt).
static uint64_t alutech_decrypt(uint64_t data) {
    const uint8_t *buffer = alutech_at4n_rainbow;
    uint8_t *p = (uint8_t *)&data;
    uint32_t data1 = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
    uint32_t data2 = p[4] << 24 | p[5] << 16 | p[6] << 8 | p[7];
    uint32_t data3 = 0;
    uint32_t md[] = {alutech_magic(buffer, 0), alutech_magic(buffer, 1), alutech_magic(buffer, 2),
                     alutech_magic(buffer, 3), alutech_magic(buffer, 4), alutech_magic(buffer, 5)};
    uint32_t i = md[0];
    do {
        data2 -= ((md[1] + (data1 << 4)) ^ ((md[2] + (data1 >> 5)) ^ (data1 + i)));
        data3 = data2 + i;
        i += md[3];
        data1 -= ((md[4] + (data2 << 4)) ^ ((md[5] + (data2 >> 5)) ^ data3));
    } while (i != 0);
    p[0] = data1 >> 24; p[1] = data1 >> 16; p[3] = data1; p[4] = data2 >> 24;
    p[5] = data2 >> 16; p[2] = data1 >> 8; p[6] = data2 >> 8; p[7] = data2;
    return data;
}

std::vector<int> encode_alutech_at4n(uint64_t data72) {
    // data72 holds the 64-bit payload; CRC is recomputed here.
    const int te_short = 400, te_long = 800;
    uint64_t enc = alutech_encrypt(data72);
    uint64_t frame = reverse_bits(enc, 64);
    uint8_t crc = (uint8_t)reverse_bits(alutech_crc(enc), 8);

    std::vector<int> t;
    for (int i = 0; i < 12; i++) { t.push_back(te_short); t.push_back(-te_short); } // preamble
    t.back() -= te_short * 9;                                                       // extend last LOW
    for (int i = 64; i > 0; i--) {
        if ((frame >> (i - 1)) & 1) { t.push_back(te_short); t.push_back(-te_long); }
        else { t.push_back(te_long); t.push_back(-te_short); }
    }
    for (int i = 8; i > 0; i--) {
        if ((crc >> (i - 1)) & 1) { t.push_back(te_short); t.push_back(-te_long); }
        else { t.push_back(te_long); t.push_back(-te_short); }
    }
    t.back() -= te_long * 20; // inter-frame silence
    return t;
}

// ---------------------------------------------------------------------------
// Beninca ARC (ported from beninca_arc.c). AES-128 over the 16-byte key
// (placeholder zeros until supplied). 3 packets with mini-counter 2/4/6.
// ---------------------------------------------------------------------------
static void beninca_packet(std::vector<int> &t, uint64_t data, uint64_t data2) {
    const int te_short = 300, te_long = 600;
    for (int i = 64; i > 0; i--) {
        if ((data >> (i - 1)) & 1) { t.push_back(te_short); t.push_back(-te_long); }
        else { t.push_back(te_long); t.push_back(-te_short); }
    }
    for (int i = 64; i > 0; i--) {
        if ((data2 >> (i - 1)) & 1) { t.push_back(te_short); t.push_back(-te_long); }
        else { t.push_back(te_long); t.push_back(-te_short); }
    }
    t.push_back(te_short);          // stop bit
    t.push_back(-(te_long * 15));   // gap between packets
}

static std::vector<int> build_beninca_upload(uint32_t serial, uint8_t btn, uint32_t cnt, uint16_t seed) {
    std::vector<int> t;
    uint8_t expanded[176];
    aes_key_expansion(beninca_arc_aes_key, expanded);
    for (uint8_t i = 0; i < 3; i++) {
        uint64_t middle = (uint64_t)((i + 1) * 2); // mini-counter 2/4/6
        uint8_t pt[16];
        pt[0] = serial >> 24; pt[1] = serial >> 16; pt[2] = serial >> 8; pt[3] = serial;
        pt[4] = btn;
        pt[5] = (middle >> 32) & 0xFF; pt[6] = (middle >> 24) & 0xFF; pt[7] = (middle >> 16) & 0xFF;
        pt[8] = (middle >> 8) & 0xFF; pt[9] = middle & 0xFF;
        pt[10] = cnt >> 24; pt[11] = cnt >> 16; pt[12] = cnt >> 8; pt[13] = cnt;
        pt[14] = seed >> 8; pt[15] = seed;
        aes128_encrypt(expanded, pt);
        reverse_bits_in_bytes(pt, 16);
        uint64_t data = 0, data2 = 0;
        for (uint8_t j = 0; j < 8; j++) { data = (data << 8) | pt[j]; data2 = (data2 << 8) | pt[j + 8]; }
        beninca_packet(t, data, data2);
    }
    return t;
}

// ---------------------------------------------------------------------------
// Security+ 1.0 (ported from secplus_v1.c). Keyless rolling code: base-3
// (tri-state) encoding of a 32-bit fixed value and a 32-bit rolling counter
// across two 21-symbol packets. te_short = 500.
// ---------------------------------------------------------------------------
std::vector<int> encode_security_plus_1(uint32_t fixed, uint32_t rolling) {
    const int te = 500;
    uint8_t data_array[42] = {0};
    uint8_t rolling_array[20] = {0}, fixed_array[20] = {0};

    rolling = (uint32_t)reverse_bits(rolling, 32);
    uint32_t r = rolling, f = fixed;
    for (int i = 19; i >= 0; i--) {
        rolling_array[i] = r % 3; r /= 3;
        fixed_array[i] = f % 3; f /= 3;
    }
    data_array[0] = 0x00;  // packet 1 header marker
    data_array[21] = 0x01; // packet 2 header marker
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

    std::vector<int> t;
    auto emit_trit = [&](uint8_t v) {
        if (v == 0) { t.push_back(-(te * 3)); t.push_back(te); }
        else if (v == 1) { t.push_back(-(te * 2)); t.push_back(te * 2); }
        else { t.push_back(-te); t.push_back(te * 3); }
    };
    // Packet 1
    t.push_back(-(te * (116 + 3)));
    t.push_back(te);
    for (int i = 1; i < 21; i++) emit_trit(data_array[i]);
    // Packet 2
    t.push_back(-(te * 116));
    t.push_back(te * 3);
    for (int i = 22; i < 42; i++) emit_trit(data_array[i]);
    return t;
}

// ---------------------------------------------------------------------------
// Security+ 2.0 (ported from secplus_v2.c — the argilo v2 algorithm). Keyless
// rolling code: 28-bit rolling counter encoded base-3, split/scrambled across
// two 62-bit Manchester packets. te_short = 250, te_long = 500.
// ---------------------------------------------------------------------------
#define SECPLUS_V2_HEADER   0x3C0000000000ULL
#define SECPLUS_V2_PACKET_1 0x000000000000ULL
#define SECPLUS_V2_PACKET_2 0x010000000000ULL

static bool secplus_v2_mix_order_encode(uint8_t order, uint16_t p[]) {
    uint16_t a, b, c;
    switch (order) {
        case 0x06: case 0x09: a = p[2]; b = p[1]; c = p[0]; break;
        case 0x08: case 0x04: a = p[1]; b = p[2]; c = p[0]; break;
        case 0x01:            a = p[2]; b = p[0]; c = p[1]; break;
        case 0x00:            a = p[0]; b = p[2]; c = p[1]; break;
        case 0x05:            a = p[1]; b = p[0]; c = p[2]; break;
        case 0x02: case 0x0A: a = p[0]; b = p[1]; c = p[2]; break;
        default: return false;
    }
    p[0] = a; p[1] = b; p[2] = c;
    return true;
}

static bool secplus_v2_mix_invet(uint8_t invert, uint16_t p[]) {
    switch (invert) {
        case 0x00: p[0] = ~p[0] & 0x3FF; p[1] = ~p[1] & 0x3FF; break;
        case 0x01: p[1] = ~p[1] & 0x3FF; break;
        case 0x02: p[2] = ~p[2] & 0x3FF; break;
        case 0x04: p[0] = ~p[0] & 0x3FF; p[1] = ~p[1] & 0x3FF; p[2] = ~p[2] & 0x3FF; break;
        case 0x05: case 0x0A: p[0] = ~p[0] & 0x3FF; p[2] = ~p[2] & 0x3FF; break;
        case 0x06: p[1] = ~p[1] & 0x3FF; p[2] = ~p[2] & 0x3FF; break;
        case 0x08: p[0] = ~p[0] & 0x3FF; break;
        case 0x09: break;
        default: return false;
    }
    return true;
}

static bool secplus_v2_mix_order_decode(uint8_t order, uint16_t p[]) {
    uint16_t a = p[0], b = p[1], c = p[2];
    switch (order) {
        case 0x06: case 0x09: p[2] = a; p[0] = c; break;
        case 0x08: case 0x04: p[1] = a; p[2] = b; p[0] = c; break;
        case 0x01:            p[2] = a; p[0] = b; p[1] = c; break;
        case 0x00:            p[2] = b; p[1] = c; break;
        case 0x05:            p[1] = a; p[0] = b; break;
        case 0x02: case 0x0A: break;
        default: return false;
    }
    return true;
}

// Decode one Security+ 2.0 half-packet (ported from secplus_v2_decode_half).
static bool secplus_v2_decode_half(uint64_t data, uint8_t roll_array[], uint32_t *fixed) {
    uint8_t order = (data >> 34) & 0x0f;
    uint8_t invert = (data >> 30) & 0x0f;
    uint16_t p[3] = {0};
    for (int i = 29; i >= 0; i -= 3) {
        p[0] = p[0] << 1 | ((data >> i) & 1);
        p[1] = p[1] << 1 | ((data >> (i - 1)) & 1);
        p[2] = p[2] << 1 | ((data >> (i - 2)) & 1);
    }
    if (!secplus_v2_mix_invet(invert, p)) return false;
    if (!secplus_v2_mix_order_decode(order, p)) return false;
    uint64_t d = order << 4 | invert;
    int k = 0;
    for (int i = 6; i >= 0; i -= 2) { roll_array[k] = (d >> i) & 0x03; if (roll_array[k++] == 3) return false; }
    for (int i = 8; i >= 0; i -= 2) { roll_array[k] = (p[2] >> i) & 0x03; if (roll_array[k++] == 3) return false; }
    fixed[0] = p[0] << 10 | p[1];
    return true;
}

static uint64_t secplus_v2_encode_half(const uint8_t roll_array[], uint32_t fixed) {
    uint64_t data = 0;
    uint16_t p[3] = {(uint16_t)((fixed >> 10) & 0x3FF), (uint16_t)(fixed & 0x3FF), 0};
    uint8_t order = roll_array[0] << 2 | roll_array[1];
    uint8_t invert = roll_array[2] << 2 | roll_array[3];
    p[2] = (uint16_t)roll_array[4] << 8 | roll_array[5] << 6 | roll_array[6] << 4 |
           roll_array[7] << 2 | roll_array[8];
    if (!secplus_v2_mix_order_encode(order, p)) return 0;
    if (!secplus_v2_mix_invet(invert, p)) return 0;
    for (int i = 0; i < 10; i++) {
        data <<= 3;
        data |= ((p[0] >> (9 - i)) & 1) << 2 | ((p[1] >> (9 - i)) & 1) << 1 | ((p[2] >> (9 - i)) & 1);
    }
    data |= ((uint64_t)order) << 34 | ((uint64_t)invert) << 30;
    return data;
}

// Merged biphase-Manchester body (MSB first). bit 1 = low,high; bit 0 = high,low;
// adjacent equal half-bits coalesce into one te_long pulse.
static std::vector<int> manchester_body(uint64_t value, int nbits, int te_short) {
    std::vector<int> t;
    for (int i = nbits; i > 0; i--) {
        bool b = (value >> (i - 1)) & 1;
        if (b) {
            if (!t.empty() && t.back() < 0) { t.back() -= te_short; t.push_back(te_short); }
            else { t.push_back(-te_short); t.push_back(te_short); }
        } else {
            if (!t.empty() && t.back() > 0) { t.back() += te_short; t.push_back(-te_short); }
            else { t.push_back(te_short); t.push_back(-te_short); }
        }
    }
    return t;
}

static std::vector<int> build_secplus_v2_upload(uint32_t serial, uint8_t btn, uint32_t rolling28) {
    uint32_t fixed_1 = (uint32_t)btn << 12 | (serial >> 20);
    uint32_t fixed_2 = serial & 0xFFFFF;
    uint8_t rolling_digits[18] = {0}, roll_1[9] = {0}, roll_2[9] = {0};

    uint32_t rolling = (uint32_t)reverse_bits(rolling28, 28);
    for (int i = 17; i >= 0; i--) { rolling_digits[i] = rolling % 3; rolling /= 3; }

    roll_2[8] = rolling_digits[0];  roll_1[8] = rolling_digits[1];
    roll_2[4] = rolling_digits[2];  roll_2[5] = rolling_digits[3];
    roll_2[6] = rolling_digits[4];  roll_2[7] = rolling_digits[5];
    roll_1[4] = rolling_digits[6];  roll_1[5] = rolling_digits[7];
    roll_1[6] = rolling_digits[8];  roll_1[7] = rolling_digits[9];
    roll_2[0] = rolling_digits[10]; roll_2[1] = rolling_digits[11];
    roll_2[2] = rolling_digits[12]; roll_2[3] = rolling_digits[13];
    roll_1[0] = rolling_digits[14]; roll_1[1] = rolling_digits[15];
    roll_1[2] = rolling_digits[16]; roll_1[3] = rolling_digits[17];

    uint64_t packet_1 = SECPLUS_V2_HEADER | SECPLUS_V2_PACKET_1 | secplus_v2_encode_half(roll_1, fixed_1);
    uint64_t packet_2 = SECPLUS_V2_HEADER | SECPLUS_V2_PACKET_2 | secplus_v2_encode_half(roll_2, fixed_2);

    const int te_short = 250, te_long = 500;
    std::vector<int> t = manchester_body(packet_1, 62, te_short);
    t.push_back(-(te_long * 136));
    std::vector<int> p2 = manchester_body(packet_2, 62, te_short);
    t.insert(t.end(), p2.begin(), p2.end());
    t.push_back(-(te_long * 136));
    return t;
}

std::vector<int> encode_security_plus_2(uint64_t data) {
    // Kept for the declared interface: data = serial<<28 | (rolling28). btn=0.
    return build_secplus_v2_upload((uint32_t)(data >> 28), 0, (uint32_t)(data & 0x0FFFFFFF));
}

// ---------------------------------------------------------------------------
// Somfy Telis / Keytis (ported from somfy_telis.c / somfy_keytis.c). Manchester
// over a scrambled frame; self-contained (no key). te_short = 640.
// ---------------------------------------------------------------------------
static std::vector<int> somfy_manchester(const uint8_t *frame, int nbits, uint8_t hw_sync) {
    const int te_short = 640;
    std::vector<int> t;
    t.push_back(9415);
    t.push_back(-89565);
    for (uint8_t i = 0; i < hw_sync; i++) { t.push_back(te_short * 4); t.push_back(-(te_short * 4)); }
    t.push_back(4550);
    t.push_back(-te_short);

    for (int i = nbits; i > 0; i--) {
        int pos = nbits - i; // 0 = MSB of frame[0]
        bool bitset = (frame[pos / 8] >> (7 - (pos % 8))) & 1;
        if (bitset) {
            if (t.back() < 0) { t.back() *= 2; t.push_back(te_short); }
            else { t.push_back(-te_short); t.push_back(te_short); }
        } else {
            if (t.back() > 0) { t.back() *= 2; t.push_back(-te_short); }
            else { t.push_back(te_short); t.push_back(-te_short); }
        }
    }
    if (t.back() < 0) t.back() -= 30415;
    else t.push_back(-30415);
    return t;
}

std::vector<int> encode_somfy(const uint8_t frame[7]) { return somfy_manchester(frame, 56, 2); }
std::vector<int> encode_somfy_keytis(const uint8_t frame[10]) { return somfy_manchester(frame, 80, 2); }

// ===========================================================================
// High-level dispatcher: crypto + frame assembly + encoder selection.
// ===========================================================================
static uint64_t mf_key_for(const RollingProtocol *proto, uint8_t *out_type) {
    uint64_t k = 0;
    uint8_t type = 0;
    if (proto->mf_key_name) rolling_mf_key_lookup(proto->mf_key_name, &k, &type);
    if (out_type) *out_type = type;
    return k;
}

std::vector<int> rolling_encode(RfCodes &data, const RollingProtocol *proto) {
    std::vector<int> timings;
    if (!proto) return timings;

    switch (proto->family) {
        case RF_FAMILY_KEELOQ: {
            // KeeLoq pipeline. data.mf_name (chosen at runtime) selects the key
            // and serial-masking branch in keeloq_step() — KeeLoq is one protocol,
            // manufacturer is a parameter, exactly as the Flipper handles it.
            data.fix = data.btn << 28 | data.serial;
            data.Bit = 64;
            data.keeloq_step(0); // recompute at current counter
            timings = encode_keeloq(data.key, proto->te_short_us, proto->te_long_us);
            break;
        }
        case RF_FAMILY_FAAC_SLH: {
            uint8_t type;
            uint64_t mfkey = mf_key_for(proto, &type);
            uint32_t fix = (data.serial << 4) | (data.btn & 0xF);
            uint8_t fixx[8];
            int shiftby = 32;
            for (int i = 0; i < 8; i++) fixx[i] = (fix >> (shiftby -= 4)) & 0xF;
            uint32_t decrypt;
            if ((data.cnt % 2) == 0)
                decrypt = (uint32_t)fixx[6] << 28 | (uint32_t)fixx[7] << 24 | (uint32_t)fixx[5] << 20 |
                          (data.cnt & 0xFFFFF);
            else
                decrypt = (uint32_t)fixx[2] << 28 | (uint32_t)fixx[3] << 24 | (uint32_t)fixx[4] << 20 |
                          (data.cnt & 0xFFFFF);
            uint64_t man = faac_slh_derive_key(data.seed, mfkey);
            uint32_t hop = keeloq_encrypt(decrypt, man);
            uint64_t data64 = (uint64_t)fix << 32 | hop;
            data.key = data64;
            data.Bit = 64;
            timings = encode_faac_slh(data64);
            break;
        }
        case RF_FAMILY_JAROLIFT: {
            uint8_t type;
            uint64_t mfkey = mf_key_for(proto, &type);
            uint32_t hop_decrypted = (uint32_t)((data.seed >> 8) & 0xFF) << 24 |
                                     ((data.serial & 0xFF) << 16) | (data.cnt & 0xFFFF);
            uint64_t man = keeloq_normal_learning(data.serial, mfkey);
            uint64_t hop_encrypted = keeloq_encrypt(hop_decrypted, man);
            uint64_t fix = (uint64_t)data.btn << 60 |
                           ((uint64_t)(data.serial & 0xFFFFFFF) << 32) | hop_encrypted;
            uint64_t frame = reverse_bits(fix, 64);
            uint8_t tail = (uint8_t)reverse_bits((data.seed & 0xFF), 8);
            data.key = frame;
            data.Bit = 72;
            timings = encode_jarolift(frame, tail);
            break;
        }
        case RF_FAMILY_NICE_FLOR_S: {
            data.Bit = 52;
            data.key = ((uint64_t)data.serial << 16) | data.cnt;
            timings = build_nice_upload(data.serial, data.cnt, data.btn, false);
            break;
        }
        case RF_FAMILY_NICE_ONE: {
            data.Bit = 72;
            data.key = ((uint64_t)data.serial << 16) | data.cnt;
            timings = build_nice_upload(data.serial, data.cnt, data.btn, true);
            break;
        }
        case RF_FAMILY_ALUTECH_AT4N: {
            uint8_t crc = alutech_data_crc((uint8_t)(data.cnt & 0xFF));
            uint64_t payload = (uint64_t)crc << 56 | (uint64_t)data.serial << 24 |
                               (uint32_t)data.cnt << 8 | (data.btn & 0xFF);
            data.crc_field = crc;
            data.key = payload;
            data.Bit = 72;
            timings = encode_alutech_at4n(payload);
            break;
        }
        case RF_FAMILY_BENINCA_ARC: {
            data.Bit = 128;
            timings = build_beninca_upload(data.serial, data.btn, data.cnt, (uint16_t)data.seed);
            break;
        }
        case RF_FAMILY_SECURITY_PLUS_1: {
            // Keyless rolling code. serial = fixed value; rolling counter mapped
            // into the protocol's valid window (>= 0xE6000000) via the 16-bit cnt.
            uint32_t fixed = data.serial;
            uint32_t rolling = 0xE6000000u + data.cnt;
            data.key = ((uint64_t)fixed << 32) | rolling;
            data.Bit = 64;
            timings = encode_security_plus_1(fixed, rolling);
            break;
        }
        case RF_FAMILY_SECURITY_PLUS_2: {
            // Keyless rolling code. 28-bit rolling counter mapped into the valid
            // window (>= 0xE500000) via the 16-bit cnt.
            uint32_t rolling = 0xE500000u + data.cnt;
            data.key = ((uint64_t)(data.serial & 0x0FFFFFFF) << 28) | rolling;
            data.Bit = 62;
            timings = build_secplus_v2_upload(data.serial & 0x0FFFFFFF, data.btn, rolling);
            break;
        }
        case RF_FAMILY_SOMFY_TELIS: {
            uint8_t frame[7];
            frame[0] = data.somfy_key ? data.somfy_key : 0xA7;
            frame[1] = (data.btn & 0xF) << 4;
            frame[2] = data.cnt >> 8;
            frame[3] = data.cnt;
            frame[4] = data.serial >> 16;
            frame[5] = data.serial >> 8;
            frame[6] = data.serial;
            uint8_t cks = 0;
            for (int i = 0; i < 7; i++) cks ^= frame[i] ^ (frame[i] >> 4);
            frame[1] |= (cks & 0xF);
            for (int i = 1; i < 7; i++) frame[i] ^= frame[i - 1];
            uint64_t d = 0;
            for (int i = 0; i < 7; i++) { d <<= 8; d |= frame[i]; }
            data.key = d;
            data.Bit = 56;
            timings = encode_somfy(frame);
            break;
        }
        case RF_FAMILY_SOMFY_KEYTIS: {
            uint8_t frame[10];
            frame[0] = (0xA << 4) | (data.btn & 0xF);
            frame[1] = 0xF << 4;
            frame[2] = data.cnt >> 8;
            frame[3] = data.cnt;
            frame[4] = data.serial >> 16;
            frame[5] = data.serial >> 8;
            frame[6] = data.serial;
            frame[7] = 0xC4;
            frame[8] = 0x00;
            frame[9] = 0x19;
            uint8_t cks = 0;
            for (int i = 0; i < 7; i++) cks ^= frame[i] ^ (frame[i] >> 4);
            frame[1] |= (cks & 0xF);
            for (int i = 1; i < 7; i++) frame[i] ^= frame[i - 1];
            uint64_t d = 0;
            for (int i = 0; i < 7; i++) { d <<= 8; d |= frame[i]; }
            data.key = d;
            data.Bit = 80;
            timings = encode_somfy_keytis(frame);
            break;
        }
    }
    return timings;
}

// ===========================================================================
// Decode (scan). Demodulates a captured pulse train for the identified family.
// ===========================================================================

// Demodulate a PWM bitstream. Walks (HIGH,LOW) pairs; a pair is '1' when the
// HIGH half is the long one (bit1_long_high), else '0'. Returns the last
// `want_bits` decoded bits packed MSB-first (skips header/preamble garbage).
static uint64_t pwm_demod(const std::vector<int> &p, int want_bits, bool bit1_long_high) {
    uint64_t bits = 0;
    int count = 0;
    for (size_t i = 0; i + 1 < p.size(); i += 2) {
        int hi = p[i] > 0 ? p[i] : -p[i];
        int lo = p[i + 1] < 0 ? -p[i + 1] : p[i + 1];
        if (hi < 80 || lo < 80) continue;        // skip noise/gaps
        if (hi > 4000 || lo > 4000) { continue; } // skip preamble/long gaps
        bool one = bit1_long_high ? (hi > lo) : (lo > hi);
        bits = (bits << 1) | (one ? 1 : 0);
        count++;
    }
    if (count < want_bits) return 0;
    return bits & ((want_bits >= 64) ? ~0ULL : ((1ULL << want_bits) - 1));
}

// Demodulate biphase-Manchester into a frame byte buffer (MSB first). Expands
// pulses into te_short half-slots, then reads pairs: low,high = 1; high,low = 0.
static int manchester_demod(const std::vector<int> &p, int te_short, uint8_t *out, int max_bytes) {
    // Build half-slot level sequence.
    std::vector<bool> slots;
    for (size_t i = 0; i < p.size(); i++) {
        int d = p[i] > 0 ? p[i] : -p[i];
        if (d > 5000) { if (!slots.empty()) break; else continue; } // frame boundary
        int n = (d + te_short / 2) / te_short;
        if (n < 1) n = 1;
        if (n > 4) continue; // preamble noise
        for (int k = 0; k < n; k++) slots.push_back(p[i] > 0);
    }
    int nbits = 0;
    for (size_t i = 0; i + 1 < slots.size() && nbits < max_bytes * 8; i += 2) {
        bool b = (!slots[i] && slots[i + 1]); // low,high = 1
        out[nbits / 8] = (out[nbits / 8] << 1) | (b ? 1 : 0);
        nbits++;
    }
    return nbits;
}

// Split a pulse train into segments at long (>5000µs) gaps.
static std::vector<std::vector<int>> split_segments(const std::vector<int> &p) {
    std::vector<std::vector<int>> segs;
    std::vector<int> cur;
    for (int d : p) {
        if ((d < 0 ? -d : d) > 5000) { if (!cur.empty()) { segs.push_back(cur); cur.clear(); } continue; }
        cur.push_back(d);
    }
    if (!cur.empty()) segs.push_back(cur);
    return segs;
}

// Manchester-demod one segment into an nbit value (MSB first). low,high = 1.
static uint64_t manchester_demod_value(const std::vector<int> &seg, int te_short, int nbits) {
    std::vector<bool> slots;
    for (int d : seg) {
        int a = d > 0 ? d : -d;
        int n = (a + te_short / 2) / te_short;
        if (n < 1) n = 1;
        if (n > 3) continue;
        for (int k = 0; k < n; k++) slots.push_back(d > 0);
    }
    uint64_t v = 0;
    int cnt = 0;
    for (size_t i = 0; i + 1 < slots.size() && cnt < nbits; i += 2) {
        v = (v << 1) | ((!slots[i] && slots[i + 1]) ? 1 : 0);
        cnt++;
    }
    return v;
}

// Tri-state demod for Security+ 1.0: 20 symbols, classified by the LOW duration
// (≈3te → 0, 2te → 1, 1te → 2). Fills out[1..20]. te = 500.
static bool secplus_v1_demod_packet(const std::vector<int> &seg, int te, uint8_t *out) {
    int sym = 0;
    for (size_t i = 0; i + 1 < seg.size() && sym < 20; i++) {
        if (seg[i] >= 0) continue;             // want a LOW pulse
        if (i + 1 >= seg.size() || seg[i + 1] <= 0) continue; // followed by HIGH
        int lo = -seg[i];
        int u = (lo + te / 2) / te;
        if (u < 1) u = 1;
        if (u > 3) continue;                   // header low, skip
        out[1 + sym] = (uint8_t)(3 - u);       // 3te→0, 2te→1, 1te→2
        sym++;
        i++;                                   // consume the HIGH half
    }
    return sym >= 20;
}

bool rolling_decode(RfCodes &data, const std::vector<int> &pulses) {
    const RollingProtocol *proto = rolling_protocol_by_name(data.rolling_protocol.c_str());
    if (!proto) return false;

    switch (proto->family) {
        case RF_FAMILY_FAAC_SLH: {
            uint64_t f = pwm_demod(pulses, 64, true);
            if (!f) return false;
            uint32_t fix = f >> 32;
            data.serial = fix >> 4;
            data.btn = fix & 0xF;
            data.key = f;
            // Counter sits in the encrypted hop; recover it if the key is loaded.
            uint64_t mfkey = 0;
            if (proto->mf_key_name && rolling_mf_key_lookup(proto->mf_key_name, &mfkey, nullptr) && mfkey) {
                uint64_t man = faac_slh_derive_key(data.seed, mfkey);
                uint32_t dec = keeloq_decrypt((uint32_t)(f & 0xFFFFFFFF), man);
                data.cnt = dec & 0xFFFF;
            }
            return true;
        }
        case RF_FAMILY_JAROLIFT: {
            uint64_t frame = pwm_demod(pulses, 72, false); // bit1 = short-high
            if (!frame) return false;
            uint64_t fix = reverse_bits(frame, 64);
            data.btn = fix >> 60;
            data.serial = (fix >> 32) & 0xFFFFFFF;
            data.key = frame;
            uint64_t mfkey = 0;
            if (proto->mf_key_name && rolling_mf_key_lookup(proto->mf_key_name, &mfkey, nullptr) && mfkey) {
                uint64_t man = keeloq_normal_learning(data.serial, mfkey);
                uint32_t hop = keeloq_decrypt((uint32_t)(fix & 0xFFFFFFFF), man);
                data.cnt = hop & 0xFFFF;
            }
            return true;
        }
        case RF_FAMILY_NICE_FLOR_S:
        case RF_FAMILY_NICE_ONE: {
            uint64_t frame = pwm_demod(pulses, 52, true);
            if (!frame) return false;
            data.key = frame;
            uint64_t dec = nice_flor_s_decrypt(frame & 0xFFFFFFFFFFFFULL);
            data.cnt = dec & 0xFFFF;
            data.serial = (dec >> 16) & 0xFFFFFFF;
            data.btn = (frame >> 48) & 0xF; // button is in the unencrypted upper nibble of the OTA frame
            return true;
        }
        case RF_FAMILY_ALUTECH_AT4N: {
            uint64_t frame = pwm_demod(pulses, 72, false);
            if (!frame) return false;
            data.key = frame;
            uint64_t dec = alutech_decrypt(reverse_bits(frame, 64));
            data.serial = (dec >> 24) & 0xFFFFFFFF;
            data.cnt = (dec >> 8) & 0xFFFF;
            data.btn = dec & 0xFF;
            return true;
        }
        case RF_FAMILY_BENINCA_ARC: {
            // Beninca ARC frame = two 64-bit halves separated by a ~9000µs gap.
            // split_segments() breaks on gaps > 5000µs, giving us each half separately.
            auto segs = split_segments(pulses);
            if (segs.size() < 2) return false;
            uint64_t d1 = pwm_demod(segs[segs.size() - 2], 64, true);
            uint64_t d2 = pwm_demod(segs[segs.size() - 1], 64, true);
            if (!d1 || !d2) return false;
            data.key = d1;
            uint8_t enc[16];
            for (int i = 0; i < 8; i++) enc[i]     = (d1 >> (56 - i * 8)) & 0xFF;
            for (int i = 0; i < 8; i++) enc[i + 8] = (d2 >> (56 - i * 8)) & 0xFF;
            reverse_bits_in_bytes(enc, 16);
            uint8_t expanded[176];
            aes_key_expansion(beninca_arc_aes_key, expanded);
            aes128_decrypt(expanded, enc);
            data.serial = (enc[0] << 24) | (enc[1] << 16) | (enc[2] << 8) | enc[3];
            data.btn = enc[4];
            data.cnt = (enc[12] << 8) | enc[13];
            return true;
        }
        case RF_FAMILY_SOMFY_TELIS:
        case RF_FAMILY_SOMFY_KEYTIS: {
            uint8_t frame[10] = {0};
            int n = manchester_demod(pulses, 640, frame, 10);
            if (n < 56) return false;
            for (int i = 6; i >= 1; i--) frame[i] ^= frame[i - 1]; // un-scramble
            data.somfy_key = frame[0];
            data.btn = frame[1] >> 4;
            data.cnt = (frame[2] << 8) | frame[3];
            data.serial = (frame[4] << 16) | (frame[5] << 8) | frame[6];
            return true;
        }
        case RF_FAMILY_SECURITY_PLUS_1: {
            auto segs = split_segments(pulses);
            if (segs.size() < 2) return false;
            uint8_t da[42] = {0};
            if (!secplus_v1_demod_packet(segs[segs.size() - 2], 500, da)) return false;
            uint8_t da2[42] = {0};
            if (!secplus_v1_demod_packet(segs[segs.size() - 1], 500, da2)) return false;
            for (int i = 0; i < 20; i++) da[22 + i] = da2[1 + i];
            uint32_t rolling = 0, fixed = 0, acc = 0;
            uint8_t digit;
            for (uint8_t i = 1; i < 21; i += 2) {
                digit = da[i]; rolling = rolling * 3 + digit; acc += digit;
                digit = (60 + da[i + 1] - acc) % 3; fixed = fixed * 3 + digit; acc += digit;
            }
            acc = 0;
            for (uint8_t i = 22; i < 42; i += 2) {
                digit = da[i]; rolling = rolling * 3 + digit; acc += digit;
                digit = (60 + da[i + 1] - acc) % 3; fixed = fixed * 3 + digit; acc += digit;
            }
            rolling = (uint32_t)reverse_bits(rolling, 32);
            data.serial = fixed;
            data.cnt = rolling & 0xFFFF;
            data.key = ((uint64_t)fixed << 32) | rolling;
            return true;
        }
        case RF_FAMILY_SECURITY_PLUS_2: {
            auto segs = split_segments(pulses);
            if (segs.size() < 2) return false;
            uint64_t pk1 = manchester_demod_value(segs[segs.size() - 2], 250, 62);
            uint64_t pk2 = manchester_demod_value(segs[segs.size() - 1], 250, 62);
            uint32_t fixed_1, fixed_2;
            uint8_t roll_1[9] = {0}, roll_2[9] = {0};
            if (!secplus_v2_decode_half(pk1, roll_1, &fixed_1)) return false;
            if (!secplus_v2_decode_half(pk2, roll_2, &fixed_2)) return false;
            uint8_t rd[18];
            rd[0] = roll_2[8]; rd[1] = roll_1[8];
            rd[2] = roll_2[4]; rd[3] = roll_2[5]; rd[4] = roll_2[6]; rd[5] = roll_2[7];
            rd[6] = roll_1[4]; rd[7] = roll_1[5]; rd[8] = roll_1[6]; rd[9] = roll_1[7];
            rd[10] = roll_2[0]; rd[11] = roll_2[1]; rd[12] = roll_2[2]; rd[13] = roll_2[3];
            rd[14] = roll_1[0]; rd[15] = roll_1[1]; rd[16] = roll_1[2]; rd[17] = roll_1[3];
            uint32_t rolling = 0;
            for (int i = 0; i < 18; i++) rolling = rolling * 3 + rd[i];
            if (rolling >= 0x10000000) return false;
            uint32_t cnt28 = (uint32_t)reverse_bits(rolling, 28);
            data.cnt = cnt28 & 0xFFFF;
            data.btn = fixed_1 >> 12;
            data.serial = fixed_1 << 20 | fixed_2;
            data.key = ((uint64_t)data.serial << 28) | cnt28;
            return true;
        }
        default:
            // KeeLoq is handled by the existing keeloq_identify path.
            return false;
    }
}

void rolling_randomize(RfCodes &data, const RollingProtocol *proto) {
    if (!proto) return;
    if (proto->serial_bytes) {
        uint32_t mask = (proto->serial_bytes >= 4) ? 0xFFFFFFFF
                                                    : ((1UL << (proto->serial_bytes * 8)) - 1);
        data.serial = esp_random() & mask;
    }
    if (proto->button_bytes) data.btn = (esp_random() & 0xFF) | 0x01;
    if (proto->counter_bytes) {
        uint32_t mask = (proto->counter_bytes >= 4) ? 0xFFFFFFFF
                                                     : ((1UL << (proto->counter_bytes * 8)) - 1);
        data.cnt = esp_random() & mask & 0xFFFF; // RfCodes::cnt is 16-bit
    }
    if (proto->has_seed && proto->seed_bytes) {
        uint32_t mask = (proto->seed_bytes >= 4) ? 0xFFFFFFFF
                                                  : ((1UL << (proto->seed_bytes * 8)) - 1);
        data.seed = esp_random() & mask;
    }
    if (proto->family == RF_FAMILY_SOMFY_TELIS || proto->family == RF_FAMILY_SOMFY_KEYTIS) {
        data.somfy_key = 0xA0 | (esp_random() & 0x0F);
        if (data.cnt == 0) data.cnt = 1;
    }
    if (proto->family == RF_FAMILY_SECURITY_PLUS_1) {
        data.serial &= 0xCFD41B90; // Security+ 1.0 fixed value max
    }
}
