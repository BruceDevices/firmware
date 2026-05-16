//
// PNGDisplay class demo
//
// This sketch shows how to use the new helper class, PNGDisplay to more easily
// display PNG images on displays supported by my bb_spi_lcd library
// There are only two overloaded methods exposed by the library: loadPNG(), getPNGInfo()
// They allow you to pass PNG image data as a pointer or a filename on a uSD card
// loadPNG() requires a x,y position for where to draw the image. This code doesn't
// currently support clipping, so attempts to draw off the edge of the display
// will return with an error.
// An optional background color can be specified for PNGs with an alpha channel
//
#include <PNGDisplay.h>
#include <PNGDisplay.inl>
#include <bb_spi_lcd.h>
#include <SPI.h>
#include <SD.h>
#include "octocat_small.h"

BB_SPI_LCD lcd, sprite; // one instance for the display and another for a 'sprite'
PNGDisplay pd; // only one instance of this class is needed
SPIClass SD_SPI;
bool bSD = false;
// These GPIOs are for the uSD card slot on the JC4827W543 "Cheap Yellow Display"
#define SD_CS 10
#define SD_MOSI 11
#define SD_SCK 12
#define SD_MISO 13

void setup() {
  int x, y, w, h, bpp;
  lcd.begin(DISPLAY_CYD_543);
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_GREEN);
  lcd.setFont(FONT_12x16);

  SD_SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SD_SPI, 10000000)) { // Faster than 10MHz seems to fail on the CYDs
    lcd.println("Card Mount Failed");
  } else {
    lcd.println("Card Mount Succeeded");
    bSD = true;
  }
// Load a PNG from the uSD card
  if (bSD) {
    // The PNG can be directly decoded to the LCD
    if (pd.getPNGInfo(&w, &h, &bpp, "/tulips_320x213.png")) { // get info
        // center it on the LCD
        x = (lcd.width() - w)/2;
        y = (lcd.height() - h)/2;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        pd.loadPNG(&lcd, x, y, "/tulips_320x213.png"); // load this image from the root dir of the SD card
        delay(5000);
    }
  }
  //
  // Load and display the PNG image all over the display by using a 'sprite'
  // First, create a sprite instance of BB_SPI_LCD with the createVirtual() method
  // Next, decode a PNG image directly into the sprite memory
  // And finally, draw it in multiple places on the LCD
  //
  lcd.fillScreen(TFT_BLACK);
  // You can request the dimensions and bit depth of the image BEFORE decoding it
  if (pd.getPNGInfo(&w, &h, &bpp, octocat_small, sizeof(octocat_small))) {
    sprite.createVirtual(w, h); // create a sprite of the PNG image size
    // The PNG image can be decoded directly into the sprite instance
    pd.loadPNG(&sprite, 0, 0, octocat_small, sizeof(octocat_small), 0);
    for (int y = 0; y < lcd.height(); y += h) { // now draw it all over the LCD
        for (int x = 0; x < lcd.width(); x += w) {
          lcd.drawSprite(x, y, &sprite, 0xffffffff); // 0xffffffff = no transparent color
        } // for x
    } // for y
    sprite.freeVirtual(); // free the sprite memory
  }
}

void loop() {
}
