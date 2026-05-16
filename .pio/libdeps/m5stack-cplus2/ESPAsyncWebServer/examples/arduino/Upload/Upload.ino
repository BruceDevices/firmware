// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Demo text, binary and file upload
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

  // 1. Generate a Lorem_ipsum.txt file of about 20KB of text
  //
  // 3. Run: curl -v -F "data=@Lorem_ipsum.txt" http://192.168.4.1/upload/text
  //
  server.on(
    "/upload/text", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (!request->_tempObject) {
        return request->send(400, "text/plain", "Nothing uploaded");
      }
      StreamString *buffer = reinterpret_cast<StreamString *>(request->_tempObject);
      Serial.printf("Text uploaded:\n%s\n", buffer->c_str());
      delete buffer;
      request->_tempObject = nullptr;
      request->send(200, "text/plain", "OK");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      Serial.printf("Upload[%s]: start=%u, len=%u, final=%d\n", filename.c_str(), index, len, final);

      if (!index) {
        // first pass
        StreamString *buffer = new StreamString();
        size_t size = std::max(4094l, request->header("Content-Length").toInt());
        Serial.printf("Allocating string buffer of %u bytes\n", size);
        if (!buffer->reserve(size)) {
          delete buffer;
          request->abort();
          return;
        }
        request->_tempObject = buffer;
      }

      if (len) {
        reinterpret_cast<StreamString *>(request->_tempObject)->write(data, len);
      }
    }
  );

  // 1. Generate a Lorem_ipsum.txt file of about 20KB of text
  //
  // 3. Run: curl -v -F "data=@Lorem_ipsum.txt" http://192.168.4.1/upload/file
  //
  server.on(
    "/upload/file", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (request->getResponse()) {
        // 400 File not available for writing
        return;
      }

      if (!LittleFS.exists("/my_file.txt")) {
        return request->send(400, "text/plain", "Nothing uploaded");
      }

      // sends back the uploaded file
      request->send(LittleFS, "/my_file.txt", "text/plain");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      Serial.printf("Upload[%s]: start=%u, len=%u, final=%d\n", filename.c_str(), index, len, final);

      if (!index) {
        request->_tempFile = LittleFS.open("/my_file.txt", "w");

        if (!request->_tempFile) {
          request->send(400, "text/plain", "File not available for writing");
          return;
        }
      }
      if (len) {
        request->_tempFile.write(data, len);
      }
      if (final) {
        request->_tempFile.close();
      }
    }
  );

  //
  // Upload a binary file: curl -v -F "data=@file.mp3" http://192.168.4.1/upload/binary
  //
  server.on(
    "/upload/binary", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      // response already set ?
      if (request->getResponse()) {
        // 400 No Content-Length
        return;
      }

      // nothing uploaded ?
      if (!request->_tempObject) {
        return request->send(400, "text/plain", "Nothing uploaded");
      }

      uint8_t *buffer = reinterpret_cast<uint8_t *>(request->_tempObject);
      // process the buffer

      delete buffer;
      request->_tempObject = nullptr;

      request->send(200, "text/plain", "OK");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      Serial.printf("Upload[%s]: start=%u, len=%u, final=%d\n", filename.c_str(), index, len, final);

      // first pass ?
      if (!index) {
        // Note: using content type to determine size is not reliable!
        size_t size = request->header("Content-Length").toInt();
        if (!size) {
          request->send(400, "text/plain", "No Content-Length");
        } else {
          Serial.printf("Allocating buffer of %u bytes\n", size);
          uint8_t *buffer = new (std::nothrow) uint8_t[size];
          if (!buffer) {
            // not enough memory
            request->abort();
            return;
          } else {
            request->_tempObject = buffer;
          }
        }
      }

      if (len) {
        memcpy(reinterpret_cast<uint8_t *>(request->_tempObject) + index, data, len);
      }
    }
  );

  server.begin();
}

// not needed
void loop() {
  delay(100);
}
