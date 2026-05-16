// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

/*
  This example demonstrates how to create a TCP Chargen server with the
  AsyncTCP library. Run on the remote computer:

    $ nc <IPAddressforESP32> 19

  it shows a continuous stream of characters like this:

  #$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghij
  $%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijk
  %&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijkl
  &'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklm
  '()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmn
  ()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmno
  )*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnop
  *+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopq
  +,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqr

  If the pattern shows broken your ESP32 is probably too busy to serve the
  data or the network is congested.

*/

#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>

// The default TCP Chargen port number is 19, see RFC 864 (Character Generator Protocol)
#define CHARGEN_PORT 19
const size_t LINE_LENGTH = 72;

// Full pattern of printable ASCII characters (95 characters)
const char CHARGEN_PATTERN_FULL[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
const size_t PATTERN_LENGTH_FULL = 95;

#define WIFI_SSID     "YourSSID"
#define WIFI_PASSWORD "YourPassword"

// This is the main asynchronous server object
AsyncServer *AsyncServerChargen = nullptr;
// This is the pointer to the single connected client
AsyncClient *AsyncClientChargen = nullptr;
// Tracks the current position in the pattern rotation for the Chargen protocol.
size_t startIndex = 0;

void makeAndSendLine();  // Forward declaration

// --- Callback Functions ---

// Called when the client acknowledges receiving data. We use this to send more.
void handleClientAck(void *arg, AsyncClient *client, size_t len, uint32_t time) {
  if (!client->disconnected() && client->space() >= (LINE_LENGTH + 2)) {
    makeAndSendLine();
  }
}

// Called periodically while the client is connected.
void handleClientPoll(void *arg, AsyncClient *client) {
  // We can reuse the same logic as the ACK handler.
  // Just try to send more data if there's space.
  handleClientAck(arg, client, 0, 0);
}

// It handles errors that are not normal disconnections.
void handleClientError(void *arg, AsyncClient *client, int error) {
  // The error codes are defined in esp_err.h
  Serial.printf("Client error! Code: %d, Message: %s\n", error, client->errorToString(error));

  // If the client is the one we have stored, clean it up.
  if (AsyncClientChargen == client) {
    Serial.println("Cleaning up global client pointer due to error.");
    AsyncClientChargen = nullptr;
  }
  // We do not need to call "delete client" here because onDisconnect will do it.
  // If the error is critical, we will do it.
  if (client->connected()) {
    client->close();
  }
}

// Called when the client disconnects.
void handleClientDisconnect(void *arg, AsyncClient *client) {
  Serial.println("Client disconnected.");
  // Set the global client pointer to null to allow a new client to connect.
  if (AsyncClientChargen == client) {
    AsyncClientChargen = nullptr;
  }
  delete client;
}

// Called when a new client tries to connect.
void handleClient(void *arg, AsyncClient *client) {
  // If there is already a client connected, reject the new one.
  if (AsyncClientChargen) {
    Serial.printf("New connection from %s rejected. Server is busy.\n", client->remoteIP().toString().c_str());
    client->close();
    return;
  }

  // Accept the new client.
  Serial.printf("New client connected from %s\n", client->remoteIP().toString().c_str());
  AsyncClientChargen = client;
  startIndex = 0;  // Reset pattern for the new client.

  // Called when previously sent data is acknowledged by the client.
  // This is the core engine for continuous data transmission (Chargen).
  AsyncClientChargen->onAck(handleClientAck, nullptr);

  // Called periodically by the AsyncTCP task.
  // Serves as a backup to resume transmission if the buffer was full and the ACK wasn't received.
  AsyncClientChargen->onPoll(handleClientPoll, nullptr);

  // Called when a communication error (e.g., protocol failure or timeout) occurs.
  // Essential for cleaning up the global client pointer and preventing resource leaks.
  AsyncClientChargen->onError(handleClientError, nullptr);

  // Called when the client actively closes the connection or if a fatal error occurs.
  // Responsible for resetting the global client pointer and freeing memory.
  AsyncClientChargen->onDisconnect(handleClientDisconnect, nullptr);

  // Start sending data immediately.
  makeAndSendLine();
}

void makeAndSendLine() {
  // Check if the client is valid and has enough space in its send buffer.
  if (AsyncClientChargen && AsyncClientChargen->canSend() && AsyncClientChargen->space() >= (LINE_LENGTH + 2)) {
    // Buffer for the line (72 characters + \r\n)
    char lineBuffer[LINE_LENGTH + 2];

    // 1. Construct the 72-character line using the rotating pattern.
    for (size_t i = 0; i < LINE_LENGTH; i++) {
      lineBuffer[i] = CHARGEN_PATTERN_FULL[(startIndex + i) % PATTERN_LENGTH_FULL];
    }

    // 2. Add the standard CHARGEN line terminator (\r\n).
    lineBuffer[LINE_LENGTH] = '\r';
    lineBuffer[LINE_LENGTH + 1] = '\n';

    // 3. Write data to the socket.
    AsyncClientChargen->write(lineBuffer, LINE_LENGTH + 2);

    // 4. Advance the starting index for the next line (rotation).
    startIndex = (startIndex + 1) % PATTERN_LENGTH_FULL;
  }
}

// ---------------------- SETUP & LOOP ----------------------

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    continue;
  }
  // Connecting to WiFi...
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Create the Async TCP Server
  AsyncServerChargen = new AsyncServer(CHARGEN_PORT);
  // Set up the callback for new client connections.
  AsyncServerChargen->onClient(&handleClient, nullptr);
  AsyncServerChargen->begin();
  Serial.printf("Chargen Server (%s) started on port %d\n", WiFi.localIP().toString().c_str(), CHARGEN_PORT);
}

void loop() {
  // The async library handles everything in the background.
  // No code is needed here for the server to run.
}
