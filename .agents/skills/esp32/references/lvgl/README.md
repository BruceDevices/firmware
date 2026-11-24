# LVGL Reference Index

> Top-level index for all LVGL version references, migration guides, and ESP32 compatibility notes.
> Structured for AI agent consumption during ESP32 GUI development tasks.

---

## Directory Structure

```
references/lvgl/
├── README.md                          # This file - top-level index
├── migration/
│   ├── v8-to-v9.md                    # Definitive v8 -> v9 migration guide
│   └── version-matrix.md             # Feature/widget/API matrix across versions
├── v8.2/                              # v8.2 API reference (if populated)
├── v8.3/                              # v8.3 API reference (if populated)
├── v8.4/                              # v8.4 API reference (if populated)
├── v9.0/                              # v9.0 API reference (if populated)
├── v9.1/                              # v9.1 API reference (if populated)
├── v9.2/                              # v9.2 API reference (if populated)
├── v9.3/                              # v9.3 API reference (if populated)
├── v9.4/                              # v9.4 API reference (if populated)
└── v9.5/                              # v9.5 API reference (if populated)
```

---

## Version Selection Guide

### Quick Decision Tree

```
Starting a new ESP32 GUI project?
├── ESP32-S3 or ESP32-P4 (with PSRAM)?
│   └── Use v9.5 (latest stable, full feature set)
├── ESP32 original with PSRAM?
│   └── Use v9.5 (good fit with PSRAM for buffers)
├── ESP32-C3/C6/H2 (no PSRAM)?
│   └── Use v8.4 (lower memory footprint)
├── Need maximum community examples/tutorials?
│   └── Start with v8.3 (most content available)
└── Need GPU acceleration or vector graphics?
    └── Use v9.5 (only option with draw pipeline + ThorVG)

Migrating an existing project?
├── Currently on v7?
│   └── Migrate directly to v9.5 (skip v8)
├── Currently on v8 and working fine?
│   ├── Need v9 features? → Migrate to v9.5
│   └── No new needs? → Stay on v8.4
└── Currently on v9.0-v9.4?
    └── Upgrade to v9.5 (backward compatible within v9)
```

---

## Version Timeline

```
2020    2021        2022        2023        2024                    2025
 |       |           |           |           |                       |
 v7.0   v8.0       v8.3        v8.3.9      v9.0  v9.1  v9.2       v9.5
 May    Jun         Jul         Aug         Jan   Mar   Aug        Feb
        |                                   |                       |
        v7.9                                v8.4                   v9.4
        Jan                                 Mar                    Oct
                                                  |
                                                 v9.3
                                                 Jun
```

### Major Version Eras

| Era | Versions | Period | Key Characteristics |
|-----|----------|--------|---------------------|
| v7 era | 7.0 - 7.9 | May 2020 - Jan 2021 | Legacy style system, basic layout |
| v8 era | 8.0 - 8.4 | Jun 2021 - Mar 2024 | Flexbox/grid, CSS-like styles, mature ecosystem |
| v9 era | 9.0 - 9.5+ | Jan 2024 - present | New driver API, draw pipeline, observer, built-in drivers |

---

## Migration Guides

| Migration Path | Document | Effort Level |
|---------------|----------|-------------|
| v8.x to v9.x | [migration/v8-to-v9.md](migration/v8-to-v9.md) | High |
| Version comparison | [migration/version-matrix.md](migration/version-matrix.md) | Reference |

### Migration Effort Summary

| From | To | Breaking Changes | Compatibility Layer | Estimated Effort |
|------|-----|-----------------|--------------------|-----------------|
| v8.3 | v8.4 | None | N/A | Trivial (recompile) |
| v8.x | v9.0+ | Major | `lv_api_map_v8.h` (partial) | 2-5 days typical |
| v9.x | v9.y | None | N/A | Trivial (recompile) |
| v7.x | v9.x | Extreme | `lv_api_map_v7.h` + `v8.h` | 5-10 days typical |

---

## ESP32 Compatibility Summary

### Recommended LVGL Version by ESP32 Chip

| ESP32 Chip | PSRAM | Recommended LVGL | Rationale |
|-----------|-------|------------------|-----------|
| ESP32 (original) | None | v8.4 | RAM too limited for v9 |
| ESP32 (original) | 4MB+ | v9.5 | PSRAM compensates for v9 RAM usage |
| ESP32-S2 | None | v8.4 | 320KB RAM, too tight for v9 |
| ESP32-S2 | 2MB+ | v9.5 | Viable with PSRAM |
| ESP32-S3 | 2-8MB | v9.5 | Ideal target for v9 |
| ESP32-C3 | None | v8.4 | No PSRAM, 400KB RAM |
| ESP32-C5 | None | v8.4 | Limited RAM, no PSRAM |
| ESP32-C6 | None | v8.4 or v9.5 | 512KB RAM, monitor usage |
| ESP32-H2 | None | v8.4 | 320KB RAM, BLE-focused chip |
| ESP32-P4 | Up to 32MB | v9.5 | Best ESP32 for GUI workloads |

### ESP-IDF Version Compatibility

| LVGL | ESP-IDF 4.4 | ESP-IDF 5.0 | ESP-IDF 5.1 | ESP-IDF 5.2+ |
|------|:-----------:|:-----------:|:-----------:|:------------:|
| v8.3/v8.4 | Yes | Yes | Yes | Yes |
| v9.0-v9.2 | Yes | Yes | Yes | Yes |
| v9.3-v9.5 | No | Yes | Yes | Yes |

### Integration Methods for ESP32

1. **ESP Component Registry** (recommended for ESP-IDF): Add `lvgl/lvgl` to `idf_component.yml`
2. **esp_lvgl_port**: Official ESP-IDF LVGL port with driver integration (supports v8 and v9)
3. **PlatformIO**: Add to `platformio.ini` lib_deps
4. **Arduino Library Manager**: Search "lvgl" in Library Manager
5. **Git submodule**: Manual integration into project

---

## Version Folder Contents

Each version folder (`v8.2/`, `v8.3/`, etc.) is intended to contain:

- API reference summaries for that specific version
- Widget documentation snapshots
- Configuration templates (`lv_conf.h` defaults)
- ESP32-specific notes and tested configurations
- Known issues and workarounds

### Currently Populated

| Folder | Status | Contents |
|--------|--------|----------|
| `v8.2/` | Empty | Placeholder |
| `v8.3/` | Empty | Placeholder |
| `v8.4/` | Empty | Placeholder |
| `v9.0/` | Empty | Placeholder |
| `v9.1/` | Empty | Placeholder |
| `v9.2/` | Empty | Placeholder |
| `v9.3/` | Empty | Placeholder |
| `v9.4/` | Empty | Placeholder |
| `v9.5/` | Empty | Placeholder |

---

## Key Differences Between Major Versions (Summary)

### v8 Highlights

- Flexbox and grid layout support
- CSS-like cascading style system with local styles per widget
- `lv_disp_drv_t` / `lv_indev_drv_t` struct-based driver model
- `lv_meter` widget for gauge displays
- `lv_msg` messaging system
- `lv_coord_t` coordinate type
- Buffer sizes in pixels
- Abbreviated API names (`btn`, `img`, `disp`, `scr`)

### v9 Highlights

- New display/indev API: opaque types with setter functions
- Draw pipeline architecture (task-based, GPU-extensible)
- Parallel rendering support
- Observer pattern replacing `lv_msg`
- Built-in drivers (SDL, Linux FB, TFT_eSPI, ST7789, ILI9341)
- `lv_scale` widget (replaces `lv_meter` ticks)
- ThorVG vector graphics on Canvas
- Full-word API names (`button`, `image`, `display`, `screen`)
- `lv_color_t` always RGB888 internally
- Buffer sizes in bytes
- Kconfig full support
- Built-in OS abstraction (pthread, FreeRTOS)
- Runtime color format adjustment
- `lv_coord_t` removed (use `int32_t`)

---

## External Resources

| Resource | URL | Description |
|----------|-----|-------------|
| LVGL Official Docs (latest) | https://docs.lvgl.io/master/ | v9.6 development docs |
| LVGL v8.3 Docs | https://docs.lvgl.io/8.3/ | Last major v8 docs |
| LVGL GitHub | https://github.com/lvgl/lvgl | Source code and releases |
| LVGL Forum | https://forum.lvgl.io/ | Community support |
| ESP Component Registry | https://components.espressif.com/components/lvgl/lvgl | ESP-IDF integration |
| lv_api_map_v8.h | https://github.com/lvgl/lvgl/blob/master/src/lv_api_map_v8.h | v8 compatibility macros |
| ESP32 LVGL Tips | https://docs.lvgl.io/master/integration/chip_vendors/espressif/tips_and_tricks.html | ESP32-specific optimization |
