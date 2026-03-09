#include "core/wifi/wifi_common.h"
#include "core/display.h"    // using displayRedStripe  and loop options
#include "core/mykeyboard.h" // usinf keyboard when calling rename
#include "core/powerSave.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/wifi/wifi_mac.h" // Set Mac Address - @IncursioHack
#include <esp_event.h>
#include <esp_netif.h>
#include <globals.h>
#include "esp_wifi.h"

extern bool showHiddenNetworks;
static TaskHandle_t timezoneTaskHandle = NULL;

void ensureWifiPlatform() {
    static bool netifInitialized = false;
    static bool eventLoopCreated = false;
    static portMUX_TYPE platformMux = portMUX_INITIALIZER_UNLOCKED;

    portENTER_CRITICAL(&platformMux);
    bool needNetif = !netifInitialized;
    bool needLoop = !eventLoopCreated;
    portEXIT_CRITICAL(&platformMux);

    if (needNetif) {
        ESP_ERROR_CHECK(esp_netif_init());
        portENTER_CRITICAL(&platformMux);
        netifInitialized = true;
        portEXIT_CRITICAL(&platformMux);
    }

    if (needLoop) {
        esp_err_t err = esp_event_loop_create_default();
        if (err != ESP_ERR_INVALID_STATE) { ESP_ERROR_CHECK(err); }
        portENTER_CRITICAL(&platformMux);
        eventLoopCreated = true;
        portEXIT_CRITICAL(&platformMux);
    }
}

bool _wifiConnect(const String &ssid, int encryption) {
    String password = bruceConfig.getWifiPassword(ssid);
    if (password == "" && encryption > 0) { password = keyboard(password, 63, "Network Password:", true); }
    bool connected = _connectToWifiNetwork(ssid, password);
    bool retry = false;

    while (!connected) {
        wakeUpScreen();

        options = {
            {"Retry",  [&]() { retry = true; } },
            {"Cancel", [&]() { retry = false; }},
        };
        loopOptions(options);

        if (!retry) {
            wifiDisconnect();
            return false;
        }

        password = keyboard(password, 63, "Network Password:", true);
        connected = _connectToWifiNetwork(ssid, password);
    }

    if (connected) {
        wifiConnected = true;
        wifiIP = WiFi.localIP().toString();
        bruceConfig.addWifiCredential(ssid, password);

        // Start timezone update in background if not already running
        if (timezoneTaskHandle == NULL) {
            xTaskCreate(updateTimezoneTask, "updateTimezone", 4096, NULL, 1, &timezoneTaskHandle);
        }
    }

    delay(200);
    return connected;
}

bool _connectToWifiNetwork(const String &ssid, const String &pwd) {
    drawMainBorderWithTitle("WiFi Connect");
    padprintln("");
    padprint("Connecting to: " + ssid + ".");
    WiFi.mode(WIFI_MODE_STA);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    WiFi.begin(ssid, pwd);

    int i = 1;
    while (WiFi.status() != WL_CONNECTED) {
        if (tft.getCursorX() >= tftWidth - 12) {
            padprintln("");
            padprint("");
        }
#ifdef HAS_SCREEN
        tft.print(".");
#else
        Serial.print(".");
#endif

        if (i > 20) {
            displayError("Wifi Offline");
            vTaskDelay(500 / portTICK_RATE_MS);
            break;
        }

        vTaskDelay(500 / portTICK_RATE_MS);
        i++;
    }

    return WiFi.status() == WL_CONNECTED;
}

bool _setupAP() {
    IPAddress AP_GATEWAY(172, 0, 0, 1);
    WiFi.softAPConfig(AP_GATEWAY, AP_GATEWAY, IPAddress(255, 255, 255, 0));
    WiFi.softAP(bruceConfig.wifiAp.ssid, bruceConfig.wifiAp.pwd, 6, 0, 4, false);
    wifiIP = WiFi.softAPIP().toString(); // update global var
    Serial.println("IP: " + wifiIP);
    wifiConnected = true;
    return true;
}

void wifiDisconnect() {
    WiFi.softAPdisconnect(true); // turn off AP mode
    WiFi.disconnect(true, true); // turn off STA mode
    WiFi.mode(WIFI_OFF);         // enforces WIFI_OFF mode
    wifiConnected = false;
    returnToMenu = true;
}

bool wifiConnectMenu(wifi_mode_t mode) {
    if (WiFi.isConnected()) return false; // safeguard

    switch (mode) {
        case WIFI_AP: // access point
            WiFi.mode(WIFI_AP);
            return _setupAP();
            break;

        case WIFI_STA: { // station mode
            WiFi.mode(WIFI_MODE_STA);
            applyConfiguredMAC();

            const int LINE_H    = 10;
            const int LIST_TOP  = 44;
            const int FOOT_Y    = tftHeight - 14;
            const int LIST_BOT  = FOOT_Y - 2;
            const int MAX_LINES = (LIST_BOT - LIST_TOP) / LINE_H;
            const int MAX_CHARS = (tftWidth - 2 * BORDER_PAD_X) / 6;

            std::vector<String> netLabels;
            std::vector<String> netSSIDs;
            std::vector<int>    netEncTypes;
            netLabels.reserve(30);
            netSSIDs.reserve(30);
            netEncTypes.reserve(30);

            bool stopScan = false;
            int  lastN    = 0;
            int  spinFrame = 0;
            unsigned long lastSpin = millis();

            drawMainBorderWithTitle("WiFi Scan");

            // Draw initial footer
            tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
            tft.drawString("Found: 0", BORDER_PAD_X, FOOT_Y, 1);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawRightString("OK=stop+select", tftWidth - BORDER_PAD_X, FOOT_Y, 1);

            // Start async scan
            WiFi.scanNetworks(true, showHiddenNetworks);

            while (!stopScan) {
                // Check buttons every loop — async scan never blocks
                if (check(SelPress) || check(NextPress)) {
                    WiFi.scanDelete();
                    stopScan = true;
                    break;
                }
                if (check(EscPress)) {
                    WiFi.scanDelete();
                    stopScan = true;
                    netLabels.clear();
                    break;
                }

                // Spin animation
                if (millis() - lastSpin > 300) {
                    spinFrame = (spinFrame + 1) & 3;
                    lastSpin = millis();
                }

                int result = WiFi.scanComplete();
                if (result >= 0) {
                    // Scan finished — rebuild list
                    int n = result;
                    netLabels.clear();
                    netSSIDs.clear();
                    netEncTypes.clear();

                    for (int i = 0; i < n; i++) {
                        String  ssid    = WiFi.SSID(i);
                        int     encType = WiFi.encryptionType(i);
                        int32_t rssi    = WiFi.RSSI(i);
                        int32_t ch      = WiFi.channel(i);
                        String  prefix  = (encType == WIFI_AUTH_OPEN) ? "" : "#";
                        String  encStr;
                        switch (encType) {
                            case WIFI_AUTH_OPEN:            encStr = "Open";  break;
                            case WIFI_AUTH_WEP:             encStr = "WEP";   break;
                            case WIFI_AUTH_WPA_PSK:         encStr = "WPA";   break;
                            case WIFI_AUTH_WPA2_PSK:        encStr = "WPA2";  break;
                            case WIFI_AUTH_WPA_WPA2_PSK:    encStr = "WPA/2"; break;
                            case WIFI_AUTH_WPA2_ENTERPRISE: encStr = "Ent";   break;
                            default:                        encStr = "?";      break;
                        }
                        if (ssid.length() == 0) ssid = "<Hidden> " + WiFi.BSSIDstr(i);
                        netLabels.push_back(prefix + ssid + " (" + String(rssi) + " ch" + String(ch) + " " + encStr + ")");
                        netSSIDs.push_back(ssid);
                        netEncTypes.push_back(encType);
                    }
                    WiFi.scanDelete();
                    lastN = n;

                    // Redraw list
                    tft.fillRect(BORDER_PAD_X, LIST_TOP, tftWidth - 2*BORDER_PAD_X, LIST_BOT - LIST_TOP, bruceConfig.bgColor);
                    for (int li = 0; li < MAX_LINES && li < n; li++) {
                        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                        String lbl = netLabels[li];
                        if ((int)lbl.length() > MAX_CHARS) lbl = lbl.substring(0, MAX_CHARS - 1);
                        tft.drawString(lbl, BORDER_PAD_X, LIST_TOP + li * LINE_H, 1);
                    }

                    // Restart for continuous scanning
                    WiFi.scanNetworks(true, showHiddenNetworks);
                }

                // Redraw footer with current count + spinner
                tft.fillRect(BORDER_PAD_X, FOOT_Y, tftWidth - 2*BORDER_PAD_X, 8, bruceConfig.bgColor);
                tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
                tft.drawString("Found: " + String(lastN), BORDER_PAD_X, FOOT_Y, 1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.drawRightString("OK=stop+select", tftWidth - BORDER_PAD_X, FOOT_Y, 1);

                vTaskDelay(20 / portTICK_PERIOD_MS);
            }

            if (netLabels.empty()) break;

            options = {};
            for (int i = 0; i < (int)netLabels.size(); i++) {
                String ssid    = netSSIDs[i];
                int    encType = netEncTypes[i];
                String lbl     = netLabels[i];
                options.push_back({lbl.c_str(), [=]() { _wifiConnect(ssid, encType); }});
            }
            options.push_back({"Hidden SSID", [=]() {
                String hiddenSsid = keyboard("", 32, "Your SSID");
                _wifiConnect(hiddenSsid.c_str(), 8);
            }});
            addOptionToMainMenu();
            loopOptions(options);
            options.clear();
        } break;
                case WIFI_AP_STA: // repeater mode
                          // _setupRepeater();
            break;

        default: // error handling
            Serial.println("Unknown wifi mode: " + String(mode));
            break;
    }

    if (returnToMenu) {
        wifiDisconnect(); // Forced turning off the wifi module if exiting back to the menu
        return false;
    }
    return wifiConnected;
}

void wifiConnectTask(void *pvParameters) {
    if (WiFi.status() == WL_CONNECTED) return;

    WiFi.mode(WIFI_MODE_STA);
    int nets = WiFi.scanNetworks();
    String ssid;
    String pwd;

    for (int i = 0; i < nets; i++) {
        ssid = WiFi.SSID(i);
        pwd = bruceConfig.getWifiPassword(ssid);
        if (pwd == "") continue;

        WiFi.begin(ssid, pwd);
        for (int j = 0; j < 50; j++) { // j avoids shadowing outer loop i
            if (WiFi.status() == WL_CONNECTED) {
                wifiConnected = true;
                wifiIP = WiFi.localIP().toString();

                // Start timezone update in background if not already running
                if (timezoneTaskHandle == NULL) {
                    xTaskCreate(updateTimezoneTask, "updateTimezone", 4096, NULL, 1, &timezoneTaskHandle);
                }
                drawStatusBar();
                break;
            }
            vTaskDelay(100 / portTICK_RATE_MS);
        }
    }
    WiFi.scanDelete();

    vTaskDelete(NULL);
    return;
}

String checkMAC() { return String(WiFi.macAddress()); }

bool wifiConnecttoKnownNet(void) {
    if (WiFi.isConnected()) return true; // safeguard
    bool result = false;
    int nets;
    // WiFi.mode(WIFI_MODE_STA);
    displayTextLine("Scanning Networks..");
    WiFi.disconnect(true, true);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    nets = WiFi.scanNetworks();
    for (int i = 0; i < nets; i++) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        String ssid = WiFi.SSID(i);
        String password = bruceConfig.getWifiPassword(ssid);
        if (password != "") {
            Serial.println("Connecting to: " + ssid);
            result = _connectToWifiNetwork(ssid, password);
        }
        // Maybe it finds a known network and can't connect, then try the next
        // until it gets connected (or not)
        if (result) {
            Serial.println("Connected to: " + ssid);
            break;
        }
    }
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        wifiIP = WiFi.localIP().toString();

        // Start timezone update in background if not already running
        if (timezoneTaskHandle == NULL) {
            xTaskCreate(updateTimezoneTask, "updateTimezone", 4096, NULL, 1, &timezoneTaskHandle);
        }
    }
    return result;
}

void updateTimezoneTask(void *pvParameters) {
    // Wait a bit for connection to stabilize before updating timezone
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Only update timezone if WiFi is still connected
    if (WiFi.isConnected() && wifiConnected) { updateClockTimezone(); }

    // Clear the task handle before deleting
    timezoneTaskHandle = NULL;
    vTaskDelete(NULL);
}
