# src/hal

Shared hardware-abstraction code for board `interface.cpp` files: input
polling (buttons, encoder, touch), power ICs (charger, fuel gauge), and
backlight PWM. Every `hal_*.cpp` file here compiles for **every** board env
(it lives under `src/`, which PlatformIO includes unconditionally) and is
selected at compile time by `build_flags` macros set per board in
`boards/<env>/platformio.ini`.

A board only needs to fill a small `Device*` struct (see `device.h`) in
`_setup_gpio()` and call the matching `hal_*_init()`/`hal_*_poll()`/
`hal_*_read()` functions — no board-specific driver code required for
anything covered here. See `boards/_New-Device-Model/interface.cpp` for a
commented worked example of every module below, and
`boards/README.md` for the full new-board porting process.

## Layout

- `device.h` — pure-data structs (`DeviceButtons`, `DeviceTouch`,
  `DeviceEncoder`, `DevicePmic`, `DeviceGauge`) a board fills in
  `_setup_gpio()` and hands to the `hal_*` functions below. No logic, no
  `<Arduino.h>` dependency — safe to include from any board regardless of
  what hardware it has.
- `inputs/buttons.*` — raw-GPIO button polling.
- `inputs/encoder.*` — rotary encoder + its select/esc buttons.
- `inputs/touch.*` — touchscreen drivers.
- `inputs/keyboard.*` — **intentionally empty.** TCA8418/I2C-custom/
  GPIO-matrix keyboards work differently enough from each other that a
  shared HAL isn't worth it; keyboard reading stays board-specific in each
  `interface.cpp` (see `lilygo-t-lora-pager`, `lilygo-t-deck`,
  `m5stack-cardputer` for reference implementations).
- `power/pmic.*` — battery-charger IC.
- `power/gauge.*` — fuel-gauge IC.
- `bright/bright.*` — backlight PWM curve.

## Conventions

- Function naming: `hal_<module>_init(...)`, `hal_<module>_poll(...)` (or a
  more specific verb, e.g. `hal_gauge_get_percent()`).
- Chip/family selection is a `build_flags` macro set per board, mirroring
  the existing `HAS_*` convention (e.g. `-DPMIC_BQ25896`,
  `-DGAUGE_BQ27220`, `-DTOUCH_CTRL_GT911`).
- To avoid pulling in a chip-specific library for boards that don't need
  it, all library-specific `#include`s and code inside a `hal_*.cpp` file
  must sit behind the board's selection macro (`#ifdef PMIC_BQ25896 ...
  #endif`), never at file scope — PlatformIO's dependency finder (LDF)
  respects these guards when deciding which libraries to link. When a
  macro isn't defined, every `hal_*` function still exists and returns a
  neutral failure value (`false`/`-1`/no-op) instead of failing to link.
- A `hal_*` function must never depend on `<Arduino.h>` unless the
  underlying vendor library requires it (XPowersLib, bq27220,
  RotaryEncoder, SensorLib all do) — prefer `src/idf/BRUCE_platform.h`
  wrappers (`launcherGpioRead`, `launcherMillis`, ...) for anything else.
- Boards using a higher-level abstraction that already covers input/power
  (e.g. M5Unified's `M5.Touch`/`M5.BtnA`/`M5.Power`) fall outside this HAL
  by design — wire them up directly in the board's own `interface.cpp`
  instead of forcing them through `Device*`/`hal_*`.

## `inputs/buttons.*`

Raw-GPIO button polling, no debounce library — for `HAS_1_BUTTON` /
`HAS_3_BUTTONS` / `HAS_5_BUTTONS` / `HAS_6_BUTTONS` (single/double/long-press
timing baked into each `hal_buttons_poll_N()`) and `HAS_2_BUTTONS` (backed
by the ESP-IDF `button` component instead of raw polling, gated by
`BUTTONS_IDF_COMPONENT=1`).

- `hal_buttons_init(cfg, count)` / `hal_buttons_poll_1/_3/_5/_6(cfg)` — call
  `_init` once from `_setup_gpio()`, the matching `_poll_N` every
  `InputHandler()` cycle. `count` must match which `_poll_N` you call.
  `cfg.pullup = false` for boards without internal/external pull-ups.
- `hal_buttons_init_2(cfg, long_press_ms = 600)` / `hal_buttons_poll_2()` —
  the `HAS_2_BUTTONS` pair, needs `-DBUTTONS_IDF_COMPONENT=1` (the component
  itself is vendored in `lib/ESP32_Button`, so no `lib_deps` entry).
  btn1: short click → Next, double-click/hold → Sel. btn2: short click →
  Prev, double-click/hold → Esc. Double-click also raises Sel/Esc the same
  way a long press does.
- 1/3/5/6-button layouts (which logical button maps to which pin) are
  documented as comments in `device.h`'s `DeviceButtons` fields and in
  `platformio.ini`'s `HAS_*_BUTTON` flags.

## `inputs/encoder.*`

Rotary encoder + its Sel/Esc buttons, backed by `mathertel/RotaryEncoder`
(vendored in `lib/RotaryEncoder`, no `lib_deps` entry needed). Guarded end-to-end by `#if
defined(HAS_ENCODER)` (stubs otherwise), so the LDF never needs
`RotaryEncoder` in `lib_deps` for boards without it.

- `hal_encoder_init(cfg, mode = EncoderLatchMode::TWO03)` — call once from
  `_setup_gpio()`. `mode` is `FOUR3`/`FOUR0`/`TWO03` depending on how the
  encoder's quadrature output latches; `TWO03` fits most cheap EC11-style
  encoders, `FOUR3` is needed by at least one board with an inverted
  Next/Prev feel (see `lilygo-t-lora-pager`, which also swaps `pin_a`/
  `pin_b` in its own `encoderCfg()` since `hal_encoder_poll()` has no
  invert flag).
- `hal_encoder_poll(cfg)` — call every `InputHandler()` cycle. Sets
  `NextPress`/`PrevPress` from rotation and `SelPress`/`EscPress` from
  `cfg.pin_sel`/`cfg.pin_esc`. Set `cfg.pin_esc = -1` if the board has no
  dedicated esc button (only the encoder + a select button).

## `inputs/touch.*`

Touchscreen drivers selected by one `TOUCH_CTRL_*` build flag:

| Macro | Chip / family | Bus | Notes |
|---|---|---|---|
| `TOUCH_CTRL_XPT2046` | XPT2046 (resistive) | SPI, via `CYD28_TouchscreenR` (bit-banged, or the display's own SPI when `hal_touch_init(..., xpt_shared_spi=true)`) | Pins are separate `CYD28_TouchR_*` macros in `platformio.ini`, not `DeviceTouch` fields. Needs `-DHAS_RESISTIVE_TOUCH=1` for the calibration screen. |
| `TOUCH_CTRL_GT911` | GT911 | I2C | Reports its own resolution. |
| `TOUCH_CTRL_CST8XX` | CST816/CST820/CST92xx family | I2C | Auto-detects across the CST8xx family. |
| `TOUCH_CTRL_FT6X36` | FT6206/FT6236/FT6336(U)/FT3267/FT5336/FT3068 family | I2C | One driver covers the whole family — there's no separate "FT6336" driver. Polling mode (`interruptPolling()`), not IRQ-driven. |
| `TOUCH_CTRL_GT9895` | GT9895 | I2C | Digitizer grid is larger than the panel and the chip doesn't report native resolution over I2C — the board must supply it via `DeviceTouch.raw_width`/`raw_height` (a fixed vendor/panel constant, e.g. 1060x2400). |
| `TOUCH_CTRL_HI8561` | HI8561 | I2C | Already reports in panel coordinates, no scaling needed. |

API:

- `hal_touch_init(cfg, i2c_addr = 0x5D, xpt_shared_spi = true)` — call from
  `_setup_gpio()` (or `_post_setup_gpio()` if the panel/bus needs to come up
  first).
- `hal_touch_read(cfg, BruceTouchPoint &out)` — call every `InputHandler()`
  cycle. XPT2046/FT6X36 return a raw point and the HAL applies
  `cfg.SwapXY`/`MirrorX`/`MirrorY` on the host side; GT911/CST8xx/GT9895/
  HI8561 hand rotation to the driver itself
  (`setSwapXY()`/`setMirrorXY()`). Either way, `MirrorX`/`MirrorY`/`SwapXY`
  are indexed by the display's current rotation (0-3) — copy the values
  from an already-working board with the same panel rather than guessing;
  they only become obvious once you can touch the 4 corners on real
  hardware.
- `hal_touch_apply(t, wakeUp = true)` — wakes the screen (swallowing this
  press as the wake-up tap, same convention as every other input source)
  and publishes the point to `touchPoint`/`touchHeatMap()`. Returns `false`
  if `InputHandler()` should return immediately instead of processing the
  point further.
- `hal_touch_read_raw(out)` / `hal_touch_get_resolution(width, height)` — a
  raw escape hatch for `TOUCH_CTRL_GT911`/`_CST8XX`/`_FT6X36`/`_GT9895`/
  `_HI8561`: returns the point exactly as the driver reports it (no
  swap/mirror/scaling) plus the controller's own native resolution, for a
  board whose rotation/scale math doesn't fit `hal_touch_read()`'s model
  (e.g. a panel whose native touch resolution doesn't match its pixel
  dimensions).
- `hal_touch_set_home_button(x, y, cb, user_data = nullptr)` — works for
  both `TOUCH_CTRL_GT911` and `TOUCH_CTRL_CST8XX` (shared `SensorLib`
  callback). GT911 fires it off a hardware key bit rather than proximity
  to the given coordinate (which it ignores — pass `0, 0`); CST8xx uses the
  coordinate.
- `hal_touch_set_threshold(value)` — `TOUCH_CTRL_FT6X36`-only passthrough
  for boards that need a non-default touch-sensitivity register.
- `hal_touch_disable_auto_sleep()` — `TOUCH_CTRL_CST8XX`-only passthrough.
- `DeviceTouch.i2c_bus` (`void*`, cast to `TwoWire*` inside `touch.cpp`,
  `nullptr` = the global `Wire`) — for a board whose touch controller sits
  on a second I2C bus (e.g. `Wire1`). `void*` rather than `TwoWire*` so
  `device.h` — included by every board regardless of what sensors it has —
  stays Arduino-free.
- `settings.cpp`'s touch-calibration screen (`HAS_RESISTIVE_TOUCH`) reaches
  the XPT2046 driver via `extern CYD28_TouchR touch;`, which `touch.cpp`
  defines with external linkage specifically so that keeps working.

## `power/pmic.*`

Battery-charger IC. Currently implements **`PMIC_BQ25896` only** (via
`XPowersLib`). A board on a different charger (AXP2101, AXP192, SY6970,
...) stays fully board-specific in its own `interface.cpp` until HAL
support for that chip is added — don't force it through `DevicePmic`.

- `hal_pmic_init(cfg, input_current_limit_ma = 3250)` — `Wire` must already
  be begun. Applies a fixed operating point (charge target 4208mV,
  precharge 64mA, constant current 832mA, current-limit pin disabled,
  measurement + charging enabled).
- `hal_pmic_init_via_callbacks(address, readReg, writeReg,
  input_current_limit_ma = 3250)` — same, for a PMIC sharing an I2C bus
  another driver already began (no second `Wire.begin()`).
- `hal_pmic_shutdown()` — powers the device off via the PMIC.
- `hal_pmic_get_input_current_limit_ma()` / `_get_charger_constant_curr_ma()`
  / `_get_system_voltage_mv()` — passthrough getters.
- `hal_pmic_enable_otg()` / `_disable_otg()` -- 5V boost output (`USE_BOOST`
  code paths). `hal_pmic_is_charging()` / `_is_charge_done()` /
  `_get_batt_voltage_mv()` / `_disable_bat_load()` -- charger status for boards
  that estimate battery % from the charger (no fuel gauge).
  `DevicePmic.charge_target_mv`/`charge_current_ma` override the default
  4208mV/832mA, and leaving `pin_sda`/`pin_scl` at -1 uses the driver's own
  default I2C init.
- `hal_pmic_get_ntc_percent()` — battery percent estimated from the
  charger's own NTC reading, for a board with no separate fuel gauge
  (instead of `hal_pmic_get_system_voltage_mv()`).
- With no `PMIC_*` macro defined, every function above is a no-op returning
  `false`/`-1`.

## `power/gauge.*`

Battery level. Two sources, chosen at compile time:

- **`GAUGE_BQ27220`** -- BQ27220 fuel-gauge IC. A board with a MAX17048 or
  other gauge stays board-specific until HAL support is added.
- **`ANALOG_BAT_PIN`** (no `GAUGE_*` macro) -- voltage divider on an ADC pin.
  Tuned by build flags: `ANALOG_BAT_MULTIPLIER` (divider ratio, default
  `2.0`), `ANALOG_BAT_MIN_MV` (0%, default `3300`), `ANALOG_BAT_MAX_MV`
  (100%, default `4100`). No board code is needed -- the default
  `getBattery()` in `core/utils.cpp` calls `hal_gauge_get_percent()`.
  `hal_gauge_init()` may override the pin/ratio/range through `DeviceGauge`.

- `hal_gauge_init(cfg)` -- BQ27220: sets the design capacity
  (`cfg.design_capacity_mah`) if it differs from what's programmed. Leave at 0
  to skip.
- `hal_gauge_get_percent()` -- 0-100, or -1 if unavailable/unread.
- `hal_gauge_is_charging()`.
- `hal_gauge_has_info()` / `hal_gauge_get_info(DeviceGaugeInfo&)` -- detailed
  BQ27220 readings (capacities, voltages, currents, time to empty), used by
  the device-info screen and the JS `getBatteryDetailed()`.

## `bright/bright.*`

Backlight PWM curve, shared by every plain-PWM/ledc backlight board:
`HAL_BRIGHT_PWM_FREQ=5000`, `HAL_BRIGHT_PWM_RES_BITS=8`,
`duty = pwm_min + round(pow(percent/100, gamma) * (pwm_max - pwm_min))`
with defaults `pwm_min=0, pwm_max=255, gamma=2.2` (`HalBrightCurve`). No
board currently overrides the curve — every plain-PWM board shares the
exact same feel on purpose.

- `hal_bright_attach(pin)` / `hal_bright_attach(pins[], count)` — call once
  from `_post_setup_gpio()`. Attaches one or more backlight pins to ledc at
  the shared frequency/resolution.
- `hal_bright_set(pin, percent, curve = HalBrightCurve())` /
  `hal_bright_set(pins[], count, percent, curve)` — call from
  `_setBrightness()`. The multi-pin overload is for a board with more than
  one backlight rail driven together (e.g. screen + keyboard backlight, or
  warm+cool channels). Retries once via `ledcDetach()`/`ledcAttach()` if
  the first `ledcWrite()` fails.
- Not applicable to: boards driving brightness through a higher-level API
  instead of a raw PWM pin (M5Unified's `M5.Display.setBrightness()`, an
  AXP192 `ScreenBreath()` register write, a panel-controller
  `setBrightness()` over DSI/SPI, an external I2C backlight-controller
  chip) or boards with no backlight hardware at all (e-paper). Those keep
  their own `_setBrightness()` body untouched.

## Input task and the shared-SPI touchscreen

`taskInputHandler()` (main.cpp) runs `InputHandler()` in its own task on every
board, including the ones whose XPT2046 shares the display's SPI bus (the old
`USE_TFT_eSPI_TOUCH` flag that moved those reads into the main loop is gone).
The task holds a recursive mutex (`inputLock()`/`inputUnlock()`, mykeyboard.cpp)
while it updates the input globals; `check()` and `_getKeyPress()` take the same
mutex instead of suspending the task. The bus itself is protected by the SPI
transaction lock -- `CYD28_TouchscreenR` and TFT_eSPI (`SUPPORT_TRANSACTIONS`)
both take it.
