# LVGL v9.1 API Reference - New and Changed APIs

> **Scope**: APIs added or changed in v9.1.0 compared to v9.0.0
> **Breaking Changes**: None - all v9.0 APIs remain compatible

---

## New APIs

### Input Device (indev)

```c
/* Crown input support for rotary devices (e.g., smartwatch crowns) */
/* Set the crown sensitivity on a pointer input device */
void lv_indev_set_type(lv_indev_t * indev, lv_indev_type_t type);
/* Crown events generate LV_EVENT_ROTARY events */
/* Configure via LV_SDL_MOUSEWHEEL_MODE = LV_SDL_MOUSEWHEEL_MODE_CROWN */
```

### Vector Graphics

```c
/* Append an arc segment to a vector path */
void lv_vector_path_append_arc(lv_vector_path_t * path,
                               const lv_fpoint_t * center,
                               lv_value_precise_t radius,
                               lv_value_precise_t start_angle,
                               lv_value_precise_t sweep,
                               bool pie);
```

### Image Cache

```c
/* Resize the image cache at runtime */
void lv_image_cache_resize(uint32_t new_entry_cnt, bool evict_now);

/* Drop image header cache entries */
void lv_image_header_cache_drop(const void * src);

/* Invalidate draw buffer cache */
void lv_draw_buf_invalidate_cache(lv_draw_buf_t * draw_buf);
```

### Cache Framework (Refactored)

```c
/* New unified cache APIs (replacing fragmented cache management) */
lv_cache_t * lv_cache_create(const lv_cache_class_t * cache_class,
                              size_t node_size, size_t max_size,
                              lv_cache_compare_cb_t compare_cb);
void lv_cache_destroy(lv_cache_t * cache, void * user_data);
lv_cache_entry_t * lv_cache_acquire(lv_cache_t * cache,
                                     const void * search_key,
                                     void * user_data);
void lv_cache_release(lv_cache_t * cache, lv_cache_entry_t * entry,
                       void * user_data);
lv_cache_entry_t * lv_cache_add(lv_cache_t * cache, const void * key,
                                 void * user_data);
void lv_cache_drop(lv_cache_t * cache, const void * key,
                    void * user_data);
void lv_cache_set_max_size(lv_cache_t * cache, size_t max_size,
                            void * user_data);
```

### Display

```c
/* Save a screenshot of the current display to a file */
lv_result_t lv_display_save_screenshot(lv_display_t * disp,
                                        const char * fn);
```

### Event System

```c
/* lv_obj_add_event_cb now returns the event descriptor */
/* This allows later removal via lv_obj_remove_event_cb_with_dsc */
lv_event_dsc_t * lv_obj_add_event_cb(lv_obj_t * obj,
                                      lv_event_cb_t event_cb,
                                      lv_event_code_t filter,
                                      void * user_data);

/* Remove event callback using the descriptor */
bool lv_obj_remove_event_cb_with_dsc(lv_obj_t * obj,
                                      lv_event_dsc_t * dsc);
```

### Draw Buffer

```c
/* Distinguish between lv_image_dsc_t and lv_draw_buf_t */
/* draw_buf_t now has clearer separation from image descriptors */
bool lv_draw_buf_is_buf(const void * buf);

/* Copy draw buffer with indexed image support */
void lv_draw_buf_copy(lv_draw_buf_t * dest,
                       const lv_area_t * dest_area,
                       const lv_draw_buf_t * src,
                       const lv_area_t * src_area);
```

### Draw Descriptors

```c
/* Convenience methods for safely getting correct draw descriptor type */
lv_draw_rect_dsc_t * lv_draw_task_get_rect_dsc(lv_draw_task_t * task);
lv_draw_label_dsc_t * lv_draw_task_get_label_dsc(lv_draw_task_t * task);
lv_draw_image_dsc_t * lv_draw_task_get_image_dsc(lv_draw_task_t * task);
lv_draw_arc_dsc_t * lv_draw_task_get_arc_dsc(lv_draw_task_t * task);
lv_draw_line_dsc_t * lv_draw_task_get_line_dsc(lv_draw_task_t * task);
lv_draw_triangle_dsc_t * lv_draw_task_get_triangle_dsc(lv_draw_task_t * task);
lv_draw_layer_dsc_t * lv_draw_task_get_layer_dsc(lv_draw_task_t * task);
```

### System Monitor

```c
/* Show max memory usage in perf monitor */
/* Automatically displayed when LV_USE_SYSMON is enabled */
/* sysmon now shows peak memory alongside current usage */
```

### Filesystem - LittleFS

```c
/* Initialize LittleFS filesystem driver */
void lv_fs_littlefs_init(void);
/* Configure via LV_USE_FS_LITTLEFS and LV_FS_LITTLEFS_LETTER in lv_conf.h */
```

### Profiler

```c
/* Profiler now supports multithreading */
/* No new public API - existing profiler API works across threads */
/* Enable via LV_USE_PROFILER with multithreading support auto-detected */
```

---

## Changed APIs

### Font Glyph Format (Refactored)

```c
/* Old: glyph format was part of lv_font_t directly */
/* New: Separate lv_font_glyph_format_t type */
typedef enum {
    LV_FONT_GLYPH_FORMAT_NONE,
    LV_FONT_GLYPH_FORMAT_A1,
    LV_FONT_GLYPH_FORMAT_A2,
    LV_FONT_GLYPH_FORMAT_A4,
    LV_FONT_GLYPH_FORMAT_A8,
    /* ... additional formats */
} lv_font_glyph_format_t;

/* Font draw process now uses lv_font_glyph_format_t */
```

### Image Descriptor

```c
/* lv_image_dsc_t now includes data_size field for all C-array images */
typedef struct {
    lv_image_header_t header;
    uint32_t data_size;    /* NEW: size of data in bytes */
    const uint8_t * data;
    const void * reserved;
} lv_image_dsc_t;
```

### Layer Buffer Size Config

```c
/* Old (v9.0): */
#define LV_DRAW_SW_LAYER_SIMPLE_BUF_SIZE  (24 * 1024)

/* New (v9.1): Renamed to be renderer-agnostic */
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE     (24 * 1024)
```

---

## New Color Format

```c
/* ARGB8565: 8-bit alpha + 5-6-5 RGB = 24 bits per pixel */
/* Saves 25% RAM vs ARGB8888 while maintaining full alpha range */
LV_COLOR_FORMAT_ARGB8565
/* Supported in VG-Lite, software renderer, and image tools */
```

---

## New Configuration Defines

```c
/*=== New in lv_conf.h for v9.1 ===*/

/* Rendering */
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE  (24 * 1024)  /* Renamed from LV_DRAW_SW_LAYER_SIMPLE_BUF_SIZE */
#define LV_USE_NATIVE_HELIUM_ASM       0             /* ARM Helium SIMD */

/* VG-Lite GPU */
#define LV_VG_LITE_FLUSH_MAX_COUNT     8             /* Max flush commands */
#define LV_VG_LITE_USE_BOX_SHADOW      0             /* Box shadow via VG-Lite */
#define LV_VG_LITE_GRAD_CACHE_SIZE     32            /* Gradient cache entries */
#define LV_VG_LITE_THORVG_BUF_ADDR_ALIGN  64        /* Buffer alignment */

/* Filesystem */
#define LV_USE_FS_LITTLEFS             0             /* LittleFS driver */
#define LV_FS_LITTLEFS_LETTER          '\0'          /* Drive letter */

/* Input */
#define LV_SDL_MOUSEWHEEL_MODE         LV_SDL_MOUSEWHEEL_MODE_ENCODER  /* Crown/encoder */

/* Linux drivers */
#define LV_USE_LIBINPUT                0             /* libinput driver */
#define LV_LIBINPUT_BSD                0             /* BSD variant */
#define LV_LIBINPUT_XKB                0             /* XKB keyboard layout */
```

---

## Removed/Deprecated Defines

```c
/* Removed in v9.1 */
#define LV_DRAW_SW_LAYER_SIMPLE_BUF_SIZE  /* -> LV_DRAW_LAYER_SIMPLE_BUF_SIZE */
#define LV_FREETYPE_CACHE_SIZE            /* FreeType cache reworked */
#define LV_FREETYPE_CACHE_FT_FACES        /* Removed */
#define LV_FREETYPE_CACHE_FT_SIZES        /* Removed */
#define LV_USE_LZ4                        /* Removed */
#define LV_DEMO_WIDGETS_SLIDESHOW         /* Removed */
```

---

## VG-Lite Specific APIs

```c
/* Stroke path rendering (new) */
/* Enabled automatically when LV_USE_DRAW_VG_LITE is set */
/* Supports stroke width, cap style, join style */

/* Auto path type selection based on opacity */
/* VG-Lite automatically selects optimal path type when alpha < 255 */

/* Gradient cache with auto-release */
/* Configured via LV_VG_LITE_GRAD_CACHE_SIZE */

/* Matrix assertion for debugging */
/* Development aid: validates VG-Lite matrix operations */

/* Index format decode support */
/* VG-Lite can now decode indexed-color images directly */

/* GPU idle flush */
/* Flush pending GPU operations when GPU goes idle */
```

---

## Key Behavioral Changes (Non-Breaking)

| Behavior | v9.0 | v9.1 |
|----------|------|------|
| Scroll + pressed state | Object stays PRESSED during scroll | PRESSED state cleared on scroll start |
| Image decoder cache | Always attempts cache | Skips cache when `no_cache` flag set |
| FreeType italic | Fixed tilt angle | Configurable tilt angle |
| Event callback return | Returns `void` / `lv_result_t` | Returns `lv_event_dsc_t *` for later removal |
| Layer buffer config | SW-renderer specific name | Renderer-agnostic name |
| Profiler | Single-thread only | Multi-thread safe |
