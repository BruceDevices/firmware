# Waveshare SPI LCD Display Modules

> Comprehensive reference for all Waveshare standalone SPI-connected LCD display modules.
> Last updated: 2026-04-13

## Quick Reference Table

| Model | Size | Resolution | Controller | Touch | Touch IC | Voltage | Connector |
|-------|------|-----------|------------|-------|----------|---------|-----------|
| 0.96inch LCD Module | 0.96" | 160x80 | ST7735S | No | - | 3.3V/5V | PH2.0 8PIN |
| 1.14inch LCD Module | 1.14" | 240x135 | ST7789VW | No | - | 3.3V/5V | PH2.0 8PIN |
| 1.28inch LCD Module | 1.28" | 240x240 | GC9A01 | No | - | 3.3V/5V | PH2.0 8PIN |
| 1.28inch Touch LCD | 1.28" | 240x240 | GC9A01 | Yes | CST816S (I2C) | 3.3V/5V | GH1.25 |
| 1.3inch LCD Module | 1.3" | 240x240 | ST7789VW | No | - | 3.3V/5V | PH2.0 8PIN |
| 1.44inch LCD HAT | 1.44" | 128x128 | ST7735S | No | - | 3.3V | 40PIN GPIO |
| 1.47inch LCD Module | 1.47" | 172x320 | ST7789V3 | No | - | 3.3V/5V | PH2.0 8PIN |
| 1.47inch Touch LCD | 1.47" | 172x320 | JD9853 | Yes | AXS5106L (I2C) | 3.3V/5V | GH1.25 |
| 1.54inch LCD Module | 1.54" | 240x240 | ST7789VW | No | - | 3.3V/5V | PH2.0 8PIN |
| 1.69inch LCD Module | 1.69" | 240x280 | ST7789V2 | No | - | 3.3V/5V | GH1.25 8PIN |
| 1.69inch Touch LCD Module | 1.69" | 240x280 | ST7789V2 | Yes | CST816T (I2C) | 3.3V/5V | GH1.25 |
| 1.8inch LCD Module | 1.8" | 128x160 | ST7735S | No | - | 3.3V/5V | PH2.0 8PIN |
| 1.83inch LCD Module | 1.83" | 240x280 | NV3030B | No | - | 3.3V/5V | GH1.25 8PIN |
| 1.83inch Touch LCD Module | 1.83" | 240x284 | ST7789V2 | Yes | CST816D (I2C) | 3.3V/5V | GH1.25 |
| 1.9inch LCD Module | 1.9" | 170x320 | ST7789V2 | No | - | 3.3V/5V | GH1.25 8PIN |
| 2inch LCD Module | 2.0" | 240x320 | ST7789VW | No | - | 3.3V/5V | PH2.0 8PIN |
| 2inch Capacitive Touch LCD | 2.0" | 240x320 | ST7789T3 | Yes | CST816D (I2C) | 3.3V/5V | 15PIN/18PIN FPC |
| 2.4inch LCD Module | 2.4" | 240x320 | ILI9341 | No | - | 3.3V/5V | PH2.0 8PIN |
| 2.8inch Capacitive Touch LCD | 2.8" | 240x320 | ST7789T3 | Yes | CST328 (I2C) | 3.3V/5V | 13PIN/18PIN FPC |
| 3.5inch Capacitive Touch LCD | 3.5" | 320x480 | ST7796S | Yes | FT6336U (I2C) | 3.3V/5V | 15PIN/18PIN FPC |

---

## Detailed Specifications

### 0.96inch LCD Module

- **Product URL**: https://www.waveshare.com/0.96inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/0.96inch_LCD_Module
- **Controller**: ST7735S (132x162 internal, uses 160x80 window)
- **Resolution**: 160 x 80 pixels
- **Display Type**: IPS
- **Colors**: 65K (RGB565)
- **Color Formats**: RGB444, RGB565, RGB666
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 21.70 x 10.80 mm
- **Pixel Size**: 0.1356 x 0.135 mm
- **Module Size**: 52.00 x 25.00 mm
- **Touch**: None
- **Connector**: PH2.0 8PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 1.14inch LCD Module

- **Product URL**: https://www.waveshare.com/1.14inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.14inch_LCD_Module
- **Controller**: ST7789VW (240x320 internal, uses 135x240 window)
- **Resolution**: 240 x 135 pixels
- **Display Type**: IPS
- **Colors**: 65K (RGB565)
- **Color Formats**: RGB444, RGB565, RGB666
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Touch**: None
- **Connector**: PH2.0 8PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 1.28inch LCD Module (Round)

- **Product URL**: https://www.waveshare.com/1.28inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.28inch_LCD_Module
- **Controller**: GC9A01 (240x240 with 129,600 bytes GRAM)
- **Resolution**: 240 x 240 pixels
- **Display Type**: IPS, Round
- **Colors**: 65K (RGB565)
- **Color Formats**: RGB444, RGB565, RGB666
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: dia. 32.4 mm
- **Pixel Size**: 0.135 x 0.135 mm
- **Module Size**: 40.4 x 37.5 mm (dia. 37.5 mm)
- **Touch**: None
- **Connector**: PH2.0 8PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 1.28inch Touch LCD (Round, with Touch)

- **Product URL**: https://www.waveshare.com/1.28inch-touch-lcd.htm
- **Wiki**: https://www.waveshare.com/wiki/1.28inch_Touch_LCD
- **Display Controller**: GC9A01
- **Resolution**: 240 x 240 pixels
- **Display Type**: IPS, Round
- **Colors**: 65K (RGB565)
- **Interface**: 4-wire SPI (display) + I2C (touch)
- **Operating Voltage**: 3.3V / 5V
- **Touch**: Yes - CST816S capacitive touch (I2C)
- **Touch Points**: Single point
- **Connector**: GH1.25

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| MOSI | SPI MOSI | LCD data input |
| SCLK | SPI SCK | LCD clock |
| LCD_CS | SPI CS | LCD chip select, low active |
| LCD_DC | Data/Command | High = data, Low = command |
| LCD_RST | Reset | LCD reset, low active |
| LCD_BL | Backlight | Backlight control |
| TP_SDA | I2C SDA | Touch data |
| TP_SCL | I2C SCL | Touch clock |
| TP_INT | Interrupt | Touch interrupt |
| TP_RST | Reset | Touch reset |

---

### 1.3inch LCD Module

- **Product URL**: https://www.waveshare.com/1.3inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.3inch_LCD_Module
- **Controller**: ST7789VW (240x320 internal, uses 240x240 window)
- **Resolution**: 240 x 240 pixels
- **Display Type**: IPS
- **Colors**: 65K (RGB565)
- **Color Formats**: RGB444, RGB565, RGB666
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Touch**: None
- **Connector**: PH2.0 8PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 1.44inch LCD HAT

- **Product URL**: https://www.waveshare.com/1.44inch-lcd-hat.htm
- **Wiki**: https://www.waveshare.com/wiki/1.44inch_LCD_HAT
- **Controller**: ST7735S (132x162 internal, uses 128x128 window)
- **Resolution**: 128 x 128 pixels
- **Display Type**: TFT
- **Colors**: 65K (RGB565)
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V
- **Display Size**: 25.5 x 26.5 mm
- **Module Size**: 65 x 30.2 mm
- **Pixel Size**: 0.129(W) x 0.219(H) mm
- **Touch**: None
- **Connector**: 40PIN Raspberry Pi GPIO header
- **Extras**: Built-in joystick (5-position) + 3 buttons

#### Pinout (RPi GPIO mapping)

| Pin | RPi GPIO | Function |
|-----|----------|----------|
| DIN | GPIO 10 (SPI0_MOSI) | SPI data input |
| CLK | GPIO 11 (SPI0_SCLK) | SPI clock |
| CS | GPIO 8 (SPI0_CE0) | Chip select |
| DC | GPIO 25 | Data/Command |
| RST | GPIO 27 | Reset |
| BL | GPIO 24 | Backlight |

---

### 1.47inch LCD Module

- **Product URL**: https://www.waveshare.com/1.47inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.47inch_LCD_Module
- **Controller**: ST7789V3 (240x320 internal, uses 172x320 window)
- **Resolution**: 172 x 320 pixels
- **Display Type**: IPS
- **Colors**: 262K
- **Color Formats**: RGB444, RGB565, RGB666
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 17.39 x 32.35 mm
- **Pixel Pitch**: 0.0337(H) x 0.1011(V) mm
- **Module Size**: 22.0 x 38.5 mm
- **Touch**: None
- **Connector**: PH2.0 8PIN
- **Note**: Rounded corner display

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 1.47inch Touch LCD

- **Product URL**: https://www.waveshare.com/1.47inch-touch-lcd.htm
- **Wiki**: https://www.waveshare.com/wiki/1.47inch_Touch_LCD
- **Display Controller**: JD9853
- **Resolution**: 172 x 320 pixels
- **Display Type**: IPS
- **Colors**: 262K
- **Interface**: 4-wire SPI (display) + I2C (touch)
- **Operating Voltage**: 3.3V / 5V
- **Touch**: Yes - AXS5106L capacitive touch (I2C)
- **Touch Points**: Single point
- **Connector**: GH1.25

---

### 1.54inch LCD Module

- **Product URL**: https://www.waveshare.com/1.54inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.54inch_LCD_Module
- **Controller**: ST7789VW (240x320 internal, uses 240x240 window)
- **Resolution**: 240 x 240 pixels
- **Display Type**: IPS
- **Colors**: 65K (RGB565)
- **Color Formats**: RGB444, RGB565, RGB666
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Touch**: None
- **Connector**: PH2.0 8PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 1.69inch LCD Module

- **Product URL**: https://www.waveshare.com/1.69inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.69inch_LCD_Module
- **Controller**: ST7789V2 (240x320 internal, uses 240x280 window)
- **Resolution**: 240 x 280 pixels
- **Display Type**: IPS
- **Colors**: 262K
- **Color Formats**: RGB444, RGB565, RGB666
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 27.972 x 32.634 mm
- **Display R angle**: 4-R5 mm
- **Pixel Pitch**: 0.11655 x 0.11655 mm
- **Module Size**: 31.5 x 39.0 mm
- **Touch**: None
- **Connector**: GH1.25 8PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 1.69inch Touch LCD Module

- **Product URL**: https://www.waveshare.com/1.69inch-touch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module
- **Display Controller**: ST7789V2
- **Resolution**: 240 x 280 pixels
- **Display Type**: IPS
- **Colors**: 262K
- **Interface**: 4-wire SPI (display) + I2C (touch)
- **Operating Voltage**: 3.3V / 5V
- **Touch**: Yes - CST816T capacitive touch (I2C)
- **Touch Points**: Single point
- **Connector**: GH1.25

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| MOSI | SPI MOSI | LCD data input |
| SCLK | SPI SCK | LCD clock |
| LCD_CS | SPI CS | LCD chip select, low active |
| LCD_DC | Data/Command | High = data, Low = command |
| LCD_RST | Reset | LCD reset, low active |
| LCD_BL | Backlight | Backlight control |
| TP_SDA | I2C SDA | Touch data |
| TP_SCL | I2C SCL | Touch clock |
| TP_INT | Interrupt | Touch interrupt |
| TP_RST | Reset | Touch reset |

---

### 1.8inch LCD Module

- **Product URL**: https://www.waveshare.com/1.8inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.8inch_LCD_Module
- **Controller**: ST7735S (132x162 internal, uses 128x160 window)
- **Resolution**: 128 x 160 pixels
- **Display Type**: TFT
- **Colors**: 65K (RGB565)
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 35.04 x 28.03 mm
- **Pixel Size**: 0.219 x 0.219 mm
- **Module Size**: 56.5 x 34 mm
- **Touch**: None
- **Connector**: PH2.0 8PIN
- **Note**: Display starts from 2nd pixel horizontally, 1st pixel vertically (ST7735S offset)

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 1.83inch LCD Module

- **Product URL**: https://www.waveshare.com/1.83inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.83inch_LCD_Module
- **Controller**: NV3030B
- **Resolution**: 240 x 280 pixels
- **Display Type**: IPS
- **Colors**: 65K
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 30.20 x 35.23 mm
- **Pixel Size**: 0.126 x 0.126 mm
- **Module Size**: 33.00 x 40.00 mm
- **Touch**: None
- **Connector**: GH1.25 8PIN
- **Note**: Rounded corner display

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 1.83inch Touch LCD Module

- **Product URL**: https://www.waveshare.com/product/displays/lcd-oled/1.83inch-touch-lcd-module.htm
- **Display Controller**: ST7789V2
- **Resolution**: 240 x 284 pixels
- **Display Type**: IPS
- **Colors**: 262K
- **Interface**: 4-wire SPI (display) + I2C (touch)
- **Operating Voltage**: 3.3V / 5V
- **Module Size**: 35.00 x 42.00 mm
- **Touch**: Yes - CST816D capacitive touch (I2C, 10KHz-400KHz)
- **Touch Points**: Single point

---

### 1.9inch LCD Module

- **Product URL**: https://www.waveshare.com/1.9inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.9inch_LCD_Module
- **Controller**: ST7789V2
- **Resolution**: 170 x 320 pixels
- **Display Type**: IPS
- **Colors**: 262K
- **Color Formats**: RGB444, RGB565, RGB666
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 22.70 x 42.72 mm
- **Pixel Size**: 0.1335 x 0.1335 mm
- **Module Size**: 27.3 x 51.2 mm
- **Touch**: None
- **Connector**: GH1.25 8PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 2inch LCD Module

- **Product URL**: https://www.waveshare.com/2inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/2inch_LCD_Module
- **Controller**: ST7789VW
- **Resolution**: 240 x 320 pixels
- **Display Type**: IPS
- **Colors**: 262K
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Touch**: None
- **Connector**: PH2.0 8PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 2inch Capacitive Touch LCD

- **Product URL**: https://www.waveshare.com/2inch-capacitive-touch-lcd.htm
- **Wiki**: https://www.waveshare.com/wiki/2inch_Capacitive_Touch_LCD
- **Display Controller**: ST7789T3
- **Resolution**: 240 x 320 pixels
- **Display Type**: IPS
- **Colors**: 262K
- **Interface**: 4-wire SPI (display) + I2C (touch)
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 30.6 x 40.8 mm
- **Touch**: Yes - CST816D capacitive touch (I2C)
- **Touch Points**: Single point
- **Connector**: 15PIN terminal / 18PIN FPC slot
- **Extras**: TF card slot

---

### 2.4inch LCD Module

- **Product URL**: https://www.waveshare.com/2.4inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/2.4inch_LCD_Module
- **Controller**: ILI9341
- **Resolution**: 240 x 320 pixels
- **Display Type**: TFT
- **Colors**: 65K (RGB565)
- **Interface**: 4-wire SPI
- **Operating Voltage**: 3.3V / 5V
- **Display Size**: 36.72 x 48.96 mm
- **Pixel Size**: 0.153 x 0.153 mm
- **Module Size**: 70.50 x 43.30 mm
- **Touch**: None
- **Connector**: PH2.0 8PIN

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock input |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BL | Backlight | Backlight control |

---

### 2.8inch Capacitive Touch LCD

- **Product URL**: https://www.waveshare.com/2.8inch-capacitive-touch-lcd.htm
- **Wiki**: https://www.waveshare.com/wiki/2.8inch_Capacitive_Touch_LCD
- **Display Controller**: ST7789T3
- **Resolution**: 240 x 320 pixels
- **Display Type**: TFT
- **Colors**: 262K
- **Interface**: SPI (display) + I2C (touch)
- **Operating Voltage**: 3.3V / 5V
- **Touch**: Yes - CST328 capacitive touch (I2C)
- **Touch Points**: Up to 5 simultaneous
- **Connector**: 13PIN / 18PIN FPC

---

### 3.5inch Capacitive Touch LCD

- **Product URL**: https://www.waveshare.com/3.5inch-capacitive-touch-lcd.htm
- **Wiki**: https://www.waveshare.com/wiki/3.5inch_Capacitive_Touch_LCD
- **Display Controller**: ST7796S
- **Resolution**: 320 x 480 pixels
- **Display Type**: IPS
- **Colors**: 262K
- **Interface**: 4-wire SPI (display) + I2C (touch)
- **Operating Voltage**: 3.3V / 5V
- **Pixel Pitch**: 0.051 x 0.153 mm
- **Display Size**: 48.96 x 73.44 mm
- **Module Size**: 61.00 x 92.44 mm
- **Touch**: Yes - FT6336U capacitive touch (I2C)
- **Touch Points**: 2-point
- **Connector**: 15PIN / 18PIN FPC
- **Extras**: TF card slot

---

## Controller IC Summary

| Controller | Type | Max Resolution | GRAM | Color Depth | Common Sizes |
|-----------|------|---------------|------|-------------|--------------|
| ST7735S | LCD | 132x162 | Built-in | 65K/262K | 0.96", 1.44", 1.8" |
| ST7789VW | LCD | 240x320 | Built-in | 65K/262K | 1.14", 1.3", 1.54", 2.0" |
| ST7789V2 | LCD | 240x320 | Built-in | 262K | 1.69", 1.9" |
| ST7789V3 | LCD | 240x320 | Built-in | 262K | 1.47" |
| ST7789T3 | LCD | 240x320 | Built-in | 262K | 2.0" touch, 2.8" touch |
| NV3030B | LCD | 240x280 | Built-in | 65K | 1.83" |
| JD9853 | LCD | 172x320 | Built-in | 262K | 1.47" touch |
| GC9A01 | LCD | 240x240 | 129,600 bytes | 65K/262K | 1.28" (round) |
| ILI9341 | LCD | 240x320 | 172,800 bytes | 262K | 2.4", 2.8", 3.2" |
| ST7796S | LCD | 320x480 | Built-in | 262K | 3.5" |

## Touch Controller IC Summary

| Controller | Type | Interface | Points | Speed | Used On |
|-----------|------|-----------|--------|-------|---------|
| CST816S | Capacitive | I2C | 1 | 10-400 KHz | 1.28" Touch LCD |
| CST816T | Capacitive | I2C | 1 | 10-400 KHz | 1.69" Touch LCD |
| CST816D | Capacitive | I2C | 1 | 10-400 KHz | 1.83", 2.0" Touch LCD |
| CST328 | Capacitive | I2C | 5 | Standard I2C | 2.8" Touch LCD |
| AXS5106L | Capacitive | I2C | 1 | Standard I2C | 1.47" Touch LCD |
| FT6336U | Capacitive | I2C | 2 | Standard I2C | 3.5" Touch LCD |
| XPT2046 | Resistive | SPI | 1 | 125 KHz ADC | 3.2", 3.5" Res Touch |

## ESP32 SPI Wiring Reference

Standard wiring for non-touch SPI displays to ESP32:

| Display Pin | ESP32 GPIO (typical) | Notes |
|------------|---------------------|-------|
| VCC | 3.3V | Use 3.3V rail |
| GND | GND | Common ground |
| DIN (MOSI) | GPIO 23 (or any) | VSPI default MOSI |
| CLK (SCK) | GPIO 18 (or any) | VSPI default SCK |
| CS | GPIO 5 (or any) | VSPI default CS |
| DC | Any GPIO | Data/Command select |
| RST | Any GPIO | Can tie to EN for auto-reset |
| BL | Any GPIO (or 3.3V) | PWM for brightness control |

For touch-enabled displays, add I2C connections:

| Touch Pin | ESP32 GPIO (typical) | Notes |
|-----------|---------------------|-------|
| TP_SDA | GPIO 21 | Default I2C SDA |
| TP_SCL | GPIO 22 | Default I2C SCL |
| TP_INT | Any GPIO | Interrupt input |
| TP_RST | Any GPIO | Touch controller reset |

## SPI Communication Notes

- All modules use SPI Mode 0 (CPOL=0, CPHA=0)
- Typical SPI clock: 20-40 MHz (controller dependent)
- ST7789 family supports up to 62.5 MHz SPI clock
- ILI9341 supports up to 10 MHz read, 40 MHz write
- Data is transmitted MSB first
- CS is active low
- DC pin: LOW = command byte, HIGH = data byte
