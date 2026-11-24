# Waveshare LCD Display Board Reference

> Comprehensive reference index for all Waveshare standalone LCD, OLED, and E-Paper display modules.
> Focused on modules commonly used with ESP32 microcontrollers.
> Last updated: 2026-04-13

## Reference Files

| File | Contents |
|------|----------|
| [spi-displays.md](spi-displays.md) | All SPI-connected LCD display modules (ST7735, ST7789, ILI9341, GC9A01, etc.) |
| [i2c-displays.md](i2c-displays.md) | I2C and SPI/I2C OLED display modules (SSD1306, SSD1309, SSD1327, SH1106, etc.) |
| [parallel-displays.md](parallel-displays.md) | 8080 parallel and RGB parallel interface displays |
| [round-displays.md](round-displays.md) | Circular and round displays (GC9A01, GC9D01) |
| [e-paper.md](e-paper.md) | E-Paper/E-Ink displays with ESP32 compatibility |

---

## Master Display Index

### SPI LCD Modules (No Touch)

| Model | Size | Resolution | Controller | Colors |
|-------|------|-----------|------------|--------|
| 0.96inch LCD Module | 0.96" | 160x80 | ST7735S | 65K |
| 1.14inch LCD Module | 1.14" | 240x135 | ST7789VW | 65K |
| 1.28inch LCD Module | 1.28" | 240x240 | GC9A01 | 65K |
| 1.3inch LCD Module | 1.3" | 240x240 | ST7789VW | 65K |
| 1.44inch LCD HAT | 1.44" | 128x128 | ST7735S | 65K |
| 1.47inch LCD Module | 1.47" | 172x320 | ST7789V3 | 262K |
| 1.54inch LCD Module | 1.54" | 240x240 | ST7789VW | 65K |
| 1.69inch LCD Module | 1.69" | 240x280 | ST7789V2 | 262K |
| 1.8inch LCD Module | 1.8" | 128x160 | ST7735S | 65K |
| 1.83inch LCD Module | 1.83" | 240x280 | NV3030B | 65K |
| 1.9inch LCD Module | 1.9" | 170x320 | ST7789V2 | 262K |
| 2inch LCD Module | 2.0" | 240x320 | ST7789VW | 262K |
| 2.4inch LCD Module | 2.4" | 240x320 | ILI9341 | 65K |

### SPI LCD Modules (With Touch)

| Model | Size | Resolution | Display IC | Touch IC | Touch Type |
|-------|------|-----------|-----------|----------|------------|
| 1.28inch Touch LCD | 1.28" | 240x240 | GC9A01 | CST816S | Cap (I2C) |
| 1.47inch Touch LCD | 1.47" | 172x320 | JD9853 | AXS5106L | Cap (I2C) |
| 1.69inch Touch LCD Module | 1.69" | 240x280 | ST7789V2 | CST816T | Cap (I2C) |
| 1.83inch Touch LCD Module | 1.83" | 240x284 | ST7789V2 | CST816D | Cap (I2C) |
| 2inch Capacitive Touch LCD | 2.0" | 240x320 | ST7789T3 | CST816D | Cap (I2C) |
| 2.8inch Capacitive Touch LCD | 2.8" | 240x320 | ST7789T3 | CST328 | Cap (I2C) |
| 3.5inch Capacitive Touch LCD | 3.5" | 320x480 | ST7796S | FT6336U | Cap (I2C) |

### Round Displays

| Model | Size | Resolution | Controller | Touch |
|-------|------|-----------|------------|-------|
| 0.71inch LCD Module | 0.71" | 160x160 | GC9D01 | No |
| 0.71inch DualEye LCD Module | 0.71" x2 | 160x160 x2 | GC9D01 x2 | No |
| 1.28inch LCD Module | 1.28" | 240x240 | GC9A01 | No |
| 1.28inch Touch LCD | 1.28" | 240x240 | GC9A01 | CST816S |

### OLED Modules

| Model | Size | Resolution | Controller | Interface | Colors |
|-------|------|-----------|------------|-----------|--------|
| 0.91inch OLED Module | 0.91" | 128x32 | SSD1306 | I2C only | Mono |
| 0.96inch OLED Module | 0.96" | 128x64 | SSD1315 | SPI/I2C | Mono |
| 0.96inch RGB OLED Module | 0.96" | 64x128 | SSD1357 | SPI | 65K RGB |
| 1.27inch RGB OLED Module | 1.27" | 128x96 | SSD1351 | SPI | 262K RGB |
| 1.3inch OLED (B) | 1.3" | 128x64 | SH1106 | SPI/I2C | Mono |
| 1.3inch OLED Module (C) | 1.3" | 64x128 | SH1107 | SPI/I2C | Mono |
| 1.32inch OLED Module | 1.32" | 128x96 | SSD1327 | SPI/I2C | 16 Gray |
| 1.5inch OLED Module | 1.5" | 128x128 | SSD1327 | SPI/I2C | 16 Gray |
| 1.5inch RGB OLED Module | 1.5" | 128x128 | SSD1351 | SPI | 65K RGB |
| 1.54inch OLED Module | 1.54" | 128x64 | SSD1309 | SPI/I2C | Mono |
| 2.42inch OLED Module | 2.42" | 128x64 | SSD1309 | SPI/I2C | Mono |

### Parallel Interface Displays

| Model | Size | Resolution | Controller | Interface | Touch |
|-------|------|-----------|------------|-----------|-------|
| 3.2inch Touch LCD (D) | 3.2" | 320x240 | ILI9341 | 8080 16-bit | XPT2046 (Res) |
| 4inch Resistive Touch LCD | 4.0" | 480x800 | NT35510 | 8080 16/8-bit | XPT2046 (Res) |

### E-Paper Modules (Black/White)

| Model | Size | Resolution | Partial Refresh |
|-------|------|-----------|-----------------|
| 1.02inch e-Paper | 1.02" | 128x80 | Yes |
| 1.54inch e-Paper | 1.54" | 200x200 | Yes |
| 2.13inch e-Paper | 2.13" | 250x122 | Yes |
| 2.66inch e-Paper | 2.66" | 296x152 | Yes |
| 2.7inch e-Paper | 2.7" | 264x176 | Yes |
| 2.9inch e-Paper | 2.9" | 296x128 | Yes |
| 3.52inch e-Paper | 3.52" | 360x240 | Yes |
| 3.7inch e-Paper | 3.7" | 480x280 | Yes |
| 4.2inch e-Paper | 4.2" | 400x300 | Yes |
| 5.79inch e-Paper | 5.79" | 792x272 | No |
| 5.83inch e-Paper | 5.83" | 648x480 | No |
| 7.5inch e-Paper (V2) | 7.5" | 800x480 | No |
| 10.3inch e-Paper | 10.3" | 1872x1404 | Yes |
| 12.48inch e-Paper | 12.48" | 1304x984 | No |

### E-Paper Modules (Multi-Color)

| Model | Size | Resolution | Colors |
|-------|------|-----------|--------|
| 1.54inch e-Paper (B) | 1.54" | 200x200 | R/B/W |
| 2.13inch e-Paper (B) | 2.13" | 250x122 | R/B/W |
| 2.7inch e-Paper (B) | 2.7" | 264x176 | R/B/W |
| 2.9inch e-Paper (B) | 2.9" | 296x128 | R/B/W |
| 4.2inch e-Paper (B) | 4.2" | 400x300 | R/B/W |
| 3inch e-Paper (G) | 3.0" | 400x168 | R/Y/B/W |
| 3.5inch e-Paper (G) | 3.5" | 384x184 | R/Y/B/W |
| 4.37inch e-Paper (G) | 4.37" | 512x368 | R/Y/B/W |
| 7.3inch e-Paper (G) | 7.3" | 800x480 | R/Y/B/W |
| 4.01inch e-Paper (F) | 4.01" | 640x400 | 7-Color ACeP |
| 5.65inch e-Paper (F) | 5.65" | 600x448 | 7-Color ACeP |
| 7.3inch e-Paper (F) | 7.3" | 800x480 | 7-Color ACeP |

---

## Display Controller Quick Lookup

### LCD Controllers

| IC | Interface | Max Res | Typical Sizes | Key Feature |
|----|-----------|---------|---------------|-------------|
| ST7735S | SPI | 132x162 | 0.96", 1.44", 1.8" | Low cost, basic |
| ST7789VW | SPI | 240x320 | 1.14", 1.3", 1.54", 2.0" | Popular mid-range |
| ST7789V2 | SPI | 240x320 | 1.69", 1.9" | 262K colors |
| ST7789V3 | SPI | 240x320 | 1.47" | Rounded corners |
| ST7789T3 | SPI | 240x320 | 2.0", 2.8" (touch) | Touch display variant |
| NV3030B | SPI | 240x280 | 1.83" | Budget option |
| JD9853 | SPI | 172x320 | 1.47" (touch) | Touch display variant |
| GC9A01 | SPI | 240x240 | 1.28" | Round display standard |
| GC9D01 | SPI | 160x160 | 0.71" | Ultra-small round |
| ILI9341 | SPI/8080 | 240x320 | 2.4", 2.8", 3.2" | Versatile, well-documented |
| ILI9488 | SPI | 320x480 | 3.5" (resistive) | Large SPI display |
| ST7796S | SPI | 320x480 | 3.5" (capacitive) | Large cap touch |
| NT35510 | 8080 | 480x800 | 4.0" | Parallel only |

### OLED Controllers

| IC | Interface | Max Res | Color | Typical Sizes |
|----|-----------|---------|-------|---------------|
| SSD1306 | I2C/SPI | 128x64 | Mono | 0.91" |
| SSD1309 | I2C/SPI | 128x64 | Mono | 1.54", 2.42" |
| SSD1315 | I2C/SPI | 128x64 | Mono | 0.96" |
| SSD1327 | I2C/SPI | 128x128 | 16 Gray | 1.32", 1.5" |
| SSD1351 | SPI | 128x128 | 262K RGB | 1.27", 1.5" |
| SSD1357 | SPI | 128x128 | 65K RGB | 0.96" |
| SH1106 | I2C/SPI | 132x64 | Mono | 1.3" |
| SH1107 | I2C/SPI | 128x128 | Mono | 1.3" |

### Touch Controllers

| IC | Type | Interface | Points | Used On |
|----|------|-----------|--------|---------|
| CST816S | Capacitive | I2C | 1 | 1.28" round |
| CST816T | Capacitive | I2C | 1 | 1.69" |
| CST816D | Capacitive | I2C | 1 | 1.83", 2.0" |
| CST328 | Capacitive | I2C | 5 | 2.8" |
| AXS5106L | Capacitive | I2C | 1 | 1.47" |
| FT6336U | Capacitive | I2C | 2 | 3.5" |
| XPT2046 | Resistive | SPI | 1 | 3.2", 3.5", 4.0" |

---

## Standard Pinout Reference

### 8-Pin SPI LCD (PH2.0 / GH1.25)

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | VCC | 3.3V/5V power |
| 2 | GND | Ground |
| 3 | DIN | SPI MOSI |
| 4 | CLK | SPI SCK |
| 5 | CS | Chip select (active low) |
| 6 | DC | Data/Command |
| 7 | RST | Reset (active low) |
| 8 | BL | Backlight |

### 8-Pin SPI E-Paper

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | VCC | 3.3V power |
| 2 | GND | Ground |
| 3 | DIN | SPI MOSI |
| 4 | CLK | SPI SCK |
| 5 | CS | Chip select (active low) |
| 6 | DC | Data/Command |
| 7 | RST | Reset (active low) |
| 8 | BUSY | Busy status (active low) |

### 4-Pin I2C OLED

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | GND | Ground |
| 2 | VCC | 3.3V/5V power |
| 3 | SCL | I2C clock |
| 4 | SDA | I2C data |

### 7-Pin SPI/I2C OLED (GH1.25)

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | VCC | 3.3V/5V power |
| 2 | GND | Ground |
| 3 | DIN | SPI MOSI / I2C SDA |
| 4 | CLK | SPI SCK / I2C SCL |
| 5 | CS | SPI chip select |
| 6 | DC | Data/Command (I2C: address bit) |
| 7 | RES | Reset (active low) |

---

## ESP32 Variant Compatibility

| Display Type | ESP32 | ESP32-S2 | ESP32-S3 | ESP32-C3 | ESP32-C6 |
|-------------|-------|----------|----------|----------|----------|
| SPI LCD (small) | Good | Good | Best | Good | Good |
| SPI LCD (large) | Good | Good | Best | OK | OK |
| SPI LCD + Touch | Good | Good | Best | Good | Good |
| I2C OLED | Good | Good | Good | Good | Good |
| 8080 Parallel | Limited | OK (I2S) | Best (LCD_CAM) | No | No |
| SPI E-Paper | Good | Good | Good | Good | Good |

---

## Official Resources

- **Waveshare Wiki**: https://www.waveshare.com/wiki/Main_Page
- **Waveshare Display Catalog**: https://www.waveshare.com/product/displays.htm
- **E-Paper Catalog**: https://www.waveshare.com/epaper
- **GitHub (LCD-show)**: https://github.com/waveshareteam/LCD-show
- **E-Paper ESP32 Driver Board**: https://www.waveshare.com/e-paper-esp32-driver-board.htm
