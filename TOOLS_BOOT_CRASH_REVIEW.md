# Tools Menu Boot-Crash Review

**Date:** 2026-08-18
**Target:** LilyGO T-Embed CC1101 (`lilygo-t-embed-cc1101`)
**Scope:** Investigation of the custom `Tools` menu, the serial crash logs, and the Bruce/PlatformIO build path. This document now includes the first controlled baseline and staged hardware tests.

## Executive conclusion

The serial output does **not** establish that the `Tools` callbacks execute during boot. The callbacks are only reached after the main menu is displayed and the user selects an item. The current workspace also contains `ToolsMenu.cpp` and `ToolsMenu.h` as untracked files, but the current `MainMenu` source does not include, instantiate, or register `ToolsMenu` at all. This means the source tree and the previously flashed custom image are not proven to be the same state.

The first controlled test changes the diagnosis significantly. The current source was clean-built with the workspace Python 3.11 environment, merged using the Bruce `build-firmware` target, flashed at `0x00000`, and booted successfully. A staged version with `Tools` registered and both submenu actions reduced to display-only placeholders also booted successfully and responded to menu selection.

The strongest confirmed issue from the earlier failure is therefore an image/source-state mismatch, a different untested code revision, or another pre-existing change—not the basic `Tools` menu registration or two display-only entries.

- The downloaded Bruce image boots on the same board.
- The custom merged image is structurally valid when inspected as a full image.
- The custom and downloaded images use the same bootloader and partition-table regions, but have different application images.
- The current baseline and staged Tools images both booted on `/dev/ttyACM0`.
- The old serial log contains `load:0x00000014,len:0x254`, which is not a valid normal ESP32-S3 application segment destination. That line is consistent with the bootloader parsing an invalid or wrong image region, or with an image that does not match the flash layout being used. It is not a normal signature of entering `ToolsMenu`.

There are still code problems to fix before adding the feature back, especially missing main-menu registration, incomplete theme integration, and unsafe callback design. Those problems should be handled independently from the boot-image investigation.

## 0. Controlled test results

### Baseline build

The current pre-Tools source was cleaned and built for `lilygo-t-embed-cc1101`. The first attempt exposed a host-tool issue: the system `pio` used Python 3.14, while this PlatformIO release requires Python 3.10–3.13. The build was then completed using the workspace Python 3.11 environment at `.venv/bin/pio`.

The merged baseline image was flashed with esptool 5.3.1 at `0x00000`. The board booted and reached the normal Bruce menu. Serial output included:

```text
[PSRAM] init=1 found=1 total=8388608 ...
[RAMLOG] ... stage=setup-end
[RAMLOG] ... stage=first-mainMenu
```

This confirms the clean build/merge/flash path works for the current source state.

### Staged Tools build

The following controlled changes were made:

- included `ToolsMenu.h` in `src/core/main_menu.h`;
- added `ToolsMenu toolsMenu;` to `MainMenu`;
- added `&toolsMenu` to the main-menu item vector;
- kept both submenu entries as display-only callbacks;
- removed the Wi-Fi include and `wifiConnectMenu()` call from the staged test.

The staged image was clean-built, merged, flashed at `0x00000`, and captured for 25 seconds. It reached the main menu with no panic or watchdog reset and produced:

```text
[RAMLOG] ... stage=setup-end
[RAMLOG] ... stage=first-mainMenu
Selected: Tools
Selected: Test Wi-Fi
INFO: Test Wi-Fi
Selected: Tools
Selected: Main Menu
```

This is direct hardware evidence that the `Tools` object, main-menu registration, submenu vector, and two display-only callbacks are not sufficient to reproduce the boot crash.

The staged boot did show existing warnings unrelated to Tools:

```text
__digitalWrite(): IO 21 is not set as GPIO
Preferences.cpp:47 begin(): nvs_open failed: NOT_FOUND
theme.cpp:27 openThemeFile(): Theme file not found. Using default theme
```

These did not prevent boot. They should be tracked separately, but they are not evidence that Tools caused the original failure.

### Build-tool observation

The generic `pio` command currently points to a Python 3.14 installation and fails the project’s supported-version check. The workspace `.venv/bin/pio` succeeds. This is a reproducibility issue, not a firmware crash cause for the staged image, because the successful staged image was built with the supported interpreter.

## 1. Evidence from the serial logs

The failing log repeatedly reports:

```text
load:0x00000014,len:0x254
Guru Meditation Error: Core 0 panic'ed (StoreProhibited)
EXCCAUSE: 0x0000001d
EXCVADDR: 0x00000014
rst:0x7 (TG0WDT_SYS_RST,boot:0x8 (SPI_FAST_FLASH_BOOT))
```

Interpretation:

1. `StoreProhibited` means a write was attempted to an invalid/protected address.
2. `EXCVADDR: 0x00000014` is near-null and commonly points to a null/invalid object pointer plus a field offset.
3. However, the preceding `load:0x00000014,len:0x254` is much more important. Normal ESP32-S3 application segments use addresses in regions such as `0x3c...`, `0x3f...`, `0x403...`, `0x420...`, `0x500...`, or `0x600...`; `0x14` is not a normal application load address.
4. Therefore the log cannot be treated as proof that `helloWorldPlaceholder()` or `testWifiPlaceholder()` ran. It suggests image parsing/loading or an earlier startup path unless a separate decoded backtrace proves otherwise.
5. The `Saved PC:0x40050f3f` and watchdog reset indicate the board repeatedly restarts after the fault. They do not identify a C++ source line by themselves.

The later successful log from the downloaded binary prints normal Bruce configuration JSON and remains connected. That is strong evidence that the board, USB serial connection, flash chip, and basic flashing procedure are functional.

## 2. Current `Tools` implementation: confirmed problems

### 2.1 The current source does not register `Tools`

`src/core/menu_items/ToolsMenu.cpp` and `src/core/menu_items/ToolsMenu.h` exist, but:

- `src/core/main_menu.h` does not include `menu_items/ToolsMenu.h`.
- `MainMenu` has no `ToolsMenu toolsMenu;` member.
- `src/core/main_menu.cpp` does not add `&toolsMenu` to `_menuItems`.

This is a direct source-state discrepancy. PlatformIO normally compiles all matching `.cpp` files, so the new translation unit can be compiled and linked even when the menu item is not reachable. But the current source cannot display `Tools` through the main menu.

**Impact:** high for feature correctness; not a convincing explanation for a boot crash by itself.

**Required review question:** was the failing binary produced from another unsaved or earlier version that did register `Tools`? The current `git status` shows the new `ToolsMenu` files as untracked, while the main-menu and theme files show no tracked diff. This must be resolved before attributing the crash to the feature.

### 2.2 `ToolsMenu` has no theme integration despite the JSON entry

The custom theme JSON contains:

```json
"tools": "tools.gif"
```

But the current theme model/parser has no corresponding `tools` field:

- `src/core/theme.h` has no `themeFiles::tools` path.
- `src/core/theme.h` has no `themeInfo::tools` flag.
- `src/core/theme.cpp` has no `ThemeEntry` for `tools`.
- `ToolsMenu::hasTheme()` always returns `false`.
- `ToolsMenu::themePath()` returns a function-local empty `String`.

**Impact:** the `tools` JSON property is ignored and the Tools icon always uses `drawIcon()`. This is a broken/incomplete feature, but ignored JSON is not expected to crash boot.

### 2.3 The Wi-Fi callback is not a true placeholder

`testWifiPlaceholder()` does more than display text:

```cpp
if (!WiFi.isConnected()) {
    bool ok = wifiConnectMenu(WIFI_STA);
    ...
}
```

This callback is only called after user selection, so it should not cause a boot-time crash. Nevertheless, it is too large and risky for the first test entry because it enters the complete Wi-Fi connection UI and network startup path. It should initially be replaced by a display-only callback. Wi-Fi can be added later as a separate tested change.

**Impact:** medium for runtime stability after selection; low for the reported pre-menu crash.

### 2.4 Callback storage should follow established patterns carefully

`options` is a global `std::vector<Option>`, and each `Option` stores a `std::function<void()>`. The current code uses plain file-scope functions, which is generally safe:

```cpp
{"Hello World", helloWorldPlaceholder}
```

The `options` vector is cleared after `loopOptions()`, so there is no obvious dangling capture in these two entries. Existing Bruce menus use the same pattern. This is not currently an identified boot bug.

The safer first implementation should still use the project’s established `options = {...}; addOptionToMainMenu(); loopOptions(...); options.clear();` pattern consistently.

### 2.5 The custom icon is not a likely startup fault

`drawIcon()` only runs when the main menu renderer draws the Tools item. It performs TFT drawing and uses initialized global display/config state. It cannot run during global construction because it is only invoked through `MenuItemInterface::draw()` from the menu loop.

Potential UI concerns remain:

- dimensions are partly hard-coded (`12`, `14`, `20`, `28`, `8`), so the icon may not scale consistently;
- `int wrenchSize = scale * 55` can truncate at small scales;
- the icon is not relevant to startup unless `Tools` is actually registered and the menu renderer reaches it.

## 3. Global construction and startup ordering

`src/main.cpp` constructs several global objects before `setup()`, including `bruceConfig`, `startupApp`, and `mainMenu`. `MainMenu` itself builds a vector of pointers to menu objects in its constructor.

A `ToolsMenu` object with this constructor is trivial:

```cpp
ToolsMenu() : MenuItemInterface("Tools") {}
```

It does not access hardware, Wi-Fi, the filesystem, TFT, or theme state during construction. Therefore adding one such object should not by itself cause a pre-`setup()` crash.

The dangerous pattern would be adding any of the following to a global constructor or global initializer:

- `WiFi`, TFT, SD, I2C, or PMU operations;
- calls to `displayInfo()` or `wifiConnectMenu()`;
- dynamic global vectors/strings with unexpected initialization order dependencies;
- a global object whose destructor or constructor accesses another translation unit’s global object.

The current `ToolsMenu` files do not show those operations at global scope.

## 4. Build and artifact review

### 4.1 Official Bruce instructions

The official build page says to:

- open the repository with PlatformIO/PIOArduino;
- build the exact environment, for example `pio run -e lilygo-t-embed-cc1101`;
- run the custom `build-firmware` target to merge bootloader, partition table, and application;
- flash the merged binary using the manual flashing procedure.

The Docker example is:

```text
pio run -e lilygo-t-embed-cc1101 -t build-firmware
```

The project README similarly says to flash a merged `Bruce-<device>.bin` at `0x00000`.

### 4.2 Current target configuration

`boards/lilygo-t-embed-cc1101/lilygo-t-embed-cc1101.ini` selects:

- board `lilygo-t-embed-cc1101`;
- `qio_opi` Arduino memory type;
- `custom_16Mb.csv`;
- board-specific source directory;
- `-O2` after the global `-Os`;
- board-specific macros including `T_EMBED_1101` from the board definition.

`custom_16Mb.csv` places the factory application at `0x10000`, with no OTA partition:

```text
factory, app, factory, 0x10000, 0x470000
```

### 4.3 The current merge script is mostly structurally correct

`build.py` selects these offsets for ESP32-S3:

- bootloader: `0x0000`;
- partition table: `0x8000`;
- app: `0x10000`.

The current custom merged artifact has valid full-image headers at all three expected regions. The app extracted from `0x10000` is also a valid ESP32-S3 image with valid checksum and hash.

Important caveat: `build.py` checks the first `ota_0` partition for size validation, but this target uses a `factory` partition. This weakens the size check, although it does not change the hard-coded merge offsets.

### 4.4 The public-download helper publishes two different artifact types

`.vscode/build_public_download.py` copies both:

- raw `.pio/build/<env>/firmware.bin` (application-only);
- `Bruce-<env>.bin` (merged image).

This creates a serious operator-error risk. They must not be flashed the same way:

- merged image: flash at `0x00000`;
- app-only image: flash at its application offset (`0x10000`) with compatible bootloader/partition data.

Flashing the wrong artifact at the wrong offset can produce invalid load-address logs like `load:0x00000014`.

### 4.5 Artifact comparison evidence

The custom project artifact and downloaded stock artifact are different files, as expected:

- custom merged size: `4,266,800` bytes;
- downloaded merged size: `4,264,656` bytes;
- custom app size: `4,201,264` bytes;
- downloaded app size: `4,199,120` bytes.

Both extracted app images have:

- the same entry point (`0x403774f4`);
- seven valid ESP32-S3 segments;
- valid checksums and validation hashes.

The bootloader regions compare equal, and the partition-table regions compare equal. The difference is in the application image. This rules out a simple custom bootloader or partition-table corruption in the inspected artifacts, but it does **not** prove which exact custom source was used to produce the flashed app.

## 5. Plausible causes, ranked

### A. Stale/alternate source or wrong artifact (highest confidence)

The current source does not register Tools, while the conversation history says Tools was added to `MainMenu` and theme mappings were changed. New Tools files are untracked. This strongly suggests the failing binary may not correspond to the current workspace state, or the intended edits were lost/overwritten.

### B. App-level startup crash unrelated to Tools (plausible)

The board-specific startup path includes PMU/I2C initialization, battery-gauge calls, SPI/CC1101 initialization, TFT setup, storage/config loading, theme loading, task creation, and Wi-Fi setup. A bad pointer or library ABI/configuration mismatch in that code can generate `StoreProhibited` before the first menu.

A custom app being different from stock does not prove the new menu caused the difference. The custom workspace includes many pre-existing modifications and a newer PIOArduino/Arduino/IDF toolchain configuration.

### C. Build/toolchain divergence (plausible)

The custom build uses a date-specific PIOArduino platform/framework package and LTO. The downloaded binary reports an ESP-IDF build timestamp from July 15, 2026, but the app contents differ. A toolchain mismatch can expose existing undefined behavior, although the current evidence does not identify a compiler error.

### D. `ToolsMenu` object/linkage issue (low confidence from current code)

A correctly implemented trivial menu object should be safe. A bad version could cause trouble if it introduced global hardware work, an invalid vtable/object lifetime, or an ODR violation. The current files do not show such a defect.

### E. Theme JSON/GIF (very low confidence for pre-menu crash)

The current parser ignores `tools`, and `ToolsMenu` does not call the theme path. The theme asset cannot explain a crash before the menu unless a different, uninspected theme implementation was flashed.

## 6. Recommended no-flash investigation sequence

1. **Freeze and identify source state.** Save/commit the current source state or create a patch archive. Confirm whether the failing binary was built from this exact tree.
2. **Make a source-only baseline build.** Build the current tree for `lilygo-t-embed-cc1101` after a clean build. Record compiler/tool versions, image hash, map, and memory usage.
3. **Remove the new Tools translation unit from the build without changing anything else.** Compare the app image, map, and memory report. This is a build comparison only.
4. **Create a minimal Tools implementation.** Register only the menu class and a display-only `Hello World` callback; do not include Wi-Fi or theme code.
5. **Add the second display-only placeholder.** Confirm that adding an `Option` does not materially alter startup/global state.
6. **Add main-menu registration.** Include the header, add the member, and add the pointer in `_menuItems`. Verify the generated map contains `ToolsMenu` and the intended registration code.
7. **Add Wi-Fi behavior separately.** Only after the display-only build is proven, add `wifiConnectMenu(WIFI_STA)` and test that path when selected.
8. **Add theme support separately.** Add `tools` to `themeFiles`, `themeInfo`, the parser table, and `ToolsMenu::themePath()`. Validate the file only when the menu is opened.
9. **Use `build-firmware`, not a hand-selected raw app file.** Keep raw and merged artifacts in separate named directories to prevent accidental flashing of the wrong file.
10. **Decode a future crash against the exact matching ELF.** The raw register dump is insufficient. Use the matching `.elf`/map and exception decoder to map `PC` and backtrace addresses to source lines.

## 7. Questions to resolve before modifying code

- Which exact binary produced the failing log: the current `Bruce-lilygo-t-embed-cc1101.bin`, a prior merged file, or `.pio/build/.../firmware.bin`?
- Was the binary flashed at `0x00000` as a merged image, or at `0x10000` as an app image?
- Was the failing binary built after a clean `pio run -t clean`?
- Does the failing artifact contain `ToolsMenu` symbols and a registered `Tools` string in its map/strings output?
- Are the current untracked Tools files the same files that were used for the failing build?
- Are the PMU/encoder temporary `interface.cpp` changes still part of the failing build?

## Bottom line

Do not conclude that two display-only placeholder entries can crash Bruce during startup. The controlled staged image registered `Tools`, entered its submenu, invoked the second placeholder, returned to the main menu, and remained stable. The earlier failing binary therefore came from a different state or included another failing change.

The next isolation step is to add the Wi-Fi callback back without changing registration or theme code, build and test it separately. Theme support should remain disabled until that test passes. The exact successful staged image hash and the exact failed image hash should be recorded together whenever another test is performed.

## 8. Theme diagnosis: `BruceHoodIronySleepyTielTheme`

The current theme JSON is syntactically valid. The asset directory also contains the expected files, including `tools.gif`, and the GIF files have valid `GIF89a` signatures. The official Bruce theme documentation confirms that `.gif` assets are supported and that the theme is selected by choosing a JSON file from LittleFS or the SD card.

The displayed `5` is not a generic theme-name error. It is explicitly emitted by `BruceTheme::openThemeFile()` when ArduinoJson fails to deserialize the selected JSON:

```cpp
if (deserializeJson(jsonDoc, file)) {
    displayError("5", true);
    ...
}
```

The likely failure is therefore that the device is not reading the same 676-byte JSON file that was validated on the PC, or the file is being read through a filesystem/path issue. The current JSON itself is valid according to `jq`.

### Important path/storage behavior

`setTheme()` selects a filesystem, then calls `loopSD(*fs, true, "JSON")`. The returned path is passed directly to `openThemeFile()`. `openThemeFile()` then builds asset paths from the JSON directory:

```cpp
String baseThemePath = filepath.substring(0, filepath.lastIndexOf('/')) + "/";
```

The theme folder must therefore be copied as a complete directory to the selected filesystem, and the JSON must be selected from that filesystem. If the JSON is selected from SD while assets are on LittleFS, or if a file is copied through a host tool with truncation/encoding changes, loading will fail or assets will be missing.

### JSON compatibility issues found

The current JSON contains a `tools` key, but the current firmware theme parser does not define a `tools` field or parser entry. That key is ignored. Consequently:

- the JSON should still deserialize successfully;
- the theme can still apply to all existing menu keys;
- the Tools icon will continue using its C++ icon until theme support is added to `theme.h`, `theme.cpp`, and `ToolsMenu.h`.

The JSON also contains `name`, `author`, and `url`, which the parser ignores safely. `ledBright` is written as a JSON string (`"50"`) rather than a number; ArduinoJson’s `.as<int>()` generally accepts this, but using a numeric value is clearer and avoids conversion ambiguity. The official format lists LED values as numbers/hex strings depending on the field.

### Most likely causes of the `5`

Ranked from most to least likely:

1. The JSON on the device is malformed, truncated, or not the same file as the workspace copy.
2. The selected file is not actually `Bruce_Theme.json` from the complete theme folder.
3. The theme was copied to one filesystem but selected from the other.
4. The SD/LittleFS file read is returning an incomplete stream or a stale file.
5. The custom JSON contains an unsupported key such as `tools`; this is unlikely to cause error `5`, because unknown ArduinoJson object keys are normally ignored during deserialization.

### Safe corrective action

Before changing firmware code, simplify the theme JSON to the documented keys and remove metadata/unsupported fields temporarily. Keep `tools` out until the parser is extended. Use numeric LED values where appropriate. Then copy the complete folder, including `Bruce_Theme.json` and all GIFs, to one filesystem and select the JSON from that same filesystem.

If the simplified file still displays `5`, the next diagnostic should print the actual ArduinoJson deserialization error and the selected filepath from `openThemeFile()`. The current code discards the `DeserializationError`, so the display only shows an unhelpful numeric marker.
