# ESP32-S2 Specifications

## 1. Core Architecture
- **CPU:** Single-core Xtensa® LX7, 240 MHz.
- **Wireless:** 802.11 b/g/n (Wi-Fi 4). **Note: No Bluetooth support.**
- **Application:** Ultra-low power designs and projects requiring native USB OTG/HID.

## 2. Memory & Storage
- **SRAM:** 320 KB internal.
- **ROM:** 128 KB.
- **PSRAM:** Supports external PSRAM via SPI/QPI.

## 3. Peripheral Mapping
- **GPIO Count:** 43.
- **ADC:** 20 channels.
- **DAC:** 2 channels.
- **Touch:** 14 capacitive touch channels.
- **USB:** Native USB OTG (GPIO19/20).
- **Hardware PWM:** LEDC.
- **I2S:** For digital audio.

## 4. Hardware Safety & Constraints
- **ADC2/WiFi Conflict:** ADC2 readings are invalid when Wi-Fi is active.
- **Flash Pins:** GPIO26-32.
- **Strapping Pins:** 0, 45, 46.
- **USB Pins:** Reserve GPIO19/20 if using native USB functionality.
