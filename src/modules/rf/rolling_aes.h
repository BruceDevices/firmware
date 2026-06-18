#ifndef __ROLLING_AES_H__
#define __ROLLING_AES_H__

#include <stdint.h>

// Pure-software AES-128 (ECB). Ported from Momentum's aes_common.c, with all
// Flipper runtime dependencies removed. Depends only on stdint/string.

// Expand a 16-byte key into 176 bytes (11 round keys).
void aes_key_expansion(const uint8_t *key, uint8_t *round_keys);

// Encrypt/decrypt one 16-byte block in place using an already-expanded key.
void aes128_encrypt(const uint8_t *expanded_key, uint8_t *data);
void aes128_decrypt(const uint8_t *expanded_key, uint8_t *data);

// Reverse the bit order within each of `len` bytes (used by Beninca ARC).
void reverse_bits_in_bytes(uint8_t *data, uint8_t len);

// Convenience one-shot ECB block encrypt: out = AES128(key, in).
void rolling_aes_ecb_encrypt(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);
void rolling_aes_ecb_decrypt(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);

#endif
