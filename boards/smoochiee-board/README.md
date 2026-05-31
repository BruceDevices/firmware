# Smoochiee Board Pinout

This board targets the original Smoochiee configuration (ST7789-based display) and button-only navigation.

## ESP32-S3 / Board Pins

| Function | GPIO |
| --- | --- |
| USB serial TX | 1 |
| USB serial RX | 2 |
| I2C SDA | 47 |
| I2C SCL | 48 |
| Up button | 41 |
| Down button | 40 |
| Right button | 38 |
| Left button | 39 |
| Select button | 0 |
| IR TX LED | 5 |
| IR RX LED | 4 |
| CC1101 CS | 46 |
| NRF24 CE | 21 |
| NRF24 CS | 14 |
| RGB LED | 45 |

## Display

- Driver: `ST7789` (original configuration)
- Resolution: 170x320
- Backlight pin: GPIO 6

## Notes

- Touch support is disabled; use the hardware buttons to navigate the UI.
