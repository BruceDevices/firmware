// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include "ESPAsyncWebServer.h"
#include "WebHandlerImpl.h"

#include <string>
#include <utility>

#if defined(ESP32) || defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350) || defined(LIBRETINY) || defined(HOST)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#error Platform not supported
#endif

using namespace asyncsrv;

bool ON_STA_FILTER(AsyncWebServerRequest *request) {
#if ASYNCWEBSERVER_WIFI_SUPPORTED
  return WiFi.localIP() == request->client()->localIP();
#else
  return false;
#endif
}

bool ON_AP_FILTER(AsyncWebServerRequest *request) {
#if ASYNCWEBSERVER_WIFI_SUPPORTED
  return WiFi.localIP() != request->client()->localIP();
#else
  return false;
#endif
}

#ifndef HAVE_FS_FILE_OPEN_MODE
const char *fs::FileOpenMode::read = "r";
const char *fs::FileOpenMode::write = "w";
const char *fs::FileOpenMode::append = "a";
#endif

AsyncWebServer::AsyncWebServer(uint16_t port) : _server(port) {
  _catchAllHandler = new AsyncCallbackWebHandler();
  _server.onClient(
    [](void *s, AsyncClient *c) {
      if (c == NULL) {
        return;
      }
      c->setRxTimeout(3);
      AsyncWebServerRequest *r = new AsyncWebServerRequest((AsyncWebServer *)s, c);
      if (r == NULL) {
        c->abort();
        delete c;
      }
    },
    this
  );
}

AsyncWebServer::~AsyncWebServer() {
  reset();
  end();
  delete _catchAllHandler;
  _catchAllHandler = nullptr;  // Prevent potential use-after-free
}

AsyncWebRewrite &AsyncWebServer::addRewrite(std::shared_ptr<AsyncWebRewrite> rewrite) {
  _rewrites.emplace_back(rewrite);
  return *_rewrites.back().get();
}

AsyncWebRewrite &AsyncWebServer::addRewrite(AsyncWebRewrite *rewrite) {
  _rewrites.emplace_back(rewrite);
  return *_rewrites.back().get();
}

bool AsyncWebServer::removeRewrite(AsyncWebRewrite *rewrite) {
  return removeRewrite(rewrite->from().c_str(), rewrite->toUrl().c_str());
}

bool AsyncWebServer::removeRewrite(const char *from, const char *to) {
  for (auto r = _rewrites.begin(); r != _rewrites.end(); ++r) {
    if (r->get()->from() == from && r->get()->toUrl() == to) {
      _rewrites.erase(r);
      return true;
    }
  }
  return false;
}

AsyncWebRewrite &AsyncWebServer::rewrite(const char *from, const char *to) {
  _rewrites.emplace_back(std::make_shared<AsyncWebRewrite>(from, to));
  return *_rewrites.back().get();
}

AsyncWebHandler &AsyncWebServer::addHandler(AsyncWebHandler *handler) {
  _handlers.emplace_back(handler);
  return *(_handlers.back().get());
}

bool AsyncWebServer::removeHandler(AsyncWebHandler *handler) {
  for (auto i = _handlers.begin(); i != _handlers.end(); ++i) {
    if (i->get() == handler) {
      _handlers.erase(i);
      return true;
    }
  }
  return false;
}

void AsyncWebServer::begin() {
  _server.setNoDelay(true);
  _server.begin();
}

void AsyncWebServer::end() {
  _server.end();
}

#if ASYNC_TCP_SSL_ENABLED
void AsyncWebServer::onSslFileRequest(AcSSlFileHandler cb, void *arg) {
  _server.onSslFileRequest(cb, arg);
}

void AsyncWebServer::beginSecure(const char *cert, const char *key, const char *password) {
  _server.beginSecure(cert, key, password);
}
#endif

void AsyncWebServer::_handleDisconnect(AsyncWebServerRequest *request) {
  delete request;
}

void AsyncWebServer::_rewriteRequest(AsyncWebServerRequest *request) {
  // the last rewrite that matches the request will be used
  // we do not break the loop to allow for multiple rewrites to be applied and only the last one to be used (allows overriding)
  for (const auto &r : _rewrites) {
    if (r->match(request)) {
      request->_url = r->toUrl();
      request->_addGetParams(r->params());
    }
  }
}

void AsyncWebServer::_attachHandler(AsyncWebServerRequest *request) {
  for (auto &h : _handlers) {
    if (h->filter(request) && h->canHandle(request)) {
      request->setHandler(h.get());
      return;
    }
  }
  // ESP_LOGD("AsyncWebServer", "No handler found for %s, using _catchAllHandler pointer: %p", request->url().c_str(), _catchAllHandler);
  request->setHandler(_catchAllHandler);
}

AsyncCallbackWebHandler &AsyncWebServer::on(
  AsyncURIMatcher uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload, ArBodyHandlerFunction onBody
) {
  AsyncCallbackWebHandler *handler = new AsyncCallbackWebHandler();
  handler->setUri(std::move(uri));
  handler->setMethod(std::move(method));
  handler->onRequest(onRequest);
  handler->onUpload(onUpload);
  handler->onBody(onBody);
  addHandler(handler);
  return *handler;
}

#if ASYNC_JSON_SUPPORT == 1
AsyncCallbackJsonWebHandler &AsyncWebServer::on(AsyncURIMatcher uri, WebRequestMethodComposite method, ArJsonRequestHandlerFunction onBody) {
  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler(std::move(uri), onBody);
  handler->setMethod(std::move(method));
  addHandler(handler);
  return *handler;
}
#endif

AsyncStaticWebHandler &AsyncWebServer::serveStatic(const char *uri, fs::FS &fs, const char *path, const char *cache_control) {
  AsyncStaticWebHandler *handler = new AsyncStaticWebHandler(uri, fs, path, cache_control);
  addHandler(handler);
  return *handler;
}

void AsyncWebServer::onNotFound(ArRequestHandlerFunction fn) {
  _catchAllHandler->onRequest(fn);
}

void AsyncWebServer::onFileUpload(ArUploadHandlerFunction fn) {
  _catchAllHandler->onUpload(fn);
}

void AsyncWebServer::onRequestBody(ArBodyHandlerFunction fn) {
  _catchAllHandler->onBody(fn);
}

AsyncWebHandler &AsyncWebServer::catchAllHandler() const {
  return *_catchAllHandler;
}

void AsyncWebServer::reset() {
  _rewrites.clear();
  _handlers.clear();

  _catchAllHandler->onRequest(NULL);
  _catchAllHandler->onUpload(NULL);
  _catchAllHandler->onBody(NULL);
}

AsyncURIMatcher::AsyncURIMatcher(String uri, uint16_t modifiers) : _value(std::move(uri)) {
#ifdef ASYNCWEBSERVER_REGEX
  if (_value.startsWith("^") && _value.endsWith("$")) {
    pattern = new std::regex(_value.c_str(), (modifiers & CaseInsensitive) ? (std::regex::icase | std::regex::optimize) : (std::regex::optimize));
    return;  // no additional processing - flags are overwritten by pattern pointer
  }
#endif
  if (modifiers & CaseInsensitive) {
    _value.toLowerCase();
  }
  // Inspect _value to set flags
  // empty URI matches everything
  if (!_value.length()) {
    _flags = _toFlags(Type::All, modifiers);
  } else if (_value.endsWith("*")) {
    // wildcard match with * at the end
    _flags = _toFlags(Type::Prefix, modifiers);
    _value = _value.substring(0, _value.length() - 1);
  } else if (_value.lastIndexOf("/*.") >= 0) {
    // prefix match with /*.ext
    // matches any path ending with .ext
    // e.g. /images/*.png will match /images/pic.png and /images/2023/pic.png but not /img/pic.png
    _flags = _toFlags(Type::Extension, modifiers);
  } else {
    // backward compatible use case: exact match or prefix with trailing /
    _flags = _toFlags(Type::BackwardCompatible, modifiers);
  }
}

AsyncURIMatcher::AsyncURIMatcher(String uri, Type type, uint16_t modifiers) : _value(std::move(uri)), _flags(_toFlags(type, modifiers)) {
#ifdef ASYNCWEBSERVER_REGEX
  if (type == Type::Regex) {
    pattern = new std::regex(_value.c_str(), (modifiers & CaseInsensitive) ? (std::regex::icase | std::regex::optimize) : (std::regex::optimize));
    return;  // no additional processing - flags are overwritten by pattern pointer
  }
#endif
  if (modifiers & CaseInsensitive) {
    _value.toLowerCase();
  }
}

#ifdef ASYNCWEBSERVER_REGEX

AsyncURIMatcher::AsyncURIMatcher(const AsyncURIMatcher &c) : _value(c._value), _flags(c._flags) {
  if (_isRegex()) {
    pattern = new std::regex(*pattern);
  }
}

AsyncURIMatcher::AsyncURIMatcher(AsyncURIMatcher &&c) : _value(std::move(c._value)), _flags(c._flags) {
  c._flags = _toFlags(Type::None, None);
}

AsyncURIMatcher::~AsyncURIMatcher() {
  if (_isRegex()) {
    delete pattern;
  }
}

AsyncURIMatcher &AsyncURIMatcher::operator=(const AsyncURIMatcher &r) {
  _value = r._value;
  if (r._isRegex()) {
    // Allocate first before we delete our current state
    auto p = new std::regex(*r.pattern);
    // Safely reassign our pattern
    if (_isRegex()) {
      delete pattern;
    }
    pattern = p;
  } else {
    if (_isRegex()) {
      delete pattern;
    }
    _flags = r._flags;
  }
  return *this;
}

AsyncURIMatcher &AsyncURIMatcher::operator=(AsyncURIMatcher &&r) {
  _value = std::move(r._value);
  if (_isRegex()) {
    delete pattern;
  }
  _flags = r._flags;
  if (r._isRegex()) {
    // We have adopted it
    r._flags = _toFlags(Type::None, None);
  }
  return *this;
}

#endif

bool AsyncURIMatcher::matches(AsyncWebServerRequest *request) const {
#ifdef ASYNCWEBSERVER_REGEX
  if (_isRegex()) {
    // when type == Type::Regex, or when _value was auto-detected as regex
    std::smatch matches;
    std::string s(request->url().c_str());
    if (std::regex_search(s, matches, *pattern)) {
      for (size_t i = 1; i < matches.size(); ++i) {
        request->_pathParams.emplace_back(matches[i].str().c_str());
      }
      return true;
    }
    return false;
  }
#endif

  // extract matcher type from _flags
  Type type;
  uint16_t modifiers;
  std::tie(type, modifiers) = _fromFlags(_flags);

  // apply modifiers
  String path = request->url();
  if (modifiers & CaseInsensitive) {
    path.toLowerCase();
  }

  switch (type) {
    case Type::All:    return true;
    case Type::None:   return false;
    case Type::Exact:  return (_value == path);
    case Type::Prefix: return path.startsWith(_value);
    case Type::Extension:
    {
      int split = _value.lastIndexOf("/*.");
      return (split >= 0 && path.startsWith(_value.substring(0, split)) && path.endsWith(_value.substring(split + 2)));
    }
    case Type::BackwardCompatible: return (_value == path) || path.startsWith(_value + "/");
    default:
      // Should never happen - programming error
      assert("Invalid type");
      return false;
  }
}
