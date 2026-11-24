# Waveshare E-Paper / E-Ink Display Modules

> Reference for all Waveshare standalone e-paper display modules with ESP32 compatibility.
> Last updated: 2026-04-13

## Quick Reference Table

### Black/White Modules

| Model | Size | Resolution | Colors | Interface | Partial Refresh | Full Refresh |
|-------|------|-----------|--------|-----------|----------------|-------------|
| 1.02inch e-Paper Module | 1.02" | 128x80 | B/W | SPI | Yes | ~2s |
| 1.54inch e-Paper Module | 1.54" | 200x200 | B/W | SPI | Yes (~0.3s) | ~2s |
| 2.13inch e-Paper Module | 2.13" | 250x122 | B/W | SPI | Yes | ~2s |
| 2.66inch e-Paper Module | 2.66" | 296x152 | B/W | SPI | Yes | ~2s |
| 2.7inch e-Paper HAT | 2.7" | 264x176 | B/W | SPI | Yes | ~6s |
| 2.9inch e-Paper Module | 2.9" | 296x128 | B/W | SPI | Yes | ~2s |
| 3.52inch e-Paper HAT | 3.52" | 360x240 | B/W | SPI | Yes | ~3s |
| 3.7inch e-Paper HAT | 3.7" | 480x280 | B/W (4 gray) | SPI | Yes | ~3s |
| 4.2inch e-Paper Module | 4.2" | 400x300 | B/W | SPI | Yes | ~4s |
| 5.79inch e-Paper Module | 5.79" | 792x272 | B/W | SPI | No | ~6s |
| 5.83inch e-Paper HAT | 5.83" | 648x480 | B/W | SPI | No | ~6s |
| 7.5inch e-Paper HAT (V2) | 7.5" | 800x480 | B/W | SPI | No | ~6s |
| 7.5inch HD e-Paper HAT | 7.5" | 880x528 | B/W | SPI | No | ~6s |
| 10.3inch e-Paper HAT | 10.3" | 1872x1404 | B/W (16 gray) | SPI | Yes | ~12s |
| 12.48inch e-Paper Module | 12.48" | 1304x984 | B/W | SPI | No | ~16s |

### Multi-Color Modules (Red/Black/White)

| Model | Size | Resolution | Colors | Interface | Full Refresh |
|-------|------|-----------|--------|-----------|-------------|
| 1.54inch e-Paper Module (B) | 1.54" | 200x200 | R/B/W | SPI | ~14s |
| 2.13inch e-Paper HAT (B) | 2.13" | 250x122 | R/B/W | SPI | ~15s |
| 2.7inch e-Paper HAT (B) | 2.7" | 264x176 | R/B/W | SPI | ~15s |
| 2.9inch e-Paper Module (B) | 2.9" | 296x128 | R/B/W | SPI | ~15s |
| 4.2inch e-Paper Module (B) | 4.2" | 400x300 | R/B/W | SPI | ~15s |
| 5.83inch e-Paper HAT (B) | 5.83" | 648x480 | R/B/W | SPI | ~16s |
| 7.5inch e-Paper HAT (B) | 7.5" | 800x480 | R/B/W | SPI | ~16s |
| 12.48inch e-Paper Module (B) | 12.48" | 1304x984 | R/B/W | SPI | ~35s |

### Multi-Color Modules (Yellow/Black/White and 4-Color)

| Model | Size | Resolution | Colors | Interface | Full Refresh |
|-------|------|-----------|--------|-----------|-------------|
| 2.13inch e-Paper HAT (C) | 2.13" | 250x122 | Y/B/W | SPI | ~15s |
| 4.2inch e-Paper Module (C) | 4.2" | 400x300 | Y/B/W | SPI | ~15s |
| 5.83inch e-Paper HAT (C) | 5.83" | 648x480 | Y/B/W | SPI | ~33s |
| 3inch e-Paper Module (G) | 3.0" | 400x168 | R/Y/B/W | SPI | ~25s |
| 3.5inch e-Paper Module (G) | 3.5" | 384x184 | R/Y/B/W | SPI | ~25s |
| 4.37inch e-Paper Module (G) | 4.37" | 512x368 | R/Y/B/W | SPI | ~25s |
| 7.3inch e-Paper HAT (G) | 7.3" | 800x480 | R/Y/B/W | SPI | ~28s |

### 7-Color ACeP Modules

| Model | Size | Resolution | Colors | Interface | Full Refresh |
|-------|------|-----------|--------|-----------|-------------|
| 4.01inch e-Paper HAT (F) | 4.01" | 640x400 | 7-color ACeP | SPI | ~35s |
| 5.65inch e-Paper Module (F) | 5.65" | 600x448 | 7-color ACeP | SPI | ~35s |
| 7.3inch e-Paper HAT (F) | 7.3" | 800x480 | 7-color ACeP | SPI | ~35s |

---

## Standard SPI Pinout (All E-Paper Modules)

All Waveshare e-paper modules share the same standard 8-pin SPI interface:

| Pin | Function | Description |
|-----|----------|-------------|
| VCC | Power | 3.3V input (some accept 5V via voltage translator) |
| GND | Ground | Ground |
| DIN | SPI MOSI | Serial data input |
| CLK | SPI SCK | Serial clock |
| CS | SPI CS | Chip select, low active |
| DC | Data/Command | High = data, Low = command |
| RST | Reset | Reset, low active |
| BUSY | Busy Status | Low active when display is busy refreshing |

**Important**: The BUSY pin is unique to e-paper displays (not present on LCD/OLED). Always check BUSY before sending new commands.

---

## Detailed Specifications for Key Modules

### 1.54inch e-Paper Module

- **Product URL**: https://www.waveshare.com/1.54inch-e-paper-module.htm
- **Resolution**: 200 x 200 pixels
- **Display Size**: 27.6 x 27.6 mm
- **Colors**: Black/White
- **Interface**: 3-wire SPI / 4-wire SPI
- **Partial Refresh**: Yes (~0.3s)
- **Full Refresh**: ~2s
- **Operating Voltage**: 3.3V / 5V (onboard voltage translator)
- **Viewing Angle**: >170 degrees

---

### 2.9inch e-Paper Module

- **Product URL**: https://www.waveshare.com/2.9inch-e-paper-module.htm
- **Resolution**: 296 x 128 pixels
- **Colors**: Black/White
- **Interface**: 3-wire SPI / 4-wire SPI
- **Partial Refresh**: Yes
- **Full Refresh**: ~2s
- **Operating Voltage**: 3.3V / 5V

---

### 4.2inch e-Paper Module

- **Product URL**: https://www.waveshare.com/4.2inch-e-paper-module.htm
- **Resolution**: 400 x 300 pixels
- **Colors**: Black/White
- **Interface**: 3-wire SPI / 4-wire SPI
- **Partial Refresh**: Yes
- **Full Refresh**: ~4s
- **Operating Voltage**: 3.3V / 5V

---

### 7.5inch e-Paper HAT (V2)

- **Product URL**: https://www.waveshare.com/7.5inch-e-paper-hat.htm
- **Resolution**: 800 x 480 pixels
- **Colors**: Black/White
- **Interface**: SPI
- **Partial Refresh**: No
- **Full Refresh**: ~6s
- **Operating Voltage**: 3.3V / 5V
- **Note**: V2 has higher resolution (800x480) compared to V1 (640x384)

---

### 7.3inch e-Paper HAT (F) - 7-Color ACeP

- **Product URL**: https://www.waveshare.com/7.3inch-e-paper-hat-f.htm
- **Resolution**: 800 x 480 pixels
- **Colors**: 7 colors (Black, White, Green, Blue, Red, Yellow, Orange)
- **Technology**: Advanced Color ePaper (ACeP)
- **Interface**: SPI
- **Full Refresh**: ~35s
- **Operating Voltage**: 3.3V / 5V
- **Viewing Angle**: ~180 degrees

---

## Color Variant Naming Convention

Waveshare uses a letter suffix to indicate color capability:

| Suffix | Colors | Example |
|--------|--------|---------|
| (none) | Black/White | 2.9inch e-Paper Module |
| (B) | Red/Black/White | 2.9inch e-Paper Module (B) |
| (C) | Yellow/Black/White | 4.2inch e-Paper Module (C) |
| (D) | Flexible/special variant | 2.13inch e-Paper HAT (D) |
| (F) | 7-Color ACeP | 5.65inch e-Paper Module (F) |
| (G) | Red/Yellow/Black/White (4-color) | 3.5inch e-Paper Module (G) |

---

## ESP32 Wiring Reference

### Standard E-Paper to ESP32

| E-Paper Pin | ESP32 GPIO (typical) | Notes |
|------------|---------------------|-------|
| VCC | 3.3V | 3.3V rail |
| GND | GND | Ground |
| DIN | GPIO 23 | VSPI MOSI |
| CLK | GPIO 18 | VSPI SCK |
| CS | GPIO 5 | VSPI CS |
| DC | GPIO 17 | Any free GPIO |
| RST | GPIO 16 | Any free GPIO |
| BUSY | GPIO 4 | Any free GPIO (input) |

### ESP32 E-Paper Driver Board

Waveshare provides a dedicated **e-Paper ESP32 Driver Board** that accepts various e-paper raw panels directly:
- **Product URL**: https://www.waveshare.com/e-paper-esp32-driver-board.htm
- Built-in ESP32 with WiFi/Bluetooth
- Supports local, Bluetooth, and WiFi display modes
- Compatible with SPI e-paper raw panels

---

## Operating Environment

### All E-Paper Modules

| Parameter | Black/White | Multi-Color (R/B/W, Y/B/W) | 7-Color ACeP |
|-----------|------------|---------------------------|--------------|
| Operating Temp | 0 to 40 C | 0 to 40 C | 15 to 35 C |
| Humidity | 35-65% RH | 35-65% RH | 35-65% RH |
| Storage Temp | < 30 C | < 30 C | < 30 C |
| Storage Humidity | < 55% RH | < 55% RH | < 55% RH |
| Max Storage Time | 6 months | 6 months | 6 months |

### Key Characteristics

- **No backlight** -- displays content using reflected ambient light
- **Ultra-low power** -- power consumed only during refresh
- **Content persistence** -- image remains visible indefinitely without power
- **Wide viewing angle** -- ~170-180 degrees
- **Sunlight readable** -- performs well in direct sunlight
- **Slow refresh** -- not suitable for animation or rapidly changing content

---

## Refresh Rate Comparison

| Category | Typical Refresh Time | Partial Refresh |
|----------|---------------------|-----------------|
| B/W small (1"-3") | 2s | Yes (0.3s) |
| B/W medium (3"-5") | 3-6s | Some models |
| B/W large (5"+) | 6-16s | No |
| 3-color (R/B/W) | 14-16s | No |
| 4-color (R/Y/B/W) | 25-28s | No |
| 7-color ACeP | ~35s | No |

**Note**: Multi-color displays do NOT support partial refresh. Only black/white displays support partial refresh, and even then, ghosting may accumulate and require periodic full refreshes.

---

## Software Notes

### SPI Communication

- All e-paper modules use SPI Mode 0 (CPOL=0, CPHA=0)
- Typical SPI clock: 2-4 MHz (slower than LCD modules)
- Always check BUSY pin before sending commands
- Data is transmitted MSB first

### Update Pattern

1. Initialize display (send init commands)
2. Wait for BUSY to go idle
3. Send image data
4. Trigger refresh command
5. Wait for BUSY to go idle (this takes seconds)
6. Optionally enter deep sleep mode

### Power Management

```
// Typical power consumption pattern:
// Active refresh: ~26 mW (varies by size)
// Deep sleep: < 1 uA
// After refresh, enter deep sleep to minimize power
```

### ESPHome Integration

Waveshare e-paper displays are directly supported by ESPHome with the `waveshare_epaper` component, making them popular for ESP32-based smart home displays.

---

## Recommended Modules for ESP32 Projects

| Use Case | Recommended Module | Why |
|----------|-------------------|-----|
| Battery-powered sensor tag | 1.54" e-Paper | Small, partial refresh, ultra-low power |
| Smart home dashboard | 4.2" e-Paper | Good resolution, readable size |
| Shelf price tag | 2.9" e-Paper | Common retail size, fast partial |
| Digital signage | 7.5" e-Paper HAT (V2) | Large, high resolution |
| Photo frame | 7.3" e-Paper HAT (F) | 7-color, photo-quality |
| Meeting room sign | 4.2" e-Paper (B) | Red for status indication |
| Low-power weather station | 2.13" e-Paper | Compact, partial refresh |
