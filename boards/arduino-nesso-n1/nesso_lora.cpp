#include "core/bus_HAL.h"
#include <Arduino.h>
#include <Wire.h>

namespace {
constexpr uint8_t kExpanderAddress = 0x43;
constexpr uint8_t kExpanderGlobalControl = 0x01;
constexpr uint8_t kExpanderOutputEnable = 0x03;
constexpr uint8_t kExpanderOutputState = 0x05;
constexpr uint8_t kExpanderHighImpedance = 0x07;
constexpr uint8_t kExpanderDefaultOutput = 0x09;
constexpr uint8_t kExpanderInterruptMask = 0x11;
constexpr uint8_t kExpanderGlobalControlEnable = 0x01;
constexpr uint8_t kLoraLnaEnableBit = 5;
constexpr uint8_t kLoraAntennaSwitchBit = 6;
constexpr uint8_t kLoraEnableBit = 7;

bool readExpanderRegister(TwoWire *wire, uint8_t reg, uint8_t &value) {
    if (wire == nullptr) return false;
    wire->beginTransmission(kExpanderAddress);
    wire->write(reg);
    if (wire->endTransmission(false) != 0) return false;
    if (wire->requestFrom(kExpanderAddress, (uint8_t)1) != 1) return false;
    value = wire->read();
    return true;
}

bool writeExpanderRegister(TwoWire *wire, uint8_t reg, uint8_t value) {
    if (wire == nullptr) return false;
    wire->beginTransmission(kExpanderAddress);
    wire->write(reg);
    wire->write(value);
    return wire->endTransmission() == 0;
}

bool writeExpanderBit(TwoWire *wire, uint8_t reg, uint8_t bit, bool enabled) {
    uint8_t value = 0;
    if (!readExpanderRegister(wire, reg, value)) return false;
    uint8_t mask = 1u << bit;
    value = enabled ? (value | mask) : (value & ~mask);
    return writeExpanderRegister(wire, reg, value);
}

bool configureExpanderOutput(TwoWire *wire, uint8_t bit) {
    return writeExpanderBit(wire, kExpanderOutputEnable, bit, true) &&
           writeExpanderBit(wire, kExpanderHighImpedance, bit, false);
}

bool prepareNessoLoRaFrontend() {
    TwoWire *wire = getSysI2CBus();
    if (wire == nullptr) return false;

    uint8_t discarded = 0;
    if (!readExpanderRegister(wire, kExpanderGlobalControl, discarded)) return false;
    if (!writeExpanderRegister(wire, kExpanderGlobalControl, kExpanderGlobalControlEnable)) return false;
    if (!writeExpanderRegister(wire, kExpanderDefaultOutput, 0xFF)) return false;
    if (!writeExpanderRegister(wire, kExpanderInterruptMask, 0xFF)) return false;
    if (!configureExpanderOutput(wire, kLoraEnableBit)) return false;
    if (!configureExpanderOutput(wire, kLoraAntennaSwitchBit)) return false;
    if (!configureExpanderOutput(wire, kLoraLnaEnableBit)) return false;

    if (!writeExpanderBit(wire, kExpanderOutputState, kLoraEnableBit, false)) return false;
    delay(10);
    if (!writeExpanderBit(wire, kExpanderOutputState, kLoraEnableBit, true)) return false;
    delay(10);
    if (!writeExpanderBit(wire, kExpanderOutputState, kLoraAntennaSwitchBit, true)) return false;
    if (!writeExpanderBit(wire, kExpanderOutputState, kLoraLnaEnableBit, true)) return false;
    delay(10);
    return true;
}
} // namespace

bool prepareBoardLoRaRadio() { return prepareNessoLoRaFrontend(); }
