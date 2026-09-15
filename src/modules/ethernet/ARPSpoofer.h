#ifndef ARP_SPOOFER_H
#define ARP_SPOOFER_H
#if !defined(LITE_VERSION)
#include "Arduino.h"
#include "FS.h"
#include "modules/wifi/scan_hosts.h"

class ARPSpoofer {
private:
    uint8_t gatewayIP[4];  // Gateway IP Address
    uint8_t victimIP[4];   // Victim IP Address
    uint8_t gatewayMAC[6]; // Gateway MAC
    uint8_t victimMAC[6];  // Victim MAC
    uint8_t myMAC[6];      // ESP32 MAC Address
    bool mitm;

    File pcapFile;
    void setup(const Host &host, IPAddress gateway);
    void loop();
    bool arpPCAPfile();
    void sendARPPacket(
        uint8_t *targetIP, uint8_t *targetMAC, uint8_t *spoofedIP, uint8_t *spoofedMAC, File pcapFile
    );

public:
    // @brief Dummy default constructori
    ARPSpoofer() {};
    ARPSpoofer(const Host &host, IPAddress gateway, uint8_t gatewayMAC[6], uint8_t mac[6], bool _mitm);
    ~ARPSpoofer();
};

#endif
#endif
