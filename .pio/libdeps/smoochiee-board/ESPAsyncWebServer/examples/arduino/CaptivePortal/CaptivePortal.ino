// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include <DNSServer.h>
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
#include "ESPAsyncWebServer.h"

static DNSServer dnsServer;
static AsyncWebServer server(80);

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  bool canHandle(__asyncws_unused AsyncWebServerRequest *request) const override {
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
    response->print("<p>This is our captive portal front page.</p>");
    response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
#if ASYNCWEBSERVER_WIFI_SUPPORTED
    response->printf("<p>Try opening <a href='http://%s'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
#endif
    response->print("</body></html>");
    request->send(response);
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  if (!WiFi.softAP("esp-captive")) {
    Serial.println("Soft AP creation failed.");
    while (1);
  }

  dnsServer.start(53, "*", WiFi.softAPIP());
#endif

  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);  // only when requested from AP
  // more handlers...
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
}
