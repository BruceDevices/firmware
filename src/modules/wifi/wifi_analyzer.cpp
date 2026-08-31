/**
 * @file wifi_analyzer.cpp
 * @brief 2.4 GHz WiFi Channel Analyzer for Bruce firmware
 *
 * Displays a real-time 14-channel RSSI bar chart showing network congestion
 * across all 2.4 GHz WiFi channels. Rescans automatically every 3 seconds.
 *
 * Features:
 *  - Per-channel RSSI bar graph (channels 1–14, 320×240 landscape layout)
 *  - Color-coded congestion: green (quiet) → yellow → orange → red (congested)
 *  - Best channel highlighted with a cyan outline (least congested)
 *  - Flicker-free updates: only redraws bars whose RSSI or highlight changed
 *  - Tap any bar to see a list of APs on that channel (SSID + RSSI, sorted)
 *  - Prev/ESC returns from the AP list back to the channel graph
 *
 * Screen layout (320×240, STATUS_BAR_HEIGHT=30):
 *   y=30–51  : title "WiFi Analyzer"
 *   y=52–200 : bar graph (GRAPH_H=148px)
 *   y=203    : channel number labels (1–14, static)
 *   y=212–239: "Best: Ch X" recommendation + footer
 */

#include "wifi_analyzer.h"
#include "core/display.h"
#include "core/utils.h"
#include <globals.h>
#include <interface.h>
#include <WiFi.h>
#include <algorithm>
#include <vector>

// ── Layout constants ──────────────────────────────────────────────────────────
static const int CH_COUNT   = 14;   // 2.4 GHz channels 1–14
static const int GRAPH_TOP  = 52;   // bar graph top-y (below title)
static const int GRAPH_BOT  = 200;  // bar graph bottom-y
static const int GRAPH_H    = GRAPH_BOT - GRAPH_TOP; // 148px
static const int CH_W       = 22;   // slot width per channel (5 + 14*22 = 313 ≤ 320)
static const int BAR_W      = 18;   // bar pixel width (2px padding each side)
static const int LABEL_Y    = 203;  // channel number label top-y (below graph)
static const int REC_Y      = 222;  // "Best: Ch X" text center-y
static const int RSSI_FLOOR = -100; // dBm — treated as "no signal" (bar height 0)
static const int RSSI_CEIL  = -40;  // dBm — treated as maximum (bar height = GRAPH_H)
static const int SCAN_WAIT  = 3000; // ms to wait between scans

// ── AP record ─────────────────────────────────────────────────────────────────
struct APInfo {
    String ssid;
    int    rssi;
};

// ── Helpers ───────────────────────────────────────────────────────────────────

// Map RSSI to a traffic-light color.
static uint16_t _rssiColor(int rssi) {
    if (rssi >= -60) return TFT_RED;    // congested
    if (rssi >= -75) return TFT_ORANGE;
    if (rssi >= -90) return TFT_YELLOW;
    return TFT_GREEN;                   // quiet / empty
}

// Return the channel (1–14) tapped by the user, or 0 if outside the bar area.
static int _hitTestChannel(uint16_t tx, uint16_t ty) {
    if (ty < GRAPH_TOP || ty > LABEL_Y + 12) return 0;
    if (tx < 5) return 0;
    int ch = (int)(tx - 5) / CH_W; // 0-indexed
    if (ch < 0 || ch >= CH_COUNT)  return 0;
    return ch + 1;
}

// ── Main graph draw ───────────────────────────────────────────────────────────

/**
 * Draw or incrementally update the channel analyzer graph.
 *
 * On initialDraw=true : clears the screen area, draws the title, and draws all
 *                       14 channel labels (they are static and never change).
 * On initialDraw=false: only redraws bars whose RSSI changed or whose
 *                       "recommended" highlight status changed. This eliminates
 *                       the full-screen flicker of a naive clear-and-redraw.
 */
static void _drawAnalyzer(int16_t *chanRSSI, int recCh, bool initialDraw) {
    static int16_t prevRSSI[CH_COUNT];
    static int     prevRecCh = -1;

    tft.setTextFont(1);
    tft.setTextDatum(TL_DATUM);

    if (initialDraw) {
        for (int i = 0; i < CH_COUNT; i++) prevRSSI[i] = RSSI_FLOOR - 1;
        prevRecCh = -1;
        // Title via Bruce helper (clears content area automatically)
        drawMainBorderWithTitle("WiFi Analyzer", true);
        drawStatusBar();
        // Channel labels are static — paint once, never clear
        tft.setTextSize(1);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setTextDatum(TC_DATUM);
        for (int ch = 0; ch < CH_COUNT; ch++) {
            tft.drawString(String(ch + 1), 5 + ch * CH_W + 2 + BAR_W / 2, LABEL_Y);
        }
        tft.setTextDatum(TL_DATUM);
    }

    bool recChChanged = (recCh != prevRecCh);

    // Incremental bar update — skip unchanged channels
    for (int ch = 0; ch < CH_COUNT; ch++) {
        bool rssiChanged    = (chanRSSI[ch] != prevRSSI[ch]);
        bool thisRecChanged = recChChanged &&
                              ((ch + 1 == recCh) || (ch + 1 == prevRecCh));

        if (!rssiChanged && !thisRecChanged && !initialDraw) continue;

        int bx  = 5 + ch * CH_W + 2;
        int sig = chanRSSI[ch];
        if (sig < RSSI_FLOOR) sig = RSSI_FLOOR;
        if (sig > RSSI_CEIL)  sig = RSSI_CEIL;

        int barH = (int)((long)(sig - RSSI_FLOOR) * GRAPH_H / (RSSI_CEIL - RSSI_FLOOR));

        // Clear slot then redraw bar
        tft.fillRect(bx, GRAPH_TOP, BAR_W, GRAPH_H, bruceConfig.bgColor);
        if (barH > 0)
            tft.fillRect(bx, GRAPH_BOT - barH, BAR_W, barH, _rssiColor(sig));

        // Cyan outline marks the recommended (least congested) channel
        if (ch + 1 == recCh)
            tft.drawRect(bx - 1, GRAPH_TOP, BAR_W + 2, GRAPH_H, TFT_CYAN);

        prevRSSI[ch] = chanRSSI[ch];
    }
    prevRecCh = recCh;

    // Recommendation line — only repaint when best channel changes
    if (recChChanged || initialDraw) {
        tft.fillRect(0, REC_Y - 10, tftWidth, tftHeight - (REC_Y - 10), bruceConfig.bgColor);
        tft.setTextSize(FP);
        tft.setTextDatum(MC_DATUM);
        if (recCh > 0) {
            tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
            tft.drawString("Best: Ch " + String(recCh), tftWidth / 2, REC_Y);
        } else {
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawString("No APs found", tftWidth / 2, REC_Y);
        }
        tft.setTextDatum(TL_DATUM);
        tft.setTextFont(1);
    }
}

// ── Channel detail screen ─────────────────────────────────────────────────────

/**
 * Show all APs detected on a given channel, sorted by signal strength.
 * Exits when the user presses Prev or ESC, returning to the channel graph.
 */
static void _showChannelAPs(int ch, std::vector<APInfo> *chanAPs) {
    std::vector<APInfo> &aps = chanAPs[ch - 1];

    // Sort strongest first
    std::sort(aps.begin(), aps.end(),
              [](const APInfo &a, const APInfo &b) { return a.rssi > b.rssi; });

    String title = "Ch " + String(ch) + " — " +
                   String(aps.size()) + " AP" + (aps.size() != 1 ? "s" : "");
    drawMainBorderWithTitle(title, true);
    drawStatusBar();

    const int lineH  = 18;
    const int startY = STATUS_BAR_HEIGHT + 28;
    const int maxRows = (tftHeight - startY - 20) / lineH;

    tft.setTextFont(1);

    if (aps.empty()) {
        tft.setTextSize(FP);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawString("No APs on this channel", tftWidth / 2, tftHeight / 2);
        tft.setTextDatum(TL_DATUM);
    } else {
        int rows = std::min((int)aps.size(), maxRows);
        for (int i = 0; i < rows; i++) {
            int y = startY + i * lineH;

            // RSSI badge (color-coded, right-aligned to col 34)
            tft.setTextSize(1);
            tft.setTextColor(_rssiColor(aps[i].rssi), bruceConfig.bgColor);
            tft.setTextDatum(TL_DATUM);
            char badge[8];
            snprintf(badge, sizeof(badge), "%4d", aps[i].rssi);
            tft.drawString(badge, 4, y + 2);

            // SSID (hidden networks shown as "[hidden]", truncated if too long)
            String ssid = aps[i].ssid.isEmpty() ? String("[hidden]") : aps[i].ssid;
            if (ssid.length() > 26) ssid = ssid.substring(0, 25) + "~";
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawString(ssid, 36, y + 2);
        }
        if ((int)aps.size() > maxRows) {
            tft.setTextSize(1);
            tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            tft.drawString("+" + String((int)aps.size() - maxRows) + " more",
                           4, startY + maxRows * lineH + 2);
        }
    }

    // Footer hint
    tft.setTextSize(1);
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawString("[ Prev ] back", tftWidth / 2, tftHeight - 2);
    tft.setTextDatum(TL_DATUM);

    PrevPress = false;
    EscPress  = false;
    while (!PrevPress && !EscPress) {
        InputHandler();
        delay(50);
    }
    PrevPress = false;
    EscPress  = false;
}

// ── Entry point ───────────────────────────────────────────────────────────────

void wifiAnalyzerMenu() {
    if (WiFi.getMode() == WIFI_MODE_NULL) WiFi.mode(WIFI_STA);

    int16_t chanRSSI[CH_COUNT];
    std::vector<APInfo> chanAPs[CH_COUNT];
    for (int i = 0; i < CH_COUNT; i++) chanRSSI[i] = RSSI_FLOOR;

    _drawAnalyzer(chanRSSI, 0, true);

    bool running = true;
    while (running) {
        // Synchronous scan — ~100 ms/channel, ~1.4 s total for 14 channels
        int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true,
                                  /*passive=*/false, /*max_ms_per_chan=*/100);

        for (int i = 0; i < CH_COUNT; i++) {
            chanRSSI[i] = RSSI_FLOOR;
            chanAPs[i].clear();
        }
        for (int i = 0; i < n; i++) {
            int ch = WiFi.channel(i);
            int r  = WiFi.RSSI(i);
            if (ch >= 1 && ch <= CH_COUNT) {
                if (r > chanRSSI[ch - 1]) chanRSSI[ch - 1] = (int16_t)r;
                chanAPs[ch - 1].push_back({ WiFi.SSID(i), r });
            }
        }
        WiFi.scanDelete();

        // Best channel = lowest peak RSSI (empty channels score RSSI_FLOOR = best)
        int recCh      = 0;
        bool hasAPs    = false;
        int lowestRSSI = RSSI_CEIL + 1;
        for (int i = 0; i < CH_COUNT; i++) {
            if (chanRSSI[i] > RSSI_FLOOR) hasAPs = true;
            if (chanRSSI[i] < lowestRSSI) { lowestRSSI = chanRSSI[i]; recCh = i + 1; }
        }
        if (!hasAPs) recCh = 0;

        drawStatusBar();
        _drawAnalyzer(chanRSSI, recCh, false);

        // Wait SCAN_WAIT ms; check for ESC (exit) and tap (channel detail)
        unsigned long deadline = millis() + SCAN_WAIT;
        while (millis() < deadline && !EscPress) {
            InputHandler();

            if (touchPoint.pressed) {
                int tappedCh = _hitTestChannel(touchPoint.x, touchPoint.y);
                touchPoint.Clear();
                if (tappedCh > 0) {
                    _showChannelAPs(tappedCh, chanAPs);
                    // Restore graph after returning from detail screen
                    _drawAnalyzer(chanRSSI, recCh, true);
                    break; // restart wait loop without triggering a new scan immediately
                }
            }

            delay(50);
        }
        if (EscPress) running = false;
    }

    EscPress = false;
    WiFi.scanDelete();
}
