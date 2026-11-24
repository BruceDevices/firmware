# ESP32-S3 Specifications

## 1. Core Architecture
- **CPU:** Dual-core Xtensa® LX7, 240 MHz.
- **Wireless:** 802.11 b/g/n (Wi-Fi 4), BLE 5.0 (Mesh support).
- **Acceleration:** AI Vector instructions for accelerated ML and complex GUI rendering.
- **Application:** High-performance IoT, AI on the edge, and multimedia GUIs.

## 2. Memory & Storage
- **SRAM:** 512 KB internal.
- **PSRAM:** Support for Octal SPI PSRAM for heavy workloads.

## 3. Peripheral Mapping
- **GPIO Count:** 45.
- **ADC:** 20 channels.
- **DAC:** **None.**
- **Touch:** 14 capacitive touch channels.
- **USB:** Native USB OTG / USB-Serial/JTAG (GPIO19/20).
- **Hardware PWM:** LEDC and MCPWM (Advanced motor control).
- **LCD Interface:** Supports RGB, 8080, and I80 interfaces.

## 4. Hardware Safety & Constraints
- **ADC2/WiFi Conflict:** ADC2 readings are invalid when Wi-Fi is active.
- **Flash Pins:** GPIO26-32.
- **Strapping Pins:** 0, 3, 45, 46.
- **Octal PSRAM:** May reserve additional pins (check specific module).
