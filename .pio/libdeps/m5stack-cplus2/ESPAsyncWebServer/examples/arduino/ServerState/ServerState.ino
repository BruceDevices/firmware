// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Server state example
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

static AsyncWebServer server1(80);
static AsyncWebServer server2(80);

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // server state returns one of the tcp_state enum values:
  // enum tcp_state {
  //   CLOSED      = 0,
  //   LISTEN      = 1,
  //   SYN_SENT    = 2,
  //   SYN_RCVD    = 3,
  //   ESTABLISHED = 4,
  //   FIN_WAIT_1  = 5,
  //   FIN_WAIT_2  = 6,
  //   CLOSE_WAIT  = 7,
  //   CLOSING     = 8,
  //   LAST_ACK    = 9,
  //   TIME_WAIT   = 10
  // };

  assert(server1.state() == tcp_state::CLOSED);
  assert(server2.state() == tcp_state::CLOSED);

  server1.begin();

  assert(server1.state() == tcp_state::LISTEN);
  assert(server2.state() == tcp_state::CLOSED);

  server2.begin();

  assert(server1.state() == tcp_state::LISTEN);
  assert(server2.state() == tcp_state::CLOSED);

  Serial.println("Done!");
}

void loop() {
  delay(100);
}
