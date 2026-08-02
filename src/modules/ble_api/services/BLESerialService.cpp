#if !defined(LITE_VERSION)
#include "BLESerialService.h"
#include "modules/ble/ble_common.h" // bleNotifyRetry
#include <NimBLEDevice.h>
#include <vector>

BLESerialService::BLESerialService() : BruceBLEService() { rxMutex = xSemaphoreCreateMutex(); }

BLESerialService::~BLESerialService() {
    if (rxMutex) vSemaphoreDelete(rxMutex);
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
};

void BLESerialService::setup(NimBLEServer *pServer) {
    pService = pServer->createService(NUS_SERVICE_UUID);

    // App -> Bruce. WRITE and WRITE_NR so the app can use fast writeWithoutResponse.
    rx_char = pService->createCharacteristic(
        NUS_RX_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    callbacks = new BLESerialCallbacks(this);
    rx_char->setCallbacks(callbacks);

    // Bruce -> app.
    tx_char = pService->createCharacteristic(NUS_TX_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);

    pService->start();
    pServer->getAdvertising()->addServiceUUID(pService->getUUID());
}

void BLESerialService::end() {
    delete callbacks;
    callbacks = nullptr;
    if (rxMutex && xSemaphoreTake(rxMutex, portMAX_DELAY) == pdTRUE) {
        rxBuffer.clear();
        xSemaphoreGive(rxMutex);
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

void BLESerialService::notifyChunked(const uint8_t *data, size_t len) {
    if (tx_char == nullptr || len == 0) return;
    // Usable ATT payload = MTU - 3 (opcode + handle). Fall back to the safe 20B.
    size_t chunk = (mtu > 3) ? static_cast<size_t>(mtu - 3) : 20;
    size_t offset = 0;
    while (offset < len) {
        size_t n = (len - offset < chunk) ? (len - offset) : chunk;
        bleNotifyRetry(tx_char, data + offset, n);
        offset += n;
        vTaskDelay(pdMS_TO_TICKS(5)); // let the stack drain between chunks
    }
}

size_t BLESerialService::print(const String &s) {
    notifyChunked(reinterpret_cast<const uint8_t *>(s.c_str()), s.length());
    return s.length();
}

size_t BLESerialService::println(const String &s) {
    String toSend = s + "\r\n";
    notifyChunked(reinterpret_cast<const uint8_t *>(toSend.c_str()), toSend.length());
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
    notifyChunked(reinterpret_cast<const uint8_t *>(buf.data()), static_cast<size_t>(size));
}

size_t BLESerialService::write(uint8_t *str, size_t size) {
    notifyChunked(str, size);
    return size;
}

void BLESerialService::setMTU(uint16_t mtu) { this->mtu = mtu; }

#endif
