#ifndef KARMA_ATTACK_H
#define KARMA_ATTACK_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <queue>
#include <FS.h>

// Attack prioritization tiers
enum AttackTier {
    TIER_NONE = 0,
    TIER_FAST = 1,     // Quick opportunistic attacks
    TIER_MEDIUM = 2,   // Standard priority targets
    TIER_HIGH = 3,     // High-value targets
    TIER_CLONE = 4     // Clone network attacks
};

// Broadcast attack configuration
struct BroadcastConfig {
    bool enableBroadcast = false;
    uint32_t broadcastInterval = 150;    // ms between broadcasts
    uint16_t batchSize = 100;            // SSIDs per batch
    bool rotateChannels = true;          // Auto-rotate channels
    uint32_t channelHopInterval = 5000;  // ms between channel hops
    bool respondToProbes = true;         // Launch attacks on probe responses
    uint8_t maxActiveAttacks = 3;        // Max simultaneous attacks
    bool prioritizeResponses = true;     // Focus on SSIDs that get responses
};

// Broadcast statistics tracking
struct BroadcastStats {
    size_t totalBroadcasts = 0;
    size_t totalResponses = 0;
    size_t successfulAttacks = 0;
    std::map<String, size_t> ssidResponseCount;
    unsigned long startTime = 0;
    unsigned long lastResponseTime = 0;
};

// RSN/WPA2/WPA3 security info for encryption mimicry
typedef struct {
    uint16_t version;
    uint8_t groupCipher;
    uint8_t pairwiseCipher;
    uint8_t akmSuite; // 0 = none, 1 = WPA2, 2 = WPA3
} RSNInfo;

// Client fingerprint for tracking across MAC randomization
typedef struct {
    uint32_t ieHash;                    // Hash of Information Elements
    uint8_t supportedRates[16];          // Supported rates
    uint8_t htCapabilities[32];          // HT capabilities
    uint8_t vendorIEs[64];               // Vendor-specific IEs
    uint8_t ieCount;                     // Number of IEs
} ClientFingerprint;

// Probe request data structure
typedef struct {
    String mac;
    String ssid;
    int rssi;
    unsigned long timestamp;
    uint8_t channel;
    uint8_t frame[128];
    uint16_t frame_len;
    uint32_t fingerprint;                 // Hash of IE parameters for device tracking
} ProbeRequest;

// Client behavior tracking (keyed by fingerprint, not MAC)
typedef struct {
    uint32_t fingerprint;                 // Unique client identifier
    String lastMAC;                       // Last seen MAC (for reference only)
    unsigned long firstSeen;
    unsigned long lastSeen;
    uint32_t probeCount;
    int avgRSSI;
    std::vector<String> probedSSIDs;
    uint8_t favoriteChannel;
    unsigned long lastKarmaAttempt;
    bool isVulnerable;
} ClientBehavior;

// Active network for beaconing
typedef struct {
    String ssid;
    uint8_t channel;
    RSNInfo rsn;
    unsigned long lastActivity;
    unsigned long lastBeacon;
} ActiveNetwork;

// Network history tracking
typedef struct {
    String ssid;
    uint32_t responsesSent;
    uint32_t successfulConnections;
    unsigned long lastResponse;
} NetworkHistory;

// Probe response task for queueing
typedef struct {
    String ssid;
    String targetMAC;
    uint8_t channel;
    RSNInfo rsn;
    unsigned long timestamp;
} ProbeResponseTask;

// Portal template structure
typedef struct {
    String name;
    String filename;
    bool isDefault;
    bool verifyPassword;
} PortalTemplate;

// Pending portal attack (legacy, kept for compatibility)
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
} PendingPortal;

// Background portal instance for multi-portal management
struct BackgroundPortal {
    EvilPortal* instance;                  // Portal instance
    String portalId;                       // Unique ID for file naming
    String ssid;                           // SSID being spoofed
    uint8_t channel;                       // Channel this portal runs on
    unsigned long lastHeartbeat;            // Last time we checked this portal
    unsigned long launchTime;               // When portal was launched
    bool hasCreds;                          // Whether credentials captured
    String capturedPassword;                 // Captured password if any
    bool victimConnected;                    // Whether a victim is actively connected
    unsigned long lastClientActivity;        // Last time victim interacted
    uint32_t clientFingerprint;               // Fingerprint of connected victim
    bool markedForRemoval;                    // Flag for cleanup after capture
};

// Karma configuration
typedef struct {
    bool enableAutoKarma;
    bool enableDeauth;
    bool enableSmartHop;
    bool prioritizeVulnerable;
    bool enableAutoPortal;
    uint16_t maxClients;
} KarmaConfig;

// Attack strategy configuration
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

private:
    void loadNextBatch();
    void broadcastSSID(const String &ssid);
    void rotateChannel();
    void sendBeaconFrame(const String &ssid, uint8_t channel);
    void recordResponse(const String &ssid);
    void launchAttackForResponse(const String &ssid, const String &mac);
};

// SSID Database class
class SSIDDatabase {
private:
    static std::vector<String> ssidCache;
    static bool cacheLoaded;
    static String currentFilename;
    static bool useLittleFS;
    static bool loadFromFile();

public:
    static size_t getCount();
    static String getSSID(size_t index);
    static std::vector<String> getAllSSIDs();
    static int findSSID(const String &ssid);
    static String getRandomSSID();
    static void getBatch(size_t startIndex, size_t count, std::vector<String> &result);
    static bool contains(const String &ssid);
    static size_t getAverageLength();
    static size_t getMaxLength();
    static size_t getMinLength();
    static bool setSourceFile(const String &filename, bool useLittleFS = false);
    static bool reload();
    static void clearCache();
    static bool isLoaded();
    static String getSourceFile();
};

// Operation modes for Karma
enum KarmaMode {
    MODE_PASSIVE = 0,    // Listen only, respond to probes
    MODE_BROADCAST = 1,  // Actively advertise SSIDs + beacons
    MODE_FULL = 2        // Both passive and broadcast
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
size_t buildEnhancedProbeResponse(uint8_t *buffer, const String &ssid, 
                                 const String &targetMAC, uint8_t channel, 
                                 const RSNInfo &rsn, bool isHidden);
size_t buildBeaconFrame(uint8_t *buffer, const String &ssid, 
                        uint8_t channel, const RSNInfo &rsn);
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
void saveCredentialsToFile(String ssid, String password);
void saveProbesToPCAP(FS &fs);
void launchBackgroundPortal(const String &ssid, uint8_t channel, const String &templateName);
void checkPortals();
String generatePortalId(const String &templateName);
void savePortalCredentials(const String &ssid, const String &identifier, 
                          const String &password, const String &mac,
                          uint8_t channel, const String &templateName,
                          const String &portalId);
String getDisplayName(const String &fullPath, bool isSD);
void matchAPSignal(uint8_t channel);
void setChannelWithSecond(uint8_t channel);  // Helper for channel setting

// External variables
extern std::map<uint32_t, ClientBehavior> clientBehaviors;  // Keyed by fingerprint
extern ProbeRequest probeBuffer[200];
extern uint16_t probeBufferIndex;
extern bool bufferWrapped;
extern KarmaConfig karmaConfig;
extern AttackConfig attackConfig;
extern ActiveBroadcastAttack broadcastAttack;
extern bool screenNeedsRedraw;
extern uint32_t pmkidCaptured;
extern uint32_t assocBlocked;
extern std::vector<BackgroundPortal*> activePortals;  // Note: pointer vector
extern KarmaMode currentMode;
extern bool karmaPaused;
extern bool handshakeCaptureEnabled;
extern std::vector<HandshakeCapture> handshakeBuffer;

#endif