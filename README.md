![Bruce Main Menu](./media/pictures/bruce_banner.jpg)

# :shark: BruceIRF — Universal IR + RF firmware

**BruceIRF** is a fork of the official [Bruce](https://github.com/BruceDevices/firmware) ESP32 firmware focused on **Infrared (IR)** and **Sub-GHz RF**. It keeps the full standard Bruce feature set and adds two complete modules — **UniversalIR** and **UniversalRF** — plus a rebuilt RF database.

> Based on the official **Bruce** release. All credit for the base firmware goes to the **Bruce team** (pr3y and contributors). This fork only adds the IR/RF functionality described below.

---

## :inbox_tray: Download (Releases)

All ready-to-flash files are in the **[Releases](https://github.com/bollgio/BruceIRF/releases)** section — one click, no building needed:

| File | What it is | For |
|---|---|---|
| `BruceIRF4.0-lilygo-t-embed-cc1101.bin` | Firmware (merged image) | LilyGO T-Embed CC1101 :star: (primary) |
| `BruceIRF4.0-m5stack-sticks3.bin` | Firmware (merged image) | M5StickS3 |
| `BruceIRF4.0-m5stack-cardputer.bin` | Firmware (merged image) | M5Cardputer (v1 + ADV) |
| `BruceIR4.0-UniversalIR-RF-Full.zip` | IR 829 + RF 2052 files | SD card (T-Embed) |
| `BruceIR4.0-UniversalRF-Lite.zip` | RF 103 files | LittleFS (sticks3/Cardputer/C5) |

---

## :sparkles: What's new in BruceIRF

### v4.0
- :musical_note: **SubGHz Music RX** — RF audio player in SubGHz menu. Predefined melodies, next/prev, play/stop, volume control.
- :mango: **Pablo Mode Easter Egg** — Hidden in Others menu. Mango chaos with animated shapes, LEDC tones, escalating text. Auto-restarts after 42s.
- :earth_africa: **English Localization** — All UI strings in Universal IR/RF translated from Italian to English.
- :tv: **Built-in IR always visible** — IR categories (TV, Audio, DVD, etc.) always shown with generic functional buttons (Power, Vol+, Vol-, Mute, Ch+, Ch-) that cycle through all brands' protocol codes. **Brands** and **Orient** options included in the grid.
- :open_file_folder: **Embedded IR database** — 6 core IR files (TV, AC, Audio, Fans, LEDs, Projectors) compressed with LZ4 and embedded in the firmware. Works **without SD card or LittleFS** — on first boot, files are decompressed and written to storage automatically.
- :wrench: **MCLK pin conflict fix** on T-Embed CC1101, Dual spectrum fix, IR Clone UX, SD dedup.

### v3.6
- :repeat: **RF Bruteforce infinite loop** — loop toggle in Bruteforce menu, progress bar shows sweep count.
- :satellite: **RF Frequency Scanner** — sweeps all SubGHz frequencies, shows strongest signals in grid, SEL picks and saves.
- :clipboard: **IR Clone into DB** — capture a remote button → name it → pick category → save as `.ir` file.
- :star: **Preferiti/Recenti for IR & RF** — long-press SEL to toggle favorites, history grid for replay.

### Core features
- :satellite: **Univ. RF Remote** — Sub-GHz browser: categories → brands → signals. Reads Flipper Zero `.sub` files (RAW + BinRAW).
- :bulb: **Univ. IR Remote** — Universal IR browser (categories, brands, signal grids, spam replay).
- :wrench: **Built-in Generic RF test** — Carrier 433/315/868 MHz, OOK Keyfob, Doorbell — no database needed.
- :zap: **RF+IR Dual Detector** — captures on first band that fires, external RF/IR module selectors, SD-gated Save, replay loop.
- :card_file_box: **Rebuilt RF database** — every useful source restored, brute-force families reorganized.
- :hammer: **RCA IR protocol fix** (TCL & compatible TVs), crash fixes, onboard **CRASH DIAG**.

---

## :computer: Supported boards

| Board | File |
|---|---|
| **LilyGO T-Embed CC1101** :star: (primary, validated) | `BruceIRF4.0-lilygo-t-embed-cc1101.bin` |
| **M5StickS3** | `BruceIRF4.0-m5stack-sticks3.bin` |
| **M5Cardputer** (v1 + ADV, auto-detected) | `BruceIRF4.0-m5stack-cardputer.bin` |

The firmware `.bin` files are **merged full images** (bootloader + partitions + firmware) — ready to flash as-is.

---

## :rocket: Install — flashing

Flashing is done with **M5Launcher** (T-Embed, needs SD card) or **ESP Web Tool** (all boards). The `.bin` files are **merged full images** — ready to flash as-is.

### Tutorial 1 — LilyGO T-Embed + M5Launcher (SD card)
1. Download `BruceIRF4.0-lilygo-t-embed-cc1101.bin` from the **Releases** page.
2. Extract the **Full** DB zip onto your SD card, so the SD root contains the `UniversalIR` and `UniversalRF` folders.
3. Put the `.bin` on the same SD card (or load it over WiFi in M5Launcher).
4. Boot the T-Embed into **M5Launcher** and select the `.bin` to flash.
5. It flashes and reboots straight into BruceIRF — the database is already on the SD card.

### Tutorial 2 — LilyGO T-Embed + ESP Web Tool
1. Download `BruceIRF4.0-lilygo-t-embed-cc1101.bin` from the **Releases** page.
2. Connect the T-Embed via USB.
3. Open the ESP Web Tool (EspWebTool) in Chrome/Edge, connect the board, select the `.bin`, flash at address `0x0`.
4. Database: extract the **Full** zip onto an SD card (`UniversalIR` + `UniversalRF` at the SD root) and insert it in the T-Embed.

### Tutorial 3 — M5StickS3 + ESP Web Tool
1. Download `BruceIRF4.0-m5stack-sticks3.bin` from the **Releases** page.
2. Connect the M5StickS3 via USB.
3. Open the ESP Web Tool (EspWebTool) in Chrome/Edge, connect the board, select the `.bin`, flash at address `0x0`.
4. Database (no SD card): open Bruce's **Web UI → File Manager** and upload the contents of the **Lite** zip (`UniversalRF`) into LittleFS.

### Tutorial 4 — M5Cardputer + ESP Web Tool
1. Download `BruceIRF4.0-m5stack-cardputer.bin`.
2. Connect the Cardputer via USB.
3. Open the ESP Web Tool (EspWebTool) in Chrome/Edge, connect the board, select the `.bin`, flash at address `0x0`.
4. Database (no SD): upload the **Lite** zip contents into LittleFS via **Web UI → File Manager**.

---

## :open_file_folder: Install — RF database

The database folders are included in this repository and in the release zips:

| Package | Content | Use for |
|---|---|---|
| `BruceIR4.0-UniversalRF-Lite.zip` | RF 103 files (Garages, Gates, Vehicles) | **LittleFS** (sticks3/Cardputer/C5) |
| `BruceIR4.0-UniversalIR-RF-Full.zip` | IR 829 + RF 2052 files | **SD card** (T-Embed) |

> **Note:** IR Built-in categories (TV, Audio, DVD, etc.) are **embedded in the firmware** (LZ4-compressed) and work **without any database files**. On first boot they are decompressed to storage. Additional IR brands can be added via the Full zip on SD. The RF database is only needed for the Universal RF Remote browser.

### SD card (Full)
Extract the **Full** zip to the SD card root so you get `UniversalIR/` and `UniversalRF/` folders.

### LittleFS (Lite)
Upload the contents of the **Lite** zip (`UniversalRF/`) into LittleFS using Bruce's Web UI → File Manager.

The modules find the folders **both at the storage root and inside a single wrapper folder** — no manual moving needed.

---

## :video_game: Usage

- **IR → Univ. IR Remote** — browse IR categories/brands, open a signal grid, SEL sends.
- **RF → Univ. RF Remote** — browse RF categories/brands (`.sub` files), SEL sends.
- **RF → Generic** (first entry, always present) — built-in test signals.
- **SubGHz → SubGHz Music RX** — RF audio player with predefined melodies.
- **Others → Pablo Mode** — mango chaos easter egg.
- **RF+IR Dual** (Main Menu) — capture RF and IR at the same time.

Grid navigation: every page shows `OK=select · ESC=back` at the bottom. Prev/Next wrap, Up/Down move by column, page keys (`, ` `/` on Cardputer) jump a page, SEL selects/sends, ESC backs out.

---

## :hammer_and_wrench: Building from source

```bash
pio run -e lilygo-t-embed-cc1101 -t build-firmware
pio run -e m5stack-sticks3 -t build-firmware
pio run -e m5stack-cardputer -t build-firmware
```

Build outputs (merged bins `BruceIRF4.0-<board>.bin`) are written to the project root. See `AGENTS.md` for the full build/development notes.

---

## :heart: Credits

- **[Bruce](https://github.com/BruceDevices/firmware)** — base firmware by **pr3y** and all Bruce contributors
- **[Flipper Zero](https://github.com/flipperdevices/flipperzero-firmware)** — `.sub` file format, IR file format reference, protocol specs
- **[IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)** — IR protocol library
- **[RadioLib](https://github.com/jgromes/RadioLib)** — CC1101 / RF driver
- **[ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)** — I2S audio pipeline
- **[LilyGO](https://github.com/Xinyuan-LilyGO/T-Embed-CC1101)** — T-Embed CC1101 hardware + reference examples
- **[M5Stack](https://github.com/m5stack)** — M5StickS3 and Cardputer hardware
- **[Sloth632 Bruce-Scripts-Heaven](https://github.com/sloth632/Bruce-Scripts-Heaven)** — RF database source files
- **pablo** — you know what you did :mango:

---

## :pushpin: Notes / status

- **T-Embed** is the primary, validated target.
- **M5StickS3 / M5Cardputer** builds are release-ready.
- An **ESP32-C5 + ILI9341** build is supported from source (RISC-V) — built separately on request.
- Report issues with the on-screen **CRASH DIAG** task + backtrace — it makes debugging much faster.
