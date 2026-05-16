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
#ifndef SSE_MAX_QUEUED_MESSAGES
#define SSE_MAX_QUEUED_MESSAGES 32
#endif
#define SSE_MIN_INFLIGH 2 * 1460   // allow 2 MSS packets
#define SSE_MAX_INFLIGH 16 * 1024  // but no more than 16k, no need to blow it, since same data is kept in local Q
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>
#ifndef SSE_MAX_QUEUED_MESSAGES
#define SSE_MAX_QUEUED_MESSAGES 8
#endif
#define SSE_MIN_INFLIGH 2 * 1460  // allow 2 MSS packets
#define SSE_MAX_INFLIGH 8 * 1024  // but no more than 8k, no need to blow it, since same data is kept in local Q
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
#include <RPAsyncTCP.h>
#ifndef SSE_MAX_QUEUED_MESSAGES
#define SSE_MAX_QUEUED_MESSAGES 32
#endif
#define SSE_MIN_INFLIGH 2 * 1460   // allow 2 MSS packets
#define SSE_MAX_INFLIGH 16 * 1024  // but no more than 16k, no need to blow it, since same data is kept in local Q
#endif

#include <ESPAsyncWebServer.h>

#ifdef ESP8266
#include <Hash.h>
#ifdef CRYPTO_HASH_h  // include Hash.h from espressif framework if the first include was from the crypto library
#include <../src/Hash.h>
#endif
#endif

#include <list>
#include <memory>
#include <utility>

class AsyncEventSource;
class AsyncEventSourceResponse;
class AsyncEventSourceClient;
using ArEventHandlerFunction = std::function<void(AsyncEventSourceClient *client)>;
using ArAuthorizeConnectHandler = ArAuthorizeFunction;
// shared message object container
using AsyncEvent_SharedData_t = std::shared_ptr<String>;

/**
 * @brief Async Event Message container with shared message content data
 *
 */
class AsyncEventSourceMessage {

private:
  const AsyncEvent_SharedData_t _data;
  size_t _sent{0};   // num of bytes already sent
  size_t _acked{0};  // num of bytes acked

public:
  AsyncEventSourceMessage(AsyncEvent_SharedData_t data) : _data(data){};
#if defined(ESP32)
  AsyncEventSourceMessage(const char *data, size_t len) : _data(std::make_shared<String>(data, len)){};
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
  AsyncEventSourceMessage(const char *data, size_t len) : _data(std::make_shared<String>()) {
    if (data && len > 0) {
      _data->concat(data, len);
    }
  };
#else
  // esp8266's String does not have constructor with data/length arguments. Use a concat method here
  AsyncEventSourceMessage(const char *data, size_t len) {
    _data->concat(data, len);
  };
#endif

  /**
     * @brief acknowledge sending len bytes of data
     * @note if num of bytes to ack is larger then the unacknowledged message length the number of carried over bytes are returned
     *
     * @param len bytes to acknowledge
     * @param time
     * @return size_t number of extra bytes carried over
     */
  size_t ack(size_t len, uint32_t time = 0);

  /**
     * @brief write message data to client's buffer
     * @note this method does NOT call client's send
     *
     * @param client
     * @return size_t number of bytes written
     */
  size_t write(AsyncClient *client);

  /**
     * @brief writes message data to client's buffer and calls client's send method
     *
     * @param client
     * @return size_t returns num of bytes the clien was able to send()
     */
  size_t send(AsyncClient *client);

  // returns true if full message's length were acked
  bool finished() {
    return _acked == _data->length();
  }

  /**
     * @brief returns true if all data has been sent already
     *
     */
  bool sent() {
    return _sent == _data->length();
  }
};

/**
 * @brief class holds a sse messages queue for a particular client's connection
 *
 */
class AsyncEventSourceClient {
private:
  AsyncClient *_client;
  AsyncEventSource *_server;
  uint32_t _lastId{0};
  size_t _inflight{0};                    // num of unacknowledged bytes that has been written to socket buffer
  size_t _max_inflight{SSE_MAX_INFLIGH};  // max num of unacknowledged bytes that could be written to socket buffer
  std::list<AsyncEventSourceMessage> _messageQueue;
  mutable asyncsrv::mutex_type _lockmq;
  bool _queueMessage(const char *message, size_t len);
  bool _queueMessage(AsyncEvent_SharedData_t &&msg);
  void _runQueue();

public:
  /**
   * @brief Construct a new Async Event Source Client object
   * @note constructor would take the ownership of of AsyncTCP's client pointer from `request` parameter and call delete on it!
   *
   * @param request
   * @param server
   */
  AsyncEventSourceClient(AsyncWebServerRequest *request, AsyncEventSource *server);
  ~AsyncEventSourceClient();

  /**
     * @brief Send an SSE message to client
     * it will craft an SSE message and place it to client's message queue
     *
     * @param message body string, could be single or multi-line string sepprated by \n, \r, \r\n
     * @param event body string, a sinle line string
     * @param id sequence id
     * @param reconnect client's reconnect timeout
     * @return true if message was placed in a queue
     * @return false if queue is full
     */
  bool send(const char *message, const char *event = NULL, uint32_t id = 0, uint32_t reconnect = 0);
  bool send(const String &message, const String &event, uint32_t id = 0, uint32_t reconnect = 0) {
    return send(message.c_str(), event.c_str(), id, reconnect);
  }
  bool send(const String &message, const char *event, uint32_t id = 0, uint32_t reconnect = 0) {
    return send(message.c_str(), event, id, reconnect);
  }

  /**
     * @brief place supplied preformatted SSE message to the message queue
     * @note message must a properly formatted SSE string according to https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events
     *
     * @param message data
     * @return true on success
     * @return false on queue overflow or no client connected
     */
  bool write(AsyncEvent_SharedData_t message) {
    return connected() && _queueMessage(std::move(message));
  };

  [[deprecated("Use _write(AsyncEvent_SharedData_t message) instead to share same data with multiple SSE clients")]]
  bool write(const char *message, size_t len) {
    return connected() && _queueMessage(message, len);
  };

  // close client's connection
  void close();

  // getters

  AsyncClient *client() {
    return _client;
  }
  bool connected() const {
    return _client && _client->connected();
  }
  uint32_t lastId() const {
    return _lastId;
  }
  size_t packetsWaiting() const {
    asyncsrv::lock_guard_type lock(_lockmq);
    return _messageQueue.size();
  };

  /**
     * @brief Sets max amount of bytes that could be written to client's socket while awaiting delivery acknowledge
     * used to throttle message delivery length to tradeoff memory consumption
     * @note actual amount of data written could possible be a bit larger but no more than available socket buff space
     *
     * @param value
     */
  void set_max_inflight_bytes(size_t value);

  /**
     * @brief Get current max inflight bytes value
     *
     * @return size_t
     */
  size_t get_max_inflight_bytes() const {
    return _max_inflight;
  }

  // system callbacks (do not call if from user code!)
  void _onAck(size_t len, uint32_t time);
  void _onPoll();
  void _onTimeout(uint32_t time);
  void _onDisconnect();
};

/**
 * @brief a class that maintains all connected HTTP clients subscribed to SSE delivery
 * dispatches supplied messages to the client's queues
 *
 */
class AsyncEventSource : public AsyncWebHandler {
private:
  String _url;
  std::list<std::unique_ptr<AsyncEventSourceClient>> _clients;
  // Same as for individual messages, protect mutations of _clients list
  // since simultaneous access from different tasks is possible
  mutable asyncsrv::mutex_type _client_queue_lock;
  ArEventHandlerFunction _connectcb = nullptr;
  ArEventHandlerFunction _disconnectcb = nullptr;

  // this method manipulates in-fligh data size for connected client depending on number of active connections
  void _adjust_inflight_window();

public:
  typedef enum {
    DISCARDED = 0,
    ENQUEUED = 1,
    PARTIALLY_ENQUEUED = 2,
  } SendStatus;

  AsyncEventSource(const char *url) : _url(url){};
  AsyncEventSource(const String &url) : _url(url){};
  ~AsyncEventSource() {
    close();
  };

  const char *url() const {
    return _url.c_str();
  }
  // close all connected clients
  void close();

  /**
     * @brief set on-connect callback for the client
     * used to deliver messages to client on first connect
     *
     * @param cb
     */
  void onConnect(ArEventHandlerFunction cb) {
    _connectcb = cb;
  }

  /**
     * @brief Send an SSE message to client
     * it will craft an SSE message and place it to all connected client's message queues
     *
     * @param message body string, could be single or multi-line string sepprated by \n, \r, \r\n
     * @param event body string, a sinle line string
     * @param id sequence id
     * @param reconnect client's reconnect timeout
     * @return SendStatus if message was placed in any/all/part of the client's queues
     */
  SendStatus send(const char *message, const char *event = NULL, uint32_t id = 0, uint32_t reconnect = 0);
  SendStatus send(const String &message, const String &event, uint32_t id = 0, uint32_t reconnect = 0) {
    return send(message.c_str(), event.c_str(), id, reconnect);
  }
  SendStatus send(const String &message, const char *event, uint32_t id = 0, uint32_t reconnect = 0) {
    return send(message.c_str(), event, id, reconnect);
  }

  // The client pointer sent to the callback is only for reference purposes. DO NOT CALL ANY METHOD ON IT !
  void onDisconnect(ArEventHandlerFunction cb) {
    _disconnectcb = cb;
  }
  void authorizeConnect(ArAuthorizeConnectHandler cb);

  // returns number of connected clients
  size_t count() const;

  // returns average number of messages pending in all client's queues
  size_t avgPacketsWaiting() const;

  // system callbacks (do not call from user code!)
  void _addClient(AsyncEventSourceClient *client);
  void _handleDisconnect(AsyncEventSourceClient *client);
  bool canHandle(AsyncWebServerRequest *request) const final;
  void handleRequest(AsyncWebServerRequest *request) final;
};

class AsyncEventSourceResponse : public AsyncWebServerResponse {
private:
  AsyncEventSource *_server;
  AsyncWebServerRequest *_request;
  // this call back will switch AsyncTCP client to SSE
  void _switchClient();

public:
  AsyncEventSourceResponse(AsyncEventSource *server);
  void _respond(AsyncWebServerRequest *request) override;
  size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time) override {
    return 0;
  };
  bool _sourceValid() const override {
    return true;
  }
};
