# Waveshare Reference Documentation

Reference documentation for Waveshare ESP32 display products. Used by AI agents for hardware configuration, driver selection, and firmware development.

## Directory Structure

```
waveshare/
├── README.md                          (this file)
├── common/
│   ├── display-controllers.md         Display controller ICs
│   ├── touch-controllers.md           Touch controller ICs
│   └── interfaces.md                  Hardware interfaces and wiring
├── dev-boards/                        ESP32 development boards with integrated displays
│   └── (board-specific reference files)
└── lcd-boards/                        Standalone LCD/touch modules
    └── (module-specific reference files)
```

## Common Reference Files

### [Display Controllers](common/display-controllers.md)

All display controller ICs used across Waveshare products:

| Controller | Max Resolution | Interface | Typical Size |
|------------|---------------|-----------|--------------|
| ST7735S | 162x132 | SPI | 0.96"-1.8" |
| ST7789V | 320x240 | SPI | 1.3"-2.4" |
| ILI9341 | 320x240 | SPI, 8080 | 2.0"-3.2" |
| ILI9488 | 480x320 | SPI (18-bit), 8080 | 3.5" |
| GC9A01 | 240x240 | SPI | 1.28" round |
| GC9503 | 480x480 | RGB + SPI init | 3.4"-4.0" |
| ST7701S | 480x480 | RGB + SPI init | 4.0" round/square |
| EK9716B | 1024x600 | RGB | 7"+ |
| JD9365 | 800x1280 | MIPI-DSI | 7"+ portrait |
| NV3041A | 480x272 | QSPI, 8080 | compact |

Includes: register maps, init sequences, MADCTL rotation values, LVGL driver mapping, buffer strategies.

### [Touch Controllers](common/touch-controllers.md)

All touch controller ICs used across Waveshare products:

| Controller | Type | Interface | Address | Multi-Touch |
|------------|------|-----------|---------|-------------|
| XPT2046 | Resistive | SPI | N/A | No |
| CST816S | Capacitive | I2C | 0x15 | No |
| FT6336 | Capacitive | I2C | 0x38 | 2 points |
| GT911 | Capacitive | I2C | 0x5D/0x14 | 5 points |
| FT5x06 | Capacitive | I2C | 0x38 | 5 points |
| CST328 | Capacitive | I2C | 0x1A | 5 points |

Includes: register maps, gesture IDs, calibration (XPT2046), interrupt behavior, LVGL integration patterns.

### [Interfaces](common/interfaces.md)

Hardware interface patterns and wiring:

| Interface | Throughput | Pins | Use Case |
|-----------|-----------|------|----------|
| SPI (4-wire) | 80 Mbps | 5-6 | Small-mid displays |
| I2C | 3.2 Mbps | 2 | Touch, small OLEDs |
| 8080 Parallel | 160 Mbps | 12-13 | Mid displays |
| RGB Parallel | 400+ Mbps | 22-32 | Large displays |
| QSPI | 320 Mbps | 7-8 | Mid displays, fewer pins |
| MIPI-DSI | 4+ Gbps | 10 | Very large displays |

Includes: voltage regulators, backlight control (PWM/GPIO), ESP32 variant support matrix, pin mapping patterns.

## Quick Lookup

### By Display Size

| Size | Controller | Touch | Interface | ESP32 Variant |
|------|-----------|-------|-----------|---------------|
| 0.96"-1.8" | ST7735S | None or CST816S | SPI | Any |
| 1.28" round | GC9A01 | CST816S | SPI | Any |
| 1.3"-2.4" | ST7789V | None or CST816S | SPI | Any |
| 2.4"-3.2" | ILI9341 | XPT2046 | SPI | Any |
| 3.5" | ILI9488 | XPT2046 or FT5x06 | SPI/8080 | ESP32-S3 preferred |
| 4.0" square | GC9503/ST7701S | GT911 | RGB | ESP32-S3 (PSRAM) |
| 4.0" round | ST7701S | CST816S/GT911 | RGB | ESP32-S3 (PSRAM) |
| 7.0"+ | EK9716B | GT911 | RGB | ESP32-S3 (Octal PSRAM) |
| 7.0"+ MIPI | JD9365 | GT911 | MIPI-DSI | ESP32-P4 |

### By ESP32 Variant

| ESP32 Variant | Supported Interfaces | Max Practical Display |
|---------------|---------------------|----------------------|
| ESP32 | SPI | 320x240 (SPI) |
| ESP32-S2 | SPI, 8080 | 480x320 (8080) |
| ESP32-S3 | SPI, 8080, RGB, QSPI | 1024x600 (RGB) |
| ESP32-C3 | SPI | 240x240 (SPI) |
| ESP32-C6 | SPI | 240x240 (SPI) |
| ESP32-P4 | SPI, 8080, RGB, QSPI, MIPI-DSI | 800x1280 (MIPI-DSI) |

---
