// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Shows how to serve a static and dynamic template
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
<body>
    <h1>Hello, %USER%</h1>
</body>
</html>
)";

static const size_t htmlContentLength = strlen_P(htmlContent);

// Variables used for dynamic cacheable template
static unsigned uptimeInMinutes = 0;
static AsyncStaticWebHandler *uptimeHandler = nullptr;

// Utility function for performing that update
static void setUptimeInMinutes(unsigned t) {
  uptimeInMinutes = t;
  // Update caching header with a new value as well
  if (uptimeHandler) {
    uptimeHandler->setLastModified();
  }
}

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
    File f = LittleFS.open("/template.html", "w");
    assert(f);
    f.print(htmlContent);
    f.close();
  }

  // Serve the static template file
  //
  // This call will have caching headers automatically added as it is a static file.
  //
  // curl -v http://192.168.4.1/template.html
  server.serveStatic("/template.html", LittleFS, "/template.html");

  // Serve a template with dynamic content
  //
  // serveStatic recognizes that template processing is in use, and will not automatically
  // add caching headers.
  //
  // curl -v http://192.168.4.1/dynamic.html
  server.serveStatic("/dynamic.html", LittleFS, "/template.html").setTemplateProcessor([](const String &var) -> String {
    if (var == "USER") {
      return String("Bob ") + millis();
    }
    return asyncsrv::emptyString;
  });

  // Serve a static template with a template processor
  //
  // By explicitly calling setLastModified() on the handler object, we enable
  // sending the caching headers, even when a template is in use.
  // This pattern should never be used with template data that can change.
  // Example below: USER never changes.
  //
  // curl -v http://192.168.4.1/index.html
  server.serveStatic("/index.html", LittleFS, "/template.html")
    .setTemplateProcessor([](const String &var) -> String {
      if (var == "USER") {
        return "Bob";
      }
      return asyncsrv::emptyString;
    })
    .setLastModified("Sun, 28 Sep 2025 01:02:03 GMT");

  // Serve a template with dynamic content *and* caching
  //
  // The data used in this template is updated in loop().  loop() is then responsible
  // for calling setLastModified() on the handler object to notify any caches that
  // the data has changed.
  //
  // curl -v http://192.168.4.1/uptime.html
  uptimeHandler = &server.serveStatic("/uptime.html", LittleFS, "/template.html").setTemplateProcessor([](const String &var) -> String {
    if (var == "USER") {
      return String("Bob ") + uptimeInMinutes + " minutes";
    }
    return asyncsrv::emptyString;
  });

  // Serve a template with dynamic content based on user request
  //
  // In this case, the template is served via a callback request.  Data from the request
  // is used to generate the template callback.
  //
  // curl -v -G -d "USER=Bob" http://192.168.4.1/user_request.html
  server.on("/user_request.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/template.html", "text/html", false, [=](const String &var) -> String {
      if (var == "USER") {
        const AsyncWebParameter *param = request->getParam("USER");
        if (param) {
          return param->value();
        }
      }
      return asyncsrv::emptyString;
    });
  });

  server.begin();
}

// not needed
void loop() {
  delay(100);

  // Compute uptime
  unsigned currentUptimeInMinutes = millis() / (60 * 1000);

  if (currentUptimeInMinutes != uptimeInMinutes) {
    setUptimeInMinutes(currentUptimeInMinutes);
  }
}
