//
// 1-bpp PNG example - drawing transparent images on an SSD1306 OLED
// written by Larry Bank
// May 31, 2021
//

#include <PNGdec.h>
#include <OneBitDisplay.h>
// really 2-bpp to get a transparent color in the palette
#include "octocat_1bpp.h"

#define USE_HW_I2C 1
#define INVERT 0
#define FLIP180 0
#define OLED_ADDR -1
OBDISP obd;
PNG png;
static uint8_t ucBackBuffer[1024];

//
// Draw 1-bpp pixels onto the back buffer while respecting the transparent mask
// This needs to be done in the sketch and not the library because it's very
// specific to the memory layout of the SSD1306
//
void DrawPixelsMasked(int y, uint8_t *pPixels, uint8_t *pMask, int width)
{
int x, iBit;
uint8_t *d, ucPix, ucAlpha, ucMask;
// The display has the bytes oriented vertically while the bitmap has them packed horizontally
  d = &ucBackBuffer[(y >> 3) * 128 + 25]; // pointer to display memory (center it horizontally too)
  ucMask = 1 << (y & 7); // bit position of display memory (LSB on top)
  for (x=0; x<width; x+= 8) { // read the source image one byte at a time
    ucAlpha = *pMask++; // alpha mask is always 1-bit per pixel
    for (iBit=0; iBit<8; iBit++) {
      if (iBit == 0 || iBit == 4) // source image is 2-bpp, so work in groups of 4 per byte
         ucPix = *pPixels++;
      if (ucAlpha & 0x80) { // this pixel is opaque
        if ((ucPix & 0xc0) == 0x80) // white? a bit of a hack for this image; I should do a palette lookup here
          *d |= ucMask;
        else
          *d &= ~ucMask;
      } // pixel is opaque
      ucAlpha <<= 1;
      ucPix <<= 2;
      d++;
    } // for each bit/pixel
  } // for each group of 8 pixels
} /* DrawPixelsMasked() */

int PNGDraw(PNGDRAW *pDraw)
{
uint8_t ucMask[32];

  if (png.getAlphaMask(pDraw, ucMask, 255)) { // if any pixels are opaque, draw them
    DrawPixelsMasked(pDraw->y, pDraw->pPixels, ucMask, pDraw->iWidth);
  }
  return 1;
} /* PNGDraw() */

void setup() {
  Serial.begin(115200);
  obdI2CInit(&obd, OLED_128x64, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, -1, -1, -1, 400000L);
  obdSetBackBuffer(&obd, ucBackBuffer);
  obdFill(&obd, 0, 1);
} /* setup() */

void loop() {
int i, rc;
char szTemp[256];

  // create a 'gray' background pattern on the OLED
  for (i=0; i<1024; i+=2) {
    ucBackBuffer[i] = 0x55;
    ucBackBuffer[i+1] = 0xaa;
  }
  obdDumpBuffer(&obd, NULL); // display gray background
  delay(2000);
  rc = png.openRAM((uint8_t *)octocat_1bpp, sizeof(octocat_1bpp), PNGDraw);
  if (rc == PNG_SUCCESS) {
    sprintf(szTemp, "image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    Serial.print(szTemp);
    rc = png.decode(NULL, 0); // no private structure and skip CRC checking
    obdDumpBuffer(&obd, NULL); // show the image
    png.close();
  } // png opened successfully
  delay(2000); // rinse, repeat
} /* loop() */
