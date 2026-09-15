#pragma once

#include <Arduino.h>

// Shared spectrum-analyzer plot.
//
// Owns the layout, palette and painting for every "signal strength across a
// frequency band" screen in Bruce, so Channel Analyzer, Jam Detect and the NRF
// sweeper share one look instead of each inventing its own bars and colours.
//
// The band itself is caller supplied: modules fill an envelope of width()
// values in 0-100 and the plot turns it into a filled trace with a peak-hold
// line, an animated noise floor, a dashed reference grid and a scrolling
// waterfall. Every colour is derived from the active theme.
//
// Typical use:
//     SpectrumPlot plot;
//     if (!plot.begin("My Scanner")) return;      // frees itself on failure
//     ...
//     plot.trace(env, envPeak, hlLeft, hlRight);  // once per animation frame
//     plot.pushRow(env);                          // once per completed sweep
//     plot.ruler(cols, labels, n, current);
//     plot.status("...");
//     plot.end();
class SpectrumPlot {
public:
    bool begin(const String &title);
    void end();
    bool ready() const { return _ok; }

    // Number of columns the caller must fill, and where they land on screen.
    int width() const { return _plotW; }
    int left() const { return _plotL; }

    // Paints one frame of the live band. `env` and `envPeak` hold width()
    // values in 0-100; envPeak may be null. Columns in [hlL, hlR] are filled
    // with the highlight shade to mark the slice being measured — pass
    // hlL > hlR for none. `alert` recolours the trace to flag a bad reading.
    void trace(const uint8_t *env, const uint8_t *envPeak, int hlL, int hlR, bool alert = false);

    // Appends `env` to the waterfall history and repaints it. No-op when the
    // screen is too short for a waterfall.
    void pushRow(const uint8_t *env);

    // Labelled ticks under the plot. `cols` are column indices in 0..width()-1;
    // `highlight` indexes the entry to draw inverted, or -1 for none.
    void ruler(const int *cols, const String *labels, int count, int highlight = -1);

    // Single line of text at the bottom, truncated to fit.
    void status(const String &text, bool alert = false);

    // Colour for out-of-range readings, kept visible even on a reddish theme.
    static uint16_t alertColor();

private:
    void buildGeometry();
    void buildPalette();
    void drawWaterfall();

    bool _ok = false;

    int _plotL = 0, _plotW = 0;
    int _specTop = 0, _specBot = 0, _specH = 0;
    int _wfTop = 0, _wfRows = 0;
    int _lblY = -1;  // -1 when the screen is too short for the ruler
    int _footY = -1; // -1 when the screen is too short for the status line

    uint16_t _bg = 0, _body = 0, _bodyHl = 0, _trace = 0, _peak = 0, _grid = 0, _label = 0;

    uint8_t *_wf = nullptr; // _wfRows x _plotW ring of rendered envelopes
    int _wfHead = 0;
};
