# Waveshare I2C OLED/LCD Display Modules

> Comprehensive reference for Waveshare standalone I2C and SPI/I2C OLED display modules.
> Last updated: 2026-04-13

## Quick Reference Table

| Model | Size | Resolution | Controller | Interface | Colors | Voltage | Module Size |
|-------|------|-----------|------------|-----------|--------|---------|-------------|
| 0.91inch OLED Module | 0.91" | 128x32 | SSD1306 | I2C only | Mono (White/Blue) | 3.3V/5V | ~25 x 10 mm |
| 0.96inch OLED Module (C/D) | 0.96" | 128x64 | SSD1315 | SPI / I2C | Mono (White/Blue/Yellow-Blue) | 3.3V/5V | 26.0 x 26.0 mm |
| 0.96inch RGB OLED Module | 0.96" | 64x128 | SSD1357 | SPI only | 65K RGB | 3.3V/5V | 32.5 x 26.0 mm |
| 1.27inch RGB OLED Module | 1.27" | 128x96 | SSD1351 | SPI only | 262K RGB | 3.3V/5V | 42.2 x 29.0 mm |
| 1.3inch OLED (B) | 1.3" | 128x64 | SH1106 | SPI / I2C | Mono (Blue) | 3.3V | ~35 x 33 mm |
| 1.3inch OLED Module (C) | 1.3" | 64x128 | SH1107 | SPI / I2C | Mono (B/W) | 3.3V/5V | ~33 x 33 mm |
| 1.32inch OLED Module | 1.32" | 128x96 | SSD1327 | SPI / I2C | 16 Gray Scale | 3.3V/5V | 32.6 x 28.3 mm |
| 1.5inch OLED Module | 1.5" | 128x128 | SSD1327 | SPI / I2C | 16 Gray Scale | 3.3V/5V | ~48 x 35 mm |
| 1.5inch RGB OLED Module | 1.5" | 128x128 | SSD1351 | SPI only | 65K RGB | 3.3V/5V | 44.5 x 37.0 mm |
| 1.54inch OLED Module | 1.54" | 128x64 | SSD1309 | SPI / I2C | Mono (White/Blue) | 3.3V/5V | 43.0 x 37.5 mm |
| 2.42inch OLED Module | 2.42" | 128x64 | SSD1309 | SPI / I2C | Mono (White/Yellow) | 3.3V/5V | 61.5 x 39.5 mm |

---

## Detailed Specifications

### 0.91inch OLED Module (I2C Only)

- **Product URL**: https://www.waveshare.com/0.91inch-oled-module.htm
- **Wiki**: https://www.waveshare.com/wiki/0.91inch_OLED_Module
- **Controller**: SSD1306 (128x64 internal, uses 128x32 window)
- **Resolution**: 128 x 32 pixels
- **Display Type**: OLED
- **Colors**: Monochrome (White or Blue)
- **Interface**: I2C only (4 pins)
- **I2C Address**: 0x3C
- **Operating Voltage**: 3.3V / 5V
- **Operating Temp**: -40 to 85 C
- **Touch**: None

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| GND | Ground | Power ground |
| VCC | Power | 3.3V-5V DC input |
| SCL | I2C SCL | I2C clock line |
| SDA | I2C SDA | I2C data line |

**Note**: The SSD1306 supports 8-bit 8080, 6800, 3-wire SPI, 4-wire SPI, and I2C modes, but this module only pins out I2C to save IO resources. Only 4 pins total.

---

### 0.96inch OLED Module (C/D variants)

- **Product URL**: https://www.waveshare.com/0.96inch-oled-module-c.htm
- **Wiki**: https://www.waveshare.com/wiki/0.96inch_OLED_Module
- **Controller**: SSD1315
- **Resolution**: 128 x 64 pixels
- **Display Type**: OLED
- **Colors**: Monochrome (White / Blue / Yellow-Blue variants)
- **Interface**: 4-wire SPI (default) / I2C (selectable via resistor)
- **I2C Address**: 0x3C (DC=Low) or 0x3D (DC=High)
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 21.74 x 11.18 mm
- **Module Size**: 26.0 x 26.0 mm
- **Power**: ~25 mA fully on, ~1.5 mA fully off (at 3.3V)
- **Touch**: None
- **Connector**: GH1.25 7PIN

#### Pinout (SPI mode - default)

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI / I2C SDA | Data input |
| CLK | SPI SCK / I2C SCL | Clock |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command (I2C: address select) |
| RES | Reset | Reset, low active |

#### Interface Selection

Mode is selected via BS1/BS2 solder jumpers on the back:

| Mode | BS1 | BS2 |
|------|-----|-----|
| 4-wire SPI (default) | GND | GND |
| I2C | VCC (via pull-up) | GND |

---

### 0.96inch RGB OLED Module

- **Product URL**: https://www.waveshare.com/0.96inch-rgb-oled-module.htm
- **Wiki**: https://www.waveshare.com/wiki/0.96inch_RGB_OLED_Module
- **Controller**: SSD1357
- **Resolution**: 64 x 128 pixels
- **Display Type**: OLED (RGB)
- **Colors**: 65K (RGB565)
- **Interface**: 3/4-wire SPI only
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 10.86 x 21.74 mm
- **Pixel Size**: 0.0327(W) x 0.146(H) mm
- **Module Size**: 32.5 x 26.0 mm
- **Power**: ~25 mA fully on, ~1.5 mA fully off
- **Touch**: None
- **Connector**: GH1.25 7PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Data input |
| CLK | SPI SCK | Clock |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RES | Reset | Reset, low active |

---

### 1.27inch RGB OLED Module

- **Product URL**: https://www.waveshare.com/1.27inch-rgb-oled-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.27inch_RGB_OLED_Module
- **Controller**: SSD1351 (128x128x18bit SRAM buffer)
- **Resolution**: 128 x 96 pixels
- **Display Type**: OLED (RGB)
- **Colors**: 262K (RGB666) / 65K (RGB565)
- **Interface**: 3/4-wire SPI only
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 25.71 x 19.28 mm
- **Module Size**: 42.2 x 29.0 mm
- **Touch**: None
- **Connector**: GH1.25 7PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Data input |
| CLK | SPI SCK | Clock |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RES | Reset | Reset, low active |

---

### 1.3inch OLED (B) - SH1106

- **Product URL**: https://www.waveshare.com/1.3inch-oled-b.htm
- **Wiki**: https://www.waveshare.com/wiki/1.3inch_OLED_(B)
- **Controller**: SH1106
- **Resolution**: 128 x 64 pixels
- **Display Type**: OLED
- **Colors**: Monochrome (Blue)
- **Interface**: 3-wire SPI, 4-wire SPI, I2C (selectable)
- **Operating Voltage**: 3.3V
- **Viewing Angle**: >160 degrees
- **Operating Temp**: -30 to 70 C
- **Storage Temp**: -30 to 80 C
- **Touch**: None

---

### 1.3inch OLED Module (C) - SH1107

- **Product URL**: https://www.waveshare.com/1.3inch-oled-module-c.htm
- **Wiki**: https://www.waveshare.com/wiki/1.3inch_OLED_Module_(C)
- **Controller**: SH1107 (128x128 SRAM, supports 128x128 max)
- **Resolution**: 64 x 128 pixels
- **Display Type**: OLED
- **Colors**: Monochrome (Black/White)
- **Interface**: 4-wire SPI (default) / I2C (selectable)
- **Operating Voltage**: 3.3V / 5V
- **Touch**: None
- **Connector**: GH1.25 7PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI / I2C SDA | Data input |
| CLK | SPI SCK / I2C SCL | Clock |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RES | Reset | Reset, low active |

---

### 1.32inch OLED Module - SSD1327

- **Product URL**: https://www.waveshare.com/1.32inch-oled-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.32inch_OLED_Module
- **Controller**: SSD1327
- **Resolution**: 128 x 96 pixels
- **Display Type**: OLED
- **Colors**: 16 Gray Scale (White)
- **Interface**: 4-wire SPI (default) / I2C (selectable via resistor)
- **I2C Address**: 0x3C (DC=Low) or 0x3D (DC=High)
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 26.86 x 20.14 mm
- **Module Size**: 32.6 x 28.3 mm
- **Viewing Angle**: >160 degrees
- **Operating Temp**: -20 to 70 C
- **Storage Temp**: -30 to 80 C
- **Touch**: None
- **Connector**: GH1.25 7PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI / I2C SDA | Data input |
| CLK | SPI SCK / I2C SCL | Clock |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command (I2C: address select) |
| RES | Reset | Reset, low active |

#### Interface Selection

| Mode | BS1 | BS2 |
|------|-----|-----|
| 4-wire SPI (default) | GND via R1 | GND |
| I2C | VCC via pull-up | GND |

---

### 1.5inch OLED Module - SSD1327 (Grayscale)

- **Product URL**: https://www.waveshare.com/1.5inch-oled-module.htm
- **Controller**: SSD1327
- **Resolution**: 128 x 128 pixels
- **Display Type**: OLED
- **Colors**: 16 Gray Scale
- **Interface**: 4-wire SPI / I2C
- **Operating Voltage**: 3.3V / 5V
- **Touch**: None

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI / I2C SDA | Data input |
| CLK | SPI SCK / I2C SCL | Clock |
| CS | SPI CS | Chip select |
| DC | Data/Command | Data/Command select |
| RES | Reset | Reset, low active |

---

### 1.5inch RGB OLED Module - SSD1351

- **Product URL**: https://www.waveshare.com/1.5inch-rgb-oled-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.5inch_RGB_OLED_Module
- **Controller**: SSD1351 (128x128x18bit SRAM buffer)
- **Resolution**: 128 x 128 pixels
- **Display Type**: OLED (RGB)
- **Colors**: 65K (RGB565) / 262K (RGB666)
- **Interface**: 3/4-wire SPI only
- **Operating Voltage**: 3.3V / 5V
- **Viewing Angle**: >160 degrees
- **Module Size**: 44.5 x 37.0 mm
- **Touch**: None

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Data input |
| CLK | SPI SCK | Clock |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RES | Reset | Reset, low active |

---

### 1.54inch OLED Module - SSD1309

- **Product URL**: https://www.waveshare.com/1.54inch-oled-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.54inch_OLED_Module
- **Controller**: SSD1309
- **Resolution**: 128 x 64 pixels
- **Display Type**: OLED
- **Colors**: Monochrome (White or Blue)
- **Interface**: 4-wire SPI (default) / I2C (selectable)
- **I2C Address**: 0x3C (DC=Low) or 0x3D (DC=High)
- **Operating Voltage**: 3.3V / 5V
- **Pixel Size**: 0.25 x 0.25 mm
- **Display Size**: 35.05 x 17.52 mm
- **Module Size**: 43.00 x 37.50 mm
- **Touch**: None
- **Connector**: GH1.25 7PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI / I2C SDA | Data input |
| CLK | SPI SCK / I2C SCL | Clock |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RES | Reset | Reset, low active |

---

### 2.42inch OLED Module - SSD1309

- **Product URL**: https://www.waveshare.com/2.42inch-oled-module.htm
- **Wiki**: https://www.waveshare.com/wiki/2.42inch_OLED_Module
- **Controller**: SSD1309
- **Resolution**: 128 x 64 pixels
- **Display Type**: OLED
- **Colors**: Monochrome (White or Yellow)
- **Interface**: 4-wire SPI (default) / I2C (selectable)
- **I2C Address**: 0x3C (DC=Low) or 0x3D (DC=High)
- **Operating Voltage**: 3.3V / 5V
- **Pixel Size**: 0.4 x 0.4 mm
- **Display Size**: 55.01 x 27.49 mm
- **Module Size**: 61.50 x 39.50 mm
- **Touch**: None
- **Connector**: GH1.25 7PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI / I2C SDA | Data input |
| CLK | SPI SCK / I2C SCL | Clock |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command (I2C: address select) |
| RES | Reset | Reset, low active |

---

## OLED Controller IC Summary

| Controller | Type | Max Resolution | GRAM | Color Support | I2C | SPI |
|-----------|------|---------------|------|--------------|-----|-----|
| SSD1306 | OLED | 128x64 | Built-in | Mono | Yes | Yes |
| SSD1309 | OLED | 128x64 | Built-in | Mono | Yes | Yes |
| SSD1315 | OLED | 128x64 | Built-in | Mono | Yes | Yes |
| SSD1327 | OLED | 128x128 | Built-in | 16 Gray Scale | Yes | Yes |
| SSD1351 | OLED | 128x128 | 128x128x18bit | 65K/262K RGB | No | Yes |
| SSD1357 | OLED | 128x128 | Built-in | 65K RGB | No | Yes |
| SH1106 | OLED | 132x64 | Built-in | Mono | Yes | Yes |
| SH1107 | OLED | 128x128 | 128x128 SRAM | Mono | Yes | Yes |

## I2C Address Reference

| Module | Address (DC=Low) | Address (DC=High) |
|--------|-----------------|-------------------|
| 0.91inch (SSD1306) | 0x3C | - (fixed) |
| 0.96inch (SSD1315) | 0x3C | 0x3D |
| 1.3inch B (SH1106) | 0x3C | 0x3D |
| 1.3inch C (SH1107) | 0x3C | 0x3D |
| 1.32inch (SSD1327) | 0x3C | 0x3D |
| 1.5inch (SSD1327) | 0x3C | 0x3D |
| 1.54inch (SSD1309) | 0x3C | 0x3D |
| 2.42inch (SSD1309) | 0x3C | 0x3D |

## ESP32 I2C Wiring Reference

| OLED Pin | ESP32 GPIO (typical) | Notes |
|----------|---------------------|-------|
| VCC | 3.3V | Use 3.3V rail |
| GND | GND | Common ground |
| SDA | GPIO 21 | Default I2C SDA |
| SCL | GPIO 22 | Default I2C SCL |

For SPI mode on dual-interface modules:

| OLED Pin | ESP32 GPIO (typical) | Notes |
|----------|---------------------|-------|
| VCC | 3.3V | Use 3.3V rail |
| GND | GND | Common ground |
| DIN | GPIO 23 | VSPI MOSI |
| CLK | GPIO 18 | VSPI SCK |
| CS | GPIO 5 | VSPI CS |
| DC | Any GPIO | Data/Command select |
| RES | Any GPIO | Reset |

## Notes

- Most dual-interface modules ship in 4-wire SPI mode by default
- Interface selection is done via solder jumpers (BS1/BS2) on the back of the PCB
- I2C modules typically run at 400 KHz (Fast Mode)
- The SSD1306 in the 0.91" module uses only half its 128x64 buffer for the 128x32 display
- SSD1309 is pin-compatible and register-compatible with SSD1306 but supports larger displays
- SSD1315 is a newer replacement for SSD1306 with similar register set
- RGB OLED modules (SSD1351, SSD1357) are SPI-only -- no I2C option
