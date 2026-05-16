//
// SPDX-FileCopyrightText: 2026 Larry Bank <bitbank@pobox.com>
// SPDX-License-Identifier: Apache-2.0
//
// ESP32S3 SIMD optimized palette translation
// This asm code converts 8-bit GIF pixels through a palette
// into RGB565 output. The palette must be spread out as 32-bit entries
// in order to use the accelerated instruction available on the S3
//
// Copyright 2026 BitBank Software, Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//===========================================================================
//
#if defined (ARDUINO_ARCH_ESP32) && !defined(NO_SIMD)
#if __has_include ("dsps_fft2r_platform.h")
#include "dsps_fft2r_platform.h"
#if (dsps_fft2r_sc16_aes3_enabled == 1)
	.text
	.align 4

    .global s3_palette_lookup
    .type   s3_palette_lookup,@function
//
// Translate 8-bit pixels through a 16-bit (stretched to 32-bits) palette
// Assumes source pointer is unaligned and destination is aligned
//
//                                      A2             A3               A4                  A5   
// Call as void s3_palette_lookup(uint8_t *pSrc, uint16_t *pDest, uint32_t *pPalette, uint32_t u32Len);
  s3_palette_lookup:
  .align 4
  entry a1,16
  srli a7,a5,4                # Groups of 16 pixels to process
  ee.ld.128.usar.ip q0,a2,16  # pre-load 16 source pixels and load SAR_BYTE
  ee.vld.128.ip q1,a2,16      # second half for shifting
  loopnez a7, .sixteen_pixels
  ee.src.q q4,q0,q1           # shift unaligned dest bytes into position
  ee.xorq q5,q5,q5            # a register of 0's for zipping
  ee.vzip.8 q4,q5             # stretch 8bit->16bit data
# first 8 pixels
  ee.ldxq.32 q6,q4,a4,0,0     # load Q6:0 with first pixel's color
  ee.ldxq.32 q6,q4,a4,1,1     # load Q6:1 with second pixel's color
  ee.ldxq.32 q6,q4,a4,2,2
  ee.ldxq.32 q6,q4,a4,3,3
  ee.ldxq.32 q7,q4,a4,0,4
  ee.ldxq.32 q7,q4,a4,1,5
  ee.ldxq.32 q7,q4,a4,2,6
  ee.ldxq.32 q7,q4,a4,3,7
  ee.vunzip.16 q6,q7          # fold 32-bit values to 16-bits
  ee.vst.128.ip q6,a3,16      # write 8 x RGB565 pixels
# second 8 pixels
  ee.ldxq.32 q6,q5,a4,0,0     # load Q6:0 with first pixel's color
  ee.ldxq.32 q6,q5,a4,1,1     # load Q6:1 with second pixel's color
  ee.ldxq.32 q6,q5,a4,2,2
  ee.ldxq.32 q6,q5,a4,3,3
  ee.ldxq.32 q7,q5,a4,0,4
  ee.ldxq.32 q7,q5,a4,1,5
  ee.ldxq.32 q7,q5,a4,2,6
  ee.ldxq.32 q7,q5,a4,3,7
  ee.vunzip.16 q6,q7          # fold 32-bit values to 16-bits
  ee.vst.128.ip q6,a3,16      # write 8 x RGB565 pixels
  mv.qr q0,q1                 # shift down to next 16 source pixels
  ee.vld.128.ip q1,a2,16
  .sixteen_pixels: 
# process last 0-15 pixels
  ee.src.q q4,q0,q1           # shift unaligned dest bytes into position
  ee.xorq q5,q5,q5            # a register of 0's for zipping
  ee.vzip.8 q4,q5             # stretch 8bit->16bit data
# first 8 pixels
  ee.ldxq.32 q0,q4,a4,0,0     # load Q6:0 with first pixel's color
  ee.ldxq.32 q0,q4,a4,1,1     # load Q6:1 with second pixel's color
  ee.ldxq.32 q0,q4,a4,2,2
  ee.ldxq.32 q0,q4,a4,3,3
  ee.ldxq.32 q1,q4,a4,0,4
  ee.ldxq.32 q1,q4,a4,1,5
  ee.ldxq.32 q1,q4,a4,2,6
  ee.ldxq.32 q1,q4,a4,3,7
  ee.vunzip.16 q0,q1            # fold 32-bit values to 16-bits
# second 8 pixels
  ee.ldxq.32 q1,q5,a4,0,0     # load Q6:0 with first pixel's color
  ee.ldxq.32 q1,q5,a4,1,1     # load Q6:1 with second pixel's color
  ee.ldxq.32 q1,q5,a4,2,2
  ee.ldxq.32 q1,q5,a4,3,3
  ee.ldxq.32 q2,q5,a4,0,4
  ee.ldxq.32 q2,q5,a4,1,5
  ee.ldxq.32 q2,q5,a4,2,6
  ee.ldxq.32 q2,q5,a4,3,7
  ee.vunzip.16 q1,q2            # fold 32-bit values to 16-bits
  movi.n a4,15
  and a7,a5,a4                # Remaining pixel count (0-15)
  loopnez a7, .last_palette
  ee.movi.32.a q0,a6,0        # move the first 32-bit slot into a6
  s16i a6,a3,0                # store 4 bytes
  addi.n a3,a3,2
  ee.srci.2q q1,q0,1          # shift q0+q1 right 2 bytes
.last_palette:
  retw.n
#endif // dsps_fft2r_sc16_aes3_enabled
#endif // __has_include
#endif // ESP32
