#ifndef WIFI_DEAUTHER_H
#define WIFI_DEAUTHER_H

#include "scan_hosts.h"
#include <vector>

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

WiFiState saveWiFiState();
void restoreWiFiState(const WiFiState& state);

// Enhanced deauth menu functions
void enhancedDeauthMenu();
void showTargetSelection();
std::vector<Host> buildTargetListFromScan();

// Deauth All submenu functions
void deauthAllMenu();
void deauthAllFromScan();
void deauthAllByChannel();
void runDeauthAll(uint8_t* targetMAC, int channel);

// Deauth Target List submenu functions
void deauthTargetListMenu();
void showAPSelectionForClientDeauth();
void scanClientsOnAP(uint8_t* targetMAC, int channel);
void showClientSelectionForDeauth(const std::vector<Host>& clients, uint8_t* targetMAC, int channel);
void runDeauthTargetList(const std::vector<Host>& targets, uint8_t* targetMAC, int channel);

// Client sniffer callback
void clientSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);

#endif
