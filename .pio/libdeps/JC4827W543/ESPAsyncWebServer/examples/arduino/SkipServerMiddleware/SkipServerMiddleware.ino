// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Authentication and authorization middlewares
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

static AsyncAuthenticationMiddleware basicAuth;
static AsyncLoggingMiddleware logging;

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // basic authentication
  basicAuth.setUsername("admin");
  basicAuth.setPassword("admin");
  basicAuth.setRealm("MyApp");
  basicAuth.setAuthFailureMessage("Authentication failed");
  basicAuth.setAuthType(AsyncAuthType::AUTH_BASIC);
  basicAuth.generateHash();  // precompute hash (optional but recommended)

  // logging middleware
  logging.setEnabled(true);
  logging.setOutput(Serial);

  // we apply auth middleware to the server globally
  server.addMiddleware(&basicAuth);

  // protected endpoint: requires basic authentication
  // curl -v -u admin:admin  http://192.168.4.1/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello, world!");
  });

  // we skip all global middleware from the catchall handler
  server.catchAllHandler().skipServerMiddlewares();
  // we apply a specific middleware to the catchall handler only to log requests without a handler defined
  server.catchAllHandler().addMiddleware(&logging);

  // standard 404 handler: will display the request in the console i na curl-like style
  // curl -v -H "Foo: Bar"  http://192.168.4.1/foo
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
}

// not needed
void loop() {
  delay(100);
}
