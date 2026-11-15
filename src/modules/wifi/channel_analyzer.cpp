#if !defined(LITE_VERSION)
#include "channel_analyzer.h"

#include "esp_err.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/wifi/wifi_common.h"
#include "wifi_spectrum.h"
#include <Arduino.h>
#include <globals.h>

// 2.4GHz channels to sweep.
static const uint8_t CA_CHANNELS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
static const int CA_NCH = sizeof(CA_CHANNELS) / sizeof(CA_CHANNELS[0]);

// Counters updated from the promiscuous RX callback for the *current* channel.
static volatile uint32_t ca_bytes = 0;
static volatile uint32_t ca_pkts = 0;
static volatile int8_t ca_rssi_peak = -128;

// Keep the callback minimal: just accumulate. Airtime is estimated in the loop.
static void IRAM_ATTR ca_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
    if (!pkt) return;
    ca_pkts++;
    ca_bytes += pkt->rx_ctrl.sig_len;
    if (pkt->rx_ctrl.rssi > ca_rssi_peak) ca_rssi_peak = pkt->rx_ctrl.rssi;
}

// Tolerant WiFi bring-up. Unlike the sniffer (ESP_ERROR_CHECK), we never abort:
// if WiFi is already initialised/started we get ESP_ERR_WIFI_INIT_STATE (or
// similar) and just carry on — promiscuous mode works regardless.
static void ca_start_wifi() {
    ensureWifiPlatform();
    nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t e = esp_wifi_init(&cfg);
    if (e != ESP_OK && e != ESP_ERR_WIFI_INIT_STATE)
        Serial.printf("[ChAnalyzer] wifi_init: %s\n", esp_err_to_name(e));
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    e = esp_wifi_start();
    if (e != ESP_OK && e != ESP_ERR_WIFI_INIT_STATE)
        Serial.printf("[ChAnalyzer] wifi_start: %s\n", esp_err_to_name(e));
    esp_wifi_set_promiscuous(true);
    // Capture every frame type so the airtime estimate reflects real load.
    wifi_promiscuous_filter_t filt = {};
    filt.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(ca_rx_cb);
}

static void ca_stop_wifi() {
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    esp_wifi_stop();
    wifiDisconnect();
    vTaskDelay(1 / portTICK_RATE_MS);
}


void channel_analyzer_setup() {
    returnToMenu = false;

    uint8_t load[WifiSpectrumView::CH_MAX] = {0};
    uint8_t peak[WifiSpectrumView::CH_MAX] = {0};
    int8_t rssi[WifiSpectrumView::CH_MAX];
    for (int i = 0; i < WifiSpectrumView::CH_MAX; i++) rssi[i] = -128;

    uint16_t dwell = 350; // ms per channel, adjustable with Up/Down
    int idx = 0;

    WifiSpectrumView view;
    if (!view.begin("Channel Analyzer")) {
        displayError("Out of memory", true);
        return;
    }

    ca_start_wifi();
    view.commit(load, CA_CHANNELS[0]);
    view.status("CH" + String(CA_CHANNELS[0]) + " scanning...");

    for (;;) {
        if (returnToMenu) break;
        if (check(EscPress)) {
            returnToMenu = true;
            break;
        }
        if (check(UpPress) && dwell < 1000) dwell += 100;  // longer dwell = more accurate
        if (check(DownPress) && dwell > 150) dwell -= 100; // shorter dwell = faster sweep

        uint8_t ch = CA_CHANNELS[idx];
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);

        // reset counters for this dwell window
        ca_bytes = 0;
        ca_pkts = 0;
        ca_rssi_peak = -128;

        uint32_t t0 = millis();
        uint32_t lastFrame = 0;
        while (millis() - t0 < dwell) {
            if (check(EscPress)) {
                returnToMenu = true;
                break;
            }
            // Animate while the radio dwells, so the trace glides toward the last
            // measurement instead of snapping channel by channel.
            if (millis() - lastFrame >= 60) {
                lastFrame = millis();
                view.animate(load, peak, ch);
            }
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        if (returnToMenu) break;

        // Estimate airtime utilisation: bytes at a conservative ~6Mbps baseline
        // plus per-frame preamble/IFS overhead. Clamp to 0-100%.
        uint32_t airtime_us = (ca_bytes * 8UL) / 6UL + ca_pkts * 60UL;
        uint32_t dwell_us = (uint32_t)dwell * 1000UL;
        uint32_t l = dwell_us ? (airtime_us * 100UL / dwell_us) : 0;
        if (l > 100) l = 100;

        load[ch] = (uint8_t)l;
        if (load[ch] > peak[ch]) peak[ch] = load[ch];
        rssi[ch] = (ca_rssi_peak == -128) ? 0 : ca_rssi_peak;

        view.commit(load, ch);
        view.status(
            "CH" + String(ch) + " " + String(load[ch]) + "% PK" + String(peak[ch]) + "% " +
            String(rssi[ch]) + "dBm " + String(dwell) + "ms"
        );

        idx = (idx + 1) % CA_NCH;
        if (idx == 0) drawStatusBar(); // keep battery/clock fresh once per sweep
    }

    view.end();
    ca_stop_wifi();
}

#endif
