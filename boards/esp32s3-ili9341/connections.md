# ESP32-S3 with ILI9341 Display & Touchscreen Wiring Guide

This is the most advanced board, featuring the ESP32-S3 microcontroller with dual-core processing, native USB support, and multiple wireless capabilities.

## Components

- **MCU**: ESP32-S3 (dual-core, USB native support)
- **Display**: ILI9341 2.8" SPI TFT LCD with RGB-BGR color order (240x320)
- **Touchscreen**: XPT2046 resistive touch controller (hardware SPI)
- **Radio Options**: CC1101, NRF24, or LoRa (SPI bus)
- **I/O**: IR transmit/receive, RF transmit/receive (IR-based and 433/868MHz RF supported)
- **Button**: Single boot button (GPIO0; active-LOW)
- **LED**: Single status LED (GPIO40)
- **USB**: Native USB support (CDC + custom HID)
- **Communication**: Dedicated SPI for display/touch, separate SPI for radio modules

## Display Pinout (ILI9341 on HSPI)

| Display Pin | ESP32-S3 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| CS | 10 | Chip Select |
| RESET | 5 | Reset (active-LOW) |
| DC | 7 | Data/Command |
| MOSI | 11 | Serial Data In (HSPI) |
| MISM | 13 | Serial Data Out (HSPI) |
| SCK | 12 | Clock (HSPI) |
| BL/LED | 38 | Backlight (PWM) |

## Touchscreen Pinout (XPT2046 on HSPI)

The touchscreen shares HSPI bus (MOSI/MISO/SCK) with the display but has separate CS.

| Touch Pin | ESP32-S3 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| MOSI | 11 | Serial Data In (shared HSPI) |
| MISO | 13 | Serial Data Out (shared HSPI) |
| SCK | 12 | Clock (shared HSPI) |
| CS | 3 | Chip Select (touch only) |
| IRQ | Not used | Interrupt (optional) |

## Radio Modules (CC1101/NRF24/W5500) - SPI Bus Pins

All radio modules use the **main SPI bus** with separate chip select lines:

### CC1101 (FSK radio module)
| CC1101 Pin | ESP32-S3 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| MOSI | 11 | Serial Data In |
| MISO | 13 | Serial Data Out |
| SCK | 12 | Clock |
| CS | 10 | Chip Select |
| GDO0 | 9 | Data Output (optional) |

### NRF24 (2.4GHz wireless module)
| NRF24 Pin | ESP32-S3 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power (needs 3A supply!) |
| GND | GND | Ground |
| MOSI | 11 | Serial Data In |
| MISO | 13 | Serial Data Out |
| SCK | 12 | Clock |
| CS | 14 | Chip Select |
| CE | 16 | Chip Enable |

### W5500 (Ethernet module - optional)
| W5500 Pin | ESP32-S3 Pin | Purpose |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| MOSI | 11 | Serial Data In |
| MISO | 13 | Serial Data Out |
| SCK | 12 | Clock |
| CS | (varies) | Chip Select (externally configurable) |
| INT | (varies) | Interrupt (externally configurable) |

**Note**: CC1101 and NRF24 can be used simultaneously (different CS). W5500 requires custom pin assignment.

## IR & RF Pins (Single-Pin Modules)

For IR LED and single-pin RF receivers/transmitters:

| Function | GPIO Options |
|---|---|
| IR TX | GPIO1, GPIO2 (also Grove Y via GPIO8) |
| IR RX | GPIO1, GPIO2 (also Grove W via GPIO9) |
| RF TX (433/868MHz) | GPIO1, GPIO2 (also Grove Y via GPIO8) |
| RF RX (433/868MHz) | GPIO1, GPIO2 (also Grove W via GPIO9) |

## User Input & LED

| Input/Output | ESP32-S3 Pin | Type |
|---|---|---|
| Boot Button | 0 | Tactile switch, active-LOW (remapped as OK button) |
| Status LED | 40 | Output, active-HIGH |

## I2C Interface (Grove/Standard Sensors)

| Pin Name | ESP32-S3 Pin | Purpose |
|---|---|---|
| SDA | 8 | I2C Data (also used for IR RX) |
| SCL | 9 | I2C Clock (also used for RF RX) |

**Note**: Grove pins share GPIO with RF/IR inputs. Prioritize based on your application.

## Serial Interfaces

| Interface | TX Pin | RX Pin | Purpose |
|---|---|---|
| UART0 (USB CDC) | — | — | Native USB serial (programming & debug) |
| UART1 (GPS/Ext) | 43 | 44 | External serial (e.g., GPS module) |

USB is native on the ESP32-S3 via GPIO19/GPIO20 (internal).

## Power Supply

- **Logic voltage**: 3.3V
- **Supply current**: ~400mA typical (display + touch + MCU)
- **Peak current**:
  - ~800mA with radio + backlight
  - **~2500mA with NRF24** (provide dedicated 3A supply if using NRF24!)

Use a high-quality USB power supply (2A minimum, 3A recommended if using NRF24).

## SPI Architecture

```
ESP32-S3 Dual SPI
├─ HSPI Bus (Display/Touch):
│  ├─ Display (CS=10) — ILI9341
│  └─ Touchscreen (CS=3) — XPT2046
│  Shared: MOSI=11, MISO=13, SCK=12
│
└─ Main SPI Bus (Radio/Peripherals):
   ├─ CC1101 (CS=10, GDO0=9) — FSK transceiver
   ├─ NRF24 (CS=14, CE=16) — 2.4GHz wireless
   └─ W5500 (CS=?, INT=?) — Optional Ethernet
   Shared: MOSI=11, MISO=13, SCK=12
```

**Note**: Display CS and CC1101 CS both use GPIO10 (hardware selectable). Use firmware configuration to choose which module is available.

## Wiring Notes

1. **Dual SPI Buses**: HSPI for display/touch, main SPI for radio modules. Both use GPIO11/12/13, but separate CS lines prevent conflicts.
2. **NRF24 Power**: The NRF24 module requires a dedicated 3A power supply. Do **NOT** power it from the USB port alone—use a separate 3.3V regulator.
3. **Backlight**: GPIO38 supports PWM for brightness control (0-255).
4. **Reset**: Display RESET (GPIO5) is active LOW. Normally tied HIGH through resistor or managed by firmware.
5. **Boot Button**: GPIO0 serves dual purpose: boot selection during power-up, OK button during normal operation.
6. **USB Serial**: Unlike older ESP32, this board has native USB. Use USB for uploads and debugging (no FTDI chip needed).
7. **Color Order**: ILI9341 configured for RGB→BGR conversion.
8. **SPI Frequencies**: 40MHz write, 20MHz read, 2.5MHz touch operations.
9. **Grove I2C Conflict**: GPIO8 (SDA) overlaps with IR RX, GPIO9 (SCL) overlaps with RF RX. Configure based on priority.

## Features

- **Dual-core 240MHz processor** — More performance-intensive tasks
- **USB Native Support** — Faster uploads, serial over USB
- **More GPIO** — Supports more peripheral devices simultaneously
- **Larger Flash** — 8MB partition for bigger firmware
- **Integrated IR LED driver** — GPIO40 for IR output
- **Advanced Radio Options** — CC1101, NRF24, and optional W5500 Ethernet
