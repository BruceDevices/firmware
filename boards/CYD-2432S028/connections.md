# CYD (ESP32-2432S028) + NM-RF-Hat (NMTech) — verified pin map

This documents the NMTech **NM-RF-Hat Companion** (multi-band RF hat for the
`ESP32-2432S028` "CYD", CC1101 / nRF24 / PN532 / IR / 433 OOK / GNSS) and
confirms it matches Bruce's mainline `CYD-2432S028` build. Verified against the
NMTech booklet §1.3 "Pin map".

The hat carries a **"RF Switch — Only One Turn ON"** DIP block: CC1101, nRF24,
IR, 433 OOK and PN532 all share IO27/IO22 (and the SPI bus), so only one may be
enabled at a time. Set the DIP accordingly before using a given radio.

## SPI radios (CC1101 / nRF24) — shared VSPI bus (also SD/TF card)

| Signal        | HAT pad | Bruce define            |
| ---           | :---:   | ---                     |
| SCK           | IO18    | `SPI_SCK_PIN=18`        |
| MOSI          | IO23    | `SPI_MOSI_PIN=23`       |
| MISO          | IO19    | `SPI_MISO_PIN=19`       |
| CC1101 CSN    | IO27    | `CC1101_SS_PIN=27`      |
| CC1101 GDO0   | IO22    | `CC1101_GDO0_PIN=22`    |
| nRF24 CSN     | IO27    | `NRF24_SS_PIN=27`       |
| nRF24 CE      | IO22    | `NRF24_CE_PIN=22`       |
| SD/TF CS      | IO5     | `SDCARD_CS=5`           |

(An AT2401 2.4 GHz PA/LNA fronts the nRF24; its RX_EN also sits on IO22.)

## One-pin modules (IR / 433 OOK) and PN532 (I2C)

| Signal              | HAT pad | Bruce define                              |
| ---                 | :---:   | ---                                       |
| IR TX / 433 ASK TX  | IO22    | `IR_TX_PINS`/`RF_TX_PINS` include 22      |
| IR RX / 433 ASK RX  | IO27    | `IR_RX_PINS`/`RF_RX_PINS` include 27      |
| PN532 SDA           | IO27    | `GROVE_SDA=27`                            |
| PN532 SCL           | IO22    | `GROVE_SCL=22`                            |

## GNSS / power

| Signal   | HAT pad          | Bruce define      |
| ---      | :---:            | ---               |
| GPS TX   | UART0 TXD0 (IO1) | `GPS_SERIAL_TX=1` |
| GPS RX   | UART0 RXD0 (IO3) | `GPS_SERIAL_RX=3` |
| Backlight| IO21             | `TFT_BL=21`       |
| Power    | 5V / GND, BAT+/- | —                 |

## Notes

- Camera features (BLE > Cam Detector: Camera Scan / Camera Deauther / Flock /
  Axon / RayBan) use only the ESP32's built-in Wi-Fi + BLE and need none of the
  hat hardware.
- If the display shows inverted/negative colours after flashing, the panel is
  the 2-USB variant — build/flash the `CYD-2USB` environment instead (adds
  `TFT_INVERSION_ON`).
