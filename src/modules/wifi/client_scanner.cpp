#include "client_scanner.h"
#include "core/display.h"
#include "core/radio_mem.h"
#include "core/wifi/wifi_common.h" // wifiDisconnect()
#include "core/scrollableTextArea.h"
#include "core/wifi/webInterface.h" // cleanlyStopWebUiForWiFiFeature()
#include "modules/ble/ble_common.h" // stopBLEStack(), FORCE_RADIO_TEARDOWN_ON_SWITCH
#include "modules/wifi/wifi_atks.h" // capture_handshake()
#include "sniffer.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <globals.h>
#include <vector>

// Dwell per channel: beacons arrive every ~100 ms but clients only reveal
// themselves through traffic, so listen long. Accuracy over speed: a full
// 13-channel sweep takes about a minute. Quiet clients that send nothing
// during the dwell stay invisible — that is inherent to passive observation,
// which is why every channel visit starts with one stimulative deauth burst
// that forces re-auth traffic into the open.
#define CLIENT_SCAN_DWELL_MS 4000
#define CLIENT_SCAN_MAX_APS 64
#define CLIENT_SCAN_MAX_DETAIL 24

struct ScannedAp {
    uint8_t bssid[6];
    String ssid;
    String authStr;
    uint8_t channel;
    int32_t rssi;
    uint16_t clients;
};

static void clientScanTeardown() {
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    sniffer_set_mode(SnifferMode::HandshakesOnly); // restore default for other features
    // Full disconnect (not just esp_wifi_stop): clears the mode to OFF so the
    // status-bar WiFi icon actually disappears when we leave unconnected.
    wifiDisconnect();
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void macToStr(const uint8_t mac[6], char *out, size_t outLen) {
    snprintf(
        out, outLen, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );
}

static const char *rsnSuiteName(const uint8_t *r, int o, int elen) {
    if (o + 4 > elen) return "?";
    if (r[o] != 0x00 || r[o + 1] != 0x0F || r[o + 2] != 0xAC) return "UNK";
    switch (r[o + 3]) {
        case 0: return "NONE";
        case 1: return "WEP40";
        case 2: return "TKIP";
        case 4: return "CCMP";
        case 5: return "WEP104";
        case 6: return "BIP";
        case 8: return "GCMP";
        case 9: return "GCMP256";
        default: return "?";
    }
}

static const char *rsnAkmName(const uint8_t *r, int o, int elen) {
    if (o + 4 > elen) return "?";
    if (r[o] != 0x00 || r[o + 1] != 0x0F || r[o + 2] != 0xAC) return "UNK";
    switch (r[o + 3]) {
        case 1: return "802.1X";
        case 2: return "PSK";
        case 5: return "PSK-SHA256";
        case 6: return "802.1X-SHA256";
        case 7: return "TDLS";
        case 8: return "SAE";
        default: return "?";
    }
}

// Parse RSN IE (id 48) out of a raw beacon frame. Fills human-readable
// strings; returns false when no RSN IE is present (open/WEP networks).
static bool parseRsnInfo(
    const uint8_t *frame, uint16_t len, char *out, size_t outLen, bool *mfpc, bool *mfpr
) {
    if (!frame || len < 38 || !out || outLen == 0) return false;
    int off = 24 + 12; // 802.11 header + fixed beacon params
    while (off + 2 <= len) {
        uint8_t id = frame[off];
        uint8_t el = frame[off + 1];
        if (off + 2 + el > len) break;
        if (id == 48 && el >= 20) {
            const uint8_t *r = frame + off + 2;
            uint16_t pc = r[6] | ((uint16_t)r[7] << 8);
            int po = 8; // first pairwise suite
            int ao = po + 4 * (pc ? 1 : 0); //akm count offset (use first suite only)
            // akm count lives right after the pairwise list:
            int akmCountOff = 8 + 4 * pc;
            int akmOff = akmCountOff + 2;
            const char *pair = rsnSuiteName(r, po, el);
            const char *group = rsnSuiteName(r, 2, el);
            const char *akm = "?";
            int capOff = akmOff + 4;
            if (akmCountOff + 2 <= el) {
                uint16_t ac = r[akmCountOff] | ((uint16_t)r[akmCountOff + 1] << 8);
                if (ac > 0) akm = rsnAkmName(r, akmOff, el);
                capOff = akmOff + 4 * (ac ? 1 : 0);
            }
            bool mfpcL = false, mfprL = false;
            if (capOff + 2 <= el) {
                mfpcL = (r[capOff] & 0x80) != 0;
                mfprL = (r[capOff + 1] & 0x01) != 0;
            }
            if (mfpc) *mfpc = mfpcL;
            if (mfpr) *mfpr = mfprL;
            snprintf(out, outLen, "%s/%s/%s%s", group, pair, akm, mfpcL ? "+MFPC" : "");
            (void)ao;
            return true;
        }
        off += 2 + el;
    }
    return false;
}

static String authToString(int enc) {
    switch (enc) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA/PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2/PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2/PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2/Enterprise";
        case WIFI_AUTH_WPA3_PSK: return "WPA3/PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3/PSK";
        default: return "Unknown";
    }
}

static void showApInformation(const ScannedAp &ap) {
    ScrollableTextArea area("AP INFO");
    char macStr[18];
    macToStr(ap.bssid, macStr, sizeof(macStr));
    area.addLine("SSID: " + (ap.ssid.length() ? ap.ssid : String("HIDDEN")));
    area.addLine("BSSID: " + String(macStr));
    // NOTE: no vendor lookup here — getManufacturer() needs internet (HTTP API),
    // which is unavailable while promiscuously scanning unjoined networks, and
    // each lookup would block the UI on connection timeouts.
    area.addLine("Channel: " + String(ap.channel) + "  RSSI: " + String(ap.rssi) + "dBm");
    area.addLine("Auth: " + ap.authStr);

    uint8_t bcn[256];
    uint16_t bl = sniffer_get_beacon_frame(ap.bssid, bcn, sizeof(bcn));
    char rsn[48];
    bool mfpc = false, mfpr = false;
    if (bl > 0 && parseRsnInfo(bcn, bl, rsn, sizeof(rsn), &mfpc, &mfpr)) {
        area.addLine("Cipher: " + String(rsn));
        if (mfpr) area.addLine("PMF: required (deauth blocked)");
        else if (mfpc) area.addLine("PMF: capable");
    } else {
        area.addLine("Cipher: n/a (no beacon captured)");
    }

    uint64_t apKey = 0;
    for (int i = 0; i < 6; i++) { apKey = (apKey << 8) | ap.bssid[i]; }
    if (sniffer_is_handshake_ready(apKey)) area.addLine("Handshake: CAPTURED");
    else if (sniffer_is_handshake_crackable(apKey)) area.addLine("Handshake: crackable (M1+M2)");
    else if (sniffer_ap_has_partial(apKey)) area.addLine("Handshake: partial");
    else area.addLine("Handshake: none");

    ClientDetail det[CLIENT_SCAN_MAX_DETAIL];
    uint8_t nDet = sniffer_get_client_details(apKey, det, CLIENT_SCAN_MAX_DETAIL);
    area.addLine("Clients: " + String(ap.clients));
    char ipStr[16], cMac[18];
    for (uint8_t i = 0; i < nDet; i++) {
        macToStr(det[i].mac, cMac, sizeof(cMac));
        if (det[i].ip[0] || det[i].ip[1] || det[i].ip[2] || det[i].ip[3]) {
            snprintf(
                ipStr, sizeof(ipStr), "%u.%u.%u.%u", det[i].ip[0], det[i].ip[1], det[i].ip[2],
                det[i].ip[3]
            );
        } else {
            snprintf(ipStr, sizeof(ipStr), "no IP seen");
        }
        area.addLine(String(cMac) + " " + String(ipStr));
        area.addLine("  " + String(det[i].frames) + " frames");
    }
    area.show();
}

void clientScannerMenu() {
    if (!radioHasMemForWifi()) {
        displayError("Low RAM: free BLE/SD first", true);
        return;
    }
    cleanlyStopWebUiForWiFiFeature();
    if (FORCE_RADIO_TEARDOWN_ON_SWITCH) {
        stopBLEStack();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // --- Step 1: AP inventory via active scan ---
    displayTextLine("Scanning APs..");
    WiFi.mode(WIFI_MODE_STA);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    int nets = WiFi.scanNetworks();
    std::vector<ScannedAp> aps;
    for (int i = 0; i < nets && (int)aps.size() < CLIENT_SCAN_MAX_APS; i++) {
        ScannedAp ap;
        const uint8_t *b = WiFi.BSSID(i);
        if (!b) continue;
        memcpy(ap.bssid, b, 6);
        ap.ssid = WiFi.SSID(i);
        ap.authStr = authToString(WiFi.encryptionType(i));
        ap.channel = (uint8_t)WiFi.channel(i);
        if (ap.channel < 1 || ap.channel > 14) continue;
        ap.rssi = WiFi.RSSI(i);
        ap.clients = 0;
        aps.push_back(ap);
    }
    WiFi.scanDelete();
    if (aps.empty()) {
        displayError("No networks found", true);
        wifiDisconnect();
        return;
    }

    // Unique channel list in first-seen order.
    uint8_t channels[14];
    uint8_t nCh = 0;
    for (const auto &ap : aps) {
        bool known = false;
        for (uint8_t i = 0; i < nCh; i++) {
            if (channels[i] == ap.channel) {
                known = true;
                break;
            }
        }
        if (!known && nCh < 14) channels[nCh++] = ap.channel;
    }

    // Stimulated scan is the only mode: one deauth burst per AP forces
    // re-auth traffic into the open, which is what makes quiet clients
    // visible. Passive-only listening misses too much to be useful.
    // --- Step 2: promiscuous sweep with stimulative bursts ---
    sniffer_reset_handshake_cache();
    registeredBeacons.clear();
    sniffer_set_mode(SnifferMode::Passive);
    // Arduino API only (no raw esp_wifi_init: the stack is already up from the
    // scan above). AP mode keeps the radio on without joining anything.
    WiFi.mode(WIFI_MODE_AP);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(sniffer);

    drawMainBorderWithTitle("Client Scanner");
    bool aborted = false;
    for (uint8_t c = 0; c < nCh; c++) {
        // Keep the sniffer's beacon channel attribution correct:
        // all_wifi_channels[] is 1..12, so index = channel - 1.
        ch = (channels[c] >= 1 && channels[c] <= 12) ? channels[c] - 1 : 0;
        esp_wifi_set_channel(channels[c], WIFI_SECOND_CHAN_NONE);
        // Fresh header per channel so the progress text never floods the screen.
        drawMainBorderWithTitle("Client Scanner");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        padprintln("");
        padprintln("Listening ch " + String(channels[c]) + " (" + String(c + 1) + "/" + String(nCh) + ")");
        padprintln("Esc cancels");
        // One broadcast burst per AP on this channel: knocked clients
        // re-authenticate within the dwell below and reveal themselves.
        for (const auto &ap : aps) {
            if (ap.channel != channels[c]) continue;
            wifi_ap_record_t stimRec;
            memset(&stimRec, 0, sizeof(stimRec));
            memcpy(stimRec.bssid, ap.bssid, 6);
            wsl_bypasser_send_raw_frame(&stimRec, channels[c], _default_target);
            send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
            if (check(EscPress)) {
                aborted = true;
                break;
            }
        }
        // Live activity line: spinner + running client total, redrawn in place
        // so the user sees the scan is alive.
        const char *spin = "|/-\\";
        uint8_t spinIdx = 0;
        int16_t spinY = tft.getCursorY();
        unsigned long lastSpin = 0;
        unsigned long dwellStart = millis();
        while (millis() - dwellStart < CLIENT_SCAN_DWELL_MS) {
            if (millis() - lastSpin >= 200) {
                lastSpin = millis();
                tft.setCursor(10, spinY);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.print(
                    String("[") + spin[spinIdx++ % 4] + "] " + String(sniffer_total_clients()) +
                    " clients heard   "
                );
            }
            if (check(EscPress)) {
                aborted = true;
                break;
            }
            vTaskDelay(150 / portTICK_PERIOD_MS);
        }
        if (aborted) break;
    }

    // --- Step 3: tally + sort (busiest first, then strongest signal) ---
    uint16_t totalClients = 0;
    for (auto &ap : aps) {
        ap.clients = sniffer_count_clients(ap.bssid);
        totalClients += ap.clients;
    }
    std::sort(aps.begin(), aps.end(), [](const ScannedAp &a, const ScannedAp &b) {
        if (a.clients != b.clients) return a.clients > b.clients;
        return a.rssi > b.rssi;
    });
    clientScanTeardown();

    if (aborted && aps.empty()) return;

    drawMainBorderWithTitle("Client Scanner");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    padprintln("");
    padprintln(String(aps.size()) + " APs, " + String(totalClients) + " clients heard");
    padprintln("Quiet clients may not appear.");
    vTaskDelay(1500 / portTICK_PERIOD_MS);

    // --- Step 4: results; selecting an AP offers Information / Capture ---
    while (true) {
        options.clear();
        for (const auto &ap : aps) {
            String label = ap.ssid.length() ? ap.ssid : "HIDDEN";
            label += " ch" + String(ap.channel) + " (" + String(ap.clients) + ")";
            Option opt(label, []() {});
            options.push_back(opt);
        }
        int idx = loopOptions(options);
        if (idx < 0 || idx >= (int)aps.size()) break; // Esc: back to WiFi menu
        const ScannedAp &ap = aps[idx];

        options.clear();
        options.push_back({"Information", []() {}});
        options.push_back({"Capture Handshake", []() {}});
        int act = loopOptions(options);
        if (act == 0) {
            showApInformation(ap);
            drawMainBorderWithTitle("Client Scanner");
        } else if (act == 1) {
            char macStr[18];
            macToStr(ap.bssid, macStr, sizeof(macStr));
            capture_handshake(ap.ssid, String(macStr), ap.channel);
            // capture tore the radio down; results need no radio, loop again.
            drawMainBorderWithTitle("Client Scanner");
        }
        // Esc on the submenu (-1): fall through and reshow results.
    }
}
