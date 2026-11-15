# Seeed Studio XIAO ESP32S3 (bare module)

Env: `seeedstudio-xiao-esp32s3`

The XIAO ESP32S3 / XIAO ESP32S3 Sense module on its own — ESP32-S3R8, 8 MB
flash, 8 MB octal PSRAM, native USB-C. **Headless**: the module has no panel
and no user button, so the build uses DisplayDrivers' no-op backend
(`USE_DUMMY_TFT`) and starts the WebUI (`bruceConfig.startupApp = "WebUI"`).
Everything is driven from the WebUI or the serial commands.

- Product page: <https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html>
- For the 1.47" touch display board, see `boards/seeedstudio-esp32s3-touch-147`
  — that one needs a **XIAO ESP32-S3 Plus**, whose extra pads (D15/D17/D18/D19)
  this module doesn't have.

## Pads → GPIO

From `variants/XIAO_ESP32S3/pins_arduino.h` in arduino-esp32.

| Pad | GPIO | XIAO label | Used by this build as |
|---|---|---|---|
| D0 | 1 | A0 | IR TX / one-pin RF TX |
| D1 | 2 | A1 | IR RX / one-pin RF RX, CC1101 GDO0, NRF24 CE |
| D2 | 3 | A2 | SD card CS |
| D3 | 4 | A3 | CC1101 / NRF24 / PN532 CS |
| D4 | 5 | SDA | I2C SDA |
| D5 | 6 | SCL | I2C SCL |
| D6 | 43 | TX | UART TX (GPS, CH9329 BadUSB) |
| D7 | 44 | RX | UART RX |
| D8 | 7 | SCK | SPI clock |
| D9 | 8 | MISO | SPI MISO |
| D10 | 9 | MOSI | SPI MOSI |
| — | 21 | `LED_BUILTIN` | unused (active low) |
| — | 0 | BOOT | left alone, so it stays usable for flashing |

The module's native USB-C carries the console (`ARDUINO_USB_CDC_ON_BOOT=1`),
which is what keeps D6/D7 free as a general-purpose UART, and it is also what
makes `USB_as_HID` (BadUSB / ducky typer / U2F) work.

## Pin defaults are only defaults

`interface.cpp` seeds `bruceConfigPins` with the table above — the XIAO
footprint's own labelled functions, so a stock XIAO expansion board or Grove
wiring works out of the box. From the second boot on, `/brucePins.conf` wins,
and everything (SD, CC1101, NRF24, PN532, I2C, UART, GPS, IR, RF) is remappable
from **Config > Set Device pins**.

One shared SPI bus (D8/D9/D10) serves the SD card, CC1101, NRF24 and PN532,
with only two free pads left for chip-selects — so SD + one radio at a time.
Re-map the CS pins if you wire more than that.

## Flashing

ESP32-S3, so the bootloader goes at **0x0**:

```
esptool --chip esp32s3 write-flash \
  0x0     .pio/build/seeedstudio-xiao-esp32s3/bootloader.bin \
  0x8000  .pio/build/seeedstudio-xiao-esp32s3/partitions.bin \
  0x10000 .pio/build/seeedstudio-xiao-esp32s3/firmware.bin
```

## Not bench-tested

This env has not been run on hardware. Nothing here is board-specific beyond
the pad map (the module has no panel, PMIC, gauge or buttons to get wrong), but
the pin defaults above are a sensible wiring, not a measured one.
