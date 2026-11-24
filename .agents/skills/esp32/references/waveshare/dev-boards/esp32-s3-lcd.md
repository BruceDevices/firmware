# Waveshare ESP32-S3 LCD Boards (Without Touch)

> Reference for ESP32-S3 boards with LCD displays but NO touch capability.
> Source: Waveshare wiki, product pages, community configs.

---

## Table of Contents

- [ESP32-S3-LCD-1.28 / 1.28-B](#esp32-s3-lcd-128)
- [ESP32-S3-LCD-1.47 / 1.47B](#esp32-s3-lcd-147)
- [ESP32-S3-LCD-1.69](#esp32-s3-lcd-169)
- [ESP32-S3-LCD-1.85](#esp32-s3-lcd-185)
- [ESP32-S3-LCD-2](#esp32-s3-lcd-2)
- [ESP32-S3-LCD-Driver-Board](#esp32-s3-lcd-driver-board)

---

## ESP32-S3-LCD-1.28

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-LCD-1.28 |
| Variant | -B adds CNC metal case |
| SoC | ESP32-S3R2 |
| Flash | 16 MB (external) |
| PSRAM | 2 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB |
| ROM | 384 KB |
| WiFi | 802.11 b/g/n (2.4 GHz) |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |
| Battery | 3.7V Li-ion via header |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.28 inch (round) |
| Type | IPS LCD |
| Resolution | 240 x 240 |
| Colors | 65K |
| Driver IC | GC9A01 |
| Interface | 4-wire SPI |

### Sensors

| Component | Model | Interface |
|-----------|-------|-----------|
| 6-axis IMU | QMI8658 | I2C |

### Key Differences from Touch Variant

- No touch controller (CST816S absent)
- Fewer components, lower cost
- Same display, same SoC, same pinout for LCD
- I2C bus only carries IMU traffic

### GPIO Pinout

Same LCD pin mapping as the touch variant:

| GPIO | Function |
|------|----------|
| GPIO2 | LCD Backlight (PWM) |
| GPIO6 | I2C SDA (IMU only) |
| GPIO7 | I2C SCL (IMU only) |
| GPIO8 | LCD_DC |
| GPIO9 | LCD_CS |
| GPIO10 | LCD_CLK |
| GPIO11 | LCD_MOSI |
| GPIO14 | LCD_RST |
| GPIO43 | UART_TXD |
| GPIO44 | UART_RXD |

### Firmware Gotchas

- GC9A01 is a round display controller -- 240x240 with circular active area.
- TFT_eSPI users: set `USER_SETUP_ID` for GC9A01, set correct SPI pins.
- Some community reports of incorrect default pin configs in TFT_eSPI -- verify against table above.
- 2MB PSRAM limits frame buffer options -- use partial rendering in LVGL.

---

## ESP32-S3-LCD-1.47

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-LCD-1.47 |
| Variant | -B: Type B variant |
| SoC | ESP32-S3 (8MB PSRAM) |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.47 inch |
| Type | IPS LCD |
| Resolution | 172 x 320 |
| Colors | 262K |
| Driver IC | ST7789 |
| Interface | SPI |

### Additional Features

- Onboard colorful RGB LED (WS2812 or similar)
- Most GPIOs adapted for expansion
- LVGL-capable

### GPIO Pin Mapping (LCD SPI)

| GPIO | Function |
|------|----------|
| GPIO7 | LCD_SCL (SPI Clock) |
| GPIO6 | LCD_SDA (SPI MOSI) |
| GPIO14 | LCD_CS |
| GPIO15 | LCD_DC |
| GPIO21 | LCD_RST |
| GPIO22 | LCD_BL (Backlight) |

### Firmware Gotchas

- ST7789 at 172x320 requires display offset (typically X=34, Y=0).
- RGB LED can be used for status indication alongside display.
- -B variant may have different pin assignments -- verify against specific wiki page.

---

## ESP32-S3-LCD-1.69

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-LCD-1.69 |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |
| Battery | 3.7V Li-ion support |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.69 inch |
| Type | IPS LCD |
| Resolution | 240 x 280 |
| Colors | 262K |
| Driver IC | ST7789V2 |
| Interface | 4-wire SPI |

### Sensors

| Component | Model | Interface |
|-----------|-------|-----------|
| 6-axis IMU | QMI8658 | I2C |

### Key Differences from Touch Variant

- No CST816T touch controller
- I2C bus only carries IMU
- Otherwise identical hardware platform

### Firmware Gotchas

- Same display init as touch variant.
- No touch interrupt to handle -- simplifies event loop.

---

## ESP32-S3-LCD-1.85

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-LCD-1.85 |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.85 inch (round) |
| Type | IPS LCD |
| Resolution | 360 x 360 |
| Colors | 262K |
| Driver IC | ST77916 |
| Interface | QSPI |

### Sensors & Peripherals

| Component | Model | Interface |
|-----------|-------|-----------|
| 6-axis IMU | QMI8658 | I2C |
| Audio Codec | Onboard | I2S |
| Microphone | Onboard | I2S |
| TF Card | MicroSD slot | SPI |
| RTC | Onboard | I2C |

### Firmware Gotchas

- ST77916 uses QSPI interface -- 4 data lines, not standard SPI.
- Round 360x360 display -- circular clipping needed.
- No touch -- display-only output with audio I/O capability.
- ESPHome community has working configs for this board.

---

## ESP32-S3-LCD-2

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-LCD-2 |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |
| Battery | 3.7V Li-ion support |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 2.0 inch |
| Type | IPS LCD |
| Resolution | 240 x 320 |
| Colors | 262K |
| Driver IC | ST7789T3 |
| Interface | SPI |

### Sensors & Peripherals

| Component | Model | Interface |
|-----------|-------|-----------|
| 6-axis IMU | QMI8658 | I2C |
| Camera | 24-pin DVP connector | DVP |

### Key Features

- Camera interface (DVP) compatible with OV2640 and OV5640
- Battery charge/discharge management
- No touch controller

### Firmware Gotchas

- Camera DVP interface available for image capture without touch.
- Same LCD driver as Touch-LCD-2 (ST7789T3).
- Good for camera + display applications (viewfinder, etc.).

---

## ESP32-S3-LCD-Driver-Board

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-LCD-Driver-Board |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Purpose | Universal LCD driver for external Waveshare displays |

### Key Features

- Designed to connect to various external Waveshare LCD modules.
- Provides standard FPC connectors for display modules.
- Multiple interface support (SPI, QSPI, RGB).
- Flexible platform for testing different display sizes.

### Firmware Gotchas

- Pin mapping depends on which external display is connected.
- Refer to the specific display module documentation for pin assignments.
- Most flexible option for display prototyping.

---

## Comparison: LCD vs Touch Variants

For boards that exist in both LCD-only and Touch variants:

| Size | LCD-Only Model | Touch Model | Touch Controller Added | Price Difference |
|------|---------------|-------------|----------------------|-----------------|
| 1.28" | ESP32-S3-LCD-1.28 | ESP32-S3-Touch-LCD-1.28 | CST816S | ~$2-3 |
| 1.47" | ESP32-S3-LCD-1.47 | ESP32-S3-Touch-LCD-1.47 | Capacitive | ~$2-3 |
| 1.69" | ESP32-S3-LCD-1.69 | ESP32-S3-Touch-LCD-1.69 | CST816T | ~$2-3 |
| 1.85" | ESP32-S3-LCD-1.85 | ESP32-S3-Touch-LCD-1.85 | Capacitive | ~$2-3 |
| 2.0" | ESP32-S3-LCD-2 | ESP32-S3-Touch-LCD-2 | CST816D | ~$2-3 |

### When to Choose LCD-Only

- Lower cost
- Simpler firmware (no touch driver)
- Output-only displays (clocks, dashboards, sensor readouts)
- Fewer I2C devices on shared bus
- Button/encoder-based UI instead of touch
