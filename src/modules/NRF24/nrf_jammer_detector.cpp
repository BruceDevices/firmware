#include "nrf_jammer_detector.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "nrf_common.h"
#include <globals.h>

#define JD_CHANNELS 80
#define JD_HISTORY_BITS 16      // rolling window of sweeps used for the duty-cycle signal (W1)
#define JD_DUTY_HOT_PCT 80      // channel treated as "hot" once its duty cycle in the window hits this
#define JD_WIFI_MAX_WIDTH 24    // widest single legit emitter (~20MHz WiFi channel) in NRF24 1MHz steps
#define JD_ALERT_ENTER_SWEEPS 2 // consecutive high-score sweeps required to confirm jamming
#define JD_ALERT_HOLD_MS 4000   // cooldown with no suspicious score before falling back to Normal

// Each entry is a shift register: 1 bit per sweep, oldest bit falls off automatically.
static uint16_t hitHistory[JD_CHANNELS];

enum JamState { JD_STATE_NORMAL, JD_STATE_SUSPECT, JD_STATE_JAMMING };
static JamState currentState = JD_STATE_NORMAL;
static int highScoreStreak = 0;
static uint32_t lastSuspectMs = 0;

static void jdScanChannels() {
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);
    for (int i = 0; i < JD_CHANNELS; i++) {
        NRFradio.setChannel(i);
        NRFradio.startListening();
        delayMicroseconds(128);
        NRFradio.stopListening();
        uint16_t rpd = NRFradio.testRPD() ? 1 : 0;
        hitHistory[i] = (hitHistory[i] << 1) | rpd;
    }
    digitalWrite(bruceConfigPins.NRF24_bus.io0, HIGH);
}

// Combines two independent signals into a 0-100 score:
//  W1 (0-65): how close to a continuous carrier the "hot" channels are (sustained duty cycle)
//  W2 (0-35): how much wider/more scattered the active band is than a single legit emitter explains
static int jdComputeScore(bool hot[JD_CHANNELS]) {
    int hotCount = 0;
    long hotDutySum = 0;
    for (int i = 0; i < JD_CHANNELS; i++) {
        int duty = __builtin_popcount(hitHistory[i]) * 100 / JD_HISTORY_BITS;
        hot[i] = duty >= JD_DUTY_HOT_PCT;
        if (hot[i]) {
            hotCount++;
            hotDutySum += duty;
        }
    }

    int w1 = 0;
    if (hotCount > 0) {
        int avgHotDuty = hotDutySum / hotCount;
        w1 = ((avgHotDuty - JD_DUTY_HOT_PCT) * 65) / (100 - JD_DUTY_HOT_PCT);
        w1 = constrain(w1, 0, 65);
    }

    int numClusters = 0;
    int widestCluster = 0;
    int i = 0;
    while (i < JD_CHANNELS) {
        if (!hot[i]) {
            i++;
            continue;
        }
        int start = i;
        int end = i;
        while (i < JD_CHANNELS) {
            if (hot[i]) {
                end = i;
                i++;
            } else if (i + 1 < JD_CHANNELS && hot[i + 1]) {
                i++; // tolerate a single-channel gap inside one emitter's band
            } else {
                break;
            }
        }
        widestCluster = max(widestCluster, end - start + 1);
        numClusters++;
    }

    int clusterExcess = widestCluster > JD_WIFI_MAX_WIDTH ? widestCluster - JD_WIFI_MAX_WIDTH : 0;
    int extraClusters = numClusters > 2 ? numClusters - 2 : 0;
    int w2 = clusterExcess + extraClusters * 8;
    if (hotCount > (JD_CHANNELS * 3) / 4) w2 += 15; // >75% of the band hot at once
    w2 = constrain(w2, 0, 35);

    return w1 + w2;
}

static void jdUpdateState(int score) {
    uint32_t now = millis();
    if (score >= 40) lastSuspectMs = now;

    if (score >= 70) {
        if (++highScoreStreak >= JD_ALERT_ENTER_SWEEPS) currentState = JD_STATE_JAMMING;
    } else {
        highScoreStreak = 0;
        if (currentState != JD_STATE_JAMMING && score >= 40) currentState = JD_STATE_SUSPECT;
    }

    if (now - lastSuspectMs > JD_ALERT_HOLD_MS) currentState = JD_STATE_NORMAL;
}

static const char *jdStateLabel() {
    switch (currentState) {
        case JD_STATE_JAMMING: return "JAMMING!";
        case JD_STATE_SUSPECT: return "Suspect";
        default: return "Normal";
    }
}

static uint16_t jdStateColor() {
    switch (currentState) {
        case JD_STATE_JAMMING: return TFT_RED;
        case JD_STATE_SUSPECT: return TFT_YELLOW;
        default: return TFT_GREEN;
    }
}

#define JD_CONTENT_TOP 46 // below the title text (drawn by drawMainBorderWithTitle, ends ~y=44)
#define JD_SIDE_PAD 8     // keep bars inside the border's side walls, same inset SpectrumPlot uses
#define JD_BOTTOM_PAD 6   // keep the bottom freq labels above the border's bottom wall

static void jdDraw(bool hot[JD_CHANNELS], int score) {
    uint16_t color = jdStateColor();

    tft.fillRect(JD_SIDE_PAD, JD_CONTENT_TOP, tftWidth - 2 * JD_SIDE_PAD, LH * FM + 4, bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(color, bruceConfig.bgColor);
    char statusBuf[24];
    snprintf(statusBuf, sizeof(statusBuf), "%s %d%%", jdStateLabel(), score);
    tft.drawCentreString(statusBuf, tftWidth / 2, JD_CONTENT_TOP, 1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);

    int barTop = JD_CONTENT_TOP + LH * FM + 6;
    int barBottom = tftHeight - JD_BOTTOM_PAD - LH - 4;
    int barArea = barBottom - barTop;
    int plotW = tftWidth - 2 * JD_SIDE_PAD;

    for (int i = 0; i < JD_CHANNELS; i++) {
        int duty = __builtin_popcount(hitHistory[i]) * 100 / JD_HISTORY_BITS;
        int level = (duty * barArea) / 100;
        int x = JD_SIDE_PAD + (i * plotW) / JD_CHANNELS;
        uint16_t barColor = hot[i] ? color : bruceConfig.secColor;
        tft.drawFastVLine(x, barBottom - level, level, barColor);
        tft.drawFastVLine(x, barTop, barArea - level, bruceConfig.bgColor);
    }
}

// Shared by the on-screen menu entry (headless=false, runs until ESC) and the `bluefish nrf24`
// serial command (headless=true, bounded duration, status printed to serialDevice instead of tft
// so it works the same over USB or the BLE serial bridge).
static void jdRun(bool headless, uint32_t durationMs) {
    if (!headless) {
        drawMainBorderWithTitle("Jammer Detector");
        tft.setTextSize(FP);
        int freqY = tftHeight - JD_BOTTOM_PAD - LH;
        tft.drawString("2.40Ghz", JD_SIDE_PAD, freqY);
        tft.drawCentreString("2.44Ghz", tftWidth / 2, freqY, 1);
        tft.drawRightString("2.48Ghz", tftWidth - JD_SIDE_PAD, freqY, 1);
    }

    if (!nrf_start(NRF_MODE_SPI)) {
        if (headless) {
            serialDevice->println("bluefish.nrf24jam error=NRF24 not found");
        } else {
            Serial.println("Fail Starting radio");
            displayError("NRF24 not found");
            delay(500);
        }
        return;
    }

    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setAddressWidth(2);
    const uint8_t noiseAddress[][2] = {
        {0x55, 0x55},
        {0xAA, 0xAA},
        {0xA0, 0xAA},
        {0xAB, 0xAA},
        {0xAC, 0xAA},
        {0xAD, 0xAA}
    };
    for (uint8_t i = 0; i < 6; ++i) NRFradio.openReadingPipe(i, noiseAddress[i]);
    NRFradio.setDataRate(RF24_1MBPS);

    memset(hitHistory, 0, sizeof(hitHistory));
    currentState = JD_STATE_NORMAL;
    highScoreStreak = 0;
    lastSuspectMs = 0;

    bool hot[JD_CHANNELS] = {};
    int score = 0;
    uint32_t startMs = millis();
    uint32_t lastStatusBarMs = startMs;
    uint32_t lastReportMs = startMs;
    while (headless ? (millis() - startMs < durationMs) : !check(EscPress)) {
        jdScanChannels();
        score = jdComputeScore(hot);
        jdUpdateState(score);

        if (headless) {
            if (millis() - lastReportMs >= 1000) {
                serialDevice->println(
                    "bluefish.nrf24jam state=" + String(jdStateLabel()) + " score=" + String(score)
                );
                lastReportMs = millis();
            }
        } else {
            jdDraw(hot, score);
            if (millis() - lastStatusBarMs >= 1000) {
                drawStatusBar();
                lastStatusBarMs = millis();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (headless) {
        serialDevice->println(
            "bluefish.nrf24jam done state=" + String(jdStateLabel()) + " score=" + String(score)
        );
    }

    NRFradio.stopListening();
    NRFradio.powerDown();
    delay(250);
}

void jammer_detector() { jdRun(false, 0); }

void jammer_detector_headless(uint32_t durationMs) { jdRun(true, durationMs); }
