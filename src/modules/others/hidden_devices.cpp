/**
 * @file hidden_devices.cpp
 * @brief Hidden camera / microphone detector for Bruce firmware.
 *
 * Heuristic detection over WiFi and BLE:
 *  - Vendor OUI fingerprinting (IEEE OUI registry): dedicated camera/security
 *    vendors, professional audio vendors, and Espressif chips (weak signal,
 *    most cheap hidden cameras run ESP8266/ESP32).
 *  - SSID / BLE advertisement name patterns of common camera apps & brands
 *    (V380, iCSee, CamHi, Tuya, Tapo, ...).
 *  - Hidden-SSID networks as an extra weak signal.
 *
 * LIMITS (be honest with the user):
 *  - Heuristic only: a hit is a SUSPECT, not a confirmation. Phones/routers
 *    share OUIs with some vendors; verify physically.
 *  - No RF band sweep (CC1101) and no acoustic detection in this version:
 *    it cannot see pure analog (1.2 GHz) or fully offline devices.
 *
 * OUI data extracted from the official IEEE registry (standards-oui.ieee.org).
 */
#if !defined(LITE_VERSION)

#include "hidden_devices.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "globals.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <vector>

// ── OUI prefix tables (IEEE registry, format "AABBCC") ───────────
// Dedicated camera / security vendors (Hikvision, Dahua, Tuya, EZVIZ, Wyze, Arlo, Axis, Uniview, ...)
static const char *CAM_OUIS[] = {
    "000131", "0002D1", "000463", "000918", "0019EE", "001F54", "00337A", "00408C", "0050A1", "00BC99",
    "00E0F2", "040312", "04EECD", "083BC1", "08524E", "085411", "08A189", "08CC81", "08EDED", "0C75D2",
    "0CA64C", "0CAAEE", "1012FB", "105A17", "10D561", "14A78B", "14BA88", "183C98", "1868CB", "1869D8",
    "188025", "18DE50", "1C90FF", "202C05", "20BBBC", "20F1B2", "240F9B", "2428FD", "2432AE", "244845",
    "24526A", "24B105", "2857BE", "2CA59C", "2CAA8E", "2CB471", "30487D", "30DDAA", "340962", "34C6DD",
    "381F8D", "382CE5", "3876CA", "38A5C9", "38AF29", "38F25D", "3C0B59", "3C1BF8", "3CE36B", "3CEF8C",
    "407AA4", "40ACBF", "40B570", "4419B6", "4447CC", "44A642", "44B423", "486264", "48785B", "48EA63",
    "4C11BF", "4C1F86", "4C62DF", "4C99E8", "4CA919", "4CB008", "4CBD8F", "4CF5DC", "508A06", "508BB9",
    "50E538", "548C81", "54C415", "54D60D", "5803FB", "5850ED", "5870C6", "588FCF", "5C345B", "5CF51A",
    "607EA4", "64244D", "64DB8B", "64F2FB", "64FD29", "6849B2", "68572D", "686DBC", "6C1C71", "6CF17E",
    "708976", "70EE50", "743FC2", "74C929", "78A5DD", "78A6A0", "78C1AE", "78DF72", "7C78B2", "7CF666",
    "80482C", "80489F", "80647C", "807C62", "80BEAF", "80F5AE", "849459", "849A40", "84E342", "88263F",
    "88DE39", "8C1D55", "8C22D2", "8CE748", "8CE9B4", "8CFCA0", "9002A9", "94E1AC", "94EC13", "94F827",
    "988B0A", "989DE5", "98DF82", "98F112", "98F9CC", "9C1463", "A09208", "A0BD1D", "A0FF0C", "A41162",
    "A41437", "A42902", "A44BD9", "A4A459", "A4D5C2", "A88055", "A8CA87", "A8DDBC", "AC1C26", "ACB92F",
    "ACCB51", "ACCC8E", "ACF878", "B0FF0D", "B4101C", "B44C3B", "B4A382", "B8060D", "B88880", "B8A44F",
    "BC2978", "BC325F", "BC351E", "BC51FE", "BC5E33", "BC9B5E", "BCAD28", "BCBAC2", "C0395A", "C0517E",
    "C056E3", "C06DED", "C0F853", "C42F90", "C47905", "C482E1", "C4AAC4", "C8A702", "CC13F3", "CC8CBF",
    "D032E7", "D03F27", "D082E8", "D082EB", "D4430E", "D4A651", "D4E853", "D81F12", "D8C80C", "D8D668",
    "D8FC92", "DC07F8", "DCD26A", "E02EFE", "E0508B", "E0BAAD", "E0CA3C", "E0DF13", "E4246C", "E43022",
    "E4AEE4", "E4D58B", "E82725", "E8A0ED", "EC71DB", "EC97E0", "ECA971", "ECC89C", "F0C88B", "F47018",
    "F4B1C2", "F8172D", "F84DFC", "F8CE07", "FC2422", "FC3CD7", "FC5F49", "FC671F", "FC9C98", "FC9FFD",
    "FCB69D",
};
// Professional / wireless audio vendors (Shure, Sennheiser, Plantronics, Bose, GN Audio/Jabra, RODE, ...)
static const char *AUDIO_OUIS[] = {
    "00022E", "000389", "0004C6", "000A40", "000A45", "000C8A", "000EDD", "000FD7", "001564", "00165A",
    "001694", "001950", "00197F", "001B66", "001CD7", "00237F", "002600", "006044", "00A0DE", "00E099",
    "0452C7", "0488E2", "08C8C2", "08DF1F", "0CE0E4", "2811A5", "2856C1", "2C41A1", "305075", "3409E9",
    "384554", "385C76", "48221D", "482567", "48C1AC", "4C875D", "50C275", "50C2ED", "60ABD2", "68F21F",
    "6CFBED", "70BF92", "745C4B", "74604C", "782B64", "8C9B2D", "9C645E", "9CDF03", "9CF55F", "A056B2",
    "AC44F2", "ACBF71", "B0BC7A", "B4827B", "BC87FA", "BCF292", "C87B23", "D834EE", "E422A5", "E458BC",
    "F4B688", "F4D580", "F8E877", "FCF861",
};
// Espressif chips — WEAK signal (cheap hidden cams are ESP8266/ESP32)
static const char *ESPRESSIF_OUIS[] = {
    "004B12", "007007", "00FB4A", "048308", "04B247", "083A8D", "083AF2", "089272", "08A6F7", "08AD0A",
    "08B61F", "08D1F9", "08F9E0", "0C4EA0", "0C8B95", "0CB815", "0CDC7E", "10003B", "10061C", "1020BA",
    "1051DB", "10521C", "1091A8", "1097BD", "10A85B", "10B41D", "10BDA3", "140808", "142B2F", "14335C",
    "146393", "14C19F", "188B0E", "18FE34", "1C2904", "1C6465", "1C6920", "1C8B84", "1C8F57", "1C9DC2",
    "1CC3AB", "1CDBD4", "1CE4CB", "202565", "2043A8", "20500D", "206EF1", "209BA9", "20D5C2", "20E5A4",
    "20E7C8", "240AC4", "244CAB", "24587C", "2462AB", "246F28", "24A160", "24B2DE", "24D7EB", "24DCC3",
    "24EC4A", "2805A5", "28372F", "28562F", "288485", "2C3AE8", "2CBCBB", "2CF432", "3030F9", "3076F5",
    "308398", "30AEA4", "30C6F7", "30C922", "30EDA0", "345F45", "348518", "34865D", "349454", "34987A",
    "34AB95", "34B472", "34B7DA", "34C934", "34CDB0", "38182B", "383E51", "3844BE", "3C0D0D", "3C0F02",
    "3C6105", "3C71BF", "3C8427", "3C8A1F", "3CDC75", "3CE255", "3CE90E", "4022D8", "402BD6", "404CCA",
    "409151", "40F520", "441793", "441BF6", "441D64", "447B30", "44B176", "44BD8D", "4827E2", "4831B7",
    "483FDA", "485519", "489D31", "48AFF3", "48CA43", "48E729", "48F6EE", "4C11AE", "4C7525", "4C8421",
    "4CC382", "4CEBD6", "500291", "50787D", "5416A5", "543204", "5443B2", "545AA6", "549DEA", "582ABD",
    "582BD3", "588C81", "58B145", "58BF25", "58CF79", "58E6C5", "5C013B", "5CCF7F", "600194", "6055F9",
    "648914", "64B708", "64E833", "680947", "6825DD", "686725", "689DD2", "68B6B3", "68C63A", "68EE8F",
    "68FE71", "6C2FA0", "6C3DD8", "6CB456", "6CC4AF", "6CC840", "6CE210", "70039F", "70041D", "704BCA",
    "70AF09", "70B8F6", "744DBD", "748927", "781C3C", "782184", "78421C", "78E36D", "78EE4C", "7C0C5F",
    "7C2C67", "7C4FAD", "7C7398", "7C87CE", "7C9EBD", "7CD544", "7CDFA1", "7CE8B1", "80456B", "8053E0",
    "80646F", "806599", "807D3A", "80B54E", "80F1B2", "80F3DA", "840D8E", "841FE8", "84C7BB", "84CCA8",
    "84F3EB", "84F703", "84FCE6", "8813BF", "8856A6", "885721", "88F155", "8C4598", "8C4B14", "8C4F00",
    "8C8C29", "8C94DF", "8CAAB5", "8CBFEA", "8CCE4E", "8CFD49", "901506", "90380C", "90649B", "907069",
    "9097D5", "90B339", "90DA72", "90E5B1", "943CC6", "9451DC", "9454C5", "94A990", "94B555", "94B97E",
    "94E686", "983DAE", "9888E0", "98A316", "98C377", "98CDAC", "98F4AB", "9C139E", "9C96D5", "9C9C1F",
    "9C9E6E", "9CCC01", "A020A6", "A0764E", "A085E3", "A0A3B3", "A0B765", "A0DD6C", "A0F262", "A47B9D",
    "A4CB8F", "A4CF12", "A4E57C", "A4E972", "A4F00F", "A8032A", "A842E3", "A84674", "A848FA", "A8FD07",
    "AC0BFB", "AC1518", "AC276E", "AC67B2", "ACA26A", "ACA704", "ACD074", "ACEBE6", "B03FD3", "B08184",
    "B0A604", "B0A732", "B0B21C", "B0CBD8", "B43A45", "B48A0A", "B4A64A", "B4BFE9", "B4E62D", "B4FEB5",
    "B81F3F", "B87B4D", "B8BB11", "B8D61A", "B8F009", "B8F862", "BC1926", "BCDDC2", "BCFF4D", "C049EF",
    "C04E30", "C05D89", "C0CDD6", "C44F33", "C45BBE", "C49E7E", "C4D8D5", "C4DD57", "C4DEE2", "C82B96",
    "C82E18", "C88541", "C88A7B", "C8C9A3", "C8DA29", "C8EA71", "C8ED8B", "C8F09E", "CC50E3", "CC68C7",
    "CC7B5C", "CC7E1F", "CC8DA2", "CCBA97", "CCDBA7", "CCE732", "D09AAF", "D0CF13", "D0EF76", "D40592",
    "D48AFC", "D48C49", "D4D4DA", "D4E9F4", "D4F98D", "D8132A", "D83BDA", "D885AC", "D8A01D", "D8BC38",
    "D8BFC0", "D8F15B", "DC0675", "DC0A69", "DC1ED5", "DC4F22", "DC5475", "DC55B1", "DCB4D9", "DCDA0C",
    "E05A1B", "E072A1", "E08CFE", "E09806", "E0E2E6", "E465B8", "E4B063", "E4B323", "E80690", "E831CD",
    "E83DC1", "E868E7", "E86BEA", "E89F6D", "E8DB84", "E8EDBD", "E8F60A", "EC6260", "EC64C9", "EC94CB",
    "ECC9FF", "ECD61B", "ECDA3B", "ECE334", "ECFABC", "F008D1", "F0161D", "F024F9", "F09E9E", "F0F5BD",
    "F412FA", "F42DC9", "F4650B", "F4CFA2", "F80C4E", "F843EE", "F85B1B", "F8B3B7", "FC012C", "FCB467",
    "FCE8C0", "FCF5C4",
};

// ── Name/SSID patterns (uppercased substring match) ──────────────
static const char *SSID_PATTERNS[] = {
    "IPC-", "IPCAM", "WEBCAM", "WIFICAM", "HDWIFI", "SMARTCAM", "TAPO", "V380",
    "ICSEE", "CAMHI", "YCC365", "SRICAM", "ANSJER", "HISEEU", "XMEYE", "WYZE",
    "EZVIZ", "IMOU", "REOLINK", "FOSCAM", "HIKVISION", "DAHUA", "MI CAMERA", "XIAOMICAM",
    "MIBO", "DVR", "NVR", "ONVIF", "DOORBELL", "RING-", "BLINK", "SECURITY CAM",
    "SPY CAM", "HIDDEN CAM", "C1C",
};
static const char *BLE_CAM_PATTERNS[] = {
    "IPC", "IPCAM", "CAM-", "-CAM", "CAMERA", "TAPO", "V380", "ICSEE",
    "TUYA", "WYZE", "EZVIZ", "IMOU", "REOLINK", "HIKVISION", "DAHUA", "XIAOFANG",
    "DOORBELL", "DVR", "NVR", "ONVIF", "WIFICAM",
};
static const char *BLE_AUD_PATTERNS[] = {
    "MICROPHONE", "MICROFONO", "LAVALIER", "LAPEL", "WIRELESS MIC", " MIC", "MIC-", "LAV-",
    "JABRA", "POLY", "PLANTRONIC", "BOSE", "SENNHEI", "SHURE", "RODE", "RECORD",
    "DICTAPHONE", "DICTAFONE", "AKG", "HARMAN", "JBL", "TASCAM", "SARAMON", "BOYA",
    "HOLLYLAND", "COMICA", "TAKSTAR", "SAMSON", "BEHRINGER", "YAMAHA", "LINE 6", "GN AUDIO",
};

#define ARR_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

struct HiddenDevice {
    String name;
    String detail;
};

// Exact 6-hex-digit OUI compare ("AABBCC")
static bool inOuiTable(const char *table[], int len, const String &oui) {
    if (oui.length() != 6) return false;
    for (int i = 0; i < len; i++)
        if (oui == table[i]) return true;
    return false;
}

static String ouiFromMacStr(const String &mac) {
    // Accepts "AA:BB:CC:..." or "AA-BB-CC-..." → "AABBCC" (uppercased)
    String m = mac; m.toUpperCase(); m.replace(":", ""); m.replace("-", "");
    if (m.length() < 6) return "";
    return m.substring(0, 6);
}

// Returns bonus points for the first matching pattern (avoids double count)
static int patternScore(const String &upper, const char *pats[], int n, String *why) {
    for (int i = 0; i < n; i++) {
        if (upper.indexOf(pats[i]) >= 0) {
            if (why) *why += String("name~") + pats[i] + " | ";
            return 2;
        }
    }
    return 0;
}

static void hd_showResults(const std::vector<HiddenDevice> &devs, const char *title, int scanned) {
    if (devs.empty()) {
        displayInfo(String("No suspects found\n(") + scanned + " devices scanned)", true);
        return;
    }
    // Keep label strings alive while the menu runs
    static std::vector<String> labels;
    labels.clear();
    options.clear();
    for (const auto &d : devs) {
        labels.push_back(d.name);
        String detail = d.detail; // copy for the lambda
        options.push_back({labels.back().c_str(), [detail]() { displayInfo(detail, true); }});
    }
    options.push_back({"Main Menu", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, title);
}

// ── WiFi detector ────────────────────────────────────────────────
static void hd_wifi_scan() {
    wifi_mode_t prev = WiFi.getMode();
    WiFi.mode(WIFI_STA);
    delay(100);
    displayInfo("Scanning WiFi...\n(incl. hidden SSIDs)", false);

    int n = WiFi.scanNetworks(false, true, false, 300);
    WiFi.scanDelete();
    if (n <= 0) {
        if (prev == WIFI_MODE_NULL) WiFi.mode(WIFI_OFF);
        displayInfo("No networks found", true);
        return;
    }

    std::vector<HiddenDevice> devs;
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        bool hidden = ssid.isEmpty();
        String mac = WiFi.BSSIDstr(i);
        String oui = ouiFromMacStr(mac);
        int rssi = WiFi.RSSI(i);
        uint8_t ch = WiFi.channel(i);

        int score = 0;
        String why;
        String up = ssid; up.toUpperCase();

        if (inOuiTable(CAM_OUIS, ARR_LEN(CAM_OUIS), oui)) {
            score += 3; why += "OUI:camera-vendor | ";
        } else if (inOuiTable(ESPRESSIF_OUIS, ARR_LEN(ESPRESSIF_OUIS), oui)) {
            score += 1; why += "OUI:espressif | ";
        }
        if (!hidden) {
            score += patternScore(up, SSID_PATTERNS, ARR_LEN(SSID_PATTERNS), &why);
        } else {
            score += 1; why += "hidden-SSID | ";
        }

        if (score >= 2) {
            HiddenDevice d;
            d.name = (hidden ? String("[hidden] ") : String("")) + mac + "  " + String(rssi) + "dBm";
            d.detail = String("WiFi SUSPECT\nSSID: ") + (hidden ? "(hidden)" : ssid) +
                       "\nBSSID: " + mac + "\nCh: " + String(ch) + "  RSSI: " + String(rssi) +
                       "dBm\nWhy: " + why +
                       "\nVerify: RTSP port 554 / ONVIF 8000";
            devs.push_back(d);
        }
    }

    if (prev == WIFI_MODE_NULL) WiFi.mode(WIFI_OFF);
    hd_showResults(devs, "WiFi Suspects", n);
}

// ── BLE detector ─────────────────────────────────────────────────
static void hd_ble_scan() {
    displayInfo("Scanning BLE (5s)...", false);
    NimBLEDevice::init("Bruce");
    NimBLEScan *scan = NimBLEDevice::getScan();
    if (scan == nullptr) {
        NimBLEDevice::deinit(true);
        displayError("BLE init failed");
        return;
    }
    scan->setActiveScan(true);
    scan->setInterval(97);
    scan->setWindow(67);

    NimBLEScanResults results = scan->getResults(5 * 1000, true);
    int total = results.getCount();

    std::vector<HiddenDevice> devs;
    for (int i = 0; i < total; i++) {
        const NimBLEAdvertisedDevice *dev = results.getDevice(i);
        String name = String(dev->getName().c_str());
        String mac = String(dev->getAddress().toString().c_str());
        String oui = ouiFromMacStr(mac);
        int rssi = dev->getRSSI();

        int score = 0;
        String why;
        if (inOuiTable(CAM_OUIS, ARR_LEN(CAM_OUIS), oui)) {
            score += 3; why += "OUI:camera-vendor | ";
        }
        if (inOuiTable(AUDIO_OUIS, ARR_LEN(AUDIO_OUIS), oui)) {
            score += 3; why += "OUI:audio-vendor | ";
        }
        if (inOuiTable(ESPRESSIF_OUIS, ARR_LEN(ESPRESSIF_OUIS), oui)) {
            score += 1; why += "OUI:espressif | ";
        }
        String up = name; up.toUpperCase();
        if (!name.isEmpty()) {
            int s = patternScore(up, BLE_CAM_PATTERNS, ARR_LEN(BLE_CAM_PATTERNS), &why);
            if (s == 0) s = patternScore(up, BLE_AUD_PATTERNS, ARR_LEN(BLE_AUD_PATTERNS), &why);
            score += s;
        }

        if (score >= 2) {
            HiddenDevice d;
            d.name = (name.isEmpty() ? mac : name) + "  " + String(rssi) + "dBm";
            d.detail = String("BLE SUSPECT\nName: ") + (name.isEmpty() ? "(unknown)" : name) +
                       "\nMAC: " + mac + "\nRSSI: " + String(rssi) +
                       "dBm\nWhy: " + why +
                       "\nMic hits = wireless/BT mic\n(2.4G analog mics need RF sweep)";
            devs.push_back(d);
        }
    }

    scan->clearResults();
    NimBLEDevice::deinit(true);
    hd_showResults(devs, "BLE Suspects", total);
}

// ── Info ─────────────────────────────────────────────────────────
static void hd_about() {
    displayInfo(
        String("Hidden Device Detector\n") +
        "Heuristic WiFi+BLE scan\n" +
        "Signals: vendor OUI (IEEE),\n" +
        "SSID/adv-name patterns,\n" +
        "hidden SSID, ESP chips\n" +
        "--------------------\n" +
        "LIMITS: suspects, not proof.\n" +
        "No RF sweep (CC1101) yet:\n" +
        "analog 1.2G/2.4G cams and\n" +
        "offline mics invisible.\n" +
        "ALWAYS verify physically",
        true);
}

// ── Menu ─────────────────────────────────────────────────────────
void hidden_devices() {
    options = {
        {"WiFi Scan",  hd_wifi_scan },
        {"BLE Scan",   hd_ble_scan  },
        {"Info",       hd_about     },
        {"Main Menu",  []() { returnToMenu = true; }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Hidden Devices");
}

#endif // !LITE_VERSION
