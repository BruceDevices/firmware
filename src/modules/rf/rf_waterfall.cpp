#include "rf_waterfall.h"
#include "core/spectrum_plot.h"
#ifndef TFT_MOSI
#define TFT_MOSI -1
#endif
float m_rf_waterfall_start_freq = 433.0;
float m_rf_waterfall_end_freq = 435.0;

void rf_waterfall() {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Waterfall needs a CC1101!", true);
        return;
    }
    if (!initRfModule("rx", m_rf_waterfall_start_freq)) {
        displayError("CC1101 not found!", true);
        return;
    }

    ELECHOUSE_cc1101.setRxBW(200);
    int option, idx = 0;
select:

    option = 0;
    options = {
        {"Start Waterfall", [&]() { option = 3; }},
        {"Start Freq.",     [&]() { option = 1; }},
        {"End Freq.",       [&]() { option = 2; }},
        {"Main Menu",       [&]() { option = 4; }},
    };
    idx = loopOptions(options, idx);

    tft.fillScreen(0x0);

    if (option == 4) {
        return;
    } else if (option == 3) {
        rf_waterfall_run();
        goto select;
    } else if (option == 1) {
        rf_waterfall_boundary_freq(m_rf_waterfall_start_freq);
        goto select;
    } else if (option == 2) {
        rf_waterfall_boundary_freq(m_rf_waterfall_end_freq);
        goto select;
    }
}
void rf_waterfall_boundary_freq(float &boundary) {
    options = {};
    int ind = 0;
    int arraySize = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);
    for (int i = 0; i < arraySize; i++) {
        if (subghz_frequency_list[i] - boundary < 0.1) ind = i;
        String tmp = String(subghz_frequency_list[i], 2) + "Mhz";
        options.push_back({tmp.c_str(), [&boundary, i]() { boundary = subghz_frequency_list[i]; }});
    }
    loopOptions(options, ind);
    options.clear();
}


// ── SDR-style waterfall ─────────────────────────────────────────
// Rebuilt on the shared SpectrumPlot so it matches NRF Spectrum: a live filled
// trace with peak-hold across the top, and a theme-coloured waterfall below fed
// by the very same envelope, so the falls track the waveform above them.
//
// The band is swept in a modest number of bins (not one per pixel): retuning a
// CC1101 needs a few ms for the PLL/RSSI to settle before getRssi() is valid
// (see rf_CC1101_rssi), so sampling every pixel with a sub-ms wait would just
// read the noise floor. We sample WF_BINS points with a real settle and then
// interpolate the envelope across the plot columns for a continuous trace.
#define WF_BINS 64
#define WF_SETTLE_MS 3

void rf_waterfall_run() {
    SpectrumPlot plot;
    if (!plot.begin("RF Waterfall", /*sdrWaterfall=*/true)) { // SDR colourmap below
        displayError("Out of memory", true);
        return;
    }

    const int plotW = plot.width();
    uint8_t *env = (uint8_t *)malloc(plotW);     // freshly measured envelope
    uint8_t *disp = (uint8_t *)malloc(plotW);    // eased envelope drawn on top
    uint8_t *envPeak = (uint8_t *)malloc(plotW); // peak-hold line
    if (!env || !disp || !envPeak) {
        free(env);
        free(disp);
        free(envPeak);
        plot.end();
        displayError("Out of memory", true);
        return;
    }
    memset(env, 0, plotW);
    memset(disp, 0, plotW);
    memset(envPeak, 0, plotW);

    float f_start = m_rf_waterfall_start_freq;
    float f_end = m_rf_waterfall_end_freq;
    if (f_end < f_start) {
        float t = f_start;
        f_start = f_end;
        f_end = t;
    }

    initRfModule("rx", f_start);
    ELECHOUSE_cc1101.setRxBW(200);

    // Five evenly spaced frequency ticks under the plot; redrawn when panning.
    const int tickCount = 5;
    int cols[tickCount];
    String labels[tickCount];
    auto drawRuler = [&]() {
        for (int i = 0; i < tickCount; i++) {
            cols[i] = i * (plotW - 1) / (tickCount - 1);
            float f = f_start + (f_end - f_start) * i / (tickCount - 1);
            labels[i] = String(f, 2);
        }
        plot.ruler(cols, labels, tickCount);
    };
    drawRuler();
    plot.status("scanning...");

    // Pan step scales with the span, matching the old boundary stepping.
    float range = f_end - f_start;
    float step;
    if (range > 100) step = 10;
    else if (range > 10) step = 1;
    else if (range > 1) step = 0.1f;
    else if (range > 0.1f) step = 0.01f;
    else step = 0.001f;

    const bool sharedBus = (bruceConfigPins.CC1101_bus.mosi == TFT_MOSI);
    uint8_t bins[WF_BINS];

    uint32_t lastStatus = 0;
    while (!check(EscPress)) {
        // Sweep the band once — WF_BINS RSSI samples with a real settle so the
        // reading reflects the tuned frequency instead of the noise floor.
        int maxBin = 0;
        int maxRssi = -128;
        for (int b = 0; b < WF_BINS; b++) {
            float f = f_start + (f_end - f_start) * b / (WF_BINS - 1);
            setMHZ(f);
            vTaskDelay(pdMS_TO_TICKS(WF_SETTLE_MS)); // let the PLL/RSSI settle
            int rssi = ELECHOUSE_cc1101.getRssi();
            if (sharedBus) tft.drawPixel(0, 0, 0); // keep CC1101/TFT shared SPI happy

            int v = map(rssi, -100, -30, 0, 100);
            v = constrain(v, 0, 100);
            bins[b] = (uint8_t)v;
            if (rssi > maxRssi) {
                maxRssi = rssi;
                maxBin = b;
            }
            if (check(EscPress)) break;
        }

        // Interpolate the bins across the plot columns for a continuous trace,
        // and peak-hold with slow decay so brief bursts stay visible (as in NRF).
        int maxCol = maxBin * (plotW - 1) / (WF_BINS - 1);
        for (int i = 0; i < plotW; i++) {
            int32_t pos = (int32_t)i * (WF_BINS - 1) * 256 / (plotW - 1);
            int bi = pos >> 8;
            int frac = pos & 0xff;
            if (bi >= WF_BINS - 1) {
                bi = WF_BINS - 2;
                frac = 256;
            }
            int v = bins[bi] + (bins[bi + 1] - bins[bi]) * frac / 256;
            env[i] = (uint8_t)(v < 0 ? 0 : (v > 100 ? 100 : v));
            if (env[i] > envPeak[i]) envPeak[i] = env[i];
            else if (envPeak[i]) envPeak[i]--;

            // Ease the drawn trace toward the measurement so the top glides
            // instead of snapping, matching Jam Detect's animated sweep.
            int d = (int)env[i] - (int)disp[i];
            if (d) disp[i] = (uint8_t)((int)disp[i] + (d > 0 ? max(1, d / 3) : min(-1, d / 3)));
        }

        int hlSpan = plotW / 40;
        plot.trace(disp, envPeak, maxCol - hlSpan, maxCol + hlSpan); // eased trace on top
        plot.pushRow(env); // waterfall shows the true measurement

        if (millis() - lastStatus >= 350) {
            lastStatus = millis();
            float peakFreq = f_start + (f_end - f_start) * maxBin / (WF_BINS - 1);
            plot.status(String(maxRssi) + "dBm @" + String(peakFreq, 3) + "MHz  UP/DN pan");
        }

        // Pan the whole window and refresh the ruler + peak history.
        if (check(UpPress) || check(NextPress)) {
            f_start += step;
            f_end += step;
            memset(envPeak, 0, plotW);
            drawRuler();
            delay(80);
        } else if (check(DownPress) || check(PrevPress)) {
            f_start -= step;
            f_end -= step;
            memset(envPeak, 0, plotW);
            drawRuler();
            delay(80);
        }
    }

    free(env);
    free(disp);
    free(envPeak);
    plot.end();
    returnToMenu = true;
    deinitRfModule();
    delay(10);
}
