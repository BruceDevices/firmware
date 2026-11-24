# Waveshare ESP32-S3 Touch LCD Development Boards

> Comprehensive reference for all ESP32-S3 boards with touch-enabled LCD or AMOLED displays.
> Source: Waveshare wiki, product pages, community ESPHome/Arduino configs, schematics.

---

## Table of Contents

- [ESP32-S3-Touch-LCD-1.28](#esp32-s3-touch-lcd-128)
- [ESP32-S3-Touch-LCD-1.47](#esp32-s3-touch-lcd-147)
- [ESP32-S3-Touch-LCD-1.69](#esp32-s3-touch-lcd-169)
- [ESP32-S3-Touch-LCD-1.85](#esp32-s3-touch-lcd-185)
- [ESP32-S3-Touch-LCD-2](#esp32-s3-touch-lcd-2)
- [ESP32-S3-Touch-LCD-2.1](#esp32-s3-touch-lcd-21)
- [ESP32-S3-Touch-LCD-2.8](#esp32-s3-touch-lcd-28)
- [ESP32-S3-Touch-LCD-2.8C](#esp32-s3-touch-lcd-28c)
- [ESP32-S3-Touch-LCD-3.49](#esp32-s3-touch-lcd-349)
- [ESP32-S3-Touch-LCD-3.5 / 3.5B](#esp32-s3-touch-lcd-35)
- [ESP32-S3-Touch-LCD-4 / 4B](#esp32-s3-touch-lcd-4)
- [ESP32-S3-Touch-LCD-4.3 / 4.3B / 4.3C](#esp32-s3-touch-lcd-43)
- [ESP32-S3-Touch-LCD-5](#esp32-s3-touch-lcd-5)
- [ESP32-S3-Touch-LCD-7 / 7B](#esp32-s3-touch-lcd-7)
- [ESP32-S3 Touch AMOLED Series](#esp32-s3-touch-amoled-series)

---

## ESP32-S3-Touch-LCD-1.28

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-1.28 |
| Variant | -B adds CNC metal case |
| SoC | ESP32-S3R2 |
| Flash | 16 MB (external) |
| PSRAM | 2 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB |
| ROM | 384 KB |
| WiFi | 802.11 b/g/n (2.4 GHz) |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C (via CH343P USB-UART bridge) |
| Battery | 3.7V Li-ion via MX1.25 header |
| Battery Measurement | GPIO1 (voltage divider: 200k + 100k) |
| Dimensions | Compact wearable form factor |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.28 inch (round) |
| Type | IPS LCD |
| Resolution | 240 x 240 |
| Colors | 65K |
| Driver IC | GC9A01 |
| Interface | 4-wire SPI (up to 80 MHz) |
| Backlight Control | GPIO2 (PWM) |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | CST816S |
| Interface | I2C (shared bus) |
| Touch Points | Single touch |

### Sensors

| Sensor | Model | Interface | Function |
|--------|-------|-----------|----------|
| 6-axis IMU | QMI8658 | I2C (shared) | 3-axis accelerometer + 3-axis gyroscope |

### GPIO Pinout Table

| GPIO | Function | Notes |
|------|----------|-------|
| GPIO0 | BOOT button | Active LOW, strapping pin |
| GPIO1 | BAT_ADC | Battery voltage via divider (200k/100k) |
| GPIO2 | LCD_BL | Backlight PWM control |
| GPIO4 | MOSFET_1 | External switch control |
| GPIO5 | MOSFET_2 | External switch control |
| GPIO6 | I2C_SDA | Shared: touch (CST816S) + IMU (QMI8658) |
| GPIO7 | I2C_SCL | Shared: touch (CST816S) + IMU (QMI8658) |
| GPIO8 | LCD_DC | SPI Data/Command select |
| GPIO9 | LCD_CS | SPI Chip Select |
| GPIO10 | LCD_CLK | SPI Clock |
| GPIO11 | LCD_MOSI | SPI Data Out |
| GPIO14 | LCD_RST | LCD Reset |
| GPIO43 | UART_TXD | USB-UART via CH343P |
| GPIO44 | UART_RXD | USB-UART via CH343P |

### External Connector (SH1.0)

6 GPIOs exposed via SH1.0 connector for external peripherals (configurable as I2C, SPI, etc.). VSYS pin provides 5V input.

### Firmware Gotchas

- Uses CH343P for USB-UART -- not native USB. Flash via UART at 115200 or higher.
- I2C bus is shared between touch controller and IMU -- use appropriate addresses.
- CST816S touch I2C address: 0x15.
- QMI8658 I2C address: 0x6A or 0x6B (depending on SDO pin).
- Battery ADC requires calibration; raw ADC value must be multiplied by 3 (divider ratio).
- SPI LCD can run at 80 MHz for smooth LVGL rendering.

---

## ESP32-S3-Touch-LCD-1.47

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-1.47 |
| SoC | ESP32-S3 (8MB PSRAM variant) |
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

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Interface | I2C |

### Additional Features

- Onboard TF card slot
- Full-speed USB port
- Most GPIOs exposed for expansion

### Firmware Gotchas

- ST7789 requires proper initialization sequence for 172x320 offset.
- Display offset typically X=34, Y=0 for correct rendering.

---

## ESP32-S3-Touch-LCD-1.69

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-1.69 |
| SoC | ESP32-S3R8 |
| Flash | 16 MB (external) |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB |
| ROM | 384 KB |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |
| Battery | 3.7V Li-ion via MX1.25 header |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 1.69 inch |
| Type | IPS LCD |
| Resolution | 240 x 280 |
| Colors | 262K |
| Driver IC | ST7789V2 |
| Interface | 4-wire SPI |
| Color Formats | RGB444, RGB565, RGB666 |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | CST816T |
| Interface | I2C |

### Sensors & Peripherals

| Component | Model | Interface | Function |
|-----------|-------|-----------|----------|
| 6-axis IMU | QMI8658 | I2C | Accelerometer + Gyroscope |
| RTC | PCF85063 | I2C | Real-time clock (SH1.0 battery header, supports charging) |

### Additional Features

- RST, BOOT, and a programmable PWM button
- Reserved pads for 4x GPIO, I2C, and UART interface
- Onboard Li-ion battery charge/discharge management

### Firmware Gotchas

- ST7789V2 supports 12/16/18-bit color input (RGB444/RGB565/RGB666).
- PCF85063 RTC has separate battery header -- ensure battery is connected for RTC backup.
- CST816T I2C address: 0x15.

---

## ESP32-S3-Touch-LCD-1.85

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-1.85 |
| Variants | -C variant: Smart speaker box with case |
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
| Size | 1.85 inch (round) |
| Type | IPS LCD |
| Resolution | 360 x 360 |
| Colors | 262K |
| Driver IC | ST77916 |
| Interface | QSPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Interface | I2C |
| Note | Touch version only |

### Sensors & Peripherals

| Component | Model | Interface | Function |
|-----------|-------|-----------|----------|
| 6-axis IMU | QMI8658 | I2C | Accelerometer + Gyroscope |
| RTC | PCF85063 | I2C | Real-time clock |
| Audio Codec | Onboard | I2S | Audio decode, MIC support |
| TF Card | MicroSD slot | SPI | External storage |

### Firmware Gotchas

- ST77916 uses QSPI (4 data lines) -- different from standard SPI displays.
- Round display requires circular clipping mask for UI rendering.
- The -C variant adds speaker and case, same electrical connections.

---

## ESP32-S3-Touch-LCD-2

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-2 |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |
| Battery | 3.7V Li-ion via MX1.25 header |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 2.0 inch |
| Type | IPS LCD |
| Resolution | 240 x 320 |
| Colors | 262K |
| Driver IC | ST7789T3 |
| Interface | SPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | CST816D |
| Interface | I2C |

### Sensors & Peripherals

| Component | Model | Interface | Function |
|-----------|-------|-----------|----------|
| 6-axis IMU | QMI8658 | I2C | Accelerometer + Gyroscope |
| Camera interface | 24-pin DVP | DVP | OV2640/OV5640 compatible |

### Additional Features

- Li-ion battery charge/discharge management
- Camera connector (DVP) for OV2640/OV5640

### Firmware Gotchas

- Camera and display share some GPIO resources -- check for conflicts.
- ST7789T3 is pin-compatible with ST7789V2 in most configurations.

---

## ESP32-S3-Touch-LCD-2.1

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-2.1 |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 2.1 inch (round) |
| Type | IPS LCD |
| Resolution | 480 x 480 |
| Colors | 262K |
| Driver IC | ST7701S |
| Interface | RGB Parallel |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Interface | I2C with interrupt |

### Sensors & Peripherals

| Component | Model | Interface | Function |
|-----------|-------|-----------|----------|
| 6-axis IMU | QMI8658 | I2C | Accelerometer + Gyroscope |
| RTC | Onboard | I2C | Real-time clock |
| TF Card | MicroSD slot | SPI | External storage |
| Battery | 3.7V Li-ion | MX1.25 | Charge management |

### Firmware Gotchas

- ST7701S uses RGB parallel interface -- consumes many GPIOs for data bus.
- Round 480x480 display requires circular mask in LVGL.
- RGB interface requires continuous refresh from ESP32-S3 DMA -- CPU overhead.

---

## ESP32-S3-Touch-LCD-2.8

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-2.8 |
| SoC | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 2.8 inch |
| Type | IPS LCD |
| Resolution | 240 x 320 |
| Colors | 262K |
| Driver IC | ST7789 |
| Interface | SPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Interface | I2C |
| Points | 5-point with interrupt |

### Additional Features

- UART, I2C, and GPIO expansion interfaces
- Full-speed USB port
- Onboard antenna

---

## ESP32-S3-Touch-LCD-2.8C

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-2.8C |
| SoC | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 2.8 inch (round) |
| Type | IPS LCD |
| Resolution | 480 x 480 |
| Colors | 262K |
| Driver IC | ST77916 |
| Interface | QSPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | CST816 |
| Interface | I2C |

### Firmware Gotchas

- ST77916 QSPI interface requires 4 data lines -- not standard SPI.
- Round display; use circular clipping.
- Different from the 2.8 (non-C) which uses standard SPI ST7789 at 240x320.

---

## ESP32-S3-Touch-LCD-3.49

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-3.49 |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 3.49 inch (bar/elongated) |
| Type | IPS LCD |
| Resolution | 172 x 640 |
| Colors | 16.7M |
| Driver IC | AXS15231B |
| Interface | QSPI |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | AXS15231B (integrated) |
| Interface | I2C |
| Note | Display and touch share the same IC |

### Sensors

| Component | Model | Interface | Function |
|-----------|-------|-----------|----------|
| 6-axis IMU | QMI8658 | I2C | Accelerometer + Gyroscope |
| Dual Microphone Array | Onboard | I2S | Noise reduction + echo cancellation |

### Firmware Gotchas

- AXS15231B is an integrated display+touch controller -- simplifies driver code.
- Bar-type aspect ratio (172:640) requires custom UI layouts.
- QSPI interface provides fast refresh rates.

---

## ESP32-S3-Touch-LCD-3.5

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-3.5 |
| Variants | -B: QSPI interface; -C: includes OV5640 camera |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 3.5 inch |
| Type | IPS LCD |
| Resolution | 320 x 480 |
| Colors | 262K |
| Driver IC | ST7796 |
| Interface | SPI (standard) / QSPI (B variant) |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | FT6336 |
| Interface | I2C |

### Sensors & Peripherals

| Component | Model | Interface | Function |
|-----------|-------|-----------|----------|
| 6-axis IMU | QMI8658 | I2C | Accelerometer + Gyroscope |
| Power Management | Onboard | -- | Battery charge management |
| RTC | Onboard | I2C | Real-time clock |
| Audio Codec | Low-power | I2S | Audio support |
| Camera | 24-pin DVP connector | DVP | OV2640/OV5640 (camera included with -C variant) |

### Expansion

- 2.54mm pitch GPIO header for available IO pins

### Firmware Gotchas

- Standard variant uses SPI; B variant uses QSPI for faster display updates.
- FT6336 supports 2-point touch.
- Camera (DVP) shares GPIO with some other peripherals -- check pin conflicts.
- OV5640 only included with the -C SKU; connector present on all variants.

---

## ESP32-S3-Touch-LCD-4

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-4 |
| Variants | -B: Smart 86 wall-mount box |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 4.0 inch |
| Type | IPS LCD |
| Resolution | 480 x 480 |
| Colors | 65K |
| Driver IC | ST7701S |
| Interface | RGB Parallel |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | GT911 |
| Interface | I2C |
| Points | 5-point with interrupt |

### Peripheral Interfaces

| Interface | Details |
|-----------|---------|
| CAN | Onboard transceiver |
| RS485 | Onboard transceiver |
| I2C | Expansion header |
| TF Card | MicroSD slot |
| USB | Full-speed USB Type-C |

### Firmware Gotchas

- ST7701S RGB parallel interface consumes most GPIOs.
- GT911 I2C address: 0x5D or 0x14 (configurable via reset sequence).
- The -B variant (Smart 86 Box) is designed for wall-mount installation with 86mm standard box.
- CAN and RS485 interfaces available for industrial applications.

---

## ESP32-S3-Touch-LCD-4.3

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-4.3 |
| Variants | -B: improved variant; -C: AI voice with dual MIC |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB (OPI) |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB |
| ROM | 384 KB |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 4.3 inch |
| Type | IPS LCD |
| Resolution | 800 x 480 |
| Colors | 65K (RGB565) |
| Driver IC | EK9716 |
| Interface | 16-bit RGB Parallel (RGB565) |
| Pixel Clock | 16 MHz preferred |
| HSYNC Polarity | 0 (active low) |
| VSYNC Polarity | 0 (active low) |
| PCLK Active Neg | 1 |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | GT911 |
| Interface | I2C |
| I2C Address | 0x5D or 0x14 |
| Points | 5-point with interrupt |

### IO Expander

The board uses **CH422G** I2C IO expander because the RGB parallel display consumes most ESP32-S3 GPIOs.

| CH422G Pin | Function | Description |
|------------|----------|-------------|
| EXIO1 | TP_RST | Touch panel reset |
| EXIO2 | LCD_BL | LCD backlight control |
| EXIO3 | LCD_RST | LCD reset |
| EXIO4 | SD_CS | TF card chip select |
| EXIO5 | USB_SEL | USB path select |

CH422G I2C address range: **0x20-0x27, 0x30-0x3F** (avoid these addresses for other I2C devices).

### Complete LCD RGB Pin Mapping

| Signal | GPIO | Description |
|--------|------|-------------|
| LCD_DE | GPIO5 | Display Enable |
| LCD_VSYNC | GPIO3 | Vertical Sync |
| LCD_HSYNC | GPIO46 | Horizontal Sync |
| LCD_PCLK | GPIO7 | Pixel Clock |
| R0 | GPIO1 | Red bit 0 (LSB) |
| R1 | GPIO2 | Red bit 1 |
| R2 | GPIO42 | Red bit 2 |
| R3 | GPIO41 | Red bit 3 |
| R4 | GPIO40 | Red bit 4 (MSB) |
| G0 | GPIO39 | Green bit 0 (LSB) |
| G1 | GPIO0 | Green bit 1 |
| G2 | GPIO45 | Green bit 2 |
| G3 | GPIO48 | Green bit 3 |
| G4 | GPIO47 | Green bit 4 |
| G5 | GPIO21 | Green bit 5 (MSB) |
| B0 | GPIO14 | Blue bit 0 (LSB) |
| B1 | GPIO38 | Blue bit 1 |
| B2 | GPIO18 | Blue bit 2 |
| B3 | GPIO17 | Blue bit 3 |
| B4 | GPIO10 | Blue bit 4 (MSB) |

### I2C Bus Pins

| GPIO | Function |
|------|----------|
| GPIO8 | I2C SDA |
| GPIO9 | I2C SCL |

### Peripheral Interfaces

| Interface | Details |
|-----------|---------|
| RS485 | Onboard, auto TX/RX switching |
| CAN | Onboard transceiver |
| TF Card | MicroSD (CS via CH422G EXIO4) |
| I2C | Expansion header on GPIO8/9 |

### Firmware Gotchas

- **CRITICAL**: The 16-bit RGB interface consumes nearly all GPIOs. Use CH422G expander for peripheral control.
- CH422G must be initialized before LCD backlight, touch reset, or TF card access.
- GT911 reset sequence determines I2C address (0x5D or 0x14) -- controlled via CH422G EXIO1.
- RS485 uses auto-switching -- no manual DE/RE GPIO needed.
- PSRAM is OPI (Octal) -- ensure `CONFIG_SPIRAM_MODE_OCT` is enabled.
- Display requires continuous DMA refresh -- allocate frame buffer in PSRAM.
- GPIO0 is used for G1 -- this is also a boot strapping pin. Ensure no conflict during boot.
- GPIO45 is used for G2 -- also a strapping pin (VDD_SPI voltage select).

---

## ESP32-S3-Touch-LCD-5

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-5 |
| SoC | ESP32-S3R8 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |
| Power Input | 7-36V wide range (onboard regulator) |

### Display Specifications

| Property | Value |
|----------|-------|
| Size | 5.0 inch |
| Type | IPS LCD |
| Resolution | 800 x 480 |
| Colors | 65K (RGB565) |
| Driver IC | EK9716 |
| Interface | 16-bit RGB Parallel |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | GT911 |
| Interface | I2C |
| Points | 5-point with interrupt |

### IO Expander

Uses **CH422G** (same architecture as 4.3" board).

| CH422G Pin | Function |
|------------|----------|
| TP_RST | Touch panel reset |
| LCD_BL | LCD backlight control |
| LCD_RST | LCD reset |
| SD_CS | TF card chip select |
| USB_SEL | USB path select |

### Peripheral Interfaces

| Interface | Details |
|-----------|---------|
| RS485 | Onboard transceiver |
| CAN | Onboard transceiver |
| I2C | Expansion header |
| TF Card | MicroSD slot |
| RTC | Onboard with battery holder |
| Battery | MX1.25 2P Li-ion header |
| Digital Isolated IO | Onboard |

### Firmware Gotchas

- Same RGB parallel architecture as the 4.3" -- nearly identical driver code.
- Supports **7-36V** wide input -- suitable for industrial power supplies.
- Onboard RTC chip with dedicated battery holder.
- CH422G must be initialized first (same as 4.3").
- Digital isolated IO interfaces for industrial applications.

---

## ESP32-S3-Touch-LCD-7

### Overview

| Property | Value |
|----------|-------|
| Product Name | ESP32-S3-Touch-LCD-7 |
| Variant | -B: 1024x600 resolution |
| SoC | ESP32-S3R8 |
| Flash | 16 MB (7: 8MB reported in some sources) |
| PSRAM | 8 MB (OPI) |
| Processor | Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB |
| ROM | 384 KB |
| WiFi | 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB Connector | USB Type-C |

### Display Specifications

**Standard (7):**

| Property | Value |
|----------|-------|
| Size | 7.0 inch |
| Resolution | 800 x 480 |
| Colors | 65K |
| Driver IC | EK9716 |
| Interface | RGB Parallel (RGB565) |

**Type B (7B):**

| Property | Value |
|----------|-------|
| Size | 7.0 inch |
| Resolution | 1024 x 600 |
| Colors | 65K |
| Driver IC | RGB Parallel |
| Interface | RGB Parallel |

### Touch Specifications

| Property | Value |
|----------|-------|
| Type | Capacitive |
| Controller | GT911 |
| Interface | I2C |
| I2C Address | 0x5D or 0x14 |
| Points | 5-point with interrupt |

### IO Expander

**CH422G** at I2C address 0x24 (same as 4.3" and 5").

### I2C Bus

| GPIO | Function |
|------|----------|
| GPIO8 | I2C SDA |
| GPIO9 | I2C SCL |

### Peripheral Interfaces

| Interface | Details |
|-----------|---------|
| RS485 | Onboard transceiver |
| CAN | Onboard transceiver |
| I2C | Expansion header |
| TF Card | MicroSD slot |

### Firmware Gotchas

- Same RGB parallel + CH422G architecture as 4.3" and 5" boards.
- Pin mapping for LCD is identical/very similar to the 4.3" variant.
- 7B variant at 1024x600 requires more PSRAM for frame buffer.
- Large display benefits from LVGL's partial rendering mode to save memory.
- At 800x480 RGB565, one frame buffer = 768 KB. At 1024x600, one frame buffer = 1.2 MB.

---

## ESP32-S3 Touch AMOLED Series

All AMOLED boards use ESP32-S3R8 (240 MHz dual-core, 16MB Flash, 8MB PSRAM) and QSPI display interface. AMOLED provides superior contrast, deeper blacks, and lower power for dark UIs compared to LCD.

### ESP32-S3-Touch-AMOLED-1.32

| Property | Value |
|----------|-------|
| Display | 1.32" AMOLED Round |
| Resolution | 466 x 466 |
| Colors | 16.7M |
| Display Driver | CO5300 (QSPI) |
| Touch Controller | CST820 (I2C) |
| Sensors | QMI8658 IMU |
| Other | RTC, battery management |

### ESP32-S3-Touch-AMOLED-1.43

| Property | Value |
|----------|-------|
| Display | 1.43" AMOLED Round |
| Resolution | 466 x 466 |
| Colors | 16.7M |
| Display Driver | CO5300 (QSPI) |
| Touch Controller | Capacitive (I2C) |
| Sensors | QMI8658 IMU |
| Other | Optional CNC metal case |

### ESP32-S3-Touch-AMOLED-1.64

| Property | Value |
|----------|-------|
| Display | 1.64" AMOLED |
| Resolution | 280 x 456 |
| Colors | 16.7M |
| Display Driver | QSPI |
| Touch Controller | Capacitive (I2C) |
| Sensors | QMI8658 IMU |

### ESP32-S3-Touch-AMOLED-1.75

| Property | Value |
|----------|-------|
| Display | 1.75" AMOLED Round |
| Resolution | 466 x 466 |
| Colors | 16.7M |
| Display Driver | CO5300 (QSPI) |
| Touch Controller | CST9217 (I2C) |
| Variant | -C adds aluminum alloy case |
| Audio | Dual MIC array, ES7210 ADC, ES8311 DAC |

#### Verified Pin Mapping (from ESPHome community)

| GPIO | Function |
|------|----------|
| GPIO12 | Display CS |
| GPIO39 | Display RST |
| GPIO38 | SPI CLK |
| GPIO4 | SPI D0 |
| GPIO5 | SPI D1 |
| GPIO6 | SPI D2 |
| GPIO7 | SPI D3 |
| GPIO15 | I2C SDA (Touch + IMU) |
| GPIO14 | I2C SCL (Touch + IMU) |
| GPIO11 | Touch INT |
| GPIO40 | Touch RST |
| GPIO45 | I2S LRCLK (Speaker) |
| GPIO9 | I2S BCLK |
| GPIO42 | I2S MCLK |
| GPIO8 | I2S DOUT (Speaker) |
| GPIO10 | I2S DIN (Microphone) |
| GPIO46 | Speaker Enable |

### ESP32-S3-Touch-AMOLED-1.8

| Property | Value |
|----------|-------|
| Display | 1.8" AMOLED |
| Resolution | 368 x 448 |
| Colors | 16.7M |
| Display Driver | SH8601 (QSPI) |
| Touch Controller | FT3168 (I2C) |
| Sensors | QMI8658 IMU |
| Other | Power management IC, RTC, audio codec |

### ESP32-S3-Touch-AMOLED-2.06

| Property | Value |
|----------|-------|
| Display | 2.06" AMOLED (watch form) |
| Resolution | 410 x 502 |
| Colors | 16.7M |
| Display Driver | QSPI |
| Touch Controller | Capacitive (I2C) |
| Sensors | QMI8658 IMU, RTC |
| Audio | Dual digital microphone array, audio codec |

### ESP32-S3-Touch-AMOLED-2.16

| Property | Value |
|----------|-------|
| Display | 2.16" AMOLED |
| Resolution | 480 x 480 |
| Colors | 16.7M |
| Display Driver | QSPI |
| Touch Controller | Capacitive (I2C) |

### ESP32-S3-Touch-AMOLED-2.41

| Property | Value |
|----------|-------|
| Display | 2.41" AMOLED |
| Resolution | 600 x 450 |
| Colors | 16.7M |
| Display Driver | QSPI |
| Touch Controller | Capacitive (I2C) |
| Sensors | QMI8658 IMU |

### AMOLED Series Firmware Gotchas

- All AMOLED displays use **QSPI** (4 data lines + CLK + CS) -- not standard SPI.
- CO5300 driver used on round 466x466 displays requires specific initialization commands.
- SH8601 (1.8" model) has different init sequence from CO5300.
- AMOLED displays are self-emitting -- no backlight pin needed (lower power on dark themes).
- CST9217 supports multi-touch gestures; CST820 is single-touch only.
- For LVGL: use `LV_COLOR_FORMAT_RGB565` and enable QSPI DMA for smooth rendering.
- Round AMOLED displays need circular clipping mask in UI framework.
- ES8311 DAC + ES7210 ADC combo on audio-equipped models requires I2S bus configuration with MCLK.

---

## Cross-Board Comparison: Interface Types

### SPI-Based Displays (fewer GPIOs, lower bandwidth)

Best for small displays up to ~320x480.

| Board | Display Size | Resolution | SPI Speed |
|-------|-------------|-----------|-----------|
| Touch-LCD-1.28 | 1.28" | 240x240 | Up to 80 MHz |
| Touch-LCD-1.47 | 1.47" | 172x320 | Standard SPI |
| Touch-LCD-1.69 | 1.69" | 240x280 | Standard SPI |
| Touch-LCD-2 | 2.0" | 240x320 | Standard SPI |
| Touch-LCD-2.8 | 2.8" | 240x320 | Standard SPI |
| Touch-LCD-3.5 | 3.5" | 320x480 | Standard SPI |

### QSPI-Based Displays (moderate GPIOs, higher bandwidth)

Good balance of speed and GPIO usage.

| Board | Display Size | Resolution |
|-------|-------------|-----------|
| Touch-LCD-1.85 | 1.85" | 360x360 |
| Touch-LCD-2.8C | 2.8" | 480x480 |
| Touch-LCD-3.49 | 3.49" | 172x640 |
| Touch-LCD-3.5B | 3.5" | 320x480 |
| All AMOLED | Various | Various |

### RGB Parallel Displays (most GPIOs, highest bandwidth)

Required for large/high-resolution displays. Need CH422G IO expander.

| Board | Display Size | Resolution | IO Expander |
|-------|-------------|-----------|-------------|
| Touch-LCD-2.1 | 2.1" | 480x480 | Varies |
| Touch-LCD-4 | 4.0" | 480x480 | CH422G |
| Touch-LCD-4.3 | 4.3" | 800x480 | CH422G |
| Touch-LCD-5 | 5.0" | 800x480 | CH422G |
| Touch-LCD-7 | 7.0" | 800x480 | CH422G |
| Touch-LCD-7B | 7.0" | 1024x600 | CH422G |
