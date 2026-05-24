// glass2.h — M5Stack Glass2 Unit secondary display
// 1.51" transparent OLED, SSD1309 driver, 128x64, I2C 0x3C
// Grove port: SDA=GPIO2, SCL=GPIO1 (Cardputer ADV)
//
// Requires: M5GFX (ships M5UnitGLASS2.h — already a dependency via M5Unified)
// No additional library needed.
//
// Usage:
//   glass2Init();
//   glass2Show("MODE: KARMA", "SSID: Home...", "CLTS: 3", "CRED: 1");
//   glass2Clear();

#pragma once

#include <M5UnitGLASS2.h>

#define GLASS2_SDA  2
#define GLASS2_SCL  1

static M5UnitGLASS2 _g2(GLASS2_SDA, GLASS2_SCL);
static bool _g2_ready = false;

inline void glass2Init() {
    _g2_ready = _g2.init();
    if (_g2_ready) {
        _g2.fillScreen(TFT_BLACK);
        _g2.setTextColor(TFT_WHITE, TFT_BLACK);
    }
}

// Show up to 4 lines. Pass nullptr to leave a line blank.
inline void glass2Show(const char* l1,
                       const char* l2 = nullptr,
                       const char* l3 = nullptr,
                       const char* l4 = nullptr) {
    if (!_g2_ready) return;
    _g2.fillScreen(TFT_BLACK);
    _g2.setTextSize(1);
    _g2.setTextColor(TFT_WHITE, TFT_BLACK);
    if (l1) { _g2.setCursor(0,  0); _g2.print(l1); }
    if (l2) { _g2.setCursor(0, 16); _g2.print(l2); }
    if (l3) { _g2.setCursor(0, 32); _g2.print(l3); }
    if (l4) { _g2.setCursor(0, 48); _g2.print(l4); }
}

inline void glass2Clear() {
    if (!_g2_ready) return;
    _g2.fillScreen(TFT_BLACK);
}
