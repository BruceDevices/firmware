// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include "WebAuthentication.h"
#include <ESPAsyncWebServer.h>

#include <list>

AsyncMiddlewareChain::~AsyncMiddlewareChain() {
  for (AsyncMiddleware *m : _middlewares) {
    if (m->_freeOnRemoval) {
      delete m;
    }
  }
}

void AsyncMiddlewareChain::addMiddleware(ArMiddlewareCallback fn) {
  AsyncMiddlewareFunction *m = new AsyncMiddlewareFunction(fn);
  m->_freeOnRemoval = true;
  _middlewares.emplace_back(m);
}

void AsyncMiddlewareChain::addMiddleware(AsyncMiddleware *middleware) {
  if (middleware) {
    _middlewares.emplace_back(middleware);
  }
}

void AsyncMiddlewareChain::addMiddlewares(std::vector<AsyncMiddleware *> middlewares) {
  for (AsyncMiddleware *m : middlewares) {
    addMiddleware(m);
  }
}

bool AsyncMiddlewareChain::removeMiddleware(AsyncMiddleware *middleware) {
  // remove all middlewares from _middlewares vector being equal to middleware, delete them having _freeOnRemoval flag to true and resize the vector.
  const size_t size = _middlewares.size();
  _middlewares.erase(
    std::remove_if(
      _middlewares.begin(), _middlewares.end(),
      [middleware](AsyncMiddleware *m) {
        if (m == middleware) {
          if (m->_freeOnRemoval) {
            delete m;
          }
          return true;
        }
        return false;
      }
    ),
    _middlewares.end()
  );
  return size != _middlewares.size();
}

void AsyncMiddlewareChain::_runChain(AsyncWebServerRequest *request, ArMiddlewareNext finalizer) {
  if (!_middlewares.size()) {
    return finalizer();
  }
  ArMiddlewareNext next;
  std::list<AsyncMiddleware *>::iterator it = _middlewares.begin();
  next = [this, &next, &it, request, finalizer]() {
    if (it == _middlewares.end()) {
      return finalizer();
    }
    AsyncMiddleware *m = *it;
    it++;
    return m->run(request, next);
  };
  return next();
}

void AsyncAuthenticationMiddleware::setUsername(const char *username) {
  _username = username;
  _hasCreds = _username.length() && _credentials.length();
}

void AsyncAuthenticationMiddleware::setPassword(const char *password) {
  _credentials = password;
  _hash = false;
  _hasCreds = _username.length() && _credentials.length();
}

void AsyncAuthenticationMiddleware::setPasswordHash(const char *hash) {
  _credentials = hash;
  _hash = _credentials.length();
  _hasCreds = _username.length() && _credentials.length();
}

void AsyncAuthenticationMiddleware::setToken(const char *token) {
  _credentials = token;
  _hash = _credentials.length();
  _hasCreds = _credentials.length();
}

bool AsyncAuthenticationMiddleware::generateHash() {
  // ensure we have all the necessary data
  if (!_hasCreds) {
    return false;
  }

  // if we already have a hash, do nothing
  if (_hash) {
    return false;
  }

  switch (_authMethod) {
    case AsyncAuthType::AUTH_DIGEST:
      _credentials = generateDigestHash(_username.c_str(), _credentials.c_str(), _realm.c_str());
      if (_credentials.length()) {
        _hash = true;
        return true;
      } else {
        return false;
      }

    case AsyncAuthType::AUTH_BASIC:
      _credentials = generateBasicHash(_username.c_str(), _credentials.c_str());
      if (_credentials.length()) {
        _hash = true;
        return true;
      } else {
        return false;
      }

    default: return false;
  }
}

bool AsyncAuthenticationMiddleware::allowed(AsyncWebServerRequest *request) const {
  switch (_authMethod) {
    case AsyncAuthType::AUTH_NONE:   return true;
    case AsyncAuthType::AUTH_DENIED: return false;
    case AsyncAuthType::AUTH_BEARER: return _authcFunc(request);
    case AsyncAuthType::AUTH_OTHER:  return _authcFunc(request);
    case AsyncAuthType::AUTH_BASIC:  return !_hasCreds || _authcFunc(request);
    case AsyncAuthType::AUTH_DIGEST: return !_hasCreds || _authcFunc(request);
    default:                         return false;
  }
}

void AsyncAuthenticationMiddleware::run(AsyncWebServerRequest *request, ArMiddlewareNext next) {
  return allowed(request) ? next() : request->requestAuthentication(_authMethod, _realm.c_str(), _authFailMsg.c_str());
}

void AsyncHeaderFreeMiddleware::run(AsyncWebServerRequest *request, ArMiddlewareNext next) {
  std::list<const char *> toRemove;
  for (auto &h : request->getHeaders()) {
    bool keep = false;
    for (const char *k : _toKeep) {
      if (strcasecmp(h.name().c_str(), k) == 0) {
        keep = true;
        break;
      }
    }
    if (!keep) {
      toRemove.push_back(h.name().c_str());
    }
  }
  for (const char *h : toRemove) {
    request->removeHeader(h);
  }
  next();
}

void AsyncHeaderFilterMiddleware::run(AsyncWebServerRequest *request, ArMiddlewareNext next) {
  for (auto it = _toRemove.begin(); it != _toRemove.end(); ++it) {
    request->removeHeader(*it);
  }
  next();
}

void AsyncLoggingMiddleware::run(AsyncWebServerRequest *request, ArMiddlewareNext next) {
  if (!isEnabled()) {
    next();
    return;
  }
  _out->print(F("* Connection from "));
#ifndef LIBRETINY
  _out->print(request->client()->remoteIP().toString());
#else
  _out->print(request->client()->remoteIP());
#endif
  _out->print(':');
  _out->println(request->client()->remotePort());
  _out->print('>');
  _out->print(' ');
  _out->print(request->methodToString());
  _out->print(' ');
  _out->print(request->url().c_str());
  _out->print(F(" HTTP/1."));
  _out->println(request->version());
  for (auto &h : request->getHeaders()) {
    if (h.value().length()) {
      _out->print('>');
      _out->print(' ');
      _out->print(h.name());
      _out->print(':');
      _out->print(' ');
      _out->println(h.value());
    }
  }
  _out->println(F(">"));
  uint32_t elapsed = millis();
  next();
  elapsed = millis() - elapsed;
  AsyncWebServerResponse *response = request->getResponse();
  if (response) {
    _out->print(F("* Processed in "));
    _out->print(elapsed);
    _out->println(F(" ms"));
    _out->print('<');
    _out->print(F(" HTTP/1."));
    _out->print(request->version());
    _out->print(' ');
    _out->print(response->code());
    _out->print(' ');
    _out->println(AsyncWebServerResponse::responseCodeToString(response->code()));
    for (auto &h : response->getHeaders()) {
      if (h.value().length()) {
        _out->print('<');
        _out->print(' ');
        _out->print(h.name());
        _out->print(':');
        _out->print(' ');
        _out->println(h.value());
      }
    }
    _out->println('<');
  } else {
    _out->println(F("* Connection closed!"));
  }
}

void AsyncCorsMiddleware::addCORSHeaders(AsyncWebServerRequest *request, AsyncWebServerResponse *response) {
  if (request != nullptr && _credentials && _origin == "*") {
    // cannot use wildcard when allowing credentials
    response->addHeader(asyncsrv::T_CORS_ACAO, request->header(asyncsrv::T_CORS_O).c_str());
  } else {
    response->addHeader(asyncsrv::T_CORS_ACAO, _origin.c_str());
  }
  response->addHeader(asyncsrv::T_CORS_ACAM, _methods.c_str());
  response->addHeader(asyncsrv::T_CORS_ACAH, _headers.c_str());
  response->addHeader(asyncsrv::T_CORS_ACAC, _credentials ? asyncsrv::T_TRUE : asyncsrv::T_FALSE);
  response->addHeader(asyncsrv::T_CORS_ACMA, String(_maxAge).c_str());
}

void AsyncCorsMiddleware::run(AsyncWebServerRequest *request, ArMiddlewareNext next) {
  // Origin header ? => CORS handling
  if (request->hasHeader(asyncsrv::T_CORS_O)) {
    // check if this is a preflight request => handle it and return
    if (request->method() == AsyncWebRequestMethod::HTTP_OPTIONS) {
      AsyncWebServerResponse *response = request->beginResponse(200);
      addCORSHeaders(request, response);
      request->send(response);
      return;
    }

    // CORS request, no options => let the request pass and add CORS headers after
    next();
    AsyncWebServerResponse *response = request->getResponse();
    if (response) {
      addCORSHeaders(request, response);
    }

  } else {
    // NO Origin header => no CORS handling
    next();
  }
}

bool AsyncRateLimitMiddleware::isRequestAllowed(uint32_t &retryAfterSeconds) {
  uint32_t now = millis();

  while (!_requestTimes.empty() && _requestTimes.front() <= now - _windowSizeMillis) {
    _requestTimes.pop_front();
  }

  _requestTimes.push_back(now);

  if (_requestTimes.size() > _maxRequests) {
    _requestTimes.pop_front();
    retryAfterSeconds = (_windowSizeMillis - (now - _requestTimes.front())) / 1000 + 1;
    return false;
  }

  retryAfterSeconds = 0;
  return true;
}

void AsyncRateLimitMiddleware::run(AsyncWebServerRequest *request, ArMiddlewareNext next) {
  uint32_t retryAfterSeconds;
  if (isRequestAllowed(retryAfterSeconds)) {
    next();
  } else {
    AsyncWebServerResponse *response = request->beginResponse(429);
    response->addHeader(asyncsrv::T_retry_after, retryAfterSeconds);
    request->send(response);
  }
}
