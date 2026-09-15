#pragma once
#if !defined(LITE_VERSION)
#include "BruceBLEService.hpp"

#include <SerialDevice.h>
#include <freertos/semphr.h>
#include <string>

// Nordic UART Service (NUS) - standard UUIDs so any generic BLE tooling
// (nRF Connect, the iOS companion app, ...) can discover and talk to Bruce.
#define NUS_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_CHAR_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // app -> Bruce (write)
#define NUS_TX_CHAR_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // Bruce -> app (notify)

class BLESerialCallbacks;

class BLESerialService : public BruceBLEService, public SerialDevice {
    NimBLECharacteristic *rx_char = nullptr; // written by the central (app)
    NimBLECharacteristic *tx_char = nullptr; // notified to the central (app)
    BLESerialCallbacks *callbacks = nullptr;

    // Bytes received from the central are queued here by the write callback
    // (NimBLE host task) and consumed by the serial-commands task, so access
    // is guarded by a mutex.
    std::string rxBuffer;
    SemaphoreHandle_t rxMutex = nullptr;

    // Outgoing bytes are coalesced here instead of emitting one notification per
    // print()/println() call: a command that prints 40 lines used to cost 40
    // notifications, each waiting for its own connection event. Producers are the
    // serial-commands task and the tft_logger async task, so this is mutex-guarded
    // too (which also keeps a single write() atomic on the wire).
    std::string txBuffer;
    SemaphoreHandle_t txMutex = nullptr;
    // millis() of the oldest buffered byte, 0 when the buffer is empty.
    volatile uint32_t txPendingSince = 0;
    // Set from onSubscribe on the TX characteristic. With nobody subscribed every
    // notify() fails, so without this each chunk would burn the whole retry budget.
    volatile bool txSubscribed = false;

    static constexpr uint32_t TX_FLUSH_INTERVAL_MS = 12; // max time a tail sits buffered
    static constexpr uint8_t TX_NOTIFY_RETRIES = 25;     // backpressure when the stack is full

    size_t txChunkSize() const;
    // Both require txMutex to be held by the caller.
    void drainTx(bool sendPartial);
    void queueTx(const uint8_t *data, size_t len);

public:
    BLESerialService();
    ~BLESerialService() override;
    void setup(NimBLEServer *pServer) override;
    void end() override;
    size_t println() override;
    size_t println(size_t n) override;
    size_t println(const String &s) override;
    size_t println(int n, int format) override;
    size_t print(const String &s) override;
    size_t print(int n, int format = DEC) override;
    void vprintf(const char *str, va_list args) override;
    size_t println(uint32_t n) override;
    size_t write(uint8_t *str, size_t size) override;
    int read() override;
    void flush() override;
    String readStringUntil(char terminator) override;
    int available() override;
    void setMTU(uint16_t mtu);

    // Called by the write callback to enqueue received bytes.
    void feedRx(const uint8_t *data, size_t len);
    // Called by the subscribe callback on the TX characteristic.
    void setSubscribed(bool subscribed) { txSubscribed = subscribed; }
    // Called when the central goes away: nothing can be delivered any more.
    void onDisconnected();
};
#endif
