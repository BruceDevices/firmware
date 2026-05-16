/*
  Descendant of SerialCsvOut.ino
  Sample code of JPEG Decoder for Arduino
  Public domain, Makoto Kurauchi <http://yushakobo.jp>
*/

// Adapted to display images on a 480 x 320 HX8357 or ILI9481
// 16 bit parallel TFT by Bodmer (aka rowboteer)
// Version 0.05b 15/2/16

// This sketch renders JPEG images stored on an SD Card.

// Mega TFT library here:
// https://github.com/Bodmer/TFT_HX8357

// Due TFT library here:
// https://github.com/Bodmer/TFT_HX8357_Due

// As an example Baboon40.jpg is compressed from ~460 kBytes (24 bit colour) to a mere
// 24.4 kBytes (~19 times smaller), Mouse480.jpg is 6.45 kBytes (~70 times smaller)

// As well as the HX8357 TFT library you will need the JPEG Decoder library.
// A copy can be downloaded here, it is based on the library by Makoto Kurauchi.
// The following copy has some bug fixes and extra/adapted member functions:
// https://github.com/Bodmer/JPEGDecoder

// The Mega implementation uses the stock SD library built into the IDE
// A preconfigured copy of the SdFat library to work with the Due is available here:
// https://github.com/Bodmer/SdFat

// A function called createArray() is provided to covert a jpeg file stored on SD Card
// into a C array that can be pasted into any jpegX.h header file. The array is sent
// to the Serial Monitor Window.

// Images on SD Card must be put in the root folder (top level) to be found
// Use the SdFat or SD library examples to verify your SD Card interface works!

// Uncomment the #define LOAD_SDFAT_LIBRARY in User_Config.h in the JPEGDecoder src folder
// if the Due is used with the SdFat library. SdFat allows bit bashing the SPI if needed.

// The example images used to test this sketch can be found in the library
// JPEGDecoder/extras folder
//----------------------------------------------------------------------------------------------------

#include <SPI.h>
#include <arduino.h>

// Next the libraries are selected depending on whether it is an AVR (Mega) or otherwise a Due
// >>>> Note: This works OK with IDE 1.6.7 but might produce errors with other IDE versions. <<<<
// >>>> If you get errors here then edit or comment out the lines not needed.                <<<<


#if defined (ARDUINO_ARCH_AVR)
  // Mega libraries
  #include <SD.h>                // Use the SD library for the Mega
  #include <TFT_HX8357.h>        // Hardware-specific Mega library
  TFT_HX8357 tft = TFT_HX8357(); // Invoke custom Mega library
#elif defined (ARDUINO_ARCH_SAM)
  // Due libraries
  #include <SdFat.h>             // Use the SdFat library for the Due, see sketch header notes above
  SdFat SD;                      // Permit SD function call for the Due
  #include <TFT_HX8357_Due.h>    // Hardware-specific Due library
  TFT_HX8357_Due tft = TFT_HX8357_Due(); // Invoke custom Due library
#endif

// JPEG decoder library
#include <JPEGDecoder.h>

// Chip Select Pin for SD card
byte SD_CS = 53; //Mega256 requirement

// Count how many times the image is drawn for test purposes
uint32_t icount = 0;
//----------------------------------------------------------------------------------------------------
#define TFT_BLACK   0x0000
#define TFT_BLUE    0x001F
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_CYAN    0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_YELLOW  0xFFE0  
#define TFT_WHITE   0xFFFF
//----------------------------------------------------------------------------------------------------


//####################################################################################################
// Setup
//####################################################################################################
void setup() {
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);  // landscape
  tft.fillScreen(TFT_BLACK);

  Serial.print("Initialising SD card...");

  if (!SD.begin(SD_CS)) {
    Serial.println("initialisation failed!");
    return;
  }

  Serial.println("initialisation done.");
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

  drawSdJpeg("EagleEye.jpg", x, y);     // This draws a jpeg pulled off the SD Card
  // createArray("EagleEye.jpg");  // This pulls a jpeg image off the SD Card and serial dumps an array
  delay(2000);

  tft.setRotation(2);  // portrait
  tft.fillScreen(random(0xFFFF));
  drawSdJpeg("Baboon40.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card
  // createArray("Baboon40.jpg");  // This pulls a jpeg image off the SD Card and serial dumps an array
  delay(2000);

  tft.setRotation(2);  // portrait
  tft.fillScreen(random(0xFFFF));
  drawSdJpeg("lena20k.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card
  //createArray("lena20k.jpg");  // This pulls a jpeg image off the SD Card and serial dumps an array
  delay(2000);

  tft.setRotation(1);  // landscape
  tft.fillScreen(random(0xFFFF));
  drawSdJpeg("Mouse480.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card

  
  createArray("Mouse480.jpg");  // This pulls a jpeg image off the SD Card and serial dumps in
                                // the correct format for a C array.
  delay(2000);

  while(1); // Wait here
}

//####################################################################################################
// Draw a JPEG on the TFT pulled from SD Card
//####################################################################################################
// xpos, ypos is top left corner of plotted image
void drawSdJpeg(char *filename, int xpos, int ypos) {

  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library
 
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");

  // Use one of the following methods to initialise the decoder:
  boolean decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  //boolean decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)

  if (decoded) {
    // print information about the image to the serial port
    jpegInfo();
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {

  //jpegInfo(); // Print information from the JPEG file (could comment this line out)

  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  uint32_t min_w = min(mcu_w, max_x % mcu_w);
  uint32_t min_h = min(mcu_h, max_y % mcu_h);

  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  uint32_t drawTime = millis();

  max_x += xpos;
  max_y += ypos;

  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    uint32_t mcu_pixels = win_w * win_h;

    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
    {
      // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
      tft.setWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);

      // Push all MCU pixels to the TFT window
      while (mcu_pixels--) {
        // Push each pixel to the TFT MCU area
        tft.pushColor(*pImg++);
      }

      // Push all MCU pixels to the TFT window, ~18% faster to pass an array pointer and length to the library
      // tft.pushColor16(pImg, mcu_pixels); //  To be supported in HX8357 library at a future date

    }
    else if ( (mcu_y + win_h) >= tft.height()) JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  showTime(millis() - drawTime); // These lines are for sketch testing only
  //Serial.print(" Draw count:");
  //Serial.println(icount++);
}

//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
// JpegDec.decodeFile(...) or JpegDec.decodeArray(...) must be called before this info is available!
void jpegInfo() {

  // Print information extracted from the JPEG file
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
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

//####################################################################################################
// Pull a jpeg file off the SD Card and send it as a "C" formatted as an array to the serial port
//####################################################################################################
// The array can be cut and pasted from the Serial Monitor window into jpegX.h attached to this sketch
void createArray(const char *filename) {
  File jpgFile;
  uint8_t  sdbuffer[32];   // SD read pixel buffer (16 bits per pixel)

  // Check file exists and open it
  if ((jpgFile = SD.open(filename)) == NULL) {
    Serial.println(F("JPEG file not found"));
    return;
  }

  uint8_t data;
  byte line_len = 0;

  Serial.print("const uint8_t ");
  // Make the array the same as the file name with the .(file extension) removed
  while (*filename != '.') Serial.print(*filename++);

  Serial.println("[] PROGMEM = {"); // PROGMEM added for AVR processors, it is ignored by Due
  // Pull all data falues from file and print in the array format
  while (jpgFile.available()) {
    data = jpgFile.read();
    Serial.print("0x");                        // Add hexadecimal prefix
    if (abs(data) < 16) Serial.print("0");     // Add a leading zero to create a neater array
    Serial.print(data, HEX); Serial.print(",");// Add value and comma
    line_len++;
    // Add a newline every 32 bytes
    if (line_len >= 32) {
      line_len = 0;
      Serial.println();
    }
  }
  Serial.println("};");
  Serial.println();
  // close the file:
  jpgFile.close();
}

