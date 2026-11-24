# LVGL v8.4 Reference -- Final v8 Series Release

> **Status**: v8.4.0 is the **last release** in the LVGL v8 series. No further v8.x patches will be issued.
> The v8 branch is in **maintenance-only** mode. All new development targets v9.x.

---

## Release History

| Version | Tag | Release Date | Notes |
|---------|-----|-------------|-------|
| **v8.4.0** | `v8.4.0` | 2024-03-19 | Final v8 release. 3 new features, 7 bug fixes |

There are **no patch releases** beyond v8.4.0. The `release/v8.4` branch exists on GitHub but no v8.4.1+ tags were created.

### Previous v8 Minor Releases (for context)

| Series | Final Patch | Patches |
|--------|------------|---------|
| v8.3 | v8.3.11 | 12 releases (8.3.0 -- 8.3.11) |
| v8.2 | v8.2.0 | 1 release |
| v8.1 | v8.1.0 | 1 release |
| v8.0 | v8.0.2 | 3 releases (8.0.0 -- 8.0.2) |

---

## New Features in v8.4.0 (vs v8.3.11)

### 1. PXP GPU Zephyr Support (PR #5838)

Added Zephyr RTOS support for NXP Pixel Pipeline (PXP) hardware acceleration on i.MX RT platforms. This enables hardware-accelerated graphics rendering when running Zephyr on NXP i.MX RT series MCUs.

```c
// PXP acceleration is auto-detected when building for Zephyr on i.MX RT
// No API changes -- existing LV_USE_GPU_NXP_PXP config enables it
#define LV_USE_GPU_NXP_PXP 1
```

### 2. Calendar Custom Year List (PR #5275)

The calendar widget header dropdown now supports custom year ranges, allowing developers to restrict the available year selection.

```c
// Before v8.4: year list was fixed
// v8.4: set a custom year list for the calendar header dropdown
lv_calendar_header_dropdown_create(calendar);
// Custom year range can now be configured
```

### 3. pkg-config File (PR #5067)

A `pkgconfig` file is now generated during the build process, simplifying integration with autotools, Meson, and CMake projects that use `pkg-config` for dependency discovery.

```bash
# Now works after installing LVGL
pkg-config --cflags --libs lvgl
```

---

## Bug Fixes in v8.4.0

| # | Component | Description | PR/Commit |
|---|-----------|-------------|-----------|
| 1 | **Canvas** | Fixed `lv_canvas_transform` when negative `offset_y` parameter is used | PR #5846 |
| 2 | **GPU: ST-DMA2D** | Removed unused functions to reduce code footprint | PR #5561 |
| 3 | **GPU: ARM-2D** | Fixed blending issue in `blend-normal-with-mask-and-opa` mode | PR #5163 |
| 4 | **Screen Loading** | Fixed crash when starting two screen loads with animations simultaneously | PR #5062 |
| 5 | **Chart** | Fixed memory leak in `lv_chart_remove_series()` | PR #5001 |
| 6 | **Snapshot** | Set `data_size` on returned descriptor (was uninitialized) | PR #4972 |
| 7 | **Meter Docs** | Corrected `LV_PART_TICK` to `LV_PART_TICKS` in documentation | commit e277114 |

### Other Changes

| Component | Description | Reference |
|-----------|-------------|-----------|
| **Group** | Avoid null pointer access in group handling | PR #5864 |
| **Code Quality** | Code formatting improvements | commit 8588762 |
| **Docs** | General typo fixes | PR #5502 |
| **Docs** | SJPG decoder: color depth no longer limited to 16 bits | PR #4971 |
| **Docs** | Bidirectional font support documentation updated | PR #5416 |
| **Docs** | Added documentation banner | commit b7a20df |

---

## Performance Characteristics

No explicit performance improvements were made in v8.4.0. Performance characteristics remain identical to v8.3.11.

### v8 Performance Baseline

| Metric | Typical Value |
|--------|--------------|
| Minimum RAM | 16 KB (absolute minimum) |
| Minimum Flash | 64 KB (absolute minimum) |
| Recommended RAM | 48+ KB for practical UIs |
| Recommended Flash | 180+ KB with typical features |
| Draw buffer | 1/10 of screen size minimum recommended |
| Tick resolution | 1--10 ms (`lv_tick_inc()`) |
| Task handler | 5--10 ms recommended interval |

---

## Display Driver Architecture (v8)

The v8 display driver uses a **struct-based registration model**:

```c
// 1. Initialize draw buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[DISP_HOR_RES * 10];
lv_disp_draw_buf_init(&draw_buf, buf1, NULL, DISP_HOR_RES * 10);

// 2. Initialize and configure display driver
static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = DISP_HOR_RES;
disp_drv.ver_res = DISP_VER_RES;
disp_drv.flush_cb = my_flush_cb;
disp_drv.draw_buf = &draw_buf;

// 3. Register
lv_disp_t * disp = lv_disp_drv_register(&disp_drv);
```

### Key Display Driver Structures

| Structure | Purpose |
|-----------|---------|
| `lv_disp_drv_t` | Display driver descriptor (hor_res, ver_res, flush_cb, etc.) |
| `lv_disp_draw_buf_t` | Draw buffer descriptor (buf1, buf2, size) |
| `lv_disp_t` | Registered display handle (returned by register) |
| `lv_disp_rot_t` | Rotation enum (0/90/180/270) |

### Input Device Driver Architecture (v8)

```c
static lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type = LV_INDEV_TYPE_POINTER;
indev_drv.read_cb = my_touchpad_read;
lv_indev_t * indev = lv_indev_drv_register(&indev_drv);
```

### Input Device Types

| Type | Constant |
|------|----------|
| Touchpad/Mouse | `LV_INDEV_TYPE_POINTER` |
| Keypad | `LV_INDEV_TYPE_KEYPAD` |
| Encoder (rotary) | `LV_INDEV_TYPE_ENCODER` |
| Button (external) | `LV_INDEV_TYPE_BUTTON` |

---

## Widget Catalog (v8.4)

### Core Widgets (15)

| Widget | Type Name | Create Function | Description |
|--------|-----------|----------------|-------------|
| Arc | `lv_arc` | `lv_arc_create(parent)` | Circular progress/input |
| Bar | `lv_bar` | `lv_bar_create(parent)` | Linear progress indicator |
| Button | `lv_btn` | `lv_btn_create(parent)` | Clickable button |
| Button Matrix | `lv_btnmatrix` | `lv_btnmatrix_create(parent)` | Grid of buttons |
| Canvas | `lv_canvas` | `lv_canvas_create(parent)` | Freeform drawing surface |
| Checkbox | `lv_checkbox` | `lv_checkbox_create(parent)` | Boolean toggle with label |
| Dropdown | `lv_dropdown` | `lv_dropdown_create(parent)` | Collapsible option list |
| Image | `lv_img` | `lv_img_create(parent)` | Image display |
| Label | `lv_label` | `lv_label_create(parent)` | Text rendering |
| Line | `lv_line` | `lv_line_create(parent)` | Line/polyline drawing |
| Roller | `lv_roller` | `lv_roller_create(parent)` | Scrollable value picker |
| Slider | `lv_slider` | `lv_slider_create(parent)` | Draggable value input |
| Switch | `lv_switch` | `lv_switch_create(parent)` | On/off toggle |
| Table | `lv_table` | `lv_table_create(parent)` | Tabular data display |
| Text Area | `lv_textarea` | `lv_textarea_create(parent)` | Multi-line text input |

### Extra Widgets (12)

| Widget | Type Name | Create Function | Description |
|--------|-----------|----------------|-------------|
| Animation Image | `lv_animimg` | `lv_animimg_create(parent)` | Animated image sequence |
| Calendar | `lv_calendar` | `lv_calendar_create(parent)` | Date picker |
| Chart | `lv_chart` | `lv_chart_create(parent)` | Data visualization |
| Color Wheel | `lv_colorwheel` | `lv_colorwheel_create(parent, knob_recolor)` | HSV color picker |
| Image Button | `lv_imgbtn` | `lv_imgbtn_create(parent)` | Button with image states |
| Keyboard | `lv_keyboard` | `lv_keyboard_create(parent)` | On-screen keyboard |
| LED | `lv_led` | `lv_led_create(parent)` | LED indicator |
| List | `lv_list` | `lv_list_create(parent)` | Scrollable item list |
| Menu | `lv_menu` | `lv_menu_create(parent)` | Hierarchical menu |
| Meter | `lv_meter` | `lv_meter_create(parent)` | Gauge/meter display |
| Message Box | `lv_msgbox` | `lv_msgbox_create(parent, title, txt, btns, add_close_btn)` | Modal dialog |
| Span | `lv_span` | `lv_spangroup_create(parent)` | Rich text spans |
| Spinbox | `lv_spinbox` | `lv_spinbox_create(parent)` | Numeric input with +/- |
| Spinner | `lv_spinner` | `lv_spinner_create(parent, arc_time, arc_length)` | Loading indicator |
| Tabview | `lv_tabview` | `lv_tabview_create(parent, tab_pos, tab_size)` | Tabbed container |
| Tile View | `lv_tileview` | `lv_tileview_create(parent)` | Swipeable tile pages |
| Window | `lv_win` | `lv_win_create(parent, header_height)` | Window with header |

---

## ESP32 Specific Considerations

### Supported ESP32 Variants

| Chip | SRAM | PSRAM Support | Display Interface | Notes |
|------|------|---------------|-------------------|-------|
| ESP32 | 520 KB | Up to 4 MB (SPI) | SPI, I2C, 8-bit parallel | Most common for LVGL projects |
| ESP32-S2 | 320 KB | Up to 2 MB | SPI, I2C, 8-bit parallel | Less SRAM than ESP32 |
| ESP32-S3 | 512 KB | Up to 16 MB (Octal) | SPI, I2C, 8/16-bit parallel, RGB | Best for LVGL with RGB displays |
| ESP32-C3 | 400 KB | None | SPI, I2C | RISC-V, limited display options |

### Memory Configuration for ESP32

```c
// lv_conf.h recommended settings for ESP32
#define LV_MEM_CUSTOM      0           // Use LVGL's built-in memory manager
#define LV_MEM_SIZE         (48U * 1024U)  // 48 KB for LVGL heap (adjust per needs)

// For SPI displays with 16-bit color, byte-swap is often needed
#define LV_COLOR_DEPTH      16
#define LV_COLOR_16_SWAP    1           // Required for most SPI displays

// Performance: place hot functions in IRAM
// In ESP-IDF menuconfig: CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM=y
```

### Display Buffer Strategies

| Strategy | Buffer Size | RAM Cost (320x240 16-bit) | Performance |
|----------|------------|---------------------------|-------------|
| Single small | 1/10 screen | ~15 KB | Acceptable |
| Single full | Full screen | ~150 KB | Good |
| Double small | 2 x 1/10 screen | ~30 KB | Good (DMA overlap) |
| Double full | 2 x full screen | ~300 KB | Best (requires PSRAM) |

### PSRAM Considerations

- LVGL draw buffers can be placed in PSRAM for larger displays, but **PSRAM is NOT accessible by DMA** on most ESP32 variants
- ESP32-S3 with RGB display: use **Bounce Buffer** mode -- CPU copies PSRAM to SRAM, then GDMA transfers to RGB peripheral
- When using Bounce Buffer with PSRAM: set `CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y` to avoid screen drift
- For SPI displays: keep draw buffers in internal SRAM for DMA compatibility

### ESP-IDF Integration

The recommended integration path for v8.4 on ESP-IDF:

```c
// Use esp_lvgl_port component (supports LVGL v8 and v9)
// Install via: idf.py add-dependency "espressif/esp_lvgl_port"

#include "esp_lvgl_port.h"

// Initialize LVGL port
const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
lvgl_port_init(&lvgl_cfg);
```

### PlatformIO Configuration

```ini
; platformio.ini for LVGL v8.4.0
[env:esp32]
platform = espressif32
board = esp32dev
framework = espidf
lib_deps =
    lvgl/lvgl@^8.4.0

; For Arduino framework
[env:esp32-arduino]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    lvgl/lvgl@^8.4.0
```

---

## Layout System

### Flex Layout

```c
// Enable flex layout on a container
lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);          // or COLUMN, ROW_WRAP, etc.
lv_obj_set_flex_align(cont, main_place, cross_place, track_cross_place);

// Make a child fill available space
lv_obj_set_flex_grow(child, 1);
```

| Flow Constant | Direction |
|--------------|-----------|
| `LV_FLEX_FLOW_ROW` | Horizontal left-to-right |
| `LV_FLEX_FLOW_COLUMN` | Vertical top-to-bottom |
| `LV_FLEX_FLOW_ROW_WRAP` | Horizontal with line wrapping |
| `LV_FLEX_FLOW_COLUMN_WRAP` | Vertical with column wrapping |
| `LV_FLEX_FLOW_ROW_REVERSE` | Horizontal right-to-left |
| `LV_FLEX_FLOW_COLUMN_REVERSE` | Vertical bottom-to-top |

| Align Constant | Behavior |
|---------------|----------|
| `LV_FLEX_ALIGN_START` | Pack to start |
| `LV_FLEX_ALIGN_END` | Pack to end |
| `LV_FLEX_ALIGN_CENTER` | Center items |
| `LV_FLEX_ALIGN_SPACE_EVENLY` | Equal space around and between |
| `LV_FLEX_ALIGN_SPACE_AROUND` | Equal space around each item |
| `LV_FLEX_ALIGN_SPACE_BETWEEN` | Equal space between items only |

### Grid Layout

```c
// Define grid tracks
static lv_coord_t col_dsc[] = {70, 70, 70, LV_GRID_TEMPLATE_LAST};
static lv_coord_t row_dsc[] = {50, 50, 50, LV_GRID_TEMPLATE_LAST};

lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);

// Place child in grid cell
lv_obj_set_grid_cell(child,
    LV_GRID_ALIGN_STRETCH, col, col_span,
    LV_GRID_ALIGN_STRETCH, row, row_span);
```

| Grid Align Constant | Behavior |
|--------------------|----------|
| `LV_GRID_ALIGN_START` | Align to track start |
| `LV_GRID_ALIGN_END` | Align to track end |
| `LV_GRID_ALIGN_CENTER` | Center in track |
| `LV_GRID_ALIGN_STRETCH` | Fill track |
| `LV_GRID_ALIGN_SPACE_EVENLY` | Equal space distribution |
| `LV_GRID_ALIGN_SPACE_AROUND` | Space around items |
| `LV_GRID_ALIGN_SPACE_BETWEEN` | Space between items |

---

## GPU Acceleration Support (v8.4)

| GPU Backend | Config Macro | Platforms |
|-------------|-------------|-----------|
| NXP PXP | `LV_USE_GPU_NXP_PXP` | i.MX RT (bare-metal + **Zephyr** in v8.4) |
| NXP VG-Lite | `LV_USE_GPU_NXP_VG_LITE` | i.MX RT with VG-Lite GPU |
| STM32 DMA2D | `LV_USE_GPU_STM32_DMA2D` | STM32 with Chrom-ART |
| ARM-2D | `LV_USE_GPU_ARM2D` | Arm Cortex-M with Helium/MVE |
| SDL GPU | `LV_USE_GPU_SDL` | Desktop simulation via SDL2 |
| SWM341 DMA2D | `LV_USE_GPU_SWM341_DMA2D` | Synwit SWM341 |

---

## What v8.4 Does NOT Have (Added in v9)

This section is critical for understanding the limitations of staying on v8.4 and what v9 brings.

### Architectural Changes in v9

| Feature | v8.4 | v9.x |
|---------|------|------|
| **Display driver model** | Struct-based (`lv_disp_drv_t`) | Function-based (`lv_display_create()`) |
| **Input device model** | Struct-based (`lv_indev_drv_t`) | Function-based (`lv_indev_create()`) |
| **Rendering pipeline** | Single-threaded | Parallel rendering support |
| **Color format** | `LV_IMG_CF_*` constants | `LV_COLOR_FORMAT_*` system |
| **24-bit color** | Not supported | `LV_COLOR_DEPTH 24` (RGB888) |
| **lv_color_t** | Depends on `LV_COLOR_DEPTH` | Always RGB888 internally |
| **Buffer size units** | Pixels | Bytes |
| **Messaging** | `lv_msg` | Observer pattern (`lv_observer`) |

### Widget/API Renames in v9

| v8.4 Name | v9 Name |
|-----------|---------|
| `lv_btn` | `lv_button` |
| `lv_btnmatrix` | `lv_buttonmatrix` |
| `lv_img` | `lv_image` |
| `lv_imgbtn` | `lv_imagebutton` |
| `lv_disp_*` | `lv_display_*` |
| `lv_obj_clear_flag()` | `lv_obj_remove_flag()` |
| `lv_obj_clear_state()` | `lv_obj_remove_state()` |
| `zoom` property | `scale` property |
| `angle` property | `rotation` property |

### New Features in v9 Not Available in v8.4

| Feature | Description |
|---------|-------------|
| **Vector graphics (ThorVG)** | SVG-like vector rendering to canvas |
| **Native blur/drop shadow** | Software blur on all targets without GPU |
| **3D model loading (glTF)** | Load 3D models directly into LVGL UIs |
| **Built-in display drivers** | SDL, Linux framebuffer, TFT_eSPI built into repo |
| **Property interface** | Uniform get/set for widget state (data binding) |
| **lv_scale widget** | Replaces chart tick functionality |
| **Image stretch/tile** | `lv_image` supports align, stretch, tile modes |
| **OpenGL draw backend** | NanoVG-based full GPU rendering |
| **EVE GPU offloading** | Bridgetek EVE chip support via SPI |
| **ESP32-P4 acceleration** | Native hardware acceleration (30% faster) |
| **Display render modes** | PARTIAL, DIRECT, FULL modes explicitly selectable |

### Widgets Removed/Changed in v9

| v8.4 Widget | v9 Status |
|-------------|-----------|
| `lv_meter` | **Removed** -- use `lv_scale` + `lv_arc` |
| `lv_colorwheel` | **Removed** -- use `lv_colorpicker` (v9.2+) |
| Chart ticks | **Removed** -- use `lv_scale` alongside `lv_chart` |
| `lv_win` | Functionality preserved but API changed |

### Memory Impact

v9.x uses approximately **5--6 KB more RAM** than v8.x for equivalent UIs. This matters for constrained ESP32 targets.

---

## Recommended lv_conf.h for ESP32 + v8.4

```c
// Key settings for ESP32 with LVGL v8.4
#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        1       // For SPI displays
#define LV_MEM_SIZE             (48 * 1024)
#define LV_DISP_DEF_REFR_PERIOD 30     // 33 FPS
#define LV_INDEV_DEF_READ_PERIOD 30
#define LV_TICK_CUSTOM          1       // Use ESP timer for tick
#define LV_USE_LOG              0       // Disable in production
#define LV_USE_ASSERT_NULL      1
#define LV_USE_ASSERT_MALLOC    1
#define LV_FONT_MONTSERRAT_14   1       // Default font
#define LV_USE_THEME_DEFAULT    1

// Disable unused features to save flash/RAM
#define LV_USE_ANIMIMG          0
#define LV_USE_COLORWHEEL       0
#define LV_USE_WIN              0

// GPU -- typically none for ESP32
#define LV_USE_GPU_STM32_DMA2D  0
#define LV_USE_GPU_NXP_PXP      0
#define LV_USE_GPU_NXP_VG_LITE  0
#define LV_USE_GPU_SDL          0
```

---

## Source References

- GitHub release: `https://github.com/lvgl/lvgl/releases/tag/v8.4.0`
- Official changelog: `https://docs.lvgl.io/8.4/CHANGELOG.html`
- Blog post: `https://lvgl.io/blog/release-v8-4-0`
- v8.4 docs: `https://docs.lvgl.io/8.4/`
- ESP component: `https://components.espressif.com/components/lvgl/lvgl/versions/8.4.0`
- v9 changes discussion: `https://github.com/lvgl/lvgl/issues/3298`
