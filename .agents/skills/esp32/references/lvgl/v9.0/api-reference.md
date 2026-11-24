# LVGL v9.0 API Reference

> Complete API surface for LVGL v9.0. Organized by module for AI agent consumption.

---

## Table of Contents

1. [Core Initialization](#1-core-initialization)
2. [Display API](#2-display-api)
3. [Input Device API](#3-input-device-api)
4. [Object (Widget Base) API](#4-object-widget-base-api)
5. [Event API](#5-event-api)
6. [Style API](#6-style-api)
7. [Animation API](#7-animation-api)
8. [Timer API](#8-timer-api)
9. [Widget APIs](#9-widget-apis)
10. [Layout API](#10-layout-api)
11. [Draw API](#11-draw-api)
12. [Observer API](#12-observer-api)
13. [Color API](#13-color-api)
14. [Font API](#14-font-api)
15. [Image API](#15-image-api)
16. [File System API](#16-file-system-api)
17. [Memory API](#17-memory-api)
18. [Logging API](#18-logging-api)
19. [Group API](#19-group-api)
20. [Theme API](#20-theme-api)

---

## 1. Core Initialization

```c
void lv_init(void);
void lv_deinit(void);
bool lv_is_initialized(void);

// Tick (MUST be called periodically from timer ISR or similar)
void lv_tick_inc(uint32_t tick_period_ms);
uint32_t lv_tick_get(void);
uint32_t lv_tick_elaps(uint32_t prev_tick);

// Main loop handler (call in main loop or RTOS task)
uint32_t lv_timer_handler(void);

// Screen management
lv_obj_t *lv_screen_active(void);           // was lv_scr_act()
void lv_screen_load(lv_obj_t *scr);         // was lv_scr_load()
void lv_screen_load_anim(lv_obj_t *scr, lv_scr_load_anim_t anim,
                         uint32_t time, uint32_t delay, bool auto_del);
```

---

## 2. Display API

### Creation and Configuration
```c
lv_display_t *lv_display_create(int32_t hor_res, int32_t ver_res);
void lv_display_delete(lv_display_t *disp);

void lv_display_set_flush_cb(lv_display_t *disp,
    void (*flush_cb)(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map));
void lv_display_set_buffers(lv_display_t *disp,
    void *buf1, void *buf2, uint32_t buf_size_in_bytes,
    lv_display_render_mode_t render_mode);
void lv_display_set_color_format(lv_display_t *disp, lv_color_format_t cf);
void lv_display_set_resolution(lv_display_t *disp, int32_t hor_res, int32_t ver_res);
void lv_display_set_offset(lv_display_t *disp, int32_t x, int32_t y);
void lv_display_set_rotation(lv_display_t *disp, lv_display_rotation_t rotation);
void lv_display_set_dpi(lv_display_t *disp, int32_t dpi);
void lv_display_set_default(lv_display_t *disp);
void lv_display_set_user_data(lv_display_t *disp, void *user_data);
void lv_display_set_driver_data(lv_display_t *disp, void *driver_data);
```

### Flush Completion
```c
// Call at end of flush_cb when async transfer complete
void lv_display_flush_ready(lv_display_t *disp);
// Check if flushing
bool lv_display_flush_is_last(lv_display_t *disp);
```

### Getters
```c
lv_display_t *lv_display_get_default(void);
lv_display_t *lv_display_get_next(lv_display_t *disp);

int32_t lv_display_get_hor_res(lv_display_t *disp);
int32_t lv_display_get_ver_res(lv_display_t *disp);
int32_t lv_display_get_dpi(lv_display_t *disp);
lv_color_format_t lv_display_get_color_format(lv_display_t *disp);
lv_display_rotation_t lv_display_get_rotation(lv_display_t *disp);
void *lv_display_get_user_data(lv_display_t *disp);
void *lv_display_get_driver_data(lv_display_t *disp);
```

### Screen Management
```c
lv_obj_t *lv_display_get_screen_active(lv_display_t *disp);
lv_obj_t *lv_display_get_screen_prev(lv_display_t *disp);
lv_obj_t *lv_display_get_layer_top(lv_display_t *disp);
lv_obj_t *lv_display_get_layer_sys(lv_display_t *disp);
lv_obj_t *lv_display_get_layer_bottom(lv_display_t *disp);
```

### Events
```c
void lv_display_add_event_cb(lv_display_t *disp,
    lv_event_cb_t event_cb, lv_event_code_t filter, void *user_data);
uint32_t lv_display_get_event_count(lv_display_t *disp);
lv_event_dsc_t *lv_display_get_event_dsc(lv_display_t *disp, uint32_t index);
lv_result_t lv_display_send_event(lv_display_t *disp, lv_event_code_t code, void *param);
```

### Render Modes
```c
typedef enum {
    LV_DISPLAY_RENDER_MODE_PARTIAL,   // Small buffers, partial refresh
    LV_DISPLAY_RENDER_MODE_DIRECT,    // Screen-sized buffer, direct mode
    LV_DISPLAY_RENDER_MODE_FULL,      // Full screen redraw each cycle
} lv_display_render_mode_t;
```

### Display Rotation
```c
typedef enum {
    LV_DISPLAY_ROTATION_0,
    LV_DISPLAY_ROTATION_90,
    LV_DISPLAY_ROTATION_180,
    LV_DISPLAY_ROTATION_270,
} lv_display_rotation_t;
```

### Color Formats
```c
typedef enum {
    LV_COLOR_FORMAT_UNKNOWN = 0,
    LV_COLOR_FORMAT_L8,               // 8-bit luminance
    LV_COLOR_FORMAT_I1,               // 1-bit indexed
    LV_COLOR_FORMAT_I2,               // 2-bit indexed
    LV_COLOR_FORMAT_I4,               // 4-bit indexed
    LV_COLOR_FORMAT_I8,               // 8-bit indexed
    LV_COLOR_FORMAT_A8,               // 8-bit alpha only
    LV_COLOR_FORMAT_RGB565,           // 16-bit RGB
    LV_COLOR_FORMAT_RGB565_SWAPPED,   // 16-bit RGB byte-swapped (SPI displays)
    LV_COLOR_FORMAT_RGB888,           // 24-bit RGB
    LV_COLOR_FORMAT_ARGB8888,         // 32-bit ARGB
    LV_COLOR_FORMAT_XRGB8888,         // 32-bit RGB (alpha ignored)
    LV_COLOR_FORMAT_NATIVE,           // Native format per LV_COLOR_DEPTH
    LV_COLOR_FORMAT_NATIVE_REVERSED,  // Native byte-swapped
    LV_COLOR_FORMAT_NATIVE_ALPHA,     // Native with alpha channel
} lv_color_format_t;
```

---

## 3. Input Device API

### Creation and Configuration
```c
lv_indev_t *lv_indev_create(void);
void lv_indev_delete(lv_indev_t *indev);

void lv_indev_set_type(lv_indev_t *indev, lv_indev_type_t type);
void lv_indev_set_read_cb(lv_indev_t *indev,
    void (*read_cb)(lv_indev_t *indev, lv_indev_data_t *data));
void lv_indev_set_display(lv_indev_t *indev, lv_display_t *disp);
void lv_indev_set_group(lv_indev_t *indev, lv_group_t *group);
void lv_indev_set_cursor(lv_indev_t *indev, lv_obj_t *cur_obj);
void lv_indev_set_user_data(lv_indev_t *indev, void *user_data);
void lv_indev_set_driver_data(lv_indev_t *indev, void *driver_data);
```

### Getters
```c
lv_indev_type_t lv_indev_get_type(lv_indev_t *indev);
lv_indev_t *lv_indev_get_next(lv_indev_t *indev);
lv_indev_t *lv_indev_active(void);
void *lv_indev_get_user_data(lv_indev_t *indev);
void *lv_indev_get_driver_data(lv_indev_t *indev);
lv_obj_t *lv_indev_get_active_obj(lv_indev_t *indev);
void lv_indev_get_point(lv_indev_t *indev, lv_point_t *point);
uint32_t lv_indev_get_key(lv_indev_t *indev);
lv_dir_t lv_indev_get_scroll_dir(lv_indev_t *indev);
lv_obj_t *lv_indev_get_scroll_obj(lv_indev_t *indev);
void lv_indev_get_vect(lv_indev_t *indev, lv_point_t *point);
```

### Input Device Types
```c
typedef enum {
    LV_INDEV_TYPE_NONE,
    LV_INDEV_TYPE_POINTER,   // Touchpad, mouse
    LV_INDEV_TYPE_KEYPAD,    // Keyboard, keypad
    LV_INDEV_TYPE_BUTTON,    // External hardware buttons
    LV_INDEV_TYPE_ENCODER,   // Rotary encoder
} lv_indev_type_t;
```

### Read Callback Data
```c
typedef struct {
    lv_point_t point;         // For pointer type
    uint32_t key;             // For keypad type
    uint32_t btn_id;          // For button type
    int16_t enc_diff;         // For encoder type
    lv_indev_state_t state;   // LV_INDEV_STATE_PRESSED or LV_INDEV_STATE_RELEASED
    bool continue_reading;    // Set true if more data available
} lv_indev_data_t;
```

---

## 4. Object (Widget Base) API

### Creation and Deletion
```c
lv_obj_t *lv_obj_create(lv_obj_t *parent);
void lv_obj_delete(lv_obj_t *obj);            // was lv_obj_del()
void lv_obj_delete_async(lv_obj_t *obj);      // was lv_obj_del_async()
void lv_obj_clean(lv_obj_t *obj);             // Delete all children
void lv_obj_delete_delayed(lv_obj_t *obj, uint32_t delay_ms);
```

### Position and Size
```c
void lv_obj_set_pos(lv_obj_t *obj, int32_t x, int32_t y);
void lv_obj_set_x(lv_obj_t *obj, int32_t x);
void lv_obj_set_y(lv_obj_t *obj, int32_t y);
void lv_obj_set_size(lv_obj_t *obj, int32_t w, int32_t h);
void lv_obj_set_width(lv_obj_t *obj, int32_t w);
void lv_obj_set_height(lv_obj_t *obj, int32_t h);
void lv_obj_set_content_width(lv_obj_t *obj, int32_t w);
void lv_obj_set_content_height(lv_obj_t *obj, int32_t h);

int32_t lv_obj_get_x(lv_obj_t *obj);
int32_t lv_obj_get_y(lv_obj_t *obj);
int32_t lv_obj_get_x2(lv_obj_t *obj);
int32_t lv_obj_get_y2(lv_obj_t *obj);
int32_t lv_obj_get_width(lv_obj_t *obj);
int32_t lv_obj_get_height(lv_obj_t *obj);
int32_t lv_obj_get_content_width(lv_obj_t *obj);
int32_t lv_obj_get_content_height(lv_obj_t *obj);
int32_t lv_obj_get_self_width(lv_obj_t *obj);
int32_t lv_obj_get_self_height(lv_obj_t *obj);
```

### Alignment
```c
void lv_obj_set_align(lv_obj_t *obj, lv_align_t align);
void lv_obj_align(lv_obj_t *obj, lv_align_t align, int32_t x_ofs, int32_t y_ofs);
void lv_obj_align_to(lv_obj_t *obj, const lv_obj_t *base,
                     lv_align_t align, int32_t x_ofs, int32_t y_ofs);
void lv_obj_center(lv_obj_t *obj);

// Alignment constants
typedef enum {
    LV_ALIGN_DEFAULT,
    LV_ALIGN_TOP_LEFT, LV_ALIGN_TOP_MID, LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_MID, LV_ALIGN_BOTTOM_RIGHT,
    LV_ALIGN_LEFT_MID, LV_ALIGN_RIGHT_MID, LV_ALIGN_CENTER,
    LV_ALIGN_OUT_TOP_LEFT, LV_ALIGN_OUT_TOP_MID, LV_ALIGN_OUT_TOP_RIGHT,
    LV_ALIGN_OUT_BOTTOM_LEFT, LV_ALIGN_OUT_BOTTOM_MID, LV_ALIGN_OUT_BOTTOM_RIGHT,
    LV_ALIGN_OUT_LEFT_TOP, LV_ALIGN_OUT_LEFT_MID, LV_ALIGN_OUT_LEFT_BOTTOM,
    LV_ALIGN_OUT_RIGHT_TOP, LV_ALIGN_OUT_RIGHT_MID, LV_ALIGN_OUT_RIGHT_BOTTOM,
} lv_align_t;
```

### Flags
```c
void lv_obj_add_flag(lv_obj_t *obj, lv_obj_flag_t f);
void lv_obj_remove_flag(lv_obj_t *obj, lv_obj_flag_t f);    // was lv_obj_clear_flag()
bool lv_obj_has_flag(lv_obj_t *obj, lv_obj_flag_t f);
bool lv_obj_has_flag_any(lv_obj_t *obj, lv_obj_flag_t f);

// Common flags
#define LV_OBJ_FLAG_HIDDEN
#define LV_OBJ_FLAG_CLICKABLE
#define LV_OBJ_FLAG_CLICK_FOCUSABLE
#define LV_OBJ_FLAG_CHECKABLE
#define LV_OBJ_FLAG_SCROLLABLE
#define LV_OBJ_FLAG_SCROLL_ELASTIC
#define LV_OBJ_FLAG_SCROLL_MOMENTUM
#define LV_OBJ_FLAG_SCROLL_ONE
#define LV_OBJ_FLAG_SCROLL_CHAIN_HOR
#define LV_OBJ_FLAG_SCROLL_CHAIN_VER
#define LV_OBJ_FLAG_SCROLL_ON_FOCUS
#define LV_OBJ_FLAG_SCROLL_WITH_ARROW
#define LV_OBJ_FLAG_SNAPPABLE
#define LV_OBJ_FLAG_PRESS_LOCK
#define LV_OBJ_FLAG_EVENT_BUBBLE
#define LV_OBJ_FLAG_GESTURE_BUBBLE
#define LV_OBJ_FLAG_ADV_HITTEST
#define LV_OBJ_FLAG_IGNORE_LAYOUT
#define LV_OBJ_FLAG_FLOATING
#define LV_OBJ_FLAG_OVERFLOW_VISIBLE
#define LV_OBJ_FLAG_FLEX_IN_NEW_TRACK
#define LV_OBJ_FLAG_LAYOUT_1
#define LV_OBJ_FLAG_LAYOUT_2
#define LV_OBJ_FLAG_WIDGET_1
#define LV_OBJ_FLAG_WIDGET_2
#define LV_OBJ_FLAG_USER_1
#define LV_OBJ_FLAG_USER_2
#define LV_OBJ_FLAG_USER_3
#define LV_OBJ_FLAG_USER_4
```

### States
```c
void lv_obj_add_state(lv_obj_t *obj, lv_state_t state);
void lv_obj_remove_state(lv_obj_t *obj, lv_state_t state);  // was lv_obj_clear_state()
bool lv_obj_has_state(lv_obj_t *obj, lv_state_t state);
lv_state_t lv_obj_get_state(lv_obj_t *obj);

// State constants
#define LV_STATE_DEFAULT     0x0000
#define LV_STATE_CHECKED     0x0001
#define LV_STATE_FOCUSED     0x0002
#define LV_STATE_FOCUS_KEY   0x0004
#define LV_STATE_EDITED      0x0008
#define LV_STATE_HOVERED     0x0010
#define LV_STATE_PRESSED     0x0020
#define LV_STATE_SCROLLED    0x0040
#define LV_STATE_DISABLED    0x0080
#define LV_STATE_USER_1      0x1000
#define LV_STATE_USER_2      0x2000
#define LV_STATE_USER_3      0x4000
#define LV_STATE_USER_4      0x8000
#define LV_STATE_ANY         0xFFFF
```

### Parent/Child
```c
void lv_obj_set_parent(lv_obj_t *obj, lv_obj_t *parent);
lv_obj_t *lv_obj_get_parent(lv_obj_t *obj);
lv_obj_t *lv_obj_get_child(lv_obj_t *obj, int32_t idx);
lv_obj_t *lv_obj_get_child_by_type(lv_obj_t *obj, int32_t idx, const lv_obj_class_t *class_p);
uint32_t lv_obj_get_child_count(lv_obj_t *obj);
uint32_t lv_obj_get_child_count_by_type(lv_obj_t *obj, const lv_obj_class_t *class_p);
int32_t lv_obj_get_index(lv_obj_t *obj);
lv_obj_t *lv_obj_get_screen(lv_obj_t *obj);
lv_display_t *lv_obj_get_display(lv_obj_t *obj);
```

### Scrolling
```c
void lv_obj_set_scrollbar_mode(lv_obj_t *obj, lv_scrollbar_mode_t mode);
void lv_obj_set_scroll_dir(lv_obj_t *obj, lv_dir_t dir);
void lv_obj_set_scroll_snap_x(lv_obj_t *obj, lv_scroll_snap_t align);
void lv_obj_set_scroll_snap_y(lv_obj_t *obj, lv_scroll_snap_t align);

void lv_obj_scroll_by(lv_obj_t *obj, int32_t dx, int32_t dy, lv_anim_enable_t anim_en);
void lv_obj_scroll_to(lv_obj_t *obj, int32_t x, int32_t y, lv_anim_enable_t anim_en);
void lv_obj_scroll_to_x(lv_obj_t *obj, int32_t x, lv_anim_enable_t anim_en);
void lv_obj_scroll_to_y(lv_obj_t *obj, int32_t y, lv_anim_enable_t anim_en);
void lv_obj_scroll_to_view(lv_obj_t *obj, lv_anim_enable_t anim_en);
void lv_obj_scroll_to_view_recursive(lv_obj_t *obj, lv_anim_enable_t anim_en);

int32_t lv_obj_get_scroll_x(lv_obj_t *obj);
int32_t lv_obj_get_scroll_y(lv_obj_t *obj);
int32_t lv_obj_get_scroll_top(lv_obj_t *obj);
int32_t lv_obj_get_scroll_bottom(lv_obj_t *obj);
int32_t lv_obj_get_scroll_left(lv_obj_t *obj);
int32_t lv_obj_get_scroll_right(lv_obj_t *obj);
```

### Miscellaneous
```c
void lv_obj_invalidate(lv_obj_t *obj);
bool lv_obj_is_valid(lv_obj_t *obj);
void lv_obj_set_user_data(lv_obj_t *obj, void *user_data);
void *lv_obj_get_user_data(lv_obj_t *obj);
void lv_obj_move_foreground(lv_obj_t *obj);
void lv_obj_move_background(lv_obj_t *obj);
void lv_obj_swap(lv_obj_t *obj1, lv_obj_t *obj2);
void lv_obj_update_layout(lv_obj_t *obj);
```

---

## 5. Event API

### Registration
```c
lv_event_dsc_t *lv_obj_add_event_cb(lv_obj_t *obj, lv_event_cb_t event_cb,
                                     lv_event_code_t filter, void *user_data);
bool lv_obj_remove_event_cb(lv_obj_t *obj, lv_event_cb_t event_cb);
bool lv_obj_remove_event_dsc(lv_obj_t *obj, lv_event_dsc_t *dsc);
lv_result_t lv_obj_send_event(lv_obj_t *obj, lv_event_code_t event, void *param);

// Callback signature
typedef void (*lv_event_cb_t)(lv_event_t *e);
```

### Event Data Access
```c
lv_event_code_t lv_event_get_code(lv_event_t *e);
void *lv_event_get_target(lv_event_t *e);               // Returns void*
lv_obj_t *lv_event_get_target_obj(lv_event_t *e);       // Returns lv_obj_t* (preferred)
void *lv_event_get_current_target(lv_event_t *e);       // Returns void*
lv_obj_t *lv_event_get_current_target_obj(lv_event_t *e);
void *lv_event_get_user_data(lv_event_t *e);
void *lv_event_get_param(lv_event_t *e);
```

### Common Event Codes
```c
// Input events
LV_EVENT_PRESSED
LV_EVENT_PRESSING
LV_EVENT_PRESS_LOST
LV_EVENT_SHORT_CLICKED
LV_EVENT_LONG_PRESSED
LV_EVENT_LONG_PRESSED_REPEAT
LV_EVENT_CLICKED
LV_EVENT_RELEASED
LV_EVENT_SCROLL_BEGIN
LV_EVENT_SCROLL_THROW_BEGIN
LV_EVENT_SCROLL_END
LV_EVENT_SCROLL
LV_EVENT_GESTURE
LV_EVENT_KEY
LV_EVENT_FOCUSED
LV_EVENT_DEFOCUSED
LV_EVENT_LEAVE
LV_EVENT_HIT_TEST

// Drawing events
LV_EVENT_COVER_CHECK
LV_EVENT_REFR_EXT_DRAW_SIZE
LV_EVENT_DRAW_MAIN_BEGIN
LV_EVENT_DRAW_MAIN
LV_EVENT_DRAW_MAIN_END
LV_EVENT_DRAW_POST_BEGIN
LV_EVENT_DRAW_POST
LV_EVENT_DRAW_POST_END
LV_EVENT_DRAW_TASK_ADDED

// Special events
LV_EVENT_VALUE_CHANGED
LV_EVENT_INSERT
LV_EVENT_REFRESH
LV_EVENT_READY
LV_EVENT_CANCEL
LV_EVENT_DELETE
LV_EVENT_CHILD_CHANGED
LV_EVENT_CHILD_CREATED
LV_EVENT_CHILD_DELETED
LV_EVENT_SCREEN_UNLOAD_START
LV_EVENT_SCREEN_LOAD_START
LV_EVENT_SCREEN_LOADED
LV_EVENT_SCREEN_UNLOADED
LV_EVENT_SIZE_CHANGED
LV_EVENT_STYLE_CHANGED
LV_EVENT_LAYOUT_CHANGED
LV_EVENT_GET_SELF_SIZE

// Display events
LV_EVENT_RENDER_START
LV_EVENT_RENDER_READY
LV_EVENT_FLUSH_START
LV_EVENT_FLUSH_FINISH
```

---

## 6. Style API

### Style Object
```c
void lv_style_init(lv_style_t *style);
void lv_style_reset(lv_style_t *style);

// Apply to objects
void lv_obj_add_style(lv_obj_t *obj, lv_style_t *style, lv_style_selector_t selector);
void lv_obj_remove_style(lv_obj_t *obj, lv_style_t *style, lv_style_selector_t selector);
void lv_obj_remove_style_all(lv_obj_t *obj);
void lv_obj_refresh_style(lv_obj_t *obj, lv_style_selector_t selector, lv_style_prop_t prop);

// Selector: part | state
// Example: LV_PART_MAIN | LV_STATE_DEFAULT
// Example: LV_PART_INDICATOR | LV_STATE_PRESSED
```

### Widget Parts
```c
#define LV_PART_MAIN         0x000000
#define LV_PART_SCROLLBAR    0x010000
#define LV_PART_INDICATOR    0x020000
#define LV_PART_KNOB         0x030000
#define LV_PART_SELECTED     0x040000
#define LV_PART_ITEMS        0x050000
#define LV_PART_CURSOR       0x060000
#define LV_PART_CUSTOM_FIRST 0x080000
#define LV_PART_ANY          0x0F0000
```

### Style Property Setters (All lv_style_set_* functions)

#### Size and Position
```c
void lv_style_set_width(lv_style_t *s, int32_t v);
void lv_style_set_min_width(lv_style_t *s, int32_t v);
void lv_style_set_max_width(lv_style_t *s, int32_t v);
void lv_style_set_height(lv_style_t *s, int32_t v);
void lv_style_set_min_height(lv_style_t *s, int32_t v);
void lv_style_set_max_height(lv_style_t *s, int32_t v);
void lv_style_set_x(lv_style_t *s, int32_t v);
void lv_style_set_y(lv_style_t *s, int32_t v);
void lv_style_set_align(lv_style_t *s, lv_align_t v);
```

#### Padding and Margin
```c
void lv_style_set_pad_top(lv_style_t *s, int32_t v);
void lv_style_set_pad_bottom(lv_style_t *s, int32_t v);
void lv_style_set_pad_left(lv_style_t *s, int32_t v);
void lv_style_set_pad_right(lv_style_t *s, int32_t v);
void lv_style_set_pad_row(lv_style_t *s, int32_t v);
void lv_style_set_pad_column(lv_style_t *s, int32_t v);
void lv_style_set_margin_top(lv_style_t *s, int32_t v);
void lv_style_set_margin_bottom(lv_style_t *s, int32_t v);
void lv_style_set_margin_left(lv_style_t *s, int32_t v);
void lv_style_set_margin_right(lv_style_t *s, int32_t v);
```

#### Background
```c
void lv_style_set_bg_color(lv_style_t *s, lv_color_t v);
void lv_style_set_bg_opa(lv_style_t *s, lv_opa_t v);
void lv_style_set_bg_grad_color(lv_style_t *s, lv_color_t v);
void lv_style_set_bg_grad_dir(lv_style_t *s, lv_grad_dir_t v);
void lv_style_set_bg_main_stop(lv_style_t *s, int32_t v);
void lv_style_set_bg_grad_stop(lv_style_t *s, int32_t v);
void lv_style_set_bg_image_src(lv_style_t *s, const void *v);   // was bg_img_src
void lv_style_set_bg_image_opa(lv_style_t *s, lv_opa_t v);
void lv_style_set_bg_image_recolor(lv_style_t *s, lv_color_t v);
void lv_style_set_bg_image_recolor_opa(lv_style_t *s, lv_opa_t v);
void lv_style_set_bg_image_tiled(lv_style_t *s, bool v);
```

#### Border
```c
void lv_style_set_border_color(lv_style_t *s, lv_color_t v);
void lv_style_set_border_opa(lv_style_t *s, lv_opa_t v);
void lv_style_set_border_width(lv_style_t *s, int32_t v);
void lv_style_set_border_side(lv_style_t *s, lv_border_side_t v);
void lv_style_set_border_post(lv_style_t *s, bool v);
```

#### Outline
```c
void lv_style_set_outline_width(lv_style_t *s, int32_t v);
void lv_style_set_outline_color(lv_style_t *s, lv_color_t v);
void lv_style_set_outline_opa(lv_style_t *s, lv_opa_t v);
void lv_style_set_outline_pad(lv_style_t *s, int32_t v);
```

#### Shadow
```c
void lv_style_set_shadow_width(lv_style_t *s, int32_t v);
void lv_style_set_shadow_offset_x(lv_style_t *s, int32_t v);
void lv_style_set_shadow_offset_y(lv_style_t *s, int32_t v);
void lv_style_set_shadow_spread(lv_style_t *s, int32_t v);
void lv_style_set_shadow_color(lv_style_t *s, lv_color_t v);
void lv_style_set_shadow_opa(lv_style_t *s, lv_opa_t v);
```

#### Text
```c
void lv_style_set_text_color(lv_style_t *s, lv_color_t v);
void lv_style_set_text_opa(lv_style_t *s, lv_opa_t v);
void lv_style_set_text_font(lv_style_t *s, const lv_font_t *v);
void lv_style_set_text_letter_space(lv_style_t *s, int32_t v);
void lv_style_set_text_line_space(lv_style_t *s, int32_t v);
void lv_style_set_text_decor(lv_style_t *s, lv_text_decor_t v);
void lv_style_set_text_align(lv_style_t *s, lv_text_align_t v);
```

#### Image
```c
void lv_style_set_image_opa(lv_style_t *s, lv_opa_t v);         // was img_opa
void lv_style_set_image_recolor(lv_style_t *s, lv_color_t v);   // was img_recolor
void lv_style_set_image_recolor_opa(lv_style_t *s, lv_opa_t v); // was img_recolor_opa
```

#### Line
```c
void lv_style_set_line_width(lv_style_t *s, int32_t v);
void lv_style_set_line_dash_width(lv_style_t *s, int32_t v);
void lv_style_set_line_dash_gap(lv_style_t *s, int32_t v);
void lv_style_set_line_rounded(lv_style_t *s, bool v);
void lv_style_set_line_color(lv_style_t *s, lv_color_t v);
void lv_style_set_line_opa(lv_style_t *s, lv_opa_t v);
```

#### Arc
```c
void lv_style_set_arc_width(lv_style_t *s, int32_t v);
void lv_style_set_arc_rounded(lv_style_t *s, bool v);
void lv_style_set_arc_color(lv_style_t *s, lv_color_t v);
void lv_style_set_arc_opa(lv_style_t *s, lv_opa_t v);
void lv_style_set_arc_image_src(lv_style_t *s, const void *v);
```

#### Transform
```c
void lv_style_set_transform_width(lv_style_t *s, int32_t v);
void lv_style_set_transform_height(lv_style_t *s, int32_t v);
void lv_style_set_translate_x(lv_style_t *s, int32_t v);
void lv_style_set_translate_y(lv_style_t *s, int32_t v);
void lv_style_set_transform_scale_x(lv_style_t *s, int32_t v);  // was transform_zoom
void lv_style_set_transform_scale_y(lv_style_t *s, int32_t v);  // was transform_zoom
void lv_style_set_transform_rotation(lv_style_t *s, int32_t v); // was transform_angle
void lv_style_set_transform_pivot_x(lv_style_t *s, int32_t v);
void lv_style_set_transform_pivot_y(lv_style_t *s, int32_t v);
```

#### Miscellaneous
```c
void lv_style_set_radius(lv_style_t *s, int32_t v);
void lv_style_set_clip_corner(lv_style_t *s, bool v);
void lv_style_set_opa(lv_style_t *s, lv_opa_t v);
void lv_style_set_color_filter_dsc(lv_style_t *s, const lv_color_filter_dsc_t *v);
void lv_style_set_color_filter_opa(lv_style_t *s, lv_opa_t v);
void lv_style_set_anim(lv_style_t *s, const lv_anim_t *v);
void lv_style_set_anim_time(lv_style_t *s, uint32_t v);
void lv_style_set_anim_speed(lv_style_t *s, uint32_t v);
void lv_style_set_transition(lv_style_t *s, const lv_style_transition_dsc_t *v);
void lv_style_set_blend_mode(lv_style_t *s, lv_blend_mode_t v);
void lv_style_set_layout(lv_style_t *s, uint16_t v);
void lv_style_set_base_dir(lv_style_t *s, lv_base_dir_t v);
```

### Local Styles (Set directly on object)
```c
// Pattern: lv_obj_set_style_<prop>(obj, value, selector)
void lv_obj_set_style_width(lv_obj_t *obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_text_font(lv_obj_t *obj, const lv_font_t *value, lv_style_selector_t selector);
// ... (same pattern for all style properties)
```

---

## 7. Animation API

```c
void lv_anim_init(lv_anim_t *a);
lv_anim_t *lv_anim_start(lv_anim_t *a);
bool lv_anim_delete(void *var, lv_anim_exec_xcb_t exec_cb);

void lv_anim_set_var(lv_anim_t *a, void *var);
void lv_anim_set_exec_cb(lv_anim_t *a, lv_anim_exec_xcb_t exec_cb);
void lv_anim_set_time(lv_anim_t *a, uint32_t duration);
void lv_anim_set_delay(lv_anim_t *a, uint32_t delay);
void lv_anim_set_values(lv_anim_t *a, int32_t start, int32_t end);
void lv_anim_set_path_cb(lv_anim_t *a, lv_anim_path_cb_t path_cb);
void lv_anim_set_start_cb(lv_anim_t *a, lv_anim_start_cb_t start_cb);
void lv_anim_set_ready_cb(lv_anim_t *a, lv_anim_ready_cb_t ready_cb);
void lv_anim_set_deleted_cb(lv_anim_t *a, lv_anim_deleted_cb_t deleted_cb);
void lv_anim_set_playback_time(lv_anim_t *a, uint32_t time);
void lv_anim_set_playback_delay(lv_anim_t *a, uint32_t delay);
void lv_anim_set_repeat_count(lv_anim_t *a, uint16_t cnt);
void lv_anim_set_repeat_delay(lv_anim_t *a, uint32_t delay);
void lv_anim_set_early_apply(lv_anim_t *a, bool en);

// Built-in path callbacks
int32_t lv_anim_path_linear(const lv_anim_t *a);
int32_t lv_anim_path_ease_in(const lv_anim_t *a);
int32_t lv_anim_path_ease_out(const lv_anim_t *a);
int32_t lv_anim_path_ease_in_out(const lv_anim_t *a);
int32_t lv_anim_path_overshoot(const lv_anim_t *a);
int32_t lv_anim_path_bounce(const lv_anim_t *a);
int32_t lv_anim_path_step(const lv_anim_t *a);

// Repeat constants
#define LV_ANIM_REPEAT_INFINITE  0xFFFF
```

---

## 8. Timer API

```c
lv_timer_t *lv_timer_create(lv_timer_cb_t timer_xcb, uint32_t period, void *user_data);
void lv_timer_delete(lv_timer_t *timer);
void lv_timer_pause(lv_timer_t *timer);
void lv_timer_resume(lv_timer_t *timer);
void lv_timer_set_cb(lv_timer_t *timer, lv_timer_cb_t timer_cb);
void lv_timer_set_period(lv_timer_t *timer, uint32_t period);
void lv_timer_set_repeat_count(lv_timer_t *timer, int32_t repeat_count);
void lv_timer_ready(lv_timer_t *timer);
void lv_timer_reset(lv_timer_t *timer);

// Callback signature
typedef void (*lv_timer_cb_t)(lv_timer_t *timer);
```

---

## 9. Widget APIs

### Label (lv_label)
```c
lv_obj_t *lv_label_create(lv_obj_t *parent);
void lv_label_set_text(lv_obj_t *obj, const char *text);
void lv_label_set_text_fmt(lv_obj_t *obj, const char *fmt, ...);
void lv_label_set_text_static(lv_obj_t *obj, const char *text);
void lv_label_set_long_mode(lv_obj_t *obj, lv_label_long_mode_t long_mode);
void lv_label_set_text_selection_start(lv_obj_t *obj, uint32_t index);
void lv_label_set_text_selection_end(lv_obj_t *obj, uint32_t index);
const char *lv_label_get_text(lv_obj_t *obj);
lv_label_long_mode_t lv_label_get_long_mode(lv_obj_t *obj);

// Long modes
LV_LABEL_LONG_WRAP          // Wrap text
LV_LABEL_LONG_DOT           // Add dots at end
LV_LABEL_LONG_SCROLL        // Scroll left/right
LV_LABEL_LONG_SCROLL_CIRCULAR  // Circular scroll
LV_LABEL_LONG_CLIP          // Clip overflow
```

### Button (lv_button) -- was lv_btn
```c
lv_obj_t *lv_button_create(lv_obj_t *parent);  // was lv_btn_create()
// Button has no specific API -- use lv_obj_* functions
// Style with LV_PART_MAIN
// Typically add a label child:
lv_obj_t *label = lv_label_create(btn);
lv_label_set_text(label, "Click me");
lv_obj_center(label);
```

### Image (lv_image) -- was lv_img
```c
lv_obj_t *lv_image_create(lv_obj_t *parent);          // was lv_img_create()
void lv_image_set_src(lv_obj_t *obj, const void *src); // was lv_img_set_src()
void lv_image_set_offset_x(lv_obj_t *obj, int32_t x);
void lv_image_set_offset_y(lv_obj_t *obj, int32_t y);
void lv_image_set_rotation(lv_obj_t *obj, int32_t angle);  // was lv_img_set_angle()
void lv_image_set_scale(lv_obj_t *obj, uint32_t zoom);     // was lv_img_set_zoom()
void lv_image_set_scale_x(lv_obj_t *obj, uint32_t zoom);
void lv_image_set_scale_y(lv_obj_t *obj, uint32_t zoom);
void lv_image_set_pivot(lv_obj_t *obj, int32_t x, int32_t y);
void lv_image_set_inner_align(lv_obj_t *obj, lv_image_align_t align);  // NEW

// Image alignment (new in v9)
LV_IMAGE_ALIGN_DEFAULT
LV_IMAGE_ALIGN_TOP_LEFT
LV_IMAGE_ALIGN_TOP_MID
LV_IMAGE_ALIGN_TOP_RIGHT
LV_IMAGE_ALIGN_BOTTOM_LEFT
LV_IMAGE_ALIGN_BOTTOM_MID
LV_IMAGE_ALIGN_BOTTOM_RIGHT
LV_IMAGE_ALIGN_LEFT_MID
LV_IMAGE_ALIGN_RIGHT_MID
LV_IMAGE_ALIGN_CENTER
LV_IMAGE_ALIGN_STRETCH
LV_IMAGE_ALIGN_TILE

const void *lv_image_get_src(lv_obj_t *obj);
int32_t lv_image_get_rotation(lv_obj_t *obj);
uint32_t lv_image_get_scale(lv_obj_t *obj);
```

### Slider (lv_slider)
```c
lv_obj_t *lv_slider_create(lv_obj_t *parent);
void lv_slider_set_value(lv_obj_t *obj, int32_t value, lv_anim_enable_t anim);
void lv_slider_set_left_value(lv_obj_t *obj, int32_t value, lv_anim_enable_t anim);
void lv_slider_set_range(lv_obj_t *obj, int32_t min, int32_t max);
void lv_slider_set_mode(lv_obj_t *obj, lv_slider_mode_t mode);
int32_t lv_slider_get_value(lv_obj_t *obj);
int32_t lv_slider_get_left_value(lv_obj_t *obj);
int32_t lv_slider_get_min_value(lv_obj_t *obj);
int32_t lv_slider_get_max_value(lv_obj_t *obj);
bool lv_slider_is_dragged(lv_obj_t *obj);

// Modes: LV_SLIDER_MODE_NORMAL, LV_SLIDER_MODE_SYMMETRICAL, LV_SLIDER_MODE_RANGE
// Parts: LV_PART_MAIN (background), LV_PART_INDICATOR (filled), LV_PART_KNOB
```

### Arc (lv_arc)
```c
lv_obj_t *lv_arc_create(lv_obj_t *parent);
void lv_arc_set_start_angle(lv_obj_t *obj, uint32_t start);
void lv_arc_set_end_angle(lv_obj_t *obj, uint32_t end);
void lv_arc_set_angles(lv_obj_t *obj, uint32_t start, uint32_t end);
void lv_arc_set_bg_start_angle(lv_obj_t *obj, uint32_t start);
void lv_arc_set_bg_end_angle(lv_obj_t *obj, uint32_t end);
void lv_arc_set_bg_angles(lv_obj_t *obj, uint32_t start, uint32_t end);
void lv_arc_set_rotation(lv_obj_t *obj, int32_t rotation);
void lv_arc_set_mode(lv_obj_t *obj, lv_arc_mode_t type);
void lv_arc_set_value(lv_obj_t *obj, int32_t value);
void lv_arc_set_range(lv_obj_t *obj, int32_t min, int32_t max);
void lv_arc_set_change_rate(lv_obj_t *obj, uint32_t rate);
void lv_arc_set_knob_offset(lv_obj_t *obj, int32_t offset);
```

### Bar (lv_bar)
```c
lv_obj_t *lv_bar_create(lv_obj_t *parent);
void lv_bar_set_value(lv_obj_t *obj, int32_t value, lv_anim_enable_t anim);
void lv_bar_set_start_value(lv_obj_t *obj, int32_t value, lv_anim_enable_t anim);
void lv_bar_set_range(lv_obj_t *obj, int32_t min, int32_t max);
void lv_bar_set_mode(lv_obj_t *obj, lv_bar_mode_t mode);
int32_t lv_bar_get_value(lv_obj_t *obj);
int32_t lv_bar_get_start_value(lv_obj_t *obj);
int32_t lv_bar_get_min_value(lv_obj_t *obj);
int32_t lv_bar_get_max_value(lv_obj_t *obj);
```

### Switch (lv_switch)
```c
lv_obj_t *lv_switch_create(lv_obj_t *parent);
// Use state: lv_obj_add_state(sw, LV_STATE_CHECKED) to turn on
// Use state: lv_obj_remove_state(sw, LV_STATE_CHECKED) to turn off  (was clear_state)
// Check: lv_obj_has_state(sw, LV_STATE_CHECKED)
```

### Checkbox (lv_checkbox)
```c
lv_obj_t *lv_checkbox_create(lv_obj_t *parent);
void lv_checkbox_set_text(lv_obj_t *obj, const char *txt);
void lv_checkbox_set_text_static(lv_obj_t *obj, const char *txt);
const char *lv_checkbox_get_text(lv_obj_t *obj);
```

### Dropdown (lv_dropdown)
```c
lv_obj_t *lv_dropdown_create(lv_obj_t *parent);
void lv_dropdown_set_text(lv_obj_t *obj, const char *txt);
void lv_dropdown_set_options(lv_obj_t *obj, const char *options);
void lv_dropdown_set_options_static(lv_obj_t *obj, const char *options);
void lv_dropdown_add_option(lv_obj_t *obj, const char *option, uint32_t pos);
void lv_dropdown_set_selected(lv_obj_t *obj, uint32_t sel_opt);
void lv_dropdown_set_dir(lv_obj_t *obj, lv_dir_t dir);
void lv_dropdown_set_symbol(lv_obj_t *obj, const void *symbol);
void lv_dropdown_set_selected_highlight(lv_obj_t *obj, bool en);
uint32_t lv_dropdown_get_selected(lv_obj_t *obj);
uint32_t lv_dropdown_get_option_count(lv_obj_t *obj);
void lv_dropdown_get_selected_str(lv_obj_t *obj, char *buf, uint32_t buf_size);
const char *lv_dropdown_get_options(lv_obj_t *obj);
void lv_dropdown_open(lv_obj_t *obj);
void lv_dropdown_close(lv_obj_t *obj);
bool lv_dropdown_is_open(lv_obj_t *obj);
```

### Roller (lv_roller)
```c
lv_obj_t *lv_roller_create(lv_obj_t *parent);
void lv_roller_set_options(lv_obj_t *obj, const char *options, lv_roller_mode_t mode);
void lv_roller_set_selected(lv_obj_t *obj, uint32_t sel_opt, lv_anim_enable_t anim);
void lv_roller_set_visible_row_count(lv_obj_t *obj, uint32_t row_cnt);
uint32_t lv_roller_get_selected(lv_obj_t *obj);
void lv_roller_get_selected_str(lv_obj_t *obj, char *buf, uint32_t buf_size);
uint32_t lv_roller_get_option_count(lv_obj_t *obj);
```

### Text Area (lv_textarea)
```c
lv_obj_t *lv_textarea_create(lv_obj_t *parent);
void lv_textarea_add_char(lv_obj_t *obj, uint32_t c);
void lv_textarea_add_text(lv_obj_t *obj, const char *txt);
void lv_textarea_delete_char(lv_obj_t *obj);
void lv_textarea_delete_char_forward(lv_obj_t *obj);
void lv_textarea_set_text(lv_obj_t *obj, const char *txt);
void lv_textarea_set_placeholder_text(lv_obj_t *obj, const char *txt);
void lv_textarea_set_cursor_pos(lv_obj_t *obj, int32_t pos);
void lv_textarea_set_cursor_click_pos(lv_obj_t *obj, bool en);
void lv_textarea_set_password_mode(lv_obj_t *obj, bool en);
void lv_textarea_set_one_line(lv_obj_t *obj, bool en);
void lv_textarea_set_accepted_chars(lv_obj_t *obj, const char *list);
void lv_textarea_set_max_length(lv_obj_t *obj, uint32_t num);
void lv_textarea_set_insert_replace(lv_obj_t *obj, const char *txt);
void lv_textarea_set_password_show_time(lv_obj_t *obj, uint32_t time);
const char *lv_textarea_get_text(lv_obj_t *obj);
const char *lv_textarea_get_placeholder_text(lv_obj_t *obj);
lv_obj_t *lv_textarea_get_label(lv_obj_t *obj);
uint32_t lv_textarea_get_cursor_pos(lv_obj_t *obj);
bool lv_textarea_get_password_mode(lv_obj_t *obj);
bool lv_textarea_get_one_line(lv_obj_t *obj);
const char *lv_textarea_get_accepted_chars(lv_obj_t *obj);
uint32_t lv_textarea_get_max_length(lv_obj_t *obj);
bool lv_textarea_text_is_selected(lv_obj_t *obj);
```

### Keyboard (lv_keyboard)
```c
lv_obj_t *lv_keyboard_create(lv_obj_t *parent);
void lv_keyboard_set_textarea(lv_obj_t *kb, lv_obj_t *ta);
void lv_keyboard_set_mode(lv_obj_t *kb, lv_keyboard_mode_t mode);
void lv_keyboard_set_popovers(lv_obj_t *kb, bool en);
void lv_keyboard_set_map(lv_obj_t *kb, lv_keyboard_mode_t mode,
                         const char *map[], const lv_buttonmatrix_ctrl_t ctrl_map[]);
lv_obj_t *lv_keyboard_get_textarea(lv_obj_t *kb);
lv_keyboard_mode_t lv_keyboard_get_mode(lv_obj_t *kb);

// Modes: LV_KEYBOARD_MODE_TEXT_LOWER, _TEXT_UPPER, _SPECIAL, _NUMBER
```

### Scale (lv_scale) -- NEW, replaces lv_meter
```c
lv_obj_t *lv_scale_create(lv_obj_t *parent);
void lv_scale_set_mode(lv_obj_t *obj, lv_scale_mode_t mode);
void lv_scale_set_total_tick_count(lv_obj_t *obj, uint32_t total_tick_count);
void lv_scale_set_major_tick_every(lv_obj_t *obj, uint32_t major_tick_every);
void lv_scale_set_label_show(lv_obj_t *obj, bool show_label);
void lv_scale_set_range(lv_obj_t *obj, int32_t min, int32_t max);
void lv_scale_set_angle_range(lv_obj_t *obj, uint32_t angle_range);
void lv_scale_set_rotation(lv_obj_t *obj, int32_t rotation);

// Sections (subsections with custom styles)
lv_scale_section_t *lv_scale_add_section(lv_obj_t *obj);
void lv_scale_section_set_range(lv_scale_section_t *section, int32_t min, int32_t max);
void lv_scale_section_set_style(lv_scale_section_t *section, uint32_t part, lv_style_t *style);

// Modes: LV_SCALE_MODE_HORIZONTAL_TOP, _HORIZONTAL_BOTTOM,
//        LV_SCALE_MODE_VERTICAL_LEFT, _VERTICAL_RIGHT,
//        LV_SCALE_MODE_ROUND_INNER, _ROUND_OUTER
```

### Chart (lv_chart)
```c
lv_obj_t *lv_chart_create(lv_obj_t *parent);
void lv_chart_set_type(lv_obj_t *obj, lv_chart_type_t type);
void lv_chart_set_point_count(lv_obj_t *obj, uint32_t cnt);
void lv_chart_set_range(lv_obj_t *obj, lv_chart_axis_t axis, int32_t min, int32_t max);
void lv_chart_set_update_mode(lv_obj_t *obj, lv_chart_update_mode_t update_mode);
void lv_chart_set_div_line_count(lv_obj_t *obj, uint8_t hdiv, uint8_t vdiv);
void lv_chart_set_zoom_x(lv_obj_t *obj, uint16_t zoom_x);
void lv_chart_set_zoom_y(lv_obj_t *obj, uint16_t zoom_y);

lv_chart_series_t *lv_chart_add_series(lv_obj_t *obj, lv_color_t color, lv_chart_axis_t axis);
void lv_chart_remove_series(lv_obj_t *obj, lv_chart_series_t *series);
void lv_chart_set_next_value(lv_obj_t *obj, lv_chart_series_t *ser, int32_t value);
void lv_chart_set_next_value2(lv_obj_t *obj, lv_chart_series_t *ser, int32_t x, int32_t y);
void lv_chart_set_value_by_id(lv_obj_t *obj, lv_chart_series_t *ser, uint32_t id, int32_t value);
void lv_chart_set_value_by_id2(lv_obj_t *obj, lv_chart_series_t *ser, uint32_t id, int32_t x, int32_t y);
void lv_chart_set_ext_y_array(lv_obj_t *obj, lv_chart_series_t *ser, int32_t array[]);
void lv_chart_set_ext_x_array(lv_obj_t *obj, lv_chart_series_t *ser, int32_t array[]);
void lv_chart_refresh(lv_obj_t *obj);

// Note: Chart ticks removed -- use lv_scale widget
```

### Table (lv_table)
```c
lv_obj_t *lv_table_create(lv_obj_t *parent);
void lv_table_set_cell_value(lv_obj_t *obj, uint32_t row, uint32_t col, const char *txt);
void lv_table_set_cell_value_fmt(lv_obj_t *obj, uint32_t row, uint32_t col, const char *fmt, ...);
void lv_table_set_row_count(lv_obj_t *obj, uint32_t row_cnt);
void lv_table_set_column_count(lv_obj_t *obj, uint32_t col_cnt);
void lv_table_set_column_width(lv_obj_t *obj, uint32_t col_id, int32_t w);
void lv_table_add_cell_ctrl(lv_obj_t *obj, uint32_t row, uint32_t col, lv_table_cell_ctrl_t ctrl);
void lv_table_clear_cell_ctrl(lv_obj_t *obj, uint32_t row, uint32_t col, lv_table_cell_ctrl_t ctrl);
uint32_t lv_table_get_row_count(lv_obj_t *obj);
uint32_t lv_table_get_column_count(lv_obj_t *obj);
int32_t lv_table_get_column_width(lv_obj_t *obj, uint32_t col);
const char *lv_table_get_cell_value(lv_obj_t *obj, uint32_t row, uint32_t col);
void lv_table_get_selected_cell(lv_obj_t *obj, uint32_t *row, uint32_t *col);
```

### Canvas (lv_canvas)
```c
lv_obj_t *lv_canvas_create(lv_obj_t *parent);
void lv_canvas_set_draw_buf(lv_obj_t *obj, lv_draw_buf_t *draw_buf);  // NEW API
void lv_canvas_set_px(lv_obj_t *obj, int32_t x, int32_t y, lv_color_t color, lv_opa_t opa);
lv_color_t lv_canvas_get_px(lv_obj_t *obj, int32_t x, int32_t y);
lv_draw_buf_t *lv_canvas_get_draw_buf(lv_obj_t *obj);
void lv_canvas_fill_bg(lv_obj_t *obj, lv_color_t color, lv_opa_t opa);

// Canvas drawing layer
lv_layer_t *lv_canvas_init_layer(lv_obj_t *canvas);
void lv_canvas_finish_layer(lv_obj_t *canvas, lv_layer_t *layer);

// Draw buffer macro
LV_DRAW_BUF_DEFINE(name, w, h, cf);  // Define a static draw buffer
```

### Button Matrix (lv_buttonmatrix) -- was lv_btnmatrix
```c
lv_obj_t *lv_buttonmatrix_create(lv_obj_t *parent);  // was lv_btnmatrix_create()
void lv_buttonmatrix_set_map(lv_obj_t *obj, const char *map[]);
void lv_buttonmatrix_set_ctrl_map(lv_obj_t *obj, const lv_buttonmatrix_ctrl_t ctrl_map[]);
void lv_buttonmatrix_set_selected_button(lv_obj_t *obj, uint32_t btn_id);
void lv_buttonmatrix_set_button_ctrl(lv_obj_t *obj, uint32_t btn_id, lv_buttonmatrix_ctrl_t ctrl);
void lv_buttonmatrix_clear_button_ctrl(lv_obj_t *obj, uint32_t btn_id, lv_buttonmatrix_ctrl_t ctrl);
void lv_buttonmatrix_set_button_ctrl_all(lv_obj_t *obj, lv_buttonmatrix_ctrl_t ctrl);
void lv_buttonmatrix_clear_button_ctrl_all(lv_obj_t *obj, lv_buttonmatrix_ctrl_t ctrl);
void lv_buttonmatrix_set_button_width(lv_obj_t *obj, uint32_t btn_id, uint32_t width);
void lv_buttonmatrix_set_one_checked(lv_obj_t *obj, bool en);
uint32_t lv_buttonmatrix_get_selected_button(lv_obj_t *obj);
const char *lv_buttonmatrix_get_button_text(lv_obj_t *obj, uint32_t btn_id);
bool lv_buttonmatrix_has_button_ctrl(lv_obj_t *obj, uint32_t btn_id, lv_buttonmatrix_ctrl_t ctrl);
```

### Spinbox (lv_spinbox)
```c
lv_obj_t *lv_spinbox_create(lv_obj_t *parent);
void lv_spinbox_set_value(lv_obj_t *obj, int32_t i);
void lv_spinbox_set_rollover(lv_obj_t *obj, bool b);
void lv_spinbox_set_digit_format(lv_obj_t *obj, uint32_t digit_count, uint32_t separator_position);
void lv_spinbox_set_step(lv_obj_t *obj, uint32_t step);
void lv_spinbox_set_range(lv_obj_t *obj, int32_t range_min, int32_t range_max);
void lv_spinbox_set_cursor_pos(lv_obj_t *obj, uint32_t pos);
void lv_spinbox_set_digit_step_direction(lv_obj_t *obj, lv_dir_t direction);
int32_t lv_spinbox_get_value(lv_obj_t *obj);
int32_t lv_spinbox_get_step(lv_obj_t *obj);
void lv_spinbox_step_next(lv_obj_t *obj);
void lv_spinbox_step_prev(lv_obj_t *obj);
void lv_spinbox_increment(lv_obj_t *obj);
void lv_spinbox_decrement(lv_obj_t *obj);
```

### Spinner (lv_spinner)
```c
lv_obj_t *lv_spinner_create(lv_obj_t *parent);
void lv_spinner_set_anim_params(lv_obj_t *obj, uint32_t t, uint32_t angle);
```

### LED (lv_led)
```c
lv_obj_t *lv_led_create(lv_obj_t *parent);
void lv_led_set_color(lv_obj_t *obj, lv_color_t color);
void lv_led_set_brightness(lv_obj_t *obj, uint8_t bright);
void lv_led_on(lv_obj_t *obj);
void lv_led_off(lv_obj_t *obj);
void lv_led_toggle(lv_obj_t *obj);
uint8_t lv_led_get_brightness(lv_obj_t *obj);
```

### Message Box (lv_msgbox)
```c
lv_obj_t *lv_msgbox_create(lv_obj_t *parent);
void lv_msgbox_add_title(lv_obj_t *obj, const char *title);
void lv_msgbox_add_header_button(lv_obj_t *obj, const void *icon);
void lv_msgbox_add_text(lv_obj_t *obj, const char *text);
void lv_msgbox_add_footer_button(lv_obj_t *obj, const char *text);
void lv_msgbox_add_close_button(lv_obj_t *obj);
lv_obj_t *lv_msgbox_get_title(lv_obj_t *obj);
lv_obj_t *lv_msgbox_get_header(lv_obj_t *obj);
lv_obj_t *lv_msgbox_get_content(lv_obj_t *obj);
lv_obj_t *lv_msgbox_get_footer(lv_obj_t *obj);
void lv_msgbox_close(lv_obj_t *obj);
void lv_msgbox_close_async(lv_obj_t *obj);
```

### Line (lv_line)
```c
lv_obj_t *lv_line_create(lv_obj_t *parent);
void lv_line_set_points(lv_obj_t *obj, const lv_point_precise_t points[], uint32_t point_num);
void lv_line_set_y_invert(lv_obj_t *obj, bool en);
```

### Calendar (lv_calendar)
```c
lv_obj_t *lv_calendar_create(lv_obj_t *parent);
void lv_calendar_set_today_date(lv_obj_t *obj, uint32_t year, uint32_t month, uint32_t day);
void lv_calendar_set_showed_date(lv_obj_t *obj, uint32_t year, uint32_t month);
void lv_calendar_set_highlighted_dates(lv_obj_t *obj, lv_calendar_date_t dates[], uint32_t date_num);
const lv_calendar_date_t *lv_calendar_get_pressed_date(lv_obj_t *obj);
lv_obj_t *lv_calendar_header_arrow_create(lv_obj_t *parent);
lv_obj_t *lv_calendar_header_dropdown_create(lv_obj_t *parent);
```

### Menu (lv_menu)
```c
lv_obj_t *lv_menu_create(lv_obj_t *parent);
lv_obj_t *lv_menu_page_create(lv_obj_t *parent, const char *title);
lv_obj_t *lv_menu_cont_create(lv_obj_t *parent);
lv_obj_t *lv_menu_section_create(lv_obj_t *parent);
lv_obj_t *lv_menu_separator_create(lv_obj_t *parent);
void lv_menu_set_page(lv_obj_t *obj, lv_obj_t *page);
void lv_menu_set_sidebar_page(lv_obj_t *obj, lv_obj_t *page);
void lv_menu_set_mode_header(lv_obj_t *obj, lv_menu_mode_header_t mode);
void lv_menu_set_mode_root_back_button(lv_obj_t *obj, lv_menu_mode_root_back_button_t mode);
void lv_menu_clear_history(lv_obj_t *obj);
void lv_menu_back(lv_obj_t *obj);
lv_obj_t *lv_menu_get_cur_main_page(lv_obj_t *obj);
lv_obj_t *lv_menu_get_cur_sidebar_page(lv_obj_t *obj);
lv_obj_t *lv_menu_get_main_header(lv_obj_t *obj);
lv_obj_t *lv_menu_get_main_header_back_button(lv_obj_t *obj);
lv_obj_t *lv_menu_get_sidebar_header(lv_obj_t *obj);
lv_obj_t *lv_menu_get_sidebar_header_back_button(lv_obj_t *obj);
```

### Tabview (lv_tabview)
```c
lv_obj_t *lv_tabview_create(lv_obj_t *parent);
lv_obj_t *lv_tabview_add_tab(lv_obj_t *tv, const char *name);
void lv_tabview_rename_tab(lv_obj_t *tv, uint32_t idx, const char *new_name);
void lv_tabview_set_active(lv_obj_t *tv, uint32_t idx, lv_anim_enable_t anim_en);
uint32_t lv_tabview_get_tab_active(lv_obj_t *tv);
lv_obj_t *lv_tabview_get_content(lv_obj_t *tv);
lv_obj_t *lv_tabview_get_tab_bar(lv_obj_t *tv);
void lv_tabview_set_tab_bar_position(lv_obj_t *obj, lv_dir_t dir);
void lv_tabview_set_tab_bar_size(lv_obj_t *obj, int32_t size);
```

### Tileview (lv_tileview)
```c
lv_obj_t *lv_tileview_create(lv_obj_t *parent);
lv_obj_t *lv_tileview_add_tile(lv_obj_t *tv, uint32_t col_id, uint32_t row_id, lv_dir_t dir);
void lv_tileview_set_tile(lv_obj_t *tv, lv_obj_t *tile_obj, lv_anim_enable_t anim_en);
void lv_tileview_set_tile_by_index(lv_obj_t *tv, uint32_t col_id, uint32_t row_id, lv_anim_enable_t anim_en);
lv_obj_t *lv_tileview_get_tile_active(lv_obj_t *obj);
```

### Window (lv_win)
```c
lv_obj_t *lv_win_create(lv_obj_t *parent);
lv_obj_t *lv_win_add_title(lv_obj_t *win, const char *txt);
lv_obj_t *lv_win_add_button(lv_obj_t *win, const void *icon, int32_t btn_w);
lv_obj_t *lv_win_get_header(lv_obj_t *win);
lv_obj_t *lv_win_get_content(lv_obj_t *win);
```

---

## 10. Layout API

### Flex Layout
```c
void lv_obj_set_flex_flow(lv_obj_t *obj, lv_flex_flow_t flow);
void lv_obj_set_flex_align(lv_obj_t *obj, lv_flex_align_t main_place,
                           lv_flex_align_t cross_place, lv_flex_align_t track_cross_place);
void lv_obj_set_flex_grow(lv_obj_t *obj, uint8_t grow);

// Flow types
LV_FLEX_FLOW_ROW
LV_FLEX_FLOW_COLUMN
LV_FLEX_FLOW_ROW_WRAP
LV_FLEX_FLOW_COLUMN_WRAP
LV_FLEX_FLOW_ROW_REVERSE
LV_FLEX_FLOW_COLUMN_REVERSE
LV_FLEX_FLOW_ROW_WRAP_REVERSE
LV_FLEX_FLOW_COLUMN_WRAP_REVERSE

// Alignment
LV_FLEX_ALIGN_START
LV_FLEX_ALIGN_END
LV_FLEX_ALIGN_CENTER
LV_FLEX_ALIGN_SPACE_EVENLY
LV_FLEX_ALIGN_SPACE_AROUND
LV_FLEX_ALIGN_SPACE_BETWEEN
```

### Grid Layout
```c
void lv_obj_set_grid_dsc_array(lv_obj_t *obj, const int32_t col_dsc[], const int32_t row_dsc[]);
void lv_obj_set_grid_align(lv_obj_t *obj, lv_grid_align_t column_align, lv_grid_align_t row_align);
void lv_obj_set_grid_cell(lv_obj_t *obj, lv_grid_align_t x_align, int32_t col_pos, int32_t col_span,
                          lv_grid_align_t y_align, int32_t row_pos, int32_t row_span);

// Grid helpers
#define LV_GRID_FR(x)             // Fractional unit
#define LV_GRID_CONTENT           // Size to content
#define LV_GRID_TEMPLATE_LAST     // End marker for dsc arrays

// Grid alignment
LV_GRID_ALIGN_START
LV_GRID_ALIGN_CENTER
LV_GRID_ALIGN_END
LV_GRID_ALIGN_STRETCH
LV_GRID_ALIGN_SPACE_EVENLY
LV_GRID_ALIGN_SPACE_AROUND
LV_GRID_ALIGN_SPACE_BETWEEN
```

---

## 11. Draw API

### Draw Functions
```c
void lv_draw_rect(lv_layer_t *layer, const lv_draw_rect_dsc_t *dsc, const lv_area_t *coords);
void lv_draw_label(lv_layer_t *layer, const lv_draw_label_dsc_t *dsc, const lv_area_t *coords);
void lv_draw_image(lv_layer_t *layer, const lv_draw_image_dsc_t *dsc, const lv_area_t *coords);
void lv_draw_line(lv_layer_t *layer, const lv_draw_line_dsc_t *dsc);
void lv_draw_arc(lv_layer_t *layer, const lv_draw_arc_dsc_t *dsc);
void lv_draw_triangle(lv_layer_t *layer, const lv_draw_triangle_dsc_t *dsc);

// Descriptor initialization
void lv_draw_rect_dsc_init(lv_draw_rect_dsc_t *dsc);
void lv_draw_label_dsc_init(lv_draw_label_dsc_t *dsc);
void lv_draw_image_dsc_init(lv_draw_image_dsc_t *dsc);
void lv_draw_line_dsc_init(lv_draw_line_dsc_t *dsc);
void lv_draw_arc_dsc_init(lv_draw_arc_dsc_t *dsc);
void lv_draw_triangle_dsc_init(lv_draw_triangle_dsc_t *dsc);
```

### Draw Buffer
```c
lv_draw_buf_t *lv_draw_buf_create(int32_t w, int32_t h, lv_color_format_t cf, uint32_t stride);
void lv_draw_buf_destroy(lv_draw_buf_t *buf);
void lv_draw_buf_invalidate_cache(lv_draw_buf_t *buf, const lv_area_t *area);
void *lv_draw_buf_goto_xy(lv_draw_buf_t *buf, int32_t x, int32_t y);
uint32_t lv_draw_buf_width_to_stride(uint32_t w, lv_color_format_t cf);

// Static buffer macro
LV_DRAW_BUF_DEFINE(name, w, h, cf);
```

### RGB565 Swap Helper
```c
void lv_draw_sw_rgb565_swap(void *buf, uint32_t buf_size_px);
// Call in flush_cb for SPI displays that need byte-swapped RGB565
```

---

## 12. Observer API

See Section 6 of README.md for complete observer documentation.

```c
// Subject lifecycle
void lv_subject_init_int(lv_subject_t *subject, int32_t value);
void lv_subject_init_float(lv_subject_t *subject, float value);
void lv_subject_init_string(lv_subject_t *subject, char *buf, char *prev_buf,
                            size_t size, const char *value);
void lv_subject_init_pointer(lv_subject_t *subject, void *value);
void lv_subject_init_color(lv_subject_t *subject, lv_color_t value);
void lv_subject_init_group(lv_subject_t *subject, lv_subject_t *list[], uint32_t cnt);
void lv_subject_deinit(lv_subject_t *subject);

// Getters
int32_t lv_subject_get_int(lv_subject_t *subject);
float lv_subject_get_float(lv_subject_t *subject);
const char *lv_subject_get_string(lv_subject_t *subject);
void *lv_subject_get_pointer(lv_subject_t *subject);
lv_color_t lv_subject_get_color(lv_subject_t *subject);
lv_subject_t *lv_subject_get_group_element(lv_subject_t *subject, int32_t index);

// Previous value getters
int32_t lv_subject_get_previous_int(lv_subject_t *subject);
float lv_subject_get_previous_float(lv_subject_t *subject);
const char *lv_subject_get_previous_string(lv_subject_t *subject);
void *lv_subject_get_previous_pointer(lv_subject_t *subject);
lv_color_t lv_subject_get_previous_color(lv_subject_t *subject);

// Setters
void lv_subject_set_int(lv_subject_t *subject, int32_t value);
void lv_subject_set_float(lv_subject_t *subject, float value);
void lv_subject_copy_string(lv_subject_t *subject, const char *buf);
void lv_subject_set_pointer(lv_subject_t *subject, void *ptr);
void lv_subject_set_color(lv_subject_t *subject, lv_color_t color);

// Observer subscription
lv_observer_t *lv_subject_add_observer(lv_subject_t *subject,
    lv_observer_cb_t cb, void *user_data);
lv_observer_t *lv_subject_add_observer_obj(lv_subject_t *subject,
    lv_observer_cb_t cb, lv_obj_t *obj, void *user_data);
lv_observer_t *lv_subject_add_observer_with_target(lv_subject_t *subject,
    lv_observer_cb_t cb, void *target, void *user_data);
void lv_observer_remove(lv_observer_t *observer);

// Widget bindings
void lv_obj_bind_flag_if_eq(lv_obj_t *obj, lv_subject_t *subject, lv_obj_flag_t flag, int32_t ref);
void lv_obj_bind_flag_if_not_eq(lv_obj_t *obj, lv_subject_t *subject, lv_obj_flag_t flag, int32_t ref);
void lv_obj_bind_state_if_eq(lv_obj_t *obj, lv_subject_t *subject, lv_state_t state, int32_t ref);
void lv_obj_bind_state_if_not_eq(lv_obj_t *obj, lv_subject_t *subject, lv_state_t state, int32_t ref);
void lv_obj_bind_checked(lv_obj_t *obj, lv_subject_t *subject);

// Event-triggered subject modifications
void lv_obj_add_subject_toggle_event(lv_obj_t *obj, lv_subject_t *subject);
void lv_obj_add_subject_set_int_event(lv_obj_t *obj, lv_subject_t *subject,
    lv_event_code_t event, int32_t value);
void lv_obj_add_subject_set_string_event(lv_obj_t *obj, lv_subject_t *subject,
    lv_event_code_t event, const char *value);
lv_subject_increment_dsc_t *lv_obj_add_subject_increment_event(lv_obj_t *obj,
    lv_subject_t *subject, lv_event_code_t event, int32_t step);

// Observer callback signature
typedef void (*lv_observer_cb_t)(lv_observer_t *observer, lv_subject_t *subject);
```

---

## 13. Color API

```c
// Construction
lv_color_t lv_color_hex(uint32_t c);        // e.g., lv_color_hex(0xFF0000) for red
lv_color_t lv_color_hex3(uint16_t c);       // e.g., lv_color_hex3(0xF00) for red
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b);

// Predefined colors
lv_color_t lv_color_white(void);
lv_color_t lv_color_black(void);

// Color manipulation
lv_color_t lv_color_lighten(lv_color_t c, lv_opa_t lvl);
lv_color_t lv_color_darken(lv_color_t c, lv_opa_t lvl);
lv_color_t lv_color_mix(lv_color_t c1, lv_color_t c2, uint8_t mix);

// HSV
lv_color_t lv_color_hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v);
lv_color_hsv_t lv_color_rgb_to_hsv(uint8_t r, uint8_t g, uint8_t b);
lv_color_hsv_t lv_color_to_hsv(lv_color_t color);

// Brightness
uint8_t lv_color_brightness(lv_color_t color);

// lv_color_t is always RGB888 in v9
typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} lv_color_t;

// Opacity constants
#define LV_OPA_TRANSP  0
#define LV_OPA_0       0
#define LV_OPA_10      25
#define LV_OPA_20      51
#define LV_OPA_30      76
#define LV_OPA_40      102
#define LV_OPA_50      127
#define LV_OPA_60      153
#define LV_OPA_70      178
#define LV_OPA_80      204
#define LV_OPA_90      229
#define LV_OPA_100     255
#define LV_OPA_COVER   255
```

---

## 14. Font API

```c
// Built-in fonts (enable in lv_conf.h)
extern const lv_font_t lv_font_montserrat_8;
extern const lv_font_t lv_font_montserrat_10;
extern const lv_font_t lv_font_montserrat_12;
extern const lv_font_t lv_font_montserrat_14;  // Default
extern const lv_font_t lv_font_montserrat_16;
extern const lv_font_t lv_font_montserrat_18;
extern const lv_font_t lv_font_montserrat_20;
extern const lv_font_t lv_font_montserrat_22;
extern const lv_font_t lv_font_montserrat_24;
extern const lv_font_t lv_font_montserrat_26;
extern const lv_font_t lv_font_montserrat_28;
extern const lv_font_t lv_font_montserrat_30;
extern const lv_font_t lv_font_montserrat_32;
extern const lv_font_t lv_font_montserrat_34;
extern const lv_font_t lv_font_montserrat_36;
extern const lv_font_t lv_font_montserrat_38;
extern const lv_font_t lv_font_montserrat_40;
extern const lv_font_t lv_font_montserrat_42;
extern const lv_font_t lv_font_montserrat_44;
extern const lv_font_t lv_font_montserrat_46;
extern const lv_font_t lv_font_montserrat_48;

// Symbol font
extern const lv_font_t lv_font_montserrat_14;  // Includes basic symbols

// Font utility
uint32_t lv_font_get_line_height(const lv_font_t *font);
```

---

## 15. Image API

```c
// Image declaration (for C arrays)
LV_IMAGE_DECLARE(image_name);  // was LV_IMG_DECLARE

// Image descriptor
typedef struct {
    lv_image_header_t header;
    uint32_t data_size;
    const uint8_t *data;
} lv_image_dsc_t;

// Image header
typedef struct {
    uint32_t cf : 8;           // Color format (lv_color_format_t)
    uint32_t w : 16;           // Width
    uint32_t h : 16;           // Height
    uint32_t stride : 16;      // Row stride in bytes
    uint32_t reserved_2 : 16;
} lv_image_header_t;

// Image decoder registration
lv_image_decoder_t *lv_image_decoder_create(void);
void lv_image_decoder_delete(lv_image_decoder_t *decoder);
void lv_image_decoder_set_info_cb(lv_image_decoder_t *decoder, lv_image_decoder_info_f_t info_cb);
void lv_image_decoder_set_open_cb(lv_image_decoder_t *decoder, lv_image_decoder_open_f_t open_cb);
void lv_image_decoder_set_close_cb(lv_image_decoder_t *decoder, lv_image_decoder_close_f_t close_cb);
```

---

## 16. File System API

```c
// File operations
lv_fs_res_t lv_fs_open(lv_fs_file_t *file, const char *path, lv_fs_mode_t mode);
lv_fs_res_t lv_fs_close(lv_fs_file_t *file);
lv_fs_res_t lv_fs_read(lv_fs_file_t *file, void *buf, uint32_t btr, uint32_t *br);
lv_fs_res_t lv_fs_write(lv_fs_file_t *file, const void *buf, uint32_t btw, uint32_t *bw);
lv_fs_res_t lv_fs_seek(lv_fs_file_t *file, uint32_t pos, lv_fs_whence_t whence);
lv_fs_res_t lv_fs_tell(lv_fs_file_t *file, uint32_t *pos);

// Directory operations
lv_fs_res_t lv_fs_dir_open(lv_fs_dir_t *dir, const char *path);
lv_fs_res_t lv_fs_dir_read(lv_fs_dir_t *dir, char *fn, uint32_t fn_len);
lv_fs_res_t lv_fs_dir_close(lv_fs_dir_t *dir);

// Modes
#define LV_FS_MODE_WR   0x01
#define LV_FS_MODE_RD   0x02
```

---

## 17. Memory API

```c
void *lv_malloc(size_t size);
void *lv_malloc_zeroed(size_t size);
void *lv_realloc(void *data, size_t new_size);
void lv_free(void *data);
void lv_memset(void *dst, uint8_t v, size_t len);
void lv_memcpy(void *dst, const void *src, size_t len);
size_t lv_strlen(const char *str);
char *lv_strdup(const char *src);
char *lv_strncpy(char *dst, const char *src, size_t dest_size);

// Memory monitoring
void lv_mem_monitor(lv_mem_monitor_t *mon);
lv_result_t lv_mem_test(void);

// Monitor struct
typedef struct {
    uint32_t total_size;
    uint32_t free_cnt;
    uint32_t free_size;
    uint32_t free_biggest_size;
    uint32_t used_cnt;
    uint32_t max_used;
    uint8_t used_pct;
    uint8_t frag_pct;
} lv_mem_monitor_t;

// Pool management
void lv_mem_add_pool(void *mem, size_t bytes);
```

---

## 18. Logging API

```c
// Enable: #define LV_USE_LOG 1
// Level: #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

void lv_log_register_print_cb(void (*print_cb)(lv_log_level_t level, const char *buf));

// Log macros
LV_LOG_TRACE(msg, ...)
LV_LOG_INFO(msg, ...)
LV_LOG_WARN(msg, ...)
LV_LOG_ERROR(msg, ...)
LV_LOG_USER(msg, ...)

// Levels
LV_LOG_LEVEL_TRACE
LV_LOG_LEVEL_INFO
LV_LOG_LEVEL_WARN
LV_LOG_LEVEL_ERROR
LV_LOG_LEVEL_USER
LV_LOG_LEVEL_NONE
```

---

## 19. Group API

```c
lv_group_t *lv_group_create(void);
void lv_group_delete(lv_group_t *group);
void lv_group_set_default(lv_group_t *group);
lv_group_t *lv_group_get_default(void);

void lv_group_add_obj(lv_group_t *group, lv_obj_t *obj);
void lv_group_remove_obj(lv_obj_t *obj);
void lv_group_remove_all_objs(lv_group_t *group);

void lv_group_focus_obj(lv_obj_t *obj);
void lv_group_focus_next(lv_group_t *group);
void lv_group_focus_prev(lv_group_t *group);
void lv_group_focus_freeze(lv_group_t *group, bool en);

void lv_group_set_focus_cb(lv_group_t *group, lv_group_focus_cb_t focus_cb);
void lv_group_set_editing(lv_group_t *group, bool edit);
void lv_group_set_wrap(lv_group_t *group, bool en);

lv_obj_t *lv_group_get_focused(lv_group_t *group);
bool lv_group_get_editing(lv_group_t *group);
bool lv_group_get_wrap(lv_group_t *group);
uint32_t lv_group_get_obj_count(lv_group_t *group);

lv_result_t lv_group_send_data(lv_group_t *group, uint32_t c);
```

---

## 20. Theme API

```c
// Apply theme
lv_theme_t *lv_theme_default_init(lv_display_t *disp, lv_color_t primary,
    lv_color_t secondary, bool dark, const lv_font_t *font);
void lv_display_set_theme(lv_display_t *disp, lv_theme_t *th);
lv_theme_t *lv_display_get_theme(lv_display_t *disp);

// Simple theme
lv_theme_t *lv_theme_simple_init(lv_display_t *disp);

// Mono theme
lv_theme_t *lv_theme_mono_init(lv_display_t *disp, bool dark_bg, const lv_font_t *font);

// Custom theme
lv_theme_t *lv_theme_get_from_obj(lv_obj_t *obj);
void lv_theme_apply(lv_obj_t *obj);
```

---

## Common Geometric Types

```c
typedef struct {
    int32_t x1, y1, x2, y2;
} lv_area_t;

typedef struct {
    int32_t x, y;
} lv_point_t;

typedef struct {
    float x, y;      // Note: uses float for sub-pixel precision
} lv_point_precise_t;

// Area utilities
void lv_area_set(lv_area_t *area, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
int32_t lv_area_get_width(const lv_area_t *area);
int32_t lv_area_get_height(const lv_area_t *area);
bool lv_area_is_point_on(const lv_area_t *a, const lv_point_t *p, int32_t radius);
bool lv_area_intersect(lv_area_t *res, const lv_area_t *a1, const lv_area_t *a2);
```

---

## Percentage and Special Values

```c
#define LV_SIZE_CONTENT   // Auto-size to content
#define LV_PCT(x)         // Percentage of parent, e.g., LV_PCT(50)
#define LV_DPX(x)         // DPI-aware pixel value
#define LV_RADIUS_CIRCLE  0x7FFF  // Maximum radius (circle)
```

---

## Key Symbols (Built-in)

```c
LV_SYMBOL_AUDIO
LV_SYMBOL_VIDEO
LV_SYMBOL_LIST
LV_SYMBOL_OK
LV_SYMBOL_CLOSE
LV_SYMBOL_POWER
LV_SYMBOL_SETTINGS
LV_SYMBOL_HOME
LV_SYMBOL_DOWNLOAD
LV_SYMBOL_DRIVE
LV_SYMBOL_REFRESH
LV_SYMBOL_MUTE
LV_SYMBOL_VOLUME_MID
LV_SYMBOL_VOLUME_MAX
LV_SYMBOL_IMAGE
LV_SYMBOL_EDIT
LV_SYMBOL_PREV
LV_SYMBOL_PLAY
LV_SYMBOL_PAUSE
LV_SYMBOL_STOP
LV_SYMBOL_NEXT
LV_SYMBOL_EJECT
LV_SYMBOL_LEFT
LV_SYMBOL_RIGHT
LV_SYMBOL_PLUS
LV_SYMBOL_MINUS
LV_SYMBOL_EYE_OPEN
LV_SYMBOL_EYE_CLOSE
LV_SYMBOL_WARNING
LV_SYMBOL_SHUFFLE
LV_SYMBOL_UP
LV_SYMBOL_DOWN
LV_SYMBOL_LOOP
LV_SYMBOL_DIRECTORY
LV_SYMBOL_UPLOAD
LV_SYMBOL_CALL
LV_SYMBOL_CUT
LV_SYMBOL_COPY
LV_SYMBOL_SAVE
LV_SYMBOL_BARS
LV_SYMBOL_ENVELOPE
LV_SYMBOL_CHARGE
LV_SYMBOL_PASTE
LV_SYMBOL_BELL
LV_SYMBOL_KEYBOARD
LV_SYMBOL_GPS
LV_SYMBOL_FILE
LV_SYMBOL_WIFI
LV_SYMBOL_BATTERY_FULL
LV_SYMBOL_BATTERY_3
LV_SYMBOL_BATTERY_2
LV_SYMBOL_BATTERY_1
LV_SYMBOL_BATTERY_EMPTY
LV_SYMBOL_USB
LV_SYMBOL_BLUETOOTH
LV_SYMBOL_TRASH
LV_SYMBOL_BACKSPACE
LV_SYMBOL_SD_CARD
LV_SYMBOL_NEW_LINE
```
