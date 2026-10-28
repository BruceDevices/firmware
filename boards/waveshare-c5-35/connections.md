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
| 15 | GPIO0, **not connected as shipped** (R55) | 16 | EXIO13 |
| 17 | GPIO1, **not connected as shipped** (R58) | 18 | EXIO14 |
| 19 | GPIO9, **not connected as shipped** (R57; GPIO9 is the SD card CS) | 20 | EXIO15 |
| 21 | **GPIO28** (BOOT button = Back in Bruce, strapping pin) | 22 | CHIP_PU (reset) |
| 23 | EXIO_ADC | 24 | PWRON |
| 25 | I2C SCL (GPIO26) | 26 | UART0 TX (GPIO11), **not connected as shipped** (R56) |
| 27 | I2C SDA (GPIO27) | 28 | UART0 RX (GPIO12), **not connected as shipped** (R59) |
| 29 | GND | 30 | GND |
| 31 | 3V3 | 32 | 3V3 |

### Header GPIOs behind 0R selectors

Per Waveshare's schematic ([waveshareteam/ESP32-C5-Touch-LCD-3.5](https://github.com/waveshareteam/ESP32-C5-Touch-LCD-3.5),
`hardware/schematic/ESP32-C5-Touch-LCD-3.5.pdf`), these ESP32 GPIOs reach J3 only through a
3-pad 0R selector: pad 1 is the ESP32 pin, pad 2 is marked **0R** (fitted, the on-board
function) and pad 3 is marked **NC** (not fitted, the header). As shipped, the header side is
open, so these J3 holes are not connected to the chip, even on boards without a camera:

| Selector | Pad 1 (ESP32) | Pad 2, 0R (fitted) | Pad 3, NC (header) |
| :---: | --- | --- | --- |
| R55 | GPIO0  | CAM_D0     | J3-15 |
| R58 | GPIO1  | CAM_D1     | J3-17 |
| R56 | GPIO11 | CAM_PCLK   | J3-26 (U0TXD) |
| R59 | GPIO12 | CAM_VSYNC  | J3-28 (U0RXD) |
| R57 | GPIO9  | SD card CS | J3-19 |

R57 is the same part used for GPIO9, and it shows which pad is live: the microSD card works,
so pad 2 (0R, SD CS) is the connected side and pad 3 (the header) is the open one. Hardware
testing agrees: on two no-camera units, a firmware pin probe saw no signal reach GPIO0, 1, 11
or 12 from the header holes, even with a known-good source on the wire (GPS TX measured
3.247 V at the wire tip).

So the header UART, IR and RF holes do nothing without rework. Per the schematic, moving a
selector's 0R from pad 2 to pad 3 connects the header and disconnects the on-board function
(for R57, that would cut the SD card).

The EXIO lines belong to the I/O expander (a CH32V006 MCU at 0x24), not to the ESP32.

UART GPS over the header (TX 11 / RX 12, J3-26 / J3-28) did NOT work on the no-camera units
tested — no bytes reached the chip. GPS can work on this board over I2C on SDA/SCL (GPIO27/26)
using the LC76G's I2C mode — verified here with a standalone I2C test driver; Bruce has no I2C
GPS driver yet, so GPS is unsupported in Bruce on this board for now. CH9329 BadUSB over the
header UART is untested here.

GPIO 16 and 17 are the module's internal flash/PSRAM pins. Never assign them.
Audio: I2S BCLK 23, LRCK 10, DOUT 25 to the ES8311, then the NS4150B amp and the onboard speaker.
Microphone: onboard mic into the ES8311 ADC, recorded on GPIO 24 (I2S data in).
