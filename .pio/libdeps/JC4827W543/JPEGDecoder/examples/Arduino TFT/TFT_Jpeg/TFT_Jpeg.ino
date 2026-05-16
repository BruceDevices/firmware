/*

  Arduino TFT Jpeg example

  This example reads a Jpeg image file from a micro-SD card
  and draws it on the screen.

  In this sketch, the Arduino logo is read from a micro-SD card.
  There is a arduino.jpg file included with this sketch.
  - open the sketch folder (Ctrl-K or Cmd-K)
  - copy the "arduino.jpg" file to a micro-SD
  - put the SD into the SD slot of the Arduino TFT module.

  This example code is in the public domain.

  Original sketch "TFTBitmapLogo" created 19 April 2013 by Enrico Gueli

  Adapted by Bodmer 20 January 2017 to display a jpeg image
  rather than a bitmap

  Display details here:
  https://www.arduino.cc/en/Main/GTFT

  The decoding of jpeg images involves a lot of complex maths and
  requires a processor with at least 8 kbytes of RAM. This sketch has
  been tested with the Arduino Mega and Due boards.
*/

// include the necessary libraries
#include <SPI.h>
#include <SD.h>
#include <TFT.h>  // Arduino LCD library

//#include <TFT_HX8357.h>        // Hardware-specific library
//TFT_HX8357 TFTscreen = TFT_HX8357(); // Invoke custom library

#include <JPEGDecoder.h>  // JPEG decoder library

// pin definition for the Mega
#define sd_cs  53
#define lcd_cs 49
#define dc     48
#define rst    47

#define TFT_WHITE 0xFFFF
#define TFT_BLACK 0x0000
#define TFT_RED   0xF800

// TFT driver and graphics library
TFT TFTscreen = TFT(lcd_cs, dc, rst);

// this function determines the minimum of two numbers
#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

//====================================================================================
//   setup
//====================================================================================
void setup() {
  // initialize the GLCD and show a message
  // asking the user to open the serial line
  TFTscreen.begin();
  TFTscreen.fillScreen(TFT_WHITE); // Alternative: TFTscreen.background(255, 255, 255);

  TFTscreen.setTextColor(TFT_RED); // Alternative: TFTscreen.stroke(0, 0, 255);
  TFTscreen.println();
  TFTscreen.println(F("Arduino TFT Jpeg Example"));

  TFTscreen.setTextColor(TFT_BLACK); // Alternative: TFTscreen.stroke(0, 0, 0);
  TFTscreen.println(F("Open serial monitor"));
  TFTscreen.println(F("to run the sketch"));
delay(10000);
  // initialize the serial port: it will be used to
  // print some diagnostic info
  Serial.begin(9600);
  while (!Serial) {
    // wait for serial port to connect. Needed for native USB port only
  }

  // clear the GLCD screen before starting
//  TFTscreen.background(255, 255, 255);
  TFTscreen.fillScreen(TFT_WHITE);

  // try to access the SD card. If that fails (e.g.
  // no card present), the setup process will stop.
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(sd_cs)) {
    Serial.println(F("failed!"));
    while (1); // SD initialisation failed so wait here
  }
  Serial.println(F("OK!"));

  // initialize and clear the GLCD screen
  TFTscreen.begin();
  TFTscreen.fillScreen(TFT_WHITE); // Alternative: TFTscreen.background(255, 255, 255);

  // now that the SD card can be accessed, check the
  // image file exists.
  if (SD.exists("arduino.jpg")) {
    Serial.println("arduino.jpg found on SD card.");
  } else {
    Serial.println("arduino.jpg not found on SD card.");
    while (1); // Image file missing so stay here
  }

}

//====================================================================================
//   Main loop
//====================================================================================
void loop() {

  // open the image file
  File jpgFile = SD.open( "arduino.jpg", FILE_READ);

  // initialise the decoder to give access to image information
  JpegDec.decodeSdFile(jpgFile);

  // print information about the image to the serial port
  jpegInfo();

  // render the image onto the screen at coordinate 0,0
  renderJPEG(0, 0);

  // wait a little bit before clearing the screen to random color and drawing again
  delay(4000);

  // clear screen
  TFTscreen.fillScreen(random(0xFFFF));  // Alternative: TFTscreen.background(255, 255, 255);
}


//====================================================================================
//   Print information about the image
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


//====================================================================================
//   Decode and paint onto the TFT screen
//====================================================================================
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
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

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
  while ( JpegDec.read()) {

    // save a pointer to the image block
    pImg = JpegDec.pImage;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right and bottom edges
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    // draw image block if it will fit on the screen
    if ( ( mcu_x + win_w) <= TFTscreen.width() && ( mcu_y + win_h) <= TFTscreen.height()) {
      // open a window onto the screen to paint the pixels into
      //TFTscreen.setAddrWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);
      TFTscreen.setAddrWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);
      // push all the image block pixels to the screen
      while (mcu_pixels--) TFTscreen.pushColor(*pImg++); // Send to TFT 16 bits at a time
    }

    // stop drawing blocks if the bottom of the screen has been reached
    // the abort function will close the file
    else if ( ( mcu_y + win_h) >= TFTscreen.height()) JpegDec.abort();

  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime; // Calculate the time it took

  // print the results to the serial port
  Serial.print  ("Total render time was    : "); Serial.print(drawTime); Serial.println(" ms");
  Serial.println("=====================================");

}

