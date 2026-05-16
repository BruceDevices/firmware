// This sketch draws a Jpeg file stored in an array onto the TFT screen using the UTFT library
// by Henning Karlsen.
// web: http://www.RinkyDinkElectronics.com/
//
// This demo uses an Arduino Due and a 320x240 ILI9341 SPI based TFT screen.
// The deom should also run on an Arduino Mega, but will be much slower
/*
  By default the UTFT library does not configure the Gamma curve settings for the ILI9341 TFT,
  so photo images may not render well.  To correct this ensure the set Gamma curve section
  in initlcd.h (library folder UTFT\tft_drivers\ili9341\s5p\initlcd.h) is NOT commented out.
*/

//  The latest JPEGDecoder library can be found here:
//  https://github.com/Bodmer/JPEGDecoder

//  Information on JPEG compression can be found here:
//  https://en.wikipedia.org/wiki/JPEG 
  
#include <UTFT.h>

#include <JPEGDecoder.h>  // JPEG decoder library

// Set the pins to the correct ones for your SPI TFT pins
// ------------------------------------------------------------
#define TFT_SDA    7  // Serial data pin
#define TFT_SCL    6  // Serial clock pin
#define TFT_CS    10  // Chip Select for LCD
#define TFT_RS     9  // Register select (also called DC)
#define TFT_RST    8  // uncomment if you have ILI9340

// Remember to change the model parameter to suit your display module!
UTFT myGLCD(ILI9341_S5P,TFT_SDA, TFT_SCL, TFT_CS, TFT_RST, TFT_RS);

// Include the sketch header file that contains the image stored as an array of bytes
// More than one image array could be stored in each header file.
#include "jpeg1.h"

void setup()
{
  Serial.begin(9600);
  myGLCD.InitLCD(PORTRAIT);  // The test image is for a portrait orientation
}

void loop()
{
  uint32_t clearTime = millis();
  myGLCD.fillScr(255, 255, 255);
  // calculate how long it took to draw the image
  clearTime = millis() - clearTime; // Calculate the time it took

  // print the results to the serial port
  Serial.print(F(  "Total clear time was    : ")); Serial.print(clearTime); Serial.println(F(" ms"));
  Serial.println(F(""));
  
  // Draw a jpeg image stored in memory
  drawArrayJpeg(Baboon, sizeof(Baboon), 0, 0);

  delay(5000);
}

//####################################################################################################
// Draw a JPEG on the TFT pulled from a program memory array
//####################################################################################################
void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int x, int y) {

  JpegDec.decodeArray(arrayname, array_size);

  jpegInfo(); // Print information from the JPEG file (could comment this line out)

  renderJPEG(x, y);
  
  Serial.println("#########################");
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void renderJPEG(int xpos, int ypos) {

  // retrieve infomration about the image
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = min(mcu_w, max_x % mcu_w);
  uint32_t min_h = min(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while (JpegDec.read()) {    // While there is more data in the file

  // save a pointer to the image block
  pImg = JpegDec.pImage ;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

   // check if the image block size needs to be changed for the right and bottom edges
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= myGLCD.getDisplayXSize() && ( mcu_y + win_h ) <= myGLCD.getDisplayYSize())
    {
      // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
      digitalWrite(TFT_CS, LOW); // Set chip select low so we can take control of display
      myGLCD.setXY(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);


      // Write all MCU pixels to the TFT window
      while (mcu_pixels--) {
        // Push each pixel to the TFT MCU area
        uint8_t col_h = (*pImg) >> 8;    // High byte
        uint8_t col_l = (*pImg) & 0xFF;  // Low byte
        pImg++;                          // Increment pointer
        myGLCD.LCD_Write_DATA(col_h, col_l); // Sent pixel colour to window
      }
      digitalWrite(TFT_CS, HIGH);  // Set chip select high
    }
    else if ( (mcu_y + win_h) >= myGLCD.getDisplayYSize()) JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime; // Calculate the time it took

  // print the results to the serial port
  Serial.print(F(  "Total render time was    : ")); Serial.print(drawTime); Serial.println(F(" ms"));
  Serial.println(F(""));
}

//====================================================================================
//   Print information about the decoded Jpeg image
//====================================================================================

void jpegInfo() {
  Serial.println(F("==============="));
  Serial.println(F("JPEG image info"));
  Serial.println(F("==============="));
  Serial.print(F(  "Width      :")); Serial.println(JpegDec.width);
  Serial.print(F(  "Height     :")); Serial.println(JpegDec.height);
  Serial.print(F(  "Components :")); Serial.println(JpegDec.comps);
  Serial.print(F(  "MCU / row  :")); Serial.println(JpegDec.MCUSPerRow);
  Serial.print(F(  "MCU / col  :")); Serial.println(JpegDec.MCUSPerCol);
  Serial.print(F(  "Scan type  :")); Serial.println(JpegDec.scanType);
  Serial.print(F(  "MCU width  :")); Serial.println(JpegDec.MCUWidth);
  Serial.print(F(  "MCU height :")); Serial.println(JpegDec.MCUHeight);
  Serial.println(F("==============="));
}

