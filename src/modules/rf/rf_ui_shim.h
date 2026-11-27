#ifndef __RF_UI_SHIM_H__
#define __RF_UI_SHIM_H__

// Gradient drawing for the SubGHz / nRF24 pages ported from Vandeta.
//
// Those pages called Vandeta's core/ui_gradient.h, which reads a theme stored in
// bruceConfig (uiGradient). Bruce has no such setting, so the four calls the
// pages make are implemented here on top of the configured primary/secondary
// colours: the same visible result (a band that runs from priColor to secColor)
// without adding a config field or a settings page.

#include "core/display.h"
#include <globals.h>
#include <math.h>

// Blend priColor towards secColor. t = 0 -> priColor, t = 1 -> secColor.
inline uint16_t uiGradientColorAt(float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const uint16_t a = bruceConfig.priColor;
    const uint16_t b = bruceConfig.secColor;

    // 5-6-5: rrrrrggggggbbbbb
    const int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
    const int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;

    const int r = ar + (int)((br - ar) * t);
    const int g = ag + (int)((bg - ag) * t);
    const int bl = ab + (int)((bb - ab) * t);

    return (uint16_t)((r << 11) | (g << 5) | bl);
}

// Fill a rectangle with a left-to-right gradient.
inline void uiGradientFillRect(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    for (int i = 0; i < w; i++) {
        tft.drawFastVLine(x + i, y, h, uiGradientColorAt((float)i / (float)w));
    }
}

// Rounded variant. The corner radius is small enough that a re-drawn rectangle
// plus two corner dots reads the same at this size, and it costs one pass.
inline void uiGradientFillRoundRect(int x, int y, int w, int h, int r) {
    uiGradientFillRect(x + r, y, w - 2 * r > 0 ? w - 2 * r : w, h);
    uiGradientFillRect(x, y + r, r, h - 2 * r > 0 ? h - 2 * r : h);
    uiGradientFillRect(x + w - r, y + r, r, h - 2 * r > 0 ? h - 2 * r : h);
}

#endif
