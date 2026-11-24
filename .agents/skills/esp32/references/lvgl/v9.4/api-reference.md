# LVGL v9.4 API Reference -- New and Changed APIs

This document covers APIs that are **new or changed in v9.4.0** compared to v9.3.0. For the full API, see [docs.lvgl.io/9.4](https://docs.lvgl.io/9.4/).

---

## glTF 3D Model API (New)

Enable with `LV_USE_GLTF`. Requires OpenGL ES 2.0 with EGL.

```c
/* Create a glTF viewer widget */
lv_obj_t * lv_gltf_create(lv_obj_t * parent);

/* Load a glTF model */
void lv_gltf_set_src(lv_obj_t * obj, const char * path);

/* Configure the viewer */
void lv_gltf_set_env_image(lv_obj_t * obj, const char * path);
```

**Configuration:**
```c
#define LV_USE_GLTF          1
#define LV_USE_OPENGL        1  /* Required */
#define LV_USE_EGL           1  /* Required */
```

---

## GStreamer API (New)

Enable with `LV_USE_GSTREAMER`. Linux platforms only.

```c
/* Create a GStreamer video widget */
lv_obj_t * lv_gstreamer_create(lv_obj_t * parent);

/* Set video source */
void lv_gstreamer_set_src(lv_obj_t * obj, const char * uri);
```

**Configuration:**
```c
#define LV_USE_GSTREAMER     1
```

---

## DRM + EGL Display Driver API (New)

```c
/* Create DRM display with EGL support */
lv_display_t * lv_drm_egl_display_create(void);

/* Select display mode */
void lv_drm_set_mode(lv_display_t * disp, int mode_index);
```

**Configuration:**
```c
#define LV_USE_DRM           1
#define LV_USE_EGL           1
```

---

## OpenGL / EGL API Changes

### EGL Support (New)

```c
/* Create an OpenGL display using EGL (instead of GLFW) */
lv_display_t * lv_egl_display_create(void);

/* Create display from existing texture */
lv_display_t * lv_opengles_texture_display_create(GLuint texture_id);

/* GLSL version 100 support for default shader (broader compatibility) */
```

### Performance Measurement (New)

```c
/* OpenGL ES performance measurement points added */
/* Automatic -- no API call needed, uses internal profiler */
```

---

## EVE GPU API (New)

Enable with `LV_USE_EVE`.

```c
/* EVE draw unit is automatically created when enabled */

/* Pre-upload assets to EVE chip memory */
void lv_eve_upload_asset(const void * data, size_t size, uint32_t addr);

/* Control write buffering */
void lv_eve_set_write_buffering(bool enable);
```

**Configuration:**
```c
#define LV_USE_EVE           1
```

---

## ESP PPA Hardware Accelerator API (New/Refined)

```c
/* No direct API -- enabled via Kconfig */
/* PPA draw unit registers automatically when configured */
```

**Configuration (sdkconfig):**
```
CONFIG_LV_USE_PPA=y                 # LVGL PPA draw unit
CONFIG_LVGL_PORT_ENABLE_PPA=y       # Display driver PPA (rotation/mirroring)
```

**Performance:** Non-blocking DMA mode with tuned burst sizes.

---

## Arc Label Widget API (New)

New widget: `lv_arc_label`. Enable with `LV_USE_ARC_LABEL`.

```c
/* Create an arc label */
lv_obj_t * lv_arc_label_create(lv_obj_t * parent);

/* Set text */
void lv_arc_label_set_text(lv_obj_t * obj, const char * text);

/* Set text with formatting */
void lv_arc_label_set_text_fmt(lv_obj_t * obj, const char * fmt, ...);

/* Configure arc geometry */
void lv_arc_label_set_radius(lv_obj_t * obj, int32_t radius);
void lv_arc_label_set_radius_pct(lv_obj_t * obj, int32_t pct);  /* Percentage mode */
void lv_arc_label_set_start_angle(lv_obj_t * obj, int32_t angle);

/* Text direction */
void lv_arc_label_set_dir(lv_obj_t * obj, lv_arc_label_dir_t dir);
/* LV_ARC_LABEL_DIR_CLOCKWISE, LV_ARC_LABEL_DIR_COUNTER_CLOCKWISE */

/* Alignment */
void lv_arc_label_set_align_h(lv_obj_t * obj, lv_text_align_t align);
void lv_arc_label_set_align_v(lv_obj_t * obj, lv_text_align_t align);

/* Text offset */
void lv_arc_label_set_text_offset(lv_obj_t * obj, int32_t offset);

/* Letter spacing */
void lv_arc_label_set_letter_space(lv_obj_t * obj, int32_t space);

/* Recolor support (inherited from label) */
void lv_arc_label_set_recolor(lv_obj_t * obj, bool en);
```

---

## Translation System API (New)

Enable with `LV_USE_TRANSLATION`.

```c
/* Initialize translation system */
void lv_translation_init(void);

/* Set current language */
void lv_translation_set_language(const char * lang);

/* Add translation entries */
void lv_translation_add(const char * key, const char * lang, const char * value);

/* Bind a translation tag to a label -- label auto-updates on language change */
void lv_label_bind_translation_tag(lv_obj_t * label, const char * tag);

/* Language change event */
LV_EVENT_LANGUAGE_CHANGED   /* Fired when lv_translation_set_language() is called */
```

---

## Chart API Changes

### Stacked Chart (New)

```c
/* Set chart type to stacked */
lv_chart_set_type(chart, LV_CHART_TYPE_BAR);
/* Stacked mode is configured via series overlap -- no separate enum */
/* feat(chart): add stacked chart support */
```

### Cursor Removal (New)

```c
void lv_chart_remove_cursor(lv_obj_t * chart, lv_chart_cursor_t * cursor);
```

---

## Style API Changes

### Style Merge (New)

```c
/* Merge source style properties into destination */
void lv_style_merge(lv_style_t * dest, const lv_style_t * src);
```

---

## Observer/Subject API Changes

### Subject Increment Events (Restored)

```c
/* Re-added from earlier versions */
void lv_subject_set_increment_event(lv_subject_t * subject, int32_t min, int32_t max);
```

### Style Binding (New)

```c
/* Bind a style to a subject -- style updates when subject changes */
void lv_obj_bind_style(lv_obj_t * obj, lv_style_t * style, lv_subject_t * subject);
```

---

## Display API Changes

### Deprecation

```c
/* DEPRECATED in v9.4 */
void lv_display_set_antialiasing(lv_display_t * disp, bool en);
/* No replacement -- antialiasing is handled differently now */
```

### Get Current Screen Loading (New)

```c
lv_obj_t * lv_display_get_screen_loading(lv_display_t * disp);
```

---

## OSAL (OS Abstraction) Changes

### Sleep (New)

```c
void lv_sleep_ms(uint32_t ms);
```

### Linux Features with OS_NONE (New)

```c
/* Linux-specific features now available even with LV_USE_OS LV_OS_NONE */
```

---

## QR Code API Changes

### Set Data Helper (New)

```c
void lv_qrcode_set_data(lv_obj_t * obj, const void * data, size_t data_len);
```

### Quiet Zone (New)

```c
void lv_qrcode_set_quiet_zone(lv_obj_t * obj, int32_t size);
```

---

## Roller API Changes

### Get Option String (New)

```c
const char * lv_roller_get_option_str(lv_obj_t * obj, uint32_t index);
```

---

## Tabview API Changes

### Get Tab Button by Index (New)

```c
lv_obj_t * lv_tabview_get_tab_button(lv_obj_t * obj, uint32_t index);
```

---

## Span API Changes

### Bind Span Text (New)

```c
void lv_spangroup_bind_span_text(lv_obj_t * obj, lv_subject_t * subject);
void lv_spangroup_set_span_text_fmt(lv_obj_t * obj, uint32_t index, const char * fmt, ...);
```

---

## Scale API Changes

### Bind Section Values (New)

```c
void lv_scale_bind_section_min_value(lv_obj_t * obj, lv_subject_t * subject);
void lv_scale_bind_section_max_value(lv_obj_t * obj, lv_subject_t * subject);
```

---

## Image API Changes

### Data Binding (New)

```c
/* Bind image source to a subject */
void lv_image_bind_src(lv_obj_t * obj, lv_subject_t * subject);
```

### Color Key Support (New)

```c
/* Set transparent color key for images */
void lv_image_set_colorkey(lv_obj_t * obj, lv_color_t color);
```

---

## Bar API Changes

### Data Binding (New)

```c
void lv_bar_bind_value(lv_obj_t * obj, lv_subject_t * subject);
```

---

## Draw API Changes

### Draw Unit Event Callback (New)

```c
/* Add event callback to draw unit lifecycle */
void lv_draw_unit_set_event_cb(lv_draw_unit_t * unit, lv_event_cb_t cb);
```

### Draw Buffer Copy Override (New)

```c
/* GPU can now override buf_copy for hardware-accelerated copies */
void lv_draw_buf_set_copy_cb(void (*cb)(lv_draw_buf_t * dest, const lv_draw_buf_t * src, const lv_area_t * area));
```

---

## Event API Changes

### Delete on Event (New)

```c
/* Delete display or indev from within an event callback */
void lv_display_delete_on_event(lv_display_t * disp, lv_event_code_t event);
void lv_indev_delete_on_event(lv_indev_t * indev, lv_event_code_t event);
```

---

## SDL Display API Changes

### Window Accessor (New)

```c
/* Get SDL_Window from display */
SDL_Window * lv_sdl_get_window(lv_display_t * disp);
```

### Window Size (New)

```c
void lv_sdl_set_window_size(lv_display_t * disp, int32_t w, int32_t h);
```

---

## System Monitor API Changes

### Start/Stop and FPS Dump (New)

```c
void lv_sysmon_start(void);
void lv_sysmon_stop(void);
void lv_sysmon_dump_fps(void);
```

---

## Font Manager Changes

### Resource Leak Check (New)

```c
/* Font manager now checks for resource leaks before removing a font source */
/* Automatic -- internal improvement */
```

---

## FrogFS Support (New)

```c
/* Pack directories into blobs and load at runtime */
#define LV_USE_FS_FROGFS     1
```

---

## Evdev Changes

### Adopt Existing File Descriptor (New)

```c
/* Use an existing fd instead of opening a new one */
lv_indev_t * lv_evdev_create_from_fd(int fd);
```

---

## NEON Acceleration API

No direct user API -- enabled automatically via build configuration:

```c
#define LV_USE_DRAW_SW_NEON   1  /* Enable ARM NEON acceleration */
```

Affects:
- RGB888 blending
- XRGB8888 blending
- RGB565 64-bit blending
- Alpha premultiply operations

---

## Configuration Defines (New in v9.4 lv_conf.h)

```c
#define LV_USE_GLTF             1  /* glTF 3D model support */
#define LV_USE_GSTREAMER        1  /* GStreamer multimedia */
#define LV_USE_EGL              1  /* EGL display driver */
#define LV_USE_EVE              1  /* EVE GPU renderer */
#define LV_USE_ARC_LABEL        1  /* Arc label widget */
#define LV_USE_TRANSLATION      1  /* Translation system */
#define LV_USE_FS_FROGFS        1  /* FrogFS file system */
#define LV_USE_DRAW_SW_NEON     1  /* ARM NEON acceleration */
#define LV_USE_PPA              1  /* ESP32-P4 PPA (refined from v9.3) */
#define LV_USE_API_MAPPING      0  /* Disable backward API map (optional) */
#define LV_USE_VGLITE_HAL       1  /* VGLite HAL driver */
#define LV_USE_VGLITE_KERNEL    1  /* VGLite kernel driver */
```

---

## Deprecated APIs in v9.4

| Deprecated | Replacement | Notes |
|---|---|---|
| `lv_display_set_antialiasing()` | None | Antialiasing handled differently |
| `lv_obj_find_by_id()` | `lv_obj_find_by_name()` | Deprecated since v9.3 |
| WL_SHELL (Wayland) | XDG shell | Removed entirely |

---

## Backward Compatibility

v9.4 includes the v9.2 API mapping by default. Code targeting v9.2 or v9.3 should compile without changes. The mapping can be disabled:

```c
#define LV_USE_API_MAPPING  0  /* Disable v9.2/v9.3 backward compat */
```

---

## Sources

- [LVGL v9.4 API Docs](https://docs.lvgl.io/9.4/)
- [LVGL v9.4 Changelog](https://docs.lvgl.io/9.4/CHANGELOG.html)
- [LVGL v9.4 Release Announcement](https://forum.lvgl.io/t/lvgl-v9-4-is-released/22502)
- [glTF Documentation](https://docs.lvgl.io/master/details/libs/gltf.html)
- [ESP32 PPA Documentation](https://docs.lvgl.io/9.5/integration/chip_vendors/espressif/hardware_accelerator_ppa.html)
