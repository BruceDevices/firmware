#ifndef ARP_SCANNER_H
#define ARP_SCANNER_H
#if !defined(LITE_VERSION)
#include "Arduino.h"
#include "IPAddress.h"
#include "modules/wifi/scan_hosts.h"
#include "stdint.h"
#include <set>
#include <vector>

class ARPScanner {
private:
    esp_netif_t *esp_net_interface;
    TickType_t arpRequestDelay =
        20u / portTICK_PERIOD_MS; // can be relatively low, helps to not overwhelm the stream

    void setup();
    void readArpTableETH(netif *iface);
    IPAddress gateway;

    std::vector<Host> hostslist_eth;
    void afterScanOptions(const Host &host);
    bool macStringToByteArray(const std::string &macStr, uint8_t macArray[6]);

public:
    ARPScanner() {};
    ARPScanner(esp_netif_t *esp_net_interface);
    ~ARPScanner();
};

void run_arp_scanner();
#endif // ARP_SCANNER_H
#endif
