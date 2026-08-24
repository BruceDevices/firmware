#include "crypto1.h"

#define LF_POLY_ODD  (0x29CE5Cu)
#define LF_POLY_EVEN (0x870804u)

#define C1_BIT(x, n)   ((uint32_t)(((uint64_t)(x)) >> (n)) & 1u)
#define C1_BEBIT(x, n) C1_BIT((x), (n) ^ 24)

// Crypto1 non-linear filter function f(x).
static inline uint32_t crypto1_filter(uint32_t x) {
    uint32_t f;
    f = 0xf22c0u >> (x & 0xfu) & 16u;
    f |= 0x6c9c0u >> ((x >> 4) & 0xfu) & 8u;
    f |= 0x3c8b0u >> ((x >> 8) & 0xfu) & 4u;
    f |= 0x1e458u >> ((x >> 12) & 0xfu) & 2u;
    f |= 0x0d938u >> ((x >> 16) & 0xfu) & 1u;
    return C1_BIT(0xEC57E80Au, f);
}

// Parity of the masked feedback value (LFSR feedback bit).
static inline uint32_t crypto1_parity(uint32_t x) {
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    return C1_BIT(0x6996u, x & 0xfu);
}

void crypto1_init(Crypto1State *s, uint64_t key) {
    s->odd = 0;
    s->even = 0;
    for (int i = 47; i > 0; i -= 2) {
        s->odd = (s->odd << 1) | C1_BIT(key, (i - 1) ^ 7);
        s->even = (s->even << 1) | C1_BIT(key, i ^ 7);
    }
}

uint8_t crypto1_bit(Crypto1State *s, uint8_t in, int is_encrypted) {
    uint8_t ret = (uint8_t)crypto1_filter(s->odd);

    uint32_t feedin = (uint32_t)(ret & (is_encrypted ? 1u : 0u));
    feedin ^= (uint32_t)(in ? 1u : 0u);
    feedin ^= LF_POLY_ODD & s->odd;
    feedin ^= LF_POLY_EVEN & s->even;
    s->even = (s->even << 1) | crypto1_parity(feedin);

    uint32_t t = s->odd;
    s->odd = s->even;
    s->even = t;

    return ret;
}

uint8_t crypto1_byte(Crypto1State *s, uint8_t in, int is_encrypted) {
    uint8_t ret = 0;
    for (int i = 0; i < 8; i++) { ret |= (uint8_t)(crypto1_bit(s, (uint8_t)C1_BIT(in, i), is_encrypted) << i); }
    return ret;
}

uint32_t crypto1_word(Crypto1State *s, uint32_t in, int is_encrypted) {
    uint32_t ret = 0;
    for (int i = 0; i < 32; i++) {
        ret |= (uint32_t)crypto1_bit(s, (uint8_t)C1_BEBIT(in, i), is_encrypted) << (24 ^ i);
    }
    return ret;
}

uint8_t crypto1_filter_bit(const Crypto1State *s) { return (uint8_t)crypto1_filter(s->odd); }

static inline uint32_t crypto1_swapendian(uint32_t x) {
    x = (x >> 8 & 0xff00ffu) | ((x & 0xff00ffu) << 8);
    x = (x >> 16) | (x << 16);
    return x;
}

uint32_t prng_successor(uint32_t x, uint32_t n) {
    x = crypto1_swapendian(x);
    while (n--) { x = (x >> 1) | ((((x >> 16) ^ (x >> 18) ^ (x >> 19) ^ (x >> 21)) & 1u) << 31); }
    return crypto1_swapendian(x);
}

uint8_t nfc_oddparity(uint8_t b) {
    b ^= b >> 4;
    b ^= b >> 2;
    b ^= b >> 1;
    return (uint8_t)((b & 1u) ^ 1u);
}
