#if !defined(LITE_VERSION)
#include "BLESerialService.h"
#include <NimBLEDevice.h>

BLESerialService::BLESerialService() : BruceBLEService() {}

BLESerialService::~BLESerialService() {}

static bool newValue = false;

class BLESerialCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
        newValue = true;
    }
};

void BLESerialService::setup(NimBLEServer *pServer) {
    pService = pServer->createService("4371ec0b-3d43-49f9-b731-7c72a4a7bb91");

    serial_char = pService->createCharacteristic(
        "d555ed97-bf2a-4f46-b3eb-d1fcdd7325e9", // Battery Level
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE
    );

    callbacks = new BLESerialCallbacks();
    serial_char->setCallbacks(callbacks);

    pService->start();
    pServer->getAdvertising()->addServiceUUID(pService->getUUID());
}

void BLESerialService::end() { delete callbacks; }

int BLESerialService::available() {
    if (!newValue) return 0;
    newValue = false;

    return serial_char->getValue().size();
}

size_t BLESerialService::println(const String &s) {
    String toSend = s + "\r\n";
    serial_char->notify(toSend);
    vTaskDelay(pdMS_TO_TICKS(10)); // Add some delay to ensure data is read by the client
    return toSend.length();
}

size_t BLESerialService::print(const String &s) {
    serial_char->notify(s);
    vTaskDelay(pdMS_TO_TICKS(10));
    return s.length();
}

size_t BLESerialService::println(size_t n) {
    String s = String(n);
    return println(s);
}

void BLESerialService::vprintf(const char *fmt, va_list args) {
    int size = vsnprintf(NULL, 0, fmt, args) + 1;
    char str[BUFFER_SIZE];
    sprintf(str, fmt, args);

    serial_char->notify(reinterpret_cast<const uint8_t *>(str), size);
    vTaskDelay(pdMS_TO_TICKS(10));
}

String BLESerialService::readStringUntil(char terminator) {
    Serial.println("readStringUntil");
    String result = "";
    std::string value = serial_char->getValue();
    for (char c : value) {
        result += c;
        if (c == terminator) break;
    }
    return result;
}

size_t BLESerialService::println(const uint32_t n) {
    String s = String(n);
    return println(s);
}

size_t BLESerialService::print(const int n, int format) {
    String s = String(n, format);
    return print(s);
}

size_t BLESerialService::println(const int n, int format) {
    String s = String(n, format);
    return println(s);
}

size_t BLESerialService::println() { return println(""); }

size_t BLESerialService::write(uint8_t *str, size_t size) {
    serial_char->notify(str, size);
    vTaskDelay(pdMS_TO_TICKS(10));
    return size;
}

void BLESerialService::setMTU(uint16_t mtu) { this->mtu = mtu; }

#endif
