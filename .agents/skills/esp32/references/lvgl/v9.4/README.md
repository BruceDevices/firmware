# LVGL v9.4 Reference

## Release Information

| Field | Value |
|---|---|
| Version | v9.4.0 |
| Release Date | 16 October 2025 |
| Previous Version | v9.3.0 |
| Patch Releases | None (v9.4.0 is the only release in this series) |
| GitHub Compare | [v9.3.0...v9.4.0](https://github.com/lvgl/lvgl/compare/v9.3.0...v9.4.0) |

## Summary of Changes from v9.3

v9.4.0 focuses on **performance**, **GPU expansion**, and **MPU/Linux platform support**. Major additions include glTF 3D model rendering, GStreamer multimedia, DRM+EGL Linux display driver, ARM NEON optimization, ESP32-P4 PPA hardware acceleration, and an expanded EVE GPU renderer. The XML engine received further refinements for LVGL Pro Editor integration.

---

## New Features Added Since v9.3

### glTF 3D Model Support (Major)

- Load and render glTF 2.0 3D models directly in LVGL UIs
- Requires OpenGL ES 2.0 with extensions
- Works with new EGL display driver
- Configurable default environment image

### GStreamer Multimedia (Major)

- Play videos and multimedia content within LVGL UIs
- Full GStreamer codec support
- Audio-free source support (camera streams)
- Linux-focused feature

### DRM + EGL Display Driver (Major)

- High-performance Linux display rendering
- Native DRM and OpenGL ES (EGL) support
- Required for glTF 3D model rendering
- Display mode selection support
- Correct EGL config matching

### ARM NEON Optimization (Major)

- Up to 33% faster software rendering on NEON-capable platforms
- RGB888 and XRGB8888 support
- RGB565 64-bit blending operations
- Alpha premultiply with MVE and NEON
- Disabled invalid intrinsics on arm32

### EVE GPU Renderer (Major)

- Offload rendering to external EVE chips via SPI
- Asset pre-upload functions
- Fonts with stride alignment
- Bitmaps larger than 511x511
- Buffered writes for performance
- Configurable write buffering (can be disabled)

### XML Engine Enhancements

- Scrollbar mode support
- Spinbox widget support
- QR code support with quiet zone
- Arc rotation parsing
- Grid support and fixes
- `<include_timeline>` support
- Subject toggle events with limits
- Testing support (`click_on`, `set_language` steps)
- Load from directory and blob (FrogFS)
- Remove style and remove_style_all support
- Screen create/load events
- Switch widget support

### New Widget: Arc Label (`lv_arc_label`)

- Text rendered along an arc path
- Recolor support
- Vertical and horizontal alignment
- Text offset and letter spacing
- Counter-clockwise text direction
- Radius percentage mode
- High-accuracy calculation mode

### GPU Driver Improvements

| GPU | Changes in v9.4 |
|---|---|
| ESP PPA | Non-blocking mode, DMA2D burst tuning, 30% faster rendering, 30% lower CPU on ESP32-P4 |
| NemaGFX | Vector draw task support, complex gradients (linear, radial), encoded images |
| VGLite | Unified driver across platforms, HAL support, kernel driver, NXP compatibility, refactored config |
| Dave2D | Block processing performance, remove TODOs, lower CPU on Renesas/Alif |
| DMA2D | STM32N6 clock enable, non-async mode fix |
| G2D | RGB565, PNG, tiled images, rotation support, version <2.3.0 compatibility |
| PXP | Tiled image support |

### Display and Driver Additions

| Driver | Description |
|---|---|
| NV3007 | New display driver with docs |
| Lovyan | Initial integration |
| OpenGL | GLSL version 100 for default shader, texture display from existing texture, performance measurement |
| SDL | Window accessor function, `set_window_size`, UTF-8 keyboard support |
| Wayland | Touchscreen support restored, deprecated WL_SHELL removed, window decorations for dmabuf |

### MPU/Linux Features

- Linux-specific features enabled with `os_none`
- DRM display mode selection
- POSIX profiler porting
- `lv_sleep_ms()` interface

### Translation System (New)

- Full translation support (`feat(translation): add translation support`)
- Language change events
- Label translation tag binding (`lv_label_bind_translation_tag`)

### Chart Improvements

- Stacked chart support
- Cursor removal method (`lv_chart_remove_cursor`)
- Division line count fix (prevent division by zero)

### Observer/Subject Additions

- Subject increment events restored
- `lv_obj_bind_style` -- bind styles to subjects
- Subject set and increment functions
- Float subject events in XML

### Other Notable Features

| Feature | Description |
|---|---|
| `lv_style_merge()` | Merge two styles together |
| `lv_qrcode_set_data()` | Helper function for QR code data |
| `lv_roller_get_option_str()` | Get roller option as string |
| Event delete | Delete display and indev on event |
| Draw unit event_cb | Event callback in draw unit |
| Sysmon CPU usage | CPU proc usage in monitor log mode |
| Display screen loading | Get current screen being loaded |
| API mapping disable | Ability to disable backward API mapping |
| Kconfig verify | Space indentation check |
| `lv_display_set_antialiasing` | Deprecated |
| `lv_tabview_get_tab_button` | Get tab buttons by index |
| `lv_spangroup_bind_span_text` | Bind span text + `set_span_text_fmt` |
| `lv_scale_bind_section_min/max_value` | Bind scale section bounds |
| `lv_image` data binding | Data binding to image src |
| `lv_bar` data binding | Data binding to bar widget |
| `lv_obj` screen create/load event | API and XML support |
| GIF library | 3x faster drop-in replacement |
| FrogFS | Pack directory trees into blobs, load at runtime |

---

## Performance Improvements

| Area | Improvement |
|---|---|
| EVE GPU | Buffered writes optimization |
| EGL | Rendering performance improvement |
| GIF | 3x faster with drop-in replacement library |
| VGLite | GPU fill rendering optimization, memory usage and search speed |
| VGLite | Removed duplicate compilation in tests |
| Snapshot | Render from top object when taking snapshot |
| Text | Reduced glyph function calls |
| Draw dispatch | Only dispatch refreshing display |
| Arc label | Algorithm optimization |
| Test perf | 60 FPS refresh rate |

---

## ESP32 Specific Notes

### ESP32-P4 PPA Hardware Acceleration (Production)

v9.4 is the production release for ESP32-P4 PPA support (experimental in v9.3).

```
# sdkconfig.defaults
CONFIG_LV_USE_PPA=y                    # LVGL-level PPA draw unit
CONFIG_LVGL_PORT_ENABLE_PPA=y          # Display driver hardware rotation/mirroring
```

**Capabilities:**
- Rectangle rendering offloaded to hardware
- Filling tasks offloaded to hardware
- 30% faster render times vs software
- 30% reduced CPU usage
- Non-blocking DMA mode
- Tuned DMA2D burst sizes for PPA performance

**Known Issues (v9.4.0-dev):**
- Tearing artifacts reported when enabling PPA
- Dirty area redraw errors possible
- Queue overflow messages under heavy load

### Per-Chip Configuration

```
sdkconfig.esp32p4    # ESP32-P4 defaults
sdkconfig.esp32s3    # ESP32-S3 defaults
sdkconfig.esp32      # Base ESP32 defaults
```

### ESP-IDF Integration

```bash
idf.py add-dependency "lvgl/lvgl^9.4.0"
idf.py add-dependency "espressif/esp_lvgl_port^2.3.0"
```

### ESP32-S3 Assembly Optimizations

Hand-written Xtensa LX7 SIMD assembly routines for pixel blending continue from v9.3.

---

## Architectural Changes from v9.3

- **Unified VGLite renderer**: Single driver across all platforms, replacing platform-specific implementations
- **Theme re-initialization**: Theme auto-reinitializes on display resolution change
- **Assert handler refactoring**: `LV_ASSERT_HANDLER` handling restructured
- **Cache module**: Made private (`lv_cache` no longer public API)
- **API mapping**: Can now be disabled entirely with config option
- **lv_global_t**: `user_data` moved to top of struct

---

## Bug Fixes (Selected Critical)

| Area | Fix |
|---|---|
| Windows | Window-size calculation corrected |
| DMA2D | Non-async mode compilation fixed |
| Label | Empty translation tags ignored, recolor state preserved across line wrapping, long dot mode with static text |
| Chart | Division-by-zero when div line count is 1 |
| Wayland | Multiple window fixes, driver data binding, draw buffer binding, stride calculation |
| Memory | Leaks in file_explorer, msgbox, opengles shader, image_decoder (multi-core), SVG decoder |
| Indev | Elastic scroll off-by-one, scroll momentum decay decoupled |
| Font | Stride calculation fix |
| Draw | Triangle rendering with overhanging points, sw_mask buffer overflow |
| Flex | RTL alignment in flex layout |
| Textarea | Letter space selection background, cursor scroll on resize |
| Dropdown | Null options handling, button text alignment |
| Table | Free calls to user data removed |
| Tabview | VALUE_CHANGED event on header-button taps |
| Menu | Section as parent of container, bitfield size for Windows |
| OS | `lv_lock`/`lv_unlock` made public |
| Grid | Ignore grids without row/column descriptors, uninitialized var warnings |
| Animation | Play from current progress, crash from delete in callback |
| SVG | Stroke dash style restore, memory leaks, header parsing |
| v9.2 Compat | Included v9.2 API map to avoid breaking changes |

---

## Breaking Changes / Migration Notes

### From v9.3 to v9.4

- `lv_display_set_antialiasing()` is **deprecated**
- WL_SHELL removed from Wayland driver (use XDG shell)
- Cache module (`lv_cache`) is now **private** -- do not use directly
- SimSun font removed, replaced with SourceHanSansSC (already in v9.3)
- `lv_obj_find_by_id()` remains deprecated (use `lv_obj_find_by_name()`)
- v9.2 API mapping included by default to maintain backward compatibility
- API mapping can now be disabled: `LV_USE_API_MAPPING 0`

### Backward Compatibility

v9.4 includes the v9.2 API map by default (`fix: include v9.2 api map to avoid breaking changes`), so code written for v9.2 should generally compile without changes.

---

## Distribution Channels

| Channel | Status |
|---|---|
| Arduino Libraries | Available |
| PlatformIO | Available |
| ESP Component Registry | Available |
| ARM CMSIS-Pack | Available |
| Zephyr | Upcoming |

---

## Sources

- [LVGL v9.4 Changelog](https://docs.lvgl.io/9.4/CHANGELOG.html)
- [LVGL v9.4 Release Announcement](https://forum.lvgl.io/t/lvgl-v9-4-is-released/22502)
- [GitHub Release v9.4.0](https://github.com/lvgl/lvgl/releases/tag/v9.4.0)
- [v9.4 Planning Issue](https://github.com/lvgl/lvgl/issues/8444)
- [ESP32-P4 PPA Documentation](https://docs.lvgl.io/9.5/integration/chip_vendors/espressif/hardware_accelerator_ppa.html)
