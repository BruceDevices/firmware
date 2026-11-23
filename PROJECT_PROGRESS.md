# Bruce Firmware Project Tracker — ESP32-S3-Zero (4MB) with SH1106 128x64 I2C OLED

**Last Updated:** 2026-11-23  
**Status:** Successfully compiled, flashed, and verified custom 4MB ESP32-S3-Zero build with 1.3" SH1106 I2C OLED (GPIO 7/8), dark OLED polarity, compact 128x64 UI layout, Wi-Fi auto-connect ("Compass1"), rock-solid WebUI Navigator, and fully interactive USB-CDC Serial CLI on `/dev/ttyACM0`.

---

## 1. Hardware Inventory & Target Specs

| Board | Chip / Memory | Target Mode | Display & Pins | Flash & RAM Footprint |
| :--- | :--- | :--- | :--- | :--- |
| **Waveshare ESP32-S3-Zero** | ESP32-S3FH4R2 (4MB Quad Flash, 2MB Quad PSRAM) | 1.3" 128x64 I2C OLED + WebUI & Serial Navigator | SH1106 controller (132x64 GRAM, offset_x=2), SDA=GPIO 7, SCL=GPIO 8 | **Flash: 60.9%** (2,552,834 / 4,194,304 B)<br>**RAM: 35.1%** (114,892 / 327,680 B) |

---

## 2. Pin Map for Waveshare ESP32-S3-Zero

All pin conflicts with GPIO 7, 8, onboard WS2812 LED, flash/PSRAM, and strapping pins have been resolved:

| Peripheral | Assigned Pin(s) | Notes |
| :--- | :--- | :--- |
| **I2C Display (SH1106) & Grove** | **SDA = GPIO 7, SCL = GPIO 8** | 400 kHz, default 0x3C (fallback probe 0x3D). Shared cleanly with Bruce HAL |
| **Onboard RGB LED** | **GPIO 21** | WS2812B, GRB order (strictly reserved for RGB LED) |
| **RF Transceiver (CC1101 / NRF)** | **TX = GPIO 4, RX = GPIO 5** | Reassigned from GPIO 8/46 to eliminate pin conflict |
| **Infrared (IR)** | **TX = GPIO 6, RX = GPIO 14** | Reassigned away from GPIO 40/46 |
| **Hardware USB CDC & UART** | **Native USB-CDC (`/dev/ttyACM0`) & UART0 (RX=44, TX=43)** | Native USB-Serial/JTAG CDC active (`-DARDUINO_USB_CDC_ON_BOOT=1`, `-DARDUINO_USB_MODE=1`) |
| **GPS Module** | **RX = GPIO 44, TX = GPIO 43** | Reassigned away from GPIO 46/8 |
| **BadUSB (CH9329 Serial)** | **RX = GPIO 1, TX = GPIO 2** | Reassigned away from GPIO 46/8 |
| **SPI Bus (RC522 / CC1101)** | **SCK=12, MISO=13, MOSI=11, CS=10, GDO0=9** | Standard exposed SPI header |
| **MicroSD Card Bus** | **NC (unconfigured)** | Boot does not block waiting for SD/SPI hardware |
| **Internal Flash / PSRAM** | **GPIO 26–37** | Strictly untouched / reserved |
| **Strapping / Unexposed** | **GPIO 45, GPIO 46** | Strictly unused |

---

## 3. Implementation Details

1. **Lightweight SH1106 I2C Backend (`lib/DisplayDrivers/src/backends/sh1106.h`):**
   - Direct Arduino `Wire` communication at 400 kHz.
   - Probes `0x3C` default with fallback probe to `0x3D`.
   - 1024-byte RAM framebuffer (128x64 bits) with dirty page bitmask tracking.
   - SH1106 GRAM addressing: 132 columns with horizontal column offset of 2 (`0x02`, `0x10`).
   - Clean 1-bit color mapping: `0x0000` is OFF (0), any non-zero color is ON (1), preserving inverted highlight bars for selected menu items (black text inside solid white highlight bar).
   - Embedded 256-glyph 5x7 ASCII font in PROGMEM for self-contained rendering.
2. **Display Dimensions & Small Screen Scaling:**
   - `TFT_WIDTH = 128`, `TFT_HEIGHT = 64`, `ROTATION = 0`, `-DFP=1 -DFM=1 -DFG=1 -DDEFAULT_COLOR_INVERTED=0`.
   - `STATUS_BAR_HEIGHT` scaled to 11px and `rowHeightPx` to 10px when `TFT_HEIGHT <= 64` so 5 menu rows fit comfortably without vertical clipping.
   - Status bar replaced 16x16 icon bitmaps with compact single-character badges (`W`, `@`, `B`, `S`) to eliminate submenu title overlap.
   - `MenuItemInterface` coordinate initialization fixed so Carousel and Grid mode icons and labels never crop.
   - `WebUI` info screen rewritten to display `Net:`, `IP:`, `Web:`, `Url:`, and `Esc to exit` cleanly within 64px.
   - `progressHandler` and alert banners scaled down to sleek 4px and 12-14px heights.
   - Compact 1-second splash screen on small screens instead of 7-second blocking wait.
3. **Wi-Fi Auto-Connect & Background WebUI:**
   - Pre-registers SSID `"Compass1"` with password `"Compass2026"` and enables `wifiAtStartup = 1`.
   - Auto-connects on boot and starts WebUI non-blocking in background, dropping directly into the Bruce main menu on the OLED.
   - AP mode fallback (`BruceNet` / `172.0.0.1`) if `"Compass1"` is out of range.
4. **WebUI Navigator Crash Fixes:**
   - Guarded `tftLogger` (`imageSlot >= MAX_LOG_IMAGES`, null/empty strings, packet bounds). Previously, requesting `/getscreen` during/after boot caused an out-of-bounds slot lookup (`LoadProhibited` panic) and crashed the ESP32.
   - Converted `/cm` navigation handlers to non-blocking with `inputLock()` mutex protection.
   - Bumpered `loopTask` stack to 16KB and `wifiConnectTask` stack to 6KB.
5. **Interactive USB-CDC Serial CLI (`/dev/ttyACM0`) & `my_bruce` Compatibility:**
   - Removed `-DARDUINO_USB_CDC_ON_BOOT` from `build_unflags` so PlatformIO preserves `-DARDUINO_USB_CDC_ON_BOOT=1` and `-DARDUINO_USB_MODE=1`. This properly routes `Serial` to the ESP32-S3 USB-Serial/JTAG controller (`303a:1001`) instead of UART0 pins.
   - Fixed `USBSerial::vprintf`: Replaced corrupted variadic `out->printf(fmt, args)` call with `vsnprintf` formatting to prevent memory pointer pollution.
   - Fixed `my_bruce` Web Serial probe compatibility (`https://bruce.computer/my_bruce`): Added 150ms idle line flush timeout to execute commands without trailing newlines; suppressed raw character echo; formatted unmounted SD as `[E][sd_diskio.cpp: No SD card installed` to match expected regex.
   - Protected read-only query commands (`info`, `uptime`, `free`, `storage`, `display`, `date`, `i2c`, `help`) from triggering `backToMenu()`, allowing background serial probes without disturbing active UI navigation.
6. **WebUI Navigator & TFT Logger Enhancements:**
   - Fixed `tft_logger::fillScreen`: Added missing `restoreLogger()` call. Previously, `fillScreen` left `logging = false`, preventing all subsequent menu/grid elements from ever being recorded in `tftLogger`.
   - Converted `/getscreen` from static raw buffer to `AsyncResponseStream`, eliminating asynchronous buffer racing and packet corruption.
   - Removed `options.clear()` from `startWebUiBackground()`, preventing the background web server initialization from corrupting `loopTask`'s active options vector.
   - Fixed `loopOptions` in `src/core/display.cpp`: Decoupled `UpPress` from `PrevPress`, added a 750ms watchdog timeout to the `PrevPress` holding loop, and ensured `check(PrevPress)` is always cleared.
   - Chronological circular buffer ordering in `tft_logger::getBinLog` ensuring draw order remains consistent across wrapped logs.
7. **CI Workflow (`.github/workflows/build-s3-zero.yml`):**
   - Automatically installs `platformio requests esptool intelhex`, compiles `esp32-s3-zero-4mb`, and uploads `Bruce-esp32-s3-zero-4mb.bin`.

---

## 4. Flashing Instructions

### Command Line (`esptool.py`):
1. Put the board into download mode (hold **BOOT**, press & release **RESET**, then release **BOOT**).
2. Run:
   ```bash
   esptool.py --chip esp32s3 write_flash 0x0 Bruce-esp32-s3-zero-4mb.bin
   ```
   *(Or specify port if needed, e.g., `-p /dev/ttyACM0 -b 921600`)*
3. Tap **RESET**. The OLED will display the boot screen, and Serial CLI / WebUI will initialize.
