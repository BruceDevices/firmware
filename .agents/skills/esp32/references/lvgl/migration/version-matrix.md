# LVGL Version Matrix

> Feature availability, widget support, and API compatibility across LVGL major versions.
> Use this document to determine which version fits your project requirements.

---

## Version Timeline

| Version | Release Date | Status | Maintenance |
|---------|-------------|--------|-------------|
| v7.0.0 | May 2020 | EOL | No |
| v7.9.0 | Jan 2021 | EOL | No |
| v8.0.0 | Jun 2021 | Maintenance only | Security/critical fixes |
| v8.3.0 | Jul 2022 | LTS (last feature release) | Bug fixes |
| v8.3.9 | Aug 2023 | Last v8.3 patch | Bug fixes |
| v8.4.0 | Mar 2024 | Last v8 release | Minimal |
| v9.0.0 | Jan 22, 2024 | Superseded | No |
| v9.1.0 | Mar 20, 2024 | Superseded | No |
| v9.2.0 | Aug 26, 2024 | Superseded | No |
| v9.3.0 | Jun 3, 2025 | Superseded | No |
| v9.4.0 | Oct 16, 2025 | Superseded | No |
| v9.5.0 | Feb 18, 2025 | Current stable | Active |
| v9.6.x | In development | Master/dev | Active |

---

## Feature Availability Matrix

### Core Architecture

| Feature | v7 | v8 | v9 |
|---------|:--:|:--:|:--:|
| Object-oriented widget system | Yes | Yes | Yes |
| Flexbox layout | No | Yes | Yes |
| Grid layout | No | Yes | Yes |
| CSS-like style system | Partial | Yes | Yes |
| Style transitions | Yes | Yes | Yes |
| Anti-aliasing | Yes | Yes | Yes |
| Parallel rendering | No | No | Yes |
| Draw pipeline (task-based) | No | No | Yes |
| GPU draw unit integration | No | Limited | Yes |
| Observer pattern (data binding) | No | No | Yes |
| ThorVG vector graphics | No | No | Yes |
| Runtime color format change | No | No | Yes |
| Multi-display support | Yes | Yes | Yes (improved) |
| Built-in OS abstraction | No | No | Yes |
| Built-in display/touch drivers | No | No | Yes |

### Color and Rendering

| Feature | v7 | v8 | v9 |
|---------|:--:|:--:|:--:|
| RGB565 (16-bit) | Yes | Yes | Yes |
| RGB888 (24-bit) rendering | No | No | Yes |
| ARGB8888 (32-bit) | Yes | Yes | Yes |
| Internal color always RGB888 | No | No | Yes |
| `LV_COLOR_16_SWAP` | Yes | Yes | Removed (manual in flush_cb) |
| Alpha blending | Yes | Yes | Yes (improved) |
| Image color format constants | `LV_IMG_CF_*` | `LV_IMG_CF_*` | `LV_COLOR_FORMAT_*` |

### Configuration System

| Feature | v7 | v8 | v9 |
|---------|:--:|:--:|:--:|
| `lv_conf.h` | Yes | Yes | Yes (restructured) |
| Kconfig support | Partial | Partial | Yes (full CMake) |
| `LV_CONF_INCLUDE_SIMPLE` | Yes | Yes | Yes |
| ESP-IDF menuconfig | Yes (v7.7+) | Yes | Yes |

### Driver Architecture

| Feature | v7 | v8 | v9 |
|---------|:--:|:--:|:--:|
| `lv_disp_drv_t` struct | Yes | Yes | Removed |
| `lv_indev_drv_t` struct | Yes | Yes | Removed |
| `lv_display_t` opaque + setters | No | No | Yes |
| `lv_indev_t` opaque + setters | No | No | Yes |
| `monitor_cb` | Yes | Yes | Removed (use events) |
| `feedback_cb` | Yes | Yes | Removed (use events) |
| Built-in SDL driver | No | No | Yes |
| Built-in Linux FB driver | No | No | Yes |
| Built-in TFT_eSPI wrapper | No | No | Yes |
| Built-in ST7789/ILI9341 | No | No | Yes |
| Buffer size unit | Pixels | Pixels | Bytes |

### Type System

| Type | v7 | v8 | v9 |
|------|:--:|:--:|:--:|
| `lv_coord_t` | `int16_t` | `int16_t`/`int32_t` | Removed (use `int32_t`) |
| `lv_color_t` | Depth-dependent | Depth-dependent | Always RGB888 |
| `lv_res_t` | Yes | Yes | Renamed `lv_result_t` |
| `lv_img_dsc_t` | Yes | Yes | Renamed `lv_image_dsc_t` |

---

## Widget Availability Matrix

| Widget | v7 | v8 | v9 | Notes |
|--------|:--:|:--:|:--:|-------|
| `lv_obj` (base) | Yes | Yes | Yes | |
| `lv_label` | Yes | Yes | Yes | |
| `lv_btn` / `lv_button` | Yes | Yes | Yes | Renamed in v9 |
| `lv_btnmatrix` / `lv_buttonmatrix` | Yes | Yes | Yes | Renamed in v9 |
| `lv_img` / `lv_image` | Yes | Yes | Yes | Renamed in v9; v9 adds align/stretch/tile |
| `lv_imgbtn` / `lv_imagebutton` | Yes | Yes | Yes | Renamed in v9 |
| `lv_slider` | Yes | Yes | Yes | |
| `lv_switch` | Yes | Yes | Yes | v9: property interface |
| `lv_arc` | Yes | Yes | Yes | v9: property interface |
| `lv_bar` | Yes | Yes | Yes | v9: property interface |
| `lv_checkbox` | Yes | Yes | Yes | v9: property interface |
| `lv_dropdown` | Yes | Yes | Yes | |
| `lv_roller` | Yes | Yes | Yes | |
| `lv_textarea` | Yes | Yes | Yes | |
| `lv_table` | Yes | Yes | Yes | v9: property interface |
| `lv_chart` | Yes | Yes | Yes | v9: tick support removed (use `lv_scale`) |
| `lv_keyboard` | Yes | Yes | Yes | |
| `lv_list` | Yes | Yes | Yes | |
| `lv_msgbox` | Yes | Yes | Yes | v9: refactored (regular buttons, not btnmatrix) |
| `lv_tabview` | Yes | Yes | Yes | v9: updated API, property interface |
| `lv_tileview` | Yes | Yes | Yes | v9: renamed tile functions |
| `lv_calendar` | No | Yes | Yes | |
| `lv_colorwheel` | No | Yes | Yes | |
| `lv_led` | Yes | Yes | Yes | v9: property interface |
| `lv_line` | Yes | Yes | Yes | v9: property interface |
| `lv_spinbox` | Yes | Yes | Yes | v9: property interface |
| `lv_spinner` | Yes | Yes | Yes | v9: property interface |
| `lv_span` | No | Yes | Yes | v9: property interface |
| `lv_menu` | No | Yes | Yes | v9: property interface |
| `lv_meter` | No | Yes | **Removed** | Use `lv_scale` in v9 |
| `lv_scale` | No | No | Yes | Replaces `lv_meter` ticks; standalone scale |
| `lv_canvas` | Yes | Yes | Yes | v9: ThorVG vector support |
| `lv_win` | Yes | Yes | Yes | |
| `lv_animimg` | No | Yes | Yes | |

---

## API Compatibility Across Versions

### Breaking Change Severity

| Transition | Severity | Migration Effort |
|-----------|----------|-----------------|
| v7 -> v8 | **Major** | High - style system rewrite, layout system added |
| v8 -> v9 | **Major** | High - driver API rewrite, naming overhaul, widget removal |
| v8.x -> v8.y | Minor | Low - backward compatible within v8 |
| v9.x -> v9.y | Minor | Low - backward compatible within v9 |

### Compatibility Layers

| Layer | Provided | Covers |
|-------|----------|--------|
| v7 -> v8 | `lv_api_map_v7.h` | Basic function renames |
| v8 -> v9 | `lv_api_map_v8.h` | Function renames, constants, typedefs |
| Neither covers | -- | Structural changes (driver init, removed widgets, draw pipeline) |

### API Naming Convention Evolution

| Concept | v7 | v8 | v9 |
|---------|----|----|-----|
| Display | `lv_disp_*` | `lv_disp_*` | `lv_display_*` |
| Screen | `lv_scr_*` | `lv_scr_*` | `lv_screen_*` |
| Button | `lv_btn_*` | `lv_btn_*` | `lv_button_*` |
| Image | `lv_img_*` | `lv_img_*` | `lv_image_*` |
| Delete | `*_del()` | `*_del()` | `*_delete()` |
| Clear flag | N/A | `*_clear_flag()` | `*_remove_flag()` |
| Zoom | `zoom` | `zoom` | `scale` |
| Angle | `angle` | `angle` | `rotation` |
| Active | `*_act()` | `*_act()` | `*_active()` |
| Count | `*_cnt()` | `*_cnt()` | `*_count()` |

---

## Recommended Version by Use Case

### New Projects (Starting Fresh)

| Scenario | Recommended | Rationale |
|----------|-------------|-----------|
| General new project | **v9.5** (latest stable) | Best features, active support |
| Minimum RAM (no PSRAM) | **v8.3** or **v8.4** | Lower memory footprint |
| GPU-accelerated rendering | **v9.5** | Draw unit architecture |
| Vector graphics needed | **v9.5** | ThorVG support |
| Maximum community examples | **v8.3** | Most tutorials written for v8 |
| ESP-IDF component registry | **v9.5** | Listed on ESP Component Registry |
| Arduino ecosystem | **v8.3** or **v9.5** | Both have Arduino support |
| Production stability | **v8.4** or **v9.5** | v8.4 is battle-tested; v9.5 is mature |

### Existing Projects

| Scenario | Recommended | Rationale |
|----------|-------------|-----------|
| Working v8 project, no issues | **Stay on v8.4** | No migration cost |
| Need v9 features (observer, parallel) | **Migrate to v9.5** | Worth the effort |
| v7 project needing updates | **Migrate to v9.5** | Skip v8 entirely |
| Memory-constrained, working v8 | **Stay on v8.3/v8.4** | v9 uses more RAM |

### ESP32 Variant Recommendations

| ESP32 Variant | RAM | PSRAM | Best LVGL Version |
|---------------|-----|-------|--------------------|
| ESP32 (original) | 520KB | Optional 4MB | v8.4 (no PSRAM) / v9.5 (with PSRAM) |
| ESP32-S2 | 320KB | Optional 2MB | v8.4 preferred; v9.5 with PSRAM |
| ESP32-S3 | 512KB | Optional 2-8MB | v9.5 (excellent fit) |
| ESP32-C3 | 400KB | None | v8.4 (safer); v9.5 (tight but possible) |
| ESP32-C6 | 512KB | None | v8.4 or v9.5 (monitor RAM) |
| ESP32-H2 | 320KB | None | v8.4 (v9 too heavy without PSRAM) |
| ESP32-P4 | 768KB | Up to 32MB | v9.5 (ideal target) |

---

## Memory Footprint Comparison

| Metric | v7 | v8 | v9 |
|--------|:--:|:--:|:--:|
| Minimum `LV_MEM_SIZE` (simple GUI) | ~16KB | ~32KB | ~48KB |
| Typical `LV_MEM_SIZE` (moderate GUI) | ~32KB | ~55KB | ~80-110KB |
| Flash (library code, typical config) | ~100KB | ~150KB | ~180KB |
| `lv_color_t` size (RGB565 depth) | 2 bytes | 2 bytes | 3 bytes (always RGB888 internally) |
| Relative RAM to v7 baseline | 1x | ~1.5x | ~2.25x |

**Note**: These are approximate figures. Actual usage depends heavily on enabled features, number of widgets, screen resolution, and buffer configuration.

---

## ESP-IDF Compatibility

| LVGL Version | ESP-IDF 4.4 | ESP-IDF 5.0 | ESP-IDF 5.1 | ESP-IDF 5.2+ |
|-------------|:-----------:|:-----------:|:-----------:|:------------:|
| v7.x | Yes | Untested | Untested | No |
| v8.3 | Yes | Yes | Yes | Yes |
| v8.4 | Yes | Yes | Yes | Yes |
| v9.0-v9.2 | Yes | Yes | Yes | Yes |
| v9.3+ | Partial | Yes | Yes | Yes |
| v9.5 | No | Yes | Yes | Yes |

### Integration Methods

| Method | v8 | v9 | Notes |
|--------|:--:|:--:|-------|
| ESP Component Registry | Yes | Yes | `idf_component.yml` |
| Git submodule | Yes | Yes | Manual integration |
| `esp_lvgl_port` | Yes | Yes | Recommended for ESP-IDF |
| PlatformIO library | Yes | Yes | `platformio.ini` |
| Arduino Library Manager | Yes | Yes | |

---

## Sources

- [LVGL GitHub Releases](https://github.com/lvgl/lvgl/releases)
- [LVGL v9.0 Changelog](https://github.com/lvgl/lvgl/blob/release/v9.0/docs/CHANGELOG.rst)
- [LVGL v8.3 Documentation](https://docs.lvgl.io/8.3/)
- [LVGL v9 Documentation](https://docs.lvgl.io/master/index.html)
- [ESP32 LVGL Version Forum Thread](https://forum.lvgl.io/t/what-version-of-lvgl-is-available-for-esp32/7956)
- [LVGL Memory Usage Forum Thread](https://forum.lvgl.io/t/minimum-ram-memory-usage-v6-v7-v8-v9/17458)
- [LVGL ESP Component Registry](https://components.espressif.com/components/lvgl/lvgl)
- [LVGL v9.5 Release Blog](https://lvgl.io/blog/release-v9-5)
