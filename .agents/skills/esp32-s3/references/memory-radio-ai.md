# ESP32-S3: memory, dual-core concurrency, radio coexistence, AI acceleration

## PSRAM and flash: check the module variant before assuming it's there

The bare ESP32-S3 chip has no PSRAM. Whether a board has PSRAM depends on
which module variant it uses:

- Module part numbers with an `R2` suffix have 2MB of **quad** PSRAM
  in-package.
- Module part numbers with an `R8` suffix (e.g. `N8R8`, `N16R8`) have 8MB
  of **octal** PSRAM in-package — faster than quad, but uses more of the
  chip's dedicated SPI pins to do it.
- Bare `N`-only modules (no `R` suffix) have no PSRAM at all.

**Don't assume PSRAM is present or free GPIOs are unaffected** — octal
PSRAM/flash modules dedicate additional GPIOs to the wider SPI bus versus
quad variants. Check the specific board's schematic/pinout reference for
exactly which pins that costs, rather than assuming a fixed number — it's
module- and board-dependent enough that asserting a specific pin range
here would be misleading.

Enabling PSRAM is a build-time config choice, not automatic:

- **Arduino IDE**: Tools menu → PSRAM → set to the mode matching the
  board's actual module (OPI/Octal for `R8` boards, QSPI/Quad for `R2`
  boards). Leaving this on "Disabled" when the board does have PSRAM is a
  very common cause of "why do I run out of memory so fast" reports.
- **ESP-IDF**: `idf.py menuconfig` → Component config → ESP PSRAM → enable
  and select the matching SPIRAM mode
  (`CONFIG_SPIRAM_MODE_OCTAL`/`CONFIG_SPIRAM_MODE_QUAD`).

If a user's board should have PSRAM (per its board skill's quick specs)
but `esp_get_free_heap_size()`/`ESP.getPsramSize()` reports zero, the build
config not matching the module is the first thing to check.

## Dual-core: PRO_CPU and APP_CPU

The two Xtensa LX7 cores are conventionally called `PRO_CPU` (core 0) and
`APP_CPU` (core 1) — this is FreeRTOS-SMP terminology carried over from
the original ESP32, still used on the S3.

- `xTaskCreatePinnedToCore(...)` pins a FreeRTOS task to a specific core;
  plain `xTaskCreate` lets the scheduler place it on either.
- In Arduino-ESP32's default setup, the WiFi/BT stack and the core Arduino
  runtime typically run on `PRO_CPU` (core 0), while the user's `loop()`
  runs on `APP_CPU` (core 1) — this is why a tight, blocking `loop()` can
  still coexist with WiFi without starving it, but a user who explicitly
  spawns their own heavy task on core 0 can absolutely starve the network
  stack.
- Interrupt service routines that must run reliably even while flash is
  busy (e.g. during an OTA write, or any other flash operation that
  temporarily disables cached-flash execution) need to be placed in
  IRAM via `IRAM_ATTR` — code left in flash can hang or crash if it's
  invoked from an ISR context while flash access is briefly unavailable.
  This is a real gotcha for timing-critical ISRs (e.g. a fast GPIO
  interrupt) combined with any flash-writing operation happening
  concurrently.

### Task stacks are small — keep big structs off them

An ESP-IDF task stack is whatever you passed to `xTaskCreate` — typically
2–8 KB — and every local variable lives there. A single ~3 KB message
struct declared as a local in a UI task was enough to overflow it in a
field build (Cardputer Adv, Oct 2026). Put large buffers and message
structs in `static` storage, on the heap, or in a queue item, and turn on
`CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY` (the default) so an overflow
panics with the task name instead of corrupting memory.
`uxTaskGetStackHighWaterMark(NULL)` tells you how close a task runs to
the limit.

## WiFi/BLE coexistence

The S3 has **one 2.4GHz radio** shared between WiFi and Bluetooth (there's
no separate radio per protocol) — the two are time-division multiplexed,
not truly simultaneous. Practical implications:

- WiFi station mode + BLE (scanning, advertising, or connected) generally
  coexists in a stable way — this is the common case (e.g. WiFi telemetry
  + a BLE peripheral role) and works reasonably well out of the box.
- WiFi **SoftAP** mode combined with BLE is less reliable — beacon
  transmission is fine, but a device actually connecting to the SoftAP
  while BLE is active has been reported as unstable. If a user needs both
  robustly, prefer WiFi station mode over SoftAP where possible.
- WiFi sniffer mode and ESP-NOW have similar coexistence limitations with
  BLE active.
- `CONFIG_ESP_COEX_SW_COEXIST_ENABLE` needs to be on (default in modern
  ESP-IDF) for the coexistence scheduler to manage this at all.
- Espressif's own guidance: don't tune custom WiFi power-save parameters
  unless they've been specifically tested with coexistence active — the
  defaults exist for a reason here, and non-default connectionless
  power-save settings are called out as a common cause of degraded
  coexistence behavior.

If a user reports "BLE drops packets/disconnects whenever WiFi is doing
something," or vice versa, this shared-radio time-slicing is the first
thing to explain — it's often not a bug in their code.

## SIMD/vector instructions for on-device AI

The Xtensa LX7 cores in the S3 include vector/SIMD instruction extensions
absent on the plain ESP32's LX6 cores. These aren't a dedicated NPU, but
they meaningfully accelerate the multiply-accumulate-heavy math behind
signal processing and neural network inference — real-world dot-product
benchmarks show roughly 4x (and depending on the operation, reportedly
much more) speedup over scalar code on the same clock speed.

Three libraries actually use this hardware:

- **esp-dsp** — Espressif's DSP function library (FFT, dot products,
  filters, matrix ops) with SIMD-optimized implementations.
- **ESP-DL** — Espressif's own deep-learning inference library, tuned for
  this chip.
- **TensorFlow Lite Micro** — runs small-to-medium quantized (int8) models;
  the S3 is a commonly-recommended target specifically because of this
  vector extension.

This matters for M5Stack use cases involving on-device wake-word/keyword
detection, simple vision models on camera-equipped boards, or any
sensor-fusion math heavy enough that scalar C would be a bottleneck. Point
users doing this kind of work at `esp-dsp` or `ESP-DL` rather than having
them hand-write scalar loops — the compiler will not auto-vectorize into
these extensions without going through code that's written to use them
(via the library or, for advanced cases, DSP-specific intrinsics).
