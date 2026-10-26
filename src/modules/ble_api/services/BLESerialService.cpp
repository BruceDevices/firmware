#if !defined(LITE_VERSION)
#include "BLESerialService.h"
#include "modules/ble/ble_common.h" // bleNotifyRetry
#include <NimBLEDevice.h>
#include <vector>

BLESerialService::BLESerialService() : BruceBLEService() {
    rxMutex = xSemaphoreCreateMutex();
    txMutex = xSemaphoreCreateMutex();
}

BLESerialService::~BLESerialService() {
    if (rxMutex) vSemaphoreDelete(rxMutex);
    if (txMutex) vSemaphoreDelete(txMutex);
}

class BLESerialCallbacks : public NimBLECharacteristicCallbacks {
    BLESerialService *service;

public:
    explicit BLESerialCallbacks(BLESerialService *service) : service(service) {}

    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
        std::string value = pCharacteristic->getValue();
        if (!value.empty())
            service->feedRx(reinterpret_cast<const uint8_t *>(value.data()), value.size());
    }

    // Fires on the TX characteristic's CCCD. Bit 0 is "notifications enabled".
    void onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue)
        override {
        service->setSubscribed((subValue & 0x0001) != 0);
    }
};

void BLESerialService::setup(NimBLEServer *pServer) {
    pService = pServer->createService(NUS_SERVICE_UUID);

    // App -> Bruce. WRITE and WRITE_NR so the app can use fast writeWithoutResponse.
    rx_char = pService->createCharacteristic(
        NUS_RX_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    callbacks = new BLESerialCallbacks(this);
    rx_char->setCallbacks(callbacks);

    // Bruce -> app. Same callbacks object: onWrite never fires here (no write
    // property), but onSubscribe does, which is what tells us anyone is listening.
    tx_char = pService->createCharacteristic(NUS_TX_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);
    tx_char->setCallbacks(callbacks);

    pService->start();
    pServer->getAdvertising()->addServiceUUID(pService->getUUID());
}

void BLESerialService::end() {
    // Drop the characteristics first: BLEDevice::deinit() frees them right after
    // this returns, and other tasks (tft_logger) may still be writing.
    txSubscribed = false;
    if (rx_char) rx_char->setCallbacks(nullptr);
    if (tx_char) tx_char->setCallbacks(nullptr);
    rx_char = nullptr;
    tx_char = nullptr;

    delete callbacks;
    callbacks = nullptr;
    if (rxMutex && xSemaphoreTake(rxMutex, portMAX_DELAY) == pdTRUE) {
        rxBuffer.clear();
        xSemaphoreGive(rxMutex);
    }
    if (txMutex && xSemaphoreTake(txMutex, portMAX_DELAY) == pdTRUE) {
        txBuffer.clear();
        txPendingSince = 0;
        xSemaphoreGive(txMutex);
    }
}

void BLESerialService::onDisconnected() {
    txSubscribed = false;
    mtu = 23; // renegotiated on the next connection
    if (txMutex && xSemaphoreTake(txMutex, portMAX_DELAY) == pdTRUE) {
        txBuffer.clear();
        txPendingSince = 0;
        xSemaphoreGive(txMutex);
    }
}

void BLESerialService::feedRx(const uint8_t *data, size_t len) {
    if (!rxMutex) return;
    if (xSemaphoreTake(rxMutex, portMAX_DELAY) == pdTRUE) {
        rxBuffer.append(reinterpret_cast<const char *>(data), len);
        xSemaphoreGive(rxMutex);
    }
}

int BLESerialService::available() {
    // The serial-commands task polls this continuously, so it doubles as the tick
    // that pushes out a buffered tail nobody called flush() for. Reading
    // txPendingSince unlocked is benign: worst case the flush happens one poll
    // early or late.
    uint32_t pendingSince = txPendingSince;
    if (pendingSince != 0 && (millis() - pendingSince) >= TX_FLUSH_INTERVAL_MS) flush();

    if (!rxMutex) return 0;
    int result = 0;
    if (xSemaphoreTake(rxMutex, portMAX_DELAY) == pdTRUE) {
        // Only report data once a full line is buffered, so the command handler
        // fires on complete commands and partial BLE writes accumulate.
        size_t nl = rxBuffer.find('\n');
        if (nl != std::string::npos) result = static_cast<int>(nl + 1);
        xSemaphoreGive(rxMutex);
    }
    return result;
}

int BLESerialService::read() {
    if (!rxMutex) return -1;
    int result = -1;
    if (xSemaphoreTake(rxMutex, portMAX_DELAY) == pdTRUE) {
        if (!rxBuffer.empty()) {
            result = static_cast<unsigned char>(rxBuffer.front());
            rxBuffer.erase(0, 1);
        }
        xSemaphoreGive(rxMutex);
    }
    return result;
}

String BLESerialService::readStringUntil(char terminator) {
    if (!rxMutex) return String("");
    String result = "";
    if (xSemaphoreTake(rxMutex, portMAX_DELAY) == pdTRUE) {
        size_t pos = rxBuffer.find(terminator);
        if (pos != std::string::npos) {
            result = String(rxBuffer.substr(0, pos).c_str());
            rxBuffer.erase(0, pos + 1); // consume the line including the terminator
        } else {
            result = String(rxBuffer.c_str());
            rxBuffer.clear();
        }
        xSemaphoreGive(rxMutex);
    }
    return result;
}

// Usable ATT payload = MTU - 3 (opcode + handle). Fall back to the safe 20B.
size_t BLESerialService::txChunkSize() const {
    return (mtu > 3) ? static_cast<size_t>(mtu - 3) : 20;
}

// Caller must hold txMutex. Emits full chunks; the trailing partial one only when
// sendPartial is set, so a burst of small prints travels as few large packets.
void BLESerialService::drainTx(bool sendPartial) {
    const size_t chunk = txChunkSize();
    while (!txBuffer.empty()) {
        size_t n = txBuffer.size();
        if (n < chunk) {
            if (!sendPartial) break;
        } else {
            n = chunk;
        }
        if (!bleNotifyRetry(
                tx_char, reinterpret_cast<const uint8_t *>(txBuffer.data()), n, TX_NOTIFY_RETRIES
            )) {
            // The link is stalled or gone. Dropping is what the previous code did
            // anyway, and it keeps the calling task from blocking indefinitely.
            txBuffer.clear();
            break;
        }
        txBuffer.erase(0, n);
    }
    if (txBuffer.empty()) txPendingSince = 0;
}

void BLESerialService::queueTx(const uint8_t *data, size_t len) {
    if (len == 0) return;
    // No subscriber means every notify() would fail and burn the whole retry
    // budget, which used to slow down all serial output while BLE was enabled.
    if (tx_char == nullptr || !txSubscribed || !txMutex) return;
    if (xSemaphoreTake(txMutex, portMAX_DELAY) != pdTRUE) return;
    if (txBuffer.empty()) txPendingSince = millis();
    txBuffer.append(reinterpret_cast<const char *>(data), len);
    drainTx(false);
    xSemaphoreGive(txMutex);
}

void BLESerialService::flush() {
    if (!txMutex) return;
    if (xSemaphoreTake(txMutex, portMAX_DELAY) == pdTRUE) {
        drainTx(true);
        xSemaphoreGive(txMutex);
    }
}

size_t BLESerialService::print(const String &s) {
    queueTx(reinterpret_cast<const uint8_t *>(s.c_str()), s.length());
    return s.length();
}

size_t BLESerialService::println(const String &s) {
    String toSend = s + "\r\n";
    queueTx(reinterpret_cast<const uint8_t *>(toSend.c_str()), toSend.length());
    return toSend.length();
}

size_t BLESerialService::println(size_t n) { return println(String(n)); }

size_t BLESerialService::println(const uint32_t n) { return println(String(n)); }

size_t BLESerialService::print(const int n, int format) { return print(String(n, format)); }

size_t BLESerialService::println(const int n, int format) { return println(String(n, format)); }

size_t BLESerialService::println() { return println(String("")); }

void BLESerialService::vprintf(const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);
    if (size <= 0) return;

    std::vector<char> buf(size + 1);
    vsnprintf(buf.data(), buf.size(), fmt, args);
    queueTx(reinterpret_cast<const uint8_t *>(buf.data()), static_cast<size_t>(size));
}

size_t BLESerialService::write(uint8_t *str, size_t size) {
    queueTx(str, size);
    return size;
}

void BLESerialService::setMTU(uint16_t mtu) { this->mtu = mtu; }

#endif
