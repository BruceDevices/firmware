#pragma once
#if !defined(LITE_VERSION)
#include "BruceBLEService.hpp"

#include <SerialDevice.h>

#define BUFFER_SIZE 128

class BLESerialCallbacks;

class BLESerialService : public BruceBLEService, public SerialDevice {
    NimBLECharacteristic *serial_char = nullptr;
    BLESerialCallbacks *callbacks = nullptr;

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
    void flush() override {}
    String readStringUntil(char terminator) override;
    int available() override;
    void setMTU(uint16_t mtu);
};
#endif
