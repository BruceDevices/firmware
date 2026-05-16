// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#pragma once

#include <Arduino.h>

#if defined(ESP32) || defined(LIBRETINY) || defined(HOST)
#include <AsyncTCP.h>
#ifdef LIBRETINY
#ifdef round
#undef round
#endif
#endif
#include <mutex>
#ifndef WS_MAX_QUEUED_MESSAGES
#define WS_MAX_QUEUED_MESSAGES 32
#endif
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>
#ifndef WS_MAX_QUEUED_MESSAGES
#define WS_MAX_QUEUED_MESSAGES 8
#endif
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
#include <RPAsyncTCP.h>
#ifndef WS_MAX_QUEUED_MESSAGES
#define WS_MAX_QUEUED_MESSAGES 32
#endif
#endif

#include <ESPAsyncWebServer.h>
#include <AsyncWebServerLogging.h>

#include <cstdio>
#include <deque>
#include <list>
#include <memory>

#if defined(ESP8266) || defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
#include <Hash.h>
#ifdef CRYPTO_HASH_h  // include Hash.h from espressif framework if the first include was from the crypto library
#include <../src/Hash.h>
#endif
#endif

#ifndef DEFAULT_MAX_WS_CLIENTS
#if defined(ESP32) || defined(HOST)
#define DEFAULT_MAX_WS_CLIENTS 8
#else
#define DEFAULT_MAX_WS_CLIENTS 4
#endif
#endif

using AsyncWebSocketSharedBuffer = std::shared_ptr<std::vector<uint8_t>>;

class AsyncWebSocket;
class AsyncWebSocketResponse;
class AsyncWebSocketClient;

/*
 * Control Frame
 */

class AsyncWebSocketControl {
private:
  uint8_t _opcode;
  uint8_t *_data;
  size_t _len;
  bool _mask;
  bool _finished;

public:
  AsyncWebSocketControl(uint8_t opcode, const uint8_t *data = NULL, size_t len = 0, bool mask = false)
    : _opcode(opcode), _len(len), _mask(len && mask), _finished(false) {
    if (data == NULL) {
      _len = 0;
    }
    if (_len) {
      if (_len > 125) {
        _len = 125;
      }

      _data = (uint8_t *)malloc(_len);

      if (_data == NULL) {
        async_ws_log_e("Failed to allocate");
        _len = 0;
      } else {
        memcpy(_data, data, len);
      }
    } else {
      _data = NULL;
    }
  }

  ~AsyncWebSocketControl() {
    if (_data != NULL) {
      free(_data);
    }
  }

  bool finished() const {
    return _finished;
  }
  uint8_t opcode() {
    return _opcode;
  }
  uint8_t len() {
    return _len + 2;
  }
  size_t send(AsyncClient *client);
};

typedef struct {
  /** Message type as defined by enum AwsFrameType.
     * Note: Applications will only see WS_TEXT and WS_BINARY.
     * All other types are handled by the library. */
  uint8_t message_opcode;
  /** Frame number of a fragmented message. */
  uint32_t num;
  /** Is this the last frame in a fragmented message ?*/
  uint8_t final;
  /** Is this frame masked? */
  uint8_t masked;
  /** Message type as defined by enum AwsFrameType.
     * This value is the same as message_opcode for non-fragmented
     * messages, but may also be WS_CONTINUATION in a fragmented message. */
  uint8_t opcode;
  /** Length of the current frame.
     * This equals the total length of the message if num == 0 && final == true */
  uint64_t len;
  /** Mask key */
  uint8_t mask[4];
  /** Offset of the data inside the current frame. */
  uint64_t index;
} AwsFrameInfo;

typedef enum {
  WS_DISCONNECTED,
  WS_CONNECTED,
  WS_DISCONNECTING
} AwsClientStatus;
typedef enum {
  WS_CONTINUATION,
  WS_TEXT,
  WS_BINARY,
  WS_DISCONNECT = 0x08,
  WS_PING,
  WS_PONG
} AwsFrameType;
typedef enum {
  WS_MSG_SENDING,
  WS_MSG_SENT,
  WS_MSG_ERROR
} AwsMessageStatus;
typedef enum {
  WS_EVT_CONNECT,
  WS_EVT_DISCONNECT,
  WS_EVT_PING,
  WS_EVT_PONG,
  WS_EVT_ERROR,
  WS_EVT_DATA
} AwsEventType;

class AsyncWebSocketMessageBuffer {
  friend AsyncWebSocket;
  friend AsyncWebSocketClient;

private:
  AsyncWebSocketSharedBuffer _buffer;

public:
  AsyncWebSocketMessageBuffer() {}
  explicit AsyncWebSocketMessageBuffer(size_t size);
  AsyncWebSocketMessageBuffer(const uint8_t *data, size_t size);
  //~AsyncWebSocketMessageBuffer();
  bool reserve(size_t size);
  uint8_t *get() {
    return _buffer->data();
  }
  size_t length() const {
    return _buffer->size();
  }
};

class AsyncWebSocketMessage {
  friend AsyncWebSocketClient;

private:
  size_t _remainingBytesToSend() const {
    return _WSbuffer->size() - _sent;
  }

  AsyncWebSocketSharedBuffer _WSbuffer;
  uint8_t _opcode{WS_TEXT};
  bool _mask{false};
  AwsMessageStatus _status{WS_MSG_ERROR};
  size_t _sent{};
  size_t _ack{};
  size_t _acked{};

public:
  AsyncWebSocketMessage(AsyncWebSocketSharedBuffer buffer, uint8_t opcode = WS_TEXT, bool mask = false);

  bool finished() const {
    return _status != WS_MSG_SENDING;
  }
  bool betweenFrames() const {
    return _acked == _ack;
  }

  size_t ack(size_t len, uint32_t time);
  size_t send(AsyncClient *client);
};

class AsyncWebSocketClient {
private:
  AsyncClient *_client;
  AsyncWebSocket *_server;
  uint32_t _clientId;
  AwsClientStatus _status;
  uint8_t _pstate;
  uint32_t _lastMessageTime;
  uint32_t _keepAlivePeriod;
  mutable asyncsrv::mutex_type _queue_lock;
  std::deque<AsyncWebSocketControl> _controlQueue;
  std::deque<AsyncWebSocketMessage> _messageQueue;
  bool _closeWhenFull = false;

  AwsFrameInfo _pinfo;

  bool _queueControl(uint8_t opcode, const uint8_t *data = NULL, size_t len = 0, bool mask = false);
  bool _queueMessage(AsyncWebSocketSharedBuffer buffer, uint8_t opcode = WS_TEXT, bool mask = false);
  void _runQueue();
  void _clearQueue();

  // this function is called when a text message is received, in order to copy the buffer and place a null terminator at the end of the buffer for easier handling of text messages.
  void _handleDataEvent(uint8_t *data, size_t len, bool endOfPaquet);

public:
  void *_tempObject;

  AsyncWebSocketClient(AsyncClient *client, AsyncWebSocket *server);

  /**
   * @brief Construct a new Async Web Socket Client object
   * @note constructor would take the ownership of of AsyncTCP's client pointer from `request` parameter and call delete on it!
   * @param request
   * @param server
   */
  AsyncWebSocketClient(AsyncWebServerRequest *request, AsyncWebSocket *server) : AsyncWebSocketClient(request->clientRelease(), server){};
  ~AsyncWebSocketClient();

  // client id increments for the given server
  uint32_t id() const {
    return _clientId;
  }
  AwsClientStatus status() const {
    return _status;
  }
  AsyncClient *client() {
    return _client;
  }
  const AsyncClient *client() const {
    return _client;
  }
  AsyncWebSocket *server() {
    return _server;
  }
  const AsyncWebSocket *server() const {
    return _server;
  }
  AwsFrameInfo const &pinfo() const {
    return _pinfo;
  }

  // CloseClientOnQueueFull:
  //
  // - If "true", the client will be closed if the message queue becomes full.
  // This is the default behavior in yubox-node-org, which is not silently discarding messages but instead closes the connection.
  // The big issue with this behavior is  that is can cause the UI to automatically re-create a new WS connection, which can be filled again,
  // and so on, causing a resource exhaustion.
  // Also this can lead to a crash as explained in this issue: https://github.com/ESP32Async/ESPAsyncWebServer/issues/433
  //
  // - If "false" (default in this library), the incoming message will be discarded if the queue is full.
  // This is the default behavior in the original ESPAsyncWebServer library from me-no-dev.
  // This behavior allows the best performance at the expense of unreliable message delivery in case the queue is full (some messages may be lost).
  //
  // With recent refactorings of the library, the queue is barely used and the library supports a fast sending rate of messages. So if the queue is growing:
  //  - either the server is sending messages at an insane fast rate, faster than what the client can acknowledge, which can be the case if the client is slow or if the messages are big and the network is slow,
  //  - or there is a network issue causing the client to not receive messages, or network is broken. In that case, if the network is broken, the queue will fill temporarily until the connection is closed and client removed.
  //
  // In case your application requires a fast and high frequency message sending and you foresee some queue usage, you can:
  //  - increase the queue side to allow some room
  //  - check some functions status before or when sending in order to decrease your sending rate to let the queue drain, or take action by closing this client if necessary.
  //
  // This has to be an application-specific deicison that the library cannot take for you.
  // Here are a list of some functions that you can use and check the boolean value returned:
  // - the send methods
  // - queueIsFull()
  // - availableForWriteAll()
  // - availableForWrite(clientId)
  //
  // When the queue is full, a message is logged in case it is discarded.
  void setCloseClientOnQueueFull(bool close) {
    _closeWhenFull = close;
  }
  bool willCloseClientOnQueueFull() const {
    return _closeWhenFull;
  }

  IPAddress remoteIP() const;
  uint16_t remotePort() const;

  bool shouldBeDeleted() const {
    return !_client;
  }

  // control frames
  void close(uint16_t code = 0, const char *message = NULL);
  bool ping(const uint8_t *data = NULL, size_t len = 0);

  // set auto-ping period in seconds. disabled if zero (default)
  void keepAlivePeriod(uint16_t seconds) {
    _keepAlivePeriod = seconds * 1000;
  }
  uint16_t keepAlivePeriod() {
    return (uint16_t)(_keepAlivePeriod / 1000);
  }

  // data packets
  bool message(AsyncWebSocketSharedBuffer buffer, uint8_t opcode = WS_TEXT, bool mask = false) {
    return _queueMessage(buffer, opcode, mask);
  }
  bool queueIsFull() const;
  size_t queueLen() const;

  size_t printf(const char *format, ...) __attribute__((format(printf, 2, 3)));

  bool text(AsyncWebSocketSharedBuffer buffer);
  bool text(const uint8_t *message, size_t len);
  bool text(const char *message, size_t len);
  bool text(const char *message);
  bool text(const String &message);
  bool text(AsyncWebSocketMessageBuffer *buffer);

  bool binary(AsyncWebSocketSharedBuffer buffer);
  bool binary(const uint8_t *message, size_t len);
  bool binary(const char *message, size_t len);
  bool binary(const char *message);
  bool binary(const String &message);
  bool binary(AsyncWebSocketMessageBuffer *buffer);

  bool canSend() const;

  // system callbacks (do not call)
  void _onAck(size_t len, uint32_t time);
  void _onError(int8_t);
  void _onPoll();
  void _onTimeout(uint32_t time);
  void _onDisconnect();
  void _onData(void *pbuf, size_t plen);

#ifdef ESP8266
  size_t printf_P(PGM_P formatP, ...) __attribute__((format(printf, 2, 3)));
  bool text(const __FlashStringHelper *message);
  bool binary(const __FlashStringHelper *message, size_t len);
#endif
};

using AwsHandshakeHandler = std::function<bool(AsyncWebServerRequest *request)>;
using AwsEventHandler = std::function<void(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)>;

// WebServer Handler implementation that plays the role of a socket server
class AsyncWebSocket : public AsyncWebHandler {
private:
  String _url;
  std::list<AsyncWebSocketClient> _clients;
  uint32_t _cNextId;
  AwsEventHandler _eventHandler;
  AwsHandshakeHandler _handshakeHandler;
  bool _enabled;
  mutable asyncsrv::mutex_type _ws_clients_lock;

public:
  typedef enum {
    DISCARDED = 0,
    ENQUEUED = 1,
    PARTIALLY_ENQUEUED = 2,
  } SendStatus;

  explicit AsyncWebSocket(const char *url, AwsEventHandler handler = nullptr) : _url(url), _cNextId(1), _eventHandler(handler), _enabled(true) {}
  AsyncWebSocket(const String &url, AwsEventHandler handler = nullptr) : _url(url), _cNextId(1), _eventHandler(handler), _enabled(true) {}
  ~AsyncWebSocket(){};
  const char *url() const {
    return _url.c_str();
  }
  void enable(bool e) {
    _enabled = e;
  }
  bool enabled() const {
    return _enabled;
  }
  bool availableForWriteAll();
  bool availableForWrite(uint32_t id);

  size_t count() const;
  AsyncWebSocketClient *client(uint32_t id);
  bool hasClient(uint32_t id) {
    return client(id) != nullptr;
  }

  void close(uint32_t id, uint16_t code = 0, const char *message = NULL);
  void closeAll(uint16_t code = 0, const char *message = NULL);
  void cleanupClients(uint16_t maxClients = DEFAULT_MAX_WS_CLIENTS);

  bool ping(uint32_t id, const uint8_t *data = NULL, size_t len = 0);
  SendStatus pingAll(const uint8_t *data = NULL, size_t len = 0);  //  done

  bool text(uint32_t id, const uint8_t *message, size_t len);
  bool text(uint32_t id, const char *message, size_t len);
  bool text(uint32_t id, const char *message);
  bool text(uint32_t id, const String &message);
  bool text(uint32_t id, AsyncWebSocketMessageBuffer *buffer);
  bool text(uint32_t id, AsyncWebSocketSharedBuffer buffer);

  SendStatus textAll(const uint8_t *message, size_t len);
  SendStatus textAll(const char *message, size_t len);
  SendStatus textAll(const char *message);
  SendStatus textAll(const String &message);
  SendStatus textAll(AsyncWebSocketMessageBuffer *buffer);
  SendStatus textAll(AsyncWebSocketSharedBuffer buffer);

  bool binary(uint32_t id, const uint8_t *message, size_t len);
  bool binary(uint32_t id, const char *message, size_t len);
  bool binary(uint32_t id, const char *message);
  bool binary(uint32_t id, const String &message);
  bool binary(uint32_t id, AsyncWebSocketMessageBuffer *buffer);
  bool binary(uint32_t id, AsyncWebSocketSharedBuffer buffer);

  SendStatus binaryAll(const uint8_t *message, size_t len);
  SendStatus binaryAll(const char *message, size_t len);
  SendStatus binaryAll(const char *message);
  SendStatus binaryAll(const String &message);
  SendStatus binaryAll(AsyncWebSocketMessageBuffer *buffer);
  SendStatus binaryAll(AsyncWebSocketSharedBuffer buffer);

  size_t printf(uint32_t id, const char *format, ...) __attribute__((format(printf, 3, 4)));
  size_t printfAll(const char *format, ...) __attribute__((format(printf, 2, 3)));

#ifdef ESP8266
  bool text(uint32_t id, const __FlashStringHelper *message);
  SendStatus textAll(const __FlashStringHelper *message);
  bool binary(uint32_t id, const __FlashStringHelper *message, size_t len);
  SendStatus binaryAll(const __FlashStringHelper *message, size_t len);
  size_t printf_P(uint32_t id, PGM_P formatP, ...) __attribute__((format(printf, 3, 4)));
  size_t printfAll_P(PGM_P formatP, ...) __attribute__((format(printf, 2, 3)));
#endif

  void onEvent(AwsEventHandler handler) {
    _eventHandler = handler;
  }
  void handleHandshake(AwsHandshakeHandler handler) {
    _handshakeHandler = handler;
  }

  // system callbacks (do not call)
  uint32_t _getNextId() {
    return _cNextId++;
  }
  AsyncWebSocketClient *_newClient(AsyncWebServerRequest *request);
  void _handleDisconnect(AsyncWebSocketClient *client);
  void _handleEvent(AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
  bool canHandle(AsyncWebServerRequest *request) const final;
  void handleRequest(AsyncWebServerRequest *request) final;

  //  messagebuffer functions/objects.
  AsyncWebSocketMessageBuffer *makeBuffer(size_t size = 0);
  AsyncWebSocketMessageBuffer *makeBuffer(const uint8_t *data, size_t size);

  std::list<AsyncWebSocketClient> &getClients() {
    return _clients;
  }
};

// WebServer response to authenticate the socket and detach the tcp client from the web server request
class AsyncWebSocketResponse : public AsyncWebServerResponse {
private:
  String _content;
  AsyncWebSocket *_server;
  AsyncWebServerRequest *_request;
  // this call back will switch AsyncTCP client to WebSocket
  void _switchClient();

public:
  AsyncWebSocketResponse(const String &key, AsyncWebSocket *server);
  void _respond(AsyncWebServerRequest *request) override;
  size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time) override {
    return 0;
  };
  bool _sourceValid() const override {
    return true;
  }
};

class AsyncWebSocketMessageHandler {
public:
  AwsEventHandler eventHandler() const {
    return _handler;
  }

  void onConnect(std::function<void(AsyncWebSocket *server, AsyncWebSocketClient *client)> onConnect) {
    _onConnect = onConnect;
  }

  void onDisconnect(std::function<void(AsyncWebSocket *server, uint32_t clientId)> onDisconnect) {
    _onDisconnect = onDisconnect;
  }

  /**
   * Error callback
   * @param reason null-terminated string
   * @param len length of the string
   */
  void onError(std::function<void(AsyncWebSocket *server, AsyncWebSocketClient *client, uint16_t errorCode, const char *reason, size_t len)> onError) {
    _onError = onError;
  }

  /**
   * Complete message callback
   * @param data pointer to the data (binary or null-terminated string). This handler expects the user to know which data type he uses.
   */
  void onMessage(std::function<void(AsyncWebSocket *server, AsyncWebSocketClient *client, const uint8_t *data, size_t len)> onMessage) {
    _onMessage = onMessage;
  }

  /**
   * Fragmented message callback
   * @param data pointer to the data (binary or null-terminated string), will be null-terminated. This handler expects the user to know which data type he uses.
   */
  // clang-format off
  void onFragment(std::function<void(AsyncWebSocket *server, AsyncWebSocketClient *client, const AwsFrameInfo *frameInfo, const uint8_t *data, size_t len)> onFragment) {
    _onFragment = onFragment;
  }
  // clang-format on

private:
  // clang-format off
  std::function<void(AsyncWebSocket *server, AsyncWebSocketClient *client)> _onConnect;
  std::function<void(AsyncWebSocket *server, AsyncWebSocketClient *client, uint16_t errorCode, const char *reason, size_t len)> _onError;
  std::function<void(AsyncWebSocket *server, AsyncWebSocketClient *client, const uint8_t *data, size_t len)> _onMessage;
  std::function<void(AsyncWebSocket *server, AsyncWebSocketClient *client, const AwsFrameInfo *frameInfo, const uint8_t *data, size_t len)> _onFragment;
  std::function<void(AsyncWebSocket *server, uint32_t clientId)> _onDisconnect;
  // clang-format on

  // this handler is meant to only support 1-frame messages (== unfragmented messages)
  AwsEventHandler _handler = [this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      if (_onConnect) {
        _onConnect(server, client);
      }
    } else if (type == WS_EVT_DISCONNECT) {
      if (_onDisconnect) {
        _onDisconnect(server, client->id());
      }
    } else if (type == WS_EVT_ERROR) {
      if (_onError) {
        _onError(server, client, *((uint16_t *)arg), (const char *)data, len);
      }
    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      if (info->final && info->index == 0 && info->len == len) {
        if (_onMessage) {
          _onMessage(server, client, data, len);
        }
      } else {
        if (_onFragment) {
          _onFragment(server, client, info, data, len);
        }
      }
    }
  };
};
