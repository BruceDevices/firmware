#include <bb_spi_lcd.h>
#include <PNGdec.h>

#include "lodepng.h"
// sample images
#include "zoidberg_320x240_24b.h"
#include "zoidberg_320x240_4b.h"
#include "zebra.h"
#include "stackoverflow.h"
#include "snoopy_128x128.h"
#include "snoopy_128x128_4b.h"
#include "octocat_4bpp.h"
#include "arduino_screen.h"

static uint8_t u8GFXBuf[4096];
int bDisplay;
PNG *png;
BB_SPI_LCD lcd;
uint16_t u16LodeColor, u16PNGColor;
uint16_t __attribute__((aligned (16))) usPixels[480];

void DecodeTest(int bLode, const uint8_t *pData, size_t data_size, const char *szName)
{
  unsigned int width, height;
  int rc;
  uint8_t *image;
  long l;
  char szTemp[64];

    lcd.setFont(FONT_8x8);
    if (bDisplay && !bLode) {
      lcd.fillScreen(TFT_BLACK);
    }
     if (bLode) {
       lcd.setTextColor(u16LodeColor, TFT_BLACK);
       l = micros();
       lodepng_decode32(&image, &width, &height, pData, data_size);
       l = micros() - l;
       if (image) {
         if (!bDisplay) { sprintf(szTemp, "LodePNG decoded in %d us\n", (int)l); lcd.print(szTemp); }
         free(image);
       } else {
         if (!bDisplay) lcd.println("LodePNG decode failed");
       }
     } else {
       lcd.setTextColor(u16PNGColor, TFT_BLACK);
       l = micros();
       png = (PNG *)malloc(sizeof(PNG));
       rc = png->openFLASH((uint8_t *)pData, data_size, PNGDraw);
       if (rc == PNG_SUCCESS) {
         rc = png->decode(NULL, PNG_FAST_PALETTE);
         if (rc == PNG_SUCCESS) {
           l = micros() - l;
           sprintf(szTemp, "PNGdec decode in %d us\n", (int)l);
           if (bDisplay) {
              Serial.print(szTemp);
           } else {
              lcd.print(szTemp);
           }
         } else {
           if (!bDisplay) lcd.println("PNGdec decode failed");
         }
       } else {
          if (!bDisplay) lcd.println("PNGdec open failed");
       }
       free(png);
     }
    if (bLode == 1) {
      lcd.setTextColor(TFT_MAGENTA, TFT_BLACK);
      if (bDisplay)
        lcd.setCursor(0, 224);
      else
         Serial.println(szName);
      lcd.println(szName);
    }
} /* DecodeTest() */

int PNGDraw(PNGDRAW *pDraw)
{
  if (bDisplay) {
    if (pDraw->y == 0) {
      lcd.setAddrWindow(0, 0, pDraw->iWidth, 240);
    }
    png->getLineAsRGB565(pDraw, usPixels, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    lcd.pushPixels(usPixels, pDraw->iWidth, DRAW_TO_LCD | DRAW_WITH_DMA);
  }
  return 1;
} /* PNGDraw() */

void setup() {
  Serial.begin(115200);
  spilcdSetTXBuffer(u8GFXBuf, 4096);
//  lcd.begin(DISPLAY_CYD);
//  lcd.begin(DISPLAY_TUFTY2040);
  lcd.begin(DISPLAY_T_DISPLAY_S3_PRO);
  //lcd.begin(LCD_ILI9341, FLAGS_NONE, 60000000, 10, 9, 8, -1, 12, 11, 13); // Teensy 4.x + ILI9341
  lcd.setRotation(270);//  lcd.begin(DISPLAY_CYD); //DISPLAY_CYD_22C);
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(FONT_12x16);
  u16PNGColor = TFT_YELLOW;
  u16LodeColor = TFT_GREEN;
  lcd.setTextColor(u16LodeColor, TFT_BLACK);
  lcd.print("LodePNG");
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.print(" vs. ");
  lcd.setTextColor(u16PNGColor, TFT_BLACK);
  lcd.println("PNGdec");
} /* setup() */

void loop() {
  char szTemp[64];

  for (bDisplay = 0; bDisplay < 2; bDisplay++) { // first pass no display, second pass with display
    DecodeTest(0, arduino_screen, sizeof(arduino_screen), "Arduino IDE 480x222x32-bpp");
    DecodeTest(1, arduino_screen, sizeof(arduino_screen), "Arduino IDE 480x222x32-bpp");
    if (bDisplay) delay(3000);
    DecodeTest(0, stackoverflow, sizeof(stackoverflow), "stackoverflow 320x84x32-bpp");
    DecodeTest(1, stackoverflow, sizeof(stackoverflow), "stackoverflow 320x84x32-bpp");
    if (bDisplay) delay(3000);
    DecodeTest(0, zoidberg_320x240_24b, sizeof(zoidberg_320x240_24b), "zoidberg 320x240x24-bpp");
    DecodeTest(1, zoidberg_320x240_24b, sizeof(zoidberg_320x240_24b), "zoidberg 320x240x24-bpp");
    if (bDisplay) delay(3000);
    DecodeTest(0, zoidberg_320x240_4b, sizeof(zoidberg_320x240_4b), "zoidberg 320x240x4-bpp");
    DecodeTest(1, zoidberg_320x240_4b, sizeof(zoidberg_320x240_4b), "zoidberg 320x240x4-bpp");
    if (bDisplay) delay(3000);
    DecodeTest(0, snoopy_128x128, sizeof(snoopy_128x128), "snoopy 128x128x32-bpp");
    DecodeTest(1, snoopy_128x128, sizeof(snoopy_128x128), "snoopy 128x128x32-bpp");
    if (bDisplay) delay(3000);
    DecodeTest(0, octocat_4bpp, sizeof(octocat_4bpp), "octocat 240x200x4-bpp");
    DecodeTest(1, octocat_4bpp, sizeof(octocat_4bpp), "octocat 240x200x4-bpp");
    if (bDisplay) delay(3000);
    DecodeTest(0, zebra, sizeof(zebra), "zebra 320x240x24-bpp");
    DecodeTest(1, zebra, sizeof(zebra), "zebra 320x240x24-bpp");
    if (bDisplay) delay(3000);
    DecodeTest(0, snoopy_128x128_4b, sizeof(snoopy_128x128_4b), "snoopy 128x128x4-bpp");
    DecodeTest(1, snoopy_128x128_4b, sizeof(snoopy_128x128_4b), "snoopy 128x128x4-bpp");
    
    if (bDisplay == 0) {
      lcd.setFont(FONT_12x16);
      lcd.setTextColor(TFT_WHITE, TFT_BLACK);
      lcd.println("Now to show the images...");
      delay(5000);
      lcd.fillScreen(TFT_BLACK);
    }
  } // for bDisplay
  delay(5000);
  lcd.fillScreen(TFT_BLACK);
  lcd.setCursor(0,0);
  lcd.setFont(FONT_12x16);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.println("Q: Why did LodePNG fail?");
  lcd.println("A: It uses too much RAM\n");
  lcd.println("It wasn't designed to run");
  lcd.println("on embedded devices, but");
  lcd.println("PNGdec is written for MCUs");
  sprintf(szTemp, "It only needs %d bytes\n\r", sizeof(PNG));
  lcd.print(szTemp);
  lcd.println("(and it's much faster!)");

    while (1) {
      delay(1000);
    };
}
