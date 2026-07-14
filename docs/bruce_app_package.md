# BAP (Bruce App Package)

`bbt` supports building apps as `.bruce` files. These are essentially `.elf` executables with extra metadata bundled in a `BapHeader`.

Apps are built with the `bbt.py build` command. They can be deployed to the device by copying the output `.bruce` file to the `/apps/` directory on the SD card or LittleFS.

`.bruce` apps do not depend on being run on a specific firmware build. Compatibility is determined by the app's metadata, which includes the required [API version](#api-versioning).

---

## How to set up an app to be built as a BAP

BAPs are standalone projects. To build your app as a `.bruce` package:

1. Create a folder with your app's source code anywhere on your system.
2. Write its code using the [BruceAPI](api_reference.md) functions.
3. Create an `application.bam` manifest file. See [Bruce App Manifests](bruce_app_manifest.md) for details.
4. Run `python bbt.py build` from the app's folder.

### Building and deploying

```bash
# Build your app
cd my_app/
python /path/to/bbt.py build

# Build for ESP32-S3 instead of ESP32
python /path/to/bbt.py build --arch esp32s3

# Clean build artifacts
python /path/to/bbt.py clean
```

### Scaffold a new project

BBT can generate a starter project for you:

```bash
python /path/to/bbt.py create my_cool_app
```

This creates:
```
my_cool_app/
├── application.bam       # Manifest
├── app.c                 # Entry point with boilerplate
└── assets/               # Image assets folder
```

---

## BAP Assets

BAPs can include **static and animated images as private assets**. They will be automatically compiled alongside app sources and can be referenced the same way as assets from the main firmware.

To use this feature, put your images in a subfolder inside your app's folder (default: `assets/`), then reference that folder in your app's manifest in the `assets_dir` field. See [Bruce App Manifests](bruce_app_manifest.md) for details.

To use these assets in your app, add `#include "bruce_assets.h"` in your app's source code. Then you can use all assets as C arrays:

```c
#include "bruce_api.h"
#include "bruce_assets.h"

void app_main(BruceAPI* api) {
    // logo.png → logo_data[], logo_width, logo_height
    api->draw_image(10, 20, logo_data, logo_width, logo_height);
}
```

### Supported image formats

| Format | Extension | Notes |
|--------|-----------|-------|
| PNG | `.png` | Recommended. Supports transparency (alpha is ignored). |
| BMP | `.bmp` | Legacy support. Uncompressed only. |

### Naming convention

The asset variable name is derived from the filename:

| Filename | C Variables |
|----------|-------------|
| `logo.png` | `logo_data[]`, `logo_width`, `logo_height` |
| `icon-wifi.png` | `icon_wifi_data[]`, `icon_wifi_width`, `icon_wifi_height` |
| `3d_cube.bmp` | `_3d_cube_data[]`, `_3d_cube_width`, `_3d_cube_height` |

Rules:
- Dashes (`-`) and spaces are converted to underscores (`_`).
- Names starting with a digit get a leading underscore.

### Image size guidelines

| Device | Screen | Max full-screen image |
|--------|--------|----------------------|
| M5Stack Cardputer | 240×135 | 63 KB (almost hits 64KB quota!) |
| M5Stack Core | 320×240 | 150 KB (exceeds quota) |

> **Tip:** Use small icons (32×32 = 2 KB, 64×64 = 8 KB) to stay well within the memory quota.

---

## Using Third-Party Libraries

You can use **any pure C/C++ library** by placing its source files inside your app directory. BBT automatically discovers and compiles all `.c`, `.cpp`, `.cc`, `.cxx` files recursively.

```
my_app/
├── application.bam
├── app.c
└── libs/
    ├── cJSON.c         # Third-party JSON parser
    └── cJSON.h
```

No additional configuration is needed — BBT handles everything.

### Important notes on external libraries

- Libraries are **statically linked** into your app's ELF. The firmware does not need to know about them.
- Libraries that call `malloc()` internally will work, but their allocations are **not tracked** by the firmware's memory manager. Use libraries with custom allocator support when possible.
- Libraries that depend on POSIX, Linux, or other OS features will **not work** on ESP32.

---

## How Bruce runs an app from the SD card

Bruce's MCU cannot run code directly from external storage, so it needs to be copied to RAM first. This is done by the **BAP Loader**, which is responsible for:

1. Loading the `.bruce` file from the SD card
2. Verifying its integrity and compatibility (magic, architecture, API version)
3. Copying the ELF payload to executable RAM (IRAM)
4. Processing Xtensa relocations to adjust for the new memory location
5. Calling the app's entry point with a pointer to the `BruceAPI` jump table

Since the app has to be loaded to RAM, the amount of free heap is reduced. Note that only code and data sections are loaded — the `.bruce` file also contains relocation tables and headers that are discarded after loading.

Apps run **synchronously** in the firmware's main task. When `app_main()` returns, the firmware reclaims all memory and returns to the menu.

---

## API Versioning

The `BruceAPI` struct includes an `api_version` field as its very first member. This allows apps to detect firmware compatibility at runtime:

```c
void app_main(BruceAPI* api) {
    if (api->api_version < BRUCE_API_VERSION) {
        api->bruce_log("Firmware too old!\n");
        return;
    }
    // ...
}
```

| API Version | Changes |
|-------------|---------|
| 1 | Initial: display, input, memory, logging, random, time |
| 2 | Added `draw_image`, `draw_pixel` |

**Compatibility rule:** New fields are always **appended** to the end of `BruceAPI`. Existing fields are **never removed or reordered**. This means apps built for API v1 will run correctly on firmware with API v2 — they simply won't see the new functions.

---

## Debugging BAPs

BBT does not currently support GDB-based debugging for loaded apps, since app code is relocated to RAM at runtime and its address is not known until load time.

For debugging, use `api->bruce_log()` to output debug information to the Serial port:

```c
api->bruce_log("x=%d, y=%d, state=%s\n", x, y, state_name);
```

All output appears on the Serial Monitor at **115200 baud**.

### Common debug patterns

```c
// Print heap info
api->bruce_log("Free heap: use bruce_millis=%u\n", api->bruce_millis());

// Print pointer addresses
api->bruce_log("Buffer at %p, size %d\n", buf, size);

// Print screen dimensions
api->bruce_log("Screen: %dx%d\n", api->get_tft_width(), api->get_tft_height());
```
