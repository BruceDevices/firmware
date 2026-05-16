// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// WebSocket example
//

#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>

#include <ESPAsyncWebServer.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

void setup() {
  Serial.begin(115200);

#if SOC_WIFI_SUPPORTED || CONFIG_ESP_WIFI_REMOTE_ENABLED || LT_ARD_HAS_WIFI || CONFIG_ESP32_WIFI_ENABLED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  //
  // Run in terminal 1: websocat ws://192.168.4.1/ws => should stream data
  // Run in terminal 2: websocat ws://192.168.4.1/ws => should stream data
  // Run in terminal 3: websocat ws://192.168.4.1/ws => should fail:
  //
  // To send a message to the WebSocket server:
  //
  // echo "Hello!" | websocat ws://192.168.4.1/ws
  //
  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    (void)len;

    if (type == WS_EVT_CONNECT) {
      ws.textAll("new client connected");
      Serial.println("ws connect");
      client->ping();

    } else if (type == WS_EVT_DISCONNECT) {
      ws.textAll("client disconnected");
      Serial.println("ws disconnect");

    } else if (type == WS_EVT_ERROR) {
      Serial.println("ws error");

    } else if (type == WS_EVT_PONG) {
      Serial.println("ws pong");

    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      String msg = "";
      if (info->final && info->index == 0 && info->len == len) {
        if (info->message_opcode == WS_TEXT) {
          Serial.printf("ws text: %s\n", (char *)data);
        }
      }
    }
  });

  // shows how to prevent a third WS client to connect
  server.addHandler(&ws).addMiddleware([](AsyncWebServerRequest *request, ArMiddlewareNext next) {
    // ws.count() is the current count of WS clients: this one is trying to upgrade its HTTP connection
    if (ws.count() > 1) {
      // if we have 2 clients or more, prevent the next one to connect
      request->send(503, "text/plain", "Server is busy");
    } else {
      // process next middleware and at the end the handler
      next();
    }
  });

  server.addHandler(&ws);

  server.begin();
}

static uint32_t lastWS = 0;
static uint32_t deltaWS = 100;

static uint32_t lastHeap = 0;

void loop() {
  uint32_t now = millis();

  if (now - lastWS >= deltaWS) {
    ws.printfAll("kp%.4f", (10.0 / 3.0));
    lastWS = millis();
  }

  if (now - lastHeap >= 2000) {
    // cleanup disconnected clients or too many clients
    ws.cleanupClients();

    Serial.printf("Free heap: %" PRIu32 "\n", ESP.getFreeHeap());
    lastHeap = now;
  }
}
