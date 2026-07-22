#include "jammer_identifier.h"

#include "core/display.h"
#include "core/wifi/webInterface.h"
#include "core/wifi/wifi_common.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <Arduino.h>
#include <globals.h>

namespace {
constexpr uint8_t JAMMER_CHANNELS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
constexpr uint32_t JAMMER_SAMPLE_MS = 700;
constexpr uint32_t JAMMER_SWITCH_MS = 800;
constexpr uint32_t JAMMER_DEAUTH_THRESHOLD = 8;

volatile uint32_t g_jammer_packets = 0;
volatile uint32_t g_jammer_deauths = 0;
volatile uint32_t g_last_jammer_sample = 0;
volatile bool g_last_jammer_state = false;
uint8_t g_current_channel_index = 0;

bool isDeauthOrDisassocFrame(const uint8_t *frame) {
    if (!frame || frame[0] < 0x80) return false;
    const uint8_t subtype = frame[0] & 0xFC;
    return subtype == 0xC0 || subtype == 0xA0;
}

void jammerSniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!buf) return;

    auto *pkt = reinterpret_cast<wifi_promiscuous_pkt_t *>(buf);
    if (!pkt || pkt->rx_ctrl.sig_len < 24) return;

    ++g_jammer_packets;
    if (isDeauthOrDisassocFrame(pkt->payload)) ++g_jammer_deauths;
}

bool detectJammerAttack(uint32_t &samplePackets, uint32_t &sampleDeauths) {
    const uint32_t now = millis();
    if (now - g_last_jammer_sample < JAMMER_SAMPLE_MS) {
        samplePackets = g_jammer_packets;
        sampleDeauths = g_jammer_deauths;
        return g_last_jammer_state;
    }

    samplePackets = g_jammer_packets;
    sampleDeauths = g_jammer_deauths;
    g_last_jammer_sample = now;

    const bool active = (sampleDeauths >= JAMMER_DEAUTH_THRESHOLD) ||
                        (samplePackets > 0 && sampleDeauths * 10 >= samplePackets);
    g_last_jammer_state = active;

    g_jammer_packets = 0;
    g_jammer_deauths = 0;
    return active;
}

void drawStatus(const String &message, bool active) {
    tft.fillRect(0, 0, tft.width(), tft.height(), bruceConfig.bgColor);
    drawMainBorderWithTitle("Jammer Identifier");

    if (active) {
        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        tft.setTextSize(2);
        tft.drawCentreString("ACTIVE JAMMER ATTACK", tft.width() / 2, 58, SMOOTH_FONT);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.drawCentreString("Deauth/Disassoc frames detected", tft.width() / 2, 110, SMOOTH_FONT);
    } else {
        tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
        tft.setTextSize(2);
        tft.drawCentreString("SCANNING", tft.width() / 2, 70, SMOOTH_FONT);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.drawCentreString(message, tft.width() / 2, 110, SMOOTH_FONT);
    }

    tft.setTextColor(TFT_ORANGE, bruceConfig.bgColor);
    tft.setTextSize(1);
    tft.drawCentreString("2.4GHz only", tft.width() / 2, tft.height() - 35, SMOOTH_FONT);

    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawCentreString("Press any key to stop", tft.width() / 2, tft.height() - 18, SMOOTH_FONT);
}
}  // namespace

void jammerIdentifier() {
    cleanlyStopWebUiForWiFiFeature();
    resetTftDisplay();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_MODE_STA);
    delay(60);

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&(wifi_promiscuous_filter_t){.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL});
    esp_wifi_set_promiscuous_rx_cb(jammerSniffer);

    g_jammer_packets = 0;
    g_jammer_deauths = 0;
    g_last_jammer_sample = 0;
    g_last_jammer_state = false;

    uint32_t lastChannelChange = millis();
    uint32_t lastStatusUpdate = 0;
    String statusText = "Listening on Wi-Fi channels";

    while (!check(AnyKeyPress) && !check(EscPress)) {
        if (millis() - lastChannelChange >= JAMMER_SWITCH_MS) {
            g_current_channel_index = (g_current_channel_index + 1) % (sizeof(JAMMER_CHANNELS) / sizeof(JAMMER_CHANNELS[0]));
            const uint8_t ch = JAMMER_CHANNELS[g_current_channel_index];
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            lastChannelChange = millis();
            statusText = "Scanning channel " + String(ch);
        }

        uint32_t samplePackets = 0;
        uint32_t sampleDeauths = 0;
        const bool active = detectJammerAttack(samplePackets, sampleDeauths);

        if (millis() - lastStatusUpdate >= 250) {
            lastStatusUpdate = millis();
            drawStatus(active ? "Active deauth flood detected" : statusText, active);
            if (active) {
                tft.setTextColor(TFT_RED, bruceConfig.bgColor);
                tft.setTextSize(1);
                tft.drawCentreString("Deauths: " + String(sampleDeauths) + " / Packets: " + String(samplePackets),
                                     tft.width() / 2, 140, SMOOTH_FONT);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }

    esp_wifi_set_promiscuous_rx_cb(nullptr);
    esp_wifi_set_promiscuous(false);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    resetTftDisplay();
    returnToMenu = true;
}
