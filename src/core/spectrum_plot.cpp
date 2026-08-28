#include "spectrum_plot.h"

#include "core/display.h"
#include <globals.h>

// Waterfall intensity ramp, quantised so identical columns collapse into runs.
static const int SP_HEAT_N = 16;
static uint16_t sp_heat[SP_HEAT_N];

static uint32_t sp_rnd_state = 0x2a3b4c5d;

static inline uint32_t sp_rnd() {
    sp_rnd_state ^= sp_rnd_state << 13;
    sp_rnd_state ^= sp_rnd_state >> 17;
    sp_rnd_state ^= sp_rnd_state << 5;
    return sp_rnd_state;
}

uint16_t SpectrumPlot::alertColor() {
    uint16_t pri = bruceConfig.priColor;
    int r = (pri >> 11) & 0x1f, g = (pri >> 5) & 0x3f, b = pri & 0x1f;
    // A red alert would vanish on a red theme, so fall back to amber there.
    bool reddish = (r > 18 && (g >> 1) < 12 && b < 12);
    return reddish ? TFT_ORANGE : TFT_RED;
}

void SpectrumPlot::buildGeometry() {
    _plotL = 8;
    _plotW = tftWidth - 16;
    if (_plotW < 32) {
        _plotL = 2;
        _plotW = tftWidth - 4;
    }

    int top = BORDER_PAD_Y + 8 * FM + 2; // just below the title
    _footY = tftHeight - 8 * FP - 8;
    _lblY = _footY - 8 * FP - 2;

    int avail = _lblY - top - 2;
    if (avail < 20) { // no room for the ruler
        _lblY = -1;
        avail = _footY - top - 2;
    }
    if (avail < 14) { // no room for the status line either
        _footY = -1;
        avail = tftHeight - 6 - top;
    }
    if (avail < 8) avail = 8;

    _wfRows = 0;
    if (avail >= 36) {
        _wfRows = avail / 3;
        if (_wfRows > 24) _wfRows = 24;
    }
    _specTop = top;
    _specH = avail - _wfRows - (_wfRows ? 2 : 0);
    _specBot = _specTop + _specH - 1;
    _wfTop = _specBot + 3;
}

void SpectrumPlot::buildPalette() {
    uint16_t pri = bruceConfig.priColor;
    _bg = bruceConfig.bgColor;
    _trace = pri;
    _body = blendColors(_bg, pri, 95);
    _bodyHl = blendColors(_bg, pri, 165);
    _peak = blendColors(pri, TFT_WHITE, 150);
    _grid = blendColors(_bg, pri, 55);
    _label = blendColors(_bg, pri, 170);
    buildHeatPalette(sp_heat, SP_HEAT_N);
}

bool SpectrumPlot::begin(const String &title) {
    buildGeometry();
    buildPalette();

    if (_plotW < 8) return false;

    if (_wfRows) {
        _wf = (uint8_t *)calloc((size_t)_wfRows * _plotW, 1);
        if (!_wf) _wfRows = 0; // degrade to a plot without history rather than fail
    }
    _wfHead = 0;
    _ok = true;

    drawMainBorderWithTitle(title); // clears the screen itself

    drawWaterfall();
    return true;
}

void SpectrumPlot::end() {
    free(_wf);
    _wf = nullptr;
    _wfRows = 0;
    _ok = false;
}

void SpectrumPlot::trace(const uint8_t *env, const uint8_t *envPeak, int hlL, int hlR, bool alert) {
    if (!_ok || !env) return;

    uint16_t trace = alert ? alertColor() : _trace;
    uint16_t bodyHl = alert ? blendColors(_bg, trace, 165) : _bodyHl;

    int gy[3];
    gy[0] = _specBot - _specH / 4;
    gy[1] = _specBot - _specH / 2;
    gy[2] = _specBot - (_specH * 3) / 4;

    // Every pixel of the band is written exactly once per frame, which keeps the
    // animation flicker free without needing a full-screen sprite.
    for (int i = 0; i < _plotW; i++) {
        int x = _plotL + i;

        int hLive = (int)env[i] * (_specH - 1) / 100;
        int grass = (int)(sp_rnd() % 3); // animated noise floor
        if (hLive < grass) hLive = grass;
        int hPeak = envPeak ? (int)envPeak[i] * (_specH - 1) / 100 : 0;
        if (hPeak < hLive) hPeak = hLive;

        int yLive = _specBot - hLive;
        int yPeak = _specBot - hPeak;

        if (yPeak > _specTop) tft.drawFastVLine(x, _specTop, yPeak - _specTop, _bg);
        if (hPeak > hLive) {
            tft.drawPixel(x, yPeak, _peak);
            if (yLive > yPeak + 1) tft.drawFastVLine(x, yPeak + 1, yLive - yPeak - 1, _bg);
        }
        tft.drawPixel(x, yLive, trace);
        if (hLive > 0)
            tft.drawFastVLine(x, yLive + 1, hLive, (i >= hlL && i <= hlR) ? bodyHl : _body);

        // dashed reference grid, visible only through the empty sky
        if ((i & 3) == 0) {
            for (int k = 0; k < 3; k++)
                if (gy[k] > _specTop && gy[k] < yLive - 1) tft.drawPixel(x, gy[k], _grid);
        }
    }
}

// Newest row sits right under the trace baseline and older ones fall away.
void SpectrumPlot::drawWaterfall() {
    if (!_wfRows || !_wf) return;

    for (int r = 0; r < _wfRows; r++) {
        int idx = (_wfHead - r + 2 * _wfRows) % _wfRows;
        const uint8_t *row = _wf + (size_t)idx * _plotW;
        int y = _wfTop + r;

        // flush equal-coloured columns as single spans, the rows are wide
        int runStart = 0;
        uint16_t runCol = sp_heat[row[0] * (SP_HEAT_N - 1) / 100];
        for (int i = 1; i <= _plotW; i++) {
            bool last = (i == _plotW);
            uint16_t c = last ? runCol : sp_heat[row[i] * (SP_HEAT_N - 1) / 100];
            if (last || c != runCol) {
                tft.drawFastHLine(_plotL + runStart, y, i - runStart, runCol);
                runStart = i;
                runCol = c;
            }
        }
    }
}

void SpectrumPlot::pushRow(const uint8_t *env) {
    if (!_ok || !_wfRows || !_wf || !env) return;
    _wfHead = (_wfHead + 1) % _wfRows;
    memcpy(_wf + (size_t)_wfHead * _plotW, env, _plotW);
    drawWaterfall();
}

void SpectrumPlot::ruler(const int *cols, const String *labels, int count, int highlight) {
    if (!_ok || _lblY < 0 || !cols || !labels) return;

    int h = 8 * FP;
    tft.fillRect(_plotL, _lblY - 1, _plotW, h + 2, _bg);
    tft.setTextSize(FP);

    for (int i = 0; i < count; i++) {
        int w = labels[i].length() * FP * LW;
        int tx = _plotL + cols[i] - w / 2;
        // keep edge labels inside the plot
        if (tx < _plotL) tx = _plotL;
        if (tx + w > _plotL + _plotW) tx = _plotL + _plotW - w;

        if (i == highlight) {
            tft.fillRect(tx - 2, _lblY - 1, w + 4, h + 2, bruceConfig.priColor);
            tft.setTextColor(_bg, bruceConfig.priColor);
        } else {
            tft.setTextColor(_label, _bg);
        }
        tft.drawString(labels[i], tx, _lblY, 1);
    }
}

void SpectrumPlot::status(const String &text, bool alert) {
    if (!_ok || _footY < 0) return;

    tft.fillRect(_plotL, _footY, _plotW, 8 * FP, _bg);
    tft.setTextSize(FP);
    tft.setTextColor(alert ? alertColor() : _label, _bg);

    String s = text;
    int maxChars = _plotW / (FP * LW);
    if ((int)s.length() > maxChars) s = s.substring(0, maxChars);
    tft.drawString(s, _plotL, _footY, 1);
}
