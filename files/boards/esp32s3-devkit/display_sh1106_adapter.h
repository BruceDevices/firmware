#ifndef DISPLAY_SH1106_ADAPTER_H
#define DISPLAY_SH1106_ADAPTER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include "pins_arduino.h"

// Adapter public API
bool initOLED();
void displayClear();
void displaySetCursor(int x, int y);
void displayPrint(const String &s);
void displayDisplay();
void displayDrawRect(int x, int y, int w, int h);
void displayFillRect(int x, int y, int w, int h);
void displayDrawCentreString(const String &s, int cx, int cy);

#endif /* DISPLAY_SH1106_ADAPTER_H */
