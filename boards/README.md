# Adding a new board to Bruce

This file is the porting guide. It is written to be followed step by step, by a person or
by an AI agent, using `boards/_New-Device-Model/` as the template.

**The rule that matters most: copy the template, don't invent a layout.**
`boards/_New-Device-Model/` is kept in sync with the code and every flag in it is
commented. Everything below is about filling it in correctly.

---

## 1. Repository layout

```
.
├── platformio.ini              root env: shared build_flags, lib_deps, src filter
├── custom_4Mb.csv / custom_8Mb.csv / custom_16Mb.csv   partition tables
├── include/precompiler_flags.h defaults for every optional -D (read this when unsure
│                               whether a flag has a fallback)
├── src/hal/                    shared input/power/brightness HAL + its own README.md
└── boards
    ├── _New-Device-Model       ← THE TEMPLATE. Copy this folder.
    │   ├── _New-Device-Model.ini
    │   └── interface.cpp
    ├── _jsonfiles
    │   ├── [mcu].json          esp32, esp32s3, esp32c3, esp32c5, esp32c6, esp32p4,
    │   │                       esp32p4_es, esp32s2
    │   ├── [mcu].h             generic Arduino pin defaults for that chip family
    │   ├── pins_arduino.h      dispatcher: includes the right [mcu].h for the build
    │   └── variant.cpp
    └── [board]
        ├── [board].ini         PlatformIO env + all -D flags
        ├── interface.cpp       the board's only C++ file
        └── connections.md      (optional) wiring/pinout notes
```

There is **no** per-board JSON and no per-board `pins_arduino.h`. `boards/_jsonfiles/`
holds one generic board JSON per MCU family; a board points at the JSON for its chip
(`board = esp32s3`) and overrides in its `.ini` only what differs.

`platformio.ini` picks boards up automatically through `extra_configs = boards/*/*.ini`,
so creating the folder is enough for `pio run -e <env>` to see the new env.

---

## 2. Procedure

### Step 1 — copy the template

```sh
cp -r boards/_New-Device-Model boards/<board>
mv boards/<board>/_New-Device-Model.ini boards/<board>/<board>.ini
```

`<board>` is the folder name; the env name inside the `.ini` (`[env:<env>]`) does not
have to match it, but keeping them equal avoids confusion.

Then, in the new `.ini`, update the three places that repeat the folder name:

```ini
[env:<env>]
build_src_filter = ${env.build_src_filter} +<../boards/<board>>
build_flags =
	${env.build_flags}
	-Iboards/<board>
```

`build_src_filter` is what compiles `interface.cpp`. **Forgetting it still links** —
`_setup_gpio()` and `InputHandler()` have weak no-op defaults in `src/main.cpp` — and the
board simply boots with no pins configured and no working buttons. If a freshly ported
board does nothing at all, check this line first.

### Step 2 — MCU, flash and PSRAM

```ini
board = esp32s3                            ; must match a file in boards/_jsonfiles/
board_build.partitions = custom_8Mb.csv    ; custom_4Mb / _8Mb / _16Mb.csv
board_upload.flash_size = 8MB
board_upload.maximum_size = 8388608
board_build.arduino.memory_type = qio_qspi ; qio_opi for octal PSRAM
```

Also add `-DBOARD_HAS_PSRAM=1` to `build_flags` when the device has PSRAM — it selects the
larger FreeRTOS task stacks in `include/precompiler_flags.h` and the PSRAM-only code paths.
A `memory_type` that doesn't match the hardware makes the board boot-loop.

### Step 3 — display

The display is configured entirely in the `.ini`. Pick the driver library with exactly one
of `USE_TFT_ESPI` / `USE_M5GFX` / `USE_LOVYANGFX` / `USE_GXEPD2` / `USE_EPD_PAINTER` /
`USE_DUMMY_TFT` (no flag at all = Arduino_GFX, the library default), add the matching
`lib_deps` entry, and see `lib/DisplayDrivers/README.md`. For `USE_TFT_ESPI` the panel
driver, pins, `TFT_WIDTH`/`TFT_HEIGHT`, SPI frequencies and `TFT_BL` are read directly by
TFT_eSPI out of these `-D` values — copy a board with the same panel.

### Step 4 — feature flags

Go through `boards/_New-Device-Model/_New-Device-Model.ini` top to bottom and uncomment
what the device really has. Every flag there carries its own comment; the ones that decide
the shape of `interface.cpp` are:

| Group | Flags | What it obliges you to do in `interface.cpp` |
|---|---|---|
| Buttons | `HAS_1_BUTTON`, `HAS_2_BUTTONS` (+`BUTTONS_IDF_COMPONENT`), `HAS_3_BUTTONS`, `HAS_4_BUTTONS`, `HAS_5_BUTTONS`, `HAS_6_BUTTONS` | fill a `DeviceButtons`, call `hal_buttons_init()` + `hal_buttons_poll_N()` |
| Encoder | `HAS_ENCODER` | fill a `DeviceEncoder`, call `hal_encoder_init()` + `hal_encoder_poll()` |
| Keyboard | `HAS_KEYBOARD` | no HAL — scan the keyboard yourself in `InputHandler()` |
| Touch | `HAS_TOUCH` + one `TOUCH_CTRL_*` | fill a `DeviceTouch`, call `hal_touch_init()` + `hal_touch_read()`/`hal_touch_apply()` |
| Power | `PMIC_BQ25896`, `GAUGE_BQ27220` | fill a `DevicePmic`/`DeviceGauge`, call `hal_pmic_init()`/`hal_gauge_init()` |
| Power | `ANALOG_BAT_PIN` (+ `_MULTIPLIER`/`_MIN_MV`/`_MAX_MV`) | nothing — the shared `getBattery()` handles it |

**At most one `HAS_x_BUTTON(S)` flag per env** — they are mutually exclusive, and
`include/precompiler_flags.h` already derives the extra ones a layout implies (e.g.
`HAS_6_BUTTONS` implies `HAS_5_BUTTONS` for the on-screen keyboard, `HAS_4_BUTTONS`
implies `HAS_3_BUTTONS`), so setting two by hand breaks that.

Every other input flag combines freely: a board may have buttons *and* `HAS_TOUCH` *and*
`HAS_KEYBOARD` *and* `HAS_ENCODER` at the same time. Real examples:
`lilygo-t-lora-pager` (encoder + keyboard), `marauder-touch` (encoder + touch),
`lilygo-t-display-s3-touch` (2 buttons + touch). `InputHandler()` then simply calls one
poll per source.

**Don't override the plain Arduino pin names** (`SDA`, `SCL`, `SS`, `TX`, `RX`, `MOSI`,
`MISO`, `SCK`, `A0`–`A19`, `LED_BUILTIN`, …) with `-D`. They are ordinary
`static const uint8_t` variables in `boards/_jsonfiles/[mcu].h` on purpose — turning them
into macros makes the substitution global and silently breaks third-party library code
that uses the same short names as local variables (this really did break a build during
the boards/ refactor). If the real wiring differs, give it a Bruce-specific name
(`SYS_I2C_SDA`, `SPI_SS_PIN`, …) and use that name explicitly in `_setup_gpio()`.

### Step 5 — `interface.cpp`

The template is a commented skeleton: uncomment and fill the blocks that apply, delete the
rest. Everything except `InputHandler()` has a weak default in `src/`, so a minimal board
is `_setup_gpio()` + `InputHandler()` and nothing else.

What goes where:

- **`-D` flags** — hard wiring that can never change at runtime: display, backlight,
  buttons, LEDs, battery ADC.
- **`bruceConfigPins.*` in `_setup_gpio()`** — everything the user may re-map from
  *Config > Set Device pins*: SD card, CC1101, NRF24, PN532/RC522/ST25R, W5500, LoRa, I2C,
  UART, GPS, IR, one-pin RF, speaker, microphone, buzzer. What you assign here is only the
  **default**; from the second boot on, `/brucePins.conf` wins.

Boot order in `src/main.cpp`:

| Hook | Runs | Typical content |
|---|---|---|
| `_setup_gpio()` | first, before display and filesystem | `bruceConfigPins.*`, power-enable pins, parking SPI chip-selects HIGH, input `hal_*_init()`, PMIC/gauge |
| `_pre_storage_gpio()` | after the first TFT use, before LittleFS/SD mount | rare storage/bus fix-ups (`m5stack-cores3`) |
| `_post_setup_gpio()` | after display + storage | backlight, and touch controllers that share the display bus |
| `_late_setup_gpio()` | after the input task exists | rare (`m5stack-tab5`) |

`InputHandler()` runs in its own task every ~10ms. The task clears all the input globals,
holds the input lock and calls `checkPowerSaveTime()` around it — so `InputHandler()` must
only *read* the hardware and set the globals, and must never block.

### Step 6 — build and register the env

```sh
pio run -e <env>
```

Once it builds and works on hardware, add the env name to the (commented) `default_envs`
catalogue in `platformio.ini`, to the matrix in `.github/workflows/PR_All_envs.yml`, and to
the `options` list in `.github/workflows/manual_build_sel_env.yml`.

Optionally add `boards/<board>/connections.md` with the pinout and wiring notes.

---

## 3. The shared HAL — what you do *not* have to write

`src/hal/` (see `src/hal/README.md` for the full API) covers buttons, rotary encoders,
touch controllers, the BQ25896 charger, the BQ27220 fuel gauge and PWM backlight. A board
fills a small plain-data struct from `src/hal/device.h` and calls the matching
`hal_*_init()` / `hal_*_poll()` — no driver code.

Every `hal_*` function exists for every board: when the selecting macro isn't defined it
compiles to a no-op returning `false`/`-1`, so board code needs no `#ifdef` around it.

Deliberately **outside** the HAL:

- **Keyboards** — too different per device; scan them in `interface.cpp`
  (`m5stack-cardputer`, `lilygo-t-deck`, `lilygo-t-lora-pager`).
- **Other PMICs/gauges** (AXP2101, AXP192, SY6970, MAX17048, …) — drive them directly and
  override `getBattery()`/`isCharging()`. Don't force them through `DevicePmic`.
- **Boards on a higher-level stack** (M5Unified's `M5.BtnA`/`M5.Touch`/`M5.Power`) — wire
  them up directly instead.
- **Non-PWM brightness** (`M5.Display.setBrightness()`, AXP192 `ScreenBreath()`, panel
  commands, e-paper) — write `_setBrightness()` by hand.

---

## 4. Reference boards

When in doubt, copy from a board that already does the same thing:

| You need | Look at |
|---|---|
| 6 buttons, plain GPIO | `boards/lilka` |
| 2 buttons (IDF component) | `boards/lilygo-t-display-s3` |
| Rotary encoder + PMIC + gauge | `boards/lilygo-t-embed-cc1101` |
| Resistive touch (XPT2046) on the display SPI | `boards/phantom` |
| Capacitive touch (GT911) | `boards/lilygo-t-deck` |
| Capacitive touch (CST8xx) | `boards/lilygo-t-display-s3` (`-touch` env) |
| Physical keyboard | `boards/m5stack-cardputer`, `boards/lilygo-t-lora-pager` |
| Several envs from one folder | `boards/lilygo-t-display-s3`, `boards/lilygo-t-embed-cc1101` |
| A fully worked, heavily commented `.ini` | `boards/ESP-General/ESP-General.ini` |

---

## 5. Checklist

- [ ] Folder copied from `_New-Device-Model`, `.ini` renamed.
- [ ] `build_src_filter`, `-Iboards/<board>` and `[env:<env>]` all point at the new folder.
- [ ] `board = <mcu>` matches a file in `boards/_jsonfiles/`.
- [ ] Partition table, flash size and `memory_type`/`BOARD_HAS_PSRAM` match the hardware.
- [ ] Exactly one display driver flag, with its `lib_deps` entry.
- [ ] At most one `HAS_x_BUTTON(S)` flag; every input flag present has its matching
      `hal_*_init()` + `hal_*_poll()` pair in `interface.cpp`.
- [ ] `DEVICE_NAME`, `ROTATION`, `BTN_ALIAS` set.
- [ ] No `-D` overriding a plain Arduino pin name (`SDA`, `SCL`, `SS`, `MOSI`, …).
- [ ] `bruceConfigPins.*` filled for every module the board actually carries.
- [ ] Unused template functions and comments deleted rather than left as empty overrides or reminders.
- [ ] `pio run -e <env>` is clean.
- [ ] Env added to `platformio.ini` and to both GitHub workflows.
