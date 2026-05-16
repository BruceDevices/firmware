// This sketch draws Jpeg files stored on an SD card on a TFT screen, it is based on the UTFT_Bitmap
// by Henning Karlsen.
// web: http://www.RinkyDinkElectronics.com/
//
// This demo uses an Arduino Due and a 320x240 ILI9341 SPI based TFT screen.
// The demo should also run on an Arduino Mega, but it will be slower
/*
  By default the UTFT library does not configure the Gamma curve settings for the ILI9341 TFT,
  so photo images may not render well.  To correct this ensure the set Gamma curve section
  in initlcd.h (library folder UTFT\tft_drivers\ili9341\s5p\initlcd.h) is NOT commented out.

  You can generate your own Jpeg images from digital photographs by cropping and resizing
  by using commonly available picture/image editors such as Paint or IrfanView.
*/

//  The latest JPEGDecoder library can be found here:
//  https://github.com/Bodmer/JPEGDecoder

//  Information on JPEG compression can be found here:
//  https://en.wikipedia.org/wiki/JPEG 
  


//====================================================================================
//  libraries
//====================================================================================

#include <SPI.h>
#include <SD.h>  // Use the Arduino IDE built-in SD library

#include <UTFT.h>

#include <JPEGDecoder.h>  // JPEG decoder library


//====================================================================================
//  definitions
//====================================================================================

// Set the pins to the correct ones for your SPI TFT
// ------------------------------------------------------------
#define TFT_SDA    7  // Do not use hardware SPI MOSI pin
#define TFT_SCL    6  // Do not use hardware SPI SCK pin
#define TFT_CS    10  // Chip Select for TFT
#define TFT_RS     9  // Register select (also called DC)
#define TFT_RST    8  // uncomment if you have ILI9340

// SD card connects to hardware SPI pins MOSI, MISO and SCK and the following chip select
#define SD_CS 4       // Chip Select for SD card

// this function determines the minimum of two numbers
#define minimum(a,b)     (((a) < (b)) ? (a) : (b))


//====================================================================================
//  setup
//====================================================================================

// Remember to change the model parameter to suit your display module!
UTFT myGLCD(ILI9341_S5P,TFT_SDA, TFT_SCL, TFT_CS, TFT_RST, TFT_RS);

void setup()
{
  Serial.begin(9600);

  myGLCD.InitLCD(PORTRAIT);  // The test image is for a portrait orientation

  myGLCD.fillScr(255, 255, 255);
  
    // Initialise the SD card interface, check it is OK
  Serial.print(F("Initialising SD card..."));

  if (!SD.begin(SD_CS)){
    Serial.println(F("failed!"));
    return;
  }
  Serial.println(F("OK!"));
}


//====================================================================================
//  loop
//====================================================================================

void loop()
{
  // draw jpeg image at 0,0
  jpegDraw( "tiger.jpg", 0, 0 );

  // draw Arduino logo at a random position within the screen area
  jpegDraw( "arduino.jpg", random(myGLCD.getDisplayXSize() - 160), random(myGLCD.getDisplayYSize() - 128) );
 
  delay(5000);
}

//====================================================================================
