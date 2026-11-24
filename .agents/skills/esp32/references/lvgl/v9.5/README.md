# LVGL v9.5 Reference Guide

> Definitive reference for LVGL v9.5.x series. Released February 18, 2026.
> Official docs: https://docs.lvgl.io/9.5/

## Release Timeline

| Version  | Date             | Type    | Notes                                    |
|----------|------------------|---------|------------------------------------------|
| v9.5.0   | 2026-02-18       | Major   | Feature release with 400+ commits        |

Previous minor: v9.4 (2025). Next minor: v9.6 (in development on `master`).

---

## New Features Since v9.4

### Native Blur and Drop Shadow

Software-only blur and drop shadow rendering. No GPU required. Works on all targets including MCUs.

- Frosted-glass effects and background dimming
- Drop shadow with configurable opacity, color, blur radius, and x/y offset
- RGB565 and RGB565_SWAPPED support for blur
- Public `lv_draw_mask_rect_dsc_t` for mask-based rendering

### Bezier Curved Charts

New `LV_CHART_TYPE_CURVE` draws smooth Bezier curves between data points instead of straight line segments. Requires Vector Graphics support enabled.

### LV_STATE_ALT for Dark/Light Mode

New widget state `LV_STATE_ALT` designed for dark/light mode switching. Tag styles with `LV_STATE_ALT` and toggle the entire UI between modes with a single call, replacing the need for two full theme trees.

### Theme Management API

- `lv_obj_remove_theme()` - Cleanly detach a theme from an object
- `lv_obj_bind_style_prop()` - Data-driven style binding for runtime value tracking
- Full theme create/copy/delete API

### Property Interface Expansion

Consistent property get/set API rolled out to these widgets:
- Arc, Bar, Switch, Checkbox, LED, Line
- Scale, Spinbox, Spinner, Table, Tabview
- Buttonmatrix, Span, Menu, Chart

### WebP Image Decoder

Native WebP image decoding alongside existing JPEG, PNG, GIF support.

### Radio Button Support

New `LV_OBJ_FLAG_RADIO_BUTTON` flag and `lv_obj_set_radio_button()` for effortless radio button groups.

---

## Rendering and Graphics

### NanoVG OpenGL Backend

Complete OpenGL draw unit providing GPU-accelerated vector rendering, anti-aliased shapes, gradients, and image compositing. Configure via:

```c
#define LV_USE_DRAW_NANOVG  1
```

Backend selection available for different OpenGL targets. Auto-initialization supported.

### Wayland Driver Rewrite

Complete rewrite of the Wayland display driver:
- SHM backend with double-buffered direct mode rendering
- New EGL backend for full hardware-accelerated OpenGL rendering
- Display rotation support (SHM backend)
- Compositor event handling improvements

### SDL EGL Support

Added EGL rendering capability to the SDL driver for hardware-accelerated rendering.

### Cross-Platform 3D via EGL

EGL support now available across SDL, Wayland, DRM, and GLFW backends. DRM EGL config auto-inference added.

### OpenGL Improvements

- Matrix transformations support
- RGBA-only texture format
- GLSL 330 support
- `glScissor` for opaque fills (performance)
- RGB fallback for desktop GL
- Dynamic video textures in OpenGLES
- GLFW uses GLAD instead of GLEW

---

## 3D Capabilities

### Runtime glTF Manipulation

Read and modify glTF model nodes while the application is running:
- Scaling, rotation, translation
- Animation state accessible at runtime
- Alternate blending mode support
- Material negotiation optimization
- `linear_output` optional flag
- Vertical flip by default fix

### Raycasting Utilities

New API for:
- 3D-to-2D coordinate conversion
- Ray-object intersection calculations
- Interactive 3D scene manipulation

---

## Input Handling Improvements

- **Key remapping at driver level** - Remap keys at the indev level
- **Configurable gesture thresholds** - Per input device gesture threshold API
- **Keypad events without groups** - Keypad events now emit even without assigned groups
- **Keyboard control mode** - Button definitions for keyboard control mode

---

## Performance Improvements

### RISC-V Vector Extension

SIMD acceleration for software rendering on RISC-V vector-capable cores:

```c
#define LV_USE_DRAW_SW_ASM  LV_DRAW_SW_ASM_RISCV_V
```

### Rendering Optimizations

- `glScissor` for opaque fills in OpenGL
- Label `LAYOUT_COMPLETED` moved to display event (reduces overhead)
- VG-Lite bitmap font cache for non-aligned fonts
- Line drawing skips when fewer than 2 points
- glTF material negotiation optimization
- OpenGL shader program destruction delay removed
- Reduced `lv_label_refr_text` calls

### GPU Accelerator Updates

- NemaGFX: Cortex M7 and M55 support
- VG-Lite: Special format support, DST_IN blend emulation, dynamic parameter printing control
- PPA: Burst length configuration (`CONFIG_LV_PPA_BURST_LENGTH`)
- Dave2D: Cache invalidation for Renesas RZA
- DMA2D draw buffer D-Cache flush

---

## Media Support

### GStreamer Enhancement

- Supports video-only sources without audio tracks (camera stream integration)
- End-of-stream events

### FFmpeg Updates

- Player default decoder specification
- FFmpeg decoder setter API

### Image Handling

- JPEG orientation and CMYK support
- GIF stride mode support
- Image AL88 color format transform support
- Image L8 to ARGB8888 blend
- PNG performance measurement details
- Image decoder performance measurement

---

## Bug Fixes (v9.5.0)

### Rendering Fixes
- Blur non-backdrop rendering timing
- Software blur RGB565_SWAPPED support
- Draw line buffer overflow outside display
- Draw line horizontal dash length
- OpenGL ES red/blue channel swap in fills
- OpenGL shader index out of bounds
- OpenGL RGB fallback for desktop GL
- GLES matrix transpose before shader upload
- DRM draw buffer stride
- DRM/EGL RGB565 channel swap

### Widget Fixes
- Arc indicator padding and invalidation
- Arclabel opacity setting, arc length in recolor mode, arc length precision
- Dropdown symbol property behavior
- Textarea scroll on style change
- Grid negative width with column span in RTL
- Object position incorrect x in RTL, alignment RTL switching
- Flex min size when item grows with content parent
- Span undefined `lv_subject_t`

### Driver Fixes
- Wayland compositor event reading and flushing
- Wayland SHM surface damage before return
- Wayland touch driver data setting
- Fbdev display offset and clip area, errno.h include
- Lovyan GFX double rotation in touch
- LTDC rotation warning without DRAW_SW

### 3D / glTF Fixes
- glTF vertical flip by default
- glTF value_changed node attribute reset
- glTF designated initializer warnings
- glTF header C++ extern
- glTF missing INVALID check
- glTF resource cleanup leaks
- glTF heap use-after-free timer deletion
- IBL sampler GL state and resource deletion, fallback improvements

### Memory and System Fixes
- Lodepng cache allocation failure leak
- Animation large duration handling
- Image header cache dump iteration
- Refresh flag when tasks added to screen layers
- Refresh layer property restoration on failure
- Indev reset only when no previous object
- Stdlib mem_add_junk variable name

### Build Fixes
- CMake fatfs private dependencies
- Blur VS compiler error
- Draw helium C declarations include
- SVG decoder stride warning
- Font manager format specifier warning

---

## Breaking Changes

| Change | Migration Path |
|--------|----------------|
| Wayland client-side decorations removed | Use LVGL widgets like `lv_win` for window chrome |
| XML UI engine removed from repository | Development continues outside main LVGL repo |

## Deprecations

| Deprecated | Replacement | Removal Timeline |
|------------|-------------|------------------|
| `lv_fragment` | Plan migration to alternative view lifecycle | Future release |
| `lv_wayland_display_close_cb_t` | Use `LV_EVENT_DELETE` instead | Future release |

---

## ESP32 Specific Notes

No ESP32-exclusive features in v9.5.0, but the following are relevant:

- **PPA (Pixel Processing Accelerator)**: ESP32-P4 PPA burst length configuration via `CONFIG_LV_PPA_BURST_LENGTH` (128/64/32/16/8)
- **PPA Buffer Alignment**: Set `CONFIG_LV_DRAW_BUF_ALIGN=64` to match cache line size when using PPA
- **PSRAM**: v9.5 continues to support PSRAM for draw buffers and direct mode with dual buffering
- **esp_lvgl_port**: Recommended integration component (v2.3.0+) handles FreeRTOS task management and LVGL timer handler
- **Software blur**: Works on ESP32 targets without GPU (new in v9.5)
- **Drop shadow**: Works on all MCU targets including ESP32 (new in v9.5)

---

## Preview/Experimental Features

- **NanoVG rendering backend**: First release, may see API refinements
- **Runtime glTF manipulation**: First release of runtime node modification API
- **Raycasting utilities**: New 3D interaction API
- **LV_STATE_ALT**: New state flag, first release

---

## Compatibility Matrix

| Component          | Minimum Version | Recommended    |
|--------------------|-----------------|----------------|
| ESP-IDF            | v4.4            | v5.x+          |
| esp_lvgl_port      | v2.3.0          | Latest          |
| Arduino IDE        | 1.8.x           | 2.x+           |
| PlatformIO         | 6.x             | Latest          |
| GCC (RISC-V)       | 12+             | 13+ (for V ext) |

---

## Source Links

- Release announcement: https://lvgl.io/blog/release-v9-5
- Documentation: https://docs.lvgl.io/9.5/
- GitHub releases: https://github.com/lvgl/lvgl/releases
- Changelog: https://docs.lvgl.io/9.5/CHANGELOG.html
- v9.5 planning issue: https://github.com/lvgl/lvgl/issues/9254
