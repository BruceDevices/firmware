#include "bus_HAL.h"
#include "core/configPins.h"
#include "globals.h"
#include "soc/soc_caps.h"
#include <esp32-hal-periman.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// # define BUSHAL_DBG
#ifdef BUSHAL_DBG
#define BUSHAL_DBG(...) Serial.printf(__VA_ARGS__)
#else
#define BUSHAL_DBG(...)
#endif

#if __has_include(<M5Unified.h>)
#include <M5Unified.h>
#define BRUCE_HAS_M5UNIFIED 1
#endif

#ifdef BRUCE_HAS_M5UNIFIED
// Guards every sys_i2c transaction issued through M5SysWireAdapter (below) against M5.update()'s
// own internal I2C polling, which bypasses the adapter entirely and would otherwise race it from
// a different FreeRTOS task - see the comment on lockSysI2CBus() in bus_HAL.h.
static SemaphoreHandle_t sysI2CMutex() {
    static SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    return mutex;
}
#endif

void lockSysI2CBus() {
#ifdef BRUCE_HAS_M5UNIFIED
    xSemaphoreTake(sysI2CMutex(), portMAX_DELAY);
#endif
}

void unlockSysI2CBus() {
#ifdef BRUCE_HAS_M5UNIFIED
    xSemaphoreGive(sysI2CMutex());
#endif
}

bool trylockSysI2CBus() {
#ifdef BRUCE_HAS_M5UNIFIED
    return xSemaphoreTake(sysI2CMutex(), 0) == pdTRUE;
#else
    return true;
#endif
}

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

    // Held from the first bus access of a transaction (beginTransmission, or a standalone
    // requestFrom) until the actual I2C stop condition is issued - see lockSysI2CBus() in
    // bus_HAL.h. Matches _open's write->restart-read chaining, not just a 1:1 call wrapper.
    void beginTransmission(uint8_t address) override {
        lockSysI2CBus();
        _addr = address;
        _error = 0;
        _open = _i2c.start(address, false, _freq);
        if (!_open) _error = 4;
    }

    uint8_t endTransmission(bool stopBit) override {
        // A failed write phase ends the transaction even without a stop request: callers abandon
        // it on error (no requestFrom follows), which would otherwise leave the sys lock held and
        // deadlock the next sys_i2c transaction.
        if (stopBit || _error != 0) {
            if (_open && !_i2c.stop() && _error == 0) _error = 4;
            _open = false;
            unlockSysI2CBus();
        }
        return _error;
    }
    uint8_t endTransmission() override { return endTransmission(true); }

    size_t requestFrom(uint8_t address, size_t len, bool stopBit) override {
        if (!_open) lockSysI2CBus(); // standalone read, not chained off beginTransmission
        if (len > sizeof(_rxbuf)) len = sizeof(_rxbuf);
        bool ok = _open ? _i2c.restart(address, true, _freq) : _i2c.start(address, true, _freq);
        _open = false;
        _rxLen = _rxPos = 0;
        if (ok && _i2c.read(_rxbuf, len, true)) _rxLen = len;
        if (stopBit) _i2c.stop();
        unlockSysI2CBus(); // requestFrom is always the terminal call of a transaction
        return _rxLen;
    }
    size_t requestFrom(uint8_t address, size_t len) override { return requestFrom(address, len, true); }

    size_t write(uint8_t data) override {
        if (!_open || !_i2c.write(data)) {
            _error = 4;
            return 0;
        }
        return 1;
    }
    size_t write(const uint8_t *buf, size_t len) override {
        if (!_open || !_i2c.write(buf, len)) {
            _error = 4;
            return 0;
        }
        return len;
    }

    int available() override { return (int)(_rxLen - _rxPos); }
    int read() override { return _rxPos < _rxLen ? _rxbuf[_rxPos++] : -1; }
    int peek() override { return _rxPos < _rxLen ? _rxbuf[_rxPos] : -1; }

    void onReceive(const std::function<void(int)> &) override {}
    void onRequest(const std::function<void()> &) override {}

    uint32_t clock() const { return _freq; }

private:
    m5::I2C_Class &_i2c;
    uint32_t _freq = 400000;
    uint8_t _addr = 0;
    bool _open = false;
    uint8_t _error = 0;
    // Must hold a whole read in one transaction: Adafruit BusIO chunks reads up to 250 bytes and
    // treats a short requestFrom() as failure; the EMV reader needs an extended PN532 frame (281).
    uint8_t _rxbuf[288];
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

TwoWire *__attribute__((weak)) acquireBoardI2CBus(int8_t sda, int8_t scl) {
    (void)sda;
    (void)scl;
    return nullptr;
}

bool __attribute__((weak)) releaseBoardI2CBus(TwoWire *wire) {
    (void)wire;
    return false;
}

void setSysI2CBus(TwoWire *wire) { sysWire = wire; }

TwoWire *getSysI2CBus() {
#ifdef BRUCE_HAS_M5UNIFIED
    return sysWireAdapter();
#else
    return sysWire;
#endif
}

// ---------------- I2C pin helpers ----------------

static bool sysConfigured() { return bruceConfigPins.sys_i2c.sda >= 0 && bruceConfigPins.sys_i2c.scl >= 0; }

static bool pinsAreSys(int8_t sda, int8_t scl) {
    return sysConfigured() && sda == (int8_t)bruceConfigPins.sys_i2c.sda &&
           scl == (int8_t)bruceConfigPins.sys_i2c.scl;
}

static bool pinsOverlapSys(int8_t sda, int8_t scl) {
    if (!sysConfigured()) return false;
    int8_t s = (int8_t)bruceConfigPins.sys_i2c.sda;
    int8_t c = (int8_t)bruceConfigPins.sys_i2c.scl;
    return sda == s || sda == c || scl == s || scl == c;
}

// Both lines must idle high before a controller is started on them: a line held low means
// something else is driving that pin (or a wedged slave), and starting I2C there would only
// produce driver errors or disturb that other function.
static bool linesIdle(int8_t sda, int8_t scl) {
    pinMode(sda, INPUT_PULLUP);
    pinMode(scl, INPUT_PULLUP);
    delayMicroseconds(50);
    return digitalRead(sda) == HIGH && digitalRead(scl) == HIGH;
}

// ---------------- User bus (reference counted) ----------------

static TwoWire *userWire = nullptr;
static int8_t userSda = -1;
static int8_t userScl = -1;
static uint16_t userRefs = 0;
static bool userIsBoard = false; // opened through acquireBoardI2CBus()

// Opens the user bus on the given pins, or joins it if it is already open on those same pins.
// Returns nullptr when it is open on other pins, when the SoC's only general-purpose controller
// already serves sys_i2c, when a line isn't idle, or when the controller fails to start.
static TwoWire *userBusOpen(int8_t sda, int8_t scl) {
    if (sda < 0 || scl < 0) return nullptr;
    if (userRefs > 0) {
        if (sda == userSda && scl == userScl) {
            userRefs++;
            return userWire;
        }
        BUSHAL_DBG("[busHAL] user bus busy on %d/%d, refused %d/%d\n", userSda, userScl, sda, scl);
        return nullptr;
    }

    TwoWire *wire = acquireBoardI2CBus(sda, scl);
    bool board = wire != nullptr;
    if (!board) {
#ifdef BRUCE_HAS_DUAL_I2C
        wire = (sysWire == &Wire) ? &Wire1 : &Wire;
#else
        // The only general-purpose controller already serves sys_i2c: begin() can't re-pin a
        // started bus (it keeps the sys pins), and ending it would take the system peripherals
        // down with it. Boards that can time-share it provide acquireBoardI2CBus().
        if (sysConfigured()) return nullptr;
        wire = &Wire;
#endif
        if (!linesIdle(sda, scl)) {
            BUSHAL_DBG("[busHAL] %d/%d not idle (line held low), not starting I2C\n", sda, scl);
            return nullptr;
        }
        if (!wire->begin(sda, scl)) {
            BUSHAL_DBG("[busHAL] begin(%d/%d) failed\n", sda, scl);
            wire->end();
            return nullptr;
        }
    }
    userWire = wire;
    userSda = sda;
    userScl = scl;
    userRefs = 1;
    userIsBoard = board;
    BUSHAL_DBG("[busHAL] user bus opened on %d/%d (%s)\n", sda, scl, board ? "board" : "hw");
    return wire;
}

static void userBusUnref() {
    if (userRefs == 0) return;
    if (--userRefs > 0) return;
    if (!(userIsBoard && releaseBoardI2CBus(userWire))) userWire->end();
    BUSHAL_DBG("[busHAL] user bus closed (%d/%d)\n", userSda, userScl);
    userWire = nullptr;
    userSda = -1;
    userScl = -1;
    userIsBoard = false;
}

// ---------------- I2C leases ----------------

struct I2CLeaseAccess {
    static I2CLease make(I2CBusId bus, TwoWire *wire, int8_t sda, int8_t scl) {
        I2CLease lease;
        lease._bus = bus;
        lease._wire = wire;
        lease._sda = sda;
        lease._scl = scl;
        return lease;
    }
    static void setFound(I2CLease &lease, bool found) { lease._found = found; }
};

I2CLease::I2CLease(I2CLease &&other) noexcept { *this = std::move(other); }

I2CLease &I2CLease::operator=(I2CLease &&other) noexcept {
    if (this == &other) return *this;
    release();
    _wire = other._wire;
    _bus = other._bus;
    _found = other._found;
    _sda = other._sda;
    _scl = other._scl;
    _clockSaved = other._clockSaved;
    _savedClock = other._savedClock;
    _timeoutSaved = other._timeoutSaved;
    _savedTimeout = other._savedTimeout;
    other._wire = nullptr;
    other._bus = I2CBusId::None;
    other._found = false;
    other._clockSaved = false;
    other._timeoutSaved = false;
    return *this;
}

static uint32_t readSysClock(TwoWire *wire) {
#ifdef BRUCE_HAS_M5UNIFIED
    // The adapter isn't a real controller: TwoWire::getClock() would query port 255.
    if (wire == sysWireAdapter()) return static_cast<M5SysWireAdapter *>(wire)->clock();
#endif
    return wire->getClock();
}

void I2CLease::setClock(uint32_t hz) {
    if (_wire == nullptr) return;
    if (_bus == I2CBusId::Sys && !_clockSaved) {
        _savedClock = readSysClock(_wire);
        _clockSaved = _savedClock != 0;
    }
    _wire->setClock(hz);
}

void I2CLease::setTimeOut(uint16_t ms) {
    if (_wire == nullptr) return;
    if (_bus == I2CBusId::Sys && !_timeoutSaved) {
        _savedTimeout = _wire->getTimeOut();
        _timeoutSaved = true;
    }
    _wire->setTimeOut(ms);
}

void I2CLease::release() {
    if (_wire == nullptr) return;
    if (_bus == I2CBusId::Sys) {
        if (_clockSaved) _wire->setClock(_savedClock);
        if (_timeoutSaved) _wire->setTimeOut(_savedTimeout);
    } else if (_bus == I2CBusId::User) {
        userBusUnref();
    }
    _wire = nullptr;
    _bus = I2CBusId::None;
    _found = false;
    _sda = -1;
    _scl = -1;
    _clockSaved = false;
    _timeoutSaved = false;
}

static I2CLease leaseSys() {
    if (!sysConfigured()) return I2CLease();
    // A board hook (e.g. Nesso N1) lent the sys controller to the user bus: sys_i2c is unusable
    // until that lease ends, and its adapter would deadlock on the lock the hook holds.
    if (userIsBoard && userRefs > 0) return I2CLease();
    return I2CLeaseAccess::make(
        I2CBusId::Sys,
        getSysI2CBus(),
        (int8_t)bruceConfigPins.sys_i2c.sda,
        (int8_t)bruceConfigPins.sys_i2c.scl
    );
}

I2CLease i2cLeasePins(int8_t sda, int8_t scl) {
    if (sda < 0 || scl < 0) return I2CLease();
    if (pinsAreSys(sda, scl)) return leaseSys();
    if (pinsOverlapSys(sda, scl)) {
        BUSHAL_DBG("[busHAL] %d/%d partially overlaps sys_i2c, refused\n", sda, scl);
        return I2CLease();
    }
    TwoWire *wire = userBusOpen(sda, scl);
    if (wire == nullptr) return I2CLease();
    return I2CLeaseAccess::make(I2CBusId::User, wire, sda, scl);
}

I2CLease i2cLeaseBus(I2CBusId id) {
    if (id == I2CBusId::Sys) return leaseSys();
    if (id == I2CBusId::User)
        return i2cLeasePins((int8_t)bruceConfigPins.i2c_bus.sda, (int8_t)bruceConfigPins.i2c_bus.scl);
    return I2CLease();
}

bool i2cProbe(const I2CLease &lease, uint8_t addr) {
    TwoWire *wire = lease.wire();
    if (wire == nullptr) return false;
    // On M5Unified boards the sys wire is M5SysWireAdapter, which takes the sys_i2c lock itself.
    wire->beginTransmission(addr);
    return wire->endTransmission() == 0;
}

I2CLease i2cLeaseDevice(const I2CDeviceQuery &q) {
    const I2CBusId order[2] = {
        q.prefer == I2CPrefer::SysFirst ? I2CBusId::Sys : I2CBusId::User,
        q.prefer == I2CPrefer::SysFirst ? I2CBusId::User : I2CBusId::Sys,
    };
    const uint8_t tries = q.retries ? q.retries : 1;
    bool sysTried = false;

    for (I2CBusId id : order) {
        I2CLease lease = i2cLeaseBus(id);
        if (!lease) continue;
        // i2c_bus configured on the sys_i2c pins resolves to the sys bus: probe it only once.
        if (lease.bus() == I2CBusId::Sys) {
            if (sysTried) continue;
            sysTried = true;
        }
        for (uint8_t t = 0; t < tries; t++) {
            if (i2cProbe(lease, q.addr) && (q.verify == nullptr || q.verify(*lease.wire(), q.addr))) {
                BUSHAL_DBG("[busHAL] 0x%02X found on %s\n", q.addr, i2cBusLabel(lease.bus()).c_str());
                I2CLeaseAccess::setFound(lease, true);
                return lease;
            }
            if (t + 1 < tries) delay(10);
        }
        // Not here: the lease goes out of scope and the bus is released before the next candidate.
    }

    BUSHAL_DBG("[busHAL] 0x%02X not found on any bus\n", q.addr);
    if (q.fallbackToUser) return i2cLeaseBus(I2CBusId::User);
    return I2CLease();
}

int i2cScanAll(void (*cb)(I2CBusId bus, uint8_t addr, void *ctx), void *ctx) {
    const I2CBusId order[2] = {I2CBusId::Sys, I2CBusId::User};
    bool anyBus = false;
    bool sysScanned = false;
    int count = 0;
    for (I2CBusId id : order) {
        I2CLease lease = i2cLeaseBus(id);
        if (!lease) continue;
        if (lease.bus() == I2CBusId::Sys) {
            if (sysScanned) continue;
            sysScanned = true;
        }
        anyBus = true;
        for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
            if (!i2cProbe(lease, addr)) continue;
            count++;
            if (cb) cb(lease.bus(), addr, ctx);
        }
    }
    return anyBus ? count : -1;
}

String i2cBusLabel(I2CBusId id) {
    if (id == I2CBusId::Sys) {
        return "SYS " + String((int)bruceConfigPins.sys_i2c.sda) + "/" + String((int)bruceConfigPins.sys_i2c.scl);
    }
    if (id == I2CBusId::User) {
        return "EXT " + String((int)bruceConfigPins.i2c_bus.sda) + "/" + String((int)bruceConfigPins.i2c_bus.scl);
    }
    return "NONE";
}

bool checkAndRecoverSysI2CBus() {
    int8_t sda = (int8_t)bruceConfigPins.sys_i2c.sda;
    int8_t scl = (int8_t)bruceConfigPins.sys_i2c.scl;
    if (sda < 0 || scl < 0) return false; // board has no sys_i2c
    // A board hook lent the sys controller to the user bus (sys pins are idle by design then).
    if (userIsBoard && userRefs > 0) return false;

    static unsigned long sdaLowSinceMs = 0;
    static unsigned long sclLowSinceMs = 0;
    static unsigned long lastCheckMs = 0;
    const unsigned long kCheckIntervalMs = 300;
    // Well beyond any legitimate transaction (including PN532's target-mode arm
    // writes and the PMIC's own reads) - only a genuinely wedged bus stays low
    // this long.
    const unsigned long kStuckThresholdMs = 1500;

    unsigned long now = millis();
    if (now - lastCheckMs < kCheckIntervalMs) return false;
    lastCheckMs = now;

#ifndef BRUCE_HAS_M5UNIFIED
    // On boards whose sys_i2c pins double as Grove (Mako, Pebble...), another function (e.g. RF
    // TX on rfTx) may have taken them over: a long low level there is that function working, not
    // a wedged bus, and "recovering" would break it. Only watch pins still muxed to I2C. (M5
    // boards drive the sys port through M5Unified, which doesn't register pins with periman.)
    if (perimanGetPinBusType(sda) != ESP32_BUS_TYPE_I2C_MASTER_SDA ||
        perimanGetPinBusType(scl) != ESP32_BUS_TYPE_I2C_MASTER_SCL) {
        sdaLowSinceMs = 0;
        sclLowSinceMs = 0;
        return false;
    }
#endif

    // digitalRead() only samples the GPIO input register - it doesn't touch pin
    // mode or issue any I2C transaction, so this can't contend with a transfer
    // that's genuinely in flight.
    bool sdaLow = digitalRead(sda) == LOW;
    bool sclLow = digitalRead(scl) == LOW;

    if (sdaLow) {
        if (sdaLowSinceMs == 0) sdaLowSinceMs = now;
    } else {
        sdaLowSinceMs = 0;
    }
    if (sclLow) {
        if (sclLowSinceMs == 0) sclLowSinceMs = now;
    } else {
        sclLowSinceMs = 0;
    }

    bool stuck = (sdaLowSinceMs != 0 && now - sdaLowSinceMs > kStuckThresholdMs) ||
                 (sclLowSinceMs != 0 && now - sclLowSinceMs > kStuckThresholdMs);
    if (!stuck) return false;

    BUSHAL_DBG(
        "[busHAL] sys_i2c(%d/%d) looks stuck (SDA held low=%lums SCL held low=%lums) - recovering\n",
        (int)sda,
        (int)scl,
        sdaLowSinceMs ? (now - sdaLowSinceMs) : 0,
        sclLowSinceMs ? (now - sclLowSinceMs) : 0
    );

#ifdef BRUCE_HAS_M5UNIFIED
    // The sys port is owned by M5.In_I2C, not by a TwoWire: the adapter's end() is a no-op and
    // TwoWire::begin() on it fails (fake port 255), which would leave the pins detached from the
    // controller after the bit-bang below. Release/re-begin M5.In_I2C itself instead, without
    // racing an RFID transaction in progress through the adapter (skip and retry next round).
    if (!trylockSysI2CBus()) return false;
    i2c_port_t m5Port = M5.In_I2C.getPort();
    M5.In_I2C.release();
#else
    TwoWire *wire = getSysI2CBus();
    uint32_t sysClock = wire ? wire->getClock() : 0;
    if (wire) wire->end();
#endif

    // Standard I2C bus recovery (per NXP UM10204 3.1.16): toggle SCL as a plain
    // GPIO up to 9 times to walk any slave stuck mid-byte/clock-stretch out of
    // its transaction (each pulse lets a stuck slave clock out one more bit and
    // release SDA once it reaches a byte boundary), then issue a manual STOP
    // condition (SDA low->high while SCL is high) to leave the bus in the
    // expected idle state before handing it back to the driver.
    pinMode(scl, OUTPUT_OPEN_DRAIN);
    pinMode(sda, OUTPUT_OPEN_DRAIN);
    digitalWrite(scl, HIGH);
    digitalWrite(sda, HIGH);
    delayMicroseconds(5);
    for (int i = 0; i < 9 && digitalRead(sda) == LOW; i++) {
        digitalWrite(scl, LOW);
        delayMicroseconds(5);
        digitalWrite(scl, HIGH);
        delayMicroseconds(5);
    }
    digitalWrite(sda, LOW);
    delayMicroseconds(5);
    digitalWrite(scl, HIGH);
    delayMicroseconds(5);
    digitalWrite(sda, HIGH);
    delayMicroseconds(5);

#ifdef BRUCE_HAS_M5UNIFIED
    M5.In_I2C.begin(m5Port, sda, scl);
    unlockSysI2CBus();
#else
    if (wire) wire->begin(sda, scl, sysClock);
#endif

    sdaLowSinceMs = 0;
    sclLowSinceMs = 0;
    BUSHAL_DBG("[busHAL] sys_i2c recovery complete\n");
    return true;
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
        if (AUX_SPI.begin((int8_t)sck, (int8_t)miso, (int8_t)mosi)) {
            // StickCPluses share SCK pins with SDCard, but not MISO and MOSI
            // AUX_SPI must be restarted every time we use it with every module in legacy mode
            // so if it doesn't conflict, save the variables to no reset the bus (T-HMI touch)
            if (!bruceConfigPins.SDCARD_bus.checkConflict(sck)) {
                sharedSpiMiso = miso;
                sharedSpiMosi = mosi;
            }
            // save only `sharedSpiSck` to force `AUX_SPI.end();`
            sharedSpiSck = sck;
        } else return nullptr;
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
