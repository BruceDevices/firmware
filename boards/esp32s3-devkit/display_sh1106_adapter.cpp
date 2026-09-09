#include "display_sh1106_adapter.h"
#include <Adafruit_GFX.h>

// Use Adafruit SH1106 G driver for 128x64
static Adafruit_SH1106G sh1106(128, 64, &Wire, -1);

bool initOLED() {
    Wire.begin(SDA, SCL);
    Wire.setClock(400000);

    if (!sh1106.begin(0x3C, true)) {
        Serial.println("ERROR: SH1106 OLED failed to initialize");
        return false;
    }

    sh1106.clearDisplay();
    sh1106.setTextSize(1);
    sh1106.setTextColor(SH110X_WHITE);
    sh1106.setCursor(0, 0);
    sh1106.println("SH1106 initialized");
    sh1106.display();
    delay(50);
    return true;
}

void displayClear() {
    sh1106.clearDisplay();
}

void displaySetCursor(int x, int y) {
    sh1106.setCursor(x, y);
}

void displayPrint(const String &s) {
    sh1106.print(s);
}

void displayDisplay() {
    sh1106.display();
}

void displayDrawRect(int x, int y, int w, int h) {
    sh1106.drawRect(x, y, w, h, SH110X_WHITE);
}

void displayFillRect(int x, int y, int w, int h) {
    sh1106.fillRect(x, y, w, h, SH110X_WHITE);
}

void displayDrawCentreString(const String &s, int cx, int cy) {
    // Approximate width: 6 pixels per char at size 1. Use current text size.
    uint8_t ts = sh1106.getTextSize();
    int pw = s.length() * 6 * ts;
    int px = cx - (pw / 2);
    // Use baseline y as cy - (font height/2) ~ font height 8*ts
    int py = cy - (8 * ts) / 2;
    sh1106.setCursor(px, py);
    sh1106.print(s);
}
