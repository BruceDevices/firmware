/*
  Bruce Enhanced Karma attack module v2
  Author: Ninja-Jr. (@Ninja-jr)
  Version: 2.0
  Last updated: 27/08/2026
*/

#ifndef KARMA_ATTACK_H
#define KARMA_ATTACK_H

#ifndef LITE_VERSION
#include "evil_portal.h"
#include "band_types.h"
#include <Arduino.h>
#include <FS.h>
#include <map>
#include <queue>
#include <vector>

// Attack prioritization tiers
enum AttackTier {
    TIER_NONE = 0,
    TIER_FAST = 1,   // Quick opportunistic attacks
    TIER_MEDIUM = 2, // Standard priority targets
    TIER_HIGH = 3,   // High-value targets
    TIER_CLONE = 4   // Clone network attacks
};

// Broadcast attack configuration
struct BroadcastConfig {
    bool enableBroadcast = false;
    uint32_t broadcastInterval = 150;   // ms between broadcasts
    uint16_t batchSize = 200;           // SSIDs per batch
    bool rotateChannels = true;         // Auto-rotate channels
    uint32_t channelHopInterval = 5000; // ms between channel hops
    bool respondToProbes = true;        // Launch attacks on probe responses
    uint8_t maxActiveAttacks = 3;       // Max queued auto-attacks
    bool prioritizeResponses = true;    // Focus on SSIDs that get responses
    bool randomizeInterval = true;      // Add jitter to avoid detection
    uint32_t minInterval = 100;
    uint32_t maxInterval = 300;
    bool useExponentialBackoff = true;
    uint8_t maxRetries = 5;
};

// Broadcast statistics tracking
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

// RSN/WPA2/WPA3 security info for encryption mimicry
typedef struct {
    uint16_t version;
    uint8_t groupCipher;
    uint8_t pairwiseCipher;
    uint8_t akmSuite; // 0 = none, 1 = WPA2, 2 = WPA3
    bool isTransitionMode; // WPA2/WPA3 transition
} RSNInfo;

// Client fingerprint for tracking across MAC randomization
typedef struct {
    uint32_t ieHash;            // Hash of Information Elements
    uint8_t supportedRates[16]; // Supported rates
    uint8_t htCapabilities[32]; // HT capabilities
    uint8_t vendorIEs[64];      // Vendor-specific IEs
    uint8_t ieCount;            // Number of IEs
} ClientFingerprint;

constexpr size_t PROBE_MAC_STR_LEN = 18;
constexpr size_t PROBE_SSID_MAX_LEN = 32;
constexpr size_t PROBE_SSID_BUF_LEN = PROBE_SSID_MAX_LEN + 1;
constexpr size_t PROBE_FRAME_CAPTURE_LEN = 128;

// Probe request data structure
typedef struct {
    char mac[PROBE_MAC_STR_LEN];
    char ssid[PROBE_SSID_BUF_LEN];
    int8_t rssi;
    uint32_t timestamp;
    uint8_t channel;
    uint16_t frame_len;
    uint32_t fingerprint; // Hash of IE parameters for device tracking
    uint8_t *frame;
    bool isPMKID; // Indicates if this probe contains PMKID
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

// Client behavior tracking with enhanced metrics
typedef struct {
    uint32_t fingerprint; // Unique client identifier
    String lastMAC;       // Last seen MAC (for reference only)
    unsigned long firstSeen;
    unsigned long lastSeen;
    uint32_t probeCount;
    int avgRSSI;
    std::vector<String> probedSSIDs;
    uint8_t favoriteChannel;
    unsigned long lastKarmaAttempt;
    bool isVulnerable;
    // Enhanced tracking
    uint32_t successfulInteractions = 0;
    uint32_t failedInteractions = 0;
    float successRate = 0.0f;
    uint8_t consecutiveFailures = 0;
    unsigned long lastSuccessTime = 0;
    std::map<String, uint8_t> ssidAttemptCount;
    bool isPermanentTarget = false;
    uint8_t priorityScore = 0;
} ClientBehavior;

// Active network for beaconing
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

// Network history tracking
typedef struct {
    String ssid;
    uint32_t responsesSent;
    uint32_t successfulConnections;
    unsigned long lastResponse;
    uint8_t failureCount;
    uint8_t successRate; // Percentage
    unsigned long lastAttempt;
} NetworkHistory;

// Probe response task for queueing
typedef struct {
    String ssid;
    String targetMAC;
    uint8_t channel;
    RSNInfo rsn;
    unsigned long timestamp;
    uint8_t retryCount;
    bool isRetry;
} ProbeResponseTask;

// Portal template structure
typedef struct {
    String name;
    String filename;
    bool isDefault;
    bool verifyPassword;
    uint8_t priority; // For A/B testing
    String category;  // corporate, home, public, etc.
} PortalTemplate;

// Pending portal attack with enhanced fields
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
    uint32_t duration;
    bool isCloneAttack;
    uint32_t probeCount;
    uint32_t clientFingerprint;
    bool isHighValueTarget;
    uint8_t failureCount;
    unsigned long lastAttempt;
} PendingPortal;

// Single active portal instance
struct BackgroundPortal {
    EvilPortal *instance;             // Portal instance
    String portalId;                  // Unique ID for file naming
    String ssid;                      // SSID being spoofed
    uint8_t channel;                   // Channel this portal runs on
    unsigned long lastHeartbeat;      // Last time we checked this portal
    unsigned long launchTime;         // When portal was launched
    bool hasCreds;                    // Whether credentials captured
    String capturedPassword;          // Captured password if any
    uint32_t clientFingerprint;       // Fingerprint of connected victim
    String capturedIdentifier;        // Username/identifier if captured
    bool targetEngaged;               // Whether target viewed the portal
    unsigned long engagementTime;     // When engagement started
    uint8_t pageViewCount;            // Number of pages viewed
};

// Enhanced Karma configuration
typedef struct {
    bool enableAutoKarma;
    bool enableDeauth;
    bool enableSmartHop;
    bool prioritizeVulnerable;
    bool enableAutoPortal;
    uint16_t maxClients;
    uint8_t rateLimitPerTarget;      // Max responses per target per minute
    uint32_t rateLimitWindow;        // Window for rate limiting (ms)
    bool enableDetectionEvasion;
    uint8_t beaconJitterPercent;     // Random jitter for beacons (0-100)
    bool enablePermanentTargets;      // Track high-value targets persistently
    uint8_t permanentThreshold;      // Interactions before becoming permanent
} KarmaConfig;

// Attack strategy configuration
typedef struct {
    AttackTier defaultTier;
    bool enableCloneMode;
    bool enableTieredAttack;
    uint8_t priorityThreshold;
    uint8_t cloneThreshold;
    bool enableBeaconing;
    uint32_t highTierDuration;
    uint32_t mediumTierDuration;
    uint32_t fastTierDuration;
    uint32_t cloneDuration;
    uint8_t maxCloneNetworks;
    uint32_t baseDuration;
    uint32_t extendedDuration;
    bool enableTemplateABTesting;
    uint8_t templateRotationInterval; // Minutes between template rotation
    bool enableContextualTemplate;    // Choose template based on SSID context
} AttackConfig;

// Handshake capture structure
struct HandshakeCapture {
    uint8_t bssid[6];
    String ssid;
    uint8_t channel;
    uint32_t timestamp;
    uint8_t eapolFrame[256];
    uint16_t frameLen;
    bool complete;
    uint8_t messageType; // 1-4 for EAPOL messages
    uint32_t keyInfo;
    bool isValid;
};

// Synchronization state for multi-device
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

// SSID Database class
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
    static size_t getCacheSize();
};

// Operation modes for Karma
enum KarmaMode {
    MODE_PASSIVE = 0,   // Listen only, respond to probes
    MODE_BROADCAST = 1, // Actively advertise SSIDs + beacons
    MODE_FULL = 2       // Both passive and broadcast
};

// ============================================================================
// Band Detection and Adaptive Functions
// ============================================================================

// Detect which bands the current hardware supports
void detectSupportedBands();

// Check if a specific band is supported
bool isBandSupported(int band);

// Get the list of supported bands
SupportedBands getSupportedBands();

// Get band name as string
String getBandName(int band);

// Build channel list for Karma based on supported bands
std::vector<int> buildKarmaChannelList();

// Karma-specific adaptive channel management
void karmaAdaptiveHop();

// Set Karma channel with validation
void setKarmaChannel(uint8_t channel);

// Check if a channel is valid for the current hardware
bool isKarmaChannelValid(uint8_t channel);

// ============================================================================
// Existing Function Prototypes
// ============================================================================

void karma_setup();
void clearProbes();
void saveProbesToFile(FS &fs, bool compressed);
void sendProbeResponse(const String &ssid, const String &mac, uint8_t channel);
void sendDeauth(const String &mac, uint8_t channel, bool broadcast);
void launchManualEvilPortal(const String &ssid, uint8_t channel, bool verifyPwd);
void launchTieredEvilPortal(PendingPortal &portal);
std::vector<ProbeRequest> getUniqueProbes();
std::vector<ClientBehavior> getVulnerableClients();
size_t buildEnhancedProbeResponse(
    uint8_t *buffer, const String &ssid, const String &targetMAC, uint8_t channel, const RSNInfo &rsn,
    bool isHidden
);
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
void launchBackgroundPortal(
    const String &ssid, uint8_t channel, const String &templateName, const String &templateFile = ""
);
void checkPortals();
String generatePortalId(const String &templateName);
void savePortalCredentials(
    const String &ssid, const String &identifier, const String &password, const String &mac, uint8_t channel,
    const String &templateName, const String &portalId
);
String getDisplayName(const String &fullPath, bool isSD);
void matchAPSignal(uint8_t channel);
void setChannelWithSecond(uint8_t channel);
bool isPMKIDValid(const uint8_t *frame, int len);
void updateClientSuccessRate(uint32_t fingerprint, bool success);
String getContextualTemplate(const String &ssid);
void syncMultiDeviceState();
void handlePermanentTarget(ClientBehavior &client);

#endif // LITE_VERSION
#endif // KARMA_ATTACK_H
