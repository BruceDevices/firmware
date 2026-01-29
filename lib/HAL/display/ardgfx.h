#ifndef LIB_HAL_ARDGFX_H
#define LIB_HAL_ARDGFX_H
#include <pins_arduino.h>

class TFT_eSPI;
class tft_sprite;
class tft_logger;

#if defined(USE_ARDUINO_GFX)
#include "tft_defines.h"
#include <Arduino_GFX_Library.h>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <memory>
// clang-format off
// Check Data bus
#if !defined(TFT_DATABUS_N) || TFT_DATABUS_N > 4
    #warning "Please define the Data bus used for this board:\n \
    - 0: Arduino_HWSPI: Shared SPI with other devices;\n \
    - 1: Arduino_ESP32QSPI: Quad SPI, used in AMOLED and some new displays;\n \
    - 2: Arduino_ESP32PAR8Q: Parallel 8 bit;\n \
    - 3: Arduino_ESP32RGBPanel: Parallel 16 bit;\n \
    - 4: Arduino_ESP32DSIPanel: DSI Panel for P4 devices;\n \
    \n \
    Using Arduino_HWSPI as default."
    #define TFT_DATABUS_N 0
#endif

#if TFT_DATABUS_N == 0
    #define TFT_DATABUS Arduino_HWSPI
    #if !defined(TFT_DC) || !defined(TFT_CS) || !defined(TFT_SCLK) || !defined(TFT_MOSI) || !defined(TFT_MISO)
        #error "Missing Macros, please check the definitions of: TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO"
    #endif
#elif TFT_DATABUS_N == 1
    #define TFT_DATABUS Arduino_ESP32QSPI
    #if !defined(TFT_RST) || !defined(TFT_ROTATION) || !defined(TFT_IPS) || !defined(TFT_WIDTH) ||               \
        !defined(TFT_HEIGHT) || !defined(TFT_COL_OFS1) || !defined(TFT_ROW_OFS1) || !defined(TFT_COL_OFS2) ||    \
        !defined(TFT_ROW_OFS2)
        #error "Missing Macros definitions of: TFT_RST, TFT_ROTATION, TFT_IPS, TFT_WIDTH,\
                TFT_HEIGHT, TFT_COL_OFS1, TFT_ROW_OFS1, TFT_COL_OFS2,TFT_ROW_OFS2 "
    #endif
    #if !defined(TFT_CS) ||  !defined(TFT_SCLK) || !defined(TFT_D0) || !defined(TFT_D1) || !defined(TFT_D2) || !defined(TFT_D3)
        #error "Missing Definitions for: TFT_CS, TFT_SCLK, TFT_D0, TFT_D1, TFT_D2, TFT_D3"
    #endif

#elif TFT_DATABUS_N == 2
    #define TFT_DATABUS Arduino_ESP32PAR8Q
    #if !defined(TFT_DC) || !defined(TFT_CS) || !defined(TFT_WR) || !defined(TFT_RD) || !defined(TFT_D0) || \
    !defined(TFT_D1) || !defined(TFT_D2) || !defined(TFT_D3) || !defined(TFT_D4) || !defined(TFT_D5) || \
    !defined(TFT_D6) || !defined(TFT_D7)
        #error "Missing definitions for: TFT_DC, TFT_CS, TFT_WR, TFT_RD, TFT_D0, \
                TFT_D1, TFT_D2, TFT_D3, TFT_D4, TFT_D5, TFT_D6, TFT_D7"
    #endif
    #if !defined(TFT_RST) || !defined(TFT_ROTATION) || !defined(TFT_IPS) || !defined(TFT_WIDTH) ||               \
        !defined(TFT_HEIGHT) || !defined(TFT_COL_OFS1) || !defined(TFT_ROW_OFS1) || !defined(TFT_COL_OFS2) ||    \
        !defined(TFT_ROW_OFS2)
        #error "Missing Macros definitions of: TFT_RST, TFT_ROTATION, TFT_IPS, TFT_WIDTH,\
                TFT_HEIGHT, TFT_COL_OFS1, TFT_ROW_OFS1, TFT_COL_OFS2,TFT_ROW_OFS2 "
    #endif

#elif TFT_DATABUS_N == 3
    #define TFT_DATABUS Arduino_ESP32RGBPanel
// for ST7262 panels (SUNTON boards) R and B are changed
    #if !defined(TFT_DE) || !defined(TFT_VSYNC) || !defined(TFT_HSYNC) || !defined(TFT_PCLK) || !defined(TFT_R0) || \
    !defined(TFT_R1) || !defined(TFT_R2) || !defined(TFT_R3) || !defined(TFT_R4) || !defined(TFT_G0) || \
    !defined(TFT_G1) || !defined(TFT_G2) || !defined(TFT_G3) || !defined(TFT_G4) || !defined(TFT_G5) || \
    !defined(TFT_B0) || !defined(TFT_B1) || !defined(TFT_B2) || !defined(TFT_B3) || !defined(TFT_B4) || \
    !defined(TFT_HSYNC_POL) || !defined(TFT_HSYNC_FRONT_PORCH) || !defined(TFT_HSYNC_PULSE_WIDTH) || \
    !defined(TFT_HSYNC_BACK_PORCH) || !defined(TFT_VSYNC_POL) || !defined(TFT_VSYNC_FRONT_PORCH) || \
    !defined(TFT_VSYNC_PULSE_WIDTH) || !defined(TFT_VSYNC_BACK_PORCH) || !defined(TFT_PCLK_ACTIVE_NEG) || \
    !defined(TFT_PREF_SPEED)
        #error "Missing Definitions for: \n \
                TFT_DE, TFT_VSYNC, TFT_HSYNC, TFT_PCLK,\n \
                TFT_R0, TFT_R1, TFT_R2, TFT_R3, TFT_R4,\n \
                TFT_G0, TFT_G1, TFT_G2, TFT_G3, TFT_G4, TFT_G5,\n \
                TFT_B0, TFT_B1, TFT_B2, TFT_B3, TFT_B4,\n \
                TFT_HSYNC_POL,\n \
                TFT_HSYNC_FRONT_PORCH,\n \
                TFT_HSYNC_PULSE_WIDTH,\n \
                TFT_HSYNC_BACK_PORCH,\n \
                TFT_VSYNC_POL,\n \
                TFT_VSYNC_FRONT_PORCH,\n \
                TFT_VSYNC_PULSE_WIDTH,\n \
                TFT_VSYNC_BACK_PORCH,\n \
                TFT_PCLK_ACTIVE_NEG,\n \
                TFT_PREF_SPEED"
    #endif

    #if !defined(TFT_WIDTH) || !defined(TFT_HEIGHT)
        #error "Missing Macros definitions of: TFT_WIDTH, TFT_HEIGHT"
    #endif
#elif TFT_DATABUS_N == 4
    #include "ardgfx_inits.h"
    #define TFT_DATABUS_TYPE Arduino_ESP32DSIPanel
    #define TFT_DATABUS Arduino_ESP32DSIPanel
    #if !defined(TFT_HSYNC_PULSE_WIDTH) || !defined(TFT_HSYNC_BACK_PORCH) || !defined(TFT_HSYNC_FRONT_PORCH) || \
        !defined(TFT_VSYNC_PULSE_WIDTH) || !defined(TFT_VSYNC_BACK_PORCH) || !defined(TFT_VSYNC_FRONT_PORCH) || \
        !defined(TFT_PREF_SPEED)
        #error "Missing Definitions for: \n \
                TFT_HSYNC_FRONT_PORCH,\n \
                TFT_HSYNC_PULSE_WIDTH,\n \
                TFT_HSYNC_BACK_PORCH,\n \
                TFT_VSYNC_FRONT_PORCH,\n \
                TFT_VSYNC_PULSE_WIDTH,\n \
                TFT_VSYNC_BACK_PORCH,\n \
                TFT_PREF_SPEED"
    #endif

    #if !defined(TFT_WIDTH) || !defined(TFT_HEIGHT) || !defined(TFT_RST) || !defined(TFT_DSI_INIT)
        #error "Missing Macros definitions of: TFT_WIDTH, TFT_HEIGHT, TFT_RST, TFT_DSI_INIT"
    #endif
    #ifndef TFT_DISPLAY_DRIVER_N
    #define TFT_DISPLAY_DRIVER_N 50
    #endif
#else
    #error "Invalid TFT_DATABUS_N\n Please define the Data bus used for this board:\n \
    - 0: Arduino_HWSPI: Shared SPI with other devices;\n \
    - 1: Arduino_ESP32QSPI: Quad SPI, used in AMOLED and some new displays;\n \
    - 2: Arduino_ESP32PAR8Q: Parallel 8 bit;\n \
    - 3: Arduino_ESP32RGBPanel: Parallel 16 bit;\n \
    - 4: Arduino_ESP32DSIPanel: DSI Panel for P4 devices;"
#endif

// Set Databus Type for constructor
#if defined(RGB_PANEL)
    #if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32P4)
        #error "MCU must be ESP32-S3 or ESP32-P4 to use RGB_PANEL"
    #endif
    #define TFT_DATABUS_TYPE Arduino_ESP32RGBPanel
#else
    #ifndef TFT_DATABUS_TYPE
        #define TFT_DATABUS_TYPE Arduino_DataBus
    #endif
#endif

#if !defined(TFT_DISPLAY_DRIVER_N)
    #error "Please define the Display Driver used by this board (TFT_DISPLAY_DRIVER_N):\n \
    [bus,rst,r,ips,w,h,ofs1,ofs2] 0-22\n \
    0 ST7735; 1 ST7789; 2 ST7796; 3 ST77916; 4 ILI9341; 5 GC9A01; 6 GC9C01; 7 GC9D01; 8 GC9106; 9 GC9107;\n \
    10 HX8347C; 11 HX8347D; 12 HX8352C; 13 HX8369A; 14 NT35310; 15 NT35510; 16 NT39125; 17 NV3007;\n \
    18 NV3023; 19 NV3041A; 20 OTM8009A; 21 JBT6K71; 22 AXS15231B;\n \
    [bus,rst,r,ips] 23-35\n \
    23 ILI9331; 24 ILI9342; 25 ILI9481_18bit; 26 ILI9486; 27 ILI9486_18bit; 28 ILI9488;\n \
    29 ILI9488_18bit; 30 ILI9488_3bit; 31 ILI9806; 32 HX8357A; 33 HX8357B; 34 R61529; 35 RM67162;\n \
    [bus,rst,r,w,h,ofs1,ofs2] 36-43\n \
    36 SSD1283A; 37 SSD1331; 38 SSD1351; 39 SH8601; 40 RM690B0; 41 CO5300; 42 JD9613; 43 SEPS525;\n \
    [bus,rst,r] 44 ILI9225;\n \
    [bus,rst] 45 SPD2010; 46 WEA2012;\n \
    [bus,rst,w,h] 47 SSD1306; 48 SH1106;\n \
    [RGB] 49 Arduino_RGB_Display (Arduino_ESP32RGBPanel: ST7262/ST7701);\n \
    [DSI] 50 Arduino_DSI_Display;\n \
    "
#endif

#if !defined(TFT_DISPLAY_DRIVER)
    #if TFT_DISPLAY_DRIVER_N == 0
        #define TFT_DISPLAY_DRIVER Arduino_ST7735
    #elif TFT_DISPLAY_DRIVER_N == 1
        #define TFT_DISPLAY_DRIVER Arduino_ST7789
    #elif TFT_DISPLAY_DRIVER_N == 2
        #define TFT_DISPLAY_DRIVER Arduino_ST7796
    #elif TFT_DISPLAY_DRIVER_N == 3
        #define TFT_DISPLAY_DRIVER Arduino_ST77916
    #elif TFT_DISPLAY_DRIVER_N == 4
        #define TFT_DISPLAY_DRIVER Arduino_ILI9341
    #elif TFT_DISPLAY_DRIVER_N == 5
        #define TFT_DISPLAY_DRIVER Arduino_GC9A01
    #elif TFT_DISPLAY_DRIVER_N == 6
        #define TFT_DISPLAY_DRIVER Arduino_GC9C01
    #elif TFT_DISPLAY_DRIVER_N == 7
        #define TFT_DISPLAY_DRIVER Arduino_GC9D01
    #elif TFT_DISPLAY_DRIVER_N == 8
        #define TFT_DISPLAY_DRIVER Arduino_GC9106
    #elif TFT_DISPLAY_DRIVER_N == 9
        #define TFT_DISPLAY_DRIVER Arduino_GC9107
    #elif TFT_DISPLAY_DRIVER_N == 10
        #define TFT_DISPLAY_DRIVER Arduino_HX8347C
    #elif TFT_DISPLAY_DRIVER_N == 11
        #define TFT_DISPLAY_DRIVER Arduino_HX8347D
    #elif TFT_DISPLAY_DRIVER_N == 12
        #define TFT_DISPLAY_DRIVER Arduino_HX8352C
    #elif TFT_DISPLAY_DRIVER_N == 13
        #define TFT_DISPLAY_DRIVER Arduino_HX8369A
    #elif TFT_DISPLAY_DRIVER_N == 14
        #define TFT_DISPLAY_DRIVER Arduino_NT35310
    #elif TFT_DISPLAY_DRIVER_N == 15
        #define TFT_DISPLAY_DRIVER Arduino_NT35510
    #elif TFT_DISPLAY_DRIVER_N == 16
        #define TFT_DISPLAY_DRIVER Arduino_NT39125
    #elif TFT_DISPLAY_DRIVER_N == 17
        #define TFT_DISPLAY_DRIVER Arduino_NV3007
    #elif TFT_DISPLAY_DRIVER_N == 18
        #define TFT_DISPLAY_DRIVER Arduino_NV3023
    #elif TFT_DISPLAY_DRIVER_N == 19
        #define TFT_DISPLAY_DRIVER Arduino_NV3041A
    #elif TFT_DISPLAY_DRIVER_N == 20
        #define TFT_DISPLAY_DRIVER Arduino_OTM8009A
    #elif TFT_DISPLAY_DRIVER_N == 21
        #define TFT_DISPLAY_DRIVER Arduino_JBT6K71
    #elif TFT_DISPLAY_DRIVER_N == 22
        #define TFT_DISPLAY_DRIVER Arduino_AXS15231B
    #elif TFT_DISPLAY_DRIVER_N == 23
        #define TFT_DISPLAY_DRIVER Arduino_ILI9331
    #elif TFT_DISPLAY_DRIVER_N == 24
        #define TFT_DISPLAY_DRIVER Arduino_ILI9342
    #elif TFT_DISPLAY_DRIVER_N == 25
        #define TFT_DISPLAY_DRIVER Arduino_ILI9481_18bit
    #elif TFT_DISPLAY_DRIVER_N == 26
        #define TFT_DISPLAY_DRIVER Arduino_ILI9486
    #elif TFT_DISPLAY_DRIVER_N == 27
        #define TFT_DISPLAY_DRIVER Arduino_ILI9486_18bit
    #elif TFT_DISPLAY_DRIVER_N == 28
        #define TFT_DISPLAY_DRIVER Arduino_ILI9488
    #elif TFT_DISPLAY_DRIVER_N == 29
        #define TFT_DISPLAY_DRIVER Arduino_ILI9488_18bit
    #elif TFT_DISPLAY_DRIVER_N == 30
        #define TFT_DISPLAY_DRIVER Arduino_ILI9488_3bit
    #elif TFT_DISPLAY_DRIVER_N == 31
        #define TFT_DISPLAY_DRIVER Arduino_ILI9806
    #elif TFT_DISPLAY_DRIVER_N == 32
        #define TFT_DISPLAY_DRIVER Arduino_HX8357A
    #elif TFT_DISPLAY_DRIVER_N == 33
        #define TFT_DISPLAY_DRIVER Arduino_HX8357B
    #elif TFT_DISPLAY_DRIVER_N == 34
        #define TFT_DISPLAY_DRIVER Arduino_R61529
    #elif TFT_DISPLAY_DRIVER_N == 35
        #define TFT_DISPLAY_DRIVER Arduino_RM67162
    #elif TFT_DISPLAY_DRIVER_N == 36
        #define TFT_DISPLAY_DRIVER Arduino_SSD1283A
    #elif TFT_DISPLAY_DRIVER_N == 37
        #define TFT_DISPLAY_DRIVER Arduino_SSD1331
    #elif TFT_DISPLAY_DRIVER_N == 38
        #define TFT_DISPLAY_DRIVER Arduino_SSD1351
    #elif TFT_DISPLAY_DRIVER_N == 39
        #define TFT_DISPLAY_DRIVER Arduino_SH8601
    #elif TFT_DISPLAY_DRIVER_N == 40
        #define TFT_DISPLAY_DRIVER Arduino_RM690B0
    #elif TFT_DISPLAY_DRIVER_N == 41
        #define TFT_DISPLAY_DRIVER Arduino_CO5300
    #elif TFT_DISPLAY_DRIVER_N == 42
        #define TFT_DISPLAY_DRIVER Arduino_JD9613
    #elif TFT_DISPLAY_DRIVER_N == 43
        #define TFT_DISPLAY_DRIVER Arduino_SEPS525
    #elif TFT_DISPLAY_DRIVER_N == 44
        #define TFT_DISPLAY_DRIVER Arduino_ILI9225
    #elif TFT_DISPLAY_DRIVER_N == 45
        #define TFT_DISPLAY_DRIVER Arduino_SPD2010
    #elif TFT_DISPLAY_DRIVER_N == 46
        #define TFT_DISPLAY_DRIVER Arduino_WEA2012
    #elif TFT_DISPLAY_DRIVER_N == 47
        #define TFT_DISPLAY_DRIVER Arduino_SSD1306
    #elif TFT_DISPLAY_DRIVER_N == 48
        #define TFT_DISPLAY_DRIVER Arduino_SH1106
    #elif TFT_DISPLAY_DRIVER_N == 49
        #define TFT_DISPLAY_DRIVER Arduino_RGB_Display
    #elif TFT_DISPLAY_DRIVER_N == 50
        #define TFT_DISPLAY_DRIVER Arduino_DSI_Display
    #else
        #error "Invalid TFT_DISPLAY_DRIVER_N"
    #endif
#endif
// clang-format on
class tft_display {
protected:
    TFT_DATABUS_TYPE *bus = nullptr;
    Arduino_GFX *_gfx = nullptr;
    uint16_t _height = TFT_HEIGHT;
    uint16_t _width = TFT_WIDTH;

public:
    explicit tft_display(int16_t _W = TFT_WIDTH, int16_t _H = TFT_HEIGHT);
    friend class tft_sprite;
    friend class tft_logger;

    void begin(uint32_t speed = 0);
    void init(uint8_t tc = 0);
    void setRotation(uint8_t r);
    void drawPixel(int32_t x, int32_t y, uint32_t color);
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color);
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color);
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
    void fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
    void fillScreen(uint32_t color);
    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color);
    void fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color);
    void drawArc(
        int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle,
        uint32_t fg_color, uint32_t bg_color, bool smoothArc = true
    );
    void drawWideLine(
        float ax, float ay, float bx, float by, float wd, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF
    );
    void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
    void drawXBitmap(
        int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg
    );
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8, uint16_t *cmap);
    void
    pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8, uint16_t *cmap);
    void invertDisplay(bool i);
    void sleep(bool value);
    void setSwapBytes(bool swap);
    bool getSwapBytes() const;
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) const;

    int16_t textWidth(const String &s, uint8_t font = 1) const;
    int16_t textWidth(const char *s, uint8_t font = 1) const;

    void setCursor(int16_t x, int16_t y);
    int16_t getCursorX() const;
    int16_t getCursorY() const;
    void setTextSize(uint8_t s);
    void setTextColor(uint16_t c);
    void setTextColor(uint16_t c, uint16_t b, bool bgfill = false);
    void setTextDatum(uint8_t d);
    uint8_t getTextDatum() const;
    void setTextFont(uint8_t f);
    void setTextWrap(bool wrapX, bool wrapY = false);
    int16_t drawString(const String &string, int32_t x, int32_t y, uint8_t font = 1);
    int16_t drawCentreString(const String &string, int32_t x, int32_t y, uint8_t font = 1);
    int16_t drawRightString(const String &string, int32_t x, int32_t y, uint8_t font = 1);

    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    template <typename T> size_t print(const T &val) { return _gfx ? _gfx->print(val) : 0; }
    template <typename T> size_t println(const T &val) { return _gfx ? _gfx->println(val) : 0; }
    size_t println();

    size_t printf(const char *fmt, ...);

    int16_t width() const;
    int16_t height() const;
    SPIClass &getSPIinstance() const;
    void writecommand(uint8_t c);

    uint32_t getTextColor() const;
    uint32_t getTextBgColor() const;
    uint8_t getTextSize() const;
    uint8_t getRotation() const;
    int16_t fontHeight(int16_t font = 1) const;
    Arduino_GFX *native();

private:
    template <typename Ptr> void pushImageFallback(int32_t x, int32_t y, int32_t w, int32_t h, Ptr data) {
        if (!_gfx || !data) return;
        for (int32_t row = 0; row < h; ++row) {
            for (int32_t col = 0; col < w; ++col) {
                uint16_t color = data[row * w + col];
                if (_swapBytes) color = static_cast<uint16_t>((color >> 8) | (color << 8));
                _gfx->drawPixel(x + col, y + row, color);
            }
        }
    }

    void drawWideLineFallback(float ax, float ay, float bx, float by, float wd, uint32_t color);

    int16_t drawAlignedString(const String &s, int32_t x, int32_t y, uint8_t datum);

    bool _swapBytes = false;
    uint32_t _textColor = 0xFFFF;
    uint32_t _textBgColor = 0x0000;
    uint8_t _textSize = 1;
    uint8_t _textDatum = 0;
    uint8_t _textFont = 1;
    uint8_t _rotation = 0;
};

class tft_sprite {
public:
    explicit tft_sprite(tft_display *parent);
    ~tft_sprite() = default;

    void *createSprite(int16_t w, int16_t h, uint8_t frames = 1);
    void deleteSprite();

    void fillScreen(uint32_t color);
    void setColorDepth(uint8_t depth);
    void setCursor(int16_t x, int16_t y);
    void setTextColor(uint16_t c);
    void setTextColor(uint16_t c, uint16_t b);
    void setTextSize(uint8_t s);
    void setTextDatum(uint8_t d);

    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color);

    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);

    void fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color);

    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);

    void pushSprite(int32_t x, int32_t y, uint32_t transparent = TFT_TRANSPARENT);

    void pushToSprite(tft_sprite *dest, int32_t x, int32_t y, uint32_t transparent = TFT_TRANSPARENT);

    int16_t width() const;
    int16_t height() const;
    int16_t fontHeight(int16_t font = 1) const;
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
    void drawPixel(int32_t x, int32_t y, uint32_t color);
    void drawXBitmap(
        int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg = 0
    );
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8, uint16_t *cmap);
    void
    pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8, uint16_t *cmap);
    void fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
    void fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
    int16_t drawString(const String &string, int32_t x, int32_t y, uint8_t font = 1);

private:
    bool _hasBuffer() const;

    void setPixel(int32_t x, int32_t y, uint16_t color);

    void drawHLine(int32_t x, int32_t y, int32_t w, uint16_t color);

    tft_display *_display = nullptr;
    int16_t _width = 0;
    int16_t _height = 0;
    std::vector<uint16_t> _buffer;
    uint8_t _colorDepth = 16;
    int16_t _cursorX = 0;
    int16_t _cursorY = 0;
    uint32_t _textColor = TFT_WHITE;
    uint32_t _textBgColor = TFT_BLACK;
    uint8_t _textSize = 1;
    uint8_t _textDatum = 0;
};
#endif
#endif // LIB_HAL_ARDGFX_H
