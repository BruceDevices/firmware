#ifndef TEA5767_H
#define TEA5767_H

#include <Arduino.h>
#include <Wire.h>

class TEA5767 {
public:
    TEA5767();
    bool init();
    void setFrequency(float frequency);
    float getFrequency();
    void setVolume(uint8_t volume);
    uint8_t getSignalLevel();
    void powerOn();
    void powerOff();

private:
    void writeRegister(uint8_t reg, uint8_t val);
    void readRegisters();
    uint8_t registers[5];
    float current_freq;
    uint8_t current_volume;
};

#endif
