/**
 * @file ARPScanner.cpp
 * @brief ARP Scanner module for every esp-netif
 * @version 0.1
 * @date 2025-05-15
 */

#include "ARPScanner.h"
#include "ARPSpoofer.h"
#include "ARPoisoner.h"
#include "HostInfo.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "modules/wifi/clients.h"
#include "modules/wifi/deauther.h"
#include "modules/wifi/scan_hosts.h"
#if !defined(LITE_VERSION)
#include "modules/ethernet/ARPoisoner.h"
#include "modules/ethernet/DHCPStarvation.h"
#include "modules/ethernet/MACFlooding.h"
#include <ETH.h>
#endif
#include <globals.h>
#include <sstream>
void run_arp_scanner() {
#if !defined(LITE_VERSION)
    // Prefer the Arduino ETH netif, fall back to legacy key-based lookup
    esp_netif_t *esp_netinterface = ETH.netif();
    if (esp_netinterface == nullptr) { esp_netinterface = esp_netif_get_handle_from_ifkey("ETH_DEF"); }
    if (esp_netinterface == nullptr) {
        Serial.println("Failed to get netif handle");
        return;
    }
    ARPScanner{esp_netinterface};
#endif
}

ARPScanner::ARPScanner(esp_netif_t *_esp_net_interface) {
    esp_net_interface = _esp_net_interface;
    setup();
}

ARPScanner::~ARPScanner() {}

#define ETH_HDRLEN 14 // Ethernet header length
#define ARP_HDRLEN 28 // ARP header length
#define ETH_HW_TYPE 0x0800
#define ARP_REQUEST_OPCODE 1
#define ARP_REPLY_OPCODE 2
#define MAC_ADDRESS_LENGTH 6
#define IPV4_LENGTH 4
#define ETH_ARP_HW_TYPE 1
#define ETHERNET_PROTOCOL_ARP 0x0806
#define PACKET_LENGTH 42 // ETH packet + ARP packet

uint8_t broadcast_mac_address[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t ether_frame[PACKET_LENGTH];
uint8_t mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
uint8_t target_ip[4] = {192, 168, 1, 254};
uint8_t target_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void toBytes(IPAddress ip, uint8_t *bytes) {
    bytes[0] = ip[0];
    bytes[1] = ip[1];
    bytes[2] = ip[2];
    bytes[3] = ip[3];
}

void ARPScanner::readArpTableETH(netif *iface) {
    for (uint32_t i = 0; i < ARP_TABLE_SIZE; ++i) {
        ip4_addr_t *ip_ret;
        eth_addr *eth_ret;
        if (etharp_get_entry(i, &ip_ret, &iface, &eth_ret)) { hostslist_eth.emplace_back(ip_ret, eth_ret); }
    }
    etharp_cleanup_netif(iface);
}

#include "ARPSpoofer.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_ping.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "ping/ping_sock.h"

bool wait_ping = true;

static void ping_cb(esp_ping_handle_t hdl, void *args) {
    // Turn on just for debug
    //Serial.println("Ping done");
    wait_ping = false;
    esp_ping_stop(hdl);
    esp_ping_delete_session(hdl);
}

static void ping_cb_fail(esp_ping_handle_t hdl, void *args) {
    //Serial.println("Ping Fail");
    wait_ping = false;
    esp_ping_stop(hdl);
    esp_ping_delete_session(hdl);
}

void ping_target(ip_addr_t target) {
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target;
    ping_config.count = 4; // Number of pings
    /* set callback functions */
    esp_ping_callbacks_t cbs = {
        .cb_args = NULL,
        .on_ping_success = ping_cb,
        .on_ping_timeout = ping_cb_fail,
        .on_ping_end = ping_cb_fail,
    };
    esp_ping_handle_t ping_handle;
    esp_err_t err = esp_ping_new_session(&ping_config, &cbs, &ping_handle);
    if (err == ESP_OK) {
        esp_ping_start(ping_handle); // Non-blocking; replies handled in callback
    } else {
        wait_ping = false;
        Serial.printf("\nesp_ping_new_session with error: %s\n\n", esp_err_to_name(err));
    }
}

void ARPScanner::setup() {
    LOCK_TCPIP_CORE();
    hostslist_eth.clear();

    options.clear();
    // IPAddress uint32_t op returns number in big-endian
    // for simplicity of iteration and arithmetics convert to little-endian

    esp_netif_ip_info_t ip_info;

    if (esp_netif_get_ip_info(esp_net_interface, &ip_info) != ESP_OK) {
        Serial.println("Can't get IP informations");
        return;
    }

    if (ip_info.ip.addr == 0 || ip_info.netmask.addr == 0) {
        Serial.println("Ethernet has no IP/netmask, aborting ARP scan");
        displayError("Ethernet not ready", true);
        return;
    }

    ip_info.ip.addr = ntohl(ip_info.ip.addr);
    ip_info.netmask.addr = ntohl(ip_info.netmask.addr);
    gateway = ip_info.gw.addr;

    const uint32_t networkAddress = ntohl(gateway) & ip_info.netmask.addr;
    const uint32_t broadcast = networkAddress | ~ip_info.netmask.addr;

    // get iface
    struct netif *net_iface = (struct netif *)esp_netif_get_netif_impl(esp_net_interface);
    if (net_iface == nullptr || net_iface->linkoutput == nullptr ||
        net_iface->hwaddr_len != MAC_ADDRESS_LENGTH) {
        Serial.println("Network interface not ready for ARP scan");
        return;
    }

    etharp_cleanup_netif(net_iface); // to avoid gateway duplication

    // send arp requests, read table each ARP_TABLE_SIZE requests
    uint16_t tableReadCounter = 0;
    uint32_t hostsScanned = 0;
    const uint32_t totalHosts = broadcast - networkAddress - 1;
    static uint32_t lastUpdate = 0;

    for (uint32_t ip_le = networkAddress + 1; ip_le < broadcast; ip_le++) {
        if (ip_le == ip_info.ip.addr || ip_le == gateway) continue;

        ip4_addr_t ip_be{htonl(ip_le)}; // big endian

        hostsScanned++;
        if (millis() - lastUpdate > 500) { // Update display every 500ms
            displayRedStripe(
                "Probing " + String(hostsScanned) + " of " + String(totalHosts) + " hosts",
                getComplementaryColor2(bruceConfig.priColor),
                bruceConfig.priColor
            );
            lastUpdate = millis();
        }

        err_t res = etharp_request(net_iface, &ip_be);

        if (res != ERR_OK) {
            Serial.println("Arp req for: " + IPAddress(ip_be.addr).toString() + "failed with ec: " + res);
        } else {
            ++tableReadCounter;
        }

        vTaskDelay(arpRequestDelay);

        // read table if we sent ARP_TABLE_SIZE requests
        if (tableReadCounter == ARP_TABLE_SIZE) {
            readArpTableETH(net_iface);
            tableReadCounter = 0;
        }
        // Stops search on EscPress
        if (check(EscPress)) break;
    }
    UNLOCK_TCPIP_CORE();
    auto it = std::find_if(hostslist_eth.begin(), hostslist_eth.end(), [this](const Host &host) {
        return host.ip == gateway;
    });

    // Sometimes happens that gateway is not scanned, so force ping and then read again ARP table
    if (it == hostslist_eth.end()) {
        ip_addr_t target;
        target.type = IPADDR_TYPE_V4;
        target.u_addr.ip4.addr = gateway;
        ping_target(target); // Ping target to force ARP request

        while (wait_ping) { delay(1); }

        struct eth_addr *eth_ret = NULL;
        const ip4_addr_t *ipaddr_ret = NULL;
        ip4_addr_t gateway_ip;
        gateway_ip.addr = gateway;

        // Search gateway in the ARP table
        s8_t arp_find_result = etharp_find_addr(net_iface, &gateway_ip, &eth_ret, &ipaddr_ret);

        if (arp_find_result < 0) { Serial.println("Gateway MAC not found."); }

        readArpTableETH(net_iface); // Update hostlists
    }

ScanHostMenu:
    if (hostslist_eth.empty()) {
        tft.println("No hosts found");
        delay(2000);
        return;
    }

    for (auto host : hostslist_eth) {
        Serial.println(host.ip.toString());
        String result = host.ip.toString();
        if (host.ip == gateway) result += "(GTW)";
        options.push_back({result.c_str(), [this, host]() { afterScanOptions(host); }});
    }
    addOptionToMainMenu();

    loopOptions(options);
    options.clear();

    if (!returnToMenu) goto ScanHostMenu;
    hostslist_eth.clear();
}

bool ARPScanner::macStringToByteArray(const std::string &macStr, uint8_t macArray[6]) {
    std::istringstream iss(macStr);
    std::string byteStr;
    int byteCount = 0;

    // Remove delimiters and convert to byte array
    while (std::getline(iss, byteStr, ':') || std::getline(iss, byteStr, '-')) {
        if (byteCount >= 6) {
            // More than 6 bytes; invalid MAC address
            return false;
        }

        try {
            uint8_t byte = std::stoul(byteStr, nullptr, 16);
            macArray[byteCount++] = byte;
        } catch (const std::invalid_argument &e) {
            // Invalid byte value; not a valid hexadecimal number
            return false;
        } catch (const std::out_of_range &e) {
            // Byte value out of range; not a valid byte
            return false;
        }
    }

    // Check if exactly 6 bytes were parsed
    return byteCount == 6;
}
void ARPScanner::afterScanOptions(const Host &host) {
    int opt = 0;
    IPAddress gw = gateway;
    options = {
        {"Host info",
         [=]() {
             HostInfo(
                 host, wifiConnected
             ); // At this point we are sure that if user use WiFi is connected for sure
         }},
#ifndef LITE_VERSION
        {"SSH Connect", lambdaHelper(ssh_setup, host.ip.toString())},
#endif
        {"Station Deauth",
         [=]() {
             if (!wifiConnected) {
                 displayError("Station deauth not available on ethernet");
             } else {
                 stationDeauth(host);
             }
         }},
        {"ARP Spoofing",
         [this, host, gw]() {
             auto it = std::find_if(hostslist_eth.begin(), hostslist_eth.end(), [this](const Host &host) {
                 return host.ip == gateway;
             });

             uint8_t mac[6];
             esp_err_t err = esp_netif_get_mac(esp_net_interface, mac);
             if (err == ESP_OK) {
                 uint8_t gw_mac[6];
                 macStringToByteArray((*it).mac.c_str(), gw_mac);
                 ARPSpoofer(host, gw, gw_mac, mac, false);
             } else {
                 ESP_LOGE("MAC Address", "Failed to get MAC address: %s", esp_err_to_name(err));
             }
         }},
#if !defined(LITE_VERSION)
        {"ARP Poisoning", [this]() { ARPoisoner{gateway}; }},
        {"DHCP Starvation", [=]() { DHCPStarvation(); }      },
        {"MAC Flooding",    [=]() { MACFlooding(); }         },
#endif
    };
    // if(sdcardMounted && bruceConfig.devMode) options.push_back({"ARP MITM (WIP)",  [&](){ opt=5;  }});
    loopOptions(options);
    if (opt == 5) {
        Serial.println("Starting MITM");
        // arpSpoofing(host, true);
    }
}
