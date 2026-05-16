// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Perf tests
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
static constexpr char characters[] = "0123456789ABCDEF";
static size_t charactersIndex = 0;

static AsyncWebServer server(80);
static AsyncEventSource events("/events");

static volatile size_t requests = 0;

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // Pauses in the request parsing phase
  //
  // autocannon -c 32 -w 32 -a 96 -t 30 --renderStatusCodes -m POST -H "Content-Type: application/json" -b '{"foo": "bar"}' http://192.168.4.1/delay
  //
  // curl -v -X POST -H "Content-Type: application/json" -d '{"game": "test"}' http://192.168.4.1/delay
  //
  server.onNotFound([](AsyncWebServerRequest *request) {
    requests = requests + 1;
    if (request->url() == "/delay") {
      request->send(200, "application/json", "{\"status\":\"OK\"}");
    } else {
      request->send(404, "text/plain", "Not found");
    }
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (request->url() == "/delay") {
      delay(3000);
    }
  });

  // HTTP endpoint
  //
  // > autocannon -c 16 -w 16 -d 20 --renderStatusCodes  http://192.168.4.1/
  // > ab -c 16 -t 20 http://192.168.4.1/
  //
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // need to cast to uint8_t*
    // if you do not, the const char* will be copied in a temporary String buffer
    requests = requests + 1;
    request->send(200, "text/html", (uint8_t *)htmlContent, htmlContentLength);
  });

  // IMPORTANT - DO NOT WRITE SUCH CODE IN PRODUCTON !
  //
  // This example simulates the slowdown that can happen when:
  // - downloading a huge file from sdcard
  // - doing some file listing on SDCard because it is horribly slow to get a file listing with file stats on SDCard.
  // So in both cases, ESP would deadlock or TWDT would trigger.
  //
  // This example simulats that by slowing down the chunk callback:
  // - d=2000 is the delay in ms in the callback
  // - l=10000 is the length of the response
  //
  // time curl -N -v -G -d 'd=2000' -d 'l=10000'  http://192.168.4.1/slow.html --output -
  //
  // THIS CODE WILL CRASH BECAUSE OF THE WATCHDOG.
  // IF YOU REALLY NEED TO DO THIS, YOU MUST DISABLE THE TWDT
  //
  // CORRECT WAY IS TO USE SSE OR WEBSOCKETS TO DO THE COSTLY PROCESSING ASYNC.
  //
  server.on("/slow.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    requests = requests + 1;
    uint32_t d = request->getParam("d")->value().toInt();
    uint32_t l = request->getParam("l")->value().toInt();
    Serial.printf("d = %" PRIu32 ", l = %" PRIu32 "\n", d, l);
    AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [d, l](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      Serial.printf("%u\n", index);
      // finished ?
      if (index >= l) {
        return 0;
      }

      // slow down the task to simulate some heavy processing, like SD card reading
      delay(d);

      memset(buffer, characters[charactersIndex], 256);
      charactersIndex = (charactersIndex + 1) % sizeof(characters);
      return 256;
    });

    request->send(response);
  });

  // SSS endpoint
  //
  // launch 16 concurrent workers for 30 seconds
  // > for i in {1..16}; do ( count=$(gtimeout 30 curl -s -N -H "Accept: text/event-stream" http://192.168.4.1/events 2>&1 | grep -c "^data:"); echo "Total: $count events, $(echo "$count / 4" | bc -l) events / second" ) & done;
  //
  // With AsyncTCP, with 16 workers: a lot of "Event message queue overflow: discard message", no crash
  //
  // Total: 1711 events, 427.75 events / second
  // Total: 1711 events, 427.75 events / second
  // Total: 1626 events, 406.50 events / second
  // Total: 1562 events, 390.50 events / second
  // Total: 1706 events, 426.50 events / second
  // Total: 1659 events, 414.75 events / second
  // Total: 1624 events, 406.00 events / second
  // Total: 1706 events, 426.50 events / second
  // Total: 1487 events, 371.75 events / second
  // Total: 1573 events, 393.25 events / second
  // Total: 1569 events, 392.25 events / second
  // Total: 1559 events, 389.75 events / second
  // Total: 1560 events, 390.00 events / second
  // Total: 1562 events, 390.50 events / second
  // Total: 1626 events, 406.50 events / second
  //
  // With AsyncTCP, with 10 workers:
  //
  // Total: 2038 events, 509.50 events / second
  // Total: 2120 events, 530.00 events / second
  // Total: 2119 events, 529.75 events / second
  // Total: 2038 events, 509.50 events / second
  // Total: 2037 events, 509.25 events / second
  // Total: 2119 events, 529.75 events / second
  // Total: 2119 events, 529.75 events / second
  // Total: 2120 events, 530.00 events / second
  // Total: 2038 events, 509.50 events / second
  // Total: 2038 events, 509.50 events / second
  //
  // With AsyncTCPSock, with 16 workers: ESP32 CRASH !!!
  //
  // With AsyncTCPSock, with 10 workers:
  //
  // Total: 1242 events, 310.50 events / second
  // Total: 1242 events, 310.50 events / second
  // Total: 1242 events, 310.50 events / second
  // Total: 1242 events, 310.50 events / second
  // Total: 1181 events, 295.25 events / second
  // Total: 1182 events, 295.50 events / second
  // Total: 1240 events, 310.00 events / second
  // Total: 1181 events, 295.25 events / second
  // Total: 1181 events, 295.25 events / second
  // Total: 1183 events, 295.75 events / second
  //
  server.addHandler(&events);

  server.begin();
}

static uint32_t lastSSE = 0;
static uint32_t deltaSSE = 10;

static uint32_t lastHeap = 0;

void loop() {
  uint32_t now = millis();
  if (now - lastSSE >= deltaSSE) {
    events.send(String("ping-") + now, "heartbeat", now);
    lastSSE = millis();
  }

#ifdef ESP32
  if (now - lastHeap >= 2000) {
    Serial.printf(
      "Uptime: %3lu s, requests: %3u, Free heap: %" PRIu32 ", Min free heap: %" PRIu32 "\n", millis() / 1000, requests, ESP.getFreeHeap(), ESP.getMinFreeHeap()
    );
    lastHeap = now;
  }
#endif
}
