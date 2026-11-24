# ESP32-C5 Specifications

## 1. Core Architecture
- **CPU:** Single-core RISC-V, 240 MHz.
- **Wireless:** Dual-band Wi-Fi 6 (2.4/5GHz), BLE 5.0, Zigbee/Thread (802.15.4).
- **Application:** High-speed IoT connectivity, dual-band Wi-Fi requirements.

## 2. Memory
- **SRAM:** 400 KB internal.

## 3. Peripheral Mapping
- **GPIO Count:** ~20-30 (refer to specific module datasheet).
- **Hardware PWM:** LEDC.
- **Advanced Connectivity:** Support for Wi-Fi 6 features (TWT, etc.).

## 4. Hardware Safety & Constraints
- **ADC/WiFi Conflict:** **None.**
- **Dual-Band Usage:** Respect antenna design for 5GHz operations.
