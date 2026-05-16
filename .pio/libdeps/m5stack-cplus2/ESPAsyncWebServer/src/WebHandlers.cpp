// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include "ESPAsyncWebServer.h"
#include "WebHandlerImpl.h"
#include "AsyncWebServerLogging.h"

#include <cstdio>
#include <utility>

using namespace asyncsrv;

AsyncWebHandler &AsyncWebHandler::setFilter(ArRequestFilterFunction fn) {
  _filter = fn;
  return *this;
}
AsyncWebHandler &AsyncWebHandler::setAuthentication(const char *username, const char *password, AsyncAuthType authMethod) {
  if (!_authMiddleware) {
    _authMiddleware = new AsyncAuthenticationMiddleware();
    _authMiddleware->_freeOnRemoval = true;
    addMiddleware(_authMiddleware);
  }
  _authMiddleware->setUsername(username);
  _authMiddleware->setPassword(password);
  _authMiddleware->setAuthType(authMethod);
  return *this;
};

AsyncStaticWebHandler::AsyncStaticWebHandler(const char *uri, FS &fs, const char *path, const char *cache_control)
  : _fs(fs), _uri(uri), _path(path), _default_file(F("index.htm")), _cache_control(cache_control), _last_modified(), _callback(nullptr) {
  // Ensure leading '/'
  if (_uri.length() == 0 || _uri[0] != '/') {
    _uri = String('/') + _uri;
  }
  if (_path.length() == 0 || _path[0] != '/') {
    _path = String('/') + _path;
  }

  // If path ends with '/' we assume a hint that this is a directory to improve performance.
  // However - if it does not end with '/' we, can't assume a file, path can still be a directory.
  _isDir = _path[_path.length() - 1] == '/';

  // Remove the trailing '/' so we can handle default file
  // Notice that root will be "" not "/"
  if (_uri[_uri.length() - 1] == '/') {
    _uri = _uri.substring(0, _uri.length() - 1);
  }
  if (_path[_path.length() - 1] == '/') {
    _path = _path.substring(0, _path.length() - 1);
  }
}

AsyncStaticWebHandler &AsyncStaticWebHandler::setTryGzipFirst(bool value) {
  _tryGzipFirst = value;
  return *this;
}

AsyncStaticWebHandler &AsyncStaticWebHandler::setIsDir(bool isDir) {
  _isDir = isDir;
  return *this;
}

AsyncStaticWebHandler &AsyncStaticWebHandler::setDefaultFile(const char *filename) {
  _default_file = filename;
  return *this;
}

AsyncStaticWebHandler &AsyncStaticWebHandler::setCacheControl(const char *cache_control) {
  _cache_control = cache_control;
  return *this;
}

AsyncStaticWebHandler &AsyncStaticWebHandler::setLastModified(const char *last_modified) {
  _last_modified = last_modified;
  return *this;
}

AsyncStaticWebHandler &AsyncStaticWebHandler::setLastModified(struct tm *last_modified) {
  char result[30];
#ifdef ESP8266
  auto formatP = PSTR("%a, %d %b %Y %H:%M:%S GMT");
  char format[strlen_P(formatP) + 1];  // NOLINT(runtime/arrays)
  strcpy_P(format, formatP);
#else
  static constexpr const char *format = "%a, %d %b %Y %H:%M:%S GMT";
#endif

  strftime(result, sizeof(result), format, last_modified);
  _last_modified = result;
  return *this;
}

AsyncStaticWebHandler &AsyncStaticWebHandler::setLastModified(time_t last_modified) {
  return setLastModified((struct tm *)gmtime(&last_modified));
}

AsyncStaticWebHandler &AsyncStaticWebHandler::setLastModified() {
  time_t last_modified;
  if (time(&last_modified) == 0) {  // time is not yet set
    return *this;
  }
  return setLastModified(last_modified);
}

bool AsyncStaticWebHandler::canHandle(AsyncWebServerRequest *request) const {
  return request->isHTTP() && request->method() == AsyncWebRequestMethod::HTTP_GET && request->url().startsWith(_uri) && _getFile(request);
}

bool AsyncStaticWebHandler::_getFile(AsyncWebServerRequest *request) const {
  // Remove the found uri
  String path = request->url().substring(_uri.length());

  // We can skip the file check and look for default if request is to the root of a directory or that request path ends with '/'
  bool canSkipFileCheck = (_isDir && path.length() == 0) || (path.length() && path[path.length() - 1] == '/');

  path = _path + path;

  // Do we have a file or .gz file
  if (!canSkipFileCheck && const_cast<AsyncStaticWebHandler *>(this)->_searchFile(request, path)) {
    return true;
  }

  // Can't handle if not default file
  if (_default_file.length() == 0) {
    return false;
  }

  // Try to add default file, ensure there is a trailing '/' to the path.
  if (path.length() == 0 || path[path.length() - 1] != '/') {
    path += String('/');
  }
  path += _default_file;

  return const_cast<AsyncStaticWebHandler *>(this)->_searchFile(request, path);
}

#ifdef ESP32
#define FILE_IS_REAL(f) (f == true && !f.isDirectory())
#else
#define FILE_IS_REAL(f) (f == true)
#endif

bool AsyncStaticWebHandler::_searchFile(AsyncWebServerRequest *request, const String &path) {
  bool fileFound = false;
  bool gzipFound = false;

  String gzip = path + T__gz;

  if (_tryGzipFirst) {
    if (_fs.exists(gzip)) {
      request->_tempFile = _fs.open(gzip, fs::FileOpenMode::read);
      gzipFound = FILE_IS_REAL(request->_tempFile);
    }
    if (!gzipFound) {
      if (_fs.exists(path)) {
        request->_tempFile = _fs.open(path, fs::FileOpenMode::read);
        fileFound = FILE_IS_REAL(request->_tempFile);
      }
    }
  } else {
    if (_fs.exists(path)) {
      request->_tempFile = _fs.open(path, fs::FileOpenMode::read);
      fileFound = FILE_IS_REAL(request->_tempFile);
    }
    if (!fileFound) {
      if (_fs.exists(gzip)) {
        request->_tempFile = _fs.open(gzip, fs::FileOpenMode::read);
        gzipFound = FILE_IS_REAL(request->_tempFile);
      }
    }
  }

  bool found = fileFound || gzipFound;

  if (found) {
    // Extract the file name from the path and keep it in _tempObject
    size_t pathLen = path.length();
    char *_tempPath = (char *)malloc(pathLen + 1);
    if (_tempPath == NULL) {
      async_ws_log_e("Failed to allocate");
      request->abort();
      request->_tempFile.close();
      return false;
    }
    snprintf_P(_tempPath, pathLen + 1, PSTR("%s"), path.c_str());
    request->_tempObject = (void *)_tempPath;
  }

  return found;
}

/**
 * @brief Handles an incoming HTTP request for a static file.
 *
 * This method processes a request for serving static files asynchronously.
 * It determines the correct ETag (entity tag) for caching, checks if the file
 * has been modified, and prepares the appropriate response (file response or 304 Not Modified).
 *
 * @param request Pointer to the incoming AsyncWebServerRequest object.
 */
void AsyncStaticWebHandler::handleRequest(AsyncWebServerRequest *request) {
  // Get the filename from request->_tempObject and free it
  String filename((char *)request->_tempObject);
  free(request->_tempObject);
  request->_tempObject = nullptr;

  if (request->_tempFile != true) {
    request->send(404);
    return;
  }

  // Get server ETag. If file is not GZ and we have a Template Processor, ETag is set to an empty string
  char etag[11];
  const char *tempFileName = request->_tempFile.name();
  const size_t lenFilename = strlen(tempFileName);

  if (lenFilename > T__GZ_LEN && memcmp(tempFileName + lenFilename - T__GZ_LEN, T__gz, T__GZ_LEN) == 0) {
    //File is a gz, get etag from CRC in trailer
    if (!AsyncWebServerRequest::_getEtag(request->_tempFile, etag)) {
      // File is corrupted or invalid
      async_ws_log_w("File is corrupted or invalid: %s", tempFileName);
      request->send(404);
      return;
    }

    // Reset file position to the beginning so the file can be served from the start.
    request->_tempFile.seek(0);
  } else if (_callback == nullptr) {
    // We don't have a Template processor
    uint32_t etagValue;
    time_t lastWrite = request->_tempFile.getLastWrite();
    if (lastWrite > 0) {
      // Use timestamp-based ETag
      etagValue = static_cast<uint32_t>(lastWrite);
    } else {
      // No timestamp available, use filesize-based ETag
      size_t fileSize = request->_tempFile.size();
      etagValue = static_cast<uint32_t>(fileSize);
    }
    // RFC9110 Section-8.8.3: Value of the ETag response must be enclosed in double quotes
    snprintf(etag, sizeof(etag), "\"%08" PRIx32 "\"", etagValue);
  } else {
    etag[0] = '\0';
  }

  AsyncWebServerResponse *response;

  bool notModified = false;
  // 1. If the client sent If-None-Match and we have an ETag → compare
  if (*etag != '\0' && request->header(T_INM) == etag) {
    notModified = true;
  }
  // 2. Otherwise, if there is no ETag but we have Last-Modified and Last-Modified matches
  else if (*etag == '\0' && _last_modified.length() > 0 && request->header(T_IMS) == _last_modified) {
    async_ws_log_d("_last_modified: %s", _last_modified.c_str());
    notModified = true;
  }

  if (notModified) {
    request->_tempFile.close();
    response = new AsyncBasicResponse(304);  // Not modified
  } else {
    response = new AsyncFileResponse(request->_tempFile, filename, asyncsrv::emptyString, false, _callback);
  }

  if (!response) {
    async_ws_log_e("Failed to allocate");
    request->abort();
    return;
  }

  if (!notModified) {
    // Set ETag header
    if (*etag != '\0') {
      response->addHeader(T_ETag, etag, true);
    }
    // Set Last-Modified header
    if (_last_modified.length()) {
      response->addHeader(T_Last_Modified, _last_modified.c_str(), true);
    }
  }

  // Set cache control
  if (_cache_control.length()) {
    response->addHeader(T_Cache_Control, _cache_control.c_str(), false);
  } else {
    response->addHeader(T_Cache_Control, T_no_cache, false);
  }

  request->send(response);
}

AsyncStaticWebHandler &AsyncStaticWebHandler::setTemplateProcessor(AwsTemplateProcessor newCallback) {
  _callback = newCallback;
  return *this;
}

void AsyncCallbackWebHandler::setUri(AsyncURIMatcher uri) {
  _uri = std::move(uri);
}

bool AsyncCallbackWebHandler::canHandle(AsyncWebServerRequest *request) const {
  if (!_onRequest || !request->isHTTP() || !_method.matches(request->method())) {
    return false;
  }
  return _uri.matches(request);
}

void AsyncCallbackWebHandler::handleRequest(AsyncWebServerRequest *request) {
  if (_onRequest) {
    _onRequest(request);
  } else {
    request->send(404, T_text_plain, "Not found");
  }
}
void AsyncCallbackWebHandler::handleUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (_onUpload) {
    _onUpload(request, filename, index, data, len, final);
  }
}
void AsyncCallbackWebHandler::handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  // ESP_LOGD("AsyncWebServer", "AsyncCallbackWebHandler::handleBody");
  if (_onBody) {
    _onBody(request, data, len, index, total);
  }
}
