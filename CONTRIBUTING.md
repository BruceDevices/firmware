# Contributing to Bruce

Thank you for your interest in contributing to **Bruce**! This guide covers setting up your development environment, understanding the firmware architecture, adding new features or boards, and submitting pull requests.

- Porting a new device? Read **[boards/README.md](boards/README.md)** and **[src/hal/README.md](src/hal/README.md)**.
- Exploring the directory tree? Read **[LAYOUT.md](LAYOUT.md)**, **[include/README](include/README)**, and **[lib/README](lib/README)**.

---

## 1. Prerequisites

1. **Git** (with submodule support):
   ```sh
   git clone --recursive https://github.com/BruceDevices/firmware.git
   cd firmware
   ```
   If you already cloned without `--recursive`, initialize submodules manually (or let `check_submodules.py` fetch them during the first build):
   ```sh
   git submodule update --init --recursive
   ```
2. **Python 3.9+** (`requests` package is used by `patch.py` when minifying changed WebUI assets; set `BRUCE_OFFLINE_WEB_ASSETS=1` for offline builds).
3. **PlatformIO Core (CLI)** or the **PlatformIO IDE** extension for VS Code.
4. **Host C Compiler (`gcc` / `clang`)**: Required when modifying JavaScript interpreter standard library bindings (`gen_mqjs_headers.py` compiles a small host generator).

---

## 2. Building the Firmware

### Build a Single Environment Locally

Each supported board defines one or more environments in `boards/<board>/<board>.ini`. To build a specific target (for example, `m5stack-cardputer`):

```sh
pio run -e m5stack-cardputer
```

To build and flash over USB:

```sh
pio run -e m5stack-cardputer -t upload
```

You can also use the interactive build helper:

```sh
python3 build.py -e m5stack-cardputer
```

### Batch / Reproducible Builds with Docker

To build inside the standardized CI container using Docker Compose:

```sh
# Build a specific environment (or comma/space-separated list)
PIO_ENVS="m5stack-cardputer" docker compose run --rm platformio_build

# Build all environments defined across boards/*/*.ini
PIO_ENVS="" docker compose run --rm platformio_build
```

### Pre-Build Scripts Overview

When `pio run` starts, the `[env]` section in `platformio.ini` runs several `extra_scripts`:
- `pre:check_submodules.py` — Verifies that git submodules in `lib/` (`DisplayDrivers`, `SensorLib`, `XPowersLib`, `ESP32_Button`, `RotaryEncoder`) are populated.
- `pre:pre_build_current_year.py` — Generates `include/current_year.h` with `#define CURRENT_YEAR <YYYY>`.
- `pre:patch.py` — Weakens `ieee80211_raw_frame_sanity_check` in the ESP32 `libnet80211.a` archive (enabling raw 802.11 frame injection) and compiles `embedded_resources/web_interface/` into `include/webFiles.h` whenever WebUI sources change.
- `pre:patch_library_conflicts.py` — Applies targeted hotfixes to downloaded libraries in `.pio/libdeps/` (resolving symbol clashes and ST25R3916 I2C polling behavior).
- `pre:gen_mqjs_headers.py` — Rebuilds precompiled MicroQuickJS bytecode headers in `lib/mquickjs_headers/` when `src/modules/bjs_interpreter/mqjs_stdlib.c` or JS binding files are modified.
- `pre:flto_prep.py` — Strips `-fno-lto` from linker flags on environments that enable Link-Time Optimization (`-flto`).

---

## 3. Architecture Overview

### Main Loop & Menu Dispatch (`src/main.cpp` & `MenuItemInterface`)

1. At boot, `setup()` initializes GPIOs (`setup_gpio()`), mounts LittleFS and SD storage (`begin_storage()`), loads JSON configuration files, initializes the display (`begin_tft()`), spawns the FreeRTOS input task (`taskInputHandler`), and starts the background Serial CLI task.
2. `loop()` calls `mainMenu.begin()` (in `src/core/main_menu.cpp`), which iterates over registered `MenuItemInterface` instances (`wifiMenu`, `bleMenu`, `rfMenu`, `rfidMenu`, etc.) and populates the global `options` vector (`std::vector<Option>`).
3. `loopOptions(options, ...)` (in `src/core/display.cpp`) renders either the carousel or grid main menu and blocks until the user selects an option, which invokes the selected item's `optionsMenu()` method.

### Input Handling (`InputHandler`, `inputLock()`, and `check()`)

1. Input polling runs asynchronously on a dedicated FreeRTOS task (`taskInputHandler` in `src/main.cpp`) every 10 ms.
2. Before resetting and polling hardware inputs, `taskInputHandler` acquires `inputLock()`, clears the volatile navigation flags (`NextPress`, `PrevPress`, `UpPress`, `DownPress`, `SelPress`, `EscPress`, `AnyKeyPress`, `touchPoint`), calls the board's `InputHandler()`, and releases `inputUnlock()`.
3. **In feature and UI code, never read or clear navigation booleans directly.** Always consume them with `check(SelPress)`, `check(EscPress)`, `check(NextPress)`, etc. (defined in `src/core/mykeyboard.cpp`). `check()` safely acquires `inputLock()`, reads and clears the flag, updates activity timers, and plays UI feedback.

### Configuration Persistence (`bruceConfig` & `bruceConfigPins`)

Bruce separates user preferences from hardware wiring into two global objects (declared in `include/globals.h`):
- **`bruceConfig`** (`src/core/config.h` / `config.cpp`, persisted to `/bruce.conf` on LittleFS and mirrored to SD): Stores UI colors, brightness, dimmer timeout, Wi-Fi/WebUI credentials, MIFARE keys, sound settings, and theme paths.
- **`bruceConfigPins`** (`src/core/configPins.h` / `configPins.cpp`, persisted to `/brucePins.conf`): Stores runtime-customizable peripheral pin mappings (CC1101 SPI bus, NRF24 SPI bus, RFID/NFC module type, IR TX/RX pins, RF TX/RX pins, UART/I2C/SPI pins, microphone type, and screen rotation).

---

## 4. Adding a New Feature Module

Follow these steps to add a new feature under `src/modules/` and expose it in the UI and Serial CLI:

### Step 1: Create the Module Under `src/modules/<feature>/`
Create your header and implementation files (for example, `src/modules/myfeature/myfeature.h` and `myfeature.cpp`).
- Use `loopOptions(options, MENU_TYPE_SUBMENU, "Title")` to present interactive submenus.
- In long-running loops, poll `if (check(EscPress) || returnToMenu) break;` and call `vTaskDelay(pdMS_TO_TICKS(10))` so the watchdog and background tasks stay responsive.
- If the module is too large for 4 MB flash boards without PSRAM, guard its inclusion and menu registration with `#if !defined(LITE_VERSION)`.

### Step 2: Register the Menu Item
If adding a new option to an existing menu (e.g., `OthersMenu` or `WifiMenu`), add an `Option` entry inside `src/core/menu_items/<Menu>.cpp`.

If adding a new top-level main menu category:
1. Create `src/core/menu_items/MyFeatureMenu.h` and `MyFeatureMenu.cpp` inheriting from `MenuItemInterface` (`include/MenuItemInterface.h`).
2. Implement `optionsMenu()`, `drawIcon(float scale)`, `hasTheme()`, and `themePath()`.
3. Instantiate `MyFeatureMenu` inside `MainMenu` (`src/core/main_menu.h`) and append its pointer to `_menuItems` in `MainMenu::MainMenu()` (`src/core/main_menu.cpp`).

### Step 3: Register a Serial CLI Command (Optional)
1. Create `src/core/serial_commands/myfeature_commands.h` and `myfeature_commands.cpp` defining `void createMyFeatureCommands(SimpleCLI *cli)`.
2. Register your command callback with `cli->addCommand("myfeature", myFeatureCallback)` (see `src/core/serial_commands/wire_commands.cpp` for a complete example).
3. Include your header in `src/core/serial_commands/cli.cpp` and call `createMyFeatureCommands(&_cli)` inside `SerialCli::setup()`.

---

## 5. Porting a New Board

Do not copy random existing board folders when porting new hardware. Always start from `boards/_New-Device-Model/` and follow the step-by-step instructions in:
- **[boards/README.md](boards/README.md)** — Board `.ini` configuration, MCU JSON selection (`boards/_jsonfiles/`), display setup, and lifecycle hooks.
- **[src/hal/README.md](src/hal/README.md)** — Shared HAL drivers for buttons, rotary encoders, touch controllers, PMICs, fuel gauges, and backlight PWM.

---

## 6. Code Style & Pull Request Checklist

### Code Formatting (`.clang-format`)
Bruce enforces consistent C/C++ formatting via `.clang-format` at the repository root:
- **Base style:** LLVM
- **Indentation:** 4 spaces (no tabs)
- **Column limit:** 110 characters
- **Braces:** Attached (`BreakBeforeBraces: Attach`)

Format all modified `.cpp` and `.h` files with `clang-format -i <file>` before committing.

### Pull Request Checklist
- [ ] Code is formatted according to `.clang-format` (4-space indent, 110-column limit).
- [ ] Builds cleanly on at least one full-featured ESP32-S3 target (e.g., `pio run -e m5stack-cardputer`) and, if touching core/shared code, one `LITE_VERSION` / 4 MB target (e.g., `pio run -e m5stack-cplus1_1`).
- [ ] Memory safety: avoids unbounded stack allocations (respect `SAFE_STACK_BUFFER_SIZE`), checks heap/PSRAM allocations, and frees buffers on all exit paths.
- [ ] Hardware-specific code or heavy libraries are guarded with appropriate build flags (`#if !defined(LITE_VERSION)`, `#ifdef HAS_...`) so unrelated boards do not pull in unnecessary dependencies.
- [ ] Any new third-party code or library is compatible with `AGPL-3.0-or-later` and documented in [`THIRD_PARTY.md`](THIRD_PARTY.md).
