# LVGL v8.2 API Reference

> Detailed function signatures for display drivers, widgets, styles, events, animations, and layouts.
> For overview and configuration, see [README.md](./README.md).

---

## Table of Contents

1. [Display Driver API](#1-display-driver-api)
2. [Input Device Driver API](#2-input-device-driver-api)
3. [Base Object API](#3-base-object-api)
4. [Core Widget APIs](#4-core-widget-apis)
5. [Extra Widget APIs](#5-extra-widget-apis)
6. [Style API](#6-style-api)
7. [Event API](#7-event-api)
8. [Animation API](#8-animation-api)
9. [Layout APIs (Flex and Grid)](#9-layout-apis)
10. [Color and Drawing API](#10-color-and-drawing-api)
11. [File System API](#11-file-system-api)
12. [Timer API](#12-timer-api)
13. [Font API](#13-font-api)
14. [Memory API](#14-memory-api)

---

## 1. Display Driver API

Header: `lv_hal_disp.h`

### Structures

#### lv_disp_draw_buf_t

Holds one or two pixel buffers for rendering.

```c
typedef struct {
    void * buf1;              // First buffer (mandatory)
    void * buf2;              // Second buffer (optional, enables DMA double-buffering)
    void * buf_act;           // Currently active buffer
    uint32_t size;            // Buffer size in pixels (not bytes)
    lv_area_t area;           // Current flush area
    volatile int flushing;    // 1 while flush_cb is running
    volatile int flushing_last; // 1 if this is the last chunk to flush
} lv_disp_draw_buf_t;
```

#### lv_disp_drv_t (key fields)

```c
typedef struct _lv_disp_drv_t {
    lv_coord_t hor_res;
    lv_coord_t ver_res;
    lv_disp_draw_buf_t * draw_buf;

    // Callbacks
    void (*flush_cb)(struct _lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
    void (*rounder_cb)(struct _lv_disp_drv_t * disp_drv, lv_area_t * area);
    void (*set_px_cb)(struct _lv_disp_drv_t * disp_drv, uint8_t * buf, lv_coord_t buf_w,
                      lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa);
    void (*monitor_cb)(struct _lv_disp_drv_t * disp_drv, uint32_t time, uint32_t px);
    void (*wait_cb)(struct _lv_disp_drv_t * disp_drv);
    void (*clean_dcache_cb)(struct _lv_disp_drv_t * disp_drv);
    void (*gpu_fill_cb)(struct _lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
                        const lv_area_t * fill_area, lv_color_t color);

    // Configuration
    uint32_t direct_mode  : 1;
    uint32_t full_refresh : 1;
    uint32_t sw_rotate    : 1;
    uint32_t antialiasing : 1;
    lv_disp_rot_t rotated;
    uint32_t screen_transp : 1;
    uint32_t dpi;
    lv_color_t color_chroma_key;
    void * user_data;
} lv_disp_drv_t;
```

### Functions

```c
/**
 * Initialize a display draw buffer.
 * @param draw_buf   pointer to lv_disp_draw_buf_t to initialize
 * @param buf1       first buffer (mandatory), array of lv_color_t
 * @param buf2       second buffer (NULL if not used), same size as buf1
 * @param size_in_px_cnt  buffer size in pixel count (e.g., hor_res * 10)
 */
void lv_disp_draw_buf_init(lv_disp_draw_buf_t * draw_buf,
                           void * buf1, void * buf2,
                           uint32_t size_in_px_cnt);

/**
 * Initialize a display driver with default values.
 * @param driver   pointer to lv_disp_drv_t to initialize
 */
void lv_disp_drv_init(lv_disp_drv_t * driver);

/**
 * Register a display driver. Creates an lv_disp_t object.
 * First registered display becomes the default.
 * @param driver   pointer to initialized and configured lv_disp_drv_t
 * @return         pointer to created lv_disp_t (display handle)
 */
lv_disp_t * lv_disp_drv_register(lv_disp_drv_t * driver);

/**
 * Update display driver at runtime (e.g., resolution change).
 * @param disp     display handle
 * @param new_drv  pointer to new driver configuration
 */
void lv_disp_drv_update(lv_disp_t * disp, lv_disp_drv_t * new_drv);

/**
 * Remove a display and free its resources.
 * @param disp   display handle to remove
 */
void lv_disp_remove(lv_disp_t * disp);

/**
 * MUST be called at the end of flush_cb to signal completion.
 * @param disp_drv   driver pointer passed to flush_cb
 */
void lv_disp_flush_ready(lv_disp_drv_t * disp_drv);

/**
 * Tell LVGL that flushing is in progress (for async DMA).
 * @param disp_drv   driver pointer
 * @return           true if still flushing
 */
bool lv_disp_flush_is_last(lv_disp_drv_t * disp_drv);

// Display getters/setters
lv_disp_t * lv_disp_get_default(void);
void lv_disp_set_default(lv_disp_t * disp);
lv_coord_t lv_disp_get_hor_res(lv_disp_t * disp);
lv_coord_t lv_disp_get_ver_res(lv_disp_t * disp);
bool lv_disp_get_antialiasing(lv_disp_t * disp);
lv_coord_t lv_disp_get_dpi(const lv_disp_t * disp);
void lv_disp_set_rotation(lv_disp_t * disp, lv_disp_rot_t rotation);
lv_disp_rot_t lv_disp_get_rotation(lv_disp_t * disp);

// Display background
void lv_disp_set_bg_color(lv_disp_t * disp, lv_color_t color);
void lv_disp_set_bg_image(lv_disp_t * disp, const void * img);
void lv_disp_set_bg_opa(lv_disp_t * disp, lv_opa_t opa);

// Screen management
lv_obj_t * lv_disp_get_scr_act(lv_disp_t * disp);
lv_obj_t * lv_disp_get_scr_prev(lv_disp_t * disp);
void lv_disp_load_scr(lv_obj_t * scr);
lv_obj_t * lv_disp_get_layer_top(lv_disp_t * disp);
lv_obj_t * lv_disp_get_layer_sys(lv_disp_t * disp);
void lv_disp_set_theme(lv_disp_t * disp, lv_theme_t * th);
lv_theme_t * lv_disp_get_theme(lv_disp_t * disp);
```

### Rotation Constants

```c
LV_DISP_ROT_NONE   // 0 degrees
LV_DISP_ROT_90     // 90 degrees clockwise
LV_DISP_ROT_180    // 180 degrees
LV_DISP_ROT_270    // 270 degrees clockwise
```

---

## 2. Input Device Driver API

Header: `lv_hal_indev.h`

### Functions

```c
/**
 * Initialize an input device driver with default values.
 * @param driver   pointer to lv_indev_drv_t to initialize
 */
void lv_indev_drv_init(lv_indev_drv_t * driver);

/**
 * Register an input device driver.
 * @param driver   pointer to initialized lv_indev_drv_t
 * @return         pointer to created lv_indev_t (input device handle)
 */
lv_indev_t * lv_indev_drv_register(lv_indev_drv_t * driver);

/**
 * Update driver configuration at runtime.
 * @param indev    input device handle
 * @param new_drv  pointer to new driver configuration
 */
void lv_indev_drv_update(lv_indev_t * indev, lv_indev_drv_t * new_drv);

/**
 * Delete an input device.
 * @param indev   input device handle to delete
 */
void lv_indev_delete(lv_indev_t * indev);

/**
 * Get next registered input device (NULL to start).
 * @param indev   current device (NULL for first)
 * @return        next device or NULL
 */
lv_indev_t * lv_indev_get_next(lv_indev_t * indev);

// Pointer-specific
void lv_indev_set_cursor(lv_indev_t * indev, lv_obj_t * cur_obj);

// Keypad/encoder-specific
void lv_indev_set_group(lv_indev_t * indev, lv_group_t * group);

// Button-specific
void lv_indev_set_button_points(lv_indev_t * indev, const lv_point_t points[]);

// Getters
lv_indev_type_t lv_indev_get_type(const lv_indev_t * indev);
void lv_indev_get_point(const lv_indev_t * indev, lv_point_t * point);
lv_dir_t lv_indev_get_gesture_dir(const lv_indev_t * indev);
uint32_t lv_indev_get_key(const lv_indev_t * indev);
lv_dir_t lv_indev_get_scroll_dir(const lv_indev_t * indev);
lv_obj_t * lv_indev_get_scroll_obj(const lv_indev_t * indev);
void lv_indev_get_vect(const lv_indev_t * indev, lv_point_t * point);
void lv_indev_wait_release(lv_indev_t * indev);
lv_obj_t * lv_indev_get_obj_act(void);
lv_indev_t * lv_indev_get_act(void);
```

### lv_indev_drv_t Key Fields

```c
typedef struct _lv_indev_drv_t {
    lv_indev_type_t type;                // LV_INDEV_TYPE_POINTER/KEYPAD/ENCODER/BUTTON
    void (*read_cb)(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
    void (*feedback_cb)(struct _lv_indev_drv_t * indev_drv, uint8_t event_code);
    lv_disp_t * disp;                    // Associated display (NULL = default)
    lv_timer_t * read_timer;             // Read timer handle
    uint8_t scroll_limit;                // Pixels before scroll gesture
    uint8_t scroll_throw;                // Scroll momentum (lower = more momentum)
    uint8_t gesture_min_velocity;        // Minimum velocity for gesture detection
    uint8_t gesture_limit;               // Gesture threshold
    uint16_t long_press_time;            // Long press threshold (ms)
    uint16_t long_press_repeat_time;     // Long press repeat interval (ms)
    void * user_data;
} lv_indev_drv_t;
```

### lv_indev_data_t

```c
typedef struct {
    lv_point_t point;           // For POINTER: current position
    uint32_t key;               // For KEYPAD: current key (LV_KEY_*)
    uint32_t btn_id;            // For BUTTON: button index
    int16_t enc_diff;           // For ENCODER: steps since last read
    lv_indev_state_t state;     // LV_INDEV_STATE_PRESSED or LV_INDEV_STATE_RELEASED
    bool continue_reading;      // true = more buffered data, call read_cb again immediately
} lv_indev_data_t;
```

### Key Codes (LV_KEY_*)

| Constant | Value | Description |
|----------|-------|-------------|
| `LV_KEY_UP` | 17 | Navigate up |
| `LV_KEY_DOWN` | 18 | Navigate down |
| `LV_KEY_RIGHT` | 19 | Navigate right |
| `LV_KEY_LEFT` | 20 | Navigate left |
| `LV_KEY_ESC` | 27 | Escape/back |
| `LV_KEY_DEL` | 127 | Delete forward |
| `LV_KEY_BACKSPACE` | 8 | Delete backward |
| `LV_KEY_ENTER` | 10 | Confirm/select |
| `LV_KEY_NEXT` | 9 | Focus next (Tab) |
| `LV_KEY_PREV` | 11 | Focus previous |
| `LV_KEY_HOME` | 2 | Go to beginning |
| `LV_KEY_END` | 3 | Go to end |

### Group API

```c
lv_group_t * lv_group_create(void);
void lv_group_del(lv_group_t * group);
void lv_group_set_default(lv_group_t * group);
lv_group_t * lv_group_get_default(void);
void lv_group_add_obj(lv_group_t * group, lv_obj_t * obj);
void lv_group_remove_obj(lv_obj_t * obj);
void lv_group_remove_all_objs(lv_group_t * group);
void lv_group_focus_obj(lv_obj_t * obj);
void lv_group_focus_next(lv_group_t * group);
void lv_group_focus_prev(lv_group_t * group);
void lv_group_focus_freeze(lv_group_t * group, bool en);
lv_obj_t * lv_group_get_focused(const lv_group_t * group);
uint32_t lv_group_get_obj_count(lv_group_t * group);
void lv_group_set_editing(lv_group_t * group, bool edit);
bool lv_group_get_editing(const lv_group_t * group);
void lv_group_set_wrap(lv_group_t * group, bool en);
bool lv_group_get_wrap(lv_group_t * group);
```

---

## 3. Base Object API

Header: `lv_obj.h`

### Creation and Deletion

```c
lv_obj_t * lv_obj_create(lv_obj_t * parent);  // NULL parent = new screen
void lv_obj_del(lv_obj_t * obj);
void lv_obj_del_async(lv_obj_t * obj);
void lv_obj_del_delayed(lv_obj_t * obj, uint32_t delay_ms);
void lv_obj_clean(lv_obj_t * obj);             // Delete all children
```

### Flags

```c
void lv_obj_add_flag(lv_obj_t * obj, lv_obj_flag_t f);
void lv_obj_clear_flag(lv_obj_t * obj, lv_obj_flag_t f);
bool lv_obj_has_flag(lv_obj_t * obj, lv_obj_flag_t f);
bool lv_obj_has_flag_any(lv_obj_t * obj, lv_obj_flag_t f);
```

### States

```c
void lv_obj_add_state(lv_obj_t * obj, lv_state_t state);
void lv_obj_clear_state(lv_obj_t * obj, lv_state_t state);
lv_state_t lv_obj_get_state(const lv_obj_t * obj);
bool lv_obj_has_state(const lv_obj_t * obj, lv_state_t state);
```

### Position and Size

```c
void lv_obj_set_pos(lv_obj_t * obj, lv_coord_t x, lv_coord_t y);
void lv_obj_set_x(lv_obj_t * obj, lv_coord_t x);
void lv_obj_set_y(lv_obj_t * obj, lv_coord_t y);
void lv_obj_set_size(lv_obj_t * obj, lv_coord_t w, lv_coord_t h);
void lv_obj_set_width(lv_obj_t * obj, lv_coord_t w);
void lv_obj_set_height(lv_obj_t * obj, lv_coord_t h);
void lv_obj_set_content_width(lv_obj_t * obj, lv_coord_t w);
void lv_obj_set_content_height(lv_obj_t * obj, lv_coord_t h);
void lv_obj_set_layout(lv_obj_t * obj, uint32_t layout);  // LV_LAYOUT_FLEX or LV_LAYOUT_GRID

lv_coord_t lv_obj_get_x(const lv_obj_t * obj);
lv_coord_t lv_obj_get_y(const lv_obj_t * obj);
lv_coord_t lv_obj_get_x_aligned(const lv_obj_t * obj);
lv_coord_t lv_obj_get_y_aligned(const lv_obj_t * obj);
lv_coord_t lv_obj_get_width(const lv_obj_t * obj);
lv_coord_t lv_obj_get_height(const lv_obj_t * obj);
lv_coord_t lv_obj_get_content_width(const lv_obj_t * obj);
lv_coord_t lv_obj_get_content_height(const lv_obj_t * obj);
void lv_obj_get_content_coords(const lv_obj_t * obj, lv_area_t * area);
lv_coord_t lv_obj_get_self_width(const lv_obj_t * obj);
lv_coord_t lv_obj_get_self_height(const lv_obj_t * obj);
```

### Alignment

```c
/**
 * Align object within parent.
 * @param obj    object to align
 * @param align  alignment anchor (LV_ALIGN_*)
 * @param x_ofs  X offset from aligned position
 * @param y_ofs  Y offset from aligned position
 */
void lv_obj_align(lv_obj_t * obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);

/**
 * Align object relative to another object.
 */
void lv_obj_align_to(lv_obj_t * obj, const lv_obj_t * base, lv_align_t align,
                     lv_coord_t x_ofs, lv_coord_t y_ofs);

void lv_obj_set_align(lv_obj_t * obj, lv_align_t align);
lv_align_t lv_obj_get_align(const lv_obj_t * obj);

void lv_obj_center(lv_obj_t * obj);  // Shorthand for lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0)
```

#### Alignment Constants (LV_ALIGN_*)

| Constant | Description |
|----------|-------------|
| `LV_ALIGN_DEFAULT` | Use default alignment |
| `LV_ALIGN_TOP_LEFT` | Top-left of parent |
| `LV_ALIGN_TOP_MID` | Top-center of parent |
| `LV_ALIGN_TOP_RIGHT` | Top-right of parent |
| `LV_ALIGN_BOTTOM_LEFT` | Bottom-left of parent |
| `LV_ALIGN_BOTTOM_MID` | Bottom-center of parent |
| `LV_ALIGN_BOTTOM_RIGHT` | Bottom-right of parent |
| `LV_ALIGN_LEFT_MID` | Left-center of parent |
| `LV_ALIGN_RIGHT_MID` | Right-center of parent |
| `LV_ALIGN_CENTER` | Center of parent |
| `LV_ALIGN_OUT_TOP_LEFT` | Outside above-left (for align_to) |
| `LV_ALIGN_OUT_TOP_MID` | Outside above-center |
| `LV_ALIGN_OUT_TOP_RIGHT` | Outside above-right |
| `LV_ALIGN_OUT_BOTTOM_LEFT` | Outside below-left |
| `LV_ALIGN_OUT_BOTTOM_MID` | Outside below-center |
| `LV_ALIGN_OUT_BOTTOM_RIGHT` | Outside below-right |
| `LV_ALIGN_OUT_LEFT_TOP` | Outside left-top |
| `LV_ALIGN_OUT_LEFT_MID` | Outside left-center |
| `LV_ALIGN_OUT_LEFT_BOTTOM` | Outside left-bottom |
| `LV_ALIGN_OUT_RIGHT_TOP` | Outside right-top |
| `LV_ALIGN_OUT_RIGHT_MID` | Outside right-center |
| `LV_ALIGN_OUT_RIGHT_BOTTOM` | Outside right-bottom |

### Scrolling

```c
void lv_obj_set_scrollbar_mode(lv_obj_t * obj, lv_scrollbar_mode_t mode);
  // LV_SCROLLBAR_MODE_OFF, _ON, _ACTIVE, _AUTO
void lv_obj_set_scroll_dir(lv_obj_t * obj, lv_dir_t dir);
  // LV_DIR_NONE, _LEFT, _RIGHT, _TOP, _BOTTOM, _HOR, _VER, _ALL
void lv_obj_set_scroll_snap_x(lv_obj_t * obj, lv_scroll_snap_t align);
void lv_obj_set_scroll_snap_y(lv_obj_t * obj, lv_scroll_snap_t align);
  // LV_SCROLL_SNAP_NONE, _START, _END, _CENTER

void lv_obj_scroll_by(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_anim_enable_t en);
void lv_obj_scroll_to(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_anim_enable_t en);
void lv_obj_scroll_to_x(lv_obj_t * obj, lv_coord_t x, lv_anim_enable_t en);
void lv_obj_scroll_to_y(lv_obj_t * obj, lv_coord_t y, lv_anim_enable_t en);
void lv_obj_scroll_to_view(lv_obj_t * obj, lv_anim_enable_t en);
void lv_obj_scroll_to_view_recursive(lv_obj_t * obj, lv_anim_enable_t en);

lv_coord_t lv_obj_get_scroll_x(const lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_y(const lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_top(lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_bottom(lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_left(lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_right(lv_obj_t * obj);
lv_scrollbar_mode_t lv_obj_get_scrollbar_mode(const lv_obj_t * obj);
lv_dir_t lv_obj_get_scroll_dir(const lv_obj_t * obj);
```

### Hierarchy

```c
void lv_obj_set_parent(lv_obj_t * obj, lv_obj_t * parent);
lv_obj_t * lv_obj_get_parent(const lv_obj_t * obj);
lv_obj_t * lv_obj_get_child(const lv_obj_t * obj, int32_t id);  // 0=first, -1=last
uint32_t lv_obj_get_child_cnt(const lv_obj_t * obj);
uint32_t lv_obj_get_index(const lv_obj_t * obj);
void lv_obj_move_foreground(lv_obj_t * obj);
void lv_obj_move_background(lv_obj_t * obj);
void lv_obj_swap(lv_obj_t * obj1, lv_obj_t * obj2);

lv_obj_t * lv_obj_get_screen(const lv_obj_t * obj);
lv_disp_t * lv_obj_get_disp(const lv_obj_t * obj);
```

### Screen Management

```c
lv_obj_t * lv_scr_act(void);                     // Active screen (default display)
void lv_scr_load(lv_obj_t * scr);                // Load screen instantly
void lv_scr_load_anim(lv_obj_t * scr,
    lv_scr_load_anim_t anim_type,                // See animation types below
    uint32_t time,                                // Duration in ms
    uint32_t delay,                               // Delay before start in ms
    bool auto_del);                               // Auto-delete previous screen

lv_obj_t * lv_layer_top(void);
lv_obj_t * lv_layer_sys(void);
```

#### Screen Load Animation Types

| Constant | Description |
|----------|-------------|
| `LV_SCR_LOAD_ANIM_NONE` | Instant switch |
| `LV_SCR_LOAD_ANIM_OVER_LEFT` | New screen slides in from right |
| `LV_SCR_LOAD_ANIM_OVER_RIGHT` | New screen slides in from left |
| `LV_SCR_LOAD_ANIM_OVER_TOP` | New screen slides in from bottom |
| `LV_SCR_LOAD_ANIM_OVER_BOTTOM` | New screen slides in from top |
| `LV_SCR_LOAD_ANIM_MOVE_LEFT` | Both screens move left |
| `LV_SCR_LOAD_ANIM_MOVE_RIGHT` | Both screens move right |
| `LV_SCR_LOAD_ANIM_MOVE_TOP` | Both screens move up |
| `LV_SCR_LOAD_ANIM_MOVE_BOTTOM` | Both screens move down |
| `LV_SCR_LOAD_ANIM_FADE_ON` | Fade in new screen |

---

## 4. Core Widget APIs

### Label (lv_label)

Header: `widgets/lv_label.h`

```c
lv_obj_t * lv_label_create(lv_obj_t * parent);

void lv_label_set_text(lv_obj_t * obj, const char * text);
void lv_label_set_text_fmt(lv_obj_t * obj, const char * fmt, ...);
void lv_label_set_text_static(lv_obj_t * obj, const char * text);  // No copy, pointer stored
void lv_label_set_long_mode(lv_obj_t * obj, lv_label_long_mode_t long_mode);
void lv_label_set_recolor(lv_obj_t * obj, bool en);  // Enable #RRGGBB color# syntax
void lv_label_set_text_sel_start(lv_obj_t * obj, uint32_t index);
void lv_label_set_text_sel_end(lv_obj_t * obj, uint32_t index);

char * lv_label_get_text(const lv_obj_t * obj);
lv_label_long_mode_t lv_label_get_long_mode(const lv_obj_t * obj);
bool lv_label_get_recolor(const lv_obj_t * obj);
void lv_label_get_letter_pos(const lv_obj_t * obj, uint32_t char_id, lv_point_t * pos);
uint32_t lv_label_get_letter_on(const lv_obj_t * obj, lv_point_t * pos_in);
bool lv_label_is_char_under_pos(const lv_obj_t * obj, lv_point_t * pos);

void lv_label_ins_text(lv_obj_t * obj, uint32_t pos, const char * txt);
void lv_label_cut_text(lv_obj_t * obj, uint32_t pos, uint32_t cnt);
```

#### Long Mode Constants

| Constant | Description |
|----------|-------------|
| `LV_LABEL_LONG_WRAP` | Wrap text, expand object height |
| `LV_LABEL_LONG_DOT` | Truncate with "..." |
| `LV_LABEL_LONG_SCROLL` | Scroll text horizontally |
| `LV_LABEL_LONG_SCROLL_CIRCULAR` | Circular scroll |
| `LV_LABEL_LONG_CLIP` | Clip text at object boundary |

### Button (lv_btn)

Header: `widgets/lv_btn.h`

```c
lv_obj_t * lv_btn_create(lv_obj_t * parent);
// Buttons are styled lv_obj_t with LV_OBJ_FLAG_CLICKABLE set.
// No additional API -- use base object functions.
// Style LV_PART_MAIN for appearance.
```

### Slider (lv_slider)

Header: `widgets/lv_slider.h`

```c
lv_obj_t * lv_slider_create(lv_obj_t * parent);

void lv_slider_set_value(lv_obj_t * obj, int32_t value, lv_anim_enable_t anim);
void lv_slider_set_left_value(lv_obj_t * obj, int32_t value, lv_anim_enable_t anim);
void lv_slider_set_range(lv_obj_t * obj, int32_t min, int32_t max);
void lv_slider_set_mode(lv_obj_t * obj, lv_slider_mode_t mode);
  // LV_SLIDER_MODE_NORMAL, _SYMMETRICAL, _RANGE

int32_t lv_slider_get_value(const lv_obj_t * obj);
int32_t lv_slider_get_left_value(const lv_obj_t * obj);
int32_t lv_slider_get_min_value(const lv_obj_t * obj);
int32_t lv_slider_get_max_value(const lv_obj_t * obj);
bool lv_slider_is_dragged(const lv_obj_t * obj);
lv_slider_mode_t lv_slider_get_mode(lv_obj_t * obj);
```

**Parts**: `LV_PART_MAIN` (background), `LV_PART_INDICATOR` (filled portion), `LV_PART_KNOB` (draggable handle)

### Bar (lv_bar)

Header: `widgets/lv_bar.h`

```c
lv_obj_t * lv_bar_create(lv_obj_t * parent);

void lv_bar_set_value(lv_obj_t * obj, int32_t value, lv_anim_enable_t anim);
void lv_bar_set_start_value(lv_obj_t * obj, int32_t value, lv_anim_enable_t anim);
void lv_bar_set_range(lv_obj_t * obj, int32_t min, int32_t max);
void lv_bar_set_mode(lv_obj_t * obj, lv_bar_mode_t mode);
  // LV_BAR_MODE_NORMAL, _SYMMETRICAL, _RANGE

int32_t lv_bar_get_value(const lv_obj_t * obj);
int32_t lv_bar_get_start_value(const lv_obj_t * obj);
int32_t lv_bar_get_min_value(const lv_obj_t * obj);
int32_t lv_bar_get_max_value(const lv_obj_t * obj);
lv_bar_mode_t lv_bar_get_mode(lv_obj_t * obj);
```

### Arc (lv_arc)

Header: `widgets/lv_arc.h`

```c
lv_obj_t * lv_arc_create(lv_obj_t * parent);

void lv_arc_set_start_angle(lv_obj_t * obj, uint16_t start);
void lv_arc_set_end_angle(lv_obj_t * obj, uint16_t end);
void lv_arc_set_angles(lv_obj_t * obj, uint16_t start, uint16_t end);
void lv_arc_set_bg_start_angle(lv_obj_t * obj, uint16_t start);
void lv_arc_set_bg_end_angle(lv_obj_t * obj, uint16_t end);
void lv_arc_set_bg_angles(lv_obj_t * obj, uint16_t start, uint16_t end);
void lv_arc_set_rotation(lv_obj_t * obj, uint16_t rotation);
void lv_arc_set_mode(lv_obj_t * obj, lv_arc_mode_t type);
  // LV_ARC_MODE_NORMAL, _SYMMETRICAL, _REVERSE
void lv_arc_set_value(lv_obj_t * obj, int16_t value);
void lv_arc_set_range(lv_obj_t * obj, int16_t min, int16_t max);
void lv_arc_set_change_rate(lv_obj_t * obj, uint16_t rate);

uint16_t lv_arc_get_angle_start(lv_obj_t * obj);
uint16_t lv_arc_get_angle_end(lv_obj_t * obj);
uint16_t lv_arc_get_bg_angle_start(lv_obj_t * obj);
uint16_t lv_arc_get_bg_angle_end(lv_obj_t * obj);
int16_t lv_arc_get_value(const lv_obj_t * obj);
int16_t lv_arc_get_min_value(const lv_obj_t * obj);
int16_t lv_arc_get_max_value(const lv_obj_t * obj);
lv_arc_mode_t lv_arc_get_mode(const lv_obj_t * obj);
```

**Parts**: `LV_PART_MAIN` (background arc), `LV_PART_INDICATOR` (value arc), `LV_PART_KNOB` (drag handle)

### Switch (lv_switch)

```c
lv_obj_t * lv_switch_create(lv_obj_t * parent);
// Uses LV_STATE_CHECKED to toggle. No widget-specific set/get beyond base obj.
// Check: lv_obj_add_state(sw, LV_STATE_CHECKED)
// Uncheck: lv_obj_clear_state(sw, LV_STATE_CHECKED)
// Query: lv_obj_has_state(sw, LV_STATE_CHECKED)
```

**Parts**: `LV_PART_MAIN` (background), `LV_PART_INDICATOR` (fill area), `LV_PART_KNOB` (circle handle)

### Checkbox (lv_checkbox)

```c
lv_obj_t * lv_checkbox_create(lv_obj_t * parent);
void lv_checkbox_set_text(lv_obj_t * obj, const char * txt);
void lv_checkbox_set_text_static(lv_obj_t * obj, const char * txt);
const char * lv_checkbox_get_text(const lv_obj_t * obj);
// Toggle via LV_STATE_CHECKED (same as switch)
```

**Parts**: `LV_PART_MAIN` (text background), `LV_PART_INDICATOR` (check box square)

### Dropdown (lv_dropdown)

```c
lv_obj_t * lv_dropdown_create(lv_obj_t * parent);

void lv_dropdown_set_text(lv_obj_t * obj, const char * txt);
void lv_dropdown_set_options(lv_obj_t * obj, const char * options);  // "\n" separated
void lv_dropdown_set_options_static(lv_obj_t * obj, const char * options);
void lv_dropdown_add_option(lv_obj_t * obj, const char * option, uint32_t pos);
void lv_dropdown_clear_options(lv_obj_t * obj);
void lv_dropdown_set_selected(lv_obj_t * obj, uint16_t sel_opt);
void lv_dropdown_set_dir(lv_obj_t * obj, lv_dir_t dir);
void lv_dropdown_set_symbol(lv_obj_t * obj, const void * symbol);
void lv_dropdown_set_selected_highlight(lv_obj_t * obj, bool en);

uint16_t lv_dropdown_get_selected(const lv_obj_t * obj);
uint16_t lv_dropdown_get_option_cnt(const lv_obj_t * obj);
void lv_dropdown_get_selected_str(const lv_obj_t * obj, char * buf, uint32_t buf_size);
const char * lv_dropdown_get_options(const lv_obj_t * obj);
int32_t lv_dropdown_get_option_index(lv_obj_t * obj, const char * option);
lv_obj_t * lv_dropdown_get_list(lv_obj_t * obj);  // Get the dropdown list object (when open)

void lv_dropdown_open(lv_obj_t * dropdown_obj);
void lv_dropdown_close(lv_obj_t * obj);
bool lv_dropdown_is_open(lv_obj_t * obj);
```

### Roller (lv_roller)

```c
lv_obj_t * lv_roller_create(lv_obj_t * parent);

void lv_roller_set_options(lv_obj_t * obj, const char * options, lv_roller_mode_t mode);
  // mode: LV_ROLLER_MODE_NORMAL, LV_ROLLER_MODE_INFINITE
void lv_roller_set_visible_row_count(lv_obj_t * obj, uint8_t row_cnt);
void lv_roller_set_selected(lv_obj_t * obj, uint16_t sel_opt, lv_anim_enable_t anim);

uint16_t lv_roller_get_selected(const lv_obj_t * obj);
void lv_roller_get_selected_str(const lv_obj_t * obj, char * buf, uint32_t buf_size);
uint16_t lv_roller_get_option_cnt(const lv_obj_t * obj);
```

### Button Matrix (lv_btnmatrix)

```c
lv_obj_t * lv_btnmatrix_create(lv_obj_t * parent);

void lv_btnmatrix_set_map(lv_obj_t * obj, const char * map[]);  // NULL-terminated array, "\n" = new row
void lv_btnmatrix_set_ctrl_map(lv_obj_t * obj, const lv_btnmatrix_ctrl_t ctrl_map[]);
void lv_btnmatrix_set_selected_btn(lv_obj_t * obj, uint16_t btn_id);
void lv_btnmatrix_set_btn_ctrl(lv_obj_t * obj, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_clear_btn_ctrl(lv_obj_t * obj, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_set_btn_ctrl_all(lv_obj_t * obj, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_clear_btn_ctrl_all(lv_obj_t * obj, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_set_btn_width(lv_obj_t * obj, uint16_t btn_id, uint8_t width);
void lv_btnmatrix_set_one_checked(lv_obj_t * obj, bool en);

const char ** lv_btnmatrix_get_map(const lv_obj_t * obj);
uint16_t lv_btnmatrix_get_selected_btn(const lv_obj_t * obj);
const char * lv_btnmatrix_get_btn_text(const lv_obj_t * obj, uint16_t btn_id);
bool lv_btnmatrix_has_btn_ctrl(lv_obj_t * obj, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl);
bool lv_btnmatrix_get_one_checked(const lv_obj_t * obj);
```

#### Button Matrix Control Flags

| Flag | Description |
|------|-------------|
| `LV_BTNMATRIX_CTRL_HIDDEN` | Hide button |
| `LV_BTNMATRIX_CTRL_NO_REPEAT` | Disable long-press repeat |
| `LV_BTNMATRIX_CTRL_DISABLED` | Disable button |
| `LV_BTNMATRIX_CTRL_CHECKABLE` | Enable toggle |
| `LV_BTNMATRIX_CTRL_CHECKED` | Currently checked |
| `LV_BTNMATRIX_CTRL_CLICK_TRIG` | Send events on click (vs press) |
| `LV_BTNMATRIX_CTRL_POPOVER` | Show button text in popup |
| `LV_BTNMATRIX_CTRL_RECOLOR` | Enable text recoloring |
| `LV_BTNMATRIX_CTRL_CUSTOM_1/2` | Custom flags |

### Text Area (lv_textarea)

```c
lv_obj_t * lv_textarea_create(lv_obj_t * parent);

void lv_textarea_add_char(lv_obj_t * obj, uint32_t c);
void lv_textarea_add_text(lv_obj_t * obj, const char * txt);
void lv_textarea_del_char(lv_obj_t * obj);
void lv_textarea_del_char_forward(lv_obj_t * obj);
void lv_textarea_set_text(lv_obj_t * obj, const char * txt);
void lv_textarea_set_placeholder_text(lv_obj_t * obj, const char * txt);
void lv_textarea_set_cursor_pos(lv_obj_t * obj, int32_t pos);  // LV_TEXTAREA_CURSOR_LAST
void lv_textarea_set_cursor_click_pos(lv_obj_t * obj, bool en);
void lv_textarea_set_password_mode(lv_obj_t * obj, bool en);
void lv_textarea_set_one_line(lv_obj_t * obj, bool en);
void lv_textarea_set_accepted_chars(lv_obj_t * obj, const char * list);
void lv_textarea_set_max_length(lv_obj_t * obj, uint32_t num);
void lv_textarea_set_insert_replace(lv_obj_t * obj, const char * txt);
void lv_textarea_set_text_selection(lv_obj_t * obj, bool en);
void lv_textarea_set_password_show_time(lv_obj_t * obj, uint16_t time);
void lv_textarea_set_align(lv_obj_t * obj, lv_text_align_t align);

const char * lv_textarea_get_text(const lv_obj_t * obj);
const char * lv_textarea_get_placeholder_text(lv_obj_t * obj);
lv_obj_t * lv_textarea_get_label(const lv_obj_t * obj);
uint32_t lv_textarea_get_cursor_pos(const lv_obj_t * obj);
bool lv_textarea_get_cursor_click_pos(lv_obj_t * obj);
bool lv_textarea_get_password_mode(const lv_obj_t * obj);
bool lv_textarea_get_one_line(const lv_obj_t * obj);
const char * lv_textarea_get_accepted_chars(lv_obj_t * obj);
uint32_t lv_textarea_get_max_length(lv_obj_t * obj);
bool lv_textarea_text_is_selected(const lv_obj_t * obj);
bool lv_textarea_get_text_selection(lv_obj_t * obj);
uint16_t lv_textarea_get_password_show_time(lv_obj_t * obj);

void lv_textarea_clear_selection(lv_obj_t * obj);
void lv_textarea_cursor_right(lv_obj_t * obj);
void lv_textarea_cursor_left(lv_obj_t * obj);
void lv_textarea_cursor_down(lv_obj_t * obj);
void lv_textarea_cursor_up(lv_obj_t * obj);
```

### Table (lv_table)

```c
lv_obj_t * lv_table_create(lv_obj_t * parent);

void lv_table_set_cell_value(lv_obj_t * obj, uint16_t row, uint16_t col, const char * txt);
void lv_table_set_cell_value_fmt(lv_obj_t * obj, uint16_t row, uint16_t col, const char * fmt, ...);
void lv_table_set_row_cnt(lv_obj_t * obj, uint16_t row_cnt);
void lv_table_set_col_cnt(lv_obj_t * obj, uint16_t col_cnt);
void lv_table_set_col_width(lv_obj_t * obj, uint16_t col_id, lv_coord_t w);
void lv_table_add_cell_ctrl(lv_obj_t * obj, uint16_t row, uint16_t col, lv_table_cell_ctrl_t ctrl);
void lv_table_clear_cell_ctrl(lv_obj_t * obj, uint16_t row, uint16_t col, lv_table_cell_ctrl_t ctrl);

const char * lv_table_get_cell_value(lv_obj_t * obj, uint16_t row, uint16_t col);
uint16_t lv_table_get_row_cnt(lv_obj_t * obj);
uint16_t lv_table_get_col_cnt(lv_obj_t * obj);
lv_coord_t lv_table_get_col_width(lv_obj_t * obj, uint16_t col);
bool lv_table_has_cell_ctrl(lv_obj_t * obj, uint16_t row, uint16_t col, lv_table_cell_ctrl_t ctrl);
void lv_table_get_selected_cell(lv_obj_t * obj, uint16_t * row, uint16_t * col);
```

### Image (lv_img)

```c
lv_obj_t * lv_img_create(lv_obj_t * parent);

void lv_img_set_src(lv_obj_t * obj, const void * src);  // File path, symbol, or lv_img_dsc_t *
void lv_img_set_offset_x(lv_obj_t * obj, lv_coord_t x);
void lv_img_set_offset_y(lv_obj_t * obj, lv_coord_t y);
void lv_img_set_angle(lv_obj_t * obj, int16_t angle);   // 0.1 degree units
void lv_img_set_zoom(lv_obj_t * obj, uint16_t zoom);    // 256 = 1x
void lv_img_set_pivot(lv_obj_t * obj, lv_coord_t x, lv_coord_t y);
void lv_img_set_antialias(lv_obj_t * obj, bool antialias);
void lv_img_set_size_mode(lv_obj_t * obj, lv_img_size_mode_t mode);
  // LV_IMG_SIZE_MODE_VIRTUAL, LV_IMG_SIZE_MODE_REAL

const void * lv_img_get_src(lv_obj_t * obj);
lv_coord_t lv_img_get_offset_x(lv_obj_t * obj);
lv_coord_t lv_img_get_offset_y(lv_obj_t * obj);
uint16_t lv_img_get_angle(lv_obj_t * obj);
uint16_t lv_img_get_zoom(lv_obj_t * obj);
bool lv_img_get_antialias(lv_obj_t * obj);
lv_img_size_mode_t lv_img_get_size_mode(lv_obj_t * obj);
```

### Canvas (lv_canvas)

```c
lv_obj_t * lv_canvas_create(lv_obj_t * parent);

void lv_canvas_set_buffer(lv_obj_t * obj, void * buf, lv_coord_t w, lv_coord_t h,
                          lv_img_cf_t cf);  // LV_IMG_CF_TRUE_COLOR, _TRUE_COLOR_ALPHA, etc.
void lv_canvas_set_px_color(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_color_t c);
void lv_canvas_set_px_opa(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_opa_t opa);
void lv_canvas_set_palette(lv_obj_t * obj, uint8_t id, lv_color32_t c);

lv_color_t lv_canvas_get_px(lv_obj_t * obj, lv_coord_t x, lv_coord_t y);
lv_img_dsc_t * lv_canvas_get_img(lv_obj_t * obj);

void lv_canvas_copy_buf(lv_obj_t * obj, const void * to_copy, lv_coord_t x, lv_coord_t y,
                        lv_coord_t w, lv_coord_t h);
void lv_canvas_fill_bg(lv_obj_t * obj, lv_color_t color, lv_opa_t opa);
void lv_canvas_transform(lv_obj_t * obj, lv_img_dsc_t * img, int16_t angle,
                         uint16_t zoom, lv_coord_t offset_x, lv_coord_t offset_y,
                         int32_t pivot_x, int32_t pivot_y, bool antialias);

// Drawing on canvas
void lv_canvas_draw_rect(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_coord_t w,
                         lv_coord_t h, const lv_draw_rect_dsc_t * draw_dsc);
void lv_canvas_draw_text(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_coord_t max_w,
                         lv_draw_label_dsc_t * draw_dsc, const char * txt);
void lv_canvas_draw_img(lv_obj_t * obj, lv_coord_t x, lv_coord_t y,
                        const void * src, const lv_draw_img_dsc_t * draw_dsc);
void lv_canvas_draw_line(lv_obj_t * obj, const lv_point_t points[], uint32_t point_cnt,
                         const lv_draw_line_dsc_t * draw_dsc);
void lv_canvas_draw_polygon(lv_obj_t * obj, const lv_point_t points[], uint32_t point_cnt,
                            const lv_draw_rect_dsc_t * draw_dsc);
void lv_canvas_draw_arc(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_coord_t r,
                        int32_t start_angle, int32_t end_angle,
                        const lv_draw_arc_dsc_t * draw_dsc);
```

### Line (lv_line)

```c
lv_obj_t * lv_line_create(lv_obj_t * parent);

void lv_line_set_points(lv_obj_t * obj, const lv_point_t points[], uint16_t point_num);
void lv_line_set_y_invert(lv_obj_t * obj, bool en);  // Invert Y axis (0 at bottom)

bool lv_line_get_y_invert(const lv_obj_t * obj);
```

---

## 5. Extra Widget APIs

### Chart (lv_chart)

```c
lv_obj_t * lv_chart_create(lv_obj_t * parent);

void lv_chart_set_type(lv_obj_t * obj, lv_chart_type_t type);
  // LV_CHART_TYPE_NONE, _LINE, _BAR, _SCATTER
void lv_chart_set_point_count(lv_obj_t * obj, uint16_t cnt);
void lv_chart_set_range(lv_obj_t * obj, lv_chart_axis_t axis, lv_coord_t min, lv_coord_t max);
  // axis: LV_CHART_AXIS_PRIMARY_Y, _SECONDARY_Y, _PRIMARY_X, _SECONDARY_X
void lv_chart_set_update_mode(lv_obj_t * obj, lv_chart_update_mode_t update_mode);
  // LV_CHART_UPDATE_MODE_SHIFT, _CIRCULAR
void lv_chart_set_div_line_count(lv_obj_t * obj, uint8_t hdiv, uint8_t vdiv);
void lv_chart_set_zoom_x(lv_obj_t * obj, uint16_t zoom_x);   // 256 = 1x
void lv_chart_set_zoom_y(lv_obj_t * obj, uint16_t zoom_y);
void lv_chart_set_axis_tick(lv_obj_t * obj, lv_chart_axis_t axis,
    lv_coord_t major_len, lv_coord_t minor_len,
    lv_coord_t major_cnt, lv_coord_t minor_cnt, bool label_en, lv_coord_t draw_size);

// Series management
lv_chart_series_t * lv_chart_add_series(lv_obj_t * obj, lv_color_t color, lv_chart_axis_t axis);
void lv_chart_remove_series(lv_obj_t * obj, lv_chart_series_t * series);
void lv_chart_hide_series(lv_obj_t * obj, lv_chart_series_t * series, bool hide);
void lv_chart_set_series_color(lv_obj_t * obj, lv_chart_series_t * series, lv_color_t color);

// Data
void lv_chart_set_next_value(lv_obj_t * obj, lv_chart_series_t * ser, lv_coord_t value);
void lv_chart_set_next_value2(lv_obj_t * obj, lv_chart_series_t * ser,
                              lv_coord_t x_value, lv_coord_t y_value);  // For scatter
void lv_chart_set_value_by_id(lv_obj_t * obj, lv_chart_series_t * ser,
                              uint16_t id, lv_coord_t value);
void lv_chart_set_value_by_id2(lv_obj_t * obj, lv_chart_series_t * ser,
                               uint16_t id, lv_coord_t x_value, lv_coord_t y_value);
void lv_chart_set_ext_y_array(lv_obj_t * obj, lv_chart_series_t * ser, lv_coord_t array[]);
void lv_chart_set_ext_x_array(lv_obj_t * obj, lv_chart_series_t * ser, lv_coord_t array[]);
lv_coord_t * lv_chart_get_y_array(const lv_obj_t * obj, lv_chart_series_t * ser);
lv_coord_t * lv_chart_get_x_array(const lv_obj_t * obj, lv_chart_series_t * ser);

// Cursor
lv_chart_cursor_t * lv_chart_add_cursor(lv_obj_t * obj, lv_color_t color, lv_dir_t dir);
void lv_chart_set_cursor_pos(lv_obj_t * obj, lv_chart_cursor_t * cursor, lv_point_t * pos);
void lv_chart_set_cursor_point(lv_obj_t * obj, lv_chart_cursor_t * cursor,
                               lv_chart_series_t * ser, uint16_t point_id);
lv_point_t lv_chart_get_cursor_point(lv_obj_t * obj, lv_chart_cursor_t * cursor);

void lv_chart_refresh(lv_obj_t * obj);  // Force redraw
uint16_t lv_chart_get_point_count(const lv_obj_t * obj);
uint16_t lv_chart_get_x_start_point(const lv_obj_t * obj, lv_chart_series_t * ser);
lv_chart_type_t lv_chart_get_type(const lv_obj_t * obj);
uint16_t lv_chart_get_pressed_point(const lv_obj_t * obj);
```

### Calendar (lv_calendar)

```c
lv_obj_t * lv_calendar_create(lv_obj_t * parent);

void lv_calendar_set_today_date(lv_obj_t * obj, uint32_t year, uint32_t month, uint32_t day);
void lv_calendar_set_showed_date(lv_obj_t * obj, uint32_t year, uint32_t month);
void lv_calendar_set_highlighted_dates(lv_obj_t * obj, lv_calendar_date_t highlighted[],
                                       uint16_t date_num);
void lv_calendar_set_day_names(const char ** day_names);  // 7-element array

const lv_calendar_date_t * lv_calendar_get_today_date(const lv_obj_t * calendar);
const lv_calendar_date_t * lv_calendar_get_showed_date(const lv_obj_t * calendar);
lv_res_t lv_calendar_get_pressed_date(const lv_obj_t * calendar, lv_calendar_date_t * date);

// Calendar header (optional arrow navigation)
lv_obj_t * lv_calendar_header_arrow_create(lv_obj_t * parent);
lv_obj_t * lv_calendar_header_dropdown_create(lv_obj_t * parent);
```

### Meter (lv_meter)

```c
lv_obj_t * lv_meter_create(lv_obj_t * parent);

// Scale
lv_meter_scale_t * lv_meter_add_scale(lv_obj_t * obj);
void lv_meter_set_scale_ticks(lv_obj_t * obj, lv_meter_scale_t * scale,
    uint16_t cnt, uint16_t width, uint16_t len, lv_color_t color);
void lv_meter_set_scale_major_ticks(lv_obj_t * obj, lv_meter_scale_t * scale,
    uint16_t nth, uint16_t width, uint16_t len, lv_color_t color, int16_t label_gap);
void lv_meter_set_scale_range(lv_obj_t * obj, lv_meter_scale_t * scale,
    int32_t min, int32_t max, uint32_t angle_range, uint32_t rotation);

// Indicators
lv_meter_indicator_t * lv_meter_add_needle_line(lv_obj_t * obj, lv_meter_scale_t * scale,
    uint16_t width, lv_color_t color, int16_t r_mod);
lv_meter_indicator_t * lv_meter_add_needle_img(lv_obj_t * obj, lv_meter_scale_t * scale,
    const void * src, lv_coord_t pivot_x, lv_coord_t pivot_y);
lv_meter_indicator_t * lv_meter_add_arc(lv_obj_t * obj, lv_meter_scale_t * scale,
    uint16_t width, lv_color_t color, int16_t r_mod);
lv_meter_indicator_t * lv_meter_add_scale_lines(lv_obj_t * obj, lv_meter_scale_t * scale,
    lv_color_t color_start, lv_color_t color_end, bool local, int16_t width_mod);

void lv_meter_set_indicator_value(lv_obj_t * obj, lv_meter_indicator_t * indic, int32_t value);
void lv_meter_set_indicator_start_value(lv_obj_t * obj, lv_meter_indicator_t * indic, int32_t value);
void lv_meter_set_indicator_end_value(lv_obj_t * obj, lv_meter_indicator_t * indic, int32_t value);
```

### Menu (lv_menu) -- NEW in v8.2

```c
lv_obj_t * lv_menu_create(lv_obj_t * parent);

// Pages
lv_obj_t * lv_menu_page_create(lv_obj_t * parent, char * title);
void lv_menu_set_page(lv_obj_t * obj, lv_obj_t * page);

// Sections and containers
lv_obj_t * lv_menu_section_create(lv_obj_t * parent);
lv_obj_t * lv_menu_cont_create(lv_obj_t * parent);
lv_obj_t * lv_menu_separator_create(lv_obj_t * parent);

// Configuration
void lv_menu_set_sidebar_page(lv_obj_t * obj, lv_obj_t * page);
void lv_menu_set_mode_header(lv_obj_t * obj, lv_menu_mode_header_t mode);
  // LV_MENU_HEADER_TOP_FIXED, _TOP_UNFIXED, _BOTTOM_FIXED
void lv_menu_set_mode_root_back_btn(lv_obj_t * obj, lv_menu_mode_root_back_btn_t mode);
  // LV_MENU_ROOT_BACK_BTN_DISABLED, _ENABLED
void lv_menu_set_load_page_event(lv_obj_t * menu, lv_obj_t * obj, lv_obj_t * page);
void lv_menu_clear_history(lv_obj_t * obj);
void lv_menu_back(lv_obj_t * obj);

lv_obj_t * lv_menu_get_cur_main_page(lv_obj_t * obj);
lv_obj_t * lv_menu_get_cur_sidebar_page(lv_obj_t * obj);
lv_obj_t * lv_menu_get_main_header(lv_obj_t * obj);
lv_obj_t * lv_menu_get_main_header_back_btn(lv_obj_t * obj);
lv_obj_t * lv_menu_get_sidebar_header(lv_obj_t * obj);
lv_obj_t * lv_menu_get_sidebar_header_back_btn(lv_obj_t * obj);
```

### Keyboard (lv_keyboard)

```c
lv_obj_t * lv_keyboard_create(lv_obj_t * parent);

void lv_keyboard_set_textarea(lv_obj_t * kb, lv_obj_t * ta);
void lv_keyboard_set_mode(lv_obj_t * kb, lv_keyboard_mode_t mode);
  // LV_KEYBOARD_MODE_TEXT_LOWER, _TEXT_UPPER, _SPECIAL, _NUMBER
void lv_keyboard_set_map(lv_obj_t * kb, lv_keyboard_mode_t mode,
                         const char * map[], const lv_btnmatrix_ctrl_t ctrl_map[]);
void lv_keyboard_set_popovers(lv_obj_t * kb, bool en);

lv_obj_t * lv_keyboard_get_textarea(const lv_obj_t * kb);
lv_keyboard_mode_t lv_keyboard_get_mode(const lv_obj_t * kb);
bool lv_keyboard_get_popovers(const lv_obj_t * kb);
```

### List (lv_list)

```c
lv_obj_t * lv_list_create(lv_obj_t * parent);

lv_obj_t * lv_list_add_text(lv_obj_t * list, const char * txt);      // Section header
lv_obj_t * lv_list_add_btn(lv_obj_t * list, const char * icon, const char * txt);

const char * lv_list_get_btn_text(lv_obj_t * list, lv_obj_t * btn);
```

### Message Box (lv_msgbox)

```c
/**
 * Create a message box.
 * @param parent       parent object (NULL = full-screen modal)
 * @param title        title string
 * @param txt          body text
 * @param btn_txts     NULL-terminated array of button labels
 * @param add_close_btn  add X close button to title
 */
lv_obj_t * lv_msgbox_create(lv_obj_t * parent, const char * title, const char * txt,
                            const char * btn_txts[], bool add_close_btn);

lv_obj_t * lv_msgbox_get_title(lv_obj_t * obj);
lv_obj_t * lv_msgbox_get_close_btn(lv_obj_t * obj);
lv_obj_t * lv_msgbox_get_text(lv_obj_t * obj);
lv_obj_t * lv_msgbox_get_btns(lv_obj_t * obj);   // Returns lv_btnmatrix
uint16_t lv_msgbox_get_active_btn(lv_obj_t * mbox);
const char * lv_msgbox_get_active_btn_text(lv_obj_t * mbox);

void lv_msgbox_close(lv_obj_t * mbox);
void lv_msgbox_close_async(lv_obj_t * mbox);
```

### Tab View (lv_tabview)

```c
lv_obj_t * lv_tabview_create(lv_obj_t * parent, lv_dir_t tab_pos, lv_coord_t tab_size);
  // tab_pos: LV_DIR_TOP, _BOTTOM, _LEFT, _RIGHT

lv_obj_t * lv_tabview_add_tab(lv_obj_t * tv, const char * name);  // Returns page content obj
void lv_tabview_set_act(lv_obj_t * obj, uint32_t id, lv_anim_enable_t anim);

uint16_t lv_tabview_get_tab_act(lv_obj_t * tv);
lv_obj_t * lv_tabview_get_content(lv_obj_t * tv);
lv_obj_t * lv_tabview_get_tab_btns(lv_obj_t * tv);
```

### Tile View (lv_tileview)

```c
lv_obj_t * lv_tileview_create(lv_obj_t * parent);

lv_obj_t * lv_tileview_add_tile(lv_obj_t * tv, uint8_t col_id, uint8_t row_id, lv_dir_t dir);
  // dir: allowed scroll directions from this tile

void lv_obj_set_tile(lv_obj_t * tv, lv_obj_t * tile_obj, lv_anim_enable_t anim);
void lv_obj_set_tile_id(lv_obj_t * tv, uint32_t col_id, uint32_t row_id, lv_anim_enable_t anim);

lv_obj_t * lv_tileview_get_tile_act(lv_obj_t * obj);
```

### Spinbox (lv_spinbox)

```c
lv_obj_t * lv_spinbox_create(lv_obj_t * parent);

void lv_spinbox_set_value(lv_obj_t * obj, int32_t i);
void lv_spinbox_set_rollover(lv_obj_t * obj, bool b);
void lv_spinbox_set_digit_format(lv_obj_t * obj, uint8_t digit_count, uint8_t separator_position);
void lv_spinbox_set_step(lv_obj_t * obj, uint32_t step);
void lv_spinbox_set_range(lv_obj_t * obj, int32_t range_min, int32_t range_max);
void lv_spinbox_set_cursor_pos(lv_obj_t * obj, uint8_t pos);
void lv_spinbox_set_digit_step_direction(lv_obj_t * obj, lv_dir_t direction);

int32_t lv_spinbox_get_value(lv_obj_t * obj);
int32_t lv_spinbox_get_step(lv_obj_t * obj);

void lv_spinbox_step_next(lv_obj_t * obj);
void lv_spinbox_step_prev(lv_obj_t * obj);
void lv_spinbox_increment(lv_obj_t * obj);
void lv_spinbox_decrement(lv_obj_t * obj);
```

### LED (lv_led)

```c
lv_obj_t * lv_led_create(lv_obj_t * parent);

void lv_led_set_color(lv_obj_t * led, lv_color_t color);
void lv_led_set_brightness(lv_obj_t * led, uint8_t bright);  // 0 (off) to 255 (full)
void lv_led_on(lv_obj_t * led);                              // Max brightness
void lv_led_off(lv_obj_t * led);                             // Min brightness
void lv_led_toggle(lv_obj_t * led);

uint8_t lv_led_get_brightness(const lv_obj_t * obj);
```

### Color Wheel (lv_colorwheel)

```c
lv_obj_t * lv_colorwheel_create(lv_obj_t * parent, bool knob_recolor);

bool lv_colorwheel_set_hsv(lv_obj_t * obj, lv_color_hsv_t hsv);
bool lv_colorwheel_set_rgb(lv_obj_t * obj, lv_color_t color);
void lv_colorwheel_set_mode(lv_obj_t * obj, lv_colorwheel_mode_t mode);
  // LV_COLORWHEEL_MODE_HUE, _SATURATION, _VALUE
void lv_colorwheel_set_mode_fixed(lv_obj_t * obj, bool fixed);

lv_color_hsv_t lv_colorwheel_get_hsv(lv_obj_t * obj);
lv_color_t lv_colorwheel_get_rgb(lv_obj_t * obj);
lv_colorwheel_mode_t lv_colorwheel_get_color_mode(lv_obj_t * obj);
bool lv_colorwheel_get_color_mode_fixed(lv_obj_t * obj);
```

### Span Group (lv_spangroup)

```c
lv_obj_t * lv_spangroup_create(lv_obj_t * parent);

lv_span_t * lv_spangroup_new_span(lv_obj_t * obj);
void lv_spangroup_del_span(lv_obj_t * obj, lv_span_t * span);
void lv_span_set_text(lv_span_t * span, const char * text);
void lv_span_set_text_static(lv_span_t * span, const char * text);

void lv_spangroup_set_align(lv_obj_t * obj, lv_text_align_t align);
void lv_spangroup_set_overflow(lv_obj_t * obj, lv_span_overflow_t overflow);
void lv_spangroup_set_indent(lv_obj_t * obj, lv_coord_t indent);
void lv_spangroup_set_mode(lv_obj_t * obj, lv_span_mode_t mode);
  // LV_SPAN_MODE_FIXED, _EXPAND, _BREAK

lv_span_t * lv_spangroup_get_child(const lv_obj_t * obj, int32_t id);
uint32_t lv_spangroup_get_child_cnt(const lv_obj_t * obj);
lv_text_align_t lv_spangroup_get_align(lv_obj_t * obj);
lv_span_overflow_t lv_spangroup_get_overflow(lv_obj_t * obj);
lv_coord_t lv_spangroup_get_indent(lv_obj_t * obj);
lv_span_mode_t lv_spangroup_get_mode(lv_obj_t * obj);

// v8.2 BREAKING CHANGE: new max_width parameter
lv_coord_t lv_spangroup_get_expand_width(lv_obj_t * obj, uint32_t max_width);
lv_coord_t lv_spangroup_get_expand_height(lv_obj_t * obj, lv_coord_t width);

void lv_spangroup_refr_mode(lv_obj_t * obj);
```

### Window (lv_win)

```c
lv_obj_t * lv_win_create(lv_obj_t * parent, lv_coord_t header_height);

lv_obj_t * lv_win_add_title(lv_obj_t * win, const char * txt);
lv_obj_t * lv_win_add_btn(lv_obj_t * win, const void * icon, lv_coord_t btn_w);

lv_obj_t * lv_win_get_header(lv_obj_t * win);
lv_obj_t * lv_win_get_content(lv_obj_t * win);
```

---

## 6. Style API

Header: `lv_obj_style.h`, `lv_style.h`

### Style Management

```c
// Initialization (required before use)
void lv_style_init(lv_style_t * style);
void lv_style_reset(lv_style_t * style);  // Reset to defaults

// Property setters -- pattern: lv_style_set_<property>(&style, value)
void lv_style_set_width(lv_style_t * style, lv_coord_t value);
void lv_style_set_height(lv_style_t * style, lv_coord_t value);
void lv_style_set_bg_color(lv_style_t * style, lv_color_t value);
void lv_style_set_bg_opa(lv_style_t * style, lv_opa_t value);
void lv_style_set_border_color(lv_style_t * style, lv_color_t value);
void lv_style_set_border_width(lv_style_t * style, lv_coord_t value);
void lv_style_set_text_color(lv_style_t * style, lv_color_t value);
void lv_style_set_text_font(lv_style_t * style, const lv_font_t * value);
void lv_style_set_radius(lv_style_t * style, lv_coord_t value);
void lv_style_set_pad_all(lv_style_t * style, lv_coord_t value);
void lv_style_set_pad_top(lv_style_t * style, lv_coord_t value);
void lv_style_set_pad_bottom(lv_style_t * style, lv_coord_t value);
void lv_style_set_pad_left(lv_style_t * style, lv_coord_t value);
void lv_style_set_pad_right(lv_style_t * style, lv_coord_t value);
void lv_style_set_pad_row(lv_style_t * style, lv_coord_t value);
void lv_style_set_pad_column(lv_style_t * style, lv_coord_t value);
void lv_style_set_pad_gap(lv_style_t * style, lv_coord_t value);  // Sets both row and column
void lv_style_set_shadow_width(lv_style_t * style, lv_coord_t value);
void lv_style_set_shadow_color(lv_style_t * style, lv_color_t value);
void lv_style_set_outline_width(lv_style_t * style, lv_coord_t value);
void lv_style_set_outline_color(lv_style_t * style, lv_color_t value);
void lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value);
void lv_style_set_blend_mode(lv_style_t * style, lv_blend_mode_t value);
void lv_style_set_layout(lv_style_t * style, uint16_t value);
void lv_style_set_base_dir(lv_style_t * style, lv_base_dir_t value);
// ... (every style property has a setter following this pattern)

// Property removal and queries
void lv_style_remove_prop(lv_style_t * style, lv_style_prop_t prop);
lv_res_t lv_style_get_prop(const lv_style_t * style, lv_style_prop_t prop, lv_style_value_t * value);
bool lv_style_is_empty(const lv_style_t * style);

// Constant styles (ROM-friendly, zero RAM)
// LV_STYLE_CONST_INIT(var_name, prop_array)
```

### Object Style Functions

```c
/**
 * Add a style to an object for a given part and state.
 * @param obj      target object
 * @param style    pointer to style (must persist -- NOT stack-allocated)
 * @param selector LV_PART_xxx | LV_STATE_xxx
 */
void lv_obj_add_style(lv_obj_t * obj, lv_style_t * style, lv_style_selector_t selector);
void lv_obj_remove_style(lv_obj_t * obj, lv_style_t * style, lv_style_selector_t selector);
void lv_obj_remove_style_all(lv_obj_t * obj);
void lv_obj_refresh_style(lv_obj_t * obj, lv_style_selector_t selector, lv_style_prop_t prop);
void lv_obj_report_style_change(lv_style_t * style);  // Notify all users of this style

// Local (inline) style setters -- pattern: lv_obj_set_style_<property>(obj, value, selector)
void lv_obj_set_style_bg_color(lv_obj_t * obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_bg_opa(lv_obj_t * obj, lv_opa_t value, lv_style_selector_t selector);
void lv_obj_set_style_text_color(lv_obj_t * obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_text_font(lv_obj_t * obj, const lv_font_t * value, lv_style_selector_t selector);
// ... (pattern for all style properties)

// Computed style getters -- pattern: lv_obj_get_style_<property>(obj, part)
lv_color_t lv_obj_get_style_bg_color(const lv_obj_t * obj, uint32_t part);
lv_opa_t lv_obj_get_style_bg_opa(const lv_obj_t * obj, uint32_t part);
lv_color_t lv_obj_get_style_text_color(const lv_obj_t * obj, uint32_t part);
const lv_font_t * lv_obj_get_style_text_font(const lv_obj_t * obj, uint32_t part);
// ... (pattern for all style properties)
```

### Transitions

```c
/**
 * Initialize a transition descriptor.
 * @param tr         pointer to transition descriptor
 * @param props      zero-terminated array of style properties to animate
 * @param path_cb    animation easing function (e.g., lv_anim_path_ease_in_out)
 * @param time       duration in ms
 * @param delay      delay before start in ms
 * @param user_data  custom pointer (optional)
 */
void lv_style_transition_dsc_init(lv_style_transition_dsc_t * tr,
    const lv_style_prop_t props[],
    lv_anim_path_cb_t path_cb,
    uint32_t time,
    uint32_t delay,
    void * user_data);
```

---

## 7. Event API

Header: `lv_event.h`

### Adding and Removing Handlers

```c
/**
 * Add event handler to an object.
 * @param obj        target object
 * @param event_cb   callback function
 * @param filter     event code filter (LV_EVENT_ALL to receive all)
 * @param user_data  custom data passed to callback
 * @return           event descriptor (for removal)
 */
struct _lv_event_dsc_t * lv_obj_add_event_cb(lv_obj_t * obj,
    lv_event_cb_t event_cb,
    lv_event_code_t filter,
    void * user_data);

bool lv_obj_remove_event_cb(lv_obj_t * obj, lv_event_cb_t event_cb);
bool lv_obj_remove_event_dsc(lv_obj_t * obj, struct _lv_event_dsc_t * event_dsc);
```

### Event Callback Signature

```c
typedef void (*lv_event_cb_t)(lv_event_t * e);
```

### Event Accessors

```c
lv_event_code_t lv_event_get_code(lv_event_t * e);         // Event type
lv_obj_t * lv_event_get_target(lv_event_t * e);            // Object that received event
lv_obj_t * lv_event_get_current_target(lv_event_t * e);    // Object with the handler
void * lv_event_get_user_data(lv_event_t * e);             // User data from add_event_cb
void * lv_event_get_param(lv_event_t * e);                 // Parameter from lv_event_send
lv_indev_t * lv_event_get_indev(lv_event_t * e);          // Input device (for input events)
lv_obj_draw_part_dsc_t * lv_event_get_draw_part_dsc(lv_event_t * e);  // Draw event data
const lv_area_t * lv_event_get_clip_area(lv_event_t * e); // Clip area for draw events
uint32_t lv_event_get_key(lv_event_t * e);                // Key code (for LV_EVENT_KEY)
lv_anim_t * lv_event_get_scroll_anim(lv_event_t * e);     // Scroll animation
```

### Sending Events

```c
lv_res_t lv_event_send(lv_obj_t * obj, lv_event_code_t event_code, void * param);
lv_res_t lv_obj_event_base(const lv_obj_class_t * class_p, lv_event_t * e);
```

### Custom Events

```c
uint32_t lv_event_register_id(void);  // Returns unique event ID

// Usage:
static uint32_t MY_EVENT;
MY_EVENT = lv_event_register_id();
lv_event_send(obj, MY_EVENT, &my_data);
```

### Event Codes Reference

#### Input Events

| Code | Description |
|------|-------------|
| `LV_EVENT_PRESSED` | Object pressed |
| `LV_EVENT_PRESSING` | Still being pressed (continuous) |
| `LV_EVENT_PRESS_LOST` | Press moved outside object |
| `LV_EVENT_SHORT_CLICKED` | Released quickly, no scroll |
| `LV_EVENT_LONG_PRESSED` | Pressed for `long_press_time` ms |
| `LV_EVENT_LONG_PRESSED_REPEAT` | Repeated after long press |
| `LV_EVENT_CLICKED` | Released (no scroll occurred) |
| `LV_EVENT_RELEASED` | Released (regardless of scroll) |
| `LV_EVENT_SCROLL_BEGIN` | Scroll started |
| `LV_EVENT_SCROLL_END` | Scroll ended |
| `LV_EVENT_SCROLL` | Scrolling |
| `LV_EVENT_GESTURE` | Gesture detected |
| `LV_EVENT_KEY` | Key press sent to object |
| `LV_EVENT_FOCUSED` | Object received focus |
| `LV_EVENT_DEFOCUSED` | Object lost focus |
| `LV_EVENT_LEAVE` | Object losing focus to next |
| `LV_EVENT_HIT_TEST` | Advanced hit testing |

#### Drawing Events

| Code | Description |
|------|-------------|
| `LV_EVENT_COVER_CHECK` | Check if object fully covers an area |
| `LV_EVENT_REFR_EXT_DRAW_SIZE` | Query extra draw area needed |
| `LV_EVENT_DRAW_MAIN_BEGIN` | Before main drawing |
| `LV_EVENT_DRAW_MAIN` | Main drawing |
| `LV_EVENT_DRAW_MAIN_END` | After main drawing |
| `LV_EVENT_DRAW_POST_BEGIN` | Before post drawing |
| `LV_EVENT_DRAW_POST` | Post drawing (on top of children) |
| `LV_EVENT_DRAW_POST_END` | After post drawing |
| `LV_EVENT_DRAW_PART_BEGIN` | Before drawing a part (customization hook) |
| `LV_EVENT_DRAW_PART_END` | After drawing a part |

#### Widget-Specific Events

| Code | Description |
|------|-------------|
| `LV_EVENT_VALUE_CHANGED` | Value changed (slider, checkbox, dropdown, etc.) |
| `LV_EVENT_INSERT` | Text inserted (textarea) |
| `LV_EVENT_REFRESH` | Manual refresh notification |
| `LV_EVENT_READY` | Process completed |
| `LV_EVENT_CANCEL` | Process cancelled |

#### Lifecycle Events

| Code | Description |
|------|-------------|
| `LV_EVENT_DELETE` | Object being deleted |
| `LV_EVENT_CHILD_CHANGED` | Child added/removed |
| `LV_EVENT_CHILD_CREATED` | Child created |
| `LV_EVENT_CHILD_DELETED` | Child deleted |
| `LV_EVENT_SIZE_CHANGED` | Size changed |
| `LV_EVENT_STYLE_CHANGED` | Style changed |
| `LV_EVENT_LAYOUT_CHANGED` | Layout recalculated |
| `LV_EVENT_GET_SELF_SIZE` | Query widget's own content size |

#### Screen Events

| Code | Description |
|------|-------------|
| `LV_EVENT_SCREEN_UNLOAD_START` | Screen unload animation started |
| `LV_EVENT_SCREEN_LOAD_START` | Screen load animation started |
| `LV_EVENT_SCREEN_LOADED` | Screen fully loaded |
| `LV_EVENT_SCREEN_UNLOADED` | Screen fully unloaded |

---

## 8. Animation API

Header: `lv_anim.h`

### Animation Configuration

```c
lv_anim_t a;
lv_anim_init(&a);                                    // Initialize with defaults

// Required
lv_anim_set_var(&a, obj);                            // Target variable/object
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);  // Animator function
lv_anim_set_values(&a, start_val, end_val);          // Start and end values
lv_anim_set_time(&a, duration_ms);                   // Duration

// Optional
lv_anim_set_delay(&a, delay_ms);                     // Delay before start
lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);  // Easing function
lv_anim_set_ready_cb(&a, my_ready_cb);               // Called when animation completes
lv_anim_set_start_cb(&a, my_start_cb);               // Called when animation starts
lv_anim_set_playback_time(&a, playback_ms);          // Reverse animation duration
lv_anim_set_playback_delay(&a, delay_ms);            // Delay before reverse
lv_anim_set_repeat_count(&a, count);                 // Repeat count (LV_ANIM_REPEAT_INFINITE)
lv_anim_set_repeat_delay(&a, delay_ms);              // Delay between repeats
lv_anim_set_early_apply(&a, true);                   // Apply start value before delay

// Start
lv_anim_start(&a);                                   // Begin animation
```

### Animation Control

```c
/**
 * Delete animation by variable and exec_cb pair.
 * @param var   animation target variable
 * @param exec_cb  exec callback (NULL to delete all animations on var)
 * @return      true if animation was found and deleted
 */
bool lv_anim_del(void * var, lv_anim_exec_xcb_t exec_cb);

lv_anim_t * lv_anim_get(void * var, lv_anim_exec_xcb_t exec_cb);
uint16_t lv_anim_count_running(void);
uint32_t lv_anim_speed_to_time(uint32_t speed, int32_t start, int32_t end);

// Callback signatures
typedef void (*lv_anim_exec_xcb_t)(void * var, int32_t value);
typedef void (*lv_anim_ready_cb_t)(lv_anim_t * a);
typedef void (*lv_anim_start_cb_t)(lv_anim_t * a);
typedef int32_t (*lv_anim_path_cb_t)(const lv_anim_t * a);
```

### Built-in Animation Paths (Easing Functions)

```c
int32_t lv_anim_path_linear(const lv_anim_t * a);       // Constant speed
int32_t lv_anim_path_ease_in(const lv_anim_t * a);      // Slow start, fast end
int32_t lv_anim_path_ease_out(const lv_anim_t * a);     // Fast start, slow end
int32_t lv_anim_path_ease_in_out(const lv_anim_t * a);  // Slow start and end
int32_t lv_anim_path_overshoot(const lv_anim_t * a);    // Exceed target, then settle
int32_t lv_anim_path_bounce(const lv_anim_t * a);       // Bounce at end
int32_t lv_anim_path_step(const lv_anim_t * a);         // Jump to end value instantly
```

### Animation Timeline

```c
lv_anim_timeline_t * lv_anim_timeline_create(void);

/**
 * Add animation to timeline at a specific start time.
 * @param at         timeline handle
 * @param start_time  start time in ms from timeline start
 * @param a          animation to add (copied internally)
 */
void lv_anim_timeline_add(lv_anim_timeline_t * at, uint32_t start_time, lv_anim_t * a);

void lv_anim_timeline_start(lv_anim_timeline_t * at);
void lv_anim_timeline_stop(lv_anim_timeline_t * at);
void lv_anim_timeline_del(lv_anim_timeline_t * at);
void lv_anim_timeline_set_reverse(lv_anim_timeline_t * at, bool reverse);
void lv_anim_timeline_set_progress(lv_anim_timeline_t * at, uint16_t progress);  // 0-65535
uint32_t lv_anim_timeline_get_playtime(lv_anim_timeline_t * at);
bool lv_anim_timeline_get_reverse(lv_anim_timeline_t * at);
```

### Common Animation Patterns

```c
// Fade in
lv_anim_t a;
lv_anim_init(&a);
lv_anim_set_var(&a, obj);
lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);  // Note: needs wrapper
lv_anim_set_time(&a, 500);
lv_anim_start(&a);

// Move object
lv_anim_init(&a);
lv_anim_set_var(&a, obj);
lv_anim_set_values(&a, lv_obj_get_x(obj), 200);
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
lv_anim_set_time(&a, 300);
lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
lv_anim_start(&a);

// Infinite pulse (scale up/down)
lv_anim_init(&a);
lv_anim_set_var(&a, obj);
lv_anim_set_values(&a, 256, 280);  // 256 = 1x zoom
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)set_zoom_wrapper);
lv_anim_set_time(&a, 800);
lv_anim_set_playback_time(&a, 800);
lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
lv_anim_start(&a);
```

---

## 9. Layout APIs

### Flex Layout

Header: `lv_flex.h` | Config: `LV_USE_FLEX 1`

```c
// Set flex layout on container
lv_obj_set_layout(obj, LV_LAYOUT_FLEX);

/**
 * Set flex flow direction and wrapping.
 * @param obj   container object
 * @param flow  LV_FLEX_FLOW_* constant
 */
void lv_obj_set_flex_flow(lv_obj_t * obj, lv_flex_flow_t flow);

/**
 * Set flex alignment for main axis, cross axis, and tracks.
 * @param obj               container object
 * @param main_place        main axis alignment
 * @param cross_place       cross axis alignment
 * @param track_cross_place alignment of tracks relative to each other
 */
void lv_obj_set_flex_align(lv_obj_t * obj,
    lv_flex_align_t main_place,
    lv_flex_align_t cross_place,
    lv_flex_align_t track_cross_place);

/**
 * Set flex grow factor on a child (> 0 to participate in free space distribution).
 * @param obj    child object
 * @param grow   grow factor (0 = no grow)
 */
void lv_obj_set_flex_grow(lv_obj_t * obj, uint8_t grow);
```

#### Flex Flow Constants

| Constant | Direction | Wrap |
|----------|-----------|------|
| `LV_FLEX_FLOW_ROW` | Horizontal | No |
| `LV_FLEX_FLOW_COLUMN` | Vertical | No |
| `LV_FLEX_FLOW_ROW_WRAP` | Horizontal | Yes |
| `LV_FLEX_FLOW_COLUMN_WRAP` | Vertical | Yes |
| `LV_FLEX_FLOW_ROW_REVERSE` | Horizontal RTL | No |
| `LV_FLEX_FLOW_COLUMN_REVERSE` | Vertical BTT | No |
| `LV_FLEX_FLOW_ROW_WRAP_REVERSE` | Horizontal RTL | Yes |
| `LV_FLEX_FLOW_COLUMN_WRAP_REVERSE` | Vertical BTT | Yes |

#### Flex Alignment Constants

| Constant | Description |
|----------|-------------|
| `LV_FLEX_ALIGN_START` | Pack items at start |
| `LV_FLEX_ALIGN_END` | Pack items at end |
| `LV_FLEX_ALIGN_CENTER` | Center items |
| `LV_FLEX_ALIGN_SPACE_EVENLY` | Equal spacing around all items |
| `LV_FLEX_ALIGN_SPACE_AROUND` | Equal spacing around each item |
| `LV_FLEX_ALIGN_SPACE_BETWEEN` | Space between items only |

#### Flex Example

```c
lv_obj_t * cont = lv_obj_create(lv_scr_act());
lv_obj_set_size(cont, 300, 200);
lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
lv_obj_set_style_pad_row(cont, 10, 0);
lv_obj_set_style_pad_column(cont, 10, 0);

for (int i = 0; i < 5; i++) {
    lv_obj_t * btn = lv_btn_create(cont);
    lv_obj_set_size(btn, 80, 40);
    if (i == 2) lv_obj_set_flex_grow(btn, 1);  // This button expands
}

// Force new track (line break)
lv_obj_add_flag(some_child, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
```

### Grid Layout

Header: `lv_grid.h` | Config: `LV_USE_GRID 1`

```c
// Set grid layout on container
lv_obj_set_layout(obj, LV_LAYOUT_GRID);

/**
 * Set grid column and row descriptors.
 * @param obj      container object
 * @param col_dsc  array of column widths ending with LV_GRID_TEMPLATE_LAST
 * @param row_dsc  array of row heights ending with LV_GRID_TEMPLATE_LAST
 */
void lv_obj_set_grid_dsc_array(lv_obj_t * obj,
    const lv_coord_t col_dsc[],
    const lv_coord_t row_dsc[]);

/**
 * Set grid track alignment.
 * @param obj        container object
 * @param col_align  column track alignment
 * @param row_align  row track alignment
 */
void lv_obj_set_grid_align(lv_obj_t * obj,
    lv_grid_align_t col_align,
    lv_grid_align_t row_align);

/**
 * Place a child in a grid cell.
 * @param obj        child object
 * @param col_align  alignment within cell (LV_GRID_ALIGN_*)
 * @param col_pos    column index (0-based)
 * @param col_span   number of columns to span (>= 1)
 * @param row_align  alignment within cell
 * @param row_pos    row index (0-based)
 * @param row_span   number of rows to span (>= 1)
 */
void lv_obj_set_grid_cell(lv_obj_t * obj,
    lv_grid_align_t col_align, uint8_t col_pos, uint8_t col_span,
    lv_grid_align_t row_align, uint8_t row_pos, uint8_t row_span);
```

#### Grid Special Values

| Value | Description |
|-------|-------------|
| `LV_GRID_TEMPLATE_LAST` | End of descriptor array |
| `LV_GRID_CONTENT` | Track size matches largest child |
| `LV_GRID_FR(x)` | Proportional free space (1 FR = 1 share) |

#### Grid Alignment Constants

| Constant | Cell Use | Track Use |
|----------|----------|-----------|
| `LV_GRID_ALIGN_START` | Align to start of cell | Pack tracks at start |
| `LV_GRID_ALIGN_END` | Align to end of cell | Pack tracks at end |
| `LV_GRID_ALIGN_CENTER` | Center in cell | Center tracks |
| `LV_GRID_ALIGN_STRETCH` | Fill cell entirely | N/A |
| `LV_GRID_ALIGN_SPACE_EVENLY` | N/A | Even spacing |
| `LV_GRID_ALIGN_SPACE_AROUND` | N/A | Space around each track |
| `LV_GRID_ALIGN_SPACE_BETWEEN` | N/A | Space between tracks |

#### Grid Example

```c
static lv_coord_t col_dsc[] = {70, LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
static lv_coord_t row_dsc[] = {50, 50, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

lv_obj_t * cont = lv_obj_create(lv_scr_act());
lv_obj_set_size(cont, 300, 200);
lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
lv_obj_set_style_pad_row(cont, 5, 0);
lv_obj_set_style_pad_column(cont, 5, 0);

// Place button at column 0, row 0, spanning 1 col and 1 row
lv_obj_t * btn = lv_btn_create(cont);
lv_obj_set_grid_cell(btn,
    LV_GRID_ALIGN_STRETCH, 0, 1,    // col: stretch, pos 0, span 1
    LV_GRID_ALIGN_CENTER, 0, 1);    // row: center, pos 0, span 1

// Label spanning 2 columns
lv_obj_t * label = lv_label_create(cont);
lv_label_set_text(label, "Spanning 2 cols");
lv_obj_set_grid_cell(label,
    LV_GRID_ALIGN_CENTER, 1, 2,     // col: center, pos 1, span 2
    LV_GRID_ALIGN_CENTER, 0, 1);    // row: center, pos 0, span 1
```

---

## 10. Color and Drawing API

Header: `lv_color.h`, `lv_draw.h`

### Color Functions

```c
// Color creation
lv_color_t lv_color_hex(uint32_t c);                    // e.g., lv_color_hex(0xFF0000) = red
lv_color_t lv_color_hex3(uint32_t c);                   // e.g., lv_color_hex3(0xF00) = red
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b);

// Predefined colors
lv_color_t lv_color_white(void);
lv_color_t lv_color_black(void);

// Palette colors
lv_color_t lv_palette_main(lv_palette_t p);             // Main palette color
lv_color_t lv_palette_lighten(lv_palette_t p, uint8_t lvl);  // lvl: 1-5
lv_color_t lv_palette_darken(lv_palette_t p, uint8_t lvl);   // lvl: 1-4

// Color operations
lv_color_t lv_color_mix(lv_color_t c1, lv_color_t c2, uint8_t mix);  // mix: 0=c2, 255=c1
uint8_t lv_color_brightness(lv_color_t c);              // 0-255 perceived brightness
lv_color_hsv_t lv_color_to_hsv(lv_color_t color);
lv_color_t lv_color_hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v);
lv_color_t lv_color_chroma_key(void);
```

### Palette Names (LV_PALETTE_*)

`RED`, `PINK`, `PURPLE`, `DEEP_PURPLE`, `INDIGO`, `BLUE`, `LIGHT_BLUE`, `CYAN`, `TEAL`, `GREEN`, `LIGHT_GREEN`, `LIME`, `YELLOW`, `AMBER`, `ORANGE`, `DEEP_ORANGE`, `BROWN`, `BLUE_GREY`, `GREY`

### Opacity Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `LV_OPA_TRANSP` | 0 | Fully transparent |
| `LV_OPA_0` | 0 | 0% |
| `LV_OPA_10` | 25 | 10% |
| `LV_OPA_20` | 51 | 20% |
| `LV_OPA_30` | 76 | 30% |
| `LV_OPA_40` | 102 | 40% |
| `LV_OPA_50` | 127 | 50% |
| `LV_OPA_60` | 153 | 60% |
| `LV_OPA_70` | 178 | 70% |
| `LV_OPA_80` | 204 | 80% |
| `LV_OPA_90` | 229 | 90% |
| `LV_OPA_100` | 255 | 100% |
| `LV_OPA_COVER` | 255 | Fully opaque |

### Draw Descriptors

```c
// Rectangle drawing descriptor
lv_draw_rect_dsc_t rect_dsc;
lv_draw_rect_dsc_init(&rect_dsc);
rect_dsc.bg_color = lv_color_hex(0xFF0000);
rect_dsc.bg_opa = LV_OPA_COVER;
rect_dsc.border_width = 2;
rect_dsc.border_color = lv_color_black();
rect_dsc.radius = 10;
rect_dsc.shadow_width = 5;

// Label drawing descriptor
lv_draw_label_dsc_t label_dsc;
lv_draw_label_dsc_init(&label_dsc);
label_dsc.color = lv_color_white();
label_dsc.font = &lv_font_montserrat_14;

// Image drawing descriptor
lv_draw_img_dsc_t img_dsc;
lv_draw_img_dsc_init(&img_dsc);
img_dsc.recolor = lv_color_hex(0xFF0000);
img_dsc.recolor_opa = LV_OPA_50;
img_dsc.angle = 450;  // 45.0 degrees
img_dsc.zoom = 512;   // 2x zoom

// Line drawing descriptor
lv_draw_line_dsc_t line_dsc;
lv_draw_line_dsc_init(&line_dsc);
line_dsc.color = lv_color_hex(0x00FF00);
line_dsc.width = 3;
line_dsc.round_start = 1;
line_dsc.round_end = 1;

// Arc drawing descriptor
lv_draw_arc_dsc_t arc_dsc;
lv_draw_arc_dsc_init(&arc_dsc);
arc_dsc.color = lv_palette_main(LV_PALETTE_BLUE);
arc_dsc.width = 5;
```

---

## 11. File System API

Header: `lv_fs.h`

```c
// File system driver registration
void lv_fs_drv_init(lv_fs_drv_t * drv);
void lv_fs_drv_register(lv_fs_drv_t * drv);

// lv_fs_drv_t key fields:
// .letter           -- drive letter (e.g., 'S' for stdio)
// .ready_cb         -- check if driver is ready
// .open_cb          -- open file
// .close_cb         -- close file
// .read_cb          -- read from file (CHANGED in v8.2: caching support)
// .write_cb         -- write to file
// .seek_cb          -- seek in file
// .tell_cb          -- get current position
// .dir_open_cb      -- open directory
// .dir_read_cb      -- read directory entry
// .dir_close_cb     -- close directory

// File operations
lv_fs_res_t lv_fs_open(lv_fs_file_t * file_p, const char * path, lv_fs_mode_t mode);
  // mode: LV_FS_MODE_WR, LV_FS_MODE_RD
lv_fs_res_t lv_fs_close(lv_fs_file_t * file_p);
lv_fs_res_t lv_fs_read(lv_fs_file_t * file_p, void * buf, uint32_t btr, uint32_t * br);
lv_fs_res_t lv_fs_write(lv_fs_file_t * file_p, const void * buf, uint32_t btw, uint32_t * bw);
lv_fs_res_t lv_fs_seek(lv_fs_file_t * file_p, uint32_t pos, lv_fs_whence_t whence);
  // whence: LV_FS_SEEK_SET, LV_FS_SEEK_CUR, LV_FS_SEEK_END
lv_fs_res_t lv_fs_tell(lv_fs_file_t * file_p, uint32_t * pos_p);

// Directory operations
lv_fs_res_t lv_fs_dir_open(lv_fs_dir_t * rddir_p, const char * path);
lv_fs_res_t lv_fs_dir_read(lv_fs_dir_t * rddir_p, char * fn);
lv_fs_res_t lv_fs_dir_close(lv_fs_dir_t * rddir_p);
```

---

## 12. Timer API

Header: `lv_timer.h`

```c
/**
 * Create a timer that calls cb periodically.
 * @param timer_xcb  callback function
 * @param period     call period in ms
 * @param user_data  custom data
 * @return           timer handle
 */
lv_timer_t * lv_timer_create(lv_timer_cb_t timer_xcb, uint32_t period, void * user_data);

void lv_timer_del(lv_timer_t * timer);
void lv_timer_pause(lv_timer_t * timer);
void lv_timer_resume(lv_timer_t * timer);
void lv_timer_set_cb(lv_timer_t * timer, lv_timer_cb_t timer_cb);
void lv_timer_set_period(lv_timer_t * timer, uint32_t period);
void lv_timer_set_repeat_count(lv_timer_t * timer, int32_t repeat_count);  // -1 = infinite
void lv_timer_reset(lv_timer_t * timer);
void lv_timer_ready(lv_timer_t * timer);  // Force next call immediately
lv_timer_t * lv_timer_get_next(lv_timer_t * timer);  // NULL to start

// Main timer handler (call from main loop)
uint32_t lv_timer_handler(void);  // Returns ms until next timer needs to run

// Callback signature
typedef void (*lv_timer_cb_t)(lv_timer_t * timer);
```

---

## 13. Font API

Header: `lv_font.h`

### Built-in Montserrat Fonts

Enable in `lv_conf.h` with `LV_FONT_MONTSERRAT_<size>`:

| Variable | Size | Coverage |
|----------|------|----------|
| `lv_font_montserrat_8` | 8px | Basic Latin |
| `lv_font_montserrat_10` | 10px | Basic Latin |
| `lv_font_montserrat_12` | 12px | Basic Latin |
| `lv_font_montserrat_14` | 14px | Basic Latin (default) |
| `lv_font_montserrat_16` | 16px | Basic Latin |
| `lv_font_montserrat_18` | 18px | Basic Latin |
| `lv_font_montserrat_20` | 20px | Basic Latin |
| `lv_font_montserrat_22` | 22px | Basic Latin |
| `lv_font_montserrat_24` | 24px | Basic Latin |
| `lv_font_montserrat_26` | 26px | Basic Latin |
| `lv_font_montserrat_28` | 28px | Basic Latin |
| `lv_font_montserrat_30` | 30px | Basic Latin |
| `lv_font_montserrat_32` | 32px | Basic Latin |
| `lv_font_montserrat_34` | 34px | Basic Latin |
| `lv_font_montserrat_36` | 36px | Basic Latin |
| `lv_font_montserrat_38` | 38px | Basic Latin |
| `lv_font_montserrat_40` | 40px | Basic Latin |
| `lv_font_montserrat_42` | 42px | Basic Latin |
| `lv_font_montserrat_44` | 44px | Basic Latin |
| `lv_font_montserrat_46` | 46px | Basic Latin |
| `lv_font_montserrat_48` | 48px | Basic Latin |

### Special Fonts

| Variable | Description |
|----------|-------------|
| `lv_font_montserrat_12_subpx` | 12px with sub-pixel rendering |
| `lv_font_montserrat_28_compressed` | 28px compressed (3x smaller, slower) |
| `lv_font_dejavu_16_persian_hebrew` | Persian, Hebrew + Basic Latin |
| `lv_font_simsun_16_cjk` | CJK Unified Ideographs |
| `lv_font_unscii_8` | 8px monospace pixel font |
| `lv_font_unscii_16` | 16px monospace pixel font |

### Symbol Fonts

Built-in symbols accessible via `LV_SYMBOL_*` macros:

| Symbol | Constant | Symbol | Constant |
|--------|----------|--------|----------|
| Audio | `LV_SYMBOL_AUDIO` | Video | `LV_SYMBOL_VIDEO` |
| List | `LV_SYMBOL_LIST` | OK | `LV_SYMBOL_OK` |
| Close | `LV_SYMBOL_CLOSE` | Power | `LV_SYMBOL_POWER` |
| Settings | `LV_SYMBOL_SETTINGS` | Home | `LV_SYMBOL_HOME` |
| Download | `LV_SYMBOL_DOWNLOAD` | Drive | `LV_SYMBOL_DRIVE` |
| Refresh | `LV_SYMBOL_REFRESH` | Play | `LV_SYMBOL_PLAY` |
| Pause | `LV_SYMBOL_PAUSE` | Stop | `LV_SYMBOL_STOP` |
| Next | `LV_SYMBOL_NEXT` | Prev | `LV_SYMBOL_PREV` |
| Edit | `LV_SYMBOL_EDIT` | Plus | `LV_SYMBOL_PLUS` |
| Minus | `LV_SYMBOL_MINUS` | Warning | `LV_SYMBOL_WARNING` |
| Trash | `LV_SYMBOL_TRASH` | WiFi | `LV_SYMBOL_WIFI` |
| Battery | `LV_SYMBOL_BATTERY_*` | Bluetooth | `LV_SYMBOL_BLUETOOTH` |
| Left | `LV_SYMBOL_LEFT` | Right | `LV_SYMBOL_RIGHT` |
| Up | `LV_SYMBOL_UP` | Down | `LV_SYMBOL_DOWN` |
| Backspace | `LV_SYMBOL_BACKSPACE` | Eye Open | `LV_SYMBOL_EYE_OPEN` |
| Eye Close | `LV_SYMBOL_EYE_CLOSE` | Keyboard | `LV_SYMBOL_KEYBOARD` |

### Font Fallback (NEW in v8.2)

```c
// Set fallback font for missing glyphs
static lv_font_t my_font = /* ... */;
my_font.fallback = &lv_font_montserrat_14;
// If my_font doesn't contain a glyph, lv_font_montserrat_14 is tried
```

### FreeType Integration

```c
// In lv_conf.h:
#define LV_USE_FREETYPE 1

// Create FreeType font
lv_ft_info_t info;
info.name = "/path/to/font.ttf";
info.weight = 24;
info.style = FT_FONT_STYLE_NORMAL;  // or FT_FONT_STYLE_BOLD, FT_FONT_STYLE_ITALIC (v8.2)
lv_ft_font_init(&info);
// Use info.font as lv_font_t pointer
```

---

## 14. Memory API

Header: `lv_mem.h`

```c
// Allocation
void * lv_mem_alloc(size_t size);
void * lv_mem_realloc(void * data_p, size_t new_size);
void lv_mem_free(void * data);

// Utilities
void lv_memcpy(void * dst, const void * src, size_t len);
void lv_memset(void * dst, uint8_t v, size_t len);
void lv_memset_00(void * dst, size_t len);   // memset to 0
void lv_memset_ff(void * dst, size_t len);   // memset to 0xFF

// Monitoring
typedef struct {
    uint32_t total_size;          // Total pool size
    uint32_t free_cnt;            // Number of free chunks
    uint32_t free_size;           // Total free bytes
    uint32_t free_biggest_size;   // Largest contiguous free block
    uint32_t used_cnt;            // Number of used chunks
    uint8_t used_pct;             // Used percentage (0-100)
    uint8_t frag_pct;             // Fragmentation percentage (0-100)
} lv_mem_monitor_t;

void lv_mem_monitor(lv_mem_monitor_t * mon_p);

// Buffer management
void * lv_mem_buf_get(uint32_t size);    // Get temporary buffer
void lv_mem_buf_release(void * p);       // Release temporary buffer
void lv_mem_buf_free_all(void);          // Free all temporary buffers
```

---

## Appendix: Image Formats

### Image Color Formats (lv_img_cf_t)

| Constant | Description |
|----------|-------------|
| `LV_IMG_CF_TRUE_COLOR` | Matches LV_COLOR_DEPTH |
| `LV_IMG_CF_TRUE_COLOR_ALPHA` | True color + 8-bit alpha |
| `LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED` | True color with chroma key transparency |
| `LV_IMG_CF_INDEXED_1BIT` | 2-color palette |
| `LV_IMG_CF_INDEXED_2BIT` | 4-color palette |
| `LV_IMG_CF_INDEXED_4BIT` | 16-color palette |
| `LV_IMG_CF_INDEXED_8BIT` | 256-color palette |
| `LV_IMG_CF_ALPHA_1BIT` | 1-bit alpha mask |
| `LV_IMG_CF_ALPHA_2BIT` | 2-bit alpha mask |
| `LV_IMG_CF_ALPHA_4BIT` | 4-bit alpha mask |
| `LV_IMG_CF_ALPHA_8BIT` | 8-bit alpha mask |
| `LV_IMG_CF_RAW` | Custom format (user decoder) |
| `LV_IMG_CF_RAW_ALPHA` | Custom format with alpha |
| `LV_IMG_CF_RAW_CHROMA_KEYED` | Custom format with chroma key |

### Image Declaration for C Arrays

```c
// Generated by LVGL image converter tool
const lv_img_dsc_t my_image = {
    .header = {
        .cf = LV_IMG_CF_TRUE_COLOR,
        .always_zero = 0,
        .reserved = 0,
        .w = 100,
        .h = 100,
    },
    .data_size = 100 * 100 * LV_COLOR_SIZE / 8,
    .data = my_image_map,
};

// Usage:
lv_img_set_src(img_obj, &my_image);

// File path (with registered FS driver):
lv_img_set_src(img_obj, "S:path/to/image.bin");

// Symbol:
lv_img_set_src(img_obj, LV_SYMBOL_OK);
```
