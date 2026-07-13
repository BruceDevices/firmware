#include "bus_HAL.h"
#include "core/configPins.h"
#include "globals.h"
#include "soc/soc_caps.h"

#if __has_include(<M5Unified.h>)
#include <M5Unified.h>
#define BRUCE_HAS_M5UNIFIED 1
#endif

// Some chips (e.g. ESP32-C6/-C5: SOC_HP_I2C_NUM == 1) only have one general-purpose I2C
// controller. Wire1 may still exist as a symbol there (backed by the separate, restricted
// LP_I2C peripheral used for ULP/deep-sleep access), but it isn't a second general-purpose bus
// and I2C_NUM_1 isn't even a valid i2c_port_t value, so boards/*/interface.cpp can't reference
// it. Everything in this file must fall back to sharing the single Wire controller instead.
#if defined(SOC_HP_I2C_NUM) && SOC_HP_I2C_NUM >= 2
#define BRUCE_HAS_DUAL_I2C 1
#endif

// Matches the default documented in boards/*/interface.cpp: system peripherals sit on Wire1
// whenever sys_i2c pins differ from i2c_bus, and share Wire otherwise. Boards where that isn't
// true must call setSysI2CBus() during setup.
#ifdef BRUCE_HAS_DUAL_I2C
static TwoWire *sysWire = &Wire1;
#else
static TwoWire *sysWire = &Wire;
#endif

#ifdef BRUCE_HAS_M5UNIFIED
// On M5Stack boards the sys_i2c port's ESP-IDF i2c_master_bus_handle_t is created and owned by
// M5Unified/LovyanGFX (m5gfx::i2c), not by Arduino Wire/Wire1 — M5.begin() never calls Wire.begin()
// on that port. A second TwoWire trying to install its own bus handle on an already-claimed port
// fails, leaving every later transaction returning ESP_ERR_INVALID_STATE (this is what crashed
// ST25R3916 I2C init: silent bus install failure, then a busy-retry loop that smashed the stack).
// This adapter presents the standard TwoWire virtual interface (HardwareI2C/Stream/Print are all
// virtual, so any code holding a TwoWire* works unmodified) while forwarding every call onto
// M5.In_I2C/M5.Ex_I2C, i.e. the bus handle that actually owns the port.
class M5SysWireAdapter : public TwoWire {
  public:
    M5SysWireAdapter(m5::I2C_Class &i2c) : TwoWire(255), _i2c(i2c) {}

    bool end() override { return true; } // Shared system bus: never actually torn down.
    bool setClock(uint32_t freq) override {
        _freq = freq;
        return true;
    }

    void beginTransmission(uint8_t address) override {
        _addr = address;
        _open = _i2c.start(address, false, _freq);
    }

    uint8_t endTransmission(bool stopBit) override {
        if (stopBit) {
            _i2c.stop();
            _open = false;
        }
        return 0;
    }
    uint8_t endTransmission() override { return endTransmission(true); }

    size_t requestFrom(uint8_t address, size_t len, bool stopBit) override {
        if (len > sizeof(_rxbuf)) len = sizeof(_rxbuf);
        bool ok = _open ? _i2c.restart(address, true, _freq) : _i2c.start(address, true, _freq);
        _open = false;
        _rxLen = _rxPos = 0;
        if (ok && _i2c.read(_rxbuf, len, true)) _rxLen = len;
        if (stopBit) _i2c.stop();
        return _rxLen;
    }
    size_t requestFrom(uint8_t address, size_t len) override { return requestFrom(address, len, true); }

    size_t write(uint8_t data) override { return _i2c.write(data) ? 1 : 0; }
    size_t write(const uint8_t *buf, size_t len) override { return _i2c.write(buf, len) ? len : 0; }

    int available() override { return (int)(_rxLen - _rxPos); }
    int read() override { return _rxPos < _rxLen ? _rxbuf[_rxPos++] : -1; }
    int peek() override { return _rxPos < _rxLen ? _rxbuf[_rxPos] : -1; }

    void onReceive(const std::function<void(int)> &) override {}
    void onRequest(const std::function<void()> &) override {}

  private:
    m5::I2C_Class &_i2c;
    uint32_t _freq = 400000;
    uint8_t _addr = 0;
    bool _open = false;
    uint8_t _rxbuf[64];
    size_t _rxLen = 0;
    size_t _rxPos = 0;
};

// Every boards/*/interface.cpp maps sysWire (Wire/Wire1) to whichever port M5.In_I2C sits on
// (see setSysI2CBus(M5.In_I2C.getPort() == I2C_NUM_1 ? &Wire1 : &Wire)), so M5.In_I2C is always
// the bus handle that actually owns the sys_i2c port.
static TwoWire *sysWireAdapter() {
    static M5SysWireAdapter *adapter = nullptr;
    if (adapter == nullptr) adapter = new M5SysWireAdapter(M5.In_I2C);
    return adapter;
}
#endif

static TwoWire *userWire = nullptr;
static bool userBusShared = false;
static int8_t activeSda = -1;
static int8_t activeScl = -1;

void setSysI2CBus(TwoWire *wire) { sysWire = wire; }

TwoWire *getSysI2CBus() {
#ifdef BRUCE_HAS_M5UNIFIED
    return sysWireAdapter();
#else
    return sysWire;
#endif
}

TwoWire *acquireI2CBus(int8_t sda, int8_t scl) {
    bool sharesSysBus =
        (gpio_num_t)sda == bruceConfigPins.sys_i2c.sda && (gpio_num_t)scl == bruceConfigPins.sys_i2c.scl;

    if (sharesSysBus) {
        userWire = nullptr;
        userBusShared = true;
#ifdef BRUCE_HAS_M5UNIFIED
        return sysWireAdapter();
#else
        return sysWire;
#endif
    }

#ifdef BRUCE_HAS_DUAL_I2C
    TwoWire *wire = (sysWire == &Wire) ? &Wire1 : &Wire;
#else
    // Only one general-purpose I2C controller exists: i2c_bus has no choice but to time-share
    // the same Wire used by sys_i2c, reconfiguring its pins on demand (like AUX_SPI below).
    TwoWire *wire = &Wire;
#endif
    if (userWire != wire || activeSda != sda || activeScl != scl) { wire->begin(sda, scl); }
    userWire = wire;
    userBusShared = false;
    activeSda = sda;
    activeScl = scl;
    return wire;
}

TwoWire *acquireI2CBus() {
    return acquireI2CBus((int8_t)bruceConfigPins.i2c_bus.sda, (int8_t)bruceConfigPins.i2c_bus.scl);
}

void releaseI2CBus() {
    if (userBusShared) return;
    if (userWire == nullptr) return;
    userWire->end();
    userWire = nullptr;
    activeSda = -1;
    activeScl = -1;
}

// ---------------- SPI bus arbitration ----------------

// Pins currently configured on the shared auxiliary bus (AUX_SPI), so repeated acquisitions
// with the same pins skip a redundant end()/begin() cycle that could disturb whoever else is
// mid-transaction on it.
static gpio_num_t sharedSpiSck = GPIO_NUM_NC;
static gpio_num_t sharedSpiMiso = GPIO_NUM_NC;
static gpio_num_t sharedSpiMosi = GPIO_NUM_NC;

static SPIClass *acquireSharedSPI(gpio_num_t sck, gpio_num_t miso, gpio_num_t mosi) {
    if (sharedSpiSck != sck || sharedSpiMiso != miso || sharedSpiMosi != mosi) {
        if (sharedSpiSck != GPIO_NUM_NC) AUX_SPI.end();
        AUX_SPI.begin((int8_t)sck, (int8_t)miso, (int8_t)mosi);
        sharedSpiSck = sck;
        sharedSpiMiso = miso;
        sharedSpiMosi = mosi;
    }
    return &AUX_SPI;
}

SPIClass *acquireSPIBus(gpio_num_t sck, gpio_num_t miso, gpio_num_t mosi) {
    if (sck == GPIO_NUM_NC || miso == GPIO_NUM_NC || mosi == GPIO_NUM_NC) return nullptr;

#if TFT_MOSI > 0
    // Same physical wiring as the display: it already owns a hardware controller, so reuse it
    // instead of trying to claim a second one on the same pins.
    if (mosi == (gpio_num_t)TFT_MOSI) return &tft.getSPIinstance();
#endif

    // Same physical wiring as the SD card: it is mounted for the whole program lifetime, so its
    // bus is already up and must not be reconfigured.
    if (bruceConfigPins.SDCARD_bus.mosi != GPIO_NUM_NC && mosi == bruceConfigPins.SDCARD_bus.mosi) {
        return &sdcardSPI;
    }

    // Neither the display nor the SD card own these pins: fall back to the one remaining hardware
    // SPI controller, shared across every other peripheral one owner at a time.
    return acquireSharedSPI(sck, miso, mosi);
}
