#include "rf_jammer_detector.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "rf_utils.h"
#include <globals.h>

#define RJD_MAX_FREQS 57 // matches subghz_frequency_list[]
#define RJD_HISTORY_BITS 16
#define RJD_HOT_RSSI (-70)   // dBm treated as "energy present" (same reference rf_CC1101_rssi() uses)
#define RJD_FLAG_DUTY_PCT 35 // frequency counted as "flagged" once its duty in the window hits this
#define RJD_FULL_DUTY_PCT 85 // flagged frequency classified as FULL POWER above this duty
#define RJD_ALERT_ENTER_SWEEPS 2
#define RJD_ALERT_HOLD_MS 4000

// Each entry is a shift register: 1 bit per sweep, oldest bit falls off automatically.
static uint16_t hitHistory[RJD_MAX_FREQS];

enum RfJamState { RJD_STATE_NORMAL, RJD_STATE_SUSPECT, RJD_STATE_JAMMING };
static RfJamState currentState = RJD_STATE_NORMAL;
static int highScoreStreak = 0;
static uint32_t lastSuspectMs = 0;

struct RjdResult {
    int score;
    int worstIdx; // index within the active range, -1 if nothing flagged
    bool isFull;
};

static void jrdSweep(int start, int count) {
    for (int i = 0; i < count; i++) {
        if (EscPress || SelPress) break;
        setMHZ(subghz_frequency_list[start + i]);
        vTaskDelay(pdMS_TO_TICKS(5));
        int rssi = ELECHOUSE_cc1101.getRssi();
        tft.drawPixel(0, 0, 0); // keeps CC1101/TFT shared SPI bus well-behaved, same as rf_CC1101_rssi()
        uint16_t hot = (rssi >= RJD_HOT_RSSI) ? 1 : 0;
        hitHistory[i] = (hitHistory[i] << 1) | hot;
    }
}

// FULL POWER keeps a near-continuous carrier, so its duty cycle across the window stays high and
// steady. INTERMITTENT works through pulse trains with gaps, so a sample landing once per sweep
// catches it less consistently. hotCount gates against broadband noise: the jammers this firmware
// ships stay on one fixed frequency, so energy spread across many at once is unlikely to be them.
static RjdResult jrdComputeScore(int count) {
    RjdResult r{0, -1, false};
    int hotCount = 0;
    int worstDuty = -1;

    for (int i = 0; i < count; i++) {
        int duty = __builtin_popcount(hitHistory[i]) * 100 / RJD_HISTORY_BITS;
        if (duty >= RJD_FLAG_DUTY_PCT) {
            hotCount++;
            if (duty > worstDuty) {
                worstDuty = duty;
                r.worstIdx = i;
            }
        }
    }
    if (r.worstIdx < 0) return r;

    int w1 = ((worstDuty - RJD_FLAG_DUTY_PCT) * 65) / (100 - RJD_FLAG_DUTY_PCT);
    w1 = constrain(w1, 0, 65);

    int w2;
    if (hotCount <= 2) w2 = 35;
    else if (hotCount == 3) w2 = 20;
    else w2 = 0;

    r.score = w1 + w2;
    r.isFull = worstDuty >= RJD_FULL_DUTY_PCT;
    return r;
}

static void jrdUpdateState(int score) {
    uint32_t now = millis();
    if (score >= 40) lastSuspectMs = now;

    if (score >= 70) {
        if (++highScoreStreak >= RJD_ALERT_ENTER_SWEEPS) currentState = RJD_STATE_JAMMING;
    } else {
        highScoreStreak = 0;
        if (currentState != RJD_STATE_JAMMING && score >= 40) currentState = RJD_STATE_SUSPECT;
    }

    if (now - lastSuspectMs > RJD_ALERT_HOLD_MS) currentState = RJD_STATE_NORMAL;
}

static const char *jrdStateLabel() {
    switch (currentState) {
        case RJD_STATE_JAMMING: return "JAMMING!";
        case RJD_STATE_SUSPECT: return "Suspect";
        default: return "Normal";
    }
}

static uint16_t jrdStateColor() {
    switch (currentState) {
        case RJD_STATE_JAMMING: return TFT_RED;
        case RJD_STATE_SUSPECT: return TFT_YELLOW;
        default: return TFT_GREEN;
    }
}

#define RJD_CONTENT_TOP 46 // below the title text (drawn by drawMainBorderWithTitle, ends ~y=44)
#define RJD_SIDE_PAD 8     // keep bars inside the border's side walls, same inset SpectrumPlot uses
#define RJD_BOTTOM_PAD 6   // keep the bottom range label above the border's bottom wall

static void jrdDraw(int start, int count, const RjdResult &res) {
    uint16_t color = jrdStateColor();

    // FP, not FM: with the vendor/classification text appended this line can run past 20 chars,
    // which would overflow the screen width at FM.
    tft.fillRect(RJD_SIDE_PAD, RJD_CONTENT_TOP, tftWidth - 2 * RJD_SIDE_PAD, LH + 4, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setTextColor(color, bruceConfig.bgColor);
    char statusBuf[40];
    if (currentState == RJD_STATE_JAMMING && res.worstIdx >= 0) {
        snprintf(
            statusBuf,
            sizeof(statusBuf),
            "%s %s %.2f",
            jrdStateLabel(),
            res.isFull ? "FULL POWER" : "INTERMITTENT",
            subghz_frequency_list[start + res.worstIdx]
        );
    } else {
        snprintf(statusBuf, sizeof(statusBuf), "%s %d%%", jrdStateLabel(), res.score);
    }
    tft.drawCentreString(statusBuf, tftWidth / 2, RJD_CONTENT_TOP, 1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    int barTop = RJD_CONTENT_TOP + LH + 6;
    int barBottom = tftHeight - RJD_BOTTOM_PAD - LH - 4;
    int barArea = barBottom - barTop;
    int plotW = tftWidth - 2 * RJD_SIDE_PAD;

    for (int i = 0; i < count; i++) {
        int duty = __builtin_popcount(hitHistory[i]) * 100 / RJD_HISTORY_BITS;
        bool flagged = duty >= RJD_FLAG_DUTY_PCT;
        int level = (duty * barArea) / 100;
        int x = RJD_SIDE_PAD + (i * plotW) / count;
        uint16_t barColor = flagged ? color : bruceConfig.secColor;
        tft.drawFastVLine(x, barBottom - level, level, barColor);
        tft.drawFastVLine(x, barTop, barArea - level, bruceConfig.bgColor);
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    char rangeBuf[48];
    snprintf(
        rangeBuf,
        sizeof(rangeBuf),
        "%.3f-%.3fMHz SEL:range",
        subghz_frequency_list[start],
        subghz_frequency_list[start + count - 1]
    );
    tft.drawString(rangeBuf, RJD_SIDE_PAD, tftHeight - RJD_BOTTOM_PAD - LH, 1);
}

// Shared by the on-screen menu entry (headless=false, ESC to exit, SEL to change range) and the
// `bluefish subghz` serial command (headless=true, bounded duration, no display, status printed to
// serialDevice so it works the same over USB or the BLE serial bridge).
static void jrdRun(bool headless, uint32_t durationMs) {
#if !defined(LITE_VERSION)
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        if (headless) serialDevice->println("bluefish.subghzjam error=only for CC1101 module");
        else displayError("only for CC1101 module", true);
        return;
    }

    int start = range_limits[bruceConfigPins.rfScanRange][0];
    int count = range_limits[bruceConfigPins.rfScanRange][1] - start + 1;
    if (count > RJD_MAX_FREQS) count = RJD_MAX_FREQS;

    memset(hitHistory, 0, sizeof(hitHistory));
    currentState = RJD_STATE_NORMAL;
    highScoreStreak = 0;
    lastSuspectMs = 0;

    if (!initRfModule("rx", bruceConfigPins.rfFreq)) {
        if (headless) serialDevice->println("bluefish.subghzjam error=module start failed");
        else displayError("Error starting module", true);
        return;
    }
    if (!headless) drawMainBorderWithTitle("CC1101 Jammer");

    RjdResult res{0, -1, false};
    uint32_t startMs = millis();
    uint32_t lastStatusBarMs = startMs;
    uint32_t lastReportMs = startMs;
    while (headless ? (millis() - startMs < durationMs) : true) {
        jrdSweep(start, count);
        res = jrdComputeScore(count);
        jrdUpdateState(res.score);

        if (headless) {
            if (millis() - lastReportMs >= 1000) {
                serialDevice->println(
                    "bluefish.subghzjam state=" + String(jrdStateLabel()) + " score=" + String(res.score)
                );
                lastReportMs = millis();
            }
        } else {
            jrdDraw(start, count, res);
            if (millis() - lastStatusBarMs >= 1000) {
                drawStatusBar();
                lastStatusBarMs = millis();
            }

            if (check(EscPress)) break;
            if (check(SelPress)) {
                deinitRfModule();
                rf_range_selection(bruceConfigPins.rfFreq);

                start = range_limits[bruceConfigPins.rfScanRange][0];
                count = range_limits[bruceConfigPins.rfScanRange][1] - start + 1;
                if (count > RJD_MAX_FREQS) count = RJD_MAX_FREQS;
                memset(hitHistory, 0, sizeof(hitHistory));

                if (!initRfModule("rx", bruceConfigPins.rfFreq)) {
                    displayError("Error starting module", true);
                    break;
                }
                drawMainBorderWithTitle("CC1101 Jammer");
            }
        }
    }

    if (headless) {
        serialDevice->println(
            "bluefish.subghzjam done state=" + String(jrdStateLabel()) + " score=" + String(res.score)
        );
    }

    deinitRfModule();
#else
    if (headless) serialDevice->println("bluefish.subghzjam error=not available on Launcher version");
    else displayError("Not available on Launcher version");
#endif
}

void rf_jammer_detector() { jrdRun(false, 0); }

void rf_jammer_detector_headless(uint32_t durationMs) { jrdRun(true, durationMs); }
