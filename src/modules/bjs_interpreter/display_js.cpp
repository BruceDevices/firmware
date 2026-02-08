#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "display_js.h"

#include "core/settings.h"
#include "helpers_js.h"
#include "stdio.h"
#include "user_classes_js.h"

JSValue native_color(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int r = 0, g = 0, b = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &r, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &g, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &b, argv[2]);
    int color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    int mode = 16;
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &mode, argv[3]);
    if (mode == 16) {
        return JS_NewInt32(ctx, color);
    } else {
        return JS_NewInt32(ctx, ((color & 0xE000) >> 8) | ((color & 0x0700) >> 6) | ((color & 0x0018) >> 3));
    }
}

#if defined(HAS_SCREEN)
typedef struct {
    tft_sprite *sprite;
} SpriteData;
#endif

/* Finalizer called by mquickjs when a Sprite JS object is freed. */
void native_sprite_finalizer(JSContext *ctx, void *opaque) {
#if defined(HAS_SCREEN)
    SpriteData *d = (SpriteData *)opaque;
    if (!d) return;
    if (d->sprite) {
        delete d->sprite;
        d->sprite = NULL;
    }
    free(d);
#endif
}

#if defined(HAS_SCREEN)
struct DisplayTarget {
    tft_display *display;
    tft_sprite *sprite;
    bool isSprite;
};

static DisplayTarget get_display_target(JSContext *ctx, JSValue *this_val) {
    DisplayTarget target{static_cast<tft_display *>(&tft), nullptr, false};
    if (this_val && JS_IsObject(ctx, *this_val)) {
        int cid = JS_GetClassID(ctx, *this_val);
        if (cid == JS_CLASS_SPRITE) {
            void *opaque = JS_GetOpaque(ctx, *this_val);
            if (opaque) {
                SpriteData *d = (SpriteData *)opaque;
                if (d->sprite) {
                    target.sprite = d->sprite;
                    target.isSprite = true;
                }
            }
        }
    }
    return target;
}
#else
static SerialDisplayClass *get_display(JSContext *ctx, JSValue *this_val) {
    return static_cast<SerialDisplayClass *>(&tft);
}
#endif

JSValue native_setTextColor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int c = 0;
    int bg = -1;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &c, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &bg, argv[1]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) {
        if (bg >= 0) target.sprite->setTextColor(c, bg);
        else target.sprite->setTextColor(c);
    } else {
        if (bg >= 0) target.display->setTextColor(c, bg);
        else target.display->setTextColor(c);
    }
#else
    if (bg >= 0) get_display(ctx, this_val)->setTextColor(c, bg);
    else get_display(ctx, this_val)->setTextColor(c);
#endif
    return JS_UNDEFINED;
}

JSValue native_setTextSize(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int s = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &s, argv[0]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->setTextSize(s);
    else target.display->setTextSize(s);
#else
    get_display(ctx, this_val)->setTextSize(s);
#endif
    return JS_UNDEFINED;
}

JSValue native_setTextAlign(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    uint8_t align = 0;
    uint8_t baseline = 0;
    if (argc > 0) {
        if (JS_IsString(ctx, argv[0])) {
            JSCStringBuf sb;
            const char *s = JS_ToCString(ctx, argv[0], &sb);
            if (s) {
                if (s[0] == 'l') align = 0;
                else if (s[0] == 'c') align = 1;
                else if (s[0] == 'r') align = 2;
            }
        } else if (JS_IsNumber(ctx, argv[0])) {
            JS_ToInt32(ctx, (int *)&align, argv[0]);
        }
    }
    if (argc > 1) {
        if (JS_IsString(ctx, argv[1])) {
            JSCStringBuf sb;
            const char *s = JS_ToCString(ctx, argv[1], &sb);
            if (s) {
                if (s[0] == 't') baseline = 0;
                else if (s[0] == 'm') baseline = 1;
                else if (s[0] == 'b') baseline = 2;
                else if (s[0] == 'a') baseline = 3;
            }
        } else if (JS_IsNumber(ctx, argv[1])) {
            JS_ToInt32(ctx, (int *)&baseline, argv[1]);
        }
    }
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->setTextDatum(align + baseline * 3);
    else target.display->setTextDatum(align + baseline * 3);
#else
    get_display(ctx, this_val)->setTextDatum(align + baseline * 3);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, w = 0, h = 0, color = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &w, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &h, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &color, argv[4]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->drawRect(x, y, w, h, color);
    else target.display->drawRect(x, y, w, h, color);
#else
    get_display(ctx, this_val)->drawRect(x, y, w, h, color);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawFillRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, w = 0, h = 0, color = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &w, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &h, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &color, argv[4]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->fillRect(x, y, w, h, color);
    else target.display->fillRect(x, y, w, h, color);
#else
    get_display(ctx, this_val)->fillRect(x, y, w, h, color);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawFillRectGradient(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, w = 0, h = 0, c1 = 0, c2 = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &w, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &h, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &c1, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &c2, argv[5]);

#if defined(HAS_SCREEN)
    char mode = 'h';
    if (argc > 6 && JS_IsString(ctx, argv[6])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[6], &sb);
        if (s) mode = s[0];
    }
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) {
        if (mode == 'h') target.sprite->fillRectHGradient(x, y, w, h, c1, c2);
        else target.sprite->fillRectVGradient(x, y, w, h, c1, c2);
    } else {
        if (mode == 'h') target.display->fillRectHGradient(x, y, w, h, c1, c2);
        else target.display->fillRectVGradient(x, y, w, h, c1, c2);
    }
#else
    get_display(ctx, this_val)->fillRect(x, y, w, h, c1);
#endif

    return JS_UNDEFINED;
}

JSValue native_drawRoundRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, w = 0, h = 0, r = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &w, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &h, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &r, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &c, argv[5]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->drawRoundRect(x, y, w, h, r, c);
    else target.display->drawRoundRect(x, y, w, h, r, c);
#else
    get_display(ctx, this_val)->drawRoundRect(x, y, w, h, r, c);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawFillRoundRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, w = 0, h = 0, r = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &w, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &h, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &r, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &c, argv[5]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->fillRoundRect(x, y, w, h, r, c);
    else target.display->fillRoundRect(x, y, w, h, r, c);
#else
    get_display(ctx, this_val)->fillRoundRect(x, y, w, h, r, c);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawTriangle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x0, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y0, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &x1, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &y1, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &x2, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &y2, argv[5]);
    if (argc > 6 && JS_IsNumber(ctx, argv[6])) JS_ToInt32(ctx, &c, argv[6]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    target.display->drawTriangle(x0, y0, x1, y1, x2, y2, c);
#else
    get_display(ctx, this_val)->drawTriangle(x0, y0, x1, y1, x2, y2, c);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawFillTriangle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x0, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y0, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &x1, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &y1, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &x2, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &y2, argv[5]);
    if (argc > 6 && JS_IsNumber(ctx, argv[6])) JS_ToInt32(ctx, &c, argv[6]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    target.display->fillTriangle(x0, y0, x1, y1, x2, y2, c);
#else
    get_display(ctx, this_val)->fillTriangle(x0, y0, x1, y1, x2, y2, c);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawCircle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, r = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &r, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &c, argv[3]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->drawCircle(x, y, r, c);
    else target.display->drawCircle(x, y, r, c);
#else
    get_display(ctx, this_val)->drawCircle(x, y, r, c);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawFillCircle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, r = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &r, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &c, argv[3]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->fillCircle(x, y, r, c);
    else target.display->fillCircle(x, y, r, c);
#else
    get_display(ctx, this_val)->fillCircle(x, y, r, c);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawArc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, r = 0, ir = 0, startAngle = 0, endAngle = 0, fg_color = 0, bg_color = 0;
    bool smooth = false;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &r, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &ir, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &startAngle, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &endAngle, argv[5]);
    if (argc > 6 && JS_IsNumber(ctx, argv[6])) JS_ToInt32(ctx, &fg_color, argv[6]);
    if (argc > 7 && JS_IsNumber(ctx, argv[7])) JS_ToInt32(ctx, &bg_color, argv[7]);
    if (argc > 8 && JS_IsBool(argv[8])) smooth = JS_ToBool(ctx, argv[8]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    target.display->drawArc(x, y, r, ir, startAngle, endAngle, fg_color, bg_color, smooth);
#else
    get_display(ctx, this_val)->drawArc(x, y, r, ir, startAngle, endAngle, fg_color, bg_color, smooth);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawWideLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0, width = 0, color = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x0, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y0, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &x1, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &y1, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &width, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &color, argv[5]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    target.display->drawWideLine(x0, y0, x1, y1, width, color);
#else
    get_display(ctx, this_val)->drawWideLine(x0, y0, x1, y1, width, color);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int xs = 0, ys = 0, xe = 0, ye = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &xs, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &ys, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &xe, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &ye, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &c, argv[4]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->drawLine(xs, ys, xe, ye, c);
    else target.display->drawLine(xs, ys, xe, ye, c);
#else
    get_display(ctx, this_val)->drawLine(xs, ys, xe, ye, c);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawFastVLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, h = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &h, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &c, argv[3]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    target.display->drawFastVLine(x, y, h, c);
#else
    get_display(ctx, this_val)->drawFastVLine(x, y, h, c);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawFastHLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, w = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &w, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &c, argv[3]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    target.display->drawFastHLine(x, y, w, c);
#else
    get_display(ctx, this_val)->drawFastHLine(x, y, w, c);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawPixel(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->drawPixel(x, y, c);
    else target.display->drawPixel(x, y, c);
#else
    get_display(ctx, this_val)->drawPixel(x, y, c);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawBitmap(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int bitmapWidth = 0, bitmapHeight = 0, bpp = 8;
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &bitmapWidth, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &bitmapHeight, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &bpp, argv[5]);

    size_t bitmapSize = 0;
    const uint8_t *bitmapPointer = NULL;

    if (argc > 2) {
        const char *s = NULL;
        if (JS_IsString(ctx, argv[2])) {
            JSCStringBuf sb;
            s = JS_ToCStringLen(ctx, &bitmapSize, argv[2], &sb);
        } else if (JS_IsTypedArray(ctx, argv[2])) {
            s = JS_GetTypedArrayBuffer(ctx, &bitmapSize, argv[2]);
        }
        if (s) { bitmapPointer = (const uint8_t *)s; }
    }

    if (bitmapPointer == NULL) {
        return JS_ThrowTypeError(
            ctx, "%s: Expected string/ArrayBuffer/Uint8Array for bitmap data", "drawBitmap"
        );
    }

    // Calculate expected bitmap size
    size_t expectedSize;
    bool bpp8 = false;
    if (bpp == 16) {
        expectedSize = bitmapWidth * bitmapHeight * 2; // 16bpp (RGB565)
    } else if (bpp == 8) {
        expectedSize = bitmapWidth * bitmapHeight; // 8bpp (RGB332)
        bpp8 = true;
    } else if (bpp == 4) {
        expectedSize = (bitmapWidth * bitmapHeight + 1) / 2; // 4bpp (2 pixels per byte)
    } else if (bpp == 1) {
        expectedSize = ((bitmapWidth + 7) / 8) * bitmapHeight; // 1bpp (8 pixels per byte)
    } else {
        return JS_ThrowTypeError(ctx, "%s: Unsupported bpp value! Use 16, 8, 4, or 1.", "drawBitmap");
    }

    if (bitmapSize != expectedSize) {
        return JS_ThrowTypeError(
            ctx,
            "%s: Bitmap size mismatch! Got %lu bytes, expected %lu bytes for %dx%d at %dbpp.",
            "drawBitmap",
            (unsigned long)bitmapSize,
            (unsigned long)expectedSize,
            bitmapWidth,
            bitmapHeight,
            bpp
        );
    }

    int x = 0, y = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);

    // Handle palette if needed (only for 4bpp and 1bpp)
    uint16_t *palette = nullptr;
    size_t paletteSize = 0;
    if ((bpp == 4 || bpp == 1) && JS_IsTypedArray(ctx, argv[6])) {
        palette = (uint16_t *)JS_GetTypedArrayBuffer(ctx, &paletteSize, argv[6]);
        if (!palette || paletteSize == 0) {
            return JS_ThrowTypeError(ctx, "%s: Invalid palette! Expected a valid ArrayBuffer.", "drawBitmap");
        }
    }

#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) {
        target.sprite->pushImage(x, y, bitmapWidth, bitmapHeight, bitmapPointer, bpp8, palette);
    } else {
        target.display->pushImage(x, y, bitmapWidth, bitmapHeight, bitmapPointer, bpp8, palette);
    }
#else
    get_display(ctx, this_val)->pushImage(x, y, bitmapWidth, bitmapHeight, bitmapPointer, bpp8, palette);
#endif

    return JS_UNDEFINED;
}

JSValue native_drawXBitmap(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // Accept strings, Uint8Array, ArrayBuffer or JS arrays containing byte values.
    int bitmapWidth = 0, bitmapHeight = 0;
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &bitmapWidth, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &bitmapHeight, argv[4]);

    size_t len = 0;
    const uint8_t *data = NULL;

    if (argc > 2) {
        const char *s = NULL;
        if (JS_IsString(ctx, argv[2])) {
            JSCStringBuf sb;
            s = JS_ToCStringLen(ctx, &len, argv[2], &sb);
        } else if (JS_IsTypedArray(ctx, argv[2])) {
            s = JS_GetTypedArrayBuffer(ctx, &len, argv[2]);
        }
        if (s) { data = (const uint8_t *)s; }
    }

    if (data == NULL) {
        return JS_ThrowTypeError(
            ctx, "%s: Expected string/ArrayBuffer/Uint8Array for bitmap data", "drawXBitmap"
        );
    }

    size_t expectedSize = ((bitmapWidth + 7) / 8) * bitmapHeight;
    if (len != expectedSize) {
        Serial.printf("data = %s\n", data);
        return JS_ThrowTypeError(ctx, "Bitmap size mismatch, len: %d, expected: %d", len, expectedSize);
    }

    int x = 0, y = 0, fg = 0, bg = -1;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &fg, argv[5]);
    if (argc > 6 && JS_IsNumber(ctx, argv[6])) JS_ToInt32(ctx, &bg, argv[6]);

#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) {
        if (bg >= 0) {
            target.sprite->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg, bg);
        } else {
            target.sprite->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg);
        }
    } else {
        if (bg >= 0) {
            target.display->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg, bg);
        } else {
            target.display->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg);
        }
    }
#else
    if (bg >= 0) {
        get_display(ctx, this_val)->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg, bg);
    } else {
        get_display(ctx, this_val)->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg);
    }
#endif

    return JS_UNDEFINED;
}

JSValue native_drawString(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc > 0 && (JS_IsString(ctx, argv[0]) || JS_IsNumber(ctx, argv[0]) || JS_IsBool(argv[0]) ||
                     JS_IsObject(ctx, argv[0]))) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[0], &sb);
        int x = 0, y = 0;
        if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &x, argv[1]);
        if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &y, argv[2]);
#if defined(HAS_SCREEN)
        DisplayTarget target = get_display_target(ctx, this_val);
        if (target.isSprite) target.sprite->drawString(s, x, y);
        else target.display->drawString(s, x, y);
#else
        get_display(ctx, this_val)->drawString(s, x, y);
#endif
    }
    return JS_UNDEFINED;
}

JSValue native_setCursor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite) target.sprite->setCursor(x, y);
    else target.display->setCursor(x, y);
#else
    get_display(ctx, this_val)->setCursor(x, y);
#endif
    return JS_UNDEFINED;
}

JSValue native_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    internal_print(ctx, this_val, argc, argv, true, false);
    return JS_UNDEFINED;
}

JSValue native_println(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    internal_print(ctx, this_val, argc, argv, true, true);
    return JS_UNDEFINED;
}

JSValue native_fillScreen(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN)
    int c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &c, argv[0]);
    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite && target.sprite) target.sprite->fillScreen(c);
    else target.display->fillScreen(c);
#endif
    return JS_UNDEFINED;
}

JSValue native_width(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    int width = target.isSprite ? target.sprite->width() : target.display->width();
#else
    int width = get_display(ctx, this_val)->width();
#endif
    return JS_NewInt32(ctx, width);
}

JSValue native_height(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN)
    DisplayTarget target = get_display_target(ctx, this_val);
    int height = target.isSprite ? target.sprite->height() : target.display->height();
#else
    int height = get_display(ctx, this_val)->height();
#endif
    return JS_NewInt32(ctx, height);
}

JSValue native_drawImage(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    FileParamsJS file = js_get_path_from_params(ctx, argv, true, true);
    drawImg(*file.fs, file.path, 0, 0, 0);
    return JS_UNDEFINED;
}

JSValue native_drawJpg(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    FileParamsJS file;
    int x = 0, y = 0, center = 0;
    int base = 0;

    if (!JS_IsTypedArray(ctx, argv[0])) {
        file = js_get_path_from_params(ctx, argv, true, true);
        base = file.paramOffset;
    }

    if (argc > base && JS_IsNumber(ctx, argv[base + 1])) JS_ToInt32(ctx, &x, argv[base + 1]);
    if (argc > base + 1 && JS_IsNumber(ctx, argv[base + 2])) JS_ToInt32(ctx, &y, argv[base + 2]);
    if (argc > base + 2) {
        if (JS_IsBool(argv[base + 3])) center = JS_ToBool(ctx, argv[base + 3]);
        else if (JS_IsNumber(ctx, argv[base + 3])) JS_ToInt32(ctx, &center, argv[base + 3]);
    }

    if (!JS_IsTypedArray(ctx, argv[0])) {
        showJpeg(*file.fs, file.path, x, y, center);
    } else {
        size_t jpgSize = 0;
        const uint8_t *jpgData = (const uint8_t *)JS_GetTypedArrayBuffer(ctx, &jpgSize, argv[0]);
        if (!jpgData) { return JS_ThrowTypeError(ctx, "drawJpg: Invalid Uint8Array data"); }
        showJpeg(jpgData, jpgSize, x, y, center);
    }
    return JS_UNDEFINED;
}

JSValue native_drawGif(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if !defined(LITE_VERSION)
    FileParamsJS file = js_get_path_from_params(ctx, argv, true, true);

    int x = 0, y = 0, center = 0, playDurationMs = 0;
    int base = file.paramOffset;
    if (argc > base && JS_IsNumber(ctx, argv[base])) JS_ToInt32(ctx, &x, argv[base]);
    if (argc > base + 1 && JS_IsNumber(ctx, argv[base + 1])) JS_ToInt32(ctx, &y, argv[base + 1]);
    if (argc > base + 2) {
        if (JS_IsBool(argv[base + 2])) center = JS_ToBool(ctx, argv[base + 2]);
        else if (JS_IsNumber(ctx, argv[base + 2])) JS_ToInt32(ctx, &center, argv[base + 2]);
    }
    if (argc > base + 3 && JS_IsNumber(ctx, argv[base + 3])) JS_ToInt32(ctx, &playDurationMs, argv[base + 3]);

    showGif(file.fs, file.path.c_str(), x, y, center != 0, playDurationMs);
#endif
    return JS_UNDEFINED;
}

#if !defined(LITE_VERSION)
typedef struct {
    Gif *gif;
} GifData;
#endif

void native_gif_finalizer(JSContext *ctx, void *opaque) {
#if !defined(LITE_VERSION)
    GifData *d = (GifData *)opaque;
    if (!d) return;
    if (d->gif) {
        delete d->gif;
        d->gif = NULL;
    }
    free(d);
#endif
}

JSValue native_gifPlayFrame(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    uint8_t result = 0;
#if !defined(LITE_VERSION)
    int x = 0, y = 0, bSync = 1;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &bSync, argv[2]);

    if (JS_GetClassID(ctx, *this_val) == JS_CLASS_GIF) {
        GifData *opaque = (GifData *)JS_GetOpaque(ctx, *this_val);
        if (!opaque) return JS_ThrowInternalError(ctx, "Invalid GIF object");
        Gif *gif = (Gif *)opaque->gif;
        if (gif) { result = gif->playFrame(x, y, bSync); }
    }
#endif
    return JS_NewInt32(ctx, result);
}

JSValue native_gifDimensions(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if !defined(LITE_VERSION)
    if (!this_val || !JS_IsObject(ctx, *this_val)) return JS_NewInt32(ctx, 0);
    GifData *opaque = (GifData *)JS_GetOpaque(ctx, *this_val);
    if (!opaque) return JS_ThrowInternalError(ctx, "Invalid GIF object");
    Gif *gif = (Gif *)opaque->gif;
    if (!gif) return JS_NewInt32(ctx, 0);
    int canvasWidth = gif->getCanvasWidth();
    int canvasHeight = gif->getCanvasHeight();
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "width", JS_NewInt32(ctx, canvasWidth));
    JS_SetPropertyStr(ctx, obj, "height", JS_NewInt32(ctx, canvasHeight));
    return obj;
#else
    return JS_NULL;
#endif
}

JSValue native_gifReset(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    uint8_t result = 0;
#if !defined(LITE_VERSION)
    if (this_val && JS_IsObject(ctx, *this_val)) {
        GifData *opaque = (GifData *)JS_GetOpaque(ctx, *this_val);
        if (!opaque) return JS_ThrowInternalError(ctx, "Invalid GIF object");
        Gif *gif = (Gif *)opaque->gif;
        if (gif) {
            gif->reset();
            result = 1;
        }
    }
#endif
    return JS_NewInt32(ctx, result);
}

JSValue native_gifClose(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if !defined(LITE_VERSION)
    JSValue obj = JS_UNDEFINED;
    if (argc > 0 && JS_IsObject(ctx, argv[0])) obj = argv[0];
    else if (this_val && JS_IsObject(ctx, *this_val)) obj = *this_val;
    if (JS_IsUndefined(obj)) return JS_NewInt32(ctx, 0);

    int cid = JS_GetClassID(ctx, obj);
    if (cid == JS_CLASS_GIF) {
        void *opaque = JS_GetOpaque(ctx, obj);
        if (!opaque) return JS_NewInt32(ctx, 0);
        native_gif_finalizer(ctx, opaque);
        JS_SetOpaque(ctx, obj, NULL);
        return JS_NewInt32(ctx, 1);
    }
#endif
    return JS_NewInt32(ctx, 0);
}

JSValue native_gifOpen(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if !defined(LITE_VERSION)
    FileParamsJS file = js_get_path_from_params(ctx, argv, true, true);
    Gif *gif = new Gif();
    bool success = gif->openGIF(file.fs, file.path.c_str());
    if (success) {
        JSValue obj = JS_NewObjectClassUser(ctx, JS_CLASS_GIF);
        if (JS_IsException(obj)) {
            delete gif;
            return obj;
        }
        GifData *d = (GifData *)malloc(sizeof(GifData));
        if (!d) {
            delete gif;
            return JS_ThrowOutOfMemory(ctx);
        }
        d->gif = gif;
        JS_SetOpaque(ctx, obj, d);

        return obj;
    }
#endif
    return JS_ThrowTypeError(ctx, "Failed to open GIF file");
}

JSValue native_deleteSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSValue obj = JS_UNDEFINED;
    if (argc > 0 && JS_IsObject(ctx, argv[0])) obj = argv[0];
    else if (this_val && JS_IsObject(ctx, *this_val)) obj = *this_val;
    if (JS_IsUndefined(obj)) return JS_NewInt32(ctx, 0);

    int cid = JS_GetClassID(ctx, obj);
    if (cid == JS_CLASS_SPRITE) {
        void *opaque = JS_GetOpaque(ctx, obj);
        if (!opaque) return JS_NewInt32(ctx, 0);
        native_sprite_finalizer(ctx, opaque);
        JS_SetOpaque(ctx, obj, NULL);
        return JS_NewInt32(ctx, 1);
    }

    return JS_NewInt32(ctx, 0);
}

JSValue native_pushSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN) && defined(BOARD_HAS_PSRAM)
    int x = 0, y = 0, transparent = TFT_TRANSPARENT;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &transparent, argv[2]);

    DisplayTarget target = get_display_target(ctx, this_val);
    if (target.isSprite && target.sprite) target.sprite->pushSprite(x, y, transparent);
#endif
    return JS_UNDEFINED;
}

JSValue native_createSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN)
    int16_t width = tft.width();
    int16_t height = tft.height();
    uint8_t colorDepth = 16;
    uint8_t frames = 1;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, (int *)&width, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, (int *)&height, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, (int *)&colorDepth, argv[2]);

    JSValue obj = JS_NewObjectClassUser(ctx, JS_CLASS_SPRITE);
    if (JS_IsException(obj)) return obj;

    SpriteData *d = (SpriteData *)malloc(sizeof(SpriteData));
    if (!d) return JS_ThrowOutOfMemory(ctx);
    d->sprite = new tft_sprite(static_cast<tft_display *>(&tft));
    if (!d->sprite) {
        free(d);
        return JS_ThrowOutOfMemory(ctx);
    }
    d->sprite->setColorDepth(colorDepth);
    d->sprite->createSprite(width, height, frames);

    // set opaque pointer so finalizer can free C resources
    JS_SetOpaque(ctx, obj, d);

    return obj;
#else
    return JS_NewObject(ctx);
#endif
}

JSValue native_getRotation(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewInt32(ctx, bruceConfigPins.rotation);
}

JSValue native_getBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewInt32(ctx, currentScreenBrightness);
}

JSValue native_setBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int brightness = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &brightness, argv[0]);
    bool save = false;
    if (argc > 1 && JS_IsObject(ctx, argv[1])) {
        JSValue v = JS_GetPropertyStr(ctx, argv[1], "save");
        if (!JS_IsUndefined(v)) save = JS_ToBool(ctx, v);
    }
    setBrightness(brightness, save);
    return JS_NewInt32(ctx, 1);
}

JSValue native_restoreBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    setBrightness(bruceConfig.bright, false);
    return JS_UNDEFINED;
}
#endif
