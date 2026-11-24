---
name: esp32-s3
description: Chip-level ESP-IDF capability reference for the ESP32-S3 SoC (Xtensa LX7 dual-core) — what the chip can do, distinct from any board's wiring. Use when a user wants to go beyond basic Arduino sketches and exploit ESP32-S3 hardware — RMT (IR/WS2812/custom protocols), LEDC (PWM), I2S (audio), ADC/touch sensing, PCNT (encoders), MCPWM (motor control), TWAI/CAN, SDMMC/SD-SPI, native USB OTG vs the USB-Serial-JTAG controller, sleep modes and wake sources, ULP-FSM/ULP-RISC-V coprocessors, octal/quad PSRAM config, dual-core FreeRTOS task pinning and IRAM/ISR placement, WiFi/BLE coexistence, and the SIMD instructions behind esp-dsp/ESP-DL/TFLite Micro. Trigger on "how do I use RMT/LEDC/I2S/ULP/deep sleep/dual core/PSRAM/USB OTG on an ESP32-S3", low-power/performance-tuned S3 firmware questions, or any M5Stack S3 board (Cardputer, Cardputer Adv, AtomS3, AtomS3R, CoreS3, StickS3, Stamp-S3) needing depth beyond its own SKILL.md. For pinouts/wiring use that board's skill — this is the shared chip layer it points into.
---

# ESP32-S3 chip capabilities

Bruce integration, based on [iot-forge/m5stack-skills at b56afab](https://github.com/iot-forge/m5stack-skills/tree/b56afab4dafc08e241962d1634981ae44650f74a/plugins/esp32-chips/skills/esp32-s3).
Use this as a chip-level reference. Get T-Embed CC1101 Plus wiring from the
LILYGO schematic and its Bruce board profile. Match ESP-IDF examples to the
version bundled by the selected Arduino-ESP32/PlatformIO profile. In USB work,
verify both Arduino USB flags, the physical PHY and the compiled descriptors.

This skill is the **chip layer**, not a board layer. A board skill (e.g.
`m5stack-cardputer-adv`) tells you what's wired to what on one specific
product; this skill tells you what the ESP32-S3 silicon itself can do —
the peripherals, low-power modes, memory options, and concurrency model
that are the same on every ESP32-S3 board regardless of vendor. Read a
board's own skill first for pin numbers and I2C addresses, then come here
when the user wants to go deeper on a capability than "call the vendor's
high-level library and move on" — e.g. writing a custom RMT-based protocol,
tuning deep-sleep wake behavior, pinning tasks to cores, or getting real
throughput out of I2S/USB.

## Not classic ESP32, not S2, not C3/C6/C5, not H2, not P4

M5Stack (and the ESP32 family generally) spans several very different chips
that are easy to conflate. Get this wrong and code silently targets the
wrong architecture or assumes a peripheral that doesn't exist.

| Chip | Cores / arch | Wireless | Native USB | Notable extras | Not on this chip |
|---|---|---|---|---|---|
| **ESP32-S3** (this skill) | 2× Xtensa LX7 @ up to 240MHz + 1 low-power coprocessor core | WiFi 4 (2.4GHz only) + BLE 5 | Yes — OTG + separate USB-Serial-JTAG | SIMD/vector instructions for AI (esp-dsp/ESP-DL/TFLite Micro), ULP-RISC-V *and* ULP-FSM | Thread/Zigbee/802.15.4, WiFi 6, MIPI-DSI/CSI |
| ESP32 (classic) | 2× Xtensa LX6 @ 240MHz | WiFi 4 + Bluetooth Classic + BLE 4.2 | No | Widest library/example coverage, oldest silicon | Native USB, SIMD extensions, ULP-RISC-V |
| ESP32-S2 | 1× Xtensa LX7 @ 240MHz | WiFi 4 only, **no Bluetooth at all** | Yes — OTG | Very low deep-sleep current | Second core, Bluetooth |
| ESP32-C3 | 1× RISC-V @ 160MHz | WiFi 4 + BLE 5 | No | Cheapest/smallest | Second core, native USB, SIMD |
| ESP32-C6 | 1× RISC-V @ 160MHz + LP core | WiFi 6 (2.4GHz) + BLE 5.3 + Thread/Zigbee (802.15.4) | No | First ESP32 with WiFi 6 + Matter-ready radio stack | Second core, native USB, hardware FPU |
| ESP32-C5 | 1× RISC-V @ 240MHz + LP core | WiFi 6 **dual-band (2.4+5GHz)** + BLE 5 + 802.15.4 | No | Only dual-band-WiFi6 ESP32 variant | Second core, native USB |
| ESP32-H2 | 1× RISC-V @ 96MHz | **No WiFi** — BLE 5 + Thread/Zigbee only | No | Purpose-built low-power mesh/Matter radio | WiFi entirely, second core |
| ESP32-P4 | 2× RISC-V @ 400MHz + LP core | **No radio at all** | Yes — OTG (high-speed) | MIPI-DSI/CSI, H.264 encode, up to 32MB in-package PSRAM, most raw CPU power | Any wireless — always pairs with a companion chip (M5Stack's Tab5 pairs it with a C6) |

If the user's board pairs an ESP32-S3 with a *separate* radio or a
different main SoC (not the case for any current M5Stack board, but worth
checking if a new one shows up), treat that companion chip as out of this
skill's scope — it needs its own chip skill.

## Quick specs

- **CPU**: dual-core Xtensa LX7, up to 240MHz, plus one low-power
  coprocessor core (see "Power management" below)
- **SRAM**: ~512KB on-chip
- **Flash**: external SPI/QSPI/OSPI, not on the bare chip die — size
  (4/8/16/32MB) depends on the module variant the board uses
- **PSRAM**: not present on every module. Many common module variants embed
  it in the same package — e.g. `N8R8`/`N16R8` (octal, 8MB) or `R2` (quad,
  2MB) suffixes on the module part number — see `references/memory-radio-ai.md`
  before assuming PSRAM is available or free GPIOs are unaffected.
- **Wireless**: WiFi 4 (802.11b/g/n, 2.4GHz only, no 5GHz), Bluetooth LE 5.0
  (no Bluetooth Classic, no Thread/Zigbee/802.15.4)
- **USB**: native USB OTG (full-speed, TinyUSB-based, device *and* host
  mode) plus a separate fixed-function USB-Serial-JTAG controller — see
  `references/usb.md`, this trips people up
- **GPIO**: up to 45 pins on the bare chip; how many are actually free
  depends on the module (octal PSRAM/flash modules reserve some) and the
  board (M5Stack boards route many to on-board peripherals — check that
  board's schematic and pinout)
- **AI acceleration**: SIMD/vector instruction extensions for int8/fp32
  workloads — not a dedicated NPU, but real speedup for on-device inference;
  see `references/memory-radio-ai.md`
- **Security**: hardware HMAC and Digital Signature peripherals, plus the
  usual ESP-IDF flash encryption / secure boot v2 support (not detailed in
  this skill — see ESP-IDF's security guides if the user needs this)

## Peripheral capability map

Full detail, with typical use cases and gotchas, is in
`references/peripherals.md`. Quick index of what's covered there:

| Peripheral | For |
|---|---|
| RMT | Precisely-timed signal generation/capture — IR remotes, WS2812/NeoPixel, custom one-wire-style protocols |
| LEDC | PWM — LED dimming, buzzers/tone generation, simple motor speed control |
| I2S | Digital audio in/out (mic, speaker, codec chips), also usable as a fast generic parallel data bus |
| ADC | Analog sensing (battery voltage, analog sensors, mic level) |
| Capacitive touch sensor | Touch-button input without extra hardware |
| PCNT | Hardware pulse counting — rotary encoders, flow meters, tachometers |
| MCPWM | Motor control PWM — H-bridges, ESCs, servo-adjacent timing needs beyond what LEDC offers |
| TWAI | CAN bus (automotive/industrial protocols) |
| SDMMC / SD-SPI | SD card access, two different driver paths with different speed/pin tradeoffs |
| GPTimer / dedicated GPIO | General-purpose hardware timers; low-latency bit-banged GPIO |
| Temperature sensor | Internal die temperature (not ambient — don't let a user mistake it for a room-temperature sensor) |

## Power management

Light sleep, deep sleep, wake sources, and the two very different
low-power coprocessors (ULP-FSM vs. ULP-RISC-V) are in
`references/power-sleep-ulp.md`. Read this before telling a user "just
call `esp_deep_sleep_start()`" if they actually need something to keep
running (a sensor poll, a timer) while the main cores are off.

## USB: two controllers, one physical port

The S3 has both a native USB OTG peripheral and a separate USB-Serial-JTAG
controller sharing the same physical D+/D- pins — they are not the same
thing and can cause real confusion when a board exposes only one USB
connector. Full detail in `references/usb.md`.

## Memory, concurrency, radio coexistence, and AI acceleration

PSRAM/flash config (and the GPIO cost of octal variants), dual-core task
pinning and ISR/IRAM placement, WiFi/BLE coexistence behavior, and the
SIMD instructions behind esp-dsp/ESP-DL/TFLite Micro are all in
`references/memory-radio-ai.md`.

## Which M5Stack boards use this chip

Cardputer, Cardputer Adv, AtomS3, AtomS3R, CoreS3, and StickS3 are
ESP32-S3-based, along with the various Stamp-S3 module variants. Tab5 is
**not** — it's ESP32-P4 with an ESP32-C6 radio co-processor; don't apply
this skill's chip-level claims to Tab5 without checking the `m5stack-tab5`
skill first. Core2 is classic ESP32, not S3 — see the `esp32` skill.

## Official resources

- Datasheet (PDF): https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- Product page: https://www.espressif.com/en/products/socs/esp32-s3
- ESP-IDF peripherals API reference (esp32s3 target): https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/index.html
- Sleep modes: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/sleep_modes.html
- ULP-RISC-V coprocessor programming: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/ulp-risc-v.html
- USB Device Stack (native OTG / TinyUSB): https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/usb_device.html
- USB Serial/JTAG console: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html
- RF coexistence (WiFi/BLE): https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/coexist.html
- Flash/PSRAM configuration: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/flash_psram_config.html

## Working with the user

- If they're getting compile/link errors from code copy-pasted for a
  different ESP32 variant, check the "Not classic ESP32..." table above
  first — RISC-V-vs-Xtensa and missing-peripheral mismatches are the most
  common cause.
- If they ask for USB functionality and only get garbled output or nothing
  at all, check `references/usb.md` — it's very often the OTG-vs-Serial/JTAG
  mismatch.
- If they want something to keep running during sleep (a timer, a sensor
  poll, blinking an LED on a schedule) rather than just "wake up
  periodically and do work," point them at the ULP coprocessors in
  `references/power-sleep-ulp.md` rather than fighting `esp_deep_sleep_start()`.
- If they're combining WiFi and BLE and seeing flaky behavior (especially
  SoftAP + BLE together), check `references/memory-radio-ai.md`'s
  coexistence section before assuming it's an application bug.
- **ESP-IDF v6 changes that break older examples:** hardware AES/SHA is
  reached through the PSA Crypto API only (mbedTLS 4), see
  `references/peripherals.md`. Write I2C against the `i2c_master` driver,
  not legacy `driver/i2c.h`. Ask which ESP-IDF version they're on before
  writing crypto or I2C code.
- This is chip-level guidance, cross-checked against Espressif's own
  ESP-IDF docs at time of writing — but ESP-IDF version-to-version API
  changes happen. If the user hits a compile error on a specific function
  signature, point them at the current ESP-IDF docs for their target
  version rather than assuming this file is byte-exact.
