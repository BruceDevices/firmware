// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#pragma once

#ifdef Arduino_h
// arduino is not compatible with std::vector
#undef min
#undef max
#endif
#include <cbuf.h>

#include <memory>
#include <vector>

#include "./literals.h"

#ifndef CONFIG_LWIP_TCP_MSS
#ifdef TCP_MSS  // ESP8266
#define CONFIG_LWIP_TCP_MSS TCP_MSS
#else
// as it is defined for ESP32's Arduino LWIP
#define CONFIG_LWIP_TCP_MSS 1436
#endif
#endif

#define ASYNC_RESPONCE_BUFF_SIZE CONFIG_LWIP_TCP_MSS * 2
// It is possible to restore these defines, but one can use _min and _max instead. Or std::min, std::max.

class AsyncBasicResponse : public AsyncWebServerResponse {
private:
  String _content;
  // buffer to accumulate all response headers
  String _assembled_headers;
  // amount of headers buffer writtent to sockbuff
  size_t _writtenHeadersLength{0};

public:
  explicit AsyncBasicResponse(int code, const char *contentType = asyncsrv::empty, const char *content = asyncsrv::empty);
  AsyncBasicResponse(int code, const String &contentType, const String &content = asyncsrv::emptyString)
    : AsyncBasicResponse(code, contentType.c_str(), content.c_str()) {}
  void _respond(AsyncWebServerRequest *request) final;
  size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time) final {
    return write_send_buffs(request, len, time);
  };
  bool _sourceValid() const final {
    return true;
  }

protected:
  /**
   * @brief write next portion of response data to send buffs
   * this method (re)fills tcp send buffers, it could be called either at will
   * or from a tcp_recv/tcp_poll callbacks from AsyncTCP
   *
   * @param request - used to access client object
   * @param len - size of acknowledged data from the remote side (TCP window update, not TCP ack!)
   * @param time - time passed between last sent and received packet
   * @return size_t amount of response data placed to TCP send buffs for delivery (defined by sdkconfig value CONFIG_LWIP_TCP_SND_BUF_DEFAULT)
   */
  size_t write_send_buffs(AsyncWebServerRequest *request, size_t len, uint32_t time);
};

class AsyncAbstractResponse : public AsyncWebServerResponse {
private:
#if ASYNCWEBSERVER_USE_CHUNK_INFLIGHT
  // amount of response data in-flight, i.e. sent, but not acked yet
  size_t _in_flight{0};
  // in-flight queue credits
  size_t _in_flight_credit{2};
#endif
  // buffer to accumulate all response headers
  String _assembled_headers;
  // amount of headers buffer writtent to sockbuff
  size_t _writtenHeadersLength{0};
  // Data is inserted into cache at begin().
  // This is inefficient with vector, but if we use some other container,
  // we won't be able to access it as contiguous array of bytes when reading from it,
  // so by gaining performance in one place, we'll lose it in another.
  std::vector<uint8_t> _cache;
  // intermediate buffer to copy outbound data to, also it will keep pending data between _send calls
  std::unique_ptr<std::array<uint8_t, ASYNC_RESPONCE_BUFF_SIZE> > _send_buffer;
  // buffer data size specifiers
  size_t _send_buffer_offset{0}, _send_buffer_len{0};
  size_t _readDataFromCacheOrContent(uint8_t *data, const size_t len);
  size_t _fillBufferAndProcessTemplates(uint8_t *buf, size_t maxLen);

protected:
  AwsTemplateProcessor _callback;
  /**
   * @brief write next portion of response data to send buffs
   * this method (re)fills tcp send buffers, it could be called either at will
   * or from a tcp_recv/tcp_poll callbacks from AsyncTCP
   *
   * @param request - used to access client object
   * @param len - size of acknowledged data from the remote side (TCP window update, not TCP ack!)
   * @param time - time passed between last sent and received packet
   * @return size_t amount of response data placed to TCP send buffs for delivery (defined by sdkconfig value CONFIG_LWIP_TCP_SND_BUF_DEFAULT)
   */
  size_t write_send_buffs(AsyncWebServerRequest *request, size_t len, uint32_t time);

public:
  AsyncAbstractResponse(AwsTemplateProcessor callback = nullptr);
  virtual ~AsyncAbstractResponse() {}
  void _respond(AsyncWebServerRequest *request) final;
  size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time) final {
    return write_send_buffs(request, len, time);
  };
  virtual bool _sourceValid() const {
    return false;
  }
  virtual size_t _fillBuffer(uint8_t *buf __attribute__((unused)), size_t maxLen __attribute__((unused))) {
    return 0;
  }
};

#ifndef TEMPLATE_PLACEHOLDER
#define TEMPLATE_PLACEHOLDER '%'
#endif

#define TEMPLATE_PARAM_NAME_LENGTH 32
class AsyncFileResponse : public AsyncAbstractResponse {
  using File = fs::File;
  using FS = fs::FS;

private:
  File _content;
  void _setContentTypeFromPath(const String &path);

public:
  AsyncFileResponse(FS &fs, const String &path, const char *contentType = asyncsrv::empty, bool download = false, AwsTemplateProcessor callback = nullptr);
  AsyncFileResponse(FS &fs, const String &path, const String &contentType, bool download = false, AwsTemplateProcessor callback = nullptr)
    : AsyncFileResponse(fs, path, contentType.c_str(), download, callback) {}
  AsyncFileResponse(
    File content, const String &path, const char *contentType = asyncsrv::empty, bool download = false, AwsTemplateProcessor callback = nullptr
  );
  AsyncFileResponse(File content, const String &path, const String &contentType, bool download = false, AwsTemplateProcessor callback = nullptr)
    : AsyncFileResponse(content, path, contentType.c_str(), download, callback) {}
  ~AsyncFileResponse() {
    _content.close();
  }
  bool _sourceValid() const final {
    return !!(_content);
  }
  size_t _fillBuffer(uint8_t *buf, size_t maxLen) final;
};

class AsyncStreamResponse : public AsyncAbstractResponse {
private:
  Stream *_content;

public:
  AsyncStreamResponse(Stream &stream, const char *contentType, size_t len, AwsTemplateProcessor callback = nullptr);
  AsyncStreamResponse(Stream &stream, const String &contentType, size_t len, AwsTemplateProcessor callback = nullptr)
    : AsyncStreamResponse(stream, contentType.c_str(), len, callback) {}
  bool _sourceValid() const final {
    return !!(_content);
  }
  size_t _fillBuffer(uint8_t *buf, size_t maxLen) final;
};

class AsyncCallbackResponse : public AsyncAbstractResponse {
private:
  AwsResponseFiller _content;
  size_t _filledLength;

public:
  AsyncCallbackResponse(const char *contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr);
  AsyncCallbackResponse(const String &contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr)
    : AsyncCallbackResponse(contentType.c_str(), len, callback, templateCallback) {}
  bool _sourceValid() const final {
    return !!(_content);
  }
  size_t _fillBuffer(uint8_t *buf, size_t maxLen) final;
};

class AsyncChunkedResponse : public AsyncAbstractResponse {
private:
  AwsResponseFiller _content;
  size_t _filledLength;

public:
  AsyncChunkedResponse(const char *contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr);
  AsyncChunkedResponse(const String &contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr)
    : AsyncChunkedResponse(contentType.c_str(), callback, templateCallback) {}
  bool _sourceValid() const final {
    return !!(_content);
  }
  size_t _fillBuffer(uint8_t *buf, size_t maxLen) final;
};

class AsyncProgmemResponse : public AsyncAbstractResponse {
private:
  const uint8_t *_content;
  // offset index (how much we've sent already)
  size_t _index;

public:
  AsyncProgmemResponse(int code, const char *contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback = nullptr);
  AsyncProgmemResponse(int code, const String &contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback = nullptr)
    : AsyncProgmemResponse(code, contentType.c_str(), content, len, callback) {}
  bool _sourceValid() const final {
    return true;
  }
  size_t _fillBuffer(uint8_t *buf, size_t maxLen) final;
};

class AsyncResponseStream : public AsyncAbstractResponse, public Print {
private:
  std::unique_ptr<cbuf> _content;

public:
  AsyncResponseStream(const char *contentType, size_t bufferSize);
  AsyncResponseStream(const String &contentType, size_t bufferSize) : AsyncResponseStream(contentType.c_str(), bufferSize) {}
  bool _sourceValid() const final {
    return (_state < RESPONSE_END);
  }
  size_t _fillBuffer(uint8_t *buf, size_t maxLen) final;
  size_t write(const uint8_t *data, size_t len);
  size_t write(uint8_t data);
  /**
   * @brief Returns the number of bytes available in the stream.
   */
  size_t available() const {
    return _content->available();
  }
  using Print::write;
};
