#include "eapol_ident.h"

#include "core/display.h"
#include "core/scrollableTextArea.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <globals.h>
#include <string.h>

// EAPOL rides in ordinary 802.11 data frames with an LLC/SNAP header whose
// ethertype is 0x888E. The sniffer sees those frames even when the client is
// still associating, which is exactly when the identity goes out in the clear.
namespace EapolIdent {

namespace {

struct IdentityEntry {
    uint8_t bssid[6];
    char user[64];
    bool used;
};

static IdentityEntry s_ids[8];
static uint16_t s_frames = 0;
static uint16_t s_starts = 0;
static uint16_t s_logoffs = 0;
static uint16_t s_peap = 0;
static uint16_t s_ttls = 0;
static uint16_t s_md5 = 0;
static uint16_t s_other = 0;
static uint8_t s_channel = 1;

static bool sameMac(const uint8_t *a, const uint8_t *b) { return memcmp(a, b, 6) == 0; }

static void macStr(const uint8_t *m, char *out, size_t n) {
    snprintf(out, n, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}

static void countMethod(uint8_t method) {
    switch (method) {
        case 25: s_peap++; break;      // EAP-PEAP, the one this task is about
        case 21: s_ttls++; break;      // EAP-TTLS
        case 4: s_md5++; break;        // EAP-MD5, cleartext-challenge legacy
        default: s_other++; break;
    }
}

// The identity field is a human-readable realm user; anything that is not
// printable is dropped rather than written as mojibake into the report.
static void noteIdentity(const uint8_t *bssid, const uint8_t *data, int len) {
    char user[64];
    int j = 0;
    for (int i = 0; i < len && j < 63; i++) {
        const uint8_t c = data[i];
        if (c < 0x20 || c > 0x7E) continue;
        user[j++] = (char)c;
    }
    user[j] = 0;
    if (j == 0) return;

    for (int i = 0; i < 8; i++) {
        if (s_ids[i].used && sameMac(s_ids[i].bssid, bssid) && strcmp(s_ids[i].user, user) == 0) return;
    }
    for (int i = 0; i < 8; i++) {
        if (!s_ids[i].used) {
            s_ids[i].used = true;
            memcpy(s_ids[i].bssid, bssid, 6);
            strncpy(s_ids[i].user, user, sizeof(s_ids[i].user) - 1);
            s_ids[i].user[sizeof(s_ids[i].user) - 1] = 0;
            return;
        }
    }
}

static void packetCb(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_DATA) return;
    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
    const uint8_t *p = pkt->payload;
    const int len = pkt->rx_ctrl.sig_len;
    if (len < 24 + 8 + 4) return;

    const uint8_t fc1 = p[1];
    const bool toDS = (fc1 & 0x01) != 0;
    const bool fromDS = (fc1 & 0x02) != 0;
    int hdr = 24;
    if (toDS && fromDS) hdr = 30;
    if (((p[0] >> 4) & 0x8) != 0) hdr += 2; // QoS data
    if (hdr + 8 + 4 > len) return;

    const uint8_t *snap = p + hdr;
    if (!(snap[0] == 0xAA && snap[1] == 0xAA && snap[2] == 0x03 && snap[6] == 0x88 && snap[7] == 0x8E)) return;

    const uint8_t *eapol = p + hdr + 8;
    const int avail = len - hdr - 8 - 4; // payload minus FCS
    if (avail < 4) return;

    s_frames++;
    const uint8_t eapolType = eapol[1];
    if (eapolType == 1) { // EAPOL-Start: a client is about to authenticate
        s_starts++;
        return;
    }
    if (eapolType == 2) { // EAPOL-Logoff
        s_logoffs++;
        return;
    }
    if (eapolType != 0 || avail < 9) return;

    const uint8_t *eap = eapol + 4;
    const uint8_t code = eap[0];
    const uint16_t eapLen = (uint16_t)eap[2] | ((uint16_t)eap[3] << 8);
    if (eapLen < 5) return;
    const uint8_t method = eap[4];
    // Request (1) tells us what the authenticator asked for; Response (2) is the
    // client answering with its identity. Both are useful, neither is secret.
    if (code == 1 || code == 2) {
        countMethod(method);
        if (method == 1) {
            int n = (int)eapLen - 5;
            if (n > avail - 9) n = avail - 9;
            if (n > 0) noteIdentity(p + 16, eap + 5, n);
        }
    }
}

} // namespace

String harvest(int seconds) {
    if (seconds < 1) seconds = 1;
    if (seconds > 120) seconds = 120;

    memset(s_ids, 0, sizeof(s_ids));
    s_frames = s_starts = s_logoffs = s_peap = s_ttls = s_md5 = s_other = 0;

    // Same setup as the counter-surveillance pass: a promiscuous STA with no
    // association, hopping the 2.4 GHz channels the driver actually allows.
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_STA);
    wifiConnected = false;
    wifiIP = "";

    wifi_promiscuous_filter_t filter = {};
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA;
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(packetCb);
    esp_wifi_set_promiscuous(true);

    const uint32_t end = millis() + (uint32_t)seconds * 1000UL;
    while ((int32_t)(millis() - end) < 0) {
        s_channel = (s_channel % 13) + 1;
        esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
        delay(80);
    }
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    WiFi.mode(WIFI_OFF);
    wifiConnected = false;
    wifiIP = "";

    String out = "eapol: " + String(s_frames) + " EAPOL frames, " + String(s_starts) + " start, " +
                 String(s_logoffs) + " logoff in " + String(seconds) + "s\n";

    int shown = 0;
    for (int i = 0; i < 8; i++) {
        if (!s_ids[i].used) continue;
        char mac[18];
        macStr(s_ids[i].bssid, mac, sizeof(mac));
        out += "  identity \"" + String(s_ids[i].user) + "\" -> " + String(mac) + "\n";
        shown++;
    }
    if (shown == 0) out += "  no identity in the clear";
    else out += "  " + String(shown) + " outer identity(ies)";
    out += "\n";

    if (s_frames > 0) {
        out += "  methods requested: PEAP " + String(s_peap) + ", TTLS " + String(s_ttls) + ", MD5 " +
               String(s_md5) + ", other " + String(s_other) + "\n";
    } else {
        out += "  no 802.1X client seen (nothing to harvest)\n";
    }
    // Say what this is and is not: the MSCHAPv2 challenge/response lives inside
    // the TLS tunnel, so a passive listener can never see it.
    out += "  outer identity only; PEAP tunnel needs a TLS server, not attempted\n";
    return out;
}

void menu() {
    ScrollableTextArea area = ScrollableTextArea("EAPOL Harvest");
    area.fromString(harvest(20));
    area.show();
    returnToMenu = false;
}

} // namespace EapolIdent
