#pragma once

#if !defined(LITE_VERSION)

#include "core/spectrum_plot.h"
#include <Arduino.h>

// 2.4GHz WiFi channel view, built on SpectrumPlot.
//
// Turns per-channel levels into overlapping spectral lobes on a real frequency
// axis, so the 22MHz overlap between neighbours is visible and a sweep reads as
// one continuous trace instead of eleven isolated bars.
//
// Callers own the radio and feed normalised 0-100 levels indexed by channel
// number; everything on screen belongs to the view.
class WifiSpectrumView {
public:
    static const int CHANNELS = 11;
    static const int CH_MAX = 12; // arrays are indexed by channel number

    // Allocates the column buffers and paints the frame. Returns false when
    // there is not enough memory, in which case nothing was drawn.
    bool begin(const String &title);
    void end();
    bool ready() const { return _env != nullptr; }

    // One animation frame. The drawn levels ease toward `level` so the sweep
    // glides instead of snapping when a measurement lands. `alert` recolours
    // the trace to flag an abnormal reading.
    void animate(const uint8_t *level, const uint8_t *peak, uint8_t curCh, bool alert = false);

    // Call once per completed measurement: pushes a waterfall row and repaints
    // the channel ruler with `curCh` highlighted.
    void commit(const uint8_t *level, uint8_t curCh);

    void status(const String &text, bool alert = false) { _plot.status(text, alert); }

private:
    void envelope(const uint8_t *v, uint8_t *env) const;

    SpectrumPlot _plot;
    uint8_t *_env = nullptr;
    uint8_t *_envPeak = nullptr;
    uint8_t _disp[CH_MAX] = {0};
};

#endif
