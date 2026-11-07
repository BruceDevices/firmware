# Heltec WiFi LoRa 32 V4-R8 + Expansion Kit V2

Build target: `heltec-v4-expansion-v2`.

This target is for the ESP32-S3 V4-R8 with 16 MB flash, 8 MB OPI PSRAM and the
Expansion Kit V2. It uses the high-frequency SX1262 hardware. The original
Expansion Kit, bare WiFi LoRa 32 boards and low-frequency radio variants need
their own hardware definitions.

```sh
pio run -e heltec-v4-expansion-v2
```

The board uses Bruce's standard 16 MB partition layout, native USB console and
USB HID support. As on Bruce's other ESP32-S3 targets, the console uses hardware
CDC at boot and selecting a USB function starts TinyUSB on the same connector.
The serial console can disconnect when that function takes over; restart to
restore it. Bootloader download mode is available by holding USER/G0
while resetting or connecting USB. Restore a full flash backup at address zero
if recovery to the previous firmware is required.

## Installation images

The build automatically creates these files in the repository root:

| File | Installation route |
| --- | --- |
| `Bruce-heltec-v4-expansion-v2-FACTORY-0x0.bin` | Standalone USB/web flashing at address `0x0` |
| `Bruce-heltec-v4-expansion-v2-LAUNCHER-INSTALL.bin` | Select **Install** on this file inside Bmorcelli's Launcher |
| `Bruce-heltec-v4-expansion-v2.bin` | Existing Bruce merged-image name, retained for normal tooling |
| `Bruce-heltec-v4-expansion-v2.sha256` | SHA-256 checksums for all three BIN names |
| `Bruce-heltec-v4-expansion-v2.json` | Application identity, partition layout and artifact hashes |

All three BIN files are byte-identical merged images from the same application
build. The explicit names identify the intended installation workflow. Flashing
the factory image at `0x0` replaces the installed bootloader and partition table;
use Launcher's **Install** action to keep Launcher as the boot environment.

Copy `LAUNCHER-INSTALL.bin` to the microSD card, open it in Launcher and select
**Install**. Launcher's merged-image installer reads the partition table at
`0x8000`, extracts the application at `0x10000` and chooses its destination
partition. The installer also sees the `spiffs` entry even though no filesystem
payload is included. Bruce needs this partition for LittleFS even when SD is
available; a raw application-only BIN cannot communicate that requirement.
This format was reviewed against [Launcher commit `2f43aa7`](https://github.com/bmorcelli/Launcher/blob/2f43aa7bd3ef03e770c0a97135cbcfcc79c15aba/src/sd_functions.cpp#L750).

At that revision, a 16 MB device receives a default empty 448 KiB `spiffs`
partition for this package, instead of reserving the standalone image's large
filesystem. Bruce initializes LittleFS on that partition. Store larger files on
SD. See [Launcher's default data-partition sizes](https://github.com/bmorcelli/Launcher/blob/2f43aa7bd3ef03e770c0a97135cbcfcc79c15aba/src/partition_table_model.cpp#L171).
Older Launcher versions can have different installation behavior; verify the
installed version, free flash and filesystem creation before relying on them.
Launcher can reuse an existing `spiffs` partition. Bruce uses LittleFS and
formats that partition if mounting fails, so back up any existing application
filesystem before installing. Keeping other application binaries does not
guarantee their shared filesystem contents are preserved.

To return to Launcher, exit the active Bruce feature, restart with USER/G0
released and tap the center of Launcher's startup screen. Holding USER/G0
during reset enters USB download mode instead. Launcher's button mapping also
differs from Bruce's: USER click moves forward and hold/double-click selects;
IO click moves backward and hold/double-click returns.

The packager checks ESP32-S3 image identity, partition-table MD5, application
fit and exact application bytes before creating the named files. It does not
prove a hardware installation succeeded. Launcher installation, boot,
settings retention, other installed applications and return to the Launcher
menu must be verified on the device and recorded with the Launcher version.

To regenerate the names and checksums from an existing build without compiling:

```sh
python boards/heltec-v4-expansion-v2/package_release.py --build-dir .pio/build/heltec-v4-expansion-v2
```

## Hardware

| Function | Connection |
| --- | --- |
| ST7789 display | 240 × 320, HSPI: SCK16, MOSI15, MISO45, CS47, DC48, reset21 |
| Backlight | GPIO44, active high PWM |
| microSD | Same HSPI bus, CS3; mounted through Bruce's shared SPI HAL |
| CHSC6xxx touch | I²C address `0x2e`, SDA17, SCL18; polled without the optional interrupt pad |
| Display/touch/SD power | GPIO40, active low |
| SX1262 | FSPI: SCK9, MOSI10, MISO11, CS8, reset12, DIO1 14, BUSY13 |
| LoRa front end | Power7, CSD2, CTX5; 1.8 V TCXO |
| L76K GNSS | 9600 baud, MCU RX39/TX38, enable42 active low, PPS41 |
| Buzzer | GPIO4 |
| Battery voltage | GPIO1 through a 390 kΩ / 100 kΩ divider |
| Buttons | USER/G0 active low; KEY/GPIO46 active high |

TFT_eSPI owns HSPI and the SD driver uses that same SPI instance. The second
controller is reserved for the SX1262. Touch starts after the display reset,
because GPIO21 resets both devices. GNSS runs from boot and Bruce's existing GPS
modules parse its NMEA data without startup commands intended for other receivers.
The touchscreen uses `Wire1` explicitly. The default `Wire` SDA/SCL aliases are
disconnected, so an unavailable optional I²C accessory cannot fall back to the
touch pins and detach their system bus.

The LoRa target is constrained to 863–928 MHz occupied bandwidth. Raw SX1262
transmit drive is conservatively capped at 0 dBm pending measurement of the
board's external amplifier; this is **not** a claim of 0 dBm antenna output or
calibrated maximum power. Use the correct antenna and regional settings.

Both hardware SPI controllers are occupied. CC1101, NRF24, NFC, Ethernet,
infrared, FM radio, microphone and RGB hardware are not fitted. No spare
multiwire expansion bus is available in this target. GPIO6 is the sole
unreserved pad and may be explicitly configured for a suitable single-wire
accessory. Built-in SD, touch, LoRa and GNSS pins are fixed; loading or saving
configuration restores their wiring and rejects conflicting optional pins.

## Controls

- Touch Bruce's on-screen controls; the bottom strip provides Previous, Select
  and Next. The upper-left touch area returns/back.
- Tap USER/G0 to select; hold it for 650 ms to go back.
- Tap KEY/GPIO46 for Next; hold it for 650 ms for Previous.
- Bruce's on-screen keyboard supports text entry using touch or the buttons.
- The first input while the screen is dimmed/off wakes the screen. Release and
  press again to activate a control.

All four display rotations map touch against the complete physical display,
including the navigation strip. Button clicks are debounced and emitted on
release; a hold does not also emit a click when released. Input sampling runs
each input-task tick and uses an eight-event queue, so the UI's event-consumption
delay does not discard a rapid second touch.

## Power

Battery percentage is an approximate voltage estimate, not a fuel-gauge
measurement. The kit provides no readable charging-status signal to this port.

Power off and Deep sleep enter ESP32 deep sleep and disable the display/SD
rail, GNSS main supply and radio front end. They do not disconnect the battery
or remove the L76K backup supply. USER/G0 wakes the device. The hardware power
button controls the expansion board's physical power latch.

## Validation

Host tests for CHSC decoding, rotation and button transitions are in
`tests/heltec_v4_expansion_v2`. Compilation and those tests do not establish
physical-device acceptance. Display/touch alignment, SD readback, GPS fixes,
battery estimates, USB, LoRa reception/transmission and sleep/wake must be
verified on the V4-R8 Expansion Kit V2 before claiming full hardware support.

## Hardware references

- [Heltec Expansion Kit V2](https://heltec.org/project/v4-r8-ex/)
- [Expansion board V2.03 schematic](https://resource.heltec.cn/download/WiFi_LoRa_32_Expansion_Kit-V2/schematic/Expansion_board_V2.03.pdf)
- [Heltec V4-R8 factory example](https://github.com/HelTecAutomation/Heltec_ESP32/blob/dc5e7f92701b80b0e30d90980820dd742b99df01/examples/Factory_Test/Lora32_R8_V4.3_Exp_Board_HF/Lora32_R8_V4.3_Exp_Board_HF.ino)
- [LORA Tunnel hardware implementation](https://github.com/valleytechsolutions/LORA-Tunnel), attribution in [NOTICE.md](NOTICE.md)
