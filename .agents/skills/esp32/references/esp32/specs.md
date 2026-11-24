# ESP32 (Original) Specifications

## 1. Core Architecture
- **CPU:** Dual-core Xtensa® LX6, 240 MHz.
- **Wireless:** 802.11 b/g/n (Wi-Fi 4), Bluetooth Classic (v4.2), BLE (v4.2).
- **Application:** Legacy projects requiring Bluetooth Classic support.

## 2. Memory & Storage
- **SRAM:** 520 KB internal.
- **DRAM:** Data RAM, single-byte accessible.
- **IRAM:** Instruction RAM, code for interrupts/flash writes must reside here.
- **PSRAM:** Supports external Pseudo-Static RAM (standard on WROVER modules).

## 3. Peripheral Mapping
- **GPIO Count:** 34 (GPIO0 to GPIO39).
- **ADC:** 18 channels (ADC1: 8, ADC2: 10).
- **DAC:** 2 channels (GPIO25, 26).
- **Touch:** 10 capacitive touch channels.
- **Hardware PWM:** LEDC (all output-capable GPIOs).
- **UART:** 3 controllers (UART0, UART1, UART2).
- **I2C:** 2 controllers (any GPIO via matrix).
- **SPI:** 2 user-accessible (VSPI, HSPI).

## 4. Hardware Safety & Constraints
- **Flash Voltage Trap:** GPIO12 (MTDI). If HIGH at boot, sets flash to 1.8V — bricking risk for 3.3V modules.
- **ADC2/WiFi Conflict:** ADC2 cannot be used when Wi-Fi or Bluetooth is active. Use ADC1 (GPIO32-39).
- **Input-Only Pins:** GPIO34, 35, 36, 39. No output drivers, no internal pulls.
- **Flash Pins:** GPIO6-11 (RESERVED — Never use).
- **PSRAM Pins:** GPIO16, 17 (RESERVED on WROVER modules).
- **Strapping Pins:** 0, 2, 5, 12, 15.
