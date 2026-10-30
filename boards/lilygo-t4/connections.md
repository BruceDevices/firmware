# LilyGO TTGO T4 v1.3 — Pinouts to use Bruce

ESP32-WROVER board with a 2.4" ILI9341 320x240 SPI display, a microSD slot and
three front buttons. The display and the SD card sit on **two separate SPI
buses**.

## Display (ILI9341, VSPI)

| Signal | MOSI | MISO | SCLK | CS | DC | RST | BL |
| ---    | :--: | :--: | :--: | :-:| :-:| :-: | :-:|
| TFT    | 23   | 12   | 18   | 27 | 32 |  5  |  4 |

## SD Card (HSPI)

| Device  | SCK | MISO | MOSI | CS |
| ---     | :-: | :--: | :--: | :-:|
| SD Card | 14  |  2   | 15   | 13 |

## Buttons (active LOW, on input-only GPIOs with external pull-ups)

| Button | GPIO | Action                                   |
| ---    | :--: | ---                                      |
| LEFT   | 38   | Previous / Up                            |
| CENTER | 37   | Select (click), Escape (double / hold)   |
| RIGHT  | 39   | Next / Down                              |

## I2C (Grove) — FM Radio, PN532, other I2C devices

| Signal | SDA | SCL |
| ---    | :-: | :-: |
| I2C    | 21  | 22  |

## External RF modules (no module fitted by default)

`ALLOW_ALL_GPIO_FOR_IR_RF` is enabled, so IR/RF pins can be chosen at runtime.
Suggested defaults on free GPIOs:

| Device | SCK | MISO | MOSI | CS/SS | GDO0/CE |
| ---    | :-: | :--: | :--: | :---: | :-----: |
| CC1101 | 25  | 33   | 26   | 17    | 34      |
| NRF24  | 25  | 33   | 26   | 17    | 35      |

IR TX default: 26 · IR RX default: 25

Free GPIOs available for single-pin modules (FS1000A, IR LED/receiver, etc.):
17, 25, 26, 33 and the input-only pins 34/35/36.
