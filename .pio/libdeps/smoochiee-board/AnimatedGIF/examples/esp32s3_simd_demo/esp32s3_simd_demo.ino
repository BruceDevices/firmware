//
// SPDX-FileCopyrightText: 2026 Larry Bank <bitbank@pobox.com>
// SPDX-License-Identifier: Apache-2.0
//
// ESP32S3 SIMD Demo
// This sketch demonstrates the speed benefit of using the ESP32-S3's
// SIMD instructions to accelerate GIF playback
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
// Uncomment the OLD_WAY macro definition to measure the speed of the original C code
//#define OLD_WAY

#include <bb_spi_lcd.h>
#include <AnimatedGIF.h>
#include "simpson_family_320x240.h"
AnimatedGIF gif;
BB_SPI_LCD lcd;
int w, h, xoff, yoff;
uint8_t *pFrameBuf;
uint16_t  __attribute__((aligned(16))) u16Pixels[400];
uint32_t __attribute__((aligned(16))) u32Palette[256];

extern "C" {
  void s3_palette_lookup(uint8_t *pSrc, uint16_t *pDest, uint32_t *pPalette, uint32_t u32Len);
}

//
// GIF callback function
//
// Called for each line of the image
// The pixels provided are the 8-bit palette values which must be checked for
// the transparent color (if defined) and then translated through the palette
// to produce the final output
//
void GIFDraw(GIFDRAW *pDraw)
{
uint8_t c, *s, *d, ucTransparent;
uint16_t *d16, *usPalette;
int x, y, iWidth;

  if (pDraw->y == 0) { // set the memory window when the first line is rendered
     lcd.setAddrWindow(xoff + pDraw->iX, yoff + pDraw->iY, pDraw->iWidth, pDraw->iHeight);
     for (x=0; x<256; x++) {
        u32Palette[x] = pDraw->pPalette[x]; // expand 16->32bit entries for SIMD code
     }
  }

    iWidth = pDraw->iWidth;
    if (iWidth + pDraw->iX > lcd.width()) {
       iWidth = lcd.width() - pDraw->iX;
    }
    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line
    if (y >= lcd.height() || pDraw->iX >= lcd.width() || iWidth < 1)
       return;
    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) {// restore to background color
      for (x=0; x<iWidth; x++) {
        if (s[x] == pDraw->ucTransparent)
           s[x] = pDraw->ucBackground;
      } // for x
      pDraw->ucHasTransparency = 0;
  } // if restore to background

    // Apply the new pixels to the main image
    s = pDraw->pPixels;
    d = &pFrameBuf[(y * w) + pDraw->iX]; // apply new pixels here
    if (pDraw->ucHasTransparency) { // if transparency used
      ucTransparent = pDraw->ucTransparent;
#ifdef OLD_WAY
      for (x=0; x < iWidth; x++) {
          c = *s++;
          if (c != ucTransparent) {
            *d = c;
          }
          d++;
      } // for x
#else
      gif.mergeTransparent(s, d, ucTransparent, iWidth);
#endif
    } else { // no transparent pixels
      memcpy(d, s, iWidth); // just copy
    }
    // Convert the current line through the palette and display it
    s = &pFrameBuf[(y * w) + pDraw->iX];
    d16 = u16Pixels;
#ifdef OLD_WAY
    for (x=0; x<iWidth; x++) {
      *d16++ = usPalette[*s++];
    }
#else
    s3_palette_lookup(s, d16, u32Palette, iWidth); // -166ms savings
#endif
    // Push the pixels to the display
    lcd.pushPixels(u16Pixels, iWidth, DRAW_TO_LCD | DRAW_WITH_DMA);
} /* GIFDraw() */

void setup() {
  long lTime;
  int i, iFrames;

  Serial.begin(115200);
  delay(3000);
  Serial.println("Starting...");
   lcd.begin(DISPLAY_LILYGO_T_DECK_PLUS);
//   lcd.begin(DISPLAY_WS_AMOLED_18); // ESP32-S3 w/368x448 QSPI AMOLED
   lcd.fillScreen(TFT_BLACK);
   gif.begin(BIG_ENDIAN_PIXELS);
   if (gif.open((uint8_t*)simpson_family_320x240, sizeof(simpson_family_320x240), GIFDraw)) {
      w = gif.getCanvasWidth(); h = gif.getCanvasHeight();
      // Center it on the display
      xoff = (lcd.width() - w)/2;
      yoff = (lcd.height() - h)/2;
      // Allocate memory to hold the 8-bit pixels of each frame
      // You may need to enable PSRAM for the allocation to succeed
      pFrameBuf = (uint8_t*)malloc(w * (h+1));
      // run the animation loop 5 times
      for (i = 0; i<5; i++) {
        lTime = micros();
        iFrames = 0;
        while (gif.playFrame(false, NULL)){
  //      Serial.printf("frame %d, size: %d x %d\n", iFrames, gif.getFrameWidth(), gif.getFrameHeight());
          iFrames++;
        }
        lTime = micros() - lTime;
        gif.reset(); // reset to frame 0
      } // for i
      Serial.printf("total decode time for %d frames = %d us\n", iFrames, (int)lTime);
   } // if open succeeded
} /* setup() */

void loop() {
}
