// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#pragma once

#include <stddef.h>
#include <time.h>

#include <string>
#include <utility>

class AsyncStaticWebHandler : public AsyncWebHandler {
  using File = fs::File;
  using FS = fs::FS;

private:
  bool _getFile(AsyncWebServerRequest *request) const;
  bool _searchFile(AsyncWebServerRequest *request, const String &path);

protected:
  FS _fs;
  String _uri;
  String _path;
  String _default_file;
  String _cache_control;
  String _last_modified;
  AwsTemplateProcessor _callback;
  bool _isDir;
  bool _tryGzipFirst = true;

public:
  AsyncStaticWebHandler(const char *uri, FS &fs, const char *path, const char *cache_control);
  bool canHandle(AsyncWebServerRequest *request) const final;
  void handleRequest(AsyncWebServerRequest *request) final;
  AsyncStaticWebHandler &setTryGzipFirst(bool value);
  AsyncStaticWebHandler &setIsDir(bool isDir);
  AsyncStaticWebHandler &setDefaultFile(const char *filename);
  AsyncStaticWebHandler &setCacheControl(const char *cache_control);

  /**
     * @brief Set the Last-Modified time for the object
     *
     * @param last_modified
     * @return AsyncStaticWebHandler&
     */
  AsyncStaticWebHandler &setLastModified(const char *last_modified);
  AsyncStaticWebHandler &setLastModified(struct tm *last_modified);
  AsyncStaticWebHandler &setLastModified(time_t last_modified);
  // sets to current time. Make sure sntp is running and time is updated
  AsyncStaticWebHandler &setLastModified();

  AsyncStaticWebHandler &setTemplateProcessor(AwsTemplateProcessor newCallback);
};

class AsyncCallbackWebHandler : public AsyncWebHandler {
private:
protected:
  AsyncURIMatcher _uri;
  WebRequestMethodComposite _method;
  ArRequestHandlerFunction _onRequest;
  ArUploadHandlerFunction _onUpload;
  ArBodyHandlerFunction _onBody;
  bool _isRegex;

public:
  AsyncCallbackWebHandler() : _uri(), _method(AsyncWebRequestMethod::HTTP_ALL), _onRequest(NULL), _onUpload(NULL), _onBody(NULL), _isRegex(false) {}
  void setUri(AsyncURIMatcher uri);
  void setMethod(WebRequestMethodComposite method) {
    _method = std::move(method);
  }
  void onRequest(ArRequestHandlerFunction fn) {
    _onRequest = fn;
  }
  void onUpload(ArUploadHandlerFunction fn) {
    _onUpload = fn;
  }
  void onBody(ArBodyHandlerFunction fn) {
    _onBody = fn;
  }

  bool canHandle(AsyncWebServerRequest *request) const final;
  void handleRequest(AsyncWebServerRequest *request) final;
  void handleUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) final;
  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) final;
  bool isRequestHandlerTrivial() const final {
    return !_onRequest;
  }
};
