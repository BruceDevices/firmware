# Waveshare ESP32-C6 Based Development Boards

> All Waveshare boards using the ESP32-C6 chip family.
> ESP32-C6 features WiFi 6, BLE 5.0, and IEEE 802.15.4 (Zigbee 3.0 / Thread).
> Source: Waveshare wiki, product pages, community configs.

---

## Table of Contents

- [ESP32-C6 Chip Overview](#esp32-c6-chip-overview)
- [ESP32-C6-LCD-1.47](#esp32-c6-lcd-147)
- [ESP32-C6-Touch-LCD-1.47](#esp32-c6-touch-lcd-147)
- [ESP32-C6-LCD-1.69](#esp32-c6-lcd-169)
- [ESP32-C6-Touch-LCD-1.69](#esp32-c6-touch-lcd-169)
- [ESP32-C6-Touch-LCD-1.83](#esp32-c6-touch-lcd-183)
- [ESP32-C6-LCD-1.9](#esp32-c6-lcd-19)
- [ESP32-C6-GEEK](#esp32-c6-geek)
- [ESP32-C6-Touch-AMOLED-1.32](#esp32-c6-touch-amoled-132)
- [ESP32-C6-Touch-AMOLED-1.43](#esp32-c6-touch-amoled-143)
- [ESP32-C6-Touch-AMOLED-1.8](#esp32-c6-touch-amoled-18)
- [ESP32-C6-Zero](#esp32-c6-zero)
- [ESP32-C6-Pico](#esp32-c6-pico)
- [ESP32-C6-DEV-KIT-N8](#esp32-c6-dev-kit-n8)

---

## ESP32-C6 Chip Overview

| Property | Value |
|----------|-------|
| Architecture | 32-bit RISC-V |
| High-Performance Core | Up to 160 MHz |
| Low-Power Core | Up to 20 MHz |
| ROM | 320 KB |
| HP SRAM | 512 KB |
| LP SRAM | 16 KB |
| WiFi | 802.11ax (WiFi 6), 2.4 GHz |
| Bluetooth | BLE 5.0 |
| IEEE 802.15.4 | Zigbee 3.0, Thread |
| USB | Full-speed USB 2.0 |
| ADC | 12-bit SAR, 7 channels |
| GPIO | Up to 30 |

### Key Advantages Over ESP32-S3

- **WiFi 6** (802.11ax) with lower power consumption
- **Zigbee 3.0 / Thread** support via 802.15.4 radio
- Single-core RISC-V (lower power, simpler)
- Better suited for battery-powered IoT/Matter devices

### Key Limitations vs ESP32-S3

- Single core (no parallel task execution)
- 160 MHz max (vs 240 MHz)
- No PSRAM support on most variants
- Fewer GPIOs
- Less CPU power for UI rendering (smaller displays recommended)

---

## ESP32-C6-LCD-1.47

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-LCD-1.47 |
| Chip | ESP32-C6 |
| Flash | 4 MB |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| WiFi | WiFi 6 (802.11ax) |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes (802.15.4) |
| USB Connector | USB Type-C |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.47 inch |
| Type | IPS LCD |
| Resolution | 172 x 320 |
| Colors | 262K |
| Driver IC | ST7789 |
| Interface | SPI |

### GPIO Pin Mapping (LCD SPI)

| GPIO | Function |
|------|----------|
| GPIO7 | LCD_SCL (SPI Clock) |
| GPIO6 | LCD_SDA (SPI MOSI) |
| GPIO14 | LCD_CS |
| GPIO15 | LCD_DC |
| GPIO21 | LCD_RST |
| GPIO22 | LCD_BL (Backlight) |

### Additional Features

- Onboard colorful RGB LED
- Development via Arduino IDE and ESP-IDF
- CircuitPython support available

### Firmware Gotchas

- Only 4MB Flash -- no PSRAM. Keep firmware and assets compact.
- ST7789 at 172x320 needs display offset (X=34).
- WiFi 6 requires ESP-IDF 5.x or newer for full support.
- Zigbee/Thread stack consumes significant Flash -- plan partition table accordingly.

---

## ESP32-C6-Touch-LCD-1.47

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-Touch-LCD-1.47 |
| Chip | ESP32-C6FH8 |
| Flash | 8 MB |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.47 inch |
| Resolution | 172 x 320 |
| Colors | 262K |
| Driver IC | ST7789 |
| Interface | SPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Interface | I2C |

### Key Differences from Non-Touch Variant

- 8MB Flash (vs 4MB) -- more room for assets and Zigbee stack.
- Touch controller added on I2C bus.
- Uses ESP32-C6FH8 chip variant.

---

## ESP32-C6-LCD-1.69

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-LCD-1.69 |
| Chip | ESP32-C6 |
| Flash | 16 MB |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.69 inch |
| Resolution | 240 x 280 |
| Colors | 262K |
| Driver IC | ST7789V2 |
| Interface | SPI |

### Sensors & Peripherals

| Component | Model | Interface |
|-----------|-------|-----------|
| 6-axis IMU | QMI8658 | I2C |
| RTC | Onboard | I2C |
| Audio Codec | Onboard | I2S |
| Battery | Li-ion charge manager | -- |

### Additional Features

- AI Speech capability (via audio codec + mic)
- Lithium battery recharge management

---

## ESP32-C6-Touch-LCD-1.69

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-Touch-LCD-1.69 |
| Chip | ESP32-C6 |
| Flash | 16 MB |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.69 inch |
| Resolution | 240 x 280 |
| Colors | 262K |
| Driver IC | ST7789V2 |
| Interface | SPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Interface | I2C |

### Sensors & Peripherals

Same as LCD-1.69: QMI8658 IMU, RTC, audio codec, battery management.

---

## ESP32-C6-Touch-LCD-1.83

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-Touch-LCD-1.83 |
| Chip | ESP32-C6 |
| Flash | 16 MB |
| PSRAM | None |
| HP SRAM | 512 KB |
| LP SRAM | 16 KB |
| ROM | 320 KB |
| Processor | RISC-V HP: 160 MHz, LP: 20 MHz |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes (802.15.4) |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.83 inch |
| Resolution | 240 x 284 |
| Colors | 65K |
| Driver IC | SPI-based |
| Interface | SPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Interface | I2C |

### Sensors & Peripherals

| Component | Model | Interface |
|-----------|-------|-----------|
| 6-axis IMU | QMI8658 | I2C |
| RTC | Onboard | I2C |
| Audio Codec | Onboard | I2S |
| Microphones | Built-in | I2S |
| Speaker | Built-in | I2S |
| Battery | Li-ion charge manager | -- |

---

## ESP32-C6-LCD-1.9

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-LCD-1.9 |
| Chip | ESP32-C6FH8 |
| Flash | 8 MB |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.9 inch |
| Resolution | 170 x 320 |
| Colors | 262K |
| Driver IC | ST7789 |
| Interface | SPI |
| Touch | Optional (touch and non-touch SKUs) |

### Sensors & Peripherals

| Component | Model | Interface |
|-----------|-------|-----------|
| 6-axis IMU | QMI8658 | I2C |
| TF Card | MicroSD slot | SPI |
| Battery | Li-ion header | -- |

### Additional Features

- Pico-compatible header (40-pin compatible with Raspberry Pi Pico HATs)
- Supports 90 degree hardware rotation

### Firmware Gotchas

- 170x320 resolution requires offset configuration for ST7789.
- Pico HAT compatibility opens up a large ecosystem of expansion boards.

---

## ESP32-C6-GEEK

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-GEEK |
| Chip | ESP32-C6 |
| Flash | 16 MB |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes |
| USB Connector | USB-A male port (dongle form factor) |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.14 inch |
| Resolution | 240 x 135 |
| Colors | 65K |
| Driver IC | ST7789 |
| Interface | SPI |

### Physical Design

- USB-A male plug -- plugs directly into USB ports
- White plastic case included
- Compact dongle/stick form factor

### Interfaces

| Interface | Details |
|-----------|---------|
| TF Card | MicroSD slot |
| I2C | Expansion header |
| UART | Expansion header |
| GPIO | Expansion header |

### Firmware Gotchas

- USB-A form factor -- designed to plug into computers, power banks, or USB chargers.
- 16MB Flash provides plenty of room for OTA, Zigbee stack, and web server.
- Small display suitable for status text, QR codes, simple graphics.
- ST7789 at 240x135 -- horizontal orientation by default.

---

## ESP32-C6-Touch-AMOLED-1.32

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-Touch-AMOLED-1.32 |
| Chip | ESP32-C6 |
| Flash | 16 MB |
| PSRAM | None |
| HP SRAM | 512 KB |
| LP SRAM | 16 KB |
| ROM | 320 KB |
| Processor | RISC-V, 160 MHz |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes (802.15.4) |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.32 inch (round) |
| Resolution | 466 x 466 |
| Colors | 16.7M |
| Driver IC | CO5300 |
| Interface | QSPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | CST820 |
| Interface | I2C |

### Firmware Gotchas

- 466x466 AMOLED on a single-core 160 MHz RISC-V is demanding.
- No PSRAM -- frame buffer must fit in 512KB SRAM. Use partial rendering.
- CO5300 QSPI requires specific init sequence.
- AMOLED round display needs circular clipping.
- Zigbee + AMOLED simultaneously may stress the single core.

---

## ESP32-C6-Touch-AMOLED-1.43

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-Touch-AMOLED-1.43 |
| Chip | ESP32-C6 |
| Flash | 16 MB |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.43 inch (round) |
| Resolution | 466 x 466 |
| Colors | 16.7M |
| Driver IC | CO5300 |
| Interface | QSPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Interface | I2C |

### Sensors & Peripherals

| Component | Model | Interface |
|-----------|-------|-----------|
| 6-axis IMU | QMI8658 | I2C |
| RTC | Onboard | I2C |
| Dual Microphone | Array with noise cancellation | I2S |
| Audio Codec | Low-power | I2S |
| TF Card | MicroSD slot | SPI |
| Battery | Li-ion header | -- |

---

## ESP32-C6-Touch-AMOLED-1.8

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-Touch-AMOLED-1.8 |
| Chip | ESP32-C6 |
| Flash | 16 MB |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes (802.15.4) |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.8 inch |
| Resolution | 368 x 448 |
| Colors | 16.7M |
| Driver IC | SH8601 |
| Interface | QSPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | FT3168 or FT6146 |
| Interface | I2C |

### Sensors & Peripherals

| Component | Model | Interface |
|-----------|-------|-----------|
| 6-axis IMU | QMI8658 | I2C |
| RTC | Onboard | I2C |
| Power Management | Highly integrated IC | -- |
| Audio Codec | Low-power | I2S |

### Firmware Gotchas

- SH8601 AMOLED driver has different init from CO5300.
- FT3168 and FT6146 are both supported -- check which variant you have.
- No PSRAM -- partial rendering essential for LVGL.

---

## ESP32-C6-Zero

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-Zero |
| Chip | ESP32-C6FH4 |
| Flash | 4 MB (integrated) |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| HP SRAM | 512 KB |
| LP SRAM | 16 KB |
| ROM | 320 KB |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes |
| USB Connector | USB Type-C |

### Physical Design

- Ultra-compact form factor with castellated holes
- Designed for direct PCB soldering or breadboard use

### Interfaces

- GPIO pins exposed via castellated pads
- USB Type-C for power and programming
- SPI, I2C, UART available via GPIO mux

### Firmware Gotchas

- 4MB Flash only -- plan partition table carefully if using Zigbee/Thread stack.
- No display -- general purpose IoT/Zigbee endpoint device.
- Castellated holes for manufacturing integration.

---

## ESP32-C6-Pico

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-Pico |
| Module | ESP32-C6-MINI-1 |
| Flash | 4 MB |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| HP SRAM | 512 KB |
| ROM | 320 KB |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes |

### Physical Design

- Raspberry Pi Pico-compatible form factor and pin header layout
- Compatible with most Pico HATs and add-on boards

### Firmware Gotchas

- Pico-compatible pinout -- use Pico HAT ecosystem.
- 4MB Flash limit.

---

## ESP32-C6-DEV-KIT-N8

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-C6-DEV-KIT-N8 |
| Module | ESP32-C6-WROOM-1-N8 |
| Flash | 8 MB |
| PSRAM | None |
| Processor | RISC-V, 160 MHz |
| HP SRAM | 512 KB |
| LP SRAM | 16 KB |
| ROM | 320 KB |
| WiFi | WiFi 6 |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes |
| USB Connector | USB Type-C |

### Key Features

- Onboard CH343 UART chip and CH334 USB HUB chip
- Supports USB and UART development simultaneously via single USB-C port
- Standard development board form factor with pin headers
- Most GPIOs exposed

### Firmware Gotchas

- 8MB Flash provides comfortable room for Zigbee/Matter + OTA.
- Dual USB interface (UART + native USB) via single USB-C -- convenient for debugging.
- Standard reference design for C6 development.

---

## ESP32-C6 Board Selection Guide

| Use Case | Recommended Board | Why |
|----------|------------------|-----|
| Zigbee gateway with display | ESP32-C6-LCD-1.47 or 1.69 | Small display + 802.15.4 |
| Matter/Thread endpoint | ESP32-C6-Zero or Pico | Minimal, low-power |
| Wearable with AMOLED | ESP32-C6-Touch-AMOLED-1.32 | Round AMOLED, compact |
| WiFi 6 USB dongle | ESP32-C6-GEEK | USB-A plug, display, TF card |
| Standard C6 development | ESP32-C6-DEV-KIT-N8 | Full GPIO, 8MB flash, dual USB |
| Smart home panel | ESP32-C6-Touch-LCD-1.83 | Touch, audio, sensors |
| AI voice assistant | ESP32-C6-Touch-LCD-1.69 | Audio codec + touch |
