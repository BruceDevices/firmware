# Wireless M-Bus for Bruce

[Deutsche Anleitung](wireless-mbus.de.md)

This experimental native RF app adds passive Wireless M-Bus reception through
the CC1101. It does not transmit, accept keys, or decrypt protected meter values.
It is a firmware extension, not a JavaScript app that can be copied to an SD card.
Bruce's current JavaScript interface does not provide the packet reception needed here.

## What Wireless M-Bus is used for

Wireless M-Bus (wM-Bus, EN 13757-4) carries meter readings over radio, for example
from water, gas, heat and electricity meters or heat-cost allocators to a reader
or gateway. This enables remote collection of consumption data.

It is particularly widespread in Europe. Regional profiles and frequency bands
vary; this implementation specifically receives T1/C1 at 868.95 MHz.
See the [M-Bus overview](https://m-bus.com/) and
[TI's Wireless M-Bus overview](https://www.ti.com/en-us/technologies/wired-wireless-connectivity/wireless-m-bus).

## Hardware and validation

- Tested hardware: LilyGO T-Embed CC1101 Plus, using the
  `lilygo-t-embed-cc1101` build target. This source has no separate Plus target.
- T1 and C1 at 868.95 MHz; C1 frame formats A and B are implemented.
- On 11 November 2026, the device owner reported very good reception on this board.
  This does not separately verify every mode, meter type, data field or SD operation.
- Other boards and external CC1101 arrangements have not been tested by this project.
- The implemented receiver and decoder have host tests; see below.

## Build and installation

Build the checkout containing this feature. The original development base is
`a59213f3ec6cd302fa32c2042b2700c9c24d991a`.

The tested local build used Python 3.13, PlatformIO 6.2.0 and the
`lilygo-t-embed-cc1101` environment. The branch also includes the FastLED 3.10.3
pin and an objcopy build-hook compatibility fix needed by the local Windows build.

One Windows PowerShell setup, run from the repository root with Python 3.13 available:

```powershell
py -3.13 -m venv .tools\venv
$env:PLATFORMIO_CORE_DIR = "$PWD\.tools\platformio"
$env:PLATFORMIO_CACHE_DIR = "$PWD\.tools\platformio-cache"
$env:TEMP = "$PWD\.tools\tmp"
$env:TMP = $env:TEMP
New-Item -ItemType Directory -Path $env:TEMP -Force | Out-Null
& .\.tools\venv\Scripts\python.exe -m pip install platformio==6.2.0 --cache-dir .tools\pip-cache
& .\.tools\venv\Scripts\python.exe -m platformio run -e lilygo-t-embed-cc1101
```

The build creates the app at `.pio/build/lilygo-t-embed-cc1101/firmware.bin`
and the merged image `Bruce-lilygo-t-embed-cc1101.bin` in the repository root.
For a normal standalone ESP32-S3 installation, write the merged image at address
`0x0` using esptool. Never write the app-only image at `0x0`.
Choose the COM port that belongs to your device.

A standalone flash example is:

```powershell
& .\.tools\venv\Scripts\python.exe -m pip install esptool --cache-dir .tools\pip-cache
$devicePort = 'COM7' # Replace with your device's port.
& .\.tools\venv\Scripts\python.exe -m esptool --chip esp32s3 --port $devicePort write-flash 0x0 Bruce-lilygo-t-embed-cc1101.bin
```

Wait for the write verification to finish and then open the RF menu on the device.

The full image contains the bootloader, partition table and application for
standalone Bruce on an ESP32-S3 with 16 MB flash. The documented hardware test
used an internal-flash erase followed by a fresh Bruce installation.

The build still reports warnings in existing Bruce/display/Bluetooth/audio code.
Successful compilation is not evidence that every existing firmware function works.

## Use

1. In **Radio Frequency > Config**, select the CC1101 RF module if necessary.
2. Open **Radio Frequency > Wireless M-Bus > Scan T1/C1 868.95**.
3. Wait for complete telegrams with valid link-layer CRCs. Meter intervals can be long.
4. Use the wheel to select a device; click to open its details.
5. Scroll through the details. Reception pauses in this view; click or go back to resume.
6. Go back from scanning to stop reception and return to the app menu.
7. Select **Export devices to SD** to save the device overview, or **Clear results**
   to reset it. Leaving the app discards the in-memory session.

The list holds up to 96 distinct devices, grouped by manufacturer and complete
link-layer address (ID, version and type). Details include supported headers,
RSSI, LQI, count and reception time since boot. Known numeric DIF/VIF records
are interpreted only when the application data is explicitly unencrypted.
Unknown formats remain raw bytes. A hex dump may include ciphertext and is labelled accordingly.

## SD export

Files are written as `/wmbus/devices-<boot-ms>-<number>.jsonl`, with one JSON object
per device containing its latest valid telegram and metadata. This is a snapshot,
not continuous packet logging. `frame_hex` omits DLL CRC bytes and normalizes the
L byte; it is not a byte-for-byte recording of the on-air signal. The original L
value is retained in `wire_l`. Timestamps are milliseconds since boot.

## Limits and feedback

Only the stated modes and frequency are supported. S/R/N modes, 169 MHz and other
frequencies are outside this implementation. Encrypted values remain encrypted;
manufacturer-specific, compact and unsupported records are not universally decoded.
The shared display/SD/radio SPI bus can cause reception gaps during display updates.
Invalid CRCs, truncated packets and FIFO overflows are discarded. The device table
is limited to 96 entries. A valid CRC does not authenticate the sender.

Further testing should cover T1/C1-A/C1-B against a reference receiver, repeated
telegram counts, unencrypted and protected records, SD export/error handling,
navigation and use of other RF apps after exiting. When reporting a result,
include board, firmware commit, mode if known, and steps to reproduce; redact meter
IDs and personal consumption data from public examples.

## Host tests and sources

With Python 3 and a GCC/Clang C++17 compiler supporting UndefinedBehaviorSanitizer:

```sh
python tests/wmbus/run.py
```

Use `--cxx /path/to/compiler` to select a compiler. Alternatively, install the
`ziglang` Python package in a local virtual environment and run
`python tests/wmbus/run.py --zig` (the local Windows tests use this route).
Outputs go under `.pio/wmbus-host-tests` and are not source files.

Tests cover CRC, T1/C1 A/B framing, corruption, truncation, encryption boundaries,
20,000 randomized inputs, FIFO fragmentation, long frames, overflow/timeout
recovery and radio-register restoration. The fake-radio test compiles the actual
production Receiver implementation with a simulated CC1101; it does not test RF propagation or the UI.

Protocol references: [TI AN067](https://www.ti.com/lit/an/swra234a/swra234a.pdf),
[TI C/T framing](https://www.ti.com/lit/an/swra522d/swra522d.pdf),
[CC1101 datasheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf),
[CC1101 errata](https://www.ti.com/lit/er/swrz020e/swrz020e.pdf),
[wMbus-lib](https://github.com/SzczepanLeon/wMbus-lib),
[wmbusmeters](https://github.com/wmbusmeters/wmbusmeters), and
[libmbus](https://github.com/rscada/libmbus).
These projects were consulted for radio settings and format definitions; no
decryption library is integrated. The implementation was developed with Codex
assistance. Code is distributed under AGPL-3.0-or-later; see [LICENSE](../LICENSE).
