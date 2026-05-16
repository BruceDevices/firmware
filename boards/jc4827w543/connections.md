# JC4827W543 Board Connections

## Board Overview
- **Model**: JC4827W543 4.3" ESP32-S3
- **Display**: 4.3" TFT LCD 480x272 NV3041A
- **Touch**: XPT2046 Resistive
- **MCU**: ESP32-S3-WROOM-1-N4R8

## Pin Connections

### Display (NV3041A - 8-bit Parallel)
| Function | GPIO | Notes |
|----------|------|-------|
| TFT_CS | 45 | Chip Select |
| TFT_DC | 2 | Data/Command |
| TFT_RST | -1 | Not connected |
| TFT_BL | 1 | Backlight PWM |
| TFT_WR | 7 | Write strobe |
| TFT_RD | 8 | Read strobe |
| TFT_D0 | 39 | Data bit 0 |
| TFT_D1 | 40 | Data bit 1 |
| TFT_D2 | 41 | Data bit 2 |
| TFT_D3 | 48 | Data bit 3 |
| TFT_D4 | 21 | Data bit 4 |
| TFT_D5 | 47 | Data bit 5 |
| TFT_D6 | 46 | Data bit 6 |
| TFT_D7 | 6 | Data bit 7 |

### Touch (XPT2046 - SPI)
| Function | GPIO | Notes |
|----------|------|-------|
| TOUCH_SCK | 12 | Clock |
| TOUCH_MISO | 13 | MISO |
| TOUCH_MOSI | 11 | MOSI |
| TOUCH_CS | 38 | Chip Select |
| TOUCH_INT | 3 | Interrupt (not used) |

### Serial (UART0)
| Function | GPIO | Notes |
|----------|------|-------|
| TX | 43 | Serial TX |
| RX | 44 | Serial RX |

### I2C (for future GT911 capacitive touch)
| Function | GPIO | Notes |
|----------|------|-------|
| SDA | 8 | Data |
| SCL | 4 | Clock |

### Buttons (1 button for OK, touchscreen primary)
| Function | GPIO | Notes |
|----------|------|-------|
| SEL_BTN | 14 | Select/OK button |

### SPI Bus (external modules)
| Function | GPIO | Notes |
|----------|------|-------|
| SPI_SCK | 47 | Clock |
| SPI_MOSI | 21 | MOSI |
| SPI_MISO | 48 | MISO |
| SPI_SS | 45 | Chip select |

### External Radio Pins (CC1101/NRF24)
| Function | GPIO | Notes |
|----------|------|-------|
| CC1101_GDO0 | 10 | CC1101 TX/RX |
| CC1101_SS | 9 | CC1101 CS |
| NRF24_CE | 6 | NRF24 CE |
| NRF24_SS | 7 | NRF24 CS |

### Power
| Function | GPIO | Notes |
|----------|------|-------|
| PIN_POWER_ON | 5 | Power enable |

## Build Command
```bash
pio run -e jc4827w543
```

## Upload
```bash
pio run -e jc4827w543 --target upload --upload-port /dev/ttyUSB0
```