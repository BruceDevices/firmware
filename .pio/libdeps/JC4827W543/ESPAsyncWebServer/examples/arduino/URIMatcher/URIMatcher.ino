// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// AsyncURIMatcher Examples - Advanced URI Matching and Routing
//
// This example demonstrates the various ways to use AsyncURIMatcher class
// for flexible URL routing with different matching strategies:
//
// 1. Exact matching
// 2. Prefix matching
// 3. Folder/directory matching
// 4. Extension matching
// 5. Case insensitive matching
// 6. Regex matching (if ASYNCWEBSERVER_REGEX is enabled)
// 7. Factory functions for common patterns
//
// Test URLs:
// - Exact: http://192.168.4.1/exact
// - Prefix: http://192.168.4.1/prefix-anything
// - Folder: http://192.168.4.1/api/users, http://192.168.4.1/api/posts
// - Extension: http://192.168.4.1/images/photo.jpg, http://192.168.4.1/docs/readme.pdf
// - Case insensitive: http://192.168.4.1/CaSe or http://192.168.4.1/case
// - Wildcard: http://192.168.4.1/wildcard-test

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
  Serial.println();
  Serial.println("=== AsyncURIMatcher Example ===");

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
#endif

  // =============================================================================
  // 1. AUTO-DETECTION BEHAVIOR - traditional string-based routing
  // =============================================================================

  // Traditional string-based routing with auto-detection
  // This uses URIMatchAuto which combines URIMatchPrefixFolder | URIMatchExact
  // It will match BOTH "/auto" exactly AND "/auto/" + anything
  server.on("/auto", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Auto-Detection Match (Traditional)");
    Serial.println("Matched URL: " + request->url());
    Serial.println("Uses auto-detection: exact + folder matching");
    request->send(200, "text/plain", "OK - Auto-detection match");
  });

  // Auto-detection for wildcard patterns (ends with *)
  // This auto-detects as URIMatchPrefix
  server.on("/wildcard*", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Auto-Detected Wildcard (Prefix)");
    Serial.println("Matched URL: " + request->url());
    Serial.println("Auto-detected as prefix match due to trailing *");
    request->send(200, "text/plain", "OK - Wildcard prefix match");
  });

  // Auto-detection for extension patterns (contains /*.ext)
  // This auto-detects as URIMatchExtension
  server.on("/auto-images/*.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Auto-Detected Extension Pattern");
    Serial.println("Matched URL: " + request->url());
    Serial.println("Auto-detected as extension match due to /*.png pattern");
    request->send(200, "text/plain", "OK - Extension match");
  });

  // =============================================================================
  // 2. EXACT MATCHING - matches only the exact URL (explicit)
  // =============================================================================

  // Using factory function for exact match
  server.on(AsyncURIMatcher::exact("/exact"), HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Factory Exact Match");
    Serial.println("Matched URL: " + request->url());
    Serial.println("Uses AsyncURIMatcher::exact() factory function");
    request->send(200, "text/plain", "OK - Factory exact match");
  });

  // =============================================================================
  // 3. PREFIX MATCHING - matches URLs that start with the pattern
  // =============================================================================

  // Using factory function for prefix match
  server.on(AsyncURIMatcher::prefix("/service"), HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Service Prefix Match (Factory)");
    Serial.println("Matched URL: " + request->url());
    Serial.println("Uses AsyncURIMatcher::prefix() factory function");
    request->send(200, "text/plain", "OK - Factory prefix match");
  });

  // =============================================================================
  // 4. FOLDER/DIRECTORY MATCHING - matches URLs in a folder structure
  // =============================================================================

  // Folder match using factory function (automatically adds trailing slash)
  server.on(AsyncURIMatcher::dir("/admin"), HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Admin Directory Match");
    Serial.println("Matched URL: " + request->url());
    Serial.println("This matches URLs under /admin/ directory");
    Serial.println("Note: /admin (without slash) will NOT match");
    request->send(200, "text/plain", "OK - Directory match");
  });

  // =============================================================================
  // 5. EXTENSION MATCHING - matches files with specific extensions
  // =============================================================================

  // Image extension matching
  server.on(AsyncURIMatcher::ext("/images/*.jpg"), HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("JPG Image Handler");
    Serial.println("Matched URL: " + request->url());
    Serial.println("This matches any .jpg file under /images/");
    request->send(200, "text/plain", "OK - Extension match");
  });

  // =============================================================================
  // 6. CASE INSENSITIVE MATCHING
  // =============================================================================

  // Case insensitive exact match
  server.on(AsyncURIMatcher::exact("/case", AsyncURIMatcher::CaseInsensitive), HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Case Insensitive Match");
    Serial.println("Matched URL: " + request->url());
    Serial.println("This matches /case in any case combination");
    request->send(200, "text/plain", "OK - Case insensitive match");
  });

#ifdef ASYNCWEBSERVER_REGEX
  // =============================================================================
  // 7. REGEX MATCHING (only available if ASYNCWEBSERVER_REGEX is enabled)
  // =============================================================================

  // Regex match for numeric IDs
  server.on(AsyncURIMatcher::regex("^/user/([0-9]+)$"), HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Regex Match - User ID");
    Serial.println("Matched URL: " + request->url());
    if (request->pathArg(0).length() > 0) {
      Serial.println("Captured User ID: " + request->pathArg(0));
    }
    Serial.println("This regex matches /user/{number} pattern");
    request->send(200, "text/plain", "OK - Regex match");
  });
#endif

  // =============================================================================
  // 8. COMBINED FLAGS EXAMPLE
  // =============================================================================

  // Combine multiple flags
  server.on(AsyncURIMatcher::prefix("/MixedCase", AsyncURIMatcher::CaseInsensitive), HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Combined Flags Example");
    Serial.println("Matched URL: " + request->url());
    Serial.println("Uses both AsyncURIMatcher::Prefix and AsyncURIMatcher::CaseInsensitive");
    request->send(200, "text/plain", "OK - Combined flags match");
  });

  // =============================================================================
  // 9. HOMEPAGE WITH NAVIGATION
  // =============================================================================

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Homepage accessed");
    String response = R"(<!DOCTYPE html>
<html>
<head>
  <title>AsyncURIMatcher Examples</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    h1 { color: #2E86AB; }
    .test-link { display: block; margin: 5px 0; padding: 8px; background: #f0f0f0; text-decoration: none; color: #333; border-radius: 4px; }
    .test-link:hover { background: #e0e0e0; }
    .section { margin: 20px 0; }
  </style>
</head>
<body>
  <h1>AsyncURIMatcher Examples</h1>

  <div class="section">
    <h3>Auto-Detection (Traditional String Matching)</h3>
    <a href="/auto" class="test-link">/auto (auto-detection: exact + folder)</a>
    <a href="/auto/sub" class="test-link">/auto/sub (folder match)</a>
    <a href="/wildcard-test" class="test-link">/wildcard-test (auto prefix)</a>
    <a href="/auto-images/photo.png" class="test-link">/auto-images/photo.png (auto extension)</a>
  </div>

  <div class="section">
    <h3>Factory Method Examples</h3>
    <a href="/exact" class="test-link">/exact (factory exact)</a>
    <a href="/service/status" class="test-link">/service/status (factory prefix)</a>
    <a href="/admin/users" class="test-link">/admin/users (factory directory)</a>
    <a href="/images/photo.jpg" class="test-link">/images/photo.jpg (factory extension)</a>
  </div>

  <div class="section">
    <h3>Case Insensitive Matching</h3>
    <a href="/case" class="test-link">/case (lowercase)</a>
    <a href="/CASE" class="test-link">/CASE (uppercase)</a>
    <a href="/CaSe" class="test-link">/CaSe (mixed case)</a>
  </div>

)";
#ifdef ASYNCWEBSERVER_REGEX
    response += R"(  <div class="section">
    <h3>Regex Matching</h3>
    <a href="/user/123" class="test-link">/user/123 (regex numeric ID)</a>
    <a href="/user/456" class="test-link">/user/456 (regex numeric ID)</a>
  </div>

)";
#endif
    response += R"(  <div class="section">
    <h3>Combined Flags</h3>
    <a href="/mixedcase-test" class="test-link">/mixedcase-test (prefix + case insensitive)</a>
    <a href="/MIXEDCASE/sub" class="test-link">/MIXEDCASE/sub (prefix + case insensitive)</a>
  </div>

</body>
</html>)";
    request->send(200, "text/html", response);
  });

  // =============================================================================
  // 10. NOT FOUND HANDLER
  // =============================================================================

  server.onNotFound([](AsyncWebServerRequest *request) {
    String html = "<h1>404 - Not Found</h1>";
    html += "<p>The requested URL <strong>" + request->url() + "</strong> was not found.</p>";
    html += "<p><a href='/'>← Back to Examples</a></p>";
    request->send(404, "text/html", html);
  });

  server.begin();

  Serial.println();
  Serial.println("=== Server Started ===");
  Serial.println("Open your browser and navigate to:");
  Serial.println("http://192.168.4.1/ - Main examples page");
  Serial.println();
  Serial.println("Available test endpoints:");
  Serial.println("• Auto-detection: /auto (exact+folder), /wildcard*, /auto-images/*.png");
  Serial.println("• Exact matches: /exact");
  Serial.println("• Prefix matches: /service*");
  Serial.println("• Folder matches: /admin/*");
  Serial.println("• Extension matches: /images/*.jpg");
  Serial.println("• Case insensitive: /case (try /CASE, /Case)");
#ifdef ASYNCWEBSERVER_REGEX
  Serial.println("• Regex matches: /user/123");
#endif
  Serial.println("• Combined flags: /mixedcase*");
  Serial.println();
}

void loop() {
  // Nothing to do here - the server handles everything asynchronously
  delay(1000);
}
