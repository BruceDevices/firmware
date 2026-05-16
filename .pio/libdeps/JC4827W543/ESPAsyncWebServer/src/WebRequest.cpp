// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include "ESPAsyncWebServer.h"
#include "WebAuthentication.h"
#include "WebResponseImpl.h"
#include "AsyncWebServerLogging.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>

#include "./literals.h"

static inline bool isParamChar(char c) {
  return ((c) && ((c) != '{') && ((c) != '[') && ((c) != '&') && ((c) != '='));
}

static void doNotDelete(AsyncWebServerRequest *) {}

using namespace asyncsrv;

enum {
  PARSE_REQ_START = 0,
  PARSE_REQ_HEADERS = 1,
  PARSE_REQ_BODY = 2,
  PARSE_REQ_END = 3,
  PARSE_REQ_FAIL = 4
};

enum {
  CHUNK_NONE = 0,   // Body transfer encoding is not chunked
  CHUNK_LENGTH,     // Getting chunk length - HHHH[;...] CR LF
  CHUNK_EXTENSION,  // Getting chunk extension - ;... CR LF
  CHUNK_DATA,       // Handling chunk data
  CHUNK_ERROR,      // Invalid chunk header
  CHUNK_END,        // Getting chunk end marker  - CR LF
};

AsyncWebServerRequest::AsyncWebServerRequest(AsyncWebServer *s, AsyncClient *c)
  : _client(c), _server(s), _handler(NULL), _response(NULL), _onDisconnectfn(NULL), _temp(), _parseState(PARSE_REQ_START), _version(0),
    _method(AsyncWebRequestMethod::HTTP_UNKNOWN), _url(), _host(), _contentType(), _boundary(), _authorization(), _reqconntype(RCT_HTTP),
    _authMethod(AsyncAuthType::AUTH_NONE), _isMultipart(false), _isPlainPost(false), _expectingContinue(false), _contentLength(0), _parsedLength(0),
    _multiParseState(0), _boundaryPosition(0), _itemStartIndex(0), _itemSize(0), _itemName(), _itemFilename(), _itemType(), _itemValue(), _itemBuffer(0),
    _itemBufferIndex(0), _itemIsFile(false), _chunkStartIndex(0), _chunkOffset(0), _chunkSize(0), _chunkedParseState(CHUNK_NONE), _chunkedLastChar(0),
    _tempObject(NULL) {
  c->onError(
    [](void *r, AsyncClient *c, int8_t error) {
      (void)c;
      // async_ws_log_e("AsyncWebServerRequest::_onError");
      static_cast<AsyncWebServerRequest *>(r)->_onError(error);
    },
    this
  );
  c->onAck(
    [](void *r, AsyncClient *c, size_t len, uint32_t time) {
      (void)c;
      // async_ws_log_e("AsyncWebServerRequest::_onAck");
      static_cast<AsyncWebServerRequest *>(r)->_onAck(len, time);
    },
    this
  );
  c->onDisconnect(
    [](void *r, AsyncClient *c) {
      // async_ws_log_e("AsyncWebServerRequest::_onDisconnect");
      static_cast<AsyncWebServerRequest *>(r)->_onDisconnect();
    },
    this
  );
  c->onTimeout(
    [](void *r, AsyncClient *c, uint32_t time) {
      (void)c;
      // async_ws_log_e("AsyncWebServerRequest::_onTimeout");
      static_cast<AsyncWebServerRequest *>(r)->_onTimeout(time);
    },
    this
  );
  c->onData(
    [](void *r, AsyncClient *c, void *buf, size_t len) {
      (void)c;
      // async_ws_log_e("AsyncWebServerRequest::_onData");
      static_cast<AsyncWebServerRequest *>(r)->_onData(buf, len);
    },
    this
  );
  c->onPoll(
    [](void *r, AsyncClient *c) {
      (void)c;
      // async_ws_log_e("AsyncWebServerRequest::_onPoll");
      static_cast<AsyncWebServerRequest *>(r)->_onPoll();
    },
    this
  );
}

AsyncWebServerRequest::~AsyncWebServerRequest() {
  if (_client) {
    // usually it is _client's disconnect triggers object destruct, but for completeness we define behavior
    // if for some reason *this will be destructed while client is still connected
    _client->onDisconnect(nullptr);
    delete _client;
    _client = nullptr;
  }

  if (_response) {
    delete _response;
    _response = nullptr;
  }

  _this.reset();

  if (_tempObject != NULL) {
    free(_tempObject);
  }

  if (_tempFile) {
    _tempFile.close();
  }

  if (_itemBuffer) {
    free(_itemBuffer);
  }
}

void AsyncWebServerRequest::_onData(void *buf, size_t len) {
  // SSL/TLS handshake detection
#ifndef ASYNC_TCP_SSL_ENABLED
  if (_parseState == PARSE_REQ_START && len && ((uint8_t *)buf)[0] == 0x16) {  // 0x16 indicates a Handshake message (SSL/TLS).
    async_ws_log_d("SSL/TLS handshake detected: resetting connection");
    _parseState = PARSE_REQ_FAIL;
    abort();
    return;
  }
#endif

  size_t i = 0;
  while (true) {

    if (_parseState < PARSE_REQ_BODY) {
      // Find new line in buf
      char *str = (char *)buf;
      for (i = 0; i < len; i++) {
        // Check for null characters in header
        if (!str[i]) {
          _parseState = PARSE_REQ_FAIL;
          abort();
          return;
        }
        if (str[i] == '\n') {
          break;
        }
      }
      if (i == len) {  // No new line, just add the buffer in _temp
        char ch = str[len - 1];
        str[len - 1] = 0;
        if (!_temp.reserve(_temp.length() + len)) {
          async_ws_log_e("Failed to allocate");
          _parseState = PARSE_REQ_FAIL;
          abort();
          return;
        }
        _temp.concat(str);
        _temp.concat(ch);
      } else {       // Found new line - extract it and parse
        str[i] = 0;  // Terminate the string at the end of the line.
        _temp.concat(str);
        _temp.trim();
        _parseLine();
        if (++i < len) {
          // Still have more buffer to process
          buf = str + i;
          len -= i;
          continue;
        }
      }
    } else if (_parseState == PARSE_REQ_BODY) {
      if (_chunkedParseState != CHUNK_NONE) {
        if (_parseChunkedBytes((uint8_t *)buf, len)) {
          _parseState = PARSE_REQ_END;
          _runMiddlewareChain();
          _send();
        }
        break;
      }
      // A handler should be already attached at this point in _parseLine function.
      // If handler does nothing (_onRequest is NULL), we don't need to really parse the body.
      const bool needParse = _handler && !_handler->isRequestHandlerTrivial();
      // Discard any bytes after content length; handlers may overrun their buffers
      len = std::min(len, _contentLength - _parsedLength);
      if (_isMultipart) {
        if (needParse) {
          size_t i;
          for (i = 0; i < len; i++) {
            _parseMultipartPostByte(((uint8_t *)buf)[i], i == len - 1);
            _parsedLength++;
          }
        } else {
          _parsedLength += len;
        }
      } else {
        if (_parsedLength == 0) {
          if (_contentType.startsWith(T_app_xform_urlencoded)) {
            _isPlainPost = true;
          } else if (_contentType == T_text_plain && isParamChar(((char *)buf)[0])) {
            size_t i = 0;
            char ch;
            do {
              ch = ((char *)buf)[i];
            } while (i++ < len && isParamChar(ch));
            if (i < len && ((char *)buf)[i - 1] == '=') {
              _isPlainPost = true;
            }
          }
        }
        if (!_isPlainPost) {
          // ESP_LOGD("AsyncWebServer", "_isPlainPost: %d, _handler: %p", _isPlainPost, _handler);
          if (_handler) {
            _handler->handleBody(this, (uint8_t *)buf, len, _parsedLength, _contentLength);
          }
          _parsedLength += len;
        } else if (needParse) {
          size_t i;
          for (i = 0; i < len; i++) {
            _parsedLength++;
            _parsePlainPostChar(((uint8_t *)buf)[i]);
          }
        } else {
          _parsedLength += len;
        }
      }
      if (_parsedLength == _contentLength) {
        _parseState = PARSE_REQ_END;
        _runMiddlewareChain();
        _send();
      }
    }
    break;
  }
}

void AsyncWebServerRequest::_onPoll() {
  // os_printf("p\n");
  if (_response && _client && _client->canSend()) {
    _response->_ack(this, 0, 0);
  }
}

void AsyncWebServerRequest::_onAck(size_t len, uint32_t time) {
  // os_printf("a:%u:%u\n", len, time);
  if (!_response) {
    return;
  }

  if (!_response->_finished()) {
    _response->_ack(this, len, time);
    // recheck if response has just completed, close connection
    if (_response->_finished()) {
      _client->close();  // this will trigger _onDisconnect() and object destruction
    }
  } else {
    // this will close responses that were complete via a single _send() call
    _client->close();  // this will trigger _onDisconnect() and object destruction
  }
}

void AsyncWebServerRequest::_onError(int8_t error) {
  (void)error;
}

void AsyncWebServerRequest::_onTimeout(uint32_t time) {
  (void)time;
  // os_printf("TIMEOUT: %u, state: %s\n", time, _client->stateToString());
  _client->close();
}

void AsyncWebServerRequest::onDisconnect(ArDisconnectHandler fn) {
  _onDisconnectfn = fn;
}

void AsyncWebServerRequest::_onDisconnect() {
  // os_printf("d\n");
  if (_onDisconnectfn) {
    _onDisconnectfn();
  }
  _server->_handleDisconnect(this);
}

void AsyncWebServerRequest::_addGetParams(const String &params) {
  size_t start = 0;
  while (start < params.length()) {
    int end = params.indexOf('&', start);
    if (end < 0) {
      end = params.length();
    }
    int equal = params.indexOf('=', start);
    if (equal < 0 || equal > end) {
      equal = end;
    }
    String name = urlDecode(params.substring(start, equal));
    String value = urlDecode(equal + 1 < end ? params.substring(equal + 1, end) : asyncsrv::emptyString);
    if (name.length()) {
      _params.emplace_back(name, value);
    }
    start = end + 1;
  }
}

bool AsyncWebServerRequest::_parseReqHead() {
  // Split the head into method, url and version
  int index = _temp.indexOf(' ');
  String m = _temp.substring(0, index);
  index = _temp.indexOf(' ', index + 1);
  String u = _temp.substring(m.length() + 1, index);
  _temp = _temp.substring(index + 1);

  _method = asyncsrv::stringToMethod(m);
  if (_method == AsyncWebRequestMethod::HTTP_INVALID) {
    return false;
  }

  String g;
  index = u.indexOf('?');
  if (index > 0) {
    g = u.substring(index + 1);
    u = u.substring(0, index);
  }
  _url = urlDecode(u);
  _addGetParams(g);

  if (!_url.length()) {
    return false;
  }

  if (!_temp.startsWith(T_HTTP_1_0)) {
    _version = 1;
  }

  _temp = asyncsrv::emptyString;
  return true;
}

// Returns true when done
bool AsyncWebServerRequest::_parseChunkedBytes(uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len;) {
    if (_chunkedParseState == CHUNK_DATA) {
      // In DATA state, we pass the bytes off to handleBody as a group

      // In order to avoid allocating an extra buffer, the data
      // blocks that we pass on do not necessarily correspond to
      // whole chunks.  We just send however much we already have,
      // anticipating that more will arrive later.  handleBody()
      // cannot assume that it receives entire chunks at once.
      // That should not be a problem because we do not attach
      // any semantic meaning to chunks.  That might change if
      // we were to support chunk extensions, but that seems
      // unlikely since RFC9112 suggests that they are only
      // useful for very specialized purposes.
      size_t curLen = std::min(_chunkSize - _chunkOffset, len - i);

      // On the final zero-length chunk, _chunkSize - _chunkOffset
      // will be zero, so we will call handleBody with a zero size,
      // marking the end of the data stream.

      if (_handler) {
        _handler->handleBody(this, buf + i, curLen, _chunkStartIndex, _contentLength);
      }
      _chunkOffset += curLen;
      _chunkStartIndex += curLen;
      i += curLen;
      if (_chunkOffset == _chunkSize) {
        _chunkedParseState = CHUNK_END;
      }
    } else {
      // In other states we process the bytes one by one
      uint8_t data = buf[i++];

      auto last_was_cr = _chunkedLastChar == '\r';
      _chunkedLastChar = data;

      if (_chunkedParseState == CHUNK_LENGTH) {
        // Incrementally decode a hex number
        if (data >= '0' && data <= '9') {
          if (_chunkSize >= 0x1000000) {
            _chunkedParseState = CHUNK_ERROR;
          } else {
            _chunkSize = (_chunkSize * 16) + (data - '0');
          }
        } else if (data >= 'A' && data <= 'F') {
          if (_chunkSize >= 0x1000000) {
            _chunkedParseState = CHUNK_ERROR;
          } else {
            _chunkSize = (_chunkSize * 16) + (data - 'A' + 10);
          }
        } else if (data >= 'a' && data <= 'f') {
          if (_chunkSize >= 0x1000000) {
            _chunkedParseState = CHUNK_ERROR;
          } else {
            _chunkSize = (_chunkSize * 16) + (data - 'a' + 10);
          }
        } else if (data == ';') {
          _chunkedParseState = CHUNK_EXTENSION;
        } else if (data == '\r') {
          // Wait for LF
        } else if (data == '\n') {
          if (last_was_cr) {
            _chunkOffset = 0;
            _chunkedParseState = CHUNK_DATA;

          } else {
            _chunkedParseState = CHUNK_ERROR;
          }
        } else {
          // Invalid hex character
          _chunkedParseState = CHUNK_ERROR;
        }
      } else if (_chunkedParseState == CHUNK_EXTENSION) {
        // Chunk extensions appear after a semicolon.
        // We ignore them because their use cases are
        // specialized and obscure.
        if (data == '\r') {
          // Wait for LF
        } else if (data == '\n') {
          if (last_was_cr) {
            _chunkOffset = 0;
            _chunkedParseState = CHUNK_DATA;
          } else {
            _chunkedParseState = CHUNK_ERROR;
          }
        }
      } else if (_chunkedParseState == CHUNK_END) {
        if (data == '\r') {
          // Wait for LF
        } else if (data == '\n') {
          if (last_was_cr) {
            // A zero length chunk marks the end of the chunk stream
            if (_chunkSize == 0) {
              // If we needed to support trailers, we would switch to
              // TRAILER state, but since we have no use case for them,
              // we just stop processing the body.
              return true;
            }
            _chunkSize = 0;
            _chunkedParseState = CHUNK_LENGTH;
          } else {
            _chunkedParseState = CHUNK_ERROR;
          }
        }
      }

      if (_chunkedParseState == CHUNK_ERROR) {
        // If there was an error when parsing the chunk length, the
        // rest of the data stream is unreliable.  Ideally we should
        // close the connection, but that risks leaving things dangling
        // (e.g. an open file), so it is probably best to just ignore
        // the rest of the data and give handleRequest a chance to
        // clean up.
        _chunkSize = 0;
        abort();
        return true;
      }
    }
  }
  return false;
}

bool AsyncWebServerRequest::_parseReqHeader() {
  AsyncWebHeader header = AsyncWebHeader::parse(_temp);
  if (header) {
    const String &name = header.name();
    const String &value = header.value();
    if (name.equalsIgnoreCase(T_Host)) {
      _host = value;
    } else if (name.equalsIgnoreCase(T_Content_Type)) {
      _contentType = value.substring(0, value.indexOf(';'));
      if (value.startsWith(T_MULTIPART_)) {
        _boundary = value.substring(value.indexOf('=') + 1);
        _boundary.replace(String('"'), String());
        _isMultipart = true;
      }
    } else if (name.equalsIgnoreCase(T_Content_Length) || name.equalsIgnoreCase(T_X_Expected_Entity_Length)) {
      // MacOS WebDAVFS uses X-Expected-Entity-Length to indicate the
      // total length of a chunked request body.  It is useful to
      // determine if a PUT can possibly fit in the available space.
      _contentLength = atoi(value.c_str());
    } else if (name.equalsIgnoreCase(T_EXPECT) && value.equalsIgnoreCase(T_100_CONTINUE)) {
      _expectingContinue = true;
    } else if (name.equalsIgnoreCase(T_AUTH)) {
      int space = value.indexOf(' ');
      if (space == -1) {
        _authorization = value;
        _authMethod = AsyncAuthType::AUTH_OTHER;
      } else {
        String method = value.substring(0, space);
        if (method.equalsIgnoreCase(T_BASIC)) {
          _authMethod = AsyncAuthType::AUTH_BASIC;
        } else if (method.equalsIgnoreCase(T_DIGEST)) {
          _authMethod = AsyncAuthType::AUTH_DIGEST;
        } else if (method.equalsIgnoreCase(T_BEARER)) {
          _authMethod = AsyncAuthType::AUTH_BEARER;
        } else {
          _authMethod = AsyncAuthType::AUTH_OTHER;
        }
        _authorization = value.substring(space + 1);
      }
    } else if (name.equalsIgnoreCase(T_UPGRADE) && value.equalsIgnoreCase(T_WS)) {
      // WebSocket request can be uniquely identified by header: [Upgrade: websocket]
      _reqconntype = RCT_WS;
    } else if (name.equalsIgnoreCase(T_ACCEPT)) {
      String lowcase(value);
      lowcase.toLowerCase();
#ifndef ESP8266
      const char *substr = std::strstr(lowcase.c_str(), T_text_event_stream);
#else
      const char *substr = std::strstr(lowcase.c_str(), String(T_text_event_stream).c_str());
#endif
      if (substr != NULL) {
        // WebEvent request can be uniquely identified by header:  [Accept: text/event-stream]
        _reqconntype = RCT_EVENT;
      }
    } else if (name.equalsIgnoreCase(T_Transfer_Encoding)) {
      String lowcase(value);
      lowcase.toLowerCase();
      String key;

      while (lowcase.length()) {
        auto pos = lowcase.indexOf(',');
        if (pos >= 0) {
          key = lowcase.substring(0, pos);
          lowcase = lowcase.substring(pos + 1);
        } else {
          key = lowcase;
          lowcase = "";
        }
        key.trim();
        if (key == "chunked") {
          _chunkSize = 0;
          _chunkStartIndex = 0;
          _chunkedParseState = CHUNK_LENGTH;
          break;
        }
      }
    }
    _headers.emplace_back(std::move(header));
  }
#if defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350) || defined(LIBRETINY) || defined(HOST)
  // ArduinoCore-API does not have String::clear() method 8-()
  _temp = asyncsrv::emptyString;
#else
  _temp.clear();
#endif
  return true;
}

void AsyncWebServerRequest::_parsePlainPostChar(uint8_t data) {
  if (data && (char)data != '&') {
    _temp += (char)data;
  }
  if (!data || (char)data == '&' || _parsedLength == _contentLength) {
    String name(T_BODY);
    String value(_temp);
    if (!(_temp.charAt(0) == '{') && !(_temp.charAt(0) == '[') && _temp.indexOf('=') > 0) {
      name = _temp.substring(0, _temp.indexOf('='));
      value = _temp.substring(_temp.indexOf('=') + 1);
    }
    name = urlDecode(name);
    if (name.length()) {
      _params.emplace_back(name, urlDecode(value), true);
    }

#if defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350) || defined(LIBRETINY) || defined(HOST)
    // ArduinoCore-API does not have String::clear() method 8-()
    _temp = asyncsrv::emptyString;
#else
    _temp.clear();
#endif
  }
}

void AsyncWebServerRequest::_handleUploadByte(uint8_t data, bool last) {
  _itemBuffer[_itemBufferIndex++] = data;

  if (last || _itemBufferIndex == RESPONSE_STREAM_BUFFER_SIZE) {
    // check if authenticated before calling the upload
    if (_handler) {
      _handler->handleUpload(this, _itemFilename, _itemSize - _itemBufferIndex, _itemBuffer, _itemBufferIndex, false);
    }
    _itemBufferIndex = 0;
  }
}

enum {
  EXPECT_BOUNDARY,
  PARSE_HEADERS,
  WAIT_FOR_RETURN1,
  EXPECT_FEED1,
  EXPECT_DASH1,
  EXPECT_DASH2,
  BOUNDARY_OR_DATA,
  DASH3_OR_RETURN2,
  EXPECT_FEED2,
  PARSING_FINISHED,
  PARSE_ERROR
};

void AsyncWebServerRequest::_parseMultipartPostByte(uint8_t data, bool last) {
#define itemWriteByte(b)          \
  do {                            \
    _itemSize++;                  \
    if (_itemIsFile)              \
      _handleUploadByte(b, last); \
    else                          \
      _itemValue += (char)(b);    \
  } while (0)

  if (!_parsedLength) {
    _multiParseState = EXPECT_BOUNDARY;
    _temp = asyncsrv::emptyString;
    _itemName = asyncsrv::emptyString;
    _itemFilename = asyncsrv::emptyString;
    _itemType = asyncsrv::emptyString;
  }

  if (_multiParseState == WAIT_FOR_RETURN1) {
    if (data != '\r') {
      itemWriteByte(data);
    } else {
      _multiParseState = EXPECT_FEED1;
    }
  } else if (_multiParseState == EXPECT_BOUNDARY) {
    if (_parsedLength < 2 && data != '-') {
      _multiParseState = PARSE_ERROR;
      return;
    } else if (_parsedLength - 2 < _boundary.length() && _boundary.c_str()[_parsedLength - 2] != data) {
      _multiParseState = PARSE_ERROR;
      return;
    } else if (_parsedLength - 2 == _boundary.length() && data != '\r') {
      _multiParseState = PARSE_ERROR;
      return;
    } else if (_parsedLength - 3 == _boundary.length()) {
      if (data != '\n') {
        _multiParseState = PARSE_ERROR;
        return;
      }
      _multiParseState = PARSE_HEADERS;
      _itemIsFile = false;
    }
  } else if (_multiParseState == PARSE_HEADERS) {
    if ((char)data != '\r' && (char)data != '\n') {
      _temp += (char)data;
    }
    if ((char)data == '\n') {
      if (_temp.length()) {
        if (_temp.length() > 12 && _temp.substring(0, 12).equalsIgnoreCase(T_Content_Type)) {
          _itemType = _temp.substring(14);
          _itemIsFile = true;
        } else if (_temp.length() > 19 && _temp.substring(0, 19).equalsIgnoreCase(T_Content_Disposition)) {
          _temp = _temp.substring(_temp.indexOf(';') + 2);
          while (_temp.indexOf(';') > 0) {
            String name = _temp.substring(0, _temp.indexOf('='));
            String nameVal = _temp.substring(_temp.indexOf('=') + 2, _temp.indexOf(';') - 1);
            if (name == T_name) {
              _itemName = nameVal;
            } else if (name == T_filename) {
              _itemFilename = nameVal;
              _itemIsFile = true;
            }
            _temp = _temp.substring(_temp.indexOf(';') + 2);
          }
          String name = _temp.substring(0, _temp.indexOf('='));
          String nameVal = _temp.substring(_temp.indexOf('=') + 2, _temp.length() - 1);
          if (name == T_name) {
            _itemName = nameVal;
          } else if (name == T_filename) {
            _itemFilename = nameVal;
            _itemIsFile = true;
          }
          // Add the parameters from the content-disposition header to the param list, flagged as POST and File,
          // so that they can be retrieved using getParam(name, isPost=true, isFile=true)
          // in the upload handler to correctly handle multiple file uploads within the same request.
          // Example: Content-Disposition: form-data; name="fw"; filename="firmware.bin"
          // See: https://github.com/ESP32Async/ESPAsyncWebServer/discussions/328
          if (_itemIsFile && _itemName.length() && _itemFilename.length()) {
            // add new parameters for this content-disposition
            _params.emplace_back(T_name, _itemName, true, true);
            _params.emplace_back(T_filename, _itemFilename, true, true);
          }
        }
        _temp = asyncsrv::emptyString;
      } else {
        _multiParseState = WAIT_FOR_RETURN1;
        // value starts from here
        _itemSize = 0;
        _itemStartIndex = _parsedLength;
        _itemValue = asyncsrv::emptyString;
        if (_itemIsFile) {
          if (_itemBuffer) {
            free(_itemBuffer);
          }
          _itemBuffer = (uint8_t *)malloc(RESPONSE_STREAM_BUFFER_SIZE);
          if (_itemBuffer == NULL) {
            async_ws_log_e("Failed to allocate");
            _multiParseState = PARSE_ERROR;
            abort();
            return;
          }
          _itemBufferIndex = 0;
        }
      }
    }
  } else if (_multiParseState == EXPECT_FEED1) {
    if (data != '\n') {
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r');
      _parseMultipartPostByte(data, last);
    } else {
      _multiParseState = EXPECT_DASH1;
    }
  } else if (_multiParseState == EXPECT_DASH1) {
    if (data != '-') {
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r');
      itemWriteByte('\n');
      _parseMultipartPostByte(data, last);
    } else {
      _multiParseState = EXPECT_DASH2;
    }
  } else if (_multiParseState == EXPECT_DASH2) {
    if (data != '-') {
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r');
      itemWriteByte('\n');
      itemWriteByte('-');
      _parseMultipartPostByte(data, last);
    } else {
      _multiParseState = BOUNDARY_OR_DATA;
      _boundaryPosition = 0;
    }
  } else if (_multiParseState == BOUNDARY_OR_DATA) {
    if (_boundaryPosition < _boundary.length() && _boundary.c_str()[_boundaryPosition] != data) {
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r');
      itemWriteByte('\n');
      itemWriteByte('-');
      itemWriteByte('-');
      uint8_t i;
      for (i = 0; i < _boundaryPosition; i++) {
        itemWriteByte(_boundary.c_str()[i]);
      }
      _parseMultipartPostByte(data, last);
    } else if (_boundaryPosition == _boundary.length() - 1) {
      _multiParseState = DASH3_OR_RETURN2;
      if (!_itemIsFile) {
        _params.emplace_back(_itemName, _itemValue, true);
      } else {
        if (_handler) {
          _handler->handleUpload(this, _itemFilename, _itemSize - _itemBufferIndex, _itemBuffer, _itemBufferIndex, true);
        }
        _itemBufferIndex = 0;
        _params.emplace_back(_itemName, _itemFilename, true, true, _itemSize);
        // remove previous occurrence(s) of content-disposition parameters for this upload
        _params.remove_if([this](const AsyncWebParameter &p) {
          return p.isPost() && p.isFile() && (p.name() == T_name || p.name() == T_filename);
        });
        free(_itemBuffer);
        _itemBuffer = NULL;
      }

    } else {
      _boundaryPosition++;
    }
  } else if (_multiParseState == DASH3_OR_RETURN2) {
    if (data == '-' && (_contentLength - _parsedLength - 4) != 0) {
      // os_printf("ERROR: The parser got to the end of the POST but is expecting %u bytes more!\nDrop an issue so we can have more info on the matter!\n", _contentLength - _parsedLength - 4);
      _contentLength = _parsedLength + 4;  // lets close the request gracefully
    }
    if (data == '\r') {
      _multiParseState = EXPECT_FEED2;
    } else if (data == '-' && _contentLength == (_parsedLength + 4)) {
      _multiParseState = PARSING_FINISHED;
    } else {
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r');
      itemWriteByte('\n');
      itemWriteByte('-');
      itemWriteByte('-');
      uint8_t i;
      for (i = 0; i < _boundary.length(); i++) {
        itemWriteByte(_boundary.c_str()[i]);
      }
      _parseMultipartPostByte(data, last);
    }
  } else if (_multiParseState == EXPECT_FEED2) {
    if (data == '\n') {
      _multiParseState = PARSE_HEADERS;
      _itemIsFile = false;
    } else {
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r');
      itemWriteByte('\n');
      itemWriteByte('-');
      itemWriteByte('-');
      uint8_t i;
      for (i = 0; i < _boundary.length(); i++) {
        itemWriteByte(_boundary.c_str()[i]);
      }
      itemWriteByte('\r');
      _parseMultipartPostByte(data, last);
    }
  }
}

void AsyncWebServerRequest::_parseLine() {
  if (_parseState == PARSE_REQ_START) {
    if (!_temp.length()) {
      _parseState = PARSE_REQ_FAIL;
      abort();
    } else {
      if (_parseReqHead()) {
        _parseState = PARSE_REQ_HEADERS;
      } else {
        _parseState = PARSE_REQ_FAIL;
        abort();
      }
    }
    return;
  }

  if (_parseState == PARSE_REQ_HEADERS) {
    if (!_temp.length()) {
      // end of headers
      _server->_rewriteRequest(this);
      _server->_attachHandler(this);
      if (_expectingContinue) {
        String response(T_HTTP_100_CONT);
        _client->write(response.c_str(), response.length());
      }
      if (_contentLength || _chunkedParseState != CHUNK_NONE) {
        _parseState = PARSE_REQ_BODY;
      } else {
        _parseState = PARSE_REQ_END;
        _runMiddlewareChain();
        _send();
      }
    } else {
      _parseReqHeader();
    }
  }
}

void AsyncWebServerRequest::_runMiddlewareChain() {
  if (_handler && _handler->mustSkipServerMiddlewares()) {
    _handler->_runChain(this, [this]() {
      _handler->handleRequest(this);
    });
  } else {
    _server->_runChain(this, [this]() {
      if (_handler) {
        _handler->_runChain(this, [this]() {
          _handler->handleRequest(this);
        });
      }
    });
  }
}

void AsyncWebServerRequest::_send() {
  if (!_sent && !_paused) {
    // async_ws_log_d("AsyncWebServerRequest::_send()");

    // user did not create a response ?
    if (!_response) {
      send(501, T_text_plain, "Handler did not handle the request");
    }

    // response is not valid ?
    if (!_response->_sourceValid()) {
      send(500, T_text_plain, "Invalid data in handler");
    }

    // here, we either have a response given from user or one of the two above
    _client->setRxTimeout(0);
    _response->_respond(this);
    _sent = true;
  }
}

AsyncWebServerRequestPtr AsyncWebServerRequest::pause() {
  if (_paused) {
    return _this;
  }
  client()->setRxTimeout(0);
  // this shared ptr will hold the request pointer until it gets destroyed following a disconnect.
  // this is just used as a holder providing weak observers, so the deleter is a no-op.
  _this = std::shared_ptr<AsyncWebServerRequest>(this, doNotDelete);
  _paused = true;
  return _this;
}

void AsyncWebServerRequest::abort() {
  if (!_sent) {
    _sent = true;
    _paused = false;
    _this.reset();
    // async_ws_log_e("AsyncWebServerRequest::abort");
    _client->abort();
  }
}

size_t AsyncWebServerRequest::headers() const {
  return _headers.size();
}

bool AsyncWebServerRequest::hasHeader(const char *name) const {
  for (const auto &h : _headers) {
    if (h.name().equalsIgnoreCase(name)) {
      return true;
    }
  }
  return false;
}

#ifdef ESP8266
bool AsyncWebServerRequest::hasHeader(const __FlashStringHelper *data) const {
  return hasHeader(String(data));
}
#endif

const AsyncWebHeader *AsyncWebServerRequest::getHeader(const char *name) const {
  auto iter = std::find_if(std::begin(_headers), std::end(_headers), [&name](const AsyncWebHeader &header) {
    return header.name().equalsIgnoreCase(name);
  });
  return (iter == std::end(_headers)) ? nullptr : &(*iter);
}

#ifdef ESP8266
const AsyncWebHeader *AsyncWebServerRequest::getHeader(const __FlashStringHelper *data) const {
  PGM_P p = reinterpret_cast<PGM_P>(data);
  size_t n = strlen_P(p);
  char *name = (char *)malloc(n + 1);
  if (name) {
    strcpy_P(name, p);
    const AsyncWebHeader *result = getHeader(String(name));
    free(name);
    return result;
  } else {
    return nullptr;
  }
}
#endif

const AsyncWebHeader *AsyncWebServerRequest::getHeader(size_t num) const {
  if (num >= _headers.size()) {
    return nullptr;
  }
  return &(*std::next(_headers.cbegin(), num));
}

size_t AsyncWebServerRequest::getHeaderNames(std::vector<const char *> &names) const {
  const size_t size = names.size();
  for (const auto &h : _headers) {
    names.push_back(h.name().c_str());
  }
  return names.size() - size;
}

bool AsyncWebServerRequest::removeHeader(const char *name) {
  const size_t size = _headers.size();
  _headers.remove_if([name](const AsyncWebHeader &header) {
    return header.name().equalsIgnoreCase(name);
  });
  return size != _headers.size();
}

size_t AsyncWebServerRequest::params() const {
  return _params.size();
}

bool AsyncWebServerRequest::hasParam(const char *name, bool post, bool file) const {
  for (const auto &p : _params) {
    if (p.name().equals(name) && p.isPost() == post && p.isFile() == file) {
      return true;
    }
  }
  return false;
}

const AsyncWebParameter *AsyncWebServerRequest::getParam(const char *name, bool post, bool file) const {
  for (const auto &p : _params) {
    if (p.name() == name && p.isPost() == post && p.isFile() == file) {
      return &p;
    }
  }
  return nullptr;
}

#ifdef ESP8266
const AsyncWebParameter *AsyncWebServerRequest::getParam(const __FlashStringHelper *data, bool post, bool file) const {
  return getParam(String(data), post, file);
}
#endif

const AsyncWebParameter *AsyncWebServerRequest::getParam(size_t num) const {
  if (num >= _params.size()) {
    return nullptr;
  }
  return &(*std::next(_params.cbegin(), num));
}

const String &AsyncWebServerRequest::getAttribute(const char *name, const String &defaultValue) const {
  auto it = _attributes.find(name);
  return it != _attributes.end() ? it->second : defaultValue;
}
bool AsyncWebServerRequest::getAttribute(const char *name, bool defaultValue) const {
  auto it = _attributes.find(name);
  return it != _attributes.end() ? it->second == "1" : defaultValue;
}
long AsyncWebServerRequest::getAttribute(const char *name, long defaultValue) const {
  auto it = _attributes.find(name);
  return it != _attributes.end() ? it->second.toInt() : defaultValue;
}
float AsyncWebServerRequest::getAttribute(const char *name, float defaultValue) const {
  auto it = _attributes.find(name);
  return it != _attributes.end() ? it->second.toFloat() : defaultValue;
}
double AsyncWebServerRequest::getAttribute(const char *name, double defaultValue) const {
  auto it = _attributes.find(name);
  return it != _attributes.end() ? it->second.toDouble() : defaultValue;
}

AsyncWebServerResponse *AsyncWebServerRequest::beginResponse(int code, const char *contentType, const char *content, AwsTemplateProcessor callback) {
  if (callback) {
    return new AsyncProgmemResponse(code, contentType, (const uint8_t *)content, strlen(content), callback);
  }
  return new AsyncBasicResponse(code, contentType, content);
}

AsyncWebServerResponse *
  AsyncWebServerRequest::beginResponse(int code, const char *contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback) {
  return new AsyncProgmemResponse(code, contentType, content, len, callback);
}

AsyncWebServerResponse *
  AsyncWebServerRequest::beginResponse(FS &fs, const String &path, const char *contentType, bool download, AwsTemplateProcessor callback) {
  if (fs.exists(path) || (!download && fs.exists(path + T__gz))) {
    return new AsyncFileResponse(fs, path, contentType, download, callback);
  }
  return NULL;
}

AsyncWebServerResponse *
  AsyncWebServerRequest::beginResponse(File content, const String &path, const char *contentType, bool download, AwsTemplateProcessor callback) {
  if (content == true) {
    return new AsyncFileResponse(content, path, contentType, download, callback);
  }
  return NULL;
}

AsyncWebServerResponse *AsyncWebServerRequest::beginResponse(Stream &stream, const char *contentType, size_t len, AwsTemplateProcessor callback) {
  return new AsyncStreamResponse(stream, contentType, len, callback);
}

AsyncWebServerResponse *
  AsyncWebServerRequest::beginResponse(const char *contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback) {
  return new AsyncCallbackResponse(contentType, len, callback, templateCallback);
}

AsyncWebServerResponse *
  AsyncWebServerRequest::beginChunkedResponse(const char *contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback) {
  if (_version) {
    return new AsyncChunkedResponse(contentType, callback, templateCallback);
  }
  return new AsyncCallbackResponse(contentType, 0, callback, templateCallback);
}

AsyncResponseStream *AsyncWebServerRequest::beginResponseStream(const char *contentType, size_t bufferSize) {
  return new AsyncResponseStream(contentType, bufferSize);
}

AsyncWebServerResponse *AsyncWebServerRequest::beginResponse_P(int code, const String &contentType, PGM_P content, AwsTemplateProcessor callback) {
  return new AsyncProgmemResponse(code, contentType, (const uint8_t *)content, strlen_P(content), callback);
}

void AsyncWebServerRequest::send(AsyncWebServerResponse *response) {
  // request is already sent on the wire ?
  if (_sent) {
    return;
  }

  // if we already had a response, delete it and replace it with the new one
  if (_response) {
    delete _response;
  }
  _response = response;

  // if request was paused, we need to send the response now
  if (_paused) {
    _paused = false;
    _send();
  }
}

void AsyncWebServerRequest::redirect(const char *url, int code) {
  AsyncWebServerResponse *response = beginResponse(code);
  response->addHeader(T_LOCATION, url);
  send(response);
}

bool AsyncWebServerRequest::authenticate(const char *username, const char *password, const char *realm, bool passwordIsHash) const {
  if (_authorization.length()) {
    if (_authMethod == AsyncAuthType::AUTH_DIGEST) {
      return checkDigestAuthentication(_authorization.c_str(), methodToString(), username, password, realm, passwordIsHash, NULL, NULL, NULL);
    } else if (!passwordIsHash) {
      return checkBasicAuthentication(_authorization.c_str(), username, password);
    } else {
      return _authorization.equals(password);
    }
  }
  return false;
}

bool AsyncWebServerRequest::authenticate(const char *hash) const {
  if (!_authorization.length() || hash == NULL) {
    return false;
  }

  if (_authMethod == AsyncAuthType::AUTH_DIGEST) {
    String hStr = String(hash);
    int separator = hStr.indexOf(':');
    if (separator <= 0) {
      return false;
    }
    String username = hStr.substring(0, separator);
    hStr = hStr.substring(separator + 1);
    separator = hStr.indexOf(':');
    if (separator <= 0) {
      return false;
    }
    String realm = hStr.substring(0, separator);
    hStr = hStr.substring(separator + 1);
    return checkDigestAuthentication(_authorization.c_str(), methodToString(), username.c_str(), hStr.c_str(), realm.c_str(), true, NULL, NULL, NULL);
  }

  // Basic Auth, Bearer Auth, or other
  return (_authorization.equals(hash));
}

void AsyncWebServerRequest::requestAuthentication(AsyncAuthType method, const char *realm, const char *_authFailMsg) {
  if (!realm) {
    realm = T_LOGIN_REQ;
  }

  AsyncWebServerResponse *r = _authFailMsg ? beginResponse(401, T_text_html, _authFailMsg) : beginResponse(401);

  switch (method) {
    case AsyncAuthType::AUTH_BASIC:
    {
      String header;
      if (header.reserve(sizeof(T_BASIC_REALM) - 1 + strlen(realm) + 1)) {
        header.concat(T_BASIC_REALM);
        header.concat(realm);
        header.concat('"');
        r->addHeader(T_WWW_AUTH, header.c_str());
      } else {
        async_ws_log_e("Failed to allocate");
        abort();
      }

      break;
    }
    case AsyncAuthType::AUTH_DIGEST:
    {
      size_t len = sizeof(T_DIGEST_) - 1 + sizeof(T_realm__) - 1 + sizeof(T_auth_nonce) - 1 + 32 + sizeof(T__opaque) - 1 + 32 + 1;
      String header;
      if (header.reserve(len + strlen(realm))) {
        const String nonce = genRandomMD5();
        const String opaque = genRandomMD5();
        if (nonce.length() && opaque.length()) {
          header.concat(T_DIGEST_);
          header.concat(T_realm__);
          header.concat(realm);
          header.concat(T_auth_nonce);
          header.concat(nonce);
          header.concat(T__opaque);
          header.concat(opaque);
          header.concat((char)0x22);  // '"'
          r->addHeader(T_WWW_AUTH, header.c_str());
        } else {
          async_ws_log_e("Failed to allocate");
          abort();
        }
      }
      break;
    }
    default: break;
  }

  send(r);
}

bool AsyncWebServerRequest::hasArg(const char *name) const {
  for (const auto &arg : _params) {
    if (arg.name() == name) {
      return true;
    }
  }
  return false;
}

#ifdef ESP8266
bool AsyncWebServerRequest::hasArg(const __FlashStringHelper *data) const {
  return hasArg(String(data).c_str());
}
#endif

const String &AsyncWebServerRequest::arg(const char *name) const {
  for (const auto &arg : _params) {
    if (arg.name() == name) {
      return arg.value();
    }
  }
  return asyncsrv::emptyString;
}

#ifdef ESP8266
const String &AsyncWebServerRequest::arg(const __FlashStringHelper *data) const {
  return arg(String(data).c_str());
}
#endif

const String &AsyncWebServerRequest::arg(size_t i) const {
  return getParam(i)->value();
}

const String &AsyncWebServerRequest::argName(size_t i) const {
  return getParam(i)->name();
}

const String &AsyncWebServerRequest::header(const char *name) const {
  const AsyncWebHeader *h = getHeader(name);
  return h ? h->value() : asyncsrv::emptyString;
}

#ifdef ESP8266
const String &AsyncWebServerRequest::header(const __FlashStringHelper *data) const {
  return header(String(data).c_str());
};
#endif

const String &AsyncWebServerRequest::header(size_t i) const {
  const AsyncWebHeader *h = getHeader(i);
  return h ? h->value() : asyncsrv::emptyString;
}

const String &AsyncWebServerRequest::headerName(size_t i) const {
  const AsyncWebHeader *h = getHeader(i);
  return h ? h->name() : asyncsrv::emptyString;
}

String AsyncWebServerRequest::urlDecode(const String &text) const {
  char temp[] = "0x00";
  unsigned int len = text.length();
  unsigned int i = 0;
  String decoded;
  // Allocate the string internal buffer - never longer from source text
  if (!decoded.reserve(len)) {
    async_ws_log_e("Failed to allocate");
    return asyncsrv::emptyString;
  }
  while (i < len) {
    char decodedChar;
    char encodedChar = text.charAt(i++);
    if ((encodedChar == '%') && (i + 1 < len)) {
      temp[2] = text.charAt(i++);
      temp[3] = text.charAt(i++);
      decodedChar = strtol(temp, NULL, 16);
    } else if (encodedChar == '+') {
      decodedChar = ' ';
    } else {
      decodedChar = encodedChar;  // normal ascii char
    }
    decoded.concat(decodedChar);
  }
  return decoded;
}

const char *AsyncWebServerRequest::requestedConnTypeToString() const {
  switch (_reqconntype) {
    case RCT_NOT_USED: return T_RCT_NOT_USED;
    case RCT_DEFAULT:  return T_RCT_DEFAULT;
    case RCT_HTTP:     return T_RCT_HTTP;
    case RCT_WS:       return T_RCT_WS;
    case RCT_EVENT:    return T_RCT_EVENT;
    default:           return T_ERROR;
  }
}

bool AsyncWebServerRequest::isExpectedRequestedConnType(RequestedConnectionType erct1, RequestedConnectionType erct2, RequestedConnectionType erct3) const {
  return ((erct1 != RCT_NOT_USED) && (erct1 == _reqconntype)) || ((erct2 != RCT_NOT_USED) && (erct2 == _reqconntype))
         || ((erct3 != RCT_NOT_USED) && (erct3 == _reqconntype));
}

AsyncClient *AsyncWebServerRequest::clientRelease() {
  AsyncClient *c = _client;
  _client = nullptr;
  return c;
}

namespace asyncsrv {
// WebRequestMethod conversions
WebRequestMethod stringToMethod(const String &m) {
  if (m == T_GET) {
    return AsyncWebRequestMethod::HTTP_GET;
  } else if (m == T_POST) {
    return AsyncWebRequestMethod::HTTP_POST;
  } else if (m == T_DELETE) {
    return AsyncWebRequestMethod::HTTP_DELETE;
  } else if (m == T_PUT) {
    return AsyncWebRequestMethod::HTTP_PUT;
  } else if (m == T_PATCH) {
    return AsyncWebRequestMethod::HTTP_PATCH;
  } else if (m == T_HEAD) {
    return AsyncWebRequestMethod::HTTP_HEAD;
  } else if (m == T_OPTIONS) {
    return AsyncWebRequestMethod::HTTP_OPTIONS;
  } else if (m == T_TRACE) {
    return AsyncWebRequestMethod::HTTP_TRACE;
  } else if (m == T_CONNECT) {
    return AsyncWebRequestMethod::HTTP_CONNECT;
  } else if (m == T_PURGE) {
    return AsyncWebRequestMethod::HTTP_PURGE;
  } else if (m == T_LINK) {
    return AsyncWebRequestMethod::HTTP_LINK;
  } else if (m == T_UNLINK) {
    return AsyncWebRequestMethod::HTTP_UNLINK;
  } else if (m == T_PROPFIND) {
    return AsyncWebRequestMethod::HTTP_PROPFIND;
  } else if (m == T_LOCK) {
    return AsyncWebRequestMethod::HTTP_LOCK;
  } else if (m == T_UNLOCK) {
    return AsyncWebRequestMethod::HTTP_UNLOCK;
  } else if (m == T_PROPPATCH) {
    return AsyncWebRequestMethod::HTTP_PROPPATCH;
  } else if (m == T_MKCOL) {
    return AsyncWebRequestMethod::HTTP_MKCOL;
  } else if (m == T_MOVE) {
    return AsyncWebRequestMethod::HTTP_MOVE;
  } else if (m == T_COPY) {
    return AsyncWebRequestMethod::HTTP_COPY;
  } else if (m == T_SEARCH) {
    return AsyncWebRequestMethod::HTTP_SEARCH;
  } else if (m == T_BIND) {
    return AsyncWebRequestMethod::HTTP_BIND;
  } else if (m == T_REBIND) {
    return AsyncWebRequestMethod::HTTP_REBIND;
  } else if (m == T_UNBIND) {
    return AsyncWebRequestMethod::HTTP_UNBIND;
  } else if (m == T_ACL) {
    return AsyncWebRequestMethod::HTTP_ACL;
  } else {
    return AsyncWebRequestMethod::HTTP_INVALID;
  }
}

const char *methodToString(WebRequestMethod method) {
  switch (method) {
    case AsyncWebRequestMethod::HTTP_DELETE: return T_DELETE;
    case AsyncWebRequestMethod::HTTP_GET:    return T_GET;
    case AsyncWebRequestMethod::HTTP_HEAD:   return T_HEAD;
    case AsyncWebRequestMethod::HTTP_POST:   return T_POST;
    case AsyncWebRequestMethod::HTTP_PUT:    return T_PUT;
    /* pathological */
    case AsyncWebRequestMethod::HTTP_CONNECT: return T_CONNECT;
    case AsyncWebRequestMethod::HTTP_OPTIONS: return T_OPTIONS;
    case AsyncWebRequestMethod::HTTP_TRACE:   return T_TRACE;
    /* WebDAV */
    case AsyncWebRequestMethod::HTTP_COPY:      return T_COPY;
    case AsyncWebRequestMethod::HTTP_LOCK:      return T_LOCK;
    case AsyncWebRequestMethod::HTTP_MKCOL:     return T_MKCOL;
    case AsyncWebRequestMethod::HTTP_MOVE:      return T_MOVE;
    case AsyncWebRequestMethod::HTTP_PROPFIND:  return T_PROPFIND;
    case AsyncWebRequestMethod::HTTP_PROPPATCH: return T_PROPPATCH;
    case AsyncWebRequestMethod::HTTP_SEARCH:    return T_SEARCH;
    case AsyncWebRequestMethod::HTTP_UNLOCK:    return T_UNLOCK;
    case AsyncWebRequestMethod::HTTP_BIND:      return T_BIND;
    case AsyncWebRequestMethod::HTTP_REBIND:    return T_REBIND;
    case AsyncWebRequestMethod::HTTP_UNBIND:    return T_UNBIND;
    case AsyncWebRequestMethod::HTTP_ACL:       return T_ACL;
    /* RFC-5789 */
    case AsyncWebRequestMethod::HTTP_PATCH: return T_PATCH;
    case AsyncWebRequestMethod::HTTP_PURGE: return T_PURGE;
    /* RFC-2068, section 19.6.1.2 */
    case AsyncWebRequestMethod::HTTP_LINK:   return T_LINK;
    case AsyncWebRequestMethod::HTTP_UNLINK: return T_UNLINK;
    // Unsupported
    default: return T_UNKNOWN;
  }
}
}  // namespace asyncsrv
