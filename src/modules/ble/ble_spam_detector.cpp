#include "ble_spam_detector.h"
#include "ble_common.h"
#include "core/mykeyboard.h"

#define BSD_CATEGORIES 4
enum SpamCategory { BSD_APPLE = 0, BSD_SAMSUNG, BSD_MICROSOFT, BSD_FASTPAIR };
static const char *BSD_NAMES[BSD_CATEGORIES] = {"Apple", "Samsung", "SwiftPair", "FastPair"};

#define BSD_RING                                                                                             \
    24 // event ring per vendor: (timestamp, MAC, RSSI) triples, enough for a full window even
       // at Bruce's spam rate split across getResults() chunks
#define BSD_WINDOW_MS 2000
#define BSD_RATE_SATURATE                                                                                    \
    18 // pkt/s where the rate signal (W1) maxes out. Real captured rate for a live attack tops out
       // well below the ~100pkt/s the transmitter targets (scanning itself has a duty cycle) — this
       // is calibrated to what a real attack measures as, not the transmitter's theoretical rate
#define BSD_MAC_SATURATE                                                                                     \
    8 // distinct MACs *within the window* where the churn signal (W2) maxes out — a few real
      // devices pulsing at once is normal, 8 different addresses for the same vendor in 2s isn't
// A dense-Apple-device environment can rack up just as many distinct addresses in the window as a
// real spammer, so raw churn alone isn't enough to tell them apart — see bsdAnalyzeWindow. Tight/
// loose bounds are deliberately generous: real-world RSSI jitters a few dB even from one fixed
// transmitter (multipath, orientation), and being too strict here was capping genuine attacks well
// under the JAMMING threshold.
#define BSD_RSSI_TIGHT_DB 10 // spread at/under this looks like ONE transmitter (full churn credit)
#define BSD_RSSI_LOOSE_DB 28 // spread at/over this looks like separate physical devices (credit cut)
#define BSD_JAM_SCORE 60     // score at/over this counts toward the JAMMING streak
#define BSD_SUSPECT_SCORE 35 // score at/over this is at least flagged Suspect
#define BSD_ALERT_ENTER_SWEEPS                                                                               \
    3 // consecutive high-score chunks required before flagging JAMMING — at the 500ms chunk size
      // this demands ~1.5s of sustained activity, not just one ambient burst
#define BSD_ALERT_HOLD_MS 4000
#define BSD_SCAN_CHUNK_MS 500 // getResults() dwell per redraw — short enough to feel live

// One entry per received packet. All signals below only look at entries still inside BSD_WINDOW_MS,
// so a vendor's score decays back down once the burst actually stops — unlike a plain "seen so far"
// tally, which would stay pinned high after any handful of real devices.
struct BsdEvent {
    uint32_t ts;
    uint32_t macHash;
    int8_t rssi;
};
struct CategoryState {
    BsdEvent ring[BSD_RING];
    uint8_t head;
    uint8_t len;
};
static CategoryState cat[BSD_CATEGORIES];

enum BsdJamState { BSD_STATE_NORMAL, BSD_STATE_SUSPECT, BSD_STATE_JAMMING };
static BsdJamState currentState = BSD_STATE_NORMAL;
static int highScoreStreak = 0;
static uint32_t lastSuspectMs = 0;

static uint32_t bsdHashMac(const uint8_t *mac) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < 6; i++) {
        h ^= mac[i];
        h *= 16777619u;
    }
    return h;
}

// BLE Spam's payloads mirror the real vendor formats closely enough that matching on Company ID /
// Service Data UUID is enough to bucket a packet — the giveaway isn't the payload shape, it's the
// rate and address churn behind it (see bsdComputeScore).
static int bsdClassify(const NimBLEAdvertisedDevice *dev) {
    if (dev->haveManufacturerData()) {
        std::string md = dev->getManufacturerData(0);
        if (md.size() >= 2) {
            uint16_t companyId = (uint8_t)md[0] | ((uint16_t)(uint8_t)md[1] << 8);
            if (companyId == 0x004C) return BSD_APPLE;
            if (companyId == 0x0075) return BSD_SAMSUNG;
            if (companyId == 0x0006) return BSD_MICROSOFT;
        }
    }
    if (dev->haveServiceData()) {
        for (uint8_t i = 0; i < dev->getServiceDataCount(); i++) {
            if (dev->getServiceDataUUID(i) == NimBLEUUID((uint16_t)0xFE2C)) return BSD_FASTPAIR;
        }
    }
    return -1;
}

static void bsdRecordEvent(int category, uint32_t macHash, int8_t rssi) {
    CategoryState &c = cat[category];
    c.ring[c.head] = {millis(), macHash, rssi};
    c.head = (c.head + 1) % BSD_RING;
    if (c.len < BSD_RING) c.len++;
}

struct BsdWindowStats {
    int events = 0;
    int distinctMacs = 0;
    int rssiSpreadDb = 0; // spread of RSSI across the distinct MACs, 0 when fewer than 2
};

// A rotated-address spammer is still ONE physical radio, so every fake address it sends shows up at
// roughly the same RSSI at the receiver; several distinct real devices sit at different distances
// and spread much wider. Distinct-count and rate alone can't tell a dense pocket of real Apple gear
// apart from an attack, but the RSSI spread among those "different" addresses can.
static BsdWindowStats bsdAnalyzeWindow(const CategoryState &c, uint32_t now) {
    BsdWindowStats s;
    int minRssi = 127, maxRssi = -127;
    for (int i = 0; i < c.len; i++) {
        if (now - c.ring[i].ts > BSD_WINDOW_MS) continue;
        s.events++;

        bool seen = false;
        for (int j = 0; j < i; j++) {
            if (now - c.ring[j].ts <= BSD_WINDOW_MS && c.ring[j].macHash == c.ring[i].macHash) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            s.distinctMacs++;
            if (c.ring[i].rssi < minRssi) minRssi = c.ring[i].rssi;
            if (c.ring[i].rssi > maxRssi) maxRssi = c.ring[i].rssi;
        }
    }
    if (s.distinctMacs >= 2) s.rssiSpreadDb = maxRssi - minRssi;
    return s;
}

struct BsdResult {
    int score = 0;
    int worstCat = -1;
    int rate = 0;
    int distinctMacs = 0;
};

// A real accessory (real AirPods, a real Galaxy Watch...) keeps one address for the whole window
// and advertises at most a few times a second. BLE Spam fires much faster and, with the default MAC
// rotation setting, a different address on every packet — W1 catches the volume, W2 catches the
// churn, weighted by how physically plausible that churn is (see bsdAnalyzeWindow).
static BsdResult bsdComputeScore(uint32_t now) {
    BsdResult best;
    for (int c = 0; c < BSD_CATEGORIES; c++) {
        BsdWindowStats s = bsdAnalyzeWindow(cat[c], now);
        int rate = s.events * 1000 / BSD_WINDOW_MS;

        int w1 = constrain(rate * 60 / BSD_RATE_SATURATE, 0, 60);

        int churnConfidence; // 0-100: how much the churn looks like one radio faking addresses
        if (s.distinctMacs < 3) churnConfidence = 0;
        else if (s.rssiSpreadDb <= BSD_RSSI_TIGHT_DB) churnConfidence = 100;
        else if (s.rssiSpreadDb >= BSD_RSSI_LOOSE_DB) churnConfidence = 20;
        else
            churnConfidence =
                100 - (s.rssiSpreadDb - BSD_RSSI_TIGHT_DB) * 80 / (BSD_RSSI_LOOSE_DB - BSD_RSSI_TIGHT_DB);

        int w2 = constrain(s.distinctMacs * 40 / BSD_MAC_SATURATE, 0, 40) * churnConfidence / 100;
        int score = w1 + w2;

        if (score > best.score) {
            best.score = score;
            best.worstCat = c;
            best.rate = rate;
            best.distinctMacs = s.distinctMacs;
        }
    }
    return best;
}

static void bsdUpdateState(int score) {
    uint32_t now = millis();
    if (score >= BSD_SUSPECT_SCORE) lastSuspectMs = now;

    if (score >= BSD_JAM_SCORE) {
        if (++highScoreStreak >= BSD_ALERT_ENTER_SWEEPS) currentState = BSD_STATE_JAMMING;
    } else {
        highScoreStreak = 0;
        if (currentState != BSD_STATE_JAMMING && score >= BSD_SUSPECT_SCORE) currentState = BSD_STATE_SUSPECT;
    }

    if (now - lastSuspectMs > BSD_ALERT_HOLD_MS) currentState = BSD_STATE_NORMAL;
}

static uint16_t bsdStateColor() {
    switch (currentState) {
        case BSD_STATE_JAMMING: return TFT_RED;
        case BSD_STATE_SUSPECT: return TFT_YELLOW;
        default: return TFT_GREEN;
    }
}

#define BSD_CONTENT_TOP 46 // below the title text (drawn by drawMainBorderWithTitle, ends ~y=44)
#define BSD_SIDE_PAD 8     // keep rows inside the border's side walls, same inset SpectrumPlot uses
#define BSD_BOTTOM_PAD 6   // keep the last row above the border's bottom wall

static void bsdDraw(const BsdResult &res) {
    uint16_t color = bsdStateColor();

    // FP, not FM: with a vendor name appended ("SPAM! Google FastPair 87%") this line can run
    // past 20 chars, which would overflow the screen width at FM.
    tft.fillRect(BSD_SIDE_PAD, BSD_CONTENT_TOP, tftWidth - 2 * BSD_SIDE_PAD, LH + 4, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setTextColor(color, bruceConfig.bgColor);
    char statusBuf[40];
    if (currentState == BSD_STATE_JAMMING && res.worstCat >= 0) {
        snprintf(statusBuf, sizeof(statusBuf), "SPAM! %s %d%%", BSD_NAMES[res.worstCat], res.score);
    } else {
        snprintf(
            statusBuf,
            sizeof(statusBuf),
            "%s %d%%",
            currentState == BSD_STATE_SUSPECT ? "Suspect" : "Normal",
            res.score
        );
    }
    tft.drawCentreString(statusBuf, tftWidth / 2, BSD_CONTENT_TOP, 1);

    // Rows prefer the bigger font (FM) when the screen has room for it, falling back to the
    // compact size (FP) on short screens like the Cardputer so the last row never runs past the
    // border's bottom wall.
    int rowsTop = BSD_CONTENT_TOP + LH + 12; // bigger gap below the status line, as requested
    int rowsAvail = tftHeight - BSD_BOTTOM_PAD - rowsTop;

    int rowSize = FM;
    int rowGap = 6;
    int rowH = LH * rowSize + rowGap;
    if (rowH * BSD_CATEGORIES > rowsAvail) {
        rowSize = FP;
        rowH = LH * rowSize + rowGap;
    }

    // No clearing fillRect here: rowBuf is fixed-width every frame, so the opaque drawString below
    // fully overwrites the previous frame's glyphs on its own — an extra full-row clear before that
    // just adds a visible blank-then-redraw flash on every refresh.
    tft.setTextSize(rowSize);
    uint32_t now = millis();
    for (int c = 0; c < BSD_CATEGORIES; c++) {
        BsdWindowStats s = bsdAnalyzeWindow(cat[c], now);
        int rate = s.events * 1000 / BSD_WINDOW_MS;
        bool flagged = (c == res.worstCat) && currentState != BSD_STATE_NORMAL;

        char rowBuf[24];
        snprintf(rowBuf, sizeof(rowBuf), "%-9s%3dp/s m%d", BSD_NAMES[c], rate, s.distinctMacs);
        tft.setTextColor(flagged ? color : bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawString(rowBuf, BSD_SIDE_PAD, rowsTop + c * rowH, 1);
    }
    tft.setTextSize(FP);
}

static const char *bsdStateLabel(const BsdResult &res) {
    if (currentState == BSD_STATE_JAMMING) return res.worstCat >= 0 ? BSD_NAMES[res.worstCat] : "SPAM!";
    return currentState == BSD_STATE_SUSPECT ? "Suspect" : "Normal";
}

// Shared by the on-screen menu entry (headless=false, runs until ESC) and the `bluefish blespam`
// serial command (headless=true, bounded duration, status printed to serialDevice instead of tft
// so it works the same over USB or the BLE serial bridge).
static void bsdRun(bool headless, uint32_t durationMs) {
    if (!ble_scan_setup()) return;

    memset(cat, 0, sizeof(cat));
    currentState = BSD_STATE_NORMAL;
    highScoreStreak = 0;
    lastSuspectMs = 0;

    if (!headless) drawMainBorderWithTitle("BLE Spam Detector");

    BsdResult res;
    uint32_t startMs = millis();
    uint32_t lastStatusBarMs = startMs;
    uint32_t lastReportMs = startMs;
    while (headless ? (millis() - startMs < durationMs) : true) {
        // Blocking, bounded scan chunk processed here on the main task — NimBLE's host task runs
        // with only a 4KB stack, too small for the classification/hashing work below, which is why
        // this uses getResults() instead of driving it from an onResult() callback.
        NimBLEScanResults results = pBLEScan->getResults(BSD_SCAN_CHUNK_MS, true);
        for (int i = 0; i < results.getCount(); i++) {
            const NimBLEAdvertisedDevice *dev = results.getDevice(i);
            int category = bsdClassify(dev);
            if (category >= 0)
                bsdRecordEvent(category, bsdHashMac(dev->getAddress().getVal()), dev->getRSSI());
        }
        // is_continue=true above keeps prior results instead of clearing them, so without this the
        // device list grows unbounded across chunks and eventually exhausts the heap — this is what
        // was crashing under real attack traffic (many more devices piling up per chunk).
        pBLEScan->clearResults();

        res = bsdComputeScore(millis());
        bsdUpdateState(res.score);

        if (headless) {
            if (millis() - lastReportMs >= 1000) {
                serialDevice->println(
                    "bluefish.blespam state=" + String(bsdStateLabel(res)) + " score=" + String(res.score)
                );
                lastReportMs = millis();
            }
        } else {
            bsdDraw(res);
            if (millis() - lastStatusBarMs >= 1000) {
                drawStatusBar();
                lastStatusBarMs = millis();
            }

            if (check(EscPress)) break;
        }
    }

    if (headless) {
        serialDevice->println(
            "bluefish.blespam done state=" + String(bsdStateLabel(res)) + " score=" + String(res.score)
        );
    }

    stopBLEStack();
}

void ble_spam_detector() { bsdRun(false, 0); }

void ble_spam_detector_headless(uint32_t durationMs) { bsdRun(true, durationMs); }
