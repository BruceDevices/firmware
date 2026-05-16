#include "Cryptography.h"

#include <string.h>

RadioLibAES128::RadioLibAES128() {

}

void RadioLibAES128::init(uint8_t* key) {
  this->keyPtr = key;
  this->keyExpansion(this->roundKey, key);
}

size_t RadioLibAES128::encryptECB(const uint8_t* in, size_t len, uint8_t* out) {
  size_t num_blocks = len / RADIOLIB_AES128_BLOCK_SIZE;
  if(len % RADIOLIB_AES128_BLOCK_SIZE) {
    num_blocks++;
  }

  memset(out, 0x00, RADIOLIB_AES128_BLOCK_SIZE * num_blocks);
  memcpy(out, in, len);

  for(size_t i = 0; i < num_blocks; i++) {
    this->cipher((state_t*)(out + (RADIOLIB_AES128_BLOCK_SIZE * i)), this->roundKey);
  }

  return(num_blocks*RADIOLIB_AES128_BLOCK_SIZE);
}

size_t RadioLibAES128::decryptECB(const uint8_t* in, size_t len, uint8_t* out) {
  size_t num_blocks = len / RADIOLIB_AES128_BLOCK_SIZE;
  if(len % RADIOLIB_AES128_BLOCK_SIZE) {
    num_blocks++;
  }

  memset(out, 0x00, RADIOLIB_AES128_BLOCK_SIZE * num_blocks);
  memcpy(out, in, len);

  for(size_t i = 0; i < num_blocks; i++) {
    this->decipher((state_t*)(out + (RADIOLIB_AES128_BLOCK_SIZE * i)), this->roundKey);
  }

  return(num_blocks*RADIOLIB_AES128_BLOCK_SIZE);
}

/*
 * CMAC streaming API
 *
 * Usage:
 *   RadioLibCMAC_State st;
 *   RadioLibAES128_initCMACState(&RadioLibAES128Instance, &st);
 *   RadioLibAES128_updateCMACState(&RadioLibAES128Instance, &st, chunk1, len1);
 *   RadioLibAES128_updateCMACState(&RadioLibAES128Instance, &st, chunk2, len2);
 *   uint8_t mac[16];
 *   RadioLibAES128_finishCMACState(&RadioLibAES128Instance, &st, mac);
 */

void RadioLibAES128::initCMAC(RadioLibCmacState* st) {
  if(!st) {
    return;
  }
  memset(st->X, 0x00, RADIOLIB_AES128_BLOCK_SIZE);
  memset(st->buffer, 0x00, RADIOLIB_AES128_BLOCK_SIZE);
  st->buffer_len = 0;
  st->subkeys_generated = false;
}

void RadioLibAES128::updateCMAC(RadioLibCmacState* st, const uint8_t* data, size_t len) {
  if(!st || (!data && len != 0)) {
    return;
  }

  // ensure subkeys are present
  if(!st->subkeys_generated) {
    this->generateSubkeys(st->k1, st->k2);
    st->subkeys_generated = true;
  }

  uint8_t tmp[RADIOLIB_AES128_BLOCK_SIZE];
  size_t offset = 0;

  while(len > 0) {

    // fill buffer up to one full block
    size_t to_copy = RADIOLIB_AES128_BLOCK_SIZE - st->buffer_len;
    if(to_copy > len) {
      to_copy = len;
    }

    // copy the data into the buffer
    memcpy(&st->buffer[st->buffer_len], &data[offset], to_copy);
    st->buffer_len += to_copy;
    offset += to_copy;
    len -= to_copy;

    // if we now have a full block AND there is still more input remaining,
    // this block is NOT the final one, so process it.
    if(st->buffer_len == RADIOLIB_AES128_BLOCK_SIZE && len > 0) {
      this->blockXor(tmp, st->buffer, st->X);
      this->encryptECB(tmp, RADIOLIB_AES128_BLOCK_SIZE, st->X);
      st->buffer_len = 0;
    }
  }
}

void RadioLibAES128::finishCMAC(RadioLibCmacState* st, uint8_t* out) {
  if(!st || !out) {
    return;
  }

  // ensure subkeys are present
  if(!st->subkeys_generated) {
    this->generateSubkeys(st->k1, st->k2);
    st->subkeys_generated = true;
  }

  uint8_t last[RADIOLIB_AES128_BLOCK_SIZE];
  uint8_t Y[RADIOLIB_AES128_BLOCK_SIZE];

  if(st->buffer_len == RADIOLIB_AES128_BLOCK_SIZE) {
    this->blockXor(last, st->buffer, st->k1);
  } else {
    memset(last, 0x00, RADIOLIB_AES128_BLOCK_SIZE);
    if(st->buffer_len > 0) {
      memcpy(last, st->buffer, st->buffer_len);
    }
    last[st->buffer_len] = 0x80;
    this->blockXor(last, last, st->k2);
  }

  this->blockXor(Y, last, st->X);
  this->encryptECB(Y, RADIOLIB_AES128_BLOCK_SIZE, out);
}

void RadioLibAES128::generateCMAC(const uint8_t* in, size_t len, uint8_t* cmac) {
  RadioLibCmacState st;
  this->initCMAC(&st);
  this->updateCMAC(&st, in, len);
  this->finishCMAC(&st, cmac);
}

bool RadioLibAES128::verifyCMAC(const uint8_t* in, size_t len, const uint8_t* cmac) {
  uint8_t cmacReal[RADIOLIB_AES128_BLOCK_SIZE];
  this->generateCMAC(in, len, cmacReal);
  for(size_t i = 0; i < RADIOLIB_AES128_BLOCK_SIZE; i++) {
    if((cmacReal[i] != cmac[i])) {
      return(false);
    }
  }
  return(true);
}

void RadioLibAES128::keyExpansion(uint8_t* roundKey, const uint8_t* key) {
  uint8_t tmp[4];

  // the first round key is the key itself
  for(uint8_t i = 0; i < RADIOLIB_AES128_N_K; i++) {
    for(uint8_t j = 0; j < 4; j++) {
      roundKey[(i * 4) + j] = key[(i * 4) + j];
    }
  }

  // All other round keys are found from the previous round keys.
  for(uint8_t i = RADIOLIB_AES128_N_K; i < RADIOLIB_AES128_N_B * (RADIOLIB_AES128_N_R + 1); ++i) {
    uint8_t j = (i - 1) * 4;
    for(uint8_t k = 0; k < 4; k++) {
      tmp[k] = roundKey[j + k];
    }

    if(i % RADIOLIB_AES128_N_K == 0) {
      this->rotWord(tmp);
      this->subWord(tmp);
      tmp[0] = tmp[0] ^ aesRcon[i/RADIOLIB_AES128_N_K];
    }

    j = i * 4;
    uint8_t k = (i - RADIOLIB_AES128_N_K) * 4;
    for(uint8_t l = 0; l < 4; l++) {
      roundKey[j + l] = roundKey[k + l] ^ tmp[l];
    }
  }
}

void RadioLibAES128::cipher(state_t* state, uint8_t* roundKey) {
  this->addRoundKey(0, state, roundKey);
  for(uint8_t round = 1; round < RADIOLIB_AES128_N_R; round++) {
    this->subBytes(state, aesSbox);
    this->shiftRows(state, false);
    this->mixColumns(state, false);
    this->addRoundKey(round, state, roundKey);
  }

  this->subBytes(state, aesSbox);
  this->shiftRows(state, false);
  this->addRoundKey(RADIOLIB_AES128_N_R, state, roundKey);
}


void RadioLibAES128::decipher(state_t* state, uint8_t* roundKey) {
  this->addRoundKey(RADIOLIB_AES128_N_R, state, roundKey);
  for(uint8_t round = RADIOLIB_AES128_N_R - 1; round > 0; --round) {
    this->shiftRows(state, true);
    this->subBytes(state, aesSboxInv);
    this->addRoundKey(round, state, roundKey);
    this->mixColumns(state, true);
  }

  this->shiftRows(state, true);
  this->subBytes(state, aesSboxInv);
  this->addRoundKey(0, state, roundKey);
}

void RadioLibAES128::subWord(uint8_t* word) {
  for(size_t i = 0; i < 4; i++) {
    uint8_t* ptr = const_cast<uint8_t*>(&aesSbox[word[i]]);
    word[i] = RADIOLIB_NONVOLATILE_READ_BYTE(ptr);
  }
}

void RadioLibAES128::rotWord(uint8_t* word) {
  uint8_t tmp[4];
  memcpy(tmp, word, 4);
  for(size_t i = 0; i < 4; i++) {
    word[i] = tmp[(i + 1) % 4];
  }
}

void RadioLibAES128::addRoundKey(uint8_t round, state_t* state, const uint8_t* roundKey) {
  for(size_t row = 0; row < 4; row++) {
    for(size_t col = 0; col < 4; col++) {
      (*state)[row][col] ^= roundKey[(round * RADIOLIB_AES128_N_B * 4) + (row * RADIOLIB_AES128_N_B) + col];
    }
  }
}

void RadioLibAES128::blockXor(uint8_t* dst, const uint8_t* a, const uint8_t* b) {
  for(uint8_t j = 0; j < RADIOLIB_AES128_BLOCK_SIZE; j++) {
    dst[j] = a[j] ^ b[j];
  }
}

void RadioLibAES128::blockLeftshift(uint8_t* dst, const uint8_t* src) {
  uint8_t ovf = 0x00;
  for(int8_t i = RADIOLIB_AES128_BLOCK_SIZE - 1; i >= 0; i--) {
    dst[i] = src[i] << 1;
    dst[i] |= ovf;
    ovf = (src[i] & 0x80) ? 1 : 0;
  }
}

void RadioLibAES128::generateSubkeys(uint8_t* key1, uint8_t* key2) {
  const uint8_t const_Zero[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
  };

  const uint8_t const_Rb[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x87
  };

  uint8_t L[RADIOLIB_AES128_BLOCK_SIZE];
  this->encryptECB(const_Zero, RADIOLIB_AES128_BLOCK_SIZE, L);
  this->blockLeftshift(key1, L);
  if(L[0] & 0x80) {
    this->blockXor(key1, key1, const_Rb);
  }

  this->blockLeftshift(key2, key1);
  if(key1[0] & 0x80) {
    this->blockXor(key2, key2, const_Rb);
  }
}

void RadioLibAES128::subBytes(state_t* state, const uint8_t* box) {
  for(size_t row = 0; row < 4; row++) {
    for(size_t col = 0; col < 4; col++) {
      uint8_t* ptr = const_cast<uint8_t*>(&box[(*state)[col][row]]);
      (*state)[col][row] = RADIOLIB_NONVOLATILE_READ_BYTE(ptr);
    }
  }
}

void RadioLibAES128::shiftRows(state_t* state, bool inv) {
  uint8_t tmp[4];
  for(size_t row = 1; row < 4; row++) {
    for(size_t col = 0; col < 4; col++) {
      if(!inv) {
        tmp[col] = (*state)[(row + col) % 4][row];
      } else {
        tmp[(row + col) % 4] = (*state)[col][row];
      }
    }
    for(size_t col = 0; col < 4; col++) {
      (*state)[col][row] = tmp[col];
    }
  }
}

void RadioLibAES128::mixColumns(state_t* state, bool inv) {
  uint8_t tmp[4];
  uint8_t matmul[][4] = {
    0x02, 0x03, 0x01, 0x01,
    0x01, 0x02, 0x03, 0x01,
    0x01, 0x01, 0x02, 0x03,
    0x03, 0x01, 0x01, 0x02
  };
  if(inv) {
    uint8_t matmul_inv[][4] = {
      0x0e, 0x0b, 0x0d, 0x09,
      0x09, 0x0e, 0x0b, 0x0d,
      0x0d, 0x09, 0x0e, 0x0b,
      0x0b, 0x0d, 0x09, 0x0e
    };
    memcpy(matmul, matmul_inv, sizeof(matmul_inv));
  }

  for(size_t col = 0; col < 4; col++) {
    for(size_t row = 0; row < 4; row++) {
      tmp[row] = (*state)[col][row];
    }
    for(size_t i = 0; i < 4; i++) {
      (*state)[col][i] = 0x00;
      for(size_t j = 0; j < 4; j++) {
        (*state)[col][i] ^= mul(matmul[i][j], tmp[j]);
      }
    }
  }
}

uint8_t RadioLibAES128::mul(uint8_t a, uint8_t b) {
  uint8_t sb[4];
  uint8_t out = 0;
  sb[0] = b;
  for(size_t i = 1; i < 4; i++) {
    sb[i] = sb[i - 1] << 1;
    if (sb[i - 1] & 0x80) {
      sb[i] ^= 0x1b;
    }
  }
  for(size_t i = 0; i < 4; i++) {
    if(a >> i & 0x01) {
      out ^= sb[i];
    }
  }
  return(out);
}

RadioLibAES128 RadioLibAES128Instance;