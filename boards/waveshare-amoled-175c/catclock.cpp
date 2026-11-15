/*
 * Cat Clock - animated watch face for the Waveshare ESP32-S3-Touch-AMOLED-1.75C
 *
 * A full-round (466x466) pixel-art night scene rendered in software into a
 * PSRAM framebuffer and pushed to the CO5300 in dirty 32x32 tiles:
 *   twinkling stars, shooting stars, drifting clouds, flickering city windows,
 *   a blinking antenna light, and a cat on the rooftop whose tail sways, ears
 *   twitch and who sometimes looks back at you.
 * Big 12h/24h time with rolling digits, date, battery and a seconds ring.
 *
 * When Bruce's power-save turns the screen "off" this becomes a low-power
 * always-on display: black screen, dim time + sleeping cat, redrawn once a
 * minute at 80 MHz CPU. Touch or a button wakes it back to the animated face.
 *
 * int catClockRun(): 1 = Select pressed, 2 = Back pressed, -1 = could not start
 */
#include <Arduino.h>
#include <globals.h>
#include <interface.h>

#ifdef HAS_CAT_CLOCK
// #define CATCLOCK_SHOT 1 // debug: `nav up` over serial dumps the frame
#include "core/display.h"
#include "core/powerSave.h"
#include "core/settings.h"
#include "core/utils.h"
#include <Arduino_GFX_Library.h>
#include <math.h>

#include "font/glcdfont.h" // classic 5x7 font table: `font[]`

namespace {

const int S = AMOLED_PHYS_SIZE; // 466
const int C = S / 2;
const int TILE = 16; // dirty-tile size: small tiles = fewer pixels pushed per frame
const int TN = (S + TILE - 1) / TILE; // 15
const int HORIZON = 352;              // rooftop ledge line

uint16_t *bg = nullptr;      // static scenery
uint16_t *fb = nullptr;      // composed frame
uint16_t *pushBuf = nullptr; // contiguous copy of one tile run
uint8_t dirtyCur[TN * TN], dirtyPrev[TN * TN], dset[TN * TN];
Arduino_CO5300 *panel = nullptr;

// ---------- colour helpers ----------
inline uint16_t rgb(int r, int g, int b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
inline uint16_t mix(uint16_t d, uint16_t s, int a) { // a: 0..255 of s over d
    int dr = (d >> 11) & 31, dg = (d >> 5) & 63, db = d & 31;
    int sr = (s >> 11) & 31, sg = (s >> 5) & 63, sb = s & 31;
    dr += ((sr - dr) * a) >> 8;
    dg += ((sg - dg) * a) >> 8;
    db += ((sb - db) * a) >> 8;
    return (dr << 11) | (dg << 5) | db;
}

// ---------- tiny deterministic RNG ----------
uint32_t rngState = 1;
inline uint32_t rnd() {
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return rngState;
}
inline int rndRange(int a, int b) { return a + (int)(rnd() % (uint32_t)(b - a + 1)); }

// ---------- dirty tiles ----------
void mark(int x, int y, int w, int h) {
    int x0 = max(0, x) / TILE, y0 = max(0, y) / TILE;
    int x1 = min(S - 1, x + w - 1) / TILE, y1 = min(S - 1, y + h - 1) / TILE;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) dirtyCur[ty * TN + tx] = 1;
}
bool anyDirty(int x, int y, int w, int h) {
    if (x + w <= 0 || y + h <= 0 || x >= S || y >= S) return false;
    int x0 = max(0, x) / TILE, y0 = max(0, y) / TILE;
    int x1 = min(S - 1, x + w - 1) / TILE, y1 = min(S - 1, y + h - 1) / TILE;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (dset[ty * TN + tx]) return true;
    return false;
}

// ---------- framebuffer drawing (only inside dirty tiles) ----------
int clipY0 = 0, clipY1 = S; // extra vertical clip, used by the rolling digits
inline void px(int x, int y, uint16_t c) {
    if ((unsigned)x >= (unsigned)S || y < clipY0 || y >= clipY1 || (unsigned)y >= (unsigned)S) return;
    if (!dset[(y >> 4) * TN + (x >> 4)]) return;
    fb[y * S + x] = c;
}
inline void pxa(int x, int y, uint16_t c, int a) {
    if ((unsigned)x >= (unsigned)S || y < clipY0 || y >= clipY1 || (unsigned)y >= (unsigned)S) return;
    if (!dset[(y >> 4) * TN + (x >> 4)]) return;
    uint16_t *p = &fb[y * S + x];
    *p = mix(*p, c, a);
}
void rect(int x, int y, int w, int h, uint16_t c) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++) px(i, j, c);
}
void recta(int x, int y, int w, int h, uint16_t c, int a) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++) pxa(i, j, c, a);
}
void disc(int cx, int cy, int r, uint16_t c) {
    if (!anyDirty(cx - r, cy - r, 2 * r + 1, 2 * r + 1)) return;
    for (int j = -r; j <= r; j++) {
        int hw = (int)sqrtf((float)(r * r - j * j));
        for (int i = -hw; i <= hw; i++) px(cx + i, cy + j, c);
    }
}
void ellipse(int cx, int cy, int rx, int ry, uint16_t c, int yMax = S) {
    if (!anyDirty(cx - rx, cy - ry, 2 * rx + 1, 2 * ry + 1)) return;
    for (int j = -ry; j <= ry; j++) {
        if (cy + j >= yMax) break;
        int hw = (int)(rx * sqrtf(1.0f - (float)(j * j) / (float)(ry * ry)));
        for (int i = -hw; i <= hw; i++) px(cx + i, cy + j, c);
    }
}
void tri(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
    int minX = min(x0, min(x1, x2)), maxX = max(x0, max(x1, x2));
    int minY = min(y0, min(y1, y2)), maxY = max(y0, max(y1, y2));
    int area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
    if (area == 0) return;
    for (int y = minY; y <= maxY; y++)
        for (int x = minX; x <= maxX; x++) {
            int w0 = (x1 - x0) * (y - y0) - (x - x0) * (y1 - y0);
            int w1 = (x2 - x1) * (y - y1) - (x - x1) * (y2 - y1);
            int w2 = (x0 - x2) * (y - y2) - (x - x2) * (y0 - y2);
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) px(x, y, c);
        }
}

// ---------- text (5x7 font, scaled blocks) ----------
int textW(const char *s, int sc) { return (int)strlen(s) * 6 * sc - sc; }
void glyph(int x, int y, char ch, int sc, uint16_t top, uint16_t bot, int h7 = 7) {
    const uint8_t *g = &font[(uint8_t)ch * 5];
    for (int col = 0; col < 5; col++) {
        uint8_t bits = pgm_read_byte(&g[col]);
        for (int row = 0; row < h7; row++) {
            if (!(bits & (1 << row))) continue;
            uint16_t c = (top == bot) ? top : mix(top, bot, row * 255 / 6);
            rect(x + col * sc, y + row * sc, sc, sc, c);
        }
    }
}
void text(int x, int y, const char *s, int sc, uint16_t c, uint16_t shadow = 0, int sh = 0) {
    if (!anyDirty(x, y, textW(s, sc) + sh, 8 * sc + sh)) return;
    for (int k = 0; s[k]; k++) {
        if (sh) glyph(x + k * 6 * sc + sh, y + sh, s[k], sc, shadow, shadow);
        glyph(x + k * 6 * sc, y, s[k], sc, c, c);
    }
}

// ---------- pushing tiles to the panel ----------
void pushRun(int tx0, int tx1, int ty) {
    int x = tx0 * TILE, y = ty * TILE;
    int w = min(S, (tx1 + 1) * TILE) - x, h = min(S, y + TILE) - y;
    for (int j = 0; j < h; j++) memcpy(&pushBuf[j * w], &fb[(y + j) * S + x], w * 2);
    tft.lock();
    panel->startWrite();
    panel->writeAddrWindow(x - AMOLED_WIN_OFS, y - AMOLED_WIN_OFS, w, h);
    panel->writePixels(pushBuf, (uint32_t)w * h);
    panel->endWrite();
    tft.unlock();
}
void pushDirty() {
    for (int ty = 0; ty < TN; ty++) {
        int tx = 0;
        while (tx < TN) {
            if (!dset[ty * TN + tx]) {
                tx++;
                continue;
            }
            int s0 = tx;
            while (tx < TN && dset[ty * TN + tx]) tx++;
            pushRun(s0, tx - 1, ty);
        }
    }
}
void restoreDirty() {
    for (int ty = 0; ty < TN; ty++)
        for (int tx = 0; tx < TN; tx++) {
            if (!dset[ty * TN + tx]) continue;
            int x = tx * TILE, y = ty * TILE;
            int w = min(S, x + TILE) - x, h = min(S, y + TILE) - y;
            for (int j = 0; j < h; j++) memcpy(&fb[(y + j) * S + x], &bg[(y + j) * S + x], w * 2);
        }
}
void panelBrightness(uint8_t v) {
    tft.lock();
    panel->setBrightness(v);
    tft.unlock();
}

// =====================================================================
// Scenery
// =====================================================================
struct Window {
    int16_t x, y;
    uint8_t on, warm;
};
const int MAXWIN = 420;
Window wins[MAXWIN];
int nWins = 0;

struct Star {
    int16_t x, y;
    uint8_t big, level;
    float phase, speed;
};
const int NSTARS = 48;
Star stars[NSTARS];

struct Cloud {
    float x;
    int16_t y, w;
    int lastX;
    float speed;
};
Cloud clouds[3];

const int CAT_X = 196;   // cat centre
const int ANT_X = 338;   // antenna pole
const int ANT_TOP = 280; // antenna tip

void skyColor(int y, int &r, int &g, int &b) {
    static const int stops[][4] = {
        {0,   8,   6,   30 },
        {120, 24,  15,  68 },
        {215, 70,  30,  110},
        {285, 158, 54,  122},
        {330, 236, 108, 106},
        {356, 252, 164, 112},
    };
    const int n = sizeof(stops) / sizeof(stops[0]);
    if (y <= stops[0][0]) y = stops[0][0];
    if (y >= stops[n - 1][0]) y = stops[n - 1][0];
    for (int i = 0; i < n - 1; i++) {
        if (y <= stops[i + 1][0]) {
            int span = stops[i + 1][0] - stops[i][0], t = y - stops[i][0];
            r = stops[i][1] + (stops[i + 1][1] - stops[i][1]) * t / span;
            g = stops[i][2] + (stops[i + 1][2] - stops[i][2]) * t / span;
            b = stops[i][3] + (stops[i + 1][3] - stops[i][3]) * t / span;
            return;
        }
    }
}

void bgRect(int x, int y, int w, int h, uint16_t c) {
    for (int j = max(0, y); j < min(S, y + h); j++)
        for (int i = max(0, x); i < min(S, x + w); i++) bg[j * S + i] = c;
}

void buildBackground() {
    static const int8_t bayer[4][4] = {
        {-8, 0,  -6, 2 },
        {4,  -4, 6,  -2},
        {-5, 3,  -7, 1 },
        {7,  -1, 5,  -3}
    };
    // --- dithered sunset sky ---
    for (int y = 0; y < S; y++) {
        int r, g, b;
        skyColor(y, r, g, b);
        for (int x = 0; x < S; x++) {
            int d = bayer[y & 3][x & 3];
            bg[y * S + x] =
                rgb(constrain(r + d / 2, 0, 255), constrain(g + d / 4, 0, 255), constrain(b + d / 2, 0, 255));
        }
    }
    rngState = 0xC0FFEE11; // fixed seed: the same skyline every time
    // --- faint static stars ---
    for (int i = 0; i < 150; i++) {
        int x = rndRange(8, S - 8), y = rndRange(8, 250);
        uint16_t c = mix(bg[y * S + x], rgb(255, 255, 255), rndRange(50, 120));
        bg[y * S + x] = c;
        if (rnd() & 1) bg[y * S + x + 1] = c;
    }
    // --- crescent moon with glow ---
    const int mx = 100, my = 232, mr = 24;
    for (int y = my - mr - 30; y <= my + mr + 30; y++)
        for (int x = mx - mr - 30; x <= mx + mr + 30; x++) {
            float d = sqrtf((float)((x - mx) * (x - mx) + (y - my) * (y - my)));
            float d2 = sqrtf((float)((x - mx - 11) * (x - mx - 11) + (y - my + 7) * (y - my + 7)));
            if (d <= mr && d2 > mr - 3) bg[y * S + x] = rgb(255, 240, 206);
            else if (d < mr + 30) {
                int a = (int)(70 * (1.0f - (d - mr * 0.4f) / (mr * 0.6f + 30)));
                if (a > 0) bg[y * S + x] = mix(bg[y * S + x], rgb(255, 196, 170), a);
            }
        }
    // --- far skyline ---
    int x = -6;
    while (x < S) {
        int w = rndRange(18, 40), top = rndRange(262, 312);
        bgRect(x, top, w, HORIZON - top, rgb(84, 38, 112));
        if (rnd() % 3 == 0) bgRect(x + w / 2 - 1, top - rndRange(8, 18), 3, 20, rgb(84, 38, 112)); // spire
        x += w;
    }
    // --- near skyline with windows ---
    nWins = 0;
    x = -10;
    while (x < S) {
        int w = rndRange(28, 56), top = rndRange(286, 334);
        if (x + w > CAT_X - 62 && x < CAT_X + 96) top = rndRange(328, 340); // keep the sky open behind the cat
        bgRect(x, top, w, HORIZON - top, rgb(34, 17, 62));
        bgRect(x, top, w, 2, rgb(58, 28, 88)); // roof edge catching the light
        for (int wy = top + 6; wy < HORIZON - 6; wy += 9)
            for (int wx = x + 5; wx < x + w - 6; wx += 8) {
                if (nWins >= MAXWIN) break;
                wins[nWins++] = {(int16_t)wx, (int16_t)wy, (uint8_t)(rnd() % 100 < 42), (uint8_t)(rnd() % 4 == 0)};
            }
        x += w + rndRange(0, 3);
    }
    // --- rooftop in the foreground ---
    bgRect(0, HORIZON, S, S - HORIZON, rgb(13, 7, 25));
    bgRect(0, HORIZON, S, 4, rgb(52, 28, 80)); // ledge highlight
    for (int row = 0, y = HORIZON + 12; y < S; y += 14, row++) {
        bgRect(0, y, S, 2, rgb(24, 13, 42));
        for (int bx = (row & 1) ? 20 : 0; bx < S; bx += 40) bgRect(bx, y - 12, 2, 12, rgb(24, 13, 42));
    }
    // --- antenna ---
    bgRect(ANT_X, ANT_TOP + 6, 4, HORIZON - ANT_TOP - 6, rgb(13, 7, 25));
    bgRect(ANT_X - 14, ANT_TOP + 22, 32, 3, rgb(13, 7, 25));
    bgRect(ANT_X - 9, ANT_TOP + 36, 22, 3, rgb(13, 7, 25));
    bgRect(ANT_X - 14, ANT_TOP + 16, 3, 8, rgb(13, 7, 25));
    bgRect(ANT_X + 15, ANT_TOP + 16, 3, 8, rgb(13, 7, 25));

    // --- animated element initial state ---
    rngState = esp_random() | 1;
    for (int i = 0; i < NSTARS; i++) {
        int sx, sy;
        do {
            sx = rndRange(20, S - 20);
            sy = rndRange(14, 246);
        } while ((sx - C) * (sx - C) + (sy - C) * (sy - C) > 222 * 222);
        stars[i] = {(int16_t)sx,
                    (int16_t)sy,
                    (uint8_t)(rnd() % 4 == 0),
                    0,
                    (float)(rnd() % 628) / 100.0f,
                    0.6f + (float)(rnd() % 200) / 100.0f};
    }
    clouds[0] = {40.0f, 204, 96, -999, 5.0f};
    clouds[1] = {260.0f, 232, 120, -999, 7.5f};
    clouds[2] = {-80.0f, 256, 84, -999, 4.0f};
}

// =====================================================================
// Animated elements
// =====================================================================
uint32_t nowMs = 0;
float tSec = 0;

void drawStars() {
    for (int i = 0; i < NSTARS; i++) {
        Star &s = stars[i];
        float v = 0.5f + 0.5f * sinf(s.phase + tSec * s.speed);
        uint8_t lvl = (uint8_t)(v * 15.99f);
        if (lvl != s.level) {
            s.level = lvl;
            mark(s.x - 4, s.y - 4, 10, 10);
        }
        if (!anyDirty(s.x - 4, s.y - 4, 10, 10)) continue;
        int a = 60 + lvl * 13;
        uint16_t c = rgb(235, 235, 255);
        recta(s.x, s.y, 2, 2, c, a);
        if (s.big && lvl >= 5) {
            int arm = lvl >= 11 ? 4 : 2, aa = (lvl - 4) * 11;
            recta(s.x - arm, s.y, arm, 2, c, aa);
            recta(s.x + 2, s.y, arm, 2, c, aa);
            recta(s.x, s.y - arm, 2, arm, c, aa);
            recta(s.x, s.y + 2, 2, arm, c, aa);
        }
    }
}

struct {
    bool active = false;
    uint32_t t0 = 0, next = 3000;
    float x0, y0, dx, dy;
} shoot;

void drawShootingStar() {
    if (!shoot.active) {
        if (nowMs < shoot.next) return;
        shoot.active = true;
        shoot.t0 = nowMs;
        shoot.x0 = rndRange(40, 250);
        shoot.y0 = rndRange(20, 90);
        shoot.dx = rndRange(170, 250);
        shoot.dy = shoot.dx * (0.30f + (rnd() % 30) / 100.0f);
    }
    float p = (nowMs - shoot.t0) / 750.0f;
    if (p >= 1.0f) {
        shoot.active = false;
        shoot.next = nowMs + rndRange(5000, 12000);
        return;
    }
    float hx = shoot.x0 + shoot.dx * p, hy = shoot.y0 + shoot.dy * p;
    int bx = (int)(hx - shoot.dx * 0.32f) - 6, by = (int)(hy - shoot.dy * 0.32f) - 6;
    mark(bx, by, (int)(shoot.dx * 0.32f) + 14, (int)(shoot.dy * 0.32f) + 14);
    int fade = p > 0.75f ? (int)((1.0f - p) * 4 * 255) : 255;
    for (int k = 0; k < 22; k++) {
        float q = k / 22.0f;
        int x = (int)(hx - shoot.dx * 0.30f * q), y = (int)(hy - shoot.dy * 0.30f * q);
        int a = (int)((1.0f - q) * (1.0f - q) * fade);
        int sz = k < 3 ? 3 : 2;
        recta(x, y, sz, sz, rgb(255, 250, 235), a);
    }
}

void drawClouds(float dt) {
    for (auto &c : clouds) {
        c.x += c.speed * dt;
        if (c.x > S + 10) c.x = -c.w - 10;
        int q = (int)floorf(c.x * 4.0f); // position in quarter pixels
        int ix = q >> 2, f = (q & 3) * 64;  // whole pixel + fraction (0..192 of 256)
        if (q != c.lastX) {
            c.lastX = q;
            mark(ix - 2, c.y - 2, c.w + 5, 22);
        }
        if (!anyDirty(ix, c.y, c.w + 1, 20)) continue;
        uint16_t col = rgb(255, 176, 206);
        auto band = [&](int x, int y, int w, int h, int a) {
            recta(x, y, 1, h, col, a * (256 - f) >> 8); // trailing edge fades out
            recta(x + 1, y, w - 1, h, col, a);
            recta(x + w, y, 1, h, col, a * f >> 8); // leading edge fades in
        };
        band(ix + c.w / 5, c.y, c.w / 2, 5, 40);
        band(ix + c.w / 12, c.y + 5, c.w * 5 / 6, 6, 52);
        band(ix, c.y + 11, c.w, 5, 44);
        band(ix + c.w / 6, c.y + 16, c.w * 3 / 5, 3, 30);
    }
}

void drawWindows() {
    static uint32_t nextFlip = 0;
    if (nowMs > nextFlip && nWins > 0) {
        nextFlip = nowMs + rndRange(180, 520);
        Window &w = wins[rnd() % nWins];
        w.on ^= 1;
        mark(w.x, w.y, 4, 5);
    }
    for (int i = 0; i < nWins; i++) {
        const Window &w = wins[i];
        if (!w.on || !anyDirty(w.x, w.y, 4, 5)) continue;
        rect(w.x, w.y, 4, 5, w.warm ? rgb(255, 150, 96) : rgb(255, 216, 128));
    }
}

void drawAntennaLight() {
    static bool lastOn = false;
    bool on = (nowMs % 1600) < 220;
    if (on != lastOn) {
        lastOn = on;
        mark(ANT_X - 9, ANT_TOP - 9, 22, 22);
    }
    if (!anyDirty(ANT_X - 9, ANT_TOP - 9, 22, 22)) return;
    if (on) {
        for (int j = -9; j <= 9; j++)
            for (int i = -9; i <= 9; i++) {
                int d2 = i * i + j * j;
                if (d2 < 81) pxa(ANT_X + 2 + i, ANT_TOP + 2 + j, rgb(255, 60, 60), 150 - d2 * 150 / 81);
            }
        rect(ANT_X, ANT_TOP, 4, 5, rgb(255, 120, 110));
    } else {
        rect(ANT_X, ANT_TOP, 4, 5, rgb(110, 26, 44));
    }
}

struct {
    uint32_t earUntil = 0, nextEar = 2500;
    uint32_t lookStart = 0, nextLook = 9000;
    int lastState = -1;
} cat;

void drawCat() {
    const int cx = CAT_X, by = HORIZON;
    const uint16_t fur = rgb(9, 5, 19), rim = rgb(96, 44, 104);

    // --- behaviour state ---
    if (nowMs > cat.nextEar) {
        cat.earUntil = nowMs + 160;
        cat.nextEar = nowMs + rndRange(2200, 6500);
    }
    bool earFlick = nowMs < cat.earUntil;
    if (cat.lookStart == 0 && nowMs > cat.nextLook) cat.lookStart = nowMs;
    int eyes = 0; // 0 none, 1 open, 2 blink
    if (cat.lookStart) {
        uint32_t e = nowMs - cat.lookStart;
        if (e > 3200) {
            cat.lookStart = 0;
            cat.nextLook = nowMs + rndRange(12000, 24000);
        } else eyes = (e > 1500 && e < 1650) ? 2 : 1;
    }
    int state = eyes * 2 + (earFlick ? 1 : 0);
    if (state != cat.lastState) {
        cat.lastState = state;
        mark(cx - 24, by - 96, 48, 52);
    }

    // --- tail: lies on the ledge to the right, the tip curls and sways ---
    float sway = sinf(tSec * 1.7f), sway2 = sinf(tSec * 1.7f - 0.9f);
    mark(cx + 12, by - 56, 76, 58);
    if (anyDirty(cx + 12, by - 56, 76, 58)) {
        for (int k = 0; k <= 30; k++) {
            float t = k / 30.0f;
            float x = cx + 18 + t * 46 - t * t * t * 12 * sway2;
            float y = by - 6 - t * t * (26 + 16 * sway);
            int r = 5 - (int)(t * 2.2f);
            disc((int)x, (int)y, r, fur);
        }
    }

    // --- body ---
    if (anyDirty(cx - 30, by - 96, 62, 96)) {
        ellipse(cx + 1, by - 13, 28, 15, rim, by); // haunches (rim light first, fur on top)
        ellipse(cx, by - 28, 23, 29, rim, by);
        disc(cx - 1, by - 64, 16, rim);
        ellipse(cx - 1, by - 13, 28, 15, fur, by);
        ellipse(cx - 2, by - 28, 23, 29, fur, by);
        disc(cx - 3, by - 64, 16, fur);
        // ears (the right one flicks)
        tri(cx - 17, by - 70, cx - 13, by - 90, cx - 4, by - 76, fur);
        int tipX = earFlick ? cx + 13 : cx + 9, tipY = earFlick ? by - 86 : by - 90;
        tri(cx - 2, by - 76, tipX, tipY, cx + 12, by - 70, fur);
        // looking back at you
        if (eyes == 1) {
            rect(cx - 11, by - 68, 6, 5, rgb(214, 255, 130));
            rect(cx + 1, by - 68, 6, 5, rgb(214, 255, 130));
            rect(cx - 9, by - 68, 2, 5, fur);
            rect(cx + 3, by - 68, 2, 5, fur);
        } else if (eyes == 2) {
            rect(cx - 11, by - 66, 6, 2, rgb(150, 180, 90));
            rect(cx + 1, by - 66, 6, 2, rgb(150, 180, 90));
        }
    }
}

// ---------- seconds ring ----------
void drawSecondsRing(int sec, float frac) {
    static int lastSec = -1;
    static int16_t rx[60], ry[60];
    static bool ringReady = false;
    if (!ringReady) {
        ringReady = true;
        for (int i = 0; i < 60; i++) {
            float ang = (i * 6 - 90) * DEG_TO_RAD;
            rx[i] = C + (int)lroundf(cosf(ang) * 224);
            ry[i] = C + (int)lroundf(sinf(ang) * 224);
        }
    }
    for (int i = 0; i < 60; i++) {
        int x = rx[i], y = ry[i];
        if (sec != lastSec && (i == sec || i == lastSec || sec == 0)) mark(x - 5, y - 5, 10, 10);
        if (i == sec) mark(x - 5, y - 5, 10, 10);
        if (!anyDirty(x - 5, y - 5, 10, 10)) continue;
        if (i == sec) {
            int in = frac < 0.25f ? (int)(frac * 4 * 255) : 255; // fade in, then a slow glow
            int glow = (int)(110 * (0.5f + 0.5f * cosf(frac * 6.2832f)));
            recta(x - 4, y - 4, 9, 9, rgb(255, 200, 220), glow * in >> 8);
            recta(x - 2, y - 2, 5, 5, rgb(255, 240, 245), in);
        } else if (i < sec) {
            rect(x - 2, y - 2, 4, 4, (i % 5 == 0) ? rgb(255, 196, 150) : rgb(196, 150, 250));
        } else {
            recta(x - 1, y - 1, 3, 3, rgb(210, 190, 255), (i % 5 == 0) ? 150 : 70);
        }
    }
    lastSec = sec;
}

// ---------- big time with rolling digits ----------
const int DS = 11;   // digit block size
const int TY = 84;   // top of the digits
char shownStr[8] = "";
char prevStr[8] = "";
uint32_t rollStart = 0;
const uint32_t ROLL_MS = 520;

int timeStrWidth(const char *s) {
    int w = 0;
    for (int k = 0; s[k]; k++) w += (s[k] == ':' ? 3 * DS : 6 * DS);
    return w - DS;
}
void drawBigChar(int x, int y, char ch) {
    const uint16_t top = rgb(255, 240, 200), bot = rgb(255, 150, 92), sh = rgb(34, 12, 58);
    const uint8_t *g = &font[(uint8_t)ch * 5];
    for (int pass = 0; pass < 2; pass++)
        for (int col = 0; col < 5; col++) {
            uint8_t bits = pgm_read_byte(&g[col]);
            for (int row = 0; row < 7; row++) {
                if (!(bits & (1 << row))) continue;
                if (!anyDirty(x + col * DS, y + row * DS, DS + 5, DS + 5)) continue; // skip untouched blocks
                if (pass == 0) recta(x + col * DS + 5, y + row * DS + 5, DS, DS, sh, 170);
                else {
                    uint16_t c = mix(top, bot, row * 255 / 6);
                    rect(x + col * DS, y + row * DS, DS, DS, c);
                    rect(x + col * DS, y + row * DS, DS, 2, mix(c, rgb(255, 255, 255), 90)); // pixel bevel
                }
            }
        }
}
void drawTime(const struct tm &t) {
    char s[8];
    int hr = t.tm_hour;
    if (!bruceConfig.clock24hr) {
        hr %= 12;
        if (hr == 0) hr = 12;
        snprintf(s, sizeof(s), "%d:%02d", hr, t.tm_min);
    } else snprintf(s, sizeof(s), "%02d:%02d", hr, t.tm_min);

    const int bandY = TY - 4, bandH = 7 * DS + 14;
    if (strcmp(s, shownStr) != 0) {
        strcpy(prevStr, shownStr);
        strcpy(shownStr, s);
        rollStart = nowMs;
    }
    bool rolling = rollStart && (nowMs - rollStart < ROLL_MS);
    static bool lastColon = true, wasRolling = false, last24 = true;
    bool colonOn = (nowMs % 1000) < 550;
    if (rolling || wasRolling || last24 != (bool)bruceConfig.clock24hr) mark(C - 205, bandY, 410, bandH);
    wasRolling = rolling;
    last24 = bruceConfig.clock24hr;
    int w = timeStrWidth(shownStr), x0 = C - w / 2;
    if (colonOn != lastColon) {
        lastColon = colonOn;
        const char *cp = strchr(shownStr, ':');
        int colonX = x0 + (cp ? (int)(cp - shownStr) : 2) * 6 * DS;
        mark(colonX - 2, bandY, DS + 10, bandH);
    }
    if (!anyDirty(C - 205, bandY, 410, bandH)) return;

    float p = rolling ? (nowMs - rollStart) / (float)ROLL_MS : 1.0f;
    float e = 1.0f - (1.0f - p) * (1.0f - p) * (1.0f - p); // ease-out
    bool sameLen = strlen(prevStr) == strlen(shownStr);
    clipY0 = TY - 2;
    clipY1 = TY + 7 * DS + 8;
    int x = x0;
    for (int k = 0; shownStr[k]; k++) {
        char ch = shownStr[k];
        if (ch == ':') {
            int a = colonOn ? 255 : 70;
            recta(x + 5, TY + 2 * DS + 5, DS, DS, rgb(34, 12, 58), a * 170 / 255);
            recta(x + 5, TY + 4 * DS + 5, DS, DS, rgb(34, 12, 58), a * 170 / 255);
            recta(x, TY + 2 * DS, DS, DS, rgb(255, 214, 160), a);
            recta(x, TY + 4 * DS, DS, DS, rgb(255, 176, 120), a);
            x += 3 * DS;
            continue;
        }
        bool changed = rolling && (!sameLen || prevStr[k] != ch);
        if (changed) {
            int off = (int)(e * (7 * DS + 12));
            if (sameLen && prevStr[k]) drawBigChar(x, TY - off, prevStr[k]);
            drawBigChar(x, TY + (7 * DS + 12) - off, ch);
        } else drawBigChar(x, TY, ch);
        x += 6 * DS;
    }
    clipY0 = 0;
    clipY1 = S;
    if (!bruceConfig.clock24hr) text(x0 + w + 8, TY + 7 * DS - 21, t.tm_hour < 12 ? "AM" : "PM", 3, rgb(255, 196, 150), rgb(34, 12, 58), 2);
}

// ---------- date + battery ----------
const char *const DAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const char *const MONTHS[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

void drawInfo(const struct tm &t) {
    static char lastDate[16] = "";
    static int lastBat = -1, bat = 0;
    static bool lastChg = false, chg = false;
    static uint32_t nextBat = 0;
    char d[16];
    snprintf(d, sizeof(d), "%s %d %s", DAYS[t.tm_wday % 7], t.tm_mday, MONTHS[t.tm_mon % 12]);
    if (strcmp(d, lastDate) != 0) {
        strcpy(lastDate, d);
        mark(C - 120, 50, 240, 28);
    }
    text(C - textW(d, 3) / 2, 52, d, 3, rgb(232, 218, 255), rgb(26, 10, 52), 2);

    if (nowMs >= nextBat) {
        nextBat = nowMs + 10000;
        bat = getBattery();
        chg = isCharging();
    }
    const int by = 186;
    if (bat != lastBat || chg != lastChg) {
        lastBat = bat;
        lastChg = chg;
        mark(C - 70, by - 4, 140, 26);
    }
    if (!anyDirty(C - 70, by - 4, 140, 26)) return;
    char b[12];
    snprintf(b, sizeof(b), "%d%%", bat);
    int tw = textW(b, 2), total = 34 + 8 + tw + (chg ? 16 : 0);
    int x = C - total / 2;
    uint16_t line = rgb(232, 218, 255);
    rect(x, by, 30, 2, line);
    rect(x, by + 14, 30, 2, line);
    rect(x, by, 2, 16, line);
    rect(x + 28, by, 2, 16, line);
    rect(x + 30, by + 5, 3, 6, line);
    int fill = constrain(bat, 0, 100) * 22 / 100;
    rect(x + 4, by + 4, fill, 8, bat <= 15 ? rgb(255, 90, 90) : (chg ? rgb(130, 255, 170) : rgb(255, 196, 150)));
    text(x + 42, by + 1, b, 2, line, rgb(26, 10, 52), 2);
    if (chg) { // little lightning bolt
        int lx = x + 42 + tw + 8;
        tri(lx + 6, by - 1, lx, by + 9, lx + 5, by + 9, rgb(255, 230, 120));
        tri(lx + 3, by + 7, lx + 8, by + 7, lx + 2, by + 17, rgb(255, 230, 120));
    }
}

// =====================================================================
// Frames
// =====================================================================
uint32_t dbgCompose = 0, dbgPush = 0, dbgTiles = 0;
void composeAndPush(bool full, float dt) {
    uint32_t t0 = micros();
    memset(dirtyCur, full ? 1 : 0, sizeof(dirtyCur));
    // pass 1: update state + collect dirty tiles (drawing is a no-op: dset is empty)
    // pass 2: restore those tiles from the scenery and draw everything into them
    struct tm t = rtc.getTimeStruct();
    float frac = (nowMs % 1000) / 1000.0f;
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 0) memset(dset, 0, sizeof(dset));
        else {
            for (int i = 0; i < TN * TN; i++) dset[i] = dirtyCur[i] | dirtyPrev[i];
            restoreDirty();
        }
        drawStars();
        drawClouds(pass == 0 ? dt : 0);
        drawShootingStar();
        drawWindows();
        drawAntennaLight();
        drawCat();
        drawSecondsRing(t.tm_sec, frac);
        drawTime(t);
        drawInfo(t);
    }
    uint32_t t1 = micros();
    pushDirty();
    dbgPush += micros() - t1;
    dbgCompose += t1 - t0;
    for (int i = 0; i < TN * TN; i++) dbgTiles += dset[i];
    memcpy(dirtyPrev, dirtyCur, sizeof(dirtyPrev));
}

// Push the whole composed frame as rings growing from the centre
void revealFull() {
    memset(dirtyCur, 1, sizeof(dirtyCur));
    memset(dirtyPrev, 0, sizeof(dirtyPrev));
    nowMs = millis();
    tSec = nowMs / 1000.0f;
    struct tm t = rtc.getTimeStruct();
    memset(dset, 1, sizeof(dset));
    restoreDirty();
    drawStars();
    drawClouds(0);
    drawWindows();
    drawAntennaLight();
    drawCat();
    drawSecondsRing(t.tm_sec, 0);
    drawTime(t);
    drawInfo(t);
    for (int ring = 0; ring <= TN; ring++) {
        bool any = false;
        for (int ty = 0; ty < TN; ty++)
            for (int tx = 0; tx < TN; tx++) {
                int d = max(abs(tx - TN / 2), abs(ty - TN / 2));
                dset[ty * TN + tx] = (d == ring);
                any |= (d == ring);
            }
        if (any) pushDirty();
    }
    memcpy(dirtyPrev, dirtyCur, sizeof(dirtyPrev));
}

void blackout() {
    memset(fb, 0, (size_t)S * S * 2);
    memset(dset, 1, sizeof(dset));
    pushDirty();
}

// ---------- always-on display: dim time + sleeping cat ----------
void drawAod() {
    struct tm t = rtc.getTimeStruct();
    memset(fb, 0, (size_t)S * S * 2);
    memset(dset, 1, sizeof(dset));
    // shift a little every minute so no pixel stays lit forever (AMOLED burn-in)
    int ox = ((t.tm_min * 7) % 21) - 10, oy = ((t.tm_min * 5) % 17) - 8;
    char s[8];
    int hr = t.tm_hour;
    if (!bruceConfig.clock24hr) {
        hr %= 12;
        if (hr == 0) hr = 12;
        snprintf(s, sizeof(s), "%d:%02d", hr, t.tm_min);
    } else snprintf(s, sizeof(s), "%02d:%02d", hr, t.tm_min);
    const uint16_t ink = rgb(176, 160, 230), soft = rgb(104, 92, 150);
    int sc = 9, w = textW(s, sc);
    text(C - w / 2 + ox, 150 + oy, s, sc, ink);
    if (!bruceConfig.clock24hr) text(C + w / 2 + 8 + ox, 150 + 7 * sc - 16 + oy, t.tm_hour < 12 ? "AM" : "PM", 2, soft);
    char d[16];
    snprintf(d, sizeof(d), "%s %d %s", DAYS[t.tm_wday % 7], t.tm_mday, MONTHS[t.tm_mon % 12]);
    text(C - textW(d, 2) / 2 + ox, 118 + oy, d, 2, soft);
    // sleeping cat, curled up
    int cx = C + ox, cy = 290 + oy;
    const uint16_t fur = rgb(74, 62, 118);
    ellipse(cx, cy, 44, 19, fur);
    disc(cx - 34, cy - 4, 15, fur);
    tri(cx - 47, cy - 12, cx - 45, cy - 27, cx - 35, cy - 17, fur);
    tri(cx - 33, cy - 17, cx - 24, cy - 27, cx - 22, cy - 12, fur);
    for (int k = 0; k <= 24; k++) { // tail wrapped around the body
        float a = (20 + k * 5.5f) * DEG_TO_RAD;
        disc(cx + (int)(cosf(a) * 46) - 2, cy + (int)(sinf(a) * 19) + 2, 4, fur);
    }
    rect(cx - 41, cy - 3, 6, 2, 0); // closed eyes
    rect(cx - 30, cy - 3, 6, 2, 0);
    text(cx + 18, cy - 44, "z", 2, soft);
    text(cx + 34, cy - 60, "Z", 2, soft);
    pushDirty();
}

void freeBuffers() {
    if (bg) free(bg);
    if (fb) free(fb);
    if (pushBuf) free(pushBuf);
    bg = fb = pushBuf = nullptr;
}

} // namespace

int catClockRun() {
    panel = static_cast<Arduino_CO5300 *>(tft.native());
    if (!panel) return -1;
    bg = (uint16_t *)ps_malloc((size_t)S * S * 2);
    fb = (uint16_t *)ps_malloc((size_t)S * S * 2);
    pushBuf = (uint16_t *)ps_malloc((size_t)S * TILE * 2);
    if (!bg || !fb || !pushBuf) {
        freeBuffers();
        return -1;
    }

    buildBackground();
    shownStr[0] = prevStr[0] = 0;
    rollStart = 0;
    shoot.active = false;
    shoot.next = millis() + 2500;
    cat.nextEar = millis() + 2000;
    cat.nextLook = millis() + 7000;
    cat.lookStart = 0;

    // swallow whatever press brought us here
    delay(250);
    check(SelPress);
    check(EscPress);
    check(NextPress);
    check(PrevPress);
    check(UpPress);
    check(DownPress);

    bool aod = false;
    int aodMinute = -1;
    uint32_t aodSince = 0;
    bool aodLit = false;
    int result = 0;
    uint32_t last = millis();
    bool needReveal = true;

    while (!result) {
        if (isScreenOff) {
            // ---------------- low-power always-on display ----------------
            if (!aod) {
                aod = true;
                aodMinute = -1;
                aodSince = millis();
                aodLit = false;
                blackout();
                setCpuFrequencyMhz(80);
            }
            struct tm t = rtc.getTimeStruct();
            if (t.tm_min != aodMinute) {
                aodMinute = t.tm_min;
                drawAod();
            }
            // Bruce fades the panel to 0 right after flagging screen-off; light it dimly after that
            if (!aodLit && millis() - aodSince > 700) {
                aodLit = true;
                panelBrightness(26);
            }
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }
        if (aod) { // woke up: back to the animated face
            aod = false;
            setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
            needReveal = true;
            check(SelPress);
            check(EscPress);
            check(NextPress);
            check(PrevPress);
        }
        if (needReveal) {
            needReveal = false;
            revealFull();
            last = millis();
        }

        nowMs = millis();
        tSec = nowMs / 1000.0f;
        float dt = (nowMs - last) / 1000.0f;
        last = nowMs;
        composeAndPush(false, dt);

        if (check(SelPress)) result = 1;
        else if (check(EscPress)) result = 2;
        else if (check(NextPress) || check(PrevPress)) {
            bruceConfig.setClock24Hr(!bruceConfig.clock24hr); // digits roll to the new format
        }
#ifdef CATCLOCK_SHOT
        if (check(UpPress)) { // debug: `nav up` over serial dumps the composed frame
            memset(dset, 1, sizeof(dset));
            restoreDirty();
            memset(dirtyPrev, 1, sizeof(dirtyPrev));
            composeAndPush(true, 0);
            Serial.println();
            Serial.println("CATSHOT");
            Serial.write((const uint8_t *)fb, (size_t)S * S * 2);
            Serial.println();
            Serial.println("CATEND");
            Serial.flush();
        }
#endif
        check(UpPress);
        check(DownPress);

        uint32_t spent = millis() - nowMs;
#ifdef CATCLOCK_SHOT
        static uint32_t fpsT = 0, fpsN = 0, fpsSpent = 0;
        fpsN++;
        fpsSpent += spent;
        if (millis() - fpsT > 2000) {
            Serial.printf("CATFPS frames=%u in %ums, work %ums compose %uus push %uus tiles %u", (unsigned)fpsN, (unsigned)(millis() - fpsT), (unsigned)(fpsSpent / max(1u, (unsigned)fpsN)), (unsigned)(dbgCompose / max(1u, (unsigned)fpsN)), (unsigned)(dbgPush / max(1u, (unsigned)fpsN)), (unsigned)(dbgTiles / max(1u, (unsigned)fpsN)));
            dbgCompose = dbgPush = dbgTiles = 0;
            Serial.println();
            fpsT = millis();
            fpsN = fpsSpent = 0;
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(spent < 14 ? 16 - spent : 2)); // aim for ~60 fps
    }

    blackout(); // clear the whole round panel, not just Bruce's window
    freeBuffers();
    return result;
}
#endif // HAS_CAT_CLOCK
