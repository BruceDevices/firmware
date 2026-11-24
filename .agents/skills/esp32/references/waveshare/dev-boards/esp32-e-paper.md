# Waveshare ESP32 E-Paper / E-Ink Development Boards

> Reference for all Waveshare ESP32-based boards with e-paper (e-ink) displays.
> Source: Waveshare wiki, product pages, GitHub repos.

---

## Table of Contents

- [e-Paper ESP32 Driver Board (Classic)](#e-paper-esp32-driver-board)
- [ESP32-S3-ePaper-1.54](#esp32-s3-epaper-154)
- [ESP32-S3-ePaper-3.97](#esp32-s3-epaper-397)
- [ESP32-S3-PhotoPainter](#esp32-s3-photopainter)
- [E-Paper Development Tips](#e-paper-development-tips)

---

## e-Paper ESP32 Driver Board

### Overview

| Property | Value |
|----------|-------|
| Product Name | e-Paper ESP32 Driver Board |
| Chip | ESP32 (classic, not S3) |
| Flash | 4 MB |
| PSRAM | None |
| Processor | Xtensa LX6 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BT 4.2 + BLE |
| USB Connector | Micro USB |

### Key Features

- **Universal** e-Paper driver -- supports various Waveshare SPI e-Paper raw panels
- Three demo modes: Local, Bluetooth, and WiFi
- Image processing algorithms: Level and Dithering
- Designed for wireless e-Paper content refresh

### Compatible E-Paper Panels

Supports most Waveshare SPI e-Paper raw panels including:
- 1.54", 2.13", 2.7", 2.9", 4.2", 5.65", 7.5" and larger
- Black/White, Black/White/Red, and Black/White/Yellow variants
- Various resolutions

### Use Cases

- Supermarket price tags
- E-cards / digital signage
- Serial port information monitoring
- Low-power wireless displays
- IoT dashboards

### Development Setup

1. Copy `esp32-waveshare-epd` folder to Arduino IDE `libraries` directory
2. Close and reopen Arduino IDE
3. Select board: ESP32 Dev Module
4. Choose example demo matching your e-Paper panel size

### Firmware Gotchas

- Classic ESP32 (LX6), NOT ESP32-S3. Different board selection in Arduino IDE.
- Micro USB (not Type-C) for power and programming.
- No PSRAM -- image processing must fit in SRAM.
- E-Paper refresh is slow (seconds) -- not suitable for real-time display.
- WiFi and Bluetooth demos use image processing on the ESP32 side.
- Dithering algorithm produces better gradients on B/W panels.
- Level algorithm produces sharper edges for text and line art.

---

## ESP32-S3-ePaper-1.54

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-ePaper-1.54 |
| SoC | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | 8 MB (stacked package) |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.54 inch |
| Type | E-Paper (E-Ink) |
| Resolution | 200 x 200 |
| Colors | Black / White |
| Interface | SPI |
| Viewing Angle | Wide (nearly 180 degrees) |
| Contrast | High |
| Power in Display Hold | Zero (bistable) |

### Sensors & Peripherals

| Component | Model | Interface | Function |
|-----------|-------|-----------|----------|
| Temperature/Humidity | SHTC3 | I2C | Environmental monitoring |
| RTC | Onboard | I2C | Real-time clock for timed updates |
| Audio Codec | Low-power | I2S | Voice capture and playback |
| TF Card | MicroSD slot | SPI | External storage for images/data |
| Battery | Li-ion charge management | -- | Portable operation |

### Key Features

- AIoT (AI + IoT) development board
- WiFi + BLE dual-mode communication
- AI voice interaction capability
- Ultra-low power display (e-Paper holds image with zero power)
- Environmental sensing (temperature + humidity)

### Firmware Gotchas

- E-Paper refresh time: ~2-3 seconds for full refresh, ~0.3s for partial.
- 200x200 is small but sufficient for IoT status, QR codes, simple text.
- SHTC3 provides environmental data for smart label applications.
- Audio codec enables voice-triggered display updates.
- RTC enables scheduled display updates without WiFi connection.
- Battery + e-Paper = very long battery life (weeks to months with periodic updates).

---

## ESP32-S3-ePaper-3.97

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-ePaper-3.97 |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB |
| ROM | 384 KB |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 3.97 inch |
| Type | E-Paper (E-Ink) |
| Resolution | 800 x 480 |
| Colors | Black / White |
| Interface | SPI |
| Viewing Angle | Wide |
| Contrast | High |

### Sensors & Peripherals

| Component | Model | Interface | Function |
|-----------|-------|-----------|----------|
| 6-axis IMU | QMI8658 | I2C | Accelerometer + Gyroscope |
| Temperature/Humidity | SHTC3 | I2C | Environmental monitoring |
| RTC | Onboard | I2C | Real-time clock |
| Audio Codec | Low-power | I2S | Voice capture and playback |
| Microphone | Onboard | I2S | Voice input |
| TF Card | MicroSD slot | SPI | External storage |
| Battery | Li-ion charge management | -- | Portable operation |

### Key Features

- Larger e-Paper display suitable for detailed content
- 800x480 resolution -- sufficient for text, charts, images
- AI voice interaction (codec + mic)
- Motion detection via IMU
- Environmental sensing

### Firmware Gotchas

- 800x480 B/W e-Paper: frame buffer = 48 KB (1 bit per pixel). Fits easily in SRAM.
- Full refresh can take 3-5 seconds on larger panels.
- Partial refresh available for specific regions -- much faster.
- IMU can trigger display updates on motion events.
- SHTC3 data can be displayed on e-Paper for environmental monitoring stations.
- Dithering needed to display grayscale images on B/W panel.

---

## ESP32-S3-PhotoPainter

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-PhotoPainter |
| Module | ESP32-S3-WROOM-1-N16R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 7.3 inch |
| Type | E-Paper (E-Ink) |
| Technology | E Ink Spectra 6 (E6) |
| Resolution | 800 x 480 |
| Colors | 6 primary colors (full color) |
| Color Rendering | Dithering technology for extended palette |
| Interface | SPI |

### E Ink Spectra 6 Color Details

Unlike standard B/W e-Paper, the Spectra 6 technology supports six primary colors:
- Black
- White
- Red
- Yellow
- Blue
- Green

Dithering technology blends these primaries to display a much wider apparent color range.

### Sensors & Peripherals

| Component | Model | Interface | Function |
|-----------|-------|-----------|----------|
| Temperature/Humidity | SHTC3 | I2C | Environmental monitoring |
| RTC | Onboard | I2C | Scheduled updates |
| Audio Codec | Onboard | I2S | Voice capture and playback |
| TF Card | MicroSD slot | SPI | Image storage |
| Battery | Li-ion charge management (optional) | -- | Portable operation |

### Physical Design

- Comes with **solid wood photo frame**
- Designed as an e-Paper digital photo frame
- Ultra-long standby (e-Paper holds image with zero power)
- Wall-mountable or tabletop stand

### Firmware Gotchas

- **Full-color e-Paper refresh is SLOW** -- typically 15-30 seconds for full update.
- 6-color images require special dithering algorithms (Floyd-Steinberg or similar).
- Source images should be converted to the 6-color palette before display.
- TF card useful for storing multiple images for slideshow mode.
- RTC enables automatic scheduled image rotation.
- WiFi allows remote image updates without physical access.
- GitHub repository: `waveshareteam/ESP32-S3-PhotoPainter`
- Battery optional -- can be USB-powered for permanent installation.

---

## E-Paper Development Tips

### Refresh Types

| Type | Speed | Usage | Quality |
|------|-------|-------|---------|
| Full Refresh | 2-5s (B/W), 15-30s (color) | Complete redraw | Best, no ghosting |
| Partial Refresh | 0.3-1s | Update specific region | Good, some ghosting over time |
| Fast Refresh | 0.1-0.3s | Quick update | Lower contrast, more ghosting |

### Power Consumption Profile

| State | Current | Notes |
|-------|---------|-------|
| Display holding image | 0 mA | Bistable -- no power needed |
| WiFi active | ~120-180 mA | During data transfer |
| Display refreshing | ~40-60 mA | During e-Paper update |
| Deep sleep | ~10-20 uA | ESP32-S3 deep sleep |

### Battery Life Estimation

For a typical 1000 mAh Li-ion battery updating once per hour:
- Active time per update: ~5 seconds
- Average current during update: ~150 mA
- Deep sleep current: ~20 uA
- Estimated battery life: **2-4 weeks** (B/W), **1-2 weeks** (color)

### Image Preparation

- **B/W panels**: Convert to 1-bit bitmap, apply dithering for grayscale images
- **Color panels**: Convert to nearest 6-color palette with dithering
- **Resolution matching**: Scale images to exact panel resolution
- **Waveshare provides** image conversion tools in their demo code

### Common I2C Addresses on E-Paper Boards

| Device | Address | Found On |
|--------|---------|----------|
| SHTC3 (temp/humidity) | 0x70 | ePaper-1.54, ePaper-3.97, PhotoPainter |
| QMI8658 (IMU) | 0x6A/0x6B | ePaper-3.97 |
| RTC (PCF85063 or similar) | 0x51 | All S3 ePaper boards |
| Audio Codec | Varies | ePaper-1.54, ePaper-3.97, PhotoPainter |

### Board Selection Guide

| Use Case | Recommended Board | Why |
|----------|------------------|-----|
| Universal e-Paper prototyping | e-Paper ESP32 Driver Board | Supports all SPI panels |
| Small IoT label/tag | ESP32-S3-ePaper-1.54 | Compact, sensors, voice |
| Information display | ESP32-S3-ePaper-3.97 | Large B/W, 800x480 |
| Digital photo frame | ESP32-S3-PhotoPainter | Full color, wood frame |
| Battery-powered signage | ESP32-S3-ePaper-3.97 | Good size + long battery life |
| Environmental monitor | ESP32-S3-ePaper-1.54 | SHTC3 sensor + e-Paper |
