// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

//
// Authentication and authorization middlewares
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

static AsyncWebServer server(80);

// basicAuth
static AsyncAuthenticationMiddleware basicAuth;
static AsyncAuthenticationMiddleware basicAuthHash;

// simple digest authentication
static AsyncAuthenticationMiddleware digestAuth;
static AsyncAuthenticationMiddleware digestAuthHash;

static AsyncAuthenticationMiddleware bearerAuthSharedKey;
static AsyncAuthenticationMiddleware bearerAuthJWT;

// complex authentication which adds request attributes for the next middlewares and handler
static AsyncMiddlewareFunction complexAuth([](AsyncWebServerRequest *request, ArMiddlewareNext next) {
  if (request->authenticate("Mathieu", "password")) {
    request->setAttribute("user", "Mathieu");
  } else if (request->authenticate("Bob", "password")) {
    request->setAttribute("user", "Bob");
  } else {
    return request->requestAuthentication();
  }

  if (request->getAttribute("user") == "Mathieu") {
    request->setAttribute("role", "staff");
  } else {
    request->setAttribute("role", "user");
  }

  next();
});

static AsyncAuthorizationMiddleware authz([](AsyncWebServerRequest *request) {
  return request->getAttribute("role") == "staff";
});

void setup() {
  Serial.begin(115200);

#if ASYNCWEBSERVER_WIFI_SUPPORTED
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // basic authentication
  basicAuth.setUsername("admin");
  basicAuth.setPassword("admin");
  basicAuth.setRealm("MyApp");
  basicAuth.setAuthFailureMessage("Authentication failed");
  basicAuth.setAuthType(AsyncAuthType::AUTH_BASIC);
  basicAuth.generateHash();  // precompute hash (optional but recommended)

  // basic authentication with hash
  basicAuthHash.setUsername("admin");
  basicAuthHash.setPasswordHash("YWRtaW46YWRtaW4=");  // BASE64(admin:admin)
  basicAuthHash.setRealm("MyApp");
  basicAuthHash.setAuthFailureMessage("Authentication failed");
  basicAuthHash.setAuthType(AsyncAuthType::AUTH_BASIC);

  // digest authentication
  digestAuth.setUsername("admin");
  digestAuth.setPassword("admin");
  digestAuth.setRealm("MyApp");
  digestAuth.setAuthFailureMessage("Authentication failed");
  digestAuth.setAuthType(AsyncAuthType::AUTH_DIGEST);
  digestAuth.generateHash();  // precompute hash (optional but recommended)

  // digest authentication with hash
  digestAuthHash.setUsername("admin");
  digestAuthHash.setPasswordHash("f499b71f9a36d838b79268e145e132f7");  // MD5(user:realm:pass)
  digestAuthHash.setRealm("MyApp");
  digestAuthHash.setAuthFailureMessage("Authentication failed");
  digestAuthHash.setAuthType(AsyncAuthType::AUTH_DIGEST);

  // bearer authentication with shared key
  bearerAuthSharedKey.setAuthType(AsyncAuthType::AUTH_BEARER);
  bearerAuthSharedKey.setToken("shared-secret-key");

  // bearer authentication with a JWT token
  bearerAuthJWT.setAuthType(AsyncAuthType::AUTH_BEARER);
  bearerAuthJWT.setAuthentificationFunction([](AsyncWebServerRequest *request) {
    const String &token = request->authChallenge();
    // 1. decode base64 token
    // 2. decrypt token
    const String &decrypted = "...";  // TODO
    // 3. validate token (check signature, expiration, etc)
    bool valid = token == "<token>" || token == "<another token>";
    if (!valid) {
      return false;
    }
    // 4. extract user info from token and set request attributes
    if (token == "<token>") {
      request->setAttribute("user", "Mathieu");
      request->setAttribute("role", "staff");
      return true;  // return true if token is valid, false otherwise
    }
    if (token == "<another token>") {
      request->setAttribute("user", "Bob");
      request->setAttribute("role", "user");
      return true;  // return true if token is valid, false otherwise
    }
    return false;
  });

  // basic authentication method
  // curl -v -u admin:admin  http://192.168.4.1/auth-basic
  server
    .on(
      "/auth-basic", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hello, world!");
      }
    )
    .addMiddleware(&basicAuth);

  // basic authentication method with hash
  // curl -v -u admin:admin  http://192.168.4.1/auth-basic-hash
  server
    .on(
      "/auth-basic-hash", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hello, world!");
      }
    )
    .addMiddleware(&basicAuthHash);

  // digest authentication
  // curl -v -u admin:admin --digest  http://192.168.4.1/auth-digest
  server
    .on(
      "/auth-digest", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hello, world!");
      }
    )
    .addMiddleware(&digestAuth);

  // digest authentication with hash
  // curl -v -u admin:admin --digest  http://192.168.4.1/auth-digest-hash
  server
    .on(
      "/auth-digest-hash", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hello, world!");
      }
    )
    .addMiddleware(&digestAuthHash);

  // test digest auth custom authorization middleware
  // curl -v --digest -u Mathieu:password  http://192.168.4.1/auth-custom => OK
  // curl -v --digest -u Bob:password  http://192.168.4.1/auth-custom => 403
  // curl -v --digest -u any:password  http://192.168.4.1/auth-custom => 401
  server
    .on(
      "/auth-custom", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        String buffer = "Hello ";
        buffer.concat(request->getAttribute("user"));
        buffer.concat(" with role: ");
        buffer.concat(request->getAttribute("role"));
        request->send(200, "text/plain", buffer);
      }
    )
    .addMiddlewares({&complexAuth, &authz});

  // Bearer authentication with a shared key
  // curl -v -H "Authorization: Bearer shared-secret-key" http://192.168.4.1/auth-bearer-shared-key => OK
  server
    .on(
      "/auth-bearer-shared-key", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hello, world!");
      }
    )
    .addMiddleware(&bearerAuthSharedKey);

  // Bearer authentication with a JWT token
  // curl -v -H "Authorization: Bearer <token>" http://192.168.4.1/auth-bearer-jwt => OK
  // curl -v -H "Authorization: Bearer <another token>" http://192.168.4.1/auth-bearer-jwt => 403 Forbidden
  // curl -v -H "Authorization: Bearer invalid-token" http://192.168.4.1/auth-bearer-jwt => 401 Unauthorized
  server
    .on(
      "/auth-bearer-jwt", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        Serial.println("User: " + request->getAttribute("user"));
        Serial.println("Role: " + request->getAttribute("role"));
        request->send(200, "text/plain", "Hello, world!");
      }
    )
    .addMiddlewares({&bearerAuthJWT, &authz});

  server.begin();
}

// not needed
void loop() {
  delay(100);
}
