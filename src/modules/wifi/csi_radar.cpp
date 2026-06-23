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
// CSI Data Collection Globals
// ============================================================================
static constexpr int kCsiWindow = 50;
static float gCsiAmpBuf[kCsiWindow];
static float gCsiPhaBuf[kCsiWindow];
static int gCsiAmpIdx = 0;
static int gCsiAmpFilled = 0;
static volatile float gCsiMotion = 0.0f;
static volatile int8_t gCsiRssi = -80;
static volatile uint32_t gCsiCount = 0;
static float gCsiVarMax = 0.001f;
static float gCsiVarMin = 0.0f;
static float gCsiPhaVarMax = 0.001f;
static float gCsiPhaVarMin = 0.0f;
static const float kCsiThresh = 0.15f;

// Motion history for bar graph (100 samples)
static constexpr int kHistorySize = 100;
static float gMotionHistory[kHistorySize];
static int gHistoryIdx = 0;

// Runtime state
static float gThreshold = 0.35f;
static float gHeldMotion = 0.0f;
static int gHoldCnt = 0;
static const int kHoldFrames = 150;  // ~10 seconds at 15 Hz
static uint32_t gCsiLastServiceMs = 0;
static uint32_t gLastDrawMs = 0;
static bool gModePpi = true;  // true = radar, false = text stats

// Contact blips on radar
struct Blip {
    float angle;
    float radius;
    float strength;
    uint32_t born;
};
static constexpr int kMaxBlips = 8;
static Blip gBlips[kMaxBlips];
static int gBlipCount = 0;
static float gSweepAngle = 0.0f;

// ============================================================================
// CSI Callback — ISR context (IRAM_ATTR)
// ============================================================================
static void IRAM_ATTR promiscuousRxCb(void*, wifi_promiscuous_pkt_type_t) {}

static void IRAM_ATTR csiCallback(void*, wifi_csi_info_t* info) {
    if (!info || !info->buf || info->len < 4) return;
    gCsiCount = gCsiCount + 1;
    int8_t* b = info->buf;
    int nPairs = info->len / 2;

    // Single pass: amplitude + mean sin(phase)
    float ampSum = 0.0f, sinSum = 0.0f;
    int validPairs = 0;
    for (int i = 0; i < nPairs; i++) {
        float r = (float)b[2*i];
        float im = (float)b[2*i + 1];
        float amp = sqrtf(r*r + im*im);
        ampSum += amp;
        if (amp > 1e-4f) { sinSum += im / amp; validPairs++; }
    }
    float meanAmp = ampSum / (float)nPairs;
    float meanSinPhase = validPairs > 0 ? sinSum / (float)validPairs : 0.0f;

    gCsiAmpBuf[gCsiAmpIdx] = meanAmp;
    gCsiPhaBuf[gCsiAmpIdx] = meanSinPhase;
    gCsiAmpIdx = (gCsiAmpIdx + 1) % kCsiWindow;
    if (gCsiAmpFilled < kCsiWindow) gCsiAmpFilled++;
    int n = gCsiAmpFilled;

    // Amplitude variance
    float vsum = 0.0f;
    for (int i = 0; i < n; i++) vsum += gCsiAmpBuf[i];
    float vmean = vsum / (float)n;
    float var = 0.0f;
    for (int i = 0; i < n; i++) { float d = gCsiAmpBuf[i] - vmean; var += d*d; }
    var /= (float)n;

    // Phase variance
    float psum = 0.0f;
    for (int i = 0; i < n; i++) psum += gCsiPhaBuf[i];
    float pmean = psum / (float)n;
    float pvar = 0.0f;
    for (int i = 0; i < n; i++) { float d = gCsiPhaBuf[i] - pmean; pvar += d*d; }
    pvar /= (float)n;

    // Normalize amplitude: asymmetric EMA
    if (gCsiVarMin < 0.0001f) gCsiVarMin = var;
    else gCsiVarMin += (var - gCsiVarMin) * ((var < gCsiVarMin) ? 0.1f : 0.002f);
    if (var > gCsiVarMax) gCsiVarMax = var;
    else gCsiVarMax += (var - gCsiVarMax) * 0.005f;
    float range = gCsiVarMax - gCsiVarMin;
    float ampMotion = (range > 0.0001f) ? ((var - gCsiVarMin) / range) : 0.0f;
    if (ampMotion < 0.0f) ampMotion = 0.0f;
    if (ampMotion > 1.0f) ampMotion = 1.0f;

    // Normalize phase: same approach
    if (gCsiPhaVarMin < 0.0001f) gCsiPhaVarMin = pvar;
    else gCsiPhaVarMin += (pvar - gCsiPhaVarMin) * ((pvar < gCsiPhaVarMin) ? 0.1f : 0.002f);
    if (pvar > gCsiPhaVarMax) gCsiPhaVarMax = pvar;
    else gCsiPhaVarMax += (pvar - gCsiPhaVarMax) * 0.005f;
    float prange = gCsiPhaVarMax - gCsiPhaVarMin;
    float phaMotion = (prange > 0.0001f) ? ((pvar - gCsiPhaVarMin) / prange) : 0.0f;
    if (phaMotion < 0.0f) phaMotion = 0.0f;
    if (phaMotion > 1.0f) phaMotion = 1.0f;

    // Blend: 60% amplitude, 40% phase
    gCsiMotion = 0.6f * ampMotion + 0.4f * phaMotion;
    gCsiRssi = info->rx_ctrl.rssi;
}

// ============================================================================
// Service CSI at ~15 Hz - hold/coast logic
// ============================================================================
static void serviceCsi() {
    uint32_t now = millis();
    if (now - gCsiLastServiceMs < 66) return;  // ~15 Hz
    gCsiLastServiceMs = now;

    float m = gCsiMotion;
    bool present = false;

    // Hold/coast: decay over 150 frames
    if (m > kCsiThresh) {
        gHoldCnt = kHoldFrames;
        gHeldMotion = m;
        present = true;
    } else if (gHoldCnt > 0) {
        gHoldCnt--;
        float fade = gHoldCnt / (float)kHoldFrames;
        m = gHeldMotion * (0.10f + 0.90f * fade);
        present = true;
    } else {
        present = false;
        m = 0.0f;
    }

    // Add to history
    gMotionHistory[gHistoryIdx] = m;
    gHistoryIdx = (gHistoryIdx + 1) % kHistorySize;

    // Manage blips
    if (present && gBlipCount < kMaxBlips) {
        gBlips[gBlipCount].angle = gSweepAngle;
        gBlips[gBlipCount].radius = 65.0f - (gCsiRssi + 30) * 0.5f;  // map(-90..-30, 65..10)
        gBlips[gBlipCount].strength = m;
        gBlips[gBlipCount].born = millis();
        gBlipCount++;
    }

    // Age out old blips (120 frames ~ 8 sec at 15 Hz)
    uint32_t now_ms = millis();
    for (int i = 0; i < gBlipCount; i++) {
        uint32_t age_ms = now_ms - gBlips[i].born;
        if (age_ms > 8000) {  // 8 seconds
            gBlips[i] = gBlips[gBlipCount - 1];
            gBlipCount--;
            i--;
        }
    }
}

// ============================================================================
// Enable CSI sensing
// ============================================================================
static bool enableCsi() {
    // WiFi already should be up (caller handles connection)
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousRxCb);

    wifi_csi_config_t cfg = {};
    cfg.lltf_en = true;
    cfg.htltf_en = true;
    cfg.stbc_htltf2_en = true;
    cfg.ltf_merge_en = true;
    cfg.channel_filter_en = true;
    cfg.manu_scale = false;
    cfg.shift = 0;

    esp_err_t ret = esp_wifi_set_csi_config(&cfg);
    if (ret != ESP_OK) {
        Serial.printf("[CSI] Failed to set CSI config: %d\n", ret);
        esp_wifi_set_promiscuous(false);
        return false;
    }

    esp_wifi_set_csi_rx_cb(csiCallback, nullptr);
    ret = esp_wifi_set_csi(true);
    if (ret != ESP_OK) {
        Serial.printf("[CSI] Failed to enable CSI: %d\n", ret);
        esp_wifi_set_promiscuous(false);
        return false;
    }

    wifi_promiscuous_filter_t pf{};
    pf.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
    esp_wifi_set_promiscuous_filter(&pf);
    esp_wifi_set_promiscuous_rx_cb(promiscuousRxCb);

    return true;
}

// ============================================================================
// Disable CSI sensing
// ============================================================================
static void disableCsi() {
    esp_wifi_set_csi(false);
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
}

// ============================================================================
// Draw radar scope (PPI mode) - TODO: implement when text is working
// ============================================================================
static void drawRadarScope() {
    // Placeholder - focus on text display first
    tft.setTextColor(TFT_MAGENTA);
    tft.setTextSize(2);
    tft.drawString("RADAR MODE", 10, 30);
}

// ============================================================================
// Draw screen (simplified for clarity)
// ============================================================================
static void drawScreen() {
    uint32_t now = millis();
    if (now - gLastDrawMs < 125) return;  // ~8 fps
    gLastDrawMs = now;

    // Clear screen
    tft.fillScreen(TFT_BLACK);

    // Header
    tft.setTextColor(TFT_CYAN);
    tft.setTextSize(2);
    tft.drawString("CSI RADAR", 10, 5);

    // Status line 1: Motion value
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    int motionPercent = (int)(gCsiMotion * 100.0f);
    tft.drawString("Motion: " + String(motionPercent) + "%", 10, 30);

    // Status line 2: Threshold
    tft.setTextSize(2);
    int thrPercent = (int)(gThreshold * 100.0f);
    tft.drawString("Thr: " + String(thrPercent) + "%", 10, 55);

    // Status line 3: RSSI
    tft.setTextSize(2);
    tft.drawString("RSSI: " + String(gCsiRssi) + "dBm", 10, 80);

    // Status line 4: Frame count
    tft.setTextSize(2);
    tft.drawString("Frames: " + String(gCsiCount), 10, 105);

    // Status line 5: Blips
    tft.setTextSize(2);
    tft.drawString("Blips: " + String(gBlipCount), 10, 130);

    // Presence indicator
    bool present = gHoldCnt > 0 || gCsiMotion > kCsiThresh;
    tft.setTextColor(present ? TFT_RED : TFT_GREEN);
    tft.setTextSize(2);
    String presenceStr = present ? "CONTACT" : "CLEAR";
    tft.drawString(presenceStr, 200, 30);

    // Footer
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(1);
    tft.drawString("ENC:thr +/-  SEL:mode  ESC:exit", 10, 155);
}

// ============================================================================
// Main setup function
// ============================================================================
void csi_radar_setup() {
    returnToMenu = false;

    // Load settings
    {
        Preferences prefs;
        prefs.begin("csiRadar", true);
        gThreshold = prefs.getFloat("thr", 0.35f);
        prefs.end();
    }

    // Initialize globals
    gCsiMotion = 0.0f;
    gCsiRssi = -80;
    gHoldCnt = 0;
    gBlipCount = 0;
    gSweepAngle = 0.0f;
    gLastDrawMs = 0;
    gCsiLastServiceMs = 0;
    memset(gMotionHistory, 0, sizeof(gMotionHistory));

    drawMainBorderWithTitle("CSI RADAR");
    tft.setTextColor(TFT_WHITE);
    padprintln("Initializing WiFi...");

    // Ensure WiFi is connected
    if (!wifiConnected) {
        if (bruceConfig.wifi.size() > 0) {
            auto it = bruceConfig.wifi.begin();
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

    // Main loop
    uint32_t lastSweepUpdateMs = 0;
    while (!returnToMenu) {
        if (check(EscPress)) {
            returnToMenu = true;
            break;
        }
        if (check(SelPress)) {
            gModePpi = !gModePpi;
        }
        if (check(NextPress)) {
            gThreshold += 0.05f;
            if (gThreshold > 0.95f) gThreshold = 0.95f;
        }
        if (check(PrevPress)) {
            gThreshold -= 0.05f;
            if (gThreshold < 0.05f) gThreshold = 0.05f;
        }

        // Service CSI at ~15 Hz
        serviceCsi();

        // Update sweep angle (~0.12 rad/frame at ~8 fps)
        uint32_t now = millis();
        if (now - lastSweepUpdateMs > 125) {  // ~8 fps
            gSweepAngle += 0.12f;
            if (gSweepAngle > 2.0f * M_PI) gSweepAngle -= 2.0f * M_PI;
            lastSweepUpdateMs = now;
        }

        // Draw screen
        drawScreen();

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Cleanup
    disableCsi();

    // Save settings
    {
        Preferences prefs;
        prefs.begin("csiRadar", false);
        prefs.putFloat("thr", gThreshold);
        prefs.end();
    }

    // Leave WiFi as found (don't disconnect)
}

#endif
