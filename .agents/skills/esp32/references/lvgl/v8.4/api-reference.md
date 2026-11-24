# LVGL v8.4 API Reference

> Definitive API surface for LVGL v8.4.0 -- the final v8 release.
> All function signatures use the v8 naming conventions (`lv_btn`, `lv_img`, `lv_disp_drv_t`, etc.).

---

## Table of Contents

1. [Core Initialization](#core-initialization)
2. [Display Driver API](#display-driver-api)
3. [Input Device Driver API](#input-device-driver-api)
4. [Base Object API (lv_obj)](#base-object-api)
5. [Object Flags](#object-flags)
6. [Object States](#object-states)
7. [Widget Parts](#widget-parts)
8. [Events](#events)
9. [Style API](#style-api)
10. [Style Properties](#style-properties)
11. [Animation API](#animation-api)
12. [Timer API](#timer-api)
13. [Font API](#font-api)
14. [Image Decoder API](#image-decoder-api)
15. [File System API](#file-system-api)
16. [Layout API (Flex)](#layout-api-flex)
17. [Layout API (Grid)](#layout-api-grid)
18. [Color API](#color-api)
19. [Drawing API](#drawing-api)
20. [Group API](#group-api)
21. [Screen API](#screen-api)
22. [Core Widgets](#core-widgets)
23. [Extra Widgets](#extra-widgets)
24. [Theme API](#theme-api)
25. [GPU Backends](#gpu-backends)
26. [Memory API](#memory-api)
27. [Logging API](#logging-api)

---

## Core Initialization

```c
void lv_init(void);                              // Initialize LVGL (call once at startup)
void lv_deinit(void);                            // De-initialize LVGL (if LV_ENABLE_GC enabled)
void lv_tick_inc(uint32_t tick_period_ms);       // Call every 1-10ms from timer/ISR
uint32_t lv_tick_get(void);                      // Get elapsed ms since boot
uint32_t lv_tick_elaps(uint32_t prev_tick);      // Get elapsed ms since prev_tick
lv_res_t lv_timer_handler(void);                 // Process LVGL tasks (call in main loop)
```

### Minimal Init Sequence

```c
lv_init();

// Set up display driver (see Display Driver API)
// Set up input device (see Input Device Driver API)

// Main loop
while(1) {
    lv_tick_inc(5);          // Or use hardware timer
    lv_timer_handler();      // Process pending work
    my_delay_ms(5);
}
```

---

## Display Driver API

### Structures

| Structure | Description |
|-----------|-------------|
| `lv_disp_drv_t` | Display driver configuration descriptor |
| `lv_disp_draw_buf_t` | Draw buffer descriptor (single or double) |
| `lv_disp_t` | Registered display handle |

### Draw Buffer Functions

```c
void lv_disp_draw_buf_init(
    lv_disp_draw_buf_t * draw_buf,  // Buffer descriptor to init
    void * buf1,                     // Primary buffer
    void * buf2,                     // Secondary buffer (NULL for single)
    uint32_t size_in_px_cnt          // Buffer size in PIXELS
);
```

### Display Driver Functions

```c
void lv_disp_drv_init(lv_disp_drv_t * driver);
lv_disp_t * lv_disp_drv_register(lv_disp_drv_t * driver);
void lv_disp_drv_update(lv_disp_t * disp, lv_disp_drv_t * new_drv);
void lv_disp_remove(lv_disp_t * disp);
```

### Display Driver Fields (`lv_disp_drv_t`)

| Field | Type | Description |
|-------|------|-------------|
| `hor_res` | `lv_coord_t` | Horizontal resolution (pixels) |
| `ver_res` | `lv_coord_t` | Vertical resolution (pixels) |
| `draw_buf` | `lv_disp_draw_buf_t *` | Pointer to initialized draw buffer |
| `flush_cb` | `void (*)(lv_disp_drv_t *, const lv_area_t *, lv_color_t *)` | Flush callback |
| `rounder_cb` | `void (*)(lv_disp_drv_t *, lv_area_t *)` | Round coordinates callback |
| `set_px_cb` | `void (*)(lv_disp_drv_t *, uint8_t *, lv_coord_t, ...)` | Set pixel callback |
| `monitor_cb` | `void (*)(lv_disp_drv_t *, uint32_t, uint32_t)` | Render monitor callback |
| `clean_dcache_cb` | `void (*)(lv_disp_drv_t *)` | Clean data cache callback |
| `wait_cb` | `void (*)(lv_disp_drv_t *)` | Wait while flushing callback |
| `color_chroma_key` | `lv_color_t` | Chroma key color for transparency |
| `antialiasing` | `uint32_t : 1` | Enable anti-aliasing |
| `rotated` | `uint32_t : 2` | Rotation (0, 1=90, 2=180, 3=270) |
| `screen_transp` | `uint32_t : 1` | Transparent screen support |
| `dpi` | `uint32_t` | DPI for size calculations |
| `full_refresh` | `uint32_t : 1` | Always redraw full screen |
| `direct_mode` | `uint32_t : 1` | Direct buffer mode |
| `sw_rotate` | `uint32_t : 1` | Software rotation |
| `user_data` | `void *` | User data pointer |

### Display Query Functions

```c
lv_disp_t * lv_disp_get_default(void);
void lv_disp_set_default(lv_disp_t * disp);
lv_coord_t lv_disp_get_hor_res(lv_disp_t * disp);
lv_coord_t lv_disp_get_ver_res(lv_disp_t * disp);
bool lv_disp_get_antialiasing(lv_disp_t * disp);
lv_coord_t lv_disp_get_dpi(const lv_disp_t * disp);
void lv_disp_set_rotation(lv_disp_t * disp, lv_disp_rot_t rotation);
lv_disp_rot_t lv_disp_get_rotation(lv_disp_t * disp);
```

### Flush Callback Pattern

```c
void my_flush_cb(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p) {
    int32_t x, y;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            set_pixel(x, y, *color_p);
            color_p++;
        }
    }
    lv_disp_flush_ready(drv);   // MUST call when flush complete
}
```

### Display Background

```c
void lv_disp_set_bg_color(lv_disp_t * disp, lv_color_t color);
void lv_disp_set_bg_opa(lv_disp_t * disp, lv_opa_t opa);
void lv_disp_set_bg_image(lv_disp_t * disp, const void * img_src);
```

---

## Input Device Driver API

### Structures

| Structure | Description |
|-----------|-------------|
| `lv_indev_drv_t` | Input device driver descriptor |
| `lv_indev_t` | Registered input device handle |
| `lv_indev_data_t` | Data read from input device |

### Functions

```c
void lv_indev_drv_init(lv_indev_drv_t * driver);
lv_indev_t * lv_indev_drv_register(lv_indev_drv_t * driver);
void lv_indev_drv_update(lv_indev_t * indev, lv_indev_drv_t * new_drv);
void lv_indev_delete(lv_indev_t * indev);
lv_indev_t * lv_indev_get_next(lv_indev_t * indev);  // NULL to get first

// Query
lv_indev_type_t lv_indev_get_type(const lv_indev_t * indev);
void lv_indev_get_point(const lv_indev_t * indev, lv_point_t * point);
uint32_t lv_indev_get_key(const lv_indev_t * indev);
lv_dir_t lv_indev_get_gesture_dir(const lv_indev_t * indev);
lv_indev_state_t lv_indev_get_state(const lv_indev_t * indev);
lv_obj_t * lv_indev_get_obj_act(void);

// Feedback
void lv_indev_set_cursor(lv_indev_t * indev, lv_obj_t * cur_obj);
void lv_indev_set_group(lv_indev_t * indev, lv_group_t * group);
void lv_indev_reset(lv_indev_t * indev, lv_obj_t * obj);
void lv_indev_reset_long_press(lv_indev_t * indev);

// Enable/disable
void lv_indev_enable(lv_indev_t * indev, bool en);
```

### Input Device Types

| Constant | Value | Read Callback Data |
|----------|-------|-------------------|
| `LV_INDEV_TYPE_POINTER` | Touchpad/mouse | `data->point.x`, `data->point.y`, `data->state` |
| `LV_INDEV_TYPE_KEYPAD` | Keyboard | `data->key`, `data->state` |
| `LV_INDEV_TYPE_ENCODER` | Rotary encoder | `data->enc_diff`, `data->state` |
| `LV_INDEV_TYPE_BUTTON` | External buttons | `data->btn_id`, `data->state` |

### Read Callback Pattern

```c
void my_touchpad_read(lv_indev_drv_t * drv, lv_indev_data_t * data) {
    if(touchpad_is_pressed()) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchpad_get_x();
        data->point.y = touchpad_get_y();
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
    data->continue_reading = false;  // true if more data available
}
```

---

## Base Object API

### Create / Delete

```c
lv_obj_t * lv_obj_create(lv_obj_t * parent);
void lv_obj_del(lv_obj_t * obj);
void lv_obj_clean(lv_obj_t * obj);              // Delete all children
void lv_obj_del_delayed(lv_obj_t * obj, uint32_t delay_ms);
void lv_obj_del_anim_ready_cb(lv_anim_t * a);   // Delete when anim finishes
```

### Size

```c
void lv_obj_set_width(lv_obj_t * obj, lv_coord_t w);
void lv_obj_set_height(lv_obj_t * obj, lv_coord_t h);
void lv_obj_set_size(lv_obj_t * obj, lv_coord_t w, lv_coord_t h);
void lv_obj_set_content_width(lv_obj_t * obj, lv_coord_t w);
void lv_obj_set_content_height(lv_obj_t * obj, lv_coord_t h);
void lv_obj_refr_size(lv_obj_t * obj);

// Special size constants
#define LV_SIZE_CONTENT    // Auto-size to content
#define LV_PCT(x)         // Percentage of parent
```

### Position

```c
void lv_obj_set_x(lv_obj_t * obj, lv_coord_t x);
void lv_obj_set_y(lv_obj_t * obj, lv_coord_t y);
void lv_obj_set_pos(lv_obj_t * obj, lv_coord_t x, lv_coord_t y);
void lv_obj_set_align(lv_obj_t * obj, lv_align_t align);
void lv_obj_align(lv_obj_t * obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
void lv_obj_align_to(lv_obj_t * obj, const lv_obj_t * base, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
void lv_obj_center(lv_obj_t * obj);  // Shortcut for align CENTER
```

### Alignment Constants

| Constant | Position |
|----------|----------|
| `LV_ALIGN_DEFAULT` | Follow flow/layout |
| `LV_ALIGN_TOP_LEFT` | Top-left corner |
| `LV_ALIGN_TOP_MID` | Top center |
| `LV_ALIGN_TOP_RIGHT` | Top-right corner |
| `LV_ALIGN_BOTTOM_LEFT` | Bottom-left corner |
| `LV_ALIGN_BOTTOM_MID` | Bottom center |
| `LV_ALIGN_BOTTOM_RIGHT` | Bottom-right corner |
| `LV_ALIGN_LEFT_MID` | Left center |
| `LV_ALIGN_RIGHT_MID` | Right center |
| `LV_ALIGN_CENTER` | Center |
| `LV_ALIGN_OUT_TOP_LEFT` | Above, left-aligned |
| `LV_ALIGN_OUT_TOP_MID` | Above, centered |
| `LV_ALIGN_OUT_TOP_RIGHT` | Above, right-aligned |
| `LV_ALIGN_OUT_BOTTOM_LEFT` | Below, left-aligned |
| `LV_ALIGN_OUT_BOTTOM_MID` | Below, centered |
| `LV_ALIGN_OUT_BOTTOM_RIGHT` | Below, right-aligned |
| `LV_ALIGN_OUT_LEFT_TOP` | Left, top-aligned |
| `LV_ALIGN_OUT_LEFT_MID` | Left, centered |
| `LV_ALIGN_OUT_LEFT_BOTTOM` | Left, bottom-aligned |
| `LV_ALIGN_OUT_RIGHT_TOP` | Right, top-aligned |
| `LV_ALIGN_OUT_RIGHT_MID` | Right, centered |
| `LV_ALIGN_OUT_RIGHT_BOTTOM` | Right, bottom-aligned |

### Coordinate Getters

```c
lv_coord_t lv_obj_get_x(const lv_obj_t * obj);
lv_coord_t lv_obj_get_x2(const lv_obj_t * obj);         // Right edge
lv_coord_t lv_obj_get_y(const lv_obj_t * obj);
lv_coord_t lv_obj_get_y2(const lv_obj_t * obj);         // Bottom edge
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

### Parent / Child

```c
void lv_obj_set_parent(lv_obj_t * obj, lv_obj_t * parent);
lv_obj_t * lv_obj_get_parent(const lv_obj_t * obj);
lv_obj_t * lv_obj_get_child(const lv_obj_t * obj, int32_t id);  // -1 = last
uint32_t lv_obj_get_child_cnt(const lv_obj_t * obj);
uint32_t lv_obj_get_index(const lv_obj_t * obj);

void lv_obj_move_foreground(lv_obj_t * obj);
void lv_obj_move_background(lv_obj_t * obj);
void lv_obj_move_to_index(lv_obj_t * obj, int32_t index);
void lv_obj_swap(lv_obj_t * obj1, lv_obj_t * obj2);
```

### Flags

```c
void lv_obj_add_flag(lv_obj_t * obj, lv_obj_flag_t f);
void lv_obj_clear_flag(lv_obj_t * obj, lv_obj_flag_t f);
bool lv_obj_has_flag(const lv_obj_t * obj, lv_obj_flag_t f);
bool lv_obj_has_flag_any(const lv_obj_t * obj, lv_obj_flag_t f);
```

### States

```c
void lv_obj_add_state(lv_obj_t * obj, lv_state_t state);
void lv_obj_clear_state(lv_obj_t * obj, lv_state_t state);
lv_state_t lv_obj_get_state(const lv_obj_t * obj);
bool lv_obj_has_state(const lv_obj_t * obj, lv_state_t state);
```

### Scrolling

```c
void lv_obj_set_scrollbar_mode(lv_obj_t * obj, lv_scrollbar_mode_t mode);
void lv_obj_set_scroll_dir(lv_obj_t * obj, lv_dir_t dir);
void lv_obj_set_scroll_snap_x(lv_obj_t * obj, lv_scroll_snap_t align);
void lv_obj_set_scroll_snap_y(lv_obj_t * obj, lv_scroll_snap_t align);
lv_scrollbar_mode_t lv_obj_get_scrollbar_mode(const lv_obj_t * obj);
lv_dir_t lv_obj_get_scroll_dir(const lv_obj_t * obj);

void lv_obj_scroll_by(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_anim_enable_t anim_en);
void lv_obj_scroll_to(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_anim_enable_t anim_en);
void lv_obj_scroll_to_x(lv_obj_t * obj, lv_coord_t x, lv_anim_enable_t anim_en);
void lv_obj_scroll_to_y(lv_obj_t * obj, lv_coord_t y, lv_anim_enable_t anim_en);
void lv_obj_scroll_to_view(lv_obj_t * obj, lv_anim_enable_t anim_en);
void lv_obj_scroll_to_view_recursive(lv_obj_t * obj, lv_anim_enable_t anim_en);

lv_coord_t lv_obj_get_scroll_x(const lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_y(const lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_top(lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_bottom(lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_left(lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_right(lv_obj_t * obj);
bool lv_obj_is_scrolling(const lv_obj_t * obj);
```

### Scrollbar Modes

| Constant | Behavior |
|----------|----------|
| `LV_SCROLLBAR_MODE_OFF` | Never show scrollbar |
| `LV_SCROLLBAR_MODE_ON` | Always show scrollbar |
| `LV_SCROLLBAR_MODE_ACTIVE` | Show while scrolling |
| `LV_SCROLLBAR_MODE_AUTO` | Show when content is large enough |

### Miscellaneous Object Functions

```c
void lv_obj_set_ext_click_area(lv_obj_t * obj, lv_coord_t size);
lv_obj_t * lv_obj_get_screen(const lv_obj_t * obj);
lv_disp_t * lv_obj_get_disp(const lv_obj_t * obj);
bool lv_obj_is_valid(const lv_obj_t * obj);

void lv_obj_update_layout(const lv_obj_t * obj);  // Force layout recalculation
void lv_obj_invalidate(const lv_obj_t * obj);      // Mark for redraw
lv_area_t lv_obj_get_click_area(const lv_obj_t * obj);
bool lv_obj_hit_test(lv_obj_t * obj, lv_point_t * point);
```

---

## Object Flags

| Flag | Description |
|------|-------------|
| `LV_OBJ_FLAG_HIDDEN` | Object is not visible |
| `LV_OBJ_FLAG_CLICKABLE` | Object can be clicked |
| `LV_OBJ_FLAG_CLICK_FOCUSABLE` | Add focused state when clicked |
| `LV_OBJ_FLAG_CHECKABLE` | Toggle checked state on click |
| `LV_OBJ_FLAG_SCROLLABLE` | Object is scrollable |
| `LV_OBJ_FLAG_SCROLL_ELASTIC` | Allow elastic scroll effect |
| `LV_OBJ_FLAG_SCROLL_MOMENTUM` | Momentum on scroll release |
| `LV_OBJ_FLAG_SCROLL_ONE` | Allow scroll only one snappable child |
| `LV_OBJ_FLAG_SCROLL_CHAIN_HOR` | Chain horizontal scroll to parent |
| `LV_OBJ_FLAG_SCROLL_CHAIN_VER` | Chain vertical scroll to parent |
| `LV_OBJ_FLAG_SCROLL_CHAIN` | Chain both directions (HOR + VER) |
| `LV_OBJ_FLAG_SCROLL_ON_FOCUS` | Auto-scroll to focused child |
| `LV_OBJ_FLAG_SCROLL_WITH_ARROW` | Allow scroll with arrow keys |
| `LV_OBJ_FLAG_SNAPPABLE` | Child can be snap target |
| `LV_OBJ_FLAG_PRESS_LOCK` | Keep pressed even if slid off |
| `LV_OBJ_FLAG_EVENT_BUBBLE` | Propagate events to parent |
| `LV_OBJ_FLAG_GESTURE_BUBBLE` | Propagate gestures to parent |
| `LV_OBJ_FLAG_ADV_HITTEST` | Use advanced hit testing |
| `LV_OBJ_FLAG_IGNORE_LAYOUT` | Ignore layout placement |
| `LV_OBJ_FLAG_FLOATING` | Do not scroll with parent |
| `LV_OBJ_FLAG_OVERFLOW_VISIBLE` | Do not clip children to parent boundary |
| `LV_OBJ_FLAG_LAYOUT_1` | Custom layout flag 1 |
| `LV_OBJ_FLAG_LAYOUT_2` | Custom layout flag 2 |
| `LV_OBJ_FLAG_WIDGET_1` | Custom widget flag 1 |
| `LV_OBJ_FLAG_WIDGET_2` | Custom widget flag 2 |
| `LV_OBJ_FLAG_USER_1` | Custom user flag 1 |
| `LV_OBJ_FLAG_USER_2` | Custom user flag 2 |
| `LV_OBJ_FLAG_USER_3` | Custom user flag 3 |
| `LV_OBJ_FLAG_USER_4` | Custom user flag 4 |

---

## Object States

| State | Description |
|-------|-------------|
| `LV_STATE_DEFAULT` | Normal state (0x0000) |
| `LV_STATE_CHECKED` | Toggled/checked |
| `LV_STATE_FOCUSED` | Focused via keypad/encoder |
| `LV_STATE_FOCUS_KEY` | Focused via keypad (not encoder) |
| `LV_STATE_EDITED` | Edited via encoder |
| `LV_STATE_HOVERED` | Mouse hovered (not yet pressed) |
| `LV_STATE_PRESSED` | Being pressed |
| `LV_STATE_SCROLLED` | Being scrolled |
| `LV_STATE_DISABLED` | Disabled |
| `LV_STATE_USER_1` | Custom state 1 |
| `LV_STATE_USER_2` | Custom state 2 |
| `LV_STATE_USER_3` | Custom state 3 |
| `LV_STATE_USER_4` | Custom state 4 |
| `LV_STATE_ANY` | Wildcard (match any state) |

---

## Widget Parts

| Part | Description | Used By |
|------|-------------|---------|
| `LV_PART_MAIN` | Background rectangle | All widgets |
| `LV_PART_SCROLLBAR` | Scrollbar(s) | Scrollable objects |
| `LV_PART_INDICATOR` | Indicator (e.g., filled portion) | Bar, slider, arc, meter |
| `LV_PART_KNOB` | Draggable knob | Slider, arc, colorwheel |
| `LV_PART_SELECTED` | Currently selected item | Roller, dropdown list, tabview |
| `LV_PART_ITEMS` | Items/cells | Table, btnmatrix, chart, calendar |
| `LV_PART_TICKS` | Tick marks/labels | Meter, chart |
| `LV_PART_CURSOR` | Cursor | Text area, chart |
| `LV_PART_CUSTOM_FIRST` | Start of custom parts (0x80) | User-defined widgets |
| `LV_PART_ANY` | Wildcard (match any part) | Style operations |

---

## Events

### Event Functions

```c
struct _lv_event_dsc_t * lv_obj_add_event_cb(
    lv_obj_t * obj,
    lv_event_cb_t event_cb,
    lv_event_code_t filter,        // LV_EVENT_ALL to receive all
    void * user_data
);
bool lv_obj_remove_event_cb(lv_obj_t * obj, lv_event_cb_t event_cb);
bool lv_obj_remove_event_dsc(lv_obj_t * obj, struct _lv_event_dsc_t * dsc);
lv_res_t lv_event_send(lv_obj_t * obj, lv_event_code_t event_code, void * param);

// Inside event callback
lv_event_code_t lv_event_get_code(lv_event_t * e);
lv_obj_t * lv_event_get_target(lv_event_t * e);
lv_obj_t * lv_event_get_current_target(lv_event_t * e);  // In bubble chain
void * lv_event_get_user_data(lv_event_t * e);
void * lv_event_get_param(lv_event_t * e);
```

### Event Codes -- Complete List

#### Input Device Events

| Code | Trigger |
|------|---------|
| `LV_EVENT_PRESSED` | Object has been pressed |
| `LV_EVENT_PRESSING` | Continuously while pressed |
| `LV_EVENT_PRESS_LOST` | Still pressed but cursor slid off |
| `LV_EVENT_SHORT_CLICKED` | Short press then release (no scroll) |
| `LV_EVENT_LONG_PRESSED` | Held for `long_press_time` |
| `LV_EVENT_LONG_PRESSED_REPEAT` | Repeating after long press |
| `LV_EVENT_CLICKED` | Released without scrolling |
| `LV_EVENT_RELEASED` | Released in any case |
| `LV_EVENT_SCROLL_BEGIN` | Scroll started |
| `LV_EVENT_SCROLL_END` | Scroll ended |
| `LV_EVENT_SCROLL` | During scrolling |
| `LV_EVENT_GESTURE` | Gesture detected (get with `lv_indev_get_gesture_dir`) |
| `LV_EVENT_KEY` | Key sent to object |
| `LV_EVENT_FOCUSED` | Object focused |
| `LV_EVENT_DEFOCUSED` | Object defocused |
| `LV_EVENT_LEAVE` | Object defocused but still selected |
| `LV_EVENT_HIT_TEST` | Advanced hit test check |

#### Drawing Events

| Code | Trigger |
|------|---------|
| `LV_EVENT_COVER_CHECK` | Check if object fully covers an area |
| `LV_EVENT_REFR_EXT_DRAW_SIZE` | Get required extra draw area |
| `LV_EVENT_DRAW_MAIN_BEGIN` | Main draw phase starting |
| `LV_EVENT_DRAW_MAIN` | Perform main drawing |
| `LV_EVENT_DRAW_MAIN_END` | Main draw phase ending |
| `LV_EVENT_DRAW_POST_BEGIN` | Post-draw phase starting |
| `LV_EVENT_DRAW_POST` | Perform post drawing |
| `LV_EVENT_DRAW_POST_END` | Post-draw phase ending |
| `LV_EVENT_DRAW_PART_BEGIN` | Part draw starting (customizable) |
| `LV_EVENT_DRAW_PART_END` | Part draw ending |

#### Special Events (widget-specific)

| Code | Trigger |
|------|---------|
| `LV_EVENT_VALUE_CHANGED` | Value changed (slider, checkbox, etc.) |
| `LV_EVENT_INSERT` | Text inserted (textarea) |
| `LV_EVENT_REFRESH` | Widget should refresh |
| `LV_EVENT_READY` | Process finished |
| `LV_EVENT_CANCEL` | Process cancelled |

#### Other Events

| Code | Trigger |
|------|---------|
| `LV_EVENT_DELETE` | Object being deleted |
| `LV_EVENT_CHILD_CHANGED` | Child added/removed |
| `LV_EVENT_CHILD_CREATED` | Child created (bubbles up) |
| `LV_EVENT_CHILD_DELETED` | Child deleted (bubbles up) |
| `LV_EVENT_SIZE_CHANGED` | Size/coords changed |
| `LV_EVENT_STYLE_CHANGED` | Style changed |
| `LV_EVENT_BASE_DIR_CHANGED` | Base direction changed |
| `LV_EVENT_GET_SELF_SIZE` | Get internal widget size |
| `LV_EVENT_SCREEN_UNLOAD_START` | Screen unload started |
| `LV_EVENT_SCREEN_LOAD_START` | Screen load started |
| `LV_EVENT_SCREEN_LOADED` | Screen loaded completely |
| `LV_EVENT_SCREEN_UNLOADED` | Screen unloaded completely |

#### Wildcard

| Code | Trigger |
|------|---------|
| `LV_EVENT_ALL` | Receive all events (no filter) |

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
void lv_obj_report_style_change(lv_style_t * style);   // Notify change for shared styles
void lv_obj_refresh_style(lv_obj_t * obj, lv_style_selector_t selector, lv_style_prop_t prop);

// Selector = part | state
// Example: LV_PART_MAIN | LV_STATE_PRESSED
```

### Local Styles (per object, no shared lv_style_t)

```c
void lv_obj_set_style_width(lv_obj_t * obj, lv_coord_t value, lv_style_selector_t selector);
void lv_obj_set_style_height(lv_obj_t * obj, lv_coord_t value, lv_style_selector_t selector);
// ... (one set_ function per property, see Style Properties below)
```

### Style Transitions

```c
// Define transition descriptor
static const lv_style_prop_t props[] = {LV_STYLE_BG_COLOR, LV_STYLE_BG_OPA, 0};
static lv_style_transition_dsc_t trans;
lv_style_transition_dsc_init(&trans, props, lv_anim_path_ease_out, 300, 0, NULL);

// Apply
lv_style_set_transition(&style, &trans);
```

---

## Style Properties

### Size and Position

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_WIDTH` | `lv_style_set_width(style, v)` | `lv_coord_t` |
| `LV_STYLE_MIN_WIDTH` | `lv_style_set_min_width(style, v)` | `lv_coord_t` |
| `LV_STYLE_MAX_WIDTH` | `lv_style_set_max_width(style, v)` | `lv_coord_t` |
| `LV_STYLE_HEIGHT` | `lv_style_set_height(style, v)` | `lv_coord_t` |
| `LV_STYLE_MIN_HEIGHT` | `lv_style_set_min_height(style, v)` | `lv_coord_t` |
| `LV_STYLE_MAX_HEIGHT` | `lv_style_set_max_height(style, v)` | `lv_coord_t` |
| `LV_STYLE_X` | `lv_style_set_x(style, v)` | `lv_coord_t` |
| `LV_STYLE_Y` | `lv_style_set_y(style, v)` | `lv_coord_t` |
| `LV_STYLE_ALIGN` | `lv_style_set_align(style, v)` | `lv_align_t` |

### Transform

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_TRANSFORM_WIDTH` | `lv_style_set_transform_width(style, v)` | `lv_coord_t` |
| `LV_STYLE_TRANSFORM_HEIGHT` | `lv_style_set_transform_height(style, v)` | `lv_coord_t` |
| `LV_STYLE_TRANSLATE_X` | `lv_style_set_translate_x(style, v)` | `lv_coord_t` |
| `LV_STYLE_TRANSLATE_Y` | `lv_style_set_translate_y(style, v)` | `lv_coord_t` |
| `LV_STYLE_TRANSFORM_ZOOM` | `lv_style_set_transform_zoom(style, v)` | `lv_coord_t` (256 = 100%) |
| `LV_STYLE_TRANSFORM_ANGLE` | `lv_style_set_transform_angle(style, v)` | `lv_coord_t` (0.1 degree units) |

### Padding

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_PAD_TOP` | `lv_style_set_pad_top(style, v)` | `lv_coord_t` |
| `LV_STYLE_PAD_BOTTOM` | `lv_style_set_pad_bottom(style, v)` | `lv_coord_t` |
| `LV_STYLE_PAD_LEFT` | `lv_style_set_pad_left(style, v)` | `lv_coord_t` |
| `LV_STYLE_PAD_RIGHT` | `lv_style_set_pad_right(style, v)` | `lv_coord_t` |
| `LV_STYLE_PAD_ROW` | `lv_style_set_pad_row(style, v)` | `lv_coord_t` |
| `LV_STYLE_PAD_COLUMN` | `lv_style_set_pad_column(style, v)` | `lv_coord_t` |

### Background

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_BG_COLOR` | `lv_style_set_bg_color(style, v)` | `lv_color_t` |
| `LV_STYLE_BG_OPA` | `lv_style_set_bg_opa(style, v)` | `lv_opa_t` |
| `LV_STYLE_BG_GRAD_COLOR` | `lv_style_set_bg_grad_color(style, v)` | `lv_color_t` |
| `LV_STYLE_BG_GRAD_DIR` | `lv_style_set_bg_grad_dir(style, v)` | `lv_grad_dir_t` |
| `LV_STYLE_BG_MAIN_STOP` | `lv_style_set_bg_main_stop(style, v)` | `lv_coord_t` (0-255) |
| `LV_STYLE_BG_GRAD_STOP` | `lv_style_set_bg_grad_stop(style, v)` | `lv_coord_t` (0-255) |
| `LV_STYLE_BG_GRAD` | `lv_style_set_bg_grad(style, v)` | `lv_grad_dsc_t *` |
| `LV_STYLE_BG_DITHER_MODE` | `lv_style_set_bg_dither_mode(style, v)` | `lv_dither_mode_t` |
| `LV_STYLE_BG_IMG_SRC` | `lv_style_set_bg_img_src(style, v)` | `const void *` |
| `LV_STYLE_BG_IMG_OPA` | `lv_style_set_bg_img_opa(style, v)` | `lv_opa_t` |
| `LV_STYLE_BG_IMG_RECOLOR` | `lv_style_set_bg_img_recolor(style, v)` | `lv_color_t` |
| `LV_STYLE_BG_IMG_RECOLOR_OPA` | `lv_style_set_bg_img_recolor_opa(style, v)` | `lv_opa_t` |
| `LV_STYLE_BG_IMG_TILED` | `lv_style_set_bg_img_tiled(style, v)` | `bool` |

### Border

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_BORDER_COLOR` | `lv_style_set_border_color(style, v)` | `lv_color_t` |
| `LV_STYLE_BORDER_OPA` | `lv_style_set_border_opa(style, v)` | `lv_opa_t` |
| `LV_STYLE_BORDER_WIDTH` | `lv_style_set_border_width(style, v)` | `lv_coord_t` |
| `LV_STYLE_BORDER_SIDE` | `lv_style_set_border_side(style, v)` | `lv_border_side_t` |
| `LV_STYLE_BORDER_POST` | `lv_style_set_border_post(style, v)` | `bool` |

### Outline

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_OUTLINE_WIDTH` | `lv_style_set_outline_width(style, v)` | `lv_coord_t` |
| `LV_STYLE_OUTLINE_COLOR` | `lv_style_set_outline_color(style, v)` | `lv_color_t` |
| `LV_STYLE_OUTLINE_OPA` | `lv_style_set_outline_opa(style, v)` | `lv_opa_t` |
| `LV_STYLE_OUTLINE_PAD` | `lv_style_set_outline_pad(style, v)` | `lv_coord_t` |

### Shadow

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_SHADOW_WIDTH` | `lv_style_set_shadow_width(style, v)` | `lv_coord_t` |
| `LV_STYLE_SHADOW_OFS_X` | `lv_style_set_shadow_ofs_x(style, v)` | `lv_coord_t` |
| `LV_STYLE_SHADOW_OFS_Y` | `lv_style_set_shadow_ofs_y(style, v)` | `lv_coord_t` |
| `LV_STYLE_SHADOW_SPREAD` | `lv_style_set_shadow_spread(style, v)` | `lv_coord_t` |
| `LV_STYLE_SHADOW_COLOR` | `lv_style_set_shadow_color(style, v)` | `lv_color_t` |
| `LV_STYLE_SHADOW_OPA` | `lv_style_set_shadow_opa(style, v)` | `lv_opa_t` |

### Image

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_IMG_OPA` | `lv_style_set_img_opa(style, v)` | `lv_opa_t` |
| `LV_STYLE_IMG_RECOLOR` | `lv_style_set_img_recolor(style, v)` | `lv_color_t` |
| `LV_STYLE_IMG_RECOLOR_OPA` | `lv_style_set_img_recolor_opa(style, v)` | `lv_opa_t` |

### Line

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_LINE_WIDTH` | `lv_style_set_line_width(style, v)` | `lv_coord_t` |
| `LV_STYLE_LINE_DASH_WIDTH` | `lv_style_set_line_dash_width(style, v)` | `lv_coord_t` |
| `LV_STYLE_LINE_DASH_GAP` | `lv_style_set_line_dash_gap(style, v)` | `lv_coord_t` |
| `LV_STYLE_LINE_ROUNDED` | `lv_style_set_line_rounded(style, v)` | `bool` |
| `LV_STYLE_LINE_COLOR` | `lv_style_set_line_color(style, v)` | `lv_color_t` |
| `LV_STYLE_LINE_OPA` | `lv_style_set_line_opa(style, v)` | `lv_opa_t` |

### Arc

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_ARC_WIDTH` | `lv_style_set_arc_width(style, v)` | `lv_coord_t` |
| `LV_STYLE_ARC_ROUNDED` | `lv_style_set_arc_rounded(style, v)` | `bool` |
| `LV_STYLE_ARC_COLOR` | `lv_style_set_arc_color(style, v)` | `lv_color_t` |
| `LV_STYLE_ARC_OPA` | `lv_style_set_arc_opa(style, v)` | `lv_opa_t` |
| `LV_STYLE_ARC_IMG_SRC` | `lv_style_set_arc_img_src(style, v)` | `const void *` |

### Text

| Property | Set Function | Type | Inherited |
|----------|-------------|------|-----------|
| `LV_STYLE_TEXT_COLOR` | `lv_style_set_text_color(style, v)` | `lv_color_t` | Yes |
| `LV_STYLE_TEXT_OPA` | `lv_style_set_text_opa(style, v)` | `lv_opa_t` | Yes |
| `LV_STYLE_TEXT_FONT` | `lv_style_set_text_font(style, v)` | `const lv_font_t *` | Yes |
| `LV_STYLE_TEXT_LETTER_SPACE` | `lv_style_set_text_letter_space(style, v)` | `lv_coord_t` | Yes |
| `LV_STYLE_TEXT_LINE_SPACE` | `lv_style_set_text_line_space(style, v)` | `lv_coord_t` | Yes |
| `LV_STYLE_TEXT_DECOR` | `lv_style_set_text_decor(style, v)` | `lv_text_decor_t` | Yes |
| `LV_STYLE_TEXT_ALIGN` | `lv_style_set_text_align(style, v)` | `lv_text_align_t` | Yes |

### Miscellaneous

| Property | Set Function | Type |
|----------|-------------|------|
| `LV_STYLE_RADIUS` | `lv_style_set_radius(style, v)` | `lv_coord_t` |
| `LV_STYLE_CLIP_CORNER` | `lv_style_set_clip_corner(style, v)` | `bool` |
| `LV_STYLE_OPA` | `lv_style_set_opa(style, v)` | `lv_opa_t` (inherited) |
| `LV_STYLE_COLOR_FILTER_DSC` | `lv_style_set_color_filter_dsc(style, v)` | `const lv_color_filter_dsc_t *` |
| `LV_STYLE_COLOR_FILTER_OPA` | `lv_style_set_color_filter_opa(style, v)` | `lv_opa_t` |
| `LV_STYLE_ANIM_TIME` | `lv_style_set_anim_time(style, v)` | `uint32_t` ms |
| `LV_STYLE_ANIM_SPEED` | `lv_style_set_anim_speed(style, v)` | `uint32_t` |
| `LV_STYLE_TRANSITION` | `lv_style_set_transition(style, v)` | `const lv_style_transition_dsc_t *` |
| `LV_STYLE_BLEND_MODE` | `lv_style_set_blend_mode(style, v)` | `lv_blend_mode_t` |
| `LV_STYLE_LAYOUT` | `lv_style_set_layout(style, v)` | `uint16_t` |
| `LV_STYLE_BASE_DIR` | `lv_style_set_base_dir(style, v)` | `lv_base_dir_t` (inherited) |

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

---

## Animation API

```c
void lv_anim_init(lv_anim_t * a);
lv_anim_t * lv_anim_start(const lv_anim_t * a);
bool lv_anim_del(void * var, lv_anim_exec_xcb_t exec_cb);  // NULL exec_cb = delete all for var
void lv_anim_del_all(void);
lv_anim_t * lv_anim_get(void * var, lv_anim_exec_xcb_t exec_cb);
uint16_t lv_anim_count_running(void);
uint32_t lv_anim_speed_to_time(uint32_t speed, int32_t start, int32_t end);
void lv_anim_refr_now(void);

// Configuration (before lv_anim_start)
void lv_anim_set_var(lv_anim_t * a, void * var);
void lv_anim_set_exec_cb(lv_anim_t * a, lv_anim_exec_xcb_t exec_cb);
void lv_anim_set_time(lv_anim_t * a, uint32_t duration);
void lv_anim_set_delay(lv_anim_t * a, uint32_t delay);
void lv_anim_set_values(lv_anim_t * a, int32_t start, int32_t end);
void lv_anim_set_path_cb(lv_anim_t * a, lv_anim_path_cb_t path_cb);
void lv_anim_set_start_cb(lv_anim_t * a, lv_anim_start_cb_t start_cb);
void lv_anim_set_ready_cb(lv_anim_t * a, lv_anim_ready_cb_t ready_cb);
void lv_anim_set_deleted_cb(lv_anim_t * a, lv_anim_deleted_cb_t deleted_cb);
void lv_anim_set_playback_time(lv_anim_t * a, uint32_t time);
void lv_anim_set_playback_delay(lv_anim_t * a, uint32_t delay);
void lv_anim_set_repeat_count(lv_anim_t * a, uint16_t cnt);   // LV_ANIM_REPEAT_INFINITE
void lv_anim_set_repeat_delay(lv_anim_t * a, uint32_t delay);
void lv_anim_set_early_apply(lv_anim_t * a, bool en);
```

### Built-in Animation Paths

| Path Callback | Curve |
|--------------|-------|
| `lv_anim_path_linear` | Linear (constant speed) |
| `lv_anim_path_ease_in` | Slow start |
| `lv_anim_path_ease_out` | Slow end |
| `lv_anim_path_ease_in_out` | Slow start and end |
| `lv_anim_path_overshoot` | Overshoot target |
| `lv_anim_path_bounce` | Bounce at target |
| `lv_anim_path_step` | Immediate jump |

### Animation Example

```c
lv_anim_t a;
lv_anim_init(&a);
lv_anim_set_var(&a, obj);
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
lv_anim_set_values(&a, 0, 200);
lv_anim_set_time(&a, 500);
lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
lv_anim_set_playback_time(&a, 500);
lv_anim_start(&a);
```

---

## Timer API

```c
lv_timer_t * lv_timer_create(lv_timer_cb_t timer_xcb, uint32_t period, void * user_data);
void lv_timer_del(lv_timer_t * timer);
void lv_timer_pause(lv_timer_t * timer);
void lv_timer_resume(lv_timer_t * timer);
void lv_timer_set_cb(lv_timer_t * timer, lv_timer_cb_t timer_cb);
void lv_timer_set_period(lv_timer_t * timer, uint32_t period);
void lv_timer_set_repeat_count(lv_timer_t * timer, int32_t repeat_count); // -1 = infinite
void lv_timer_ready(lv_timer_t * timer);   // Execute on next lv_timer_handler call
void lv_timer_reset(lv_timer_t * timer);   // Reset period counter
void lv_timer_enable(bool en);             // Enable/disable all timers
uint8_t lv_timer_get_idle(void);           // Get idle percentage (0-100)
lv_timer_t * lv_timer_get_next(lv_timer_t * timer);  // Iterate timers
```

---

## Font API

### Built-in Fonts (Montserrat)

| Config Macro | Font | Size | Range |
|-------------|------|------|-------|
| `LV_FONT_MONTSERRAT_8` | `lv_font_montserrat_8` | 8px | Basic Latin |
| `LV_FONT_MONTSERRAT_10` | `lv_font_montserrat_10` | 10px | Basic Latin |
| `LV_FONT_MONTSERRAT_12` | `lv_font_montserrat_12` | 12px | Basic Latin |
| `LV_FONT_MONTSERRAT_14` | `lv_font_montserrat_14` | 14px | Basic Latin (default) |
| `LV_FONT_MONTSERRAT_16` | `lv_font_montserrat_16` | 16px | Basic Latin |
| `LV_FONT_MONTSERRAT_18` | `lv_font_montserrat_18` | 18px | Basic Latin |
| `LV_FONT_MONTSERRAT_20` | `lv_font_montserrat_20` | 20px | Basic Latin |
| `LV_FONT_MONTSERRAT_22` | `lv_font_montserrat_22` | 22px | Basic Latin |
| `LV_FONT_MONTSERRAT_24` | `lv_font_montserrat_24` | 24px | Basic Latin |
| `LV_FONT_MONTSERRAT_26` | `lv_font_montserrat_26` | 26px | Basic Latin |
| `LV_FONT_MONTSERRAT_28` | `lv_font_montserrat_28` | 28px | Basic Latin |
| `LV_FONT_MONTSERRAT_30` | `lv_font_montserrat_30` | 30px | Basic Latin |
| `LV_FONT_MONTSERRAT_32` | `lv_font_montserrat_32` | 32px | Basic Latin |
| `LV_FONT_MONTSERRAT_34` | `lv_font_montserrat_34` | 34px | Basic Latin |
| `LV_FONT_MONTSERRAT_36` | `lv_font_montserrat_36` | 36px | Basic Latin |
| `LV_FONT_MONTSERRAT_38` | `lv_font_montserrat_38` | 38px | Basic Latin |
| `LV_FONT_MONTSERRAT_40` | `lv_font_montserrat_40` | 40px | Basic Latin |
| `LV_FONT_MONTSERRAT_42` | `lv_font_montserrat_42` | 42px | Basic Latin |
| `LV_FONT_MONTSERRAT_44` | `lv_font_montserrat_44` | 44px | Basic Latin |
| `LV_FONT_MONTSERRAT_46` | `lv_font_montserrat_46` | 46px | Basic Latin |
| `LV_FONT_MONTSERRAT_48` | `lv_font_montserrat_48` | 48px | Basic Latin |

### Special Built-in Fonts

| Config Macro | Font | Description |
|-------------|------|-------------|
| `LV_FONT_MONTSERRAT_12_SUBPX` | `lv_font_montserrat_12_subpx` | 12px with sub-pixel rendering |
| `LV_FONT_MONTSERRAT_28_COMPRESSED` | `lv_font_montserrat_28_compressed` | 28px compressed (3 bpp) |
| `LV_FONT_DEJAVU_16_PERSIAN_HEBREW` | `lv_font_dejavu_16_persian_hebrew` | DejaVu with RTL scripts |
| `LV_FONT_SIMSUN_16_CJK` | `lv_font_simsun_16_cjk` | CJK characters |
| `LV_FONT_UNSCII_8` | `lv_font_unscii_8` | Monospace pixel font |
| `LV_FONT_UNSCII_16` | `lv_font_unscii_16` | Monospace pixel font |

### Font Functions

```c
const lv_font_t * lv_font_get_default(void);
bool lv_font_is_whitespace(const lv_font_t * font, uint32_t letter);
uint16_t lv_font_get_glyph_width(const lv_font_t * font, uint32_t letter, uint32_t letter_next);
lv_coord_t lv_font_get_line_height(const lv_font_t * font);
```

---

## Image Decoder API

### Image Color Formats

| Constant | Description |
|----------|-------------|
| `LV_IMG_CF_UNKNOWN` | Unknown format |
| `LV_IMG_CF_RAW` | Raw pixels with header |
| `LV_IMG_CF_RAW_ALPHA` | Raw with alpha channel |
| `LV_IMG_CF_RAW_CHROMA_KEYED` | Raw with chroma key transparency |
| `LV_IMG_CF_TRUE_COLOR` | Native color depth |
| `LV_IMG_CF_TRUE_COLOR_ALPHA` | Native + alpha byte |
| `LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED` | Native + chroma key |
| `LV_IMG_CF_INDEXED_1BIT` | 1-bit indexed (2 colors) |
| `LV_IMG_CF_INDEXED_2BIT` | 2-bit indexed (4 colors) |
| `LV_IMG_CF_INDEXED_4BIT` | 4-bit indexed (16 colors) |
| `LV_IMG_CF_INDEXED_8BIT` | 8-bit indexed (256 colors) |
| `LV_IMG_CF_ALPHA_1BIT` | 1-bit alpha mask |
| `LV_IMG_CF_ALPHA_2BIT` | 2-bit alpha mask |
| `LV_IMG_CF_ALPHA_4BIT` | 4-bit alpha mask |
| `LV_IMG_CF_ALPHA_8BIT` | 8-bit alpha mask |

### Image Functions

```c
lv_img_dsc_t * lv_img_buf_alloc(lv_coord_t w, lv_coord_t h, lv_img_cf_t cf);
void lv_img_buf_free(lv_img_dsc_t * dsc);
lv_color_t lv_img_buf_get_px_color(lv_img_dsc_t * dsc, lv_coord_t x, lv_coord_t y, lv_color_t color);
lv_opa_t lv_img_buf_get_px_alpha(lv_img_dsc_t * dsc, lv_coord_t x, lv_coord_t y);
void lv_img_buf_set_px_color(lv_img_dsc_t * dsc, lv_coord_t x, lv_coord_t y, lv_color_t c);
void lv_img_buf_set_px_alpha(lv_img_dsc_t * dsc, lv_coord_t x, lv_coord_t y, lv_opa_t opa);
void lv_img_buf_set_palette(lv_img_dsc_t * dsc, uint8_t id, lv_color32_t c);
uint32_t lv_img_buf_get_img_size(lv_coord_t w, lv_coord_t h, lv_img_cf_t cf);
```

### Image Decoders (File Formats)

| Config Macro | Format | Notes |
|-------------|--------|-------|
| `LV_USE_PNG` | PNG | libpng based |
| `LV_USE_BMP` | BMP | Built-in decoder |
| `LV_USE_SJPG` | Split JPEG | Chunk-based for low memory |
| `LV_USE_GIF` | GIF | Animation support |
| `LV_USE_QRCODE` | QR Code | QR code generator |
| `LV_USE_FFMPEG` | FFmpeg | Video/audio (desktop) |

---

## File System API

```c
// Register a driver
void lv_fs_drv_init(lv_fs_drv_t * drv);
void lv_fs_drv_register(lv_fs_drv_t * drv);

// File operations (letter: prefix e.g., "S:/path/file.txt")
lv_fs_res_t lv_fs_open(lv_fs_file_t * file_p, const char * path, lv_fs_mode_t mode);
lv_fs_res_t lv_fs_close(lv_fs_file_t * file_p);
lv_fs_res_t lv_fs_read(lv_fs_file_t * file_p, void * buf, uint32_t btr, uint32_t * br);
lv_fs_res_t lv_fs_write(lv_fs_file_t * file_p, const void * buf, uint32_t btw, uint32_t * bw);
lv_fs_res_t lv_fs_seek(lv_fs_file_t * file_p, uint32_t pos, lv_fs_whence_t whence);
lv_fs_res_t lv_fs_tell(lv_fs_file_t * file_p, uint32_t * pos_p);

// Directory operations
lv_fs_res_t lv_fs_dir_open(lv_fs_dir_t * rddir_p, const char * path);
lv_fs_res_t lv_fs_dir_read(lv_fs_dir_t * rddir_p, char * fn);
lv_fs_res_t lv_fs_dir_close(lv_fs_dir_t * rddir_p);

// Utility
bool lv_fs_is_ready(char letter);
char * lv_fs_get_letters(char * buf);
const char * lv_fs_get_ext(const char * fn);
```

---

## Layout API (Flex)

```c
// Set flex flow direction
void lv_obj_set_flex_flow(lv_obj_t * obj, lv_flex_flow_t flow);

// Set alignment of children
void lv_obj_set_flex_align(
    lv_obj_t * obj,
    lv_flex_align_t main_place,       // Along main axis
    lv_flex_align_t cross_place,      // Along cross axis
    lv_flex_align_t track_cross_place // Between tracks (wrap mode)
);

// Set flex grow factor on a child
void lv_obj_set_flex_grow(lv_obj_t * obj, uint8_t grow);
```

### Flex Flow Constants

| Constant | Direction |
|----------|-----------|
| `LV_FLEX_FLOW_ROW` | Left to right |
| `LV_FLEX_FLOW_COLUMN` | Top to bottom |
| `LV_FLEX_FLOW_ROW_WRAP` | Left to right, wrap |
| `LV_FLEX_FLOW_ROW_REVERSE` | Right to left |
| `LV_FLEX_FLOW_ROW_WRAP_REVERSE` | Right to left, wrap |
| `LV_FLEX_FLOW_COLUMN_WRAP` | Top to bottom, wrap |
| `LV_FLEX_FLOW_COLUMN_REVERSE` | Bottom to top |
| `LV_FLEX_FLOW_COLUMN_WRAP_REVERSE` | Bottom to top, wrap |

### Flex Align Constants

| Constant | Behavior |
|----------|----------|
| `LV_FLEX_ALIGN_START` | Pack to start |
| `LV_FLEX_ALIGN_END` | Pack to end |
| `LV_FLEX_ALIGN_CENTER` | Center |
| `LV_FLEX_ALIGN_SPACE_EVENLY` | Equal space everywhere |
| `LV_FLEX_ALIGN_SPACE_AROUND` | Equal space around items |
| `LV_FLEX_ALIGN_SPACE_BETWEEN` | Space between items only |

---

## Layout API (Grid)

```c
// Set grid descriptors
void lv_obj_set_grid_dsc_array(
    lv_obj_t * obj,
    const lv_coord_t col_dsc[],    // Terminated with LV_GRID_TEMPLATE_LAST
    const lv_coord_t row_dsc[]     // Terminated with LV_GRID_TEMPLATE_LAST
);

// Set grid alignment
void lv_obj_set_grid_align(
    lv_obj_t * obj,
    lv_grid_align_t column_align,
    lv_grid_align_t row_align
);

// Place a child in a grid cell
void lv_obj_set_grid_cell(
    lv_obj_t * obj,
    lv_grid_align_t column_align, uint8_t col_pos, uint8_t col_span,
    lv_grid_align_t row_align, uint8_t row_pos, uint8_t row_span
);
```

### Grid Track Size Constants

| Constant | Meaning |
|----------|---------|
| `LV_GRID_CONTENT` | Size to largest child |
| `LV_GRID_FR(x)` | Free unit (proportional) |
| `LV_GRID_TEMPLATE_LAST` | Array terminator |

### Grid Align Constants

| Constant | Behavior |
|----------|----------|
| `LV_GRID_ALIGN_START` | Align to start |
| `LV_GRID_ALIGN_END` | Align to end |
| `LV_GRID_ALIGN_CENTER` | Center in cell |
| `LV_GRID_ALIGN_STRETCH` | Fill cell |
| `LV_GRID_ALIGN_SPACE_EVENLY` | Distribute evenly |
| `LV_GRID_ALIGN_SPACE_AROUND` | Space around |
| `LV_GRID_ALIGN_SPACE_BETWEEN` | Space between |

---

## Color API

```c
// Create colors
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b);
lv_color_t lv_color_hex(uint32_t c);       // e.g., lv_color_hex(0xFF0000)
lv_color_t lv_color_hex3(uint16_t c);      // e.g., lv_color_hex3(0xF00)
lv_color_t lv_color_hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v);
lv_color_hsv_t lv_color_rgb_to_hsv(uint8_t r8, uint8_t g8, uint8_t b8);
lv_color_hsv_t lv_color_to_hsv(lv_color_t color);

// Color operations
lv_color_t lv_color_lighten(lv_color_t c, lv_opa_t lvl);
lv_color_t lv_color_darken(lv_color_t c, lv_opa_t lvl);
lv_color_t lv_color_mix(lv_color_t c1, lv_color_t c2, uint8_t mix);  // 0=c2, 255=c1
lv_color_t lv_color_change_lightness(lv_color_t c, lv_opa_t lvl);

// Conversion
uint8_t lv_color_brightness(lv_color_t color);
uint16_t lv_color_to16(lv_color_t color);
uint32_t lv_color_to32(lv_color_t color);

// Built-in color palette
lv_color_t lv_palette_main(lv_palette_t p);
lv_color_t lv_palette_lighten(lv_palette_t p, uint8_t lvl);  // lvl: 1-5
lv_color_t lv_palette_darken(lv_palette_t p, uint8_t lvl);   // lvl: 1-4
```

### Palette Constants

| Constant | Color |
|----------|-------|
| `LV_PALETTE_RED` | Red |
| `LV_PALETTE_PINK` | Pink |
| `LV_PALETTE_PURPLE` | Purple |
| `LV_PALETTE_DEEP_PURPLE` | Deep purple |
| `LV_PALETTE_INDIGO` | Indigo |
| `LV_PALETTE_BLUE` | Blue |
| `LV_PALETTE_LIGHT_BLUE` | Light blue |
| `LV_PALETTE_CYAN` | Cyan |
| `LV_PALETTE_TEAL` | Teal |
| `LV_PALETTE_GREEN` | Green |
| `LV_PALETTE_LIGHT_GREEN` | Light green |
| `LV_PALETTE_LIME` | Lime |
| `LV_PALETTE_YELLOW` | Yellow |
| `LV_PALETTE_AMBER` | Amber |
| `LV_PALETTE_ORANGE` | Orange |
| `LV_PALETTE_DEEP_ORANGE` | Deep orange |
| `LV_PALETTE_BROWN` | Brown |
| `LV_PALETTE_BLUE_GREY` | Blue grey |
| `LV_PALETTE_GREY` | Grey |

---

## Drawing API

### Draw Descriptors

```c
void lv_draw_rect_dsc_init(lv_draw_rect_dsc_t * dsc);
void lv_draw_rect(const lv_area_t * coords, const lv_area_t * clip, const lv_draw_rect_dsc_t * dsc);

void lv_draw_label_dsc_init(lv_draw_label_dsc_t * dsc);
void lv_draw_label(const lv_area_t * coords, const lv_area_t * clip, const lv_draw_label_dsc_t * dsc, const char * txt, lv_draw_label_hint_t * hint);

void lv_draw_img_dsc_init(lv_draw_img_dsc_t * dsc);
void lv_draw_img(const lv_area_t * coords, const lv_area_t * clip, const void * src, const lv_draw_img_dsc_t * dsc);

void lv_draw_line_dsc_init(lv_draw_line_dsc_t * dsc);
void lv_draw_line(const lv_point_t * p1, const lv_point_t * p2, const lv_area_t * clip, const lv_draw_line_dsc_t * dsc);

void lv_draw_arc_dsc_init(lv_draw_arc_dsc_t * dsc);
void lv_draw_arc(lv_coord_t cx, lv_coord_t cy, uint16_t r, uint16_t start_angle, uint16_t end_angle, const lv_area_t * clip, const lv_draw_arc_dsc_t * dsc);
```

### Snapshot

```c
lv_img_dsc_t * lv_snapshot_take(lv_obj_t * obj, lv_img_cf_t cf);
lv_res_t lv_snapshot_take_to_buf(lv_obj_t * obj, lv_img_cf_t cf, lv_img_dsc_t * dsc, void * buf, uint32_t buf_size);
void lv_snapshot_free(lv_img_dsc_t * dsc);
uint32_t lv_snapshot_buf_size_needed(lv_obj_t * obj, lv_img_cf_t cf);
```

---

## Group API

```c
lv_group_t * lv_group_create(void);
void lv_group_del(lv_group_t * group);
void lv_group_set_default(lv_group_t * group);
lv_group_t * lv_group_get_default(void);

void lv_group_add_obj(lv_group_t * group, lv_obj_t * obj);
void lv_group_remove_obj(lv_obj_t * obj);
void lv_group_remove_all_objs(lv_group_t * group);
lv_obj_t * lv_group_get_focused(const lv_group_t * group);

void lv_group_focus_next(lv_group_t * group);
void lv_group_focus_prev(lv_group_t * group);
void lv_group_focus_obj(lv_obj_t * obj);
void lv_group_focus_freeze(lv_group_t * group, bool en);

void lv_group_send_data(lv_group_t * group, uint32_t c);
void lv_group_set_focus_cb(lv_group_t * group, lv_group_focus_cb_t focus_cb);
void lv_group_set_editing(lv_group_t * group, bool edit);
void lv_group_set_wrap(lv_group_t * group, bool en);

uint32_t lv_group_get_obj_count(lv_group_t * group);
bool lv_group_get_editing(const lv_group_t * group);
bool lv_group_get_wrap(lv_group_t * group);
```

---

## Screen API

```c
lv_obj_t * lv_scr_act(void);                              // Get active screen
void lv_scr_load(lv_obj_t * scr);                         // Load screen (no animation)
void lv_scr_load_anim(                                     // Load with animation
    lv_obj_t * scr,
    lv_scr_load_anim_t anim_type,
    uint32_t time,
    uint32_t delay,
    bool auto_del                                          // Auto-delete old screen
);

lv_obj_t * lv_layer_top(void);     // Top layer (above screens, below popups)
lv_obj_t * lv_layer_sys(void);     // System layer (above everything)
```

### Screen Load Animation Types

| Constant | Effect |
|----------|--------|
| `LV_SCR_LOAD_ANIM_NONE` | No animation |
| `LV_SCR_LOAD_ANIM_OVER_LEFT` | New slides in from right |
| `LV_SCR_LOAD_ANIM_OVER_RIGHT` | New slides in from left |
| `LV_SCR_LOAD_ANIM_OVER_TOP` | New slides in from bottom |
| `LV_SCR_LOAD_ANIM_OVER_BOTTOM` | New slides in from top |
| `LV_SCR_LOAD_ANIM_MOVE_LEFT` | Both screens move left |
| `LV_SCR_LOAD_ANIM_MOVE_RIGHT` | Both screens move right |
| `LV_SCR_LOAD_ANIM_MOVE_TOP` | Both screens move up |
| `LV_SCR_LOAD_ANIM_MOVE_BOTTOM` | Both screens move down |
| `LV_SCR_LOAD_ANIM_FADE_ON` | Fade in new screen |

---

## Core Widgets

### Arc (`lv_arc`)

```c
lv_obj_t * lv_arc_create(lv_obj_t * parent);

// Set
void lv_arc_set_start_angle(lv_obj_t * arc, uint16_t start);
void lv_arc_set_end_angle(lv_obj_t * arc, uint16_t end);
void lv_arc_set_angles(lv_obj_t * arc, uint16_t start, uint16_t end);
void lv_arc_set_bg_start_angle(lv_obj_t * arc, uint16_t start);
void lv_arc_set_bg_end_angle(lv_obj_t * arc, uint16_t end);
void lv_arc_set_bg_angles(lv_obj_t * arc, uint16_t start, uint16_t end);
void lv_arc_set_rotation(lv_obj_t * arc, uint16_t rotation);
void lv_arc_set_mode(lv_obj_t * arc, lv_arc_mode_t type);
void lv_arc_set_value(lv_obj_t * arc, int16_t value);
void lv_arc_set_range(lv_obj_t * arc, int16_t min, int16_t max);
void lv_arc_set_change_rate(lv_obj_t * arc, uint16_t rate);

// Get
uint16_t lv_arc_get_angle_start(lv_obj_t * arc);
uint16_t lv_arc_get_angle_end(lv_obj_t * arc);
uint16_t lv_arc_get_bg_angle_start(lv_obj_t * arc);
uint16_t lv_arc_get_bg_angle_end(lv_obj_t * arc);
int16_t lv_arc_get_value(const lv_obj_t * arc);
int16_t lv_arc_get_min_value(const lv_obj_t * arc);
int16_t lv_arc_get_max_value(const lv_obj_t * arc);
lv_arc_mode_t lv_arc_get_mode(const lv_obj_t * arc);
```

**Arc Modes**: `LV_ARC_MODE_NORMAL`, `LV_ARC_MODE_SYMMETRICAL`, `LV_ARC_MODE_REVERSE`

### Bar (`lv_bar`)

```c
lv_obj_t * lv_bar_create(lv_obj_t * parent);

void lv_bar_set_value(lv_obj_t * bar, int32_t value, lv_anim_enable_t anim);
void lv_bar_set_start_value(lv_obj_t * bar, int32_t start_value, lv_anim_enable_t anim);
void lv_bar_set_range(lv_obj_t * bar, int32_t min, int32_t max);
void lv_bar_set_mode(lv_obj_t * bar, lv_bar_mode_t mode);

int32_t lv_bar_get_value(const lv_obj_t * bar);
int32_t lv_bar_get_start_value(const lv_obj_t * bar);
int32_t lv_bar_get_min_value(const lv_obj_t * bar);
int32_t lv_bar_get_max_value(const lv_obj_t * bar);
lv_bar_mode_t lv_bar_get_mode(lv_obj_t * bar);
```

**Bar Modes**: `LV_BAR_MODE_NORMAL`, `LV_BAR_MODE_SYMMETRICAL`, `LV_BAR_MODE_RANGE`

### Button (`lv_btn`)

```c
lv_obj_t * lv_btn_create(lv_obj_t * parent);
// No widget-specific functions. Use lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE) for toggle.
// Style with LV_STATE_PRESSED, LV_STATE_CHECKED, etc.
```

### Button Matrix (`lv_btnmatrix`)

```c
lv_obj_t * lv_btnmatrix_create(lv_obj_t * parent);

void lv_btnmatrix_set_map(lv_obj_t * btnm, const char * map[]);
void lv_btnmatrix_set_ctrl_map(lv_obj_t * btnm, const lv_btnmatrix_ctrl_t ctrl_map[]);
void lv_btnmatrix_set_selected_btn(lv_obj_t * btnm, uint16_t btn_id);
void lv_btnmatrix_set_btn_ctrl(lv_obj_t * btnm, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_clear_btn_ctrl(lv_obj_t * btnm, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_set_btn_ctrl_all(lv_obj_t * btnm, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_clear_btn_ctrl_all(lv_obj_t * btnm, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_set_btn_width(lv_obj_t * btnm, uint16_t btn_id, uint8_t width);
void lv_btnmatrix_set_one_checked(lv_obj_t * btnm, bool en);

const char ** lv_btnmatrix_get_map(const lv_obj_t * btnm);
uint16_t lv_btnmatrix_get_selected_btn(const lv_obj_t * btnm);
const char * lv_btnmatrix_get_btn_text(const lv_obj_t * btnm, uint16_t btn_id);
bool lv_btnmatrix_has_btn_ctrl(lv_obj_t * btnm, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl);
bool lv_btnmatrix_get_one_checked(const lv_obj_t * btnm);
```

**Button Matrix Controls**: `LV_BTNMATRIX_CTRL_HIDDEN`, `LV_BTNMATRIX_CTRL_NO_REPEAT`, `LV_BTNMATRIX_CTRL_DISABLED`, `LV_BTNMATRIX_CTRL_CHECKABLE`, `LV_BTNMATRIX_CTRL_CHECKED`, `LV_BTNMATRIX_CTRL_CLICK_TRIG`, `LV_BTNMATRIX_CTRL_POPOVER`, `LV_BTNMATRIX_CTRL_RECOLOR`, `LV_BTNMATRIX_CTRL_CUSTOM_1`, `LV_BTNMATRIX_CTRL_CUSTOM_2`

### Canvas (`lv_canvas`)

```c
lv_obj_t * lv_canvas_create(lv_obj_t * parent);

void lv_canvas_set_buffer(lv_obj_t * canvas, void * buf, lv_coord_t w, lv_coord_t h, lv_img_cf_t cf);
void lv_canvas_set_px_color(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y, lv_color_t c);
void lv_canvas_set_px_opa(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y, lv_opa_t opa);
void lv_canvas_set_palette(lv_obj_t * canvas, uint8_t id, lv_color32_t c);

lv_color_t lv_canvas_get_px(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y);
lv_img_dsc_t * lv_canvas_get_img(lv_obj_t * canvas);

void lv_canvas_copy_buf(lv_obj_t * canvas, const void * to_copy, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h);
void lv_canvas_transform(lv_obj_t * canvas, lv_img_dsc_t * img, int16_t angle, uint16_t zoom, lv_coord_t offset_x, lv_coord_t offset_y, int32_t pivot_x, int32_t pivot_y, bool antialias);
void lv_canvas_blur_hor(lv_obj_t * canvas, const lv_area_t * area, uint16_t r);
void lv_canvas_blur_ver(lv_obj_t * canvas, const lv_area_t * area, uint16_t r);
void lv_canvas_fill_bg(lv_obj_t * canvas, lv_color_t color, lv_opa_t opa);

// Draw on canvas
void lv_canvas_draw_rect(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, const lv_draw_rect_dsc_t * draw_dsc);
void lv_canvas_draw_text(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y, lv_coord_t max_w, lv_draw_label_dsc_t * draw_dsc, const char * txt);
void lv_canvas_draw_img(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y, const void * src, const lv_draw_img_dsc_t * draw_dsc);
void lv_canvas_draw_line(lv_obj_t * canvas, const lv_point_t points[], uint32_t point_cnt, const lv_draw_line_dsc_t * draw_dsc);
void lv_canvas_draw_polygon(lv_obj_t * canvas, const lv_point_t points[], uint32_t point_cnt, const lv_draw_rect_dsc_t * draw_dsc);
void lv_canvas_draw_arc(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y, lv_coord_t r, int32_t start_angle, int32_t end_angle, const lv_draw_arc_dsc_t * draw_dsc);
```

### Checkbox (`lv_checkbox`)

```c
lv_obj_t * lv_checkbox_create(lv_obj_t * parent);

void lv_checkbox_set_text(lv_obj_t * cb, const char * txt);
void lv_checkbox_set_text_static(lv_obj_t * cb, const char * txt);
const char * lv_checkbox_get_text(const lv_obj_t * cb);
// Check state: use lv_obj_add_state(cb, LV_STATE_CHECKED) / lv_obj_has_state(cb, LV_STATE_CHECKED)
```

### Dropdown (`lv_dropdown`)

```c
lv_obj_t * lv_dropdown_create(lv_obj_t * parent);

void lv_dropdown_set_text(lv_obj_t * dd, const char * txt);
void lv_dropdown_set_options(lv_obj_t * dd, const char * options);   // "\n" separated
void lv_dropdown_set_options_static(lv_obj_t * dd, const char * options);
void lv_dropdown_add_option(lv_obj_t * dd, const char * option, uint32_t pos);
void lv_dropdown_clear_options(lv_obj_t * dd);
void lv_dropdown_set_selected(lv_obj_t * dd, uint16_t sel_opt);
void lv_dropdown_set_dir(lv_obj_t * dd, lv_dir_t dir);
void lv_dropdown_set_symbol(lv_obj_t * dd, const void * symbol);
void lv_dropdown_set_selected_highlight(lv_obj_t * dd, bool en);

lv_obj_t * lv_dropdown_get_list(lv_obj_t * dd);
const char * lv_dropdown_get_text(lv_obj_t * dd);
const char * lv_dropdown_get_options(const lv_obj_t * dd);
uint16_t lv_dropdown_get_selected(const lv_obj_t * dd);
uint16_t lv_dropdown_get_option_cnt(const lv_obj_t * dd);
void lv_dropdown_get_selected_str(const lv_obj_t * dd, char * buf, uint32_t buf_size);
int32_t lv_dropdown_get_option_index(lv_obj_t * dd, const char * option);
lv_dir_t lv_dropdown_get_dir(const lv_obj_t * dd);
bool lv_dropdown_get_selected_highlight(lv_obj_t * dd);

void lv_dropdown_open(lv_obj_t * dd);
void lv_dropdown_close(lv_obj_t * dd);
bool lv_dropdown_is_open(lv_obj_t * dd);
```

### Image (`lv_img`)

```c
lv_obj_t * lv_img_create(lv_obj_t * parent);

void lv_img_set_src(lv_obj_t * img, const void * src);   // File path, symbol, or lv_img_dsc_t *
void lv_img_set_offset_x(lv_obj_t * img, lv_coord_t x);
void lv_img_set_offset_y(lv_obj_t * img, lv_coord_t y);
void lv_img_set_angle(lv_obj_t * img, int16_t angle);    // 0.1 degree units
void lv_img_set_pivot(lv_obj_t * img, lv_coord_t x, lv_coord_t y);
void lv_img_set_zoom(lv_obj_t * img, uint16_t zoom);     // 256 = 100%
void lv_img_set_antialias(lv_obj_t * img, bool antialias);
void lv_img_set_size_mode(lv_obj_t * img, lv_img_size_mode_t mode);

const void * lv_img_get_src(lv_obj_t * img);
lv_coord_t lv_img_get_offset_x(lv_obj_t * img);
lv_coord_t lv_img_get_offset_y(lv_obj_t * img);
uint16_t lv_img_get_angle(lv_obj_t * img);
void lv_img_get_pivot(lv_obj_t * img, lv_point_t * pivot);
uint16_t lv_img_get_zoom(lv_obj_t * img);
bool lv_img_get_antialias(lv_obj_t * img);
lv_img_size_mode_t lv_img_get_size_mode(lv_obj_t * img);
```

### Label (`lv_label`)

```c
lv_obj_t * lv_label_create(lv_obj_t * parent);

void lv_label_set_text(lv_obj_t * label, const char * text);
void lv_label_set_text_fmt(lv_obj_t * label, const char * fmt, ...);
void lv_label_set_text_static(lv_obj_t * label, const char * text);
void lv_label_set_long_mode(lv_obj_t * label, lv_label_long_mode_t long_mode);
void lv_label_set_recolor(lv_obj_t * label, bool en);
void lv_label_set_text_sel_start(lv_obj_t * label, uint32_t index);
void lv_label_set_text_sel_end(lv_obj_t * label, uint32_t index);

char * lv_label_get_text(const lv_obj_t * label);
lv_label_long_mode_t lv_label_get_long_mode(const lv_obj_t * label);
bool lv_label_get_recolor(const lv_obj_t * label);
void lv_label_get_letter_pos(const lv_obj_t * label, uint32_t char_id, lv_point_t * pos);
uint32_t lv_label_get_letter_on(const lv_obj_t * label, lv_point_t * pos_in);
bool lv_label_is_char_under_pos(const lv_obj_t * label, lv_point_t * pos);
uint32_t lv_label_get_text_selection_start(const lv_obj_t * label);
uint32_t lv_label_get_text_selection_end(const lv_obj_t * label);

void lv_label_ins_text(lv_obj_t * label, uint32_t pos, const char * txt);
void lv_label_cut_text(lv_obj_t * label, uint32_t pos, uint32_t cnt);
```

**Long Modes**: `LV_LABEL_LONG_WRAP`, `LV_LABEL_LONG_DOT`, `LV_LABEL_LONG_SCROLL`, `LV_LABEL_LONG_SCROLL_CIRCULAR`, `LV_LABEL_LONG_CLIP`

### Line (`lv_line`)

```c
lv_obj_t * lv_line_create(lv_obj_t * parent);

void lv_line_set_points(lv_obj_t * line, const lv_point_t points[], uint16_t point_num);
void lv_line_set_y_invert(lv_obj_t * line, bool en);
bool lv_line_get_y_invert(const lv_obj_t * line);
```

### Roller (`lv_roller`)

```c
lv_obj_t * lv_roller_create(lv_obj_t * parent);

void lv_roller_set_options(lv_obj_t * roller, const char * options, lv_roller_mode_t mode);
void lv_roller_set_selected(lv_obj_t * roller, uint16_t sel_opt, lv_anim_enable_t anim);
void lv_roller_set_visible_row_count(lv_obj_t * roller, uint8_t row_cnt);

uint16_t lv_roller_get_selected(const lv_obj_t * roller);
void lv_roller_get_selected_str(const lv_obj_t * roller, char * buf, uint32_t buf_size);
const char * lv_roller_get_options(const lv_obj_t * roller);
uint16_t lv_roller_get_option_cnt(const lv_obj_t * roller);
```

**Roller Modes**: `LV_ROLLER_MODE_NORMAL`, `LV_ROLLER_MODE_INFINITE`

### Slider (`lv_slider`)

```c
lv_obj_t * lv_slider_create(lv_obj_t * parent);

// Inherits bar API
void lv_slider_set_value(lv_obj_t * slider, int32_t value, lv_anim_enable_t anim);
void lv_slider_set_left_value(lv_obj_t * slider, int32_t value, lv_anim_enable_t anim);
void lv_slider_set_range(lv_obj_t * slider, int32_t min, int32_t max);
void lv_slider_set_mode(lv_obj_t * slider, lv_slider_mode_t mode);

int32_t lv_slider_get_value(const lv_obj_t * slider);
int32_t lv_slider_get_left_value(const lv_obj_t * slider);
int32_t lv_slider_get_min_value(const lv_obj_t * slider);
int32_t lv_slider_get_max_value(const lv_obj_t * slider);
bool lv_slider_is_dragged(const lv_obj_t * slider);
lv_slider_mode_t lv_slider_get_mode(lv_obj_t * slider);
```

### Switch (`lv_switch`)

```c
lv_obj_t * lv_switch_create(lv_obj_t * parent);
// State: use lv_obj_add_state(sw, LV_STATE_CHECKED) / lv_obj_clear_state(sw, LV_STATE_CHECKED)
// Event: LV_EVENT_VALUE_CHANGED
```

### Table (`lv_table`)

```c
lv_obj_t * lv_table_create(lv_obj_t * parent);

void lv_table_set_cell_value(lv_obj_t * table, uint16_t row, uint16_t col, const char * txt);
void lv_table_set_cell_value_fmt(lv_obj_t * table, uint16_t row, uint16_t col, const char * fmt, ...);
void lv_table_set_row_cnt(lv_obj_t * table, uint16_t row_cnt);
void lv_table_set_col_cnt(lv_obj_t * table, uint16_t col_cnt);
void lv_table_set_col_width(lv_obj_t * table, uint16_t col_id, lv_coord_t w);
void lv_table_add_cell_ctrl(lv_obj_t * table, uint16_t row, uint16_t col, lv_table_cell_ctrl_t ctrl);
void lv_table_clear_cell_ctrl(lv_obj_t * table, uint16_t row, uint16_t col, lv_table_cell_ctrl_t ctrl);

const char * lv_table_get_cell_value(lv_obj_t * table, uint16_t row, uint16_t col);
uint16_t lv_table_get_row_cnt(lv_obj_t * table);
uint16_t lv_table_get_col_cnt(lv_obj_t * table);
lv_coord_t lv_table_get_col_width(lv_obj_t * table, uint16_t col);
bool lv_table_has_cell_ctrl(lv_obj_t * table, uint16_t row, uint16_t col, lv_table_cell_ctrl_t ctrl);
void lv_table_get_selected_cell(lv_obj_t * table, uint16_t * row, uint16_t * col);
```

**Table Cell Controls**: `LV_TABLE_CELL_CTRL_MERGE_RIGHT`, `LV_TABLE_CELL_CTRL_TEXT_CROP`, `LV_TABLE_CELL_CTRL_CUSTOM_1` -- `LV_TABLE_CELL_CTRL_CUSTOM_4`

### Text Area (`lv_textarea`)

```c
lv_obj_t * lv_textarea_create(lv_obj_t * parent);

void lv_textarea_add_char(lv_obj_t * ta, uint32_t c);
void lv_textarea_add_text(lv_obj_t * ta, const char * txt);
void lv_textarea_del_char(lv_obj_t * ta);
void lv_textarea_del_char_forward(lv_obj_t * ta);

void lv_textarea_set_text(lv_obj_t * ta, const char * txt);
void lv_textarea_set_placeholder_text(lv_obj_t * ta, const char * txt);
void lv_textarea_set_cursor_pos(lv_obj_t * ta, int32_t pos);  // LV_TEXTAREA_CURSOR_LAST
void lv_textarea_set_cursor_click_pos(lv_obj_t * ta, bool en);
void lv_textarea_set_password_mode(lv_obj_t * ta, bool en);
void lv_textarea_set_one_line(lv_obj_t * ta, bool en);
void lv_textarea_set_accepted_chars(lv_obj_t * ta, const char * list);
void lv_textarea_set_max_length(lv_obj_t * ta, uint32_t num);
void lv_textarea_set_insert_replace(lv_obj_t * ta, const char * txt);
void lv_textarea_set_text_selection(lv_obj_t * ta, bool en);
void lv_textarea_set_password_show_time(lv_obj_t * ta, uint16_t time);
void lv_textarea_set_align(lv_obj_t * ta, lv_text_align_t align);

const char * lv_textarea_get_text(const lv_obj_t * ta);
const char * lv_textarea_get_placeholder_text(lv_obj_t * ta);
lv_obj_t * lv_textarea_get_label(const lv_obj_t * ta);
uint32_t lv_textarea_get_cursor_pos(const lv_obj_t * ta);
bool lv_textarea_get_cursor_click_pos(lv_obj_t * ta);
bool lv_textarea_get_password_mode(const lv_obj_t * ta);
bool lv_textarea_get_one_line(const lv_obj_t * ta);
const char * lv_textarea_get_accepted_chars(lv_obj_t * ta);
uint32_t lv_textarea_get_max_length(lv_obj_t * ta);
bool lv_textarea_text_is_selected(const lv_obj_t * ta);
bool lv_textarea_get_text_selection(lv_obj_t * ta);
uint16_t lv_textarea_get_password_show_time(lv_obj_t * ta);

void lv_textarea_clear_selection(lv_obj_t * ta);
void lv_textarea_cursor_right(lv_obj_t * ta);
void lv_textarea_cursor_left(lv_obj_t * ta);
void lv_textarea_cursor_down(lv_obj_t * ta);
void lv_textarea_cursor_up(lv_obj_t * ta);
```

---

## Extra Widgets

### Animation Image (`lv_animimg`)

```c
lv_obj_t * lv_animimg_create(lv_obj_t * parent);

void lv_animimg_set_src(lv_obj_t * img, const void * dsc[], uint8_t num);
void lv_animimg_start(lv_obj_t * img);
void lv_animimg_set_duration(lv_obj_t * img, uint32_t duration);
void lv_animimg_set_repeat_count(lv_obj_t * img, uint16_t count);
```

### Calendar (`lv_calendar`)

```c
lv_obj_t * lv_calendar_create(lv_obj_t * parent);

void lv_calendar_set_today_date(lv_obj_t * calendar, uint32_t year, uint32_t month, uint32_t day);
void lv_calendar_set_showed_date(lv_obj_t * calendar, uint32_t year, uint32_t month);
void lv_calendar_set_highlighted_dates(lv_obj_t * calendar, lv_calendar_date_t highlighted[], uint16_t date_num);
void lv_calendar_set_day_names(const char ** day_names);

const lv_calendar_date_t * lv_calendar_get_today_date(const lv_obj_t * calendar);
const lv_calendar_date_t * lv_calendar_get_showed_date(const lv_obj_t * calendar);
lv_calendar_date_t * lv_calendar_get_highlighted_dates(const lv_obj_t * calendar);
uint16_t lv_calendar_get_highlighted_dates_num(const lv_obj_t * calendar);
lv_res_t lv_calendar_get_pressed_date(const lv_obj_t * calendar, lv_calendar_date_t * date);

// Header (v8.4: supports custom year list)
lv_obj_t * lv_calendar_header_arrow_create(lv_obj_t * parent);
lv_obj_t * lv_calendar_header_dropdown_create(lv_obj_t * parent);  // v8.4: custom year list support
```

### Chart (`lv_chart`)

```c
lv_obj_t * lv_chart_create(lv_obj_t * parent);

void lv_chart_set_type(lv_obj_t * chart, lv_chart_type_t type);
void lv_chart_set_point_count(lv_obj_t * chart, uint16_t cnt);
void lv_chart_set_range(lv_obj_t * chart, lv_chart_axis_t axis, lv_coord_t min, lv_coord_t max);
void lv_chart_set_update_mode(lv_obj_t * chart, lv_chart_update_mode_t update_mode);
void lv_chart_set_div_line_count(lv_obj_t * chart, uint8_t hdiv, uint8_t vdiv);
void lv_chart_set_zoom_x(lv_obj_t * chart, uint16_t zoom_x);
void lv_chart_set_zoom_y(lv_obj_t * chart, uint16_t zoom_y);
void lv_chart_set_axis_tick(lv_obj_t * chart, lv_chart_axis_t axis, lv_coord_t major_len, lv_coord_t minor_len, lv_coord_t major_cnt, lv_coord_t minor_cnt, bool label_en);

// Series management
lv_chart_series_t * lv_chart_add_series(lv_obj_t * chart, lv_color_t color, lv_chart_axis_t axis);
void lv_chart_remove_series(lv_obj_t * chart, lv_chart_series_t * series);  // v8.4: fixed memory leak
void lv_chart_hide_series(lv_obj_t * chart, lv_chart_series_t * series, bool hide);
void lv_chart_set_series_color(lv_obj_t * chart, lv_chart_series_t * series, lv_color_t color);
void lv_chart_set_x_start_point(lv_obj_t * chart, lv_chart_series_t * ser, uint16_t id);

// Data
void lv_chart_set_next_value(lv_obj_t * chart, lv_chart_series_t * ser, lv_coord_t value);
void lv_chart_set_next_value2(lv_obj_t * chart, lv_chart_series_t * ser, lv_coord_t x_value, lv_coord_t y_value);
void lv_chart_set_value_by_id(lv_obj_t * chart, lv_chart_series_t * ser, uint16_t id, lv_coord_t value);
void lv_chart_set_value_by_id2(lv_obj_t * chart, lv_chart_series_t * ser, uint16_t id, lv_coord_t x_value, lv_coord_t y_value);
void lv_chart_set_ext_y_array(lv_obj_t * chart, lv_chart_series_t * ser, lv_coord_t array[]);
void lv_chart_set_ext_x_array(lv_obj_t * chart, lv_chart_series_t * ser, lv_coord_t array[]);

// Cursor
lv_chart_cursor_t * lv_chart_add_cursor(lv_obj_t * chart, lv_color_t color, lv_dir_t dir);
void lv_chart_set_cursor_pos(lv_obj_t * chart, lv_chart_cursor_t * cursor, lv_point_t * pos);
void lv_chart_set_cursor_point(lv_obj_t * chart, lv_chart_cursor_t * cursor, lv_chart_series_t * ser, uint16_t point_id);
lv_point_t lv_chart_get_cursor_point(lv_obj_t * chart, lv_chart_cursor_t * cursor);

// Getters
lv_chart_type_t lv_chart_get_type(const lv_obj_t * chart);
uint16_t lv_chart_get_point_count(const lv_obj_t * chart);
uint16_t lv_chart_get_x_start_point(const lv_obj_t * chart, lv_chart_series_t * ser);
void lv_chart_get_point_pos_by_id(lv_obj_t * chart, lv_chart_series_t * ser, uint16_t id, lv_point_t * p_out);
lv_coord_t * lv_chart_get_y_array(const lv_obj_t * chart, lv_chart_series_t * ser);
lv_coord_t * lv_chart_get_x_array(const lv_obj_t * chart, lv_chart_series_t * ser);
uint32_t lv_chart_get_pressed_point(const lv_obj_t * chart);

void lv_chart_refresh(lv_obj_t * chart);
```

**Chart Types**: `LV_CHART_TYPE_NONE`, `LV_CHART_TYPE_LINE`, `LV_CHART_TYPE_BAR`, `LV_CHART_TYPE_SCATTER`

### Color Wheel (`lv_colorwheel`)

```c
lv_obj_t * lv_colorwheel_create(lv_obj_t * parent, bool knob_recolor);

bool lv_colorwheel_set_hsv(lv_obj_t * colorwheel, lv_color_hsv_t hsv);
bool lv_colorwheel_set_rgb(lv_obj_t * colorwheel, lv_color_t color);
void lv_colorwheel_set_mode(lv_obj_t * colorwheel, lv_colorwheel_mode_t mode);
void lv_colorwheel_set_mode_fixed(lv_obj_t * colorwheel, bool fixed);

lv_color_hsv_t lv_colorwheel_get_hsv(lv_obj_t * colorwheel);
lv_color_t lv_colorwheel_get_rgb(lv_obj_t * colorwheel);
lv_colorwheel_mode_t lv_colorwheel_get_color_mode(lv_obj_t * colorwheel);
bool lv_colorwheel_get_color_mode_fixed(lv_obj_t * colorwheel);
```

> Note: `lv_colorwheel` is **removed** in v9. This is the final API.

### Image Button (`lv_imgbtn`)

```c
lv_obj_t * lv_imgbtn_create(lv_obj_t * parent);

void lv_imgbtn_set_src(lv_obj_t * imgbtn, lv_imgbtn_state_t state, const void * src_left, const void * src_mid, const void * src_right);
const void * lv_imgbtn_get_src_left(lv_obj_t * imgbtn, lv_imgbtn_state_t state);
const void * lv_imgbtn_get_src_middle(lv_obj_t * imgbtn, lv_imgbtn_state_t state);
const void * lv_imgbtn_get_src_right(lv_obj_t * imgbtn, lv_imgbtn_state_t state);
```

**Image Button States**: `LV_IMGBTN_STATE_RELEASED`, `LV_IMGBTN_STATE_PRESSED`, `LV_IMGBTN_STATE_DISABLED`, `LV_IMGBTN_STATE_CHECKED_RELEASED`, `LV_IMGBTN_STATE_CHECKED_PRESSED`, `LV_IMGBTN_STATE_CHECKED_DISABLED`

### Keyboard (`lv_keyboard`)

```c
lv_obj_t * lv_keyboard_create(lv_obj_t * parent);

void lv_keyboard_set_textarea(lv_obj_t * kb, lv_obj_t * ta);
void lv_keyboard_set_mode(lv_obj_t * kb, lv_keyboard_mode_t mode);
void lv_keyboard_set_map(lv_obj_t * kb, lv_keyboard_mode_t mode, const char * map[], const lv_btnmatrix_ctrl_t ctrl_map[]);
void lv_keyboard_set_popovers(lv_obj_t * kb, bool en);

lv_obj_t * lv_keyboard_get_textarea(const lv_obj_t * kb);
lv_keyboard_mode_t lv_keyboard_get_mode(const lv_obj_t * kb);
```

**Keyboard Modes**: `LV_KEYBOARD_MODE_TEXT_LOWER`, `LV_KEYBOARD_MODE_TEXT_UPPER`, `LV_KEYBOARD_MODE_SPECIAL`, `LV_KEYBOARD_MODE_NUMBER`, `LV_KEYBOARD_MODE_USER_1` -- `LV_KEYBOARD_MODE_USER_4`

### LED (`lv_led`)

```c
lv_obj_t * lv_led_create(lv_obj_t * parent);

void lv_led_set_color(lv_obj_t * led, lv_color_t color);
void lv_led_set_brightness(lv_obj_t * led, uint8_t bright);  // 0-255
void lv_led_on(lv_obj_t * led);
void lv_led_off(lv_obj_t * led);
void lv_led_toggle(lv_obj_t * led);
uint8_t lv_led_get_brightness(const lv_obj_t * led);
```

### List (`lv_list`)

```c
lv_obj_t * lv_list_create(lv_obj_t * parent);

lv_obj_t * lv_list_add_text(lv_obj_t * list, const char * txt);
lv_obj_t * lv_list_add_btn(lv_obj_t * list, const void * icon, const char * txt);
const char * lv_list_get_btn_text(lv_obj_t * list, lv_obj_t * btn);
```

### Menu (`lv_menu`)

```c
lv_obj_t * lv_menu_create(lv_obj_t * parent);

lv_obj_t * lv_menu_page_create(lv_obj_t * menu, char * title);
lv_obj_t * lv_menu_cont_create(lv_obj_t * parent);
lv_obj_t * lv_menu_section_create(lv_obj_t * parent);
lv_obj_t * lv_menu_separator_create(lv_obj_t * parent);

void lv_menu_set_page(lv_obj_t * menu, lv_obj_t * page);
void lv_menu_set_sidebar_page(lv_obj_t * menu, lv_obj_t * page);
void lv_menu_set_mode_header(lv_obj_t * menu, lv_menu_mode_header_t mode);
void lv_menu_set_mode_root_back_btn(lv_obj_t * menu, lv_menu_mode_root_back_btn_t mode);
void lv_menu_set_load_page_event(lv_obj_t * menu, lv_obj_t * obj, lv_obj_t * page);
void lv_menu_clear_history(lv_obj_t * menu);

lv_obj_t * lv_menu_get_cur_main_page(lv_obj_t * menu);
lv_obj_t * lv_menu_get_cur_sidebar_page(lv_obj_t * menu);
lv_obj_t * lv_menu_get_main_header(lv_obj_t * menu);
lv_obj_t * lv_menu_get_main_header_back_btn(lv_obj_t * menu);
lv_obj_t * lv_menu_get_sidebar_header(lv_obj_t * menu);
lv_obj_t * lv_menu_get_sidebar_header_back_btn(lv_obj_t * menu);
bool lv_menu_back_btn_is_root(lv_obj_t * menu, lv_obj_t * obj);
```

### Meter (`lv_meter`)

```c
lv_obj_t * lv_meter_create(lv_obj_t * parent);

// Scale
lv_meter_scale_t * lv_meter_add_scale(lv_obj_t * meter);
void lv_meter_set_scale_ticks(lv_obj_t * meter, lv_meter_scale_t * scale, uint16_t cnt, uint16_t width, uint16_t len, lv_color_t color);
void lv_meter_set_scale_major_ticks(lv_obj_t * meter, lv_meter_scale_t * scale, uint16_t nth, uint16_t width, uint16_t len, lv_color_t color, int16_t label_gap);
void lv_meter_set_scale_range(lv_obj_t * meter, lv_meter_scale_t * scale, int32_t min, int32_t max, uint32_t angle_range, uint32_t rotation);

// Indicators
lv_meter_indicator_t * lv_meter_add_needle_line(lv_obj_t * meter, lv_meter_scale_t * scale, uint16_t width, lv_color_t color, int16_t r_mod);
lv_meter_indicator_t * lv_meter_add_needle_img(lv_obj_t * meter, lv_meter_scale_t * scale, const void * src, lv_coord_t pivot_x, lv_coord_t pivot_y);
lv_meter_indicator_t * lv_meter_add_arc(lv_obj_t * meter, lv_meter_scale_t * scale, uint16_t width, lv_color_t color, int16_t r_mod);
lv_meter_indicator_t * lv_meter_add_scale_lines(lv_obj_t * meter, lv_meter_scale_t * scale, lv_color_t color_start, lv_color_t color_end, bool local, int16_t width_mod);

void lv_meter_set_indicator_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value);
void lv_meter_set_indicator_start_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value);
void lv_meter_set_indicator_end_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value);
```

> Note: `lv_meter` is **removed** in v9. Use `lv_scale` + `lv_arc` instead.

### Message Box (`lv_msgbox`)

```c
lv_obj_t * lv_msgbox_create(lv_obj_t * parent, const char * title, const char * txt, const char * btn_txts[], bool add_close_btn);
void lv_msgbox_close(lv_obj_t * msgbox);

lv_obj_t * lv_msgbox_get_title(lv_obj_t * msgbox);
lv_obj_t * lv_msgbox_get_close_btn(lv_obj_t * msgbox);
lv_obj_t * lv_msgbox_get_text(lv_obj_t * msgbox);
lv_obj_t * lv_msgbox_get_content(lv_obj_t * msgbox);
lv_obj_t * lv_msgbox_get_btns(lv_obj_t * msgbox);
uint16_t lv_msgbox_get_active_btn(lv_obj_t * msgbox);
const char * lv_msgbox_get_active_btn_text(lv_obj_t * msgbox);
```

### Span Group (`lv_spangroup` / `lv_span`)

```c
lv_obj_t * lv_spangroup_create(lv_obj_t * parent);

lv_span_t * lv_spangroup_new_span(lv_obj_t * spangroup);
void lv_spangroup_del_span(lv_obj_t * spangroup, lv_span_t * span);
void lv_span_set_text(lv_span_t * span, const char * text);
void lv_span_set_text_static(lv_span_t * span, const char * text);

void lv_spangroup_set_align(lv_obj_t * spangroup, lv_text_align_t align);
void lv_spangroup_set_overflow(lv_obj_t * spangroup, lv_span_overflow_t overflow);
void lv_spangroup_set_indent(lv_obj_t * spangroup, lv_coord_t indent);
void lv_spangroup_set_mode(lv_obj_t * spangroup, lv_span_mode_t mode);

lv_span_t * lv_spangroup_get_child(const lv_obj_t * spangroup, int32_t id);
uint32_t lv_spangroup_get_child_cnt(const lv_obj_t * spangroup);
lv_text_align_t lv_spangroup_get_align(lv_obj_t * spangroup);
lv_span_overflow_t lv_spangroup_get_overflow(lv_obj_t * spangroup);
lv_coord_t lv_spangroup_get_indent(lv_obj_t * spangroup);
lv_span_mode_t lv_spangroup_get_mode(lv_obj_t * spangroup);
lv_coord_t lv_spangroup_get_max_line_h(lv_obj_t * spangroup);
uint32_t lv_spangroup_get_expand_width(lv_obj_t * spangroup, uint32_t max_width);
void lv_spangroup_refr_mode(lv_obj_t * spangroup);
```

### Spinbox (`lv_spinbox`)

```c
lv_obj_t * lv_spinbox_create(lv_obj_t * parent);

void lv_spinbox_set_value(lv_obj_t * spinbox, int32_t i);
void lv_spinbox_set_rollover(lv_obj_t * spinbox, bool b);
void lv_spinbox_set_digit_format(lv_obj_t * spinbox, uint8_t digit_count, uint8_t separator_position);
void lv_spinbox_set_step(lv_obj_t * spinbox, uint32_t step);
void lv_spinbox_set_range(lv_obj_t * spinbox, int32_t range_min, int32_t range_max);
void lv_spinbox_set_cursor_pos(lv_obj_t * spinbox, uint8_t pos);
void lv_spinbox_set_digit_step_direction(lv_obj_t * spinbox, lv_dir_t direction);

int32_t lv_spinbox_get_value(lv_obj_t * spinbox);
int32_t lv_spinbox_get_step(lv_obj_t * spinbox);
bool lv_spinbox_get_rollover(lv_obj_t * spinbox);

void lv_spinbox_step_next(lv_obj_t * spinbox);
void lv_spinbox_step_prev(lv_obj_t * spinbox);
void lv_spinbox_increment(lv_obj_t * spinbox);
void lv_spinbox_decrement(lv_obj_t * spinbox);
```

### Spinner (`lv_spinner`)

```c
lv_obj_t * lv_spinner_create(lv_obj_t * parent, uint32_t time, uint32_t arc_length);
// No additional functions -- configuration via style on LV_PART_INDICATOR
```

### Tabview (`lv_tabview`)

```c
lv_obj_t * lv_tabview_create(lv_obj_t * parent, lv_dir_t tab_pos, lv_coord_t tab_size);

lv_obj_t * lv_tabview_add_tab(lv_obj_t * tabview, const char * name);
void lv_tabview_rename_tab(lv_obj_t * tabview, uint32_t tab_id, const char * new_name);
lv_obj_t * lv_tabview_get_content(lv_obj_t * tabview);
lv_obj_t * lv_tabview_get_tab_btns(lv_obj_t * tabview);
void lv_tabview_set_act(lv_obj_t * tabview, uint32_t id, lv_anim_enable_t anim_en);
uint16_t lv_tabview_get_tab_act(lv_obj_t * tabview);
```

### Tile View (`lv_tileview`)

```c
lv_obj_t * lv_tileview_create(lv_obj_t * parent);

lv_obj_t * lv_tileview_add_tile(lv_obj_t * tv, uint8_t col_id, uint8_t row_id, lv_dir_t dir);
void lv_obj_set_tile(lv_obj_t * tv, lv_obj_t * tile_obj, lv_anim_enable_t anim_en);
void lv_obj_set_tile_id(lv_obj_t * tv, uint32_t col_id, uint32_t row_id, lv_anim_enable_t anim_en);
lv_obj_t * lv_tileview_get_tile_act(lv_obj_t * tv);
```

### Window (`lv_win`)

```c
lv_obj_t * lv_win_create(lv_obj_t * parent, lv_coord_t header_height);

lv_obj_t * lv_win_add_title(lv_obj_t * win, const char * txt);
lv_obj_t * lv_win_add_btn(lv_obj_t * win, const void * icon, lv_coord_t btn_w);
lv_obj_t * lv_win_get_header(lv_obj_t * win);
lv_obj_t * lv_win_get_content(lv_obj_t * win);
```

---

## Theme API

```c
// Apply theme to display
void lv_disp_set_theme(lv_disp_t * disp, lv_theme_t * theme);
lv_theme_t * lv_disp_get_theme(lv_disp_t * disp);

// Default theme
lv_theme_t * lv_theme_default_init(
    lv_disp_t * disp,
    lv_color_t color_primary,
    lv_color_t color_secondary,
    bool dark,
    const lv_font_t * font
);
bool lv_theme_default_is_inited(void);

// Mono theme (for monochrome displays)
lv_theme_t * lv_theme_mono_init(
    lv_disp_t * disp,
    bool dark_bg,
    const lv_font_t * font
);
bool lv_theme_mono_is_inited(void);

// Basic theme (minimal, low resource)
lv_theme_t * lv_theme_basic_init(lv_disp_t * disp);
bool lv_theme_basic_is_inited(void);
```

---

## GPU Backends

### Configuration Macros

| Macro | Backend | Description |
|-------|---------|-------------|
| `LV_USE_GPU_STM32_DMA2D` | STM32 Chrom-ART | DMA2D blending/fill |
| `LV_USE_GPU_NXP_PXP` | NXP PXP | Pixel Pipeline (bare-metal + Zephyr in v8.4) |
| `LV_USE_GPU_NXP_VG_LITE` | NXP VG-Lite | Vector graphics GPU |
| `LV_USE_GPU_ARM2D` | ARM-2D | Helium/MVE acceleration |
| `LV_USE_GPU_SDL` | SDL2 | Desktop GPU rendering |
| `LV_USE_GPU_SWM341_DMA2D` | Synwit SWM341 | DMA2D acceleration |

---

## Memory API

```c
void lv_mem_init(void);
void lv_mem_deinit(void);
void * lv_mem_alloc(size_t size);
void * lv_mem_realloc(void * data_p, size_t new_size);
void lv_mem_free(void * data);
lv_res_t lv_mem_test(void);
void lv_mem_monitor(lv_mem_monitor_t * mon);

// Monitor struct fields
// mon->total_size     - Total heap size
// mon->used_cnt       - Number of allocations
// mon->free_cnt       - Number of free blocks
// mon->free_size      - Total free bytes
// mon->free_biggest_size - Largest contiguous free block
// mon->used_pct       - Used percentage
// mon->frag_pct       - Fragmentation percentage
```

---

## Logging API

```c
// Enabled via LV_USE_LOG in lv_conf.h
void lv_log_register_print_cb(lv_log_print_g_cb_t print_cb);

// Log macros
LV_LOG_TRACE(msg)    // Detailed trace (LV_LOG_LEVEL_TRACE)
LV_LOG_INFO(msg)     // Informational (LV_LOG_LEVEL_INFO)
LV_LOG_WARN(msg)     // Warnings (LV_LOG_LEVEL_WARN)
LV_LOG_ERROR(msg)    // Errors (LV_LOG_LEVEL_ERROR)
LV_LOG_USER(msg)     // User messages (LV_LOG_LEVEL_USER)

// Log levels (set via LV_LOG_LEVEL in lv_conf.h)
// LV_LOG_LEVEL_TRACE  0
// LV_LOG_LEVEL_INFO   1
// LV_LOG_LEVEL_WARN   2
// LV_LOG_LEVEL_ERROR  3
// LV_LOG_LEVEL_USER   4
// LV_LOG_LEVEL_NONE   5
```

---

## Common Symbols (Built-in Icons)

```c
LV_SYMBOL_AUDIO          LV_SYMBOL_VIDEO
LV_SYMBOL_LIST           LV_SYMBOL_OK
LV_SYMBOL_CLOSE          LV_SYMBOL_POWER
LV_SYMBOL_SETTINGS       LV_SYMBOL_HOME
LV_SYMBOL_DOWNLOAD       LV_SYMBOL_DRIVE
LV_SYMBOL_REFRESH        LV_SYMBOL_MUTE
LV_SYMBOL_VOLUME_MID     LV_SYMBOL_VOLUME_MAX
LV_SYMBOL_IMAGE          LV_SYMBOL_TINT
LV_SYMBOL_PREV           LV_SYMBOL_PLAY
LV_SYMBOL_PAUSE          LV_SYMBOL_STOP
LV_SYMBOL_NEXT           LV_SYMBOL_EJECT
LV_SYMBOL_LEFT           LV_SYMBOL_RIGHT
LV_SYMBOL_PLUS           LV_SYMBOL_MINUS
LV_SYMBOL_EYE_OPEN       LV_SYMBOL_EYE_CLOSE
LV_SYMBOL_WARNING        LV_SYMBOL_SHUFFLE
LV_SYMBOL_UP             LV_SYMBOL_DOWN
LV_SYMBOL_LOOP           LV_SYMBOL_DIRECTORY
LV_SYMBOL_UPLOAD         LV_SYMBOL_CALL
LV_SYMBOL_CUT            LV_SYMBOL_COPY
LV_SYMBOL_SAVE           LV_SYMBOL_BARS
LV_SYMBOL_ENVELOPE       LV_SYMBOL_CHARGE
LV_SYMBOL_PASTE          LV_SYMBOL_BELL
LV_SYMBOL_KEYBOARD       LV_SYMBOL_GPS
LV_SYMBOL_FILE           LV_SYMBOL_WIFI
LV_SYMBOL_BATTERY_FULL   LV_SYMBOL_BATTERY_3
LV_SYMBOL_BATTERY_2      LV_SYMBOL_BATTERY_1
LV_SYMBOL_BATTERY_EMPTY  LV_SYMBOL_USB
LV_SYMBOL_BLUETOOTH      LV_SYMBOL_TRASH
LV_SYMBOL_EDIT           LV_SYMBOL_BACKSPACE
LV_SYMBOL_SD_CARD        LV_SYMBOL_NEW_LINE
LV_SYMBOL_DUMMY          LV_SYMBOL_BULLET
```

---

## Direction Constants

```c
LV_DIR_NONE
LV_DIR_LEFT
LV_DIR_RIGHT
LV_DIR_TOP
LV_DIR_BOTTOM
LV_DIR_HOR       // LEFT | RIGHT
LV_DIR_VER       // TOP | BOTTOM
LV_DIR_ALL       // LEFT | RIGHT | TOP | BOTTOM
```

---

## Key Constants (for Keypad Input Devices)

```c
LV_KEY_UP
LV_KEY_DOWN
LV_KEY_RIGHT
LV_KEY_LEFT
LV_KEY_ESC
LV_KEY_DEL
LV_KEY_BACKSPACE
LV_KEY_ENTER
LV_KEY_NEXT
LV_KEY_PREV
LV_KEY_HOME
LV_KEY_END
```

---

## Configuration Summary (`lv_conf.h` Key Macros)

| Macro | Default | Description |
|-------|---------|-------------|
| `LV_COLOR_DEPTH` | 16 | Color depth: 1, 8, 16, 32 |
| `LV_COLOR_16_SWAP` | 0 | Byte-swap for SPI displays |
| `LV_COLOR_SCREEN_TRANSP` | 0 | Transparent screens |
| `LV_COLOR_MIX_ROUND_OFS` | 128 | Rounding in color mix |
| `LV_COLOR_CHROMA_KEY` | `lv_color_hex(0x00ff00)` | Chroma key color |
| `LV_MEM_CUSTOM` | 0 | Use stdlib malloc (1) or built-in (0) |
| `LV_MEM_SIZE` | `(48U * 1024U)` | Built-in heap size |
| `LV_MEM_ADR` | 0 | Fixed memory address (0 = dynamic) |
| `LV_DISP_DEF_REFR_PERIOD` | 30 | Default refresh period (ms) |
| `LV_INDEV_DEF_READ_PERIOD` | 30 | Default input read period (ms) |
| `LV_TICK_CUSTOM` | 0 | Use custom tick source |
| `LV_DPI_DEF` | 130 | Default DPI |
| `LV_USE_LOG` | 0 | Enable logging |
| `LV_LOG_LEVEL` | `LV_LOG_LEVEL_WARN` | Minimum log level |
| `LV_USE_ASSERT_NULL` | 1 | Assert on NULL pointers |
| `LV_USE_ASSERT_MALLOC` | 1 | Assert on failed alloc |
| `LV_USE_ASSERT_STYLE` | 0 | Assert on style errors |
| `LV_USE_ASSERT_MEM_INTEGRITY` | 0 | Assert on heap corruption |
| `LV_USE_ASSERT_OBJ` | 0 | Assert on invalid objects |
| `LV_FONT_DEFAULT` | `&lv_font_montserrat_14` | Default font |
| `LV_USE_THEME_DEFAULT` | 1 | Enable default theme |
| `LV_USE_THEME_BASIC` | 1 | Enable basic theme |
| `LV_USE_THEME_MONO` | 0 | Enable mono theme |
| `LV_USE_FLEX` | 1 | Enable flex layout |
| `LV_USE_GRID` | 1 | Enable grid layout |
| `LV_USE_SNAPSHOT` | 0 | Enable snapshot feature |
| `LV_USE_MSG` | 0 | Enable messaging |
| `LV_USE_FRAGMENT` | 0 | Enable fragment (experimental) |
| `LV_USE_IMGFONT` | 0 | Enable image-based fonts |
| `LV_USE_IME_PINYIN` | 0 | Enable Pinyin IME |

---

## Source References

- LVGL 8.4 documentation: `https://docs.lvgl.io/8.4/`
- Widget index: `https://docs.lvgl.io/8.4/widgets/index.html`
- Core widgets: `https://docs.lvgl.io/8.4/widgets/core/index.html`
- Style properties: `https://docs.lvgl.io/8.2/overview/style-props.html`
- Events: `https://docs.lvgl.io/8.3/overview/event.html`
- GitHub source: `https://github.com/lvgl/lvgl/tree/v8.4.0`
