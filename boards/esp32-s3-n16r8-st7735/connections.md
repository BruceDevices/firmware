# ESP32-S3 N16R8 + 1.8" ST7735 + 3 buttons

Environment: `esp32-s3-n16r8-st7735`. Board: ESP32-S3 N16R8 (16 MB flash, 8 MB octal PSRAM), DevKitC-1 style.
Screen: 1.8" 128x160 SPI module with ST7735 driver (the common red board, pins labelled GND VCC SCL SDA RES DC CS BL).

## Screen

| Screen pin | Board pin |
| --- | :---: |
| VCC | 3V3 |
| GND | GND |
| SCL | GPIO5 |
| SDA | GPIO6 |
| RES | GPIO15 |
| DC | GPIO7 |
| CS | GPIO16 |
| BL | GPIO4 |

## Buttons

Each button between the pin and GND (internal pull-ups, active low).

| Button | Board pin |
| --- | :---: |
| Previous / up | GPIO39 |
| Select | GPIO41 |
| Next / down | GPIO42 |

Press previous and next together for back.

## Optional modules (defaults, change them in Config > Set Device pins)

SPI modules share one bus: SCK GPIO12, MOSI GPIO11, MISO GPIO13. It is separate from the screen's bus.

| Module | CS | Other |
| --- | :---: | --- |
| SD card | GPIO21 | |
| CC1101 | GPIO10 | GDO0 GPIO9 |
| NRF24 | GPIO14 | CE GPIO17 |

| Function | Pins |
| --- | --- |
| I2C (PN532, ...) | SDA GPIO8, SCL GPIO47 |
| GPS / CH9329 serial | RX GPIO47, TX GPIO8 (shared with I2C) |
| IR LED | GPIO40 |
| IR receiver | GPIO47 |

I2C and serial use GPIO47 rather than the `esp32-s3-devkitc-1` default GPIO46: GPIO46 is a strapping pin, and a module pull-up on it can stop the board entering download mode.

## Pins to leave alone

| Pin | Why |
| --- | --- |
| GPIO0, GPIO3, GPIO45, GPIO46 | strapping pins |
| GPIO19, GPIO20 | native USB |
| GPIO35, GPIO36, GPIO37 | octal PSRAM on N16R8 modules |
| GPIO43, GPIO44 | UART0 serial |
| GPIO48 | on-board RGB LED on most boards |

## Notes

- Noise strip along two edges or wrong colours: some modules use another panel type. Change `-DST7735_GREENTAB2=1` in the `.ini` to `ST7735_BLACKTAB` or `ST7735_REDTAB` and rebuild.
