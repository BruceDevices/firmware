// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Mitch Bradley

//
// - Test for additional WebDAV request methods
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

using namespace asyncsrv;

// Tests:
//
// Send requests with various methods
//   curl -s -X PROPFIND http://192.168.4.1/
//   curl -s -X LOCK http://192.168.4.1/
//   curl -s -X UNLOCK http://192.168.4.1/
//   curl -s -X PROPPATCH http://192.168.4.1/
//   curl -s -X MKCOL http://192.168.4.1/
//   curl -s -X MOVE http://192.168.4.1/
//   curl -s -X COPY http://192.168.4.1/
//
// In all cases, the request will be accepted with text/plain response 200 like
// "Got method PROPFIND on URL /"

static AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  server.onNotFound([](AsyncWebServerRequest *request) {
    String resp("Got method ");
    resp += request->methodToString();
    resp += " on URL ";
    resp += request->url();
    resp += "\r\n";

    Serial.print(resp);

    request->send(200, "text/plain", resp.c_str());
  });

  server.begin();
}

void loop() {
  delay(100);
}
