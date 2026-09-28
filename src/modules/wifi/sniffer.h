#pragma once
#if !defined(LITE_VERSION)
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include <set>

struct HandshakeTracker {
    bool msg1 = false;
    bool msg2 = false;
    bool msg3 = false;
    bool msg4 = false;
};

extern HandshakeTracker hsTracker;

bool handshakeUsable(const HandshakeTracker &hs);

// List of channels to hop through
// priority channels are used more often
#ifdef CONFIG_IDF_TARGET_ESP32C5
const uint8_t all_wifi_channels[] = {1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,
                                     36,  40,  44,  48,  52,  56,  60,  64,  100, 104, 108, 112,
                                     116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161};
const uint8_t pri_wifi_channels[] = {1, 6, 11, 36, 40, 44, 48, 149, 153, 157, 161};
#else
const uint8_t all_wifi_channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
const uint8_t pri_wifi_channels[] = {1, 6, 11};
#endif
struct BeaconList {
    char MAC[6];
    uint8_t channel;
    // Define comparison operator to use <set>
    bool operator<(const BeaconList &other) const {
        // Compare MACs (using memcmp)
        int cmp = memcmp(MAC, other.MAC, sizeof(MAC));
        if (cmp != 0) {
            return cmp < 0; // if MACs are diferent, compares lexicografically
        }
        return channel < other.channel; // If MACs are equal, compare by channel
    }
};

enum class SnifferMode : uint8_t {
    Full,
    HandshakesOnly,
    DeauthOnly,
    Passive, // observe-only: analyze frames (client learning) but save nothing
};

extern int num_HS;
extern int num_EAPOL;
extern bool isLittleFS;
extern uint8_t ch;

void setHandshakeSniffer();
void sniffer_set_mode(SnifferMode mode);
SnifferMode sniffer_get_mode();
bool sniffer_full_mode_available();
bool sniffer_prepare_storage(FS *fs, bool sdDetected);
void sniffer_wait_for_flush(uint32_t timeoutMs = 2000);
void sniffer_reset_handshake_cache();
void markHandshakeReady(uint64_t key);
bool sniffer_is_handshake_ready(uint64_t bssidKey);
// True once M1+M2 (or M2+M3) were observed for this AP.
bool handshakeCrackable(const HandshakeTracker &hs);
bool sniffer_is_handshake_crackable(uint64_t key);
bool sniffer_ap_has_partial(uint64_t key);
// Number of distinct client MACs observed on an AP (0 when none known).
uint16_t sniffer_count_clients(const uint8_t bssid[6]);
// Total distinct clients observed across all APs (for live progress).
uint32_t sniffer_total_clients();
// APs that have observed clients but were never in a scan list (beacon
// missed, 4-addr/WDS frames, ...). Returns count; fills BSSIDs + the channel
// each was first heard on. Lets callers recover clients the AP scan missed.
uint8_t sniffer_list_client_aps(uint8_t outBssid[][6], uint8_t outCh[], uint8_t maxOut);
// Full client detail rows (MAC + last-seen IP + frame count), same sort order.
struct ClientDetail {
    uint8_t mac[6];
    uint8_t ip[4];
    uint32_t frames;
};
uint8_t sniffer_get_client_details(uint64_t apKey, ClientDetail *out, uint8_t maxOut);
// Copy of the last raw beacon frame seen for an AP (for IE parsing).
// Returns bytes copied, 0 when none cached.
uint16_t sniffer_get_beacon_frame(const uint8_t bssid[6], uint8_t *out, uint16_t outLen);

extern std::set<BeaconList> registeredBeacons;
extern std::set<String> SavedHS;

void newPacketSD(uint32_t ts_sec, uint32_t ts_usec, uint32_t len, uint8_t *buf, File pcap_file);

void openFile(FS &Fs);

bool writeHeader(File file);

void sniffer_setup();

void sniffer(void *buf, wifi_promiscuous_pkt_type_t type);

#endif
