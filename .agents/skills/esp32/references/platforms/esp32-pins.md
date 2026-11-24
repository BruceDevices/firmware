# ESP32 GPIO Pin Reference

## Table of Contents

- [ESP32 GPIO Model Overview](#esp32-gpio-model-overview)
- [Pin Categories Summary](#pin-categories-summary)
- [Complete GPIO Pin Table](#complete-gpio-pin-table)
- [Flash and PSRAM Reservations](#flash-and-psram-reservations)
  - [GPIO6-11: SPI Flash (NEVER USE)](#gpio6-11-spi-flash-never-use)
  - [GPIO16-17: PSRAM (WROVER Only)](#gpio16-17-psram-wrover-only)
- [Protocol Pin Groups](#protocol-pin-groups)
  - [I2C (Conventional Defaults)](#i2c-conventional-defaults)
  - [SPI Buses](#spi-buses)
  - [UARTs](#uarts)
  - [PWM (LEDC)](#pwm-ledc)
  - [1-Wire](#1-wire)
  - [DAC (Digital-to-Analog)](#dac-digital-to-analog)
- [ADC Channel Mapping](#adc-channel-mapping)
  - [ADC1 (Always Available — Even with WiFi)](#adc1-always-available--even-with-wifi)
  - [ADC2 (UNAVAILABLE When WiFi Active)](#adc2-unavailable-when-wifi-active)
- [Touch Channel Mapping](#touch-channel-mapping)
- [RTC GPIO Mapping](#rtc-gpio-mapping)
- [Variant Comparison Table](#variant-comparison-table)
  - [Key Variant Notes](#key-variant-notes)

---

## ESP32 GPIO Model Overview

The ESP32 GPIO system differs fundamentally from Raspberry Pi:

- **GPIO Matrix:** Most peripherals can be routed to any GPIO through the GPIO matrix, providing flexible pin assignment (unlike RPi's fixed ALT functions)
- **Special Pin Categories:** Certain pins have hardware restrictions — strapping pins affect boot behavior, flash pins are never usable, input-only pins cannot drive outputs
- **Variable Capabilities:** Not all GPIOs have the same features — ADC, touch sensing, DAC, and RTC wake capabilities vary by pin
- **Variant Differences:** ESP32 variants (S2, S3, C3, C6) have different GPIO counts, capabilities, and restrictions

**Default Reference:** This document uses **ESP32-WROOM-32** as the primary reference module unless otherwise noted.

---

## Pin Categories Summary

| Category | GPIOs | Count | Usage |
|----------|-------|-------|-------|
| **Freely Usable** | 4, 13, 14, 16*, 17*, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33 | 15 | No restrictions, safe for any purpose |
| **Strapping (use with caution)** | 0, 2, 5, 12, 15 | 5 | Affect boot behavior — safe after boot |
| **Flash Reserved** | 6, 7, 8, 9, 10, 11 | 6 | NEVER use — connected to SPI flash |
| **PSRAM Reserved** | 16, 17 | 2 | Reserved on WROVER, free on WROOM |
| **Input Only** | 34, 35, 36, 39 | 4 | Cannot output, no internal pulls |
| **Not Exposed** | 20, 24, 28-31, 37, 38 | 8 | Not available on WROOM-32 module |

*GPIO16/17 are usable on WROOM but reserved for PSRAM on WROVER modules.

---

## Complete GPIO Pin Table

Reference for ESP32-WROOM-32 module, GPIOs 0-39:

| GPIO | Strapping | Reserved | I/O | ADC | Touch | RTC | Notes |
|------|-----------|----------|-----|-----|-------|-----|-------|
| 0 | YES (boot mode) | No | I/O | ADC2_CH1 | TOUCH1 | RTC_GPIO11 | LOW=download mode; safe after boot |
| 1 | No | No* | I/O | — | — | — | UART0 TX — avoid (USB serial) |
| 2 | YES (boot mode) | No | I/O | ADC2_CH2 | TOUCH2 | RTC_GPIO12 | Often onboard LED; safe after boot |
| 3 | No | No* | I/O | — | — | — | UART0 RX — avoid (USB serial) |
| 4 | No | No | I/O | ADC2_CH0 | TOUCH0 | RTC_GPIO10 | Clean — commonly used for 1-Wire |
| 5 | YES (SDIO timing) | No | I/O | — | — | — | Internal pull-up; VSPI CS default |
| 6 | No | FLASH | I/O | — | — | — | **NEVER USE** — SPI flash CLK |
| 7 | No | FLASH | I/O | — | — | — | **NEVER USE** — SPI flash D0 |
| 8 | No | FLASH | I/O | — | — | — | **NEVER USE** — SPI flash D1 |
| 9 | No | FLASH | I/O | — | — | — | **NEVER USE** — SPI flash D2 |
| 10 | No | FLASH | I/O | — | — | — | **NEVER USE** — SPI flash D3 |
| 11 | No | FLASH | I/O | — | — | — | **NEVER USE** — SPI flash CMD |
| 12 | **YES (MTDI)** | No | I/O | ADC2_CH5 | TOUCH5 | RTC_GPIO15 | **DANGER:** flash voltage select — see esp32-specifics.md |
| 13 | No | No | I/O | ADC2_CH4 | TOUCH4 | RTC_GPIO14 | HSPI MOSI default |
| 14 | No | No | I/O | ADC2_CH6 | TOUCH6 | RTC_GPIO16 | HSPI CLK default |
| 15 | YES (MTDO) | No | I/O | ADC2_CH3 | TOUCH3 | RTC_GPIO13 | Debug log control; HSPI CS default |
| 16 | No | PSRAM* | I/O | — | — | — | Free on WROOM; reserved on WROVER |
| 17 | No | PSRAM* | I/O | — | — | — | Free on WROOM; reserved on WROVER |
| 18 | No | No | I/O | — | — | — | VSPI CLK default |
| 19 | No | No | I/O | — | — | — | VSPI MISO default |
| 20 | No | Not exposed | I/O | — | — | — | Not available on WROOM-32 |
| 21 | No | No | I/O | — | — | — | I2C SDA default |
| 22 | No | No | I/O | — | — | — | I2C SCL default |
| 23 | No | No | I/O | — | — | — | VSPI MOSI default |
| 24 | No | Not exposed | I/O | — | — | — | Not available on WROOM-32 |
| 25 | No | No | I/O | ADC2_CH8 | — | RTC_GPIO6 | DAC1 output |
| 26 | No | No | I/O | ADC2_CH9 | — | RTC_GPIO7 | DAC2 output |
| 27 | No | No | I/O | ADC2_CH7 | TOUCH7 | RTC_GPIO17 | Clean GPIO |
| 28 | No | Not exposed | I/O | — | — | — | Not available on WROOM-32 |
| 29 | No | Not exposed | I/O | — | — | — | Not available on WROOM-32 |
| 30 | No | Not exposed | I/O | — | — | — | Not available on WROOM-32 |
| 31 | No | Not exposed | I/O | — | — | — | Not available on WROOM-32 |
| 32 | No | No | I/O | ADC1_CH4 | TOUCH9 | RTC_GPIO9 | ADC1 — works with WiFi |
| 33 | No | No | I/O | ADC1_CH5 | TOUCH8 | RTC_GPIO8 | ADC1 — works with WiFi |
| 34 | No | No | **INPUT** | ADC1_CH6 | — | RTC_GPIO4 | Input only, no internal pulls |
| 35 | No | No | **INPUT** | ADC1_CH7 | — | RTC_GPIO5 | Input only, no internal pulls |
| 36 | No | No | **INPUT** | ADC1_CH0 | — | RTC_GPIO0 | Input only (SENSOR_VP) |
| 37 | No | Not exposed | **INPUT** | ADC1_CH1 | — | RTC_GPIO1 | Not available on WROOM-32 |
| 38 | No | Not exposed | **INPUT** | ADC1_CH2 | — | RTC_GPIO2 | Not available on WROOM-32 |
| 39 | No | No | **INPUT** | ADC1_CH3 | — | RTC_GPIO3 | Input only (SENSOR_VN) |

---

## Flash and PSRAM Reservations

### GPIO6-11: SPI Flash (NEVER USE)

These pins are hardwired to the SPI flash chip on all ESP32 modules:

| GPIO | Flash Function | Result if Used |
|------|----------------|----------------|
| 6 | SPICLK | Chip crash/hang |
| 7 | SPIQ (D0) | Chip crash/hang |
| 8 | SPID (D1) | Chip crash/hang |
| 9 | SPIHD (D2) | Chip crash/hang |
| 10 | SPIWP (D3) | Chip crash/hang |
| 11 | SPICS0 (CMD) | Chip crash/hang |

**Rule:** Never assign GPIO6-11 for any purpose. Attempting to use them will crash the chip immediately.

### GPIO16-17: PSRAM (WROVER Only)

| Module | GPIO16 | GPIO17 | Reason |
|--------|--------|--------|--------|
| WROOM-32 | Usable | Usable | No PSRAM on module |
| WROVER | Reserved | Reserved | Connected to PSRAM |
| WROVER-B | Reserved | Reserved | Connected to PSRAM |
| WROOM-32E | Usable | Usable | No PSRAM on module |

**Rule:** Always check module type. If using WROVER, avoid GPIO16/17.

---

## Protocol Pin Groups

### I2C (Conventional Defaults)

Unlike Raspberry Pi, ESP32 I2C can use any GPIO via the GPIO matrix:

| Signal | Default GPIO | Notes |
|--------|--------------|-------|
| SDA | GPIO21 | Convention only — any GPIO works |
| SCL | GPIO22 | Convention only — any GPIO works |

**Usage:**
```cpp
// Arduino
Wire.begin(21, 22);  // SDA, SCL

// ESP-IDF
i2c_config_t conf = {
    .sda_io_num = 21,
    .scl_io_num = 22,
    // ...
};
```

**Note:** Do not use input-only pins (34-39) for I2C — they cannot drive the bus.

### SPI Buses

ESP32 has two user-accessible SPI buses:

**VSPI (SPI3) — Recommended:**

| Signal | GPIO | Notes |
|--------|------|-------|
| MOSI | GPIO23 | |
| MISO | GPIO19 | |
| SCLK | GPIO18 | |
| CS | GPIO5 | Strapping pin — safe after boot |

**HSPI (SPI2) — Use with caution:**

| Signal | GPIO | Notes |
|--------|------|-------|
| MOSI | GPIO13 | |
| MISO | GPIO12 | **DANGER:** Strapping pin — see esp32-specifics.md |
| SCLK | GPIO14 | |
| CS | GPIO15 | Strapping pin — safe after boot |

**Warning:** HSPI uses GPIO12 which is a dangerous strapping pin. Prefer VSPI unless you specifically need both SPI buses.

### UARTs

| UART | TX | RX | Notes |
|------|----|----|-------|
| UART0 | GPIO1 | GPIO3 | USB serial — avoid unless needed |
| UART1 | Any | Any | Fully remappable via GPIO matrix |
| UART2 | Any | Any | Fully remappable via GPIO matrix |

**Common UART1/UART2 assignments:**
- UART1: GPIO4 (TX), GPIO5 (RX)
- UART2: GPIO16 (TX), GPIO17 (RX) — if not WROVER

### PWM (LEDC)

ESP32 PWM uses the LEDC peripheral, which can output to any GPIO:

- **Channels:** 16 (8 high-speed, 8 low-speed)
- **Resolution:** 1-16 bits
- **Frequency:** Up to 40MHz (hardware dependent on duty resolution)
- **Any output-capable GPIO works** — no fixed PWM pins like RPi

**Rule:** Cannot use input-only pins (34-39) for PWM output.

### 1-Wire

- **Common Pin:** GPIO4 (by convention)
- **Flexibility:** Any GPIO with output capability
- **Pull-up:** Requires 4.7kΩ external pull-up to 3.3V

### DAC (Digital-to-Analog)

| Channel | GPIO | Notes |
|---------|------|-------|
| DAC1 | GPIO25 | 8-bit resolution |
| DAC2 | GPIO26 | 8-bit resolution |

**Note:** DAC is only available on original ESP32 and ESP32-S2. Not available on S3, C3, or C6.

---

## ADC Channel Mapping

### ADC1 (Always Available — Even with WiFi)

| Channel | GPIO | Notes |
|---------|------|-------|
| ADC1_CH0 | GPIO36 | Input only (SENSOR_VP) |
| ADC1_CH1 | GPIO37 | Not on WROOM-32 |
| ADC1_CH2 | GPIO38 | Not on WROOM-32 |
| ADC1_CH3 | GPIO39 | Input only (SENSOR_VN) |
| ADC1_CH4 | GPIO32 | Full I/O |
| ADC1_CH5 | GPIO33 | Full I/O |
| ADC1_CH6 | GPIO34 | Input only |
| ADC1_CH7 | GPIO35 | Input only |

**Design Rule:** Use ADC1 channels for analog inputs in WiFi projects.

### ADC2 (UNAVAILABLE When WiFi Active)

| Channel | GPIO | Notes |
|---------|------|-------|
| ADC2_CH0 | GPIO4 | |
| ADC2_CH1 | GPIO0 | Strapping pin |
| ADC2_CH2 | GPIO2 | Strapping pin |
| ADC2_CH3 | GPIO15 | Strapping pin |
| ADC2_CH4 | GPIO13 | |
| ADC2_CH5 | GPIO12 | **DANGER:** Strapping pin |
| ADC2_CH6 | GPIO14 | |
| ADC2_CH7 | GPIO27 | |
| ADC2_CH8 | GPIO25 | Also DAC1 |
| ADC2_CH9 | GPIO26 | Also DAC2 |

**Warning:** ADC2 returns invalid readings when WiFi or Bluetooth is active. See esp32-specifics.md for details.

---

## Touch Channel Mapping

ESP32 has 10 capacitive touch sensing channels:

| Channel | GPIO | Notes |
|---------|------|-------|
| TOUCH0 | GPIO4 | |
| TOUCH1 | GPIO0 | Strapping pin |
| TOUCH2 | GPIO2 | Strapping pin |
| TOUCH3 | GPIO15 | Strapping pin |
| TOUCH4 | GPIO13 | |
| TOUCH5 | GPIO12 | **DANGER:** Strapping pin |
| TOUCH6 | GPIO14 | |
| TOUCH7 | GPIO27 | |
| TOUCH8 | GPIO33 | Also ADC1 |
| TOUCH9 | GPIO32 | Also ADC1 |

**Notes:**
- Touch sensing works in deep sleep via ULP coprocessor
- ESP32-C3 and C6 do NOT have touch sensing capability
- External components on touch pins affect sensitivity

---

## RTC GPIO Mapping

RTC GPIOs can wake the ESP32 from deep sleep and be controlled by the ULP coprocessor:

| RTC GPIO | GPIO | ADC | Touch | Notes |
|----------|------|-----|-------|-------|
| RTC_GPIO0 | GPIO36 | ADC1_CH0 | — | Input only (SENSOR_VP) |
| RTC_GPIO1 | GPIO37 | ADC1_CH1 | — | Not on WROOM-32 |
| RTC_GPIO2 | GPIO38 | ADC1_CH2 | — | Not on WROOM-32 |
| RTC_GPIO3 | GPIO39 | ADC1_CH3 | — | Input only (SENSOR_VN) |
| RTC_GPIO4 | GPIO34 | ADC1_CH6 | — | Input only |
| RTC_GPIO5 | GPIO35 | ADC1_CH7 | — | Input only |
| RTC_GPIO6 | GPIO25 | ADC2_CH8 | — | DAC1 |
| RTC_GPIO7 | GPIO26 | ADC2_CH9 | — | DAC2 |
| RTC_GPIO8 | GPIO33 | ADC1_CH5 | TOUCH8 | |
| RTC_GPIO9 | GPIO32 | ADC1_CH4 | TOUCH9 | |
| RTC_GPIO10 | GPIO4 | ADC2_CH0 | TOUCH0 | |
| RTC_GPIO11 | GPIO0 | ADC2_CH1 | TOUCH1 | Strapping |
| RTC_GPIO12 | GPIO2 | ADC2_CH2 | TOUCH2 | Strapping |
| RTC_GPIO13 | GPIO15 | ADC2_CH3 | TOUCH3 | Strapping |
| RTC_GPIO14 | GPIO13 | ADC2_CH4 | TOUCH4 | |
| RTC_GPIO15 | GPIO12 | ADC2_CH5 | TOUCH5 | **DANGER:** Strapping |
| RTC_GPIO16 | GPIO14 | ADC2_CH6 | TOUCH6 | |
| RTC_GPIO17 | GPIO27 | ADC2_CH7 | TOUCH7 | |

**Deep Sleep Wake Sources:**
- EXT0: Single RTC GPIO, level-triggered
- EXT1: Multiple RTC GPIOs, any-high or all-low trigger
- Touch pad wake: Any touch channel
- ULP coprocessor: Programmable wake conditions

---

## Variant Comparison Table

| Feature | ESP32 | ESP32-S2 | ESP32-S3 | ESP32-C3 | ESP32-C6 |
|---------|-------|----------|----------|----------|----------|
| **CPU Architecture** | Xtensa LX6 | Xtensa LX7 | Xtensa LX7 | RISC-V | RISC-V |
| **CPU Cores** | 2 | 1 | 2 | 1 | 1 |
| **GPIO Count** | 34 (0-39) | 43 | 45 | 22 (0-21) | 30 |
| **ADC Channels** | 18 | 20 | 20 | 6 | 7 |
| **ADC2/WiFi Conflict** | Yes | Yes | Yes | **No** | **No** |
| **Touch Channels** | 10 | 14 | 14 | **None** | **None** |
| **DAC Channels** | 2 | 2 | **None** | **None** | **None** |
| **USB OTG** | No | Yes (GPIO19/20) | Yes (GPIO19/20) | No | No |
| **Bluetooth** | Classic + BLE | **None** | BLE 5.0 | BLE 5.0 | BLE 5.0 |
| **WiFi** | 802.11 b/g/n | 802.11 b/g/n | 802.11 b/g/n | 802.11 b/g/n | **802.11ax (WiFi 6)** |
| **Thread/Zigbee** | No | No | No | No | **Yes** |
| **Flash Pins** | GPIO6-11 | GPIO26-32 | GPIO26-32 | GPIO12-17 | GPIO24-29 |
| **Strapping Pins** | 0, 2, 5, 12, 15 | 0, 45, 46 | 0, 3, 45, 46 | 2, 8, 9 | 8, 9, 15 |

### Key Variant Notes

- **ESP32-C3/C6:** No ADC2/WiFi conflict — all ADC channels work with WiFi
- **ESP32-S2/S3:** USB OTG requires GPIO19/20 — reserve if using native USB
- **ESP32-C6:** Only variant with WiFi 6 and Thread/Zigbee support
- **Touch sensing:** Only original ESP32, S2, and S3 have capacitive touch
- **DAC:** Only original ESP32 and S2 have analog output capability

---
