# LVGL v9.3 API Reference -- New and Changed APIs

This document covers APIs that are **new or changed in v9.3.0** compared to v9.2.x. For the full API, see [docs.lvgl.io/9.3](https://docs.lvgl.io/9.3/).

---

## XML Declarative UI API (New)

The entire XML subsystem is new in v9.3. Enable with `LV_USE_XML`.

### Core XML Functions

```c
/* Load a component library from a directory */
void lv_xml_component_register_from_dir(const char * path);

/* Create a widget/component from XML string */
lv_obj_t * lv_xml_create(lv_obj_t * parent, const char * name, const char ** attrs);

/* Set default asset path for fonts/images referenced in XML */
void lv_xml_set_default_asset_path(const char * path);

/* Load UI from a directory structure */
void lv_xml_load_from_dir(const char * path);

/* Load UI from a blob (FrogFS, embedded data) */
void lv_xml_load_from_blob(const void * data, size_t size);
```

### XML Component Pattern

Components defined in XML generate creation functions:

```c
/* Auto-generated from <component name="my_component"> */
lv_obj_t * my_component_create(lv_obj_t * parent, ...);
```

### XML Property Mapping

Each `<prop>` in the XML API maps to a widget setter:

```xml
<api>
  <prop name="text" type="string" />
  <prop name="value" type="int" />
</api>
```

Maps to:
```c
widget_set_text(obj, "hello");
widget_set_value(obj, 42);
```

### Element Access Patterns

```c
/* access="add" -- dynamic sub-elements */
widget_add_element(lv_obj_t * parent, ...);
widget_set_element_property(lv_obj_t * parent, ...);

/* access="get" -- implicit internal elements */
lv_obj_t * widget_get_element(lv_obj_t * parent, int32_t index);

/* access="set" -- indexed element properties */
widget_set_element_property(lv_obj_t * parent, int32_t index, ...);
```

---

## Display API Changes

### Triple Buffer Support (New)

```c
/* Add a third draw buffer for triple buffering */
void lv_display_set_3rd_draw_buffer(lv_display_t * disp, void * buf, uint32_t buf_size);
```

### Rotation with Transform Matrix (Changed)

```c
/* Now works with FULL render mode (not just PARTIAL) */
void lv_display_set_rotation(lv_display_t * disp, lv_display_rotation_t rotation);
/* Values: LV_DISPLAY_ROTATION_0, _90, _180, _270 */
```

### Draw Buffer Getter (New)

```c
uint32_t lv_display_get_draw_buf_size(lv_display_t * disp);
```

### VSync Events (New)

```c
void lv_display_add_vsync_event_cb(lv_display_t * disp, lv_event_cb_t cb, void * user_data);
void lv_display_remove_vsync_event_cb(lv_display_t * disp, lv_event_cb_t cb);
```

---

## Object API Changes

### Object Name (New)

```c
void lv_obj_set_name(lv_obj_t * obj, const char * name);
const char * lv_obj_get_name(lv_obj_t * obj);
lv_obj_t * lv_obj_find_by_name(lv_obj_t * parent, const char * name);

/* Note: lv_obj_find_by_id is DEPRECATED in favor of lv_obj_find_by_name */
```

### Transform Matrix (New)

```c
void lv_obj_set_transform_matrix(lv_obj_t * obj, const lv_matrix_t * matrix);
```

### LV_SIZE_CONTENT for Min/Max (New)

```c
/* LV_SIZE_CONTENT can now be used with min and max width/height styles */
lv_obj_set_style_min_width(obj, LV_SIZE_CONTENT, LV_PART_MAIN);
lv_obj_set_style_max_height(obj, LV_SIZE_CONTENT, LV_PART_MAIN);
```

### State Trickle Down (New)

```c
/* States now propagate to children automatically when configured */
lv_obj_add_state(obj, LV_STATE_DISABLED);  /* Children inherit state */
```

### Event Trickle (New)

```c
/* Events can propagate to children */
lv_obj_add_event_cb(obj, cb, LV_EVENT_xxx, user_data);
/* Events marked with trickle flag propagate downward */
```

---

## Image API Changes

### New Alignment Modes (New)

```c
lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CONTAIN);  /* Scale to fit, maintain aspect */
lv_image_set_inner_align(img, LV_IMAGE_ALIGN_COVER);    /* Scale to fill, maintain aspect */
```

### SVG API (New)

```c
/* Get original SVG dimensions */
lv_result_t lv_svg_get_width(const void * svg_data, int32_t * width);
lv_result_t lv_svg_get_height(const void * svg_data, int32_t * height);
```

---

## Input Device API Changes

### Click Detection (New Events)

```c
/* New event types */
LV_EVENT_DOUBLE_CLICKED   /* Fired on double-click */
LV_EVENT_TRIPLE_CLICKED   /* Fired on triple-click */
```

### Multi-Touch Gestures (New)

```c
/* New gesture types detected by indev */
LV_INDEV_GESTURE_PINCH    /* Two-finger pinch */
LV_INDEV_GESTURE_ROTATE   /* Two-finger rotation */
LV_INDEV_GESTURE_SWIPE    /* Two-finger swipe */
```

### Long Press Repeat Time (New)

```c
void lv_indev_set_long_press_repeat_time(lv_indev_t * indev, uint16_t time_ms);
```

---

## Observer/Subject API Changes

### Conditional Bindings (New)

```c
/* Bind widget visibility/state based on subject value comparisons */
lv_obj_bind_checked_ge(lv_obj_t * obj, lv_subject_t * subject, int32_t ref_value);
lv_obj_bind_checked_gt(lv_obj_t * obj, lv_subject_t * subject, int32_t ref_value);
lv_obj_bind_checked_le(lv_obj_t * obj, lv_subject_t * subject, int32_t ref_value);
lv_obj_bind_checked_lt(lv_obj_t * obj, lv_subject_t * subject, int32_t ref_value);
```

### Float Subject (New)

```c
void lv_subject_init_float(lv_subject_t * subject, float value);
void lv_subject_set_float(lv_subject_t * subject, float value);
float lv_subject_get_float(lv_subject_t * subject);
```

### Subject Formatting (New)

```c
void lv_subject_snprintf(lv_subject_t * subject, char * buf, size_t len, const char * fmt, ...);
```

### Notify Only on Change (New)

```c
void lv_subject_notify_if_changed(lv_subject_t * subject);
```

### Remove from Subject (New)

```c
void lv_obj_remove_from_subject(lv_obj_t * obj, lv_subject_t * subject);
```

---

## Animation API Changes

### Pause (New)

```c
void lv_anim_pause(lv_anim_t * anim);
/* Resume with existing lv_anim_start() */
```

### VSync Mode (New)

```c
void lv_anim_set_vsync(lv_anim_t * anim, bool en);
```

---

## Style API Changes

### Margin Shorthand (New)

```c
void lv_style_set_margin_all(lv_style_t * style, int32_t value);
```

### Global Recolor (New)

```c
/* Apply a recolor tint to all widgets and images */
lv_obj_set_style_recolor(obj, lv_color_hex(0xFF0000), LV_PART_MAIN);
lv_obj_set_style_recolor_opa(obj, LV_OPA_50, LV_PART_MAIN);
```

### Scrollbar Length (New)

```c
/* Custom scrollbar length */
lv_obj_set_style_length(obj, 50, LV_PART_SCROLLBAR);
```

---

## Widget-Specific API Changes

### Chart

```c
/* New: get index from x coordinate for scatter charts */
int32_t lv_chart_get_index_from_x(lv_obj_t * obj, int32_t x);

/* New: set cursor position by coordinate */
void lv_chart_set_cursor_pos_x(lv_chart_cursor_t * cursor, int32_t x);
void lv_chart_set_cursor_pos_y(lv_chart_cursor_t * cursor, int32_t y);
```

### Dropdown

```c
/* Changed: added animation parameter */
void lv_dropdown_set_selected(lv_obj_t * obj, uint32_t sel_opt, lv_anim_enable_t anim);
/* Was: lv_dropdown_set_selected(obj, sel_opt) */
```

### Switch

```c
/* New: vertical orientation */
void lv_switch_set_orientation(lv_obj_t * obj, lv_switch_orientation_t orient);
```

### Roller

```c
/* New: set roller option string */
void lv_roller_set_str(lv_obj_t * obj, const char * str);
```

### Scale

```c
/* Changed: return type fixed */
int32_t lv_scale_get_rotation(lv_obj_t * obj);
/* Was: uint16_t */
```

### Span

```c
/* New: get span by touch point */
lv_span_t * lv_spangroup_get_span_by_point(lv_obj_t * obj, lv_point_t * point);
```

### GIF

```c
/* New: loop count control */
void lv_gif_set_loop_count(lv_obj_t * obj, uint32_t count);
```

### AnimImage

```c
/* New: get underlying animation */
lv_anim_t * lv_animimage_get_anim(lv_obj_t * obj);
```

---

## Font Manager API (New Architecture)

```c
/* Multiple font backend support */
void lv_font_manager_init(void);
void lv_font_manager_add_backend(lv_font_backend_t * backend);

/* FreeType additions */
/* Colored glyphs now supported automatically */
/* Font kerning support added */
```

---

## Memory API Changes

### New Functions

```c
void * lv_calloc(size_t n, size_t size);
void * lv_reallocf(void * ptr, size_t size);  /* Free on failure */
```

---

## OS Abstraction Layer (OSAL) Changes

```c
/* New: idle percent for Linux */
uint32_t lv_os_get_idle_percent(void);

/* New: SDL2-based threading support */
/* Enable with LV_USE_OS_SDL */
```

---

## Draw API Changes

### Configurable Thread Priority (New)

```c
void lv_draw_set_thread_priority(int priority);
```

### Letter Drawing (New)

```c
void lv_draw_letter(lv_layer_t * layer, lv_draw_label_dsc_t * dsc, const lv_point_t * pos, uint32_t letter);
```

### Blend Difference Mode (New)

```c
/* New blend mode */
LV_BLEND_MODE_DIFFERENCE
```

### Custom SW Handlers (New)

```c
void lv_draw_sw_set_custom_handler(lv_draw_sw_handler_t handler);
```

---

## File System Changes

### Working Directory (New)

```c
void lv_fs_set_cwd(const char * path);
```

### POSIX Error Codes (New)

```c
/* fs_posix now converts and returns proper error codes */
```

---

## Profiler Changes

```c
/* Nanosecond accuracy support */
void lv_profiler_builtin_set_clock_ns(uint64_t (*get_ns)(void));

/* Different module divisions */
void lv_profiler_set_module(const char * module);
```

---

## Deprecated APIs in v9.3

| Deprecated | Replacement |
|---|---|
| `lv_obj_find_by_id()` | `lv_obj_find_by_name()` |
| `lv_spangroup_set_mode()` | Use updated function name |

---

## Configuration Defines (New in lv_conf.h)

```c
#define LV_USE_XML              1  /* Enable XML declarative UI */
#define LV_USE_SVG              1  /* Enable SVG rendering */
#define LV_USE_3DTEXTURE        1  /* Enable 3D texture widget */
#define LV_COLOR_FORMAT_RGB565_SWAPPED  /* New color format */
#define LV_USE_DRAW_SW_TILED    1  /* Enable tiled rendering */
#define LV_USE_FONT_MANAGER    1  /* Enable font manager with backends */
#define LV_USE_PPA              1  /* ESP32-P4 PPA acceleration */
#define LV_USE_DMA2D            1  /* STM32 DMA2D acceleration */
#define LV_USE_NEMA_GFX         1  /* NemaGFX rendering backend */
#define LV_USE_G2D              1  /* NXP G2D acceleration */
#define LV_USE_UEFI             1  /* UEFI BIOS driver */
```

---

## Sources

- [LVGL v9.3 API Docs](https://docs.lvgl.io/9.3/)
- [LVGL v9.3 XML API](https://docs.lvgl.io/9.3/details/auxiliary-modules/xml/api.html)
- [LVGL v9.3 Changelog](https://docs.lvgl.io/9.3/CHANGELOG.html)
