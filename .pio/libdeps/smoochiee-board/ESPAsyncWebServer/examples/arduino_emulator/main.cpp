#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer server(8080);

void setup() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "ESPAsyncWebServer host app is running on port 8080\n");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found\n");
  });

  PosixAsyncTCPManager::getInstance().begin();
  server.begin();
}

void loop() {
  delay(1000);
}
