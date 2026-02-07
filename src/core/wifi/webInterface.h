
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <typeinfo>

extern AsyncWebServer *server; // used to check if the webserver is running

// function defaults
String humanReadableSize(uint64_t bytes);
String listFiles(FS &fs, String folder);
String readLineFromFile(File myFile);

void loopOptionsWebUi();

void serveWebUIFile(AsyncWebServerRequest *request, String filename, const char *contentType);
void serveWebUIFile(
    AsyncWebServerRequest *request, String filename, const char *contentType, bool gzip,
    const uint8_t *originaFile, uint32_t originalFileSize
);
void configureWebServer();
void startWebUi(bool mode_ap = false);
void stopWebUi();
