/**
 * @file crypto1.h
 * @brief Compact Crypto1 cipher for MIFARE Classic (reader side).
 *
 * Re-implementation of the public Crypto1 stream cipher (as described in
 * "Dismantling MIFARE Classic", Garcia et al. 2008, and the public Crapto1
 * reference). Used by the ST25R3916 driver to authenticate, read and write
 * MIFARE Classic sectors, since the RFAL fork has no native Crypto1 support.
 *
 * The ST25R3916 must run with manual parity (PAR_TX_NONE / PAR_RX_KEEP) so the
 * encrypted ISO14443-A parity bits can be supplied/consumed by software.
 */
#pragma once
#include <stdint.h>

struct Crypto1State {
    uint32_t odd;
    uint32_t even;
};

/** Initialise the cipher state from a 48-bit key (top 16 bits of the uint64 ignored). */
void crypto1_init(Crypto1State *state, uint64_t key);

/** Clock the cipher one bit. Returns the keystream bit. */
uint8_t crypto1_bit(Crypto1State *state, uint8_t in, int is_encrypted);

/** Clock the cipher one byte (LSB first). Returns the keystream byte. */
uint8_t crypto1_byte(Crypto1State *state, uint8_t in, int is_encrypted);

/** Clock the cipher one 32-bit word (big-endian bit order). Returns keystream word. */
uint32_t crypto1_word(Crypto1State *state, uint32_t in, int is_encrypted);

/** Current filter output (next keystream bit) — used to compute encrypted parity. */
uint8_t crypto1_filter_bit(const Crypto1State *state);

/** MIFARE LCG nonce successor: advances PRNG state x by n cycles. */
uint32_t prng_successor(uint32_t x, uint32_t n);

/** ISO14443-A odd parity bit for a data byte (1 when byte has even number of set bits). */
uint8_t nfc_oddparity(uint8_t b);
