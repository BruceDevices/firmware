# LVGL v9.3 Reference

## Release Information

| Field | Value |
|---|---|
| Version | v9.3.0 |
| Release Date | 3 June 2025 |
| Support Until | 3 June 2026 |
| Previous Version | v9.2.2 |
| Patch Releases | None (v9.3.0 is the only release in this series) |
| GitHub Compare | [v9.2.2...v9.3.0](https://github.com/lvgl/lvgl/compare/v9.3.0...v9.2.2) |

## Summary of Changes from v9.2

v9.3.0 is a major feature release. The headline addition is **XML declarative UI support** enabling runtime UI loading without firmware recompilation. This release also laid groundwork for **3D texture widgets**, added numerous GPU/display drivers, overhauled text rendering, and introduced multi-touch gestures.

---

## New Features Added Since v9.2

### XML Declarative UI (Major)

- Full XML engine for describing UI declaratively
- Components built from other components and widgets, loaded at runtime
- `<component>` root element in XML without custom C code
- XML property-to-setter mapping via `<api>` and `<prop>` tags
- Support for: styles, constants, views, fonts, images, events, subjects, animations, screens
- Widgets supported in XML: obj, label, button, image, slider, arc, checkbox, switch, dropdown, roller, bar, textarea, keyboard, canvas, calendar, tabview, buttonmatrix, scale, span, spinbox, qrcode
- Data binding: `bind_text`, `bind_*` properties for reactive UI
- Subject scoping (global/local) with float, string, int, toggle events
- Grid and scroll snapping support in XML
- Animation and timeline support in XML (`<include_timeline>`)

### 3D Texture Support (Foundation)

- New `lv_3dtexture` widget and 3D draw task type
- Groundwork for rendering 3D textures as LVGL widgets

### Text Rendering Advances

- FreeType colored glyphs support
- FreeType glyph outline (vector font) support
- FreeType font kerning support
- Text recoloring restored (e.g., `"A #ff0000 red# word"`)
- Simultaneous text selection and recolor
- A1/A2/A4 bitmap GPU rendering
- 8-bit per pixel font bitmap support (`feat(font): support 8 bpp font bitmaps`)
- 3 bpp font rendering support
- Support for vector fonts in SW renderer
- A8 static font direct rendering in SW

### New Color Formats

- `RGB565_SWAPPED` -- standard for SPI-based display controllers
- `ARGB8888_PREMULTIPLIED` -- required for Wayland and Lottie animations
- ARGB1555 on top of RGB565 (DMA2D)
- ARGB1555, ARGB4444, ARGB2222 (VGLite)
- YUY2 color format (VGLite)
- A8, L8 destination buffer support (VGLite)

### Input and Gestures

- Double-click detection
- Triple-click detection
- Multi-touch gestures: swipe, pinch, rotate
- Two-finger swipe gesture support
- Long press repeat time setter (`lv_indev_set_long_press_repeat_time`)
- Rotary event dropdown animations

### Display Features

- Triple buffer support (`lv_display_set_3rd_draw_buffer()`)
- Tiled rendering for multi-core CPU optimization
- Display rotation using transformation matrices (works with FULL render mode)
- Global recolor style property (tint all widgets and images)
- VSync event subscription/unsubscription

### Image Widget Improvements

- `LV_IMAGE_ALIGN_CONTAIN` and `LV_IMAGE_ALIGN_COVER` scaling modes (preserve aspect ratio)
- Symbol images with inner alignment
- Image clip_radius and mask before transformation
- SVG image decoder for image widget

### Widget Improvements

| Widget | Change |
|---|---|
| `lv_chart` | `lv_chart_get_index_from_x()` for scatter charts, `lv_chart_set_cursor_pos_x/y()`, custom `LV_EVENT_REFR_EXT_DRAW_SIZE` |
| `lv_dropdown` | Animation parameter on `lv_dropdown_set_selected()`, rotary event animations |
| `lv_switch` | Vertical switch support |
| `lv_span` | BiDi support, `lv_spangroup_get_span_by_point()`, render performance up 50% |
| `lv_roller` | Set option with a string (`lv_roller_set_str`) |
| `lv_scale` | Additional style properties, id1/id2 for tick line descriptors |
| `lv_slider` | Orientation support, property interface |
| `lv_bar` | Property interface improvements |
| `lv_animimage` | Getter for underlying animation, reversed play order, set source with/without reverse param |
| `lv_file_explorer` | Remove `.` entry, rename `..` to `< Back` |
| `lv_scroll` | User-defined scrollbar length via `LV_STYLE_LENGTH`, adjusted non-elastic behavior |
| `lv_barcode` | Raw Code 128 support |
| `lv_gif` | Loop count control |

### SVG Support (New)

- Full SVG rendering support via ThorVG integration
- SVG image decoder
- API for getting original SVG width and height
- SVG2 special path command 'A' support

### Observer/Subject System Enhancements

- `lv_obj_bind_XXX_ge/gt/le/lt` -- conditional bindings
- Subject `snprintf` formatting
- Notify only when value changes
- Float subject type
- Subject set and increment functions
- `lv_obj_remove_from_subject()` for widget binding
- `lv_subject_t` reduced from 32 to 28 bytes

### Animation Improvements

- `lv_anim_pause()` method added
- VSync mode for animations
- Start callback called on animation restart
- Reverse play clarification in API

### Style System

- `lv_style_set_margin_all()` -- set all margins at once
- Global recolor style property

### Memory and Utility

- `lv_calloc()` function added
- `lv_reallocf()` function added
- `lv_circle_buf_t` circular buffer component
- `lv_iter_t` iterator module
- `lv_cache_lru_ll` LRU cache module
- Dynamic array with second-chance algorithm

### Object System

- Object name support (`lv_obj_set_name`, `lv_obj_find_by_name`)
- Auto-indexing with names like `mybtn_#`
- Transform matrix attribute on objects
- `LV_SIZE_CONTENT` support for min/max width/height
- State trickle down
- Event trickle mechanism (propagate events to children)
- State processing in XML parser

---

## New Drivers and Platform Support

| Driver | Description |
|---|---|
| STM32 DMA2D | GPU acceleration for STM32 (H7RS, L4, U5, N6) |
| STM32 NeoChrom | GPU acceleration via NemaGFX |
| STM32 LTDC | LCD peripheral support with rotation |
| NemaGFX | Generic rendering backend with TSC color formats, vector font support |
| NXP G2D | GPU acceleration |
| UEFI BIOS | Display driver |
| FT81X | Framebuffer driver |
| NXP ELCDIF | Initial display support |
| Wayland dmabuf | With G2D draw unit support |
| Evdev | Auto-discovery with hotplug, multi-touch support |
| Toradex | Board documentation |
| Torizon OS | Integration guide |
| Buildroot | Integration guide |

---

## Architectural Changes

- **Font manager**: Multiple font backend support architecture
- **CMake**: Native Kconfig support integrated
- **Demos**: Reorganized to `lv_demos` repository
- **SW draw unit**: Single unit with multiple threads internally (replaces multiple units)
- **Draw task DSC**: Allocation combined to reduce malloc calls
- **Thread priority**: Configurable for all drawing units

---

## Performance Improvements

| Area | Improvement |
|---|---|
| Span rendering | 50% performance improvement |
| Label rendering | Reduced `lv_text_get_size` calls during drawing |
| VGLite | Gradient cache optimization (LRU-LL), reduced matrix calculations, path data conversion, label drawing efficiency |
| Object rendering | Cached current opacity stack in layer, skip repeated flag setting, reduced border post drawing |
| Draw dispatch | Reduced empty dispatches, skip area independence tests with single draw unit |
| Matrix ops | Reduced matrix conversion and calculations |
| Bin decoder | Improved A8 decoding performance |
| OS API | Optimized calls without OS mode |
| Array module | Inline short functions to reduce jumps |
| Loop unrolling | Fixed blend loop unrolling condition |
| Benchmark | XRGB8888 images with 32-bit color depth |

---

## ESP32 Specific Notes

### Supported Chips

All ESP32 variants via ESP-IDF v4.4+. ESP32-P4 has additional PPA hardware acceleration (experimental in v9.3, refined in v9.4).

### Integration Method

```bash
# Recommended: via esp_lvgl_port
idf.py add-dependency "espressif/esp_lvgl_port^2.3.0"

# Direct LVGL
idf.py add-dependency "lvgl/lvgl^9.3.0"
```

### ESP Component Registry

v9.3.0 is available on the [ESP Component Registry](https://components.espressif.com/components/lvgl/lvgl).

### Configuration

- Access via `idf.py menuconfig` -> Component config -> LVGL configuration
- Per-chip default configs: `sdkconfig.esp32s3`, `sdkconfig.esp32p4`, etc.
- `CONFIG_LV_USE_PPA=y` -- Enable PPA hardware acceleration (ESP32-P4 only, experimental)

### ESP32-P4 PPA (Pixel Processing Accelerator)

- Initial PPA infrastructure added in v9.3 (`feat(draw/ppa): add initial Pixel Processing Accelerator infrastructure for ESP`)
- Rectangle filling acceleration
- Image blending operations (limited gains due to DMA-2D bandwidth constraints)
- Experimental stage in v9.3

### ESP32-S3 Optimizations

- Hand-written assembly routines leveraging Xtensa LX7 SIMD for pixel blending

### File System Support

- Enable `LV_USE_FS_STDIO` for ESP-IDF storage (SPIFFS, SD Card, LittleFS)
- Requires partition table modifications

### Background Operation

LVGL runs in a background task via `esp_lvgl_port` -- no need to call `lv_timer_handler()` manually.

### Makefile/CMake Fixes

- `component.mk` extended with missing elements for ESP-IDF legacy build
- Path fixes in `component.mk`

---

## Bug Fixes (Selected Critical)

| Area | Fix |
|---|---|
| Display | Fixed divide-by-zero on stride, matrix rotation precision loss |
| Calendar | Allow setting years in ascending order, fixed unsigned comparison |
| Label | Long mode clip fix, scroll circular with BIDI, long dot mode with static text |
| Chart | Variable overflow fix, scatter chart last point, divide-by-zero |
| Object | Crash with `LV_SIZE_CONTENT` parent and `%` positioned child |
| Scroll | Flickering during continuous sliding fixed |
| Flex | Min-width, grow, and wrap working together |
| Arc | Handle clicks on full circle |
| Memory | Multiple leak fixes (draw_sw, gif, cache, msgbox, image_decoder) |
| Indev | Gesture occasional crash, scroll momentum decay decoupled from read loop |
| Textarea | Selected text styling in default theme |
| Roller | Don't send click event when scrolled |
| SVG | Multiple rendering fixes, memory leak fixes |
| VGLite | Strict alias warning, path memory reallocation, stroke crash |
| NemaGFX | Bitmap handling, compilation with NEMAVG disabled |
| Wayland | Multiple window support, premultiplied alpha, XDG protocol v2 |

---

## Demos Added

- High Resolution Demo (with WiFi credentials input)
- Smartwatch Demo
- E-Bike Demo
- Looping/infinite scroll examples
- Compass-style rotating scale

---

## Sources

- [LVGL v9.3 Changelog](https://docs.lvgl.io/9.3/CHANGELOG.html)
- [GitHub Release v9.3.0](https://github.com/lvgl/lvgl/releases/tag/v9.3.0)
- [ESP Component Registry](https://components.espressif.com/components/lvgl/lvgl)
- [v9.3 Planning Issue](https://github.com/lvgl/lvgl/issues/6763)
