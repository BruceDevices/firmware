#if !defined(LITE_VERSION)

#include "csi_radar.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/wifi/wifi_common.h"
#include <Arduino.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <globals.h>
#include <math.h>

// ============================================================================
// Constants
// ============================================================================
static constexpr int   kCsiWindow   = 50;
static constexpr int   kHistorySize = 240;   // 240-sample ring buffer for graph
static constexpr int   kMaxBlips    = 12;
static constexpr int   kHoldFrames  = 150;   // ~10 s at 15 Hz
static constexpr float kCsiThresh   = 0.15f;
static constexpr float kMergeThresh = 0.20f; // fraction of scope radius for blip merge

// Radar scope geometry (fits 240×135 Cardputer screen)
// Scope: 110×110 circle in left column; status panel in right column.
static constexpr int kScopeCx = 60;   // scope center x
static constexpr int kScopeCy = 70;   // scope center y
static constexpr int kScopeR  = 55;   // scope radius (left=5, right=115, top=15, bot=125)
static constexpr int kStatX   = 120;  // status panel left edge

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

// ============================================================================
// CSI Data Globals
// ============================================================================
static float             gCsiAmpBuf[kCsiWindow];
static float             gCsiPhaBuf[kCsiWindow];
static int               gCsiAmpIdx    = 0;
static int               gCsiAmpFilled = 0;
static volatile float    gCsiMotion    = 0.0f;
static volatile int8_t   gCsiRssi      = -60;
static volatile uint32_t gCsiCount     = 0;
static float             gCsiVarMax    = 0.001f;
static float             gCsiVarMin    = 0.0f;
static float             gCsiPhaVarMax = 0.001f;
static float             gCsiPhaVarMin = 0.0f;

// Motion history ring buffer
static float gMotionHistory[kHistorySize];
static int   gHistoryIdx = 0;

// Runtime state
static float    gThreshold        = 0.35f;
static float    gHeldMotion       = 0.0f;
static int      gHoldCnt          = 0;
static uint32_t gCsiLastServiceMs = 0;
static uint32_t gLastDrawMs       = 0;
static bool     gModePpi          = true;   // true = radar scope, false = text stats
static float    gSweepAngle       = 0.0f;

// Blips on radar (use active flag; avoids index-swap corruption)
struct Blip {
    float    angle;
    float    radius;
    float    strength;
    uint32_t born;
    bool     active;
};
static Blip gBlips[kMaxBlips];

// ============================================================================
// CSI Callback — ISR context (IRAM_ATTR)
// ============================================================================
static void IRAM_ATTR promiscuousRxCb(void*, wifi_promiscuous_pkt_type_t) {}

static void IRAM_ATTR csiCallback(void*, wifi_csi_info_t* info) {
    if (!info || !info->buf || info->len < 4) return;
    gCsiCount++;
    int8_t* b     = info->buf;
    int     nPairs = info->len / 2;

    // Single pass: amplitude + mean sin(phase)
    // Phase tracks slower motion that amplitude variance misses (40% weight).
    float ampSum = 0.0f, sinSum = 0.0f;
    int   validPairs = 0;
    for (int i = 0; i < nPairs; i++) {
        float r   = (float)b[2 * i];
        float im  = (float)b[2 * i + 1];
        float amp = sqrtf(r * r + im * im);
        ampSum += amp;
        if (amp > 1e-4f) { sinSum += im / amp; validPairs++; }
    }
    float meanAmp      = ampSum / (float)nPairs;
    float meanSinPhase = validPairs > 0 ? sinSum / (float)validPairs : 0.0f;

    gCsiAmpBuf[gCsiAmpIdx] = meanAmp;
    gCsiPhaBuf[gCsiAmpIdx] = meanSinPhase;
    gCsiAmpIdx = (gCsiAmpIdx + 1) % kCsiWindow;
    if (gCsiAmpFilled < kCsiWindow) gCsiAmpFilled++;
    int n = gCsiAmpFilled;

    // Amplitude variance over window
    float vsum = 0.0f;
    for (int i = 0; i < n; i++) vsum += gCsiAmpBuf[i];
    float vmean = vsum / (float)n;
    float var   = 0.0f;
    for (int i = 0; i < n; i++) { float d = gCsiAmpBuf[i] - vmean; var += d * d; }
    var /= (float)n;

    // Phase variance over window
    float psum = 0.0f;
    for (int i = 0; i < n; i++) psum += gCsiPhaBuf[i];
    float pmean = psum / (float)n;
    float pvar  = 0.0f;
    for (int i = 0; i < n; i++) { float d = gCsiPhaBuf[i] - pmean; pvar += d * d; }
    pvar /= (float)n;

    // Normalize amplitude: asymmetric EMA floor + running max → [0, 1]
    if (gCsiVarMin < 0.0001f) gCsiVarMin = var;
    else gCsiVarMin += (var - gCsiVarMin) * ((var < gCsiVarMin) ? 0.1f : 0.002f);
    if (var > gCsiVarMax) gCsiVarMax = var;
    else gCsiVarMax += (var - gCsiVarMax) * 0.005f;
    float range     = gCsiVarMax - gCsiVarMin;
    float ampMotion = (range > 0.0001f) ? ((var - gCsiVarMin) / range) : 0.0f;
    if (ampMotion < 0.0f) ampMotion = 0.0f;
    if (ampMotion > 1.0f) ampMotion = 1.0f;

    // Normalize phase variance: same approach
    if (gCsiPhaVarMin < 0.0001f) gCsiPhaVarMin = pvar;
    else gCsiPhaVarMin += (pvar - gCsiPhaVarMin) * ((pvar < gCsiPhaVarMin) ? 0.1f : 0.002f);
    if (pvar > gCsiPhaVarMax) gCsiPhaVarMax = pvar;
    else gCsiPhaVarMax += (pvar - gCsiPhaVarMax) * 0.005f;
    float prange    = gCsiPhaVarMax - gCsiPhaVarMin;
    float phaMotion = (prange > 0.0001f) ? ((pvar - gCsiPhaVarMin) / prange) : 0.0f;
    if (phaMotion < 0.0f) phaMotion = 0.0f;
    if (phaMotion > 1.0f) phaMotion = 1.0f;

    // Blend: 60% amplitude, 40% phase
    gCsiMotion = 0.6f * ampMotion + 0.4f * phaMotion;
    gCsiRssi   = info->rx_ctrl.rssi;
}

// ============================================================================
// Service CSI at ~15 Hz — hold/coast + blip management
// ============================================================================
static void serviceCsi() {
    uint32_t now = millis();
    if (now - gCsiLastServiceMs < 66) return;
    gCsiLastServiceMs = now;

    float m       = gCsiMotion;
    bool  present = false;

    if (m > kCsiThresh) {
        gHoldCnt    = kHoldFrames;
        gHeldMotion = m;
        present     = true;
    } else if (gHoldCnt > 0) {
        gHoldCnt--;
        float fade = (float)gHoldCnt / (float)kHoldFrames;
        m       = gHeldMotion * (0.10f + 0.90f * fade);
        present = true;
    } else {
        present = false;
        m       = 0.0f;
    }

    gMotionHistory[gHistoryIdx] = m;
    gHistoryIdx = (gHistoryIdx + 1) % kHistorySize;

    // Age out expired blips
    for (int i = 0; i < kMaxBlips; i++) {
        if (gBlips[i].active && (now - gBlips[i].born) > 15000) {
            gBlips[i].active = false;
        }
    }

    if (!present) return;

    // Map RSSI → scope radius:
    //   -45 dBm (strong/near) → kScopeR * 0.25  (inner ring)
    //   -78 dBm (weak/far)    → kScopeR * 0.90  (outer ring)
    float rssiT = ((float)gCsiRssi + 45.0f) / (-33.0f);
    if (rssiT < 0.0f) rssiT = 0.0f;
    if (rssiT > 1.0f) rssiT = 1.0f;
    float targetRad = kScopeR * (0.25f + rssiT * 0.65f);

    // Merge with the closest active blip within threshold; otherwise spawn new.
    int   closestIdx  = -1;
    float closestDist = kScopeR * kMergeThresh;
    for (int i = 0; i < kMaxBlips; i++) {
        if (!gBlips[i].active) continue;
        float d = fabsf(gBlips[i].radius - targetRad);
        if (d < closestDist) { closestDist = d; closestIdx = i; }
    }

    if (closestIdx >= 0) {
        // Refresh existing blip: EMA toward new measurements
        gBlips[closestIdx].born      = now;
        gBlips[closestIdx].radius   += (targetRad - gBlips[closestIdx].radius) * 0.05f;
        gBlips[closestIdx].strength += (m - gBlips[closestIdx].strength) * 0.12f;
    } else {
        // Find a free slot, or evict the oldest
        int slot = -1;
        for (int i = 0; i < kMaxBlips; i++) {
            if (!gBlips[i].active) { slot = i; break; }
        }
        if (slot < 0) {
            uint32_t oldest = UINT32_MAX;
            for (int i = 0; i < kMaxBlips; i++) {
                if (gBlips[i].born < oldest) { oldest = gBlips[i].born; slot = i; }
            }
        }
        if (slot >= 0) {
            gBlips[slot] = { gSweepAngle, targetRad, m, now, true };
        }
    }
}

// ============================================================================
// Enable / disable CSI
// ============================================================================
static bool enableCsi() {
    // Always reset buffers on enable so stale data from a previous run doesn't skew
    // the asymmetric EMA normalization.
    gCsiAmpIdx    = 0; gCsiAmpFilled = 0;
    gCsiVarMax    = 0.001f; gCsiVarMin    = 0.0f;
    gCsiPhaVarMax = 0.001f; gCsiPhaVarMin = 0.0f;
    memset(gCsiAmpBuf, 0, sizeof(gCsiAmpBuf));
    memset(gCsiPhaBuf, 0, sizeof(gCsiPhaBuf));

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousRxCb);

    wifi_csi_config_t cfg = {};
    cfg.lltf_en        = true;
    cfg.htltf_en       = true;
    cfg.stbc_htltf2_en = true;
    cfg.ltf_merge_en   = true;
    cfg.channel_filter_en = true;
    cfg.manu_scale     = false;
    cfg.shift          = 0;

    if (esp_wifi_set_csi_config(&cfg) != ESP_OK) {
        Serial.println("[CSI] Failed to set config");
        esp_wifi_set_promiscuous(false);
        return false;
    }

    esp_wifi_set_csi_rx_cb(csiCallback, nullptr);
    if (esp_wifi_set_csi(true) != ESP_OK) {
        Serial.println("[CSI] Failed to enable");
        esp_wifi_set_promiscuous(false);
        return false;
    }

    wifi_promiscuous_filter_t pf{};
    pf.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
    esp_wifi_set_promiscuous_filter(&pf);
    // Promiscuous rx cb already set above; no second registration.

    return true;
}

static void disableCsi() {
    esp_wifi_set_csi(false);
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
}

// ============================================================================
// Draw PPI radar scope (left panel, 120×135)
// ============================================================================
static void drawRadarScope() {
    uint32_t now = millis();

    // Range rings
    uint16_t ringOuter = rgb565(0, 70, 0);
    uint16_t ringMid   = rgb565(0, 50, 0);
    uint16_t ringInner = rgb565(0, 35, 0);
    tft.drawCircle(kScopeCx, kScopeCy, kScopeR,         ringOuter);
    tft.drawCircle(kScopeCx, kScopeCy, kScopeR * 2 / 3, ringMid);
    tft.drawCircle(kScopeCx, kScopeCy, kScopeR / 3,     ringInner);

    // Cardinal spokes
    uint16_t spokeCol = rgb565(0, 30, 0);
    tft.drawLine(kScopeCx, kScopeCy - kScopeR, kScopeCx, kScopeCy + kScopeR, spokeCol);
    tft.drawLine(kScopeCx - kScopeR, kScopeCy, kScopeCx + kScopeR, kScopeCy, spokeCol);

    // Sweep trail — 10 steps behind sweep, fading quadratically
    static constexpr int   kTrailSteps = 10;
    static constexpr float kTrailStep  = 3.14159f / 22.0f;  // ~8° per step
    for (int t = kTrailSteps; t >= 1; t--) {
        float ta   = gSweepAngle - t * kTrailStep;
        float fade = 1.0f - (float)t / (float)kTrailSteps;
        uint8_t g  = (uint8_t)(200.0f * fade * fade);
        int sx     = kScopeCx + (int)(sinf(ta) * kScopeR);
        int sy     = kScopeCy - (int)(cosf(ta) * kScopeR);
        tft.drawLine(kScopeCx, kScopeCy, sx, sy, rgb565(0, g, 0));
    }

    // Current sweep arm (bright green)
    {
        int sx = kScopeCx + (int)(sinf(gSweepAngle) * kScopeR);
        int sy = kScopeCy - (int)(cosf(gSweepAngle) * kScopeR);
        tft.drawLine(kScopeCx, kScopeCy, sx, sy, TFT_GREEN);
    }

    // Blips: color shifts from cyan (faded/weak) to yellow/red (fresh/strong)
    for (int i = 0; i < kMaxBlips; i++) {
        if (!gBlips[i].active) continue;
        uint32_t age = now - gBlips[i].born;
        if (age > 15000) continue;
        float ageFade = 1.0f - (float)age / 15000.0f;
        float bx_f    = kScopeCx + sinf(gBlips[i].angle) * gBlips[i].radius;
        float by_f    = kScopeCy - cosf(gBlips[i].angle) * gBlips[i].radius;
        int bx = (int)bx_f, by = (int)by_f;
        uint8_t intensity = (uint8_t)(255.0f * ageFade * gBlips[i].strength);
        // Cyan at low strength → yellow/red at high strength
        uint8_t r_c = intensity;
        uint8_t g_c = (uint8_t)(intensity * 0.5f + 60.0f * ageFade * (1.0f - gBlips[i].strength));
        uint8_t b_c = (uint8_t)(80.0f * ageFade * (1.0f - gBlips[i].strength));
        tft.fillCircle(bx, by, 3, rgb565(r_c, g_c, b_c));
        if (gBlips[i].strength > 0.5f && ageFade > 0.4f) {
            tft.drawCircle(bx, by, 5, rgb565(intensity / 2, intensity / 3, 0));
        }
    }

    // Center dot
    tft.fillCircle(kScopeCx, kScopeCy, 2, TFT_GREEN);
}

// ============================================================================
// Draw status panel (right of scope) — used in PPI mode
// ============================================================================
static void drawStatusPanel(bool present) {
    int sx = kStatX + 2;
    char buf[22];

    // Presence label
    tft.setTextSize(1);
    tft.setTextColor(present ? TFT_RED : TFT_GREEN);
    tft.drawString(present ? ">>CONTACT<<" : "~~CLEAR~~", sx, 4);

    // Motion value
    tft.setTextColor(TFT_WHITE);
    snprintf(buf, sizeof(buf), "M:%3d%%", (int)(gCsiMotion * 100));
    tft.drawString(buf, sx, 18);

    // Motion bar
    const int barW = tftWidth - sx - 4;
    const int barH = 5;
    const int barY = 28;
    tft.drawRect(sx, barY, barW, barH, rgb565(80, 80, 80));
    int fillW = (int)(gCsiMotion * (barW - 2));
    if (fillW > barW - 2) fillW = barW - 2;
    if (fillW > 0) {
        tft.fillRect(sx + 1, barY + 1, fillW, barH - 2, present ? TFT_RED : TFT_CYAN);
    }
    // Threshold tick on bar
    int tickX = sx + 1 + (int)(gThreshold * (barW - 2));
    if (tickX < sx + barW - 1) {
        tft.drawFastVLine(tickX, barY, barH, TFT_YELLOW);
    }

    // RSSI & threshold
    tft.setTextColor(TFT_CYAN);
    snprintf(buf, sizeof(buf), "R:%4ddBm", (int)gCsiRssi);
    tft.drawString(buf, sx, 38);

    tft.setTextColor(TFT_YELLOW);
    snprintf(buf, sizeof(buf), "T:%3d%%", (int)(gThreshold * 100));
    tft.drawString(buf, sx, 50);

    // Frame count
    tft.setTextColor(rgb565(90, 90, 90));
    snprintf(buf, sizeof(buf), "F:%lu", (unsigned long)gCsiCount);
    tft.drawString(buf, sx, 62);

    // Motion history mini-graph (fills remaining height above footer)
    const int gh  = tftHeight - 76 - 10;  // remaining height minus footer
    const int gw  = tftWidth - sx - 4;
    const int gy  = 74;
    tft.drawRect(sx, gy, gw, gh, rgb565(60, 60, 60));
    int thr_y = gy + gh - 1 - (int)(gThreshold * (gh - 2));
    tft.drawFastHLine(sx + 1, thr_y, gw - 2, TFT_YELLOW);
    for (int i = 0; i < gw - 2; i++) {
        int   histI = (gHistoryIdx - (gw - 2) + i + kHistorySize) % kHistorySize;
        float v     = gMotionHistory[histI];
        int   bh    = (int)(v * (gh - 2));
        if (bh <= 0) continue;
        if (bh > gh - 2) bh = gh - 2;
        tft.drawFastVLine(sx + 1 + i, gy + gh - 1 - bh, bh,
                          (v > gThreshold) ? TFT_RED : TFT_CYAN);
    }
}

// ============================================================================
// Draw full text stats (alternative view)
// ============================================================================
static void drawTextStats(bool present) {
    char buf[32];

    tft.setTextColor(TFT_CYAN);
    tft.setTextSize(2);
    tft.drawString("CSI RADAR", 10, 4);

    tft.setTextSize(1);

    tft.setTextColor(TFT_WHITE);
    snprintf(buf, sizeof(buf), "Motion:  %3d%%", (int)(gCsiMotion * 100));
    tft.drawString(buf, 10, 26);

    snprintf(buf, sizeof(buf), "Held:    %3d%%", (int)(gHeldMotion * 100));
    tft.drawString(buf, 10, 38);

    tft.setTextColor(TFT_YELLOW);
    snprintf(buf, sizeof(buf), "Threshold: %3d%%", (int)(gThreshold * 100));
    tft.drawString(buf, 10, 50);

    tft.setTextColor(TFT_CYAN);
    snprintf(buf, sizeof(buf), "RSSI:  %d dBm", (int)gCsiRssi);
    tft.drawString(buf, 10, 62);

    tft.setTextColor(rgb565(100, 100, 100));
    snprintf(buf, sizeof(buf), "Frames: %lu", (unsigned long)gCsiCount);
    tft.drawString(buf, 10, 74);

    int blipCount = 0;
    for (int i = 0; i < kMaxBlips; i++) if (gBlips[i].active) blipCount++;
    snprintf(buf, sizeof(buf), "Blips:  %d", blipCount);
    tft.drawString(buf, 10, 86);

    // Motion history graph
    const int gx = 10, gy = 100, gw = tftWidth - 20, gh = tftHeight - 112;
    tft.drawRect(gx, gy, gw, gh, rgb565(60, 60, 60));
    int thr_y = gy + gh - 1 - (int)(gThreshold * (gh - 2));
    tft.drawFastHLine(gx + 1, thr_y, gw - 2, TFT_YELLOW);
    for (int i = 0; i < gw - 2; i++) {
        int   histI = (gHistoryIdx - (gw - 2) + i + kHistorySize) % kHistorySize;
        float v     = gMotionHistory[histI];
        int   bh    = (int)(v * (gh - 2));
        if (bh <= 0) continue;
        if (bh > gh - 2) bh = gh - 2;
        tft.drawFastVLine(gx + 1 + i, gy + gh - 1 - bh, bh,
                          (v > gThreshold) ? TFT_RED : TFT_CYAN);
    }

    // Presence indicator (top-right, beside title)
    tft.setTextSize(1);
    tft.setTextColor(present ? TFT_RED : TFT_GREEN);
    tft.drawString(present ? "CONTACT" : "CLEAR", tftWidth - 50, 8);
}

// ============================================================================
// Top-level draw dispatch
// ============================================================================
static void drawScreen() {
    uint32_t now = millis();
    if (now - gLastDrawMs < 125) return;
    gLastDrawMs = now;

    bool present = gHoldCnt > 0 || gCsiMotion > kCsiThresh;

    tft.fillScreen(TFT_BLACK);

    if (gModePpi) {
        drawRadarScope();
        drawStatusPanel(present);
    } else {
        drawTextStats(present);
    }

    // Footer key hints (full width, bottom)
    tft.setTextColor(rgb565(70, 70, 70));
    tft.setTextSize(1);
    tft.drawString("NXT/PRV:thr  SEL:mode  ESC:exit", 4, tftHeight - 9);
}

// ============================================================================
// Main entry point
// ============================================================================
void csi_radar_setup() {
    returnToMenu = false;

    {
        Preferences prefs;
        prefs.begin("csiRadar", true);
        gThreshold = prefs.getFloat("thr", 0.35f);
        gModePpi   = prefs.getBool("ppi", true);
        prefs.end();
    }

    // Initialize runtime state
    gCsiMotion        = 0.0f;
    gCsiRssi          = -60;
    gHoldCnt          = 0;
    gHeldMotion       = 0.0f;
    gSweepAngle       = 0.0f;
    gLastDrawMs       = 0;
    gCsiLastServiceMs = 0;
    gHistoryIdx       = 0;
    memset(gMotionHistory, 0, sizeof(gMotionHistory));
    for (int i = 0; i < kMaxBlips; i++) gBlips[i].active = false;

    drawMainBorderWithTitle("CSI RADAR");
    tft.setTextColor(TFT_WHITE);
    padprintln("Initializing WiFi...");

    if (!wifiConnected) {
        if (bruceConfig.wifi.size() > 0) {
            auto   it   = bruceConfig.wifi.begin();
            String ssid = it->first;
            String pass = it->second;
            padprintln("Connecting: " + ssid);
            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid.c_str(), pass.c_str());
            int timeout = 50;
            while (WiFi.status() != WL_CONNECTED && timeout--) {
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            if (WiFi.status() != WL_CONNECTED) {
                displayError("WiFi connect failed", true);
                return;
            }
        } else {
            padprintln("No saved WiFi. Opening menu...");
            if (!wifiConnectMenu()) {
                displayError("WiFi connect cancelled", true);
                return;
            }
        }
    }

    padprintln("Enabling CSI...");
    if (!enableCsi()) {
        displayError("CSI init failed", true);
        return;
    }
    padprintln("CSI Radar active!");
    vTaskDelay(pdMS_TO_TICKS(500));

    uint32_t lastSweepMs = 0;

    while (!returnToMenu) {
        if (check(EscPress)) break;
        if (check(SelPress)) gModePpi = !gModePpi;
        if (check(NextPress)) {
            gThreshold += 0.05f;
            if (gThreshold > 0.95f) gThreshold = 0.95f;
        }
        if (check(PrevPress)) {
            gThreshold -= 0.05f;
            if (gThreshold < 0.05f) gThreshold = 0.05f;
        }

        serviceCsi();

        // Advance sweep at ~0.12 rad/frame tied to the 8 fps draw rate
        uint32_t now = millis();
        if (now - lastSweepMs >= 125) {
            gSweepAngle += 0.12f;
            if (gSweepAngle >= 2.0f * (float)M_PI) gSweepAngle -= 2.0f * (float)M_PI;
            lastSweepMs = now;
        }

        drawScreen();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    disableCsi();

    {
        Preferences prefs;
        prefs.begin("csiRadar", false);
        prefs.putFloat("thr", gThreshold);
        prefs.putBool("ppi", gModePpi);
        prefs.end();
    }
}

#endif
