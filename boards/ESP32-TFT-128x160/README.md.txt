# ESP32 Generic + 1.8" ST7735 TFT (128×160)

Hi, I'm **RED-collabs**.

This board support was added because I accidentally purchased a **1.8" ST7735 128×160 SPI TFT** instead of the recommended display. I modified the firmware so Bruce boots and runs correctly on this hardware.

If you have the same display, you can use this board definition without modifying the source code.

## Requirements

- Visual Studio Code
- PlatformIO extension
- ESP32 development board
- ST7735 1.8" 128×160 SPI TFT

## Build

platformio run -e esp32-tft-128x160

##Upload

platformio run -e esp32-tft-128x160 -t upload

## TFT

| TFT | ESP32  |
|-----|--------|
| VCC | 3.3V   |
| GND | GND    |
| SCK | GPIO18 |
| SDA | GPIO23 |
| CS  | GPIO5  |
| DC  | GPIO16 |
| RST | GPIO17 |
| BL  | 3.3V   |

## SD Card

| SD   | ESP32  |
|------|--------|
| CS   | GPIO4  |
| MOSI | GPIO23 |
| MISO | GPIO19 |
| CLK  | GPIO18 |

## Buttons

| Function | GPIO   |
|----------|--------|
| UP       | GPIO33 |
| DOWN     | GPIO27 |
| SELECT   | GPIO25 |
| BACK     | GPIO26 |
