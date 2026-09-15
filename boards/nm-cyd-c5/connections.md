# Pinouts diagram to use Bruce

## USING NM-CYD-C5, with SPI / CC1101 and NRF24 work with NM-RF-HAT

| Device  | SCK   | MISO  | MOSI  | CS    | GDO0/CE | TFT_DC | TFT_RST | TFT_BL |
| ---     | :---: | :---: | :---: | :---: | :---:   | :---:  | :---:   | :---:  |
| Display | 6     | 2     | 7     | 23    | ---     | 24     | C5 RST  | 25     |
| SD Card | 6     | 2     | 7     | 10    | ---     | ---    |  ---    | ---    |
| CC1101  | 6     | 2     | 7     | 9*    | 8*      | ---    |  ---    | ---    |
| NRF24   | 6     | 2     | 7     | 9*    | 8*      | ---    |  ---    | ---    |

(*) CC1101, NRF24, W5500 use the same pinouts, need to add a switch on CS and CE/GDO0 to choose which to use.


If using ST7789 with XPT2046 fo touchscreen, in this case you have 2 GPIO available (0 and 28) to use on CC1101/NRF24
| Device  | SCK   | MISO  | MOSI  | CS    | IRQ   |
| ---     | :---: | :---: | :---: | :---: | :---: |
| Display | 6     | 2     | 7     | 23    | ---   |
| Touch   | 6     | 2     | 7     | 1     | ---   |


| Device  | RX    | TX    | GPIO  |
| ---     | :---: | :---: | :---: |
| GPS     | 4     | 5     | ---   |
| IR RX   |  ---  | ---   | 9     |
| IR TX   |  ---  | ---   | 8     |
| LED     |  ---  | ---   | 27    |
| 433 RX  |  ---  | ---   | 9     |
| 433 TX  |  ---  | ---   | 8     |

ESP32-C5 doesn't support USB-OTG, for BadUSB you need to use a CH9329 module

FM Radio, PN532 on I2C, other I2C devices, CH9329, Temperature and humidity sensor: BME280.
I2C SDA: 9
I2C SCL: 8

Serial interface to other devices (Flipper) - USB TypeC interface on NM-CYD-C5
Serial Tx: 11
Serial Rx: 12
