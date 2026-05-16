// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Test for ESPAsyncWebServer URI matching
//
// Usage: upload, connect to the AP and run test_routes.sh
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

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // Status endpoint
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Exact paths, plus the subpath (/exact matches /exact/sub but not /exact-no-match)
  server.on("/exact", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  server.on("/api/users", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Prefix matching
  server.on("/api/*", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  server.on("/files/*", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Extensions
  server.on("/*.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"status\":\"OK\"}");
  });

  server.on("/*.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/css", "/* OK */");
  });

  // =============================================================================
  // NEW ASYNCURIMATCHER FACTORY METHODS TESTS
  // =============================================================================

  // Exact match using factory method (does NOT match subpaths like traditional)
  server.on(AsyncURIMatcher::exact("/factory/exact"), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Prefix match using factory method
  server.on(AsyncURIMatcher::prefix("/factory/prefix"), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Directory match using factory method (matches /dir/anything but not /dir itself)
  server.on(AsyncURIMatcher::dir("/factory/dir"), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Extension match using factory method
  server.on(AsyncURIMatcher::ext("/factory/files/*.txt"), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // =============================================================================
  // CASE INSENSITIVE MATCHING TESTS
  // =============================================================================

  // Case insensitive exact match
  server.on(AsyncURIMatcher::exact("/case/exact", AsyncURIMatcher::CaseInsensitive), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Case insensitive prefix match
  server.on(AsyncURIMatcher::prefix("/case/prefix", AsyncURIMatcher::CaseInsensitive), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Case insensitive directory match
  server.on(AsyncURIMatcher::dir("/case/dir", AsyncURIMatcher::CaseInsensitive), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Case insensitive extension match
  server.on(AsyncURIMatcher::ext("/case/files/*.PDF", AsyncURIMatcher::CaseInsensitive), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

#ifdef ASYNCWEBSERVER_REGEX
  // Traditional regex patterns (backward compatibility)
  server.on("^/user/([0-9]+)$", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  server.on("^/blog/([0-9]{4})/([0-9]{2})/([0-9]{2})$", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // =============================================================================
  // NEW ASYNCURIMATCHER REGEX FACTORY METHODS
  // =============================================================================

  // Regex match using factory method
  server.on(AsyncURIMatcher::regex("^/factory/user/([0-9]+)$"), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Case insensitive regex match using factory method
  server.on(AsyncURIMatcher::regex("^/factory/search/(.+)$", AsyncURIMatcher::CaseInsensitive), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // Complex regex with multiple capture groups
  server.on(AsyncURIMatcher::regex("^/factory/blog/([0-9]{4})/([0-9]{2})/([0-9]{2})$"), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });
#endif

  // =============================================================================
  // SPECIAL MATCHERS
  // =============================================================================

  // Match all POST requests (catch-all before 404)
  server.on(AsyncURIMatcher::all(), HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  // 404 handler
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not Found");
  });

  server.begin();
  Serial.println("Server ready");
}

// not needed
void loop() {
  delay(100);
}
