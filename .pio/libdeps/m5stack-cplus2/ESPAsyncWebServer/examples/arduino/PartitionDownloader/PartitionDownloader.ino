// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// - Download ESP32 partition by name and/or type and/or subtype
// - Support encrypted and non-encrypted partitions
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

#ifndef ESP32
// this example is only for the ESP32
void setup() {}
void loop() {}
#else

#include <esp_partition.h>

static AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  LittleFS.begin(true);

  // To upload the FS partition, run:
  // > pio run -e arduino-3 -t buildfs
  // > pio run -e arduino-3 -t uploadfs
  //
  // Examples:
  //
  // - Download the partition named "spiffs": http://192.168.4.1/partition?label=spiffs
  // - Download the partition named "spiffs" with type "data": http://192.168.4.1/partition?label=spiffs&type=1
  // - Download the partition named "spiffs" with type "data" and subtype "spiffs": http://192.168.4.1/partition?label=spiffs&type=1&subtype=130
  // - Download the partition with subtype "nvs": http://192.168.4.1/partition?type=1&subtype=2
  //
  // "type" and "subtype" IDs can be found in esp_partition.h header file.
  //
  // Add "&raw=false" parameter to download the partition unencrypted (for encrypted partitions).
  // By default, the raw partition is downloaded, so if a partition is encrypted, the encrypted data will be downloaded.
  //
  // To browse a downloaded LittleFS partition, you can use https://tniessen.github.io/littlefs-disk-img-viewer/ (block size is 4096)
  //
  server.on("/partition", HTTP_GET, [](AsyncWebServerRequest *request) {
    const AsyncWebParameter *pLabel = request->getParam("label");
    const AsyncWebParameter *pType = request->getParam("type");
    const AsyncWebParameter *pSubtype = request->getParam("subtype");
    const AsyncWebParameter *pRaw = request->getParam("raw");

    if (!pLabel && !pType && !pSubtype) {
      request->send(400, "text/plain", "Bad request: missing parameter");
      return;
    }

    esp_partition_type_t type = ESP_PARTITION_TYPE_ANY;
    esp_partition_subtype_t subtype = ESP_PARTITION_SUBTYPE_ANY;
    const char *label = nullptr;
    bool raw = true;

    if (pLabel) {
      label = pLabel->value().c_str();
    }

    if (pType) {
      type = (esp_partition_type_t)pType->value().toInt();
    }

    if (pSubtype) {
      subtype = (esp_partition_subtype_t)pSubtype->value().toInt();
    }

    if (pRaw && pRaw->value() == "false") {
      raw = false;
    }

    const esp_partition_t *partition = esp_partition_find_first(type, subtype, label);

    if (!partition) {
      request->send(404, "text/plain", "Partition not found");
      return;
    }

    AsyncWebServerResponse *response =
      request->beginChunkedResponse("application/octet-stream", [partition, raw](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        const size_t remaining = partition->size - index;
        if (!remaining) {
          return 0;
        }
        const size_t len = std::min(maxLen, remaining);
        if (raw && esp_partition_read_raw(partition, index, buffer, len) == ESP_OK) {
          return len;
        }
        if (!raw && esp_partition_read(partition, index, buffer, len) == ESP_OK) {
          return len;
        }
        return 0;
      });

    response->addHeader("Content-Disposition", "attachment; filename=" + String(partition->label) + ".bin");
    response->setContentLength(partition->size);

    request->send(response);
  });

  server.begin();
}

void loop() {
  delay(100);
}

#endif
