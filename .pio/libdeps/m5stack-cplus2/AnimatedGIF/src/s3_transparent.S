//
// SPDX-FileCopyrightText: 2026 Larry Bank <bitbank@pobox.com>
// SPDX-License-Identifier: Apache-2.0
//
// ESP32S3 SIMD optimized transparent pixel merging
// This asm code quickly combines a row of decoded 8-bit GIF pixels
// with an existing row of pixels and skips the transparent pixels.
// This is simple to do with both scalar and vector instructions.
// The challenge with the ESP32-S3 is that when using SIMD instructions
// reading and writing to memory must be on 16-byte aligned addresses.
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

    .global s3_merge_transparent
    .type   s3_merge_transparent,@function
//
// Combine opaque 8-bit pixels, skipping transparent ones
// The source pointer is assumed to be 16-byte aligned
// The destination pointer can be unaligned
//
//                                       A2            A3            A4                          A5
// Call as void s3_merge_transparent(uint8_t *pSrc, uint8_t *pDest, uint8_t *pTransparent, int iLen);
  s3_merge_transparent:
    # no idea what this frequency keyword does
#    .frequency 1.000 0.000
    .align 4
    entry    a1,16
  movi.n  a8,15             # mask to test address alignment
  ee.vldbc.8 q3,a4          # broadcast transparent color to all 16 slots
  and a7,a3,a8              # test destination pointer alignment
  bnez.n a7,.unaligned_dest
  srli a7,a5,4            # number of 16-byte blocks to process
  loopnez a7, .big_chunk_loop
  ee.vld.128.ip    q0,a2,16   # load 16 source pixels
  ee.vld.128.ip    q1,a3,0    # load 16 destination pixels
  ee.vcmp.eq.s8 q2,q0,q3      # compare source pixels with transparent color
  ee.andq q4,q2,q1            # keep destination pixels using the mask
  ee.notq q2,q2               # invert the mask
  ee.andq q0,q2,q0            # keep source opaque pixels
  ee.orq q0,q0,q4             # combine src+dst pixels
  ee.vst.128.ip q0,a3,16      # store 16 finished pixels
  .big_chunk_loop:
  and a7,a5,a8                # leftover count
  bnez.n a7,.leftover_aligned 
  retw.n                      # length is a multiple of 16
.leftover_aligned:
// Read and process 16 pixels, but store the actual length requested
  ee.vld.128.ip    q1,a3,0    # load 16 destination pixels
.leftover_unaligned:
  ee.vld.128.ip    q0,a2,16   # load 16 source pixels
  ee.vcmp.eq.s8 q2,q0,q3      # compare source pixels with transparent color
  ee.andq q4,q2,q1            # keep destination pixels using the mask
  ee.notq q2,q2               # invert the mask
  ee.andq q0,q2,q0            # keep source opaque pixels
  ee.orq q0,q4,q0             # combine src+dst pixels
  movi.n a6,12
  and a7,a5,a6                # loop over groups of 4 bytes
  beqz.n a7,.last3_aligned
  srli a7,a7,2                # number of groups of 4 bytes
  loopnez a7, .last3_aligned
  ee.movi.32.a q0,a6,0        # move the first 32-bit slot into a6
  s32i a6,a3,0                # store 4 bytes
  addi.n a3,a3,4
  ee.srci.2q q1,q0,3          # shift q0 right 4 bytes
 .last3_aligned:
  ee.movi.32.a q0,a6,0        # move the first 32-bit slot into a6
  movi.n a4,3
  and a7,a5,a4
  loopnez a7, .last3_bytes
  s8i a6,a3,0                 # store low byte
  addi.n a3,a3,1
  srli a6,a6,8                # shift down the next byte
 .last3_bytes:
  retw.n
# The destination pointer is not aligned on a 16-byte address
# We need to go through all sorts of pain to make it work on the ESP32-S3
.unaligned_dest:
  mov a4,a3                   # keep original destination pointer
  ee.ld.128.usar.ip q1,a3,16  # pre-load 32 bytes and SAR_BYTE with address inc
  ee.vld.128.ip q2,a3,16      # second half for shifting
  srli a7,a5,4            # number of 16-byte blocks to process
  loopnez a7, .big_chunk_unaligned
  ee.vld.128.ip    q0,a2,16   # load 16 source pixels
  ee.src.q q4,q1,q2           # shift unaligned dest bytes into position
  ee.vcmp.eq.s8 q6,q0,q3      # compare source pixels with transparent color
  ee.andq q5,q4,q6            # keep destination pixels using the mask
  ee.notq q6,q6               # invert the mask
  ee.andq q0,q6,q0            # keep source opaque pixels
  ee.orq q0,q0,q5             # combine src+dst pixels
# We need to store the result 4-bytes at a time since the non-vector write
# instruction allows unaligned addresses
  ee.movi.32.a q0,a6,0        # move the first 32-bit slot into a8
  s32i a6,a4,0                # store first 4 finished pixels
  ee.movi.32.a q0,a6,1        # second word
  s32i a6,a4,4
  ee.movi.32.a q0,a6,2        # third word
  s32i a6,a4,8
  ee.movi.32.a q0,a6,3        # fourth word
  s32i a6,a4,12
  addi.n a4,a4,16             # advance dest address by 16 bytes
  mv.qr q1,q2                 # prepare for next set of 16
  ee.vld.128.ip q2,a3,16      # read next 16 unaligned bytes
  .big_chunk_unaligned:
  ee.src.q q1,q1,q2           # prepare last set of (0-15) dest pixels
  mov a3,a4                   # put correct destination address back in A3
  and a7,a5,a8                # leftover count
  bnez.n a7,.leftover_unaligned
  retw.n
#endif // dsps_fft2r_sc16_aes3_enabled
#endif // __has_include
#endif // ESP32
