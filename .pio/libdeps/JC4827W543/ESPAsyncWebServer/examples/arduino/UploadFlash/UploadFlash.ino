// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Demo to upload a firmware and filesystem image via multipart form data
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
#include <StreamString.h>
#include <LittleFS.h>

// ESP32 example ONLY
#ifdef ESP32
#include <Update.h>
#endif

static AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin()) {
    LittleFS.format();
    LittleFS.begin();
  }

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

// ESP32 example ONLY
#ifdef ESP32

  // Shows how to get the fw and fs  (names) and filenames from a multipart upload,
  // and also how to handle multiple file uploads in a single request.
  //
  // This example also shows how to pass and handle different parameters having the same name in query string, post form and content-disposition.
  //
  // Execute in the terminal, in order:
  //
  // 1. Build firmware: pio run -e arduino-3
  // 2. Build FS image: pio run -e arduino-3 -t buildfs
  // 3. Flash both at the same time: curl -v -F "name=Bob" -F "fw=@.pio/build/arduino-3/firmware.bin" -F "fs=@.pio/build/arduino-3/littlefs.bin" http://192.168.4.1/flash?name=Bill
  //
  server.on(
    "/flash", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (request->getResponse()) {
        // response already created
        return;
      }

      // list all parameters
      Serial.println("Request parameters:");
      const size_t params = request->params();
      for (size_t i = 0; i < params; i++) {
        const AsyncWebParameter *p = request->getParam(i);
        Serial.printf("Param[%u]: %s=%s, isPost=%d, isFile=%d, size=%u\n", i, p->name().c_str(), p->value().c_str(), p->isPost(), p->isFile(), p->size());
      }

      Serial.println("Flash / Filesystem upload completed");

      request->send(200, "text/plain", "Upload complete");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      Serial.printf("Upload[%s]: index=%u, len=%u, final=%d\n", filename.c_str(), index, len, final);

      if (request->getResponse() != nullptr) {
        // upload aborted
        return;
      }

      // start a new content-disposition upload
      if (!index) {
        // list all parameters
        const size_t params = request->params();
        for (size_t i = 0; i < params; i++) {
          const AsyncWebParameter *p = request->getParam(i);
          Serial.printf("Param[%u]: %s=%s, isPost=%d, isFile=%d, size=%u\n", i, p->name().c_str(), p->value().c_str(), p->isPost(), p->isFile(), p->size());
        }

        // get the content-disposition parameter
        const AsyncWebParameter *p = request->getParam(asyncsrv::T_name, true, true);
        if (p == nullptr) {
          request->send(400, "text/plain", "Missing content-disposition 'name' parameter");
          return;
        }

        // determine upload type based on the parameter name
        if (p->value() == "fs") {
          Serial.printf("Filesystem image upload for file: %s\n", filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
            Update.printError(Serial);
            request->send(400, "text/plain", "Update begin failed");
            return;
          }

        } else if (p->value() == "fw") {
          Serial.printf("Firmware image upload for file: %s\n", filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
            Update.printError(Serial);
            request->send(400, "text/plain", "Update begin failed");
            return;
          }

        } else {
          Serial.printf("Unknown upload type for file: %s\n", filename.c_str());
          request->send(400, "text/plain", "Unknown upload type");
          return;
        }
      }

      // some bytes to write ?
      if (len) {
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
          Update.end();
          request->send(400, "text/plain", "Update write failed");
          return;
        }
      }

      // finish the content-disposition upload
      if (final) {
        if (!Update.end(true)) {
          Update.printError(Serial);
          request->send(400, "text/plain", "Update end failed");
          return;
        }

        // success response is created in the final request handler when all uploads are completed
        Serial.printf("Upload success of file %s\n", filename.c_str());
      }
    }
  );

#endif

  server.begin();
}

// not needed
void loop() {
  delay(100);
}
