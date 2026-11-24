# Waveshare ESP32 General Purpose Development Boards

> Non-display-specific ESP32 development boards for general IoT, industrial, connectivity, and audio applications.
> Source: Waveshare wiki, product pages, community configs.

---

## Table of Contents

- [ESP32-S3-DEV-KIT-N8R8](#esp32-s3-dev-kit-n8r8)
- [ESP32-S3-Pico](#esp32-s3-pico)
- [ESP32-S3-Zero](#esp32-s3-zero)
- [ESP32-S3-Nano](#esp32-s3-nano)
- [ESP32-S3-Matrix](#esp32-s3-matrix)
- [ESP32-S3-ETH](#esp32-s3-eth)
- [ESP32-S3-Relay-6CH](#esp32-s3-relay-6ch)
- [ESP32-S3-A7670E-4G](#esp32-s3-a7670e-4g)
- [ESP32-S3-AUDIO-Board](#esp32-s3-audio-board)
- [ESP32-H2-Zero](#esp32-h2-zero)
- [e-Paper ESP32 Driver Board (Classic ESP32)](#e-paper-esp32-driver-board)

---

## ESP32-S3-DEV-KIT-N8R8

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-DEV-KIT-N8R8 |
| Module | ESP32-S3-WROOM-1 |
| Flash | 8 / 16 / 32 MB (configurable SKU) |
| PSRAM | 8 / 16 MB (configurable SKU) |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB |
| ROM | 384 KB |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Key Features

- Standard ESP32-S3 development board with full GPIO breakout
- Multiple Flash/PSRAM configurations available (N8R8 = 8MB Flash + 8MB PSRAM)
- All GPIOs exposed via dual pin headers
- Native USB and UART interfaces
- Breadboard-compatible form factor

### Available SKUs

| SKU Suffix | Flash | PSRAM |
|-----------|-------|-------|
| N8R8 | 8 MB | 8 MB |
| N16R8 | 16 MB | 8 MB |
| N32R8 | 32 MB | 8 MB |
| N8R16 | 8 MB | 16 MB |

### GPIO Layout

Standard ESP32-S3-WROOM-1 module pinout with 2x20 pin headers. All functional GPIOs exposed including:

- 2x SPI, 2x I2C, 3x UART, 2x I2S
- 20x ADC channels (12-bit)
- USB OTG (GPIO19 D-, GPIO20 D+)
- JTAG (GPIO39-42)

### Firmware Gotchas

- Default flash configuration varies by SKU -- verify with `esptool.py flash_id`.
- N32R8 variant: 32MB flash requires custom partition table.
- PSRAM mode: OPI (Octal) for R8/R16 variants -- enable in menuconfig.
- GPIO19/20 reserved for USB if using native USB mode.

---

## ESP32-S3-Pico

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Pico |
| Chip | ESP32-S3R2 |
| Flash | 16 MB |
| PSRAM | 2 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |
| Form Factor | Raspberry Pi Pico compatible |

### Key Features

- Pico-compatible form factor -- works with Pico HATs and add-ons
- Rich peripheral interfaces via Pico-style pin header
- Small and low-cost
- 2MB PSRAM (sufficient for most non-display applications)

### Interfaces

- 4x SPI, 2x I2C, 3x UART, 2x I2S, 2x ADC
- GPIO pins on Pico-compatible 2x20 header

### Firmware Gotchas

- Only 2MB PSRAM -- not suitable for frame buffers or large data structures.
- Pico-compatible pinout may differ from standard ESP32-S3 breakouts.
- Good entry-level S3 board for sensor and connectivity projects.

---

## ESP32-S3-Zero

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Zero |
| Chip | ESP32-S3FH4R2 |
| Flash | 4 MB (integrated) |
| PSRAM | 2 MB (integrated) |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |
| Dimensions | 18.0 x 23.5 mm |

### Key Features

- Ultra-compact form factor (18mm x 23.5mm)
- Castellated holes for direct PCB soldering
- Onboard WS2812 RGB LED on GPIO21
- Most GPIOs exposed despite tiny size

### GPIO Specifications

| Property | Value |
|----------|-------|
| Digital I/O pins | 24 (with interrupt support) |
| Analog inputs (ADC) | 2 |
| PWM-capable pins | 24 |
| SPI | 4 channels |
| I2C | 2 channels |
| UART | 3 channels |
| I2S | 2 channels |

### Safe GPIO Pins (General Use)

GPIO1, GPIO2, GPIO4, GPIO5, GPIO6, GPIO7, GPIO8, GPIO15, GPIO16, GPIO17, GPIO18

### Pins to Avoid / Reserved

| GPIO Range | Reserved For |
|-----------|-------------|
| GPIO9-14, GPIO38 | Flash memory data/control |
| GPIO39-42 | JTAG debugging |
| GPIO3, GPIO45 | Boot strapping pins |
| GPIO21 | WS2812 RGB LED |

### UART Pins

| GPIO | Function |
|------|----------|
| GPIO43 | TX (UART0) |
| GPIO44 | RX (UART0) |

### Flash Configuration

- Mode: QIO
- Max sketch size: 4096 KB (4 MB)
- Data storage: 512 KB

### Firmware Gotchas

- Only 4MB Flash -- plan partition table carefully. No room for large OTA partitions.
- 2MB PSRAM -- sufficient for WiFi + BLE but not large buffers.
- Castellated holes allow direct soldering to carrier PCBs.
- WS2812 on GPIO21 -- useful for status indication.
- Flash pins (GPIO9-14, GPIO38) must NOT be used for other purposes.

---

## ESP32-S3-Nano

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Nano |
| Chip | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| Form Factor | Arduino Nano compatible |

### Key Features

- Arduino Nano-compatible pin header layout
- Compatible with Nano shields and add-on boards
- Generous 16MB Flash + 8MB PSRAM

### Firmware Gotchas

- Nano-compatible pinout -- check pin mapping against standard Nano shields.
- More memory than Zero/Pico variants -- suitable for more demanding applications.

---

## ESP32-S3-Matrix

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Matrix |
| Chip | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Key Features

- Onboard **8x8 RGB LED Matrix** (WS2812-compatible, 64 LEDs)
- Onboard **QMI8658C** 9-axis attitude sensor (accelerometer + gyroscope + magnetometer)
- 2x 10-pin GPIO headers (20 GPIOs exposed)
- Compact form factor

### GPIO Headers

Two 10-pin headers exposing GPIOs, UART, and power signals.

### Sensors

| Component | Model | Interface |
|-----------|-------|-----------|
| Attitude Sensor | QMI8658C | I2C |

### Use Cases

- RGB LED effects and animations
- Motion-controlled LED displays
- Robotics orientation sensing
- IoT status indicators

### Firmware Gotchas

- 64 WS2812 LEDs driven by single GPIO pin (NeoPixel protocol).
- QMI8658C has 9-axis (vs QMI8658 6-axis on other boards) -- includes magnetometer.
- LED matrix consumes significant current at full brightness (~3.8A at max white).
- Use external 5V supply for full-brightness LED operation.
- Zephyr RTOS support available.

---

## ESP32-S3-ETH

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-ETH |
| Chip | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| Ethernet | 10/100 Mbps via RJ45 |
| USB Connector | USB Type-C |

### Key Features

- **10/100 Mbps Ethernet** via RJ45 jack (W5500 SPI Ethernet controller)
- Optional PoE (Power over Ethernet)
- 24-pin DVP camera interface (OV2640 / OV5640 compatible)
- TF card slot
- Pico-compatible interface for peripheral expansion
- Supports wired + wireless simultaneously

### Peripheral Interfaces

| Interface | Details |
|-----------|---------|
| Ethernet | 10/100 Mbps RJ45 (W5500) |
| Camera | 24-pin DVP (OV2640/OV5640) |
| TF Card | MicroSD slot |
| USB | Type-C (power + debug) |
| Pico Header | Pico HAT compatible expansion |

### Firmware Gotchas

- W5500 Ethernet controller uses SPI interface -- shares SPI bus resources.
- PoE option requires compatible PoE injector/switch (802.3af).
- Camera + Ethernet + WiFi simultaneously is feasible but check bandwidth.
- Pico-compatible header allows expansion with standard HATs.
- Good for IoT gateways requiring wired reliability.

---

## ESP32-S3-Relay-6CH

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Relay-6CH |
| Chip | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |

### Key Features

- **6 relay channels** with contact rating up to **10A 250VAC / 30VDC**
- Onboard RS485 interface for industrial communication
- 40-pin header compatible with Raspberry Pi Pico HATs
- Industrial-grade relay module

### Relay Specifications

| Property | Value |
|----------|-------|
| Channels | 6 |
| Max AC Load | 10A @ 250V AC |
| Max DC Load | 10A @ 30V DC |
| Control | WiFi / Bluetooth / RS485 |

### Interfaces

| Interface | Details |
|-----------|---------|
| RS485 | Onboard transceiver |
| Pico Header | 40-pin Pico HAT compatible |
| Relay Output | Screw terminals |

### Firmware Gotchas

- Relay switching creates electrical noise -- use proper debouncing in firmware.
- RS485 interface for integration with industrial control systems (Modbus, etc.).
- 10A relay rating -- suitable for home automation and light industrial control.
- Consider relay driver current when calculating total power budget.

---

## ESP32-S3-A7670E-4G

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-A7670E-4G-EN |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| Cellular | LTE Cat-1 / 2G (A7670E module) |

### Key Features

- **SIMCOM A7670E** LTE Cat-1 / 2G module
- Telephone calls, SMS, and data
- GNSS positioning (GPS/GLONASS/BeiDou)
- 24-pin DVP camera interface (OV2640/OV5640)
- SIM card slot
- Onboard antenna connectors

### Cellular Capabilities

| Feature | Details |
|---------|---------|
| LTE Cat-1 | Data up to 10 Mbps DL / 5 Mbps UL |
| 2G Fallback | GPRS/EDGE |
| Voice | Telephone calls via AT commands |
| SMS | Send/receive SMS |
| GNSS | GPS, GLONASS, BeiDou positioning |

### Firmware Gotchas

- A7670E module communicates via UART AT commands.
- SIM card required for cellular connectivity.
- Cellular module has separate power management -- check current draw.
- GNSS antenna required for positioning.
- Can use WiFi + Cellular simultaneously for redundant connectivity.
- Camera interface for remote imaging applications.

---

## ESP32-S3-AUDIO-Board

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-AUDIO-Board |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |

### Key Features

- AI smart speaker development board
- **Dual microphone array** with noise reduction and echo cancellation
- Onboard speaker driver
- Surround RGB LED lighting effects
- Designed for voice assistant applications

### Audio Specifications

| Component | Details |
|-----------|---------|
| Microphones | Dual digital MEMS array |
| Speaker | Onboard amplifier |
| Audio Processing | Noise reduction, echo cancellation |
| Audio Codec | I2S interface |
| LED | Surround RGB lighting |

### Interfaces

- I2S for audio I/O
- USB Type-C for power and programming
- GPIO expansion headers

### Firmware Gotchas

- Designed primarily for ESP-ADF (Audio Development Framework).
- Dual mic array enables beam-forming and wake word detection.
- Echo cancellation requires proper DSP configuration.
- RGB surround LEDs for voice assistant visual feedback (listening, thinking, speaking states).
- 8MB PSRAM sufficient for audio buffering and processing.

---

## ESP32-H2-Zero

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-H2-Zero |
| Chip | ESP32-H2 |
| Flash | 4 MB |
| PSRAM | None |
| Processor | RISC-V single-core, 96 MHz |
| WiFi | None |
| Bluetooth | BLE 5.0 |
| Zigbee/Thread | Yes (802.15.4) |
| USB Connector | USB Type-C |

### Key Features

- Ultra-low-power Zigbee/Thread/BLE device
- No WiFi -- designed for mesh networking
- Compact form factor with castellated holes
- Ideal for Matter endpoints

### Firmware Gotchas

- NO WiFi -- BLE + 802.15.4 only.
- 96 MHz single-core RISC-V -- limited processing power.
- Primary use: Zigbee endpoints, Thread devices, Matter accessories.
- 4MB Flash -- partition carefully for Zigbee/Thread stack.

---

## Board Selection Guide

| Use Case | Recommended Board | Key Reason |
|----------|------------------|-----------|
| General S3 prototyping | ESP32-S3-DEV-KIT-N8R8 | Full GPIO, configurable memory |
| Space-constrained IoT | ESP32-S3-Zero | 18x23.5mm, castellated |
| Pico HAT ecosystem | ESP32-S3-Pico | Pico-compatible headers |
| Arduino shield ecosystem | ESP32-S3-Nano | Nano-compatible headers |
| LED animations + motion | ESP32-S3-Matrix | 8x8 RGB + 9-axis IMU |
| Wired networking | ESP32-S3-ETH | Ethernet + optional PoE |
| Industrial relay control | ESP32-S3-Relay-6CH | 6x 10A relays + RS485 |
| Remote/cellular IoT | ESP32-S3-A7670E-4G | LTE + GPS + camera |
| Voice assistant | ESP32-S3-AUDIO-Board | Dual mic + speaker + RGB |
| Zigbee/Thread endpoint | ESP32-H2-Zero | Ultra-low-power mesh |
