/**
 * SOCKS4 / SOCKS4a proxy **server** for Bruce (ESP32 as proxy).
 *
 * Protocol: https://www.openssh.com/txt/socks4.protocol
 *
 * Asio: espressif/asio is archived; Asio lives in esp-protocols (upstream Asio).
 * Official socks4 example is a **client** (connect to proxy). We implement the server.
 * See socks4_proxy.h for links.
 */
#include "modules/wifi/socks4_proxy.h"
#include "core/display.h"
#include "core/wifi/wifi_common.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

static const uint8_t SOCKS4_VERSION = 4;
static const uint8_t SOCKS4_CMD_CONNECT = 1;
static const uint8_t SOCKS4_REP_GRANTED = 90;
static const uint8_t SOCKS4_REP_REJECTED = 91;
static const size_t RELAY_BUF_SIZE = 512;
static const unsigned long RELAY_POLL_MS = 10;

static void relayLoop(WiFiClient &client, WiFiClient &target) {
    uint8_t buf[RELAY_BUF_SIZE];
    while (client.connected() && target.connected()) {
        if (check(EscPress)) break;
        int n = 0;
        if (client.available()) {
            n = client.read(buf, sizeof(buf));
            if (n > 0) target.write(buf, (size_t)n);
        }
        if (target.available()) {
            n = target.read(buf, sizeof(buf));
            if (n > 0) client.write(buf, (size_t)n);
        }
        if (n <= 0) delay(RELAY_POLL_MS);
    }
}

static bool readSocks4Request(WiFiClient &client, uint8_t &cd, uint16_t &dstPort,
                             uint8_t dstIp[4], char *hostname, size_t hostnameSize) {
    if (hostname && hostnameSize) hostname[0] = '\0';
    uint8_t vn;
    if (client.read(&vn, 1) != 1 || vn != SOCKS4_VERSION) return false;
    if (client.read(&cd, 1) != 1) return false;
    uint8_t portBuf[2];
    if (client.read(portBuf, 2) != 2) return false;
    dstPort = (uint16_t)portBuf[0] << 8 | portBuf[1];
    if (client.read(dstIp, 4) != 4) return false;
    // USERID: read until null
    int c;
    while ((c = client.read()) != -1 && c != 0) {}
    if (c != 0) return false;
    // SOCKS4a: if DSTIP is 0.0.0.x with x != 0, read hostname
    if (hostname && hostnameSize && dstIp[0] == 0 && dstIp[1] == 0 && dstIp[2] == 0 && dstIp[3] != 0) {
        size_t i = 0;
        while (i < hostnameSize - 1) {
            c = client.read();
            if (c == -1) return false;
            if (c == 0) break;
            hostname[i++] = (char)c;
        }
        hostname[i] = '\0';
    }
    return true;
}

static void sendSocks4Reply(WiFiClient &client, uint8_t cd, uint16_t dstPort, const uint8_t *dstIp) {
    uint8_t reply[8] = {0, cd, (uint8_t)(dstPort >> 8), (uint8_t)(dstPort & 0xff),
                        dstIp[0], dstIp[1], dstIp[2], dstIp[3]};
    client.write(reply, 8);
}

void socks4Proxy(uint16_t port) {
    if (!wifiConnected) {
        wifiConnectMenu();
        if (!wifiConnected) {
            displayError("Connect to WiFi first", true);
            return;
        }
    }

    WiFiServer server(port);
    server.begin();

    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(2);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("SOCKS4 Proxy", tftWidth / 2, 10, 1);
    tft.setCursor(0, 28);
    tft.setTextSize(1);
    tft.println("Port: " + String(port));
    tft.println("IP: " + WiFi.localIP().toString());
    tft.println("Esc: exit");
    tft.println("Waiting for clients...");
    tft.println("");
    Serial.println("[SOCKS4] Listening on " + WiFi.localIP().toString() + ":" + String(port));

    char hostname[256];
    uint8_t dstIp[4];
    uint8_t cd;
    uint16_t dstPort;
    int connCount = 0;

    for (;;) {
        WiFiClient client = server.accept();
        if (check(EscPress)) {
            displayInfo("SOCKS4 proxy stopped", true);
            server.stop();
            return;
        }
        if (!client) continue;

        connCount++;
        client.setNoDelay(true);
        String clientIp = client.remoteIP().toString();
        tft.println("---");
        tft.println("CLIENT #" + String(connCount) + " " + clientIp);
        Serial.println("[SOCKS4] Client #" + String(connCount) + " connected from " + clientIp);

        if (!readSocks4Request(client, cd, dstPort, dstIp, hostname, sizeof(hostname))) {
            sendSocks4Reply(client, SOCKS4_REP_REJECTED, 0, dstIp);
            client.stop();
            tft.println("  Bad request");
            Serial.println("[SOCKS4] Bad request from " + clientIp);
            continue;
        }
        if (cd != SOCKS4_CMD_CONNECT) {
            sendSocks4Reply(client, SOCKS4_REP_REJECTED, 0, dstIp);
            client.stop();
            tft.println("  Unsupported command");
            continue;
        }

        String destStr = hostname[0] != '\0'
                             ? (String(hostname) + ":" + String(dstPort))
                             : (IPAddress(dstIp[0], dstIp[1], dstIp[2], dstIp[3]).toString() + ":" + String(dstPort));
        tft.println("  -> " + destStr + " ...");
        Serial.println("[SOCKS4] Connecting to " + destStr);

        WiFiClient target;
        bool ok = false;
        if (hostname[0] != '\0') {
            ok = target.connect(hostname, dstPort, 15000);
        } else {
            IPAddress ip(dstIp[0], dstIp[1], dstIp[2], dstIp[3]);
            ok = target.connect(ip, dstPort, 15000);
        }

        if (!ok) {
            sendSocks4Reply(client, SOCKS4_REP_REJECTED, 0, dstIp);
            client.stop();
            tft.println("  FAILED");
            Serial.println("[SOCKS4] Connect failed: " + destStr);
            continue;
        }

        tft.println("  OK - relaying");
        Serial.println("[SOCKS4] Tunnel up: " + clientIp + " <-> " + destStr);

        target.setNoDelay(true);
        sendSocks4Reply(client, SOCKS4_REP_GRANTED, dstPort, dstIp);
        relayLoop(client, target);

        target.stop();
        client.stop();
        tft.println("  Closed");
        Serial.println("[SOCKS4] Tunnel closed: " + clientIp + " -> " + destStr);
    }
}
