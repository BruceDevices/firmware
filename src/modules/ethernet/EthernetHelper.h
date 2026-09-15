#if !defined(LITE_VERSION)

#ifndef ETHERNET_HELPER_H
#define ETHERNET_HELPER_H
#include <ETH.h>
#include <Arduino.h>
#include <Network.h>
#include <SPI.h>
#include <esp_netif.h>
#include <stdint.h>

class EthernetHelper {
private:
    uint8_t mac[6];
    void generate_mac();
    SPIClass *ethSpi = nullptr;
    network_event_handle_t ethEventId = 0;
    esp_netif_t *ethNetif = nullptr;

public:
    EthernetHelper(/* args */);
    ~EthernetHelper();
    bool setup();
    bool is_connected();
    esp_netif_t *netif() const { return ethNetif; }
    void stop();
};

#endif // ETHERNET_H
#endif
