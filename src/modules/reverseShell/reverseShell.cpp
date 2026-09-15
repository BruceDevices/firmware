#if !defined(LITE_VERSION)
#include "core/display.h"
#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

void ReverseShell() {
    AsyncWebServer webServer(80);
    AsyncWebSocket ws("/ws");
    DNSServer dnsServer;
    IPAddress apGateway(192, 168, 4, 1);
    WiFiServer tcpServer(23);
    WiFiClient tcpClient;
    bool shellConnected = false;
    bool wsConnected = false;

    // ── WebSocket Event Handler ────────────────────────────────
    auto onWsEvent = [&](AsyncWebSocket *server, AsyncWebSocketClient *client,
                         AwsEventType type, void *arg, uint8_t *data, size_t len) {
        switch (type) {
            case WS_EVT_CONNECT:
                wsConnected = true;
                client->text("Connected to BruceShell!\r\n");
                break;

            case WS_EVT_DISCONNECT:
                wsConnected = false;
                break;

            case WS_EVT_DATA:
                if (shellConnected && tcpClient) {
                    String cmd = String((char*)data);
                    cmd.trim();
                    if (cmd.length() > 0) {
                        tcpClient.println(cmd);

                        // Read output and send back via WebSocket
                        String output = "";
                        unsigned long timeout = millis() + 3000;
                        while (millis() < timeout) {
                            if (tcpClient.available()) {
                                output += tcpClient.readString();
                            }
                            if (output.endsWith("\n> ") || output.endsWith("\n$ ") || output.endsWith("\n# ")) {
                                break;
                            }
                            delay(10);
                        }
                        if (output.length() > 0) {
                            client->text(output);
                        } else {
                            client->text("[Command executed, no output]\r\n");
                        }
                    }
                } else {
                    client->text("Error: No shell connected.\r\n");
                }
                break;

            case WS_EVT_ERROR:
                // Handle error silently
                break;
        }
    };

    // ── Setup ──────────────────────────────────────────────────
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("Reverse Shell", tftWidth / 2, 10, 1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(15, 33);
    tft.println("Developed by Fourier & Ninja-jr");
    tft.println("Starting reverse shell server...");

    WiFi.mode(WIFI_AP);
    if (!WiFi.softAPConfig(apGateway, apGateway, IPAddress(255, 255, 255, 0))) {
        tft.println("Failed to configure AP");
        return;
    }

    // ── AP Password: bruce ─────────────────────────────────────
    if (!WiFi.softAP("BruceShell", "bruce")) {
        tft.println("Failed to start AP");
        return;
    }

    tft.println("Wi-Fi AP Started: BruceShell (pass: bruce)");
    tft.println("IP: " + apGateway.toString());

    tcpServer.begin();
    tft.println("TCP server started on port 23.");

    // ── Web Interface ──────────────────────────────────────────
    ws.onEvent(onWsEvent);
    webServer.addHandler(&ws);

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
            <!DOCTYPE html>
            <html>
            <head>
                <title>BruceShell</title>
                <style>
                    body { background: #0a0a1a; color: #00ff41; font-family: 'Courier New', monospace; margin: 0; padding: 20px; }
                    .container { max-width: 800px; margin: auto; }
                    .status { display: inline-block; width: 12px; height: 12px; border-radius: 50%; }
                    .online { background: #00ff41; }
                    .offline { background: #ff0040; }
                    input { width: 70%; padding: 10px; background: #16213e; color: #00ff41; border: 1px solid #00ff41; }
                    button { padding: 10px 20px; background: #16213e; color: #00ff41; border: 1px solid #00ff41; cursor: pointer; }
                    button:hover { background: #1a2a4e; }
                    #output { background: #0a0a1a; padding: 10px; min-height: 300px; max-height: 400px; white-space: pre-wrap; border: 1px solid #00ff41; margin-top: 10px; overflow-y: auto; }
                    .footer { margin-top: 20px; font-size: 12px; color: #666; }
                </style>
            </head>
            <body>
                <div class="container">
                    <h1>🔴 BruceShell <span id="statusDot" class="status offline"></span> <span id="statusText">Offline</span></h1>
                    <p>IP: 192.168.4.1 | Port: 23</p>
                    <div style="display: flex; gap: 10px; flex-wrap: wrap;">
                        <input type="text" id="cmd" placeholder="Enter command..." onkeyup="if(event.keyCode==13) sendCommand();" autofocus>
                        <button onclick="sendCommand();">Execute</button>
                        <button onclick="clearOutput();">Clear</button>
                    </div>
                    <div id="output">~ BruceShell\n~ Connected: Waiting for shell...</div>
                    <div class="footer">Connect via WebSocket ws://192.168.4.1/ws</div>
                </div>
                <script>
                    var ws = new WebSocket('ws://192.168.4.1/ws');
                    ws.onopen = function() {
                        document.getElementById('statusDot').className = 'status online';
                        document.getElementById('statusText').innerText = 'Online';
                    };
                    ws.onclose = function() {
                        document.getElementById('statusDot').className = 'status offline';
                        document.getElementById('statusText').innerText = 'Offline';
                    };
                    ws.onmessage = function(e) {
                        document.getElementById('output').innerText += e.data;
                        document.getElementById('output').scrollTop = document.getElementById('output').scrollHeight;
                    };
                    function sendCommand() {
                        var cmd = document.getElementById('cmd').value;
                        if (cmd) {
                            ws.send(cmd + '\n');
                            document.getElementById('cmd').value = '';
                            document.getElementById('cmd').focus();
                        }
                    }
                    function clearOutput() {
                        document.getElementById('output').innerText = '';
                    }
                </script>
            </body>
            </html>
        )rawliteral";
        request->send(200, "text/html", html);
    });

    webServer.begin();
    tft.println("Web server started on port 80!");
    tft.println("WebSocket server started on /ws");

    dnsServer.start(53, "*", apGateway);

    // ── Main Loop ──────────────────────────────────────────────
    while (true) {
        dnsServer.processNextRequest();
        ws.cleanupClients();

        if (!shellConnected) {
            tcpClient = tcpServer.accept();
            if (tcpClient) {
                tft.println("Client connected.");
                tcpClient.println("~Welcome to BruceShell.");
                tcpClient.println("~Developed by Fourier & Ninja-jr");
                tcpClient.println("~Type 'help' for available commands");
                shellConnected = true;
            }
        }

        if (shellConnected && !tcpClient.connected()) {
            tft.println("Client disconnected.");
            shellConnected = false;
            tcpClient.stop();
        }

        if (check(EscPress)) {
            tft.println("Exiting reverse shell server...");
            tcpServer.stop();
            ws.closeAll();
            webServer.end();
            dnsServer.stop();
            break;
        }
        delay(10);
    }
}
#endif
