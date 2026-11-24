# ESP32-C3 Specifications

## 1. Core Architecture
- **CPU:** Single-core RISC-V, 160 MHz.
- **Wireless:** 802.11 b/g/n (Wi-Fi 4), BLE 5.0.
- **Application:** Standard budget IoT node, secure connectivity.

## 2. Memory
- **SRAM:** 400 KB internal.
- **ROM:** 384 KB.

## 3. Peripheral Mapping
- **GPIO Count:** 22 (GPIO0-21).
- **ADC:** 6 channels.
- **DAC:** **None.**
- **Touch:** **None.**
- **Hardware PWM:** LEDC (6 channels).
- **I2S:** 1 controller.

## 4. Hardware Safety & Constraints
- **ADC/WiFi Conflict:** **None.** All ADC channels work while Wi-Fi is active.
- **Flash Pins:** GPIO12-17 (RESERVED).
- **Strapping Pins:** 2, 8, 9.
- **UART0:** Default on GPIO20/21.
