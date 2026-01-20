#include "tft.h"

#if defined(USE_M5GFX)

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
static SemaphoreHandle_t tftMutex;
#define RUN_ON_MUTEX(fn)                                                                                     \
    xSemaphoreTakeRecursive(tftMutex, portMAX_DELAY);                                                        \
    fn;                                                                                                      \
    xSemaphoreGiveRecursive(tftMutex);

tft_display::tft_display(int16_t _W, int16_t _H) : _height(_H), _width(_W) {}

void tft_display::begin(uint32_t speed) { (void)speed; }

void tft_display::init(uint8_t tc) {
    (void)tc;
    M5.begin();
}

void tft_display::setRotation(uint8_t r) {
    M5.Display.setRotation(r);
    _rotation = r;
}

void tft_display::drawPixel(int32_t x, int32_t y, uint32_t color) {
    RUN_ON_MUTEX(M5.Display.drawPixel(x, y, (uint16_t)color));
}

void tft_display::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
    RUN_ON_MUTEX(M5.Display.drawLine(x0, y0, x1, y1, (uint16_t)color));
}

void tft_display::drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) {
    RUN_ON_MUTEX(M5.Display.drawFastHLine(x, y, w, (uint16_t)color));
}

void tft_display::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) {
    RUN_ON_MUTEX(M5.Display.drawFastVLine(x, y, h, (uint16_t)color));
}

void tft_display::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    RUN_ON_MUTEX(M5.Display.drawRect(x, y, w, h, (uint16_t)color));
}

void tft_display::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    RUN_ON_MUTEX(M5.Display.fillRect(x, y, w, h, (uint16_t)color));
}

void tft_display::fillRectHGradient(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2
) {
    (void)color2;
    fillRect(x, y, w, h, (uint16_t)color1);
}

void tft_display::fillRectVGradient(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2
) {
    (void)color2;
    fillRect(x, y, w, h, (uint16_t)color1);
}

void tft_display::fillScreen(uint32_t color) { RUN_ON_MUTEX(M5.Display.fillScreen((uint16_t)color)); }

void tft_display::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
    RUN_ON_MUTEX(M5.Display.drawRoundRect(x, y, w, h, r, (uint16_t)color));
}

void tft_display::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
    RUN_ON_MUTEX(M5.Display.fillRoundRect(x, y, w, h, r, (uint16_t)color));
}

void tft_display::drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
    RUN_ON_MUTEX(M5.Display.drawCircle(x0, y0, r, (uint16_t)color));
}

void tft_display::fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
    RUN_ON_MUTEX(M5.Display.fillCircle(x0, y0, r, (uint16_t)color));
}

void tft_display::drawTriangle(
    int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color
) {
    RUN_ON_MUTEX(M5.Display.drawTriangle(x0, y0, x1, y1, x2, y2, (uint16_t)color));
}

void tft_display::fillTriangle(
    int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color
) {
    RUN_ON_MUTEX(M5.Display.fillTriangle(x0, y0, x1, y1, x2, y2, (uint16_t)color));
}

void tft_display::drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
    RUN_ON_MUTEX(M5.Display.drawEllipse(x0, y0, rx, ry, (uint16_t)color));
}

void tft_display::fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color) {
    RUN_ON_MUTEX(M5.Display.fillEllipse(x0, y0, rx, ry, (uint16_t)color));
}

void tft_display::drawArc(
    int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle, uint32_t fg_color,
    uint32_t bg_color, bool smoothArc
) {
    (void)bg_color;
    (void)smoothArc;
    RUN_ON_MUTEX(M5.Display.fillArc(x, y, r, ir, startAngle + 90, endAngle + 90, (uint16_t)fg_color));
}

void tft_display::drawWideLine(
    float ax, float ay, float bx, float by, float wd, uint32_t fg_color, uint32_t bg_color
) {
    (void)bg_color;
    RUN_ON_MUTEX(M5.Display.drawWideLine(ax, ay, bx, by, wd, (uint16_t)fg_color));
}

void tft_display::drawXBitmap(
    int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color
) {
    if (!bitmap) return;
    RUN_ON_MUTEX(M5.Display.drawXBitmap(x, y, bitmap, w, h, (uint16_t)color));
}

void tft_display::drawXBitmap(
    int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg
) {
    if (!bitmap) return;
    RUN_ON_MUTEX(M5.Display.drawXBitmap(x, y, bitmap, w, h, (uint16_t)color, (uint16_t)bg));
}

void tft_display::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data) {
    RUN_ON_MUTEX(M5.Display.pushImage(x, y, w, h, data));
}

void tft_display::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data) {
    RUN_ON_MUTEX(M5.Display.pushImage(x, y, w, h, data));
}

void tft_display::pushImage(
    int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8, uint16_t *cmap
) {
    if (!data || !bpp8 || !cmap) return;
    for (int32_t row = 0; row < h; ++row) {
        for (int32_t col = 0; col < w; ++col) {
            uint8_t idx = data[row * w + col];
            uint16_t color = cmap[idx];
            if (_swapBytes) color = static_cast<uint16_t>((color >> 8) | (color << 8));
            RUN_ON_MUTEX(M5.Display.drawPixel(x + col, y + row, (uint16_t)color));
        }
    }
}

void tft_display::pushImage(
    int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8, uint16_t *cmap
) {
    pushImage(x, y, w, h, const_cast<uint8_t *>(data), bpp8, cmap);
}

void tft_display::invertDisplay(bool i) {
    (void)i;
    M5.Display.invertDisplay(i);
}

void tft_display::sleep(bool value) {
    if (value) {
        writecommand(0x10);
        delay(5);
    } else {
        writecommand(0x11);
        delay(120);
    }
}

void tft_display::setSwapBytes(bool swap) { _swapBytes = swap; }

bool tft_display::getSwapBytes() const { return _swapBytes; }

uint16_t tft_display::color565(uint8_t r, uint8_t g, uint8_t b) const {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

int16_t tft_display::textWidth(const String &s, uint8_t font) const {
    (void)font;
    return static_cast<int16_t>(s.length() * 6 * _textSize);
}

int16_t tft_display::textWidth(const char *s, uint8_t font) const {
    (void)font;
    return static_cast<int16_t>(strlen(s) * 6 * _textSize);
}

void tft_display::setCursor(int16_t x, int16_t y) { M5.Display.setCursor(x, y); }

int16_t tft_display::getCursorX() const { return M5.Display.getCursorX(); }

int16_t tft_display::getCursorY() const { return M5.Display.getCursorY(); }

void tft_display::setTextSize(uint8_t s) {
    _textSize = s ? s : 1;
    M5.Display.setTextSize(_textSize);
}

void tft_display::setTextColor(uint16_t c) {
    _textColor = c;
    M5.Display.setTextColor((uint16_t)c);
}

void tft_display::setTextColor(uint16_t c, uint16_t b, bool bgfill) {
    (void)bgfill;
    _textColor = c;
    _textBgColor = b;
    M5.Display.setTextColor((uint16_t)c, (uint16_t)b);
}

void tft_display::setTextDatum(uint8_t d) { _textDatum = d; }

uint8_t tft_display::getTextDatum() const { return _textDatum; }

void tft_display::setTextFont(uint8_t f) { _textFont = f; }

void tft_display::setTextWrap(bool wrapX, bool wrapY) {
    (void)wrapY;
    M5.Display.setTextWrap(wrapX);
}

int16_t tft_display::drawString(const String &string, int32_t x, int32_t y, uint8_t font) {
    (void)font;
    return drawAlignedString(string, x, y, _textDatum);
}

int16_t tft_display::drawCentreString(const String &string, int32_t x, int32_t y, uint8_t font) {
    (void)font;
    return drawAlignedString(string, x, y, TC_DATUM);
}

int16_t tft_display::drawRightString(const String &string, int32_t x, int32_t y, uint8_t font) {
    (void)font;
    return drawAlignedString(string, x, y, TR_DATUM);
}

size_t tft_display::write(uint8_t c) {
    size_t t = 0;
    RUN_ON_MUTEX(t = M5.Display.write(c));
    return t;
}

size_t tft_display::write(const uint8_t *buffer, size_t size) {
    size_t t = 0;
    RUN_ON_MUTEX(t = M5.Display.write(buffer, size));
    return t;
}

size_t tft_display::println() {
    size_t t = 0;
    RUN_ON_MUTEX(t = M5.Display.println());
    return t;
}

size_t tft_display::printf(const char *fmt, ...) {
    if (!fmt) return 0;
    va_list args;
    size_t t = 0;
    va_start(args, fmt);
    char stackBuf[128];
    int len = vsnprintf(stackBuf, sizeof(stackBuf), fmt, args);
    va_end(args);
    if (len < 0) return 0;
    if (static_cast<size_t>(len) < sizeof(stackBuf)) {
        RUN_ON_MUTEX(t = M5.Display.print(stackBuf));
        return t;
    }
    std::unique_ptr<char[]> buf(new char[len + 1]);
    va_start(args, fmt);
    vsnprintf(buf.get(), len + 1, fmt, args);
    va_end(args);
    RUN_ON_MUTEX(t = M5.Display.print(buf.get()));
    return t;
}

int16_t tft_display::width() const { return M5.Display.width(); }

int16_t tft_display::height() const { return M5.Display.height(); }

SPIClass &tft_display::getSPIinstance() const { return SPI; }

void tft_display::writecommand(uint8_t c) { (void)c; }

uint32_t tft_display::getTextColor() const { return _textColor; }

uint32_t tft_display::getTextBgColor() const { return _textBgColor; }

uint8_t tft_display::getTextSize() const { return _textSize; }

uint8_t tft_display::getRotation() const { return _rotation; }

int16_t tft_display::fontHeight(int16_t font) const {
    (void)font;
    return static_cast<int16_t>(_textSize * 8);
}

lgfx::LGFX_Device *tft_display::native() { return nullptr; }

int16_t tft_display::drawAlignedString(const String &s, int32_t x, int32_t y, uint8_t datum) {
    int16_t w = static_cast<int16_t>(s.length() * 6 * _textSize);
    int16_t h = static_cast<int16_t>(8 * _textSize);
    int32_t cx = x;
    int32_t cy = y;
    switch (datum) {
        case TC_DATUM: cx -= w / 2; break;
        case TR_DATUM: cx -= w; break;
        case MC_DATUM:
            cx -= w / 2;
            cy -= h / 2;
            break;
        case MR_DATUM:
            cx -= w;
            cy -= h / 2;
            break;
        case BC_DATUM:
            cx -= w / 2;
            cy -= h;
            break;
        case BR_DATUM:
            cx -= w;
            cy -= h;
            break;
        case BL_DATUM: cy -= h; break;
        default: break;
    }
    M5.Display.setCursor(cx, cy);
    RUN_ON_MUTEX(M5.Display.print(s));
    return w;
}

tft_sprite::tft_sprite(tft_display *parent) : lgfx::LGFX_Sprite(parent ? &M5.Display : nullptr) {}

void *tft_sprite::createSprite(int16_t w, int16_t h, uint8_t frames) {
    (void)frames;
    return lgfx::LGFX_Sprite::createSprite(w, h);
}

void tft_sprite::deleteSprite() { lgfx::LGFX_Sprite::deleteSprite(); }

void tft_sprite::setColorDepth(uint8_t depth) { lgfx::LGFX_Sprite::setColorDepth(depth); }

void tft_sprite::fillScreen(uint32_t color) { lgfx::LGFX_Sprite::fillScreen((uint16_t)color); }

void tft_sprite::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    lgfx::LGFX_Sprite::fillRect(x, y, w, h, (uint16_t)color);
}

void tft_sprite::fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color) {
    lgfx::LGFX_Sprite::fillCircle(x, y, r, (uint16_t)color);
}

void tft_sprite::fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color) {
    lgfx::LGFX_Sprite::fillEllipse(x, y, rx, ry, (uint16_t)color);
}

void tft_sprite::fillTriangle(
    int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color
) {
    lgfx::LGFX_Sprite::fillTriangle(x0, y0, x1, y1, x2, y2, (uint16_t)color);
}

void tft_sprite::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) {
    lgfx::LGFX_Sprite::drawFastVLine(x, y, h, (uint16_t)color);
}

void tft_sprite::pushSprite(int32_t x, int32_t y, uint32_t transparent) {
    RUN_ON_MUTEX(lgfx::LGFX_Sprite::pushSprite(x, y, (uint16_t)transparent));
}

void tft_sprite::pushToSprite(tft_sprite *dest, int32_t x, int32_t y, uint32_t transparent) {
    lgfx::LGFX_Sprite::pushSprite(static_cast<lgfx::LGFX_Sprite *>(dest), x, y, (uint16_t)transparent);
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

void tft_sprite::fillRectHGradient(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2
) {
    (void)color2;
    lgfx::LGFX_Sprite::fillRect(x, y, w, h, (uint16_t)color1);
}

void tft_sprite::fillRectVGradient(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2
) {
    lgfx::LGFX_Sprite::fillRect(x, y, w, h, (uint16_t)color1);
}

int16_t tft_sprite::width() const { return static_cast<int16_t>(lgfx::LGFX_Sprite::width()); }

int16_t tft_sprite::height() const { return static_cast<int16_t>(lgfx::LGFX_Sprite::height()); }

#endif
