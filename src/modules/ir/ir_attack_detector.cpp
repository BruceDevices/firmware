#include "ir_attack_detector.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <IRrecv.h>
#include <globals.h>

// Time is bucketed instead of channel-swept (IR has no frequency axis worth
// plotting here): each bucket counts how many IR events landed in it and how
// many of those failed to decode into a known protocol.
#define IRD_BUCKET_MS 200
#define IRD_BUCKETS 40      // 8s of history
#define IRD_WINDOW 10       // last 2s used for scoring
#define IRD_RATE_SATURATE 6 // events/sec where the rate signal (W1) maxes out
#define IRD_PROTO_RING 8    // recent valid decode_type values, for distinct-protocol counting
#define IRD_ALERT_ENTER_SWEEPS 2
#define IRD_ALERT_HOLD_MS 4000
#define IRD_CONTENT_TOP 46 // below the title text (drawn by drawMainBorderWithTitle, ends ~y=44)
#define IRD_SIDE_PAD 8     // keep bars inside the border's side walls, same inset SpectrumPlot uses
#define IRD_BOTTOM_PAD 6   // keep the bottom info line above the border's bottom wall

static uint8_t bucketCount[IRD_BUCKETS];
static uint8_t bucketInvalid[IRD_BUCKETS];
static int bucketHead = 0;
static uint32_t bucketStartMs = 0;

static int protoRing[IRD_PROTO_RING];
static int protoRingLen = 0;
static int protoRingHead = 0;

enum IrJamState { IRD_STATE_NORMAL, IRD_STATE_SUSPECT, IRD_STATE_JAMMING };
static IrJamState currentState = IRD_STATE_NORMAL;
static int highScoreStreak = 0;
static uint32_t lastSuspectMs = 0;

struct IrdResult {
    int score = 0;
    int eventsPerSec = 0;
    int validPct = 0;
    int distinctProtocols = 0;
    bool isTvBGone = false;
    bool isJammer = false;
};

static void irdRecordProtocol(int type) {
    protoRing[protoRingHead] = type;
    protoRingHead = (protoRingHead + 1) % IRD_PROTO_RING;
    if (protoRingLen < IRD_PROTO_RING) protoRingLen++;
}

static int irdCountDistinctProtocols() {
    int distinct = 0;
    for (int i = 0; i < protoRingLen; i++) {
        bool seen = false;
        for (int j = 0; j < i; j++) {
            if (protoRing[j] == protoRing[i]) {
                seen = true;
                break;
            }
        }
        if (!seen) distinct++;
    }
    return distinct;
}

// TV-B-Gone fires a long, steady sequence of *valid* codes (varying protocol
// per brand) roughly every ~200ms. IR Jammer fires far more raw pulses per
// second that mostly fail to decode into any known protocol. W1 flags "IR
// activity well above what a person pressing a remote produces"; W2 uses the
// decode success ratio and protocol variety to tell the two apart.
static IrdResult irdComputeScore() {
    IrdResult r;
    int totalEvents = 0, totalInvalid = 0;
    for (int i = 0; i < IRD_WINDOW; i++) {
        int idx = (bucketHead - i + IRD_BUCKETS) % IRD_BUCKETS;
        totalEvents += bucketCount[idx];
        totalInvalid += bucketInvalid[idx];
    }
    if (totalEvents == 0) return r;

    r.eventsPerSec = totalEvents * 1000 / (IRD_WINDOW * IRD_BUCKET_MS);
    r.validPct = (totalEvents - totalInvalid) * 100 / totalEvents;
    r.distinctProtocols = irdCountDistinctProtocols();

    int w1 = (r.eventsPerSec * 60) / IRD_RATE_SATURATE;
    w1 = constrain(w1, 0, 60);

    int w2;
    if (r.validPct >= 70 && r.distinctProtocols >= 3) {
        w2 = 40;
        r.isTvBGone = true;
    } else if (r.validPct <= 30) {
        w2 = 40;
        r.isJammer = true;
    } else {
        w2 = 15; // high volume but ambiguous decode mix — still abnormal, just unclassified
    }

    r.score = constrain(w1 + w2, 0, 100);
    return r;
}

static void irdUpdateState(int score) {
    uint32_t now = millis();
    if (score >= 40) lastSuspectMs = now;

    if (score >= 70) {
        if (++highScoreStreak >= IRD_ALERT_ENTER_SWEEPS) currentState = IRD_STATE_JAMMING;
    } else {
        highScoreStreak = 0;
        if (currentState != IRD_STATE_JAMMING && score >= 40) currentState = IRD_STATE_SUSPECT;
    }

    if (now - lastSuspectMs > IRD_ALERT_HOLD_MS) currentState = IRD_STATE_NORMAL;
}

static const char *irdStateLabel(const IrdResult &res) {
    if (currentState == IRD_STATE_JAMMING) {
        if (res.isTvBGone) return "TV-B-GONE!";
        if (res.isJammer) return "IR JAMMER!";
        return "IR ATTACK!";
    }
    return currentState == IRD_STATE_SUSPECT ? "Suspect" : "Normal";
}

static uint16_t irdStateColor() {
    switch (currentState) {
        case IRD_STATE_JAMMING: return TFT_RED;
        case IRD_STATE_SUSPECT: return TFT_YELLOW;
        default: return TFT_GREEN;
    }
}

static void irdDraw(const IrdResult &res) {
    uint16_t color = irdStateColor();

    tft.fillRect(
        IRD_SIDE_PAD, IRD_CONTENT_TOP, tftWidth - 2 * IRD_SIDE_PAD, LH * FM + 4, bruceConfig.bgColor
    );
    tft.setTextSize(FM);
    tft.setTextColor(color, bruceConfig.bgColor);
    char statusBuf[32];
    snprintf(statusBuf, sizeof(statusBuf), "%s %d%%", irdStateLabel(res), res.score);
    tft.drawCentreString(statusBuf, tftWidth / 2, IRD_CONTENT_TOP, 1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);

    int barTop = IRD_CONTENT_TOP + LH * FM + 6;
    int barBottom = tftHeight - IRD_BOTTOM_PAD - LH - 4;
    int barArea = barBottom - barTop;
    int plotW = tftWidth - 2 * IRD_SIDE_PAD;

    for (int i = 0; i < IRD_BUCKETS; i++) {
        int idx = (bucketHead - (IRD_BUCKETS - 1 - i) + IRD_BUCKETS) % IRD_BUCKETS;
        int count = bucketCount[idx];
        int invalid = bucketInvalid[idx];
        int level = count * barArea / 10; // 10 events/bucket (200ms) already saturates the plot
        if (level > barArea) level = barArea;
        int x = IRD_SIDE_PAD + (i * plotW) / IRD_BUCKETS;

        uint16_t barColor = bruceConfig.secColor;
        if (count > 0) barColor = (invalid * 2 > count) ? TFT_RED : TFT_GREEN;

        tft.drawFastVLine(x, barBottom - level, level, barColor);
        tft.drawFastVLine(x, barTop, barArea - level, bruceConfig.bgColor);
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    char infoBuf[48];
    snprintf(
        infoBuf,
        sizeof(infoBuf),
        "%d ev/s valid%d%% proto%d",
        res.eventsPerSec,
        res.validPct,
        res.distinctProtocols
    );
    tft.drawString(infoBuf, IRD_SIDE_PAD, tftHeight - IRD_BOTTOM_PAD - LH, 1);
}

// Shared by the on-screen menu entry (headless=false, runs until ESC) and the `bluefish ir` serial
// command (headless=true, bounded duration, status printed to serialDevice instead of tft so it
// works the same over USB or the BLE serial bridge).
static void irdRun(bool headless, uint32_t durationMs) {
    IRrecv irrecv(bruceConfigPins.irRx, SAFE_STACK_BUFFER_SIZE / 2, 50);
    irrecv.enableIRIn();
    decode_results results;

    memset(bucketCount, 0, sizeof(bucketCount));
    memset(bucketInvalid, 0, sizeof(bucketInvalid));
    bucketHead = 0;
    bucketStartMs = millis();
    protoRingLen = 0;
    protoRingHead = 0;
    currentState = IRD_STATE_NORMAL;
    highScoreStreak = 0;
    lastSuspectMs = 0;

    if (!headless) drawMainBorderWithTitle("IR Attack Detector");

    IrdResult res;
    uint32_t startMs = millis();
    uint32_t lastStatusBarMs = startMs;
    uint32_t lastReportMs = startMs;
    while (headless ? (millis() - startMs < durationMs) : !check(EscPress)) {
        uint32_t now = millis();
        while (now - bucketStartMs >= IRD_BUCKET_MS) {
            bucketHead = (bucketHead + 1) % IRD_BUCKETS;
            bucketCount[bucketHead] = 0;
            bucketInvalid[bucketHead] = 0;
            bucketStartMs += IRD_BUCKET_MS;
        }

        if (irrecv.decode(&results)) {
            bool valid = (results.decode_type != decode_type_t::UNKNOWN);
            if (bucketCount[bucketHead] < 255) bucketCount[bucketHead]++;
            if (!valid && bucketInvalid[bucketHead] < 255) bucketInvalid[bucketHead]++;
            if (valid) irdRecordProtocol((int)results.decode_type);
            irrecv.resume();
        }

        res = irdComputeScore();
        irdUpdateState(res.score);

        if (headless) {
            if (now - lastReportMs >= 1000) {
                serialDevice->println(
                    "bluefish.irdetect state=" + String(irdStateLabel(res)) + " score=" + String(res.score)
                );
                lastReportMs = now;
            }
        } else {
            irdDraw(res);
            if (now - lastStatusBarMs >= 1000) {
                drawStatusBar();
                lastStatusBarMs = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    if (headless) {
        serialDevice->println(
            "bluefish.irdetect done state=" + String(irdStateLabel(res)) + " score=" + String(res.score)
        );
    }
}

void ir_attack_detector() { irdRun(false, 0); }

void ir_attack_detector_headless(uint32_t durationMs) { irdRun(true, durationMs); }
