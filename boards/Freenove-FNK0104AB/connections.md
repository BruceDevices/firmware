# Freenove FNK0104B — Bruce wiring / pinouts

Freenove FNK0104B (2.8" ILI9341 ESP32-S3, FT6336 capacitive touch) — the same
hardware design as the ES3C28P. Display, touch, SD card, RGB LED and the BOOT
button are on-board and already configured; you only wire the optional add-on
radios/modules.

## On-board (already configured — for reference)

| Function | Pins |
| --- | --- |
| Display (ILI9341, SPI) | SCLK 12, MOSI 11, MISO 13, CS 10, DC 46, RST board-reset, BL 45 |
| Touch (FT6336, I2C @0x38) | SDA 16, SCL 15, INT 17, RST 18 |
| SD card | on-board slot (SDMMC) — just insert a microSD |
| RGB LED (WS2812) | 40 |
| Button (Select) | 0 (BOOT) |
| Battery ADC | 8 (x2 divider) |

## Free GPIOs (broken-out headers)

| Header | Free GPIOs | Notes |
| --- | --- | --- |
| Expanded IO | 2, 3, 14, 21 | + 3V3 + GND |
| UART | 43 (TX0), 44 (RX0) | free if the serial console is unused |
| IIC | 16 (SDA), 15 (SCL) | shared touch/audio bus — for I2C add-ons |

Power modules from the header **3V3** pin — CC1101 and nRF24 are 3.3V, not 5V-tolerant.

## Sub-GHz / 2.4 GHz radios (SPI, shared bus)

| Device | SCK | MOSI | MISO | CS | GDO0/CE |
| --- | :---: | :---: | :---: | :---: | :---: |
| CC1101 | 14 | 3 | 2 | 21 | 43 (GDO0) |
| nRF24  | 14 | 3 | 2 | 44 | 3V3 * (CE) |

- CC1101 and nRF24 share SCK/MOSI/MISO. Set pins in Bruce → Settings → Hardware/Pins.
- **CC1101 GDO0** default collides with MISO (GPIO 2) — set GDO0 to **43** in the UI.
- Both radios at once need 7 GPIOs but only 6 are free (4 Expanded IO + 2 UART).
  Either tie nRF24 **CE to 3V3** ( * ) so Bruce drives it over SPI only, or use one
  radio at a time and swap CS in the UI.
- nRF24: add a decoupling cap across the module VCC/GND (10 uF; 100 uF for PA+LNA).

## I2C add-ons (no extra pins)

PN532 (NFC), DS3231 (RTC) and other I2C devices join the IIC header
(SDA 16 / SCL 15) at their own addresses.

> Match wiring by **GPIO number to the header silkscreen** (IO2/IO3/IO14/IO21,
> TX/RX), not by connector position.
