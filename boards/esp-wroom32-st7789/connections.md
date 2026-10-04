# ESP-WROOM32 with ST7789 Display & Touchscreen Wiring Guide

This board features an ESP32 microcontroller paired with a 2.8" ST7789 display with integrated XPT2046 resistive touchscreen.

## Components

- **MCU**: ESP-WROOM32 (ESP32)
- **Display**: ST7789 2.8" SPI TFT LCD with RGB-BGR color order (240x320)
- **Touchscreen**: XPT2046 resistive touch controller (shares SPI bus)
- **SD Card**: Slot on main SPI bus
- **Button**: Single boot button (GPIO0; active-LOW)
- **LED**: Single status LED
- **Communication**: SPI for display/touch/SD card, I2C for sensors

## Display Pinout (ST7789)

| Display Pin | ESP32 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| CS | 17 | Chip Select |
| RESET | 5 | Reset |
| DC | 16 | Data/Command |
| MOSI | 23 | Serial Data In |
| MISO | 19 | Serial Data Out (not typically used) |
| SCK | 18 | Clock |
| BL/LED | 32 | Backlight (PWM) |

## Touchscreen Pinout (XPT2046)

The touchscreen shares the SPI bus (MOSI/MISO/SCK) with the display but has separate CS.

| Touch Pin | ESP32 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| MOSI | 23 | Serial Data In (shared with display) |
| MISO | 19 | Serial Data Out (shared with display) |
| SCK | 18 | Clock (shared with display) |
| CS | 21 | Chip Select (touch only) |
| IRQ | Not used | Interrupt (optional) |

## SD Card Slot Pinout

| SD Card Pin | ESP32 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| MOSI | 23 | Serial Data In (shared with display/touch) |
| MISO | 19 | Serial Data Out (shared) |
| SCK | 18 | Clock (shared) |
| CS | 12 | Chip Select (SD card only) |

## Radio Modules (CC1101/NRF24) - Shared SPI Bus

| Radio Pin | ESP32 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| MOSI | 23 | Serial Data In (shared) |
| MISO | 19 | Serial Data Out (shared) |
| SCK | 18 | Clock (shared) |
| CS | 15 | Chip Select (radio only) |
| *Specific to RF module* | | See module documentation |

**Note**: Display, touchscreen, SD card, and radio modules all share MOSI/MISO/SCK but each has its own CS line for isolation.

## User Input

| Input | ESP32 Pin | Type |
|---|---|---|
| Boot Button | 0 | Tactile switch, active-LOW (can be remapped as OK button) |

## I2C Sensors (Grove/Standard)

| Sensor Pin | ESP32 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| SCL | 22 | I2C Clock |
| SDA | 21 | I2C Data (warning: shared with touch CS—check firmware) |

## Status LED

| LED Pin | ESP32 Pin | Purpose |
|---|---|---|
| Anode | 2 | Active HIGH output |
| Cathode | GND | Ground |

## Serial Port

| Pin | ESP32 Pin | Type |
|---|---|---|
| RX | 3 | UART0 Input (programming/debug) |
| TX | 1 | UART0 Output (programming/debug) |

## Power Supply

- **Logic voltage**: 3.3V
- **Supply current**: ~600mA typical (display + touch + MCU)
- **Peak current**: ~1000mA (during radio transmission + backlight at full brightness)

Use a quality USB power supply or regulated 3.3V source rated for at least 1.5A.

## SPI Bus Architecture

```
ESP32 (VSPI Bus)
├─ Display (CS=17) — ST7789 + BL
├─ Touchscreen (CS=21) — XPT2046
├─ SD Card (CS=12) — microSD
└─ Radio (CS=15) — NRF24 or CC1101

Shared lines: MOSI=23, MISO=19, SCK=18
```

## Wiring Notes

1. **Multi-Device SPI**: All four devices (display, touch, SD, radio) share the same MOSI/MISO/SCK lines. Each device gets its own CS line. The firmware manages CS timing automatically.
2. **Touch CS Conflict**: GPIO21 is used for both touch CS and Grove SDA (I2C). If using Grove I2C sensors, disable touch or use alternate GPIO.
3. **Backlight**: The BL pin (GPIO32) supports PWM for brightness control (0-255).
4. **Reset**: Display RESET (GPIO5) is active LOW. Normally tied HIGH through 10kΩ resistor or managed by firmware.
5. **SPI Frequency**: Write at 40MHz, read at 20MHz. Touch operations at 2.5MHz.
6. **Display Rotation**: Set to 90° rotation (landscape).
7. **Color Order**: ST7789 configured for RGB→BGR conversion.
