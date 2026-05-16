// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Shows how to send and receive Message Pack data
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

#if ASYNC_JSON_SUPPORT == 1
static AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/msgpack2");
#endif

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

#if ASYNC_JSON_SUPPORT == 1
  //
  // sends MessagePack using AsyncMessagePackResponse
  //
  // curl -v http://192.168.4.1/msgpack1
  //
  server.on("/msgpack1", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncMessagePackResponse *response = new AsyncMessagePackResponse();
    JsonObject root = response->getRoot().to<JsonObject>();
    root["hello"] = "world";
    response->setLength();
    request->send(response);
  });

  // Send MessagePack using AsyncResponseStream
  //
  // curl -v http://192.168.4.1/msgpack2
  //
  // Save file: curl -v http://192.168.4.1/msgpack2 -o msgpack.bin
  //
  server.on("/msgpack2", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/msgpack");
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["name"] = "Bob";
    serializeMsgPack(root, *response);
    request->send(response);
  });

  // POST file:
  //
  // curl -v -X POST -H 'Content-Type: application/msgpack' --data-binary @msgpack.bin http://192.168.4.1/msgpack2
  //
  handler->setMethod(HTTP_POST | HTTP_PUT);
  handler->onRequest([](AsyncWebServerRequest *request, JsonVariant &json) {
    Serial.printf("Body request /msgpack2 : ");  // should print: Body request /msgpack2 : {"name":"Bob"}
    serializeJson(json, Serial);
    Serial.println();
    AsyncMessagePackResponse *response = new AsyncMessagePackResponse();
    JsonObject root = response->getRoot().to<JsonObject>();
    root["hello"] = json.as<JsonObject>()["name"];
    response->setLength();
    request->send(response);
  });

  server.addHandler(handler);

  // New Json API since 3.8.2, which works for both Json and MessagePack bodies
  //
  // curl -v -X POST -H 'Content-Type: application/json' -d '{"name":"You"}' http://192.168.4.1/msgpack3
  // curl -v -X POST -H 'Content-Type: application/msgpack' --data-binary @msgpack.bin http://192.168.4.1/msgpack3
  //
  server.on("/msgpack3", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
    Serial.printf("Body request /msgpack3 : ");  // should print: Body request /msgpack3 : {"name":"Bob"}
    serializeJson(json, Serial);
    Serial.println();
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonObject root = response->getRoot().to<JsonObject>();
    root["hello"] = json.as<JsonObject>()["name"];
    response->setLength();
    request->send(response);
  });
#endif

  server.begin();
}

// not needed
void loop() {
  delay(100);
}
