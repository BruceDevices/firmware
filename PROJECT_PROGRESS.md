# Bruce Firmware Project Tracker — ESP32-S3-Zero (4MB) & ESP32-WROOM

**Last Updated:** 2026-11-22  
**Status:** Ready to compile & flash custom 4MB ESP32-S3-Zero build

---

## 1. Hardware Inventory & Target Specs

| Board | Chip / Memory | Target Mode | Flash Constraint |
| :--- | :--- | :--- | :--- |
| **ESP32-S3-Zero** | ESP32-S3FH4R2 (4MB Quad Flash, 2MB Quad PSRAM) | Headless WebUI (optional 1" OLED later) | Prebuilt S3 binaries expect 8MB/16MB + Octal PSRAM (`qio_opi`) -> requires custom `4MB` + `qio_qspi` build |
| **ESP32-WROOM** | ESP32 (4MB Flash, no PSRAM) | Headless WebUI | Compatible with standard 4MB headless builds |

---

## 2. Files Included in This Pack

Extract this archive into the root of your cloned or forked `BruceDevices/firmware` repository:

- `boards/ESP-General/esp32-s3-zero-4mb.ini` — Drop-in PlatformIO environment (auto-discovered via `extra_configs = boards/*/*.ini`; avoids editing upstream `ESP-General.ini`).
- `.github/workflows/build-s3-zero.yml` — One-click cloud build workflow on GitHub Actions that produces `Bruce-esp32-s3-zero-4mb.bin`.
- `flash_s3_zero.sh` / `flash_s3_zero.ps1` — Flashing scripts for Linux/Ubuntu and Windows (`esptool.py` at `0x0`).
- `PROJECT_PROGRESS.md` — This tracker.

---

## 3. Progress Checklist

### Phase A — ESP32-S3-Zero Custom 4MB Build
- [x] Diagnosed boot-loop cause (8MB/16MB partition table + `qio_opi` PSRAM mismatch on 4MB S3-Zero).
- [x] Created drop-in PlatformIO config (`esp32-s3-zero-4mb`) using `custom_4Mb_full.csv`, `qio_qspi`, `${env_4mb.build_flags}`, `-DLITE_VERSION=1`, and `-DARDUINO_USB_CDC_ON_BOOT=1`.
- [x] Created GitHub Actions cloud build workflow (`build-s3-zero.yml`) and flashing scripts.
- [ ] Fork or clone `https://github.com/BruceDevices/firmware` and drop in the files from this pack.
- [ ] Run build:
  - **Cloud:** GitHub **Actions** -> **Build Bruce ESP32-S3-Zero 4MB** -> **Run workflow** -> download artifact `Bruce-esp32-s3-zero-4mb.bin`.
  - **Local PlatformIO:** `pio run -e esp32-s3-zero-4mb`
- [ ] Enter ROM download mode (hold **BOOT**, tap **RST**, release **BOOT**) and flash `Bruce-esp32-s3-zero-4mb.bin` at `0x0` (via `espressif.github.io/esptool-js/` or `flash_s3_zero.sh`).
- [ ] Tap **RST** and verify clean boot over USB serial (`115200` baud).

### Phase B — Headless WebUI Bootstrap (S3-Zero or WROOM)
- [ ] Open serial terminal at `115200` baud.
- [ ] Configure Wi-Fi and start WebUI:
  ```text
  wifi add <YourSSID> <YourPassword>
  webui
  info
  ```
- [ ] Browse to `http://bruce.local` (or the IP printed by `info`) and open the **Navigator** tab.
- [ ] Enable persistence in WebUI:
  - **Config -> Startup WiFi -> Enable**
  - **Config -> Startup App -> WebUI**

### Phase C — Optional Hardware Expansion (Later)
- [ ] Test removing `-DLITE_VERSION=1` if flash usage leaves enough headroom.
- [ ] Wire external 1" I2C OLED or RF/IR modules (remember to override `GROVE_SCL` away from GPIO 46 and avoid internal Flash/PSRAM GPIOs 26–37).

---

## 4. Quick Notes & Gotchas

- **ESP32-S3-Zero Reserved Pins:** Do **not** use GPIOs 26–37 (Quad Flash/PSRAM or unexposed). `ESP-General.ini` sets `GROVE_SCL=46`, which is unexposed on the S3-Zero edge header—uncomment `-UGROVE_SCL` / `-DGROVE_SCL=9` in `esp32-s3-zero-4mb.ini` before wiring external Grove/IR/RF hardware.
- **Native USB Port:** Because the S3-Zero uses native USB CDC (`-DARDUINO_USB_CDC_ON_BOOT=1`), its serial port may re-enumerate after resetting out of download mode (`/dev/ttyACM0` on Linux or a new `COMx` on Windows).
- **Build Log Troubleshooting:** If the compile overflows flash, check `lib_ignore` in `esp32-s3-zero-4mb.ini` and keep `-DLITE_VERSION=1` enabled; you can also inspect which libraries consume space from the PlatformIO linker report.
