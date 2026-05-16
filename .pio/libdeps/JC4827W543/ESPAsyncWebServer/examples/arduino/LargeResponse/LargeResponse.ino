// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Example to send a large response and control the filling of the buffer.
//
// This is also a MRE for:
// - https://github.com/ESP32Async/ESPAsyncWebServer/issues/242
// - https://github.com/ESP32Async/ESPAsyncWebServer/issues/315
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

static const size_t totalResponseSize = 16 * 1000;  // 16 KB
static char fillChar = 'A';

class CustomResponse : public AsyncAbstractResponse {
public:
  explicit CustomResponse() {
    _code = 200;
    _contentType = "text/plain";
    _sendContentLength = false;
  }

  bool _sourceValid() const override {
    return true;
  }

  size_t _fillBuffer(uint8_t *buf, size_t buflen) override {
    if (_sent == RESPONSE_TRY_AGAIN) {
      Serial.println("Simulating temporary unavailability of data...");
      _sent = 0;
      return RESPONSE_TRY_AGAIN;
    }
    size_t remaining = totalResponseSize - _sent;
    if (remaining == 0) {
      return 0;
    }
    if (buflen > remaining) {
      buflen = remaining;
    }
    Serial.printf("Filling '%c' @ sent: %u, buflen: %u\n", fillChar, _sent, buflen);
    std::fill_n(buf, buflen, static_cast<uint8_t>(fillChar));
    _sent += buflen;
    fillChar = (fillChar == 'Z') ? 'A' : fillChar + 1;
    return buflen;
  }

private:
  char fillChar = 'A';
  size_t _sent = 0;
};

// Code to reproduce issues:
// - https://github.com/ESP32Async/ESPAsyncWebServer/issues/242
// - https://github.com/ESP32Async/ESPAsyncWebServer/issues/315
//
// https://github.com/ESP32Async/ESPAsyncWebServer/pull/317#issuecomment-3421141039
//
// I cracked it.
// So this is how it works:
// That space that _tcp is writing to identified by CONFIG_TCP_SND_BUF_DEFAULT (and is value-matching with default TCP windows size which is very confusing itself).
// The space returned by client()->write() and client->space() somehow might not be atomically/thread synced (had not dived that deep yet). So if first call to _fillBuffer is done via user-code thread and ended up with some small amount of data consumed and second one is done by _poll or _ack? returns full size again! This is where old code fails.
// If you change your class this way it will fail 100%.
class CustomResponseMRE : public AsyncAbstractResponse {
public:
  explicit CustomResponseMRE() {
    _code = 200;
    _contentType = "text/plain";
    _sendContentLength = false;
    // add some useless headers
    addHeader("Clear-Site-Data", "Clears browsing data (e.g., cookies, storage, cache) associated with the requesting website.");
    addHeader(
      "No-Vary-Search", "Specifies a set of rules that define how a URL's query parameters will affect cache matching. These rules dictate whether the same "
                        "URL with different URL parameters should be saved as separate browser cache entries"
    );
  }

  bool _sourceValid() const override {
    return true;
  }

  size_t _fillBuffer(uint8_t *buf, size_t buflen) override {
    if (fillChar == NULL) {
      fillChar = 'A';
      return RESPONSE_TRY_AGAIN;
    }
    if (_sent == RESPONSE_TRY_AGAIN) {
      Serial.println("Simulating temporary unavailability of data...");
      _sent = 0;
      return RESPONSE_TRY_AGAIN;
    }
    size_t remaining = totalResponseSize - _sent;
    if (remaining == 0) {
      return 0;
    }
    if (buflen > remaining) {
      buflen = remaining;
    }
    Serial.printf("Filling '%c' @ sent: %u, buflen: %u\n", fillChar, _sent, buflen);
    std::fill_n(buf, buflen, static_cast<uint8_t>(fillChar));
    _sent += buflen;
    fillChar = (fillChar == 'Z') ? 'A' : fillChar + 1;
    return buflen;
  }

private:
  char fillChar = NULL;
  size_t _sent = 0;
};

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // Example to use a AwsResponseFiller
  //
  // curl -v http://192.168.4.1/1 | grep -o '.' | sort | uniq -c
  //
  // Should output 16000 and a distribution of letters which is the same in ESP32 logs and console
  //
  server.on("/1", HTTP_GET, [](AsyncWebServerRequest *request) {
    fillChar = 'A';
    AsyncWebServerResponse *response = request->beginResponse("text/plain", totalResponseSize, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      size_t remaining = totalResponseSize - index;
      size_t toSend = (remaining < maxLen) ? remaining : maxLen;
      Serial.printf("Filling '%c' @ index: %u, maxLen: %u, toSend: %u\n", fillChar, index, maxLen, toSend);
      std::fill_n(buffer, toSend, static_cast<uint8_t>(fillChar));
      fillChar = (fillChar == 'Z') ? 'A' : fillChar + 1;
      return toSend;
    });
    request->send(response);
  });

  // Example to use a AsyncAbstractResponse
  //
  // curl -v http://192.168.4.1/2 | grep -o '.' | sort | uniq -c
  //
  // Should output 16000 and a distribution of letters which is the same in ESP32 logs and console
  //
  server.on("/2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(new CustomResponse());
  });

  // Example to use a AsyncAbstractResponse
  //
  // curl -v http://192.168.4.1/3 | grep -o '.' | sort | uniq -c
  //
  // Should output 16000 and a distribution of letters which is the same in ESP32 logs and console
  //
  server.on("/3", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(new CustomResponseMRE());
  });

  server.begin();
}

void loop() {
  delay(100);
}
