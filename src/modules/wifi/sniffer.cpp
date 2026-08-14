/*
  ===========================================
       Copyright (c) 2017 Stefan Kremser
              github.com/spacehuhn
  ===========================================
*/
#if !defined(LITE_VERSION)
#include "sniffer.h"
/* include all necessary libraries */
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
// #include "esp_wifi_internal.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "lwip/err.h"
// #include "esp_event_loop.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include <algorithm>
#include <ctype.h>
#include <map>
#include <set>
#include <vector>

#include "FS.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/wifi/webInterface.h"
#include "core/wifi/wifi_common.h"
#include <Arduino.h>
#include <globals.h>
#if defined(ESP32)
#include "FS.h"
// #include "SD.h"
#else
#include <SPI.h>
#include <SdFat.h>
#endif
#include "modules/wifi/wifi_atks.h" // to use deauth frames and cmds

//===== SETTINGS =====//
#define FILENAME "raw_"
#define SAVE_INTERVAL 10            // save new file every 30s
#define CHANNEL_HOPPING true        // if true it will scan on all channels
#define HOP_INTERVAL 214            // in ms (only necessary if channelHopping is true)
#define DEAUTH_INTERVAL (15 * 1000) // Send deauth packets every ms
#define EAPOL_ONLY true

//===== Run-Time variables =====//
unsigned long lastTime = 0;
unsigned long lastChannelChange = 0;
uint32_t lastRedraw = 0;
uint8_t ch = 0;
bool rawFileOpen = false;
bool isLittleFS = true;
bool littleFsWasFull = false; // true when we exit because LittleFS ran out
volatile bool littleFsSpaceAvailable = true;
int num_EAPOL = 0;
int num_HS = 0;
uint32_t packet_counter = 0;
uint32_t deauth_counter = 0;
uint32_t beacon_frames = 0;
uint32_t start_time = 0;
long deauth_tmp = 0;

File _pcap_file;
File _deauth_file;
bool deauthFileOpen = false;
SnifferMode currentMode = SnifferMode::HandshakesOnly;
bool sdDetected = false;
FS *activeFs = &LittleFS;
SemaphoreHandle_t fileMutex = nullptr;
QueueHandle_t snifferQueue = nullptr;
TaskHandle_t snifferWriterHandle = nullptr;
StaticSemaphore_t fileMutexBuffer;
SemaphoreHandle_t handshakeMutex = nullptr;
StaticSemaphore_t handshakeMutexBuffer;
std::set<BeaconList> registeredBeacons;
std::set<String> SavedHS; // Saves the MAC of beacon HS detected in the session
String filename = "/BrucePCAP/" + (String)FILENAME + ".pcap";
String deauthFilename = "/BrucePCAP/deauth_0.pcap";
int deauthFileIndex = 0;
int rawFileIndex = 0;
std::map<uint64_t, String> beaconSsidCache;
const size_t MAX_CAPTURE_SSID_LEN = 32;
const size_t SNIFFER_QUEUE_DEPTH = 48;
std::set<uint64_t> handshakeReadyBssids;
portMUX_TYPE handshakeReadyMux = portMUX_INITIALIZER_UNLOCKED;
std::set<uint64_t> handshakeBeaconLogged;
std::map<uint64_t, HandshakeTracker> perApHandshakeTracker;

// --- 4-way handshake frame buffer ---
// Buffer M1, M2, and M3 in memory; flush all 4 frames when M4 arrives.
constexpr size_t EAPOL_BUF_SIZE = 256;
struct EapolFrame {
    uint8_t data[EAPOL_BUF_SIZE] = {0};
    uint16_t len = 0;
    uint32_t timestamp_sec = 0;
    uint32_t timestamp_usec = 0;
};
struct Eapol4WayBuffer {
    EapolFrame m1;
    EapolFrame m2;
    EapolFrame m3;
};
std::map<uint64_t, Eapol4WayBuffer> eapol4WayBuffer;

// --- Raw beacon cache (per AP) ---
// Keeps the last beacon frame seen for an AP so it can be written as the
// FIRST record of a handshake pcap (before M1..M4). wifi_recover.cpp's
// parser stops reading once it has M2+M3, so a beacon appended only after
// the handshake completes would never be seen; writing it up front fixes
// that without needing a live beacon after the handshake.
constexpr size_t BEACON_BUF_SIZE = 512;
// Cap on how many APs we keep raw beacons for at once. At ~570 B/entry this
// bounds the cache to ~36 KB even in very dense environments (hundreds of
// networks). Stale entries are also pruned by time in cleanupStaleBeacons();
// this cap only guards against bursts of many simultaneously-active APs.
constexpr size_t MAX_BEACON_CACHE = 64;
struct BeaconFrame {
    uint8_t data[BEACON_BUF_SIZE] = {0};
    uint16_t len = 0;
    uint32_t timestamp_sec = 0;
    uint32_t timestamp_usec = 0;
};
std::map<uint64_t, BeaconFrame> beaconRawCache;

// --- New globals for beacon last-seen tracking & cleanup ---
std::map<uint64_t, uint32_t> beaconLastSeen; // key = macToKey(mac) -> last seen millis()
const uint32_t BEACON_TIMEOUT_MS = 120000;   // 2 minutes
unsigned long lastBeaconCleanup = 0;

struct SnifferQueueItem {
    wifi_promiscuous_pkt_t *packet = nullptr;
    uint32_t ts_sec = 0;
    uint32_t ts_usec = 0;
    uint16_t raw_len = 0;
    wifi_promiscuous_pkt_type_t type = WIFI_PKT_MISC;
    bool isBeacon = false;
    bool isHandshakeFrame = false;
    bool isDeauthFrame = false;
    bool saveRaw = false;
    bool saveHandshake = false;
    bool saveDeauth = false;
    uint8_t bssid[6] = {0};
    char ssid[MAX_CAPTURE_SSID_LEN + 1] = {0};
};

struct FrameInfo {
    bool valid = false;
    bool isBeacon = false;
    bool isDeauth = false;
    bool isEapol = false;
    int eapolMsgNum = -1;
    uint8_t apAddr[6] = {0};
    uint64_t apKey = 0;
    String ssid;
};

static bool ensureSnifferBackend();
static void snifferWriterTask(void *param);
static wifi_promiscuous_pkt_t *duplicatePacket(const wifi_promiscuous_pkt_t *pkt, uint16_t length);
static void releasePacketCopy(wifi_promiscuous_pkt_t *packet);
static uint64_t macToKey(const void *mac); // changed to const void *
static void copyMac(uint8_t *dest, const uint8_t *src);
static String extractSsid(const wifi_promiscuous_pkt_t *packet);
static void copySsidToBuffer(const String &ssid, char *buffer, size_t len);
static String sanitizeSsid(const char *ssid);
static String macToHex(const uint8_t *mac);
static String buildHandshakePath(const uint8_t *mac, const char *ssid);
static bool handshakeFileExists(const String &path);
static bool shouldSaveBeaconForHandshake(const uint8_t *mac);
static void resetHandshakeTracking();
static bool handshakeRecordExists(const String &path);
static void registerHandshakeRecord(const String &path);
static bool handshakeBeaconRecorded(uint64_t key);
static void registerHandshakeBeacon(uint64_t key);
static void resetHandshakeBeaconCache();
static void ensureDirectories(FS &Fs);
static void openDeauthFile(FS &Fs);
static void closeRawFile();
static void closeDeauthFile();
static bool lockFileMutex(TickType_t ticks = portMAX_DELAY);
static void unlockFileMutex();
static String currentModeString();
static bool rawCaptureEnabled();
static bool handshakeCaptureEnabled();
static bool deauthCaptureEnabled();
static FrameInfo analyzeFrame(wifi_promiscuous_pkt_t *pkt);
static String resolveSsidForFrame(FrameInfo &info, const wifi_promiscuous_pkt_t *packet);
static void registerBeacon(const uint8_t *apAddr);

// --- New helper prototypes ---
static void cleanupStaleBeacons();
static size_t countActiveBeaconsOnChannel(uint8_t channel);
static std::vector<String> recentSsidsOnChannel(uint8_t channel, size_t maxItems = 5);

// --Deauth sent clean
bool deauth_displayed = false;
unsigned long deauth_display_ts = 0;
const unsigned long DEAUTH_MSG_MS = 1500; // how long to keep message visible (ms)

// message position/size (tweak width/height if needed)
const int DEAUTH_MSG_X = 10;
const int DEAUTH_MSG_Y = tftHeight - 27;
const int DEAUTH_MSG_W = 75;
const int DEAUTH_MSG_H = 8;
const uint16_t DEAUTH_BG = TFT_BLACK; // background color used to clear the text

//===== FUNCTIONS =====//

// Thank you 7h30th3r0n3 for helping me solve this issue! and for sharing your EAPOL/Handshake sniffer
// please, give stars to his project: https://github.com/7h30th3r0n3/Evil-M5Core2/

// Handshake detection
bool isItEAPOL(const wifi_promiscuous_pkt_t *packet) {
    const uint8_t *payload = packet->payload;
    int len = packet->rx_ctrl.sig_len;

    // length check to ensure packet is large enough for EAPOL (minimum length)
    if (len < (24 + 8 + 4)) { // 24 bytes for the MAC header, 8 for LLC/SNAP, 4 for EAPOL minimum
        return false;
    }

    // check for LLC/SNAP header indicating EAPOL payload
    // LLC: AA-AA-03, SNAP: 00-00-00-88-8E for EAPOL
    if (payload[24] == 0xAA && payload[25] == 0xAA && payload[26] == 0x03 && payload[27] == 0x00 &&
        payload[28] == 0x00 && payload[29] == 0x00 && payload[30] == 0x88 && payload[31] == 0x8E) {
        return true;
    }

    // handle QoS tagging which shifts the start of the LLC/SNAP headers by 2 bytes
    // check if the frame control field's subtype indicates a QoS data subtype (0x08)
    if ((payload[0] & 0x0F) == 0x08) {
        // Adjust for the QoS Control field and recheck for LLC/SNAP header
        if (payload[26] == 0xAA && payload[27] == 0xAA && payload[28] == 0x03 && payload[29] == 0x00 &&
            payload[30] == 0x00 && payload[31] == 0x00 && payload[32] == 0x88 && payload[33] == 0x8E) {
            return true;
        }
    }

    return false;
}

HandshakeTracker hsTracker;

bool handshakeUsable(const HandshakeTracker &hs) { return hs.msg1 && hs.msg2 && hs.msg3 && hs.msg4; }

// Analyze the EAPOL Message Number
int classifyEapolMessage(const wifi_promiscuous_pkt_t *pkt) {
    const uint8_t *payload = pkt->payload;
    // QoS frames add 2 bytes to MAC header
    int qosOffset = ((payload[0] & 0x0F) == 0x08) ? 2 : 0;

    // Offset to Key Information field:
    // MAC header (24 + qosOffset) + LLC/SNAP (8) + EAPOL header (4) + Descriptor Type (1)
    int keyInfoOffset = 24 + qosOffset + 8 + 4 + 1;

    if (pkt->rx_ctrl.sig_len < keyInfoOffset + 2) return -1; // safety check

    uint16_t keyInfo = (payload[keyInfoOffset] << 8) | payload[keyInfoOffset + 1];

    bool install = keyInfo & (1 << 6);
    bool ack = keyInfo & (1 << 7);
    bool mic = keyInfo & (1 << 8);
    bool secure = keyInfo & (1 << 9);

    if (ack && !mic && !install) return 1;            // Message 1
    if (!ack && mic && !install && !secure) return 2; // Message 2
    if (ack && mic && install) return 3;              // Message 3
    if (!ack && mic && !install && secure) return 4;  // Message 4

    return -1; // Unknown
}

bool matchesTargetAP(const wifi_promiscuous_pkt_t *pkt, const uint8_t targetBssid[6]) {
    const uint8_t *payload = pkt->payload;

    const uint8_t *addr1 = payload + 4;
    const uint8_t *addr2 = payload + 10;
    const uint8_t *addr3 = payload + 16; // BSSID

    return memcmp(addr1, targetBssid, 6) == 0 || memcmp(addr2, targetBssid, 6) == 0 ||
           memcmp(addr3, targetBssid, 6) == 0;
}

// Définition de l'en-tête d'un paquet PCAP
typedef struct pcaprec_hdr_s {
    uint32_t ts_sec;   /* timestamp secondes */
    uint32_t ts_usec;  /* timestamp microsecondes */
    uint32_t incl_len; /* nombre d'octets du paquet enregistrés dans le fichier */
    uint32_t orig_len; /* longueur réelle du paquet */
} pcaprec_hdr_t;

void saveHandshake(const wifi_promiscuous_pkt_t *packet, bool beacon, FS &Fs, const char *ssidLabel) {
    const uint8_t *addr1 = packet->payload + 4;
    const uint8_t *addr2 = packet->payload + 10;
    const uint8_t *bssid = packet->payload + 16;
    const uint8_t *apAddr;

    if (memcmp(addr1, bssid, 6) == 0) {
        apAddr = addr1;
    } else {
        apAddr = addr2;
    }

    uint64_t apKey = macToKey(apAddr);
    String sanitizedSsid = sanitizeSsid(ssidLabel);
    String filePath = buildHandshakePath(apAddr, sanitizedSsid.c_str());
    bool fichierExiste = handshakeFileExists(filePath);

    // Beacon: only save if handshake file already exists (handshake was captured)
    if (beacon) {
        if (!fichierExiste) { return; }
        uint64_t beaconKey = apKey;
        if (handshakeBeaconRecorded(beaconKey)) { return; }
        registerHandshakeBeacon(beaconKey);

        File f = Fs.open(filePath, FILE_APPEND);
        if (!f) { return; }
        pcaprec_hdr_t hdr;
        hdr.ts_sec = packet->rx_ctrl.timestamp / 1000000;
        hdr.ts_usec = packet->rx_ctrl.timestamp % 1000000;
        hdr.incl_len = packet->rx_ctrl.sig_len;
        hdr.orig_len = packet->rx_ctrl.sig_len;
        f.write((const byte *)&hdr, sizeof(pcaprec_hdr_t));
        f.write(packet->payload, packet->rx_ctrl.sig_len);
        f.close();
        return;
    }

    // --- EAPOL frame handling (require M1+M2+M3+M4) ---
    int eapolMsg = classifyEapolMessage(packet);
    if (eapolMsg < 1 || eapolMsg > 4) { return; }

    auto &tracker = perApHandshakeTracker[apKey];
    auto &buf = eapol4WayBuffer[apKey];
    uint16_t dataLen = std::min<uint16_t>(packet->rx_ctrl.sig_len, EAPOL_BUF_SIZE);

    // M1: buffer — don't write to file yet
    if (eapolMsg == 1) {
        if (tracker.msg1) { return; }
        tracker.msg1 = true;
        memcpy(buf.m1.data, packet->payload, dataLen);
        buf.m1.len = dataLen;
        buf.m1.timestamp_sec = packet->rx_ctrl.timestamp / 1000000;
        buf.m1.timestamp_usec = packet->rx_ctrl.timestamp % 1000000;
        return;
    }

    // M2: buffer — don't write to file yet
    if (eapolMsg == 2) {
        if (!tracker.msg1 || tracker.msg2) { return; }
        tracker.msg2 = true;
        memcpy(buf.m2.data, packet->payload, dataLen);
        buf.m2.len = dataLen;
        buf.m2.timestamp_sec = packet->rx_ctrl.timestamp / 1000000;
        buf.m2.timestamp_usec = packet->rx_ctrl.timestamp % 1000000;
        return;
    }

    // M3: buffer — don't write to file yet
    if (eapolMsg == 3) {
        if (!tracker.msg2 || tracker.msg3) { return; }
        tracker.msg3 = true;
        memcpy(buf.m3.data, packet->payload, dataLen);
        buf.m3.len = dataLen;
        buf.m3.timestamp_sec = packet->rx_ctrl.timestamp / 1000000;
        buf.m3.timestamp_usec = packet->rx_ctrl.timestamp % 1000000;
        return;
    }

    // M4: flush all 4 frames to file, mark handshake valid
    if (eapolMsg == 4) {
        if (!tracker.msg3 || tracker.msg4) { return; }
        tracker.msg4 = true;

        if (!Fs.exists("/BrucePCAP")) { Fs.mkdir("/BrucePCAP"); }
        if (!Fs.exists("/BrucePCAP/handshakes")) { Fs.mkdir("/BrucePCAP/handshakes"); }

        registerHandshakeRecord(filePath);

        File f = Fs.open(filePath, FILE_APPEND);
        if (!f) {
            eapol4WayBuffer.erase(apKey);
            return;
        }

        if (f.size() == 0) {
            writeHeader(f);
            // Write the last-seen beacon for this AP first, so the SSID is
            // available to the cracker before it stops reading at M2+M3.
            auto beaconIt = beaconRawCache.find(apKey);
            if (beaconIt != beaconRawCache.end() && beaconIt->second.len > 0) {
                pcaprec_hdr_t bhdr;
                bhdr.ts_sec = beaconIt->second.timestamp_sec;
                bhdr.ts_usec = beaconIt->second.timestamp_usec;
                bhdr.incl_len = beaconIt->second.len;
                bhdr.orig_len = beaconIt->second.len;
                f.write((const byte *)&bhdr, sizeof(pcaprec_hdr_t));
                f.write(beaconIt->second.data, beaconIt->second.len);
                registerHandshakeBeacon(apKey);
            }
        }

        pcaprec_hdr_t hdr;
        auto writeFrame = [&](const EapolFrame &frame) {
            hdr.ts_sec = frame.timestamp_sec;
            hdr.ts_usec = frame.timestamp_usec;
            hdr.incl_len = frame.len;
            hdr.orig_len = frame.len;
            f.write((const byte *)&hdr, sizeof(pcaprec_hdr_t));
            f.write(frame.data, frame.len);
        };

        writeFrame(buf.m1);
        writeFrame(buf.m2);
        writeFrame(buf.m3);

        uint16_t m4Len = dataLen;
        hdr.ts_sec = packet->rx_ctrl.timestamp / 1000000;
        hdr.ts_usec = packet->rx_ctrl.timestamp % 1000000;
        hdr.incl_len = m4Len;
        hdr.orig_len = m4Len;
        f.write((const byte *)&hdr, sizeof(pcaprec_hdr_t));
        f.write(packet->payload, m4Len);

        f.close();
        eapol4WayBuffer.erase(apKey);

        if (handshakeReadyBssids.find(apKey) == handshakeReadyBssids.end()) { num_HS++; }
        markHandshakeReady(apKey);
        return;
    }
}

// Returns "" when the SSID is unknown, so buildHandshakePath() falls back to
// a MAC-based (per-AP unique) filename instead of bucketing every AP with an
// unresolved SSID into the same file.
static String sanitizeSsid(const char *ssid) {
    if (!ssid || ssid[0] == '\0') { return ""; }
    String sanitized = "";
    for (size_t i = 0; ssid[i] != '\0' && i < MAX_CAPTURE_SSID_LEN; ++i) {
        const char c = ssid[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
            c == '_' || c == '.') {
            sanitized += c;
        } else {
            sanitized += '_';
        }
    }
    return sanitized;
}

static String macToHex(const uint8_t *mac) {
    char buffer[13] = {0};
    snprintf(
        buffer, sizeof(buffer), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );
    return String(buffer);
}

static bool handshakeFileExists(const String &path) { return handshakeRecordExists(path); }

static String buildHandshakePath(const uint8_t *mac, const char *ssid) {
    // Always prefix with the MAC (unique per AP), then append the SSID when
    // it's known: HS_{MAC}_{SSID}.pcap, or HS_{MAC}.pcap when unresolved.
    String path = "/BrucePCAP/handshakes/HS_";
    path += macToHex(mac);
    if (ssid && ssid[0] != '\0') {
        path += '_';
        path += ssid;
    }
    path += ".pcap";
    return path;
}

static bool shouldSaveBeaconForHandshake(const uint8_t *mac) {
    if (!mac) return false;
    uint64_t key = macToKey(mac);
    bool ready = false;
    portENTER_CRITICAL(&handshakeReadyMux);
    ready = handshakeReadyBssids.find(key) != handshakeReadyBssids.end();
    portEXIT_CRITICAL(&handshakeReadyMux);
    return ready;
}

static bool hasCompleteHandshake(uint64_t apKey) {
    bool ready = false;
    portENTER_CRITICAL(&handshakeReadyMux);
    ready = handshakeReadyBssids.find(apKey) != handshakeReadyBssids.end();
    portEXIT_CRITICAL(&handshakeReadyMux);
    return ready;
}

void markHandshakeReady(uint64_t key) {
    portENTER_CRITICAL(&handshakeReadyMux);
    handshakeReadyBssids.insert(key);
    portEXIT_CRITICAL(&handshakeReadyMux);
}

bool sniffer_is_handshake_ready(uint64_t key) {
    bool ready = false;
    portENTER_CRITICAL(&handshakeReadyMux);
    ready = handshakeReadyBssids.find(key) != handshakeReadyBssids.end();
    portEXIT_CRITICAL(&handshakeReadyMux);
    return ready;
}

static void resetHandshakeTracking() {
    portENTER_CRITICAL(&handshakeReadyMux);
    handshakeReadyBssids.clear();
    portEXIT_CRITICAL(&handshakeReadyMux);
}

static bool handshakeRecordExists(const String &path) {
    if (!handshakeMutex) { return SavedHS.find(path) != SavedHS.end(); }
    if (xSemaphoreTake(handshakeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        bool exists = SavedHS.find(path) != SavedHS.end();
        xSemaphoreGive(handshakeMutex);
        return exists;
    }
    return SavedHS.find(path) != SavedHS.end();
}

static void registerHandshakeRecord(const String &path) {
    if (!handshakeMutex) {
        SavedHS.insert(path);
        return;
    }
    if (xSemaphoreTake(handshakeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        SavedHS.insert(path);
        xSemaphoreGive(handshakeMutex);
    } else {
        SavedHS.insert(path);
    }
}

static bool handshakeBeaconRecorded(uint64_t key) {
    if (!handshakeMutex) { return handshakeBeaconLogged.find(key) != handshakeBeaconLogged.end(); }
    if (xSemaphoreTake(handshakeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        bool exists = handshakeBeaconLogged.find(key) != handshakeBeaconLogged.end();
        xSemaphoreGive(handshakeMutex);
        return exists;
    }
    return handshakeBeaconLogged.find(key) != handshakeBeaconLogged.end();
}

static void registerHandshakeBeacon(uint64_t key) {
    if (!handshakeMutex) {
        handshakeBeaconLogged.insert(key);
        return;
    }
    if (xSemaphoreTake(handshakeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        handshakeBeaconLogged.insert(key);
        xSemaphoreGive(handshakeMutex);
    } else {
        handshakeBeaconLogged.insert(key);
    }
}

static void resetHandshakeBeaconCache() {
    if (!handshakeMutex) {
        handshakeBeaconLogged.clear();
        return;
    }
    if (xSemaphoreTake(handshakeMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        handshakeBeaconLogged.clear();
        xSemaphoreGive(handshakeMutex);
    } else {
        handshakeBeaconLogged.clear();
    }
}

static void registerBeacon(const uint8_t *apAddr) {
    if (!apAddr) return;
    BeaconList beacon;
    memcpy(beacon.MAC, apAddr, sizeof(beacon.MAC));
    beacon.channel = all_wifi_channels[ch];
    registeredBeacons.insert(beacon);
}

static void cacheBeaconFrame(uint64_t apKey, const wifi_promiscuous_pkt_t *packet) {
    if (!packet) return;
    if (beaconRawCache.find(apKey) == beaconRawCache.end() && beaconRawCache.size() >= MAX_BEACON_CACHE) {
        return;
    }
    uint16_t len = std::min<uint16_t>(packet->rx_ctrl.sig_len, BEACON_BUF_SIZE);
    BeaconFrame &frame = beaconRawCache[apKey];
    memcpy(frame.data, packet->payload, len);
    frame.len = len;
    frame.timestamp_sec = packet->rx_ctrl.timestamp / 1000000;
    frame.timestamp_usec = packet->rx_ctrl.timestamp % 1000000;
}

static String resolveSsidForFrame(FrameInfo &info, const wifi_promiscuous_pkt_t *packet) {
    if (!packet) return "";
    if (info.isBeacon) {
        beacon_frames++;
        String ssid = extractSsid(packet);
        beaconSsidCache[info.apKey] = ssid;
        cacheBeaconFrame(info.apKey, packet);
        return ssid;
    }
    auto it = beaconSsidCache.find(info.apKey);
    if (it != beaconSsidCache.end()) { return it->second; }
    return "";
}

static FrameInfo analyzeFrame(wifi_promiscuous_pkt_t *pkt) {
    FrameInfo info;
    if (!pkt) { return info; } // removed redundant pkt->payload check
    const uint16_t len = pkt->rx_ctrl.sig_len;
    if (len < 24) { return info; }

    info.valid = true;
    const uint8_t *frame = pkt->payload;
    const uint16_t frameControl = (uint16_t)frame[0] | ((uint16_t)frame[1] << 8);
    const uint8_t frameType = (frameControl & 0x0C) >> 2;
    const uint8_t frameSubType = (frameControl & 0xF0) >> 4;

    const uint8_t *addr1 = frame + 4;
    const uint8_t *addr2 = frame + 10;
    const uint8_t *bssid = frame + 16;
    const uint8_t *apAddr = (memcmp(addr1, bssid, 6) == 0) ? addr1 : addr2;
    copyMac(info.apAddr, apAddr);
    info.apKey = macToKey(info.apAddr);

    info.isBeacon = (frameType == 0x00 && frameSubType == 0x08);
    info.isDeauth = (frameType == 0x00) && (frameSubType == 0x0C || frameSubType == 0x0A);
    info.isEapol = isItEAPOL(pkt);

    if (info.isEapol && matchesTargetAP(pkt, targetBssid)) {
        int msg = classifyEapolMessage(pkt);
        info.eapolMsgNum = msg;
        // Update handshake tracker
        switch (msg) {
            case 1: hsTracker.msg1 = true; break;
            case 2: hsTracker.msg2 = true; break;
            case 3: hsTracker.msg3 = true; break;
            case 4: hsTracker.msg4 = true; break;
        }
    }

    info.ssid = resolveSsidForFrame(info, pkt);
    if (info.isBeacon) {
        registerBeacon(info.apAddr);
        // UPDATE last-seen timestamp for this beacon
        beaconLastSeen[info.apKey] = (uint32_t)millis();
    }

    return info;
}

static uint64_t macToKey(const void *mac) {
    const uint8_t *u = reinterpret_cast<const uint8_t *>(mac);
    uint64_t key = 0;
    for (int i = 0; i < 6; ++i) { key = (key << 8) | (uint64_t)u[i]; }
    return key;
}

static void copyMac(uint8_t *dest, const uint8_t *src) { memcpy(dest, src, 6); }

static void copySsidToBuffer(const String &ssid, char *buffer, size_t len) {
    if (!buffer || len == 0) return;
    size_t copyLen = std::min<size_t>(ssid.length(), len - 1);
    memcpy(buffer, ssid.c_str(), copyLen);
    buffer[copyLen] = '\0';
}

static String extractSsid(const wifi_promiscuous_pkt_t *packet) {
    if (!packet) return "";
    const uint8_t *payload = packet->payload;
    int len = packet->rx_ctrl.sig_len;
    if (len < 36) return "";
    int offset = 36;
    while (offset + 1 < len) {
        uint8_t tagNumber = payload[offset];
        uint8_t tagLength = payload[offset + 1];
        if (offset + 2 + tagLength > len) break;
        if (tagNumber == 0x00) {
            String ssid = "";
            for (int i = 0; i < tagLength; ++i) {
                uint8_t chValue = payload[offset + 2 + i];
                if (isprint(chValue)) { ssid += (char)chValue; }
            }
            return ssid;
        }
        offset += 2 + tagLength;
    }
    return "";
}

static wifi_promiscuous_pkt_t *duplicatePacket(const wifi_promiscuous_pkt_t *pkt, uint16_t length) {
    size_t total = sizeof(wifi_pkt_rx_ctrl_t) + length;
    uint8_t *buffer = (uint8_t *)heap_caps_malloc(total, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (!buffer) { buffer = (uint8_t *)heap_caps_malloc(total, MALLOC_CAP_8BIT); }
    if (!buffer) { buffer = (uint8_t *)malloc(total); }
    if (!buffer) { return nullptr; }
    auto *copy = reinterpret_cast<wifi_promiscuous_pkt_t *>(buffer);
    memcpy(copy, pkt, sizeof(wifi_pkt_rx_ctrl_t));
    memcpy(buffer + sizeof(wifi_pkt_rx_ctrl_t), pkt->payload, length);
    copy->rx_ctrl.sig_len = length;
    return copy;
}

static void releasePacketCopy(wifi_promiscuous_pkt_t *packet) {
    if (!packet) return;
    heap_caps_free(packet);
}

static bool lockFileMutex(TickType_t ticks) {
    if (!fileMutex) return true;
    return xSemaphoreTake(fileMutex, ticks) == pdTRUE;
}

static void unlockFileMutex() {
    if (!fileMutex) return;
    xSemaphoreGive(fileMutex);
}

static void ensureDirectories(FS &Fs) {
    if (!Fs.exists("/BrucePCAP")) { Fs.mkdir("/BrucePCAP"); }
    if (!Fs.exists("/BrucePCAP/handshakes")) { Fs.mkdir("/BrucePCAP/handshakes"); }
}

static void openDeauthFile(FS &Fs) {
    ensureDirectories(Fs);
    closeDeauthFile();
    deauthFilename = "/BrucePCAP/deauth_" + String(deauthFileIndex) + ".pcap";
    while (Fs.exists(deauthFilename)) {
        deauthFileIndex++;
        deauthFilename = "/BrucePCAP/deauth_" + String(deauthFileIndex) + ".pcap";
    }
    if (lockFileMutex(pdMS_TO_TICKS(200))) {
        _deauth_file = Fs.open(deauthFilename, FILE_WRITE);
        deauthFileOpen = _deauth_file && writeHeader(_deauth_file);
        unlockFileMutex();
        if (!deauthFileOpen) { Serial.println("Fail opening deauth capture file"); }
    }
}

static void closeRawFile() {
    if (lockFileMutex(pdMS_TO_TICKS(200))) {
        if (_pcap_file) {
            _pcap_file.flush();
            _pcap_file.close();
        }
        rawFileOpen = false;
        unlockFileMutex();
    }
}

static void closeDeauthFile() {
    if (lockFileMutex(pdMS_TO_TICKS(200))) {
        if (_deauth_file) {
            _deauth_file.flush();
            _deauth_file.close();
        }
        deauthFileOpen = false;
        unlockFileMutex();
    }
}

static bool rawCaptureEnabled() { return currentMode == SnifferMode::Full && rawFileOpen && _pcap_file; }
static bool handshakeCaptureEnabled() { return currentMode != SnifferMode::DeauthOnly; }
static bool deauthCaptureEnabled() {
    return currentMode == SnifferMode::DeauthOnly && deauthFileOpen && _deauth_file;
}

static String currentModeString() {
    switch (currentMode) {
        case SnifferMode::Full: return "Full Sniff";
        case SnifferMode::DeauthOnly: return "Deauth Frames";
        default: return "EAPOL/Handshakes";
    }
}

static bool ensureSnifferBackend() {
    if (!fileMutex) { fileMutex = xSemaphoreCreateMutexStatic(&fileMutexBuffer); }
    if (!handshakeMutex) { handshakeMutex = xSemaphoreCreateMutexStatic(&handshakeMutexBuffer); }
    if (!snifferQueue) { snifferQueue = xQueueCreate(SNIFFER_QUEUE_DEPTH, sizeof(SnifferQueueItem)); }
    if (!snifferQueue) { return false; }
    if (!snifferWriterHandle) {
#if SOC_CPU_CORES_NUM > 1
        BaseType_t res = xTaskCreatePinnedToCore(
            snifferWriterTask, "sniff_writer", 4096, nullptr, 4, &snifferWriterHandle, 1
        );
#else
        BaseType_t res =
            xTaskCreate(snifferWriterTask, "sniff_writer", 4096, nullptr, 4, &snifferWriterHandle);
#endif
        if (res != pdPASS) { snifferWriterHandle = nullptr; }
    }
    return snifferWriterHandle != nullptr;
}

static void handleRawWrite(const SnifferQueueItem &item) {
    if (!rawCaptureEnabled() || !item.packet) { return; }
    if (lockFileMutex(pdMS_TO_TICKS(200))) {
        newPacketSD(item.ts_sec, item.ts_usec, item.raw_len, item.packet->payload, _pcap_file);
        unlockFileMutex();
    }
}

static void handleHandshakeWrite(const SnifferQueueItem &item) {
    if (!handshakeCaptureEnabled() || !item.packet) { return; }
    saveHandshake(item.packet, item.isBeacon, *activeFs, item.ssid);
}

static void handleDeauthWrite(const SnifferQueueItem &item) {
    if (!deauthCaptureEnabled() || !item.packet) { return; }
    if (lockFileMutex(pdMS_TO_TICKS(200))) {
        newPacketSD(item.ts_sec, item.ts_usec, item.raw_len, item.packet->payload, _deauth_file);
        unlockFileMutex();
    }
}

static void snifferWriterTask(void *param) {
    (void)param;
    SnifferQueueItem item;
    while (true) {
        if (xQueueReceive(snifferQueue, &item, portMAX_DELAY) == pdTRUE) {
            if (item.saveRaw) { handleRawWrite(item); }
            if (item.saveHandshake) { handleHandshakeWrite(item); }
            if (item.saveDeauth) { handleDeauthWrite(item); }
            releasePacketCopy(item.packet);
        }
    }
}

bool sniffer_prepare_storage(FS *fs, bool sdDetectedParam) {
    if (!ensureSnifferBackend()) { return false; }
    if (!fs) { fs = &LittleFS; }
    activeFs = fs;
    isLittleFS = (fs == &LittleFS);
    sdDetected = sdDetectedParam;
    ensureDirectories(*activeFs);
    littleFsSpaceAvailable = !isLittleFS || checkLittleFsSizeNM();
    littleFsWasFull = !littleFsSpaceAvailable && isLittleFS;
    if (currentMode == SnifferMode::Full && !sdDetected) { currentMode = SnifferMode::HandshakesOnly; }
    return true;
}

void sniffer_set_mode(SnifferMode mode) {
    if (mode == SnifferMode::Full && !sdDetected) { mode = SnifferMode::HandshakesOnly; }
    if (mode == currentMode) { return; }
    sniffer_wait_for_flush(500);
    if (currentMode == SnifferMode::Full) { closeRawFile(); }
    if (currentMode == SnifferMode::DeauthOnly) { closeDeauthFile(); }
    currentMode = mode;
    if (currentMode == SnifferMode::Full) {
        openFile(*activeFs);
    } else if (currentMode == SnifferMode::DeauthOnly) {
        openDeauthFile(*activeFs);
    } else {
        closeRawFile();
        closeDeauthFile();
    }
}

SnifferMode sniffer_get_mode() { return currentMode; }

bool sniffer_full_mode_available() { return sdDetected; }

void sniffer_wait_for_flush(uint32_t timeoutMs) {
    if (!snifferQueue) { return; }
    TickType_t start = xTaskGetTickCount();
    TickType_t deadline = pdMS_TO_TICKS(timeoutMs);
    while (uxQueueMessagesWaiting(snifferQueue) > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (timeoutMs == 0) { continue; }
        if ((xTaskGetTickCount() - start) > deadline) { break; }
    }
}

void sniffer_reset_handshake_cache() {
    if (handshakeMutex && xSemaphoreTake(handshakeMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        SavedHS.clear();
        xSemaphoreGive(handshakeMutex);
    } else {
        SavedHS.clear();
    }
    resetHandshakeTracking();
    resetHandshakeBeaconCache();
    perApHandshakeTracker.clear();
    eapol4WayBuffer.clear();
}

void printAddress(const uint8_t *addr) {
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", addr[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
}

/* write packet to file */
void newPacketSD(uint32_t ts_sec, uint32_t ts_usec, uint32_t len, uint8_t *buf, File pcap_file) {
    if (pcap_file) {

        uint32_t orig_len = len;
        uint32_t incl_len = len;
        // if(incl_len > snaplen) incl_len = snaplen; /* safty check that the packet isn't too big (I ran into
        // problems here) */

        pcap_file.write((uint8_t *)&ts_sec, sizeof(ts_sec));
        pcap_file.write((uint8_t *)&ts_usec, sizeof(ts_usec));
        pcap_file.write((uint8_t *)&incl_len, sizeof(incl_len));
        pcap_file.write((uint8_t *)&orig_len, sizeof(orig_len));

        pcap_file.write(buf, incl_len);
    }
}

bool writeHeader(File file) {
    uint32_t magic_number = 0xa1b2c3d4;
    uint16_t version_major = 2;
    uint16_t version_minor = 4;
    uint32_t thiszone = 0;
    uint32_t sigfigs = 0;
    uint32_t snaplen = 2500;
    uint32_t network = 105;

    if (file) {

        file.write((uint8_t *)&magic_number, sizeof(magic_number));
        file.write((uint8_t *)&version_major, sizeof(version_major));
        file.write((uint8_t *)&version_minor, sizeof(version_minor));
        file.write((uint8_t *)&thiszone, sizeof(thiszone));
        file.write((uint8_t *)&sigfigs, sizeof(sigfigs));
        file.write((uint8_t *)&snaplen, sizeof(snaplen));
        file.write((uint8_t *)&network, sizeof(network));

        return true;
    }
    return false;
}

/* will be executed on every packet the ESP32 gets while being in promiscuous mode */
// Sniffer callback
void sniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!snifferQueue && !ensureSnifferBackend()) { return; }
    // If using LittleFS to save .pcaps and storage is exhausted, stop promiscuous mode
    if (isLittleFS && !littleFsSpaceAvailable) {
        littleFsWasFull = true; // storage triggered exit
        returnToMenu = true;
        esp_wifi_set_promiscuous(false);
        return;
    }

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;

    packet_counter++;

    FrameInfo frameInfo = analyzeFrame(pkt);
    if (!frameInfo.valid) { return; }
    if (frameInfo.isEapol) { num_EAPOL++; }

    bool saveRaw = rawCaptureEnabled();
    bool saveHandshake = false;
    if (handshakeCaptureEnabled()) {
        if (frameInfo.isEapol && !hasCompleteHandshake(frameInfo.apKey)) {
            saveHandshake = true;
        } else if (frameInfo.isBeacon && shouldSaveBeaconForHandshake(frameInfo.apAddr)) {
            saveHandshake = true;
        }
    }
    bool saveDeauth = deauthCaptureEnabled() && frameInfo.isDeauth;

    if (!saveRaw && !saveHandshake && !saveDeauth) { return; }

    wifi_promiscuous_pkt_t *copy = duplicatePacket(pkt, ctrl.sig_len);
    if (!copy) { return; }
    if (frameInfo.isBeacon && copy->rx_ctrl.sig_len >= 4) { copy->rx_ctrl.sig_len -= 4; }

    SnifferQueueItem item;
    item.packet = copy;
    uint64_t pktTimestamp = copy->rx_ctrl.timestamp;
    item.ts_sec = pktTimestamp / 1000000ULL;
    item.ts_usec = pktTimestamp % 1000000ULL;
    item.raw_len = ctrl.sig_len;
    if (type == WIFI_PKT_MGMT && item.raw_len >= 4) { item.raw_len -= 4; }
    item.type = type;
    item.isBeacon = frameInfo.isBeacon;
    item.isHandshakeFrame = frameInfo.isEapol;
    item.isDeauthFrame = frameInfo.isDeauth;
    item.saveRaw = saveRaw;
    item.saveHandshake = saveHandshake;
    item.saveDeauth = saveDeauth;
    copyMac(item.bssid, frameInfo.apAddr);
    // Leave empty when unresolved: saveHandshake()/buildHandshakePath() then
    // fall back to a per-AP MAC-based filename instead of forcing "UNKNOWN",
    // which would collide different APs into the same handshake pcap.
    copySsidToBuffer(frameInfo.ssid, item.ssid, sizeof(item.ssid));

    BaseType_t taskWoken = pdFALSE;
    if (xQueueSendFromISR(snifferQueue, &item, &taskWoken) != pdTRUE) {
        releasePacketCopy(copy);
    } else if (taskWoken) {
        portYIELD_FROM_ISR();
    }
}

// esp_err_t event_handler(void *ctx, system_event_t *event){ return ESP_OK; }
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                // Ação para quando a estação WiFi inicia
                break;
                // Outros casos...
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                // Ação para quando a estação WiFi obtém um endereço IP
                break;
                // Outros casos...
        }
    }
}

void openFile(FS &Fs) {
    ensureDirectories(Fs);
    closeRawFile();
    filename = "/BrucePCAP/" + (String)FILENAME + String(rawFileIndex) + ".pcap";
    while (Fs.exists(filename)) {
        rawFileIndex++;
        filename = "/BrucePCAP/" + (String)FILENAME + String(rawFileIndex) + ".pcap";
    }
    if (lockFileMutex(pdMS_TO_TICKS(200))) {
        _pcap_file = Fs.open(filename, FILE_WRITE);
        rawFileOpen = _pcap_file && writeHeader(_pcap_file);
        unlockFileMutex();
        if (!rawFileOpen) { Serial.println("Fail opening the file"); }
    }
}

// --- New helper implementations ---

static void cleanupStaleBeacons() {
    unsigned long now = millis();
    std::vector<BeaconList> toRemove;
    for (auto it = registeredBeacons.begin(); it != registeredBeacons.end(); ++it) {
        uint64_t key = macToKey(it->MAC);
        auto lastIt = beaconLastSeen.find(key);
        if (lastIt == beaconLastSeen.end() || (now - (unsigned long)lastIt->second) > BEACON_TIMEOUT_MS) {
            toRemove.push_back(*it);
        }
    }
    for (const auto &b : toRemove) {
        // erase by matching MAC bytes
        for (auto it = registeredBeacons.begin(); it != registeredBeacons.end();) {
            if (memcmp(it->MAC, b.MAC, 6) == 0) {
                it = registeredBeacons.erase(it);
            } else {
                ++it;
            }
        }
        uint64_t key = macToKey(b.MAC);
        beaconSsidCache.erase(key);
        beaconLastSeen.erase(key);
        beaconRawCache.erase(key);
    }
}

static size_t countActiveBeaconsOnChannel(uint8_t channel) {
    unsigned long now = millis();
    size_t cnt = 0;
    for (const auto &b : registeredBeacons) {
        if (b.channel != channel) continue;
        uint64_t key = macToKey(b.MAC);
        auto it = beaconLastSeen.find(key);
        if (it != beaconLastSeen.end() && (now - (unsigned long)it->second) <= BEACON_TIMEOUT_MS) { ++cnt; }
    }
    return cnt;
}

static std::vector<String> recentSsidsOnChannel(uint8_t channel, size_t maxItems) {
    std::vector<String> out;
    unsigned long now = millis();
    for (const auto &b : registeredBeacons) {
        if (b.channel != channel) continue;
        uint64_t key = macToKey(b.MAC);
        auto lastIt = beaconLastSeen.find(key);
        if (lastIt == beaconLastSeen.end() || (now - (unsigned long)lastIt->second) > BEACON_TIMEOUT_MS)
            continue;
        auto ssidIt = beaconSsidCache.find(key);
        if (ssidIt == beaconSsidCache.end()) continue;
        String ss = ssidIt->second;
        if (ss.length() == 0) continue;
        bool dup = false;
        for (auto &x : out)
            if (x == ss) {
                dup = true;
                break;
            }
        if (!dup) {
            out.push_back(ss);
            if (out.size() >= maxItems) break;
        }
    }
    return out;
}

static void sendDeauthNow() {
    bool deauth_sent = false;
    if (registeredBeacons.size() > 40) registeredBeacons.clear();
    Serial.println("<<---- Sending deauth packets ---->>");
    for (const auto &registeredBeacon : registeredBeacons) {
        if (registeredBeacon.channel == all_wifi_channels[ch]) {
            memcpy(&ap_record.bssid, registeredBeacon.MAC, 6);
            wsl_bypasser_send_raw_frame(&ap_record, registeredBeacon.channel);
            send_raw_frame(deauth_frame, 26);
            deauth_sent = true;
            deauth_counter++;
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
    if (deauth_sent) {
        tft.setTextSize(1);
        tft.setTextDatum(0);
        tft.drawString("Deauth sent.", DEAUTH_MSG_X, DEAUTH_MSG_Y);
        deauth_displayed = true;
        deauth_display_ts = millis();
    }
}

//===== SETUP =====//
void sniffer_setup() {
    // Stop WebUI before setting WiFi mode for sniffer
    cleanlyStopWebUiForWiFiFeature();

    FS *Fs;
    int redraw = true;
    bool clearScreen = true;
    String FileSys = "LittleFS";
    bool deauth = false;
    unsigned long lastLittleFsCheck = 0;
    start_time = millis();
    drawMainBorderWithTitle("pcap sniffer");
    lastRedraw = millis();
    // closeSdCard();

    if (setupSdCard()) {
        Fs = &SD; // if SD is present and mounted, start writing on SD Card
        FileSys = "SD";
        isLittleFS = false;
    } else {
        Fs = &LittleFS; // if not, use the internal memory.
        isLittleFS = true;
    }

    rawFileIndex = 0;
    deauthFileIndex = 0;
    if (!sniffer_prepare_storage(Fs, !isLittleFS)) {
        displayError("Sniffer queue error", true);
        return;
    }

    SnifferMode startMode = sniffer_full_mode_available() ? SnifferMode::Full : SnifferMode::HandshakesOnly;
    sniffer_set_mode(startMode);

    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(10, BORDER_PAD_Y + FM * LH);
    tft.println("Sniffing Started");

    sniffer_reset_handshake_cache(); // Need to clear to restart HS count
    registeredBeacons.clear();
    beaconSsidCache.clear();
    beaconLastSeen.clear(); // ensure starts empty
    beaconRawCache.clear();

    /* setup wifi */
    ensureWifiPlatform();
    nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    wifi_config_t wifi_config;
    strlcpy((char *)wifi_config.ap.ssid, "BruceSniffer", sizeof(wifi_config.ap.ssid));
    strlcpy((char *)wifi_config.ap.password, "brucenet", sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = strlen("BruceSniffer");
    wifi_config.ap.channel = 1;                   // Channel
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK; // auth mode
    wifi_config.ap.ssid_hidden = 1;               // 1 to hidden SSID, 0 to visivle
    wifi_config.ap.max_connection = 2;            // Max connections
    wifi_config.ap.beacon_interval = 100;         // beacon interval in ms

    // Configura o modo AP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(sniffer);
    wifi_second_chan_t secondCh = (wifi_second_chan_t)NULL;
    esp_wifi_set_channel(all_wifi_channels[ch], secondCh);

    Serial.println("Sniffer started!");
    vTaskDelay(1000 / portTICK_RATE_MS);

    if (isLittleFS && !checkLittleFsSize()) {
        littleFsWasFull = true; // storage triggered exit
        goto Exit;
    }
    littleFsSpaceAvailable = !isLittleFS || checkLittleFsSizeNM();
    littleFsWasFull = !littleFsSpaceAvailable && isLittleFS;
    lastLittleFsCheck = millis();
    num_EAPOL = 0;
    num_HS = 0;
    packet_counter = 0;
    deauth_tmp = millis();
    // Prepare deauth frame for each AP record
    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));
    // Main sniffer loop

    for (;;) {
        if (returnToMenu) {
            if (littleFsWasFull) {
                Serial.println("Not enough space on LittleFS");
                displayError("LittleFS Full", true);
            }
            break; // user exit or storage exit — either way stop loop
        }
        unsigned long currentTime = millis();
        if (isLittleFS && (currentTime - lastLittleFsCheck) > 500) {
            littleFsSpaceAvailable = checkLittleFsSizeNM();
            if (!littleFsSpaceAvailable) {
                littleFsWasFull = true;
                returnToMenu = true;
            } else littleFsWasFull = false;
            lastLittleFsCheck = currentTime;
        }

        /* Channel Hopping */
        if (check(NextPress)) {
            esp_wifi_set_promiscuous(false);
            esp_wifi_set_promiscuous_rx_cb(nullptr);
            ch++; // increase channel
            if (ch >= sizeof(all_wifi_channels)) ch = 0;
            wifi_second_chan_t secondCh = (wifi_second_chan_t)NULL;
            esp_wifi_set_channel(all_wifi_channels[ch], secondCh);
            redraw = true;
            vTaskDelay(50 / portTICK_RATE_MS);
            esp_wifi_set_promiscuous(true);
            esp_wifi_set_promiscuous_rx_cb(sniffer);
        }

        if (PrevPress) {
#if !defined(HAS_KEYBOARD) && !defined(HAS_ENCODER)
            LongPress = true;
            long _tmp = millis();
            while (PrevPress) {
                if (millis() - _tmp > 150)
                    tft.drawArc(
                        tftWidth / 2,
                        tftHeight / 2,
                        25,
                        15,
                        0,
                        360 * (millis() - _tmp) / 700,
                        getColorVariation(bruceConfig.priColor),
                        bruceConfig.bgColor
                    );
                vTaskDelay(10 / portTICK_RATE_MS);
            }
            if (millis() - _tmp > 700) { // longpress detected to exit
                returnToMenu = true;
                _pcap_file.close();
                break;
            }
#endif
            check(PrevPress);
            esp_wifi_set_promiscuous(false);
            esp_wifi_set_promiscuous_rx_cb(nullptr);

            if (ch == 0) ch = sizeof(all_wifi_channels) - 1;
            else ch--; // decrease channel
            wifi_second_chan_t secondCh = (wifi_second_chan_t)NULL;
            esp_wifi_set_channel(all_wifi_channels[ch], secondCh);
            redraw = true;
            vTaskDelay(50 / portTICK_PERIOD_MS);
            esp_wifi_set_promiscuous(true);
            esp_wifi_set_promiscuous_rx_cb(sniffer);
        }

#if defined(HAS_KEYBOARD) || defined(T_EMBED)
        // T-Embed has a different btn for Escape, different from StickCs that uses Previous btn
        if (check(EscPress)) {
            returnToMenu = true;
            _pcap_file.close();
            break;
        }
#endif

        if (check(SelPress)) { // pressed ok - show menu
            options = {
                {"New File",
                 [=]() {
                     sniffer_wait_for_flush(1000);
                     if (sniffer_get_mode() == SnifferMode::Full) {
                         rawFileIndex++;
                         openFile(*Fs);
                     } else if (sniffer_get_mode() == SnifferMode::DeauthOnly) {
                         deauthFileIndex++;
                         openDeauthFile(*Fs);
                     }
                 }                                                                                        },
                {"Capture Mode",
                 [&]() {
                     std::vector<Option> modeOptions;
                     if (sniffer_full_mode_available()) {
                         modeOptions.push_back({"Full Sniff", [&]() {
                                                    sniffer_set_mode(SnifferMode::Full);
                                                }});
                     }
                     modeOptions.push_back({"Only EAPOL/HS", [&]() {
                                                sniffer_set_mode(SnifferMode::HandshakesOnly);
                                            }});
                     modeOptions.push_back({"Sniff Deauth", [&]() {
                                                sniffer_set_mode(SnifferMode::DeauthOnly);
                                            }});
                     loopOptions(modeOptions, MENU_TYPE_SUBMENU, "Capture Mode");
                     redraw = true;
                 }                                                                                        },
                {deauth ? "Disable deauth attack" : "Enable deauth attack", [&]() { deauth = !deauth; }   },
                {"Deauth Now",                                              [&]() { sendDeauthNow(); }    },
                {"Reset Counters",
                 [&]() {
                     packet_counter = 0;
                     num_EAPOL = 0;
                     num_HS = 0;
                     start_time = millis();
                     beacon_frames = 0;
                     registeredBeacons.clear();
                     beaconSsidCache.clear();
                     beaconRawCache.clear();
                     sniffer_reset_handshake_cache();
                     deauth_tmp = millis();
                 }                                                                                        },
                {"Exit Sniffer",                                            [&]() { returnToMenu = true; }},
            };
            loopOptions(options);
            clearScreen = true;
        }

        // perform stale-beacon cleanup every 5s
        if ((currentTime - lastBeaconCleanup) > 5000) {
            cleanupStaleBeacons();
            lastBeaconCleanup = currentTime;
        }

        if (redraw) { // Redraw UI
            redraw = false;
            // calculate run time
            uint32_t runtime = (millis() - start_time) / 1000;

            if (returnToMenu) goto Exit;
            tft.drawPixel(0, 0, 0);
            drawMainBorderWithTitle("pcap sniffer", clearScreen); // Clear Screen and redraw border
            clearScreen = false;
            tft.setTextSize(FP);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            String activeFile = "File: ";
            if (sniffer_get_mode() == SnifferMode::Full && rawCaptureEnabled()) {
                activeFile += FileSys + ":" + filename;
            } else if (sniffer_get_mode() == SnifferMode::DeauthOnly && deauthCaptureEnabled()) {
                activeFile += FileSys + ":" + deauthFilename;
            } else {
                activeFile += "handshake pcaps";
            }
            padprintln(activeFile);
            padprintln("Sniffer Mode: " + currentModeString());
            if (deauth) {
                tft.setTextColor(bruceConfig.bgColor, bruceConfig.priColor);
                padprintln(
                    "Deauth: in " + String((DEAUTH_INTERVAL - (millis() - deauth_tmp)) / 1000) + "s, total " +
                    String(deauth_counter) + " pkts sent"
                );
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

            } else padprintln("Silent mode.");

            padprintln("Run time " + String(runtime / 60) + ":" + String(runtime % 60));

            // New: show beacon counts and recent SSIDs
            size_t activeOnChannel = countActiveBeaconsOnChannel(all_wifi_channels[ch]);
            padprintln(
                "Beacons " + String(beacon_frames) + " tot. /" + String(registeredBeacons.size()) +
                " cached / ch " + String(activeOnChannel) + " active"
            );

            // show a short list of recent SSIDs on this channel (comma-separated)
            std::vector<String> recentSsids = recentSsidsOnChannel(all_wifi_channels[ch], 5);
            if (!recentSsids.empty()) {
                String s = "SSIDs: ";
                for (size_t i = 0; i < recentSsids.size(); ++i) {
                    s += recentSsids[i];
                    if (i + 1 < recentSsids.size()) s += ", ";
                }
                padprintln(s);
            }

            // make a nice reverse video bar
            tft.setTextColor(bruceConfig.bgColor, bruceConfig.priColor);
            tft.drawRightString(
                "Ch" +
                    String(
                        all_wifi_channels[ch] < 10    ? "  "
                        : all_wifi_channels[ch] < 100 ? " "
                                                      : ""
                    ) +
                    String(all_wifi_channels[ch]) + " (Next)",
                tftWidth - 10,
                tftHeight - 18,
                1
            );
            tft.drawString(
                " EAPOL: " + String(num_EAPOL) + " HS: " + String(num_HS) + " ", 10, tftHeight - 18
            );
            tft.drawCentreString("Packets " + String(packet_counter), tftWidth / 2, tftHeight - 26, 1);
        }

        if (currentTime - lastTime > 100) tft.drawPixel(0, 0, 0);

        if ((rawCaptureEnabled() || deauthCaptureEnabled()) && currentTime - lastTime > 1000) {
            if (lockFileMutex(pdMS_TO_TICKS(50))) {
                if (rawCaptureEnabled()) { _pcap_file.flush(); }
                if (deauthCaptureEnabled()) { _deauth_file.flush(); }
                unlockFileMutex();
            }
            lastTime = currentTime; // update time
        }

        if (deauth && (millis() - deauth_tmp) > DEAUTH_INTERVAL) {
            sendDeauthNow();
            deauth_tmp = millis();
        }

        // clear the message after timeout so it disappears
        if (deauth_displayed && (millis() - deauth_display_ts) > DEAUTH_MSG_MS) {
            // erase the area where the message was (fill with background)
            tft.fillRect(DEAUTH_MSG_X, DEAUTH_MSG_Y, DEAUTH_MSG_W, DEAUTH_MSG_H, DEAUTH_BG);
            deauth_displayed = false;
            // optionally force a redraw of other UI elements next cycle
            redraw = true;
        }

        if (millis() - lastRedraw > 1000) {
            redraw = true;
            lastRedraw = millis();
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
Exit:
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_set_promiscuous_rx_cb(NULL);
    // DO NOT call esp_wifi_deinit() here - let wifi_common.h handle it
    sniffer_wait_for_flush(1000);
    closeRawFile();
    closeDeauthFile();
    wifiDisconnect();
    vTaskDelay(1 / portTICK_RATE_MS);
}

void setHandshakeSniffer() {
    esp_wifi_set_promiscuous_rx_cb(NULL);
    esp_wifi_set_promiscuous_rx_cb(sniffer);
}
#endif
