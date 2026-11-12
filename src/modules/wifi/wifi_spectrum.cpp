#if !defined(LITE_VERSION)
#include "wifi_spectrum.h"

#include "core/display.h"
#include <globals.h>
#include <math.h>

// Frequency axis in 0.1MHz units: ch1 = 2412.0MHz, 5MHz spacing, plus a small
// margin on each side so the outer lobes are not cut too abruptly.
static const int WS_FCH1 = 24120;
static const int WS_FSTEP = 50;
static const int WS_FMARGIN = 70;
static const int WS_FMIN = WS_FCH1 - WS_FMARGIN;
static const int WS_FSPAN = (WifiSpectrumView::CHANNELS - 1) * WS_FSTEP + 2 * WS_FMARGIN;
static const int WS_FLOBE = 220; // lobe reach: +/-22MHz

// Lobe shape sampled over 0..22MHz from the carrier, built once per process.
static const int WS_SHAPE_N = 48;
static uint8_t ws_shape[WS_SHAPE_N];
static bool ws_shape_ready = false;

static void ws_build_shape() {
    if (ws_shape_ready) return;
    for (int i = 0; i < WS_SHAPE_N; i++) {
        float d = 22.0f * i / (WS_SHAPE_N - 1); // MHz from the carrier
        float a;
        if (d <= 11.0f) a = 0.5f * (1.0f + cosf(PI * d / 11.0f)); // main lobe
        else a = 0.05f * (1.0f - cosf(2.0f * PI * (d - 11.0f) / 11.0f)); // side lobe
        ws_shape[i] = (uint8_t)(a * 255.0f + 0.5f);
    }
    ws_shape_ready = true;
}

static inline int ws_freq(uint8_t ch) { return WS_FCH1 + (ch - 1) * WS_FSTEP; }

// Column index for a frequency, in plot coordinates.
static inline int ws_col(int fq, int plotW) {
    return (int)((int32_t)(fq - WS_FMIN) * (plotW - 1) / WS_FSPAN);
}

bool WifiSpectrumView::begin(const String &title) {
    ws_build_shape();
    if (!_plot.begin(title)) return false;

    _env = (uint8_t *)malloc(_plot.width());
    _envPeak = (uint8_t *)malloc(_plot.width());
    if (!_env || !_envPeak) {
        end();
        return false;
    }
    memset(_disp, 0, sizeof(_disp));
    memset(_env, 0, _plot.width());
    return true;
}

void WifiSpectrumView::end() {
    free(_env);
    free(_envPeak);
    _env = _envPeak = nullptr;
    _plot.end();
}

// Envelope of the per-channel values across the plot, taking the strongest
// contributor at each column so overlapping lobes read as one skyline.
void WifiSpectrumView::envelope(const uint8_t *v, uint8_t *env) const {
    int plotW = _plot.width();
    for (int i = 0; i < plotW; i++) {
        int fq = WS_FMIN + (int)((int32_t)i * WS_FSPAN / (plotW - 1));
        uint8_t best = 0;
        for (int ch = 1; ch <= CHANNELS; ch++) {
            if (!v[ch]) continue;
            int d = fq - ws_freq(ch);
            if (d < 0) d = -d;
            if (d >= WS_FLOBE) continue;
            uint8_t a = (uint8_t)((uint16_t)v[ch] * ws_shape[d * (WS_SHAPE_N - 1) / WS_FLOBE] / 255);
            if (a > best) best = a;
        }
        env[i] = best;
    }
}

void WifiSpectrumView::animate(const uint8_t *level, const uint8_t *peak, uint8_t curCh, bool alert) {
    if (!ready()) return;

    for (int ch = 1; ch <= CHANNELS; ch++) {
        int d = (int)level[ch] - (int)_disp[ch];
        if (d) _disp[ch] = (uint8_t)((int)_disp[ch] + (d > 0 ? max(1, d / 3) : min(-1, d / 3)));
    }
    envelope(_disp, _env);
    envelope(peak, _envPeak);

    // highlight the 22MHz slice the radio is parked on
    int plotW = _plot.width();
    _plot.trace(_env, _envPeak, ws_col(ws_freq(curCh) - 110, plotW), ws_col(ws_freq(curCh) + 110, plotW), alert);
}

void WifiSpectrumView::commit(const uint8_t *level, uint8_t curCh) {
    if (!ready()) return;

    envelope(level, _env);
    _plot.pushRow(_env);

    int plotW = _plot.width();
    int cols[CHANNELS];
    String labels[CHANNELS];
    int n = 0, highlight = -1;
    // drop even channels when the ruler would collide, but never the current one
    bool sparse = (plotW * WS_FSTEP) / WS_FSPAN < (2 * FP * LW + 4);
    for (int ch = 1; ch <= CHANNELS; ch++) {
        if (sparse && !(ch & 1) && ch != curCh) continue;
        if (ch == curCh) highlight = n;
        cols[n] = ws_col(ws_freq(ch), plotW);
        labels[n] = String(ch);
        n++;
    }
    _plot.ruler(cols, labels, n, highlight);
}

#endif
