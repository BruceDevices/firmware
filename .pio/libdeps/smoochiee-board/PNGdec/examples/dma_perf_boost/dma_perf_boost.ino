//
// Example sketch showing the performance benefit of using
// DMA when pushing pixels to serial connected LCD displays
//
// written by Larry Bank (bitbank@pobox.com)
// Febrary 27, 2025
//
#include <PNGdec.h>
#include <bb_spi_lcd.h>
#include "../../test_images/octocat.h"
PNG png;
BB_SPI_LCD lcd;
int w, h, xoff, yoff;
bool bDMA;
static uint16_t usPixels[320]; // make sure there is enough room for the full image width

//
// PNG draw callback
// Called with a full row of pixels
// for every row in the image
//
int PNGDraw(PNGDRAW *pDraw)
{
    if (pDraw->y == 0) {
      // set the address window when we get the first line
      lcd.setAddrWindow(xoff, yoff, w, h);
    }
    // There's a risk of overwriting pixels that are still being sent to the display if we only use a single \
    // DMA buffer, **BUT** in this case, the PNG decoding plus the pixel conversion takes a long time
    // relative to sending pixels to the display. If the display were a REALLY slow one, then it would be
    // prodent to use a dual (ping-pong) buffer scheme to avoid that risk.
    png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_BIG_ENDIAN, 0); // get help converting to RGB565
    if (bDMA) {
      lcd.pushPixels(usPixels, pDraw->iWidth, DRAW_TO_LCD | DRAW_WITH_DMA);
    } else {
      lcd.pushPixels(usPixels, pDraw->iWidth);
    }
    return 1;
} /* PNGDraw() */

void setup()
{
  long lTime;
  lcd.begin(DISPLAY_CYD_543 /*DISPLAY_WS_AMOLED_18*/);
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(FONT_12x16);
  lcd.setTextColor(TFT_GREEN);
  lcd.setCursor((lcd.width() - 17*12)/2, 0);
  lcd.print("PNG DMA Perf Demo");
  png.openFLASH((uint8_t *)octocat, sizeof(octocat), PNGDraw);
  w = png.getWidth();
  h = png.getHeight();
  xoff = (lcd.width() - w)/2; // center on the LCD
  yoff = (lcd.height() - h)/2;
  bDMA = false; // first time without DMA
  lTime = micros();
  png.decode(NULL, PNG_FAST_PALETTE); // use the RGB565 palette to speed up color conversion for the LCD
  lTime = micros() - lTime;
  png.close();
  lcd.setCursor((lcd.width() - (20*12))/2, lcd.height()-32);
  lcd.printf("Without DMA: %d us\n", (int)lTime);
  png.openFLASH((uint8_t *)octocat, sizeof(octocat), PNGDraw);
  w = png.getWidth();
  h = png.getHeight();
  xoff = (lcd.width() - w)/2; // center on the LCD
  yoff = (lcd.height() - h)/2;
  bDMA = true; // second time with DMA
  lTime = micros();
  png.decode(NULL, PNG_FAST_PALETTE); // use the RGB565 palette to speed up color conversion for the LCD
  lTime = micros() - lTime;
  png.close();
  lcd.setCursor((lcd.width() - (17*12))/2, lcd.height()-16);
  lcd.printf("With DMA: %d us\n", (int)lTime);
}

void loop()
{

}
