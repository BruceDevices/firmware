#ifndef BAD_USB_LIB_H
#define BAD_USB_LIB_H

#include "keys.h"
#include <Arduino.h>
class HIDInterface : public Print {
public:
    virtual void begin(Stream &stream, const uint8_t *layout) {};
    virtual void begin(const uint8_t *layout) {};
    virtual void end(void) {};
    virtual int getReportData(uint8_t *buffer, size_t size) { return 0; };
    virtual size_t write(uint8_t k) { return 0; };
    virtual size_t write(const uint8_t *buffer, size_t size) { return 0; };
    virtual size_t press(uint8_t k) { return 0; };
    virtual size_t pressRaw(uint8_t k) { return 0; };
    virtual size_t press(const MediaKeyReport k) { return 0; };
    virtual size_t release(uint8_t k) { return 0; };
    virtual size_t releaseRaw(uint8_t k) { return 0; };
    virtual void releaseAll(void) {};
    virtual bool isConnected() { return false; };
    virtual void setLayout(const uint8_t *layout) {};
    virtual void setDelay(uint32_t ms) {};
};
#endif
