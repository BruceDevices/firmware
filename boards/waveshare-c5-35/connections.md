# Waveshare ESP32-C5-Touch-LCD-3.5 pinout used by Bruce

Everything except the expansion header is onboard. The display and the microSD card share one SPI bus.

| Device  | SCK   | MISO  | MOSI  | CS    | DC    | RST        | Backlight        |
| ---     | :---: | :---: | :---: | :---: | :---: | :---:      | :---:            |
| Display | 6     | 2     | 7     | 8     | 5     | EXIO1      | EXIO5 + PWM reg  |
| SD Card | 6     | 2     | 7     | 9     | ---   | ---        | ---              |

| I2C device (SDA 27 / SCL 26) | Address | Notes |
| ---                          | :---:   | ---   |
| FT6336 capacitive touch      | 0x38    | INT on GPIO 3, reset on EXIO0 |
| I/O expander                 | 0x24    | EXIO0 touch reset, EXIO1 LCD reset, EXIO5 panel power, reg 0x05 backlight PWM |
| ES8311 audio codec           | 0x18    | speaker via I2S, clocked off BCLK (no MCLK) |
| AXP2101 PMU                  | 0x34    | not used by Bruce yet |
| PCF85063 RTC                 | 0x51    | not used by Bruce yet |
| QMI8658 IMU                  | 0x6B    | not used by Bruce yet |

External I2C modules (for example PN532) share the onboard bus at SDA 27 / SCL 26.

## 2x16 header J3 (from the Waveshare schematic)

| Pin | Signal | Pin | Signal |
| :---: | --- | :---: | --- |
| 1 | VBUS | 2 | VBAT |
| 3 | GND | 4 | GND |
| 5 | SWDIO (I/O expander MCU) | 6 | EXIO8 |
| 7 | NC | 8 | EXIO9 |
| 9 | NC | 10 | EXIO10 |
| 11 | GPIO22 (C6 boards only) | 12 | EXIO11 |
| 13 | GPIO21 (C6 boards only) | 14 | EXIO12 |
| 15 | **GPIO0**: IR / RF RX | 16 | EXIO13 |
| 17 | **GPIO1**: IR / RF TX | 18 | EXIO14 |
| 19 | GPIO9 (SD card CS, do not use) | 20 | EXIO15 |
| 21 | **GPIO28** (BOOT button = Back in Bruce, strapping pin) | 22 | CHIP_PU (reset) |
| 23 | EXIO_ADC | 24 | PWRON |
| 25 | I2C SCL (GPIO26) | 26 | UART0 TX (GPIO11) |
| 27 | I2C SDA (GPIO27) | 28 | UART0 RX (GPIO12) |
| 29 | GND | 30 | GND |
| 31 | 3V3 | 32 | 3V3 |

GPIO0/1 reach the header through 0R selectors (R55/R58) that pick between the header and the
camera connector; boards without a camera route them to the header.
The EXIO lines belong to the I/O expander (a CH32V006 MCU at 0x24), not to the ESP32.

UART (GPS / CH9329 BadUSB): TX 11, RX 12 (J3-26 / J3-28).

GPIO 16 and 17 are the module's internal flash/PSRAM pins. Never assign them.
Audio: I2S BCLK 23, LRCK 10, DOUT 25 to the ES8311, then the NS4150B amp and the onboard speaker.
Microphone: onboard mic into the ES8311 ADC, recorded on GPIO 24 (I2S data in).
