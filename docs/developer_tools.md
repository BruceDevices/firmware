# Developer Tools

Setting up everything you need to develop apps for Bruce firmware.

---

## BBT — Bruce Build Tool

For easy app development, a command-line tool called **BBT** (`bbt.py`) is provided. It handles all the heavy lifting: compiling source code, converting image assets, and packaging the final `.bruce` file.

BBT is analogous to Flipper Zero's [uFBT](https://github.com/flipperdevices/flipperzero-ufbt).

### Prerequisites

| Component | Version | Purpose |
|-----------|---------|---------|
| Python | 3.10 – 3.13 | Runs `bbt.py` |
| xtensa-esp32-elf-gcc | ESP-IDF v5.x | Cross-compiler for ESP32 |
| Pillow (optional) | Any | Image asset compilation |

### Installing the Xtensa Toolchain

The easiest way is to install [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/):

```bash
# Windows (PowerShell) — after installing ESP-IDF:
C:\Espressif\frameworks\esp-idf\export.ps1

# Linux / macOS:
source $HOME/esp/esp-idf/export.sh
```

Verify the installation:

```bash
xtensa-esp32-elf-gcc --version
# Expected output: xtensa-esp32-elf-gcc (crosstool-NG ...) 13.2.0
```

### Installing Pillow (optional)

Only needed if your app uses image assets.

```bash
pip install Pillow
```

### BBT Commands

| Command | Description |
|---------|-------------|
| `python bbt.py build` | Build the app in the current directory |
| `python bbt.py build --arch esp32s3` | Build for ESP32-S3 |
| `python bbt.py build --sdk /path/to/include` | Specify SDK path manually |
| `python bbt.py clean` | Remove build artifacts (`.bbt_build/`, `dist/`) |
| `python bbt.py create <appid>` | Scaffold a new app project |

### Build Output

After a successful build, the `.bruce` file is placed in the `dist/` subdirectory of your app folder:

```
my_app/
└── dist/
    └── my_app.bruce    ← Copy this to your device
```

---

## Hardware Debugger

Bruce firmware supports debugging over JTAG using ESP-PROG or any compatible debugger. See [ESP-IDF Debugging Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/jtag-debugging/) for setup instructions.

> **Note:** Debugging dynamically loaded apps (`.bruce` files) is limited to `bruce_log()` output via Serial Monitor. GDB breakpoints inside loaded apps are not currently supported because app code is relocated to RAM at runtime.

---

## Serial Monitor

All debug output from both the firmware and loaded apps is sent to the Serial port at **115200 baud**.

Use any Serial terminal:
- **PlatformIO Serial Monitor**: `pio device monitor`
- **Arduino IDE**: Tools → Serial Monitor
- **PuTTY / minicom / screen**: Connect to the device's COM port

```bash
# Example: PlatformIO
pio device monitor -b 115200
```
