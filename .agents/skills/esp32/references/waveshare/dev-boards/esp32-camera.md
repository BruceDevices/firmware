# Waveshare ESP32 Camera and Thermal Imaging Boards

> Reference for all Waveshare ESP32-based boards with camera or thermal imaging capabilities.
> Source: Waveshare wiki, product pages, community configs.

---

## Table of Contents

- [ESP32-S3-CAM-OV5640](#esp32-s3-cam-ov5640)
- [Thermal Camera ESP32 Module (45deg / 90deg)](#thermal-camera-esp32-module)
- [Boards with Camera Interfaces](#boards-with-camera-interfaces)

---

## ESP32-S3-CAM-OV5640

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-CAM-OV5640 |
| SoC | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB |
| ROM | 384 KB |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Camera Specifications

| Property | Value |
|----------|-------|
| Camera Sensor | OV5640 |
| Resolution | 5 MP (2592 x 1944) |
| Interface | 24-pin DVP |
| Compatible Cameras | OV3660, OV5640, GC0308, GC2145 |
| Features | Auto-focus, multiple resolution modes |

### Display Interface

| Property | Value |
|----------|-------|
| Connector | 18-pin FPC |
| Compatible Displays | 3.5", 2.8", 2", 1.83" Waveshare display modules |
| Interface | SPI / QSPI (depends on display) |

### Audio

| Component | Details |
|-----------|---------|
| Microphones | Dual digital MEMS |
| Speaker | Onboard amplifier |
| Processing | Echo cancellation circuit |
| Interface | I2S |

### Use Cases

- Image capture and recognition
- AI speech interaction
- Edge vision detection (object detection, face recognition)
- HMI display applications
- Smart cameras / video surveillance
- Real-time multi-object detection

### Firmware Gotchas

- DVP camera interface consumes many GPIOs -- limited remaining pins.
- OV5640 at full 5MP resolution requires significant PSRAM bandwidth.
- For video streaming, use lower resolutions (VGA/QVGA) for better frame rates.
- External display connects via 18-pin FPC -- select compatible Waveshare display module.
- Dual microphone array enables voice interaction alongside camera.
- ESP-WHO and ESP-DL frameworks support face detection/recognition on this board.

---

## Thermal Camera ESP32 Module

### Overview

| Property | Value |
|----------|-------|
| Product Name | Thermal Camera ESP32 Module |
| Variants | Thermal-45 (45deg FOV), Thermal-90 (90deg FOV) |
| Module | ESP32-S3-WROOM-1 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB |
| ROM | 384 KB |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Thermal Sensor Specifications

| Property | Value |
|----------|-------|
| Technology | Hybrid microbolometer + thermopile |
| Resolution | 80 x 62 pixels |
| Refresh Rate | Up to 25 FPS |
| Accuracy | ±1 degree C |
| FOV (45deg variant) | 45 degrees |
| FOV (90deg variant) | 90 degrees (wide angle) |

### GPIO & Expansion

| Property | Value |
|----------|-------|
| GPIO Header | 20-pin with 14 GPIOs |
| I2C | Available on header |
| UART | Available on header |
| Battery | 3.7V Li-ion via header (optional) |

### Variant Comparison

| Property | Thermal-45 | Thermal-90 |
|----------|-----------|-----------|
| FOV | 45 degrees | 90 degrees |
| Best For | Focused measurement | Wide area scanning |
| Price | ~$87 | ~$87 |

### Firmware Gotchas

- Thermal sensor data is 80x62 array -- small enough to process on ESP32-S3.
- ±1 degree C accuracy is industrial-grade for this price point.
- 25 FPS is sufficient for real-time thermal imaging display.
- Use pseudo-color mapping (iron bow, rainbow, etc.) for visualization.
- WiFi streaming of thermal data is feasible at this resolution.
- Battery header allows portable/handheld operation.
- 14 GPIOs available for additional sensors or peripherals.

---

## Boards with Camera Interfaces

Several other Waveshare ESP32-S3 boards include camera connectors (24-pin DVP) but are primarily display or connectivity boards:

| Board | Primary Purpose | Camera Connector | Compatible Cameras |
|-------|----------------|-----------------|-------------------|
| ESP32-S3-Touch-LCD-2 | 2" touch display | 24-pin DVP | OV2640, OV5640 |
| ESP32-S3-LCD-2 | 2" display (no touch) | 24-pin DVP | OV2640, OV5640 |
| ESP32-S3-Touch-LCD-3.5 | 3.5" touch display | 24-pin DVP | OV2640, OV5640 |
| ESP32-S3-Touch-LCD-3.5B | 3.5" touch display (QSPI) | 24-pin DVP | OV2640, OV5640 |
| ESP32-S3-Touch-LCD-3.5-C | 3.5" touch display | 24-pin DVP + OV5640 included | OV5640 (included) |
| ESP32-S3-ETH | Ethernet dev board | 24-pin DVP | OV2640, OV5640 |
| ESP32-S3-A7670E-4G | 4G cellular board | 24-pin DVP | OV2640, OV5640 |

### Camera Selection Guide

| Camera | Resolution | Interface | Best For |
|--------|-----------|-----------|----------|
| OV2640 | 2 MP (1600x1200) | DVP | Basic imaging, lower cost |
| OV5640 | 5 MP (2592x1944) | DVP | Higher quality, auto-focus |
| OV3660 | 3 MP | DVP | Mid-range option |
| GC0308 | VGA (640x480) | DVP | Ultra-low-cost, basic |
| GC2145 | 2 MP | DVP | Budget alternative to OV2640 |

### Camera DVP Pin Considerations

When using the DVP camera interface, be aware:

- DVP uses 8-10 data pins + PCLK + HREF + VSYNC + XCLK + PWDN + RESET = 14-16 GPIOs
- This significantly limits available GPIOs for other peripherals
- Boards with both camera DVP and display may have GPIO conflicts
- When using camera + display simultaneously, prefer SPI/QSPI displays (fewer pins)
- RGB parallel displays (4"+) are generally NOT compatible with simultaneous camera use due to GPIO exhaustion
