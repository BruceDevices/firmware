#include "tft.h"

#if defined(USE_TFT_ESPI)
// This library uses Mutex locally! don'r need to rewrite the whole thing!!

tft_display::tft_display(int16_t _W, int16_t _H) : TFT_eSPI(_W, _H) {}

uint32_t tft_display::getTextColor() const { return TFT_eSPI::textcolor; }

uint32_t tft_display::getTextBgColor() const { return TFT_eSPI::textbgcolor; }

uint8_t tft_display::getTextSize() const { return TFT_eSPI::textsize; }

uint8_t tft_display::getRotation() { return TFT_eSPI::getRotation(); }

TFT_eSPI *tft_display::native() { return static_cast<TFT_eSPI *>(this); }

tft_sprite::tft_sprite(tft_display *parent)
    : TFT_eSprite(static_cast<TFT_eSPI *>(parent)), _dmaParent(parent) {}

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
#if defined(ACCRETION_TFT_DMA) && defined(ESP32_DMA)
    // Fast path: send the whole sprite to the panel over SPI-DMA instead of the
    // default register-banged loop (16 pixels per CPU-driven transaction).
    // This is what actually removes the "wipe"/flicker on screen transitions:
    // the DMA engine streams the frame to the ILI9341 as one continuous burst
    // that FreeRTOS task switches / interrupts can no longer split into a
    // half-drawn frame the eye can see.
    //
    // Only handles the common case this firmware actually uses for full-screen
    // UI sprites: 16bpp, opaque (no transparent colour key, since the DMA path
    // sends the raw buffer window as-is and can't skip pixels). Anything else
    // (8bpp/4bpp palette sprites, or a transparent-colour push) falls back to
    // the normal blocking path below, unchanged.
    //
    // We deliberately wait for the transfer to finish (dmaWait()) before
    // returning, instead of leaving it running in the background. A true
    // non-blocking double buffer would need a second full-screen copy of
    // every persistent canvas in this firmware (Home Screen, Doom, the NES
    // core, MJPEG...), and this board's internal RAM is already tight (see
    // the JPEGDEC/PSRAM notes elsewhere in this codebase) - duplicating those
    // buffers risks trading flicker for crashes. Waiting still gives the
    // benefit: one fast uninterruptible DMA burst per frame instead of a
    // slow, preemptible, hand-rolled SPI loop.
    if (_dmaParent && _bpp == 16 && transparent == TFT_TRANSPARENT) {
        TFT_eSPI *native = _dmaParent->native();
        if (native && native->DMA_Enabled) {
            bool oldSwapBytes = native->getSwapBytes();
            native->setSwapBytes(false);
            native->startWrite();
            native->pushImageDMA(x, y, _dwidth, _dheight, _img); // buffer=nullptr: no extra RAM used
            native->dmaWait();
            native->endWrite();
            native->setSwapBytes(oldSwapBytes);
            return;
        }
    }
#endif
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
