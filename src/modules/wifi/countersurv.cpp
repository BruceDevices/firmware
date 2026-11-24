#include "countersurv.h"

#include "core/odid.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <globals.h>
#include <string.h>

namespace CounterSurv {

struct ApEntry {
    uint8_t bssid[6];
    char ssid[33];
    uint8_t channel;
    bool used;
};

struct SsidObs {
    uint8_t bssid[6];
    char ssid[33];
    bool used;
};

struct DeauthCount {
    uint8_t bssid[6];
    uint16_t count;
    bool used;
};

struct FpEntry {
    uint32_t hash;
    uint8_t mac[6];
    uint32_t firstMs;
    uint32_t lastMs;
    uint16_t seen;
    bool used;
};

struct ArpEntry {
    uint32_t ip;
    uint8_t mac[6];
    bool used;
};

static ApEntry s_aps[24];
static SsidObs s_obs[64];
static DeauthCount s_deauth[16];
static FpEntry s_fp[32];
static ArpEntry s_arp[16];

static uint16_t s_deauthTotal = 0;
static uint16_t s_arpConflicts = 0;
static uint16_t s_evilTwins = 0;
static uint16_t s_karma = 0;
static uint16_t s_rotations = 0;
static uint16_t s_probes = 0;
static uint16_t s_drones = 0;
static char s_droneId[21] = {0};
static uint16_t s_droneAltitude = 0;

static bool s_fpOn = false;
static uint8_t s_channel = 1;

static bool sameMac(const uint8_t *a, const uint8_t *b) { return memcmp(a, b, 6) == 0; }

static void copySsid(char *dst, const uint8_t *ie, int len) {
    if (len > 32) len = 32;
    memcpy(dst, ie, len);
    dst[len] = 0;
}

// Iterate the tagged parameters of a management frame starting at `off`.
static void scanIes(const uint8_t *p, int len, int off, void (*cb)(uint8_t, const uint8_t *, int, void *), void *ctx) {
    while (off + 2 <= len) {
        const uint8_t id = p[off];
        const int l = p[off + 1];
        if (off + 2 + l > len) break;
        cb(id, p + off + 2, l, ctx);
        off += 2 + l;
    }
}

struct SsidCtx {
    char *out;
};
static void ssidCb(uint8_t id, const uint8_t *d, int l, void *ctx) {
    if (id != 0) return;
    SsidCtx *c = (SsidCtx *)ctx;
    if (c->out[0] == 0) copySsid(c->out, d, l);
}

struct OdidCtx {
    bool found;
    OdidData data;
};
static void odidCb(uint8_t id, const uint8_t *d, int l, void *ctx) {
    if (id != 221 || l < 4) return;
    OdidCtx *c = (OdidCtx *)ctx;
    OdidData out;
    if (odidDecodeBeaconVendorIe(d - 2, l + 2, out) && out.valid) {
        c->found = true;
        c->data = out;
    }
}

static void noteAp(const uint8_t *bssid, const char *ssid, uint8_t ch) {
    if (ssid[0] == 0) return;
    for (int i = 0; i < 24; i++) {
        if (s_aps[i].used && sameMac(s_aps[i].bssid, bssid)) {
            if (strncmp(s_aps[i].ssid, ssid, 32) != 0) {
                // The same BSSID advertising a second SSID is what KARMA looks
                // like from the air.
                for (int k = 0; k < 64; k++) {
                    if (!s_obs[k].used) {
                        memcpy(s_obs[k].bssid, bssid, 6);
                        strncpy(s_obs[k].ssid, ssid, 32);
                        s_obs[k].used = true;
                        break;
                    }
                }
            }
            return;
        }
    }
    for (int i = 0; i < 24; i++) {
        if (!s_aps[i].used) {
            memcpy(s_aps[i].bssid, bssid, 6);
            strncpy(s_aps[i].ssid, ssid, 32);
            s_aps[i].channel = ch;
            s_aps[i].used = true;
            return;
        }
    }
}

static void noteFingerprint(const uint8_t *mac, const uint8_t *ies, int len) {
    if (!s_fpOn || len <= 0) return;
    s_probes++;
    uint32_t h = 2166136261u;
    for (int i = 0; i < len; i++) {
        h ^= ies[i];
        h *= 16777619u;
    }
    const uint32_t now = millis();
    for (int i = 0; i < 32; i++) {
        if (s_fp[i].used && s_fp[i].hash == h && sameMac(s_fp[i].mac, mac)) {
            s_fp[i].lastMs = now;
            if (s_fp[i].seen < 0xFFFF) s_fp[i].seen++;
            return;
        }
    }
    for (int i = 0; i < 32; i++) {
        if (s_fp[i].used && s_fp[i].hash == h && !sameMac(s_fp[i].mac, mac) && now - s_fp[i].lastMs < 60000) {
            s_rotations++; // same IE set, new MAC: one device rotating
            s_fp[i].lastMs = now;
            return;
        }
    }
    for (int i = 0; i < 32; i++) {
        if (!s_fp[i].used) {
            s_fp[i].used = true;
            s_fp[i].hash = h;
            memcpy(s_fp[i].mac, mac, 6);
            s_fp[i].firstMs = now;
            s_fp[i].lastMs = now;
            s_fp[i].seen = 1;
            return;
        }
    }
}

static void noteDeauth(const uint8_t *bssid) {
    s_deauthTotal++;
    for (int i = 0; i < 16; i++) {
        if (s_deauth[i].used && sameMac(s_deauth[i].bssid, bssid)) {
            if (s_deauth[i].count < 0xFFFF) s_deauth[i].count++;
            return;
        }
    }
    for (int i = 0; i < 16; i++) {
        if (!s_deauth[i].used) {
            memcpy(s_deauth[i].bssid, bssid, 6);
            s_deauth[i].count = 1;
            s_deauth[i].used = true;
            return;
        }
    }
}

static void noteArp(const uint8_t *payload, int len) {
    // 24 byte 802.11 header + 8 byte LLC/SNAP (AA AA 03 00 00 00 08 06)
    if (len < 24 + 8 + 28) return;
    const uint8_t *p = payload + 32;
    if (!(p[-2] == 0x08 && p[-1] == 0x06)) return;
    if (p[0] != 0x00 || p[1] != 0x01) return; // Ethernet
    const uint8_t *sha = p + 8;
    const uint8_t *spa = p + 14;
    const uint32_t ip = (uint32_t)spa[0] | ((uint32_t)spa[1] << 8) | ((uint32_t)spa[2] << 16) |
                        ((uint32_t)spa[3] << 24);
    if (ip == 0) return;
    for (int i = 0; i < 16; i++) {
        if (s_arp[i].used && s_arp[i].ip == ip && !sameMac(s_arp[i].mac, sha)) {
            s_arpConflicts++;
            return;
        }
        if (!s_arp[i].used) {
            s_arp[i].used = true;
            s_arp[i].ip = ip;
            memcpy(s_arp[i].mac, sha, 6);
            return;
        }
    }
}

static void packetCb(void *buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
    const uint8_t *p = pkt->payload;
    const int len = pkt->rx_ctrl.sig_len;
    if (len < 24) return;
    const uint8_t ftype = (p[0] >> 2) & 0x3;
    const uint8_t subtype = (p[0] >> 4) & 0xF;

    if (ftype == 0) {
        if (subtype == 8 || subtype == 5) { // beacon / probe response
            char ssid[33] = {0};
            SsidCtx sc{ssid};
            scanIes(p, len, 36, ssidCb, &sc);
            noteAp(p + 16, ssid, pkt->rx_ctrl.channel);
            if (subtype == 8) {
                OdidCtx oc{false, {}};
                scanIes(p, len, 36, odidCb, &oc);
                if (oc.found) {
                    s_drones++;
                    strncpy(s_droneId, oc.data.uasId, 20);
                    s_droneAltitude = (uint16_t)(oc.data.altGeo > 0 ? oc.data.altGeo : 0);
                }
            }
            return;
        }
        if (subtype == 4) { // probe request
            noteFingerprint(p + 10, p + 24, len - 24);
            return;
        }
        if (subtype == 12 || subtype == 10) { // deauth / disassoc
            noteDeauth(p + 16);
            return;
        }
    }
    if (ftype == 2) noteArp(p, len);
}

static void buildReport(String &out) {
    out += "counter-surveillance: " + String(s_deauthTotal) + " deauth/disassoc, " + String(s_evilTwins) +
           " evil twin, " + String(s_karma) + " karma, " + String(s_arpConflicts) + " arp conflict, " +
           String(s_drones) + " remote id\n";

    if (s_deauthTotal >= 20) out += "ALERT deauth flood: " + String(s_deauthTotal) + " frames\n";
    for (int i = 0; i < 16; i++) {
        if (s_deauth[i].used && s_deauth[i].count >= 10) {
            char mac[18];
            snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", s_deauth[i].bssid[0], s_deauth[i].bssid[1],
                     s_deauth[i].bssid[2], s_deauth[i].bssid[3], s_deauth[i].bssid[4], s_deauth[i].bssid[5]);
            out += "  flood source " + String(mac) + " (" + String(s_deauth[i].count) + ")\n";
        }
    }

    // Evil twin: same SSID, different BSSID, different OUI.
    for (int i = 0; i < 24; i++) {
        if (!s_aps[i].used) continue;
        for (int j = i + 1; j < 24; j++) {
            if (!s_aps[j].used) continue;
            if (strncmp(s_aps[i].ssid, s_aps[j].ssid, 32) != 0) continue;
            if (memcmp(s_aps[i].bssid, s_aps[j].bssid, 3) == 0) continue;
            s_evilTwins++;
            out += "ALERT evil twin: " + String(s_aps[i].ssid) + " seen from two OUIs\n";
        }
    }

    // KARMA: one BSSID, several SSIDs.
    for (int i = 0; i < 64; i++) {
        if (!s_obs[i].used) continue;
        int n = 1;
        for (int j = i + 1; j < 64; j++) {
            if (s_obs[j].used && sameMac(s_obs[i].bssid, s_obs[j].bssid)) n++;
        }
        if (n >= 3) {
            s_karma++;
            out += "ALERT karma: one BSSID answering " + String(n) + " SSIDs\n";
            break;
        }
    }

    if (s_drones > 0) {
        out += "remote id: " + String(s_droneId) + " alt~" + String(s_droneAltitude) + " m\n";
    }
    if (s_fpOn) {
        out += "fingerprints: " + String(s_probes) + " probes, " + String(s_rotations) + " mac rotations\n";
        int shown = 0;
        for (int i = 0; i < 32 && shown < 8; i++) {
            if (!s_fp[i].used) continue;
            char mac[18];
            snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", s_fp[i].mac[0], s_fp[i].mac[1],
                     s_fp[i].mac[2], s_fp[i].mac[3], s_fp[i].mac[4], s_fp[i].mac[5]);
            out += "  fp " + String((uint32_t)s_fp[i].hash, HEX) + " " + String(mac) + " x" +
                   String(s_fp[i].seen) + "\n";
            shown++;
        }
    }
}

String run(int seconds, bool fingerprint) {
    memset(s_aps, 0, sizeof(s_aps));
    memset(s_obs, 0, sizeof(s_obs));
    memset(s_deauth, 0, sizeof(s_deauth));
    memset(s_fp, 0, sizeof(s_fp));
    memset(s_arp, 0, sizeof(s_arp));
    s_deauthTotal = s_arpConflicts = s_evilTwins = s_karma = s_rotations = s_probes = s_drones = 0;
    s_droneId[0] = 0;
    s_fpOn = fingerprint;

    // Leave any station/AP session first: promiscuous capture on top of a
    // connected STA misses most management frames.
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_STA);
    wifiConnected = false;
    wifiIP = "";

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(packetCb);
    esp_wifi_set_promiscuous(true);

    const uint32_t end = millis() + (uint32_t)max(1, seconds) * 1000UL;
    while ((int32_t)(millis() - end) < 0) {
        s_channel = (s_channel % 13) + 1;
        esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
        delay(60);
    }
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    WiFi.mode(WIFI_OFF);
    wifiConnected = false;
    wifiIP = "";

    String out;
    buildReport(out);
    return out;
}

} // namespace CounterSurv
