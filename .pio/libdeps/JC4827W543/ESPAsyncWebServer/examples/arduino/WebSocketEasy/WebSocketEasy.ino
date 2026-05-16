// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// WebSocket example using the easy to use AsyncWebSocketMessageHandler handler that only supports unfragmented messages
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

static AsyncWebServer server(80);

// create an easy-to-use handler
static AsyncWebSocketMessageHandler wsHandler;

// add it to the websocket server
static AsyncWebSocket ws("/ws", wsHandler.eventHandler());

// alternatively you can do as usual:
//
// static AsyncWebSocket ws("/ws");
// ws.onEvent(wsHandler.eventHandler());

static const char *htmlContent PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
  <title>WebSocket</title>
</head>
<body>
  <h1>WebSocket Example</h1>
  <p>Open your browser console!</p>
  <input type="text" id="message" placeholder="Type a message">
  <button onclick='sendMessage()'>Send</button>
  <script>
    var ws = new WebSocket('ws://192.168.4.1/ws');
    ws.onopen = function() {
      console.log("WebSocket connected");
    };
    ws.onmessage = function(event) {
      console.log("WebSocket message: " + event.data);
    };
    ws.onclose = function() {
      console.log("WebSocket closed");
    };
    ws.onerror = function(error) {
      console.log("WebSocket error: " + error);
    };
    function sendMessage() {
      var message = document.getElementById("message").value;
      ws.send(message);
      console.log("WebSocket sent: " + message);
    }
  </script>
</body>
</html>
  )";
static const size_t htmlContentLength = strlen_P(htmlContent);

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // serves root html page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (const uint8_t *)htmlContent, htmlContentLength);
  });

  wsHandler.onConnect([](AsyncWebSocket *server, AsyncWebSocketClient *client) {
    Serial.printf("Client %" PRIu32 " connected\n", client->id());
    server->textAll("New client: " + String(client->id()));
  });

  wsHandler.onDisconnect([](AsyncWebSocket *server, uint32_t clientId) {
    Serial.printf("Client %" PRIu32 " disconnected\n", clientId);
    server->textAll("Client " + String(clientId) + " disconnected");
  });

  wsHandler.onError([](AsyncWebSocket *server, AsyncWebSocketClient *client, uint16_t errorCode, const char *reason, size_t len) {
    Serial.printf("Client %" PRIu32 " error: %" PRIu16 ": %s\n", client->id(), errorCode, reason);
  });

  wsHandler.onMessage([](AsyncWebSocket *server, AsyncWebSocketClient *client, const uint8_t *data, size_t len) {
    Serial.printf("Client %" PRIu32 " data: %s\n", client->id(), (const char *)data);
    server->textAll(data, len);
  });

  wsHandler.onFragment([](AsyncWebSocket *server, AsyncWebSocketClient *client, const AwsFrameInfo *frameInfo, const uint8_t *data, size_t len) {
    Serial.printf("Client %" PRIu32 " fragment %" PRIu32 ": %s\n", client->id(), frameInfo->num, (const char *)data);
  });

  server.addHandler(&ws);
  server.begin();
}

static uint32_t lastWS = 0;
static uint32_t deltaWS = 2000;

void loop() {
  uint32_t now = millis();

  if (now - lastWS >= deltaWS) {
    ws.cleanupClients();
    ws.printfAll("now: %" PRIu32 "\n", now);
    lastWS = millis();
#ifdef ESP32
    Serial.printf("Free heap: %" PRIu32 "\n", ESP.getFreeHeap());
#endif
  }
}
