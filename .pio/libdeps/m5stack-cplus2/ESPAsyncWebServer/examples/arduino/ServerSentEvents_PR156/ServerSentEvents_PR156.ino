// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// SSE example
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
  <title>Server-Sent Events</title>
  <script>
    if (!!window.EventSource) {
      var source = new EventSource('/events');
      source.onopen = function(e) {
        console.log("Events Connected");
      };
      source.onerror = function(e) {
        if (e.target.readyState != EventSource.OPEN) {
          console.log("Events Disconnected");
        }
        // Uncomment below to prevent the client from proactively establishing a new connection.
        // source.close();
      };
      source.onmessage = function(e) {
        console.log("Message: " + e.data);
      };
      source.addEventListener('heartbeat', function(e) {
        console.log("Heartbeat", e.data);
      }, false);
    }
  </script>
</head>
<body>
  <h1>Open your browser console!</h1>
</body>
</html>
)";

static const size_t htmlContentLength = strlen_P(htmlContent);

static AsyncWebServer server(80);
static AsyncEventSource events("/events");

static volatile size_t connectionCount = 0;
static volatile uint32_t timestampConnected = 0;
static constexpr uint32_t timeoutClose = 15000;

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // curl -v http://192.168.4.1/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // need to cast to uint8_t*
    // if you do not, the const char* will be copied in a temporary String buffer
    request->send(200, "text/html", (uint8_t *)htmlContent, htmlContentLength);
  });

  events.onConnect([](AsyncEventSourceClient *client) {
    /**
     * @brief: Purpose for a test case: count() function
     * Task watchdog shall be triggered due to a self-deadlock by mutex handling of the AsyncEventSource.
     *
     * E (61642) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
     * E (61642) task_wdt:  - async_tcp (CPU 0/1)
     *
     * Resolve: using recursive_mutex insteads of mutex.
    */
    connectionCount = events.count();

    timestampConnected = millis();
    Serial.printf("SSE Client connected! ID: %" PRIu32 "\n", client->lastId());
    client->send("hello!", NULL, millis(), 1000);
    Serial.printf("Number of connected clients: %u\n", connectionCount);
  });

  events.onDisconnect([](AsyncEventSourceClient *client) {
    connectionCount = events.count();
    Serial.printf("SSE Client disconnected! ID: %" PRIu32 "\n", client->lastId());
    Serial.printf("Number of connected clients: %u\n", connectionCount);
  });

  server.addHandler(&events);

  server.begin();
}

static constexpr uint32_t deltaSSE = 3000;
static uint32_t lastSSE = 0;
static uint32_t lastHeap = 0;

void loop() {
  uint32_t now = millis();
  if (connectionCount > 0) {
    if (now - lastSSE >= deltaSSE) {
      events.send(String("ping-") + now, "heartbeat", now);
      lastSSE = millis();
    }

    /**
     * @brief: Purpose for a test case: close() function
     * Task watchdog shall be triggered due to a self-deadlock by mutex handling of the AsyncEventSource.
     *
     * E (61642) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
     * E (61642) task_wdt:  - async_tcp (CPU 0/1)
     *
     * Resolve: using recursive_mutex insteads of mutex.
    */
    if (now - timestampConnected >= timeoutClose) {
      Serial.printf("SSE Clients close\n");
      events.close();
    }
  }

#ifdef ESP32
  if (now - lastHeap >= 2000) {
    Serial.printf("Free heap: %" PRIu32 "\n", ESP.getFreeHeap());
    lastHeap = now;
  }
#endif
}
