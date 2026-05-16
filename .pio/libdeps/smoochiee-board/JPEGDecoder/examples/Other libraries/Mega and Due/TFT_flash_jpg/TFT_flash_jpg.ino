// Sketch to display images on a 480 x 320 HX8357 or ILI9481
// 16 bit parallel TFT by Bodmer (aka rowboteer)
// Version 0.10c 23/1/17

// Renders images stored in an array in program (FLASH)
// JPEG images are stored in header files (see jpeg1.h etc)

// The sketch does not need the SD or sdFat libraries since it does not access an
// SD Card. You can edit the "User_Config.h" file inside the JPEGDecoder library folder
// to stop the decode loading the SD or sdFat library.
// See comments in "User_Config.h". Editting the User_Config.h file is optional
// and NOT essential but it will save some FLASH space.

// Mega TFT library here:
// https://github.com/Bodmer/TFT_HX8357

// Due TFT library here:
// https://github.com/Bodmer/TFT_HX8357_Due

// This example draws a jpeg compressed image stored in memory onto the screen
// the image is 480 x 320 pixels.

// As an example Baboon40.jpg is compressed from ~460 kBytes (24 bit colour) to a mere
// 24.4 kBytes (~19 times smaller), Mouse480.jpg is 6.45 kBytes (~70 times smaller)

// As well as the HX8357 TFT library you will need the JPEG Decoder library.
// A copy can be downloaded here, it is based on the library by Makoto Kurauchi.
// The following copy has some bug fixes and extra/adapted member functions:
// https://github.com/Bodmer/JPEGDecoder

// As supplied this sketch will draw 4 images to screen in 4 different sizes
// Note the whole JPEG has to be decoded to draw scaled images so drawing scaled
// pictures is not significantly faster than drawing the 1:1 (unscaled) image

//----------------------------------------------------------------------------------------------------

#include <SPI.h>
#include <arduino.h>

// Next the libraries are selected depending on whether it is an AVR (Mega) or otherwise a Due

#if defined (ARDUINO_ARCH_AVR)
  // Mega libraries
  #include <TFT_HX8357.h>        // Hardware-specific Mega library
  TFT_HX8357 tft = TFT_HX8357(); // Invoke custom Mega library
#elif defined (ARDUINO_ARCH_SAM)
  // Due libraries
  #include <TFT_HX8357_Due.h>    // Hardware-specific Due library
  TFT_HX8357_Due tft = TFT_HX8357_Due(); // Invoke custom Due library
#endif

// JPEG decoder library
#include <JPEGDecoder.h>

// Chip Select Pin for SD card
#define SD_CS 53

// Include the sketch header file that contains the image stored as an array of bytes
// More than one image array could be stored in each header file.
#include "jpeg1.h"
#include "jpeg2.h"
#include "jpeg3.h"
#include "jpeg4.h"

// Count how many times the image is drawn for test purposes
uint32_t icount = 0;
//----------------------------------------------------------------------------------------------------


//####################################################################################################
// Setup
//####################################################################################################
void setup() {
  Serial.begin(115200);
  tft.begin();
}

//####################################################################################################
// Main loop
//####################################################################################################
void loop() {

  tft.setRotation(2);  // portrait
  tft.fillScreen(random(0xFFFF));

  // The image is 300 x 300 pixels so we do some sums to position image in the middle of the screen!
  // Doing this by reading the image width and height from the jpeg info is left as an exercise!
  int x = (tft.width()  - 300) / 2 - 1;
  int y = (tft.height() - 300) / 2 - 1;

  drawArrayJpeg(EagleEye, sizeof(EagleEye), x, y); // Draw a jpeg image stored in memory at x,y
  delay(2000);


  tft.setRotation(2);  // portrait
  tft.fillScreen(random(0xFFFF));
  drawArrayJpeg(Baboon40, sizeof(Baboon40), 0, 0); // Draw a jpeg image stored in memory
  delay(2000);


  tft.setRotation(2);  // portrait
  tft.fillScreen(random(0xFFFF));
  drawArrayJpeg(lena20k, sizeof(lena20k), 0, 0); // Draw a jpeg image stored in memory
  delay(2000);

  tft.setRotation(1);  // landscape
  tft.fillScreen(random(0xFFFF));

  // This image will be deliberately cropped as it is 480 x 320 thes extends off the screen when plotted
  // at coordinate 100,100
  drawArrayJpeg(Mouse480, sizeof(Mouse480), 100, 100); // Draw a jpeg image stored in memory, test cropping
  //drawArrayJpeg(Mouse480, sizeof(Mouse480), 0, 0); // Draw a jpeg image stored in memory
  delay(2000);
}

//####################################################################################################
// Draw a JPEG on the TFT pulled from a program memory array
//####################################################################################################
void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos) {

  int x = xpos;
  int y = ypos;

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
  while (JpegDec.read()) {
	  
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
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
    {
      // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
      tft.setWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);

      // Write all MCU pixels to the TFT window
      while (mcu_pixels--) {
        // Push each pixel to the TFT MCU area
        tft.pushColor(*pImg++);
      }

    }
    else if ( (mcu_y + win_h) >= tft.height()) JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime;

  // print the results to the serial port
  Serial.print(F(  "Total render time was    : ")); Serial.print(drawTime); Serial.println(F(" ms"));
  Serial.println(F(""));
}

//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
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

//####################################################################################################
// Show the execution time (optional)
//####################################################################################################
// WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
// sketch sizes greater than ~70KBytes because 16 bit address pointers are used in some libraries.

// The Due will work fine with the HX8357_Due library.

void showTime(uint32_t msTime) {
  //tft.setCursor(0, 0);
  //tft.setTextFont(1);
  //tft.setTextSize(2);
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.print(F(" JPEG drawn in "));
  //tft.print(msTime);
  //tft.println(F(" ms "));
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}

