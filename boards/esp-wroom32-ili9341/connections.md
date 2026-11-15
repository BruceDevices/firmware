# ESP-WROOM32 with ILI9341 Display Wiring Guide

This board features an ESP32 microcontroller paired with a 2.4" ILI9341 display and supports 5-way button navigation.

## Components

- **MCU**: ESP-WROOM32 (ESP32)
- **Display**: ILI9341 2.4" SPI TFT LCD (240x320)
- **buttons**: 5-way tactile button pad (up/down/left/right/select)
- **LED**: Single status LED
- **Communication**: SPI for display/radio modules, I2C for sensors

## Display Pinout (ILI9341)

| Display Pin | ESP32 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| CS | 17 | Chip Select |
| RESET | 5 | Reset |
| DC | 16 | Data/Command |
| MOSI | 23 | Serial Data In |
| MISO | 19 | Serial Data Out |
| SCK | 18 | Clock |
| BL/LED | 32 | Backlight (PWM) |

## Button Pinout (5-Way Tactile)

| Button | ESP32 Pin | Function |
|---|---|---|
| Select/OK | 35 | Center button |
| Up | 34 | Navigation up |
| Down | 26 | Navigation down |
| Right | 27 | Navigation right |
| Left | 33 | Navigation left |

All buttons are **active-LOW** (GND when pressed).

## Radio Modules (CC1101/NRF24) - Shared SPI Bus

| Radio Pin | ESP32 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| MOSI | 23 | Serial Data In (shared with display) |
| MISO | 19 | Serial Data Out (shared with display) |
| SCK | 18 | Clock (shared with display) |
| CS | 15 | Chip Select (software selectable) |
| *Specific to RF module* | | See module documentation |

**Note**: Radio modules share the SPI bus with the display but have separate chip select (CS) lines. The firmware manages CS timing to prevent conflicts.

## I2C Sensors (Grove/Standard)

| Sensor Pin | ESP32 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| SCL | 22 | I2C Clock |
| SDA | 21 | I2C Data |

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
- **Supply current**: ~500mA typical (display + MCU)
- **Peak current**: ~800mA (during radio transmission)

Use a quality USB power supply or regulated 3.3V source rated for at least 1A.

## Wiring Notes

1. **SPI Bus Sharing**: Display, radio, and SD card (if used) all share SCK/MOSI/MISO lines. Each device has its own CS line to prevent conflicts.
2. **Button Ground**: All 5 buttons should be pulled to GND when pressed. Use 10kΩ pull-up resistors on pins if needed.
3. **Backlight**: The BL pin (GPIO32) supports PWM for brightness control (0-255).
4. **Reset**: The display RESET (GPIO5) is active LOW. Normally tied HIGH through 10kΩ resistor.
5. **Debouncing**: The firmware handles software debouncing for buttons (typical bounce time ~20ms).
