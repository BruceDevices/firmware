// @IncursioHack
#include "core/wifi/wifi_mac.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include <esp_wifi.h>

void applyConfiguredMAC() {
    if (bruceConfig.wifiMAC.length() == 17 && validateMACFormat(bruceConfig.wifiMAC)) {
        uint8_t newMAC[6];
        sscanf(
            bruceConfig.wifiMAC.c_str(),
            "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &newMAC[0],
            &newMAC[1],
            &newMAC[2],
            &newMAC[3],
            &newMAC[4],
            &newMAC[5]
        );

        if (esp_wifi_set_mac(WIFI_IF_STA, newMAC) == ESP_OK) {
            Serial.println("[WiFi] Custom MAC applied: " + bruceConfig.wifiMAC);
        } else {
            Serial.println("[WiFi] Failed to apply custom MAC, using default");
        }
    }
}

bool validateMACFormat(const String &mac) {
    if (mac.length() != 17) return false;
    for (int i = 0; i < 17; i++) {
        if ((i + 1) % 3 == 0) {
            if (mac[i] != ':') return false;
        } else {
            if (!isxdigit(mac[i])) return false;
        }
    }
    return true;
}

bool setCustomMAC(const String &mac) {
    if (!validateMACFormat(mac)) {
        displayError("Invalid MAC Format!");
        return false;
    }
    bruceConfig.wifiMAC = mac;
    bruceConfig.saveFile();
    return true;
}

String generateRandomMAC() {
    uint8_t mac[6];
    mac[0] = 0x02;
    for (int i = 1; i < 6; i++) mac[i] = random(0, 256);

    char buf[18];
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

void wifiMACMenu() {
    String currentMAC;

    if (bruceConfig.wifiMAC != "" && validateMACFormat(bruceConfig.wifiMAC)) {
        currentMAC = bruceConfig.wifiMAC + " (Custom)";
    } else {
        currentMAC = WiFi.macAddress() + " (Default)";
    }

    displayTextLine("Current MAC: " + currentMAC);
    delay(1000);

    options.clear();
    options.push_back({"Default MAC", []() {
                           bruceConfig.wifiMAC = "";
                           bruceConfig.saveFile();
                           displayTextLine("Default MAC set");
                       }});

    options.push_back({"Set MAC", []() {
                           String newMAC = keyboard("", 17, "Enter MAC XX:YY:ZZ:AA:BB:CC");
                           if (setCustomMAC(newMAC)) { displayTextLine("MAC Saved: " + newMAC); }
                       }});

    options.push_back({"Random MAC", []() {
                           String randMAC = generateRandomMAC();
                           setCustomMAC(randMAC);
                           displayTextLine("Random MAC: " + randMAC);
                       }});

    loopOptions(options);
}
