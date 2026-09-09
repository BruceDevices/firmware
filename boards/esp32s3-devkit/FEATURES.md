# ESP32-S3 DevKit - Bruce Firmware Feature Support

## Board Overview
**ESP32-S3 DevKit** with integrated hardware for Bruce offensive security toolkit.

### Display & Interface
- **Display**: SH1106 128x64 I2C OLED (GPIO 8=SDA, GPIO 9=SCL)
- **Buttons**: 6 Navigation buttons (UP, DOWN, LEFT, RIGHT, SELECT, BACK)
- **Serial**: USB CDC-ACM at 115200 baud

---

## Available Features

### ✅ WiFi Attacks (Built-in ESP32-S3)
- [x] WiFi Scanning
- [x] Beacon Spam
- [x] Target Deauth Attacks
- [x] Evil Portal + Deauth
- [x] Deauth Flooding
- [x] WiFi Access Point Mode
- [x] Wardriving (with GPS module)
- [x] Responder
- [x] ARP Spoofing/Poisoning
- [x] Brucegotchi (Pwnagotchi friend)
- [x] WireGuard Tunneling

### ✅ BLE/Bluetooth Attacks (Built-in ESP32-S3)
- [x] BLE Scanning
- [x] Bad BLE (Ducky Scripts)
- [x] BLE Spam (iOS, Windows, Samsung, Android)
- [x] BLE Beacon Attacks

### ✅ RF/Sub-GHz Attacks (CC1101 Module - SPI)
**PIN Configuration**:
- `CC1101_SS_PIN = GPIO 10`
- `CC1101_SCK_PIN = GPIO 12`
- `CC1101_MOSI_PIN = GPIO 11`
- `CC1101_MISO_PIN = GPIO 13`
- `CC1101_GDO0_PIN = GPIO 1`

**Features**:
- [x] Sub-GHz Scanning
- [x] Signal Replay
- [x] Custom Frequency Payload
- [x] RF Jamming (Full & Intermittent)
- [x] Spectrum Analysis
- [x] Signal Capture & Decode

### ✅ NRF24 2.4GHz Attacks (NRF24 Module - SPI)
**PIN Configuration**:
- `NRF24_SS_PIN = GPIO 10` (shared)
- `NRF24_CE_PIN = GPIO 2`
- `NRF24_SCK_PIN = GPIO 12`
- `NRF24_MOSI_PIN = GPIO 11`
- `NRF24_MISO_PIN = GPIO 13`

**Features**:
- [x] NRF24 Scanning
- [x] 2.4GHz Spectrum
- [x] Mousejacking
- [x] Keyboard Hijacking

### ✅ RFID/NFC (PN532 Module - I2C/SPI)
**PIN Configuration** (I2C Mode - Default):
- `PN532_SDA = GPIO 8` (shared with OLED)
- `PN532_SCL = GPIO 9` (shared with OLED)

**Features**:
- [x] RFID Tag Reading
- [x] RFID Tag Cloning
- [x] NFC Emulation
- [x] NDEF Record Writing
- [x] 125kHz Reading
- [x] Chameleon Mode
- [x] Amiibolink

### ✅ Infrared (IR TX/RX)
**PIN Configuration**:
- `IR_TX_PIN = GPIO 3`
- `IR_RX_PIN = GPIO 2`
- `IR_LED = GPIO 4` (indicator)

**Features**:
- [x] IR Receiver
- [x] TV-B-Gone (Power off any TV)
- [x] Custom IR Codes (NEC, RC5, RC6, Samsung, etc.)
- [x] Signal Replay

### ✅ Storage & File Management
- [x] LittleFS (Internal Flash)
- [x] SD Card Support (SPI via GPIO 10-13)
- [x] File Browser
- [x] Script Loading

### ✅ Other Features
- [x] QR Code Generation
- [x] JavaScript Interpreter
- [x] WebUI Control
- [x] Wardriving with Wigle Logging
- [x] Bad USB (via USB CDC-ACM)
- [x] Spectrum Analysis (Microphone)
- [x] RTC Support (via I2C)
- [x] GPS Support (via Serial UART)

---

## Hardware Connections Summary

### Power & USB
- **5V**: USB Port
- **GND**: USB Port & All modules

### Display (SH1106 OLED 128x64)
| Pin | GPIO | Signal |
|-----|------|--------|
| VCC | 3.3V | Power  |
| GND | GND  | Ground |
| SDA | GPIO 8 | I2C Data |
| SCL | GPIO 9 | I2C Clock |

### Buttons
| Button | GPIO | Active |
|--------|------|--------|
| UP     | 35   | LOW    |
| DOWN   | 36   | LOW    |
| LEFT   | 37   | LOW    |
| RIGHT  | 38   | LOW    |
| SELECT | 39   | LOW    |
| BACK   | 40   | LOW    |

### SPI Bus (Shared: CC1101, NRF24, SD Card)
| Signal | GPIO |
|--------|------|
| SCLK   | 12   |
| MOSI   | 11   |
| MISO   | 13   |
| CS (RFID/NFC) | 10 |
| CS (SD Card)  | 15 |
| NRF24 CE | 2  |

### Infrared
| Function | GPIO |
|----------|------|
| TX | 3 |
| RX | 2 |
| Status LED | 4 |

### Serial/UART (GPS Optional)
| Signal | GPIO |
|--------|------|
| TX | 43 |
| RX | 44 |

---

## Installation Instructions

### 1. Arduino IDE Setup
```bash
# Install ESP32 core version 3.x+
# Add board manager URL:
# https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

# Install required libraries:
# - Adafruit SH1106
# - SmartRC CC1101 Driver (for RF)
# - RF24 (for NRF24)
# - RadioLib (alternative RF library)
# - Adafruit PN532 (for RFID/NFC)
```

### 2. PlatformIO Setup
```bash
# platformio.ini already configured for esp32s3-devkit
pio run -e esp32s3-devkit -t upload
```

### 3. Hardware Assembly
1. Connect OLED display to GPIO 8 (SDA) and GPIO 9 (SCL)
2. Connect buttons to GPIO 35-40
3. Connect RF modules via SPI bus (GPIO 11-13 + CS pins)
4. Connect SD card to SPI bus with CS on GPIO 15
5. Connect IR TX to GPIO 3, RX to GPIO 2

---

## Troubleshooting

### OLED Not Displaying
- Check I2C address (default 0x3C)
- Verify power supply (3.3V)
- Check SDA/SCL connections (GPIO 8/9)

### RF Modules Not Detected
- Verify SPI connections (GPIO 11-13)
- Check CS pin (GPIO 10 for CC1101/NRF24)
- Verify chip select logic level

### Button Input Not Working
- Check GPIO pins (35-40)
- Verify pullup resistors or use INPUT_PULLUP mode
- Check button contacts (GND when pressed)

### SD Card Not Mounting
- Format as FAT32
- Check CS pin (GPIO 15)
- Verify SPI bus connections

---

## Additional Resources

- **Wiki**: https://wiki.bruce.computer
- **GitHub**: https://github.com/a77336402-rgb/firmware
- **Discord**: https://discord.gg/WJ9XF9czVT

---

## Notes for Developers

- **Shared I2C Bus**: OLED and PN532 share GPIO 8/9 - ensure proper address configuration
- **Shared SPI Bus**: CC1101, NRF24, and SD Card use same SCLK/MOSI/MISO but different CS pins
- **Button Debouncing**: Implemented in ISR handlers with configurable delay
- **Power Budget**: All modules draw from USB - ensure adequate supply (2A+ recommended)

