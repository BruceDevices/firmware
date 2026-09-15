#include "tft.h"

#if defined(USE_TFT_ESPI)
// This library uses Mutex locally! don'r need to rewrite the whole thing!!

tft_display::tft_display(int16_t _W, int16_t _H) : TFT_eSPI(_W, _H) {}

uint32_t tft_display::getTextColor() const { return TFT_eSPI::textcolor; }

uint32_t tft_display::getTextBgColor() const { return TFT_eSPI::textbgcolor; }

uint8_t tft_display::getTextSize() const { return TFT_eSPI::textsize; }

uint8_t tft_display::getRotation() { return TFT_eSPI::getRotation(); }

TFT_eSPI *tft_display::native() { return static_cast<TFT_eSPI *>(this); }

tft_sprite::tft_sprite(tft_display *parent) : TFT_eSprite(static_cast<TFT_eSPI *>(parent)) {}

void *tft_sprite::createSprite(int16_t w, int16_t h, uint8_t frames) {
    return TFT_eSprite::createSprite(w, h, frames);
}

void tft_sprite::deleteSprite() { TFT_eSprite::deleteSprite(); }

void tft_sprite::setColorDepth(uint8_t depth) { TFT_eSprite::setColorDepth(depth); }

void tft_sprite::fillScreen(uint32_t color) { TFT_eSprite::fillSprite(color); }

void tft_sprite::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    TFT_eSprite::fillRect(x, y, w, h, color);
}

void tft_sprite::fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color) {
    TFT_eSprite::fillCircle(x, y, r, color);
}

void tft_sprite::fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color) {
    TFT_eSprite::fillEllipse(x, y, rx, ry, color);
}

void tft_sprite::fillTriangle(
    int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color
) {
    TFT_eSprite::fillTriangle(x0, y0, x1, y1, x2, y2, color);
}

void tft_sprite::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) {
    TFT_eSprite::drawFastVLine(x, y, h, color);
}

void tft_sprite::pushSprite(int32_t x, int32_t y, uint32_t transparent) {
    TFT_eSprite::pushSprite(x, y, transparent);
}

void tft_sprite::pushToSprite(tft_sprite *dest, int32_t x, int32_t y, uint32_t transparent) {
    TFT_eSprite::pushToSprite(static_cast<TFT_eSprite *>(dest), x, y, transparent);
}

void tft_sprite::pushImage(
    int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8, uint16_t *cmap
) {
    if (!data || !bpp8 || !cmap) return;
    for (int32_t row = 0; row < h; ++row) {
        for (int32_t col = 0; col < w; ++col) { drawPixel(x + col, y + row, cmap[data[row * w + col]]); }
    }
}

void tft_sprite::pushImage(
    int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8, uint16_t *cmap
) {
    pushImage(x, y, w, h, const_cast<uint8_t *>(data), bpp8, cmap);
}

TFT_eSprite *tft_sprite::nativeSprite() { return static_cast<TFT_eSprite *>(this); }

#endif
