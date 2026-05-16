// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Shows how to trigger an async client request from a browser request and send the client response back to the browser through websocket
//

#include <Arduino.h>
#if defined(ESP32) || defined(LIBRETINY)
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
#include <RPAsyncTCP.h>
#include <WiFi.h>
#endif

#include <ESPAsyncWebServer.h>

#define WIFI_SSID     "IoT"
#define WIFI_PASSWORD ""

static AsyncWebServer server(80);
static AsyncWebSocketMessageHandler wsHandler;
static AsyncWebSocket ws("/ws", wsHandler.eventHandler());

static const char *htmlContent PROGMEM = R"(
  <!DOCTYPE html>
  <html>
  <head>
    <title>WebSocket Tunnel Example</title>
  </head>
  <body>
    <h1>WebSocket Tunnel Example</h1>
    <div><input type="text" id="url" value="http://www.google.com" /></div>
    <div><button onclick='fetch()'>Fetch</button></div>
    <div><pre id="response"></pre></div>
    <script>
      var ws = new WebSocket('/ws');
      ws.binaryType = "arraybuffer";
      ws.onopen = function() {
        console.log("WebSocket connected");
      };
      ws.onmessage = function(event) {
        let uint8array = new Uint8Array(event.data);
        let string = new TextDecoder().decode(uint8array);
        console.log("WebSocket message: " + string);
        document.getElementById("response").innerText += string;
      };
      ws.onclose = function() {
        console.log("WebSocket closed");
      };
      ws.onerror = function(error) {
        console.log("WebSocket error: " + error);
      };
      function fetch() {
        document.getElementById("response").innerText = "";
        var url = document.getElementById("url").value;
        ws.send(url);
        console.log("WebSocket sent: " + url);
      }
    </script>
  </body>
  </html>
    )";
static const size_t htmlContentLength = strlen_P(htmlContent);

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected to WiFi!");
  Serial.println(WiFi.localIP());
#endif

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (const uint8_t *)htmlContent, htmlContentLength);
  });

  wsHandler.onMessage([](AsyncWebSocket *server, AsyncWebSocketClient *wsClient, const uint8_t *data, size_t len) {
    String url;
    String host;
    String port;
    String path;

    url.concat((const char *)data, len);

    if (!url.startsWith("http://")) {
      return;
    }

    if (!url.endsWith("/")) {
      url += "/";
    }

    // Parse the URL to extract the host and port
    int start = url.indexOf("://") + 3;
    int end = url.indexOf("/", start);
    if (end == -1) {
      end = url.length();
    }
    String hostPort = url.substring(start, end);
    int colonIndex = hostPort.indexOf(":");
    if (colonIndex != -1) {
      host = hostPort.substring(0, colonIndex);
      port = hostPort.substring(colonIndex + 1);
    } else {
      host = hostPort;
      port = "80";  // Default HTTP port
    }
    path = url.substring(end);

    Serial.printf("Host: %s\n", host.c_str());
    Serial.printf("Port: %s\n", port.c_str());
    Serial.printf("Path: %s\n", path.c_str());

    // Ensure client does not get deleted while the websocket holds a reference to it
    std::shared_ptr<AsyncClient> *safeAsyncClient = new std::shared_ptr<AsyncClient>(std::make_shared<AsyncClient>());
    AsyncClient *asyncClient = safeAsyncClient->get();

    asyncClient->onDisconnect([safeAsyncClient](void *arg, AsyncClient *client) {
      Serial.printf("Tunnel disconnected!\n");
      delete safeAsyncClient;
    });

    // register a callback when an error occurs
    // note: onDisconnect also called on error
    asyncClient->onError([](void *arg, AsyncClient *client, int8_t error) {
      Serial.printf("Tunnel error: %s\n", client->errorToString(error));
    });

    // register a callback when data arrives, to accumulate it
    asyncClient->onPacket(
      [safeAsyncClient](void *arg, AsyncClient *, struct pbuf *pb) {
        std::shared_ptr<AsyncClient> safeAsyncClientRef = *safeAsyncClient;  // add a reference
        AsyncWebSocketClient *wsClient = (AsyncWebSocketClient *)arg;
        Serial.printf("Tunnel received %u bytes\n", pb->len);
        AsyncWebSocketSharedBuffer wsBuffer =
          AsyncWebSocketSharedBuffer(new std::vector<uint8_t>((uint8_t *)pb->payload, (uint8_t *)pb->payload + pb->len), [=](std::vector<uint8_t> *bufptr) {
            delete bufptr;
            Serial.printf("ACK %u bytes\n", pb->len);
            safeAsyncClientRef->ackPacket(pb);
          });
        Serial.printf("Tunnel sending %u bytes\n", wsBuffer->size());
        Serial.printf("%.*s\n", (int)wsBuffer->size(), wsBuffer->data());
        wsClient->binary(std::move(wsBuffer));
      },
      wsClient
    );

    asyncClient->onConnect([=](void *arg, AsyncClient *client) {
      Serial.printf("Tunnel connected!\n");

      client->write("GET ");
      client->write(path.c_str());
      client->write(" HTTP/1.1\r\n");
      client->write("Host: ");
      client->write(host.c_str());
      client->write(":");
      client->write(port.c_str());
      client->write("\r\n");
      client->write("User-Agent: ESP32\r\n");
      client->write("Accept: */*\r\n");
      client->write("Connection: close\r\n");
      client->write("\r\n");
    });

    Serial.printf("Fetching: http://%s:%s%s\n", host.c_str(), port.c_str(), path.c_str());

    if (!asyncClient->connect(host.c_str(), port.toInt())) {
      Serial.printf("Failed to open tunnel!\n");
      delete safeAsyncClient;
    }
  });

  wsHandler.onConnect([](AsyncWebSocket *server, AsyncWebSocketClient *client) {
    Serial.printf("Client %" PRIu32 " connected\n", client->id());
    client->binary("WebSocket connected!");
  });

  wsHandler.onDisconnect([](AsyncWebSocket *server, uint32_t clientId) {
    Serial.printf("Client %" PRIu32 " disconnected\n", clientId);
  });

  server.addHandler(&ws);
  server.begin();
  Serial.println("Server started!");
}

static uint32_t lastHeap = 0;

void loop() {
  ws.cleanupClients(2);

#ifdef ESP32
  uint32_t now = millis();
  if (now - lastHeap >= 2000) {
    Serial.printf("Uptime: %3lu s, Free heap: %" PRIu32 "\n", millis() / 1000, ESP.getFreeHeap());
    lastHeap = now;
  }
#endif

  delay(500);
}
