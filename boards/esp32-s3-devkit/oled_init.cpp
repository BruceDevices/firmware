#include <Wire.h>
#include <Adafruit_SH110X.h>

Adafruit_SH1106G sh1106_display(128, 64, &Wire, -1);

bool initOLED() {
    Wire.begin(8, 9);
    Wire.setClock(400000);

    if (!sh1106_display.begin(0x3C, true)) {
        Serial.println("ERROR: SH1106 OLED failed!");
        return false;
    }

    sh1106_display.clearDisplay();
    sh1106_display.setTextSize(1);
    sh1106_display.setTextColor(SH110X_WHITE);
    sh1106_display.setCursor(0, 0);
    sh1106_display.println("Bruce - ESP32-S3");
    sh1106_display.display();

    Serial.println("OLED initialized!");
    return true;
}
