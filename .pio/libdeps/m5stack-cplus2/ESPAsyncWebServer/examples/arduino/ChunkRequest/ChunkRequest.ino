// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Mitch Bradley

//
// - Test for chunked encoding in requests
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
#include <LittleFS.h>

using namespace asyncsrv;

// Tests:
//
// Upload a file with PUT
//   curl -T myfile.txt http://192.168.4.1/
//
// Upload a file with PUT using chunked encoding
//   curl -T bigfile.txt -H 'Transfer-Encoding: chunked' http://192.168.4.1/
//   ** Note: If the file will not fit in the available space, the server
//   ** does not know that in advance due to the lack of a Content-Length header.
//   ** The transfer will proceed until the filesystem fills up, then the transfer
//   ** will fail and the partial file will be deleted.  This works correctly with
//   ** recent versions (e.g. pioarduino) of the arduinoespressif32 framework, but
//   ** fails with the stale 3.20017.241212+sha.dcc1105b version due to a LittleFS
//   ** bug that has since been fixed.
//
// Immediately reject a chunked PUT that will not fit in available space
//   curl -T bigfile.txt -H 'Transfer-Encoding: chunked' -H 'X-Expected-Entity-Length: 99999999' http://192.168.4.1/
//   ** Note: MacOS WebDAVFS supplies the X-Expected-Entity-Length header with its
//   ** chunked PUTs
// Malformed chunk (triggers abort)
//   printf 'PUT /bad HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\n12345\r\nZ\r\n' | nc 192.168.4.1 80

// This struct is used with _tempObject to communicate between handleBody and a subsequent handleRequest
struct RequestState {
  File outFile;
};

void handleRequest(AsyncWebServerRequest *request) {
  Serial.print(request->methodToString());
  Serial.print(" ");
  Serial.println(request->url());

  if (request->method() != HTTP_PUT) {
    request->send(400);  // Bad Request
    return;
  }

  // If request->_tempObject is not null, handleBody already
  // did the necessary work for a PUT operation.  Otherwise,
  // handleBody was either not called, or did nothing, so we
  // handle the request later in this routine.  That happens
  // when a non-chunked PUT has Content-Length: 0.
  auto state = static_cast<RequestState *>(request->_tempObject);
  if (state) {
    // If handleBody successfully opened the file, whether or not it
    // wrote data to it, we close it here and send the "created"
    // response.  If handleBody did not open the file, because the
    // open attempt failed or because the operation was rejected,
    // state will be non-null but state->outFile will be false.  In
    // that case, handleBody has already sent an appropriate
    // response code.

    if (state->outFile) {
      // The file was already opened and written in handleBody so
      // we close it here and issue the appropriate response.
      state->outFile.close();
      request->send(201);  // Created
    }
    // The resources used by state will be automatically freed
    // when the framework frees the _tempObject pointer
    return;
  }

  String path = request->url();

  // This PUT code executes if the body was empty, which
  // can happen if the client creates a zero-length file.
  // MacOS WebDAVFS does that, then later LOCKs the file
  // and issues a subsequent PUT with body contents.

#ifdef ESP32
  File file = LittleFS.open(path, "w", true);
#else
  File file = LittleFS.open(path, "w");
#endif

  if (file) {
    file.close();
    request->send(201);  // Created
    return;
  }
  request->send(403);
}

void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (request->method() == HTTP_PUT) {
    auto state = static_cast<RequestState *>(request->_tempObject);
    if (index == 0) {
      // parse the url to a proper path
      String path = request->url();

      // Allocate the _tempObject memory
      request->_tempObject = std::malloc(sizeof(RequestState));

      // Use placement new to construct the RequestState object therein
      state = new (request->_tempObject) RequestState{File()};

      // If the client disconnects or there is a parsing error,
      // handleRequest will not be called so we need to close
      // the file.  The memory backing _tempObject will be freed
      // automatically.
      request->onDisconnect([request]() {
        Serial.println("Client disconnected");
        auto state = static_cast<RequestState *>(request->_tempObject);
        if (state) {
          if (state->outFile) {
            state->outFile.close();
          }
        }
      });

      if (total) {
#ifdef ESP32
        size_t avail = LittleFS.totalBytes() - LittleFS.usedBytes();
#else
        FSInfo info;
        LittleFS.info(info);
        auto avail = info.totalBytes - info.usedBytes;
#endif
        avail = (avail >= 4096) ? avail - 4096 : avail;  // Reserve a block for overhead
        if (total > avail) {
          Serial.printf("PUT %zu bytes will not fit in available space (%zu).\n", total, avail);
          request->send(507, "text/plain", "Too large for available storage\r\n");
          return;
        }
      }
      Serial.print("Opening ");
      Serial.print(path);
      Serial.println(" from handleBody");
#ifdef ESP32
      File file = LittleFS.open(path, "w", true);
#else
      File file = LittleFS.open(path, "w");
#endif
      if (!file) {
        request->send(500, "text/plain", "Cannot create the file");
        return;
      }
      if (file.isDirectory()) {
        file.close();
        Serial.println("Cannot PUT to a directory");
        request->send(403, "text/plain", "Cannot PUT to a directory");
        return;
      }
      // If we already returned, the File object in
      // request->_tempObject is the default-constructed one.  The
      // presence of a non-default-constructed File in state->outFile
      // indicates that the file was opened successfully and is ready
      // to receive body data.  The File will be closed later when
      // handleRequest is called after all calls to handleBody

      std::swap(state->outFile, file);
      // Now request->_tempObject contains the actual file object which owns it,
      // and default-constructed File() object is in file, which will
      // go out of scope
    }
    if (state && state->outFile) {
      Serial.printf("Writing %zu bytes at offset %zu\n", len, index);
      auto actual = state->outFile.write(data, len);
      if (actual != len) {
        Serial.println("WebDAV write failed.  Deleting file.");

        // Replace the File object in state with a null one
        File file{};
        std::swap(state->outFile, file);
        file.close();

        String path = request->url();
        LittleFS.remove(path);
        request->send(507, "text/plain", "Too large for available storage\r\n");
        return;
      }
    }
  }
}

static AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

#ifdef ESP32
  LittleFS.begin(true);
#else
  LittleFS.begin();
#endif

  server.onRequestBody(handleBody);
  server.onNotFound(handleRequest);

  server.begin();
}

void loop() {
  delay(100);
}
