#include "core/bus_HAL.h"
#include <Arduino.h>
#include <M5Unified.h>

namespace {
constexpr uint8_t kNessoExpanderAddress = 0x44;
constexpr uint8_t kGrovePowerBit = 2;
constexpr uint32_t kDefaultI2CFrequency = 100000;

class NessoGroveM5Wire : public TwoWire {
public:
    NessoGroveM5Wire() : TwoWire(255) {}

    bool begin(int sda, int scl, uint32_t frequency = kDefaultI2CFrequency) {
        _freq = frequency;
        return M5.Ex_I2C.begin(I2C_NUM_0, sda, scl);
    }

    bool begin(int sda, int scl) { return begin(sda, scl, _freq); }
    bool end() override { return M5.Ex_I2C.release(); }
    bool setClock(uint32_t freq) override {
        _freq = freq;
        return true;
    }

    void beginTransmission(uint8_t address) override {
        _addr = address;
        _error = 0;
        _open = M5.Ex_I2C.start(address, false, _freq);
        if (!_open) _error = 4;
    }

    uint8_t endTransmission(bool sendStop) override {
        if (sendStop) {
            if (_open && !M5.Ex_I2C.stop()) _error = 4;
            _open = false;
        }
        return _error;
    }

    uint8_t endTransmission() override { return endTransmission(true); }

    size_t requestFrom(uint8_t address, size_t len, bool sendStop) override {
        if (len > sizeof(_rxBuf)) len = sizeof(_rxBuf);
        bool ok = _open ? M5.Ex_I2C.restart(address, true, _freq) : M5.Ex_I2C.start(address, true, _freq);
        _open = false;
        _rxLen = _rxPos = 0;
        if (ok && M5.Ex_I2C.read(_rxBuf, len, true)) _rxLen = len;
        if (sendStop) M5.Ex_I2C.stop();
        return _rxLen;
    }

    size_t requestFrom(uint8_t address, size_t len) override { return requestFrom(address, len, true); }

    size_t write(uint8_t data) override {
        if (!_open || !M5.Ex_I2C.write(data)) {
            _error = 4;
            return 0;
        }
        return 1;
    }

    size_t write(const uint8_t *data, size_t quantity) override {
        if (!_open || !M5.Ex_I2C.write(data, quantity)) {
            _error = 4;
            return 0;
        }
        return quantity;
    }

    int available() override { return (int)(_rxLen - _rxPos); }
    int read() override { return _rxPos < _rxLen ? _rxBuf[_rxPos++] : -1; }
    int peek() override { return _rxPos < _rxLen ? _rxBuf[_rxPos] : -1; }

    void onReceive(const std::function<void(int)> &) override {}
    void onRequest(const std::function<void()> &) override {}

private:
    uint32_t _freq = kDefaultI2CFrequency;
    uint8_t _addr = 0;
    bool _open = false;
    uint8_t _error = 0;
    uint8_t _rxBuf[64];
    size_t _rxLen = 0;
    size_t _rxPos = 0;
};


bool nessoExpanderSetBit(uint8_t reg, uint8_t bit, bool enabled) {
    uint8_t value = 0;
    if (!M5.In_I2C.readRegister(kNessoExpanderAddress, reg, &value, 1, kDefaultI2CFrequency)) return false;
    uint8_t updated = enabled ? (value | (1 << bit)) : (value & ~(1 << bit));
    if (updated == value) return true;
    return M5.In_I2C.writeRegister8(kNessoExpanderAddress, reg, updated, kDefaultI2CFrequency);
}

bool nessoEnableGrovePower() {
    static bool enabled = false;
    if (enabled) return true;

    bool ok = nessoExpanderSetBit(0x03, kGrovePowerBit, true);
    ok = nessoExpanderSetBit(0x07, kGrovePowerBit, false) && ok;
    ok = nessoExpanderSetBit(0x05, kGrovePowerBit, true) && ok;
    if (ok) {
        enabled = true;
        delay(20);
    }
    return ok;
}

NessoGroveM5Wire &nessoGroveWire() {
    static NessoGroveM5Wire wire;
    return wire;
}
} // namespace

bool enableNessoGrovePower() { return nessoEnableGrovePower(); }

TwoWire *acquireBoardI2CBus(int8_t sda, int8_t scl) {
    if (sda != GROVE_SDA || scl != GROVE_SCL) return nullptr;
    enableNessoGrovePower();

    lockSysI2CBus();
    M5.In_I2C.release();

    NessoGroveM5Wire &wire = nessoGroveWire();
    if (!wire.begin(sda, scl, kDefaultI2CFrequency)) {
        M5.In_I2C.begin(I2C_NUM_0, SYS_I2C_SDA, SYS_I2C_SCL);
        unlockSysI2CBus();
        return nullptr;
    }
    return &wire;
}

bool releaseBoardI2CBus(TwoWire *wire) {
    if (wire != &nessoGroveWire()) return false;

    nessoGroveWire().end();
    M5.In_I2C.begin(I2C_NUM_0, SYS_I2C_SDA, SYS_I2C_SCL);
    unlockSysI2CBus();
    return true;
}
