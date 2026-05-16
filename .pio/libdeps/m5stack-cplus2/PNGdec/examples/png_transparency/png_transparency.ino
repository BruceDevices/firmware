//
// PNG Drawing with transparency example
//
// loads a 120x100 4-bpp palette image
// and draws it at random locations on the LCD
//
#include <PNGdec.h>
#include <bb_spi_lcd.h>
#include <Wire.h>

// 120x100x4-bpp image with transparency
// The small file size (852 bytes) allows it to be decoded and drawn quickly
#include "octocat_4bpp.h"

PNG png;
SPILCD lcd;
static uint8_t ucTXBuf[1024];
#define TFT_CS 10
#define TFT_RST -1
#define TFT_DC 9
#define TFT_CLK 13
#define TFT_MOSI 11

//
// Create a private structure to pass info to the draw callback
// For this example we want to pass a random x/y starting point
//
typedef struct my_private_struct
{
  int xoff, yoff; // corner offset
} PRIVATE;

int PNGDraw(PNGDRAW *pDraw)
{
uint16_t usPixels[320];
uint8_t ucMask[40];
PRIVATE *pPriv = (PRIVATE *)pDraw->pUser;

  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  if (png.getAlphaMask(pDraw, ucMask, 255)) { // if any pixels are opaque, draw them
    spilcdWritePixelsMasked(&lcd, pPriv->xoff, pPriv->yoff + pDraw->y, (uint8_t *)usPixels, ucMask, pDraw->iWidth, DRAW_TO_LCD);
  }
  return 1;
} /* PNGDraw() */

void setup() {
  spilcdSetTXBuffer(ucTXBuf, sizeof(ucTXBuf));
  spilcdInit(&lcd, LCD_ILI9341, FLAGS_NONE, 40000000, TFT_CS, TFT_DC, TFT_RST, -1, -1, TFT_MOSI, TFT_CLK);
  spilcdSetOrientation(&lcd, LCD_ORIENTATION_270);
  spilcdFill(&lcd, 0, DRAW_TO_LCD);
  spilcdWriteString(&lcd, 0, 4, (char *)"PNG Test", 0xffff, 0, FONT_12x16, DRAW_TO_LCD);
  Serial.begin(115200);
  delay(2000);
}

void loop() {
  int rc, i = 0;
  PRIVATE priv;

  spilcdFill(&lcd, 0xf81f, DRAW_TO_LCD);
  rc = png.openRAM((uint8_t *)octocat_4bpp, sizeof(octocat_4bpp), PNGDraw);
  if (rc == PNG_SUCCESS) {
      Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
      while (1) { // loop forever
        priv.xoff = rand() & 0xff;
        priv.yoff = rand() % 160;
        rc = png.decode((void *)&priv, 0);
      }
      png.close();
  }
} /* loop() */
