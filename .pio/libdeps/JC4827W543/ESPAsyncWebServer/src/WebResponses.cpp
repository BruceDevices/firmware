// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include "ESPAsyncWebServer.h"
#include "WebResponseImpl.h"
#include "AsyncWebServerLogging.h"

#include <algorithm>
#include <memory>
#include <utility>

#ifndef CONFIG_LWIP_TCP_WND_DEFAULT
#ifdef TCP_WND  // ESP8266
#define CONFIG_LWIP_TCP_WND_DEFAULT TCP_WND
#else
// as it is defined for esp32's LWIP
#define CONFIG_LWIP_TCP_WND_DEFAULT 5760
#endif
#endif

using namespace asyncsrv;

/*
 * Abstract Response
 *
 */

STR_RETURN_TYPE AsyncWebServerResponse::responseCodeToString(int code) {
  switch (code) {
    case 100: return STR(T_HTTP_CODE_100);
    case 101: return STR(T_HTTP_CODE_101);
    case 200: return STR(T_HTTP_CODE_200);
    case 201: return STR(T_HTTP_CODE_201);
    case 202: return STR(T_HTTP_CODE_202);
    case 203: return STR(T_HTTP_CODE_203);
    case 204: return STR(T_HTTP_CODE_204);
    case 205: return STR(T_HTTP_CODE_205);
    case 206: return STR(T_HTTP_CODE_206);
    case 300: return STR(T_HTTP_CODE_300);
    case 301: return STR(T_HTTP_CODE_301);
    case 302: return STR(T_HTTP_CODE_302);
    case 303: return STR(T_HTTP_CODE_303);
    case 304: return STR(T_HTTP_CODE_304);
    case 305: return STR(T_HTTP_CODE_305);
    case 307: return STR(T_HTTP_CODE_307);
    case 400: return STR(T_HTTP_CODE_400);
    case 401: return STR(T_HTTP_CODE_401);
    case 402: return STR(T_HTTP_CODE_402);
    case 403: return STR(T_HTTP_CODE_403);
    case 404: return STR(T_HTTP_CODE_404);
    case 405: return STR(T_HTTP_CODE_405);
    case 406: return STR(T_HTTP_CODE_406);
    case 407: return STR(T_HTTP_CODE_407);
    case 408: return STR(T_HTTP_CODE_408);
    case 409: return STR(T_HTTP_CODE_409);
    case 410: return STR(T_HTTP_CODE_410);
    case 411: return STR(T_HTTP_CODE_411);
    case 412: return STR(T_HTTP_CODE_412);
    case 413: return STR(T_HTTP_CODE_413);
    case 414: return STR(T_HTTP_CODE_414);
    case 415: return STR(T_HTTP_CODE_415);
    case 416: return STR(T_HTTP_CODE_416);
    case 417: return STR(T_HTTP_CODE_417);
    case 429: return STR(T_HTTP_CODE_429);
    case 500: return STR(T_HTTP_CODE_500);
    case 501: return STR(T_HTTP_CODE_501);
    case 502: return STR(T_HTTP_CODE_502);
    case 503: return STR(T_HTTP_CODE_503);
    case 504: return STR(T_HTTP_CODE_504);
    case 505: return STR(T_HTTP_CODE_505);
    case 507: return STR(T_HTTP_CODE_507);
    default:  return STR(T_HTTP_CODE_ANY);
  }
}

AsyncWebServerResponse::AsyncWebServerResponse()
  : _code(0), _contentType(), _contentLength(0), _sendContentLength(true), _chunked(false), _headLength(0), _sentLength(0), _ackedLength(0), _writtenLength(0),
    _state(RESPONSE_SETUP) {
  for (const auto &header : DefaultHeaders::Instance()) {
    _headers.emplace_back(header);
  }
}

void AsyncWebServerResponse::setCode(int code) {
  if (_state == RESPONSE_SETUP) {
    _code = code;
  }
}

void AsyncWebServerResponse::setContentLength(size_t len) {
  if (_state == RESPONSE_SETUP && addHeader(T_Content_Length, len, true)) {
    _contentLength = len;
  }
}

void AsyncWebServerResponse::setContentType(const char *type) {
  if (_state == RESPONSE_SETUP && addHeader(T_Content_Type, type, true)) {
    _contentType = type;
  }
}

bool AsyncWebServerResponse::removeHeader(const char *name) {
  bool h_erased = false;
  for (auto i = _headers.begin(); i != _headers.end();) {
    if (i->name().equalsIgnoreCase(name)) {
      _headers.erase(i);
      h_erased = true;
    } else {
      ++i;
    }
  }
  return h_erased;
}

bool AsyncWebServerResponse::removeHeader(const char *name, const char *value) {
  for (auto i = _headers.begin(); i != _headers.end(); ++i) {
    if (i->name().equalsIgnoreCase(name) && i->value().equalsIgnoreCase(value)) {
      _headers.erase(i);
      return true;
    }
  }
  return false;
}

const AsyncWebHeader *AsyncWebServerResponse::getHeader(const char *name) const {
  auto iter = std::find_if(std::begin(_headers), std::end(_headers), [&name](const AsyncWebHeader &header) {
    return header.name().equalsIgnoreCase(name);
  });
  return (iter == std::end(_headers)) ? nullptr : &(*iter);
}

bool AsyncWebServerResponse::headerMustBePresentOnce(const String &name) {
  for (uint8_t i = 0; i < T_only_once_headers_len; i++) {
    if (name.equalsIgnoreCase(T_only_once_headers[i])) {
      return true;
    }
  }
  return false;
}

bool AsyncWebServerResponse::addHeader(AsyncWebHeader &&header, bool replaceExisting) {
  if (!header) {
    return false;  // invalid header
  }
  for (auto i = _headers.begin(); i != _headers.end(); ++i) {
    if (i->name().equalsIgnoreCase(header.name())) {
      // header already set
      if (replaceExisting) {
        // remove, break and add the new one
        _headers.erase(i);
        break;
      } else if (headerMustBePresentOnce(i->name())) {  // we can have only one header with that name
        // do not update
        return false;
      } else {
        break;  // accept multiple headers with the same name
      }
    }
  }
  // header was not found found, or existing one was removed
  _headers.emplace_back(std::move(header));
  return true;
}

bool AsyncWebServerResponse::addHeader(const char *name, const char *value, bool replaceExisting) {
  for (auto i = _headers.begin(); i != _headers.end(); ++i) {
    if (i->name().equalsIgnoreCase(name)) {
      // header already set
      if (replaceExisting) {
        // remove, break and add the new one
        _headers.erase(i);
        break;
      } else if (headerMustBePresentOnce(i->name())) {  // we can have only one header with that name
        // do not update
        return false;
      } else {
        break;  // accept multiple headers with the same name
      }
    }
  }
  // header was not found found, or existing one was removed
  _headers.emplace_back(name, value);
  return true;
}

void AsyncWebServerResponse::_assembleHead(String &buffer, uint8_t version) {
  if (version) {
    addHeader(T_Accept_Ranges, T_none, false);
    if (_chunked) {
      addHeader(T_Transfer_Encoding, T_chunked, false);
    }
  }

  if (_sendContentLength) {
    addHeader(T_Content_Length, String(_contentLength), false);
  }

  if (_contentType.length()) {
    addHeader(T_Content_Type, _contentType.c_str(), false);
  }

  // precompute buffer size to avoid reallocations by String class
  size_t len = 0;
  len += 50;  // HTTP/1.1 200 <reason>\r\n
  for (const auto &header : _headers) {
    len += header.name().length() + header.value().length() + 4;
  }

  // prepare buffer
  buffer.reserve(len);

  // HTTP header
#ifdef ESP8266
  buffer.concat(PSTR("HTTP/1."));
#else
  buffer.concat("HTTP/1.");
#endif
  buffer.concat(version);
  buffer.concat(' ');
  buffer.concat(_code);
  buffer.concat(' ');
  buffer.concat(responseCodeToString(_code));
  buffer.concat(T_rn);

  // Add headers
  for (const auto &header : _headers) {
    buffer.concat(header.name());
#ifdef ESP8266
    buffer.concat(PSTR(": "));
#else
    buffer.concat(": ");
#endif
    buffer.concat(header.value());
    buffer.concat(T_rn);
  }

  buffer.concat(T_rn);
  _headLength = buffer.length();
}

bool AsyncWebServerResponse::_started() const {
  return _state > RESPONSE_SETUP;
}
bool AsyncWebServerResponse::_finished() const {
  return _state > RESPONSE_WAIT_ACK;
}
bool AsyncWebServerResponse::_failed() const {
  return _state == RESPONSE_FAILED;
}
bool AsyncWebServerResponse::_sourceValid() const {
  return false;
}
void AsyncWebServerResponse::_respond(AsyncWebServerRequest *request) {
  _state = RESPONSE_END;
}

/*
 * String/Code Response
 * */
AsyncBasicResponse::AsyncBasicResponse(int code, const char *contentType, const char *content) {
  _code = code;
  _content = content;
  _contentType = contentType;
  if (_content.length()) {
    _contentLength = _content.length();
    if (!_contentType.length()) {
      _contentType = T_text_plain;
    }
  }
  addHeader(T_Connection, T_close, false);
}

void AsyncBasicResponse::_respond(AsyncWebServerRequest *request) {
  _state = RESPONSE_HEADERS;
  _assembleHead(_assembled_headers, request->version());
  write_send_buffs(request, 0, 0);
}

size_t AsyncBasicResponse::write_send_buffs(AsyncWebServerRequest *request, size_t len, uint32_t time) {
  (void)time;

  // this is not functionally needed in AsyncBasicResponse itself, but kept for compatibility if some of the derived classes are rely on it somehow
  _ackedLength += len;
  size_t payloadlen{0};  // amount of data to be written to tcp sockbuff during this call, used as return value of this method

  // send http headers first
  if (_state == RESPONSE_HEADERS) {
    // copy headers buffer to sock buffer
    size_t const pcb_written = request->client()->add(_assembled_headers.c_str() + _writtenHeadersLength, _assembled_headers.length() - _writtenHeadersLength);
    _writtenLength += pcb_written;
    _writtenHeadersLength += pcb_written;
    if (_writtenHeadersLength < _assembled_headers.length()) {
      // we were not able to fit all headers in current buff, send this part here and return later for the rest
      if (!request->client()->send()) {
        // something is wrong, what should we do here?
        request->client()->close();
        return 0;
      }
      return pcb_written;
    }
    // otherwise we've added all the (remainder) headers in current buff, go on with content
    _state = RESPONSE_CONTENT;
    payloadlen += pcb_written;
    _assembled_headers = String();  // clear
  }

  if (_state == RESPONSE_CONTENT) {
    size_t const pcb_written = request->client()->write(_content.c_str() + _sentLength, _content.length() - _sentLength);
    _writtenLength += pcb_written;  // total written data (hdrs + body)
    _sentLength += pcb_written;     // body written data
    payloadlen += pcb_written;      // data writtent in current buff
    if (_sentLength >= _content.length()) {
      // we've just sent all the (remainder) data in current buff, complete the response
      _state = RESPONSE_END;
    }
  }

  // implicit complete
  if (_state == RESPONSE_WAIT_ACK) {
    _state = RESPONSE_END;
  }

  return payloadlen;
}

/*
 * Abstract Response
 *
 */
AsyncAbstractResponse::AsyncAbstractResponse(AwsTemplateProcessor callback) : _callback(callback) {
  // In case of template processing, we're unable to determine real response size
  if (callback) {
    _contentLength = 0;
    _sendContentLength = false;
    _chunked = true;
  }
}

void AsyncAbstractResponse::_respond(AsyncWebServerRequest *request) {
  addHeader(T_Connection, T_close, false);
  _assembleHead(_assembled_headers, request->version());
  _state = RESPONSE_HEADERS;
  write_send_buffs(request, 0, 0);
}

size_t AsyncAbstractResponse::write_send_buffs(AsyncWebServerRequest *request, size_t len, uint32_t time) {
  (void)time;
  if (!_sourceValid()) {
    _state = RESPONSE_FAILED;
    request->client()->close();
    return 0;
  }

#if ASYNCWEBSERVER_USE_CHUNK_INFLIGHT
  /*
    for response payloads with unknown length or length larger than TCP_WND we need to control AsyncTCP's queue and in-flight fragmentation.
    Either user callback could fill buffer with very small chunks or long running large response could receive a lot of poll() calls here,
    both could flood asynctcp's queue with large number of events to handle and fragment socket buffer space for large responses.
    Let's ignore polled acks and acks in case when available window size is less than our used buffer size since we won't be able to fill and send it whole
    That way we could balance on having at least half tcp win in-flight while minimizing send/ack events in asynctcp Q
    This could decrease sustained bandwidth for one single connection but would drastically improve parallelism and equalize bandwidth sharing
  */
  // return a credit for each chunk of acked data (polls does not give any credits)
  if (len) {
    ++_in_flight_credit;
    _in_flight -= std::min(len, _in_flight);
  }

  if (_chunked || !_sendContentLength || (_sentLength > CONFIG_LWIP_TCP_WND_DEFAULT)) {
    if (!_in_flight_credit || (ASYNC_RESPONCE_BUFF_SIZE > request->client()->space())) {
      // async_ws_log_d("defer user call in_flight:%u, tcpwin:%u", _in_flight, request->client()->space());
      // take the credit back since we are ignoring this ack and rely on other inflight data acks
      if (len) {
        --_in_flight_credit;
      }
      return 0;
    }
  }
#endif

  // this is not functionally needed in AsyncAbstractResponse itself, but kept for compatibility if some of the derived classes are rely on it somehow
  _ackedLength += len;

  size_t payloadlen{0};  // amount of data to be written to tcp sockbuff during this call, used as return value of this method

  // send http headers first
  if (_state == RESPONSE_HEADERS) {
    // copy headers buffer to sock buffer
    size_t const pcb_written = request->client()->add(_assembled_headers.c_str() + _writtenHeadersLength, _assembled_headers.length() - _writtenHeadersLength);
    _writtenLength += pcb_written;
    _writtenHeadersLength += pcb_written;
    if (_writtenHeadersLength < _assembled_headers.length()) {
// we were not able to fit all headers in current buff, send this part here and return later for the rest
#if ASYNCWEBSERVER_USE_CHUNK_INFLIGHT
      _in_flight += pcb_written;
      --_in_flight_credit;  // take a credit
#endif
      if (!request->client()->send()) {
        // something is wrong, what should we do here?
        request->client()->close();
        return 0;
      }
      return pcb_written;
    }
    // otherwise we've added all the (remainder) headers in current buff
    _state = RESPONSE_CONTENT;
    payloadlen += pcb_written;
    _assembled_headers = String();  // clear
  }

  // send content body
  if (_state == RESPONSE_CONTENT) {
    do {
      if (_send_buffer_len && _send_buffer) {
        // data is pending in buffer from a previous call or previous iteration
        size_t const added_len =
          request->client()->add(reinterpret_cast<char *>(_send_buffer->data() + _send_buffer_offset), _send_buffer_len - _send_buffer_offset);
        if (added_len != _send_buffer_len - _send_buffer_offset) {
          // we were not able to add entire buffer's content to tcp buffs, leave it for later
          // (this should not happen normally unless connection's TCP window suddenly changed from remote or mem pressure)
          _send_buffer_offset += added_len;
          break;
        } else {
          _send_buffer_len = _send_buffer_offset = 0;  // consider buffer empty
        }
        payloadlen += added_len;
      }

      auto tcp_win = request->client()->space();
      if (tcp_win == 0 || _state == RESPONSE_END) {
        break;  // no room left or no more data
      }

      if ((_chunked || !_sendContentLength) && (tcp_win < CONFIG_LWIP_TCP_MSS / 2)) {
        // available window size is not enough to send a new chunk sized half of tcp mss, let's wait for better chance and reduce pressure to AsyncTCP's event Q
        break;
      }

      if (!_send_buffer) {
        auto p = new (std::nothrow) std::array<uint8_t, ASYNC_RESPONCE_BUFF_SIZE>;
        if (p) {
          _send_buffer.reset(p);
          _send_buffer_len = _send_buffer_offset = 0;
        } else {
          break;  // OOM
        }
      }

      if (_chunked) {
        // HTTP 1.1 allows leading zeros in chunk length. Or spaces may be added.
        // See https://datatracker.ietf.org/doc/html/rfc9112#section-7.1
        size_t const readLen =
          _fillBufferAndProcessTemplates(_send_buffer->data() + 6, std::min(_send_buffer->size(), tcp_win) - 8);  // reserve 8 bytes for chunk size data
        if (readLen != RESPONSE_TRY_AGAIN) {
          // Write 4 hex digits directly without null terminator
          static constexpr char hexChars[] = "0123456789abcdef";
          _send_buffer->data()[0] = hexChars[(readLen >> 12) & 0xF];
          _send_buffer->data()[1] = hexChars[(readLen >> 8) & 0xF];
          _send_buffer->data()[2] = hexChars[(readLen >> 4) & 0xF];
          _send_buffer->data()[3] = hexChars[readLen & 0xF];
          _send_buffer->data()[4] = '\r';
          _send_buffer->data()[5] = '\n';
          // data (readLen bytes) is already there
          _send_buffer->at(readLen + 6) = '\r';
          _send_buffer->at(readLen + 7) = '\n';
          _send_buffer_len += readLen + 8;  // set buffers's size to match added data
          _sentLength += readLen;           // data is not sent yet, but we won't get a chance to count this later properly for chunked data
          if (!readLen) {
            // last chunk?
            _state = RESPONSE_END;
          }
        }
      } else {
        // Non-chunked data. We can either have a response:
        // - with a known content-length (example: Json response), in that case we pass the remaining length if lower than tcp_win
        // - or with unknown content-length (see LargeResponse example, like ESP32Cam with streaming), in that case we just fill as much as tcp_win allows
        size_t maxLen = std::min(_send_buffer->size(), tcp_win);
        if (_contentLength) {
          maxLen = _contentLength > _sentLength ? std::min(maxLen, _contentLength - _sentLength) : 0;
        }

        size_t const readLen = _fillBufferAndProcessTemplates(_send_buffer->data(), maxLen);

        if (readLen == 0) {
          // no more data to send
          _state = RESPONSE_END;
        } else if (readLen != RESPONSE_TRY_AGAIN) {
          _send_buffer_len += readLen;  // set buffers's size to match added data
          _sentLength += readLen;       // data is not sent yet, but we need it to understand that it would be last block
          if (_sendContentLength && (_sentLength == _contentLength)) {
            // it was last piece of content
            _state = RESPONSE_END;
          }
        }
      }
    } while (_send_buffer_len);  // go on till we have something in buffer pending to send

    // execute sending whatever we have in sock buffs now
    request->client()->send();
    _writtenLength += payloadlen;
#if ASYNCWEBSERVER_USE_CHUNK_INFLIGHT
    _in_flight += payloadlen;
    --_in_flight_credit;  // take a credit
#endif
    if (_send_buffer_len == 0) {
      // buffer empty, we can release mem, otherwise need to keep it till next run (should not happen under normal conditions)
      _send_buffer.reset();
    }
    return payloadlen;
  }  // (_state == RESPONSE_CONTENT)

  // implicit check
  if (_state == RESPONSE_WAIT_ACK) {
    // we do not need to wait for any acks actually if we won't send any more data,
    // connection would be closed gracefully with last piece of data (in AsyncWebServerRequest::_onAck)
    _state = RESPONSE_END;
  }
  return 0;
}

size_t AsyncAbstractResponse::_readDataFromCacheOrContent(uint8_t *data, const size_t len) {
  // If we have something in cache, copy it to buffer
  const size_t readFromCache = std::min(len, _cache.size());
  if (readFromCache) {
    memcpy(data, _cache.data(), readFromCache);
    _cache.erase(_cache.begin(), _cache.begin() + readFromCache);
  }
  // If we need to read more...
  const size_t needFromFile = len - readFromCache;
  const size_t readFromContent = _fillBuffer(data + readFromCache, needFromFile);
  return readFromCache + readFromContent;
}

size_t AsyncAbstractResponse::_fillBufferAndProcessTemplates(uint8_t *data, size_t len) {
  if (!_callback) {
    return _fillBuffer(data, len);
  }

  const size_t originalLen = len;
  len = _readDataFromCacheOrContent(data, len);
  // Now we've read 'len' bytes, either from cache or from file
  // Search for template placeholders
  uint8_t *pTemplateStart = data;
  while ((pTemplateStart < &data[len]) && (pTemplateStart = (uint8_t *)memchr(pTemplateStart, TEMPLATE_PLACEHOLDER, &data[len - 1] - pTemplateStart + 1))) {
    // data[0] ... data[len - 1]
    uint8_t *pTemplateEnd =
      (pTemplateStart < &data[len - 1]) ? (uint8_t *)memchr(pTemplateStart + 1, TEMPLATE_PLACEHOLDER, &data[len - 1] - pTemplateStart) : nullptr;
    // temporary buffer to hold parameter name
    uint8_t buf[TEMPLATE_PARAM_NAME_LENGTH + 1];
    String paramName;
    // If closing placeholder is found:
    if (pTemplateEnd) {
      // prepare argument to callback
      const size_t paramNameLength = std::min((size_t)sizeof(buf) - 1, (size_t)(pTemplateEnd - pTemplateStart - 1));
      if (paramNameLength) {
        memcpy(buf, pTemplateStart + 1, paramNameLength);
        buf[paramNameLength] = 0;
        paramName = String(reinterpret_cast<char *>(buf));
      } else {  // double percent sign encountered, this is single percent sign escaped.
        // remove the 2nd percent sign
        memmove(pTemplateEnd, pTemplateEnd + 1, &data[len] - pTemplateEnd - 1);
        len += _readDataFromCacheOrContent(&data[len - 1], 1) - 1;
        ++pTemplateStart;
      }
    } else if (&data[len - 1] - pTemplateStart + 1
               < TEMPLATE_PARAM_NAME_LENGTH + 2) {  // closing placeholder not found, check if it's in the remaining file data
      memcpy(buf, pTemplateStart + 1, &data[len - 1] - pTemplateStart);
      const size_t readFromCacheOrContent =
        _readDataFromCacheOrContent(buf + (&data[len - 1] - pTemplateStart), TEMPLATE_PARAM_NAME_LENGTH + 2 - (&data[len - 1] - pTemplateStart + 1));
      if (readFromCacheOrContent) {
        pTemplateEnd = (uint8_t *)memchr(buf + (&data[len - 1] - pTemplateStart), TEMPLATE_PLACEHOLDER, readFromCacheOrContent);
        if (pTemplateEnd) {
          // prepare argument to callback
          *pTemplateEnd = 0;
          paramName = String(reinterpret_cast<char *>(buf));
          // Copy remaining read-ahead data into cache
          _cache.insert(_cache.begin(), pTemplateEnd + 1, buf + (&data[len - 1] - pTemplateStart) + readFromCacheOrContent);
          pTemplateEnd = &data[len - 1];
        } else  // closing placeholder not found in file data, store found percent symbol as is and advance to the next position
        {
          // but first, store read file data in cache
          _cache.insert(_cache.begin(), buf + (&data[len - 1] - pTemplateStart), buf + (&data[len - 1] - pTemplateStart) + readFromCacheOrContent);
          ++pTemplateStart;
        }
      } else {  // closing placeholder not found in content data, store found percent symbol as is and advance to the next position
        ++pTemplateStart;
      }
    } else {  // closing placeholder not found in content data, store found percent symbol as is and advance to the next position
      ++pTemplateStart;
    }
    if (paramName.length()) {
      // call callback and replace with result.
      // Everything in range [pTemplateStart, pTemplateEnd] can be safely replaced with parameter value.
      // Data after pTemplateEnd may need to be moved.
      // The first byte of data after placeholder is located at pTemplateEnd + 1.
      // It should be located at pTemplateStart + numBytesCopied (to begin right after inserted parameter value).
      const String paramValue(_callback(paramName));
      const char *pvstr = paramValue.c_str();
      const unsigned int pvlen = paramValue.length();
      const size_t numBytesCopied = std::min(pvlen, static_cast<unsigned int>(&data[originalLen - 1] - pTemplateStart + 1));
      // make room for param value
      // 1. move extra data to cache if parameter value is longer than placeholder AND if there is no room to store
      if ((pTemplateEnd + 1 < pTemplateStart + numBytesCopied) && (originalLen - (pTemplateStart + numBytesCopied - pTemplateEnd - 1) < len)) {
        _cache.insert(_cache.begin(), &data[originalLen - (pTemplateStart + numBytesCopied - pTemplateEnd - 1)], &data[len]);
        // 2. parameter value is longer than placeholder text, push the data after placeholder which not saved into cache further to the end
        memmove(pTemplateStart + numBytesCopied, pTemplateEnd + 1, &data[originalLen] - pTemplateStart - numBytesCopied);
        len = originalLen;  // fix issue with truncated data, not sure if it has any side effects
      } else if (pTemplateEnd + 1 != pTemplateStart + numBytesCopied) {
        // 2. Either parameter value is shorter than placeholder text OR there is enough free space in buffer to fit.
        //    Move the entire data after the placeholder
        memmove(pTemplateStart + numBytesCopied, pTemplateEnd + 1, &data[len] - pTemplateEnd - 1);
      }
      // 3. replace placeholder with actual value
      memcpy(pTemplateStart, pvstr, numBytesCopied);
      // If result is longer than buffer, copy the remainder into cache (this could happen only if placeholder text itself did not fit entirely in buffer)
      if (numBytesCopied < pvlen) {
        _cache.insert(_cache.begin(), pvstr + numBytesCopied, pvstr + pvlen);
      } else if (pTemplateStart + numBytesCopied < pTemplateEnd + 1) {  // result is copied fully; if result is shorter than placeholder text...
        // there is some free room, fill it from cache
        const size_t roomFreed = pTemplateEnd + 1 - pTemplateStart - numBytesCopied;
        const size_t totalFreeRoom = originalLen - len + roomFreed;
        len += _readDataFromCacheOrContent(&data[len - roomFreed], totalFreeRoom) - roomFreed;
      } else {  // result is copied fully; it is longer than placeholder text
        const size_t roomTaken = pTemplateStart + numBytesCopied - pTemplateEnd - 1;
        len = std::min(len + roomTaken, originalLen);
      }
    }
  }  // while(pTemplateStart)
  return len;
}

/*
 * File Response
 * */

/**
 * @brief Sets the content type based on the file path extension
 *
 * This method determines the appropriate MIME content type for a file based on its
 * file extension. It supports both external content type functions (if available)
 * and an internal mapping of common file extensions to their corresponding MIME types.
 *
 * @param path The file path string from which to extract the extension
 * @note The method modifies the internal _contentType member variable
 */
void AsyncFileResponse::_setContentTypeFromPath(const String &path) {
#if HAVE_EXTERN_GET_Content_Type_FUNCTION
#ifndef ESP8266
  extern const char *getContentType(const String &path);
#else
  extern const __FlashStringHelper *getContentType(const String &path);
#endif
  _contentType = getContentType(path);
#else
  const char *cpath = path.c_str();
  const char *dot = strrchr(cpath, '.');

  if (!dot) {
    _contentType = T_application_octet_stream;
    return;
  }

  if (strcmp(dot, T__html) == 0 || strcmp(dot, T__htm) == 0) {
    _contentType = T_text_html;
  } else if (strcmp(dot, T__css) == 0) {
    _contentType = T_text_css;
  } else if (strcmp(dot, T__js) == 0 || strcmp(dot, T__mjs) == 0) {
    _contentType = T_text_javascript;
  } else if (strcmp(dot, T__json) == 0) {
    _contentType = T_application_json;
  } else if (strcmp(dot, T__png) == 0) {
    _contentType = T_image_png;
  } else if (strcmp(dot, T__ico) == 0) {
    _contentType = T_image_x_icon;
  } else if (strcmp(dot, T__svg) == 0) {
    _contentType = T_image_svg_xml;
  } else if (strcmp(dot, T__jpg) == 0) {
    _contentType = T_image_jpeg;
  } else if (strcmp(dot, T__webp) == 0) {
    _contentType = T_image_webp;
  } else if (strcmp(dot, T__avif) == 0) {
    _contentType = T_image_avif;
  } else if (strcmp(dot, T__gif) == 0) {
    _contentType = T_image_gif;
  } else if (strcmp(dot, T__woff2) == 0) {
    _contentType = T_font_woff2;
  } else if (strcmp(dot, T__woff) == 0) {
    _contentType = T_font_woff;
  } else if (strcmp(dot, T__ttf) == 0) {
    _contentType = T_font_ttf;
  } else if (strcmp(dot, T__xml) == 0) {
    _contentType = T_text_xml;
  } else if (strcmp(dot, T__pdf) == 0) {
    _contentType = T_application_pdf;
  } else if (strcmp(dot, T__mp4) == 0) {
    _contentType = T_video_mp4;
  } else if (strcmp(dot, T__opus) == 0) {
    _contentType = T_audio_opus;
  } else if (strcmp(dot, T__webm) == 0) {
    _contentType = T_video_webm;
  } else if (strcmp(dot, T__txt) == 0) {
    _contentType = T_text_plain;
  } else {
    _contentType = T_application_octet_stream;
  }
#endif
}

/**
 * @brief Constructor for AsyncFileResponse that handles file serving with compression support
 *
 * This constructor creates an AsyncFileResponse object that can serve files from a filesystem,
 * with automatic fallback to gzip-compressed versions if the original file is not found.
 * It also handles ETag generation for caching and supports both inline and download modes.
 *
 * @param fs Reference to the filesystem object used to open files
 * @param path Path to the file to be served (without compression extension)
 * @param contentType MIME type of the file content (empty string for auto-detection)
 * @param download If true, file will be served as download attachment; if false, as inline content
 * @param callback Template processor callback for dynamic content processing
 */
AsyncFileResponse::AsyncFileResponse(FS &fs, const String &path, const char *contentType, bool download, AwsTemplateProcessor callback)
  : AsyncAbstractResponse(callback) {

  // Try to open the uncompressed version first
  _content = fs.open(path, fs::FileOpenMode::read);
  if (!_content.available()) {
    // If not available try to open the compressed version (.gz)
    String gzPath;
    uint16_t pathLen = path.length();
    gzPath.reserve(pathLen + 3);
    gzPath.concat(path);
    gzPath.concat(asyncsrv::T__gz);
    _content = fs.open(gzPath, fs::FileOpenMode::read);

    char serverETag[11];
    if (AsyncWebServerRequest::_getEtag(_content, serverETag)) {
      addHeader(T_Content_Encoding, T_gzip, false);
      _callback = nullptr;  // Unable to process zipped templates
      _sendContentLength = true;
      _chunked = false;

      // Add ETag and cache headers
      addHeader(T_ETag, serverETag, true);
      addHeader(T_Cache_Control, T_no_cache, true);

      _content.seek(0);
    } else {
      // File is corrupted or invalid
      _code = 404;
      return;
    }
  }

  _contentLength = _content.size();

  if (*contentType == '\0') {
    _setContentTypeFromPath(path);
  } else {
    _contentType = contentType;
  }

  if (download) {
    // Extract filename from path and set as download attachment
    int filenameStart = path.lastIndexOf('/') + 1;
    const char *filename = path.c_str() + filenameStart;
    String buf;
    buf.reserve(sizeof(T_attachment) - 1 + strlen(filename) + 2);
    buf = T_attachment;
    buf += filename;
    buf += "\"";
    addHeader(T_Content_Disposition, buf, false);
  } else {
    // Serve file inline (display in browser)
    addHeader(T_Content_Disposition, T_inline, false);
  }

  _code = 200;
}

AsyncFileResponse::AsyncFileResponse(File content, const String &path, const char *contentType, bool download, AwsTemplateProcessor callback)
  : AsyncAbstractResponse(callback) {
  _code = 200;

  if (String(content.name()).endsWith(T__gz) && !path.endsWith(T__gz)) {
    addHeader(T_Content_Encoding, T_gzip, false);
    _callback = nullptr;  // Unable to process gzipped templates
    _sendContentLength = true;
    _chunked = false;
  }

  _content = content;
  _contentLength = _content.size();

  if (*contentType == '\0') {
    _setContentTypeFromPath(path);
  } else {
    _contentType = contentType;
  }

  if (download) {
    // Extract filename from path and set as download attachment
    int filenameStart = path.lastIndexOf('/') + 1;
    const char *filename = path.c_str() + filenameStart;
    String buf;
    buf.reserve(sizeof(T_attachment) - 1 + strlen(filename) + 2);
    buf = T_attachment;
    buf += filename;
    buf += "\"";
    addHeader(T_Content_Disposition, buf, false);
  } else {
    // Serve file inline (display in browser)
    addHeader(T_Content_Disposition, T_inline, false);
  }
}

size_t AsyncFileResponse::_fillBuffer(uint8_t *data, size_t len) {
  return _content.read(data, len);
}

/*
 * Stream Response
 * */

AsyncStreamResponse::AsyncStreamResponse(Stream &stream, const char *contentType, size_t len, AwsTemplateProcessor callback) : AsyncAbstractResponse(callback) {
  _code = 200;
  _content = &stream;
  _contentLength = len;
  _contentType = contentType;
}

size_t AsyncStreamResponse::_fillBuffer(uint8_t *data, size_t len) {
  size_t available = _content->available();
  size_t outLen = (available > len) ? len : available;
  size_t i;
  for (i = 0; i < outLen; i++) {
    data[i] = _content->read();
  }
  return outLen;
}

/*
 * Callback Response
 * */

AsyncCallbackResponse::AsyncCallbackResponse(const char *contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback)
  : AsyncAbstractResponse(templateCallback) {
  _code = 200;
  _content = callback;
  _contentLength = len;
  if (!len) {
    _sendContentLength = false;
  }
  _contentType = contentType;
  _filledLength = 0;
}

size_t AsyncCallbackResponse::_fillBuffer(uint8_t *data, size_t len) {
  size_t ret = _content(data, len, _filledLength);
  if (ret != RESPONSE_TRY_AGAIN) {
    _filledLength += ret;
  }
  return ret;
}

/*
 * Chunked Response
 * */

AsyncChunkedResponse::AsyncChunkedResponse(const char *contentType, AwsResponseFiller callback, AwsTemplateProcessor processorCallback)
  : AsyncAbstractResponse(processorCallback) {
  _code = 200;
  _content = callback;
  _contentLength = 0;
  _contentType = contentType;
  _sendContentLength = false;
  _chunked = true;
  _filledLength = 0;
}

size_t AsyncChunkedResponse::_fillBuffer(uint8_t *data, size_t len) {
  size_t ret = _content(data, len, _filledLength);
  if (ret != RESPONSE_TRY_AGAIN) {
    _filledLength += ret;
  }
  return ret;
}

/*
 * Progmem Response
 * */

AsyncProgmemResponse::AsyncProgmemResponse(int code, const char *contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback)
  : AsyncAbstractResponse(callback), _content(content), _index(0) {
  _code = code;
  _contentType = contentType;
  _contentLength = len;
}

size_t AsyncProgmemResponse::_fillBuffer(uint8_t *data, size_t len) {
  size_t read_size = std::min(len, _contentLength - _index);
  memcpy_P(data, _content + _index, read_size);
  _index += read_size;
  return read_size;
}

/*
 * Response Stream (You can print/write/printf to it, up to the contentLen bytes)
 * */

AsyncResponseStream::AsyncResponseStream(const char *contentType, size_t bufferSize) {
  _code = 200;
  _contentLength = 0;
  _contentType = contentType;
  // internal buffer will be null on allocation failure
  _content = std::unique_ptr<cbuf>(new cbuf(bufferSize));
  if (bufferSize && _content->size() < bufferSize) {
    async_ws_log_e("Failed to allocate");
  }
}

size_t AsyncResponseStream::_fillBuffer(uint8_t *buf, size_t maxLen) {
  return _content->read((char *)buf, maxLen);
}

size_t AsyncResponseStream::write(const uint8_t *data, size_t len) {
  if (_started()) {
    return 0;
  }
  if (len > _content->room()) {
    size_t needed = len - _content->room();
    _content->resizeAdd(needed);
    // log a warning if allocation failed, but do not return: keep writing the bytes we can
    // with _content->write: if len is more than the available size in the buffer, only
    // the available size will be written
    if (len > _content->room()) {
      async_ws_log_e("Failed to allocate");
    }
  }
  size_t written = _content->write((const char *)data, len);
  _contentLength += written;
  return written;
}

size_t AsyncResponseStream::write(uint8_t data) {
  return write(&data, 1);
}
