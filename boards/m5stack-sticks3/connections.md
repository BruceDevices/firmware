# Pinouts diagram to use Bruce

## Not compatible with StickCPlus (1.1 and 2)
The reason for the incompatibility is that the "Boot" GPIO, that corresponds to GPIO 0, is also connected to PMIC (Power Management IC), which do not allows G0 to drive the SPI SCK pin properly, causing the SPI communication to fail.

This is the same reason why this device is not compatible with the following Hat series products: Hat Mini JoyC (SKU: U156), Hat Mini EncoderC (SKU: U157), and Hat 18650C (SKU: U080).


## StickS3 Module connections

| Device  | SCK   | MISO  | MOSI  | CS    | GDO0/CE   |
| ---     | :---: | :---: | :---: | :---: | :---:     |
| SD Card | 5     | 4     | 6     | 7     | ---       |
| CC1101  | 5     | 4     | 6     | 2     | 3         |
| NRF24   | 5     | 4     | 6     | 8     | 1         |
| PN532   | 5     | 4     | 6     | 43    | --        |
| WS500   | 5     | 4     | 6     | **    | **        |
| LoRa    | 5     | 4     | 6     | **    | **        |

** WS500 need to be configured in brucePins.config to set the pis according to your need.

GPS, PN532 and SI4713 uses the Grove Connector port.

| Device  | SCL/RX | SDA/TX |
| ---     | :---:  | :---:  |
| PN532   | 9      | 10     |
| SI4713  | 9      | 10     |
| GPS     | 9      | 10     |

Serial TX/RX interface with other devices:

| Device  | RX     | TX     |
| ---     | :---:  | :---:  |
| UART    | 44     | 43     |
