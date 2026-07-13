#include "PCA9555.h"

#define PCA9555_INPUT_PORT0 0x00
#define PCA9555_OUTPUT_PORT0 0x02
#define PCA9555_CONFIG_PORT0 0x06

PCA9555::PCA9555() {}
PCA9555::~PCA9555() {}

bool PCA9555::begin(uint8_t address, TwoWire *wire) {
    _i2caddr = address;
    _wire = wire;

    uint8_t dummy = 0;
    if (!readRegister(PCA9555_INPUT_PORT0, dummy)) return false;

    if (!readRegister(PCA9555_OUTPUT_PORT0, output_state[0])) output_state[0] = 0;
    if (!readRegister(PCA9555_OUTPUT_PORT0 + 1, output_state[1])) output_state[1] = 0;
    if (!readRegister(PCA9555_CONFIG_PORT0, direction_state[0])) direction_state[0] = 0xFF;
    if (!readRegister(PCA9555_CONFIG_PORT0 + 1, direction_state[1])) direction_state[1] = 0xFF;

    return true;
}

bool PCA9555::outputGPIO(uint16_t pins) {
    output_state[0] = pins & 0xFF;
    output_state[1] = (pins >> 8) & 0xFF;
    return writeRegister(PCA9555_OUTPUT_PORT0, output_state[0]) &&
           writeRegister(PCA9555_OUTPUT_PORT0 + 1, output_state[1]);
}

uint16_t PCA9555::inputGPIO() {
    uint8_t l = 0, h = 0;
    readRegister(PCA9555_INPUT_PORT0, l);
    readRegister(PCA9555_INPUT_PORT0 + 1, h);
    return (h << 8) | l;
}

bool PCA9555::configureDirection(uint16_t pins) {
    direction_state[0] = pins & 0xFF;
    direction_state[1] = (pins >> 8) & 0xFF;
    return writeRegister(PCA9555_CONFIG_PORT0, direction_state[0]) &&
           writeRegister(PCA9555_CONFIG_PORT0 + 1, direction_state[1]);
}

void PCA9555::pinMode(uint8_t pin, uint8_t mode) {
    if (pin > 15) return;
    uint8_t port = pin / 8;
    uint8_t bit = pin % 8;

    if (mode == OUTPUT) {
        direction_state[port] &= ~(1 << bit);
    } else {
        direction_state[port] |= (1 << bit);
    }

    writeRegister(PCA9555_CONFIG_PORT0 + port, direction_state[port]);
}

void PCA9555::digitalWrite(uint8_t pin, bool val) {
    if (pin > 15) return;
    uint8_t port = pin / 8;
    uint8_t bit = pin % 8;

    if (val) output_state[port] |= (1 << bit);
    else output_state[port] &= ~(1 << bit);

    writeRegister(PCA9555_OUTPUT_PORT0 + port, output_state[port]);
}

bool PCA9555::digitalRead(uint8_t pin) {
    if (pin > 15) return false;
    uint8_t port = pin / 8;
    uint8_t bit = pin % 8;

    uint8_t val = 0;
    if (!readRegister(PCA9555_INPUT_PORT0 + port, val)) return false;

    return (val >> bit) & 0x1;
}

bool PCA9555::writeRegister(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(_i2caddr);
    _wire->write(reg);
    _wire->write(value);
    return (_wire->endTransmission() == 0);
}

bool PCA9555::readRegister(uint8_t reg, uint8_t &value) {
    _wire->beginTransmission(_i2caddr);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) return false;
    if (_wire->requestFrom((int)_i2caddr, 1) != 1) return false;
    value = _wire->read();
    return true;
}
