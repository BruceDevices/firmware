// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include <ESPAsyncWebServer.h>

const AsyncWebHeader AsyncWebHeader::parse(const char *data) {
  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers
  // In HTTP/1.X, a header is a case-insensitive name followed by a colon, then optional whitespace which will be ignored, and finally by its value
  if (!data) {
    return AsyncWebHeader();  // nullptr
  }
  if (data[0] == '\0') {
    return AsyncWebHeader();  // empty string
  }
  if (strchr(data, '\n') || strchr(data, '\r')) {
    return AsyncWebHeader();  // Invalid header format
  }
  const char *colon = strchr(data, ':');
  if (!colon) {
    return AsyncWebHeader();  // separator not found
  }
  if (colon == data) {
    return AsyncWebHeader();  // Header name cannot be empty
  }
  const char *startOfValue = colon + 1;  // Skip the colon
  // skip one optional whitespace after the colon
  if (*startOfValue == ' ') {
    startOfValue++;
  }
  String name;
  name.reserve(colon - data);
  name.concat(data, colon - data);
  return AsyncWebHeader(name, String(startOfValue));
}
