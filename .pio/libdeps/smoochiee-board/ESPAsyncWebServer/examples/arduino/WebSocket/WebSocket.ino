// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// WebSocket example
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
    setInterval(function() {
      if (ws.readyState === WebSocket.OPEN) {
        ws.send("msg from browser");
      }
    }, 1000);
  </script>
</body>
</html>
  )";
static const size_t htmlContentLength = strlen_P(htmlContent);

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

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

  //
  // Run in terminal 1: websocat ws://192.168.4.1/ws => should stream data
  // Run in terminal 2: websocat ws://192.168.4.1/ws => should stream data
  // Run in terminal 3: websocat ws://192.168.4.1/ws => should fail:
  //
  // To send a message to the WebSocket server (\n at the end):
  // > echo "Hello!" | websocat ws://192.168.4.1/ws
  //
  // Generates 2001 characters (\n at the end) to cause a fragmentation (over TCP MSS):
  // > openssl rand -hex 1000 | websocat ws://192.168.4.1/ws
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
      Serial.printf(
        "index: %" PRIu64 ", len: %" PRIu64 ", final: %" PRIu8 ", opcode: %" PRIu8 ", framelen: %d\n", info->index, info->len, info->final,
        info->message_opcode, len
      );

      // complete frame
      if (info->final && info->index == 0 && info->len == len) {
        if (info->message_opcode == WS_TEXT) {
          Serial.printf("ws text: %s\n", (char *)data);
          client->ping();
          // Also send a message in the message queue when we get one
          ws.textAll("Message received: " + String((char *)data));
        }

      } else {
        // incomplete frame
        if (info->index == 0) {
          if (info->num == 0) {
            Serial.printf(
              "ws[%s][%" PRIu32 "] [%" PRIu32 "] MSG START %s\n", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary"
            );
          }
          Serial.printf("ws[%s][%" PRIu32 "] [%" PRIu32 "] FRAME START len=%" PRIu64 "\n", server->url(), client->id(), info->num, info->len);
        }

        Serial.printf(
          "ws[%s][%" PRIu32 "] [%" PRIu32 "] FRAME %s, index=%" PRIu64 ", len=%" PRIu32 "]: ", server->url(), client->id(), info->num,
          (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, (uint32_t)len
        );

        if (info->message_opcode == WS_TEXT) {
          Serial.printf("%s\n", (char *)data);
        } else {
          for (size_t i = 0; i < len; i++) {
            Serial.printf("%02x ", data[i]);
          }
          Serial.printf("\n");
        }

        if ((info->index + len) == info->len) {
          Serial.printf("ws[%s][%" PRIu32 "] [%" PRIu32 "] FRAME END\n", server->url(), client->id(), info->num);

          if (info->final) {
            Serial.printf("ws[%s][%" PRIu32 "] [%" PRIu32 "] MSG END\n", server->url(), client->id(), info->num);
          }
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

#ifdef ESP32
static const uint32_t deltaWS = 50;
#else
static const uint32_t deltaWS = 200;
#endif

static uint32_t lastWS = 0;
static uint32_t lastHeap = 0;

void loop() {
  uint32_t now = millis();

  if (now - lastWS >= deltaWS) {
    ws.printfAll("kp:%.4f", (10.0 / 3.0));
    lastWS = millis();
  }

  if (now - lastHeap >= 5000) {
    Serial.printf("Connected clients: %u / %u total\n", ws.count(), ws.getClients().size());

    // this can be called to also set a soft limit on the number of connected clients
    ws.cleanupClients(2);  // no more than 2 clients

    String random;
    random.reserve(8192);
    for (size_t i = 0; i < 8192; i++) {
      random += "x";
    }
    ws.textAll(random);

    // ping twice (2 control frames)
    ws.pingAll();
    ws.pingAll();

#ifdef ESP32
    Serial.printf("Uptime: %3lu s, Free heap: %" PRIu32 ", Min free heap: %" PRIu32 "\n", millis() / 1000, ESP.getFreeHeap(), ESP.getMinFreeHeap());
#endif
    lastHeap = now;
  }
}
