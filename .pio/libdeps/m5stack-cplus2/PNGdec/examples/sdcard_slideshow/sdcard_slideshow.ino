// A simple image slideshow which reads all the .JPG files from the root
// directory of a SD card and shows each for 1 second on an ILI9341 display.

#include <PNGdec.h>
#include <ILI9341_t3.h>
#include <Bounce.h>
#include <SD.h>

#define TFT_DC  9
#define TFT_CS 10

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
PNG png;

// Setup - initialize ILI9341 display, wait for serial monitor, open SD card
void setup() {
  pinMode(34, INPUT_PULLDOWN);
  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH); // pushbuttons short pins 33 & 34 together
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.println("Waiting for Arduino Serial Monitor...");

  while (!Serial && millis() < 3000); // wait up to 3 seconds for Arduino Serial Monitor
  Serial.println("ILI9341 Slideshow");
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);

  while (!SD.begin(4/*BUILTIN_SDCARD*/)) {
    Serial.println("Unable to access SD Card");
    tft.println("Unable to access SD Card");
    delay(1000);
  }
}

// Functions to access a file on the SD card
File myfile;

void * myOpen(const char *filename, int32_t *size) {
  Serial.printf("Attempting to open %s\n", filename);
  myfile = SD.open(filename);
  *size = myfile.size();
  return &myfile;
}
void myClose(void *handle) {
  if (myfile) myfile.close();
}
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!myfile) return 0;
  return myfile.read(buffer, length);
}
int32_t mySeek(PNGFILE *handle, int32_t position) {
  if (!myfile) return 0;
  return myfile.seek(position);
}

// Function to draw pixels to the display
int PNGDraw(PNGDRAW *pDraw) {
uint16_t usPixels[320];

  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
  tft.writeRect(0, pDraw->y + 24, pDraw->iWidth, 1, usPixels);
  return 1;
}

// Main loop, scan for all .PNG files on the card and display them
void loop() {
  int rc, filecount = 0;
  tft.setCursor(0, 0);
  File dir = SD.open("/");
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    if (entry.isDirectory() == false) {
      const char *name = entry.name();
      const int len = strlen(name);
      if (len > 3 && strcmp(name + len - 3, "PNG") == 0) {
        Serial.print("File: ");
        Serial.println(name);
        tft.fillScreen(ILI9341_BLACK);
        tft.setCursor(0, 0);
        tft.print("File: ");
        tft.println(name);
        rc = png.open((const char *)name, myOpen, myClose, myRead, mySeek, PNGDraw);
       if (rc == PNG_SUCCESS) {
          Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
          rc = png.decode(NULL, 0);
          png.close();
       }
        filecount = filecount + 1;
//        if (digitalRead(34) == LOW) {
          // skip delay between images when pushbutton is pressed
          delay(2000);
//        }
      }
    }
    entry.close();
  }
  if (filecount == 0) {
    Serial.println("No .PNG files found");
    tft.println("No .PNG files found");
    delay(2000);
  }
}
