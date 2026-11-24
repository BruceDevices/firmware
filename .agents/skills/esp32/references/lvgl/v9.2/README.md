# LVGL v9.2.x Reference

> **Baseline**: v9.1.0 | **Target**: v9.2.0, v9.2.1, v9.2.2
> **Release Date**: August 26, 2024 (v9.2.0)
> **Patch Versions**: v9.2.0, v9.2.1 (2024-10-24), v9.2.2 (2024-10-29)

---

## Summary of Changes from v9.1

v9.2.0 is a **major feature release** with one **breaking change** (`lvgl_private.h`). Key additions: Wayland driver, L8/I1 color format rendering, matrix transformations, Lottie animation widget, OpenGL ES driver, radial/conic/skew gradients, mouse hover support, QNX/MQX RTOS support, Renesas GLCDC driver, Chinese calendar, and property system for widgets. Over 120 new features and 150+ bug fixes.

---

## Release Timeline

| Version | Date | Notes |
|---------|------|-------|
| v9.2.0 | 2024-08-26 | Major feature release |
| v9.2.1 | 2024-10-24 | Backported fixes + GIF/animimg features |
| v9.2.2 | 2024-10-29 | **Hotfix**: Reverts Kconfig break from v9.2.1. **Use v9.2.2 instead of v9.2.1.** |

---

## Breaking Changes

### `lvgl_private.h` Separation

**This is the only breaking change.** Internal structs and APIs are now behind `lvgl_private.h`.

```c
/* If your code accesses internal LVGL structures, you have two options: */

/* Option 1: Include the private header where needed */
#include "lvgl_private.h"

/* Option 2: Enable globally in lv_conf.h */
#define LV_USE_PRIVATE_API  1
/* This makes lvgl.h internally include lvgl_private.h */
```

**What moved to private**: Internal struct members of `lv_obj_t`, `lv_display_t`, `lv_indev_t`, `lv_font_t`, and most `_lv_*` prefixed functions. Public API function signatures remain unchanged.

---

## New Features

### Major New Capabilities

| Feature | PR | Description |
|---------|----|-------------|
| **Lottie widget** | [commit](https://github.com/lvgl/lvgl/commit/9c5ca0e) | ThorVG-based Lottie animation player widget (`LV_USE_LOTTIE`) |
| **Wayland driver** | [#6549](https://github.com/lvgl/lvgl/pull/6549) | Full Wayland display driver (ported from v8), with XDG shell and optional wl_shell |
| **OpenGL ES driver** | [#6254](https://github.com/lvgl/lvgl/pull/6254), [#6600](https://github.com/lvgl/lvgl/pull/6600) | OpenGL ES rendering with multi-window + external texture embedding |
| **Matrix transformations** | [#4883](https://github.com/lvgl/lvgl/pull/4883) | Global matrix-based drawing modes for arbitrary 2D transforms |
| **L8 color format** | [#5800](https://github.com/lvgl/lvgl/pull/5800) | 8-bit luminance (grayscale) rendering support in SW renderer |
| **I1 color format** | [#6345](https://github.com/lvgl/lvgl/pull/6345) | 1-bit indexed (monochrome) rendering support in SW renderer |
| **Radial gradients** | [#6170](https://github.com/lvgl/lvgl/pull/6170) | Software-rendered radial gradient backgrounds |
| **Mouse hover** | [#5947](https://github.com/lvgl/lvgl/pull/5947) | Hover state support for pointer devices (`LV_STATE_HOVERED`) |
| **Chinese calendar** | [#5940](https://github.com/lvgl/lvgl/pull/5940) | Chinese lunisolar calendar support (`LV_USE_CALENDAR_CHINESE`) |
| **Property system** | [#6329](https://github.com/lvgl/lvgl/pull/6329), [#6275](https://github.com/lvgl/lvgl/pull/6275) | Generic property get/set API for widgets with name resolution |
| **Object ID system** | [#6278](https://github.com/lvgl/lvgl/pull/6278) | API to set/get object IDs for identification and testing |
| **API JSON generator** | [#5677](https://github.com/lvgl/lvgl/pull/5677) | Tool to generate JSON API documentation from headers |

### Platform Support

| Feature | PR | Description |
|---------|----|-------------|
| QNX screen driver | [#6507](https://github.com/lvgl/lvgl/pull/6507) | BlackBerry QNX RTOS display driver |
| MQX OSAL | [#6191](https://github.com/lvgl/lvgl/pull/6191) | NXP MQX RTOS OS abstraction layer |
| Zephyr module | [#6460](https://github.com/lvgl/lvgl/pull/6460) | LVGL as a native Zephyr compatible module |
| NXP PXP Zephyr | [#6298](https://github.com/lvgl/lvgl/pull/6298) | PXP GPU acceleration on Zephyr |
| Renesas GLCDC | [commit](https://github.com/lvgl/lvgl/commit/4d12d64) | Renesas GLCDC display driver with rotation |
| UEFI CI build | [#5964](https://github.com/lvgl/lvgl/pull/5964) | UEFI build verification in CI |
| Windows MSVC/GCC CI | [#6015](https://github.com/lvgl/lvgl/pull/6015) | Windows build in CI |
| Arduino SD driver | [#5968](https://github.com/lvgl/lvgl/pull/5968) | SD card filesystem for Arduino |
| Arduino ESP LittleFS | [#5905](https://github.com/lvgl/lvgl/pull/5905) | ESP32 LittleFS via Arduino framework |
| fbdev/SDL rotation | [#5703](https://github.com/lvgl/lvgl/pull/5703) | Display rotation for fbdev and SDL drivers |
| evdev auto-calibration | [#5989](https://github.com/lvgl/lvgl/pull/5989) | Automatic pointer calibration for evdev |
| Default FS drive letter | [#6367](https://github.com/lvgl/lvgl/pull/6367) | `LV_FS_DEFAULT_DRIVE_LETTER` config + ESP FS docs |
| NuttX lv_nuttx_run | [#6371](https://github.com/lvgl/lvgl/pull/6371) | Simplified NuttX run loop |

### Widget Enhancements

| Feature | PR | Description |
|---------|----|-------------|
| Bar orientation | [#6212](https://github.com/lvgl/lvgl/pull/6212) | Explicit horizontal/vertical orientation for bar widget |
| Scale tick order | [#6185](https://github.com/lvgl/lvgl/pull/6185) | Control tick drawing order (over/under) |
| Scale multiple needles | [#5937](https://github.com/lvgl/lvgl/pull/5937) | Multiple line needles on a single scale widget |
| Textarea editable | [#6467](https://github.com/lvgl/lvgl/pull/6467) | Make textarea editable property controllable |
| Table cell selection | [#6163](https://github.com/lvgl/lvgl/pull/6163) | Programmatic cell selection in table widget |
| Barcode non-tiled | [#6462](https://github.com/lvgl/lvgl/pull/6462) | Non-tiled barcode rendering mode |
| Gridnav single axis | [#6044](https://github.com/lvgl/lvgl/pull/6044) | Constrain grid navigation to single axis |
| Label properties | [#6575](https://github.com/lvgl/lvgl/pull/6575) | Property API support for labels |
| Textarea properties | [commit](https://github.com/lvgl/lvgl/commit/357d5b7) | Property API support for textarea |
| Dropdown properties | [commit](https://github.com/lvgl/lvgl/commit/7c1a8a5) | Property API support for dropdown |
| Keyboard properties | [commit](https://github.com/lvgl/lvgl/commit/cd48c3c) | Property API support for keyboard |
| Roller properties | [commit](https://github.com/lvgl/lvgl/commit/a793178) | Property API support for roller |
| Object properties | [#6537](https://github.com/lvgl/lvgl/pull/6537) | Extended object property API |
| GIF loop control | [#6922](https://github.com/lvgl/lvgl/pull/6922) | Control GIF animation loop count (backported to v9.2.1) |
| Animimg getter | [#6923](https://github.com/lvgl/lvgl/pull/6923) | Get underlying animation from animimg (backported to v9.2.1) |

### Drawing and Rendering

| Feature | PR | Description |
|---------|----|-------------|
| SW radial gradient | [#6170](https://github.com/lvgl/lvgl/pull/6170) | Software-rendered radial gradient backgrounds |
| VG-Lite radial gradient | [#5836](https://github.com/lvgl/lvgl/pull/5836) | GPU-accelerated radial gradients |
| Complex gradients config | lv_conf | `LV_USE_DRAW_SW_COMPLEX_GRADIENTS` enables radial/conic/skew |
| Physical clipping area | [#6703](https://github.com/lvgl/lvgl/pull/6703) | Solves scaling accuracy problems |
| Draw config for code size | [#6313](https://github.com/lvgl/lvgl/pull/6313) | Configuration to reduce compiled code size |
| L8 rotation support | [#6520](https://github.com/lvgl/lvgl/pull/6520) | L8 (luminance) format rotation in SW renderer |
| Image file bitmap masks | [#5911](https://github.com/lvgl/lvgl/pull/5911) | Bitmap masks loadable from files |
| Premultiply image tool | [#6175](https://github.com/lvgl/lvgl/pull/6175) | Premultiplied alpha in image conversion tool |
| SDL all draw task types | [#6437](https://github.com/lvgl/lvgl/pull/6437) | SDL renderer handles all draw operations |
| SDL hardware acceleration | lv_conf | `LV_SDL_ACCELERATED` config option |
| Helium ASM optimization | [#5702](https://github.com/lvgl/lvgl/pull/5702) | Further optimized Helium assembly |
| Custom draw buffer | [#5974](https://github.com/lvgl/lvgl/pull/5974), [#5982](https://github.com/lvgl/lvgl/pull/5982) | User-defined draw buffer instances; separate font draw buf |
| Draw thread stack config | [#5910](https://github.com/lvgl/lvgl/pull/5910) | `LV_DRAW_THREAD_STACK_SIZE` for multi-threaded rendering |
| Draw buf OOP style | [#6427](https://github.com/lvgl/lvgl/pull/6427) | More object-oriented draw buffer API |
| LV_DRAW_BUF_INIT macro | [#6102](https://github.com/lvgl/lvgl/pull/6102) | Static draw buffer initialization with alignment |
| VG-Lite image clip corners | [#6121](https://github.com/lvgl/lvgl/pull/6121) | Image corner clipping via VG-Lite GPU |
| VG-Lite partial border | [#5912](https://github.com/lvgl/lvgl/pull/5912) | Partial border drawing with VG-Lite |

### Core and OS

| Feature | PR | Description |
|---------|----|-------------|
| Recursive mutex default | [#6573](https://github.com/lvgl/lvgl/pull/6573) | OS mutex is recursive by default |
| `lv_obj_null_on_delete` | [#6599](https://github.com/lvgl/lvgl/pull/6599) | Auto-NULL pointer on object deletion |
| `lv_group_get_obj_by_index` | [#6589](https://github.com/lvgl/lvgl/pull/6589) | Get object from group by index |
| `lv_obj_remove_from_subject` | [#6341](https://github.com/lvgl/lvgl/pull/6341) | Remove object from observer subject |
| Long press time setter | [#6664](https://github.com/lvgl/lvgl/pull/6664) | Runtime long press duration config |
| Scroll time/throw setters | [#6723](https://github.com/lvgl/lvgl/pull/6723) | Runtime scroll physics config |
| `lv_anim_speed_to_time` | [#6531](https://github.com/lvgl/lvgl/pull/6531) | Restored utility (was in v8) |
| Anim timeline repeat | [#6127](https://github.com/lvgl/lvgl/pull/6127) | Animation timeline repeat + completed_cb ([#6085](https://github.com/lvgl/lvgl/pull/6085)) |
| FreeRTOS CPU usage | [#6619](https://github.com/lvgl/lvgl/pull/6619) | Better CPU usage measurement for FreeRTOS |
| Log print callback | [#6095](https://github.com/lvgl/lvgl/pull/6095) | `LV_LOG_PRINT_CB` default log callback |
| LV_COLOR_16_SWAP compat | [#6225](https://github.com/lvgl/lvgl/pull/6225) | Backward compatibility for v8 color swap |
| BiDi neutral string | [#6146](https://github.com/lvgl/lvgl/pull/6146) | Set neutral string for bidirectional text |
| `lv_strncat` / `lv_strlcpy` | [#5927](https://github.com/lvgl/lvgl/pull/5927), [#6204](https://github.com/lvgl/lvgl/pull/6204) | Safe string utilities |
| CMake version available | [#6654](https://github.com/lvgl/lvgl/pull/6654) | LVGL version as CMake variables |

### Font

| Feature | PR | Description |
|---------|----|-------------|
| SimSun CJK font | lv_conf | `LV_FONT_SIMSUN_14_CJK` - 1000 most common CJK radicals |
| Font glyph release API | [#5985](https://github.com/lvgl/lvgl/pull/5985) | `lv_font_glyph_release_draw_data` to free glyph data |
| Font glyph refactor | [#5884](https://github.com/lvgl/lvgl/pull/5884) | Refactored glyph data acquisition |
| Tiny TTF cache config | lv_conf | `LV_TINY_TTF_CACHE_GLYPH_CNT` (default 256) |

---

## Performance Improvements

| Optimization | PR | Impact |
|--------------|----|--------|
| Skip empty draw tasks | [#6720](https://github.com/lvgl/lvgl/pull/6720) | Avoid processing no-op draw commands |
| VG-Lite DST_IN rounding | [#6623](https://github.com/lvgl/lvgl/pull/6623) | Faster rounded corner cropping |
| Array push_back / erase | [#6431](https://github.com/lvgl/lvgl/pull/6431), [#6544](https://github.com/lvgl/lvgl/pull/6544) | Core data structure speedup |
| VG-Lite stroke path cache | [#6502](https://github.com/lvgl/lvgl/pull/6502) | Cached stroke paths for reuse |
| QR code drawing speed | [#6475](https://github.com/lvgl/lvgl/pull/6475) | Faster QR code rendering |
| Lottie premultiply removal | [#6358](https://github.com/lvgl/lvgl/pull/6358) | Removed redundant premultiplication |
| VG-Lite font Y-axis inversion | [#6353](https://github.com/lvgl/lvgl/pull/6353) | Pre-inverted coordinates for vector fonts |
| Skip unchanged parent | [#6283](https://github.com/lvgl/lvgl/pull/6283) | Early return when parent unchanged |
| Theme call optimization | [#5971](https://github.com/lvgl/lvgl/pull/5971) | Optimized theme application order |
| Skip border when side=none | [#5959](https://github.com/lvgl/lvgl/pull/5959) | Avoid border draw when not visible |
| Skip area independence test | [#6825](https://github.com/lvgl/lvgl/pull/6825) | v9.2.1: skip test with single draw unit |
| VG-Lite reduce matrix calcs | [#6800](https://github.com/lvgl/lvgl/pull/6800) | v9.2.1: fewer matrix/radius calculations |

---

## Bug Fixes (Key Highlights)

### v9.2.0 Fixes

#### Widget Fixes

| Fix | PR |
|-----|----|
| Spinbox missing value update | [#6719](https://github.com/lvgl/lvgl/pull/6719) |
| Roller: no move with single option | [#6717](https://github.com/lvgl/lvgl/pull/6717) |
| Chart memory leak | [#6727](https://github.com/lvgl/lvgl/pull/6727) |
| Scroll jumping on scroll end | [#6393](https://github.com/lvgl/lvgl/pull/6393) |
| Label: don't break last line for LONG_DOT | [#6362](https://github.com/lvgl/lvgl/pull/6362) |
| IME Pinyin buffer overflow | [#6501](https://github.com/lvgl/lvgl/pull/6501) |
| Display: cancelled screen anim blocks input | [#6277](https://github.com/lvgl/lvgl/pull/6277) |
| Elastic scrolling with snapping | [#6230](https://github.com/lvgl/lvgl/pull/6230) |
| Msgbox auto content height | [#6176](https://github.com/lvgl/lvgl/pull/6176) |
| Textarea password bullets | [#5943](https://github.com/lvgl/lvgl/pull/5943) |
| Flex SPACE_BETWEEN single item | [#5915](https://github.com/lvgl/lvgl/pull/5915) |
| Span Chinese line break | [#6222](https://github.com/lvgl/lvgl/pull/6222) |
| Span height calculation | [#6243](https://github.com/lvgl/lvgl/pull/6243), [#6775](https://github.com/lvgl/lvgl/pull/6775) |
| Grid rounding for fr units | [#6255](https://github.com/lvgl/lvgl/pull/6255) |
| Infinite loop in scroll_end | [#6109](https://github.com/lvgl/lvgl/pull/6109) |
| Scale needle sliding | [#6343](https://github.com/lvgl/lvgl/pull/6343) |

#### Rendering Fixes

| Fix | PR |
|-----|----|
| ARGB8888 rotation artifact | [#6794](https://github.com/lvgl/lvgl/pull/6794) |
| Swapped 90/270 rotation RGB888 | [#6642](https://github.com/lvgl/lvgl/pull/6642) |
| I1 rendering compiler/runtime issues | [#6714](https://github.com/lvgl/lvgl/pull/6714) |
| FreeType outline font cropping | [#6639](https://github.com/lvgl/lvgl/pull/6639) |
| Image inner align behavior | [#6946](https://github.com/lvgl/lvgl/pull/6946) |
| Draw buf negative coordinates | [#6510](https://github.com/lvgl/lvgl/pull/6510) |
| Canvas indexed images | [#6226](https://github.com/lvgl/lvgl/pull/6226) |
| Coordinate percent range > 1000 | [#6051](https://github.com/lvgl/lvgl/pull/6051) |

### v9.2.1 Fixes (Backported)

| Fix | PR |
|-----|----|
| Layout: content width using x alignment | [#6948](https://github.com/lvgl/lvgl/pull/6948) |
| Calendar Chinese compile error | [#6894](https://github.com/lvgl/lvgl/pull/6894) |
| Hovering disabled obj resets indev | [#6855](https://github.com/lvgl/lvgl/pull/6855) |
| GIF decoder bounds check | [#6863](https://github.com/lvgl/lvgl/pull/6863) |
| FreeRTOS sync signal from ISR | [#6793](https://github.com/lvgl/lvgl/pull/6793) |
| Bar bit overflow | [#6841](https://github.com/lvgl/lvgl/pull/6841) |
| Roller string overflow | [#6826](https://github.com/lvgl/lvgl/pull/6826) |
| Arc: ignore hits outside drawn arc | [#6753](https://github.com/lvgl/lvgl/pull/6753) |
| IME crash on long input | [#6767](https://github.com/lvgl/lvgl/pull/6767) |
| Textarea placeholder centering | [#6879](https://github.com/lvgl/lvgl/pull/6879) |
| Dropdown auto-center content | [commit](https://github.com/lvgl/lvgl/commit/e961669) |
| Style: remove transitions on local property set | [commit](https://github.com/lvgl/lvgl/commit/3d33421) |
| `lv_fs_dir_t` added to `lv_fs.h` | [#6943](https://github.com/lvgl/lvgl/pull/6943) |

### v9.2.2 Fixes

| Fix | PR |
|-----|----|
| **Revert Kconfig break** | Reverts [#7131](https://github.com/lvgl/lvgl/pull/7131) that broke Kconfig builds |
| NuttX LCD draw buffer assert | [#7159](https://github.com/lvgl/lvgl/pull/7159) |

---

## ESP32-Specific Notes

| Item | Details |
|------|---------|
| **Arduino ESP LittleFS** | [#5905](https://github.com/lvgl/lvgl/pull/5905) - New `LV_USE_FS_ARDUINO_ESP_LITTLEFS` driver for ESP32 LittleFS via Arduino |
| **Arduino SD** | [#5968](https://github.com/lvgl/lvgl/pull/5968) - New `LV_USE_FS_ARDUINO_SD` for SD card access |
| **Default drive letter** | [#6367](https://github.com/lvgl/lvgl/pull/6367) - `LV_FS_DEFAULT_DRIVE_LETTER` + ESP FS documentation |
| **ESP CMake demos** | [#6220](https://github.com/lvgl/lvgl/pull/6220) - LVGL9 demos added to `esp.cmake` |
| **Arduino example** | [#6001](https://github.com/lvgl/lvgl/pull/6001) - Easier to use `LVGL_Arduino.ino`; millis() as tick ([#5999](https://github.com/lvgl/lvgl/pull/5999)) |
| **FreeRTOS ISR fix** | [#6793](https://github.com/lvgl/lvgl/pull/6793) - v9.2.1 fixed sync signal from ISR |
| **FreeRTOS CPU usage** | [#6619](https://github.com/lvgl/lvgl/pull/6619) - Better CPU measurement for FreeRTOS |
| **Kconfig** | v9.2.1 broke Kconfig; **use v9.2.2** which reverts the bad change |
| **esp_lvgl_port** | Espressif documentation updated to recommend `esp_lvgl_port` component ([#6658](https://github.com/lvgl/lvgl/pull/6658)) |
| **Private API** | If using internal LVGL structs, set `LV_USE_PRIVATE_API 1` in lv_conf.h or menuconfig |

---

## lv_conf.h Changes (v9.1 -> v9.2)

### New Config Options

| Option | Default | Description |
|--------|---------|-------------|
| `LV_STDINT_INCLUDE` | `<stdint.h>` | Configurable standard includes |
| `LV_STDDEF_INCLUDE` | `<stddef.h>` | Configurable standard includes |
| `LV_STDBOOL_INCLUDE` | `<stdbool.h>` | Configurable standard includes |
| `LV_INTTYPES_INCLUDE` | `<inttypes.h>` | Configurable standard includes |
| `LV_LIMITS_INCLUDE` | `<limits.h>` | Configurable standard includes |
| `LV_STDARG_INCLUDE` | `<stdarg.h>` | Configurable standard includes |
| `LV_DRAW_TRANSFORM_USE_MATRIX` | `0` | Enable matrix-based transformations |
| `LV_DRAW_THREAD_STACK_SIZE` | `8 * 1024` | Stack size for draw threads (bytes) |
| `LV_USE_DRAW_SW_COMPLEX_GRADIENTS` | `0` | Enable radial, conic, skew gradients in SW renderer |
| `LV_VG_LITE_STROKE_CACHE_CNT` | `32` | VG-Lite stroke path cache entries |
| `LV_OBJ_ID_AUTO_ASSIGN` | `LV_USE_OBJ_ID` | Auto-assign object IDs |
| `LV_USE_OBJ_PROPERTY_NAME` | `1` | Enable property name resolution |
| `LV_VG_LITE_THORVG_LINEAR_GRADIENT_EXT_SUPPORT` | `0` | Extended gradient support for ThorVG |
| `LV_USE_MATRIX` | `0` | Enable matrix math module |
| `LV_USE_PRIVATE_API` | `0` | **Include lvgl_private.h in lvgl.h** |
| `LV_FONT_SIMSUN_14_CJK` | `0` | Built-in SimSun CJK font |
| `LV_USE_CALENDAR_CHINESE` | `0` | Chinese lunisolar calendar |
| `LV_USE_LOTTIE` | `0` | Lottie animation widget (requires canvas + ThorVG) |
| `LV_FS_DEFAULT_DRIVE_LETTER` | `'\0'` | Default filesystem drive letter |
| `LV_USE_FS_ARDUINO_ESP_LITTLEFS` | `0` | Arduino ESP LittleFS driver |
| `LV_FS_ARDUINO_ESP_LITTLEFS_LETTER` | `'\0'` | Drive letter |
| `LV_USE_FS_ARDUINO_SD` | `0` | Arduino SD card driver |
| `LV_FS_ARDUINO_SD_LETTER` | `'\0'` | Drive letter |
| `LV_TINY_TTF_CACHE_GLYPH_CNT` | `256` | Tiny TTF glyph cache count |
| `LV_SDL_ACCELERATED` | `1` | SDL hardware acceleration |
| `LV_USE_WAYLAND` | `0` | Wayland display driver |
| `LV_WAYLAND_WINDOW_DECORATIONS` | `0` | Client-side decorations for Mutter/GNOME |
| `LV_WAYLAND_WL_SHELL` | `0` | Legacy wl_shell protocol |
| `LV_USE_RENESAS_GLCDC` | `0` | Renesas GLCDC display driver |
| `LV_USE_OPENGLES` | `0` | OpenGL ES driver |
| `LV_USE_OPENGLES_DEBUG` | `1` | OpenGL ES debug mode |
| `LV_USE_QNX` | `0` | QNX screen driver |
| `LV_QNX_BUF_COUNT` | `1` | QNX buffer count (1 or 2) |

### Changed Config Options

| Option | v9.1 | v9.2 | Notes |
|--------|------|------|-------|
| `LV_VG_LITE_GRAD_CACHE_SIZE` | `32` | Renamed to `LV_VG_LITE_GRAD_CACHE_CNT` | Count semantics |
| `LV_USE_OBJ_ID_BUILTIN` | `0` | `1` | Now enabled by default |
| `LV_GIF_CACHE_DECODE_DATA` | top-level | nested under `LV_USE_GIF` | Indentation/scoping change |

---

## Migration Notes

### From v9.1 to v9.2

1. **`lvgl_private.h`** (required if using internal APIs):
   ```c
   /* Option A: Include where needed */
   #include "lvgl_private.h"

   /* Option B: Global config */
   #define LV_USE_PRIVATE_API 1
   ```

2. **Kconfig users**: Skip v9.2.1, use v9.2.2 directly.

3. **VG-Lite gradient config**: Rename `LV_VG_LITE_GRAD_CACHE_SIZE` to `LV_VG_LITE_GRAD_CACHE_CNT`.

4. **Object ID**: `LV_USE_OBJ_ID_BUILTIN` is now `1` by default. Disable if you have a custom ID system.

5. **Standard includes**: New configurable include paths (`LV_STDINT_INCLUDE`, etc.) allow non-standard environments.

### From v9.0 to v9.2

Apply v9.1 migration first (no changes needed), then follow steps above.

### From v8.x to v9.2

Follow the full v9.0 migration guide, then apply steps above.
