# Waveshare Round/Circular LCD Display Modules

> Reference for all Waveshare standalone round and circular display modules.
> Last updated: 2026-04-13

## Quick Reference Table

| Model | Size | Resolution | Controller | Shape | Touch | Touch IC | Voltage |
|-------|------|-----------|------------|-------|-------|----------|---------|
| 0.71inch LCD Module | 0.71" | 160x160 | GC9D01 | Round | No | - | 3.3V/5V |
| 0.71inch DualEye LCD Module | 0.71" x2 | 160x160 x2 | GC9D01 x2 | Round (dual) | No | - | 3.3V/5V |
| 1.28inch LCD Module | 1.28" | 240x240 | GC9A01 | Round | No | - | 3.3V/5V |
| 1.28inch Touch LCD | 1.28" | 240x240 | GC9A01 | Round | Yes | CST816S (I2C) | 3.3V/5V |
| 1.69inch Touch LCD Module | 1.69" | 240x280 | ST7789V2 | Rounded rect | Yes | CST816T (I2C) | 3.3V/5V |

---

## Detailed Specifications

### 0.71inch LCD Module (Round)

- **Product URL**: https://www.waveshare.com/0.71inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/0.71inch_LCD_Module
- **Controller**: GC9D01
- **Resolution**: 160 x 160 pixels
- **Display Type**: IPS, Round
- **Colors**: 65K (RGB565)
- **Interface**: SPI
- **Operating Voltage**: 3.3V / 5V
- **Touch**: None
- **Connector**: SH1.0 8PIN

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

#### Notes

- Smallest round display in the Waveshare lineup
- GC9D01 is a compact LCD controller optimized for small circular displays
- Suitable for wearables, robot eyes, indicator displays

---

### 0.71inch DualEye LCD Module

- **Product URL**: https://www.waveshare.com/0.71inch-dualeye-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/0.71inch_DualEye_LCD_Module
- **Controller**: GC9D01 x2 (one per eye)
- **Resolution**: 160 x 160 pixels (each display)
- **Display Type**: IPS, Round (dual)
- **Colors**: 65K (RGB565)
- **Interface**: SPI (shared bus, separate CS)
- **Operating Voltage**: 3.3V / 5V
- **Touch**: None

#### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V/5V input |
| GND | Ground | Ground |
| DIN | SPI MOSI | Shared serial data input |
| CLK | SPI SCK | Shared serial clock |
| CS1 | SPI CS (Left) | Left eye chip select |
| CS2 | SPI CS (Right) | Right eye chip select |
| DC | Data/Command | Shared, High = data, Low = command |
| RST | Reset | Shared reset, low active |
| BL | Backlight | Backlight control |

#### Notes

- Two 0.71" round displays on a single PCB
- Designed for robot eye projects and expressive displays
- Both displays share the SPI bus but have independent CS lines
- Can display different content on each eye simultaneously

---

### 1.28inch LCD Module (Round)

- **Product URL**: https://www.waveshare.com/1.28inch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.28inch_LCD_Module
- **Controller**: GC9A01 (240RGB x 240 dots, 129,600 bytes GRAM)
- **Resolution**: 240 x 240 pixels
- **Display Type**: IPS, Round
- **Colors**: 65K (RGB565)
- **Color Formats**: RGB444, RGB565, RGB666
- **Interface**: 4-wire SPI (supports 12/16/18-bit MCU interface)
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

#### Notes

- Most popular round display module from Waveshare
- GC9A01 is the de facto standard controller for round displays
- Extensively supported by TFT_eSPI, LVGL, and other ESP32 graphics libraries
- Ideal for smartwatch interfaces, gauges, dials, and clocks

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
| TP_INT | Interrupt | Touch interrupt, low active |
| TP_RST | Reset | Touch reset, low active |

#### Touch Controller Details (CST816S)

- Single-point capacitive touch
- I2C interface at 10-400 KHz
- I2C Address: 0x15
- Supports gestures: tap, long press, swipe (up/down/left/right)
- Low power consumption with auto-sleep

---

### 1.69inch Touch LCD Module (Rounded Rectangle)

- **Product URL**: https://www.waveshare.com/1.69inch-touch-lcd-module.htm
- **Wiki**: https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module
- **Display Controller**: ST7789V2
- **Resolution**: 240 x 280 pixels
- **Display Type**: IPS, Rounded rectangle
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

#### Notes

- Not truly circular but has rounded corners creating a smartwatch-like appearance
- Popular for smartwatch and wearable prototypes

---

## Round Display Controller IC Summary

| Controller | Resolution | GRAM | Color Depth | Shape | Notes |
|-----------|-----------|------|-------------|-------|-------|
| GC9D01 | 160x160 | Built-in | 65K | Round | Ultra-small displays |
| GC9A01 | 240x240 | 129,600 bytes | 65K/262K | Round | Most popular round display IC |
| ST7789V2 | 240x320 | Built-in | 262K | Rect (round corners) | General purpose, used for rounded rect |

## ESP32 Wiring for Round Displays

### Non-Touch (1.28inch LCD Module)

| Display Pin | ESP32 GPIO | Notes |
|------------|-----------|-------|
| VCC | 3.3V | 3.3V rail |
| GND | GND | Ground |
| DIN | GPIO 23 | VSPI MOSI |
| CLK | GPIO 18 | VSPI SCK |
| CS | GPIO 5 | VSPI CS |
| DC | GPIO 2 | Any free GPIO |
| RST | GPIO 4 | Any free GPIO |
| BL | GPIO 15 | PWM-capable for brightness |

### Touch-Enabled (1.28inch Touch LCD)

| Display Pin | ESP32 GPIO | Notes |
|------------|-----------|-------|
| VCC | 3.3V | 3.3V rail |
| GND | GND | Ground |
| MOSI | GPIO 23 | VSPI MOSI |
| SCLK | GPIO 18 | VSPI SCK |
| LCD_CS | GPIO 5 | VSPI CS |
| LCD_DC | GPIO 2 | Any free GPIO |
| LCD_RST | GPIO 4 | Any free GPIO |
| LCD_BL | GPIO 15 | PWM-capable |
| TP_SDA | GPIO 21 | I2C SDA |
| TP_SCL | GPIO 22 | I2C SCL |
| TP_INT | GPIO 36 | Input-only OK |
| TP_RST | GPIO 16 | Any free GPIO |

## Software Library Support

### TFT_eSPI Configuration for GC9A01

```
// In User_Setup.h or platformio.ini
#define GC9A01_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 240
#define TFT_MOSI   23
#define TFT_SCLK   18
#define TFT_CS     5
#define TFT_DC     2
#define TFT_RST    4
#define TFT_BL     15
#define SPI_FREQUENCY  80000000  // GC9A01 supports up to 80MHz
```

### LVGL Round Display Configuration

For LVGL with round displays, set the display area to circular by using a mask or by configuring the display resolution to match the circular viewport (240x240 for the 1.28" module).

## Use Cases

| Application | Recommended Module | Why |
|------------|-------------------|-----|
| Robot eyes | 0.71inch DualEye | Two eyes, small, expressive |
| Smartwatch prototype | 1.28inch Touch LCD | Round, touch, high res |
| Gauge/dial indicator | 1.28inch LCD Module | Round, no touch needed |
| Fitness band | 1.69inch Touch LCD | Tall aspect, touch |
| Dashboard widget | 1.28inch LCD Module | Clean circular display |
| Desk clock | 1.28inch LCD Module | Analog clock face |
