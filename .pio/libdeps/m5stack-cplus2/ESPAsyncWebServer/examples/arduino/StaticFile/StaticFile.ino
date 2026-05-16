// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Shows how to serve a static file
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

static AsyncWebServer server(80);

static const char *htmlContent PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Sample HTML</title>
</head>
<body>
    <h1>Hello, World!</h1>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
</body>
</html>
)";

static const size_t htmlContentLength = strlen_P(htmlContent);

// sample_html_gz.h
static const uint8_t index2_html_gz[] = {
  0x1f, 0x8b, 0x08, 0x08, 0x13, 0x45, 0x92, 0x68, 0x00, 0x03, 0x69, 0x6e, 0x64, 0x65, 0x78, 0x32, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x00, 0xed, 0x98, 0xcb,
  0x6e, 0xdc, 0x30, 0x0c, 0x45, 0xf7, 0xf3, 0x15, 0xcc, 0xde, 0xb0, 0x91, 0xbd, 0xe1, 0x4d, 0x1f, 0x48, 0x81, 0xbc, 0x8a, 0x26, 0x2d, 0xba, 0xe4, 0x48,
  0x8c, 0x87, 0x81, 0x1e, 0x0e, 0x25, 0x19, 0xc8, 0xdf, 0x97, 0xb2, 0x67, 0x82, 0x6c, 0xfa, 0x07, 0x32, 0x0c, 0x58, 0xa6, 0xa8, 0xcb, 0x4b, 0xe9, 0xac,
  0x34, 0x5e, 0x7d, 0x7d, 0xf8, 0xf2, 0xf4, 0xf7, 0xf1, 0x1b, 0x9c, 0xb2, 0x77, 0xd3, 0x61, 0xbc, 0x7c, 0x08, 0xed, 0x74, 0x00, 0x7d, 0xc6, 0xcc, 0xd9,
  0xd1, 0xf4, 0x0b, 0xfd, 0xe2, 0x08, 0x6e, 0x9e, 0xee, 0x6e, 0xc7, 0x61, 0x0f, 0x1d, 0xc6, 0x61, 0x4f, 0x1b, 0x8f, 0xd1, 0xbe, 0x9f, 0xb3, 0x4f, 0xd7,
  0xd3, 0x0d, 0x39, 0x17, 0x3b, 0xf8, 0x13, 0xc5, 0xd9, 0x2b, 0xcd, 0xb9, 0x3e, 0x4f, 0x2d, 0xd3, 0x6d, 0x14, 0xf2, 0xc0, 0x4b, 0x2a, 0x1e, 0x6c, 0x74,
  0x51, 0x20, 0x71, 0x06, 0xf4, 0x94, 0x3b, 0x30, 0x31, 0x24, 0x32, 0x99, 0x72, 0x11, 0x40, 0xcb, 0x0b, 0x27, 0xc3, 0x61, 0x06, 0x72, 0x9c, 0x7b, 0x78,
  0x94, 0xc8, 0x01, 0xa8, 0x70, 0xf2, 0xd1, 0x76, 0xb0, 0x14, 0x29, 0x09, 0xf0, 0x12, 0xd8, 0xe4, 0xe5, 0x14, 0x83, 0x29, 0xa9, 0x83, 0x22, 0x01, 0xcf,
  0x35, 0x4c, 0x91, 0xa4, 0x89, 0x1e, 0x53, 0xc2, 0x4e, 0xb3, 0xc1, 0xb2, 0xc9, 0x1a, 0xcf, 0xea, 0x50, 0xe3, 0xaf, 0x25, 0xe5, 0x08, 0x68, 0xf6, 0x41,
  0x0f, 0x3f, 0x55, 0xee, 0xad, 0x10, 0x14, 0xe7, 0xd0, 0x9b, 0x28, 0x0b, 0xc9, 0x26, 0x8d, 0x62, 0x0a, 0x04, 0x32, 0x90, 0xa3, 0xe8, 0xfb, 0x79, 0xbe,
  0x83, 0x95, 0x1c, 0xbc, 0x90, 0x78, 0x0a, 0x55, 0x79, 0x97, 0xfc, 0xf8, 0xef, 0xe1, 0x37, 0xaf, 0xe8, 0xb5, 0x56, 0x22, 0x5b, 0x53, 0xb5, 0xdd, 0x92,
  0xb7, 0xa6, 0x76, 0x65, 0x63, 0x8a, 0x4f, 0x18, 0x6a, 0xf7, 0x73, 0xad, 0xbc, 0x4f, 0x07, 0xd6, 0x95, 0xcf, 0xb9, 0x3a, 0xde, 0x05, 0x75, 0xe0, 0x50,
  0xbb, 0x83, 0x15, 0x85, 0xf5, 0x33, 0x0b, 0xae, 0x6c, 0xb1, 0x26, 0xe3, 0xb9, 0x9b, 0x1e, 0xee, 0xab, 0x2f, 0x78, 0x41, 0xc3, 0x8e, 0x13, 0xf7, 0x5b,
  0x81, 0x1f, 0x21, 0xd3, 0x4c, 0xba, 0xa3, 0xc5, 0x54, 0xe7, 0x9f, 0x37, 0xb9, 0xb8, 0x2c, 0x6c, 0x98, 0x74, 0xe5, 0xf7, 0x92, 0x0c, 0xa9, 0xeb, 0x32,
  0x33, 0xea, 0x51, 0x78, 0xfe, 0x38, 0x17, 0x38, 0xf2, 0x91, 0x82, 0xd5, 0xce, 0x56, 0x5e, 0x49, 0x44, 0xb7, 0x31, 0x8a, 0x61, 0x70, 0x14, 0x37, 0x7d,
  0x8b, 0x0b, 0x1f, 0xd5, 0x50, 0xed, 0xa8, 0x03, 0xb6, 0x17, 0x83, 0x49, 0xcf, 0xd9, 0x16, 0xae, 0x91, 0xcd, 0x78, 0x3f, 0x0e, 0x4b, 0xc3, 0xa0, 0x61,
  0xd0, 0x30, 0x68, 0x18, 0x34, 0x0c, 0x1a, 0x06, 0x0d, 0x83, 0x86, 0x41, 0xc3, 0xa0, 0x61, 0xd0, 0x30, 0x68, 0x18, 0x34, 0x0c, 0x1a, 0x06, 0xff, 0xc1,
  0x60, 0x1c, 0xf6, 0xab, 0x85, 0x71, 0xd8, 0xee, 0x25, 0xfe, 0x01, 0xa0, 0xec, 0x78, 0xfe, 0xae, 0x10, 0x00, 0x00
};

static const size_t index2_html_gz_len = sizeof(index2_html_gz);

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

  {
    File f = LittleFS.open("/index.html", "w");
    assert(f);
    f.print(htmlContent);
    f.close();
  }

  {
    File f = LittleFS.open("/index2.html.gz", "w");
    assert(f);
    f.write(index2_html_gz, index2_html_gz_len);
    f.close();
  }

  LittleFS.mkdir("/files");

  {
    File f = LittleFS.open("/files/a.txt", "w");
    assert(f);
    f.print("Hello from a.txt");
    f.close();
  }

  {
    File f = LittleFS.open("/files/b.txt", "w");
    assert(f);
    f.print("Hello from b.txt");
    f.close();
  }

  // curl -v http://192.168.4.1/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/index.html");
  });

  // curl -v http://192.168.4.1/index.html
  server.serveStatic("/index.html", LittleFS, "/index.html");

  // curl -v http://192.168.4.1/index2.html | gunzip -c
  server.serveStatic("/index2.html", LittleFS, "/index2.html");

  // Example to serve a directory content
  // curl -v http://192.168.4.1/base/ => serves a.txt
  // curl -v http://192.168.4.1/base/a.txt => serves a.txt
  // curl -v http://192.168.4.1/base/b.txt => serves b.txt
  server.serveStatic("/base", LittleFS, "/files").setDefaultFile("a.txt");

  server.begin();
}

// not needed
void loop() {
  delay(100);
}
