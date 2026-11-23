#ifndef BRUCE_USBSERIAL_H
#define BRUCE_USBSERIAL_H

#include "SerialDevice.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class USBSerial : public SerialDevice {
public:
    USBSerial(Stream *in = &Serial) : out(in) {
        serialMutex = xSemaphoreCreateRecursiveMutex();
    }
    ~USBSerial() override {
        if (serialMutex) {
            vSemaphoreDelete(serialMutex);
            serialMutex = nullptr;
        }
    }

    void lock() override {
        if (serialMutex) xSemaphoreTakeRecursive(serialMutex, portMAX_DELAY);
    }
    void unlock() override {
        if (serialMutex) xSemaphoreGiveRecursive(serialMutex);
    }

    size_t println(const String &s) override {
        lock();
        size_t res = out->println(s);
        unlock();
        return res;
    }
    size_t print(const String &s) override {
        lock();
        size_t res = out->print(s);
        unlock();
        return res;
    }
    size_t print(const int n, int format) override {
        lock();
        size_t res = out->print(n, format);
        unlock();
        return res;
    }
    void vprintf(const char *fmt, va_list args) override {
        lock();
        char buf[256];
        va_list args_copy;
        va_copy(args_copy, args);
        int len = vsnprintf(buf, sizeof(buf), fmt, args_copy);
        va_end(args_copy);
        if (len >= 0 && len < (int)sizeof(buf)) {
            out->write((const uint8_t *)buf, len);
        } else if (len >= (int)sizeof(buf)) {
            char *dyn = (char *)malloc(len + 1);
            if (dyn) {
                vsnprintf(dyn, len + 1, fmt, args);
                out->write((const uint8_t *)dyn, len);
                free(dyn);
            } else {
                out->write((const uint8_t *)buf, sizeof(buf) - 1);
            }
        }
        unlock();
    }
    size_t println() override {
        lock();
        size_t res = out->println();
        unlock();
        return res;
    }
    size_t println(size_t n) override {
        lock();
        size_t res = out->println(n);
        unlock();
        return res;
    }
    size_t println(const uint32_t n) override {
        lock();
        size_t res = out->println(n);
        unlock();
        return res;
    }
    size_t println(const int n, int format) override {
        lock();
        size_t res = out->println(n, format);
        unlock();
        return res;
    }
    String readStringUntil(char terminator) override {
        lock();
        String res = out->readStringUntil(terminator);
        unlock();
        return res;
    }
    void flush() override {
        lock();
        out->flush();
        unlock();
    }
    int available() override {
        lock();
        int res = out->available();
        unlock();
        return res;
    }
    size_t write(uint8_t *str, size_t size) override {
        lock();
        size_t res = out->write(str, size);
        unlock();
        return res;
    }
    int read() override {
        lock();
        int res = out->read();
        unlock();
        return res;
    }
    void setSerialOutput(Stream *in) {
        lock();
        out = in;
        unlock();
    }
    Stream *getSerialOutput() { return out; }

private:
    Stream *out;
    SemaphoreHandle_t serialMutex = nullptr;
};

#endif // BRUCE_USBSERIAL_H
