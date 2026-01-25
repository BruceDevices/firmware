#include "Tea5767.h"

TEA5767::TEA5767() : current_freq(87.5), current_volume(0) {}

bool TEA5767::init() {
    Wire.begin();
    Wire.beginTransmission(0x60);
    return (Wire.endTransmission() == 0);
}

void TEA5767::setFrequency(float frequency) {
    frequency = constrain(frequency, 87.5, 108.0);
    current_freq = frequency;

    uint16_t pll = (uint16_t)(4 * (frequency * 1000000 + 225000) / 32768);
    registers[0] = (pll >> 8) & 0x3F;
    registers[1] = pll & 0xFF;
    registers[2] = 0xB0; // Default settings
    registers[3] = 0x10; // Default settings
    registers[4] = 0x00; // Default settings

    // Set volume
    registers[3] = (registers[3] & 0xF0) | (current_volume & 0x0F);

    Wire.beginTransmission(0x60);
    for (int i = 0; i < 5; i++) { Wire.write(registers[i]); }
    Wire.endTransmission();
    delay(100);
}

float TEA5767::getFrequency() { return current_freq; }

void TEA5767::setVolume(uint8_t volume) {
    current_volume = constrain(volume, 0, 15);
    registers[3] = (registers[3] & 0xF0) | (current_volume & 0x0F);

    Wire.beginTransmission(0x60);
    for (int i = 0; i < 5; i++) { Wire.write(registers[i]); }
    Wire.endTransmission();
}

uint8_t TEA5767::getSignalLevel() {
    Wire.requestFrom(0x60, 5);
    if (Wire.available() == 5) {
        for (int i = 0; i < 5; i++) { registers[i] = Wire.read(); }
        return (registers[3] >> 4) & 0x03;
    }
    return 0;
}

void TEA5767::powerOn() {
    registers[2] &= ~(1 << 7);
    Wire.beginTransmission(0x60);
    for (int i = 0; i < 5; i++) { Wire.write(registers[i]); }
    Wire.endTransmission();
}

void TEA5767::powerOff() {
    registers[2] |= (1 << 7);
    Wire.beginTransmission(0x60);
    for (int i = 0; i < 5; i++) { Wire.write(registers[i]); }
    Wire.endTransmission();
}
