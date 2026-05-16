// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Shows how to wait in a chunk response for incoming data
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

static const char *htmlContent PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Sample HTML</title>
</head>
<body>
    <h1>Hello, World!</h1>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
</body>
</html>
)";

static const size_t htmlContentLength = strlen_P(htmlContent);

static AsyncWebServer server(80);
static AsyncLoggingMiddleware requestLogger;

static String triggerUART;
static int key = -1;

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // adds some internal request logging for debugging
  requestLogger.setEnabled(true);
  requestLogger.setOutput(Serial);

  server.addMiddleware(&requestLogger);

#if ASYNC_JSON_SUPPORT == 1

  //
  // HOW TO RUN THIS EXAMPLE:
  //
  // 1. Trigger a request that will be blocked for a long time:
  //    > time curl -v -X POST http://192.168.4.1/api -H "Content-Type: application/json" -d '{"input": "Please type a key to continue in Serial console..."}' --output -
  //
  // 2. While waiting, in another terminal, run some concurrent requests:
  //    > time curl -v http://192.168.4.1/
  //
  // 3. Type a key in the Serial console to continue the processing within 30 seconds.
  //    This should unblock the first request.
  //
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // need to cast to uint8_t*
    // if you do not, the const char* will be copied in a temporary String buffer
    request->send(200, "text/html", (uint8_t *)htmlContent, htmlContentLength);
  });

  server.on(
    "/api", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      // request parsing has finished
      String *data = (String *)request->_tempObject;

      if (!data) {
        request->send(400);
        return;
      }

      // no data ?
      if (!data->length()) {
        delete data;
        request->_tempObject = nullptr;
        request->send(400);
        return;
      }

      JsonDocument doc;

      // deserialize and check for errors
      if (deserializeJson(doc, *data)) {
        delete data;
        request->_tempObject = nullptr;
        request->send(400);
        return;
      }

      delete data;
      request->_tempObject = nullptr;

      // start UART com: UART will send the data to the Serial console and wait for the key press
      triggerUART = doc["input"].as<const char *>();
      key = -1;

      AsyncWebServerResponse *response = request->beginChunkedResponse("text/plain", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        // still waiting for UARY ?
        if (triggerUART.length() && key == -1) {
          return RESPONSE_TRY_AGAIN;
        }

        // finished ?
        if (!triggerUART.length() && key == -1) {
          return 0;  // 0 means we are done
        }

        // async_ws_log_d("UART answered!");

        String answer = "You typed: ";
        answer.concat((char)key);

        // note: I did not check for maxLen, but you should (see ChunkResponse.ino)
        memcpy(buffer, answer.c_str(), answer.length());

        // finish!
        triggerUART = asyncsrv::emptyString;
        key = -1;

        return answer.length();
      });

      request->send(response);
    },
    NULL,  // upload handler is not used so it should be NULL
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // async_ws_log_d("Body: index: %u, len: %u, total: %u", index, len, total);

      if (!index) {
        // async_ws_log_d("Start body parsing");
        request->_tempObject = new String();
        // cast request->_tempObject pointer to String and reserve total size
        ((String *)request->_tempObject)->reserve(total);
        // set timeout 30s
        request->client()->setRxTimeout(30);
      }

      // async_ws_log_d("Append body data");
      ((String *)request->_tempObject)->concat((const char *)data, len);
    }
  );

#endif

  server.begin();
}

void loop() {
  if (triggerUART.length() && key == -1) {
    Serial.println(triggerUART);
    // async_ws_log_d("Waiting for UART input...");
    while (!Serial.available()) {
      delay(100);
    }
    key = Serial.read();
    Serial.flush();
    // async_ws_log_d("UART input: %c", key);
    triggerUART = asyncsrv::emptyString;
  }
}
