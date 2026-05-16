// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Shows how to use request continuation to pause a request for a long processing task, and be able to resume it later.
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

#include <list>
#include <memory>
#include <mutex>

static AsyncWebServer server(80);

// ===============================================================
// The code below is used to simulate some long running operations
// ===============================================================

typedef struct {
  size_t id;
  AsyncWebServerRequestPtr requestPtr;
  uint8_t data;
} LongRunningOperation;

static std::list<std::unique_ptr<LongRunningOperation>> longRunningOperations;
static size_t longRunningOperationsCount = 0;
#ifdef ESP32
static std::mutex longRunningOperationsMutex;
#endif

static void startLongRunningOperation(AsyncWebServerRequestPtr &&requestPtr) {
#ifdef ESP32
  std::lock_guard<std::mutex> lock(longRunningOperationsMutex);
#endif

  // LongRunningOperation *op = new LongRunningOperation();
  std::unique_ptr<LongRunningOperation> op(new LongRunningOperation());
  op->id = ++longRunningOperationsCount;
  op->data = 10;

  // you need to hold the AsyncWebServerRequestPtr returned by pause();
  // This object is authorized to leave the scope of the request handler.
  op->requestPtr = std::move(requestPtr);

  Serial.printf("[%u] Start long running operation for %" PRIu8 " seconds...\n", op->id, op->data);
  longRunningOperations.push_back(std::move(op));
}

static bool processLongRunningOperation(LongRunningOperation *op) {
  // request was deleted ?
  if (op->requestPtr.expired()) {
    Serial.printf("[%u] Request was deleted - stopping long running operation\n", op->id);
    return true;  // operation finished
  }

  // processing the operation
  Serial.printf("[%u] Long running operation processing... %" PRIu8 " seconds left\n", op->id, op->data);

  // check if we have finished ?
  op->data--;
  if (op->data) {
    // not finished yet
    return false;
  }

  // Try to get access to the request pointer if it is still exist.
  // If there has been a disconnection during that time, the pointer won't be valid anymore
  if (auto request = op->requestPtr.lock()) {
    Serial.printf("[%u] Long running operation finished! Sending back response...\n", op->id);
    request->send(200, "text/plain", String(op->id) + " ");

  } else {
    Serial.printf("[%u] Long running operation finished, but request was deleted!\n", op->id);
  }

  return true;  // operation finished
}

/// ==========================================================

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // Add a middleware to see how pausing a request affects the middleware chain
  server.addMiddleware([](AsyncWebServerRequest *request, ArMiddlewareNext next) {
    Serial.printf("Middleware chain start\n");

    // continue to the next middleware, and at the end the request handler
    next();

    // we can check the request pause state after the handler was executed
    if (request->isPaused()) {
      Serial.printf("Request was paused!\n");
    }

    Serial.printf("Middleware chain ends\n");
  });

  // HOW TO RUN THIS EXAMPLE:
  //
  // 1. Open several terminals to trigger some requests concurrently that will be paused with:
  //    > time curl -v http://192.168.4.1/
  //
  // 2. Look at the output of the Serial console to see how the middleware chain is executed
  //    and to see the long running operations being processed and resume the requests.
  //
  // 3. You can try close your curl command to cancel the request and check that the request is deleted.
  //    Note: in case the network is disconnected, the request will be deleted.
  //
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Print a message in case the request is disconnected (network disconnection, client close, etc.)
    request->onDisconnect([]() {
      Serial.printf("Request was disconnected!\n");
    });

    // Instruct ESPAsyncWebServer to pause the request and get a AsyncWebServerRequestPtr to be able to access the request later.
    // The AsyncWebServerRequestPtr is the ONLY object authorized to leave the scope of the request handler.
    // The Middleware chain will continue to run until the end after this handler exit, but the request will be paused and will not
    // be sent to the client until send() is called later.
    Serial.printf("Pausing request...\n");
    AsyncWebServerRequestPtr requestPtr = request->pause();

    // start our long operation...
    startLongRunningOperation(std::move(requestPtr));
  });

  server.begin();
}

static uint32_t lastTime = 0;

void loop() {
  if (millis() - lastTime >= 1000) {

#ifdef ESP32
    Serial.printf("Free heap: %" PRIu32 "\n", ESP.getFreeHeap());
    std::lock_guard<std::mutex> lock(longRunningOperationsMutex);
#endif

    // process all long running operations
    longRunningOperations.remove_if([](const std::unique_ptr<LongRunningOperation> &op) {
      return processLongRunningOperation(op.get());
    });

    lastTime = millis();
  }
}
