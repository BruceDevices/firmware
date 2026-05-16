// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include <Arduino.h>
#include <AsyncTCP.h>
#include <StreamString.h>
#include <WiFi.h>

#include <functional>
#include <string>

#define WIFI_SSID     "IoT"
#define WIFI_PASSWORD ""

void fetchAsync(const char *host, std::function<void(const StreamString *)> onDone) {
  Serial.printf("[%s] Fetching: http://%s...\n", host, host);

  // buffer where we will accumulate the received data
  StreamString *content = new StreamString();

  // reserve enough space to avoid reallocations
  content->reserve(32 * 1024);

  // create a new client
  AsyncClient *client = new AsyncClient();

  // register a callback when the client disconnects
  client->onDisconnect([content, host, onDone](void *arg, AsyncClient *client) {
    Serial.printf("[%s] Disconnected.\n", host);
    onDone(content);
    delete client;
    delete content;
  });

  // register a callback when an error occurs
  client->onError([host, onDone](void *arg, AsyncClient *client, int8_t error) {
    Serial.printf("[%s] Error: %s\n", host, client->errorToString(error));
  });

  // register a callback when data arrives, to accumulate it
  client->onData([host, content](void *arg, AsyncClient *client, void *data, size_t len) {
    Serial.printf("[%s] Received %u bytes...\n", host, len);
    content->write((const uint8_t *)data, len);
  });

  // register a callback when we are connected
  client->onConnect([host](void *arg, AsyncClient *client) {
    Serial.printf("[%s] Connected!\n", host);

    // send request
    client->write("GET / HTTP/1.1\r\n");
    client->write("Host: ");
    client->write(host);
    client->write("\r\n");
    client->write("User-Agent: ESP32\r\n");
    client->write("Connection: close\r\n");
    client->write("\r\n");
  });

  Serial.printf("[%s] Connecting...\n", host);

  if (!client->connect(host, 80)) {
    Serial.printf("[%s] Failed to connect!\n", host);
    delete client;
    delete content;
    onDone(nullptr);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    continue;
  }

  // connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected to WiFi!");
  Serial.println(WiFi.localIP());

  // fetch asynchronously 2 websites:

  // equivalent to curl -v --raw http://www.time.org/
  fetchAsync("www.time.org", [](const StreamString *content) {
    if (content) {
      Serial.printf("[www.time.org] Fetched website:\n%s\n", content->c_str());
    } else {
      Serial.println("[www.time.org] Failed to fetch website!");
    }
  });

  // equivalent to curl -v --raw http://www.google.com/
  fetchAsync("www.google.com", [](const StreamString *content) {
    if (content) {
      Serial.printf("[www.google.com] Fetched website:\n%s\n", content->c_str());
    } else {
      Serial.println("[www.google.com] Failed to fetch website!");
    }
  });
}

void loop() {
  delay(100);
}
