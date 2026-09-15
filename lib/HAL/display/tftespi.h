#ifndef LIB_HAL_TFTESPI_H
#define LIB_HAL_TFTESPI_H
#include <pins_arduino.h>
#ifdef USE_TFT_ESPI
#include <TFT_eSPI.h>

#include "tft_defines.h"
class tft_display : private TFT_eSPI {
public:
    explicit tft_display(int16_t _W = TFT_WIDTH, int16_t _H = TFT_HEIGHT);
    friend class tft_sprite;
    friend class tft_logger;

    using TFT_eSPI::begin;
    using TFT_eSPI::color565;
    using TFT_eSPI::drawArc;
    using TFT_eSPI::drawCentreString;
    using TFT_eSPI::drawCircle;
    using TFT_eSPI::drawEllipse;
    using TFT_eSPI::drawFastHLine;
    using TFT_eSPI::drawFastVLine;
    using TFT_eSPI::drawLine;
    using TFT_eSPI::drawPixel;
    using TFT_eSPI::drawRect;
    using TFT_eSPI::drawRightString;
    using TFT_eSPI::drawRoundRect;
    using TFT_eSPI::drawString;
    using TFT_eSPI::drawTriangle;
    using TFT_eSPI::drawWideLine;
    using TFT_eSPI::drawXBitmap;
    using TFT_eSPI::fillCircle;
    using TFT_eSPI::fillEllipse;
    using TFT_eSPI::fillRect;
    using TFT_eSPI::fillRectHGradient;
    using TFT_eSPI::fillRectVGradient;
    using TFT_eSPI::fillRoundRect;
    using TFT_eSPI::fillScreen;
    using TFT_eSPI::fillTriangle;
    using TFT_eSPI::fontHeight;
    using TFT_eSPI::getCursorX;
    using TFT_eSPI::getCursorY;
    using TFT_eSPI::getSwapBytes;
    using TFT_eSPI::getTextDatum;
    using TFT_eSPI::height;
    using TFT_eSPI::init;
    using TFT_eSPI::invertDisplay;
    using TFT_eSPI::print;
    using TFT_eSPI::printf;
    using TFT_eSPI::println;
    using TFT_eSPI::pushImage;
    using TFT_eSPI::setCursor;
    using TFT_eSPI::setRotation;
    using TFT_eSPI::setSwapBytes;
    using TFT_eSPI::setTextColor;
    using TFT_eSPI::setTextDatum;
    using TFT_eSPI::setTextFont;
    using TFT_eSPI::setTextSize;
    using TFT_eSPI::setTextWrap;
    using TFT_eSPI::sleep;
    using TFT_eSPI::textWidth;
    using TFT_eSPI::width;
    using TFT_eSPI::write;
    using TFT_eSPI::writecommand;

#ifdef TOUCH_CS
#if !defined(TFT_PARALLEL_8_BIT) && !defined(RP2040_PIO_INTERFACE) && !defined(TFT_PARALLEL_16_BIT)
    // Touchscreen Functions
    using TFT_eSPI::calibrateTouch;
    using TFT_eSPI::getTouch;
    using TFT_eSPI::getTouchRaw;
    using TFT_eSPI::setTouch;
#endif
#endif

#if !defined(TFT_PARALLEL_8_BIT) && !defined(TFT_PARALLEL_16_BIT)
    using TFT_eSPI::getSPIinstance;
#endif
    uint32_t getTextColor() const;
    uint32_t getTextBgColor() const;
    uint8_t getTextSize() const;
    uint8_t getRotation();
    TFT_eSPI *native();
};

class tft_sprite : private TFT_eSprite {
public:
    explicit tft_sprite(tft_display *parent);
    ~tft_sprite() = default;

    using TFT_eSprite::drawCircle;
    using TFT_eSprite::drawLine;
    using TFT_eSprite::drawPixel;
    using TFT_eSprite::drawRect;
    using TFT_eSprite::drawRoundRect;
    using TFT_eSprite::drawString;
    using TFT_eSprite::drawXBitmap;
    using TFT_eSprite::fillCircle;
    using TFT_eSprite::fillRect;
    using TFT_eSprite::fillRectHGradient;
    using TFT_eSprite::fillRectVGradient;
    using TFT_eSprite::fillRoundRect;
    using TFT_eSprite::height;
    using TFT_eSprite::pushImage;
    using TFT_eSprite::setCursor;
    using TFT_eSprite::setTextColor;
    using TFT_eSprite::setTextDatum;
    using TFT_eSprite::setTextSize;
    using TFT_eSprite::width;

    void *createSprite(int16_t w, int16_t h, uint8_t frames = 1);
    void deleteSprite();
    void setColorDepth(uint8_t depth);

    void fillScreen(uint32_t color);
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color);
    void fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color);
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color);

    void pushSprite(int32_t x, int32_t y, uint32_t transparent = TFT_TRANSPARENT);
    void pushToSprite(tft_sprite *dest, int32_t x, int32_t y, uint32_t transparent = TFT_TRANSPARENT);
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8, uint16_t *cmap);
    void
    pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8, uint16_t *cmap);

    TFT_eSprite *nativeSprite();
};
#endif
#endif // LIB_HAL_TFTESPI_H
