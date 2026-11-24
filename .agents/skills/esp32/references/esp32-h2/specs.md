# ESP32-H2 Specifications

## 1. Core Architecture
- **CPU:** Single-core RISC-V, 96 MHz.
- **Wireless:** BLE 5.0, Zigbee/Thread (802.15.4). **Note: No Wi-Fi support.**
- **Application:** Smart home devices, Matter over Thread, low-power sensors.

## 2. Memory
- **SRAM:** 320 KB internal.
- **ROM:** 128 KB.

## 3. Peripheral Mapping
- **GPIO Count:** ~19-25.
- **Hardware PWM:** LEDC.
- **Communication:** Standard I2C, SPI, UART.

## 4. Hardware Safety & Constraints
- **Wireless Usage:** Primarily for mesh and point-to-point BLE.
- **Antenna:** Typically optimized for 2.4GHz 802.15.4.
