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

#include "NimBLEStream.h"
#if CONFIG_BT_NIMBLE_ENABLED && (CONFIG_BT_NIMBLE_ROLE_PERIPHERAL || CONFIG_BT_NIMBLE_ROLE_CENTRAL)

# include "NimBLEDevice.h"
# include "NimBLELog.h"
# ifdef USING_NIMBLE_ARDUINO_HEADERS
#  include "nimble/porting/nimble/include/os/os_mbuf.h"
#  include "nimble/porting/nimble/include/nimble/nimble_port.h"
# else
#  include "os/os_mbuf.h"
#  include "nimble/nimble_port.h"
# endif
# include <algorithm>
# include <cstdio>
# include <cstdlib>
# include <cstring>

static const char* LOG_TAG = "NimBLEStream";

struct NimBLEStream::ByteRingBuffer {
    /** @brief Guard for ByteRingBuffer to manage locking. */
    struct Guard {
        const ByteRingBuffer& _b;
        bool                  _locked;
        Guard(const ByteRingBuffer& b) : _b(b), _locked(b.lock()) {}
        ~Guard() {
            if (_locked) _b.unlock();
        }
        operator bool() const { return _locked; } // Allows: if (Guard g{*this}) { ... }
    };

    /** @brief Construct a ByteRingBuffer with the specified capacity. */
    explicit ByteRingBuffer(size_t capacity) : m_capacity(capacity) {
        memset(&m_mutex, 0, sizeof(m_mutex));
        auto rc = ble_npl_mutex_init(&m_mutex);
        if (rc != BLE_NPL_OK) {
            NIMBLE_LOGE(LOG_TAG, "Failed to initialize ring buffer mutex, error: %d", rc);
            return;
        }

        m_buf = static_cast<uint8_t*>(malloc(capacity));
        if (!m_buf) {
            NIMBLE_LOGE(LOG_TAG, "Failed to allocate ring buffer memory");
            ble_npl_mutex_deinit(&m_mutex);
            return;
        }
    }

    /** @brief Destroy the ByteRingBuffer and release resources. */
    ~ByteRingBuffer() {
        if (m_buf) {
            free(m_buf);
        }
        ble_npl_mutex_deinit(&m_mutex);
    }

    /** @brief Check if the ByteRingBuffer is valid. */
    bool valid() const { return m_buf != nullptr; }

    /** @brief Get the capacity of the ByteRingBuffer. */
    size_t capacity() const { return m_capacity; }

    /** @brief Get the current size of the ByteRingBuffer. */
    size_t size() const {
        Guard g(*this);
        return g ? m_size : 0;
    }

    /** @brief Get the available free space in the ByteRingBuffer. */
    size_t freeSize() const {
        Guard g(*this);
        return g ? m_capacity - m_size : 0;
    }

    /**
     * @brief Write data to the ByteRingBuffer.
     * @param data Pointer to the data to write.
     * @param len Length of the data to write.
     * @returns the number of bytes actually written, which may be less than len if the buffer does not have enough free space.
     */
    size_t write(const uint8_t* data, size_t len) {
        if (!data || len == 0) {
            return 0;
        }

        Guard g(*this);
        if (!g || m_size >= m_capacity) {
            return 0;
        }

        size_t count = std::min(len, m_capacity - m_size);
        size_t first = std::min(count, m_capacity - m_head);
        memcpy(m_buf + m_head, data, first);
        size_t remain = count - first;
        if (remain > 0) {
            memcpy(m_buf, data + first, remain);
        }

        m_head  = (m_head + count) % m_capacity;
        m_size += count;
        return count;
    }

    /**
     * @brief Read data from the ByteRingBuffer.
     * @param out Pointer to the buffer where read data will be stored.
     * @param len Maximum number of bytes to read.
     * @returns the number of bytes actually read.
     */
    size_t read(uint8_t* out, size_t len) {
        if (!out || len == 0) {
            return 0;
        }

        Guard g(*this);
        if (!g || m_size == 0) {
            return 0;
        }

        size_t count = std::min(len, m_size);
        size_t first = std::min(count, m_capacity - m_tail);
        memcpy(out, m_buf + m_tail, first);
        size_t remain = count - first;
        if (remain > 0) {
            memcpy(out + first, m_buf, remain);
        }

        m_tail  = (m_tail + count) % m_capacity;
        m_size -= count;
        return count;
    }

    /**
     * @brief Peek at data in the ByteRingBuffer without removing it.
     * @param out Pointer to the buffer where peeked data will be stored.
     * @param len Maximum number of bytes to peek.
     * @returns the number of bytes actually peeked.
     */
    size_t peek(uint8_t* out, size_t len) {
        if (!out || len == 0) {
            return 0;
        }

        Guard g(*this);
        if (!g || m_size == 0) {
            return 0;
        }

        size_t count = std::min(len, m_size);
        size_t first = std::min(count, m_capacity - m_tail);
        memcpy(out, m_buf + m_tail, first);
        size_t remain = count - first;
        if (remain > 0) {
            memcpy(out + first, m_buf, remain);
        }

        return count;
    }

    /**
     * @brief Drop data from the ByteRingBuffer without reading it.
     * @param len Maximum number of bytes to drop.
     * @returns the number of bytes actually dropped.
     */
    size_t drop(size_t len) {
        if (len == 0) {
            return 0;
        }

        Guard g(*this);
        if (!g || m_size == 0) {
            return 0;
        }
        size_t count  = std::min(len, m_size);
        m_tail        = (m_tail + count) % m_capacity;
        m_size       -= count;
        return count;
    }

  private:
    /**
     * @brief Lock the ByteRingBuffer for exclusive access.
     * @return true if the lock was successfully acquired, false otherwise.
     */
    bool lock() const { return valid() && ble_npl_mutex_pend(&m_mutex, BLE_NPL_TIME_FOREVER) == BLE_NPL_OK; }

    /**
     * @brief Unlock the ByteRingBuffer after exclusive access.
     */
    void unlock() const { ble_npl_mutex_release(&m_mutex); }

    uint8_t*              m_buf{nullptr};
    size_t                m_capacity{0};
    size_t                m_head{0};
    size_t                m_tail{0};
    size_t                m_size{0};
    mutable ble_npl_mutex m_mutex{};
};

// Stub Print/Stream implementations when Arduino not available
# ifndef NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
#  include <cstring>

size_t Print::print(const char* s) {
    if (!s) return 0;
    return write(reinterpret_cast<const uint8_t*>(s), strlen(s));
}

size_t Print::println(const char* s) {
    size_t            n       = print(s);
    static const char crlf[]  = "\r\n";
    n                        += write(reinterpret_cast<const uint8_t*>(crlf), 2);
    return n;
}

size_t Print::printf(const char* fmt, ...) {
    if (!fmt) {
        return 0;
    }

    char    stackBuf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(stackBuf, sizeof(stackBuf), fmt, ap);
    va_end(ap);
    if (n < 0) {
        return 0;
    }

    if (static_cast<size_t>(n) < sizeof(stackBuf)) {
        return write(reinterpret_cast<const uint8_t*>(stackBuf), static_cast<size_t>(n));
    }

    // allocate for larger output
    size_t needed = static_cast<size_t>(n) + 1;
    char*  buf    = static_cast<char*>(malloc(needed));
    if (!buf) {
        return 0;
    }

    va_start(ap, fmt);
    vsnprintf(buf, needed, fmt, ap);
    va_end(ap);
    size_t ret = write(reinterpret_cast<const uint8_t*>(buf), static_cast<size_t>(n));
    free(buf);
    return ret;
}
# endif

/**
 * @brief Initialize the NimBLEStream, creating TX and RX buffers and setting up events.
 * @return true if initialization was successful, false otherwise.
 */
bool NimBLEStream::begin() {
    if (m_txBuf || m_rxBuf) {
        NIMBLE_LOGW(LOG_TAG, "Already initialized");
        return true;
    }

    if (m_txBufSize == 0 && m_rxBufSize == 0) {
        NIMBLE_LOGE(LOG_TAG, "Cannot initialize stream with both TX and RX buffer sizes set to 0");
        return false;
    }

    if (ble_npl_callout_init(&m_txDrainCallout, nimble_port_get_dflt_eventq(), NimBLEStream::txDrainCalloutCb, this) != 0) {
        NIMBLE_LOGE(LOG_TAG, "Failed to initialize TX drain callout");
        return false;
    }
    m_coInitialized = true;

    ble_npl_event_init(&m_txDrainEvent, NimBLEStream::txDrainEventCb, this);
    m_eventInitialized = true;

    if (m_txBufSize) {
        m_txBuf = new ByteRingBuffer(m_txBufSize);
        if (!m_txBuf || !m_txBuf->valid()) {
            NIMBLE_LOGE(LOG_TAG, "Failed to create TX ringbuffer");
            end();
            return false;
        }
    }

    if (m_rxBufSize) {
        m_rxBuf = new ByteRingBuffer(m_rxBufSize);
        if (!m_rxBuf || !m_rxBuf->valid()) {
            NIMBLE_LOGE(LOG_TAG, "Failed to create RX ringbuffer");
            end();
            return false;
        }
    }

    return true;
}

/**
 * @brief Clean up the NimBLEStream, stopping events and deleting buffers.
 */
void NimBLEStream::end() {
    if (m_coInitialized) {
        ble_npl_callout_stop(&m_txDrainCallout);
        ble_npl_callout_deinit(&m_txDrainCallout);
        m_coInitialized = false;
    }

    if (m_eventInitialized) {
        ble_npl_eventq_remove(nimble_port_get_dflt_eventq(), &m_txDrainEvent);
        ble_npl_event_deinit(&m_txDrainEvent);
        m_eventInitialized = false;
    }

    if (m_txBuf) {
        delete m_txBuf;
        m_txBuf = nullptr;
    }

    if (m_rxBuf) {
        delete m_rxBuf;
        m_rxBuf = nullptr;
    }
}

/**
 * @brief Write data to the stream, which will be sent over BLE.
 * @param data Pointer to the data to write.
 * @param len Length of the data to write.
 * @return the number of bytes actually written to the stream buffer.
 */
size_t NimBLEStream::write(const uint8_t* data, size_t len) {
    if (!m_txBuf) {
        return 0;
    }

    auto written = m_txBuf->write(data, len);
    drainTx();
    return written;
}

/**
 * @brief Get the available free space in the stream's TX buffer.
 * @return the number of bytes that can be written to the stream without blocking.
 */
size_t NimBLEStream::availableForWrite() const {
    return m_txBuf ? m_txBuf->freeSize() : 0;
}

/**
 * @brief Schedule the stream to attempt to send data from the TX buffer.
 * @details This should be called whenever new data is written to the TX buffer
 * or when a send attempt fails due to lack of BLE buffers.
 */
void NimBLEStream::drainTx() {
    if (!m_txBuf || m_txBuf->size() == 0) {
        return;
    }

    ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &m_txDrainEvent);
}

/**
 * @brief Event callback for when the stream is scheduled to drain the TX buffer.
 * @param ev Pointer to the event that triggered the callback.
 * @details This will attempt to send data from the TX buffer. If sending fails due to
 * lack of BLE buffers, it will reschedule itself to try again after a short delay.
 */
void NimBLEStream::txDrainEventCb(struct ble_npl_event* ev) {
    if (!ev) {
        return;
    }

    auto* stream = static_cast<NimBLEStream*>(ble_npl_event_get_arg(ev));
    if (!stream) {
        return;
    }

    if (stream->send()) {
        // Schedule a short delayed retry to give the stack time to free buffers, use 5ms for now
        // TODO: consider options for the delay time and retry strategy if the stack is persistently out of buffers
        ble_npl_callout_reset(&stream->m_txDrainCallout, ble_npl_time_ms_to_ticks32(5));
    }
}

/**
 * @brief Callout callback for when the stream is scheduled to retry draining the TX buffer.
 * @param ev Pointer to the event that triggered the callback.
 * @details This will call drainTx() to attempt to send data from the TX buffer again.
 */
void NimBLEStream::txDrainCalloutCb(struct ble_npl_event* ev) {
    if (!ev) {
        return;
    }

    auto* stream = static_cast<NimBLEStream*>(ble_npl_event_get_arg(ev));
    if (!stream) {
        return;
    }

    stream->drainTx();
}

/**
 * @brief Get the number of bytes available to read from the stream.
 * @return the number of bytes that can be read from the stream.
 */
int NimBLEStream::available() {
    if (!m_rxBuf) {
        return 0;
    }

    return static_cast<int>(m_rxBuf->size());
}

/**
 * @brief Read a single byte from the stream.
 * @return the byte read as an int, or -1 if no data is available.
 */
int NimBLEStream::read() {
    if (!m_rxBuf) {
        return -1;
    }

    uint8_t byte = 0;
    if (m_rxBuf->read(&byte, 1) == 0) {
        return -1;
    }

    return static_cast<int>(byte);
}

/**
 * @brief Peek at the next byte in the stream without removing it.
 * @return the byte peeked as an int, or -1 if no data is available.
 */
int NimBLEStream::peek() {
    if (!m_rxBuf) {
        return -1;
    }

    uint8_t byte = 0;
    if (m_rxBuf->peek(&byte, 1) == 0) {
        return -1;
    }

    return static_cast<int>(byte);
}

/**
 * @brief Read data from the stream into a buffer.
 * @param buffer Pointer to the buffer where read data will be stored.
 * @param len Maximum number of bytes to read.
 * @return the number of bytes actually read from the stream.
 */
size_t NimBLEStream::read(uint8_t* buffer, size_t len) {
    if (!m_rxBuf) {
        return 0;
    }

    return m_rxBuf->read(buffer, len);
}

/**
 * @brief Push received data into the stream's RX buffer.
 * @param data Pointer to the data to push into the RX buffer.
 * @param len Length of the data to push.
 * @return the number of bytes actually pushed into the RX buffer, which may be less than
 * len if the buffer does not have enough free space.
 */
size_t NimBLEStream::pushRx(const uint8_t* data, size_t len) {
    if (!ready()) {
        return 0;
    }

    ByteRingBuffer::Guard g(*m_rxBuf);
    if (!g) {
        NIMBLE_LOGE(LOG_TAG, "Failed to acquire RX buffer lock to push data");
        return 0;
    }

    size_t freeSize = m_rxBuf->freeSize();
    if (len > freeSize) {
        const RxOverflowAction action = m_rxOverflowCallback ? m_rxOverflowCallback(data, len, m_rxOverflowUserArg)
                                                             : DROP_NEW_DATA;
        if (action != DROP_OLDER_DATA) {
            NIMBLE_LOGE(LOG_TAG, "RX buffer overflow, dropping current data");
            return 0;
        }

        if (len >= m_rxBuf->capacity()) {
            m_rxBuf->drop(m_rxBuf->size());
            const uint8_t* tail    = data + (len - m_rxBuf->capacity());
            size_t         written = m_rxBuf->write(tail, m_rxBuf->capacity());
            if (written != m_rxBuf->capacity()) {
                NIMBLE_LOGE(LOG_TAG, "RX buffer overflow, %zu bytes dropped", m_rxBuf->capacity() - written);
            }
            return written;
        }

        const size_t requiredSpace = len - freeSize;
        size_t       dropped       = m_rxBuf->drop(requiredSpace);
        if (dropped < requiredSpace) {
            NIMBLE_LOGE(LOG_TAG, "RX buffer overflow, failed to drop enough buffered data");
            return 0;
        }
    }

    return m_rxBuf->write(data, len);
}

# if CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
/**
 * @brief Initialize the NimBLEStreamServer with an existing characteristic.
 * @param pChr Pointer to the existing NimBLECharacteristic to use for the stream.
 * @param txBufSize Size of the TX buffer.
 * @param rxBufSize Size of the RX buffer.
 * @return true if initialization was successful, false otherwise.
 * @details The provided characteristic must have the NOTIFY property set to allow the server
 * to send data (TX) to the client, and the WRITE or WRITE_NR property set to allow the server
 * to receive data (RX) from the client.
 * The RX buffer will only be created if the characteristic has WRITE or WRITE_NR properties
 * (i.e. can receive data from the client).
 * The TX buffer will only be created if the characteristic has NOTIFY properties
 * (i.e. can send data to the client).
 */
bool NimBLEStreamServer::begin(NimBLECharacteristic* pChr, uint32_t txBufSize, uint32_t rxBufSize) {
    if (!NimBLEDevice::isInitialized()) {
        return false;
    }

    if (m_pChr) {
        NIMBLE_LOGW(LOG_TAG, "Already initialized with a characteristic");
        return true;
    }

    if (!pChr) {
        NIMBLE_LOGE(LOG_TAG, "Characteristic is null");
        return false;
    }

    auto props    = pChr->getProperties();
    bool canWrite = (props & NIMBLE_PROPERTY::WRITE) || (props & NIMBLE_PROPERTY::WRITE_NR);
    if (!canWrite && rxBufSize > 0) {
        NIMBLE_LOGW(LOG_TAG, "Characteristic does not support WRITE, ignoring RX buffer size");
    }

    bool canNotify = props & NIMBLE_PROPERTY::NOTIFY;
    if (!canNotify && txBufSize > 0) {
        NIMBLE_LOGW(LOG_TAG, "Characteristic does not support NOTIFY, ignoring TX buffer size");
    }

    m_rxBufSize = canWrite ? rxBufSize : 0;  // disable RX if not writable
    m_txBufSize = canNotify ? txBufSize : 0; // disable TX if notifications not supported

    if (!NimBLEStream::begin()) {
        NIMBLE_LOGE(LOG_TAG, "Failed to initialize stream buffers");
        return false;
    }

    m_charCallbacks.m_userCallbacks = pChr->getCallbacks();
    pChr->setCallbacks(&m_charCallbacks);
    m_pChr = pChr;
    return true;
}

/**
 * @brief Initialize the NimBLEStreamServer, creating a BLE service and characteristic for streaming.
 * @param svcUuid UUID of the BLE service to create.
 * @param chrUuid UUID of the BLE characteristic to create.
 * @param txBufSize Size of the TX buffer, set to 0 to disable TX and create a write-only characteristic.
 * @param rxBufSize Size of the RX buffer, set to 0 to disable RX and create a notify-only characteristic.
 * @param secure Whether the characteristic requires encryption.
 * @return true if initialization was successful, false otherwise.
 */
bool NimBLEStreamServer::begin(
    const NimBLEUUID& svcUuid, const NimBLEUUID& chrUuid, uint32_t txBufSize, uint32_t rxBufSize, bool secure) {
    if (!NimBLEDevice::isInitialized()) {
        NIMBLE_LOGE(LOG_TAG, "NimBLEDevice not initialized");
        return false;
    }

    if (m_pChr != nullptr) {
        NIMBLE_LOGE(LOG_TAG, "NimBLEStreamServer already initialized;");
        return false;
    }

    NimBLEServer* pServer = NimBLEDevice::getServer();
    if (!pServer) {
        pServer = NimBLEDevice::createServer();
    }

    auto pSvc = pServer->createService(svcUuid);
    if (!pSvc) {
        return false;
    }

    // Create characteristic with notify + write properties for bidirectional stream
    uint32_t props = 0;
    if (txBufSize > 0) {
        props |= NIMBLE_PROPERTY::NOTIFY;
        if (secure) {
            props |= NIMBLE_PROPERTY::READ_ENC;
        }
    }

    if (rxBufSize > 0) {
        props |= NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR;
        if (secure) {
            props |= NIMBLE_PROPERTY::WRITE_ENC;
        }
    }

    auto pChr = pSvc->createCharacteristic(chrUuid, props);
    if (!pChr) {
        NIMBLE_LOGE(LOG_TAG, "Failed to create characteristic");
        goto error;
    }

    m_deleteSvcOnEnd = true; // mark service for deletion on end since we created it here

    if (!begin(pChr, txBufSize, rxBufSize)) {
        NIMBLE_LOGE(LOG_TAG, "Failed to initialize stream with characteristic");
        goto error;
    }

    return true;

error:
    pServer->removeService(pSvc, true); // delete service and all its characteristics
    m_pChr = nullptr;                   // reset characteristic pointer as it's now invalid after service removal
    end();
    return false;
}

/**
 * @brief Stop the NimBLEStreamServer
 * @details This will stop the stream and delete the service created if it was created by this class.
 */
void NimBLEStreamServer::end() {
    if (m_pChr) {
        if (m_deleteSvcOnEnd) {
            auto pSvc = m_pChr->getService();
            if (pSvc) {
                auto pServer = pSvc->getServer();
                if (pServer) {
                    pServer->removeService(pSvc, true);
                }
            }
        } else {
            m_pChr->setCallbacks(m_charCallbacks.m_userCallbacks); // restore any user callbacks
        }
    }

    m_pChr                          = nullptr;
    m_charCallbacks.m_peerHandle    = BLE_HS_CONN_HANDLE_NONE;
    m_charCallbacks.m_userCallbacks = nullptr;
    m_deleteSvcOnEnd                = false;
    NimBLEStream::end();
}

/**
 * @brief Write data to the stream, which will be sent as BLE notifications.
 * @param data Pointer to the data to write.
 * @param len Length of the data to write.
 * @return the number of bytes actually written to the stream buffer.
 */
size_t NimBLEStreamServer::write(const uint8_t* data, size_t len) {
    if (!m_pChr || len == 0 || !ready()) {
        return 0;
    }

#  if CONFIG_NIMBLE_CPP_LOG_LEVEL >= 4
    // Skip server gap events to avoid log recursion
    static const char filterStr[] = "handleGapEvent";
    constexpr size_t  filterLen   = sizeof(filterStr) - 1;
    if (len >= filterLen + 3) {
        for (size_t i = 3; i <= len - filterLen; i++) {
            if (memcmp(data + i, filterStr, filterLen) == 0) {
                return len; // drop to avoid recursion
            }
        }
    }
#  endif

    return NimBLEStream::write(data, len);
}

/**
 * @brief Flush all pending TX data by attempting immediate sends.
 * @details This blocks while trying to drain the TX buffer. If send cannot make
 * progress (e.g. disconnected or persistent failure), queued TX/RX data is cleared.
 */
void NimBLEStreamServer::flush() {
    if (!m_txBuf || m_txBuf->size() == 0) {
        return;
    }

    const uint32_t timeoutMs  = static_cast<uint32_t>(std::min<unsigned long>(getTimeout(), 0xFFFFFFFFUL));
    const uint32_t retryDelay = std::max<uint32_t>(1, ble_npl_time_ms_to_ticks32(5));
    uint32_t       waitStart  = ble_npl_time_get();
    while (m_txBuf->size() > 0) {
        size_t before = m_txBuf->size();
        bool   retry  = send();
        size_t after  = m_txBuf->size();

        if (after == 0) {
            return;
        }

        if (after < before) {
            waitStart = ble_npl_time_get();
            continue;
        }

        if (retry && timeoutMs > 0) {
            const uint32_t elapsed = ble_npl_time_get() - waitStart;
            if (elapsed < ble_npl_time_ms_to_ticks32(timeoutMs)) {
                ble_npl_time_delay(retryDelay);
                continue;
            }
        }

        m_txBuf->drop(m_txBuf->size());
        if (m_rxBuf) {
            m_rxBuf->drop(m_rxBuf->size());
        }
        return;
    }
}

/**
 * @brief Attempt to send data from the TX buffer as BLE notifications.
 * @return true if a retry should be scheduled, false otherwise.
 * @details This will try to send as much data as possible from the TX buffer in chunks
 * that fit within the current BLE MTU. If sending fails due to lack of BLE buffers, it will return true
 * to indicate that a retry should be scheduled, but it will not drop any data from the TX buffer.
 * For other errors or if all data is sent, it returns false.
 */
bool NimBLEStreamServer::send() {
    if (!m_pChr || !m_txBuf || !ready()) {
        return false;
    }

    size_t mtu = ble_att_mtu(getPeerHandle());
    if (mtu < 23) {
        return false;
    }

    size_t maxDataLen = std::min<size_t>(mtu - 3, sizeof(m_txChunkBuf));

    while (m_txBuf->size()) {
        size_t chunkLen = m_txBuf->peek(m_txChunkBuf, maxDataLen);
        if (!chunkLen) {
            break;
        }

        if (!m_pChr->notify(m_txChunkBuf, chunkLen, getPeerHandle())) {
            if (m_rc == BLE_HS_ENOMEM || os_msys_num_free() <= 2) {
                // NimBLE stack out of buffers, likely due to pending notifications/indications
                // Don't drop data, but wait for stack to free buffers and try again later
                return true;
            }

            return false; // disconnect or other error don't retry send, preserve data for next attempt
        }

        m_txBuf->drop(chunkLen);
    }

    return false; // no more data to send
}

/**
 * @brief Check if the stream is ready to send/receive data, which requires an active BLE connection.
 * @return true if the stream is ready, false otherwise.
 */
bool NimBLEStreamServer::ready() const {
    if (m_txBufSize > 0 && !m_txBuf) {
        return false;
    }

    if (m_rxBufSize > 0 && !m_rxBuf) {
        return false;
    }

    // Write-only mode has no TX peer tracking requirements.
    if (m_txBufSize == 0) {
        return m_rxBufSize > 0;
    }

    if (m_charCallbacks.m_peerHandle == BLE_HS_CONN_HANDLE_NONE) {
        return false;
    }

    return ble_gap_conn_find(m_charCallbacks.m_peerHandle, nullptr) == 0;
}

/**
 * @brief Callback for when the characteristic is written to by a client.
 * @param pChr Pointer to the characteristic that was written to.
 * @param connInfo Information about the connection that performed the write.
 * @details This will push the received data into the RX buffer and call any user-defined callbacks.
 */
void NimBLEStreamServer::ChrCallbacks::onWrite(NimBLECharacteristic* pChr, NimBLEConnInfo& connInfo) {
    // Push received data into RX buffer
    auto val = pChr->getValue();
    m_parent->pushRx(val.data(), val.size());

    if (m_userCallbacks) {
        m_userCallbacks->onWrite(pChr, connInfo);
    }
}

/**
 * @brief Callback for when a client subscribes or unsubscribes to notifications/indications.
 * @param pChr Pointer to the characteristic that was subscribed/unsubscribed.
 * @param connInfo Information about the connection that performed the subscribe/unsubscribe.
 * @param subValue The new subscription value (0 for unsubscribe, non-zero for subscribe).
 * @details This will track the subscriber's connection handle and call any user-defined callbacks.
 * Only one subscriber is supported; if another client tries to subscribe while one is already subscribed, it will be ignored.
 */
void NimBLEStreamServer::ChrCallbacks::onSubscribe(NimBLECharacteristic* pChr, NimBLEConnInfo& connInfo, uint16_t subValue) {
    // If we have a stored peer handle, ensure it still refers to an active connection.
    // If the connection has gone away without an explicit unsubscribe, clear it so a new
    // subscriber can be accepted.
    if (m_peerHandle != BLE_HS_CONN_HANDLE_NONE) {
        if (ble_gap_conn_find(m_peerHandle, nullptr) != 0) {
            m_peerHandle = BLE_HS_CONN_HANDLE_NONE;
        }
    }

    // only one subscriber supported
    if (subValue && m_peerHandle != BLE_HS_CONN_HANDLE_NONE) {
        NIMBLE_LOGI(LOG_TAG,
                    "Already have a subscriber, rejecting new subscription from conn handle %d",
                    connInfo.getConnHandle());
        return;
    }

    m_peerHandle = subValue ? connInfo.getConnHandle() : BLE_HS_CONN_HANDLE_NONE;
    if (m_userCallbacks) {
        m_userCallbacks->onSubscribe(pChr, connInfo, subValue);
    }
}

/**
 * @brief Callback for when the connection status changes (e.g. disconnect).
 * @param pChr Pointer to the characteristic associated with the status change.
 * @param connInfo Information about the connection that changed status.
 * @param code The new status code (e.g. success or error code).
 * @details The code is used to track when the stack is out of buffers (BLE_HS_ENOMEM)
 * to trigger retries without dropping data. User-defined callbacks will also be called if set.
 */
void NimBLEStreamServer::ChrCallbacks::onStatus(NimBLECharacteristic* pChr, NimBLEConnInfo& connInfo, int code) {
    m_parent->m_rc = code;
    if (m_userCallbacks) {
        m_userCallbacks->onStatus(pChr, connInfo, code);
    }
}

# endif // CONFIG_BT_NIMBLE_ROLE_PERIPHERAL

# if CONFIG_BT_NIMBLE_ROLE_CENTRAL
/**
 * @brief Initialize the NimBLEStreamClient, setting up the remote characteristic and subscribing to notifications if requested.
 * @param pChr Pointer to the remote characteristic to use for streaming.
 * @param subscribe Whether to subscribe to notifications/indications from the characteristic for RX.
 * @param txBufSize Size of the TX buffer.
 * @param rxBufSize Size of the RX buffer.
 * @return true if initialization was successful, false otherwise.
 * @details The characteristic must support write without response for TX.
 * If subscribe is true, it will subscribe to notifications/indications for RX.
 * If subscribe is false, the RX buffer will not be created and no notifications will be received.
 */
bool NimBLEStreamClient::begin(NimBLERemoteCharacteristic* pChr, bool subscribe, uint32_t txBufSize, uint32_t rxBufSize) {
    if (!NimBLEDevice::isInitialized()) {
        NIMBLE_LOGE(LOG_TAG, "NimBLE stack not initialized, call NimBLEDevice::init() first");
        return false;
    }

    if (m_pChr) {
        NIMBLE_LOGW(LOG_TAG, "Already initialized, must end() first");
        return true;
    }

    if (!pChr) {
        NIMBLE_LOGE(LOG_TAG, "Remote characteristic is null");
        return false;
    }

    if (!pChr->canWriteNoResponse()) {
        NIMBLE_LOGE(LOG_TAG, "Characteristic does not support write without response");
        return false;
    }

    if (subscribe && !pChr->canNotify() && !pChr->canIndicate()) {
        NIMBLE_LOGW(LOG_TAG, "Characteristic does not support subscriptions, RX disabled");
        subscribe = false; // disable subscribe if not supported
    }

    m_txBufSize = txBufSize;
    m_rxBufSize = subscribe ? rxBufSize : 0; // disable RX buffer if not subscribing

    if (!NimBLEStream::begin()) {
        NIMBLE_LOGE(LOG_TAG, "Failed to initialize stream buffers");
        return false;
    }

    // Subscribe to notifications/indications for RX if requested
    if (subscribe) {
        using namespace std::placeholders;
        if (!pChr->subscribe(pChr->canNotify(), std::bind(&NimBLEStreamClient::notifyCallback, this, _1, _2, _3, _4))) {
            NIMBLE_LOGE(LOG_TAG, "Failed to subscribe for %s", pChr->canNotify() ? "notifications" : "indications");
            end();
            return false;
        }
    }

    m_pChr = pChr;
    return true;
}

/**
 * @brief Clean up the NimBLEStreamClient, unsubscribing from notifications and clearing the remote characteristic reference.
 */
void NimBLEStreamClient::end() {
    if (m_pChr && (m_pChr->canNotify() || m_pChr->canIndicate())) {
        m_pChr->unsubscribe();
    }

    m_pChr = nullptr;
    NimBLEStream::end();
}

/**
 * @brief Flush all pending TX data by attempting immediate writes.
 * @details This blocks while trying to drain the TX buffer. If send cannot make
 * progress (e.g. disconnected or persistent failure), queued TX/RX data is cleared.
 */
void NimBLEStreamClient::flush() {
    if (!m_txBuf || m_txBuf->size() == 0) {
        return;
    }

    const uint32_t timeoutMs  = static_cast<uint32_t>(std::min<unsigned long>(getTimeout(), 0xFFFFFFFFUL));
    const uint32_t retryDelay = std::max<uint32_t>(1, ble_npl_time_ms_to_ticks32(5));
    uint32_t       waitStart  = ble_npl_time_get();
    while (m_txBuf->size() > 0) {
        size_t before = m_txBuf->size();
        bool   retry  = send();
        size_t after  = m_txBuf->size();

        if (after == 0) {
            return;
        }

        if (after < before) {
            waitStart = ble_npl_time_get();
            continue;
        }

        if (retry && timeoutMs > 0) {
            const uint32_t elapsed = ble_npl_time_get() - waitStart;
            if (elapsed < ble_npl_time_ms_to_ticks32(timeoutMs)) {
                ble_npl_time_delay(retryDelay);
                continue;
            }
        }

        m_txBuf->drop(m_txBuf->size());
        if (m_rxBuf) {
            m_rxBuf->drop(m_rxBuf->size());
        }
        return;
    }
}

/**
 * @brief Write data to the stream, which will be sent as BLE writes to the remote characteristic.
 * @return True if a retry should be scheduled due to lack of BLE buffers, false otherwise.
 * @details This will try to send as much data as possible from the TX buffer in chunks
 * that fit within the memory buffers. If sending fails due to lack of BLE buffers, it will return true
 * to indicate that a retry should be scheduled, but it will not drop any data from the TX buffer.
 * For other errors or if all data is sent, it returns false.
 */
bool NimBLEStreamClient::send() {
    if (!ready()) {
        return false;
    }

    auto mtu = m_pChr->getClient()->getMTU();
    if (mtu < 23) {
        return false;
    }

    size_t maxDataLen = std::min<size_t>(mtu - 3, sizeof(m_txChunkBuf));

    while (m_txBuf->size()) {
        size_t chunkLen = m_txBuf->peek(m_txChunkBuf, maxDataLen);
        if (!chunkLen) {
            break;
        }

        if (!m_pChr->writeValue(m_txChunkBuf, chunkLen, false)) {
            if (os_msys_num_free() <= 2) {
                // NimBLE stack out of buffers, likely due to pending writes
                // Don't drop data, wait for stack to free buffers and try again later
                return true;
            }

            break; // preserve data, no retry
        }

        m_txBuf->drop(chunkLen);
    }

    return false; // don't retry, it's either sent or we are disconnected
}

/**
 * @brief Check if the stream is ready for communication.
 * @return true if the stream is ready, false otherwise.
 * @details The stream is considered ready if buffers are allocated, the remote characteristic is set,
 * and the client connection is active.
 */
bool NimBLEStreamClient::ready() const {
    if (m_txBufSize > 0 && !m_txBuf) {
        return false;
    }

    if (m_rxBufSize > 0 && !m_rxBuf) {
        return false;
    }

    return m_pChr != nullptr && m_pChr->getClient()->isConnected();
}

/**
 * @brief Callback for when a notification or indication is received from the remote characteristic.
 * @param pChar Pointer to the characteristic that sent the notification/indication.
 * @param pData Pointer to the data received in the notification/indication.
 * @param len Length of the data received.
 * @param isNotify True if the data was received as a notification, false if it was received as an indication.
 * @details This will push the received data into the RX buffer and call any user-defined callbacks.
 */
void NimBLEStreamClient::notifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t len, bool isNotify) {
    pushRx(pData, len);
    if (m_userNotifyCallback) {
        m_userNotifyCallback(pChar, pData, len, isNotify);
    }
}
# endif // CONFIG_BT_NIMBLE_ROLE_CENTRAL
#endif  // CONFIG_BT_NIMBLE_ENABLED && (CONFIG_BT_NIMBLE_ROLE_PERIPHERAL || CONFIG_BT_NIMBLE_ROLE_CENTRAL)
