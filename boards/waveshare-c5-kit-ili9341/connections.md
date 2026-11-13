# Waveshare ESP32-C5-WIFI6-KIT + 2.8" ILI9341 touch screen

Environment: `waveshare-c5-kit-ili9341`. Board: Waveshare ESP32-C5-WIFI6-KIT (ESP32-C5-WROOM-1 or -1U, N16R8 / N32R8).
Screen: 2.8" 240x320 SPI module with XPT2046 touch (LCDwiki MSP2807, "2.8 TFT SPI 240x320 V1.2").
Use the header labels printed on the board (IO1, IO2, ...).

## Why not `esp32-c5-tft`

On this kit GPIO6 is tied (R39, 0 ohm) to the battery-sense divider, which has a 100 nF capacitor to GND.
`esp32-c5-tft` clocks the screen on GPIO6, so on this kit the clock never reaches the screen and it stays white.
This env uses the display pins from Waveshare's own ILI9341 demo for the kit, and GPIO6 only drives the backlight.

## Screen and touch

| Screen pin | Board pin | Note |
| --- | :---: | --- |
| VCC | 3V3 | |
| GND | GND | |
| CS | IO5 | |
| RESET | IO4 | driven by the firmware |
| DC | IO3 | |
| SDI (MOSI) | IO2 | shared with T_DIN |
| SCK | IO1 | shared with T_CLK |
| LED | IO6 | backlight |
| SDO (MISO) | IO7 | shared with T_DO |
| T_CLK | IO1 | |
| T_CS | IO23 | |
| T_DIN | IO2 | |
| T_DO | IO7 | |
| T_IRQ | not connected | |

## Optional modules (defaults, change them in Config > Set Device pins)

All SPI modules share the screen's bus: SCK IO1, MOSI IO2, MISO IO7.

| Module | CS | Other |
| --- | :---: | --- |
| SD card | IO10 | |
| CC1101 | IO9 | GDO0 IO8 |
| NRF24 | IO9 | CE IO8 |
| W5500 | IO9 | INT IO8 |

CC1101, NRF24 and W5500 use the same two pins: fit one at a time, or add a switch on CS and GDO0/CE.

| Function | Pins |
| --- | --- |
| I2C (PN532, ...) | SDA IO24, SCL IO25 |
| GPS / CH9329 serial | RX IO24, TX IO25 (shared with I2C) |
| IR receiver | IO26 |
| IR LED | IO0 |
| RGB LED | IO27 (on the board) |

The stock C5 defaults put I2C on IO4/IO5 and the IR LED on IO3; on this board those are the screen's RESET, CS and DC.

## Pins to leave alone

| Pin | Why |
| --- | --- |
| IO6 as a clock or data line | battery-sense capacitor (see above) |
| IO15 (label NC/IO15) | PSRAM chip select inside the module; configuring it locks the chip up |
| IO28 | BOOT button, strapping pin |
| IO11, IO12 | UART0, the USB-C port labelled UART |
| IO13, IO14 | native USB, the USB-C port labelled USB (Bruce's serial log) |

## Touch

The first boot shows the four-corner calibration. To redo it later: Config > Calibrate Touch.
