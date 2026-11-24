#include "rf_freq_analyzer.h"
#include "rf_utils.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/settings.h"
#include "structs.h"
#include <algorithm>
#include <math.h>
#include <string.h>
#include <vector>
#ifndef TFT_MOSI
#define TFT_MOSI -1
#endif

// ---------------------------------------------------------------------------
// Frequency Analyzer - Flipper Zero look and behaviour, in Bruce's colours
//
// Faithful reproduction of the Flipper Zero Sub-GHz "Frequency Analyzer":
//
//   * one big frequency readout (000.000 MHz when idle),
//   * the last three frequencies that were detected,
//   * an RSSI meter,
//   * a second page (LEFT/RIGHT) with Flipper's log of detected frequencies.
//
// Flipper's panel is drawn black-on-white and appears BLACK-on-ORANGE on the
// hardware. As requested, every "black" element (title, digits, outlines, the
// bars) is drawn in Bruce's UI colour and the orange background becomes black.
// The inverted highlight Flipper uses while a signal is present (a filled box
// with knocked-out digits) becomes a Bruce-colour box with black digits.
//
// Behaviour matches Flipper as well: the coarse pass hops through the same
// standard sub-GHz frequency list, a fine 20 kHz pass pinpoints the winner, and
// a detected frequency is HELD on screen for a short moment after the signal
// stops (Flipper's sample_hold_counter) before the readout falls back to
// 000.000 - exactly the "press a car keyfob and read the number" behaviour.
// ---------------------------------------------------------------------------

#define FA_HOLD_LOOPS 20  // cycles the readout is held once the signal stops
#define FA_HOLD_HILITE 15 // ...but the highlight is dropped from here down
#define FA_NOISE_MARGIN 8 // dB over the running noise floor = a detection
// Flipper's SUBGHZ_FREQUENCY_ANALYZER_THRESHOLD is -93 dBm, and rf_scan() uses
// the same floor. -80 here was the reason a car keyfob pressed a couple of
// metres away never lit the analyzer: an ordinary OOK remote lands around
// -82..-90 dBm on this antenna, i.e. under the old absolute cut-off, while the
// noise floor (also -80..-90) stayed below it.
#define FA_MIN_ABS (-93)  // never treat anything weaker than this as a signal
#define FA_FINE_STEP 0.02f   // 20 kHz fine step (Flipper)
#define FA_FINE_SPAN 0.30f   // +-300 kHz fine window (Flipper)
#define FA_LOG_MAX 60        // Flipper: LOG_FREQUENCY_MAX_ITEMS
#define FA_RSSI_REF (-95)    // RSSI -> bar reference, like Flipper's threshold
#define FA_BIG_SIZE 5        // text size of the big number
#define FA_MAIN_ROWS 3       // history lines (Flipper)
#define FA_LOG_ROWS 5        // log rows visible at once

struct FaLogItem {
    float freq;
    uint8_t count;
    int rssiMax;
    uint32_t seq;
};

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

// Read the RSSI at one frequency, sharing the SPI bus with the TFT the same way
// the other RF screens do (the T-Embed gives the CC1101 priority).
static int fa_rssiAt(float f) {
    setMHZ(f);
    if (bruceConfigPins.CC1101_bus.mosi == TFT_MOSI) tft.drawPixel(0, 0, 0);
    // The CC1101 needs the receiver to settle after a frequency change before
    // its RSSI means anything - Flipper's analyzer waits 2 ms here. Reading too
    // early returned garbage-high values and produced fake "signals" at random
    // frequencies. Read twice and keep the second (settled) sample.
    delayMicroseconds(1500);
    rfGetRssi();
    delayMicroseconds(500);
    int rssi = rfGetRssi();
    if (bruceConfigPins.CC1101_bus.mosi == TFT_MOSI) tft.drawPixel(0, 0, 0);
    return rssi;
}

// Flipper's SNPRINTF_FREQUENCY: "%03ld.%03ld" MHz, e.g. 433.920 / 000.000.
static void fa_freqText(char *buf, size_t n, float mhz) {
    long khz = lroundf(mhz * 1000.0f);
    if (khz < 0) khz = 0;
    snprintf(buf, n, "%03ld.%03ld", khz / 1000, khz % 1000);
}

// RSSI (dBm) -> Flipper-style bar count.
static int fa_bar(int rssi) {
    int b = rssi - FA_RSSI_REF;
    return b < 0 ? 0 : b;
}

// Flipper's I_Internal_ant_1_9x11 antenna icon, 16x20 with lines.
static void fa_drawAntenna(int x, int y, uint16_t col) {
    tft.drawFastVLine(x + 8, y + 7, 12, col);
    tft.drawLine(x + 8, y + 7, x + 1, y + 1, col);
    tft.drawLine(x + 8, y + 7, x + 15, y + 1, col);
    tft.drawFastHLine(x + 1, y + 19, 15, col);
}

// Flipper's subghz_frequency_analyzer_draw_rssi: staggered columns.
static void fa_drawRssiMeter(int x, int baseY, int rssi) {
    int n = fa_bar(rssi);
    if (n <= 0) return;
    n = n / 3 + 2;
    if (n > 20) n = 20;
    int col = 0;
    for (int i = 1; i < n; i++) {
        if (i % 4) {
            col++;
            tft.fillRect(x + 5 * i, baseY - col, 4, col, bruceConfig.priColor);
        }
    }
}

// Flipper's subghz_frequency_analyzer_draw_log_rssi: a dotted horizontal gauge.
static void fa_drawLogRssi(int x, int y, int rssi) {
    int n = fa_bar(rssi);
    if (n <= 0) return;
    if (n > 54) n = 54;
    for (int i = 1; i < n; i++) {
        if (i % 5) tft.fillRect(x + 2 * i, y - 12, 2, 12, bruceConfig.priColor);
    }
}

static const char *fa_orderName(int order) {
    switch (order) {
        case 0: return "seq desc";
        case 1: return "seq asc";
        case 2: return "freq asc";
        default: return "count desc";
    }
}

static bool fa_orderLess(const FaLogItem &a, const FaLogItem &b, int order) {
    switch (order) {
        case 0: return a.seq > b.seq;
        case 1: return a.seq < b.seq;
        case 2: return a.freq < b.freq;
        default: return a.count > b.count;
    }
}

static void fa_sortLog(std::vector<FaLogItem> &log, int order) {
    // Insertion sort: the list is tiny (<= 60) and almost always ordered.
    for (size_t i = 1; i < log.size(); i++) {
        FaLogItem key = log[i];
        size_t j = i;
        while (j > 0 && fa_orderLess(key, log[j - 1], order)) {
            log[j] = log[j - 1];
            j--;
        }
        log[j] = key;
    }
}

// ---------------------------------------------------------------------------
// Layout (Flipper 128x64 scaled to the panel; all in Bruce colours on black)
// ---------------------------------------------------------------------------
#define FA_TITLE_Y 3
#define FA_BOX_X 6
#define FA_BOX_Y 40
#define FA_BOX_W (tftWidth - 12)
#define FA_BOX_H 58
#define FA_HIST_X (tftWidth / 2 + 10)
#define FA_HIST_Y 106
#define FA_HIST_STEP 22
#define FA_RSSI_Y 162

static void fa_drawMainStatic() {
    tft.setTextSize(2);
    tft.setTextColor(bruceConfig.priColor, TFT_BLACK);
    tft.setCursor(3, FA_TITLE_Y);
    tft.print("Frequency Analyzer");
    fa_drawAntenna(tftWidth - 22, 4, bruceConfig.priColor);
    tft.setCursor(3, FA_HIST_Y);
    tft.print("RSSI");
}

static void fa_drawMainDynamic(float shownFreq, int shownRssi, bool highlight, const float *history) {
    char buf[16];

    // ---- big readout -------------------------------------------------------
    fa_freqText(buf, sizeof(buf), shownFreq);
    if (highlight) {
        tft.fillRect(FA_BOX_X, FA_BOX_Y, FA_BOX_W, FA_BOX_H, bruceConfig.priColor);
        tft.setTextColor(TFT_BLACK, bruceConfig.priColor);
    } else {
        tft.fillRect(FA_BOX_X, FA_BOX_Y, FA_BOX_W, FA_BOX_H, TFT_BLACK);
        tft.setTextColor(bruceConfig.priColor, TFT_BLACK);
    }
    tft.setTextSize(FA_BIG_SIZE);
    tft.setCursor(FA_BOX_X + 16, FA_BOX_Y + 9);
    tft.print(buf);
    tft.setTextSize(2);
    tft.setCursor(FA_BOX_X + FA_BOX_W - 46, FA_BOX_Y + 21);
    tft.print("MHz");

    // ---- history (three lines, like Flipper) -------------------------------
    tft.setTextColor(bruceConfig.priColor, TFT_BLACK);
    tft.setTextSize(2);
    for (int i = 0; i < FA_MAIN_ROWS; i++) {
        int y = FA_HIST_Y + i * FA_HIST_STEP;
        tft.fillRect(FA_HIST_X, y, 96, 18, TFT_BLACK);
        if (history[i] > 0) {
            fa_freqText(buf, sizeof(buf), history[i]);
        } else {
            strcpy(buf, "---.---");
        }
        tft.setCursor(FA_HIST_X + 2, y);
        tft.print(buf);
        tft.fillRect(FA_HIST_X + 98, y, 44, 18, TFT_BLACK);
        tft.setCursor(FA_HIST_X + 100, y);
        tft.print("MHz");
    }

    // ---- RSSI meter --------------------------------------------------------
    tft.fillRect(44, FA_RSSI_Y - 46, 130, 50, TFT_BLACK);
    fa_drawRssiMeter(50, FA_RSSI_Y, shownRssi);
}

static void fa_drawLogStatic(int order) {
    tft.setTextSize(2);
    tft.setTextColor(bruceConfig.priColor, TFT_BLACK);
    tft.setCursor(3, FA_TITLE_Y);
    tft.print(String("Frequency Analyzer [") + fa_orderName(order) + "]");
}

static void fa_drawLogDynamic(const std::vector<FaLogItem> &log, int scroll) {
    char buf[16];
    const int y0 = 34;
    const int step = 26;
    tft.setTextSize(2);
    tft.setTextColor(bruceConfig.priColor, TFT_BLACK);

    if (log.empty()) {
        tft.fillRect(0, y0, tftWidth, step * FA_LOG_ROWS, TFT_BLACK);
        int bx = tftWidth / 2 - 84;
        tft.drawRoundRect(bx, y0 + 8, 168, 26, 6, bruceConfig.priColor);
        tft.setCursor(bx + 58, y0 + 14);
        tft.print("No records");
        return;
    }

    for (int i = 0; i < FA_LOG_ROWS; i++) {
        int y = y0 + i * step;
        tft.fillRect(0, y, tftWidth - 14, 20, TFT_BLACK);
        int idx = scroll + i;
        if (idx >= (int)log.size()) continue;
        const FaLogItem &it = log[idx];
        fa_freqText(buf, sizeof(buf), it.freq);
        tft.setCursor(6, y);
        tft.print(buf);
        tft.setCursor(FA_HIST_X - 40, y);
        tft.print(String(it.count));
        fa_drawLogRssi(FA_HIST_X - 40 + 70, y + 18, it.rssiMax);
    }

    // Scrollbar, like Flipper's elements_scrollbar_pos.
    tft.fillRect(tftWidth - 10, y0, 8, step * FA_LOG_ROWS, TFT_BLACK);
    if ((int)log.size() > FA_LOG_ROWS) {
        int trackH = step * FA_LOG_ROWS;
        int maxScroll = (int)log.size() - FA_LOG_ROWS;
        int barH = trackH * FA_LOG_ROWS / (int)log.size();
        if (barH < 10) barH = 10;
        int barY = y0 + (trackH - barH) * scroll / (maxScroll > 0 ? maxScroll : 1);
        tft.drawFastVLine(tftWidth - 6, y0, trackH, TFT_DARKGREY);
        tft.fillRect(tftWidth - 8, barY, 5, barH, bruceConfig.priColor);
    }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
void rf_freq_analyzer() {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Frequency Analyzer needs a CC1101!", true);
        return;
    }

    if (!initRfModule("rx", 433.92f)) {
        displayError("CC1101 not found!", true);
        return;
    }
    rfSetRxBW(200);

    const size_t freqCount = sizeof(subghz_frequency_list) / sizeof(float);

    std::vector<FaLogItem> log;
    float history[3] = {0, 0, 0};
    float shownFreq = 0;
    int shownRssi = 0;
    int hold = 0;
    bool locked = false;    // a signal is present right now
    bool highlight = false; // Flipper's inverted readout box
    int noiseFloor = -90;   // running estimate, updated from each sweep minima
    int fragment = 0;       // 0 = main (Flipper), 1 = log page
    int scroll = 0;
    int order = 0;
    uint32_t lastFreqKhz = 0;
    uint32_t seqCounter = 0;
    bool fullRedraw = true;

    while (true) {
        if (check(EscPress)) break;

        // ---- page switching / log controls ---------------------------------
        if (fragment == 0) {
            if (check(PrevPress) || check(NextPress) || check(DownPress)) {
                fragment = 1;
                scroll = 0;
                fullRedraw = true;
            }
        } else {
            if (check(SelPress)) {
                order = (order + 1) % 4;
                fa_sortLog(log, order);
                fullRedraw = true;
            }
            if (check(UpPress)) {
                if (scroll > 0) scroll--;
                fullRedraw = true;
            }
            if (check(DownPress)) {
                if (scroll + FA_LOG_ROWS < (int)log.size()) scroll++;
                fullRedraw = true;
            }
            if (check(PrevPress) || check(NextPress)) {
                fragment = 0;
                fullRedraw = true;
            }
        }
        if (fullRedraw) {
            tft.fillScreen(TFT_BLACK);
            fullRedraw = false;
        }

        // ---- pass 1: Flipper's preset list ---------------------------------
        std::vector<int> sweep(freqCount);
        float bestF = 0;
        int bestR = -127;
        for (size_t i = 0; i < freqCount && !EscPress; i++) {
            int rssi = fa_rssiAt(subghz_frequency_list[i]);
            sweep[i] = rssi;
            if (rssi > bestR) {
                bestR = rssi;
                bestF = subghz_frequency_list[i];
            }
        }
        // Median, not minimum: a single unsettled or anomalous reading cannot
        // drag the noise floor down and turn noise into fake signals.
        std::vector<int> sorted = sweep;
        std::sort(sorted.begin(), sorted.end());
        if (!sorted.empty()) noiseFloor = (noiseFloor + sorted[sorted.size() / 2]) / 2;
        int threshold = noiseFloor + FA_NOISE_MARGIN;
        if (threshold < FA_MIN_ABS) threshold = FA_MIN_ABS;

        bool signal = bestF > 0 && bestR > threshold;

        // ---- pass 2: Flipper's +-300 kHz fine scan at 20 kHz ---------------
        if (signal) {
            rfSetRxBW(58);
            int fineBest = -127;
            float fineF = bestF;
            int skip = 2; // the coarse scan ends far away; let RX settle first
            for (float f = bestF - FA_FINE_SPAN; f <= bestF + FA_FINE_SPAN && !EscPress; f += FA_FINE_STEP) {
                int rssi = fa_rssiAt(f);
                if (skip > 0) {
                    skip--;
                    continue;
                }
                if (rssi > fineBest) {
                    fineBest = rssi;
                    fineF = f;
                }
            }
            rfSetRxBW(200);
            if (fineBest > bestR) {
                bestR = fineBest;
                bestF = fineF;
            }
        }

        // ---- hold / history, mirroring Flipper's sample_hold_counter --------
        if (signal) {
            shownFreq = bestF;
            shownRssi = bestR;
            highlight = true;
            hold = FA_HOLD_LOOPS;
            locked = true;
        } else {
            highlight = false;
            // The number stays on screen, un-highlighted, until the hold runs
            // out; then it falls back to 000.000 (Flipper's sample_hold_counter).
            if (hold > 0) hold--;
            else {
                shownFreq = 0;
                shownRssi = 0;
            }
            if (locked) {
                // Signal ended: push it into the three-line history (Flipper).
                history[2] = history[1];
                history[1] = history[0];
                history[0] = shownFreq;
                locked = false;
            }
        }

        // ---- log update (Flipper's log_frequency_update) -------------------
        if (signal) {
            uint32_t khz = (uint32_t)lroundf(bestF * 1000.0f);
            bool needInsert = khz != lastFreqKhz;
            int found = -1;
            for (size_t i = 0; i < log.size(); i++) {
                if ((uint32_t)lroundf(log[i].freq * 1000.0f) == khz) {
                    found = (int)i;
                    break;
                }
            }
            if (found >= 0) {
                if (bestR > log[found].rssiMax) log[found].rssiMax = bestR;
                if (needInsert && log[found].count < 255) {
                    log[found].count++;
                    fa_sortLog(log, order);
                }
            } else if (needInsert && log.size() < FA_LOG_MAX) {
                log.push_back({bestF, 1, bestR, seqCounter++});
                fa_sortLog(log, order);
            }
            lastFreqKhz = khz;
        }

        // ---- paint ---------------------------------------------------------
        // Static parts are redrawn every loop on purpose: the firmware only
        // re-sends draws that are new since the log ring was last cleared, so a
        // viewer that connects later would otherwise never see the title. The
        // TFT logger dedupes identical draws, so redrawing them costs nothing
        // on the USB stream.
        if (fragment == 0) {
            fa_drawMainStatic();
            fa_drawMainDynamic(shownFreq, shownRssi, highlight, history);
        } else {
            fa_drawLogStatic(order);
            fa_drawLogDynamic(log, scroll);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    returnToMenu = true;
    deinitRfModule();
}
