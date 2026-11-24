# ESP32-S3 peripheral drivers

Chip-level capabilities exposed by ESP-IDF's `peripherals` API (the same
surface Arduino-ESP32's higher-level APIs are built on top of). This is
the "what can this chip actually do at the hardware level" reference —
use it when the user needs more than what a board's high-level library
(M5Unified, M5GFX, M5Cardputer, etc.) exposes, or is doing bare-metal
ESP-IDF work.

## RMT (Remote Control Transceiver)

Generates and captures precisely-timed pulse trains in hardware, offloading
the CPU from bit-banging. Despite the name, it's rarely used for actual IR
remotes anymore in hobbyist projects — its most common modern use is
driving WS2812/NeoPixel-style addressable LEDs (the `led_strip` component
built on RMT), where the strict sub-microsecond timing requirements make
software bit-banging unreliable. Also used for genuine IR transmit/receive
(a Cardputer Adv's IR emitter, for instance) and any other protocol needing
exact pulse widths (custom one-wire-style sensors, some servo protocols).
Has dedicated TX and RX channels — check the channel count in the specific
ESP-IDF version's docs, it has grown across ESP-IDF releases.

## LEDC (LED Control / PWM)

Hardware PWM generator, despite the LED-focused name it's the general-purpose
PWM peripheral. Used for LED brightness/dimming, simple tone/buzzer output,
and non-precision motor speed control. Multiple channels grouped into timer
groups sharing a base frequency — if the user needs several PWM outputs at
different frequencies, they need to understand the timer-group constraint
rather than just allocating channels freely. For anything needing
closed-loop motor control (current sensing, complementary/dead-time PWM
pairs for H-bridges), point them at MCPWM instead — LEDC is the simpler,
more limited peripheral.

## I2S (Inter-IC Sound)

Digital audio I/O — connects to codec chips, microphones (I2S PDM or
standard), and speaker amps. On the Cardputer Adv this is how the ES8311
codec's data lines work. Beyond audio, I2S's high-throughput
parallel/serial DMA transfer capability makes it usable as a general fast
data bus for things like driving certain parallel-interface displays — an
unintuitive but real use case if the user is chasing display refresh
performance and SPI isn't fast enough.

## ADC (Analog-to-Digital Converter)

Reads analog voltages — battery level sensing, analog sensors, raw mic
level. ESP32-series ADCs are known for non-linearity and reference-voltage
drift; if a user reports "my ADC readings are noisy/inconsistent," point
them at ESP-IDF's ADC calibration API (`esp_adc/adc_cali`) rather than
assuming their sensor or wiring is at fault — this is a very common
ESP32 gotcha, not unique to the S3.

## Capacitive touch sensor

Touch-button input using the chip's built-in capacitive sensing, no extra
components needed beyond a copper pad/wire. Good for simple touch buttons;
not a replacement for a real touchscreen controller (that's a separate I2C
chip like a GT911/FT-series/CST-series part on boards that have a touch
display — see the relevant board's SKILL.md for that).

## PCNT (Pulse Counter)

Hardware pulse counting with configurable edge detection, glitch filtering,
and up/down counting modes — built for rotary encoders, flow meters, and
tachometers where doing this in a GPIO ISR would be too slow or miss
pulses at high rates.

## MCPWM (Motor Control PWM)

Purpose-built for motor control: complementary PWM outputs with dead-time
insertion (for H-bridge/BLDC driving), capture inputs for feedback signals,
and fault handling. Reach for this instead of LEDC whenever the user is
actually driving a motor rather than just dimming an LED.

## TWAI (Two-Wire Automotive Interface)

ESP-IDF's name for a CAN bus controller (CAN 2.0B compatible). Needs an
external CAN transceiver chip — the ESP32-S3 only provides the controller
logic, not the differential bus driver. Relevant for M5Stack's CAN-related
Units (e.g. a CAN unit or RollerCAN-style product) rather than typical
handheld-board firmware.

## SDMMC host driver vs. SD SPI host driver

Two different ways to talk to an SD card. SDMMC uses the dedicated SD/MMC
peripheral (faster — 1-bit or 4-bit wide bus) but consumes specific pins
and isn't available on every board's wiring. SD SPI reuses a standard SPI
peripheral in slower single-bit mode but works over whatever pins the
board's SPI bus uses. Check the specific board's pinout/schematic to know
which one is actually wired up — M5Stack boards vary here, and picking the
wrong driver for the board's wiring just won't work rather than degrading
gracefully.

## GPTimer and dedicated GPIO

`GPTimer` is a general-purpose hardware timer/counter — precise periodic
interrupts, one-shot delays, or timestamp capture without burning a
FreeRTOS task on `vTaskDelay` polling. "Dedicated GPIO" is a lower-latency
GPIO access path (CPU instruction-level control rather than going through
the normal GPIO matrix/register path) for cases needing very tight,
jitter-free bit-banging timing that even a fast ISR can't guarantee.

## Temperature sensor

Reads the **die's internal temperature**, not ambient room temperature —
useful for thermal-throttling logic or as a rough sanity check, not as an
environmental sensor. If a user wants ambient temperature, they need an
external sensor (M5Stack's ENV Unit family, for instance), not this
peripheral.

## SPI (master, slave, slave half-duplex) and UART and I2C

These exist too and work as expected from any ESP32 experience — included
here mainly to note that ESP32-S3 supports multiple independent instances
of each (check the current ESP-IDF docs for exact counts, they've shifted
across versions), so a board can legitimately have more than one SPI bus
or more than one I2C bus if its schematic calls for it. Don't assume a
single shared bus without checking that board's pinout reference.

**I2C: use the `i2c_master` driver** (`driver/i2c_master.h`, ESP-IDF
v5.2+). The legacy `driver/i2c.h` API is deprecated and on the way out in
ESP-IDF v6 — don't write new code against it, and don't mix the two
drivers in one firmware. Create the bus with `i2c_new_master_bus()`, add a
device handle per address with `i2c_master_bus_add_device()`, and keep it
**synchronous** (`trans_queue_depth = 0`, the default). Async queuing can
exhaust the driver's transaction pool during a register burst and returns
`ESP_ERR_INVALID_STATE`.

## Hardware AES/SHA under ESP-IDF v6 (mbedTLS 4 / PSA Crypto)

ESP-IDF v6 moves to **mbedTLS 4**, which leaves only the **PSA Crypto
API** public. The legacy `mbedtls_aes_*` / `mbedtls_gcm_*` calls that most
ESP32 examples on the web use are gone. Hardware AES/SHA acceleration is
still used underneath, but you reach it through PSA:

- `psa_crypto_init()` once at startup.
- `psa_import_key()` → a `psa_key_id_t`, then `psa_cipher_encrypt()` /
  `psa_cipher_decrypt()` (or the multi-part `psa_cipher_*_setup/update/
  finish`), `psa_aead_*` for GCM/CCM, `psa_hash_*` for SHA.
- **Import a key once and cache the key ID.** Importing per message works
  but is slow; a field build caches imported keys and reuses them.
  `psa_destroy_key()` when the key changes.

This comes from one field build on ESP-IDF v6 (Oct 2026). If a user is on
v5.x, the legacy API still works there. Check the ESP-IDF version before
choosing which one to write.

## Security: HMAC and Digital Signature (DS) peripherals

Hardware-accelerated HMAC computation and a Digital Signature peripheral
used for TLS client-certificate-style authentication without exposing the
private key to application code. Niche — only relevant if the user is
doing device-identity/secure-provisioning work, not typical hobbyist
firmware. Not covered in depth here; point to ESP-IDF's security guides if
this comes up.
