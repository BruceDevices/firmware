/*
 * Copyright 2020-2026 Ryan Powell <ryan@nable-embedded.io> and
 * esp-nimble-cpp, NimBLE-Arduino contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NIMBLE_CPP_STREAM_H
#define NIMBLE_CPP_STREAM_H

#include "nimconfig.h"
#if CONFIG_BT_NIMBLE_ENABLED && (CONFIG_BT_NIMBLE_ROLE_PERIPHERAL || CONFIG_BT_NIMBLE_ROLE_CENTRAL)

# ifdef USING_NIMBLE_ARDUINO_HEADERS
#  include "nimble/nimble/include/nimble/nimble_npl.h"
# else
#  include "nimble/nimble_npl.h"
# endif

# include <functional>
# include <type_traits>
# include <cstdarg>

# ifndef NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
#  define NIMBLE_CPP_ARDUINO_STRING_AVAILABLE (__has_include(<Arduino.h>))
# endif

# if NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
#  include <Stream.h>
# else

// Minimal Stream/Print stubs when Arduino not available
class Print {
  public:
    virtual ~Print() {}
    virtual size_t write(uint8_t)                            = 0;
    virtual size_t write(const uint8_t* buffer, size_t size) = 0;
    size_t         print(const char* s);
    size_t         println(const char* s);
    size_t         printf(const char* format, ...) __attribute__((format(printf, 2, 3)));
};

class Stream : public Print {
  public:
    virtual int   available() = 0;
    virtual int   read()      = 0;
    virtual int   peek()      = 0;
    virtual void  flush() {}
    void          setTimeout(unsigned long timeout) { m_timeout = timeout; }
    unsigned long getTimeout() const { return m_timeout; }

  protected:
    unsigned long m_timeout{0};
};
# endif

class NimBLEStream : public Stream {
  public:
    enum RxOverflowAction {
        DROP_OLDER_DATA, // Drop older buffered data to make room for new data
        DROP_NEW_DATA    // Drop new incoming data when buffer is full
    };

    using RxOverflowCallback = std::function<RxOverflowAction(const uint8_t* data, size_t len, void* userArg)>;

    NimBLEStream() = default;
    virtual ~NimBLEStream() { end(); }

    // Print/Stream TX methods
    virtual size_t write(const uint8_t* data, size_t len) override;
    virtual size_t write(uint8_t data) override { return write(&data, 1); }

    // Template for other integral types (char, int, long, etc.)
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, uint8_t>::value, size_t>::type write(T data) {
        return write(static_cast<uint8_t>(data));
    }

    size_t availableForWrite() const;

    // Read up to len bytes into buffer (non-blocking)
    size_t read(uint8_t* buffer, size_t len);

    // Stream RX methods
    virtual int  available() override;
    virtual int  read() override;
    virtual int  peek() override;
    virtual bool ready() const = 0;

    /**
     * @brief Set a callback to be invoked when incoming data exceeds RX buffer capacity.
     * @param cb The callback function, which should return DROP_OLDER_DATA to drop older buffered data and
     * make room for the new data, or DROP_NEW_DATA to drop the new data instead.
     */
    void setRxOverflowCallback(RxOverflowCallback cb, void* userArg = nullptr) {
        m_rxOverflowCallback = cb;
        m_rxOverflowUserArg  = userArg;
    }

    operator bool() const { return ready(); }

    using Print::write;

    struct ByteRingBuffer;

  protected:
    bool         begin();
    void         drainTx();
    size_t       pushRx(const uint8_t* data, size_t len);
    virtual void end();
    virtual bool send() = 0;
    static void  txDrainEventCb(struct ble_npl_event* ev);
    static void  txDrainCalloutCb(struct ble_npl_event* ev);

    ByteRingBuffer*    m_txBuf{nullptr};
    ByteRingBuffer*    m_rxBuf{nullptr};
    uint8_t            m_txChunkBuf[CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU];
    uint32_t           m_txBufSize{1024};
    uint32_t           m_rxBufSize{1024};
    ble_npl_event      m_txDrainEvent{};
    ble_npl_callout    m_txDrainCallout{};
    RxOverflowCallback m_rxOverflowCallback{nullptr};
    void*              m_rxOverflowUserArg{nullptr};
    bool               m_coInitialized{false};
    bool               m_eventInitialized{false};
};

# if CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#  include "NimBLECharacteristic.h"

class NimBLEStreamServer : public NimBLEStream {
  public:
    NimBLEStreamServer() : m_charCallbacks(this) {}
    ~NimBLEStreamServer() override { end(); }

    // non-copyable
    NimBLEStreamServer(const NimBLEStreamServer&)            = delete;
    NimBLEStreamServer& operator=(const NimBLEStreamServer&) = delete;

    bool begin(NimBLECharacteristic* chr, uint32_t txBufSize = 1024, uint32_t rxBufSize = 1024);

    // Convenience overload to create service/characteristic internally; service will be deleted on end()
    bool begin(const NimBLEUUID& svcUuid,
               const NimBLEUUID& chrUuid,
               uint32_t          txBufSize = 1024,
               uint32_t          rxBufSize = 1024,
               bool              secure    = false);

    void     end() override;
    size_t   write(const uint8_t* data, size_t len) override;
    uint16_t getPeerHandle() const { return m_charCallbacks.m_peerHandle; }
    void     setCallbacks(NimBLECharacteristicCallbacks* pCallbacks) { m_charCallbacks.m_userCallbacks = pCallbacks; }
    bool     ready() const override;
    virtual void flush() override;

    using NimBLEStream::write; // Inherit template write overloads

  protected:
    bool send() override;

    struct ChrCallbacks : public NimBLECharacteristicCallbacks {
        ChrCallbacks(NimBLEStreamServer* parent)
            : m_parent(parent), m_userCallbacks(nullptr), m_peerHandle(BLE_HS_CONN_HANDLE_NONE) {}
        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
        void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override;
        void onStatus(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, int code) override;
        // override this to avoid recursion when debug logs are enabled
        void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
            if (m_userCallbacks != nullptr) {
                m_userCallbacks->onStatus(pCharacteristic, code);
            }
        }

        NimBLEStreamServer*            m_parent;
        NimBLECharacteristicCallbacks* m_userCallbacks;
        uint16_t                       m_peerHandle;
    } m_charCallbacks;

    NimBLECharacteristic* m_pChr{nullptr};
    int                   m_rc{0};
    // Whether to delete the BLE service when end() is called; set to false if service is managed externally
    bool                  m_deleteSvcOnEnd{false};
};
# endif // CONFIG_BT_NIMBLE_ROLE_PERIPHERAL

# if CONFIG_BT_NIMBLE_ROLE_CENTRAL
#  include "NimBLERemoteCharacteristic.h"

class NimBLEStreamClient : public NimBLEStream {
  public:
    NimBLEStreamClient() = default;
    ~NimBLEStreamClient() override { end(); }

    // non-copyable
    NimBLEStreamClient(const NimBLEStreamClient&)            = delete;
    NimBLEStreamClient& operator=(const NimBLEStreamClient&) = delete;

    // Attach a discovered remote characteristic; app owns discovery/connection.
    // Set subscribeNotify=true to receive notifications into RX buffer.
    bool         begin(NimBLERemoteCharacteristic* pChr,
                       bool                        subscribeNotify = false,
                       uint32_t                    txBufSize       = 1024,
                       uint32_t                    rxBufSize       = 1024);
    void         end() override;
    void         setNotifyCallback(NimBLERemoteCharacteristic::notify_callback cb) { m_userNotifyCallback = cb; }
    bool         ready() const override;
    virtual void flush() override;

    using NimBLEStream::write; // Inherit template write overloads

  protected:
    bool send() override;
    void notifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t len, bool isNotify);

    NimBLERemoteCharacteristic*                 m_pChr{nullptr};
    NimBLERemoteCharacteristic::notify_callback m_userNotifyCallback{nullptr};
};
# endif // CONFIG_BT_NIMBLE_ROLE_CENTRAL

#endif // CONFIG_BT_NIMBLE_ENABLED && (CONFIG_BT_NIMBLE_ROLE_PERIPHERAL || CONFIG_BT_NIMBLE_ROLE_CENTRAL)
#endif // NIMBLE_CPP_STREAM_H
