// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Shows how to use setFilter to route requests to different handlers based on WiFi mode
//

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

  void handleRequest(AsyncWebServerRequest *request) override {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
    response->print("<p>This is out captive portal front page.</p>");
    response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
#if ASYNCWEBSERVER_WIFI_SUPPORTED
    response->printf("<p>Try opening <a href='http://%s'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
#endif
    response->print("</body></html>");
    request->send(response);
  }
};

bool hit1 = false;
bool hit2 = false;

void setup() {
  Serial.begin(115200);

  server
    .on(
      "/", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        Serial.println("Captive portal request...");
#if ASYNCWEBSERVER_WIFI_SUPPORTED
        Serial.println("WiFi.localIP(): " + WiFi.localIP().toString());
#endif
        Serial.println("request->client()->localIP(): " + request->client()->localIP().toString());
#if ESP_IDF_VERSION_MAJOR >= 5
#if ASYNCWEBSERVER_WIFI_SUPPORTED
        Serial.println("WiFi.type(): " + String((int)WiFi.localIP().type()));
#endif
        Serial.println("request->client()->type(): " + String((int)request->client()->localIP().type()));
#endif
#if ASYNCWEBSERVER_WIFI_SUPPORTED
        Serial.println(WiFi.localIP() == request->client()->localIP() ? "should be: ON_STA_FILTER" : "should be: ON_AP_FILTER");
        Serial.println(WiFi.localIP() == request->client()->localIP());
        Serial.println(WiFi.localIP().toString() == request->client()->localIP().toString());
#endif
        request->send(200, "text/plain", "This is the captive portal");
        hit1 = true;
      }
    )
    .setFilter(ON_AP_FILTER);

  server
    .on(
      "/", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        Serial.println("Website request...");
#if ASYNCWEBSERVER_WIFI_SUPPORTED
        Serial.println("WiFi.localIP(): " + WiFi.localIP().toString());
#endif
        Serial.println("request->client()->localIP(): " + request->client()->localIP().toString());
#if ESP_IDF_VERSION_MAJOR >= 5
#if ASYNCWEBSERVER_WIFI_SUPPORTED
        Serial.println("WiFi.type(): " + String((int)WiFi.localIP().type()));
#endif
        Serial.println("request->client()->type(): " + String((int)request->client()->localIP().type()));
#endif
#if ASYNCWEBSERVER_WIFI_SUPPORTED
        Serial.println(WiFi.localIP() == request->client()->localIP() ? "should be: ON_STA_FILTER" : "should be: ON_AP_FILTER");
        Serial.println(WiFi.localIP() == request->client()->localIP());
        Serial.println(WiFi.localIP().toString() == request->client()->localIP().toString());
#endif
        request->send(200, "text/plain", "This is the website");
        hit2 = true;
      }
    )
    .setFilter(ON_STA_FILTER);

  // assert(WiFi.softAP("esp-captive-portal"));
  // dnsServer.start(53, "*", WiFi.softAPIP());
  // server.begin();
  // Serial.println("Captive portal started!");

  // while (!hit1) {
  //   dnsServer.processNextRequest();
  //   yield();
  // }
  // delay(1000); // Wait for the client to process the response

  // Serial.println("Captive portal opened, stopping it and connecting to WiFi...");
  // dnsServer.stop();
  // WiFi.softAPdisconnect();

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.persistent(false);
  WiFi.begin("IoT");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected to WiFi with IP address: " + WiFi.localIP().toString());
#endif

  server.begin();

  // while (!hit2) {
  //   delay(10);
  // }
  // delay(1000); // Wait for the client to process the response
  // ESP.restart();
}

void loop() {
  delay(100);
}
