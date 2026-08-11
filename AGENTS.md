# Project Handoff & Developer Notes

Bruce (Universal IR / firmware) fork. Working repo: `C:\Users\Utente\Documents\Default Project`.
Base is Bruce firmware (ESP32), stable at tag `1.16`. **Release version is now `3.5`** (user request 2026-08-08): on-device `BRUCE_VERSION` = `"3.5"`, deliverable bins `BruceIRF3.5-<board>.bin`.

## Build commands (Windows / PowerShell)

```powershell
# Normal build (redirect output or the tool kills the process on large output)
pio run -e <env> *> build_log.txt 2>&1

# Freeze fix: limit parallel jobs — full builds OOM/kill on this machine (seen on APRS.cpp)
pio run -e <env> -j 2 *> build_log.txt 2>&1
```

- PlatformIO restores `firmware.elf`/`firmware.bin` from cache → `_merge_bins_callback` does NOT run on cached builds.
- Use `pio run -e <env> -t build-firmware` when the merge post-action must run.
- Verified working envs: `lilygo-t-embed-cc1101`, `m5stack-sticks3`, `m5stack-cardputer`, `esp32-c5-tft`.
- **Reliable build on this machine (the tool/agent kills long foreground pio):** launch detached via a batch wrapper and poll the log, e.g. `build_wrapper.bat` in project root runs `call pio run -e m5stack-sticks3 -e lilygo-t-embed-cc1101 -j 2 > build_log.txt 2>&1` + `echo EXITCODE=%ERRORLEVEL%`, launched with `Start-Process cmd.exe /c build_wrapper.bat -WindowStyle Hidden -PassThru`. NEVER start two pio builds concurrently (shared build dirs → ODR/link corruption). Prefer editing source and doing a **full clean rebuild** of an env over incremental when artifacts may be stale.
- **SCons uses MD5 content signatures** — `touch`ing a file (mtime change, same content) will NOT trigger a rebuild. A force-rebuild of 2-3 files requires deleting the build dir (or the `.o` files) first, or full clean.

## Versioning rules (very important)

- `platformio.ini` (`env` `build_flags`): `-DBRUCE_VERSION='"3.5"'` → on-device displayed version. **It had regressed to `"dev"` via cherry-pick `92d24dcd` (upstream value) — set back to `"3.5"` for release (2026-08-08).**
- `build.py`: `FIRMWARE_VERSION = "3.5"` → output bin filename (`BruceIRF3.5-<board>.bin`). For a **test build** set `"3.5-test"` (and `platformio.ini` `BRUCE_VERSION` to `"3.5-test"`) → outputs `BruceIRF3.5-test-<board>.bin` (boot shows `3.5-test`). Remember to revert both to `3.5` for release.
- The "dev" bug root cause was `platformio.ini` hardcoding `-DBRUCE_VERSION='"dev"'`; a later upstream cherry-pick re-introduced it — always verify the line reads the release version before building.
- Verify after build: binary should contain `"3.5"` (or `"3.5-test"` for test builds). `"2.5"` / `"dev"` hits are unrelated library strings — don't chase them.
- Built deliverable bins live in project root; merged via `esptool.py --chip esp32s3 merge-bin` (bootloader @0x0, partitions @0x8000, firmware @0x10000).

## Deliverables (project root)

- `BruceIRF3.5-lilygo-t-embed-cc1101.bin` — **current T-Embed deliverable** (RCA fix + crash fix + spam pause + shared-SPI `_spi_started` fix (deadlock-free) + coredump CRASH DIAG + dual-detector speed fix + external RF/IR module selectors + SD-gated Save + replay loops back to menu + **UniversalRF browser dangling-reference fix** + **UniversalRF Generic/Brands flow + readable labels** + **wrapper-folder DB discovery** + **grid key UX (legend, page-flow Up/Down, page keys)** + **auto-save immediate-save rewrite + noise rejection + IR RAW fallback**). Rebuilt 2026-08-11 (release 3.5, user-tested on T-Embed).
- `BruceIRF3.5-m5stack-sticks3.bin` — **current sticks3 deliverable** (same fixes, effective `-O2`, now also with CRASH DIAG). Rebuilt 2026-08-11 (release 3.5).
- `BruceIRF3.5-m5stack-cardputer.bin` — **Cardputer deliverable (v1 + ADV)** (same fixes; one env `m5stack-cardputer` covers both variants — ADV keyboard controller detected at runtime: TCA8418 on ADV, legacy on v1; `-DARDUINO_LOOP_STACK_SIZE=16384` added for Universal menu nesting). Rebuilt 2026-08-11 (release 3.5).
- **ESP32-C5 + ILI9341:** 3.5 build was SKIPPED for the v3.5 release (user request 2026-08-08 — "c5 puoi saltarlo"). Only `BruceIRF3.0-esp32-c5-tft.bin` (2026-08-06) exists; rebuild on request. RISC-V port — CRASH DIAG auto-detects the RISC-V coredump layout; touch zones drive Prev/Next/Up/Down/SEL/ESC via `touchHeatMap`; `-DARDUINO_LOOP_STACK_SIZE=16384` added; first build auto-downloads `toolchain-riscv32-esp` (~500 MB) + `tool-riscv32-esp-elf-gdb`.
- Old 3.0 bins/zips were removed 2026-08-08 after the v3.5 release — only the release artifacts are kept. For a future test build, `platformio.ini` `BRUCE_VERSION` = `"3.5-test"` + `build.py` `FIRMWARE_VERSION` = `"3.5-test"` produce `BruceIRF3.5-test-<board>.bin` (boot shows `3.5-test`); remember to revert both for release.
- `build_wrapper.bat` / `build_release.bat` — detached background build wrappers (Start-Process pattern; see Build commands). `build_release.bat` builds all 4 envs with `-t build-firmware` sequentially.
- `BruceIR-Cyberpunk.json` — user theme (cyan/pink). priColor `07FF`, secColor `F81F`, bgColor `0010`, border 1, label 1, ledColor `FF00FF`, ledBright 50.
- IR DB content is bundled ONLY in the combined packages below (the standalone `BruceIR2.0-UniversalIR-*.zip` were removed 2026-08-05 as superseded; IR Lite ~0.16 MB / 7 files fits in the 3 MB LittleFS partition).
- RF DB content is bundled ONLY in the combined packages below (the standalone `BruceIR3.0-UniversalRF-*.zip` were removed 2026-08-05, same content merged into the combined zips).
- `BruceIR3.5-UniversalIR-RF-Lite.zip` (0.62 MB) — **combined IR+Lite + RF+Lite in one zip** (`UniversalIR/` 7 files + `UniversalRF/` 103 files). Extracts to both roots; fits in the 3 MB LittleFS partition (~1.6 MB total). Use for sticks3/Cardputer/C5 (LittleFS).
- `BruceIR3.5-UniversalIR-RF-Full.zip` (9.0 MiB) — **combined IR+Full + RF-Full** (`UniversalIR/` 829 files + `UniversalRF/` 2052 files = `Garages/` + `Gates/` + `Vehicles/` only). For SD users (T-Embed). **REBUILT 2026-08-11:** the ~11k-file junk categories (TPMS, bracelets, WeVibe, RobotDog, dog collars, etc.) were DROPPED from the package by user request ("gli unici folder davvero utili sono garage, gates e quelli di bruteforce") — the bruteforce families already live inside `Garages/`/`Gates/` (CAME/NICE/Chamberlain/Linear/OOK/deBruijn). Old 16 MiB zip backed up at `%TEMP%\opencode\old_full_16MiB.zip`.
- `sd_card_data/` — extracted DB contents.

## README (2026-08-08)

- The tutorial README (from commit `ae7ecee2`) was accidentally overwritten by cherry-pick `92d24dcd` (which re-introduced the stock Bruce README + a short "fork" blurb). User complaint: "metà spiega il fork e l'altra è normale di bruce ... nn è ciò che mi serve".
- **Restored:** `README.md` is now the tutorial again (Download table, What's new, Supported boards, 4 flashing tutorials, DB install, Usage, Building), updated for **v3.5 + all 4 boards + grid key UX**. The v3.5 GitHub release notes reuse this content.

## Universal IR module (src/modules/ir/universal_ir.cpp, universal_ir.h)

Freeze/fix work already applied:
- LittleFS fallback in `show_categories()` (SD first, else LittleFS). On M5StickS3 build, `UNIVERSAL_IR_LITTLEFS_ONLY=1` skips SD entirely and uses only LittleFS.
- 150-option cap in `generic_signal_list` to avoid heap pressure from huge brands (e.g. Samsung 60+ files).
- Orientation helpers: `apply_display_orientation`, `load_ir_orient`, `save_ir_orient`, `ORIENT_FILE`.
- Newest UI is entirely grid-based.
- Uses `sendIRCommand`, `checkIrTxPin`, `bruceConfigPins.irTx`, `gsetRotation`. All board-agnostic.
- Error messages when no files: "Put IR files in /UniversalIR (SD or LittleFS)" / "(LittleFS)" on sticks3.
- **Spam pause (2026-08-03):** `spam_index` has a SEL-hold pause/resume + ESC-abort (shows `PAUSED - SEL to resume` at bottom). `sendIRCommand` itself is single-burst with NO pause; `txIrFile` has the shared SelPress hold-pause.

## Universal RF module (src/modules/rf/universal_rf.cpp, universal_rf.h)

New module (2026-08-03) mirroring UniversalIR for RF `.sub` files. Menus: RF → **Univ. RF Remote**.

- UI is grid-based (same layout engine as UniversalIR): categories → devices → signal grid. Root `/UniversalRF` (SD first, else LittleFS; `UNIVERSAL_RF_LITTLEFS_ONLY=1` on sticks3 skips SD).
- Reads Flipper SubGHz `.sub` files (RAW + BinRAW Key format) via the standard `readSubFile()`/`txSubFile(data, true)` pipeline (`src/modules/rf/rf_send.cpp`). No `rawDataList`/`keyList` references (those globals are NOT declared in any header) — `send_sub_file()` gates on `data.frequency == 0 || data.protocol.length() == 0`.
- Navigation mirrors UniversalIR grid: Prev/Next wrap, Up/Down move by column, SEL sends (debounced 600 ms), ESC backs out, "Back" / "Main Menu" entries.
- `collect_sub_files()` is recursive (flattens deep device nesting); categories list subfolders of `/UniversalRF/assets` + `/UniversalRF` (dedup, case-insensitive) to support both Flipper-style and flat layouts.

### UniversalRF Generic/Brands + readable labels (2026-08-05)

- **Generic test category (built-in, no DB required):** `show_categories()` always injects `Generic` as the FIRST option; a DB folder named `Generic` is skipped (case-insensitive). Its signals go through the standard `sendRfCommand()` pipeline (same init/deinit as DB files), so any board with a working RF module can be verified:
  - `Carrier 433/315/868 MHz` — ~0.5 s square-wave burst (`carrier_burst(350,700)` → `generic_send_raw`), checks TX + lets a receiver confirm the band.
  - `OOK Keyfob 433/315 MHz` — `generic_send_keyfob(..., 0x5A5A5A, 24 bits, TE 333)` via protocol `Princeton`, preset `Ook650Async` (both registered: `rf_registry.cpp:31`, `rf_presets.cpp:10`).
  - `Doorbell 433 MHz` — a fixed OOK timing sequence.
- **Brands flow (mirror of UniversalIR):** `browse_devices()` now shows `Brands (N)` as the first entry (grid of the category's subfolders → signals grid), then `Direct signals (N)` for flat `.sub` files at the category level, then `Back`.
- **Readable labels:** `pretty_name()` strips `.sub`, drops `RAW-`/`RAW_RECORDING-` prefixes (→ "Recording …"), and turns `_`/`-` into spaces for category names AND signal grids (used by `labels_for()`). Long labels truncate with a `~` suffix.
- **Shared grid engine:** `grid_navigate(labels, title, on_select)` — paginated grid rendering + Prev/Next wrap, Up/Down by column, rotary (`drainRotarySteps`), SEL (600 ms debounce; callback returns true to exit), ESC exits. Used by `sub_grid()`, the Brands grid, and the Generic menu.
- By-value String params (`browse_devices` cat_path/title, closure captures of `subdirs`/`direct`/`title`) keep the dangling-reference fix intact.

### UniversalRF/UniversalIR grid key UX fixes (2026-08-08)

- **Symptom:** "aggiusta un pò i tasti nel rf remote pk nn si capisce mai un cazzo" — the RF Remote (and IR remote_grid) grid gave no on-screen hint about which button does what, Up/Down jumped to a far end of the whole list when crossing page edges, and long signal lists (e.g. Samsung 60+ files) needed dozens of linear presses.
- **Fix (BOTH `universal_rf.cpp` and `universal_ir.cpp`, kept identical):**
  - **Legend:** every grid page now draws a centered dim `OK=select  ESC=back` line at `tftHeight - 13` (in `render_page`/`remote_grid`'s renderer). Requires the footer band: `compute_grid_metrics()` changed `availH = tftHeight - HEADER - 12` → `- 26` (row counts unchanged on all 4 boards; verified no cell/legend overlap).
  - **Page-flow Up/Down:** new `grid_move_up()`/`grid_move_down()` helpers replace the `(sel ± cols) % total` wrap. Up/Down now move by column WITHIN a page, flow to the same column on the adjacent page at page edges, and wrap to the other end only at the very first/last cell (matches standard `loopOptions` wrap). The old behavior sent the cursor to a random far cell on edge crossings.
  - **Page keys:** `NextPagePress`/`PrevPagePress` (already emitted on Cardputer from `,`/`/`) jump `± perPage` cells, so long lists page fast. No-op on other boards (flags never set).
- Helpers are `static` and take `sel/total/GridMetrics` by value — no closure-capture, no dangling-reference risk.

### UniversalRF/UniversalIR wrapper-folder DB discovery (2026-08-05, 16:10/16:15 bins)

- **Symptom:** users reported that after extracting the combined zips their file manager put the content inside a single folder named after the archive (e.g. `BruceIR3.0-UniversalIR-RF-Full/UniversalIR/...`), so the DB wasn't at the FS root and the browser found nothing.
- **Root cause:** the zips themselves are correct (both folders at the zip root) — the wrapper folder is created by the *extractor* (Windows "Extract All", some Android managers). Can't be fixed in the zip.
- **Fix:** `find_db_root(fs, name)` in both `universal_rf.cpp` and `universal_ir.cpp`: uses `/UniversalRF` (resp. `/UniversalIR`) directly when present, otherwise scans `/` up to 4 levels deep (`find_named_dir`, dirs only, skips dot-folders) for a folder with that name and uses its real path as the DB root. IR stores it in the file-scope `g_ir_root` (replaces all `UNIVERSAL_IR_ROOT`/`UNIVERSAL_IR_ASSETS`/`ORIENT_FILE` macro uses: categories, `layouts.ini`, `orient.txt`, flat files). RF computes `dbRoot`/`assetsRoot` locally in `show_categories()`.
- Both extraction layouts now work: folders at root, or nested inside any single wrapper folder.

### Universal RF heap-crash fix (2026-08-04)

- **Symptom:** Univ. RF Remote crashed (CRASH DIAG: `Task: loopTask`, `Cause: 0x1C`, `vaddr 0xABBA1234`, PC in ROM) after selecting signals; also "not many brands".
- **Root cause:** bruteforce-style RAW `.sub` files (1-2 MB "chaos"/recording files like `Walgreens_Chaos.sub`, `U_UNNI-UN0581B/RAW-RECORDING*.sub`) carry HUNDREDS of `RAW_Data:` lines. `readSubFile()` buffered EVERY line into the global `rawDataList`/`keyList`/`bitList`/`bitRawList` vectors → heap exhaustion → abort in a ROM mem function. Worst line is 8.3 KB (`Motion_Sensors/1ByOne.sub`); the Honda `Lock_honda.sub` is 204 KB / 104 lines.
- **Fix (src/modules/rf/rf_send.cpp):** (1) `readSubFile()` now CLEARS the four global vectors at start — a caller that returns early (no valid Protocol/Frequency) previously leaked every file's lines into them forever (accumulates across selections); (2) collection capped at `RF_SUB_MAX_SIGNALS` = 24 lines AND `RF_SUB_MAX_RAW_BYTES` = 64 KB total RAW data. This protects ALL callers (RF Raw menu, JS, browser).
- **Fix (src/modules/rf/universal_rf.cpp):** `send_sub_file()` now skips files > `MAX_SUB_FILE_BYTES` (256 KB) → shows "File too big to send" instead of crashing. 256 KB still allows the 204 KB Honda files; only the 1-2 MB monsters are blocked.
- Both fixes shipped in the 2026-08-04 19:05/19:10 release bins (verify ASCII `1.16`, `CRASH DIAG`, `File too big to send`).

### UniversalRF/UniversalIR browser dangling-reference fix (2026-08-04, 19:40/19:44 bins)

- **Symptom:** after the heap fix, a NEW crash appeared while BROWSING the category/device list (no send involved). CRASH DIAG: `Task: loopTask`, `Cause: 0x1C`, `vaddr 0x0000004C`, PC `0x400556D2` (ROM strlen/memcpy). Backtrace resolved with addr2line: `String::operator=(char const*)` ← `loopOptions` ← `browse_devices` (universal_rf.cpp) ← `show_categories` lambda.
- **Root cause — dangling reference into a destroyed closure:** the flow functions take `const String &title`/`&cat_path` that ALIAS by-value captured `String` members of the caller's `std::function` closure. Those closures live INSIDE the shared global `options` vector (main.cpp:150). The callee does `options.clear()` (destroying the CALLER's own executing closure → its captured Strings are freed), then calls `loopOptions(..., title.c_str())` → `title` is now a dangling reference → `title.c_str()` reads freed heap (e.g. `0x4C`) → `menuOptionLabel = subText` (display.cpp:582) runs `String::operator=(const char* 0x4C)` → `strlen(0x4C)` → panic.
- **Fix applied — pass String params BY VALUE** so the flow functions own a copy that survives `options.clear()`:
  - `src/modules/rf/universal_rf.cpp` `browse_devices(FS&, String cat_path, String title)`.
  - `src/modules/ir/universal_ir.cpp` `generic_signal_list` (brand/title/brands_path), `show_brands_flow` (cat_path/title), `show_remote` (title/spam_brand/brands_path) — forward decls updated too.
- **Pattern to avoid:** never keep a `const String&` (or any ref) into a captured member of a closure stored in the global `options` after an `options.clear()` — the clear destroys the executing closure. Stack locals of the current frame are safe (the frame outlives the clear); by-value copies are safe; references INTO closure members are not.
- Same class of bug fixed in BOTH UniversalRF and UniversalIR (IR had it in `generic_signal_list`/`show_brands_flow`/`show_remote`). Verify ASCII `1.16`, `CRASH DIAG`, `File too big to send`.

### UniversalRF DB packages (2026-08-03; Full REBUILT 2026-08-05, zip TRIMMED 2026-08-11)

Shipped ONLY merged into the two combined zips (see Deliverables). Extraction source trees in `sd_card_data/`:

- **RF Lite** (`sd_card_data/UniversalRF/`, 1.46 MB / 103 files): source `sloth632/Bruce-Scripts-Heaven@Main/BruceRF`. Scope: ONLY the useful **Gates** (`Lift_master`, `Road_Arm_(UK)`), **Garages** (`LiftMaster`, `Security_2.0`), **Vehicles** (no Tesla/Ford_Remote_Blocker). Fits alongside IR Lite (~0.16 MB) in the 3 MB LittleFS partition (~1.6 MB total). This is the sticks3/`/UniversalRF` payload.
- **RF Full** (`sd_card_data/UniversalRF-Full/`, 156 MB / 13275 files / 54 category folders, rebuilt 2026-08-05). **ALL useful source content restored**, with the brute-force families REORGANIZED under the category they attack instead of being dropped (user request "ripristinare tutto"):
  - `Garages/`: `CAME-12bit-433/-fast/868/-fast`, `NICE-12bit-433/868`, `Chamberlain-9bit-315/390`, `Linear-10bit-300/310` (from `some sort of brute force`; 57-63 files each, RAW OOK).
  - `Gates/`: `Ansonic-434`, `OOK_8bit` (64), `OOK_CAME`, `OOK_NICE`, `deBruijn_Binary/Open_Sesame/Raw`.
  - `Remote_Outlet_Switches/`: `Holtek-315/433/868/915`; `Restaurant_Pagers/`: `Spacca_pager-433`.
  - `Vehicles/Tesla` restored (43 files incl. subfolders).
  - Filter: only `.sub` ≤ 256 KB (matches the browser's `MAX_SUB_FILE_BYTES`). 146 oversized files (deBruijn NSCD sequences ~386 KB, mega-`RAW_Data` recordings, TOUCH TUNES pins) excluded by design. Base copy = every source category folder except: `Jamming` (4392), `Concert bracelet`, `Car Key Jammer`, `some sort of brute force`/`flipperzero-bruteforce`/`OOK_bruteforce`/`Brujin`/`deBruijn` (folders already remapped above). Verified file-by-file vs source: only 6 loose duplicate files at the `BruceRF` root are absent (their category copies exist).
- **2026-08-11 PACKAGE SCOPE CUT (user request):** the shipped Full ZIP (`BruceIR3.5-UniversalIR-RF-Full.zip`, 9.0 MiB) now contains ONLY `UniversalRF/Garages` (753) + `UniversalRF/Gates` (1179) + `UniversalRF/Vehicles` (120) + the unchanged `UniversalIR/` (829). The ~11k-file junk categories (TPMS, bracelets, WeVibe, RobotDog, dog collars, smoke alarms, touch tunes, etc.) are EXCLUDED from the zip — "gli unici folder davvero utili sono garage, gates e quelli di bruteforce". The full 13275-file source tree is UNTOUCHED in `sd_card_data/UniversalRF-Full/` (rebuild/extend any time); the 16 MiB zip is backed up at `%TEMP%\opencode\old_full_16MiB.zip`. Zip built with bsdtar (forward-slash entries), verified 0 entries outside scope.
- Curation facts: repo total = 19,128 blobs / ~1.4 GB (`git clone` sparse → `%TEMP%\opencode\brucerf_src\BruceRF`); the raw DB is dominated by bruteforce noise. Builder script: `%TEMP%\opencode\build_full_db.ps1`; verifier: `%TEMP%\opencode\verify_db.ps1`.
- `sd_card_data/UniversalRF/` = extracted Lite contents; `sd_card_data/UniversalRF-Full/` = extracted **new** Full contents (built 2026-08-05).

## RCA IR protocol fix (2026-08-02)

- **Root cause found:** the Universal IR DB's `TCL.ir` (and many other brands) uses `protocol: RCA`, but IRremoteESP8266 v2.8.6 (the lib in `.pio/libdeps`) has NO RCA support. In `sendIRCommand` (src/modules/ir/custom_ir.cpp) RCA wasn't in the dispatch AND the generic fallback requires a non-empty `data:` field, which parsed files don't have → **selecting RCA signals transmitted nothing**.
- **Fix applied:** added `sendRCACommand()` in `src/modules/ir/custom_ir.cpp` (declared in custom_ir.h), dispatched from `sendIRCommand`. Implements Flipper Zero's RCA spec exactly: carrier 38 kHz, preamble mark/space 4000/4000 µs, bit mark 500 µs, 0-space 1000 µs, 1-space 2000 µs, 24 bits MSB-first, layout `[cmd_inv:8][addr_inv:4][cmd:8][addr:4]`. Built via `IRsend::sendGeneric(4000,4000,500,2000,500,1000,0,0,data,24,38000,true,0,50)`.
- **Validation:** Power for a TCL TV = address `0F`, command `54` → data `0xAB054F`. Flipper devs verified `0F/54` works on real TCL TVs (PR flipperdevices/flipperzero-firmware#2823, used by Thomson/TCL/other brands).
- **Open item — friend's TCL TV (model unknown):** he captured his remote's Power with an external decoder as `Protocol: PulseDistance / Code: 0x00B0B54F / Bits: 24`. That value does NOT match the RCA encoding of `0F/54` (`0xAB054F`), and the inverse-redundancy check fails, so his TV likely uses a DIFFERENT address/command (or protocol). After flashing the RCA fix, re-test the TCL files on his TV. If still dead, get a RAW capture of his remote's buttons (use T-Embed IR Tools → Receive → Save, which stores an `.ir` the module can replay 1:1) and patch/create his TCL file with the real codes.

## RF+IR Dual detector crash fix (2026-08-03)

- **Root cause:** T-Embed crash/reboot on TV-remote press in Main Menu → RF+IR Dual. FreeRTOS `_rfTask`/`_irTask` returned WITHOUT `vTaskDelete(NULL)` → task stack stayed allocated and the caller then `vTaskDelete()`d already-terminated (or running) handles → abort-reboot.
- **Fix applied:** both `_rfTask`/`_irTask` in `src/modules/ir/dual_detect.cpp` and `_dualRfTask`/`_dualIrTask` in `src/modules/bjs_interpreter/subghz_js.cpp` now do `vTaskDelete(NULL)` after signalling, with the IR-read object (`IrRead`/`IrReader` RAII) scoped in a block so its destructor (disableIRIn + timer free) runs BEFORE self-delete. Caller-side stale-handle `vTaskDelete(rfTask/irTask)` calls removed in `_dualListen` and `native_subghzReadDual`; `vSemaphoreDelete(sem)` kept.
- **Pattern for any task that must clean itself up:** scope RAII resources in `{ }`, then `give semaphore`/`log`, then `vTaskDelete(NULL)`. Never delete a task handle from another task after it may have self-deleted.

## RF+IR Dual — REAL root cause: shared-SPI mutex give-without-hold (2026-08-03, second fix)

The `vTaskDelete(NULL)` fix above was necessary but NOT sufficient — the device still crashed alone (no remote press) with the coredump diagnostics showing `Task: dualRF`, `assert failed: xTaskPriorityDisinherit task.c:5156 (pxTCB == pxCurrentTCBs[ xPortGetCoreID() ])`.

- **True root cause:** on the T-Embed the CC1101 shares the display's SPI bus. `acquireSPIBus()` (src/core/bus_HAL.cpp:326) returns `&tft.getSPIinstance()` when the peripheral's MOSI == `TFT_MOSI`, and `getSPIinstance()` returns the **exact same `SPIClass`** (`spi`) that `begin_tft_write`/`end_tft_write` use (lib/TFT_eSPI/TFT_eSPI.cpp:5998). So TFT and CC1101 share one `_inTransaction` flag plus two FreeRTOS mutexes: `SPIClass::paramLock` and the HAL's `spi->lock` (both `xSemaphoreCreateMutex`).
- TFT_eSPI holds those mutexes for the **whole duration of each drawing primitive** (`spi.beginTransaction()` in `begin_tft_write`, released only in `end_tft_write`). The dualRF task (`src/modules/ir/dual_detect.cpp` `_rfTask` → `initRfModule` → `ELECHOUSE_cc1101.Init()`/`SpiStart()`) races against the main task's 80ms spectrum-redraw.
- The killer: SmartRC's `SpiStart()` opened every burst with a **spurious `cc_spi->endTransaction()`** ("ensure opening a new session"). When the bus is idle that is a harmless no-op (`_inTransaction==false`), but when the TFT holds the transaction it **GIVES the mutexes from a task that never took them** → `xTaskPriorityDisinherit` assert → PANIC. With the spectrum animation redrawing every 80ms, an overlap is guaranteed within a listen window.
- **Fix applied:** `patch_library_conflicts.py` now patches `.pio/libdeps/*/SmartRC-CC1101-Driver-Lib/ELECHOUSE_CC1101_SRC_DRV.cpp` to drop the leading `endTransaction()` in `SpiStart` (marked with a `// Bruce patch:` comment; the replacement string is made unique so the idempotency check doesn't false-positive against `SpiEnd`, which legitimately keeps its `endTransaction`). After the patch, the CC1101's own `beginTransaction`/`endTransaction` pair **blocks correctly** on the mutex the TFT holds instead of stealing it. Same-task serialization: TFT draw holds bus → CC1101 begin blocks → draw ends → CC1101 runs → gives back → next draw proceeds. No give-without-hold remains.
- This patch is safe for sole-bus users (sticks3's external CC1101 on AUX_SPI never sees an open transaction, so the removed call was a no-op there too).
- Note for anyone revisiting: if a shared-bus driver ever gives a mutex it didn't take, you get exactly this assert (`xTaskPriorityDisinherit` in task.c, `pxTCB == pxCurrentTCBs[core]`, where pxTCB is the recorded holder ≠ current giver). `xSemaphoreGive` on a mutex always runs this check on the give path.

## RF+IR Dual — follow-up freeze (deadlock) on the 14:32 bin, and the `_spi_started` fix (2026-08-03)

The 14:32 bin (simple "delete the spurious `endTransaction`" patch) REPLACED the assert-crash with a **freeze**: user reported the screen stopping right after entering RF+IR Dual ("lo schermo si è fermato appena sn entrato"), needing a hardware RST.

- **New root cause — self-deadlock on a non-recursive mutex:** `ELECHOUSE_CC1101::Reset()` (`.pio/libdeps/*/SmartRC-CC1101-Driver-Lib/ELECHOUSE_CC1101_SRC_DRV.cpp` L208-221) does `SpiStart()` but **never calls `SpiEnd()`**. `Init()` (L242-246) wraps it as `SpiStart(); if(!Reset()) return false; SpiEnd();`. The OLD leading `endTransaction()` in `SpiStart` was exactly what closed Reset's unfinished transaction; once removed, the inner `SpiStart()` inside `Reset()` hits `spiTransaction`'s `xSemaphoreTake(spi->lock, portMAX_DELAY)` while the **same task already holds** the non-recursive mutex → permanent block → freeze on entry (no assert, no panic).
- **Second hazard — early `checkMISO()` returns:** `SpiWriteReg`/`SpiWriteBurstReg`/`SpiStrobe`/`SpiReadReg`/`SpiReadBurstReg`/`SpiReadStatus` all do `SpiStart(); if(!checkMISO()) return …;` — the old spurious `endTransaction` was also healing THOSE leaked transactions.
- **Fix applied (2026-08-03, flag-based):** `patch_library_conflicts.py` now patches BOTH files of the SmartRC lib in both envs: header adds `bool _spi_started=false;` member; `SpiStart()` closes a transaction **only if `_spi_started` is true** (i.e. only THIS lib's own leftover — same task, so the give is legal and passes `xTaskPriorityDisinherit`), then sets `_spi_started=true` after `beginTransaction`; `SpiEnd()` sets `_spi_started=false`. The TFT's held transaction is **never** touched (when `_spi_started==false`, the next `beginTransaction` simply blocks until the draw releases). All 8 `checkMISO()` early-return leaks self-heal on the next `SpiStart`. `Reset()`'s internal `SpiStart` now closes only the transaction Init's outer `SpiStart` opened — no deadlock, no give-without-hold.
- **Both deliverables must be rebuilt** with this flag-based patch (previous 14:32/14:37 bins freeze at RF+IR Dual entry). Verify ASCII `1.16`, `CRASH DIAG`, `PAUSED - SEL to resume` after rebuild.

## RF+IR Dual — speed fixes + external RF module (2026-08-03, 15:20/15:25 bins)

Two "takes forever to collect the remote signal" causes, both fixed:

- **`_dualListen` waited for BOTH bands to finish the full window** (dual_detect.cpp `while (uxSemaphoreGetCount(ctx.sem) < 2)`) — even when IR caught the press in ~1s you still had to sit out the whole RF window (default 8s). Now the loop also breaks as soon as `ctx.rf != "" || ctx.ir != ""` (first band that fires stops), aborts the other band via `ctx.abort`, and waits ≤3s for it to wind down before showing the capture.
- **`IrRead::loop_headless` slept `delay(1000)` per poll iteration** (ir_read.cpp) — a captured frame (collected by the GPIO ISR) was only noticed up to ~1s later. Replaced with a millisecond deadline (`max_loops` is still a timeout in SECONDS — other callers pass seconds too) polling every 10ms.
- **External RF modules now selectable from the detector:** `_optionsMenu()` gained `"RF module: CC1101 / M5 RF433"` → calls the existing `setRFModuleMenu()` (settings.cpp:626, probes the CC1101 and errors if not found). The listen screen RF box shows `[CC1101]` or `[M5RF]`. This is how the M5StickS3 (no internal CC1101) uses an external CC1101 on its SPI bus (SS=2, GDO0=3, SCK=5/MOSI=6/MISO=4 — from `CC1101_*_PIN` macros; `CC1101_bus` defaults in configPins.h:126-127); the sticks3 board ini does NOT default `rfModule` to CC1101 (unlike T-Embed's interface.cpp:79/93), so it must be enabled here or in Settings → RF → RF Module.
- **External IR modules now selectable from the detector:** `_optionsMenu()` gained `"IR module: <name>"` → calls `gsetIrRxPin(true)` (settings.cpp:1121), the same `IR_RX_PINS` selector Bruce uses everywhere (T-Embed: `Default`=GPIO1 / `Pin 43` / `Pin 44`; sticks3: `M5 IR Mod` on GROVE_SCL etc). Listen screen IR subtitle shows `LISTENING [<name>]`. The next `IrRead` construction picks up the new pin (`irrecv` member captures `bruceConfigPins.irRx` at construction, and `_irTask` builds a fresh `IrRead` per cycle). `_irRxName()` in dual_detect.cpp resolves the pin to its label (must assign `IR_RX_PINS` to a typed `std::vector<std::pair<String,int>>` first — it's a brace-init macro, not iterable directly).

## RF+IR Dual — SD-gated Save + replay loop (2026-08-03, 16:22/16:27 bins)

- **SD-gated Save:** `_handleCapture()` (dual_detect.cpp) offers `Save Signal` **only when `sdcardMounted`** (T-Embed has SD → save enabled; M5StickS3 has no SD → the option is absent, replay-only). `_saveCapture()` also guards `if (!sdcardMounted) { displayError("No SD card - save disabled", true); return; }` as defense-in-depth. Captures save to `/BruceRF/*.sub` (RF) or `/BruceIR/*.ir` (IR) — the standard pre-created Bruce folders (`getFsStorage()` picks SD first, else LittleFS).
- **Replay loops back to menu:** `_handleCapture()` now wraps the Replay/Save/Discard/Exit menu in `while (true)` — after a Replay (or a Save) it re-shows the menu so the user can replay again, save, discard, or exit, instead of silently dropping the capture and resuming the listen loop. `Discard` and `ESC` resume listening; `Exit to Main Menu` leaves the detector.
- Replay never auto-saves: `_replayCapture()` writes a temp `/_dualdetect.{sub,ir}` file, replays it, then `fs->remove(tmp)`.

## RF+IR Dual — same-pin RF/IR conflict fix (2026-08-06, dual-spectrum "press twice" bug on Cardputer)

- **Symptom:** on the M5Cardputer ADV the friend had to press the TV remote AT LEAST twice before Dual Spectrum caught the signal.
- **Root cause:** on boards with a single-pin RF module the RMT RF receiver listens on `bruceConfigPins.rfRx`, which on the Cardputer defaults to `GROVE_SCL` (=GPIO1). The IR RX default is `RXLED`, and `include/precompiler_flags.h:58` defines `RXLED` as `GROVE_SCL` when the board doesn't set it (Cardputer ini sets only `TXLED=44`). So IR and RF BOTH listen on GPIO1: the RMT RF receiver sees the SAME demodulated IR burst and captures it as "RF RAW" first, the detector stops showing RF, and the user must press again (race — sometimes RF wins several times in a row).
- **Fix (src/modules/ir/dual_detect.cpp):** new `_rfIrConflict()` = `(rfModule != CC1101_SPI_MODULE) && (rfRx == irRx)`. When true the detector **skips the RF task entirely** (listen IR-only, first press always caught) and shows `RF DISABLED (same pin as IR)` on the listen screen. The semaphore wait loops now use a per-call `nTasks` counter instead of a hardcoded `2` (also covers the sequential-fallback branch, which skips RF when conflicted). New `RF pin: GPIO<x>` option in the `[NEXT]` options menu (calls the existing `gsetRfRxPin(true)`, settings.cpp:1194) so the user can resolve the conflict on-device — plus the existing `IR module:` and `RF module:` selectors.
- Note: only affects single-pin RF modules; a CC1101 (`rfModule == CC1101_SPI_MODULE`) receives on GDO0, not `rfRx`, so no conflict there (T-Embed unaffected).

## RF+IR Dual — auto-save immediate-save rewrite + noise rejection + IR RAW fallback (2026-08-11)

Applied to `src/modules/ir/dual_detect.cpp` + `src/modules/ir/ir_read.cpp` (working tree, included in the rebuilt 3.5 release bins; user-tested on T-Embed — the "must press twice" issue is a known, accepted artifact, NOT a regression).

- **Auto-save rewrite:** the previous "pending" flow only WROTE a capture when a NEW, DIFFERENT signal arrived (repeat-dedup via `_fingerprint`, plus a flush on exit). Users found that confusing — a captured signal seemed to vanish. Now every capture is saved **IMMEDIATELY** (`_autoSaveCapture` returns the saved filename; result screen footer shows `AUTO-SAVE: saved <file>` or `AUTO-SAVE: FAILED`), no pending state, no dedup. Storage via `getFsStorage()` = SD first, else LittleFS (boards without SD still keep captures). Footer is parameterized (`drawResultScreen(kind, content, status)`).
- **Noise rejection:** a floating/undriven input pin (sticks3 without an external IR module, unplugged RF receiver) makes the RMT receiver report electrical noise as a "signal" → Replay menu appears with no remote present. New `_plausibleSignal(kind, content)` gate (used inside `_rfTask`/`_irTask` AND in `dualDetect()` after `_dualListen`):
  - IR parsed signals (protocol/address/command, `data` field empty) → accepted.
  - IR RAW (`data` non-empty): pulse span (`_spanOf`, sums |duration| over the space-separated list) must be ≥ 3000 µs — real frames last ms on the air, noise is sub-ms. No token-count gate (short-but-real frames like an NEC repeat are accepted).
  - RF with a decoded `Key` → accepted. RF RAW (`RAW_Data`) needs span ≥ 2000 µs.
- **RF false-decode guard:** RCSwitch-style decoders can turn a short interference burst into a plausible-looking code. `_rfCapture` now requires the decoded/RAW `_data` span ≥ 2000 µs before accepting a capture (`transitions > 20` gate kept for the RAW branch).
- **IR RAW fallback (ir_read.cpp):** a non-decodable frame in auto mode (`results.decode_type == UNKNOWN`) previously returned `""` → press lost → "press twice" behavior. Now it falls back to RAW (`raw = true`) so the already-captured frame replays 1:1 and the FIRST press always counts.

## Board porting guide

- Board envs in `boards/<board>/<board>.ini`, include via `platformio.ini` `default_envs` (enable by uncommenting).
- `tft`, `tftWidth`, `tftHeight` are board-agnostic (src/main.cpp:150-168), backed by `tft_logger` wrapper (`src/core/tftLogger/tftLogger.cpp`, `include/tftLogger.h`).
- `include/tftLogger.h`: `BRUCE_TFT_DRIVER` = `tft_display` (HAS_SCREEN) from `lib/HAL/display/tft.h`; driver selected by macro: `USE_TFT_ESPI` (default), `USE_ARDUINO_GFX`, `USE_LOVYANGFX`, `USE_M5GFX`.
- M5GFX HAL: `lib/HAL/display/m5gfx.h` (`tft_display`, `tft_sprite`, wraps `M5.Display` via `M5Unified`). Has `setRotation`, `textWidth`, `drawCentreString`, etc.
- Add `-DARDUINO_LOOP_STACK_SIZE=16384` to any board that runs deep menu nesting (freeze fix).
- **`-O2` gotcha:** framework `pioarduino-build.py:116-119` appends `-Os -ffunction-sections -fdata-sections` AFTER all user `build_flags`; gcc uses the LAST `-O`, so `-O2` in `build_flags` is silently overridden → boards actually compile at `-Os`. To make `-O2` effective on sticks3, a `post:` extra_script (`boards/m5stack-sticks3/optflags.py`) strips the framework `-Os` from CCFLAGS and appends `-O2`. Beware: mixing `-Os` and `-O2` objects in one build → LTO `-Wodr` link errors (`LGFX_FILESYSTEM_Support incompatible type`); always full-clean an env when changing opt level.
- Boards with no SD: set `-DUNIVERSAL_IR_LITTLEFS_ONLY=1` for LittleFS-only Universal IR.
- Partition sizes in `custom_8Mb.csv` (e.g. `spiffs, data, spiffs, 0x4F0000, 0x300000` → 3 MB LittleFS).
- `src/core/sd_functions.cpp:33-36`: `setupLittleFS()`.

### M5StickS3 specifics
- ESP32-S3, 8 MB flash, M5GFX ST7789 135×240, no SD slot.
- IR TX GPIO46 (`-DIR_TX_PINS` Default=TXLED=46), external IR module via Grove; also G8/G1/G0/Grove W/Y. `-DIR_RX_PINS` default RXLED=42.
- CC1101 via SPI: GDO0=3, SS=2, MOSI/SCK/MISO=SPI pins.
- I2C Grove SDA=9 / SCL=10. Buttons SEL=11, DW=12, ACT=LOW.

### Cardputer and ESP32-C5 porting notes (2026-08-06)
- **Cardputer v1 + ADV = ONE env** (`m5stack-cardputer`). The upstream `boards/m5stack-cardputer-adv/` folder was deleted by commit `10e554df` ("Cardputer ADV pinouts and enhancements"): ADV keyboard (TCA8418 on SYS I2C 8/9) is detected at RUNTIME in `boards/m5stack-cardputer/interface.cpp` (`UseTCA8418`), falls back to the legacy v1 keyboard lib. `boards/_boards_json/m5stack-cardputer-adv.json` is a byte-identical copy of the v1 json (leftover). Do NOT re-create a separate adv env.
- Cardputer has no built-in SD (SDCARD_CS=12 etc. are for an external microSD sniffer module) → UniversalIR/RF use the LittleFS fallback automatically; the `LITE` DB fits the 3 MB partition.
- **ESP32-C5 is RISC-V** (toolchain `toolchain-riscv32-esp` auto-downloads on first build, ~500 MB; gdb = `tool-riscv32-esp-elf-gdb`). Arduino 3.3.9 core has C5 support (`variants/esp32c5` + friends).
- **CRASH DIAG is RISC-V-aware** (src/core/crash_diag.cpp): `#if CONFIG_IDF_TARGET_ESP32C3/C5/C6/C61/H2/P4 → BRUCE_CRASH_DIAG_RISCV` — prints `mcause`/`mtval` + `stackdump` size instead of the Xtensa `exc_cause`/`exc_vaddr`/`bt[]`/`depth`. Fix compiled in 2026-08-06; without it the C5 build fails at `crash_diag.cpp` (~8 errors).
- **C5-TFT is touch-only** (`HAS_TOUCH`, ILI9341+XPT2046): no physical buttons; `touchHeatMap()` (src/core/utils.cpp:250) maps screen zones → `PrevPress/SelPress/NextPress/EscPress/UpPress/DownPress`, so the Universal grid UI (`grid_navigate` uses the generic `check(...)` flags) works out of the box. Tapping the grid = SEL; the "Main Menu"/"Back" entries give ESC.
- Both new envs got `-DARDUINO_LOOP_STACK_SIZE=16384` (Universal menu nesting).
- Board folders: `boards/ESP32-C5-tft/` (pins_arduino.h + interface.cpp + ini). NOTE the filesystem is case-insensitive: `boards/esp32-c5-tft` and `boards/ESP32-C5-tft` are the SAME folder — there is no duplicate env.

## Other notes

- RF rolling-code analysis was researched; modern car rolling codes are NOT decodable by CC1101-class tools (only legacy KeeLoq). Feature abandoned by user — no code written.
- Discord posting: no webhook/token; user posts manually. Blocked.
- M5StickS3 hardware not in hand; port target confirmed but untested on real hardware.
- `boards/lilygo-t-embed-cc1101/lilygo-t-embed-cc1101.ini` already has the `ARDUINO_LOOP_STACK_SIZE=16384` freeze fix (modified).
- `src/core/menu_items/IRMenu.cpp` modified for module integration.

## Open items / next steps

1. User to flash `BruceIRF3.5-m5stack-sticks3.bin` and test on real hardware (friend will test).
2. Upload Lite DB contents to `/UniversalIR` on LittleFS (3 MB partition) on the sticks3.
3. T-Embed remains the primary target; verify no regression after the guarded `#if` change (T-Embed path is identical `#else`).
4. **Hardware validation (2026-08-03):** T-Embed own test PASSED — RF+IR Dual fully working (crash gone, near-instant capture, Replay loops/Save). User asked a friend to test RF on their own T-Embed — result pending. sticks3 (friend) — NO response yet; still needs UI lag check with effective `-O2`; external CC1101 (`RF module: CC1101`) + external IR (`IR module: M5 IR Mod`) capture.
5. **RCA fix shipped (2026-08-02).** Friend must re-flash T-Embed and re-test the TCL Power signal on his TV. If it still fails, his model uses non-DB codes → capture raw via T-Embed IR Tools → Receive → Save and build a custom TCL file.
6. **Shared-SPI mutex fix shipped (2026-08-03, flag-based `_spi_started`).** User confirmed the freeze/crash is GONE on T-Embed. If it ever recurs, re-capture the CRASH DIAG backtrace numbers (`0: 0x...`…`9: 0x...`) and resolve with `xtensa-esp32s3-elf-addr2line.exe` against `.pio\build\lilygo-t-embed-cc1101\firmware.elf`.
7. **Dual-detector speed + external RF/IR module shipped (2026-08-03); SD-gated Save + replay loop (16:22/16:27 bins).** User confirmed on own T-Embed: RF+IR Dual fully working, Save/Replay loop OK. Re-test pending from friend on their T-Embed (RF). On sticks3, enable `RF module: CC1101` (external module) and/or pick `IR module: M5 IR Mod` — external CC1101 + external IR receiver on boards without internal modules.
8. **UniversalRF browser dangling-reference fix shipped (2026-08-04).** Fixed by passing `title`/`cat_path`/`brand`/`brands_path` BY VALUE into the flow functions. User to re-flash T-Embed and confirm the browser no longer crashes while navigating the Full RF DB on SD.
9. **UniversalRF Generic/Brands + readable labels + rebuilt Full DB shipped (2026-08-05).** Firmware now has a built-in `Generic` test category, a Brands flow mirroring UniversalIR, and readable labels. **Wrapper-folder DB discovery shipped**: the two folders are found even when the extractor nests them inside a wrapper folder. User to flash and test on T-Embed.
10. **Cardputer (v1+ADV) + ESP32-C5 ILI9341 ports shipped (2026-08-06).** Both need real-hardware validation (friend with ESP32-C5 + ILI9341, and a Cardputer owner). CRASH DIAG RISC-V guard added in `crash_diag.cpp`.
11. **v3.5 release PUBLISHED 2026-08-08** (GitHub release `v3.5` on bollgio/BruceIRF — isDraft=false, latest): on-device version `3.5`, `BruceIRF3.5-<board>.bin` for **T-Embed, StickS3, Cardputer** + `BruceIR3.5-UniversalIR-RF-{Full,Lite}.zip`; tutorial README restored; source pushed to fork/main (`698791c7`). **C5 intentionally NOT built for 3.5** (user request) — the 3.0 C5 bin remains in project root. Old 3.0 bins/zips removed from root (superseded). User to flash the 3.5 bins (grid key UX fix included) and test on T-Embed / friends' devices.
12. **3.5 bins REBUILT 2026-08-11 with auto-save/noise fixes** (T-Embed `3.5-test` bin user-validated on real hardware — "ok funziona"; the "must press twice" artifact accepted, not a regression). Official `3.5` bins rebuilt for all 3 envs (`lilygo-t-embed-cc1101`, `m5stack-sticks3`, `m5stack-cardputer`), version strings verified in-binary. Changes to `dual_detect.cpp`/`ir_read.cpp` still UNCOMMITTED in the working tree (auto-save immediate-save rewrite + `_plausibleSignal` noise rejection + RF false-decode guard + IR RAW fallback). C5 still intentionally skipped.
