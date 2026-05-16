// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Show how to manipulate headers in the request / response
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

// request logger
static AsyncLoggingMiddleware requestLogger;

// filter out specific headers from the incoming request
static AsyncHeaderFilterMiddleware headerFilter;

// remove all headers from the incoming request except the ones provided in the constructor
AsyncHeaderFreeMiddleware headerFree;

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  requestLogger.setEnabled(true);
  requestLogger.setOutput(Serial);

  headerFilter.filter("X-Remove-Me");

  headerFree.keep("X-Keep-Me");
  headerFree.keep("host");

  server.addMiddlewares({&requestLogger, &headerFilter});

  // x-remove-me header will be removed
  //
  // curl -v -H "X-Header: Foo" -H "x-remove-me: value" http://192.168.4.1/remove
  //
  server.on("/remove", HTTP_GET, [](AsyncWebServerRequest *request) {
    // print all headers
    for (size_t i = 0; i < request->headers(); i++) {
      const AsyncWebHeader *h = request->getHeader(i);
      Serial.printf("Header[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }
    request->send(200, "text/plain", "Hello, world!");
  });

  // Only headers x-keep-me and host will be kept
  //
  // curl -v -H "x-keep-me: value" -H "x-remove-me: value" http://192.168.4.1/keep
  //
  server
    .on(
      "/keep", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        // print all headers
        for (size_t i = 0; i < request->headers(); i++) {
          const AsyncWebHeader *h = request->getHeader(i);
          Serial.printf("Header[%s]: %s\n", h->name().c_str(), h->value().c_str());
        }
        request->send(200, "text/plain", "Hello, world!");
      }
    )
    .addMiddleware(&headerFree);

  // curl -v http://192.168.4.1/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Hello, world!");
    response->addHeader(AsyncWebHeader::parse("X-Test-1: value1"));
    response->addHeader(AsyncWebHeader::parse("X-Test-2:value2"));
    response->addHeader(AsyncWebHeader::parse("X-Test-3:"));
    response->addHeader(AsyncWebHeader::parse("X-Test-4: "));
    response->addHeader(AsyncWebHeader::parse(""));
    response->addHeader(AsyncWebHeader::parse(":"));
    request->send(response);
    /**
< HTTP/1.1 200 OK
< connection: close
< X-Test-1: value1
< X-Test-2: value2
< X-Test-3:
< X-Test-4:
< accept-ranges: none
< content-length: 13
< content-type: text/plain
     */
  });

  server.begin();
}

// not needed
void loop() {
  delay(100);
}
