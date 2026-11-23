#pragma once

/*
 * HAL for the I2C buses external modules can be wired to, plus SPI bus arbitration (below).
 *
 * bruceConfigPins.sys_i2c is the bus used by system peripherals (touchscreen, PMIC, RTC, IMU,
 * ...) set up once in boards/*\/interface.cpp and must never be stopped; some boards expose it.
 * bruceConfigPins.i2c_bus is user-configurable at runtime, its pins may double as UART/GPIO/SPI,
 * and it may share the same physical pins as sys_i2c.
 *
 * Every I2C user goes through an I2CLease (i2cLeaseDevice/i2cLeaseBus/i2cLeasePins): it finds the
 * bus a module is on, picks the right TwoWire instance, opens i2c_bus only while leased, and keeps
 * it from ever colliding with, or shutting down, the sys_i2c bus. Fixed system drivers (touch,
 * PMIC, gauge, IO expander) keep using the board's own TwoWire directly.
 */

#include <SPI.h>
#include <Wire.h>

// Tells the HAL which TwoWire instance is physically wired to bruceConfigPins.sys_i2c.
// Call from boards/*/interface.cpp (_setup_gpio/_post_setup_gpio) only if that board does not use
// the default (&Wire1).
void setSysI2CBus(TwoWire *wire);

// Returns the TwoWire instance currently wired to sys_i2c.
TwoWire *getSysI2CBus();

/*
 * I2C leases - the single entry point every I2C user (modules, tools, scripts) should go through.
 *
 * Many boards expose both sys_i2c and i2c_bus, and an external module may be plugged into
 * either. i2cLeaseDevice() looks for the module on every candidate bus (sys_i2c first by default,
 * because probing it never touches the shared i2c_bus pins) and hands back whichever bus answered.
 *
 * A lease keeps its bus usable until it is released (explicitly or by its destructor), so a driver
 * object that keeps a TwoWire* should own the lease as a member. Leases on the user bus are
 * reference counted: the last one released shuts the bus down, freeing the pins for other uses
 * (UART, GPIO, RF...). Leases on sys_i2c never shut it down, but restore any clock/timeout the
 * holder changed through the lease.
 */
enum class I2CBusId : uint8_t { None, Sys, User };
enum class I2CPrefer : uint8_t { SysFirst, UserFirst };

struct I2CDeviceQuery {
    uint8_t addr;
    I2CPrefer prefer = I2CPrefer::SysFirst;
    // Optional identity check run after the address ACKs (e.g. read a chip-ID register), so a
    // different chip answering at the same address on the other bus isn't mistaken for the module.
    bool (*verify)(TwoWire &wire, uint8_t addr) = nullptr;
    uint8_t retries = 1;         // probe attempts per bus
    bool fallbackToUser = false; // nothing answered: still return the user bus (found() == false)
                                 // so drivers that must wake the chip first (PN532) can try.
};

class I2CLease {
public:
    I2CLease() = default;
    ~I2CLease() { release(); }
    I2CLease(const I2CLease &) = delete;
    I2CLease &operator=(const I2CLease &) = delete;
    I2CLease(I2CLease &&other) noexcept;
    I2CLease &operator=(I2CLease &&other) noexcept;

    TwoWire *wire() const { return _wire; }
    I2CBusId bus() const { return _bus; }
    bool found() const { return _found; }
    int8_t sda() const { return _sda; }
    int8_t scl() const { return _scl; }
    explicit operator bool() const { return _wire != nullptr; }

    // Change the bus clock/timeout for this holder; the original values are restored on release
    // when the lease is on sys_i2c (the user bus is shut down anyway once its last lease goes).
    void setClock(uint32_t hz);
    void setTimeOut(uint16_t ms);
    void release();

private:
    friend struct I2CLeaseAccess; // bus_HAL.cpp internals
    TwoWire *_wire = nullptr;
    I2CBusId _bus = I2CBusId::None;
    bool _found = false;
    int8_t _sda = -1;
    int8_t _scl = -1;
    bool _clockSaved = false;
    uint32_t _savedClock = 0;
    bool _timeoutSaved = false;
    uint16_t _savedTimeout = 0;
};

// Looks for a device on sys_i2c and i2c_bus (order per q.prefer) and leases the bus it answered on.
// An invalid lease (operator bool false) means it wasn't found and q.fallbackToUser was not set
// (or the user bus couldn't be opened).
I2CLease i2cLeaseDevice(const I2CDeviceQuery &q);

// Leases one bus as configured (sys_i2c or i2c_bus). Leasing User when i2c_bus has the same pins
// as sys_i2c returns the sys bus (bus() == Sys).
I2CLease i2cLeaseBus(I2CBusId id);

// Leases a bus on explicit pins (scripts, serial tools, board-specific hats). Pins equal to
// sys_i2c return the sys bus. Fails (invalid lease) when the pins only partially overlap sys_i2c,
// when the user bus is already leased on other pins, when the SoC's only I2C controller belongs to
// sys_i2c, or when a line is held low before the bus is started.
I2CLease i2cLeasePins(int8_t sda, int8_t scl);

// Scans 0x08..0x77 on every configured bus (sys_i2c, then i2c_bus when distinct), calling cb for
// each ACK. Returns the number of devices found, or -1 if no bus could be opened.
int i2cScanAll(void (*cb)(I2CBusId bus, uint8_t addr, void *ctx), void *ctx);

// Single ACK check on a leased bus (takes the sys_i2c lock where needed).
bool i2cProbe(const I2CLease &lease, uint8_t addr);

// Short label for messages, e.g. "SYS 12/11" or "EXT 2/1".
String i2cBusLabel(I2CBusId id);

// Board-specific escape hatch for the user bus, used by the leases above when they open i2c_bus
// (for example, a board whose only hardware controller is reserved for sys_i2c can time-share it,
// as the Nesso N1 does). Most boards use the weak defaults in bus_HAL.cpp. releaseBoardI2CBus()
// returns true if it handled (shut down) that wire.
TwoWire *acquireBoardI2CBus(int8_t sda, int8_t scl);
bool releaseBoardI2CBus(TwoWire *wire);

// Watchdog for a wedged sys_i2c bus (a peripheral holding SDA/SCL low indefinitely - e.g. a
// target-mode PN532 session torn down mid-transaction - can leave the ESP32's I2C driver
// permanently in ESP_ERR_INVALID_STATE, taking down every other peripheral sharing the bus, such
// as the PMIC, until the whole device is power-cycled). Reads SDA/SCL via digitalRead() only, so
// it never issues an I2C transaction and can't itself contend with one in progress. Call
// periodically (a few times a second is plenty) from a task that always runs regardless of what
// menu/app is active, e.g. InputHandler(). Returns true if a stuck bus was detected and the
// standard 9-clock recovery + Wire re-init was performed.
bool checkAndRecoverSysI2CBus();

// On M5Unified boards, sys_i2c transactions issued through the M5SysWireAdapter (used by RFID
// drivers such as ST25R3916 that share sys_i2c) go straight to m5::I2C_Class, bypassing whatever
// locking M5Unified's own internal peripheral polling (PMIC/touch/IMU, driven by M5.update())
// uses. Two FreeRTOS tasks hitting the same I2C peripheral registers concurrently (the RFID
// driver's discovery loop on one task, M5.update() on taskInputHandler) corrupts M5GFX's I2C
// mutex bookkeeping and crashes with a `xTaskPriorityDisinherit` assert. The adapter takes this
// lock (blocking) around its own transactions; no-ops on boards without M5Unified.
void lockSysI2CBus();
void unlockSysI2CBus();

// Non-blocking variant for InputHandler()'s M5.update() call: RFAL's NFC-A anti-collision timing
// is tight enough (FDTMIN ~5ms) that blocking it for a whole M5.update() cycle (several PMIC/
// touch/IMU transactions) to wait out an in-progress RFID transaction reliably broke tag
// activation. M5.update() only needs to run roughly 5x/second, so skipping a cycle whenever the
// RFID driver currently holds the bus is free - it just runs again next tick. Returns true (and
// holds the lock, pair with unlockSysI2CBus()) if the bus was free; false if it was busy and the
// caller should skip. Always returns true on boards without M5Unified.
bool trylockSysI2CBus();

/*
 * SPI bus arbitration.
 *
 * The ESP32 family only has two general-purpose hardware SPI controllers available at runtime.
 * One is permanently owned by the display (and, when physically wired to the same pins, the SD
 * card too) — both are alive for the entire program lifetime, so they always win. The other is a
 * single auxiliary hardware bus, reused one owner at a time by whichever peripheral
 * (CC1101/NRF24/LoRa/W5500/ST25R3916/...) is actively in use.
 *
 * A peripheral wired to a third, genuinely distinct set of pins has no hardware controller left:
 * acquireSPIBus() returns nullptr and the caller must fall back to its own software/bit-banged
 * SPI path (if the underlying driver supports one) or fail outright.
 */

// Picks the SPIClass instance that should be used for a peripheral wired to the given pins,
// following priority Display > SDCard > shared auxiliary bus (starting/restarting the auxiliary
// bus only when the requested pins differ from whoever last used it). Returns nullptr if sck,
// miso or mosi is unset, or if the pins matched the display's bus but the board is headless.
SPIClass *acquireSPIBus(gpio_num_t sck, gpio_num_t miso, gpio_num_t mosi);
