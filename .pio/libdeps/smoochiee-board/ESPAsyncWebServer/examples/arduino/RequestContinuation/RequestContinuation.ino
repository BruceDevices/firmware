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
#include <mutex>

static AsyncWebServer server(80);

// request handler that is saved from the paused request to communicate with Serial
static String message;
static AsyncWebServerRequestPtr serialRequest;

// request handler that is saved from the paused request to communicate with GPIO
static uint8_t pin = 35;
static AsyncWebServerRequestPtr gpioRequest;

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // Post a message that will be sent to the Serial console, and pause the request until the user types a key
  //
  // curl -v -X POST -H "Content-Type: application/x-www-form-urlencoded" -d "question=Name%3F%20" http://192.168.4.1/serial
  //
  // curl output should show "Answer: [y/n]" as the response
  server.on("/serial", HTTP_POST, [](AsyncWebServerRequest *request) {
    message = request->getParam("question", true)->value();
    serialRequest = request->pause();
  });

  // Wait for a GPIO to be high
  //
  // curl -v http://192.168.4.1/gpio
  //
  // curl output should show "GPIO is high!" as the response
  server.on("/gpio", HTTP_GET, [](AsyncWebServerRequest *request) {
    gpioRequest = request->pause();
  });

  pinMode(pin, INPUT);

  server.begin();
}

void loop() {
  delay(500);

  // Check for a high voltage on the RX1 pin
  if (digitalRead(pin) == HIGH) {
    if (auto request = gpioRequest.lock()) {
      request->send(200, "text/plain", "GPIO is high!");
    }
  }

  // check for an incoming message from the Serial console
  if (message.length()) {
    Serial.printf("%s", message.c_str());
    // drops buffer
    while (Serial.available()) {
      Serial.read();
    }
    Serial.setTimeout(10000);
    String response = Serial.readStringUntil('\n');  // waits for a key to be pressed
    Serial.println();
    message = asyncsrv::emptyString;
    if (auto request = serialRequest.lock()) {
      request->send(200, "text/plain", "Answer: " + response);
    }
  }
}
