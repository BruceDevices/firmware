![Bruce Main Menu](./media/pictures/bruce_banner.jpg)

# :shark: BruceIRF — Universal IR + RF firmware

**BruceIRF** is a fork of the official [Bruce](https://github.com/BruceDevices/firmware) ESP32 firmware focused on **Infrared (IR)** and **Sub-GHz RF**. It keeps the full standard Bruce feature set and adds two complete modules — **UniversalIR** and **UniversalRF** — plus a rebuilt RF database.

> Based on the official **Bruce** release. All credit for the base firmware goes to the Bruce team. This fork only adds the IR/RF functionality described below.

---

## :inbox_tray: Download (Releases)

All ready-to-flash files are in the **[Releases](https://github.com/bollgio/BruceIRF/releases)** section — one click, no building needed:

| File | What it is | For |
|---|---|---|
| `BruceIRF3.5-lilygo-t-embed-cc1101.bin` | Firmware (merged image) | LilyGO T-Embed CC1101 :star: (primary) |
| `BruceIRF3.5-m5stack-sticks3.bin` | Firmware (merged image) | M5StickS3 |
| `BruceIRF3.5-m5stack-cardputer.bin` | Firmware (merged image) | M5Cardputer (v1 + ADV) |
| `BruceIR3.5-UniversalIR-RF-Full.zip` | IR 829 + RF 13.275 files | SD card |
| `BruceIR3.5-UniversalIR-RF-Lite.zip` | IR 7 + RF 103 files | LittleFS (no-SD boards) |

---

## :sparkles: What's new in BruceIRF

- :satellite: **Univ. RF Remote** — a complete Sub-GHz browser: categories → brands → signals. Reads Flipper Zero `.sub` files (RAW + BinRAW Key format) through the standard Bruce RF pipeline.
- :bulb: **Univ. IR Remote** — the Universal IR browser (categories, brands, signal grids, spam replay).
- :wrench: Built-in **Generic test category** (no database required) — Carrier 433 / 315 / 868 MHz, OOK Keyfob (Princeton), Doorbell 433 MHz — to verify your RF module in seconds.
- :label: **Brands flow + readable labels** — raw names like `RAW-RECORDING-0001` show as "Recording 0001", underscores/dashes become spaces.
- :card_file_box: **Rebuilt RF Full database** — every useful source category restored, brute-force families reorganized under the category they attack (CAME / NICE / Chamberlain / Linear → Garages, OOK / deBruijn → Gates, Holtek, Tesla → Vehicles...).
- :zap: **RF+IR Dual Detector** — captures on the first band that fires, external RF (CC1101 / M5 RF433) and IR module selectors, SD-gated Save, replay loop, same-pin RF/IR conflict handling.
- :tv: **Grid key UX** — every grid page shows an on-screen legend (`OK=select · ESC=back`), Up/Down flow page-to-page by column, and long lists jump a whole page at a time (Cardputer arrow keys).
- :hammer: **RCA IR protocol fix** (TCL & compatible TVs), crash fixes (heap exhaustion on huge RAW files, shared-SPI mutex between CC1101 and display, browser dangling-reference), onboard **CRASH DIAG** diagnostic screen.

---

## :computer: Supported boards

| Board | File |
|---|---|
| **LilyGO T-Embed CC1101** :star: (primary, validated) | `BruceIRF3.5-lilygo-t-embed-cc1101.bin` |
| **M5StickS3** | `BruceIRF3.5-m5stack-sticks3.bin` |
| **M5Cardputer** (v1 + ADV, auto-detected) | `BruceIRF3.5-m5stack-cardputer.bin` |

The firmware `.bin` files are **merged full images** (bootloader + partitions + firmware) — ready to flash as-is.

---

## :rocket: Install — flashing

Flashing is done with **M5Launcher** (LilyGO T-Embed only, needs an SD card) or **ESP Web Tool** (all boards). The `.bin` files are **merged full images** (bootloader + partitions + firmware) — ready to flash as-is.

### Tutorial 1 — LilyGO T-Embed + M5Launcher (SD card)
1. Download `BruceIRF3.5-lilygo-t-embed-cc1101.bin` from the **Releases** page.
2. Extract the **Full** DB zip onto your SD card, so the SD root contains the `UniversalIR` and `UniversalRF` folders.
3. Put the `.bin` on the same SD card (or load it over WiFi in M5Launcher).
4. Boot the T-Embed into **M5Launcher** and select `BruceIRF3.5-lilygo-t-embed-cc1101.bin` to flash.
5. It flashes and reboots straight into BruceIRF — the database is already on the SD card.

### Tutorial 2 — LilyGO T-Embed + ESP Web Tool
1. Download `BruceIRF3.5-lilygo-t-embed-cc1101.bin` from the **Releases** page.
2. Connect the T-Embed via USB.
3. Open the ESP Web Tool (EspWebTool) in Chrome/Edge, connect the board, select the `.bin`, flash at address `0x0`.
4. Database: extract the **Full** zip onto an SD card (`UniversalIR` + `UniversalRF` at the SD root) and insert it in the T-Embed.

### Tutorial 3 — M5StickS3 + ESP Web Tool
1. Download `BruceIRF3.5-m5stack-sticks3.bin` from the **Releases** page.
2. Connect the M5StickS3 via USB.
3. Open the ESP Web Tool (EspWebTool) in Chrome/Edge, connect the board, select the `.bin`, flash at address `0x0`.
4. Database (the M5StickS3 has **no SD card**): open Bruce's **Web UI → File Manager** and upload the contents of the **Lite** zip (`UniversalIR` + `UniversalRF`) into LittleFS.

### Tutorial 4 — M5Cardputer + ESP Web Tool
1. Download `BruceIRF3.5-m5stack-cardputer.bin`.
2. Connect the Cardputer via USB.
3. Open the ESP Web Tool (EspWebTool) in Chrome/Edge, connect the board, select the `.bin`, flash at address `0x0`.
4. Database (no SD on this board): upload the **Lite** zip contents into LittleFS via **Web UI → File Manager**.

---

## :open_file_folder: Install — IR + RF database

The database folders are included in this repository:

| Folder | Content | Use for |
|---|---|---|
| `UniversalIR/` + `UniversalRF/` | **Lite** (IR 7 + RF 103 files) | **LittleFS** (no-SD boards, M5StickS3) |
| `UniversalIR-Full/` + `UniversalRF-Full/` | **Full** (IR 829 + RF 13.275 files, ~156 MB) | **SD card** users (T-Embed) |

### SD card (Full)
Copy the **contents** of `UniversalIR-Full` and `UniversalRF-Full` to the root of your SD card, renaming the folders to `UniversalIR` and `UniversalRF` — so your SD card ends up with the folders `UniversalIR` and `UniversalRF`.

### LittleFS (Lite)
Upload the contents of `UniversalIR` and `UniversalRF` into LittleFS using Bruce's Web UI → File Manager.

The modules find the folders **both at the storage root and inside a single wrapper folder** (some file managers extract an archive into a folder named after the `.zip`) — no manual moving needed.

The same content is also distributed as two combined archives:
- `BruceIR3.5-UniversalIR-RF-Full.zip` — IR 829 + RF 13.275 files
- `BruceIR3.5-UniversalIR-RF-Lite.zip` — IR 7 + RF 103 files

---

## :video_game: Usage

- **IR → Univ. IR Remote** — browse IR categories/brands, open a signal grid, SEL sends.
- **RF → Univ. RF Remote** — browse RF categories/brands (`.sub` files), SEL sends.
- **RF → Generic** (first entry, always present) — built-in test signals: Carrier 433/315/868 MHz, OOK Keyfob, Doorbell 433 MHz.
- **RF+IR Dual** (Main Menu) — capture RF and IR at the same time; options to pick the RF module (CC1101 / M5 RF433) and the external IR receiver pin.

Grid navigation: every page shows `OK=select · ESC=back` at the bottom. Prev/Next wrap, Up/Down move by column and flow page-to-page, page keys (`, ` `/` on Cardputer) jump a whole page, SEL selects/sends, ESC backs out, "Back" / "Main Menu" entries.

---

## :hammer_and_wrench: Building from source

```bash
pio run -e lilygo-t-embed-cc1101 -t build-firmware
pio run -e m5stack-sticks3 -t build-firmware
pio run -e m5stack-cardputer -t build-firmware
pio run -e esp32-c5-tft -t build-firmware
```

Build outputs (merged bins `BruceIRF3.5-<board>.bin`) are written to the project root. See `AGENTS.md` (kept out of this repo) for the full build/development notes.

---

## :pushpin: Notes / status

- **T-Embed** is the primary, validated target.
- **M5StickS3 / M5Cardputer** builds are release-ready; the StickS3 awaits final real-hardware validation (UI + external CC1101/IR capture). An **ESP32-C5 + ILI9341** build is also supported from source (RISC-V; first build auto-downloads the ~500 MB riscv toolchain) — built separately on request.
- Report issues with the on-screen **CRASH DIAG** task + backtrace — it makes debugging much faster.
