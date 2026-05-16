#include "SX126x.h"
#include <string.h>
#include <math.h>
#if !RADIOLIB_EXCLUDE_SX126X

/*
  LR-FHSS implementation in this file is adapted from Setmech's LR-FHSS demo:
  https://github.com/Lora-net/SWDM001/tree/master/lib/sx126x_driver

  Its SX126x driver is distributed under the Clear BSD License,
  and to comply with its terms, it is reproduced below.

  The Clear BSD License
  Copyright Semtech Corporation 2021. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted (subject to the limitations in the disclaimer
  below) provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the Semtech corporation nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
  THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
  NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

// header interleaver
static const uint8_t LrFhssHeaderInterleaver[80] = {
  0,  18, 36, 54, 72, 4,  22, 40,
  58, 76, 8,  26, 44, 62, 12, 30,
  48, 66, 16, 34, 52, 70, 1,  19,
  37, 55, 73, 5,  23, 41, 59, 77,
  9,  27, 45, 63, 13, 31, 49, 67,
  17, 35, 53, 71, 2,  20, 38, 56,
  74, 6,  24, 42, 60, 78, 10, 28,
  46, 64, 14, 32, 50, 68, 3,  21,
  39, 57, 75, 7,  25, 43, 61, 79,
  11, 29, 47, 65, 15, 33, 51, 69,
};

int16_t SX126x::buildLRFHSSPacket(const uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len, size_t* out_bits, size_t* out_hops) {
  // perform payload whitening
  uint8_t lfsr = 0xFF;
  for(size_t i = 0; i < in_len; i++) {
    uint8_t u = in[i] ^ lfsr;

    // we really shouldn't reuse the caller's memory in this way ...
    // but since this is a private method it should be at least controlled, if not safe
    out[i] = ((u & 0x0F) << 4 ) | ((u & 0xF0) >> 4);
    lfsr = (lfsr << 1) | (((lfsr & 0x80) >> 7) ^ (((lfsr & 0x20) >> 5) ^ (((lfsr & 0x10) >> 4) ^ ((lfsr & 0x08) >> 3))));
  }

  // calculate the CRC-16 over the whitened data, looks like something custom
  RadioLibCRCInstance.size = 16;
  RadioLibCRCInstance.poly = 0x755B;
  RadioLibCRCInstance.init = 0xFFFF;
  RadioLibCRCInstance.out = 0x0000;
  uint16_t crc16 = RadioLibCRCInstance.checksum(out, in_len);

  // add payload CRC
  out[in_len] = (crc16 >> 8) & 0xFF;
  out[in_len + 1] = crc16 & 0xFF;
  out[in_len + 2] = 0;

  // encode the payload with CRC using convolutional coding with 1/3 rate into a temporary buffer
  uint8_t tmp[RADIOLIB_SX126X_LR_FHSS_MAX_ENC_SIZE] = { 0 };
  size_t nb_bits = 0;
  RadioLibConvCodeInstance.begin(3);
  RadioLibConvCodeInstance.encode(out, 8 * (in_len + 2) + 6, tmp, &nb_bits);
  memset(out, 0, RADIOLIB_SX126X_MAX_PACKET_LENGTH);

  // for rates other than the 1/3 base, puncture the code
  if(this->lrFhssCr != RADIOLIB_SX126X_LR_FHSS_CR_1_3) {
    uint32_t matrix_index = 0;
    const uint8_t matrix[15]   = { 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0 };
    uint8_t  matrix_len   = 0;
    switch(this->lrFhssCr) {
      case RADIOLIB_SX126X_LR_FHSS_CR_5_6:
        matrix_len = 15;
        break;
      case RADIOLIB_SX126X_LR_FHSS_CR_2_3:
        matrix_len = 6;
        break;
      case RADIOLIB_SX126X_LR_FHSS_CR_1_2:
        matrix_len = 3;
        break;
    }

    uint32_t j = 0;
    for(uint32_t i = 0; i < nb_bits; i++) {
      if(matrix[matrix_index]) {
        if(TEST_BIT_IN_ARRAY_LSB(tmp, i)) {
          SET_BIT_IN_ARRAY_LSB(out, j);
        } else {
          CLEAR_BIT_IN_ARRAY_LSB(out, j);
        }
        j++;
      }

      if(++matrix_index == matrix_len) {
        matrix_index = 0;
      }
    }

    nb_bits = j;
    memcpy(tmp, out, (nb_bits + 7) / 8);
  }

  // interleave the payload into output buffer
  uint16_t step = 0;
  while((size_t)(step * step) < nb_bits) {
    // probably the silliest sqrt() I ever saw
    step++;
  }

  const uint16_t step_v = step >> 1;
  step <<= 1;

  uint16_t pos           = 0;
  uint16_t st_idx        = 0;
  uint16_t st_idx_init   = 0;
  int16_t  bits_left     = nb_bits;
  uint16_t out_row_index = RADIOLIB_SX126X_LR_FHSS_HEADER_BITS * this->lrFhssHdrCount;

  while(bits_left > 0) {
    int16_t in_row_width = bits_left;
    if(in_row_width > RADIOLIB_SX126X_LR_FHSS_FRAG_BITS) {
      in_row_width = RADIOLIB_SX126X_LR_FHSS_FRAG_BITS;
    }

    // guard bits
    CLEAR_BIT_IN_ARRAY_LSB(out, out_row_index);
    CLEAR_BIT_IN_ARRAY_LSB(out, out_row_index + 1);
        
    for(int16_t j = 0; j < in_row_width; j++) {
      // guard bit
      if(TEST_BIT_IN_ARRAY_LSB(tmp, pos)) {
        SET_BIT_IN_ARRAY_LSB(out, j + 2 + out_row_index);
      } else {
        CLEAR_BIT_IN_ARRAY_LSB(out, j + 2 + out_row_index);
      }

      pos += step;
      if(pos >= nb_bits) {
        st_idx += step_v;
        if(st_idx >= step) {
          st_idx_init++;
          st_idx = st_idx_init;
        }
        pos = st_idx;
      }
    }

    bits_left -= RADIOLIB_SX126X_LR_FHSS_FRAG_BITS;
    out_row_index += 2 + in_row_width;
  }

  nb_bits = out_row_index - RADIOLIB_SX126X_LR_FHSS_HEADER_BITS * this->lrFhssHdrCount;

  // build the header
  uint8_t raw_header[RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2];
  raw_header[0] = in_len;
  raw_header[1] = (this->lrFhssCr << 3) | ((uint8_t)this->lrFhssGridNonFcc << 2) |
    (RADIOLIB_SX126X_LR_FHSS_HOPPING_ENABLED << 1) | (this->lrFhssBw >> 3);
  raw_header[2] = ((this->lrFhssBw & 0x07) << 5) | (this->lrFhssHopSeqId >> 4);
  raw_header[3] = ((this->lrFhssHopSeqId & 0x000F) << 4);

  // CRC-8 used seems to based on 8H2F, but without final XOR
  RadioLibCRCInstance.size = 8;
  RadioLibCRCInstance.poly = 0x2F;
  RadioLibCRCInstance.init = 0xFF;
  RadioLibCRCInstance.out = 0x00;

  uint16_t header_offset = 0;
  for(size_t i = 0; i < this->lrFhssHdrCount; i++) {
    // insert index and calculate the header CRC
    raw_header[3] = (raw_header[3] & ~0x0C) | ((this->lrFhssHdrCount - i - 1) << 2);
    raw_header[4] = RadioLibCRCInstance.checksum(raw_header, (RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2 - 1));

    // convolutional encode
    uint8_t coded_header[RADIOLIB_SX126X_LR_FHSS_HDR_BYTES] = { 0 };
    RadioLibConvCodeInstance.begin(2);
    RadioLibConvCodeInstance.encode(raw_header, 8*RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2, coded_header);
    // tail-biting seems to just do this twice ...?
    RadioLibConvCodeInstance.encode(raw_header, 8*RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2, coded_header);

    // clear guard bits
    CLEAR_BIT_IN_ARRAY_LSB(out, header_offset);
    CLEAR_BIT_IN_ARRAY_LSB(out, header_offset + 1);

    // interleave the header directly to the physical payload buffer
    for(size_t j = 0; j < (8*RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2); j++) {
      if(TEST_BIT_IN_ARRAY_LSB(coded_header, LrFhssHeaderInterleaver[j])) {
        SET_BIT_IN_ARRAY_LSB(out, header_offset + 2 + j);
      } else {
        CLEAR_BIT_IN_ARRAY_LSB(out, header_offset + 2 + j);
      }
    }
    for(size_t j = 0; j < (8*RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2); j++) {
      if(TEST_BIT_IN_ARRAY_LSB(coded_header, LrFhssHeaderInterleaver[(8*RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2) + j])) {
        SET_BIT_IN_ARRAY_LSB(out, header_offset + 2 + (8*RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2) + (8*RADIOLIB_SX126X_LR_FHSS_SYNC_WORD_BYTES) + j);
      } else {
        CLEAR_BIT_IN_ARRAY_LSB(out, header_offset + 2 + (8*RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2) + (8*RADIOLIB_SX126X_LR_FHSS_SYNC_WORD_BYTES) + j);
      }
    }

    // copy the sync word to the physical payload buffer
    for(size_t j = 0; j < (8*RADIOLIB_SX126X_LR_FHSS_SYNC_WORD_BYTES); j++) {
      if(TEST_BIT_IN_ARRAY_LSB(this->lrFhssSyncWord, j)) {
        SET_BIT_IN_ARRAY_LSB(out, header_offset + 2 + (8*RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2) + j);
      } else {
        CLEAR_BIT_IN_ARRAY_LSB(out, header_offset + 2 + (8*RADIOLIB_SX126X_LR_FHSS_HDR_BYTES/2) + j);
      }
    }

    header_offset += RADIOLIB_SX126X_LR_FHSS_HEADER_BITS;
  }

  // calculate the number of hops and total number of bits
  uint16_t length_bits = (in_len + 2) * 8 + 6;
  switch(this->lrFhssCr) {
    case RADIOLIB_SX126X_LR_FHSS_CR_5_6:
      length_bits = ( ( length_bits * 6 ) + 4 ) / 5;
      break;
    case RADIOLIB_SX126X_LR_FHSS_CR_2_3:
      length_bits = length_bits * 3 / 2;
      break;
    case RADIOLIB_SX126X_LR_FHSS_CR_1_2:
      length_bits = length_bits * 2;
      break;
    case RADIOLIB_SX126X_LR_FHSS_CR_1_3:
      length_bits = length_bits * 3;
      break;
  }

  *out_hops = (length_bits + 47) / 48 + this->lrFhssHdrCount;

  // calculate total number of payload bits, after breaking into blocks
  uint16_t payload_bits = length_bits / RADIOLIB_SX126X_LR_FHSS_FRAG_BITS * RADIOLIB_SX126X_LR_FHSS_BLOCK_BITS;
  uint16_t last_block_bits = length_bits % RADIOLIB_SX126X_LR_FHSS_FRAG_BITS;
  if(last_block_bits > 0) {
    // add the 2 guard bits for the last block + the actual remaining payload bits
    payload_bits += last_block_bits + 2;
  }

  *out_bits = (RADIOLIB_SX126X_LR_FHSS_HEADER_BITS * this->lrFhssHdrCount) + payload_bits;
  *out_len = (*out_bits + 7) / 8;

  return(RADIOLIB_ERR_NONE);
}

int16_t SX126x::resetLRFHSS() {
  // initialize hopping configuration
  const uint16_t numChan[] = { 80, 176, 280, 376, 688, 792, 1480, 1584, 3120, 3224 };
  
  // LFSR polynomials for different ranges of lrFhssNgrid
  const uint8_t lfsrPoly1[] = { 33, 45, 48, 51, 54, 57 };
  const uint8_t lfsrPoly2[] = { 65, 68, 71, 72 };
  const uint8_t lfsrPoly3[] = { 142, 149 };

  uint32_t nb_channel_in_grid = this->lrFhssGridNonFcc ? 8 : 52;
  this->lrFhssNgrid = numChan[this->lrFhssBw] / nb_channel_in_grid;
  this->lrFhssLfsrState = 6;
  switch(this->lrFhssNgrid) {
    case 10:
    case 22:
    case 28:
    case 30:
    case 35:
    case 47:
      this->lrFhssPoly = lfsrPoly1[this->lrFhssHopSeqId >> 6];
      this->lrFhssSeed = this->lrFhssHopSeqId & 0x3F;
      if(this->lrFhssHopSeqId >= 384) {
        return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
      }
      break;
    
    case 60:
    case 62:
      this->lrFhssLfsrState = 56;
      this->lrFhssPoly = lfsrPoly1[this->lrFhssHopSeqId >> 6];
      this->lrFhssSeed = this->lrFhssHopSeqId & 0x3F;
      if(this->lrFhssHopSeqId >= 384) {
        return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
      }
      break;
    
    case 86:
    case 99:
      this->lrFhssPoly = lfsrPoly2[this->lrFhssHopSeqId >> 7];
      this->lrFhssSeed = this->lrFhssHopSeqId & 0x7F;
      break;
    
    case 185:
    case 198:
      this->lrFhssPoly = lfsrPoly3[this->lrFhssHopSeqId >> 8];
      this->lrFhssSeed = this->lrFhssHopSeqId & 0xFF;
      break;
    
    case 390:
    case 403:
      this->lrFhssPoly = 264;
      this->lrFhssSeed = this->lrFhssHopSeqId;
      break;
    
    default:
      return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
  }

  return(RADIOLIB_ERR_NONE);
}

uint16_t SX126x::stepLRFHSS() {
  uint16_t hop;
  do {
    uint16_t lsb = this->lrFhssLfsrState & 1;
    this->lrFhssLfsrState >>= 1;
    if(lsb) {
     this->lrFhssLfsrState ^= this->lrFhssPoly;
    }
    hop = this->lrFhssSeed;
    if(hop != this->lrFhssLfsrState) {
      hop ^= this->lrFhssLfsrState;
    }
  } while(hop > this->lrFhssNgrid);
  return(hop);
}

int16_t SX126x::setLRFHSSHop(uint8_t index) {
  if(!this->lrFhssFrameHopsRem) {
    return(RADIOLIB_ERR_NONE);
  }

  uint16_t hop = stepLRFHSS();
  int16_t freq_table = hop - 1;
  if(freq_table >= (int16_t)(this->lrFhssNgrid >> 1)) {
    freq_table -= this->lrFhssNgrid;
  }

  uint32_t nb_channel_in_grid = this->lrFhssGridNonFcc ? 8 : 52;
  uint32_t grid_offset = (1 + (this->lrFhssNgrid % 2)) * (nb_channel_in_grid / 2);
  uint32_t grid_in_pll_steps = this->lrFhssGridNonFcc ? 4096 : 26624;
  uint32_t frf = (this->freqMHz * (uint32_t(1) << RADIOLIB_SX126X_DIV_EXPONENT)) / RADIOLIB_SX126X_CRYSTAL_FREQ;
  uint32_t freq_raw = frf - freq_table * grid_in_pll_steps - grid_offset * 512;

  if((this->lrFhssHopNum < this->lrFhssHdrCount)) {
    if((((this->lrFhssHdrCount - this->lrFhssHopNum) % 2) == 0)) {
      freq_raw += 256;
    }
  }

  const uint8_t frq[4] = { (uint8_t)((freq_raw >> 24) & 0xFF), (uint8_t)((freq_raw >> 16) & 0xFF), (uint8_t)((freq_raw >> 8) & 0xFF), (uint8_t)(freq_raw & 0xFF) };
  int16_t state = writeRegister(RADIOLIB_SX126X_REG_LR_FHSS_FREQX_0(index), frq, sizeof(freq_raw));
  RADIOLIB_ASSERT(state);

  // (LR_FHSS_HEADER_BITS + pulse_shape_compensation) symbols on first sync_word, LR_FHSS_HEADER_BITS on
  // next sync_words, LR_FHSS_BLOCK_BITS on payload
  uint16_t numSymbols = RADIOLIB_SX126X_LR_FHSS_BLOCK_BITS;
  if(index == 0) {
    numSymbols = RADIOLIB_SX126X_LR_FHSS_HEADER_BITS + 1; // the +1 is "pulse_shape_compensation", but it's constant in the demo
  } else if(index < this->lrFhssHdrCount) {
    numSymbols = RADIOLIB_SX126X_LR_FHSS_HEADER_BITS;
  } else if(this->lrFhssFrameBitsRem < RADIOLIB_SX126X_LR_FHSS_BLOCK_BITS) {
    numSymbols = this->lrFhssFrameBitsRem;
  }

  // write hop length in symbols
  const uint8_t sym[2] = { (uint8_t)((numSymbols >> 8) & 0xFF), (uint8_t)(numSymbols & 0xFF) };
  state = writeRegister(RADIOLIB_SX126X_REG_LR_FHSS_NUM_SYMBOLS_FREQX_MSB(index), sym, sizeof(uint16_t));
  RADIOLIB_ASSERT(state);

  this->lrFhssFrameBitsRem -= numSymbols;
  this->lrFhssFrameHopsRem--;
  this->lrFhssHopNum++;
  return(RADIOLIB_ERR_NONE);
}

#endif
