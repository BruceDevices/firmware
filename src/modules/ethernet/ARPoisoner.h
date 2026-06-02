#ifndef ARP_POISONER_H
#define ARP_POISONER_H
#if !defined(LITE_VERSION)
#include "Arduino.h"
#include "FS.h"

class ARPoisoner {
private:
    uint8_t gatewayIP[4];  // Gateway IP Address
    uint8_t victimIP[4];   // Victim IP Address
    uint8_t gatewayMAC[6]; // Gateway MAC
    uint8_t victimMAC[6];  // Victim MAC
    File pcapFile;
    void setup(IPAddress gateway);
    void loop();
    bool arpPCAPfile();
    void sendARPPacket(
        uint8_t *targetIP, uint8_t *targetMAC, uint8_t *spoofedIP, uint8_t *spoofedMAC, File pcapFile
    );

public:
    // @brief Dummy default constructori
    ARPoisoner() {};
    ARPoisoner(IPAddress gateway);
    ~ARPoisoner();
};

#endif
#endif
