#ifndef __WIRE_TOOLS_H__
#define __WIRE_TOOLS_H__
#if !defined(LITE_VERSION)

#include <Arduino.h>
#include <vector>

// Interactive bus tools, shared by the Wire Tools menu and the serial command interface.

// UART terminal: baudrate presets/custom, remappable pins, live send/receive view
void uart_terminal_setup();
// I2C: device scan, register read/write/dump on bruceConfigPins.i2c_bus
void i2c_bus_setup();
// SPI: JEDEC flash detection and raw transaction console on bruceConfigPins.outer_bus
void spi_bus_setup();
// JTAG: bitbang IDCODE reader (TAP reset + DR shift) with user-assigned TCK/TMS/TDI/TDO
void jtag_scan_setup();

// Non-UI primitives used by the serial commands (core/serial_commands/wire_commands.cpp)

bool wire_uart_begin(uint32_t baud, int8_t rx, int8_t tx);
void wire_uart_end();
int wire_uart_send(const String &text);
int wire_uart_read(String &out, uint32_t timeoutMs);
String wire_uart_status();

int wire_i2c_scan(std::vector<uint8_t> &found, int8_t sda, int8_t scl);
String wire_i2c_read_reg(uint8_t addr, uint8_t reg, int8_t sda, int8_t scl);
bool wire_i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value, int8_t sda, int8_t scl);

bool wire_spi_begin(int8_t sck, int8_t miso, int8_t mosi, int8_t cs, uint32_t freqHz);
void wire_spi_end();
String wire_spi_transfer(const String &hexBytes); // returns response as hex string
String wire_spi_flash_detect();

// Returns "OK <idcode_hex>" or an error description; -1 pins keep the current assignment
String wire_jtag_read_idcode(int8_t tck, int8_t tms, int8_t tdi, int8_t tdo);

#endif
#endif
