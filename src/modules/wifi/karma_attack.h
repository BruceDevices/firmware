#ifndef KARMA_ATTACK_H
#define KARMA_ATTACK_H
#ifndef LITE_VERSION
#include "evil_portal.h"
#include <Arduino.h>
#include <FS.h>
#include <map>
#include <queue>
#include <vector>

// Attack prioritization tiers
enum AttackTier {
    TIER_NONE = 0,
    TIER_FAST = 1,
    TIER_MEDIUM = 2,
    TIER_HIGH = 3,
    TIER_CLONE = 4
};

// Broadcast attack configuration with evasion
struct BroadcastConfig {
    bool enableBroadcast = false;
    uint32_t broadcastInterval = 150;
    uint16_t batchSize = 200;
    bool rotateChannels = true;
    uint32_t channelHopInterval = 5000;
    bool respondToProbes = true;
    uint8_t maxActiveAttacks = 3;
    bool prioritizeResponses = true;
    bool randomizeInterval = true;
    uint32_t minInterval = 100;
    uint32_t maxInterval = 300;
    bool useExponentialBackoff = true;
    uint8_t maxRetries = 5;
};

// Broadcast statistics
struct BroadcastStats {
    size_t totalBroadcasts = 0;
    size_t totalResponses = 0;
    size_t successfulAttacks = 0;
    std::map<String, size_t> ssidResponseCount;
    unsigned long startTime = 0;
    unsigned long lastResponseTime = 0;
    std::map<String, uint8_t> retryCount;
    uint32_t failedResponses = 0;
    uint32_t rateLimitedCount = 0;
};

// RSN/WPA2/WPA3 security info
typedef struct {
    uint16_t version;
    uint8_t groupCipher;
    uint8_t pairwiseCipher;
    uint8_t akmSuite;
    bool isTransitionMode;
} RSNInfo;

// Probe request data
constexpr size_t PROBE_MAC_STR_LEN = 18;
constexpr size_t PROBE_SSID_MAX_LEN = 32;
constexpr size_t PROBE_SSID_BUF_LEN = PROBE_SSID_MAX_LEN + 1;
constexpr size_t PROBE_FRAME_CAPTURE_LEN = 128;

typedef struct {
    char mac[PROBE_MAC_STR_LEN];
    char ssid[PROBE_SSID_BUF_LEN];
    int8_t rssi;
    uint32_t timestamp;
    uint8_t channel;
    uint16_t frame_len;
    uint32_t fingerprint;
    uint8_t *frame;
    bool isPMKID;
} ProbeRequest;

typedef struct {
    char mac[PROBE_MAC_STR_LEN];
    char ssid[PROBE_SSID_BUF_LEN];
    int8_t rssi;
    uint32_t timestamp;
    uint8_t channel;
    uint32_t fingerprint;
    RSNInfo rsn;
    bool isPMKID;
} QueuedProbeEvent;

// Enhanced client behavior tracking
typedef struct {
    uint32_t fingerprint;
    String lastMAC;
    unsigned long firstSeen;
    unsigned long lastSeen;
    uint32_t probeCount;
    int avgRSSI;
    std::vector<String> probedSSIDs;
    uint8_t favoriteChannel;
    unsigned long lastKarmaAttempt;
    bool isVulnerable;
    uint32_t successfulInteractions = 0;
    uint32_t failedInteractions = 0;
    float successRate = 0.0f;
    uint8_t consecutiveFailures = 0;
    unsigned long lastSuccessTime = 0;
    std::map<String, uint8_t> ssidAttemptCount;
    bool isPermanentTarget = false;
    uint8_t priorityScore = 0;
} ClientBehavior;

// Active network
typedef struct {
    String ssid;
    uint8_t channel;
    RSNInfo rsn;
    unsigned long lastActivity;
    unsigned long lastBeacon;
    uint32_t beaconCount;
    bool isActive;
    uint8_t responseCount;
} ActiveNetwork;

// Network history
typedef struct {
    String ssid;
    uint32_t responsesSent;
    uint32_t successfulConnections;
    unsigned long lastResponse;
    uint8_t failureCount;
    uint8_t successRate;
    unsigned long lastAttempt;
} NetworkHistory;

// Probe response task
typedef struct {
    String ssid;
    String targetMAC;
    uint8_t channel;
    RSNInfo rsn;
    unsigned long timestamp;
    uint8_t retryCount;
    bool isRetry;
} ProbeResponseTask;

// Portal template
typedef struct {
    String name;
    String filename;
    bool isDefault;
    bool verifyPassword;
    uint8_t priority;
    String category;
} PortalTemplate;

// Pending portal attack
typedef struct {
    String ssid;
    uint8_t channel;
    String targetMAC;
    unsigned long timestamp;
    bool launched;
    String templateName;
    String templateFile;
    bool isDefaultTemplate;
    bool verifyPassword;
    uint8_t priority;
    AttackTier tier;
    uint16_t duration;
    bool isCloneAttack;
    uint32_t probeCount;
    uint32_t clientFingerprint;
    bool isHighValueTarget;
    uint8_t failureCount;
    unsigned long lastAttempt;
} PendingPortal;

// Active portal instance
struct BackgroundPortal {
    EvilPortal *instance;
    String portalId;
    String ssid;
    uint8_t channel;
    unsigned long lastHeartbeat;
    unsigned long launchTime;
    bool hasCreds;
    String capturedPassword;
    uint32_t clientFingerprint;
    String capturedIdentifier;
    bool targetEngaged;
    unsigned long engagementTime;
    uint8_t pageViewCount;
};

// Enhanced Karma config
typedef struct {
    bool enableAutoKarma;
    bool enableDeauth;
    bool enableSmartHop;
    bool prioritizeVulnerable;
    bool enableAutoPortal;
    uint16_t maxClients;
    uint8_t rateLimitPerTarget;
    uint32_t rateLimitWindow;
    bool enableDetectionEvasion;
    uint8_t beaconJitterPercent;
    bool enablePermanentTargets;
    uint8_t permanentThreshold;
} KarmaConfig;

// Attack config
typedef struct {
    AttackTier defaultTier;
    bool enableCloneMode;
    bool enableTieredAttack;
    uint8_t priorityThreshold;
    uint8_t cloneThreshold;
    bool enableBeaconing;
    uint16_t highTierDuration;
    uint16_t mediumTierDuration;
    uint16_t fastTierDuration;
    uint32_t cloneDuration;
    uint8_t maxCloneNetworks;
    uint16_t baseDuration;
    uint16_t extendedDuration;
    bool enableTemplateA/BTesting;
    uint8_t templateRotationInterval;
    bool enableContextualTemplate;
} AttackConfig;

// Handshake capture
struct HandshakeCapture {
    uint8_t bssid[6];
    String ssid;
    uint8_t channel;
    uint32_t timestamp;
    uint8_t eapolFrame[256];
    uint16_t frameLen;
    bool complete;
    uint8_t messageType;
    uint32_t keyInfo;
    bool isValid;
};

// Sync state for multi-device
struct SyncState {
    uint32_t deviceId;
    unsigned long lastSync;
    std::vector<String> activePortals;
    std::map<String, uint32_t> globalTargets;
    bool isCoordinator;
    uint32_t coordinatorId;
};

// Broadcast attack class
class ActiveBroadcastAttack {
private:
    BroadcastConfig config;
    BroadcastStats stats;
    size_t currentIndex;
    size_t batchStart;
    unsigned long lastBroadcastTime;
    unsigned long lastChannelHopTime;
    bool _active;
    uint8_t currentChannel;
    size_t totalSSIDsInFile;
    size_t ssidsProcessed;
    uint8_t updateCounter;
    std::vector<String> currentBatch;
    std::vector<String> highPrioritySSIDs;
    std::map<String, uint8_t> backoffCounters;
    unsigned long lastJitterTime;
    uint32_t consecutiveFailures;

public:
    ActiveBroadcastAttack();
    void start();
    void stop();
    void restart();
    bool isActive() const;
    void setConfig(const BroadcastConfig &newConfig);
    BroadcastConfig getConfig() const;
    void setBroadcastInterval(uint32_t interval);
    void setBatchSize(uint16_t size);
    void setChannel(uint8_t channel);
    void update();
    void processProbeResponse(const String &ssid, const String &mac);
    BroadcastStats getStats() const;
    size_t getTotalSSIDs() const;
    size_t getCurrentPosition() const;
    String getProgressString() const;
    float getProgressPercent() const;
    std::vector<std::pair<String, size_t>> getTopResponses(size_t count = 10) const;
    void addHighPrioritySSID(const String &ssid);
    void clearHighPrioritySSIDs();
    void recordFailedResponse(const String &ssid);
    bool shouldBackoff(const String &ssid) const;

private:
    void loadNextBatch();
    void broadcastSSID(const String &ssid);
    void rotateChannel();
    void sendBeaconFrame(const String &ssid, uint8_t channel);
    void recordResponse(const String &ssid);
    void launchAttackForResponse(const String &ssid, const String &mac);
    uint32_t getJitteredInterval() const;
    void updateBackoffCounter(const String &ssid, bool success);
};

// SSID Database with hybrid batch + LRU cache
class SSIDDatabase {
private:
    static String currentFilename;
    static bool useLittleFS;
    static std::vector<String> currentBatch;
    static size_t currentBatchStart;
    static std::map<String, size_t> lruCache;
    static size_t totalCount;
    static bool cacheInitialized;
    static constexpr size_t BATCH_SIZE = 200;
    static constexpr size_t MAX_CACHE_SIZE = 30;

    static FS *openSourceFs();
    static bool readNextEntry(File &file, String &line);
    static void updateLRU(const String &ssid, size_t index);
    static void trimLRU();

public:
    static size_t getCount();
    static String getSSID(size_t index);
    static void getBatch(size_t startIndex, size_t count, std::vector<String> &result);
    static bool contains(const String &ssid);
    static int findSSID(const String &ssid);
    static String getRandomSSID();
    static bool setSourceFile(const String &filename, bool useLittleFS = false);
    static void clearCache();
    static void warmCache(const std::vector<String> &frequentSSIDs);
    static bool isLoaded();
    static String getSourceFile();
};

// Operation modes
enum KarmaMode {
    MODE_PASSIVE = 0,
    MODE_BROADCAST = 1,
    MODE_FULL = 2
};

// Function prototypes
void karma_setup();
void clearProbes();
void saveProbesToFile(FS &fs, bool compressed);
void sendProbeResponse(const String &ssid, const String &mac, uint8_t channel);
void sendDeauth(const String &mac, uint8_t channel, bool broadcast);
void launchManualEvilPortal(const String &ssid, uint8_t channel, bool verifyPwd);
void launchTieredEvilPortal(PendingPortal &portal);
std::vector<ProbeRequest> getUniqueProbes();
std::vector<ClientBehavior> getVulnerableClients();
size_t buildEnhancedProbeResponse(uint8_t *buffer, const String &ssid, const String &targetMAC, 
                                  uint8_t channel, const RSNInfo &rsn, bool isHidden);
size_t buildBeaconFrame(uint8_t *buffer, const String &ssid, uint8_t channel, const RSNInfo &rsn);
void generateRandomBSSID(uint8_t *bssid);
void rotateBSSID();
RSNInfo extractRSNInfo(const uint8_t *frame, int len);
uint32_t generateClientFingerprint(const uint8_t *frame, int len);
void queueProbeResponse(const ProbeRequest &probe, const RSNInfo &rsn);
void processResponseQueue();
void sendBeaconFrames();
void checkForAssociations();
void saveNetworkHistory(FS &fs);
void sendBeaconFrameHelper(const String &ssid, uint8_t channel);
void saveCredentialsToFile(const String &ssid, const String &password);
void saveProbesToPCAP(FS &fs);
void launchBackgroundPortal(const String &ssid, uint8_t channel, const String &templateName, 
                           const String &templateFile = "");
void checkPortals();
String generatePortalId(const String &templateName);
void savePortalCredentials(const String &ssid, const String &identifier, const String &password, 
                          const String &mac, uint8_t channel, const String &templateName, 
                          const String &portalId);
String getDisplayName(const String &fullPath, bool isSD);
void matchAPSignal(uint8_t channel);
void setChannelWithSecond(uint8_t channel);
bool isPMKIDValid(const uint8_t *frame, int len);
void updateClientSuccessRate(uint32_t fingerprint, bool success);
String getContextualTemplate(const String &ssid);
void syncMultiDeviceState();
void handlePermanentTarget(ClientBehavior &client);

#endif
#endif
