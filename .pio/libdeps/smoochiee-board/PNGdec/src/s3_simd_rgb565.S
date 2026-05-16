//
// ESP32-S3 SIMD optimized code
// Written by Larry Bank
// Copyright (c) 2024 BitBank Software, Inc.
// written April 17, 2024
//
#ifdef ARDUINO_ARCH_ESP32

#include "dsps_fft2r_platform.h"
#if (dsps_fft2r_sc16_aes3_enabled == 1)
	.text
	.align 4

// Convert N pixels of RGBA (RGB8888) to RGB565
//                             A2            A3            A4           A5
// Call as void s3_rgb565(uint8_t *pSrc, uint8_t *pDest, int iCount, bool bBigEndian);
	.global s3_rgb565
    .type   s3_rgb565,@function

s3_rgb565:
	# no idea what this frequency keyword does
#	.frequency 1.000 0.000
  entry  a1,16
  addi.n a4,a4,7        # process pixels in groups of 8
  movi.n a6,-8
  and  a4,a4,a6
.top_rgb565:
  ee.vld.128.ip		q0,a2,16   # load 4 pixels into Q0
  ee.vld.128.ip		q1,a2,16   # load another 4 pixels into Q1
  ee.xorq q4,q4,q4      # destination reg set to 0s
  ee.vcmp.eq.s16 q2,q2,q2  # create FFs
  ee.vunzip.16 q0,q1      # interleave RG and BA from Q0/Q1
  ee.vsubs.s16 q3,q4,q2   # make 16-bit 1's in q3
  movi.n	a6,10          # load the shift register with 10 (for green)
  wsr.sar	a6             # put it in the SAR (shift amount register)
  ee.vmul.u16 q5,q0,q3   # shift green into lower 6 bits of q5
  movi.n        a6,3           # shift value 3 for red and blue
  wsr.sar       a6
  ee.vmul.s16 q6,q0,q3   # shift red into lower 5 bits of q6
  ee.vmul.s16 q7,q1,q3   # shift blue into lower 5 bits of q7
  movi.n a6,5
  wsr.sar a6            # set shift to 5
  ee.vsl.32 q4,q3       # q4 = 0x00200020...
  ee.vsubs.s16 q4,q4,q3 # Now q4 has the r/b mask of 0x001f001f...
  ee.andq q6,q4,q6      # mask off 5 bits of red
  ee.andq q7,q4,q7      # mask off 5 bits of blue
  ee.vsl.32 q5,q5       # shift green left 5
  movi.n a6,11
  wsr.sar a6            # set shift to 11
  ee.vsl.32 q6,q6       # shift red left 11
  ee.orq q6,q6,q5      # combine red+green
  ee.orq q6,q6,q7      # combine rg with b
  mv.qr q5,q6           # in case we're generating little endian output
  beqi a5,0,.rgb565_out # RGB565 little endian?
  ee.vunzip.8 q6,q5      # swap the byte order to be big-endian
  ee.vzip.8 q5,q6
.rgb565_out:
  ee.vst.128.ip q5,a3,16  # store 8 RGB565 pixels
  addi.n a4,a4,-8
  bnez.n a4,.top_rgb565
  retw.n
#endif // dsps_fft2r_sc16_aes3_enabled
#endif // ESP32
