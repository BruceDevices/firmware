#if !defined(LITE_VERSION) && defined(NM_CYD_ESP32C5)

#include "dual_band_analyzer.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/wifi/webInterface.h"
#include <Arduino.h>
#include <WiFi.h>
#include <globals.h>

namespace {

constexpr int32_t RSSI_CEILING = -30;
constexpr int32_t RSSI_SHOW_SSID = -70;
constexpr int32_t RSSI_FLOOR = -100;
constexpr uint8_t CHANNEL_COUNT = 9;
constexpr uint8_t DISPLAY_AP_COUNT = 8;
// non-DFS (Safe) Channel
constexpr uint8_t CHANNELS[CHANNEL_COUNT] = {36, 40, 44, 48, 149, 153, 157, 161, 165};
// DFS(Dynamic Frequency Selection) channels (Optional)
// constexpr uint8_t DFS_CHANNELS[8] = {52,56,60,64,100,116,132,140};
constexpr uint16_t CHANNEL_COLORS[CHANNEL_COUNT] = {
    TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE, TFT_MAGENTA, TFT_PINK, TFT_WHITE
};

struct ChannelStats {
    uint8_t apCount;
    int32_t peak;
    String peakName;
};

struct DisplayAp {
    int channel;
    int32_t rssi;
    String name;
};

uint16_t channelIdx(int channel) {
    if (channel <= 14) return channel - 1;
    if (channel <= 68) return 14 + ((channel - 32) / 2);
    if (channel <= 144) return 41 + ((channel - 96) / 2);
    if (channel <= 177) return 67 + ((channel - 149) / 2);
    return 82;
}

int graphIndex(int channel) {
    static constexpr uint16_t referenceIndexes[CHANNEL_COUNT] = {16, 18, 20, 22, 67, 69, 71, 73, 75};
    uint16_t index = channelIdx(channel);
    for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
        if (index == referenceIndexes[i] && channel == CHANNELS[i]) return i;
    }
    return -1;
}

bool matchBssidPrefix(const uint8_t *first, const uint8_t *second) {
    for (uint8_t i = 0; i < 5; ++i) {
        if (first[i] != second[i]) return false;
    }
    return true;
}

void resetStats(ChannelStats *stats) {
    for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
        stats[i].apCount = 0;
        stats[i].peak = RSSI_FLOOR;
        stats[i].peakName = "";
    }
}

void resetDisplayAps(DisplayAp *displayAps) {
    for (uint8_t i = 0; i < DISPLAY_AP_COUNT; ++i) {
        displayAps[i].channel = 0;
        displayAps[i].rssi = RSSI_FLOOR;
        displayAps[i].name = "";
    }
}

void addDisplayAp(DisplayAp *displayAps, int channel, int32_t rssi, const String &name) {
    int insertAt = -1;
    for (uint8_t i = 0; i < DISPLAY_AP_COUNT; ++i) {
        if (rssi > displayAps[i].rssi) {
            insertAt = i;
            break;
        }
    }
    if (insertAt < 0) return;

    for (int i = DISPLAY_AP_COUNT - 1; i > insertAt; --i) displayAps[i] = displayAps[i - 1];
    displayAps[insertAt].channel = channel;
    displayAps[insertAt].rssi = rssi;
    displayAps[insertAt].name = name.length() > 0 ? name : "<hidden>";
}

void drawAnalyzer(
    const ChannelStats *stats, const DisplayAp *displayAps, uint16_t totalAp, uint16_t scanNumber
) {
    const int headerHeight = 20;
    const int statsHeight = 52;
    const int spectrumHeight = tftHeight - headerHeight - statsHeight;
    const int spectrumBottom = headerHeight + spectrumHeight;
    const int baseline = spectrumBottom - 20;
    const int graphTop = headerHeight + 12;
    const int graphHeight = baseline - graphTop;
    const int left = 8;
    const int slotWidth = (tftWidth - left * 2) / CHANNEL_COUNT;
    const int barWidth = max(3, slotWidth - 4);

    tft.fillRect(0, 0, tftWidth, headerHeight, bruceConfig.bgColor);
    tft.fillRect(0, headerHeight, tftWidth, spectrumHeight, bruceConfig.bgColor);
    tft.fillRect(0, spectrumBottom, tftWidth, statsHeight, bruceConfig.bgColor);

    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString("< BACK", left, 2, 1);
    tft.drawString("5G WiFi Analyzer", left + 42, 2, 1);
    tft.drawRightString("Scan " + String(scanNumber), tftWidth - left, 2, 1);

    tft.drawFastHLine(left, baseline, tftWidth - left * 2, bruceConfig.priColor);

    int strongestIndex = -1;
    for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
        const int x = left + i * slotWidth + 2;
        const int signalHeight =
            constrain(map(stats[i].peak, RSSI_FLOOR, RSSI_CEILING, 1, graphHeight), 1, graphHeight);

        if (stats[i].apCount > 0) {
            tft.fillRect(x, baseline - signalHeight, barWidth, signalHeight, CHANNEL_COLORS[i]);
            tft.setTextColor(CHANNEL_COLORS[i], bruceConfig.bgColor);
            tft.drawCentreString(String(stats[i].peak), x + barWidth / 2, baseline - signalHeight - 9, 1);
            if (strongestIndex < 0 || stats[i].peak > stats[strongestIndex].peak) strongestIndex = i;
        }

        tft.setTextColor(CHANNEL_COLORS[i], bruceConfig.bgColor);
        tft.drawCentreString(String(CHANNELS[i]), x + barWidth / 2, baseline + 2, 1);
        tft.setTextColor(TFT_LIGHTGREY, bruceConfig.bgColor);
        tft.drawCentreString(String(stats[i].apCount), x + barWidth / 2, baseline + 10, 1);
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString("AP " + String(totalAp) + "  5 GHz", left, spectrumBottom + 2, 1);
    if (strongestIndex >= 0) {
        String peakText = "Ch" + String(CHANNELS[strongestIndex]) + " " + String(stats[strongestIndex].peak) +
                          " " + stats[strongestIndex].peakName;
        if (peakText.length() > 31) peakText = peakText.substring(0, 31);
        tft.drawString(peakText, left, spectrumBottom + 12, 1);
    } else {
        tft.drawString("Scanning 36-165", left, spectrumBottom + 12, 1);
    }

    const int listTop = spectrumBottom + 24;
    const int columnWidth = tftWidth / 2;
    for (uint8_t i = 0; i < DISPLAY_AP_COUNT; ++i) {
        if (displayAps[i].channel == 0) break;
        const int column = i % 2;
        const int row = i / 2;
        String apText = String(i + 1) + ". Ch" + String(displayAps[i].channel) + " " +
                        String(displayAps[i].rssi) + " " + displayAps[i].name;
        const int maxChars = columnWidth / (LW * 1) - 1;
        if (apText.length() > maxChars) apText = apText.substring(0, maxChars);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawString(apText, column * columnWidth + left, listTop + row * LH, 1);
    }
}

void collectStats(ChannelStats *stats, DisplayAp *displayAps, int networks, uint16_t &totalAp) {
    for (int i = 0; i < networks; ++i) {
        int channel = WiFi.channel(i);
        int index = graphIndex(channel);
        if (index < 0) continue;

        int32_t rssi = WiFi.RSSI(i);
        String ssid = WiFi.SSID(i);
        if (ssid.length() > 0 && stats[index].peak < rssi) {
            stats[index].peak = rssi;
            stats[index].peakName = ssid;
        }

        bool duplicate = false;
        const uint8_t *bssid = WiFi.BSSID(i);
        for (int j = 0; j < i; ++j) {
            if (WiFi.channel(j) == channel && matchBssidPrefix(WiFi.BSSID(j), bssid)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            ++stats[index].apCount;
            ++totalAp;
            addDisplayAp(displayAps, channel, rssi, ssid.length() > 0 ? ssid : WiFi.BSSIDstr(i));
        }
    }
}

} // namespace

void dual_band_analyzer_setup() {
    returnToMenu = false;
    cleanlyStopWebUiForWiFiFeature();
    WiFi.mode(WIFI_STA);
    WiFi.setBandMode(WIFI_BAND_MODE_5G_ONLY);

    ChannelStats stats[CHANNEL_COUNT];
    DisplayAp displayAps[DISPLAY_AP_COUNT];
    uint16_t scanNumber = 0;
    uint16_t totalAp = 0;
    bool scanInProgress = false;
    tft.fillScreen(bruceConfig.bgColor);

    while (!returnToMenu) {
        if (check(EscPress)) {
            returnToMenu = true;
            break;
        }

        if (!scanInProgress) {
            resetStats(stats);
            resetDisplayAps(displayAps);
            totalAp = 0;
            WiFi.scanNetworks(true, true, false, scanNumber < 2 ? 30 : 300);
            scanInProgress = true;
        }

        int networks = WiFi.scanComplete();
        if (networks == WIFI_SCAN_RUNNING) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        scanInProgress = false;
        if (networks < 0) {
            WiFi.scanDelete();
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (networks > 0) collectStats(stats, displayAps, networks, totalAp);
        ++scanNumber;
        drawAnalyzer(stats, displayAps, totalAp, scanNumber);
        WiFi.scanDelete();

        uint32_t pauseStarted = millis();
        while (millis() - pauseStarted < 250) {
            if (check(EscPress)) {
                returnToMenu = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    WiFi.scanDelete();
    WiFi.setBandMode(WIFI_BAND_MODE_AUTO);
}

#endif
