// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>

// Run a server at the root of the project with:
// > python3 -m http.server 3333
// Now you can open a browser and test it works by visiting http://192.168.125.122:3333/ or http://192.168.125.122:3333/README.md
#define HOST "192.168.125.122"
#define PORT 3333

// WiFi SSID to connect to
#define WIFI_SSID "IoT"

// 16 slots on esp32 (CONFIG_LWIP_MAX_ACTIVE_TCP)
#define MAX_CLIENTS CONFIG_LWIP_MAX_ACTIVE_TCP
// #define MAX_CLIENTS 1

size_t permits = MAX_CLIENTS;

void makeRequest() {
  if (!permits) {
    return;
  }

  Serial.printf("** permits: %d\n", permits);

  AsyncClient *client = new AsyncClient;

  client->onError([](void *arg, AsyncClient *client, int8_t error) {
    Serial.printf("** error occurred %s \n", client->errorToString(error));
    client->close();
    delete client;
  });

  client->onConnect([](void *arg, AsyncClient *client) {
    permits--;
    Serial.printf("** client has been connected: %" PRIu16 "\n", client->localPort());

    client->onDisconnect([](void *arg, AsyncClient *client) {
      Serial.printf("** client has been disconnected: %" PRIu16 "\n", client->localPort());
      client->close();
      delete client;

      permits++;
      makeRequest();
    });

    client->onData([](void *arg, AsyncClient *client, void *data, size_t len) {
      Serial.printf("** data received by client: %" PRIu16 ": len=%u\n", client->localPort(), len);
    });

    client->write("GET /README.md HTTP/1.1\r\nHost: " HOST "\r\nUser-Agent: ESP\r\nConnection: close\r\n\r\n");
  });

  if (client->connect(HOST, PORT)) {
  } else {
    Serial.println("** connection failed");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    continue;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("** connected to WiFi");
  Serial.println(WiFi.localIP());

  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    makeRequest();
  }
}

void loop() {
  delay(1000);
  Serial.printf("** free heap: %" PRIu32 "\n", ESP.getFreeHeap());
}
