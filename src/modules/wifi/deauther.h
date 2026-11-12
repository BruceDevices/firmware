#ifndef WIFI_DEAUTHER_H
#define WIFI_DEAUTHER_H

#include "scan_hosts.h"
#include "band_types.h"
#include <vector>

struct WiFiState {
    bool was_connected = false;
    String ssid = "";
    String bssid = "";
    uint8_t channel = 0;
    bool ap_active = false;
    String ap_ssid = "";
    wifi_mode_t wifi_mode = WIFI_MODE_NULL;
};

// AP Info structure
struct APInfo {
    uint8_t bssid[6];
    int channel;
    int band;
    bool is_5ghz;
    int frequency;
};

// Main deauth functions
void stationDeauth(Host host, const uint8_t *apBssid = nullptr);
void deauthAll();
void deauthTargetList(const std::vector<Host> &targets);

// WiFi state management
WiFiState saveWiFiState();
void restoreWiFiState(const WiFiState &state);

// Enhanced deauth menu functions
void enhancedDeauthMenu();
void showTargetSelection();
std::vector<Host> buildTargetListFromScan();

// Deauth All submenu functions
void deauthAllMenu();
void deauthAllFromScan();
void deauthAllByChannel();
void runDeauthAll(uint8_t *targetMAC, int channel);

// Deauth Target List submenu functions
void deauthTargetListMenu();
void showAPSelectionForClientDeauth();
void scanClientsOnAP(uint8_t *targetMAC, int channel);
void showClientSelectionForDeauth(const std::vector<Host> &clients, uint8_t *targetMAC, int channel);
void runDeauthTargetList(const std::vector<Host> &targets, uint8_t *targetMAC, int channel);

// Client sniffer callback
void clientSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type);

// Channel detection - shared with wifi_atks
int getAPChannel(const uint8_t *target_bssid, bool *found = nullptr);

// =============================================================================
// Band Detection and Adaptive Functions
// =============================================================================

// Detect which bands the current hardware supports
void detectSupportedBands();

// Check if a specific band is supported
bool isBandSupported(int band);

// Get the list of supported bands
SupportedBands getSupportedBands();

// Build channel list from APs (for multi-band hopping)
std::vector<int> buildChannelListFromAPs(const std::vector<APInfo> &aps);

// Build default channel list based on supported bands
std::vector<int> buildDefaultChannelList();

// Get band name as string
String getBandName(int band);

// Cache same SSID APs across bands
void cacheSameSSIDAPs();

#endif
