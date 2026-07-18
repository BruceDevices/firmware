#pragma once

/*
 * HAL for the user-facing I2C bus (bruceConfigPins.i2c_bus).
 *
 * bruceConfigPins.sys_i2c is the bus used by system peripherals (touchscreen, PMIC, RTC, IMU,
 * ...) set up once in boards/*\/interface.cpp and must never be stopped.
 * bruceConfigPins.i2c_bus is user-configurable at runtime and may end up sharing the same
 * physical pins as sys_i2c. This HAL picks the right TwoWire instance for i2c_bus and keeps it
 * from ever colliding with, or shutting down, the sys_i2c bus.
 */

#include <SPI.h>
#include <Wire.h>

// Tells the HAL which TwoWire instance is physically wired to bruceConfigPins.sys_i2c.
// Call from boards/*/interface.cpp (_setup_gpio/_post_setup_gpio) only if that board does not use
// the default (&Wire1).
void setSysI2CBus(TwoWire *wire);

// Returns the TwoWire instance currently wired to sys_i2c.
TwoWire *getSysI2CBus();

// Acquires the TwoWire instance for bruceConfigPins.i2c_bus, starting it on the requested pins
// (defaults to bruceConfigPins.i2c_bus.sda/scl). If those pins match sys_i2c, the shared sys bus
// is returned instead of starting a second, colliding bus. Must be paired with releaseI2CBus()
// once the caller is done using the bus.
TwoWire *acquireI2CBus(int8_t sda, int8_t scl);
TwoWire *acquireI2CBus();

// Ends the i2c_bus TwoWire instance started by acquireI2CBus(), unless it turned out to be the
// same physical bus as sys_i2c (which must stay on).
void releaseI2CBus();

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
