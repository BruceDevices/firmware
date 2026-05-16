// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// HTTP Method usage example and check compatibility with Arduino HTTP Methods when HTTP_Method.h is included.
// ESP-IDf enums are imported, and HTTP_ANY is defined by Arduino Core.
// In that case, we cannot use directly asyncws enums: we have to namespace them.
// Also, asycnws HTTP_ANY is not available sine already defined by Arduino as a macro.
// So we have to use AsyncWebRequestMethod::HTTP_ALL instead of HTTP_ANY in that case.
//

#include <Arduino.h>

#if !defined(ESP8266)
#include <HTTP_Method.h>
#endif

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

#if ASYNC_JSON_SUPPORT == 1
// https://github.com/ESP32Async/ESPAsyncWebServer/issues/404
static void handlePostTest(AsyncWebServerRequest *req, JsonVariant &json) {
  AsyncWebServerResponse *response;
  if (req->method() == WebRequestMethod::HTTP_POST) {
    response = req->beginResponse(200, "application/json", "{\"msg\": \"OK\"}");
  } else {
    response = req->beginResponse(501, "application/json", "{\"msg\": \"Not Implemented\"}");
  }
  req->send(response);
}
#endif

// user defined functions that turns out to have similar signatures to the ones in global header
int stringToMethod(const String &) {
  return 0;
}
const char *methodToString(WebRequestMethod) {
  return "METHOD";
}
bool methodMatches(WebRequestMethodComposite c, WebRequestMethod m) {
  return false;
};

static WebRequestMethodComposite allowed = AsyncWebRequestMethod::HTTP_HEAD | AsyncWebRequestMethod::HTTP_OPTIONS;

class MyRequestHandler : public AsyncWebHandler {
public:
  bool canHandle(__asyncws_unused AsyncWebServerRequest *request) const override {
    // Test backward compatibility with previous way of checking if a method is allowed in a composite using a bit operator
    return allowed & request->method();
  }

  void handleRequest(AsyncWebServerRequest *request) override {
    request->send(200, "text/plain", "Hello from custom handler");
  }
};

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // curl -v http://192.168.4.1/get-or-post
  // curl -v -X POST -d "a=b" http://192.168.4.1/get-or-post
  server.on("/get-or-post", AsyncWebRequestMethod::HTTP_GET | AsyncWebRequestMethod::HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello");
  });

  // curl -v http://192.168.4.1/all
  server.on("/all", AsyncWebRequestMethod::HTTP_ALL, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello");
  });

#if ASYNC_JSON_SUPPORT == 1
  // curl -v http://192.168.4.1/test => Not Implemented
  // curl -v -X POST -H 'Content-Type: application/json' -d '{"name":"You"}' http://192.168.4.1/test => OK
  AsyncCallbackJsonWebHandler *testHandler = new AsyncCallbackJsonWebHandler("/test", handlePostTest);
  server.addHandler(testHandler);
#endif

  // curl -v -X HEAD http://192.168.4.1/custom => answers
  // curl -v -X OPTIONS http://192.168.4.1/custom => answers
  // curl -v -X POST http://192.168.4.1/custom => no answer
  server.addHandler(new MyRequestHandler());

  server.begin();

  WebRequestMethodComposite composite1 = AsyncWebRequestMethod::HTTP_GET | AsyncWebRequestMethod::HTTP_POST;
  WebRequestMethodComposite composite2 = AsyncWebRequestMethod::HTTP_GET | AsyncWebRequestMethod::HTTP_POST;
  WebRequestMethodComposite composite3 = AsyncWebRequestMethod::HTTP_HEAD | AsyncWebRequestMethod::HTTP_OPTIONS;
  WebRequestMethodComposite composite4 = composite3;
  WebRequestMethodComposite composite5 = AsyncWebRequestMethod::HTTP_GET;

  assert(composite1.matches(AsyncWebRequestMethod::HTTP_GET));
  assert(composite1.matches(AsyncWebRequestMethod::HTTP_POST));
  assert(!composite1.matches(AsyncWebRequestMethod::HTTP_HEAD));
  assert(!composite3.matches(AsyncWebRequestMethod::HTTP_GET));

  assert(composite1 == composite2);
  assert(composite3 == composite4);
  assert(composite1 != composite3);
  assert(composite5 == AsyncWebRequestMethod::HTTP_GET);
}

// not needed
void loop() {
  delay(100);
}
