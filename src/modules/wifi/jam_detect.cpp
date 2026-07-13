#if !defined(LITE_VERSION)
#include "jam_detect.h"

#include "esp_err.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/wifi/wifi_common.h"
#include <Arduino.h>
#include <globals.h>

static const uint8_t JD_CHANNELS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
static const int JD_NCH = sizeof(JD_CHANNELS) / sizeof(JD_CHANNELS[0]);

// HARDWARE CONSTRAINT: the ESP32 has a single WiFi radio, so it can only be
// tuned to one channel at a time — watching "all channels at once" is physically
// impossible. We approximate full-band coverage by hopping rapidly with a very
// short dwell, so a full sweep of channels 1-11 completes in ~1s. We also alert
// the instant a deauth is seen on the current channel rather than waiting for the
// dwell window to expire (see the sampling loop below).
//
// dwell per channel while hopping (ms). ~100ms * 11 channels ≈ 1.1s per sweep.
static const uint16_t JD_DWELL = 100;

// Counters for the current channel's dwell window.
static volatile uint32_t jd_deauth = 0;
static volatile uint32_t jd_total = 0;

static void IRAM_ATTR jd_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
    if (!pkt) return;
    jd_total++;
    if (pkt->rx_ctrl.sig_len < 2) return;
    const uint8_t *f = pkt->payload;
    uint16_t fc = (uint16_t)f[0] | ((uint16_t)f[1] << 8);
    uint8_t ftype = (fc & 0x0C) >> 2; // 0 = management
    uint8_t fsub = (fc & 0xF0) >> 4;  // 0x0C deauth, 0x0A disassoc
    if (ftype == 0x00 && (fsub == 0x0C || fsub == 0x0A)) jd_deauth++;
}

static void jd_start_wifi() {
    ensureWifiPlatform();
    nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t e = esp_wifi_init(&cfg);
    if (e != ESP_OK && e != ESP_ERR_WIFI_INIT_STATE)
        Serial.printf("[JamDetect] wifi_init: %s\n", esp_err_to_name(e));
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    e = esp_wifi_start();
    if (e != ESP_OK && e != ESP_ERR_WIFI_INIT_STATE)
        Serial.printf("[JamDetect] wifi_start: %s\n", esp_err_to_name(e));
    esp_wifi_disconnect(); // drop any STA association so channel hopping isn't locked to one channel
    esp_wifi_set_promiscuous(true);
    // CRITICAL: deauth/disassoc are management frames — capture MGMT (+DATA for
    // an activity reference). Without this the default filter may drop them.
    wifi_promiscuous_filter_t filt = {};
    filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(jd_rx_cb);
}

static void jd_stop_wifi() {
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    esp_wifi_stop();
    wifiDisconnect();
    vTaskDelay(1 / portTICK_RATE_MS);
}

static void
jd_draw(const uint16_t *dps, const uint16_t *peak, uint32_t thr, uint8_t curCh, int attackCh) {
    drawMainBorderWithTitle("Jam Detect");
    tft.setTextSize(FP);

    const int x0 = 8;
    int y = 26;

    // status banner
    bool attack = (attackCh >= 0);
    uint16_t sc = attack ? TFT_RED : TFT_GREEN;
    tft.fillRect(x0, y, tftWidth - 2 * x0, 18, sc);
    tft.setTextColor(TFT_BLACK, sc);
    String banner = attack ? ("ATTACK ch" + String(attackCh) + "  " + String(dps[attackCh]) + "/s")
                           : "scanning... no jamming";
    tft.drawCentreString(banner, tftWidth / 2, y + 3, 1);
    y += 24;

    // per-channel deauth bars
    const int labelW = 28;
    const int valW = 26;
    const int barX = x0 + labelW;
    const int bottom = tftHeight - 14;
    const int rowH = (bottom - y) / JD_NCH;
    const int barW = tftWidth - barX - valW - 6;
    uint32_t scale = thr * 2;
    if (scale < 4) scale = 4;

    for (int i = 0; i < JD_NCH; i++) {
        uint8_t ch = JD_CHANNELS[i];
        int ry = y + i * rowH;
        bool isCur = (ch == curCh);
        bool over = (dps[ch] >= thr);

        tft.setTextColor(
            isCur ? bruceConfig.bgColor : bruceConfig.priColor,
            isCur ? bruceConfig.priColor : bruceConfig.bgColor
        );
        tft.drawString("Ch" + String(ch), x0, ry, 1);

        int bh = rowH - 3;
        if (bh < 4) bh = 4;
        tft.drawRect(barX, ry, barW, bh, bruceConfig.priColor);
        tft.fillRect(barX + 1, ry + 1, barW - 2, bh - 2, bruceConfig.bgColor);
        uint32_t fillW = (uint32_t)(barW - 2) * dps[ch] / scale;
        if (fillW > (uint32_t)(barW - 2)) fillW = barW - 2;
        if (fillW > 0) tft.fillRect(barX + 1, ry + 1, (int)fillW, bh - 2, over ? TFT_RED : bruceConfig.priColor);
        // peak-hold marker
        uint32_t pkX = (uint32_t)(barX + 1) + (uint32_t)(barW - 2) * peak[ch] / scale;
        if (peak[ch] > 0 && pkX > (uint32_t)(barX + 1)) tft.drawFastVLine((int)pkX, ry + 1, bh - 2, TFT_YELLOW);

        tft.setTextColor(over ? TFT_RED : bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawString(String(dps[ch]), barX + barW + 4, ry, 1);
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString("scan ch" + String(curCh) + " thr" + String(thr) + "/s  UP/DN  ESC", x0, tftHeight - 12, 1);
}

void jam_detect_setup() {
    returnToMenu = false;

    uint16_t dps[12] = {0};  // last measured deauth/s per channel (latched)
    uint16_t peak[12] = {0}; // peak-hold
    uint32_t threshold = 10; // deauth/s to flag (adjustable)
    int idx = 0;

    jd_start_wifi();
    tft.fillScreen(bruceConfig.bgColor);

    for (;;) {
        if (returnToMenu) break;

        uint8_t ch = JD_CHANNELS[idx];
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        vTaskDelay(5 / portTICK_PERIOD_MS); // brief radio settle (kept short vs the dwell)

        jd_deauth = 0;
        jd_total = 0;

        // Sample the current channel for one short dwell. The deauth/sec rate is
        // extrapolated from however many frames we caught in this window:
        //   rate = deauths * (1000 / dwell_ms)
        // so even a single deauth in a ~100ms window registers strongly.
        // IMMEDIATE ALERT: we poll the live counter inside the dwell and, the
        // moment the extrapolated rate reaches the threshold, we latch the alert
        // and redraw RIGHT AWAY instead of waiting for the dwell to finish.
        bool tripped = false;
        uint32_t t0 = millis();
        while (millis() - t0 < JD_DWELL) {
            if (check(EscPress)) {
                returnToMenu = true;
                break;
            }
            if (check(UpPress) && threshold < 250) threshold += 5;
            if (check(DownPress) && threshold > 5) threshold -= 5;

            // live extrapolated rate for the deauths seen so far this window
            uint32_t live = (uint32_t)jd_deauth * 1000UL / JD_DWELL;
            if (live >= threshold) {
                tripped = true;
                break; // detected — stop dwelling, draw immediately, then hop on
            }
            vTaskDelay(5 / portTICK_PERIOD_MS);
        }
        if (returnToMenu) break;

        uint32_t d = (uint32_t)jd_deauth * 1000UL / JD_DWELL;
        if (d > 65535) d = 65535;
        dps[ch] = (uint16_t)d;
        if (dps[ch] > peak[ch]) peak[ch] = dps[ch];

        // worst channel currently at/over threshold (latched values persist
        // across the sweep so an attack stays flagged after we hop away).
        // `tripped` guarantees the channel we just left is considered even if a
        // later channel happens to read higher this redraw.
        int attackCh = -1;
        uint16_t worst = 0;
        for (int i = 0; i < JD_NCH; i++) {
            uint8_t c = JD_CHANNELS[i];
            if (dps[c] >= threshold && dps[c] >= worst) {
                worst = dps[c];
                attackCh = c;
            }
        }
        if (tripped && attackCh < 0) attackCh = ch; // ensure the live trip is shown

        jd_draw(dps, peak, threshold, ch, attackCh);
        idx = (idx + 1) % JD_NCH;
    }

    jd_stop_wifi();
}

#endif
