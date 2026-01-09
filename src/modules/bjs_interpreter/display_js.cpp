#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "display_js.h"

#include "core/settings.h"
#include "helpers_js.h"
#include "stdio.h"
#include <vector>

duk_ret_t putPropDisplayFunctions(duk_context *ctx, duk_idx_t obj_idx, uint8_t magic) {
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "color", native_color, 4, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "fill", native_fillScreen, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "setCursor", native_setCursor, 2, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "print", native_print, DUK_VARARGS, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "println", native_println, DUK_VARARGS, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "setTextColor", native_setTextColor, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "setTextSize", native_setTextSize, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "setTextAlign", native_setTextAlign, 2, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawText", native_drawString, 3, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawString", native_drawString, 3, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawPixel", native_drawPixel, 3, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawLine", native_drawLine, 5, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawRect", native_drawRect, 5, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawFillRect", native_drawFillRect, 5, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawFillRectGradient", native_drawFillRectGradient, 7, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawRoundRect", native_drawRoundRect, 6, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawFillRoundRect", native_drawFillRoundRect, 6, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawCircle", native_drawCircle, 4, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawFillCircle", native_drawFillCircle, 4, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawXBitmap", native_drawXBitmap, 7, magic);
    // bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawBitmap", native_drawBitmap, 4, magic); 4bpp
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawJpg", native_drawJpg, 4, magic);
#if !defined(LITE_VERSION)
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "drawGif", native_drawGif, 6, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "gifOpen", native_gifOpen, 2, magic);
#endif
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "width", native_width, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "height", native_height, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "createSprite", native_createSprite, 2, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "getRotation", native_getRotation, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "getBrightness", native_getBrightness, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "setBrightness", native_setBrightness, 2, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "restoreBrightness", native_restoreBrightness, 0, magic);

    return 0;
}

duk_ret_t registerDisplay(duk_context *ctx) {
    bduk_register_c_lightfunc(ctx, "color", native_color, 4);
    bduk_register_c_lightfunc(ctx, "fillScreen", native_fillScreen, 1);
    bduk_register_c_lightfunc(ctx, "setTextColor", native_setTextColor, 1);
    bduk_register_c_lightfunc(ctx, "setTextSize", native_setTextSize, 1);
    bduk_register_c_lightfunc(ctx, "drawString", native_drawString, 3);
    bduk_register_c_lightfunc(ctx, "setCursor", native_setCursor, 2);
    bduk_register_c_lightfunc(ctx, "print", native_print, DUK_VARARGS);
    bduk_register_c_lightfunc(ctx, "println", native_println, DUK_VARARGS);
    bduk_register_c_lightfunc(ctx, "drawPixel", native_drawPixel, 3);
    bduk_register_c_lightfunc(ctx, "drawLine", native_drawLine, 5);
    bduk_register_c_lightfunc(ctx, "drawRect", native_drawRect, 5);
    bduk_register_c_lightfunc(ctx, "drawFillRect", native_drawFillRect, 5);
    bduk_register_c_lightfunc(ctx, "drawImage", native_drawImage, 4);
    bduk_register_c_lightfunc(ctx, "drawJpg", native_drawJpg, 4);
    bduk_register_c_lightfunc(ctx, "drawGif", native_drawGif, 6);
    bduk_register_c_lightfunc(ctx, "gifOpen", native_gifOpen, 2);
    bduk_register_c_lightfunc(ctx, "width", native_width, 0);
    bduk_register_c_lightfunc(ctx, "height", native_height, 0);
    bduk_register_c_lightfunc(ctx, "getRotation", native_getRotation, 0);
    bduk_register_c_lightfunc(ctx, "getBrightness", native_getBrightness, 0);
    bduk_register_c_lightfunc(ctx, "setBrightness", native_setBrightness, 2);
    bduk_register_c_lightfunc(ctx, "restoreBrightness", native_restoreBrightness, 0);
    return 0;
}

duk_ret_t native_color(duk_context *ctx) {
    int color = ((duk_get_int(ctx, 0) & 0xF8) << 8) | ((duk_get_int(ctx, 1) & 0xFC) << 3) |
                (duk_get_int(ctx, 2) >> 3);
    if (duk_get_int_default(ctx, 3, 16) == 16) {
        duk_push_int(ctx, color);
    } else {
        duk_push_int(ctx, ((color & 0xE000) >> 8) | ((color & 0x0700) >> 6) | ((color & 0x0018) >> 3));
    }
    return 1;
}

#if defined(HAS_SCREEN)
std::vector<TFT_eSprite *> sprites;
#endif
void clearSpritesVector() {
#if defined(HAS_SCREEN)
    for (auto sprite : sprites) {
        if (sprite != 0) {
            sprite->~TFT_eSprite();
            free(sprite);
            sprite = 0;
        }
    }
    sprites.clear();
#endif
}

#if defined(HAS_SCREEN)
static inline TFT_eSPI *get_display(duk_int_t sprite) __attribute__((always_inline));
static inline TFT_eSPI *get_display(duk_int_t sprite) {
    return sprite == 0 ? static_cast<TFT_eSPI *>(&tft) : sprites.at(sprite - 1);
}
#else
static inline SerialDisplayClass *get_display(duk_int_t sprite) __attribute__((always_inline));
static inline SerialDisplayClass *get_display(duk_int_t sprite) {
    return static_cast<SerialDisplayClass *>(&tft);
}
#endif

duk_ret_t native_setTextColor(duk_context *ctx) {
    get_display(duk_get_current_magic(ctx))->setTextColor(duk_get_int(ctx, 0));
    return 0;
}

duk_ret_t native_setTextSize(duk_context *ctx) {
    get_display(duk_get_current_magic(ctx))->setTextSize(duk_get_int(ctx, 0));
    return 0;
}

duk_ret_t native_setTextAlign(duk_context *ctx) {
    // usage: setTextAlign(align: number, baseline: number)
    // align: 0 - left, 1 - center, 2 - right
    // baseline: 0 - top, 1 - middle, 2 - bottom, 3 - alphabetic
    uint8_t align = duk_get_int(ctx, 0);
    uint8_t baseline = duk_get_int_default(ctx, 1, 0);

    if (duk_is_string(ctx, 0)) {
        const char *alignString = duk_get_string(ctx, 0);
        if (alignString[0] == 'l') align = 0;
        else if (alignString[0] == 'c') align = 1;
        else if (alignString[0] == 'r') align = 2;
    }

    if (duk_is_string(ctx, 1)) {
        const char *baselineString = duk_get_string(ctx, 1);
        if (baselineString[0] == 't') baseline = 0;
        else if (baselineString[0] == 'm') baseline = 1;
        else if (baselineString[0] == 'b') baseline = 2;
        else if (baselineString[0] == 'a') baseline = 3;
    }

    get_display(duk_get_current_magic(ctx))->setTextDatum(align + baseline * 3);
    return 0;
}

duk_ret_t native_drawRect(duk_context *ctx) {
    get_display(duk_get_current_magic(ctx))
        ->drawRect(
            duk_get_int(ctx, 0),
            duk_get_int(ctx, 1),
            duk_get_int(ctx, 2),
            duk_get_int(ctx, 3),
            duk_get_int(ctx, 4)
        );
    return 0;
}

duk_ret_t native_drawFillRect(duk_context *ctx) {
    get_display(duk_get_current_magic(ctx))
        ->fillRect(
            duk_get_int(ctx, 0),
            duk_get_int(ctx, 1),
            duk_get_int(ctx, 2),
            duk_get_int(ctx, 3),
            duk_get_int(ctx, 4)
        );
    return 0;
}

duk_ret_t native_drawFillRectGradient(duk_context *ctx) {
#if defined(HAS_SCREEN)
    if (duk_get_string_default(ctx, 6, "h")[0] == 'h') {
        get_display(duk_get_current_magic(ctx))
            ->fillRectHGradient(
                duk_get_int(ctx, 0),
                duk_get_int(ctx, 1),
                duk_get_int(ctx, 2),
                duk_get_int(ctx, 3),
                duk_get_int(ctx, 4),
                duk_get_int(ctx, 5)
            );
    } else {
        get_display(duk_get_current_magic(ctx))
            ->fillRectVGradient(
                duk_get_int(ctx, 0),
                duk_get_int(ctx, 1),
                duk_get_int(ctx, 2),
                duk_get_int(ctx, 3),
                duk_get_int(ctx, 4),
                duk_get_int(ctx, 5)
            );
    }
#else
    get_display(duk_get_current_magic(ctx))
        ->fillRect(
            duk_get_int(ctx, 0),
            duk_get_int(ctx, 1),
            duk_get_int(ctx, 2),
            duk_get_int(ctx, 3),
            duk_get_int(ctx, 4)
        );
#endif
    return 0;
}

duk_ret_t native_drawRoundRect(duk_context *ctx) {
    get_display(duk_get_current_magic(ctx))
        ->drawRoundRect(
            duk_get_int(ctx, 0),
            duk_get_int(ctx, 1),
            duk_get_int(ctx, 2),
            duk_get_int(ctx, 3),
            duk_get_int(ctx, 4),
            duk_get_int(ctx, 5)
        );
    return 0;
}

duk_ret_t native_drawFillRoundRect(duk_context *ctx) {
    get_display(duk_get_current_magic(ctx))
        ->fillRoundRect(
            duk_get_int(ctx, 0),
            duk_get_int(ctx, 1),
            duk_get_int(ctx, 2),
            duk_get_int(ctx, 3),
            duk_get_int(ctx, 4),
            duk_get_int(ctx, 5)
        );
    return 0;
}

duk_ret_t native_drawCircle(duk_context *ctx) {
    get_display(duk_get_current_magic(ctx))
        ->drawCircle(duk_get_int(ctx, 0), duk_get_int(ctx, 1), duk_get_int(ctx, 2), duk_get_int(ctx, 3));
    return 0;
}

duk_ret_t native_drawFillCircle(duk_context *ctx) {
    get_display(duk_get_current_magic(ctx))
        ->fillCircle(duk_get_int(ctx, 0), duk_get_int(ctx, 1), duk_get_int(ctx, 2), duk_get_int(ctx, 3));
    return 0;
}

duk_ret_t native_drawLine(duk_context *ctx) {
    // usage: drawLine(int16_t x, int16_t y, int16_t x2, int16_t y2, uint16_t color)
    get_display(duk_get_current_magic(ctx))
        ->drawLine(
            duk_get_int(ctx, 0),
            duk_get_int(ctx, 1),
            duk_get_int(ctx, 2),
            duk_get_int(ctx, 3),
            duk_get_int(ctx, 4)
        );
    return 0;
}

duk_ret_t native_drawPixel(duk_context *ctx) {
    // usage: drawPixel(int16_t x, int16_t y, uint16_t color)
    get_display(duk_get_current_magic(ctx))
        ->drawPixel(duk_get_int(ctx, 0), duk_get_int(ctx, 1), duk_get_int(ctx, 2));
    return 0;
}

duk_ret_t native_drawXBitmap(duk_context *ctx) {
    // usage: native_drawXBitmap(x: number, y: number, bitmap: ArrayBuffer, width: number, height: number,
    // fgColor: number, bgColor?: number)
    duk_int_t bitmapWidth = duk_get_int(ctx, 3);
    duk_int_t bitmapHeight = duk_get_int(ctx, 4);
    duk_size_t bitmapSize;
    uint8_t *bitmapPointer = (uint8_t *)duk_get_buffer_data(ctx, 2, &bitmapSize);
    if (bitmapPointer == NULL) {
        return duk_error(
            ctx, DUK_ERR_TYPE_ERROR, "%s: Failed to read bitmap data! Expected an ArrayBuffer.", "drawXBitmap"
        );
    }
    duk_size_t expectedSize = ((bitmapWidth + 7) / 8) * bitmapHeight; // Ensure proper rounding
    if (bitmapSize != expectedSize) {
        return duk_error(
            ctx,
            DUK_ERR_TYPE_ERROR,
            "%s: Bitmap size mismatch! Got %lu bytes, expected %lu bytes based on width=%d and height=%d.",
            "drawXBitmap",
            (unsigned long)bitmapSize,
            (unsigned long)expectedSize,
            bitmapWidth,
            bitmapHeight
        );
    }

    if (duk_is_number(ctx, 6)) {
        get_display(duk_get_current_magic(ctx))
            ->drawXBitmap(
                duk_get_int(ctx, 0),
                duk_get_int(ctx, 1),
                bitmapPointer,
                bitmapWidth,
                bitmapHeight,
                duk_get_int(ctx, 5),
                duk_get_int(ctx, 6)
            );
    } else {
        get_display(duk_get_current_magic(ctx))
            ->drawXBitmap(
                duk_get_int(ctx, 0),
                duk_get_int(ctx, 1),
                bitmapPointer,
                bitmapWidth,
                bitmapHeight,
                duk_get_int(ctx, 5)
            );
    }
    return 0;
}

duk_ret_t native_drawBitmap(duk_context *ctx) {
#if defined(HAS_SCREEN)
    // usage: drawBitmap(x: number, y: number, bitmap: ArrayBuffer, width: number, height: number, bpp: 16 | 8
    // | 4 | 1, palette?: ArrayBuffer)

    duk_int_t x = duk_get_int(ctx, 0);
    duk_int_t y = duk_get_int(ctx, 1);
    duk_int_t width = duk_get_int(ctx, 3);
    duk_int_t height = duk_get_int(ctx, 4);
    duk_int_t bpp = duk_get_int(ctx, 5);
    duk_size_t bitmapSize;
    uint8_t *bitmapPointer = (uint8_t *)duk_get_buffer_data(ctx, 2, &bitmapSize);

    if (!bitmapPointer) {
        return duk_error(
            ctx, DUK_ERR_TYPE_ERROR, "%s: Failed to read bitmap data! Expected an ArrayBuffer.", "drawBitmap"
        );
    }

    // Calculate expected bitmap size
    duk_size_t expectedSize;
    bool bpp8 = false;
    if (bpp == 16) {
        expectedSize = width * height * 2; // 16bpp (RGB565)
    } else if (bpp == 8) {
        expectedSize = width * height; // 8bpp (RGB332)
        bpp8 = true;
    } else if (bpp == 4) {
        expectedSize = (width * height + 1) / 2; // 4bpp (2 pixels per byte)
    } else if (bpp == 1) {
        expectedSize = ((width + 7) / 8) * height; // 1bpp (8 pixels per byte)
    } else {
        return duk_error(
            ctx, DUK_ERR_TYPE_ERROR, "%s: Unsupported bpp value! Use 16, 8, 4, or 1.", "drawBitmap"
        );
    }

    if (bitmapSize != expectedSize) {
        return duk_error(
            ctx,
            DUK_ERR_TYPE_ERROR,
            "%s: Bitmap size mismatch! Got %lu bytes, expected %lu bytes for %dx%d at %dbpp.",
            "drawBitmap",
            (unsigned long)bitmapSize,
            (unsigned long)expectedSize,
            width,
            height,
            bpp
        );
    }

    // Handle palette if needed (only for 4bpp and 1bpp)
    uint16_t *palette = nullptr;
    duk_size_t paletteSize = 0;
    if ((bpp == 4 || bpp == 1) && duk_is_buffer_data(ctx, 6)) {
        palette = (uint16_t *)duk_get_buffer_data(ctx, 6, &paletteSize);
        if (!palette || paletteSize == 0) {
            return duk_error(
                ctx, DUK_ERR_TYPE_ERROR, "%s: Invalid palette! Expected a valid ArrayBuffer.", "drawBitmap"
            );
        }
    }

    // Draw bitmap
    get_display(duk_get_current_magic(ctx))->pushImage(x, y, width, height, bitmapPointer, bpp8, palette);
    return 0;
#else
    return duk_error(ctx, DUK_ERR_ERROR, "%s: not supported on this device!", "drawBitmap");
#endif
}

duk_ret_t native_drawString(duk_context *ctx) {
    // drawString(const char *string, int32_t x, int32_t y)
    get_display(duk_get_current_magic(ctx))
        ->drawString(duk_to_string(ctx, 0), duk_get_int(ctx, 1), duk_get_int(ctx, 2));
    return 0;
}

duk_ret_t native_setCursor(duk_context *ctx) {
    // setCursor(int16_t x, int16_t y)
    get_display(duk_get_current_magic(ctx))->setCursor(duk_get_int(ctx, 0), duk_get_int(ctx, 0));
    return 0;
}

duk_ret_t native_print(duk_context *ctx) {
    internal_print(ctx, true, false);
    return 0;
}

duk_ret_t native_println(duk_context *ctx) {
    internal_print(ctx, true, true);
    return 0;
}

duk_ret_t native_fillScreen(duk_context *ctx) {
    // fill the screen or sprite with the passed color
    duk_int_t magic = duk_get_current_magic(ctx);
    if (magic == 0) {
        tft.fillScreen(duk_get_int(ctx, 0));
    } else {
#if defined(HAS_SCREEN)
        ((TFT_eSprite *)get_display(magic))->fillSprite(duk_get_int(ctx, 0));
#endif
    }
    return 0;
}

duk_ret_t native_width(duk_context *ctx) {
    int width = get_display(duk_get_current_magic(ctx))->width();
    duk_push_int(ctx, width);
    return 1;
}

duk_ret_t native_height(duk_context *ctx) {
    int height = get_display(duk_get_current_magic(ctx))->height();
    duk_push_int(ctx, height);
    return 1;
}

duk_ret_t native_drawImage(duk_context *ctx) {
    FileParamsJS file = js_get_path_from_params(ctx, true, true);

    drawImg(
        *file.fs,
        file.path,
        duk_get_int_default(ctx, 1 + file.paramOffset, 0),
        duk_get_int_default(ctx, 2 + file.paramOffset, 0),
        duk_get_int_default(ctx, 3 + file.paramOffset, 0)
    );
    return 0;
}

duk_ret_t native_drawJpg(duk_context *ctx) {
    FileParamsJS file = js_get_path_from_params(ctx, true, true);

    showJpeg(
        *file.fs,
        file.path,
        duk_get_int_default(ctx, 1 + file.paramOffset, 0),
        duk_get_int_default(ctx, 2 + file.paramOffset, 0),
        duk_get_int_default(ctx, 3 + file.paramOffset, 0)
    );
    return 0;
}
#if !defined(LITE_VERSION)
duk_ret_t native_drawGif(duk_context *ctx) {
    FileParamsJS file = js_get_path_from_params(ctx, true, true);

    showGif(
        file.fs,
        file.path.c_str(),
        duk_get_int_default(ctx, 1 + file.paramOffset, 0),
        duk_get_int_default(ctx, 2 + file.paramOffset, 0),
        duk_get_int_default(ctx, 3 + file.paramOffset, 0),
        duk_get_int_default(ctx, 4 + file.paramOffset, 0)
    );
    return 0;
}

std::vector<Gif *> gifs;
void clearGifsVector() {
    for (auto gif : gifs) {
        delete gif;
        gif = NULL;
    }
    gifs.clear();
}

void clearDisplayModuleData() {
    clearGifsVector();
    clearSpritesVector();
}

duk_ret_t native_gifPlayFrame(duk_context *ctx) {
    int gifIndex = 0;
    int x = duk_get_int_default(ctx, 0, 0);
    int y = duk_get_int_default(ctx, 1, 0);
    int bSync = duk_get_int_default(ctx, 3, 1);

    duk_push_this(ctx);
    if (duk_get_prop_string(ctx, -1, "gifPointer")) { gifIndex = duk_to_int(ctx, -1) - 1; }

    uint8_t result = 0;
    if (gifIndex >= 0) {
        Gif *gif = gifs.at(gifIndex);
        if (gif != NULL) { result = gif->playFrame(x, y, bSync); }
    }

    duk_push_int(ctx, result);
    return 1;
}

duk_ret_t native_gifDimensions(duk_context *ctx) {
    int gifIndex = 0;

    duk_push_this(ctx);
    if (duk_get_prop_string(ctx, -1, "gifPointer")) { gifIndex = duk_to_int(ctx, -1) - 1; }

    if (gifIndex < 0) {
        duk_push_int(ctx, 0);
    } else {
        Gif *gif = gifs.at(gifIndex);
        if (gif != NULL) {
            int canvasWidth = gifs.at(gifIndex)->getCanvasWidth();
            int canvasHeight = gifs.at(gifIndex)->getCanvasHeight();

            duk_idx_t obj_idx = duk_push_object(ctx);
            bduk_put_prop(ctx, obj_idx, "width", duk_push_int, canvasWidth);
            bduk_put_prop(ctx, obj_idx, "height", duk_push_int, canvasHeight);
        }
    }

    return 1;
}

duk_ret_t native_gifReset(duk_context *ctx) {
    int gifIndex = 0;

    duk_push_this(ctx);
    if (duk_get_prop_string(ctx, -1, "gifPointer")) { gifIndex = duk_to_int(ctx, -1) - 1; }

    uint8_t result = 0;
    if (gifIndex >= 0) {
        Gif *gif = gifs.at(gifIndex);
        if (gif != NULL) {
            gifs.at(gifIndex)->reset();
            result = 1;
        }
    }
    duk_push_int(ctx, result);

    return 1;
}

duk_ret_t native_gifClose(duk_context *ctx) {
    int gifIndex = 0;

    if (duk_is_object(ctx, 0)) {
        duk_to_object(ctx, 0);
    } else {
        duk_push_this(ctx);
    }
    if (duk_get_prop_string(ctx, -1, "gifPointer")) { gifIndex = duk_to_int(ctx, -1) - 1; }

    uint8_t result = 0;
    if (gifIndex >= 0) {
        Gif *gif = gifs.at(gifIndex);
        if (gif != NULL) {
            delete gif;
            gifs.at(gifIndex) = NULL;
            result = 1;
            bduk_put_prop(ctx, -1, "gifPointer", duk_push_uint, 0);
        }
    }
    duk_push_int(ctx, result);

    return 1;
}

duk_ret_t native_gifOpen(duk_context *ctx) {
    FileParamsJS file = js_get_path_from_params(ctx, true, true);

    Gif *gif = new Gif();

    bool success = gif->openGIF(file.fs, file.path.c_str());
    if (!success) {
        duk_push_null(ctx); // return null if not success
    } else {
        gifs.push_back(gif);
        duk_idx_t obj_idx = duk_push_object(ctx);
        bduk_put_prop(
            ctx, obj_idx, "gifPointer", duk_push_uint, gifs.size()
        ); // MEMO: 1 is the first element so 0 can be error

        bduk_put_prop_c_lightfunc(ctx, obj_idx, "playFrame", native_gifPlayFrame, 3, 0);
        bduk_put_prop_c_lightfunc(ctx, obj_idx, "dimensions", native_gifDimensions, 0, 0);
        bduk_put_prop_c_lightfunc(ctx, obj_idx, "reset", native_gifReset, 0, 0);
        bduk_put_prop_c_lightfunc(ctx, obj_idx, "close", native_gifClose, 0, 0);

        duk_push_c_lightfunc(ctx, native_gifClose, 1, 1, 0);
        duk_set_finalizer(ctx, obj_idx);
    }

    return 1;
}
#endif

duk_ret_t native_deleteSprite(duk_context *ctx) {
    int spriteIndex = duk_get_current_magic(ctx) - 1;

    if (duk_is_object(ctx, 0)) {
        duk_to_object(ctx, 0);
        if (duk_get_prop_string(ctx, -1, "spritePointer")) { spriteIndex = duk_to_int(ctx, -1) - 1; }
    }

    uint8_t result = 0;
#if defined(HAS_SCREEN)
#ifndef BOARD_HAS_PSRAM
    duk_push_boolean(ctx, 1);
    return 1;
#else
    if (spriteIndex >= 0) {
        TFT_eSprite *sprite = sprites.at(spriteIndex);
        if (sprite != NULL) {
            sprite->~TFT_eSprite();
            free(sprite);
            sprites.at(spriteIndex) = NULL;
            result = 1;
            bduk_put_prop(ctx, -1, "spritePointer", duk_push_uint, 0);
        }
    }
#endif
#endif
    duk_push_int(ctx, result);

    return 1;
}

duk_ret_t native_pushSprite(duk_context *ctx) {
#if defined(HAS_SCREEN)
#ifndef BOARD_HAS_PSRAM
    duk_push_boolean(ctx, 1);
    return 1;
#else
    duk_int_t magic = duk_get_current_magic(ctx);
    sprites.at(magic - 1)->pushSprite(duk_get_int(ctx, 0), duk_get_int(ctx, 1));
#endif
#endif
    return 0;
}

duk_ret_t native_createSprite(duk_context *ctx) {
#if defined(HAS_SCREEN)

#ifndef BOARD_HAS_PSRAM
    duk_idx_t obj_idx = duk_push_object(ctx);
    bduk_put_prop(ctx, obj_idx, "spritePointer", duk_push_uint, 0);
    putPropDisplayFunctions(ctx, obj_idx, 0);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "pushSprite", native_pushSprite, 3, 0);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "deleteSprite", native_deleteSprite, 1, 0);
    duk_push_c_lightfunc(ctx, native_deleteSprite, 1, 1, 0);
    duk_set_finalizer(ctx, obj_idx);
    return 1;

#else

    TFT_eSprite *sprite = NULL;
    sprite = (TFT_eSprite *)(psramFound() ? ps_malloc(sizeof(TFT_eSprite)) : malloc(sizeof(TFT_eSprite)));
    // sprite = new TFT_eSprite(&tft);
    if (sprite == NULL) {
        return duk_error(ctx, DUK_ERR_ERROR, "%s: Memory allocation failed!", "createSprite");
    }
    new (sprite) TFT_eSprite(&tft);

    int16_t width = duk_get_number_default(ctx, 0, tft.width());
    int16_t height = duk_get_number_default(ctx, 1, tft.height());
    uint8_t colorDepth = duk_get_number_default(ctx, 2, 16);
    uint8_t frames = duk_get_number_default(ctx, 3, 1U);

    sprite->setColorDepth(colorDepth);
    sprite->createSprite(width, height, frames);

    sprites.push_back(sprite);

    duk_idx_t obj_idx = duk_push_object(ctx);
    bduk_put_prop(ctx, obj_idx, "spritePointer", duk_push_uint, sprites.size());
    putPropDisplayFunctions(
        ctx, obj_idx, sprites.size()
    ); // MEMO: 1 is the first element so 0 can be default tft
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "pushSprite", native_pushSprite, 3, sprites.size());
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "deleteSprite", native_deleteSprite, 1, sprites.size());

    duk_push_c_lightfunc(ctx, native_deleteSprite, 1, 1, sprites.size());
    duk_set_finalizer(ctx, obj_idx);
#endif
#else
    duk_push_object(ctx);
#endif

    return 1;
}

duk_ret_t native_getRotation(duk_context *ctx) {
    duk_push_int(ctx, bruceConfigPins.rotation);
    return 1;
}

duk_ret_t native_getBrightness(duk_context *ctx) {
    duk_push_int(ctx, currentScreenBrightness);
    return 1;
}

duk_ret_t native_setBrightness(duk_context *ctx) {
    int brightness = duk_get_int(ctx, 0);

    bool save = false;
    if (duk_is_object(ctx, 1)) {
        duk_get_prop_string(ctx, 1, "save");
        save = duk_get_boolean(ctx, -1);
    }
    setBrightness(brightness, save);

    return 1;
}

duk_ret_t native_restoreBrightness(duk_context *ctx) {
    setBrightness(bruceConfig.bright, false);
    return 0;
}
#endif
