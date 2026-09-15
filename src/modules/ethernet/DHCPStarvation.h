#ifndef DHCP_STARVATION_H
#define DHCP_STARVATION_H
#if !defined(LITE_VERSION)
#include "Arduino.h"
#include "lwip/prot/dhcp.h"
#include "lwip/prot/ip4.h"
#include "lwip/udp.h"

class DHCPStarvation {
private:
#define ETH_HDRLEN 14   // Ethernet header length
#define IPV4_HDRLEN 20  // IPV4 header length
#define UDP_HDRLEN 8    // UDP header length
#define DHCP_HDRLEN 244 // DHCP packet length

#define MAC_ADDRESS_LENGTH 6
#define IPV4_LENGTH 4
#define ETHERNET_PROTOCOL_IPV4 0x0800
#define PACKET_LENGTH_DHCP 286 // ETH header + IPV4 header + UDP header + DHCP header
    ip_hdr ipv4_pkt;
    udp_hdr udp_payload;
    dhcp_msg dhcp_payload;
    byte mac[6];
    uint8_t broadcast_mac_address[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    void send_DHCP_packet();
    void show_gui();
    struct pbuf *p;
    struct netif *netif;
    uint8_t *ethernet_frame;
    void prepare_ethernet_hdr();

    // Fill UDP and IPV4 header structure
    void prepare_udp_ipv4_hdr();

    // Fill DHCP header structure
    void prepare_dhcp_hdr();
    // Surgically change ethernet frame to include new mac address
    void change_mac_in_packet();

    void randomize_mac();

public:
    DHCPStarvation();
    ~DHCPStarvation();
    void setup();
    void loop();
};

#endif
#endif
