# LVGL v9.2 API Reference - New and Changed APIs

> **Scope**: APIs added or changed in v9.2.x compared to v9.1.0
> **Breaking Changes**: `lvgl_private.h` separation (internal struct access requires include)

---

## Breaking API Change: Private Header Separation

```c
/* v9.1: All LVGL internals accessible via lvgl.h */
#include "lvgl.h"
obj->coords.x1;  /* Direct struct access worked */

/* v9.2: Internal struct members require lvgl_private.h */
#include "lvgl.h"
#include "lvgl_private.h"  /* Required for internal access */
obj->coords.x1;  /* Now works */

/* OR: Enable globally */
#define LV_USE_PRIVATE_API 1  /* In lv_conf.h */
```

**Functions prefixed with `_lv_*` are phased out from `lvgl.h`.** Use `lvgl_private.h` or the public API equivalents.

---

## New APIs

### Object Management

```c
/* Auto-NULL pointer on object deletion */
/* Registers a pointer that gets set to NULL when the object is deleted */
void lv_obj_null_on_delete(lv_obj_t ** obj_ptr);

/* Get object from group by index */
lv_obj_t * lv_group_get_obj_by_index(lv_group_t * group, uint32_t index);

/* Set/get object ID for testing and identification */
void lv_obj_set_id(lv_obj_t * obj, void * id);
void * lv_obj_get_id(const lv_obj_t * obj);

/* Remove object from observer subject */
void lv_obj_remove_from_subject(lv_obj_t * obj, lv_subject_t * subject);

/* Renamed: lv_button_bind_checked -> lv_obj_bind_checked */
void lv_obj_bind_checked(lv_obj_t * obj, lv_subject_t * subject);
```

### Property System (New)

```c
/* Generic property get/set API for widgets */
lv_result_t lv_obj_set_property(lv_obj_t * obj, const lv_property_t * prop);
lv_property_t lv_obj_get_property(const lv_obj_t * obj, lv_prop_id_t id);

/* Property with style selector support */
lv_result_t lv_obj_set_property_with_selector(lv_obj_t * obj,
                                               const lv_property_t * prop,
                                               lv_style_selector_t selector);

/* Property name resolution */
const char * lv_property_get_name(lv_prop_id_t id);
/* Enabled via LV_USE_OBJ_PROPERTY_NAME */

/* Boolean type support in properties */
/* Properties can now hold bool values natively */
```

### Input Device

```c
/* Mouse hover state support */
/* Objects receive LV_STATE_HOVERED when pointer is over them */
/* Enabled automatically for pointer input devices */

/* Long press time setter (runtime) */
void lv_indev_set_long_press_time(lv_indev_t * indev, uint16_t time_ms);

/* Scroll physics setters (runtime) */
void lv_indev_set_scroll_throw_slow_down(lv_indev_t * indev, uint8_t slow_down);
void lv_indev_set_scroll_throw_limit(lv_indev_t * indev, uint8_t limit);
```

### Matrix Transformations (New Module)

```c
/* Enable via LV_USE_MATRIX and LV_DRAW_TRANSFORM_USE_MATRIX */

/* Matrix type */
typedef struct {
    float m[3][3];
} lv_matrix_t;

/* Matrix operations */
void lv_matrix_identity(lv_matrix_t * matrix);
void lv_matrix_translate(lv_matrix_t * matrix, float tx, float ty);
void lv_matrix_scale(lv_matrix_t * matrix, float sx, float sy);
void lv_matrix_rotate(lv_matrix_t * matrix, float angle);
void lv_matrix_skew(lv_matrix_t * matrix, float sx, float sy);
void lv_matrix_multiply(lv_matrix_t * dest, const lv_matrix_t * src);
void lv_matrix_inverse(lv_matrix_t * dest, const lv_matrix_t * src);
void lv_matrix_transform_point(const lv_matrix_t * matrix, lv_fpoint_t * point);

/* Global matrix drawing mode */
/* Set a transform matrix that applies to all subsequent drawing operations */
```

### Lottie Widget (New)

```c
/* Enable via LV_USE_LOTTIE (requires lv_canvas and thorvg) */

/* Create a Lottie animation widget */
lv_obj_t * lv_lottie_create(lv_obj_t * parent);

/* Set Lottie animation data (JSON) */
void lv_lottie_set_src_data(lv_obj_t * obj, const void * src, size_t src_size);

/* Set Lottie animation file path */
void lv_lottie_set_src_file(lv_obj_t * obj, const char * path);

/* Set the draw buffer for rendering */
void lv_lottie_set_draw_buf(lv_obj_t * obj, lv_draw_buf_t * draw_buf);
```

### Animation Timeline Enhancements

```c
/* Repeat support for animation timeline */
void lv_anim_timeline_set_repeat_count(lv_anim_timeline_t * at,
                                        uint32_t count);
void lv_anim_timeline_set_repeat_delay(lv_anim_timeline_t * at,
                                        uint32_t delay);

/* Completed callback for animation timeline */
void lv_anim_timeline_set_completed_cb(lv_anim_timeline_t * at,
                                        lv_anim_completed_cb_t cb);

/* Restored from v8 */
uint32_t lv_anim_speed_to_time(uint32_t speed, int32_t start, int32_t end);
```

### Widget Enhancements

```c
/* Bar: explicit orientation */
void lv_bar_set_orientation(lv_obj_t * obj, lv_bar_orientation_t orient);
/* LV_BAR_ORIENTATION_AUTO, LV_BAR_ORIENTATION_HORIZONTAL, LV_BAR_ORIENTATION_VERTICAL */

/* Scale: tick drawing order */
void lv_scale_set_draw_ticks_on_top(lv_obj_t * obj, bool en);

/* Scale: multiple line needles */
/* Scale can now have multiple needle line indicators */

/* Table: programmatic cell selection */
void lv_table_set_selected_cell(lv_obj_t * obj, uint16_t row, uint16_t col);

/* Textarea: editable property */
void lv_textarea_set_editable(lv_obj_t * obj, bool en);

/* Barcode: non-tiled mode */
void lv_barcode_set_tiled(lv_obj_t * obj, bool en);

/* Grid navigation: single axis constraint */
/* LV_GRIDNAV_CTRL_HORIZONTAL_ONLY, LV_GRIDNAV_CTRL_VERTICAL_ONLY */
void lv_obj_set_grid_nav(lv_obj_t * obj, lv_grid_nav_ctrl_t ctrl);

/* GIF: loop count control (v9.2.1 backport) */
void lv_gif_set_loop_count(lv_obj_t * obj, uint32_t count);
lv_result_t lv_gif_get_loop_count(lv_obj_t * obj, uint32_t * count);

/* Animimg: getter for underlying animation (v9.2.1 backport) */
lv_anim_t * lv_animimg_get_anim(lv_obj_t * obj);
```

### Calendar

```c
/* Chinese lunisolar calendar */
/* Enable via LV_USE_CALENDAR_CHINESE */
lv_calendar_chinese_t lv_calendar_gregorian_to_chinese(
    uint16_t year, uint8_t month, uint8_t day);
/* Returns year, month, day in Chinese calendar with leap month info */
```

### Display and Drawing

```c
/* PXP: rotation-only mode (no full PXP draw unit) */
/* Enable via LV_USE_PXP config - can use PXP only for rotation */

/* Draw buffer static initialization macro */
#define LV_DRAW_BUF_INIT_STATIC(name, _w, _h, _cf) \
    /* Initializes draw buffer with proper alignment at compile time */

/* Draw buffer duplication with multi-instance support */
lv_draw_buf_t * lv_draw_buf_dup(const lv_draw_buf_t * draw_buf);
```

### Filesystem

```c
/* Default drive letter (no prefix needed) */
/* Configure via LV_FS_DEFAULT_DRIVE_LETTER in lv_conf.h */
/* Example: LV_FS_DEFAULT_DRIVE_LETTER 'S' means "file.bin" = "S:file.bin" */

/* Arduino ESP LittleFS */
void lv_fs_arduino_esp_littlefs_init(void);
/* Configure: LV_USE_FS_ARDUINO_ESP_LITTLEFS, LV_FS_ARDUINO_ESP_LITTLEFS_LETTER */

/* Arduino SD */
void lv_fs_arduino_sd_init(void);
/* Configure: LV_USE_FS_ARDUINO_SD, LV_FS_ARDUINO_SD_LETTER */

/* LittleFS directory open/close support */
/* lv_fs_dir_open, lv_fs_dir_read, lv_fs_dir_close now work with LittleFS */

/* File write cache update */
/* File writes now update the internal file cache (no stale reads) */

/* lv_fs_dir_t type exported to public lv_fs.h (v9.2.1 fix) */
```

### Gradient Drawing (Complex Gradients)

```c
/* Enable via LV_USE_DRAW_SW_COMPLEX_GRADIENTS */

/* Radial gradient (new in SW renderer) */
/* Set via style properties: */
lv_style_set_bg_grad_dir(&style, LV_GRAD_DIR_RADIAL);
/* Additional properties: center point, radius, focal point */

/* Conic gradient */
lv_style_set_bg_grad_dir(&style, LV_GRAD_DIR_CONIC);

/* Skew gradient */
/* Gradient direction can be skewed with matrix transforms */

/* Standardized draw gradient API (refactored) */
/* Internal API for gradient rendering unified across renderers */
```

### String Utilities

```c
/* Safe string concatenation */
char * lv_strncat(char * dst, const char * src, size_t n);

/* Safe string copy (like BSD strlcpy) */
size_t lv_strlcpy(char * dst, const char * src, size_t size);

/* String copy consistency improvements */
char * lv_strncpy(char * dst, const char * src, size_t n);
```

### Image Decoder (Refactored)

```c
/* Decoder name field for debugging */
void lv_image_decoder_set_name(lv_image_decoder_t * decoder, const char * name);

/* Refactored: get_info passes params via dsc */
/* Image decoder get_info now uses decoder descriptor for parameter passing */
/* Reduces file operations during info retrieval */

/* Cache operation extracted to image decoder level */
/* Cache is managed by the image decoder framework, not individual decoders */
```

### Log System

```c
/* Default log print callback */
/* Configure via LV_LOG_PRINT_CB in lv_conf.h */
/* Sets a default callback without needing runtime lv_log_register_print_cb */
```

### NuttX

```c
/* Simplified run loop */
void lv_nuttx_run(void);

/* Deinit support */
void lv_nuttx_deinit(void);

/* Adaptive color format */
/* NuttX driver auto-detects best color format */
```

---

## Changed APIs

### Observer Rename

```c
/* Old (v9.1): */
void lv_button_bind_checked(lv_obj_t * obj, lv_subject_t * subject);

/* New (v9.2): More generic name */
void lv_obj_bind_checked(lv_obj_t * obj, lv_subject_t * subject);
```

### Image Inner Align

```c
/* The image alignment enum was renamed to avoid conflict with obj align */
/* Old: LV_IMAGE_ALIGN_* could conflict with lv_obj_set_align values */
/* New: Use lv_image_set_inner_align() explicitly */
void lv_image_set_inner_align(lv_obj_t * obj, lv_image_align_t align);
```

### Private API Migration

```c
/* These common patterns now require lvgl_private.h: */

/* Accessing internal struct members */
lv_obj_t * obj = ...;
obj->coords;         /* Requires lvgl_private.h */
obj->spec_attr;      /* Requires lvgl_private.h */

/* Using _lv_ prefixed functions */
_lv_area_intersect();  /* Moving to lvgl_private.h */
_lv_obj_get_ext();     /* Moving to lvgl_private.h */

/* Font internal structures */
lv_font_fmt_txt_kern_pair_t  /* Kept public per #6625 */
lv_pinyin_dict_t             /* Kept public per #6645 */
```

---

## New Configuration Defines

```c
/*=== New in lv_conf.h for v9.2 ===*/

/* Standard library include paths (for non-standard toolchains) */
#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

/* Drawing */
#define LV_DRAW_TRANSFORM_USE_MATRIX    0    /* Matrix-based transforms */
#define LV_DRAW_THREAD_STACK_SIZE  (8*1024)  /* Draw thread stack (bytes) */
#define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0   /* Radial/conic/skew gradients */

/* VG-Lite */
#define LV_VG_LITE_GRAD_CACHE_CNT      32   /* Renamed from _SIZE */
#define LV_VG_LITE_STROKE_CACHE_CNT    32   /* Stroke path cache */
#define LV_VG_LITE_THORVG_LINEAR_GRADIENT_EXT_SUPPORT 0

/* Object system */
#define LV_OBJ_ID_AUTO_ASSIGN     LV_USE_OBJ_ID  /* Auto-assign IDs */
#define LV_USE_OBJ_ID_BUILTIN    1               /* Built-in ID (now default on) */
#define LV_USE_OBJ_PROPERTY_NAME  1              /* Property name resolution */

/* Core modules */
#define LV_USE_MATRIX             0    /* Matrix math module */
#define LV_USE_PRIVATE_API        0    /* Include private headers in lvgl.h */

/* Fonts */
#define LV_FONT_SIMSUN_14_CJK    0    /* CJK font */

/* Widgets */
#define LV_USE_CALENDAR_CHINESE   0    /* Chinese calendar */
#define LV_USE_LOTTIE             0    /* Lottie widget */

/* Filesystem */
#define LV_FS_DEFAULT_DRIVE_LETTER      '\0'
#define LV_USE_FS_ARDUINO_ESP_LITTLEFS  0
#define LV_FS_ARDUINO_ESP_LITTLEFS_LETTER '\0'
#define LV_USE_FS_ARDUINO_SD            0
#define LV_FS_ARDUINO_SD_LETTER         '\0'

/* Font rendering */
#define LV_TINY_TTF_CACHE_GLYPH_CNT    256

/* Display drivers */
#define LV_SDL_ACCELERATED       1     /* SDL HW acceleration */
#define LV_USE_WAYLAND           0     /* Wayland driver */
#define LV_WAYLAND_WINDOW_DECORATIONS 0
#define LV_WAYLAND_WL_SHELL      0
#define LV_USE_RENESAS_GLCDC     0     /* Renesas GLCDC */
#define LV_USE_OPENGLES          0     /* OpenGL ES */
#define LV_USE_OPENGLES_DEBUG    1
#define LV_USE_QNX               0     /* QNX */
#define LV_QNX_BUF_COUNT         1
```

---

## Version-Specific Behavior Quick Reference

| Behavior | v9.1 | v9.2 |
|----------|------|------|
| Internal struct access | Direct via `lvgl.h` | Requires `lvgl_private.h` or `LV_USE_PRIVATE_API` |
| `_lv_*` functions | Available in `lvgl.h` | Moving to `lvgl_private.h` |
| Object ID | Opt-in, manual | Built-in by default, auto-assign available |
| Gradients | Linear only in SW | Linear + radial + conic + skew in SW |
| Color formats | RGB565, RGB888, ARGB8888, ARGB8565 | + L8 (grayscale) + I1 (monochrome) |
| Hover state | Not supported | `LV_STATE_HOVERED` on pointer devices |
| Animation timeline | Play/stop/pause | + repeat, repeat_delay, completed_cb |
| Lottie animations | Not supported | Full Lottie widget via ThorVG |
| Matrix transforms | Not supported | Full 2D matrix transform module |
| Recursive mutex | Manual config | Default on |
| VG-Lite gradient cache | `LV_VG_LITE_GRAD_CACHE_SIZE` | `LV_VG_LITE_GRAD_CACHE_CNT` |
| ESP32 LittleFS | Manual via `LV_USE_FS_LITTLEFS` | + Arduino-specific `LV_USE_FS_ARDUINO_ESP_LITTLEFS` |
| Wayland | Not available | Full driver with XDG shell |
| OpenGL ES | Not available | Multi-window + external textures |
| QNX/MQX | Not available | Full RTOS support |
