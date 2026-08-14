# Third-party code and attribution

Bruce is licensed under the **GNU Affero General Public License v3.0 or later**
(AGPL-3.0-or-later); see [`LICENSE`](LICENSE).

Parts of Bruce's RF / sub-GHz module (`src/modules/rf/`) are **derived from**
other free-software projects. Those portions were modified to fit Bruce's native
RMT engine, data model and UI, but remain the work of their original authors and
are used under their respective licenses. Each affected source file carries a
header pointing back to this document.

> This file documents attribution for copyleft compliance. It is not legal
> advice; if you redistribute Bruce, review these obligations yourself.

## Sub-GHz protocols / KeeLoq

### Flipper Zero firmware — GPL-3.0-or-later
- Upstream: <https://github.com/flipperdevices/flipperzero-firmware>
- License: GNU General Public License v3.0
- Copyright (C) Flipper Devices Inc. and the flipperzero-firmware contributors.
- Used by: the KeeLoq block cipher, the manufacturer "learning" key-derivation
  schemes, the KeeLoq frame decoder/encoder framing, the `.sub` keystore format
  and protocol identities.
  Files: `src/modules/rf/protocols/rf_keeloq.{h,cpp}`,
  `src/modules/rf/protocols/rf_decoder.{h,cpp}`,
  `src/modules/rf/protocols/rf_encoder.{h,cpp}`.

### Momentum firmware — GPL-3.0-or-later
- Upstream: <https://github.com/Next-Flip/Momentum-Firmware>
- License: GNU General Public License v3.0
- Copyright (C) the Momentum Firmware contributors.
- Used by: the extended KeeLoq manufacturer list and the model for the
  encrypted built-in keystore (`keeloq_mfcodes`), which Bruce mirrors by
  shipping an AES-encrypted keystore decrypted at runtime as a fallback.
  Files: `src/modules/rf/protocols/rf_keeloq.cpp`,
  `src/modules/rf/protocols/rf_keeloq_mfcodes_data.h` (generated),
  `tools/gen_mfcodes.py`.

> Note: the KeeLoq manufacturer keys themselves originate from the respective
> device manufacturers. They are proprietary key material; their redistribution
> is a separate concern from the GPL attribution above and is the redistributor's
> responsibility to evaluate.

### rc-switch (RCSwitch) — LGPL-2.1-or-later
- Upstream: <https://github.com/sui77/rc-switch>
- License: GNU Lesser General Public License v2.1
- Copyright (C) 2011 Suat Özgür and the rc-switch contributors.
- Used by: the classic OOK protocol timing table (the numbered `RcSwitch_N`
  protocols and the factor-based `{high,low}×te` model) and the
  capture/decode and send state machines that Bruce re-implemented over its
  native RMT engine.
  Files: `src/modules/rf/protocols/rf_registry.{h,cpp}`,
  `src/modules/rf/protocols/rf_decoder.{h,cpp}`,
  `src/modules/rf/protocols/rf_encoder.{h,cpp}`.

## NFC / RFID (ST25R3916)

The ST25R3916 NFC front-end driver (`src/modules/rfid/ST25R3916.{h,cpp}`) is a
native re-implementation over a fork of ST's RFAL stack, but several parts —
particularly the card-emulation (listen / passive-target) path — were **derived
from** the Momentum firmware, itself a fork of the Flipper Zero firmware. Those
portions were rewritten to drive the ST25R3916 register interface directly
(the RFAL fork's high-level `rfalListenStart` is a stub), but the register
sequences and protocol state machines follow the upstream NFC HAL/stack.

### Momentum firmware (Flipper Zero NFC stack) — GPL-3.0-or-later
- Upstream: <https://github.com/Next-Flip/Momentum-Firmware>
- Based on: <https://github.com/flipperdevices/flipperzero-firmware>
- License: GNU General Public License v3.0
- Copyright (C) the Momentum Firmware and flipperzero-firmware contributors.
- Used by:
  - The NFC-A listen / passive-target register configuration (MODE.targ +
    `om_targ_nfca`, PT memory load of UID/ATQA/SAK, OP_CONTROL / PASSIVE_TARGET
    / MASK_RX_TIMER setup and the target-mode IRQ mask), which follows the
    Flipper `furi_hal_nfc` sequence on the same ST25R chip family.
  - The MIFARE Classic listener-side Crypto1 emulation: authentication
    handshake, manual-parity TX/RX handling and the encrypted READ/WRITE
    service from the loaded dump.
  - The MIFARE Ultralight / NTAG21x listener command handling — `GET_VERSION`,
    `READ_SIG` (ECC signature), `READ_CNT`, `PWD_AUTH`/`PACK` and the amiibo
    (NTAG215, UID-derived password, `80 80` PACK) behaviour.
  - The FeliCa (NFC-F) and Type 4 Tag (ISO-DEP / NDEF) emulation responders.
  Files: `src/modules/rfid/ST25R3916.cpp`.

### Crypto1 cipher (Crapto1 / Proxmark lineage) — public reimplementation
- References: "Dismantling MIFARE Classic" (Garcia et al., 2008) and the public
  Crapto1 reference; the same algorithm is used by the Flipper / Momentum and
  Proxmark NFC stacks.
- Used by: the compact Crypto1 stream cipher that backs both reader-side
  MIFARE Classic auth/read/write and the listener-side emulation, since the
  RFAL fork has no native Crypto1 support.
  Files: `src/modules/rfid/crypto1.{h,cpp}`.

## Bundled libraries (linked dependencies)

Fetched via PlatformIO `lib_deps` (see `platformio.ini`) and linked into the
firmware. Each library keeps its own license; the URL is the exact source Bruce
builds against. Consult each upstream repository for the authoritative license
text and copyright holders — the SPDX identifiers below are provided as a guide.

### Bruce/community forks (adjusted to build in Bruce)

Several dependencies are **forks** maintained for the project: they carry
patches for the pioarduino ESP32 core, memory reductions, API changes or
bug-fixes that upstream has not merged. They remain the work of their original
authors under the original license; only the fork host differs.

- **IRremoteESP8266** — fork of <https://github.com/crankyoldgit/IRremoteESP8266>
  (LGPL-2.1). Bruce uses <https://github.com/bmorcelli/IRremoteESP8266> (and
  `BorisKofman/IRremoteESP8266#Espressif-version-3` in the light build) for
  ESP32-Arduino-3.x compatibility.
- **SmartRC-CC1101-Driver-Lib** — fork of the ELECHOUSE CC1101 driver
  <https://github.com/LSatan/SmartRC-CC1101-Driver-Lib> (MIT). Bruce uses
  <https://github.com/bmorcelli/SmartRC-CC1101-Driver-Lib>.
- **Adafruit BusIO** — fork <https://github.com/emericklaw/Adafruit-BusIO_Bruce>
  (`1.17.2-bruce.1`) of <https://github.com/adafruit/Adafruit_BusIO> (MIT).
- **Adafruit PN532** — fork <https://github.com/emericklaw/Adafruit-PN532_Bruce>
  (`1.3.3-bruce.1`) of <https://github.com/adafruit/Adafruit-PN532> (BSD).
- **mquickjs** — <https://github.com/BruceDevices/mquickjs>, the JS interpreter
  built on Fabrice Bellard's QuickJS / quickjs-ng (MIT). Powers the Bruce
  JavaScript interpreter.
- **WireGuard-ESP32-Arduino** — <https://github.com/sayacom/WireGuard-ESP32-Arduino>
  (`feature/esp-netif` branch), an Arduino port of Jason A. Donenfeld's WireGuard
  (the embedded crypto is derived from the reference implementation).
- **OneWire** — fork <https://github.com/bmorcelli/OneWire> (`patch-1`) of
  Paul Stoffregen's OneWire (MIT-style).
- **ESP32-PsRamFS** — fork <https://github.com/bmorcelli/ESP32-PsRamFS>
  (`patch-1`) of <https://github.com/tobozo/ESP32-PsRamFS> (MIT).
- **Arduino_MFRC522v2** — <https://github.com/rennancockles/Arduino_MFRC522v2>
  fork of <https://github.com/OSSLibraries/Arduino_MFRC522v2> (Unlicense).
- **SimpleCLI** — <https://github.com/rennancockles/SimpleCLI> fork of
  <https://github.com/spacehuhntech/SimpleCLI> (MIT).
- **ESP-ChameleonUltra** — <https://github.com/bmorcelli/ESP-ChameleonUltra>
  (ChameleonUltra host-side protocol).
- **ESP-Amiibolink** — <https://github.com/bmorcelli/ESP-Amiibolink>.
- **ESP-PN532BLE / ESP-PN532-UART / ESP-PN532Killer** — whywilson's PN532
  companion libraries: <https://github.com/whywilson/ESP-PN532BLE>,
  <https://github.com/whywilson/ESP-PN532-UART>,
  <https://github.com/whywilson/ESP-PN532Killer>.
- **ST25R3916-fork** — <https://github.com/lewisxhe/ST25R3916-fork> and
  **NFC-RFAL-fork** — <https://github.com/lewisxhe/NFC-RFAL-fork>, forks of ST's
  RFAL / ST25R3916 driver (STMicroelectronics SLA / "MIX" license). Pinned to
  fixed commits and further patched at build time by `patch_library_conflicts.py`
  (IRQ-less I2C polling; NFC-A anti-collision timing). See the NFC/RFID section
  above for the code that drives these directly.
- **BER-TLV** — <https://github.com/huckor/BER-TLV> (EMV BER-TLV parser).

### Upstream libraries (unmodified, from the PlatformIO registry)

- **Time** — `paulstoffregen/Time` (LGPL-2.1) —
  <https://github.com/PaulStoffregen/Time>
- **LibSSH-ESP32** — ESP32 port of libssh (LGPL-2.1) —
  <https://github.com/ewpa/LibSSH-ESP32>
- **NTPClient** (`3.2.1`, MIT) — <https://github.com/arduino-libraries/NTPClient>
- **ESP32Time** (`2.0.6`, MIT) — <https://github.com/fbiego/ESP32Time>
- **ArduinoJson** (MIT) — <https://github.com/bblanchon/ArduinoJson>
- **ESP8266Audio** & **ESP8266SAM** (`earlephilhower`, LGPL-2.1 / mixed) —
  <https://github.com/earlephilhower/ESP8266Audio>,
  <https://github.com/earlephilhower/ESP8266SAM>
- **TinyGPSPlus** (`mikalhart`, LGPL-2.1) —
  <https://github.com/mikalhart/TinyGPSPlus>
- **FFT** (`tinyu-zhao/FFT`) — <https://github.com/tinyu-zhao/FFT>
- **NimBLE-Arduino** (`h2zero`, `2.5`, Apache-2.0) —
  <https://github.com/h2zero/NimBLE-Arduino>
- **RF24** (`nrf24`, `1.4.11`, GPL-2.0/MIT) —
  <https://github.com/nRF24/RF24>
- **Adafruit Si4713 Library** (`1.2.3`, BSD) —
  <https://github.com/adafruit/Adafruit-Si4713-Library>
- **JPEGDecoder** (`Bodmer`) — <https://github.com/Bodmer/JPEGDecoder>
- **AnimatedGIF** (`bitbank2`, Apache-2.0) —
  <https://github.com/bitbank2/AnimatedGIF>
- **PNGdec** (`bitbank2`, `1.1.2`, Apache-2.0) —
  <https://github.com/bitbank2/PNGdec>
- **AsyncTCP** & **ESPAsyncWebServer** (`ESP32Async`, LGPL-3.0) —
  <https://github.com/ESP32Async/AsyncTCP>,
  <https://github.com/ESP32Async/ESPAsyncWebServer>
- **FastLED** (`3.10.3`, MIT) — <https://github.com/FastLED/FastLED>
- **RadioLib** (`jgromes`, `7.4.0`, MIT) — <https://github.com/jgromes/RadioLib>

## Vendored libraries (`lib/`)

Libraries checked into the tree under `lib/` (PlatformIO private libraries).
Some are Bruce-authored HAL glue; others are copied third-party code, sometimes
trimmed or adapted. Each retains its original author's license.

- **`lib/TFT_eSPI`** — Bodmer's TFT_eSPI (`2.5.43`), FreeBSD/MIT/BSD; itself
  derived from Adafruit_ILI9341 (MIT) and Adafruit_GFX (BSD). See
  `lib/TFT_eSPI/license.txt`. Upstream: <https://github.com/Bodmer/TFT_eSPI>.
- **`lib/TFT_eSPI_QRcode`** — Domenico Silletti's QR renderer for TFT_eSPI —
  <https://github.com/dsilletti/TFT_eSPI_QRcode>. Bundles a compact QR encoder
  (`src/qrencode.c` / `qrbits.h`) of the widely-copied lightweight `qrencode`
  lineage (Reed-Solomon ECC + frame masking).
- **`lib/Bad_Usb_Lib`** — BadUSB HID stack assembled from several sources, each
  keeping its own header:
  - `USBHIDKeyboard.*`, `USBHIDMouse.*`, `KeyboardLayout*` — Arduino LLC /
    Peter Barrett (LGPL-2.1); layouts derived from the Arduino Keyboard library.
  - `USBHID.*` — Espressif Systems (Apache-2.0), from the Arduino-ESP32 TinyUSB
    HID driver.
  - `BleKeyboard.*` — BLE HID keyboard/mouse (NimBLE port, in the lineage of
    T-vK's ESP32-BLE-Keyboard).
  - `CH9329_Keyboard.*` — CH9329 UART-HID bridge support.
- **`lib/PN532_SRIX`** — ST SRIX/SRI NFC support, `arduino-pn532-srix` by "Lilz"
  (GPL-3.0 / LGPL-3.0), refactored to reuse only Adafruit_PN532 constants. Header
  in `pn532_srix.h`.
- **`lib/HAL`** — Bruce's own hardware-abstraction layer
  (<https://github.com/pr3y/Bruce>). Wraps display backends (LovyanGFX, M5GFX,
  TFT_eSPI) and bundles:
  - `io_expander/Adafruit_AW9523.*` — Adafruit AW9523 GPIO expander driver (BSD),
    from <https://github.com/adafruit/Adafruit_AW9523>; plus a `PCA9555` driver.
  - `sd_card/*` — SD / SD_MMC / `sd_diskio` adapted from the Arduino-ESP32 SD
    library (Espressif / Arduino LLC, LGPL-2.1).
- **`lib/utility`** — M5Stack support code: `AXP192.*` (AXP192 PMIC),
  `bq27220.*` (TI BQ27220 fuel gauge) and `Keyboard.*` (M5Cardputer keyboard
  matrix by "Forairaaaaa"), from M5Stack's board libraries (MIT).
- **`lib/RTC`** — `cplus_RTC` (M5Stack Core/StickC BM8563) and `pcf85063_RTC`
  (NXP PCF85063) real-time-clock drivers.
- **`lib/CYD-touch`** — resistive/capacitive touch for Cheap-Yellow-Display
  boards; pin definitions from rzeldent's
  `platformio-espressif32-sunton` board support.
- **`lib/mquickjs_headers`** — headers generated from the **mquickjs** fork
  above (see `gen_mqjs_headers.py`).
