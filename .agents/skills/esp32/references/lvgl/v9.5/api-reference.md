# LVGL v9.5 API Reference

> Complete API reference for LVGL v9.5.x (latest stable as of February 2026).
> Official API docs: https://docs.lvgl.io/9.5/API/

---

## Table of Contents

1. [Configuration (lv_conf.h)](#configuration-lv_confh)
2. [Display API](#display-api)
3. [Input Device API](#input-device-api)
4. [Object (Widget) API](#object-widget-api)
5. [Style API](#style-api)
6. [Event API](#event-api)
7. [Animation API](#animation-api)
8. [Timer API](#timer-api)
9. [Font API](#font-api)
10. [Image API](#image-api)
11. [File System API](#file-system-api)
12. [Drawing API](#drawing-api)
13. [Layout API](#layout-api)
14. [Theme API](#theme-api)
15. [Group API](#group-api)
16. [Memory API](#memory-api)
17. [OS Abstraction API](#os-abstraction-api)

---

## Configuration (lv_conf.h)

Copy `lv_conf_template.h` to `lv_conf.h` and change initial `#if 0` to `#if 1`.

### Color and Display

| Define | Values | Default | Description |
|--------|--------|---------|-------------|
| `LV_COLOR_DEPTH` | 1, 8, 16, 24, 32 | 16 | Color depth in bits |
| `LV_DPI_DEF` | int | 130 | Default dots per inch |
| `LV_DEF_REFR_PERIOD` | ms | 33 | Display refresh period |

### Memory

| Define | Values | Default | Description |
|--------|--------|---------|-------------|
| `LV_MEM_SIZE` | bytes | 65536 | Available memory for lv_malloc (min 2kB) |
| `LV_MEM_POOL_EXPAND_SIZE` | bytes | 0 | Memory pool expansion size |
| `LV_MEM_ADR` | addr | 0 | Custom memory pool address (0=unused) |
| `LV_USE_STDLIB_MALLOC` | enum | builtin | Malloc impl: builtin/clib/micropython/rtthread/custom |
| `LV_USE_STDLIB_STRING` | enum | builtin | String function implementation |
| `LV_USE_STDLIB_SPRINTF` | enum | builtin | Printf implementation |

### Operating System

| Define | Values | Default | Description |
|--------|--------|---------|-------------|
| `LV_USE_OS` | enum | LV_OS_NONE | none/pthread/freertos/cmsis_rtos2/rtthread/windows/mqx/sdl2/custom |
| `LV_USE_FREERTOS_TASK_NOTIFY` | 0/1 | 0 | FreeRTOS task notification support |

### Software Drawing Engine

| Define | Values | Default | Description |
|--------|--------|---------|-------------|
| `LV_USE_DRAW_SW` | 0/1 | 1 | Enable software drawing |
| `LV_DRAW_SW_COMPLEX` | 0/1 | 1 | Rounded corners, shadows, arcs |
| `LV_DRAW_SW_DRAW_UNIT_CNT` | int | 1 | Parallel drawing threads |
| `LV_DRAW_SW_SHADOW_CACHE_SIZE` | int | 0 | Shadow buffer size |
| `LV_DRAW_SW_CIRCLE_CACHE_SIZE` | int | 4 | Cached circle data count |
| `LV_DRAW_BUF_STRIDE_ALIGN` | int | 1 | Stride alignment in bytes |
| `LV_DRAW_BUF_ALIGN` | int | 4 | Buffer address alignment |
| `LV_DRAW_TRANSFORM_USE_MATRIX` | 0/1 | 0 | Matrix transformation support |
| `LV_DRAW_LAYER_SIMPLE_BUF_SIZE` | bytes | 24576 | Layer chunk target size |
| `LV_DRAW_LAYER_MAX_MEMORY` | bytes | 0 | Maximum layer memory (0=unlimited) |
| `LV_DRAW_SW_SUPPORT_RGB565` | 0/1 | 1 | RGB565 format support |
| `LV_DRAW_SW_SUPPORT_RGB888` | 0/1 | 1 | RGB888 format support |
| `LV_DRAW_SW_SUPPORT_XRGB8888` | 0/1 | 1 | XRGB8888 format support |
| `LV_DRAW_SW_SUPPORT_ARGB8888` | 0/1 | 1 | ARGB8888 format support |
| `LV_DRAW_SW_SUPPORT_L8` | 0/1 | 1 | L8 grayscale support |
| `LV_DRAW_SW_SUPPORT_AL88` | 0/1 | 1 | AL88 alpha+luminance |
| `LV_DRAW_SW_SUPPORT_A8` | 0/1 | 1 | A8 alpha-only support |
| `LV_DRAW_SW_SUPPORT_I1` | 0/1 | 1 | 1-bit indexed support |

### GPU Accelerators

| Define | Description |
|--------|-------------|
| `LV_USE_NEMA_GFX` | Nema graphics support |
| `LV_USE_NEMA_VG` | Nema vector graphics |
| `LV_USE_PXP` | NXP PXP (iMX RTxxx) |
| `LV_USE_G2D` | NXP G2D (MPU platforms) |
| `LV_USE_DRAW_DAVE2D` | Renesas Dave2D |
| `LV_USE_DRAW_VG_LITE` | VG-Lite GPU |
| `LV_USE_DRAW_DMA2D` | STM32 DMA2D |
| `LV_USE_DRAW_SDL` | SDL texture caching |
| `LV_USE_DRAW_OPENGLES` | OpenGL ES rendering |
| `LV_USE_PPA` | Espressif PPA accelerator |
| `LV_USE_DRAW_EVE` | EVE display controller |
| `LV_USE_DRAW_NANOVG` | NanoVG renderer (new in v9.5) |
| `LV_USE_DRAW_ARM2D_SYNC` | Arm-2D acceleration |

### RISC-V Vector Extension (New in v9.5)

```c
#define LV_USE_DRAW_SW_ASM  LV_DRAW_SW_ASM_RISCV_V
```

### Threading

| Define | Values | Default | Description |
|--------|--------|---------|-------------|
| `LV_DRAW_THREAD_STACK_SIZE` | bytes | 32768 | Drawing thread stack (32KB+ recommended) |
| `LV_DRAW_THREAD_PRIO` | int | 0 | Thread priority |

### Logging

| Define | Values | Default | Description |
|--------|--------|---------|-------------|
| `LV_USE_LOG` | 0/1 | 0 | Enable logging |
| `LV_LOG_LEVEL` | enum | LV_LOG_LEVEL_WARN | Trace/Info/Warn/Error/User/None |
| `LV_LOG_PRINTF` | 0/1 | 0 | Use printf vs callback |
| `LV_LOG_USE_TIMESTAMP` | 0/1 | 1 | Include timestamps |
| `LV_LOG_USE_FILE_LINE` | 0/1 | 1 | Include file/line |
| `LV_LOG_TRACE_MEM` | 0/1 | 0 | Trace memory ops |
| `LV_LOG_TRACE_TIMER` | 0/1 | 0 | Trace timer ops |
| `LV_LOG_TRACE_INDEV` | 0/1 | 0 | Trace input device |
| `LV_LOG_TRACE_DISP_REFR` | 0/1 | 0 | Trace display refresh |
| `LV_LOG_TRACE_EVENT` | 0/1 | 0 | Trace events |
| `LV_LOG_TRACE_OBJ_CREATE` | 0/1 | 0 | Trace object creation |
| `LV_LOG_TRACE_LAYOUT` | 0/1 | 0 | Trace layout |
| `LV_LOG_TRACE_ANIM` | 0/1 | 0 | Trace animation |
| `LV_LOG_TRACE_CACHE` | 0/1 | 0 | Trace cache ops |

### Assertions

| Define | Default | Description |
|--------|---------|-------------|
| `LV_USE_ASSERT_NULL` | 1 | Null parameter checking |
| `LV_USE_ASSERT_MALLOC` | 1 | Allocation success checking |
| `LV_USE_ASSERT_STYLE` | 0 | Style initialization validation |
| `LV_USE_ASSERT_MEM_INTEGRITY` | 0 | Memory integrity checking |
| `LV_USE_ASSERT_OBJ` | 0 | Object type/existence validation |

### Object Features

| Define | Default | Description |
|--------|---------|-------------|
| `LV_OBJ_STYLE_CACHE` | 1 | 64-bit style cache per object |
| `LV_USE_OBJ_ID` | 0 | Add ID field to objects |
| `LV_USE_OBJ_NAME` | 0 | Widget name support |
| `LV_OBJ_ID_AUTO_ASSIGN` | 0 | Automatic ID assignment |
| `LV_USE_OBJ_PROPERTY` | 0 | Property get/set API |
| `LV_USE_OBJ_PROPERTY_NAME` | 0 | Property name strings |
| `LV_USE_GESTURE_RECOGNITION` | 0 | Gesture detection |

### Caching

| Define | Default | Description |
|--------|---------|-------------|
| `LV_CACHE_DEF_SIZE` | 0 | Default cache size for decoders |
| `LV_IMAGE_HEADER_CACHE_DEF_CNT` | 0 | Image header cache entries |
| `LV_GRADIENT_MAX_STOPS` | 2 | Max gradient stops |

### Built-in Fonts

| Define | Description |
|--------|-------------|
| `LV_FONT_MONTSERRAT_8` through `LV_FONT_MONTSERRAT_48` | Montserrat fonts at each size |
| `LV_FONT_MONTSERRAT_28_COMPRESSED` | 3-bpp compressed variant |
| `LV_FONT_DEJAVU_16_PERSIAN_HEBREW` | Arabic/Hebrew font |
| `LV_FONT_SOURCE_HAN_SANS_SC_14_CJK` | CJK 14px |
| `LV_FONT_SOURCE_HAN_SANS_SC_16_CJK` | CJK 16px |
| `LV_FONT_UNSCII_8` | Monospace pixel 8px |
| `LV_FONT_UNSCII_16` | Monospace pixel 16px |
| `LV_FONT_DEFAULT` | Default font (usually Montserrat 14) |

### Text

| Define | Default | Description |
|--------|---------|-------------|
| `LV_TXT_ENC` | LV_TXT_ENC_UTF8 | UTF-8 or ASCII |
| `LV_TXT_BREAK_CHARS` | " ,.;:-_)]}" | Line break characters |
| `LV_TXT_LINE_BREAK_LONG_LEN` | 0 | Long word break threshold |
| `LV_USE_BIDI` | 0 | Bidirectional text |
| `LV_USE_ARABIC_PERSIAN_CHARS` | 0 | Arabic/Persian processing |

### Widget Enables

| Define | Default | Description |
|--------|---------|-------------|
| `LV_USE_ANIMIMG` | 1 | Animated image |
| `LV_USE_ARC` | 1 | Arc |
| `LV_USE_ARCLABEL` | 1 | Arc label |
| `LV_USE_BAR` | 1 | Progress bar |
| `LV_USE_BUTTON` | 1 | Button |
| `LV_USE_BUTTONMATRIX` | 1 | Button matrix |
| `LV_USE_CALENDAR` | 1 | Calendar |
| `LV_USE_CANVAS` | 1 | Canvas |
| `LV_USE_CHART` | 1 | Chart |
| `LV_USE_CHECKBOX` | 1 | Checkbox |
| `LV_USE_DROPDOWN` | 1 | Dropdown (requires label) |
| `LV_USE_IMAGE` | 1 | Image |
| `LV_USE_IMAGEBUTTON` | 1 | Image button |
| `LV_USE_KEYBOARD` | 1 | On-screen keyboard |
| `LV_USE_LABEL` | 1 | Label |
| `LV_USE_LED` | 1 | LED indicator |
| `LV_USE_LINE` | 1 | Line |
| `LV_USE_LIST` | 1 | List |
| `LV_USE_LOTTIE` | 0 | Lottie (requires canvas+thorvg) |
| `LV_USE_MENU` | 1 | Menu |
| `LV_USE_MSGBOX` | 1 | Message box |
| `LV_USE_ROLLER` | 1 | Roller (requires label) |
| `LV_USE_SCALE` | 1 | Scale |
| `LV_USE_SLIDER` | 1 | Slider (requires bar) |
| `LV_USE_SPAN` | 1 | Span text formatting |
| `LV_USE_SPINBOX` | 1 | Spinbox |
| `LV_USE_SPINNER` | 1 | Loading spinner |
| `LV_USE_SWITCH` | 1 | Toggle switch |
| `LV_USE_TABLE` | 1 | Table |
| `LV_USE_TABVIEW` | 1 | Tabbed view |
| `LV_USE_TEXTAREA` | 1 | Text area (requires label) |
| `LV_USE_TILEVIEW` | 1 | Tile view |
| `LV_USE_WIN` | 1 | Window |
| `LV_USE_3DTEXTURE` | 0 | 3D texture widget |

### Themes

| Define | Default | Description |
|--------|---------|-------------|
| `LV_USE_THEME_DEFAULT` | 1 | Default theme |
| `LV_THEME_DEFAULT_DARK` | 0 | Dark mode |
| `LV_THEME_DEFAULT_GROW` | 1 | Press-to-grow effect |
| `LV_THEME_DEFAULT_TRANSITION_TIME` | 80 | Transition duration ms |
| `LV_USE_THEME_SIMPLE` | 1 | Simple starter theme |
| `LV_USE_THEME_MONO` | 1 | Monochrome theme |

### Layouts

| Define | Default | Description |
|--------|---------|-------------|
| `LV_USE_FLEX` | 1 | Flexbox layout |
| `LV_USE_GRID` | 1 | CSS Grid layout |

### File Systems

| Define | Description |
|--------|-------------|
| `LV_USE_FS_STDIO` | Standard C fopen/fread |
| `LV_USE_FS_POSIX` | POSIX open/read |
| `LV_USE_FS_WIN32` | Windows CreateFile |
| `LV_USE_FS_FATFS` | FatFS support |
| `LV_USE_FS_MEMFS` | Memory-mapped files |
| `LV_USE_FS_LITTLEFS` | LittleFS support |

Each FS has `_LETTER`, `_PATH`, and `_CACHE_SIZE` sub-defines.

### Miscellaneous

| Define | Default | Description |
|--------|---------|-------------|
| `LV_BIG_ENDIAN_SYSTEM` | 0 | Big-endian processor |
| `LV_USE_FLOAT` | 0 | Float precision |
| `LV_USE_MATRIX` | 0 | Matrix transformation |
| `LV_USE_PRIVATE_API` | 0 | Include internal API |
| `LV_WIDGETS_HAS_DEFAULT_VALUE` | 1 | Default widget content |

---

## Display API

### Creating a Display

```c
// Create display with resolution
lv_display_t * disp = lv_display_create(uint32_t hor_res, uint32_t ver_res);

// Set flush callback (sends pixels to hardware)
lv_display_set_flush_cb(lv_display_t * disp, lv_display_flush_cb_t flush_cb);

// Set draw buffers
lv_display_set_buffers(lv_display_t * disp, void * buf1, void * buf2,
                       uint32_t buf_size_bytes, lv_display_render_mode_t mode);
```

### Flush Callback Signature

```c
void my_flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map) {
    // Send pixels to display hardware
    // ...
    // Notify LVGL flush is complete
    lv_display_flush_ready(display);
}
```

### Render Modes

| Mode | Description |
|------|-------------|
| `LV_DISPLAY_RENDER_MODE_PARTIAL` | Render only changed areas (smallest buffer) |
| `LV_DISPLAY_RENDER_MODE_DIRECT` | Render directly into full framebuffer |
| `LV_DISPLAY_RENDER_MODE_FULL` | Always redraw entire screen |

### Display Functions

```c
// Resolution
void lv_display_set_resolution(lv_display_t * disp, uint32_t hor_res, uint32_t ver_res);
uint32_t lv_display_get_horizontal_resolution(lv_display_t * disp);
uint32_t lv_display_get_vertical_resolution(lv_display_t * disp);

// Rotation
void lv_display_set_rotation(lv_display_t * disp, lv_display_rotation_t rotation);
lv_display_rotation_t lv_display_get_rotation(lv_display_t * disp);
// Rotations: LV_DISPLAY_ROTATION_0, _90, _180, _270

// Color format
void lv_display_set_color_format(lv_display_t * disp, lv_color_format_t color_format);

// DPI
void lv_display_set_dpi(lv_display_t * disp, uint32_t dpi);
uint32_t lv_display_get_dpi(lv_display_t * disp);

// Active display
lv_display_t * lv_display_get_default(void);
void lv_display_set_default(lv_display_t * disp);

// Screens
lv_obj_t * lv_display_get_screen_active(lv_display_t * disp);
void lv_display_load_screen(lv_obj_t * screen);
void lv_screen_load_anim(lv_obj_t * screen, lv_scr_load_anim_t anim,
                          uint32_t time, uint32_t delay, bool auto_del);

// Point rotation (new in v9.5)
void lv_display_rotate_point(lv_display_t * disp, lv_point_t * point);

// Invalidation
void lv_obj_invalidate(lv_obj_t * obj);
void lv_area_invalidate(const lv_area_t * area);

// Refresh period
void lv_display_set_refr_period(lv_display_t * disp, uint32_t period_ms);

// User data
void lv_display_set_user_data(lv_display_t * disp, void * user_data);
void * lv_display_get_user_data(lv_display_t * disp);
```

### Screen Load Animations

```
LV_SCR_LOAD_ANIM_NONE
LV_SCR_LOAD_ANIM_OVER_LEFT / _RIGHT / _TOP / _BOTTOM
LV_SCR_LOAD_ANIM_MOVE_LEFT / _RIGHT / _TOP / _BOTTOM
LV_SCR_LOAD_ANIM_FADE_IN / _FADE_OUT
LV_SCR_LOAD_ANIM_OUT_LEFT / _RIGHT / _TOP / _BOTTOM
```

---

## Input Device API

### Creating Input Devices

```c
// Create input device
lv_indev_t * indev = lv_indev_create();

// Set type
lv_indev_set_type(indev, lv_indev_type_t type);

// Set read callback
lv_indev_set_read_cb(indev, lv_indev_read_cb_t read_cb);
```

### Input Device Types

| Type | Description |
|------|-------------|
| `LV_INDEV_TYPE_POINTER` | Touchscreen / mouse |
| `LV_INDEV_TYPE_KEYPAD` | Keyboard |
| `LV_INDEV_TYPE_ENCODER` | Rotary encoder |
| `LV_INDEV_TYPE_BUTTON` | External buttons |

### Read Callback Signature

```c
void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data) {
    data->point.x = touchpad_x;
    data->point.y = touchpad_y;
    data->state = touchpad_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
```

### Input Device Functions

```c
// Timing
void lv_indev_set_long_press_time(lv_indev_t * indev, uint16_t time_ms);
void lv_indev_set_scroll_limit(lv_indev_t * indev, uint8_t scroll_limit);
void lv_indev_set_scroll_throw(lv_indev_t * indev, uint8_t scroll_throw);

// Gesture threshold (new in v9.5)
void lv_indev_set_gesture_limit(lv_indev_t * indev, uint8_t limit);

// Key remapping (new in v9.5)
void lv_indev_set_key_remap(lv_indev_t * indev, const lv_key_remap_t * remap, uint8_t cnt);

// Active indev
lv_indev_t * lv_indev_active(void);
uint32_t lv_indev_get_key(lv_indev_t * indev);
lv_dir_t lv_indev_get_gesture_dir(lv_indev_t * indev);

// User data
void lv_indev_set_user_data(lv_indev_t * indev, void * user_data);
void * lv_indev_get_user_data(lv_indev_t * indev);
```

---

## Object (Widget) API

### Object Creation and Deletion

```c
lv_obj_t * lv_obj_create(lv_obj_t * parent);
void lv_obj_delete(lv_obj_t * obj);
void lv_obj_clean(lv_obj_t * obj);  // Delete all children
void lv_obj_delete_delayed(lv_obj_t * obj, uint32_t delay_ms);
```

### Position and Size

```c
// Position
void lv_obj_set_pos(lv_obj_t * obj, int32_t x, int32_t y);
void lv_obj_set_x(lv_obj_t * obj, int32_t x);
void lv_obj_set_y(lv_obj_t * obj, int32_t y);

// Size
void lv_obj_set_size(lv_obj_t * obj, int32_t w, int32_t h);
void lv_obj_set_width(lv_obj_t * obj, int32_t w);
void lv_obj_set_height(lv_obj_t * obj, int32_t h);
void lv_obj_set_content_width(lv_obj_t * obj, int32_t w);
void lv_obj_set_content_height(lv_obj_t * obj, int32_t h);

// Special values
// LV_SIZE_CONTENT  - Size to fit content
// LV_PCT(x)        - Percentage of parent (e.g., LV_PCT(50) = 50%)

// Alignment
void lv_obj_set_align(lv_obj_t * obj, lv_align_t align);
void lv_obj_align(lv_obj_t * obj, lv_align_t align, int32_t x_ofs, int32_t y_ofs);
void lv_obj_align_to(lv_obj_t * obj, lv_obj_t * base, lv_align_t align,
                      int32_t x_ofs, int32_t y_ofs);

// Min/Max constraints (new in v9.5: works with LV_SIZE_CONTENT and LV_PCT with flex grow)
void lv_obj_set_style_min_width(lv_obj_t * obj, int32_t value, lv_style_selector_t sel);
void lv_obj_set_style_max_width(lv_obj_t * obj, int32_t value, lv_style_selector_t sel);
void lv_obj_set_style_min_height(lv_obj_t * obj, int32_t value, lv_style_selector_t sel);
void lv_obj_set_style_max_height(lv_obj_t * obj, int32_t value, lv_style_selector_t sel);

// Getters
int32_t lv_obj_get_x(lv_obj_t * obj);
int32_t lv_obj_get_y(lv_obj_t * obj);
int32_t lv_obj_get_width(lv_obj_t * obj);
int32_t lv_obj_get_height(lv_obj_t * obj);
int32_t lv_obj_get_content_width(lv_obj_t * obj);
int32_t lv_obj_get_content_height(lv_obj_t * obj);
```

### Alignment Options

```
LV_ALIGN_DEFAULT
LV_ALIGN_TOP_LEFT / TOP_MID / TOP_RIGHT
LV_ALIGN_BOTTOM_LEFT / BOTTOM_MID / BOTTOM_RIGHT
LV_ALIGN_LEFT_MID / CENTER / RIGHT_MID
LV_ALIGN_OUT_TOP_LEFT / OUT_TOP_MID / OUT_TOP_RIGHT
LV_ALIGN_OUT_BOTTOM_LEFT / OUT_BOTTOM_MID / OUT_BOTTOM_RIGHT
LV_ALIGN_OUT_LEFT_TOP / OUT_LEFT_MID / OUT_LEFT_BOTTOM
LV_ALIGN_OUT_RIGHT_TOP / OUT_RIGHT_MID / OUT_RIGHT_BOTTOM
```

### Flags

```c
void lv_obj_add_flag(lv_obj_t * obj, lv_obj_flag_t flag);
void lv_obj_remove_flag(lv_obj_t * obj, lv_obj_flag_t flag);
bool lv_obj_has_flag(lv_obj_t * obj, lv_obj_flag_t flag);
```

| Flag | Description |
|------|-------------|
| `LV_OBJ_FLAG_HIDDEN` | Hidden, not drawn |
| `LV_OBJ_FLAG_CLICKABLE` | Receives click events |
| `LV_OBJ_FLAG_CLICK_FOCUSABLE` | Focusable on click |
| `LV_OBJ_FLAG_CHECKABLE` | Toggle checked state on click |
| `LV_OBJ_FLAG_SCROLLABLE` | Can be scrolled |
| `LV_OBJ_FLAG_SCROLL_ELASTIC` | Elastic scroll effect |
| `LV_OBJ_FLAG_SCROLL_MOMENTUM` | Momentum on scroll |
| `LV_OBJ_FLAG_SCROLL_ONE` | Scroll only one snappable child |
| `LV_OBJ_FLAG_SCROLL_CHAIN_HOR` | Chain horizontal scroll to parent |
| `LV_OBJ_FLAG_SCROLL_CHAIN_VER` | Chain vertical scroll to parent |
| `LV_OBJ_FLAG_SCROLL_ON_FOCUS` | Scroll to show focused child |
| `LV_OBJ_FLAG_SCROLL_WITH_ARROW` | Arrow keys scroll |
| `LV_OBJ_FLAG_SNAPPABLE` | Snap when parent scrolls |
| `LV_OBJ_FLAG_PRESS_LOCK` | Keep pressed even if cursor leaves |
| `LV_OBJ_FLAG_EVENT_BUBBLE` | Bubble events to parent |
| `LV_OBJ_FLAG_GESTURE_BUBBLE` | Bubble gestures to parent |
| `LV_OBJ_FLAG_ADV_HITTEST` | Advanced hit testing |
| `LV_OBJ_FLAG_IGNORE_LAYOUT` | Ignore layout positioning |
| `LV_OBJ_FLAG_FLOATING` | Don't scroll with parent |
| `LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS` | Send draw task events |
| `LV_OBJ_FLAG_OVERFLOW_VISIBLE` | Allow drawing outside bounds |
| `LV_OBJ_FLAG_FLEX_IN_NEW_TRACK` | Start new flex track |
| `LV_OBJ_FLAG_LAYOUT_1` | Custom layout flag 1 |
| `LV_OBJ_FLAG_LAYOUT_2` | Custom layout flag 2 |
| `LV_OBJ_FLAG_RADIO_BUTTON` | Radio button mode (new in v9.5) |

### States

```c
void lv_obj_add_state(lv_obj_t * obj, lv_state_t state);
void lv_obj_remove_state(lv_obj_t * obj, lv_state_t state);
bool lv_obj_has_state(lv_obj_t * obj, lv_state_t state);
lv_state_t lv_obj_get_state(lv_obj_t * obj);
```

| State | Value | Description |
|-------|-------|-------------|
| `LV_STATE_DEFAULT` | 0x0000 | Normal state |
| `LV_STATE_CHECKED` | 0x0001 | Toggled/checked |
| `LV_STATE_FOCUSED` | 0x0002 | Focused via keypad/encoder |
| `LV_STATE_FOCUS_KEY` | 0x0004 | Focused via keypad |
| `LV_STATE_EDITED` | 0x0008 | Edited via encoder |
| `LV_STATE_HOVERED` | 0x0010 | Hovered by mouse |
| `LV_STATE_PRESSED` | 0x0020 | Being pressed |
| `LV_STATE_SCROLLED` | 0x0040 | Being scrolled |
| `LV_STATE_DISABLED` | 0x0080 | Disabled |
| `LV_STATE_USER_1..4` | 0x1000..0x8000 | Custom states |
| `LV_STATE_ALT` | (new in v9.5) | Dark/light mode toggle |

### Scrolling

```c
void lv_obj_scroll_to(lv_obj_t * obj, int32_t x, int32_t y, lv_anim_enable_t anim);
void lv_obj_scroll_to_x(lv_obj_t * obj, int32_t x, lv_anim_enable_t anim);
void lv_obj_scroll_to_y(lv_obj_t * obj, int32_t y, lv_anim_enable_t anim);
void lv_obj_scroll_to_view(lv_obj_t * obj, lv_anim_enable_t anim);
void lv_obj_scroll_to_view_recursive(lv_obj_t * obj, lv_anim_enable_t anim);

int32_t lv_obj_get_scroll_x(lv_obj_t * obj);
int32_t lv_obj_get_scroll_y(lv_obj_t * obj);
int32_t lv_obj_get_scroll_top(lv_obj_t * obj);
int32_t lv_obj_get_scroll_bottom(lv_obj_t * obj);
int32_t lv_obj_get_scroll_left(lv_obj_t * obj);
int32_t lv_obj_get_scroll_right(lv_obj_t * obj);

void lv_obj_set_scroll_dir(lv_obj_t * obj, lv_dir_t dir);
void lv_obj_set_scroll_snap_x(lv_obj_t * obj, lv_scroll_snap_t snap);
void lv_obj_set_scroll_snap_y(lv_obj_t * obj, lv_scroll_snap_t snap);
```

### Tree Navigation

```c
lv_obj_t * lv_obj_get_parent(lv_obj_t * obj);
lv_obj_t * lv_obj_get_child(lv_obj_t * obj, int32_t idx);  // -1 = last
uint32_t lv_obj_get_child_count(lv_obj_t * obj);
uint32_t lv_obj_get_index(lv_obj_t * obj);
void lv_obj_move_to_index(lv_obj_t * obj, int32_t index);
void lv_obj_set_parent(lv_obj_t * obj, lv_obj_t * parent);
lv_obj_t * lv_obj_get_screen(lv_obj_t * obj);

// Naming
void lv_obj_set_name(lv_obj_t * obj, const char * name);
const char * lv_obj_get_name(lv_obj_t * obj);
```

### User Data

```c
void lv_obj_set_user_data(lv_obj_t * obj, void * user_data);
void * lv_obj_get_user_data(lv_obj_t * obj);

// External data and destructor (new in v9.5)
void lv_obj_set_ext_data(lv_obj_t * obj, void * data, lv_obj_ext_data_free_cb_t free_cb);
```

---

## Style API

### Style Lifecycle

```c
void lv_style_init(lv_style_t * style);
void lv_style_reset(lv_style_t * style);

// Apply to object
void lv_obj_add_style(lv_obj_t * obj, lv_style_t * style, lv_style_selector_t selector);
void lv_obj_remove_style(lv_obj_t * obj, lv_style_t * style, lv_style_selector_t selector);
void lv_obj_remove_style_all(lv_obj_t * obj);

// Refresh
void lv_obj_refresh_style(lv_obj_t * obj, lv_style_selector_t selector, lv_style_prop_t prop);
```

### Style Selector

Combines a part and a state: `LV_PART_MAIN | LV_STATE_DEFAULT`

### Widget Parts

| Part | Description |
|------|-------------|
| `LV_PART_MAIN` | Background-like main part |
| `LV_PART_SCROLLBAR` | Scrollbar |
| `LV_PART_INDICATOR` | Indicator (bar, slider, switch) |
| `LV_PART_KNOB` | Draggable knob (slider, arc) |
| `LV_PART_SELECTED` | Currently selected item |
| `LV_PART_ITEMS` | Items in containers |
| `LV_PART_CURSOR` | Cursor in text area |
| `LV_PART_TICKS` | Scale ticks |
| `LV_PART_CUSTOM_1..8` | Custom parts |

### Style Properties - Size and Position

```c
lv_style_set_width(style, value);           // int32_t
lv_style_set_min_width(style, value);       // int32_t
lv_style_set_max_width(style, value);       // int32_t
lv_style_set_height(style, value);          // int32_t
lv_style_set_min_height(style, value);      // int32_t
lv_style_set_max_height(style, value);      // int32_t
lv_style_set_length(style, value);          // int32_t
lv_style_set_x(style, value);              // int32_t
lv_style_set_y(style, value);              // int32_t
lv_style_set_align(style, value);          // lv_align_t
lv_style_set_transform_width(style, value); // int32_t
lv_style_set_transform_height(style, value);// int32_t
lv_style_set_translate_x(style, value);     // int32_t
lv_style_set_translate_y(style, value);     // int32_t
lv_style_set_transform_scale_x(style, value);  // int32_t (256 = 1.0)
lv_style_set_transform_scale_y(style, value);  // int32_t (256 = 1.0)
lv_style_set_transform_rotation(style, value); // int32_t (0.1 degree units)
lv_style_set_transform_pivot_x(style, value);  // int32_t
lv_style_set_transform_pivot_y(style, value);  // int32_t
lv_style_set_transform_skew_x(style, value);   // int32_t
lv_style_set_transform_skew_y(style, value);   // int32_t
```

### Style Properties - Padding and Margin

```c
lv_style_set_pad_top(style, value);         // int32_t
lv_style_set_pad_bottom(style, value);      // int32_t
lv_style_set_pad_left(style, value);        // int32_t
lv_style_set_pad_right(style, value);       // int32_t
lv_style_set_pad_row(style, value);         // int32_t (row gap)
lv_style_set_pad_column(style, value);      // int32_t (column gap)
lv_style_set_margin_top(style, value);      // int32_t
lv_style_set_margin_bottom(style, value);   // int32_t
lv_style_set_margin_left(style, value);     // int32_t
lv_style_set_margin_right(style, value);    // int32_t
```

### Style Properties - Background

```c
lv_style_set_bg_color(style, color);        // lv_color_t
lv_style_set_bg_opa(style, opa);            // lv_opa_t (0-255, LV_OPA_TRANSP..LV_OPA_COVER)
lv_style_set_bg_grad_color(style, color);   // lv_color_t
lv_style_set_bg_grad_dir(style, dir);       // LV_GRAD_DIR_NONE/VER/HOR
lv_style_set_bg_main_stop(style, value);    // int32_t (0-255)
lv_style_set_bg_grad_stop(style, value);    // int32_t (0-255)
lv_style_set_bg_grad(style, grad_dsc);      // const lv_grad_dsc_t *
lv_style_set_bg_image_src(style, src);      // const void * (image path or symbol)
lv_style_set_bg_image_opa(style, opa);      // lv_opa_t
lv_style_set_bg_image_recolor(style, color);// lv_color_t
lv_style_set_bg_image_recolor_opa(style, opa);// lv_opa_t
lv_style_set_bg_image_tiled(style, en);     // bool
```

### Style Properties - Border

```c
lv_style_set_border_color(style, color);    // lv_color_t
lv_style_set_border_opa(style, opa);        // lv_opa_t
lv_style_set_border_width(style, value);    // int32_t
lv_style_set_border_side(style, side);      // lv_border_side_t
lv_style_set_border_post(style, en);        // bool (draw after children)
```

### Style Properties - Outline

```c
lv_style_set_outline_width(style, value);   // int32_t
lv_style_set_outline_color(style, color);   // lv_color_t
lv_style_set_outline_opa(style, opa);       // lv_opa_t
lv_style_set_outline_pad(style, value);     // int32_t (gap from border)
```

### Style Properties - Shadow

```c
lv_style_set_shadow_width(style, value);    // int32_t (blur radius)
lv_style_set_shadow_offset_x(style, value); // int32_t
lv_style_set_shadow_offset_y(style, value); // int32_t
lv_style_set_shadow_spread(style, value);   // int32_t
lv_style_set_shadow_color(style, color);    // lv_color_t
lv_style_set_shadow_opa(style, opa);        // lv_opa_t
```

### Style Properties - Image

```c
lv_style_set_image_opa(style, opa);         // lv_opa_t
lv_style_set_image_recolor(style, color);   // lv_color_t
lv_style_set_image_recolor_opa(style, opa); // lv_opa_t
```

### Style Properties - Line

```c
lv_style_set_line_width(style, value);      // int32_t
lv_style_set_line_dash_width(style, value); // int32_t
lv_style_set_line_dash_gap(style, value);   // int32_t
lv_style_set_line_rounded(style, en);       // bool
lv_style_set_line_color(style, color);      // lv_color_t
lv_style_set_line_opa(style, opa);          // lv_opa_t
```

### Style Properties - Arc

```c
lv_style_set_arc_width(style, value);       // int32_t
lv_style_set_arc_rounded(style, en);        // bool
lv_style_set_arc_color(style, color);       // lv_color_t
lv_style_set_arc_opa(style, opa);           // lv_opa_t
lv_style_set_arc_image_src(style, src);     // const void *
```

### Style Properties - Text

```c
lv_style_set_text_color(style, color);      // lv_color_t
lv_style_set_text_opa(style, opa);          // lv_opa_t
lv_style_set_text_font(style, font);        // const lv_font_t *
lv_style_set_text_letter_space(style, value);// int32_t
lv_style_set_text_line_space(style, value); // int32_t
lv_style_set_text_decor(style, decor);      // lv_text_decor_t
lv_style_set_text_align(style, align);      // lv_text_align_t
```

### Style Properties - Blur (New in v9.5)

```c
lv_style_set_blur(style, value);            // int32_t (blur radius)
```

### Style Properties - Miscellaneous

```c
lv_style_set_radius(style, value);          // int32_t (LV_RADIUS_CIRCLE for full)
lv_style_set_clip_corner(style, en);        // bool
lv_style_set_opa(style, opa);              // lv_opa_t (overall opacity)
lv_style_set_color_filter_dsc(style, dsc);  // const lv_color_filter_dsc_t *
lv_style_set_color_filter_opa(style, opa);  // lv_opa_t
lv_style_set_anim(style, anim);            // const lv_anim_t *
lv_style_set_anim_duration(style, value);   // uint32_t (ms)
lv_style_set_blend_mode(style, mode);       // lv_blend_mode_t
lv_style_set_layout(style, layout);         // uint16_t
lv_style_set_base_dir(style, dir);          // lv_base_dir_t (LTR/RTL/AUTO)
```

### Style Properties - Flex Layout

```c
lv_style_set_flex_flow(style, flow);        // lv_flex_flow_t
lv_style_set_flex_main_place(style, place); // lv_flex_align_t
lv_style_set_flex_cross_place(style, place);// lv_flex_align_t
lv_style_set_flex_track_place(style, place);// lv_flex_align_t
lv_style_set_flex_grow(style, value);       // uint8_t
```

### Style Properties - Grid Layout

```c
lv_style_set_grid_column_dsc_array(style, arr);  // const int32_t *
lv_style_set_grid_row_dsc_array(style, arr);     // const int32_t *
lv_style_set_grid_column_align(style, align);    // lv_grid_align_t
lv_style_set_grid_row_align(style, align);       // lv_grid_align_t
lv_style_set_grid_cell_column_pos(style, pos);   // int32_t
lv_style_set_grid_cell_column_span(style, span); // int32_t
lv_style_set_grid_cell_row_pos(style, pos);      // int32_t
lv_style_set_grid_cell_row_span(style, span);    // int32_t
lv_style_set_grid_cell_x_align(style, align);    // lv_grid_align_t
lv_style_set_grid_cell_y_align(style, align);    // lv_grid_align_t
```

### Local Styles (Applied Directly to Object)

Every `lv_style_set_*` has a corresponding `lv_obj_set_style_*`:

```c
lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN);
// etc.
```

### Style Binding (New in v9.5)

```c
// Bind a style property to a subject for data-driven updates
void lv_obj_bind_style_prop(lv_obj_t * obj, lv_style_prop_t prop, lv_subject_t * subject);
```

---

## Event API

### Adding Event Handlers

```c
void lv_obj_add_event_cb(lv_obj_t * obj, lv_event_cb_t cb,
                          lv_event_code_t filter, void * user_data);
uint32_t lv_obj_get_event_count(lv_obj_t * obj);
bool lv_obj_remove_event_cb(lv_obj_t * obj, lv_event_cb_t cb);
void lv_obj_send_event(lv_obj_t * obj, lv_event_code_t code, void * param);
```

### Event Callback Signature

```c
void my_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    void * user_data = lv_event_get_user_data(e);
    void * param = lv_event_get_param(e);
}
```

### All Event Types

**Input Events:**

| Event | Description |
|-------|-------------|
| `LV_EVENT_PRESSED` | Widget pressed |
| `LV_EVENT_PRESSING` | Continuously pressing |
| `LV_EVENT_PRESS_LOST` | Cursor left widget while pressed |
| `LV_EVENT_SHORT_CLICKED` | Brief press, no scroll |
| `LV_EVENT_SINGLE_CLICKED` | First click in streak |
| `LV_EVENT_DOUBLE_CLICKED` | Second click in streak |
| `LV_EVENT_TRIPLE_CLICKED` | Third click in streak |
| `LV_EVENT_LONG_PRESSED` | Held >= long_press_time |
| `LV_EVENT_LONG_PRESSED_REPEAT` | Repeated during long press |
| `LV_EVENT_CLICKED` | Released, no scroll |
| `LV_EVENT_RELEASED` | Released (all cases) |
| `LV_EVENT_SCROLL_BEGIN` | Scroll started |
| `LV_EVENT_SCROLL_THROW_BEGIN` | Momentum scroll started |
| `LV_EVENT_SCROLL_END` | Scroll ended |
| `LV_EVENT_SCROLL` | Scrolling |
| `LV_EVENT_GESTURE` | Gesture detected |
| `LV_EVENT_KEY` | Key sent to widget |
| `LV_EVENT_FOCUSED` | Gained focus |
| `LV_EVENT_DEFOCUSED` | Lost focus |
| `LV_EVENT_LEAVE` | Defocused but still selected |
| `LV_EVENT_HIT_TEST` | Advanced hit testing |
| `LV_EVENT_INDEV_RESET` | Input device reset |
| `LV_EVENT_HOVER_OVER` | Pointer entered widget |
| `LV_EVENT_HOVER_LEAVE` | Pointer left widget |

**Drawing Events:**

| Event | Description |
|-------|-------------|
| `LV_EVENT_COVER_CHECK` | Check if widget covers an area |
| `LV_EVENT_REFR_EXT_DRAW_SIZE` | Extra draw area needed |
| `LV_EVENT_DRAW_MAIN_BEGIN` | Main draw phase starting |
| `LV_EVENT_DRAW_MAIN` | Main draw execution |
| `LV_EVENT_DRAW_MAIN_END` | Main draw phase ending |
| `LV_EVENT_DRAW_POST_BEGIN` | Post-draw starting |
| `LV_EVENT_DRAW_POST` | Post-draw execution |
| `LV_EVENT_DRAW_POST_END` | Post-draw ending |
| `LV_EVENT_DRAW_TASK_ADDED` | Draw task queued |

**Special Events:**

| Event | Description |
|-------|-------------|
| `LV_EVENT_VALUE_CHANGED` | Widget value modified |
| `LV_EVENT_INSERT` | Text inserted |
| `LV_EVENT_REFRESH` | User refresh signal |
| `LV_EVENT_READY` | Process completed |
| `LV_EVENT_CANCEL` | Process cancelled |
| `LV_EVENT_STATE_CHANGED` | Widget state changed |

**Lifecycle Events:**

| Event | Description |
|-------|-------------|
| `LV_EVENT_CREATE` | Widget created |
| `LV_EVENT_DELETE` | Widget being deleted |
| `LV_EVENT_CHILD_CHANGED` | Child modified |
| `LV_EVENT_CHILD_CREATED` | Child created |
| `LV_EVENT_CHILD_DELETED` | Child deleted |
| `LV_EVENT_SIZE_CHANGED` | Size/coords changed |
| `LV_EVENT_STYLE_CHANGED` | Style updated |
| `LV_EVENT_LAYOUT_CHANGED` | Layout recalculated |
| `LV_EVENT_GET_SELF_SIZE` | Internal size request |

**Screen Events:**

| Event | Description |
|-------|-------------|
| `LV_EVENT_SCREEN_UNLOAD_START` | Screen unload starting |
| `LV_EVENT_SCREEN_LOAD_START` | Screen load starting |
| `LV_EVENT_SCREEN_LOADED` | Screen load complete |
| `LV_EVENT_SCREEN_UNLOADED` | Screen unload complete |

**Display Events:**

| Event | Description |
|-------|-------------|
| `LV_EVENT_INVALIDATE_AREA` | Area marked for redraw |
| `LV_EVENT_RESOLUTION_CHANGED` | Resolution changed |
| `LV_EVENT_COLOR_FORMAT_CHANGED` | Color format changed |
| `LV_EVENT_REFR_REQUEST` | Redraw needed |
| `LV_EVENT_REFR_START` | Refresh cycle starting |
| `LV_EVENT_REFR_READY` | Refresh cycle done |
| `LV_EVENT_RENDER_START` | Rendering starting |
| `LV_EVENT_RENDER_READY` | Rendering complete |
| `LV_EVENT_FLUSH_START` | Flush starting |
| `LV_EVENT_FLUSH_FINISH` | Flush complete |
| `LV_EVENT_FLUSH_WAIT_START` | Flush wait starting |
| `LV_EVENT_FLUSH_WAIT_FINISH` | Flush wait done |

**Custom Events:**

```c
uint32_t my_event_id = lv_event_register_id();
lv_obj_send_event(obj, my_event_id, param);
```

---

## Animation API

```c
// Initialize
void lv_anim_init(lv_anim_t * a);

// Configure
void lv_anim_set_var(lv_anim_t * a, void * var);
void lv_anim_set_exec_cb(lv_anim_t * a, lv_anim_exec_xcb_t exec_cb);
void lv_anim_set_values(lv_anim_t * a, int32_t start, int32_t end);
void lv_anim_set_duration(lv_anim_t * a, uint32_t duration);
void lv_anim_set_delay(lv_anim_t * a, uint32_t delay);
void lv_anim_set_playback_duration(lv_anim_t * a, uint32_t duration);
void lv_anim_set_playback_delay(lv_anim_t * a, uint32_t delay);
void lv_anim_set_repeat_count(lv_anim_t * a, uint16_t cnt);  // LV_ANIM_REPEAT_INFINITE
void lv_anim_set_repeat_delay(lv_anim_t * a, uint32_t delay);
void lv_anim_set_path_cb(lv_anim_t * a, lv_anim_path_cb_t path_cb);
void lv_anim_set_ready_cb(lv_anim_t * a, lv_anim_ready_cb_t ready_cb);
void lv_anim_set_deleted_cb(lv_anim_t * a, lv_anim_deleted_cb_t deleted_cb);
void lv_anim_set_early_apply(lv_anim_t * a, bool en);

// Start
lv_anim_t * lv_anim_start(lv_anim_t * a);

// Control
bool lv_anim_delete(void * var, lv_anim_exec_xcb_t exec_cb);
void lv_anim_delete_all(void);

// Path callbacks
lv_anim_path_linear
lv_anim_path_ease_in
lv_anim_path_ease_out
lv_anim_path_ease_in_out
lv_anim_path_overshoot
lv_anim_path_bounce
lv_anim_path_step
```

---

## Timer API

```c
lv_timer_t * lv_timer_create(lv_timer_cb_t cb, uint32_t period_ms, void * user_data);
void lv_timer_delete(lv_timer_t * timer);
void lv_timer_pause(lv_timer_t * timer);
void lv_timer_resume(lv_timer_t * timer);
void lv_timer_set_period(lv_timer_t * timer, uint32_t period_ms);
void lv_timer_set_cb(lv_timer_t * timer, lv_timer_cb_t cb);
void lv_timer_ready(lv_timer_t * timer);  // Execute on next call
void lv_timer_reset(lv_timer_t * timer);  // Reset period counter

// Main handler (call periodically unless using esp_lvgl_port)
uint32_t lv_timer_handler(void);

// Tick (must be called periodically, e.g., from ISR)
void lv_tick_inc(uint32_t tick_period_ms);

// Or set tick callback
void lv_tick_set_cb(lv_tick_get_cb_t cb);
```

---

## Theme API

```c
// Create/apply theme
lv_theme_t * lv_theme_default_init(lv_display_t * disp, lv_color_t primary,
                                    lv_color_t secondary, bool dark,
                                    const lv_font_t * font);
void lv_display_set_theme(lv_display_t * disp, lv_theme_t * theme);

// Theme management (new in v9.5)
void lv_obj_remove_theme(lv_obj_t * obj);
```

---

## Group API

```c
lv_group_t * lv_group_create(void);
void lv_group_delete(lv_group_t * group);
void lv_group_set_default(lv_group_t * group);
lv_group_t * lv_group_get_default(void);
void lv_group_add_obj(lv_group_t * group, lv_obj_t * obj);
void lv_group_remove_obj(lv_obj_t * obj);
void lv_group_focus_obj(lv_obj_t * obj);
void lv_group_focus_next(lv_group_t * group);
void lv_group_focus_prev(lv_group_t * group);
lv_obj_t * lv_group_get_focused(lv_group_t * group);
void lv_indev_set_group(lv_indev_t * indev, lv_group_t * group);

// User data (new in v9.5)
void lv_group_set_user_data(lv_group_t * group, void * user_data);
void * lv_group_get_user_data(lv_group_t * group);
```

---

## Memory API

```c
void * lv_malloc(size_t size);
void * lv_realloc(void * ptr, size_t size);
void lv_free(void * ptr);
void * lv_malloc_zeroed(size_t size);
void lv_memcpy(void * dst, const void * src, size_t len);
void lv_memset(void * dst, uint8_t val, size_t len);
lv_result_t lv_mem_test(void);
void lv_mem_monitor(lv_mem_monitor_t * mon);
```

---

## Color API

```c
// Create colors
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b);
lv_color_t lv_color_hex(uint32_t hex);      // e.g., lv_color_hex(0xFF0000)
lv_color_t lv_color_hex3(uint16_t hex);     // e.g., lv_color_hex3(0xF00)

// Built-in colors
lv_color_white()
lv_color_black()

// Palette
lv_palette_main(LV_PALETTE_RED)   // LV_PALETTE_RED..DEEP_ORANGE, GREY, BLUE_GREY, NONE
lv_palette_lighten(palette, level) // level: 1-5
lv_palette_darken(palette, level)  // level: 1-4

// Opacity constants
LV_OPA_TRANSP  // 0
LV_OPA_0       // 0
LV_OPA_10      // 25
LV_OPA_20      // 51
LV_OPA_30      // 76
LV_OPA_40      // 102
LV_OPA_50      // 127
LV_OPA_60      // 153
LV_OPA_70      // 178
LV_OPA_80      // 204
LV_OPA_90      // 229
LV_OPA_100     // 255
LV_OPA_COVER   // 255
```

---

## Layout API

### Flex Layout

```c
void lv_obj_set_flex_flow(lv_obj_t * obj, lv_flex_flow_t flow);
void lv_obj_set_flex_align(lv_obj_t * obj,
                            lv_flex_align_t main_place,
                            lv_flex_align_t cross_place,
                            lv_flex_align_t track_cross_place);
void lv_obj_set_flex_grow(lv_obj_t * obj, uint8_t grow);
```

**Flow values:**
```
LV_FLEX_FLOW_ROW / ROW_WRAP / ROW_REVERSE / ROW_WRAP_REVERSE
LV_FLEX_FLOW_COLUMN / COLUMN_WRAP / COLUMN_REVERSE / COLUMN_WRAP_REVERSE
```

**Align values:**
```
LV_FLEX_ALIGN_START / END / CENTER / SPACE_EVENLY / SPACE_AROUND / SPACE_BETWEEN
```

### Grid Layout

```c
// Column/row descriptors (terminated with LV_GRID_TEMPLATE_LAST)
static int32_t col_dsc[] = {100, 200, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static int32_t row_dsc[] = {50, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

void lv_obj_set_grid_dsc_array(lv_obj_t * obj, const int32_t * col, const int32_t * row);
void lv_obj_set_grid_align(lv_obj_t * obj, lv_grid_align_t col_align, lv_grid_align_t row_align);
void lv_obj_set_grid_cell(lv_obj_t * obj, lv_grid_align_t col_align, uint8_t col_pos,
                           uint8_t col_span, lv_grid_align_t row_align, uint8_t row_pos,
                           uint8_t row_span);
```

**Grid align values:**
```
LV_GRID_ALIGN_START / END / CENTER / STRETCH / SPACE_EVENLY / SPACE_AROUND / SPACE_BETWEEN
```

**Grid size helpers:**
```
LV_GRID_FR(x)          // Fractional unit
LV_GRID_CONTENT        // Size to content
LV_GRID_TEMPLATE_LAST  // Array terminator
```

---

## OS Abstraction API

### Mutex

```c
lv_result_t lv_mutex_init(lv_mutex_t * mutex);
lv_result_t lv_mutex_lock(lv_mutex_t * mutex);
lv_result_t lv_mutex_lock_isr(lv_mutex_t * mutex);
lv_result_t lv_mutex_unlock(lv_mutex_t * mutex);
lv_result_t lv_mutex_delete(lv_mutex_t * mutex);
```

### Thread

```c
lv_result_t lv_thread_init(lv_thread_t * thread, lv_thread_prio_t prio,
                            lv_thread_func_t func, size_t stack_size, void * user_data);
lv_result_t lv_thread_delete(lv_thread_t * thread);
```

### Thread Sync

```c
lv_result_t lv_thread_sync_init(lv_thread_sync_t * sync);
lv_result_t lv_thread_sync_wait(lv_thread_sync_t * sync);
lv_result_t lv_thread_sync_signal(lv_thread_sync_t * sync);
lv_result_t lv_thread_sync_delete(lv_thread_sync_t * sync);
```

---

## Key Constants

```
LV_KEY_UP / DOWN / LEFT / RIGHT
LV_KEY_ESC / DEL / BACKSPACE / ENTER
LV_KEY_NEXT / PREV / HOME / END
```

## Direction Constants

```
LV_DIR_NONE / LEFT / RIGHT / TOP / BOTTOM
LV_DIR_HOR    // LEFT | RIGHT
LV_DIR_VER    // TOP | BOTTOM
LV_DIR_ALL    // LEFT | RIGHT | TOP | BOTTOM
```

---

## Source Links

- lv_conf.h reference: https://docs.lvgl.io/9.5/API/lv_conf_h.html
- Display API: https://docs.lvgl.io/master/API/display/lv_display_h.html
- Object API: https://docs.lvgl.io/master/API/core/lv_obj_h.html
- Event API: https://docs.lvgl.io/master/API/misc/lv_event_h.html
- Style properties: https://docs.lvgl.io/9.5/common-widget-features/styles.html
