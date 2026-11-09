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
// Logic analyzer: RMT edge capture on 1-4 GPIOs, on-device waveform viewer, CSV export,
// SUMP/OLS ("PulseView") slave mode on the UART pins or the USB CDC port.
// TODO(logic-analyzer): feature-complete but held back from this release — not exposed in
// the Wire Tools menu or the serial CLI yet (see WireToolsMenu.cpp / wire_commands.cpp)
void logic_analyzer_setup();
// SUMP/OLS protocol server. `usb=true` drives the device's USB CDC port (suspending
// the serial CLI task for the duration); otherwise it runs on bruceConfigPins.uart_bus
void sump_server_run(uint32_t baud, bool usb = false);
// Baudrate/port picker + SUMP server entry shown in the Logic Analyzer menu
void sump_menu();
// Multimeter: continuity tests only (pin-to-pin and pin-to-GND) with beeper feedback
void continuity_tester_setup();

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

// Logic analyzer (RMT edge capture). Resolutions: 100k/200k/500k/1M/2M/5M/10M/20M ticks per second,
// depth = symbol buffer per channel (1024/2048/4096). Pins < 0 reuse the Logic Analyzer menu's
// session pins. Returns "OK ..." or "ERR: ..." (captures into the session shared with the UI).
String wire_la_capture(uint32_t resHz, uint32_t depth, int8_t p1, int8_t p2, int8_t p3, int8_t p4);
// Streams the edges of the last capture as CSV lines "ch,time_us,level" to the serial device
void wire_la_dump();
// "res=<hz> depth=<symbols> ch=<n> pins=<p1>,... last=<edges per channel>"
String wire_la_status();

// Number of probes configured in the Logic Analyzer session (1-4, 0 = unconfigured)
uint8_t wire_la_channels();
// Session resolution, for display only
uint32_t wire_la_resolution();
// Single-shot RMT capture on the session pins with the given resolution, then streams
// nSamples bytes over `out` (probe c -> bit c), sampling at `rate` Hz. Applies the
// Stage-0 trigger by skipping samples before the first (sample & trigMask)==trigVals
// match. Streams directly with no large intermediate buffer, so it works on tight
// heaps. Used by the SUMP server. Returns false on failure or abort.
bool wire_la_sump_stream(
    uint32_t resolutionHz, double rate, uint32_t nSamples, uint16_t trigMask, uint16_t trigVals,
    Print &out
);

// Continuity one-shot: pinB >= 0 tests A<->B (A driven LOW, B pulled up); pinB < 0 tests A to GND.
// Returns "OK SHORT ..." or "OK OPEN", or "ERR: ..."
String wire_cont_test(int8_t pinA, int8_t pinB);

#endif
#endif
