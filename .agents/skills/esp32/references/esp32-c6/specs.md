# ESP32-C6 Specifications

## 1. Core Architecture
- **CPU:** Single-core RISC-V, 160 MHz.
- **Wireless:** Wi-Fi 6 (2.4GHz), BLE 5.3, Zigbee/Thread (802.15.4).
- **Application:** Next-gen Matter nodes, mesh networking, Wi-Fi 6 efficiency.

## 2. Memory
- **SRAM:** 512 KB internal.
- **ROM:** 320 KB.

## 3. Peripheral Mapping
- **GPIO Count:** 30.
- **ADC:** 7 channels.
- **DAC:** **None.**
- **Touch:** **None.**
- **Hardware PWM:** LEDC.
- **Connectivity:** 3x UART, 2x I2C, 1x SPI, USB-Serial/JTAG.

## 4. Hardware Safety & Constraints
- **ADC/WiFi Conflict:** **None.**
- **Flash Pins:** GPIO24-29.
- **Strapping Pins:** 4 (MTMS), 5 (MTDI), 8, 9, 15.
