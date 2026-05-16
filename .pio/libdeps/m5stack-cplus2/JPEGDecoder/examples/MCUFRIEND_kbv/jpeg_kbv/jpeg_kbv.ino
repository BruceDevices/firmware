// This sketch draws a Jpeg file stored in an array onto the TFT screen using the
// MCUFRIEND_kbv library:
// https://github.com/prenticedavid/MCUFRIEND_kbv

// The MCUFRIEND_kbv library is compatible with many different screens with
// different pixelwidths and heights.

// This sketch renders a 240 x 320 pixel image on the screen in portrait orientation.
// It has been tested on a Mega and Due with an 8 bit ILI9481 3.5" TFT shield.
// Note: an UNO does not have enough memory to run this sketch.

// Created by Bodmer 5th Feb 2017
  
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

#include <SD.h> // Not used in this example

#include <JPEGDecoder.h>  // JPEG decoder library

// Include the sketch header file that contains the image stored as an array of bytes
// More than one image array could be stored in each header file.
#include "jpeg1.h"

uint16_t ID;

void setup(void)
{
    Serial.begin(9600);
    tft.reset();
    ID = tft.readID();
    Serial.print("TFT ID = 0x");
    Serial.println(ID, HEX);
    //    if (ID == 0x00D3) ID = 0x9481; // write-only shield
    if (ID == 0x00D3) ID = 0x9486; // write-only shield
    tft.begin(ID);
    tft.setRotation(0);
}

void loop(void)
{
  uint32_t clearTime = millis();
  tft.fillScreen(random(0x10000));
  clearTime = millis() - clearTime; // Calculate the time it took

  // print the results to the serial port
  Serial.print(F(  "Total clear time was    : ")); Serial.print(clearTime); Serial.println(F(" ms"));
  Serial.println(F(""));
  
  // Draw a jpeg image stored in memory
  drawArrayJpeg(Baboon, sizeof(Baboon), 0, 0);

  delay(5000);
}

