#if !defined(LITE_VERSION)
// Bruce Jam Detector (WiFiGuardian)
// Monitors deauth/disassoc/beacon rates via promiscuous mode.
// Calibrates baseline, then flags anomalies as threat levels.
// Solid fills (no per-pixel gradients), green/yellow/red threat colors.

#include "jam_detect.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include <globals.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ─── Constants ────────────────────────────────────────────────
#define JAM_WIFI_CAL_MS        5000
#define JAM_DEAUTH_SUS_DELTA   5
#define JAM_DEAUTH_JAM_DELTA   20
#define JAM_BCN_SUS_MULT       3
#define JAM_BCN_JAM_MULT       10
#define JAM_CLEAR_TIMEOUT_MS   3000
#define JAM_NUM_CHANNELS       3
#define JAM_HOP_INTERVAL_MS    500
#define JAM_MAX_EVENTS         5

static const uint8_t jamHopChannels[JAM_NUM_CHANNELS] = {1, 6, 11};

// Threat-level colors (green→yellow→red)
static uint16_t jamThreatColor(float t) {
    if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;
    uint8_t r, g, b;
    if (t <= 0.5f) {
        float s = t / 0.5f;
        r = (uint8_t)(50  + s * (255 - 50));
        g = (uint8_t)(200 - s * (200 - 200));
        b = (uint8_t)(50  - s * 50);
    } else {
        float s = (t - 0.5f) / 0.5f;
        r = 255;
        g = (uint8_t)(200 - s * 200);
        b = (uint8_t)(0   + s * 30);
    }
    return tft.color565(r, g, b);
}

// ─── Threat levels ───────────────────────────────────────────
enum JamThreat {
    THREAT_CALIBRATING,
    THREAT_CLEAR,
    THREAT_SUSPICIOUS,
    THREAT_JAMMING
};

static const char* jamThreatText(JamThreat l) {
    switch (l) {
        case THREAT_CALIBRATING: return "CALIBRATING";
        case THREAT_CLEAR:       return "CLEAR";
        case THREAT_SUSPICIOUS:  return "SUSPICIOUS";
        case THREAT_JAMMING:     return "JAMMED!";
        default:                 return "UNKNOWN";
    }
}

// ─── State ────────────────────────────────────────────────────
static volatile uint32_t jamDeauthCnt = 0;
static volatile uint32_t jamDisassocCnt = 0;
static volatile uint32_t jamBeaconCnt = 0;
static volatile int32_t  jamLastRssi = 0;

static uint32_t jamPrevDeauth = 0;
static uint32_t jamPrevDisassoc = 0;
static uint32_t jamPrevBeacon = 0;
static uint32_t jamDeauthRate = 0;
static uint32_t jamDisassocRate = 0;
static uint32_t jamBeaconRate = 0;

static uint32_t jamBaseDeauth = 0;
static uint32_t jamBaseDisassoc = 0;
static uint32_t jamBaseBeacon = 1;
static uint32_t jamCalDeauthSum = 0;
static uint32_t jamCalDisassocSum = 0;
static uint32_t jamCalBeaconSum = 0;
static uint32_t jamCalSamples = 0;

static uint8_t jamCurChannel = 1;
static uint8_t jamHopIdx = 0;

static JamThreat jamThreat = THREAT_CALIBRATING;
static unsigned long jamCalStart = 0;
static unsigned long jamClearTimer = 0;
static unsigned long jamLastHop = 0;
static unsigned long jamLastRate = 0;
static unsigned long jamLastDraw = 0;

// Event ring buffer
struct JamEvent { unsigned long ts; char msg[27]; };
static JamEvent jamEvents[JAM_MAX_EVENTS];
static int jamEvtHead = 0;
static int jamEvtCount = 0;

static void jamAddEvent(const char* msg) {
    JamEvent& e = jamEvents[jamEvtHead];
    e.ts = millis() / 1000;
    strncpy(e.msg, msg, 26); e.msg[26] = '\0';
    jamEvtHead = (jamEvtHead + 1) % JAM_MAX_EVENTS;
    if (jamEvtCount < JAM_MAX_EVENTS) jamEvtCount++;
}

// ─── Promiscuous callback ────────────────────────────────────
static void IRAM_ATTR jamPromiscCB(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    jamLastRssi = pkt->rx_ctrl.rssi;
    uint8_t ft = pkt->payload[0];
    if (ft == 0xA0) jamDeauthCnt++;
    else if (ft == 0xC0) jamDisassocCnt++;
    else if (ft == 0x80) jamBeaconCnt++;
}

// ─── Draw solid rate bar ─────────────────────────────────────
static void jamDrawRateBar(int y, uint32_t rate, uint32_t base, const char* label) {
    int SCW = tft.width();
    int barX = 8, barW = SCW - 16, barH = 12;
    bool elevated = (rate > base + JAM_DEAUTH_SUS_DELTA);
    float severity = (base > 0) ? constrain((float)(rate - base) / (float)(base * 8 + 1), 0.0f, 1.0f) : 0.0f;

    tft.setTextColor(elevated ? TFT_WHITE : bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(barX, y + 1);
    tft.print(label);
    int bx = barX + 56;
    int bw = barW - 56;
    uint16_t barColor = elevated ? jamThreatColor(severity) : bruceConfig.priColor;
    uint32_t maxR = max((uint32_t)50, base * 15);
    int fillW = constrain((int)(rate * (bw - 2) / maxR), 0, bw - 2);
    tft.drawRect(bx, y, bw, barH, elevated ? TFT_WHITE : bruceConfig.priColor);
    if (fillW > 0) tft.fillRect(bx + 1, y + 1, fillW, barH - 2, barColor);
    if (fillW < bw - 2)
        tft.fillRect(bx + 1 + fillW, y + 1, bw - 2 - fillW, barH - 2, bruceConfig.bgColor);

    char buf[12];
    snprintf(buf, sizeof(buf), "%lu/s", (unsigned long)rate);
    tft.setTextColor(elevated ? TFT_WHITE : bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(barX + barW - 36, y + 2);
    tft.print(buf);
}

// ─── Draw threat status bar ──────────────────────────────────
static void jamDrawThreatBar(int y, JamThreat level) {
    int SCW = tft.width();
    int barX = 4, barW = SCW - 8, barH = 16;
    uint16_t baseColor;
    switch (level) {
        case THREAT_CLEAR:      baseColor = tft.color565(50, 200, 50);  break;  // green
        case THREAT_SUSPICIOUS: baseColor = tft.color565(255, 200, 30); break;  // yellow
        case THREAT_JAMMING:    baseColor = tft.color565(220, 40, 40);  break;  // red
        default:                baseColor = bruceConfig.priColor;        break;
    }
    tft.fillRect(barX, y, barW, barH, baseColor);
    tft.drawRect(barX, y, barW, barH, TFT_WHITE);

    const char* txt = jamThreatText(level);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(FP);
    tft.setCursor(barX + 4, y + 2);
    tft.print(txt);
}

// ─── Draw RSSI meter (solid bars, no wave animation) ────────
static void jamDrawRssiMeter(int y, int rssi) {
    int bars = 8;
    int bw = 12, spacing = 17, maxH = 18, minH = 3;
    int pct = constrain(map(rssi, -90, -30, 0, 100), 0, 100);
    for (int i = 0; i < bars; i++) {
        int bx = 8 + i * spacing;
        int bh = minH + (i * (maxH - minH)) / (bars - 1);
        int by = y + maxH - bh;
        int thresh = (i + 1) * 100 / bars;
        if (pct >= thresh) {
            tft.fillRect(bx, by, bw, bh, jamThreatColor((float)i / (bars - 1)));
            tft.drawRect(bx, by, bw, bh, TFT_WHITE);
        } else {
            tft.fillRect(bx, by, bw, bh, bruceConfig.bgColor);
            tft.drawRect(bx, by, bw, bh, bruceConfig.priColor);
        }
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%d dBm", rssi);
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(8 + bars * spacing + 4, y + 3);
    tft.print(buf);
}

// ═══════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════

void jam_detect_menu() {
    // ── Init state ──
    jamDeauthCnt = jamDisassocCnt = jamBeaconCnt = 0;
    jamPrevDeauth = jamPrevDisassoc = jamPrevBeacon = 0;
    jamDeauthRate = jamDisassocRate = jamBeaconRate = 0;
    jamCalDeauthSum = jamCalDisassocSum = jamCalBeaconSum = 0;
    jamCalSamples = 0;
    jamBaseDeauth = jamBaseDisassoc = 0; jamBaseBeacon = 1;
    jamEvtHead = jamEvtCount = 0;

    jamThreat = THREAT_CALIBRATING;
    jamHopIdx = 0;
    jamCurChannel = jamHopChannels[0];

    // ── Init WiFi promiscuous ──
    WiFi.mode(WIFI_OFF);
    delay(50);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t wr = esp_wifi_init(&cfg);
    if (wr != ESP_OK && wr != ESP_ERR_WIFI_INIT_STATE) ESP_ERROR_CHECK(wr);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_channel(jamCurChannel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(jamPromiscCB);

    // ── Draw UI ──
    drawMainBorderWithTitle("Jam Detector", true);

    char buf[32];
    unsigned long now;
    unsigned long startTime = millis();
    jamCalStart = startTime;
    jamLastHop = startTime;
    jamLastRate = startTime;
    jamLastDraw = 0;

    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(8, tft.height() - 10);
    tft.print("[Esc/Sel] Exit");

    // ── Main loop ──
    while (!check(EscPress) && !check(SelPress)) {
        now = millis();

        // Channel hop ~500ms
        if (now - jamLastHop >= JAM_HOP_INTERVAL_MS) {
            jamHopIdx = (jamHopIdx + 1) % JAM_NUM_CHANNELS;
            jamCurChannel = jamHopChannels[jamHopIdx];
            esp_wifi_set_channel(jamCurChannel, WIFI_SECOND_CHAN_NONE);
            jamLastHop = now;
        }

        // Rate calculation ~1s
        if (now - jamLastRate >= 1000) {
            jamDeauthRate = jamDeauthCnt - jamPrevDeauth;
            jamDisassocRate = jamDisassocCnt - jamPrevDisassoc;
            jamBeaconRate = jamBeaconCnt - jamPrevBeacon;
            jamPrevDeauth = jamDeauthCnt;
            jamPrevDisassoc = jamDisassocCnt;
            jamPrevBeacon = jamBeaconCnt;
            jamLastRate = now;

            // Calibration phase
            if (jamThreat == THREAT_CALIBRATING) {
                jamCalDeauthSum += jamDeauthRate;
                jamCalDisassocSum += jamDisassocRate;
                jamCalBeaconSum += jamBeaconRate;
                jamCalSamples++;

                int pct = constrain((int)((now - jamCalStart) * 100 / JAM_WIFI_CAL_MS), 0, 100);

                // Draw calibration progress bar
                int barX = 20, barW = tft.width() - 40, barY = 50;
                tft.drawRect(barX - 1, barY - 1, barW + 2, 12, bruceConfig.priColor);
                int fillW = (barW * pct) / 100;
                tft.fillRect(barX, barY, fillW, 10, jamThreatColor(0.0f));
                if (fillW < barW)
                    tft.fillRect(barX + fillW, barY, barW - fillW, 10, bruceConfig.bgColor);

                tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
                tft.setTextSize(FM);
                tft.fillRect(8, 46, tft.width() - 16, 30, bruceConfig.bgColor);
                tft.setCursor(8, 46);
                tft.print("Learning baseline...");
                tft.setCursor(8, 68);
                tft.printf("%d%%", pct);

                if (pct >= 100 && jamCalSamples > 0) {
                    jamBaseDeauth = jamCalDeauthSum / jamCalSamples;
                    jamBaseDisassoc = jamCalDisassocSum / jamCalSamples;
                    jamBaseBeacon = max(jamCalBeaconSum / jamCalSamples, (uint32_t)1);
                    jamThreat = THREAT_CLEAR;
                    jamClearTimer = now;
                    jamAddEvent("Baseline learned");
                }
            } else {
                // Monitoring — assess threat
                JamThreat newThreat = THREAT_CLEAR;
                uint32_t atkRate = jamDeauthRate + jamDisassocRate;
                uint32_t atkBase = jamBaseDeauth + jamBaseDisassoc;

                if (atkRate > atkBase + JAM_DEAUTH_JAM_DELTA) {
                    newThreat = THREAT_JAMMING;
                    snprintf(buf, sizeof(buf), "Deauth %lu/s ch%d", (unsigned long)atkRate, jamCurChannel);
                    jamAddEvent(buf);
                } else if (atkRate > atkBase + JAM_DEAUTH_SUS_DELTA) {
                    newThreat = THREAT_SUSPICIOUS;
                    snprintf(buf, sizeof(buf), "Deauth %lu/s ch%d", (unsigned long)atkRate, jamCurChannel);
                    jamAddEvent(buf);
                }

                if (jamBeaconRate > jamBaseBeacon * JAM_BCN_JAM_MULT) {
                    newThreat = THREAT_JAMMING;
                    snprintf(buf, sizeof(buf), "Bcn flood %lu/s", (unsigned long)jamBeaconRate);
                    jamAddEvent(buf);
                } else if (jamBeaconRate > jamBaseBeacon * JAM_BCN_SUS_MULT && newThreat < THREAT_SUSPICIOUS) {
                    newThreat = THREAT_SUSPICIOUS;
                }

                if (newThreat > THREAT_CLEAR) {
                    jamThreat = newThreat;
                    jamClearTimer = now;
                } else if (jamThreat > THREAT_CLEAR && now - jamClearTimer >= JAM_CLEAR_TIMEOUT_MS) {
                    jamThreat = THREAT_CLEAR;
                    jamAddEvent("Threat cleared");
                }
            }
        }

        // Draw main UI (non-calibration)
        if (now - jamLastDraw >= 200 && jamThreat != THREAT_CALIBRATING) {
            jamLastDraw = now;
            int SCW = tft.width();
            int y = 44;

            tft.fillRect(0, 44, SCW, tft.height() - 54, bruceConfig.bgColor);

            // Threat bar
            jamDrawThreatBar(y, jamThreat);
            y += 20;

            // DEAUTH
            jamDrawRateBar(y, jamDeauthRate, jamBaseDeauth, "DEAUTH");
            y += 14;

            // DISASSOC
            jamDrawRateBar(y, jamDisassocRate, jamBaseDisassoc, "DISASSC");
            y += 14;

            // BEACON
            jamDrawRateBar(y, jamBeaconRate, jamBaseBeacon, "BEACON");
            y += 16;

            // Divider
            tft.drawFastHLine(4, y, SCW - 8, bruceConfig.priColor);
            y += 4;

            // Channel + RSSI
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setTextSize(FP);
            tft.setCursor(8, y);
            tft.printf("Ch:%d RSSI:%d dBm", jamCurChannel, (int)jamLastRssi);
            y += 10;

            // RSSI meter (solid bars)
            jamDrawRssiMeter(y, (int)jamLastRssi);
            y += 22;

            // Event log
            tft.setTextSize(FP);
            int idx = (jamEvtHead - jamEvtCount + JAM_MAX_EVENTS) % JAM_MAX_EVENTS;
            for (int i = 0; i < JAM_MAX_EVENTS; i++) {
                tft.fillRect(4, y + i * 12, SCW - 8, 11, bruceConfig.bgColor);
                if (i < jamEvtCount) {
                    JamEvent& e = jamEvents[(idx + jamEvtCount - 1 - i) % JAM_MAX_EVENTS];
                    tft.setTextColor(jamThreat >= THREAT_SUSPICIOUS ? TFT_WHITE : bruceConfig.priColor, bruceConfig.bgColor);
                    tft.setCursor(4, y + i * 12);
                    tft.printf("[%lus] %s", (unsigned long)e.ts, e.msg);
                }
            }
        }

        delay(5);
    }

    // ── Cleanup ──
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    // Don't call esp_wifi_deinit() — wifiDisconnect() needs the driver alive
}
#endif
