#if !defined(LITE_VERSION)
// Bruce Channel Analyzer v4 — clean minimal layout
// Solid-threshold bars, compact channel labels, simple stats, signal meter.

#include "channel_analyzer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include <globals.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define CA_NUM_CHANNELS   11
#define CA_DWELL_MS      200
#define CA_SIGNAL_MTR     8
#define CA_RSSI_MIN     (-90)
#define CA_RSSI_MAX     (-30)

static const uint8_t caChannels[CA_NUM_CHANNELS] = {1,2,3,4,5,6,7,8,9,10,11};

static uint16_t caBarColor(int8_t rssi) {
    if (rssi >= -40) return tft.color565(220, 40,  40);   // Red
    if (rssi >= -55) return tft.color565(220, 120, 20);   // Orange
    if (rssi >= -70) return tft.color565(200, 200, 30);   // Yellow
    if (rssi >= -80) return tft.color565(50,  200, 50);   // Green
    return tft.color565(40,  100, 220);                     // Blue
}

// State
static volatile int32_t caRssiSum = 0;
static volatile int     caRssiCount = 0;
static int8_t  caChannelRssi[CA_NUM_CHANNELS];
static int8_t  caSmoothRssi[CA_NUM_CHANNELS];
static uint8_t caChIndex = 0;
static uint8_t caCurChannel = 1;
static unsigned long caLastHop = 0;
static int8_t  caPeakRssi = CA_RSSI_MIN;
static int     caPeakChannel = 0;
static int8_t  caPeakHoldRssi = CA_RSSI_MIN;
static int     caPeakHoldChan = 0;
static unsigned long caPeakHoldTime = 0;
static int8_t  caNoiseFloor = CA_RSSI_MIN;
static int32_t caNoiseFloorSum = 0;
static int     caNoiseFloorCount = 0;
static bool    caGotFirstData = false;

static void IRAM_ATTR caPromiscCB(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    caRssiSum += pkt->rx_ctrl.rssi;
    caRssiCount++;
}

static void caDrawGraph() {
    int SCW = tft.width();
    int gx = 10, gy = 48, gw = SCW - 20, gh = 100;
    int barW = gw / CA_NUM_CHANNELS;
    int barDw = barW - 1;

    // Clear graph area
    tft.fillRect(gx, gy, gw, gh, bruceConfig.bgColor);

    // Noise floor dashed line
    if (caNoiseFloor > CA_RSSI_MIN) {
        int nfy = gy + gh - map(caNoiseFloor, CA_RSSI_MIN, CA_RSSI_MAX, 0, gh);
        uint16_t noc = tft.color565(100, 180, 255);
        for (int xx = gx; xx < gx + gw; xx += 6) {
            tft.drawFastHLine(xx, nfy, 3, noc);
        }
    }

    // Peak-hold decay
    if (caPeakRssi > caPeakHoldRssi) {
        caPeakHoldRssi = caPeakRssi;
        caPeakHoldChan = caPeakChannel;
        caPeakHoldTime = millis();
    } else if (millis() - caPeakHoldTime > 3000) {
        caPeakHoldRssi = CA_RSSI_MIN;
        caPeakHoldChan = 0;
    }

    // Update noise floor
    int32_t ns = 0; int nc = 0;
    for (int ch = 0; ch < CA_NUM_CHANNELS; ch++) {
        int8_t r = caChannelRssi[ch];
        if (r > caPeakRssi) { caPeakRssi = r; caPeakChannel = ch; }
        if (r > CA_RSSI_MIN) { ns += r; nc++; }
    }
    if (nc > 0) {
        int8_t an = (int8_t)(ns / nc);
        caNoiseFloorSum += an; caNoiseFloorCount++;
        if (caNoiseFloorCount > 10) {
            caNoiseFloor = (int8_t)(caNoiseFloorSum / caNoiseFloorCount);
            caNoiseFloorSum = caNoiseFloorCount = 0;
        }
    }

    // Draw bars
    for (int ch = 0; ch < CA_NUM_CHANNELS; ch++) {
        int8_t rssi = caSmoothRssi[ch];
        int x = gx + ch * barW;
        int barH = map(constrain(rssi, CA_RSSI_MIN, CA_RSSI_MAX), CA_RSSI_MIN, CA_RSSI_MAX, 0, gh);
        if (barH < 0) barH = 0; if (barH > gh) barH = gh;
        if (rssi > CA_RSSI_MIN && barH < 2) barH = 2;

        if (barH > 0) {
            int by = gy + gh - barH;
            tft.fillRect(x, by, barDw, barH, caBarColor(rssi));
            if (ch == caPeakChannel && barH > 6)
                tft.drawRect(x, by, barDw, barH, TFT_WHITE);
        }
    }

    // Channel labels (FP font for compact fit)
    int labelY = gy + gh + 2;
    tft.setTextSize(FP);
    for (int ch = 0; ch < CA_NUM_CHANNELS; ch++) {
        int x = gx + ch * barW;
        tft.fillRect(x, labelY, barDw, 9, bruceConfig.bgColor);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setCursor(x + (barDw - 6) / 2 + 1, labelY + 1);
        tft.print(caChannels[ch]);
    }

    // Divider
    int divY = labelY + 12;
    tft.drawFastHLine(0, divY, SCW, bruceConfig.priColor);

    // Stats — two compact lines
    int sy = divY + 4;
    char buf[24];
    int stw = 85;
    tft.setTextSize(FP);

    // Line 1: CH  PEAK  SIG  MAX
    tft.fillRect(8, sy, SCW - 16, 8, bruceConfig.bgColor);
    tft.setCursor(10, sy);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.print("CH:");
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.printf("%d", caCurChannel);

    tft.setCursor(60, sy);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.print("PEAK: ");
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.printf("%d", caChannels[caPeakChannel]);

    tft.setCursor(155, sy);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.print("SIG: ");
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    snprintf(buf, sizeof(buf), "%ddBm", (int)caPeakRssi);
    tft.print(buf);

    if (caPeakHoldRssi > CA_RSSI_MIN) {
        snprintf(buf, sizeof(buf), " MAX:%d", (int)caPeakHoldRssi);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.drawRightString(buf, SCW - 8, sy, 1);
    } else {
        tft.fillRect(SCW - 60, sy, 56, 8, bruceConfig.bgColor);
    }

    // Line 2: NOISE  UTIL  meter
    int sy2 = sy + 10;
    tft.fillRect(8, sy2, SCW - 16, 8, bruceConfig.bgColor);
    tft.setCursor(10, sy2);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.print("NOISE:");
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    snprintf(buf, sizeof(buf), " %ddBm", (int)caNoiseFloor);
    tft.print(buf);

    int act = 0;
    for (int ch = 0; ch < CA_NUM_CHANNELS; ch++)
        if (caChannelRssi[ch] > caNoiseFloor + 3) act++;
    int util = (act * 100) / CA_NUM_CHANNELS;

    tft.setCursor(155, sy2);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.print("UTIL:");
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.printf(" %d%%", util);

    // Signal meter bar
    int mtrY = sy2 + 14;
    int barMaxH = 16, barMinH = 2, bw = 8, sp = 12;
    tft.fillRect(155, mtrY, 60, 8, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.setCursor(155, mtrY);
    tft.printf("%d%%", map(caPeakRssi, CA_RSSI_MIN, CA_RSSI_MAX, 0, 100));

    for (int i = 0; i < CA_SIGNAL_MTR; i++) {
        int bx = 10 + i * sp;
        int bh = barMinH + (i * (barMaxH - barMinH)) / (CA_SIGNAL_MTR - 1);
        int by = mtrY + barMaxH - bh;
        int thresh = (i + 1) * (CA_RSSI_MAX - CA_RSSI_MIN) / CA_SIGNAL_MTR + CA_RSSI_MIN;
        tft.fillRect(bx, by, bw, bh, bruceConfig.bgColor);
        if (caPeakRssi >= thresh) {
            tft.fillRect(bx, by, bw, bh, caBarColor((int8_t)thresh));
            tft.drawRect(bx, by, bw, bh, TFT_WHITE);
        } else {
            tft.drawRect(bx, by, bw, bh, bruceConfig.priColor);
        }
    }
}

void channel_analyzer_menu() {
    memset(caChannelRssi, CA_RSSI_MIN, sizeof(caChannelRssi));
    memset(caSmoothRssi, CA_RSSI_MIN, sizeof(caSmoothRssi));
    caChIndex = 0; caCurChannel = caChannels[0];
    caPeakRssi = CA_RSSI_MIN; caPeakChannel = 0;
    caPeakHoldRssi = CA_RSSI_MIN; caPeakHoldChan = 0;
    caNoiseFloor = CA_RSSI_MIN;
    caNoiseFloorSum = caNoiseFloorCount = 0;
    caRssiSum = caRssiCount = 0;
    caGotFirstData = false;
    caLastHop = millis();

    WiFi.mode(WIFI_OFF); delay(50);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t wr = esp_wifi_init(&cfg);
    if (wr != ESP_OK && wr != ESP_ERR_WIFI_INIT_STATE) ESP_ERROR_CHECK(wr);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_channel(caCurChannel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(caPromiscCB);

    drawMainBorderWithTitle("Channel Analyzer", true);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(8, tft.height() - 10);
    tft.print("[Esc/Sel] Exit");

    while (!check(EscPress) && !check(SelPress)) {
        unsigned long now = millis();
        if (now - caLastHop >= CA_DWELL_MS) {
            if (caRssiCount > 0) {
                int8_t raw = (int8_t)(caRssiSum / caRssiCount);
                caChannelRssi[caChIndex] = raw;
                caSmoothRssi[caChIndex] = (int8_t)(raw * 0.4f + caSmoothRssi[caChIndex] * 0.6f + 0.5f);
                caGotFirstData = true;
            } else {
                caChannelRssi[caChIndex] = CA_RSSI_MIN;
                caSmoothRssi[caChIndex] = (int8_t)(CA_RSSI_MIN * 0.4f + caSmoothRssi[caChIndex] * 0.6f + 0.5f);
            }
            caRssiSum = 0; caRssiCount = 0;
            caChIndex = (caChIndex + 1) % CA_NUM_CHANNELS;
            caCurChannel = caChannels[caChIndex];
            esp_wifi_set_channel(caCurChannel, WIFI_SECOND_CHAN_NONE);
            caLastHop = now;
            if (caGotFirstData) caDrawGraph();
        }
        delay(5);
    }

    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    // Don't call esp_wifi_deinit() — wifiDisconnect() needs the driver alive
}
#endif
