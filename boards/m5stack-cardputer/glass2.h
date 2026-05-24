// glass2.h — M5Stack Glass2 Unit secondary display
// 1.51" transparent OLED, SSD1309 driver, 128x64, I2C 0x3C
// Grove port: SDA=GPIO2, SCL=GPIO1 (Cardputer ADV)
//
// Requires: Adafruit SSD1306 library (Arduino Library Manager)
// SSD1309 is protocol-compatible with SSD1306 driver.
//
// Usage:
//   glass2Init();          // call once in setup()
//   glass2Show("MODE: KARMA", "SSID: Home...", "CLTS: 3", "CRED: 1");
//   glass2Clear();         // blank the display

#pragma once

#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define GLASS2_SDA    2
#define GLASS2_SCL    1
#define GLASS2_ADDR   0x3C
#define GLASS2_WIDTH  128
#define GLASS2_HEIGHT 64

static Adafruit_SSD1306 _g2(GLASS2_WIDTH, GLASS2_HEIGHT, &Wire, -1);
static bool _g2_ready = false;

inline void glass2Init() {
    Wire.begin(GLASS2_SDA, GLASS2_SCL);
    _g2_ready = _g2.begin(SSD1306_SWITCHCAPVCC, GLASS2_ADDR);
    if (_g2_ready) {
        _g2.clearDisplay();
        _g2.setTextColor(SSD1306_WHITE);
        _g2.display();
    }
}

// Show up to 4 lines. Pass nullptr to leave a line blank.
inline void glass2Show(const char* l1,
                       const char* l2 = nullptr,
                       const char* l3 = nullptr,
                       const char* l4 = nullptr) {
    if (!_g2_ready) return;
    _g2.clearDisplay();
    _g2.setTextSize(1);
    _g2.setTextColor(SSD1306_WHITE);
    if (l1) { _g2.setCursor(0,  0); _g2.println(l1); }
    if (l2) { _g2.setCursor(0, 16); _g2.println(l2); }
    if (l3) { _g2.setCursor(0, 32); _g2.println(l3); }
    if (l4) { _g2.setCursor(0, 48); _g2.println(l4); }
    _g2.display();
}

inline void glass2Clear() {
    if (!_g2_ready) return;
    _g2.clearDisplay();
    _g2.display();
}
