# LVGL v9.1.x Reference

> **Baseline**: v9.0.0 | **Target**: v9.1.0
> **Release Date**: March 20, 2024
> **Patch Versions**: v9.1.0 (only release in the v9.1.x series)

---

## Summary of Changes from v9.0

v9.1.0 is a feature release with **no breaking changes** from v9.0.0. It adds crown input support, bitmap masking, ARM Helium acceleration, Espressif FreeRTOS support, LittleFS filesystem driver, ARGB8565 color format, and over 150 bug fixes.

---

## Release Timeline

| Version | Date | Notes |
|---------|------|-------|
| v9.1.0 | 2024-03-20 | Feature release, API compatible with v9.0.0 |

---

## New Features

### Input Enhancements

| Feature | PR | Description |
|---------|----|-------------|
| Crown input support | [#5057](https://github.com/lvgl/lvgl/pull/5057) | Adds rotary crown input (e.g., smartwatch digital crowns) to pointer input device |
| Home and End key mapping | [#5675](https://github.com/lvgl/lvgl/pull/5675) | Keyboard Home/End keys now mapped in LV_KEY system |
| Remove PRESSED state on scroll | [#5660](https://github.com/lvgl/lvgl/pull/5660) | PRESSED visual state is cleared when user starts scrolling |
| X11 LV_KEY additions | [#5704](https://github.com/lvgl/lvgl/pull/5704) | Extended key mappings for the X11 display driver |
| SDL mousewheel crown mode | lv_conf | New `LV_SDL_MOUSEWHEEL_MODE` config for encoder/crown simulation |

### Graphics and Rendering

| Feature | PR | Description |
|---------|----|-------------|
| Bitmap masking | [#5545](https://github.com/lvgl/lvgl/pull/5545) | Pixel-level transparency control for images and layers via bitmap masks |
| ARM Helium acceleration | [#5596](https://github.com/lvgl/lvgl/pull/5596) | Hardware SIMD acceleration for Cortex-M processors with MVE extensions |
| ARGB8565 color format | [#5593](https://github.com/lvgl/lvgl/pull/5593), [#5592](https://github.com/lvgl/lvgl/pull/5592) | New color format combining 8-bit alpha with RGB565 (saves RAM vs ARGB8888) |
| Arc append API | [#5510](https://github.com/lvgl/lvgl/pull/5510) | `lv_vector_path_append_arc()` for vector path construction |
| VG-Lite stroke path | [#5831](https://github.com/lvgl/lvgl/pull/5831) | Stroke rendering support for VG-Lite GPU |
| VG-Lite async rendering | [#5398](https://github.com/lvgl/lvgl/pull/5398) | GPU operations run asynchronously on NXP i.MX RT platforms |
| VG-Lite index format decode | [#5476](https://github.com/lvgl/lvgl/pull/5476) | Indexed image format support for VG-Lite decoder |
| Indexed image draw_buf_copy | [#5686](https://github.com/lvgl/lvgl/pull/5686) | `draw_buf_copy` now supports indexed image formats |
| Font glyph format refactor | [#5540](https://github.com/lvgl/lvgl/pull/5540) | New `lv_font_glyph_format_t` type for draw and font format |

### Platform Support

| Feature | PR | Description |
|---------|----|-------------|
| Espressif FreeRTOS support | [#5862](https://github.com/lvgl/lvgl/pull/5862) | Official support for ESP-IDF's FreeRTOS flavor (critical for ESP32) |
| LittleFS driver | [#5562](https://github.com/lvgl/lvgl/pull/5562) | Filesystem driver for LittleFS (`lv_fs_littlefs`) |
| libinput/xkb Linux driver | [#5486](https://github.com/lvgl/lvgl/pull/5486) | Linux input driver with keyboard layout handling |
| NuttX CPU idle getter | [#5814](https://github.com/lvgl/lvgl/pull/5814) | CPU idle measurement for NuttX RTOS |
| NuttX display driver update | [#5752](https://github.com/lvgl/lvgl/pull/5752) | Updated NuttX display driver |

### Memory and Caching

| Feature | PR | Description |
|---------|----|-------------|
| Cache framework refactor | [#5501](https://github.com/lvgl/lvgl/pull/5501) | Unified cache with new APIs for better resource management |
| Image cache resize | [#5829](https://github.com/lvgl/lvgl/pull/5829) | Runtime image cache size adjustment |
| Image header cache drop | [#5472](https://github.com/lvgl/lvgl/pull/5472) | Separate control over image header cache |
| NuttX independent image cache heap | [#5528](https://github.com/lvgl/lvgl/pull/5528) | Dedicated heap for image caching on NuttX |
| Unified cache entry free callback | [#5612](https://github.com/lvgl/lvgl/pull/5612) | Single callback for cache entry cleanup |
| No-cache decoder flag | [#5688](https://github.com/lvgl/lvgl/pull/5688) | Skip cache entirely when `no_cache` flag is set on decoder |

### Developer Tools

| Feature | PR | Description |
|---------|----|-------------|
| Profiler multithreading | [#5490](https://github.com/lvgl/lvgl/pull/5490) | Built-in profiler works in multi-threaded environments |
| Binary image viewer script | [#5451](https://github.com/lvgl/lvgl/pull/5451) | Python tool to view `.bin` format images |
| Systrace filtering | [#5900](https://github.com/lvgl/lvgl/pull/5900) | Configurable trace log filtering for systrace |
| Max memory usage display | [#5661](https://github.com/lvgl/lvgl/pull/5661) | System monitor shows peak memory usage |
| Screenshot to file | [#5481](https://github.com/lvgl/lvgl/pull/5481) | `lv_display_save_screenshot()` saves display to file |
| Event DSC return | [#5630](https://github.com/lvgl/lvgl/pull/5630) | `lv_obj_add_event_cb` returns event descriptor for later removal |

### Font Rendering

| Feature | PR | Description |
|---------|----|-------------|
| JPEG EXIF parsing | [#5263](https://github.com/lvgl/lvgl/pull/5263) | Automatic EXIF orientation for JPEG images via libjpeg-turbo |
| FreeType italic tilt config | [#5812](https://github.com/lvgl/lvgl/pull/5812) | Configurable tilt angle for FreeType italic rendering |
| FreeType stress tests | [#5828](https://github.com/lvgl/lvgl/pull/5828) | Test suite for FreeType font engine stability |

---

## Performance Improvements

| Optimization | PR | Impact |
|--------------|----|--------|
| RGB565 blending optimization | [#5603](https://github.com/lvgl/lvgl/pull/5603) | Faster color blending on 16-bit displays |
| Simpler layer clear | [#5470](https://github.com/lvgl/lvgl/pull/5470) | Reduced overhead for layer initialization |
| VG-Lite async rendering | [#5398](https://github.com/lvgl/lvgl/pull/5398) | GPU pipeline runs in parallel with CPU on NXP platforms |

---

## Bug Fixes (Key Highlights)

Over 150 fixes. Most impactful categories:

### Widget Fixes

| Fix | PR |
|-----|----|
| Label: consider max-width | [#5644](https://github.com/lvgl/lvgl/pull/5644) |
| Calendar: fix crash when no default set | [#5621](https://github.com/lvgl/lvgl/pull/5621) |
| Msgbox: return footer correctly in `lv_msgbox_get_footer` | [#5804](https://github.com/lvgl/lvgl/pull/5804) |
| Tileview: auto-update position on size change | [#5577](https://github.com/lvgl/lvgl/pull/5577) |
| Chart: set series ID correctly | [#5482](https://github.com/lvgl/lvgl/pull/5482) |
| Bar: mask background on value adjustment | [#5426](https://github.com/lvgl/lvgl/pull/5426) |
| Textarea: accepted chars fix on big endian | [#5479](https://github.com/lvgl/lvgl/pull/5479) |
| Imagebutton: clipped mid area fix | [#5849](https://github.com/lvgl/lvgl/pull/5849) |
| Observer: fixed `lv_subject_remove_all_obj` | [#5464](https://github.com/lvgl/lvgl/pull/5464) |

### Rendering Fixes

| Fix | PR |
|-----|----|
| Non-antialiased RGB565A8 transformation | [#5782](https://github.com/lvgl/lvgl/pull/5782) |
| RGB565 with MULTIPLY blend mode | [#5749](https://github.com/lvgl/lvgl/pull/5749) |
| ARGB8888 buffer clearing in DIRECT mode | [#5741](https://github.com/lvgl/lvgl/pull/5741) |
| Invalidation of scaled areas | [#5548](https://github.com/lvgl/lvgl/pull/5548) |
| Stride consideration in partial update | [#5583](https://github.com/lvgl/lvgl/pull/5583) |
| Cover change with semi-transparent gradients | [#5531](https://github.com/lvgl/lvgl/pull/5531) |

### Memory and Stability

| Fix | PR |
|-----|----|
| TLSF pool > 4 GiB management | [#5720](https://github.com/lvgl/lvgl/pull/5720) |
| RLE buffer overflow prevention | [#5619](https://github.com/lvgl/lvgl/pull/5619) |
| VG-Lite stroke path memory leak | [#5883](https://github.com/lvgl/lvgl/pull/5883) |
| Image decoder not closing | [#5437](https://github.com/lvgl/lvgl/pull/5437) |
| SDl buf memleak on display delete | [#5692](https://github.com/lvgl/lvgl/pull/5692) |
| FreeRTOS stack size calculation | [#5647](https://github.com/lvgl/lvgl/pull/5647) |

### Platform-Specific

| Fix | PR |
|-----|----|
| DRM: default to XRGB8888 framebuffer | [#5851](https://github.com/lvgl/lvgl/pull/5851) |
| Windows: high-res tick + timer for graphics perf | [#5711](https://github.com/lvgl/lvgl/pull/5711) |
| ESP: added `*.cpp` glob in ESP configuration | [#5761](https://github.com/lvgl/lvgl/pull/5761) |
| Arduino: updated example for v9 | [#5499](https://github.com/lvgl/lvgl/pull/5499) |
| Kconfig: skip lv_conf.h by default | [#5617](https://github.com/lvgl/lvgl/pull/5617) |

---

## ESP32-Specific Notes

| Item | Details |
|------|---------|
| **FreeRTOS support** | [#5862](https://github.com/lvgl/lvgl/pull/5862) - Official support for Espressif's FreeRTOS flavor, critical for ESP-IDF integration |
| **ESP CMake** | `*.cpp` glob added to ESP configuration ([#5761](https://github.com/lvgl/lvgl/pull/5761)) |
| **Kconfig** | Kconfig updated to match lv_conf_template.h ([#5780](https://github.com/lvgl/lvgl/pull/5780)); lv_conf.h skipped by default when Kconfig used ([#5617](https://github.com/lvgl/lvgl/pull/5617)) |
| **LittleFS** | New `LV_USE_FS_LITTLEFS` driver available - relevant for ESP32 SPIFFS/LittleFS partitions |
| **Arduino** | Updated Arduino example for v9 ([#5499](https://github.com/lvgl/lvgl/pull/5499)) |
| **Stack size** | Fixed FreeRTOS stack size calculation ([#5647](https://github.com/lvgl/lvgl/pull/5647)) |
| **Color depth** | LV_COLOR_DEPTH 8 not yet supported (documented in changelog) |

---

## lv_conf.h Changes (v9.0 -> v9.1)

### New Config Options

| Option | Default | Description |
|--------|---------|-------------|
| `LV_DRAW_LAYER_SIMPLE_BUF_SIZE` | `24 * 1024` | Renamed from `LV_DRAW_SW_LAYER_SIMPLE_BUF_SIZE` |
| `LV_USE_NATIVE_HELIUM_ASM` | `0` | Enable ARM Helium (MVE) SIMD acceleration |
| `LV_VG_LITE_FLUSH_MAX_COUNT` | `8` | Max VG-Lite flush command count |
| `LV_VG_LITE_USE_BOX_SHADOW` | `0` | VG-Lite box shadow support |
| `LV_VG_LITE_GRAD_CACHE_SIZE` | `32` | Gradient cache size for VG-Lite |
| `LV_VG_LITE_THORVG_BUF_ADDR_ALIGN` | `64` | Buffer address alignment for ThorVG VG-Lite |
| `LV_USE_FS_LITTLEFS` | `0` | Enable LittleFS filesystem driver |
| `LV_FS_LITTLEFS_LETTER` | `'\0'` | Drive letter for LittleFS |
| `LV_SDL_MOUSEWHEEL_MODE` | `LV_SDL_MOUSEWHEEL_MODE_ENCODER` | SDL mousewheel behavior (encoder or crown) |
| `LV_USE_LIBINPUT` | `0` | Linux libinput driver |
| `LV_LIBINPUT_BSD` | `0` | BSD-specific libinput |
| `LV_LIBINPUT_XKB` | `0` | XKB keyboard layout support |

### Removed Config Options

| Option | Notes |
|--------|-------|
| `LV_DRAW_SW_LAYER_SIMPLE_BUF_SIZE` | Renamed to `LV_DRAW_LAYER_SIMPLE_BUF_SIZE` |
| `LV_FREETYPE_CACHE_SIZE` | Removed (FreeType cache reworked) |
| `LV_FREETYPE_CACHE_FT_FACES` | Removed |
| `LV_FREETYPE_CACHE_FT_SIZES` | Removed |
| `LV_USE_LZ4` | Removed |
| `LV_DEMO_WIDGETS_SLIDESHOW` | Removed from demo config |

### Renamed Config Options

| Old Name (v9.0) | New Name (v9.1) |
|------------------|-----------------|
| `LV_DRAW_SW_LAYER_SIMPLE_BUF_SIZE` | `LV_DRAW_LAYER_SIMPLE_BUF_SIZE` |

---

## Breaking Changes

**None.** Full backward API compatibility with v9.0.0 is maintained. No code changes required when upgrading from v9.0.x to v9.1.0.

---

## Migration Notes

- **From v9.0.x**: Drop-in upgrade. Update `lv_conf.h` to use new option names if desired (old names may still work via compatibility defines).
- **From v8.x**: Follow the v9.0 migration guide first; v9.1 adds no additional migration steps.
- **ESP32 users**: Enable `LV_USE_FREERTOS` and ensure Espressif FreeRTOS flavor is detected automatically. The stack size fix in [#5647](https://github.com/lvgl/lvgl/pull/5647) is important for constrained devices.
