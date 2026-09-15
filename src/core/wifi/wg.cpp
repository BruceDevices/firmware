bool isConnectedWireguard = false;
#ifndef LITE_VERSION
#include "wg.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/wifi/wifi_common.h"
#include <WireGuard-ESP32.h>
#include <globals.h>

char private_key[45];
IPAddress local_ip;
char public_key[45];
char endpoint_address[16];
int endpoint_port = 31337;

static constexpr const uint32_t UPDATE_INTERVAL_MS = 5000;

static WireGuard wg;

/*********************************************************************
**  Function: parse_config_file
**  parses wireguard config file wg.conf
**********************************************************************/
void parse_config_file(File configFile) {
    String line;

    while (configFile.available()) {
        line = configFile.readStringUntil('\n');
        Serial.println("==========PRINTING LINE");
        Serial.println(line);
        line.trim();

        if (line.startsWith("[Interface]") || line.isEmpty()) {
            // Skip [Interface] or empty lines
            continue;
        } else if (line.startsWith("PrivateKey")) {
            line.remove(0, line.indexOf('=') + 1);
            line.trim();
            Serial.println("Private Key: " + line);
            strncpy(private_key, line.c_str(), sizeof(private_key) - 1);
            private_key[sizeof(private_key) - 1] = '\0'; // Ensure null-terminated
        } else if (line.startsWith("Address")) {
            line.remove(0, line.indexOf('=') + 1);
            line.trim();
            Serial.println("Local IP: " + line);
            int slashIndex = line.indexOf('/');

            if (slashIndex != -1) {
                Serial.println("~~~~~~~~~~~~");
                Serial.println(line.substring(0, slashIndex));
                local_ip.fromString(line.substring(0, slashIndex));
            }

        } else if (line.startsWith("[Peer]")) {
            // add [Peer] section
        } else if (line.startsWith("PublicKey")) {
            line.remove(0, line.indexOf('=') + 1);
            line.trim();
            Serial.println("Public Key: " + line);
            strncpy(public_key, line.c_str(), sizeof(public_key) - 1);
            public_key[sizeof(public_key) - 1] = '\0'; // Ensure null-terminated
        } else if (line.startsWith("Endpoint")) {
            line.remove(0, line.indexOf('=') + 1);
            line.trim();
            int colonIndex = line.indexOf(':');

            if (colonIndex != -1) {
                // Serial.println("Endpoint Line: " + line);
                strncpy(
                    endpoint_address, line.substring(0, colonIndex).c_str(), sizeof(endpoint_address) - 1
                );
                endpoint_address[sizeof(endpoint_address) - 1] = '\0'; // Ensure null-terminated
                Serial.println("Endpoint Address: " + String(endpoint_address));
                endpoint_port = line.substring(colonIndex + 1).toInt();
                Serial.println("Endpoint Port: " + String(endpoint_port));
            }
        }
    }

    Serial.println("Closing file!");
    configFile.close();
}

/*********************************************************************
**  Function: read_and_parse_file
**  tries to open file wg.conf on local SD
**********************************************************************/
void read_and_parse_file() {
    if (!setupSdCard()) {
        sdcardMounted = false;
        Serial.println("Failed to initialize SD card");
        return;
    }

    File file = SD.open("/wg.conf");
    if (!file) {
        Serial.println("Failed to open wg.conf file");
        displayError("No wg.conf file on SD", true);
        return;
    }

    Serial.println("Readed config file!");

    Serial.println("Found file!");
    parse_config_file(file);
}

/*********************************************************************
**  Function: wg_setup
**  connect to wireguard tunnel
**********************************************************************/
void wg_setup() {
    if (!wifiConnected) wifiConnectMenu();

    read_and_parse_file();

    drawMainBorderWithTitle("WIREGUARD");
    printSubtitle("Connecting...");
    tft.setTextSize(FP);
    padprintln("");
    padprintln("Syncing time...");

    configTime(9 * 60 * 60, 0, "ntp.jst.mfeed.ad.jp", "ntp.nict.jp");

    padprintln("Initializing tunnel...");
    wg.begin(local_ip, private_key, endpoint_address, public_key, endpoint_port);

    drawMainBorderWithTitle("WIREGUARD");
    printSubtitle("Connected");
    tft.setTextSize(FP);
    padprintln("");
    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
    padprintln("Status: Connected");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    padprintln("");
    padprintln("Tunnel IP:");
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    padprintln(local_ip.toString());
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    padprintln("");
    padprintln("Endpoint: " + String(endpoint_address));
    padprintln("Port: " + String(endpoint_port));
    padprintln("");
    printFootnote("Press any key to return");

    isConnectedWireguard = true;

    while (!check(AnyKeyPress)) { delay(100); }
}
#endif
