#ifndef __SCAN_HOSTS_H__
#define __SCAN_HOSTS_H__

#include "core/net_utils.h"
#include <FS.h>
#include <WiFi.h>
#include <stdio.h>
#include <string.h>

#include "lwip/etharp.h"
#define ARP_MAXPENDING ARP_TABLE_SIZE

struct Host {
    Host(ip4_addr_t *ip, eth_addr *eth)
        : ip(ip->addr), mac(MAC(eth->addr)), hostname(""), vendor(""), rssi(0) {}

    Host(ip4_addr_t *ip, eth_addr *eth, const String &name, const String &vend, int rssi_val)
        : ip(ip->addr), mac(MAC(eth->addr)), hostname(name), vendor(vend), rssi(rssi_val) {}

    IPAddress ip;
    String mac;
    String hostname;
    String vendor;
    int rssi;
};

#endif
