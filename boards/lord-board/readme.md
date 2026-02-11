# LORD Board
## Little Offensive Reconnaissance Device

<div style="display: flex; gap: 10px; flex-wrap: wrap;">
  <img src="https://i.ibb.co/yndXB9nd/IMG-1926.avif" alt="LORD Board" width="200" />
  <img src="https://i.ibb.co/Jj1kP61q/IMG-1920.avif" alt="LORD Board" width="200" />
  <img src="https://i.ibb.co/cScxT1rp/IMG-1923.jpg" alt="LORD Board" width="200" />
  <img src="https://i.ibb.co/MDq8Pq67/IMG-1925.jpg" alt="LORD Board" width="200" />

</div>

### 🎯 Project Overview

The **LORD Board** (Little Offensive Reconnaissance Device) is a powerful BRuce  ESP32-S3 based multi-tool platform designed for security research. Built on the ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM).

---

## 📦 Required Components

### Core Components
- **ESP32-S3-WROOM-1-N16R8**
- **ILI9341 2.8" Touch TFT Display**
### RF Modules
- **CC1101 transceiver**
- **nRF24L01+**
- **Antennas for CC1101 and nRF24L01 frequency**

### Sensors & Peripherals
- **PN532 NFC module**
- **NEO-6M GPS module**
- **IR LED**
- **IR Receiver**


## 🔌 Pin Configuration

### (Display, Touch, SD Card) Connections
| Function | GPIO | Description |
|----------|------|-------------|
| TFT/SD_CARD/Touch_MISO | 11 | TFT/Touch/Sd Card Master In Slave Out |
| TFT/SD_CARD/Touch_MOSI | 12 | TFT/Touch/Sd Card Master Out Slave In |
| TFT/SD_CARD/Touch_SCK | 13 | TFT/Touch/Sd Card Clock |
| TFT_CS | 8 | Display Chip Select |
| TFT_DC | 3 | Display Data/Command |
| TFT_RST | 9 | Display Reset |
| TFT_BL | 10 | Backlight (PWM) |
| TOUCH_CS | 38 | Touch Chip Select |
| TOUCH_IRQ | 39 | Touch Interrupt |
| SD_CS | 21 | SD Card Chip Select |

### (RF Modules) Connections
| Function | GPIO | Description |
|----------|------|-------------|
| CC1101/nrf_MISO | 16 | CC1101/nrf Master In Slave Out |
| CC1101/nrf_MOSI | 17 | CC1101/nrf Master Out Slave In |
| CC1101/nrf_SCK | 18 | CC1101/nrf Clock |
| CC1101_CS | 15 | CC1101 Chip Select |
| CC1101_GDO0 | 7 | CC1101 Data Output 0 |
| CC1101_GDO2 | 6 | CC1101 Data Output 2 |
| NRF24_CS | 5 | nRF24 Chip Select |
| NRF24_CE | 4 | nRF24 Chip Enable |

### (NFC) Connections
| Function | GPIO | Description |
|----------|------|-------------|
| SDA | 45 | I2C Data |
| SCL | 42 | I2C Clock |

### (GPS) Connections
| Function | GPIO | Description |
|----------|------|-------------|
| GPS_TX (UART1) | 41 | GPS Module TX |
| GPS_RX (UART1) | 40 | GPS Module RX |

### Other Peripherals Connections
| Function | GPIO | Description |
|----------|------|-------------|
| IR_TX | 2 | Infrared Transmitter |
| IR_RX | 1 | Infrared Receiver |

### VCC and GND Connections

Connect all VCC pins to 3.3V and all GND pins to GND.


---



#
## 🔌 Connection Diagram

> **📎 See attached circuit diagram:** `circuit_image.png`


---


## 📋 Hardware Specifications

### Main Controller
| Component | Specification |
|-----------|---------------|
| MCU | ESP32-S3-WROOM-1-N16R8 |
| Flash | 16 MB |
| PSRAM | 8 MB (Octal SPI) |
| Clock | 240 MHz (Dual Core) |
| WiFi | 2.4 GHz 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB | Native USB (CDC + HID) |

### Display & Touch
| Component | Specification |
|-----------|---------------|
| Display | ILI9341 2.8" TFT |
| Resolution | 320x240 pixels |
| Touch | XPT2046 Resistive |
| Interface | SPI (Shared VSPI) |
| Backlight | PWM controlled (GPIO 10) |

### RF Modules
| Module | Frequency | Interface | Use Case |
|--------|-----------|-----------|----------|
| CC1101 | 300-928 MHz | SPI (HSPI) | Sub-GHz analysis, garage doors, weather stations |
| nRF24L01+ | 2.4 GHz | SPI (HSPI) | Wireless keyboards/mice, IoT devices |

### Additional Modules
| Module | Interface | GPIO | Purpose |
|--------|-----------|------|---------|
| PN532 NFC | I2C | SDA:45, SCL:42 | NFC/RFID read/write |
| GPS NEO-6M | UART | TX:41, RX:40 | Location tracking |
| IR TX/RX | GPIO | TX:2, RX:1 | Infrared remote control |


---


## 📞 Support & Contact

- **Issues**: [GitHub Issues](https://github.com/BruceDevices/firmware/issues)
- **Contritubed**: [Sivabala](https://github.com/sivabala21)

---

