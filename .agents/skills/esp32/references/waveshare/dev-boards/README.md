# Waveshare ESP32 Development Boards - Complete Index

> Last updated: 2026-04-13
> Source: Waveshare official product pages and wiki

This directory contains comprehensive reference files for all Waveshare development boards based on Espressif ESP32-family chips (ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2).

---

## File Index

| File | Contents |
|------|----------|
| [esp32-s3-touch-lcd.md](esp32-s3-touch-lcd.md) | ESP32-S3 Touch LCD boards (1.28" through 7") |
| [esp32-s3-lcd.md](esp32-s3-lcd.md) | ESP32-S3 LCD boards WITHOUT touch |
| [esp32-c6-lcd.md](esp32-c6-lcd.md) | All ESP32-C6 based boards (LCD, AMOLED, general) |
| [esp32-general.md](esp32-general.md) | General purpose ESP32 dev boards (non-display) |
| [esp32-camera.md](esp32-camera.md) | Camera and thermal imaging boards |
| [esp32-e-paper.md](esp32-e-paper.md) | E-paper / e-ink driver boards |

---

## Master Board List

### ESP32-S3 Touch LCD Boards

| Board | Display | Resolution | Touch | Controller | Flash/PSRAM |
|-------|---------|-----------|-------|------------|-------------|
| ESP32-S3-Touch-LCD-1.28 | 1.28" IPS Round | 240x240 | CST816S (I2C) | GC9A01 (SPI) | 16MB/2MB |
| ESP32-S3-Touch-LCD-1.28-B | 1.28" IPS Round | 240x240 | CST816S (I2C) | GC9A01 (SPI) | 16MB/2MB |
| ESP32-S3-Touch-LCD-1.47 | 1.47" IPS | 172x320 | Capacitive (I2C) | ST7789 (SPI) | 16MB/8MB |
| ESP32-S3-Touch-LCD-1.69 | 1.69" IPS | 240x280 | CST816T (I2C) | ST7789V2 (SPI) | 16MB/8MB |
| ESP32-S3-Touch-LCD-1.85 | 1.85" IPS Round | 360x360 | Capacitive (I2C) | ST77916 (QSPI) | 16MB/8MB |
| ESP32-S3-Touch-LCD-2 | 2.0" IPS | 240x320 | CST816D (I2C) | ST7789T3 (SPI) | 16MB/8MB |
| ESP32-S3-Touch-LCD-2.1 | 2.1" IPS Round | 480x480 | Capacitive (I2C) | ST7701S (RGB) | 16MB/8MB |
| ESP32-S3-Touch-LCD-2.8 | 2.8" IPS | 240x320 | Capacitive (I2C) | ST7789 (SPI) | 16MB/8MB |
| ESP32-S3-Touch-LCD-2.8C | 2.8" IPS Round | 480x480 | CST816 (I2C) | ST77916 (QSPI) | 16MB/8MB |
| ESP32-S3-Touch-LCD-3.49 | 3.49" IPS Bar | 172x640 | AXS15231B (I2C) | AXS15231B (QSPI) | 16MB/8MB |
| ESP32-S3-Touch-LCD-3.5 | 3.5" IPS | 320x480 | FT6336 (I2C) | ST7796 (SPI) | 16MB/8MB |
| ESP32-S3-Touch-LCD-3.5B | 3.5" IPS | 320x480 | FT6336 (I2C) | ST7796 (QSPI) | 16MB/8MB |
| ESP32-S3-Touch-LCD-4 | 4.0" IPS | 480x480 | GT911 (I2C) | ST7701S (RGB) | 16MB/8MB |
| ESP32-S3-Touch-LCD-4B | 4.0" Smart 86 Box | 480x480 | GT911 (I2C) | ST7701S (RGB) | 16MB/8MB |
| ESP32-S3-Touch-LCD-4.3 | 4.3" IPS | 800x480 | GT911 (I2C) | EK9716 (RGB) | 16MB/8MB |
| ESP32-S3-Touch-LCD-4.3B | 4.3" IPS | 800x480 | GT911 (I2C) | EK9716 (RGB) | 16MB/8MB |
| ESP32-S3-Touch-LCD-4.3C | 4.3" IPS (AI Voice) | 800x480 | GT911 (I2C) | EK9716 (RGB) | 16MB/8MB |
| ESP32-S3-Touch-LCD-5 | 5.0" IPS | 800x480 | GT911 (I2C) | EK9716 (RGB) | 16MB/8MB |
| ESP32-S3-Touch-LCD-7 | 7.0" IPS | 800x480 | GT911 (I2C) | EK9716 (RGB) | 16MB/8MB |
| ESP32-S3-Touch-LCD-7B | 7.0" IPS | 1024x600 | GT911 (I2C) | RGB Parallel | 16MB/8MB |

### ESP32-S3 Touch AMOLED Boards

| Board | Display | Resolution | Touch | Controller | Flash/PSRAM |
|-------|---------|-----------|-------|------------|-------------|
| ESP32-S3-Touch-AMOLED-1.32 | 1.32" AMOLED Round | 466x466 | CST820 (I2C) | CO5300 (QSPI) | 16MB/8MB |
| ESP32-S3-Touch-AMOLED-1.43 | 1.43" AMOLED Round | 466x466 | Capacitive (I2C) | CO5300 (QSPI) | 16MB/8MB |
| ESP32-S3-Touch-AMOLED-1.64 | 1.64" AMOLED | 280x456 | Capacitive (I2C) | QSPI | 16MB/8MB |
| ESP32-S3-Touch-AMOLED-1.75 | 1.75" AMOLED Round | 466x466 | CST9217 (I2C) | CO5300 (QSPI) | 16MB/8MB |
| ESP32-S3-Touch-AMOLED-1.75C | 1.75" AMOLED Round (Case) | 466x466 | CST9217 (I2C) | CO5300 (QSPI) | 16MB/8MB |
| ESP32-S3-Touch-AMOLED-1.8 | 1.8" AMOLED | 368x448 | FT3168 (I2C) | SH8601 (QSPI) | 16MB/8MB |
| ESP32-S3-Touch-AMOLED-2.06 | 2.06" AMOLED Watch | 410x502 | Capacitive (I2C) | QSPI | 16MB/8MB |
| ESP32-S3-Touch-AMOLED-2.16 | 2.16" AMOLED | 480x480 | Capacitive (I2C) | QSPI | 16MB/8MB |
| ESP32-S3-Touch-AMOLED-2.41 | 2.41" AMOLED | 600x450 | Capacitive (I2C) | QSPI | 16MB/8MB |

### ESP32-S3 LCD Boards (No Touch)

| Board | Display | Resolution | Controller | Flash/PSRAM |
|-------|---------|-----------|------------|-------------|
| ESP32-S3-LCD-1.28 | 1.28" IPS Round | 240x240 | GC9A01 (SPI) | 16MB/2MB |
| ESP32-S3-LCD-1.28-B | 1.28" IPS Round (Metal Case) | 240x240 | GC9A01 (SPI) | 16MB/2MB |
| ESP32-S3-LCD-1.47 | 1.47" IPS | 172x320 | ST7789 (SPI) | 16MB/8MB |
| ESP32-S3-LCD-1.47B | 1.47" IPS (Type B) | 172x320 | ST7789 (SPI) | 16MB/8MB |
| ESP32-S3-LCD-1.69 | 1.69" IPS | 240x280 | ST7789V2 (SPI) | 16MB/8MB |
| ESP32-S3-LCD-1.85 | 1.85" IPS Round | 360x360 | ST77916 (QSPI) | 16MB/8MB |
| ESP32-S3-LCD-2 | 2.0" IPS | 240x320 | ST7789T3 (SPI) | 16MB/8MB |
| ESP32-S3-LCD-Driver-Board | External LCD driver | Various | Various | 16MB/8MB |

### ESP32-C6 Boards

| Board | Display | Resolution | Touch | Controller | Flash |
|-------|---------|-----------|-------|------------|-------|
| ESP32-C6-LCD-1.47 | 1.47" IPS | 172x320 | No | ST7789 (SPI) | 4MB |
| ESP32-C6-Touch-LCD-1.47 | 1.47" IPS | 172x320 | Yes (I2C) | ST7789 (SPI) | 8MB |
| ESP32-C6-LCD-1.69 | 1.69" IPS | 240x280 | No | ST7789V2 (SPI) | 16MB |
| ESP32-C6-Touch-LCD-1.69 | 1.69" IPS | 240x280 | Yes (I2C) | ST7789V2 (SPI) | 16MB |
| ESP32-C6-Touch-LCD-1.83 | 1.83" IPS | 240x284 | Yes (I2C) | SPI | 16MB |
| ESP32-C6-LCD-1.9 | 1.9" IPS | 170x320 | Optional | ST7789 (SPI) | 8MB |
| ESP32-C6-GEEK | 1.14" | 240x135 | No | ST7789 (SPI) | 16MB |
| ESP32-C6-Touch-AMOLED-1.32 | 1.32" AMOLED Round | 466x466 | CST820 (I2C) | CO5300 (QSPI) | 16MB |
| ESP32-C6-Touch-AMOLED-1.43 | 1.43" AMOLED Round | 466x466 | Capacitive (I2C) | CO5300 (QSPI) | 16MB |
| ESP32-C6-Touch-AMOLED-1.8 | 1.8" AMOLED | 368x448 | FT3168/FT6146 (I2C) | SH8601 (QSPI) | 16MB |
| ESP32-C6-Zero | None | N/A | No | N/A | 4MB |
| ESP32-C6-Pico | None | N/A | No | N/A | 4MB |
| ESP32-C6-DEV-KIT-N8 | None | N/A | No | N/A | 8MB |

### ESP32-S3 General Purpose Boards

| Board | Key Feature | Flash/PSRAM |
|-------|------------|-------------|
| ESP32-S3-DEV-KIT-N8R8 | Standard dev kit | 8-32MB / 8-16MB |
| ESP32-S3-Pico | Pico form factor | 16MB/2MB |
| ESP32-S3-Zero | Ultra-compact, castellated | 4MB/2MB |
| ESP32-S3-Nano | Arduino Nano form factor | 16MB/8MB |
| ESP32-S3-Matrix | 8x8 RGB LED matrix | 16MB/8MB |
| ESP32-S3-ETH | 10/100 Ethernet, PoE | 16MB/8MB |
| ESP32-S3-Relay-6CH | 6-channel relay, RS485 | 16MB/8MB |
| ESP32-S3-A7670E-4G | LTE Cat-1 / 2G / GNSS | 16MB/8MB |
| ESP32-S3-AUDIO-Board | Smart speaker, dual mic | 16MB/8MB |

### Camera / Thermal Boards

| Board | Camera | Flash/PSRAM |
|-------|--------|-------------|
| ESP32-S3-CAM-OV5640 | OV5640 5MP DVP | 16MB/8MB |
| Thermal-45 Camera ESP32 Module | 80x62 IR, 45deg FOV | 16MB/8MB |
| Thermal-90 Camera ESP32 Module | 80x62 IR, 90deg FOV | 16MB/8MB |

### E-Paper Boards

| Board | Display | Resolution | Flash/PSRAM |
|-------|---------|-----------|-------------|
| e-Paper ESP32 Driver Board | Universal SPI e-Paper | Various | 4MB/- |
| ESP32-S3-ePaper-1.54 | 1.54" B/W | 200x200 | 16MB/8MB |
| ESP32-S3-ePaper-3.97 | 3.97" B/W | 800x480 | 16MB/8MB |
| ESP32-S3-PhotoPainter | 7.3" 6-Color | 800x480 | 16MB/8MB |

---

## Chip Family Quick Reference

| Chip | Architecture | Cores | Max Freq | WiFi | Bluetooth | Zigbee/Thread |
|------|-------------|-------|----------|------|-----------|---------------|
| ESP32 | Xtensa LX6 | 2 | 240 MHz | 802.11 b/g/n | BT 4.2 + BLE | No |
| ESP32-S3 | Xtensa LX7 | 2 | 240 MHz | 802.11 b/g/n | BLE 5.0 | No |
| ESP32-C6 | RISC-V | 1+1 LP | 160 MHz | WiFi 6 | BLE 5.0 | Yes (802.15.4) |
| ESP32-H2 | RISC-V | 1 | 96 MHz | No | BLE 5.0 | Yes (802.15.4) |

### Common Display Controllers

| Controller | Interface | Typical Resolution | Notes |
|-----------|-----------|-------------------|-------|
| GC9A01 | SPI | 240x240 (round) | Low GPIO usage, good for wearables |
| ST7789 / ST7789V2 | SPI | 172x320, 240x280, 240x320 | Very common, well supported |
| ST7789T3 | SPI | 240x320 | Variant of ST7789 |
| ST77916 | QSPI | 360x360, 480x480 | Round displays, higher res |
| ST7796 | SPI / QSPI | 320x480 | Medium displays |
| ST7701S | RGB Parallel | 480x480 | Requires many GPIOs |
| EK9716 | RGB Parallel (16-bit) | 800x480 | Large panels, needs IO expander |
| AXS15231B | QSPI | 172x640 | Bar-type display, integrated touch |
| CO5300 | QSPI | 466x466 | AMOLED round displays |
| SH8601 | QSPI | 368x448 | AMOLED rectangular |
| RM67162 | QSPI | Various | AMOLED |

### Common Touch Controllers

| Controller | Interface | Multi-touch | Notes |
|-----------|-----------|------------|-------|
| CST816S/T/D | I2C | Single | Small displays, low power |
| CST820 | I2C | Single | AMOLED round displays |
| CST9217 | I2C | Multi | Higher-end AMOLED |
| FT6336 | I2C | 2-point | Medium displays |
| FT3168 | I2C | Multi | AMOLED displays |
| GT911 | I2C | 5-point | Large displays (4"+) |
| AXS15231B | I2C | Yes | Integrated display+touch |

---

## Common Onboard Peripherals

| Component | Function | Interface | Found On |
|-----------|----------|-----------|----------|
| QMI8658 / QMI8658C | 6-axis IMU (accel+gyro) | I2C | Most wearable-size boards |
| PCF85063 | RTC (real-time clock) | I2C | 1.69", AMOLED boards |
| SHTC3 | Temperature + humidity | I2C | E-paper boards |
| ES8311 | Audio DAC | I2S | AMOLED, audio boards |
| ES7210 | Audio ADC (4-ch mic) | I2S | AMOLED, audio boards |
| CH422G | IO Expander | I2C (0x24) | 4"+, 5", 7" LCD boards |
| CH343P | USB-UART bridge | USB | Small wearable boards |
| W5500 | Ethernet controller | SPI | ESP32-S3-ETH |
