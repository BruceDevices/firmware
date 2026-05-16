// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

/*
  This example demonstrates how to send data to a remote server asynchronously.
  Run on the remote computer: nc -l -p 1234

  You should see in the logs:

Connected!
Will send 5760 bytes...
Acked 1436 bytes in 19 ms
Will send 1436 bytes...
Acked 1436 bytes in 2 ms
Will send 996 bytes...
Waiting for acks...
Acked 1436 bytes in 1 ms
Acked 1436 bytes in 5 ms
Acked 1452 bytes in 17 ms
Acked 996 bytes in 28 ms
Buffer received - next send in 2 sec
Will send 5760 bytes...
Acked 1436 bytes in 14 ms
Will send 1436 bytes...
Acked 1436 bytes in 2 ms
Acked 1436 bytes in 0 ms
Acked 1452 bytes in 1 ms
Will send 996 bytes...
Waiting for acks...
Acked 1436 bytes in 3 ms
Acked 996 bytes in 18 ms
Buffer received - next send in 2 sec

  And in the remote terminal 3072 characters sent [.........  ...........] and so on.
*/

#include <Arduino.h>
#include <AsyncTCP.h>
#include <StreamString.h>
#include <WiFi.h>
#include <assert.h>

#include <functional>
#include <string>

#define WIFI_SSID     "IoT"
#define WIFI_PASSWORD ""

#define REMOTE_IP   "192.168.125.116"
#define REMOTE_PORT 1234

#define BUFFER_SIZE 8 * 1024

static char buffer[BUFFER_SIZE] = {0};
static size_t bufferPos = 0;

// 0 == disconnected
// 1 == connecting
// 2 == connected
static uint8_t state = 0;

// number of bytes waiting for a ack
static size_t waitingAck = 0;

static AsyncClient client;

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

  // fill buffer
  buffer[0] = '[';
  for (size_t i = 1; i < BUFFER_SIZE - 1; i++) {
    buffer[i] = '.';
  }
  buffer[BUFFER_SIZE - 1] = ']';

  // register a callback when the client disconnects
  client.onDisconnect([](void *arg, AsyncClient *client) {
    Serial.printf("Disconnected.\n");
    state = 0;
  });

  // register a callback when an error occurs
  client.onError([](void *arg, AsyncClient *client, int8_t error) {
    Serial.printf("Error: %s\n", client->errorToString(error));
  });

  // register a callback when data arrives, to accumulate it
  client.onData([](void *arg, AsyncClient *client, void *data, size_t len) {
    Serial.printf("Received %u bytes...\n", len);
    Serial.write((uint8_t *)data, len);
  });

  // register a callback when we are connected
  client.onConnect([](void *arg, AsyncClient *client) {
    Serial.printf("Connected!\n");
    state = 2;
  });

  client.onAck([](void *arg, AsyncClient *client, size_t len, uint32_t time) {
    Serial.printf("Acked %u bytes in %" PRIu32 " ms\n", len, time);
    assert(waitingAck >= len);
    waitingAck -= len;
  });

  client.setRxTimeout(20000);
  client.setNoDelay(true);
}

void loop() {
  switch (state) {
    case 0:
    {
      Serial.printf("Connecting...\n");
      if (!client.connect(REMOTE_IP, REMOTE_PORT)) {
        Serial.printf("Failed to connect!\n");
        delay(1000);  // to not flood logs
      } else {
        state = 1;
      }
      break;
    }

    case 1:
    {
      Serial.printf("Still connecting...\n");
      delay(500);  // to not flood logs
      break;
    }

    case 2:
    {
      // fill PCB space until we can
      size_t willSend;
      while (bufferPos < BUFFER_SIZE && (willSend = client.write(buffer + bufferPos, BUFFER_SIZE - bufferPos))) {
        Serial.printf("Will send %u bytes...\n", willSend);
        bufferPos += willSend;
        waitingAck += willSend;
      }

      // we have sent the whole buffer ?
      if (bufferPos >= BUFFER_SIZE) {
        // wait for acks, or send again after 2 sec
        if (waitingAck) {
          Serial.printf("Waiting for acks...\n");
          delay(100);
        } else {
          Serial.printf("Buffer received - next send in 2 sec\n");
          delay(2000);
          bufferPos = 0;
        }
      }
      break;
    }

    default: break;
  }
}
