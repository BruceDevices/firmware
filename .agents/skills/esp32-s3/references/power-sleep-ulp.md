# ESP32-S3 power management: sleep modes and the ULP coprocessors

## Light sleep vs. deep sleep

| | Light sleep | Deep sleep |
|---|---|---|
| CPUs | Clock-gated, not powered off | Powered off entirely |
| RAM | Most RAM stays powered, contents preserved | Most RAM **off** — only RTC memory survives |
| Resume | Fast — execution continues where it left off | Slow — full reboot, runs from scratch (check `esp_reset_reason()`/`esp_sleep_get_wakeup_cause()` to detect a wake-from-deep-sleep boot) |
| Peripherals | Digital peripherals clock-gated | All APB-clocked digital peripherals off; only the RTC controller, RTC peripherals, and the ULP coprocessor stay powered |
| Typical use | Want to save power but keep application state and resume instantly (e.g. between short bursts of activity) | Want minimum power and don't mind a full restart (e.g. sleeping for hours between sensor readings) |

Because deep sleep wipes normal RAM, anything that must survive across a
sleep cycle (a counter, calibration data, a small state machine) needs to
live in **RTC memory**, which is explicitly preserved. In Arduino-ESP32
this is the `RTC_DATA_ATTR` variable attribute; in ESP-IDF it's placing
data in the `.rtc.data`/`RTC_DATA_ATTR` segment.

## Wake sources

- **Timer** — RTC controller's own timer, microsecond-resolution. Works
  from both light and deep sleep. The most common wake source for
  periodic-sensor-poll firmware.
- **GPIO (EXT0 / EXT1)** — wake on a specific pin level from deep sleep.
  EXT0 is a single pin; EXT1 is a bitmask of multiple RTC-capable pins with
  AND/OR trigger logic. Only RTC-capable GPIOs qualify — not every pin on
  the chip can be a deep-sleep wake source, check which pins are RTC GPIOs
  before wiring a wake button to an arbitrary pin.
- **GPIO wakeup (general)** — any GPIO, but **light sleep only**.
  **Trap:** `gpio_wakeup_enable(pin, GPIO_INTR_LOW_LEVEL)` switches that
  pin's interrupt type to *level* for normal operation too, not just for
  wake. If the pin is also an ISR source driven by a chip that holds its
  IRQ line low until a task services it (a keyboard controller, a sensor
  with a latched INT), the ISR re-fires continuously, the servicing task
  never runs, and the **interrupt watchdog** resets the chip. Fix: in the
  ISR call `gpio_intr_disable(pin)` and notify the task; the task clears
  the device's interrupt and then calls `gpio_intr_enable(pin)`. (Hit in
  the field on the Cardputer Adv's TCA8418 keyboard IRQ, G11.)
- **Touchpad** — the capacitive touch peripheral can wake the chip; works
  in both light and deep sleep on the S3.
- **ULP coprocessor** — the ULP runs its own program while the main cores
  sleep and can trigger a full wakeup when its own logic decides to (e.g.
  "woke up, sampled a sensor, value crossed a threshold — wake the main
  CPU now"). This is the mechanism for anything more sophisticated than
  "wake on a timer."
- **UART** — wake on incoming serial data via RX pin edge detection,
  **light sleep only**.

If the user needs deep-sleep-level power savings *and* something smarter
than a fixed timer (e.g. "wake only when a sensor value crosses a
threshold, checked every second, without waking the main CPU every time"),
that's exactly the ULP coprocessor's job — see below, and don't just tell
them to poll from the main core with a light-sleep loop, which burns far
more power.

Exact current-consumption figures for each mode/peripheral-combination are
not reproduced here — they vary meaningfully by which peripherals stay
enabled and change slightly across chip revisions. Check the datasheet's
power consumption tables (linked from the main `SKILL.md`) for numbers to
quote to a user rather than guessing.

## Two different low-power coprocessors: ULP-FSM vs. ULP-RISC-V

This is an S2/S3-specific point of confusion — the original ESP32 only has
the FSM one.

- **ULP-FSM**: a simple finite-state-machine coprocessor programmed in a
  restricted assembly-like language (or a small C-like DSL via
  `ulp-elf-binutils`/`ulp_fsm` tooling). Very limited instruction set,
  intended for the simplest "read a sensor, compare, maybe wake" loops.
- **ULP-RISC-V**: a genuine RISC-V core that can run code compiled with a
  normal RISC-V GCC toolchain — real C, not a restricted DSL. Far more
  capable (can do I2C transactions to poll a sensor, more complex
  decision logic) while still running independently of the main Xtensa
  cores during sleep.

For anything beyond the most trivial "read one ADC channel and compare to
a constant" logic, prefer ULP-RISC-V — it's the one worth reaching for on
an S3 project doing real low-power sensor work. Only the two ULP variants
share the same RTC_SLOW_MEM region and wake-trigger mechanism; they are
alternative choices, not both-at-once.

Both ULP variants are documented under "ULP RISC-V Coprocessor
programming" and (older doc tree) "ULP Coprocessor (FSM-based)" in
ESP-IDF's system-level API reference — see the link in the main
`SKILL.md`.

## Automatic light sleep (power management)

`CONFIG_PM_ENABLE=y` + `CONFIG_FREERTOS_USE_TICKLESS_IDLE=y`, then
`esp_pm_configure()` with `light_sleep_enable = true`, lets the chip
light-sleep whenever every task is blocked. Before turning it on, check
every GPIO you've passed to `gpio_wakeup_enable()` for the level-interrupt
trap above.

## A note on "why is my code slow to wake / not saving power"

Two common mistakes worth checking for before assuming deep-sleep
configuration is wrong:

- Forgetting that peripherals initialized before sleep (WiFi, a sensor
  left powered) can hold power domains on — explicitly deinit/power down
  what isn't needed before calling into deep sleep.
- Expecting variables to survive deep sleep without `RTC_DATA_ATTR` (or
  the ESP-IDF equivalent) — a very common "why did my counter reset to
  zero" bug report.
