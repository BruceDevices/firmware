# JC4827W543 4.3" ESP32-S3 Board

Board definition for JC4827W543 4.3" TFT display with ESP32-S3 (WROOM-1-N4R8)

## Hardware Specifications

- **Display**: 4.3" TFT LCD, 480x272 resolution, NV3041A driver
- **Touch**: XPT2046 resistive touch controller
- **MCU**: ESP32-S3-WROOM-1-N4R8 (dual-core, 240MHz)
- **RAM**: 520KB + 8MB PSRAM
- **Flash**: 4MB QSPI

## Pinout

| Function | GPIO |
|----------|------|
| TFT CS | 45 |
| TFT SCK | 47 |
| TFT D0 | 21 |
| TFT D1 | 48 |
| TFT D2 | 40 |
| TFT D3 | 39 |
| TFT BL | 1 |
| Touch SCK | 12 |
| Touch MISO | 13 |
| Touch MOSI | 11 |
| Touch CS | 38 |
| Touch INT | 3 |

## Build Target

Use `env:jc4827w543` in platformio.ini