# LilyGO T-Watch Ultra

Official references:
- https://wiki.lilygo.cc/products/t-watch-series/t-watch-ultra/
- Schematic: `T-Watch Ultra V1.0 SCH 25-07-24.pdf` in the [LilyGoLib repo](https://github.com/Xinyuan-LilyGO/LilyGoLib)

All pins follow the official LilyGO pin mapping table.

## MCU / memory

| What | Value |
|---|---|
| MCU | ESP32-S3 @ 240 MHz |
| Flash | 16 MB QSPI |
| PSRAM | 8 MB QSPI (`qio_qspi`) |
| USB | Native USB-OTG (CDC on boot, BadUSB/HID) |

## Display (CO5300 AMOLED 410x502, QSPI)

| Pin | GPIO |
|---|---|
| CS | 41 |
| SCK | 40 |
| D0 | 38 |
| D1 | 39 |
| D2 | 42 |
| D3 | 45 |
| RST | 37 |
| TE | 6 (unused by the driver) |

Power: AXP2101 ALDO2 (3.3 V), enable: XL9555 P07 (`EXPANDS_DISP_EN`).
Brightness: CO5300 register (no backlight pin) — `_setBrightness()`.

## Touch (CST9217, I2C 0x1A)

| Pin | GPIO |
|---|---|
| SDA / SCL | 3 / 2 (system I2C) |
| INT | 12 |
| RST | XL9555 P08 (`EXPANDS_TOUCH_RST`) |

## System I2C bus (SDA=3, SCL=2)

| Device | Address |
|---|---|
| AXP2101 PMU | 0x34 |
| XL9555 IO expander | 0x20 |
| PCF85063A RTC | 0x51 |
| CST9217 touch | 0x1A |
| DRV2605 haptic | 0x5A |
| BHI260AP sensor | 0x28 (not used by Bruce) |

## Shared SPI bus (SCK=35, MISO=33, MOSI=34)

| Device | CS | Notes |
|---|---|---|
| microSD | 21 | power: ALDO1; detect: XL9555 P10 |
| ST25R3916 NFC | 4 | IRQ: GPIO5; power: DLDO1 |
| SX1262 LoRa | 36 | RST: 47, BUSY: 48, DIO1/IRQ: 14; power: ALDO3; antenna switch: XL9555 P11 (HIGH = built-in) |

## Other peripherals

| Device | Pins |
|---|---|
| GNSS MIA-M10Q | TX->43 (ESP RX), RX<-44 (ESP TX), PPS 13; 38400 baud; power: BLDO1 |
| Speaker MAX98357A | BCLK 9, WCLK 10, DOUT 11; power: BLDO2 |
| Mic T3902 (PDM) | CLK 17, DATA 18 |
| Buttons | POWER (AXP2101, hold 6 s = off) + BOOT (GPIO0) |

## AXP2101 rail map (factory)

| Rail | Voltage | Powers |
|---|---|---|
| ALDO1 | 3.3 V | microSD |
| ALDO2 | 3.3 V | Display |
| ALDO3 | 3.3 V | LoRa radio |
| ALDO4 | 1.8 V | BHI260AP sensor |
| BLDO1 | 3.3 V | GNSS |
| BLDO2 | 3.3 V | Speaker amp |
| DLDO1 | 3.3 V | NFC |
