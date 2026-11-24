# ESP32-C2 Specifications

## 1. Core Architecture
- **CPU:** Single-core RISC-V, 120 MHz.
- **Wireless:** 802.11 b/g/n (Wi-Fi 4), BLE 5.0.
- **Application:** Cost-sensitive IoT nodes, direct ESP8266 replacement.

## 2. Memory
- **SRAM:** 272 KB internal.
- **ROM:** 576 KB.

## 3. Peripheral Mapping
- **ADC:** 5 channels.
- **Hardware PWM:** LEDC.
- **I2C/SPI/UART:** Standard connectivity.

## 4. Hardware Safety & Constraints
- **Flash Pins:** Integrated flash in some modules.
- **ADC/WiFi Conflict:** Limited impact due to simplified architecture.
