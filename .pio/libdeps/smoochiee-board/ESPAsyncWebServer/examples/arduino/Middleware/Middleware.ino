// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Show how to sue Middleware
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

// New middleware classes can be created!
class MyMiddleware : public AsyncMiddleware {
public:
  void run(AsyncWebServerRequest *request, ArMiddlewareNext next) override {
    Serial.printf("Before handler: %s %s\n", request->methodToString(), request->url().c_str());
    next();  // continue middleware chain
    Serial.printf("After handler: response code=%d\n", request->getResponse()->code());
  }
};

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // add a global middleware to the server
  server.addMiddleware(new MyMiddleware());

  // Test with:
  //
  // - curl -v http://192.168.4.1/            => 200 OK
  // - curl -v http://192.168.4.1/?user=anon  => 403 Forbidden
  // - curl -v http://192.168.4.1/?user=foo   => 200 OK
  // - curl -v http://192.168.4.1/?user=error => 400 ERROR
  //
  AsyncCallbackWebHandler &handler = server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("In Handler: %s %s\n", request->methodToString(), request->url().c_str());
    request->send(200, "text/plain", "Hello, world!");
  });

  // add a middleware to this handler only to send 403 if the user is anon
  handler.addMiddleware([](AsyncWebServerRequest *request, ArMiddlewareNext next) {
    Serial.println("Checking user=anon");
    if (request->hasParam("user") && request->getParam("user")->value() == "anon") {
      request->send(403, "text/plain", "Forbidden");
    } else {
      next();
    }
  });

  // add a middleware to this handler that will replace the previously created response by another one
  handler.addMiddleware([](AsyncWebServerRequest *request, ArMiddlewareNext next) {
    next();
    Serial.println("Checking user=error");
    if (request->hasParam("user") && request->getParam("user")->value() == "error") {
      request->send(400, "text/plain", "ERROR");
    }
  });

  server.begin();
}

// not needed
void loop() {
  delay(100);
}
