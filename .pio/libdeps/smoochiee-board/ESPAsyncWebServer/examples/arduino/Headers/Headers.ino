// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Query and send headers
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

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  //
  // curl -v http://192.168.4.1
  //
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    //List all collected headers
    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++) {
      const AsyncWebHeader *h = request->getHeader(i);
      Serial.printf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Hello World!");

    //Add header to the response
    response->addHeader("Server", "ESP Async Web Server");

    //Add multiple headers with the same name
    response->addHeader("Set-Cookie", "sessionId=38afes7a8", false);
    response->addHeader("Set-Cookie", "id=a3fWa; Max-Age=2592000", false);
    response->addHeader("Set-Cookie", "qwerty=219ffwef9w0f; Domain=example.com", false);

    //Remove specific header
    response->removeHeader("Set-Cookie", "sessionId=38afes7a8");

    //Remove all headers with the same name
    response->removeHeader("Set-Cookie");

    request->send(response);
  });

  server.begin();
}

void loop() {
  //Sleep in the loop task to not keep the CPU busy
  delay(1000);
}
