# Pinouts diagram to use Bruce

## USING CUSTOM BOARD, with SPI
| Device  | SCK   | MISO  | MOSI  | CS    | GDO0/CE   |
| ---     | :---: | :---: | :---: | :---: | :---:     |
| SD Card | 12    | 13    | 11    | 1     | ---       |
| CC1101  | 12    | 13    | 11    | 2     | 21        |
| NRF24   | 12    | 13    | 11    | 10    | 3         |
| WS500   | 12    | 13    | 11    | **    | **        |

** WS500 need to be configured in brucePins.config to set the pis according to your need.

| Device  | RX    | TX    | GPIO   |
| ---     | :---: | :---: | :---:  |
| GPS     | 44    | 43    | ---    |
| IR RX   |  ---  | ---   | 18/21* |
| IR TX   |  ---  | ---   | 17/3*  |

/* Pins 17 and 18 are used in touchscreen control, they will move to 18->21, 17->3 on touchscreen version

FM Radio, PN532 on I2C, other I2C devices, shared with GPS Serial interface.
I2C SDA: 44 -> will change to 18 if touch is detected
I2C SCL: 43 -> will change to 17 if touch is detected


## USING THE LILYGO SHIELD (SD_MMC Version),
Configured to be compatible with [Willy's firmware](https://willy-firmware.com/) devices
| Device  | SCK   | MISO  | MOSI  | CS     | GDO0/CE   |
| ---     | :---: | :---: | :---: | :---:  | :---:     |
| CC1101  | 43    | 2     | 3     | 1      | 44        |
| NRF24   | 43    | 2     | 3     | 18/16* | 17/44*    |
| WS500   | 43    | 2     | 3     | **     | **        |

/* Pins 17 and 18 are used in touchscreen control, they will move to 18->16, 17->44 on touchscreen version
** WS500 need to be configured in brucePins.config to set the pis according to your need.

| Device  | RX    | TX    | GPIO  |
| ---     | :---: | :---: | :---: |
| GPS     | 16    | 21    | ---   |
| IR RX   |  ---  | ---   | 44    |
| IR TX   |  ---  | ---   | 10    |

* Ir-RX need a switch to use with Willy board, you can set whatever other pin available
FM Radio, PN532 on I2C, other I2C devices, shared with GPS Serial interface. On Touchscreen device, they must use pins 18/17, sharing with Touchscreen driver.
I2C SDA: 16 -> 18 if touchscreen
I2C SCL: 21 -> 17 if touchscreen


# Bruce 1.13+ pin changes
T-Display S3 Touch uses pins 17 and 18 to control touchscreen, and it should not be interfered by NRF pins (on SD_MMC) or Infrared Pins (Custom Boards)

Same happens on SD_MMC environment, that uses Lilygo T-Display Shield, where pins 11, 12 and 13 are exclusive for SD Card, and were being used by GPS and I2C bus.. in this case, GPS and I2C changes to 16/21, and if touch is detected, I2C changes to 18/17 and NRF pins change to 16/44 respectivelly

## Manual Setup
If you are not happy with these changes, you can change them back to whatever you want:
* Activate Dev Mode: Scroll all the menu from Wifi->BLE... CFG->Wifi 5 times, non-stop and it will enable dev Mode
* Change The bus pins: CFG->Dev Mode->(Pins Menu)

