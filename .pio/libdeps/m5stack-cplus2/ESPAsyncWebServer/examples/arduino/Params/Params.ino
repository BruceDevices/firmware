// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Query parameters and body parameters
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

static const char *htmlContent PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>POST Request with Multiple Parameters</title>
</head>
<body>
    <form action="http://192.168.4.1" method="POST">
        <label for="who">Who?</label>
        <input type="text" id="who" name="who" value="Carl"><br>
        <label for="g0">g0:</label>
        <input type="text" id="g0" name="g0" value="1"><br>
        <label for="a0">a0:</label>
        <input type="text" id="a0" name="a0" value="2"><br>
        <label for="n0">n0:</label>
        <input type="text" id="n0" name="n0" value="3"><br>
        <label for="t10">t10:</label>
        <input type="text" id="t10" name="t10" value="3"><br>
        <label for="t20">t20:</label>
        <input type="text" id="t20" name="t20" value="4"><br>
        <label for="t30">t30:</label>
        <input type="text" id="t30" name="t30" value="5"><br>
        <label for="t40">t40:</label>
        <input type="text" id="t40" name="t40" value="6"><br>
        <label for="t50">t50:</label>
        <input type="text" id="t50" name="t50" value="7"><br>
        <label for="g1">g1:</label>
        <input type="text" id="g1" name="g1" value="2"><br>
        <label for="a1">a1:</label>
        <input type="text" id="a1" name="a1" value="2"><br>
        <label for="n1">n1:</label>
        <input type="text" id="n1" name="n1" value="3"><br>
        <label for="t11">t11:</label>
        <input type="text" id="t11" name="t11" value="13"><br>
        <label for="t21">t21:</label>
        <input type="text" id="t21" name="t21" value="14"><br>
        <label for="t31">t31:</label>
        <input type="text" id="t31" name="t31" value="15"><br>
        <label for="t41">t41:</label>
        <input type="text" id="t41" name="t41" value="16"><br>
        <label for="t51">t51:</label>
        <input type="text" id="t51" name="t51" value="17"><br>
        <input type="submit" value="Submit">
    </form>
</body>
</html>
)";

static const size_t htmlContentLength = strlen_P(htmlContent);

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // Get query parameters
  //
  // curl -v http://192.168.4.1/?who=Bob
  //
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("who")) {
      Serial.printf("Who? %s\n", request->getParam("who")->value().c_str());
    }

    request->send(200, "text/html", (uint8_t *)htmlContent, htmlContentLength);
  });

  // Get form body parameters
  //
  // curl -v -H "Content-Type: application/x-www-form-urlencoded" -d "who=Carl" -d "param=value" http://192.168.4.1/
  //
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    // display params
    size_t count = request->params();
    for (size_t i = 0; i < count; i++) {
      const AsyncWebParameter *p = request->getParam(i);
      Serial.printf("PARAM[%u]: %s = %s\n", i, p->name().c_str(), p->value().c_str());
    }

    // get who param
    String who;
    if (request->hasParam("who", true)) {
      who = request->getParam("who", true)->value();
    } else {
      who = "No message sent";
    }
    request->send(200, "text/plain", "Hello " + who + "!");
  });

  server.begin();
}

// not needed
void loop() {
  delay(100);
}
