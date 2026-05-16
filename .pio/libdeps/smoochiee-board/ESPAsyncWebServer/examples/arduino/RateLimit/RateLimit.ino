// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Show how to rate limit the server or some endpoints
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
static AsyncRateLimitMiddleware rateLimit;

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // maximum 5 requests per 10 seconds
  rateLimit.setMaxRequests(5);
  rateLimit.setWindowSize(10);

  // run quickly several times:
  //
  // curl -v http://192.168.4.1/
  //
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello, world!");
  });

  // run quickly several times:
  //
  // curl -v http://192.168.4.1/rate-limited
  //
  server
    .on(
      "/rate-limited", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hello, world!");
      }
    )
    .addMiddleware(&rateLimit);  // only rate limit this endpoint, but could be applied globally to the server

  server.begin();
}

// not needed
void loop() {
  delay(100);
}
