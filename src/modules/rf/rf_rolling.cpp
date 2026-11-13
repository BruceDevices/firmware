// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_rolling.h"

#include "core/display.h"
#include "core/type_convertion.h"
#include "protocols/rf_decoder.h"
#include "protocols/rf_protocol_engine.h"
#include "rf_scan.h" // rf_try_keeloq
#include "rf_utils.h"
#include "structs.h"

namespace {

struct RollingEntry {
    uint32_t serial = 0;
    uint8_t btn = 0;
    uint32_t cnt = 0;
    String mfr;
    unsigned long ms = 0;
};

void hex4(uint32_t v, char *out, size_t n) { snprintf(out, n, "%04lX", (unsigned long)(v & 0xFFFF)); }
void hex8(uint32_t v, char *out, size_t n) { snprintf(out, n, "%08lX", (unsigned long)v); }

void draw_rolling(const std::vector<RollingEntry> &hist) {
    drawMainBorderWithTitle("Rolling Code");
    const int x = BORDER_PAD_X;
    const int lh = LH * FP + 6;
    int y = BORDER_PAD_Y + FM * LH + 6;
    tft.setTextSize(FP);

    if (hist.empty()) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawString("Press the remote...", x, y, 1);
        tft.drawString("(captures KeeLoq / rolling codes)", x, y + lh, 1);
        return;
    }

    const RollingEntry &last = hist.back();

    // Verdict: compare the last two distinct counters.
    String verdict = "Fixed code (counter unchanged)";
    if (hist.size() >= 2) {
        const RollingEntry &prev = hist[hist.size() - 2];
        if (prev.cnt != last.cnt) {
            int32_t delta = (int32_t)(last.cnt - prev.cnt);
            char d[16];
            snprintf(d, sizeof(d), "%+d", (int)delta);
            verdict = String("Rolling: counter ") + d;
        }
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString(verdict, x, y, 1);
    y += lh + 2;

    char buf[24];
    tft.setTextColor(blendColors(bruceConfig.bgColor, bruceConfig.priColor, 170), bruceConfig.bgColor);
    tft.drawString("Mfr: " + last.mfr, x, y, 1);
    y += lh;
    hex8(last.serial, buf, sizeof(buf));
    tft.drawString("Serial: " + String(buf), x, y, 1);
    y += lh;
    tft.drawString("Button: " + String(last.btn), x, y, 1);
    y += lh;
    hex4(last.cnt, buf, sizeof(buf));
    tft.drawString("Counter: " + String(buf) + " (" + String(last.cnt) + ")", x, y, 1);
    y += lh + 2;

    // Recent counter history (most recent last).
    const size_t shown = hist.size() > 4 ? hist.size() - 4 : 0;
    String seq = "seq: ";
    for (size_t i = shown; i < hist.size(); i++) {
        hex4(hist[i].cnt, buf, sizeof(buf));
        if (i > shown) seq += " > ";
        seq += buf;
    }
    tft.setTextColor(blendColors(bruceConfig.bgColor, bruceConfig.priColor, 120), bruceConfig.bgColor);
    tft.drawString(seq, x, y, 1);
}

} // namespace

void rf_rolling_code() {
#if !defined(LITE_VERSION)
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Rolling Code needs a CC1101!", true);
        return;
    }
    float freq = bruceConfigPins.rfFreq;
    if (!initRfModule("rx", freq)) {
        displayError("CC1101 not found!", true);
        return;
    }

    RfRxSession rx;
    if (!rx.begin()) {
        deinitRfModule();
        return;
    }

    std::vector<RollingEntry> hist;
    draw_rolling(hist);

    while (1) {
        if (check(EscPress)) break;

        std::vector<int> durations;
        if (rx.poll(durations)) {
            RfCodes code;
            if (rf_try_keeloq(durations, code)) {
                RollingEntry e;
                e.serial = code.serial;
                e.btn = code.btn;
                e.cnt = code.cnt;
                e.mfr = code.mf_name.isEmpty() ? "Unknown" : code.mf_name;
                e.ms = millis();
                // Skip exact duplicate frames (same press re-transmitted).
                if (hist.empty() || hist.back().cnt != e.cnt || hist.back().btn != e.btn ||
                    hist.back().serial != e.serial) {
                    hist.push_back(e);
                    if (hist.size() > 8) hist.erase(hist.begin());
                }
                draw_rolling(hist);
            } else if (rf_protocol_decode_capture(
                           durations, (uint32_t)(freq * 1000000.0f), "Ook270Async", code)) {
                // Non-KeeLoq rolling/fixed code (e.g. Somfy, Nice, Came).
                RollingEntry e;
                e.serial = code.serial;
                e.btn = code.btn;
                e.cnt = code.cnt;
                e.mfr = code.protocol;
                e.ms = millis();
                if (hist.empty() || hist.back().cnt != e.cnt || hist.back().btn != e.btn ||
                    hist.back().serial != e.serial) {
                    hist.push_back(e);
                    if (hist.size() > 8) hist.erase(hist.begin());
                }
                draw_rolling(hist);
            }
        }

        if (setMHZMenu()) {
            freq = bruceConfigPins.rfFreq;
            rx.end();
            rx.begin();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    rx.end();
    deinitRfModule();
    returnToMenu = true;
#else
    displayError("Not available on Launcher version");
#endif
}
