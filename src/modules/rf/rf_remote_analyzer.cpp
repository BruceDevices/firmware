// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_remote_analyzer.h"

#include "core/display.h"
#include "rf_utils.h"
#include "structs.h"

namespace {

constexpr float kTrigger = -90.0f; // dBm threshold for a "found" carrier
constexpr float kAmDrop = 6.0f;    // dB drop on both sides => steep (AM) peak
constexpr uint32_t kFineStepHz = 20000;
constexpr uint32_t kFineSpanHz = 300000;
constexpr int kFinePoints = 31; // 600 kHz / 20 kHz + 1

int row_top() { return BORDER_PAD_Y + 8 * FM + 2; }
uint16_t dim() { return blendColors(bruceConfig.bgColor, bruceConfig.priColor, 170); }

float rssiAt(float mhz) {
    setMHZ(mhz);
    tft.drawPixel(0, 0, 0); // shared SPI with TFT: keep the bus sane
    vTaskDelay(pdMS_TO_TICKS(4));
    return (float)ELECHOUSE_cc1101.getRssi();
}

// Redraw only the result region (below the title) when the content changed.
void drawResult(bool found, float freq, float rssi, bool isAm) {
    const int y0 = row_top() - 2;
    tft.fillRect(BORDER_PAD_X, y0, tftWidth - 2 * BORDER_PAD_X, tftHeight - y0 - (LH * FP + 10), bruceConfig.bgColor);
    tft.setTextSize(FP);
    int y = row_top();
    const int lh = LH * FP + 6;

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    if (!found) {
        tft.drawString("No carrier above " + String((int)kTrigger) + " dBm", BORDER_PAD_X, y, 1);
        y += lh;
    } else {
        tft.drawString("Signal: " + String(freq, 3) + " MHz", BORDER_PAD_X, y, 1);
        y += lh;
        tft.drawString("RSSI: " + String((int)rssi) + " dBm", BORDER_PAD_X, y, 1);
        y += lh;
        tft.drawString(
            isAm ? "Modulation: AM (ASK/OOK)" : "Modulation: FM / wide", BORDER_PAD_X, y, 1);
        y += lh;
    }

    tft.setTextColor(dim(), bruceConfig.bgColor);
    tft.drawString("Scanning...", BORDER_PAD_X, y, 1);
}

} // namespace

void rf_remote_analyzer() {
#if !defined(LITE_VERSION)
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Remote analyzer needs a CC1101!", true);
        return;
    }
    if (!initRfModule("rx", 433.92)) {
        displayError("CC1101 not found!", true);
        return;
    }

    // Draw the static frame once — redrawing it each pass is what caused the
    // text to flicker on and off.
    drawMainBorderWithTitle("Remote Analyzer");
    tft.setTextSize(FP);
    tft.setTextColor(dim(), bruceConfig.bgColor);
    tft.drawString("[ESC] back", BORDER_PAD_X, tftHeight - LH * FP - 8, 1);

    const int freqCount = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);

    // Cache the displayed result so the screen is only touched when it changes.
    String lastKey = "";

    while (!check(EscPress)) {
        // ---- Coarse scan: strongest carrier across the band ----
        float bestFreq = 0, bestRssi = -127.0f;
        for (int i = 0; i < freqCount && !check(EscPress); i++) {
            const float f = subghz_frequency_list[i];
            const float r = rssiAt(f);
            if (r > bestRssi) {
                bestRssi = r;
                bestFreq = f;
            }
        }
        if (check(EscPress)) break;

        bool found = bestRssi > kTrigger;
        float fineFreq = bestFreq, fineRssi = bestRssi;
        bool isAm = false;

        // ---- Fine scan: pinpoint ±300 kHz, classify peak shape ----
        if (found) {
            float sweep[kFinePoints];
            int idx = 0;
            float peakRssi = -999.0f;
            int peakIdx = 0;
            const uint32_t center = (uint32_t)(bestFreq * 1000000.0f);
            for (uint32_t f = center - kFineSpanHz; f < center + kFineSpanHz && !check(EscPress);
                 f += kFineStepHz) {
                const float r = rssiAt(f / 1000000.0f);
                sweep[idx] = r;
                if (r > peakRssi) {
                    peakRssi = r;
                    peakIdx = idx;
                }
                idx++;
            }
            const float left = peakIdx > 0 ? sweep[peakIdx - 1] : peakRssi;
            const float right = peakIdx + 1 < idx ? sweep[peakIdx + 1] : peakRssi;
            isAm = (peakRssi - left > kAmDrop) && (peakRssi - right > kAmDrop);
            fineRssi = peakRssi;
            fineFreq = (center - kFineSpanHz + (uint32_t)peakIdx * kFineStepHz) / 1000000.0f;
        }

        // Only repaint when the result actually changed (no flicker on stable signals).
        String key = String(found) + "|" + String((int)(fineFreq * 1000)) + "|" +
                     String((int)fineRssi) + "|" + String(isAm);
        if (key != lastKey) {
            lastKey = key;
            drawResult(found, fineFreq, fineRssi, isAm);
        }

        vTaskDelay(pdMS_TO_TICKS(150));
    }

    deinitRfModule();
    returnToMenu = true;
#else
    displayError("Not available on Launcher version");
#endif
}
