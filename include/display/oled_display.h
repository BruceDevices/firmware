#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>

class OLEDDisplay {
private:
    Adafruit_SH1106G* display;
    uint16_t width;
    uint16_t height;
    uint16_t textColor;
    uint16_t bgColor;

public:
    OLEDDisplay(Adafruit_SH1106G* disp) : display(disp), width(128), height(64), 
                                          textColor(SH110X_WHITE), bgColor(SH110X_BLACK) {}

    void begin() {
        if (!display->begin(0x3C, true)) {
            Serial.println("SH1106 init failed!");
            while (1);
        }
        display->clearDisplay();
    }

    void fillScreen(uint16_t color) {
        if (color == 0) display->clearDisplay();
        else display->fillRect(0, 0, width, height, color);
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        display->writePixel(x, y, color);
    }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
        display->drawLine(x0, y0, x1, y1, color);
    }

    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        display->drawRect(x, y, w, h, color);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        display->fillRect(x, y, w, h, color);
    }

    void setTextSize(uint8_t size) {
        display->setTextSize(size);
    }

    void setTextColor(uint16_t color, uint16_t bg = 0) {
        textColor = color;
        bgColor = bg;
        display->setTextColor(color, bg);
    }

    void setCursor(int16_t x, int16_t y) {
        display->setCursor(x, y);
    }

    void print(const char* str) {
        display->print(str);
    }

    void println(const char* str) {
        display->println(str);
    }

    void println() {
        display->println();
    }

    void display() {
        this->display->display();
    }

    void clearDisplay() {
        display->clearDisplay();
    }

    uint16_t getWidth() const { return width; }
    uint16_t getHeight() const { return height; }
    uint16_t w() const { return width; }
    uint16_t h() const { return height; }
};

#endif
