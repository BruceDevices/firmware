# LVGL v9.0.x Reference Guide

> Comprehensive reference for LVGL v9.0.x -- the major version jump from v8 to v9.
> Structured for AI agent consumption during ESP32 firmware development.

---

## Release Information

| Version | Release Date | Notes |
|---------|-------------|-------|
| v9.0.0 | 2024-01-22 | Major release. Massive breaking changes from v8. |
| v9.1.0 | 2024-03-25 | First minor release after v9.0.0. |

- **Branch**: `release/v9.0`
- **Repository**: https://github.com/lvgl/lvgl
- **Documentation**: https://docs.lvgl.io/9.0/
- **Changelog**: https://docs.lvgl.io/9.0/CHANGELOG.html

---

## Critical: v8 to v9 Breaking Changes Summary

LVGL v9 is a **complete architectural overhaul**. Nearly every subsystem changed. The migration from v8 to v9 is the largest breaking change in LVGL history. Key areas:

1. Display driver API completely replaced
2. Input device driver API completely replaced
3. `lv_coord_t` removed (replaced by `int32_t`)
4. `lv_color_t` always RGB888 regardless of `LV_COLOR_DEPTH`
5. All abbreviated API names expanded to full words
6. Draw pipeline completely rewritten
7. Image format system replaced
8. Several widgets removed/replaced
9. Observer pattern added (replaces `lv_msg`)
10. `lv_conf.h` restructured significantly

---

## 1. Display Driver Abstraction (CRITICAL CHANGE)

### v8 (Removed)
```c
// v8: Driver struct pattern -- ALL REMOVED IN v9
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t draw_buf_dsc;

lv_disp_draw_buf_init(&draw_buf_dsc, buf1, buf2, buf_size_px);
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = 320;
disp_drv.ver_res = 240;
disp_drv.flush_cb = my_flush_cb;
disp_drv.draw_buf = &draw_buf_dsc;
disp_drv.monitor_cb = my_monitor_cb;
lv_disp_drv_register(&disp_drv);
```

### v9 (New API)
```c
// v9: Object-based creation pattern
lv_display_t *disp = lv_display_create(320, 240);
lv_display_set_flush_cb(disp, my_flush_cb);
lv_display_set_buffers(disp, buf1, buf2, buf_size_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

// Events replace monitor_cb
lv_display_add_event_cb(disp, render_ready_cb, LV_EVENT_RENDER_READY, NULL);
```

### Removed Types
| v8 Type | v9 Replacement |
|---------|---------------|
| `lv_disp_drv_t` | `lv_display_t` (opaque, use setters) |
| `lv_disp_draw_buf_t` | Integrated into `lv_display_set_buffers()` |
| `lv_disp_t` | `lv_display_t` |

### Buffer Size Units Changed
- **v8**: Buffer size in **pixels** (`buf_size_px`)
- **v9**: Buffer size in **bytes** (`buf_size_bytes`)

### Render Modes (v9)
| Mode | Description |
|------|-------------|
| `LV_DISPLAY_RENDER_MODE_PARTIAL` | Small buffers OK, 1/10 screen min recommended |
| `LV_DISPLAY_RENDER_MODE_DIRECT` | Full-screen buffers, auto-sync content |
| `LV_DISPLAY_RENDER_MODE_FULL` | Complete redraw every cycle |

### Display Events (v9)
- `lv_display_add_event_cb()` replaces old callback fields
- `LV_EVENT_RENDER_READY` replaces `monitor_cb`
- `lv_layer_bottom()` replaces `bg_color`/`bg_img` display properties

### Display Color Format (v9)
```c
lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
// LV_COLOR_16_SWAP removed -- use LV_COLOR_FORMAT_NATIVE_REVERSED
// Or call lv_draw_sw_rgb565_swap() in flush_cb
```

### Flush Callback Signature
```c
// v8
void my_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);

// v9
void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
// Note: px_map is uint8_t* not lv_color_t*
```

---

## 2. Input Device Abstraction (CRITICAL CHANGE)

### v8 (Removed)
```c
// v8: Driver struct pattern -- REMOVED
static lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type = LV_INDEV_TYPE_POINTER;
indev_drv.read_cb = my_touchpad_read;
lv_indev_drv_register(&indev_drv);
```

### v9 (New API)
```c
// v9: Object-based creation
lv_indev_t *indev = lv_indev_create();
lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
lv_indev_set_read_cb(indev, my_touchpad_read);
lv_indev_set_display(indev, disp);  // associate with display
```

### Removed Types
| v8 Type | v9 Replacement |
|---------|---------------|
| `lv_indev_drv_t` | `lv_indev_t` (opaque, use setters) |

### Read Callback Signature
```c
// v8
void my_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);

// v9
void my_read_cb(lv_indev_t *indev, lv_indev_data_t *data);
```

### New Input Device Features (v9)
- Key remapping at indev level
- Configurable gesture thresholds
- `LV_OBJ_FLAG_RADIO_BUTTON` for radio group behavior
- Keypad events emitted without assigned group
- Multi-touch gesture recognition (`LV_USE_GESTURE_RECOGNITION`)

---

## 3. Type System Changes (CRITICAL)

### `lv_coord_t` Removed
```c
// v8: lv_coord_t used everywhere
lv_coord_t x = 100;
lv_obj_set_pos(obj, (lv_coord_t)10, (lv_coord_t)20);

// v9: int32_t replaces lv_coord_t
int32_t x = 100;
lv_obj_set_pos(obj, 10, 20);
```

### `lv_color_t` Always RGB888
```c
// v8: lv_color_t depends on LV_COLOR_DEPTH (16-bit, 32-bit, etc.)
// v9: lv_color_t is ALWAYS RGB888 internally, regardless of LV_COLOR_DEPTH
//     LV_COLOR_DEPTH only affects display output format
```

### Color Format Constants
| v8 Constant | v9 Replacement |
|-------------|---------------|
| `LV_IMG_CF_TRUE_COLOR` | `LV_COLOR_FORMAT_NATIVE` |
| `LV_IMG_CF_TRUE_COLOR_ALPHA` | `LV_COLOR_FORMAT_NATIVE_ALPHA` |
| `LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED` | Removed |
| `LV_IMG_CF_ALPHA_1BIT` | Removed (use indexed) |
| `LV_IMG_CF_ALPHA_2BIT` | Removed (use indexed) |
| `LV_IMG_CF_ALPHA_4BIT` | Removed (use indexed) |
| `LV_IMG_CF_INDEXED_1BIT` | `LV_COLOR_FORMAT_I1` |
| `LV_IMG_CF_INDEXED_2BIT` | `LV_COLOR_FORMAT_I2` |
| `LV_IMG_CF_INDEXED_4BIT` | `LV_COLOR_FORMAT_I4` |
| `LV_IMG_CF_INDEXED_8BIT` | `LV_COLOR_FORMAT_I8` |
| `LV_IMG_CF_RAW` | Removed |
| `LV_IMG_CF_RAW_ALPHA` | Removed |
| All `LV_IMG_CF_*` | All `LV_COLOR_FORMAT_*` |

### New Color Formats in v9
- `LV_COLOR_FORMAT_RGB565`
- `LV_COLOR_FORMAT_RGB565_SWAPPED` (for SPI displays)
- `LV_COLOR_FORMAT_RGB888`
- `LV_COLOR_FORMAT_ARGB8888`
- `LV_COLOR_FORMAT_ARGB8888_PREMULTIPLIED`
- `LV_COLOR_FORMAT_XRGB8888`
- `LV_COLOR_FORMAT_NATIVE`
- `LV_COLOR_FORMAT_NATIVE_REVERSED`
- `LV_COLOR_FORMAT_NATIVE_ALPHA`

---

## 4. API Naming Renames (CRITICAL)

### Prefix Changes
| v8 Prefix | v9 Prefix |
|-----------|-----------|
| `lv_disp_*` | `lv_display_*` |
| `lv_btn_*` | `lv_button_*` |
| `lv_btnmatrix_*` | `lv_buttonmatrix_*` |
| `lv_img_*` | `lv_image_*` |
| `lv_imgbtn_*` | `lv_imagebutton_*` |

### Property Name Changes
| v8 Name | v9 Name |
|---------|---------|
| `zoom` | `scale` |
| `angle` | `rotation` |
| `scr` | `screen` |
| `act` | `active` |
| `del` | `delete` |

### Function Renames
| v8 Function | v9 Function |
|-------------|-------------|
| `lv_obj_add_event_cb()` | `lv_obj_add_event()` |
| `lv_obj_clear_flag()` | `lv_obj_remove_flag()` |
| `lv_obj_clear_state()` | `lv_obj_remove_state()` |
| `lv_disp_get_scr_act()` | `lv_display_get_screen_active()` |
| `lv_scr_act()` | `lv_screen_active()` |
| `lv_obj_del()` | `lv_obj_delete()` |
| `lv_obj_del_async()` | `lv_obj_delete_async()` |
| `lv_obj_set_zoom()` | `lv_obj_set_scale()` |
| `lv_img_set_src()` | `lv_image_set_src()` |
| `lv_img_set_zoom()` | `lv_image_set_scale()` |
| `lv_img_set_angle()` | `lv_image_set_rotation()` |

### Style Property Renames
| v8 Property | v9 Property |
|-------------|-------------|
| `lv_style_set_img_opa()` | `lv_style_set_image_opa()` |
| `lv_style_set_img_recolor()` | `lv_style_set_image_recolor()` |
| `transform_zoom` | `transform_scale_x`, `transform_scale_y` |
| `transform_angle` | `transform_rotation` |

---

## 5. Event System Changes

### Callback Signature
```c
// v8
void my_event_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);  // returns lv_obj_t*
}

// v9
void my_event_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target_obj(e);  // use _obj suffix
    // lv_event_get_target(e) returns void* in v9
}
```

### Event Registration
```c
// v8
lv_obj_add_event_cb(obj, my_event_cb, LV_EVENT_CLICKED, user_data);

// v9
lv_obj_add_event_cb(obj, my_event_cb, LV_EVENT_CLICKED, user_data);
// Note: lv_obj_add_event() also works as an alias
```

### Key Event Functions
| Function | Description |
|----------|-------------|
| `lv_event_get_target_obj(e)` | Get widget that triggered event (v9 preferred) |
| `lv_event_get_current_target_obj(e)` | Get widget event was sent to |
| `lv_event_get_user_data(e)` | Get user data from callback |
| `lv_event_get_code(e)` | Get event type code |
| `lv_event_get_param(e)` | Get event-specific parameter |

### New Event Types in v9
- `LV_EVENT_RENDER_READY` (replaces display `monitor_cb`)
- `LV_EVENT_DRAW_TASK_ADDED` (draw pipeline integration)
- `LV_EVENT_DRAW_MAIN` (widget rendering hook)
- `LV_EVENT_DRAW_POST` (post-children rendering)

---

## 6. Observer Pattern (New in v9)

The observer pattern replaces `lv_msg` from v8. Enable with `LV_USE_OBSERVER 1`.

### Subject Types
| Type | Init | Get | Set |
|------|------|-----|-----|
| Integer | `lv_subject_init_int(&subj, val)` | `lv_subject_get_int(&subj)` | `lv_subject_set_int(&subj, val)` |
| Float | `lv_subject_init_float(&subj, val)` | `lv_subject_get_float(&subj)` | `lv_subject_set_float(&subj, val)` |
| String | `lv_subject_init_string(&subj, buf, prev_buf, size, val)` | `lv_subject_get_string(&subj)` | `lv_subject_copy_string(&subj, val)` |
| Pointer | `lv_subject_init_pointer(&subj, val)` | `lv_subject_get_pointer(&subj)` | `lv_subject_set_pointer(&subj, val)` |
| Color | `lv_subject_init_color(&subj, val)` | `lv_subject_get_color(&subj)` | `lv_subject_set_color(&subj, val)` |
| Group | `lv_subject_init_group(&subj, list, cnt)` | `lv_subject_get_group_element(&subj, idx)` | N/A |

### Observer Subscription
```c
// Simple subscription
lv_observer_t *obs = lv_subject_add_observer(&subject, callback, user_data);

// With widget auto-cleanup (observer removed when widget deleted)
lv_subject_add_observer_obj(&subject, callback, widget, user_data);

// Callback signature
void callback(lv_observer_t *observer, lv_subject_t *subject) {
    int32_t val = lv_subject_get_int(subject);
}
```

### Widget Binding
```c
// Bind widget flags to subject conditions
lv_obj_bind_flag_if_eq(obj, &subject, LV_OBJ_FLAG_HIDDEN, ref_value);
lv_obj_bind_flag_if_not_eq(obj, &subject, LV_OBJ_FLAG_HIDDEN, ref_value);

// Bind widget state
lv_obj_bind_state_if_eq(obj, &subject, LV_STATE_CHECKED, ref_value);

// Two-way checked binding
lv_obj_bind_checked(obj, &subject);

// Event-triggered subject changes
lv_obj_add_subject_toggle_event(button, &subject);
lv_obj_add_subject_set_int_event(button, &subject, LV_EVENT_CLICKED, 42);
```

### Previous Value Access
```c
int32_t prev = lv_subject_get_previous_int(&subject);
const char *prev_str = lv_subject_get_previous_string(&subject);
```

---

## 7. Rendering Pipeline (New Architecture)

### Overview
LVGL v9 introduced a completely new draw pipeline with task-based rendering:

1. **Invalidation**: `lv_obj_invalidate()` marks dirty areas
2. **Task Generation**: Draw tasks created for invalidated regions
3. **Dispatching**: Tasks sent to available draw units
4. **Execution**: Units render pixels into buffers
5. **Flushing**: Completed buffers sent to hardware

### Draw Tasks (`lv_draw_task_t`)
- Task types: Fill, Border, Image, Text, Arc, Line, Triangle, Mask
- States: `WAITING` -> `QUEUED` -> `IN_PROGRESS`
- Created by `lv_draw_rect()`, `lv_draw_label()`, `lv_draw_image()`, etc.

### Draw Units (`lv_draw_unit_t`)
Pluggable rendering backends:
- **evaluate_cb**: Can this unit handle the task?
- **dispatch_cb**: Accept and manage execution
- **destroy_cb**: Cleanup

### Software Renderer
- Multi-threading: `LV_DRAW_SW_DRAW_UNIT_CNT` controls thread count
- Complex shapes: `LV_DRAW_SW_COMPLEX` enables rounded corners, shadows
- Fallback for tasks not handled by GPU

### Hardware Accelerators (v9)
| Accelerator | Platform | Config |
|-------------|----------|--------|
| VG-Lite | NXP i.MX | `LV_USE_DRAW_VG_LITE` |
| PXP | NXP i.MX RT | `LV_USE_DRAW_PXP` |
| DMA2D | STM32 | `LV_USE_DRAW_DMA2D` |
| Dave2D | Renesas RA | `LV_USE_DRAW_DAVE2D` |
| NemaGFX | Think Silicon | `LV_USE_NEMA_GFX` |
| PPA | Espressif ESP32-P4 | `LV_USE_PPA` |

### Draw Descriptors
```c
// Rectangle (combines fill, border, shadow, bg image)
lv_draw_rect_dsc_t rect_dsc;
lv_draw_rect_dsc_init(&rect_dsc);
rect_dsc.bg_color = lv_color_hex(0xff0000);
rect_dsc.radius = 10;
lv_draw_rect(layer, &rect_dsc, &area);

// Label
lv_draw_label_dsc_t label_dsc;
lv_draw_label_dsc_init(&label_dsc);
label_dsc.font = &lv_font_montserrat_14;
label_dsc.color = lv_color_white();
label_dsc.text = "Hello";
lv_draw_label(layer, &label_dsc, &area);

// Image
lv_draw_image_dsc_t img_dsc;
lv_draw_image_dsc_init(&img_dsc);
img_dsc.src = &my_image;
lv_draw_image(layer, &img_dsc, &area);
```

### Layer System
- `lv_layer_t` replaces old draw context
- Each layer has its own pixel buffer
- Supports opacity and blending
- Nested layers for hierarchical effects
- `lv_layer_bottom()` for display background

---

## 8. Widget Changes

### Removed Widgets
| v8 Widget | v9 Replacement |
|-----------|---------------|
| `lv_meter` | `lv_scale` + custom drawing |
| Chart ticks | `lv_scale` widget |

### Renamed Widgets
| v8 Name | v9 Name |
|---------|---------|
| `lv_btn` | `lv_button` |
| `lv_btnmatrix` | `lv_buttonmatrix` |
| `lv_img` | `lv_image` |
| `lv_imgbtn` | `lv_imagebutton` |

### New Widgets in v9
- `lv_scale` -- Linear or circular scale with ticks, labels, sections
- `lv_lottie` -- Lottie animation playback (via ThorVG)
- `lv_arc_label` -- Label that follows arc path

### Complete Widget List (v9.0)
`lv_obj` (base), `lv_arc`, `lv_bar`, `lv_button`, `lv_buttonmatrix`,
`lv_calendar`, `lv_canvas`, `lv_chart`, `lv_checkbox`, `lv_dropdown`,
`lv_image`, `lv_imagebutton`, `lv_keyboard`, `lv_label`, `lv_led`,
`lv_line`, `lv_list`, `lv_menu`, `lv_msgbox`, `lv_roller`, `lv_scale`,
`lv_slider`, `lv_spangroup`, `lv_spinbox`, `lv_spinner`, `lv_switch`,
`lv_table`, `lv_tabview`, `lv_textarea`, `lv_tileview`, `lv_win`

### Widget API Convention (v9)
```c
// Create: lv_<widget>_create(parent)
lv_obj_t *btn = lv_button_create(parent);

// Set properties: lv_<widget>_set_<property>(obj, value)
lv_slider_set_value(slider, 50, LV_ANIM_ON);

// Get properties: lv_<widget>_get_<property>(obj)
int32_t val = lv_slider_get_value(slider);
```

### Image Widget Enhancements (v9)
```c
// New: image alignment, stretching, tiling
lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CENTER);
lv_image_set_inner_align(img, LV_IMAGE_ALIGN_STRETCH);
lv_image_set_inner_align(img, LV_IMAGE_ALIGN_TILE);

// Scale and rotation renamed
lv_image_set_scale(img, 256);      // was lv_img_set_zoom()
lv_image_set_rotation(img, 450);   // was lv_img_set_angle()
```

---

## 9. Style System Changes

### Style Properties (v9)

The style system API pattern remains: `lv_style_set_<prop>(&style, value)`.

#### Size and Position
`width`, `min_width`, `max_width`, `height`, `min_height`, `max_height`,
`x`, `y`, `align`, `transform_width`, `transform_height`,
`translate_x`, `translate_y`, `transform_scale_x`, `transform_scale_y`,
`transform_rotation`, `transform_pivot_x`, `transform_pivot_y`

#### Padding and Margin
`pad_top`, `pad_bottom`, `pad_left`, `pad_right`, `pad_row`, `pad_column`,
`margin_top`, `margin_bottom`, `margin_left`, `margin_right`

#### Background
`bg_color`, `bg_opa`, `bg_grad_color`, `bg_grad_dir`,
`bg_main_stop`, `bg_grad_stop`, `bg_grad`,
`bg_image_src`, `bg_image_opa`, `bg_image_recolor`, `bg_image_recolor_opa`, `bg_image_tiled`

#### Border, Outline, Shadow
`border_color`, `border_opa`, `border_width`, `border_side`, `border_post`,
`outline_width`, `outline_color`, `outline_opa`, `outline_pad`,
`shadow_width`, `shadow_offset_x`, `shadow_offset_y`, `shadow_spread`, `shadow_color`, `shadow_opa`

#### Text
`text_color`, `text_opa`, `text_font`, `text_letter_space`, `text_line_space`,
`text_decor`, `text_align`

#### Image, Line, Arc
`image_opa`, `image_recolor`, `image_recolor_opa`,
`line_width`, `line_dash_width`, `line_dash_gap`, `line_rounded`, `line_color`, `line_opa`,
`arc_width`, `arc_rounded`, `arc_color`, `arc_opa`, `arc_image_src`

#### Misc
`radius`, `clip_corner`, `opa`, `color_filter_dsc`, `color_filter_opa`,
`anim`, `anim_time`, `anim_speed`, `transition`, `blend_mode`, `layout`, `base_dir`

### Key Style Renames (v8 -> v9)
| v8 | v9 |
|----|----|
| `lv_style_set_img_opa()` | `lv_style_set_image_opa()` |
| `lv_style_set_img_recolor()` | `lv_style_set_image_recolor()` |
| `lv_style_set_img_recolor_opa()` | `lv_style_set_image_recolor_opa()` |
| `bg_img_src` | `bg_image_src` |
| `bg_img_opa` | `bg_image_opa` |
| `bg_img_recolor` | `bg_image_recolor` |
| `transform_zoom` | `transform_scale_x` / `transform_scale_y` |
| `transform_angle` | `transform_rotation` |

### Observer-Bound Styles (New in v9)
```c
// Disable a style when subject value != ref_value
lv_obj_bind_style(obj, &style, selector, &subject, ref_value);
```

---

## 10. Layout Engine

Flex and Grid layouts remain but with some refinements.

### Flex Layout
```c
lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
lv_obj_set_flex_grow(child, 1);
```

### Grid Layout
```c
static int32_t col_dsc[] = {100, LV_GRID_FR(1), 100, LV_GRID_TEMPLATE_LAST};
static int32_t row_dsc[] = {50, LV_GRID_FR(1), 50, LV_GRID_TEMPLATE_LAST};
lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
lv_obj_set_grid_cell(child, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
```

### Changes from v8
- Grid/Flex descriptors now use `int32_t` instead of `lv_coord_t`
- `LV_GRID_FR()` macro still works

---

## 11. Font System

### Built-in Fonts
Montserrat fonts available: 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48 px.

Enable in `lv_conf.h`:
```c
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_DEFAULT        &lv_font_montserrat_14
```

### Configuration Options
| Option | Description |
|--------|-------------|
| `LV_FONT_FMT_TXT_LARGE` | Support fonts with large character sets |
| `LV_USE_FONT_COMPRESSED` | Compressed font bitmap support |
| `LV_TXT_ENC` | `LV_TXT_ENC_UTF8` or `LV_TXT_ENC_ASCII` |
| `LV_USE_BIDI` | Bidirectional text support |
| `LV_USE_ARABIC_PERSIAN_CHARS` | Arabic/Persian character shaping |

### FreeType Integration
```c
#define LV_USE_FREETYPE 1
// Runtime TTF/OTF font rendering
```

---

## 12. Image Handling Changes

### Image Decoder System (v9)
Pluggable decoders registered at runtime:

| Format | Config | Notes |
|--------|--------|-------|
| Binary (LVGL native) | Built-in | `lv_bin_decoder` |
| PNG | `LV_USE_LIBPNG` or `LV_USE_LODEPNG` | Lossless |
| JPEG | `LV_USE_LIBJPEG_TURBO` or `LV_USE_TJPGD` | Lossy |
| GIF | `LV_USE_GIF` | Animated |
| WebP | `LV_USE_LIBWEBP` | Lossy/lossless |
| FFmpeg | `LV_USE_FFMPEG` | Video frames |

### Image Caching
```c
#define LV_CACHE_DEF_SIZE (64 * 1024)  // Default cache size in bytes
```

### Image Source Declaration (v9)
```c
// Binary images: use LV_IMAGE_DECLARE instead of LV_IMG_DECLARE
LV_IMAGE_DECLARE(my_image);  // was LV_IMG_DECLARE(my_image)

// File path
lv_image_set_src(img, "A:path/to/file.png");  // drive letter prefix
```

### Draw Buffer for Canvas
```c
// v9 canvas uses lv_draw_buf_t
LV_DRAW_BUF_DEFINE(draw_buf, WIDTH, HEIGHT, LV_COLOR_FORMAT_ARGB8888);
lv_canvas_set_draw_buf(canvas, &draw_buf);
```

---

## 13. File System Changes

### v9 File System Drivers
| Driver | Config | Letter |
|--------|--------|--------|
| STDIO | `LV_USE_FS_STDIO` | `LV_FS_STDIO_LETTER` |
| POSIX | `LV_USE_FS_POSIX` | `LV_FS_POSIX_LETTER` |
| WIN32 | `LV_USE_FS_WIN32` | `LV_FS_WIN32_LETTER` |
| FATFS | `LV_USE_FS_FATFS` | `LV_FS_FATFS_LETTER` |
| LittleFS | `LV_USE_FS_LITTLEFS` | `LV_FS_LITTLEFS_LETTER` |

### File Path Format
```c
// Use drive letter prefix
lv_image_set_src(img, "A:images/logo.png");
// Where A is the registered driver letter
```

### Caching
```c
#define LV_FS_STDIO_CACHE_SIZE  0  // Set >0 to enable read caching
```

---

## 14. Memory Management Changes

### Allocator Selection
```c
// lv_conf.h
#define LV_USE_STDLIB_MALLOC  LV_STDLIB_BUILTIN  // or LV_STDLIB_CLIB, LV_STDLIB_CUSTOM
#define LV_USE_STDLIB_STRING  LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
```

| Backend | Constant | Description |
|---------|----------|-------------|
| Built-in TLSF | `LV_STDLIB_BUILTIN` | O(1) allocator, good for embedded |
| C Library | `LV_STDLIB_CLIB` | Standard malloc/free |
| MicroPython | `LV_STDLIB_MICROPYTHON` | MicroPython allocator |
| RT-Thread | `LV_STDLIB_RTTHREAD` | RT-Thread allocator |
| Custom | `LV_STDLIB_CUSTOM` | User-defined functions |

### Memory Pool Configuration
```c
#define LV_MEM_SIZE (64 * 1024)       // Heap size for built-in allocator
#define LV_MEM_ADR  0                  // 0 = use static array, else external SRAM address
#define LV_MEM_POOL_EXPAND_SIZE  0     // Dynamic pool expansion
```

### Memory Functions (v9)
```c
void *lv_malloc(size_t size);
void *lv_realloc(void *ptr, size_t size);
void  lv_free(void *ptr);
void  lv_memset(void *dst, uint8_t val, size_t len);
void  lv_memcpy(void *dst, const void *src, size_t len);

// Diagnostics
lv_mem_monitor_t mon;
lv_mem_monitor(&mon);  // Get heap stats
lv_mem_test();         // Verify heap integrity
```

### Runtime Pool Addition
```c
lv_mem_add_pool(void *mem, size_t size);  // Add external memory to pool
```

---

## 15. OS / Threading Support (New in v9)

### Configuration
```c
#define LV_USE_OS  LV_OS_NONE  // or LV_OS_PTHREAD, LV_OS_FREERTOS, LV_OS_RTTHREAD, etc.
```

### Thread-Safe Rendering
- `LV_DRAW_SW_DRAW_UNIT_CNT` controls parallel software render threads
- Each draw unit can run on its own thread
- Mutex protection for shared resources

### Tick and Timer
```c
// Must provide tick source
lv_tick_inc(tick_period_ms);  // Call periodically (e.g., from timer ISR)
lv_timer_handler();           // Call in main loop
```

---

## 16. lv_conf.h Changes (v8 -> v9)

### Critical Notes
1. **Start fresh**: Copy `lv_conf_template.h` to `lv_conf.h`. Do NOT reuse v8 config.
2. **No `<stdint.h>` in lv_conf.h**: v9 has assembly parts; random includes break them.
3. **Set `#if 1`** at top to enable the config.
4. **Alternative**: Use build flags with `LV_CONF_SKIP` defined.

### New Configuration Sections (v9)
| Section | Key Options |
|---------|------------|
| Stdlib | `LV_USE_STDLIB_MALLOC`, `LV_USE_STDLIB_STRING`, `LV_USE_STDLIB_SPRINTF` |
| OS | `LV_USE_OS`, `LV_DRAW_THREAD_STACK_SIZE`, `LV_DRAW_THREAD_PRIO` |
| Draw Units | `LV_USE_DRAW_SW`, `LV_DRAW_SW_DRAW_UNIT_CNT`, GPU enables |
| Draw Buffers | `LV_DRAW_BUF_STRIDE_ALIGN`, `LV_DRAW_LAYER_SIMPLE_BUF_SIZE` |
| Observer | `LV_USE_OBSERVER` |
| Vector Graphics | `LV_USE_VECTOR_GRAPHIC` (ThorVG) |
| Gesture | `LV_USE_GESTURE_RECOGNITION` |
| Property API | `LV_USE_OBJ_PROPERTY` |
| Float | `LV_USE_FLOAT` |
| Matrix | `LV_USE_MATRIX` |
| Image Decoders | `LV_USE_LIBPNG`, `LV_USE_LIBJPEG_TURBO`, `LV_USE_LODEPNG`, `LV_USE_TJPGD`, `LV_USE_GIF`, `LV_USE_LIBWEBP` |
| File Systems | `LV_USE_FS_STDIO`, `LV_USE_FS_POSIX`, `LV_USE_FS_FATFS`, `LV_USE_FS_LITTLEFS` |

### Removed Configuration Options
| Removed | Replacement |
|---------|-------------|
| `LV_COLOR_16_SWAP` | `lv_display_set_color_format()` with `LV_COLOR_FORMAT_NATIVE_REVERSED` |
| `LV_MEM_CUSTOM` | `LV_USE_STDLIB_MALLOC = LV_STDLIB_CUSTOM` |
| `LV_DISP_DEF_REFR_PERIOD` | `LV_DEF_REFR_PERIOD` |
| `LV_USE_GPU_*` | `LV_USE_DRAW_*` per accelerator |
| `LV_USE_METER` | Removed (use `lv_scale`) |
| `LV_USE_MSG` | Removed (use `LV_USE_OBSERVER`) |

---

## 17. ESP32 Specific Migration Notes

### Recommended Approach: esp_lvgl_port
```bash
# Add to your ESP-IDF project
idf.py add-dependency "espressif/esp_lvgl_port^2.3.0"
```

The `esp_lvgl_port` component:
- Supports LVGL v8 and v9
- Compatible with ESP-IDF v4.4+
- Handles display, touch, encoders, buttons, USB HID
- Manages threading, power saving, screen rotation

### Display Driver Setup (ESP32 + LVGL v9)
```c
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

// 1. Initialize SPI bus + LCD panel (ESP-IDF lcd driver)
// 2. Create LVGL display
lv_display_t *disp = lv_display_create(320, 240);
lv_display_set_flush_cb(disp, esp_lcd_flush_cb);

// Buffer allocation (PSRAM recommended for larger displays)
static uint8_t *buf1 = heap_caps_malloc(320 * 40 * 2, MALLOC_CAP_DMA);
static uint8_t *buf2 = heap_caps_malloc(320 * 40 * 2, MALLOC_CAP_DMA);
lv_display_set_buffers(disp, buf1, buf2, 320 * 40 * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
```

### Touch Panel Setup (ESP32 + LVGL v9)
```c
// I2C touch (e.g., GT911, FT5x06, CST816S)
lv_indev_t *indev = lv_indev_create();
lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
lv_indev_set_read_cb(indev, touch_read_cb);
lv_indev_set_display(indev, disp);
```

### Tick Integration (ESP32)
```c
// Option 1: ESP timer
const esp_timer_create_args_t tick_timer_args = {
    .callback = lv_tick_inc_cb,
    .arg = NULL,
    .name = "lv_tick"
};
esp_timer_handle_t tick_timer;
esp_timer_create(&tick_timer_args, &tick_timer);
esp_timer_start_periodic(tick_timer, 1000);  // 1ms

// Option 2: Use esp_lvgl_port which handles this automatically
```

### Performance Notes (ESP32)
- v9.0.0 reported ~37% performance hit vs v8.3.9 on ESP32-S3 (GitHub issue #5459)
- Later patches and v9.1+ improved performance significantly
- Use `LV_DISPLAY_RENDER_MODE_PARTIAL` with small buffers for memory-constrained ESP32
- Enable DMA buffers with `MALLOC_CAP_DMA` for SPI displays
- Consider `MALLOC_CAP_SPIRAM` (PSRAM) for larger buffers on ESP32-S3/S2
- Set `LV_DRAW_SW_DRAW_UNIT_CNT` to 1 for single-core ESP32 variants

### Common ESP32 Migration Issues
1. `lv_disp_drv_t` and `lv_indev_drv_t` not found -- use new API
2. `lv_coord_t` unknown type -- replace with `int32_t`
3. Buffer size now in bytes, not pixels
4. `lv_color_t` size changed -- check buffer calculations
5. `flush_cb` parameter type changed from `lv_color_t*` to `uint8_t*`
6. Timer handling differences -- ensure `lv_tick_inc()` is called properly

### ESP32 Compatible Display Controllers
- ILI9341, ILI9163C, ILI9486, ILI9488 (SPI)
- ST7789, ST7735, ST7796 (SPI)
- GC9A01 (round displays, SPI)
- SSD1306, SH1106 (OLED, I2C/SPI)

### Adding Display Drivers
```bash
idf.py add-dependency "espressif/esp_lcd_gc9a01^2.0.0"
idf.py add-dependency "espressif/esp_lcd_ili9341"
idf.py add-dependency "espressif/esp_lcd_st7789"
```

---

## 18. Built-in Drivers (New in v9)

LVGL v9 includes built-in driver support:
- **SDL**: Window resize, multiple windows
- **Linux framebuffer**: Direct framebuffer access
- **TFT_eSPI**: Arduino wrapper
- **NuttX**: RTOS display support
- **ST7789, ILI9341**: Direct SPI display controllers

---

## 19. Vector Graphics (New in v9)

ThorVG integration for vector graphics:
```c
#define LV_USE_VECTOR_GRAPHIC 1  // in lv_conf.h
// Draw vector graphics to canvas widget
```

---

## 20. Themes

### Available Themes
| Theme | Config |
|-------|--------|
| Default | `LV_USE_THEME_DEFAULT` |
| Simple | `LV_USE_THEME_SIMPLE` |
| Mono | `LV_USE_THEME_MONO` |

### Dark Mode
```c
#define LV_THEME_DEFAULT_DARK  1  // Enable dark mode
#define LV_THEME_DEFAULT_TRANSITION_TIME  80  // ms
```

---

## Quick Migration Checklist (v8 -> v9)

1. [ ] Copy fresh `lv_conf_template.h` to `lv_conf.h` -- do NOT reuse v8 config
2. [ ] Remove `<stdint.h>` from `lv_conf.h` if present
3. [ ] Replace all `lv_disp_drv_t` / `lv_disp_draw_buf_t` with new display API
4. [ ] Replace all `lv_indev_drv_t` with new input device API
5. [ ] Replace `lv_coord_t` with `int32_t`
6. [ ] Update flush callback: `lv_color_t*` -> `uint8_t*`
7. [ ] Update buffer sizes from pixels to bytes
8. [ ] Rename: `lv_btn_*` -> `lv_button_*`, `lv_img_*` -> `lv_image_*`
9. [ ] Rename: `lv_disp_*` -> `lv_display_*`
10. [ ] Rename: `zoom` -> `scale`, `angle` -> `rotation`
11. [ ] Rename: `lv_obj_clear_flag` -> `lv_obj_remove_flag`
12. [ ] Replace `LV_IMG_CF_*` with `LV_COLOR_FORMAT_*`
13. [ ] Replace `LV_IMG_DECLARE` with `LV_IMAGE_DECLARE`
14. [ ] Replace `lv_meter` with `lv_scale`
15. [ ] Replace `lv_msg` with `lv_observer`
16. [ ] Update event handlers: `lv_event_get_target()` returns `void*`, use `lv_event_get_target_obj()`
17. [ ] Update image converter output for new color format constants
18. [ ] Test thoroughly -- some changes do not produce compiler errors but cause runtime issues
