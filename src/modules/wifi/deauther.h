#ifndef WIFI_DEAUTHER_H
#define WIFI_DEAUTHER_H

#include "scan_hosts.h"
#include <vector>

// WiFi state for save/restore
struct WiFiState {
    bool was_connected = false;
    String ssid = "";
    String bssid = "";
    uint8_t channel = 0;
    bool ap_active = false;
    String ap_ssid = "";
};

void stationDeauth(Host host);
void deauthAll();
void deauthTargetList(const std::vector<Host>& targets);

// Helper functions for WiFi state management
WiFiState saveWiFiState();
void restoreWiFiState(const WiFiState& state);

#endif
