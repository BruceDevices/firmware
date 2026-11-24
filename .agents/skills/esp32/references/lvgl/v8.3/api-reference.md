# LVGL v8.3.x API Reference

> Detailed API documentation for new and changed functions in LVGL 8.3.x.
> Organized by module for AI agent consumption.

---

## Table of Contents

1. [Object (lv_obj) Base API](#1-object-lv_obj-base-api)
2. [Display Driver API](#2-display-driver-api)
3. [Input Device Driver API](#3-input-device-driver-api)
4. [Style API](#4-style-api)
5. [Event API](#5-event-api)
6. [Animation API](#6-animation-api)
7. [File System API](#7-file-system-api)
8. [Image Decoder API](#8-image-decoder-api)
9. [Font API](#9-font-api)
10. [Messaging API (New in 8.3)](#10-messaging-api-new-in-83)
11. [Fragment Manager API (New in 8.3)](#11-fragment-manager-api-new-in-83)
12. [GridNav API (New in 8.3)](#12-gridnav-api-new-in-83)
13. [Snapshot API (New in 8.3)](#13-snapshot-api-new-in-83)
14. [IME Pinyin API (New in 8.3)](#14-ime-pinyin-api-new-in-83)
15. [Timer API](#15-timer-api)
16. [Group API](#16-group-api)
17. [Layout API (Flex and Grid)](#17-layout-api-flex-and-grid)
18. [Theme API](#18-theme-api)
19. [Async API](#19-async-api)

---

## 1. Object (lv_obj) Base API

### Creation and Deletion

```c
lv_obj_t * lv_obj_create(lv_obj_t * parent);
void lv_obj_del(lv_obj_t * obj);
void lv_obj_clean(lv_obj_t * obj);                // Delete all children
void lv_obj_del_anim_ready_cb(lv_anim_t * anim);  // Delete after animation
void lv_obj_del_async(lv_obj_t * obj);             // Delete in next cycle
```

### Size

```c
void lv_obj_set_width(lv_obj_t * obj, lv_coord_t w);
void lv_obj_set_height(lv_obj_t * obj, lv_coord_t h);
void lv_obj_set_size(lv_obj_t * obj, lv_coord_t w, lv_coord_t h);
void lv_obj_set_content_width(lv_obj_t * obj, lv_coord_t w);
void lv_obj_set_content_height(lv_obj_t * obj, lv_coord_t h);

lv_coord_t lv_obj_get_width(lv_obj_t * obj);
lv_coord_t lv_obj_get_height(lv_obj_t * obj);
lv_coord_t lv_obj_get_content_width(lv_obj_t * obj);
lv_coord_t lv_obj_get_content_height(lv_obj_t * obj);
lv_coord_t lv_obj_get_self_width(lv_obj_t * obj);
lv_coord_t lv_obj_get_self_height(lv_obj_t * obj);
```

**Special size values:**
| Constant            | Description                              |
|---------------------|------------------------------------------|
| `LV_SIZE_CONTENT`   | Size to fit content                      |
| `LV_PCT(x)`         | Percentage of parent (0-100)             |

### Position and Alignment

```c
void lv_obj_set_x(lv_obj_t * obj, lv_coord_t x);
void lv_obj_set_y(lv_obj_t * obj, lv_coord_t y);
void lv_obj_set_pos(lv_obj_t * obj, lv_coord_t x, lv_coord_t y);

void lv_obj_set_align(lv_obj_t * obj, lv_align_t align);
void lv_obj_align(lv_obj_t * obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
void lv_obj_align_to(lv_obj_t * obj, const lv_obj_t * base, lv_align_t align,
                     lv_coord_t x_ofs, lv_coord_t y_ofs);
void lv_obj_center(lv_obj_t * obj);

lv_coord_t lv_obj_get_x(lv_obj_t * obj);
lv_coord_t lv_obj_get_y(lv_obj_t * obj);
lv_coord_t lv_obj_get_x_aligned(lv_obj_t * obj);
lv_coord_t lv_obj_get_y_aligned(lv_obj_t * obj);
void lv_obj_get_coords(lv_obj_t * obj, lv_area_t * coords);
```

**Alignment constants:**

| Constant                          | Description              |
|-----------------------------------|--------------------------|
| `LV_ALIGN_DEFAULT`                | Top-left (LTR)           |
| `LV_ALIGN_TOP_LEFT`               | Top-left corner          |
| `LV_ALIGN_TOP_MID`                | Top center               |
| `LV_ALIGN_TOP_RIGHT`              | Top-right corner         |
| `LV_ALIGN_BOTTOM_LEFT`            | Bottom-left corner       |
| `LV_ALIGN_BOTTOM_MID`             | Bottom center            |
| `LV_ALIGN_BOTTOM_RIGHT`           | Bottom-right corner      |
| `LV_ALIGN_LEFT_MID`               | Left center              |
| `LV_ALIGN_RIGHT_MID`              | Right center             |
| `LV_ALIGN_CENTER`                 | Center                   |
| `LV_ALIGN_OUT_TOP_LEFT`           | Outside top-left         |
| `LV_ALIGN_OUT_TOP_MID`            | Outside top-center       |
| `LV_ALIGN_OUT_TOP_RIGHT`          | Outside top-right        |
| `LV_ALIGN_OUT_BOTTOM_LEFT`        | Outside bottom-left      |
| `LV_ALIGN_OUT_BOTTOM_MID`         | Outside bottom-center    |
| `LV_ALIGN_OUT_BOTTOM_RIGHT`       | Outside bottom-right     |
| `LV_ALIGN_OUT_LEFT_TOP`           | Outside left-top         |
| `LV_ALIGN_OUT_LEFT_MID`           | Outside left-center      |
| `LV_ALIGN_OUT_LEFT_BOTTOM`        | Outside left-bottom      |
| `LV_ALIGN_OUT_RIGHT_TOP`          | Outside right-top        |
| `LV_ALIGN_OUT_RIGHT_MID`          | Outside right-center     |
| `LV_ALIGN_OUT_RIGHT_BOTTOM`       | Outside right-bottom     |

### Parent/Child Hierarchy

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

### Screens

```c
lv_obj_t * lv_scr_act(void);                          // Get active screen
void lv_scr_load(lv_obj_t * scr);                     // Load screen (no animation)
void lv_scr_load_anim(lv_obj_t * scr, lv_scr_load_anim_t anim,
                      uint32_t time, uint32_t delay, bool auto_del);
lv_obj_t * lv_obj_get_screen(const lv_obj_t * obj);   // Get screen of object
lv_obj_t * lv_layer_top(void);                         // Top layer (above screens)
lv_obj_t * lv_layer_sys(void);                         // System layer (topmost)
```

**Screen load animations:**

| Constant                          | Description              |
|-----------------------------------|--------------------------|
| `LV_SCR_LOAD_ANIM_NONE`           | No animation             |
| `LV_SCR_LOAD_ANIM_OVER_LEFT`      | Slide new from right     |
| `LV_SCR_LOAD_ANIM_OVER_RIGHT`     | Slide new from left      |
| `LV_SCR_LOAD_ANIM_OVER_TOP`       | Slide new from bottom    |
| `LV_SCR_LOAD_ANIM_OVER_BOTTOM`    | Slide new from top       |
| `LV_SCR_LOAD_ANIM_MOVE_LEFT`      | Move both left           |
| `LV_SCR_LOAD_ANIM_MOVE_RIGHT`     | Move both right          |
| `LV_SCR_LOAD_ANIM_MOVE_TOP`       | Move both up             |
| `LV_SCR_LOAD_ANIM_MOVE_BOTTOM`    | Move both down           |
| `LV_SCR_LOAD_ANIM_FADE_ON`        | Fade in new screen       |
| `LV_SCR_LOAD_ANIM_FADE_IN`        | Fade in (alias)          |
| `LV_SCR_LOAD_ANIM_FADE_OUT`       | Fade out old screen      |
| `LV_SCR_LOAD_ANIM_OUT_LEFT`       | **New in 8.3.** Slide old left   |
| `LV_SCR_LOAD_ANIM_OUT_RIGHT`      | **New in 8.3.** Slide old right  |
| `LV_SCR_LOAD_ANIM_OUT_TOP`        | **New in 8.3.** Slide old up     |
| `LV_SCR_LOAD_ANIM_OUT_BOTTOM`     | **New in 8.3.** Slide old down   |

### Flags

```c
void lv_obj_add_flag(lv_obj_t * obj, lv_obj_flag_t f);
void lv_obj_clear_flag(lv_obj_t * obj, lv_obj_flag_t f);
bool lv_obj_has_flag(const lv_obj_t * obj, lv_obj_flag_t f);
bool lv_obj_has_flag_any(const lv_obj_t * obj, lv_obj_flag_t f);
```

**Flag constants:**

| Flag                               | Description                                  |
|------------------------------------|----------------------------------------------|
| `LV_OBJ_FLAG_HIDDEN`               | Object is hidden                             |
| `LV_OBJ_FLAG_CLICKABLE`            | Can receive click events                     |
| `LV_OBJ_FLAG_CLICK_FOCUSABLE`      | Focusable on click                           |
| `LV_OBJ_FLAG_CHECKABLE`            | Toggle checked state on click                |
| `LV_OBJ_FLAG_SCROLLABLE`           | Can be scrolled                              |
| `LV_OBJ_FLAG_SCROLL_ELASTIC`       | Elastic scroll effect                        |
| `LV_OBJ_FLAG_SCROLL_MOMENTUM`      | Momentum-based scrolling                     |
| `LV_OBJ_FLAG_SCROLL_ONE`           | Scroll one snap unit at a time               |
| `LV_OBJ_FLAG_SCROLL_CHAIN_HOR`     | Chain horizontal scroll to parent            |
| `LV_OBJ_FLAG_SCROLL_CHAIN_VER`     | Chain vertical scroll to parent              |
| `LV_OBJ_FLAG_SCROLL_CHAIN`         | Chain scroll (both directions)               |
| `LV_OBJ_FLAG_SCROLL_ON_FOCUS`      | Scroll to show focused child                 |
| `LV_OBJ_FLAG_SCROLL_WITH_ARROW`    | Arrow keys trigger scrolling                 |
| `LV_OBJ_FLAG_SNAPPABLE`            | Snap point for scroll                        |
| `LV_OBJ_FLAG_PRESS_LOCK`           | Keep pressed even if cursor leaves           |
| `LV_OBJ_FLAG_EVENT_BUBBLE`         | Bubble events to parent                      |
| `LV_OBJ_FLAG_GESTURE_BUBBLE`       | Bubble gestures to parent                    |
| `LV_OBJ_FLAG_ADV_HITTEST`          | Advanced hit-testing via event               |
| `LV_OBJ_FLAG_IGNORE_LAYOUT`        | Ignored by layout engine                     |
| `LV_OBJ_FLAG_FLOATING`             | Not part of layout; not scrolled with parent |
| `LV_OBJ_FLAG_OVERFLOW_VISIBLE`     | Children drawn outside parent bounds         |
| `LV_OBJ_FLAG_LAYOUT_1`             | Custom layout flag 1                         |
| `LV_OBJ_FLAG_LAYOUT_2`             | Custom layout flag 2                         |
| `LV_OBJ_FLAG_WIDGET_1`             | Custom widget flag 1                         |
| `LV_OBJ_FLAG_WIDGET_2`             | Custom widget flag 2                         |
| `LV_OBJ_FLAG_USER_1..4`            | Custom user flags                            |

### States

```c
void lv_obj_add_state(lv_obj_t * obj, lv_state_t state);
void lv_obj_clear_state(lv_obj_t * obj, lv_state_t state);
bool lv_obj_has_state(const lv_obj_t * obj, lv_state_t state);
lv_state_t lv_obj_get_state(const lv_obj_t * obj);
```

| State                       | Value  | Description                |
|-----------------------------|--------|----------------------------|
| `LV_STATE_DEFAULT`          | 0x0000 | Normal state               |
| `LV_STATE_CHECKED`          | 0x0001 | Toggled/checked            |
| `LV_STATE_FOCUSED`          | 0x0002 | Focused via keypad/encoder |
| `LV_STATE_FOCUS_KEY`        | 0x0004 | Focus via keypad           |
| `LV_STATE_EDITED`           | 0x0008 | Edit mode (encoder)        |
| `LV_STATE_HOVERED`          | 0x0010 | Hovered by cursor          |
| `LV_STATE_PRESSED`          | 0x0020 | Being pressed              |
| `LV_STATE_SCROLLED`         | 0x0040 | Being scrolled             |
| `LV_STATE_DISABLED`         | 0x0080 | Disabled                   |
| `LV_STATE_USER_1..4`        | custom | Custom user states         |
| `LV_STATE_ANY`              | 0xFFFF | Match any state            |

### Scroll

```c
void lv_obj_set_scroll_dir(lv_obj_t * obj, lv_dir_t dir);
void lv_obj_set_scroll_snap_x(lv_obj_t * obj, lv_scroll_snap_t snap);
void lv_obj_set_scroll_snap_y(lv_obj_t * obj, lv_scroll_snap_t snap);
void lv_obj_set_scrollbar_mode(lv_obj_t * obj, lv_scrollbar_mode_t mode);

void lv_obj_scroll_by(lv_obj_t * obj, lv_coord_t dx, lv_coord_t dy, lv_anim_enable_t anim);
void lv_obj_scroll_to(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_anim_enable_t anim);
void lv_obj_scroll_to_view(lv_obj_t * obj, lv_anim_enable_t anim);
void lv_obj_scroll_to_view_recursive(lv_obj_t * obj, lv_anim_enable_t anim);

lv_coord_t lv_obj_get_scroll_x(const lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_y(const lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_top(lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_bottom(lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_left(lv_obj_t * obj);
lv_coord_t lv_obj_get_scroll_right(lv_obj_t * obj);
lv_dir_t lv_obj_get_scroll_dir(const lv_obj_t * obj);
lv_scrollbar_mode_t lv_obj_get_scrollbar_mode(const lv_obj_t * obj);
bool lv_obj_is_scrolling(const lv_obj_t * obj);
```

### Extended Click Area

```c
void lv_obj_set_ext_click_area(lv_obj_t * obj, lv_coord_t size);
```

---

## 2. Display Driver API

### Initialization

```c
void lv_disp_draw_buf_init(lv_disp_draw_buf_t * draw_buf,
                           void * buf1, void * buf2, uint32_t size_in_px_cnt);
void lv_disp_drv_init(lv_disp_drv_t * driver);
lv_disp_t * lv_disp_drv_register(lv_disp_drv_t * driver);
void lv_disp_drv_update(lv_disp_t * disp, lv_disp_drv_t * new_drv);
void lv_disp_remove(lv_disp_t * disp);
```

### Display Control

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

### Flush Control (used in flush_cb)

```c
void lv_disp_flush_ready(lv_disp_drv_t * disp_drv);
bool lv_disp_flush_is_last(lv_disp_drv_t * disp_drv);
```

### Background

```c
void lv_disp_set_bg_color(lv_disp_t * disp, lv_color_t color);
void lv_disp_set_bg_image(lv_disp_t * disp, const void * img_src);
void lv_disp_set_bg_opa(lv_disp_t * disp, lv_opa_t opa);
```

### Screen Management

```c
lv_obj_t * lv_disp_get_scr_act(lv_disp_t * disp);
lv_obj_t * lv_disp_get_scr_prev(lv_disp_t * disp);
void lv_disp_load_scr(lv_obj_t * scr);
lv_obj_t * lv_disp_get_layer_top(lv_disp_t * disp);
lv_obj_t * lv_disp_get_layer_sys(lv_disp_t * disp);
```

### Activity

```c
uint32_t lv_disp_get_inactive_time(const lv_disp_t * disp);
void lv_disp_trig_activity(lv_disp_t * disp);
```

### Typical Display Driver Setup (ESP32 SPI Example)

```c
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[320 * 40];
static lv_color_t buf2[320 * 40];
lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 320 * 40);

static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = 320;
disp_drv.ver_res = 240;
disp_drv.flush_cb = my_flush_cb;
disp_drv.draw_buf = &draw_buf;
disp_drv.render_start_cb = my_render_start_cb;  // New in 8.3
lv_disp_drv_register(&disp_drv);
```

---

## 3. Input Device Driver API

### Initialization

```c
void lv_indev_drv_init(lv_indev_drv_t * driver);
lv_indev_t * lv_indev_drv_register(lv_indev_drv_t * driver);
void lv_indev_drv_update(lv_indev_t * indev, lv_indev_drv_t * new_drv);
```

### Input Device Control

```c
lv_indev_t * lv_indev_get_next(lv_indev_t * indev);  // NULL for first
lv_indev_type_t lv_indev_get_type(const lv_indev_t * indev);
void lv_indev_read_timer_cb(lv_timer_t * timer);
void lv_indev_enable(lv_indev_t * indev, bool en);

void lv_indev_set_cursor(lv_indev_t * indev, lv_obj_t * cur_obj);
void lv_indev_set_group(lv_indev_t * indev, lv_group_t * group);
void lv_indev_set_button_points(lv_indev_t * indev, const lv_point_t points[]);

void lv_indev_get_point(const lv_indev_t * indev, lv_point_t * point);
lv_dir_t lv_indev_get_gesture_dir(const lv_indev_t * indev);
lv_key_t lv_indev_get_key(const lv_indev_t * indev);
lv_dir_t lv_indev_get_scroll_dir(const lv_indev_t * indev);
lv_obj_t * lv_indev_get_scroll_obj(const lv_indev_t * indev);
void lv_indev_wait_release(lv_indev_t * indev);
lv_obj_t * lv_indev_get_obj_act(void);
lv_timer_t * lv_indev_get_read_timer(lv_disp_t * indev);
lv_obj_t * lv_indev_search_obj(lv_obj_t * obj, lv_point_t * point);
```

### Key Constants

| Key                      | Value  | Description              |
|--------------------------|--------|--------------------------|
| `LV_KEY_UP`              | 17     | Up arrow                 |
| `LV_KEY_DOWN`            | 18     | Down arrow               |
| `LV_KEY_RIGHT`           | 19     | Right arrow              |
| `LV_KEY_LEFT`            | 20     | Left arrow               |
| `LV_KEY_ESC`             | 27     | Escape                   |
| `LV_KEY_DEL`             | 127    | Delete                   |
| `LV_KEY_BACKSPACE`       | 8      | Backspace                |
| `LV_KEY_ENTER`           | 10     | Enter/OK                 |
| `LV_KEY_NEXT`            | 9      | Focus next (Tab)         |
| `LV_KEY_PREV`            | 11     | Focus previous           |
| `LV_KEY_HOME`            | 2      | Home                     |
| `LV_KEY_END`             | 3      | End                      |

---

## 4. Style API

### Style Lifecycle

```c
void lv_style_init(lv_style_t * style);
void lv_style_reset(lv_style_t * style);
bool lv_style_is_empty(const lv_style_t * style);

void lv_obj_add_style(lv_obj_t * obj, lv_style_t * style, lv_style_selector_t selector);
void lv_obj_remove_style(lv_obj_t * obj, lv_style_t * style, lv_style_selector_t selector);
void lv_obj_remove_style_all(lv_obj_t * obj);
void lv_obj_report_style_change(lv_style_t * style);

void lv_obj_refresh_style(lv_obj_t * obj, lv_style_selector_t selector, lv_style_prop_t prop);
```

### Style Selector
Combine a part and a state: `LV_PART_MAIN | LV_STATE_PRESSED`

### Style Property Setters (Common)

```c
// Background
void lv_style_set_bg_color(lv_style_t * style, lv_color_t color);
void lv_style_set_bg_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_bg_grad_color(lv_style_t * style, lv_color_t color);
void lv_style_set_bg_grad_dir(lv_style_t * style, lv_grad_dir_t dir);
void lv_style_set_bg_img_src(lv_style_t * style, const void * src);
void lv_style_set_bg_img_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_bg_img_recolor(lv_style_t * style, lv_color_t color);
void lv_style_set_bg_img_recolor_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_bg_img_tiled(lv_style_t * style, bool tiled);

// Border
void lv_style_set_border_color(lv_style_t * style, lv_color_t color);
void lv_style_set_border_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_border_width(lv_style_t * style, lv_coord_t width);
void lv_style_set_border_side(lv_style_t * style, lv_border_side_t side);
void lv_style_set_border_post(lv_style_t * style, bool post);

// Outline
void lv_style_set_outline_color(lv_style_t * style, lv_color_t color);
void lv_style_set_outline_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_outline_width(lv_style_t * style, lv_coord_t width);
void lv_style_set_outline_pad(lv_style_t * style, lv_coord_t pad);

// Shadow
void lv_style_set_shadow_color(lv_style_t * style, lv_color_t color);
void lv_style_set_shadow_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_shadow_width(lv_style_t * style, lv_coord_t width);
void lv_style_set_shadow_ofs_x(lv_style_t * style, lv_coord_t ofs);
void lv_style_set_shadow_ofs_y(lv_style_t * style, lv_coord_t ofs);
void lv_style_set_shadow_spread(lv_style_t * style, lv_coord_t spread);

// Padding
void lv_style_set_pad_top(lv_style_t * style, lv_coord_t pad);
void lv_style_set_pad_bottom(lv_style_t * style, lv_coord_t pad);
void lv_style_set_pad_left(lv_style_t * style, lv_coord_t pad);
void lv_style_set_pad_right(lv_style_t * style, lv_coord_t pad);
void lv_style_set_pad_all(lv_style_t * style, lv_coord_t pad);
void lv_style_set_pad_row(lv_style_t * style, lv_coord_t pad);
void lv_style_set_pad_column(lv_style_t * style, lv_coord_t pad);
void lv_style_set_pad_gap(lv_style_t * style, lv_coord_t pad);

// Size
void lv_style_set_width(lv_style_t * style, lv_coord_t width);
void lv_style_set_min_width(lv_style_t * style, lv_coord_t width);
void lv_style_set_max_width(lv_style_t * style, lv_coord_t width);
void lv_style_set_height(lv_style_t * style, lv_coord_t height);
void lv_style_set_min_height(lv_style_t * style, lv_coord_t height);
void lv_style_set_max_height(lv_style_t * style, lv_coord_t height);

// Text
void lv_style_set_text_color(lv_style_t * style, lv_color_t color);
void lv_style_set_text_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_text_font(lv_style_t * style, const lv_font_t * font);
void lv_style_set_text_letter_space(lv_style_t * style, lv_coord_t space);
void lv_style_set_text_line_space(lv_style_t * style, lv_coord_t space);
void lv_style_set_text_decor(lv_style_t * style, lv_text_decor_t decor);
void lv_style_set_text_align(lv_style_t * style, lv_text_align_t align);

// Line
void lv_style_set_line_color(lv_style_t * style, lv_color_t color);
void lv_style_set_line_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_line_width(lv_style_t * style, lv_coord_t width);
void lv_style_set_line_dash_width(lv_style_t * style, lv_coord_t width);
void lv_style_set_line_dash_gap(lv_style_t * style, lv_coord_t gap);
void lv_style_set_line_rounded(lv_style_t * style, bool rounded);

// Arc
void lv_style_set_arc_color(lv_style_t * style, lv_color_t color);
void lv_style_set_arc_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_arc_width(lv_style_t * style, lv_coord_t width);
void lv_style_set_arc_rounded(lv_style_t * style, bool rounded);
void lv_style_set_arc_img_src(lv_style_t * style, const void * src);

// Image
void lv_style_set_img_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_img_recolor(lv_style_t * style, lv_color_t color);
void lv_style_set_img_recolor_opa(lv_style_t * style, lv_opa_t opa);

// Transform (used by layer system in 8.3)
void lv_style_set_transform_zoom(lv_style_t * style, lv_coord_t zoom);
void lv_style_set_transform_angle(lv_style_t * style, lv_coord_t angle);
void lv_style_set_transform_pivot_x(lv_style_t * style, lv_coord_t pivot);
void lv_style_set_transform_pivot_y(lv_style_t * style, lv_coord_t pivot);

// Miscellaneous
void lv_style_set_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_color_filter_cb(lv_style_t * style, lv_color_filter_cb_t cb);
void lv_style_set_color_filter_opa(lv_style_t * style, lv_opa_t opa);
void lv_style_set_anim(lv_style_t * style, const lv_anim_t * anim);
void lv_style_set_anim_time(lv_style_t * style, uint32_t time);
void lv_style_set_anim_speed(lv_style_t * style, uint32_t speed);
void lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * tr);
void lv_style_set_blend_mode(lv_style_t * style, lv_blend_mode_t mode);
void lv_style_set_layout(lv_style_t * style, uint16_t layout);
void lv_style_set_base_dir(lv_style_t * style, lv_base_dir_t dir);
```

### Local Style Setters (on object directly)

Every `lv_style_set_*` has a corresponding `lv_obj_set_style_*`:

```c
void lv_obj_set_style_bg_color(lv_obj_t * obj, lv_color_t color, lv_style_selector_t selector);
void lv_obj_set_style_text_font(lv_obj_t * obj, const lv_font_t * font, lv_style_selector_t selector);
// ... and so on for every style property
```

### Style Parts

| Part                        | Description                                |
|-----------------------------|--------------------------------------------|
| `LV_PART_MAIN`              | Main background/body                       |
| `LV_PART_SCROLLBAR`         | Scrollbar                                  |
| `LV_PART_INDICATOR`         | Indicator (slider track, bar fill, etc.)   |
| `LV_PART_KNOB`              | Draggable handle                           |
| `LV_PART_SELECTED`          | Selected item highlight                    |
| `LV_PART_ITEMS`             | Collection items (buttons, list entries)    |
| `LV_PART_TICKS`             | Tick marks and labels                      |
| `LV_PART_CURSOR`            | Text cursor                                |
| `LV_PART_CUSTOM_FIRST`      | Start of custom parts                      |
| `LV_PART_ANY`               | Match any part                             |

---

## 5. Event API

### Event Registration and Sending

```c
struct _lv_event_dsc_t * lv_obj_add_event_cb(lv_obj_t * obj, lv_event_cb_t event_cb,
                                              lv_event_code_t filter, void * user_data);
bool lv_obj_remove_event_cb(lv_obj_t * obj, lv_event_cb_t event_cb);
bool lv_obj_remove_event_cb_with_user_data(lv_obj_t * obj, lv_event_cb_t event_cb, void * user_data);
bool lv_obj_remove_event_dsc(lv_obj_t * obj, struct _lv_event_dsc_t * dsc);

lv_res_t lv_event_send(lv_obj_t * obj, lv_event_code_t event_code, void * param);
lv_res_t lv_obj_event_base(const lv_obj_class_t * class_p, lv_event_t * e);
```

### Event Data Access (inside callback)

```c
lv_obj_t * lv_event_get_target(lv_event_t * e);
lv_obj_t * lv_event_get_current_target(lv_event_t * e);
lv_event_code_t lv_event_get_code(lv_event_t * e);
void * lv_event_get_param(lv_event_t * e);
void * lv_event_get_user_data(lv_event_t * e);
lv_indev_t * lv_event_get_indev(lv_event_t * e);
lv_obj_draw_part_dsc_t * lv_event_get_draw_part_dsc(lv_event_t * e);
const lv_area_t * lv_event_get_clip_area(lv_event_t * e);
const lv_area_t * lv_event_get_old_size(lv_event_t * e);
lv_key_t lv_event_get_key(lv_event_t * e);
lv_anim_t * lv_event_get_anim(lv_event_t * e);
lv_draw_ctx_t * lv_event_get_draw_ctx(lv_event_t * e);

void lv_event_stop_bubbling(lv_event_t * e);
void lv_event_stop_processing(lv_event_t * e);

uint32_t lv_event_register_id(void);  // Register custom event
```

### Complete Event Code Reference

| Event Code                        | Category   | Description                                    |
|-----------------------------------|------------|------------------------------------------------|
| `LV_EVENT_PRESSED`               | Input      | Object pressed                                 |
| `LV_EVENT_PRESSING`              | Input      | Object being pressed (continuous)              |
| `LV_EVENT_PRESS_LOST`            | Input      | Cursor left object while pressing              |
| `LV_EVENT_SHORT_CLICKED`         | Input      | Short press and release (no scroll)            |
| `LV_EVENT_LONG_PRESSED`          | Input      | Held for long_press_time                       |
| `LV_EVENT_LONG_PRESSED_REPEAT`   | Input      | Long press repeat interval                     |
| `LV_EVENT_CLICKED`               | Input      | Released (no scroll)                           |
| `LV_EVENT_RELEASED`              | Input      | Released (always)                              |
| `LV_EVENT_SCROLL_BEGIN`          | Input      | Scroll started                                 |
| `LV_EVENT_SCROLL_END`            | Input      | Scroll ended                                   |
| `LV_EVENT_SCROLL`                | Input      | Scrolling                                      |
| `LV_EVENT_GESTURE`               | Input      | Gesture detected                               |
| `LV_EVENT_KEY`                   | Input      | Key sent to object                             |
| `LV_EVENT_FOCUSED`               | Input      | Object focused                                 |
| `LV_EVENT_DEFOCUSED`             | Input      | Object unfocused                               |
| `LV_EVENT_LEAVE`                 | Input      | Unfocused but still selected                   |
| `LV_EVENT_HIT_TEST`              | Input      | Advanced hit-testing                           |
| `LV_EVENT_COVER_CHECK`           | Draw       | Check if object covers an area                 |
| `LV_EVENT_REFR_EXT_DRAW_SIZE`    | Draw       | Get extra draw area (shadows, etc.)            |
| `LV_EVENT_DRAW_MAIN_BEGIN`       | Draw       | Main drawing phase begins                      |
| `LV_EVENT_DRAW_MAIN`             | Draw       | Perform main drawing                           |
| `LV_EVENT_DRAW_MAIN_END`         | Draw       | Main drawing phase ends                        |
| `LV_EVENT_DRAW_POST_BEGIN`       | Draw       | Post-draw phase begins                         |
| `LV_EVENT_DRAW_POST`             | Draw       | Perform post-draw                              |
| `LV_EVENT_DRAW_POST_END`         | Draw       | Post-draw phase ends                           |
| `LV_EVENT_DRAW_PART_BEGIN`       | Draw       | Widget part drawing begins                     |
| `LV_EVENT_DRAW_PART_END`         | Draw       | Widget part drawing ends                       |
| `LV_EVENT_VALUE_CHANGED`         | Special    | Object value changed                           |
| `LV_EVENT_INSERT`                | Special    | Text being inserted (textarea)                 |
| `LV_EVENT_REFRESH`               | Special    | Refresh notification                           |
| `LV_EVENT_READY`                 | Special    | Process finished                               |
| `LV_EVENT_CANCEL`                | Special    | Process cancelled                              |
| `LV_EVENT_DELETE`                | Lifecycle  | Object being deleted                           |
| `LV_EVENT_CHILD_CHANGED`         | Lifecycle  | Child added/removed                            |
| `LV_EVENT_CHILD_CREATED`         | Lifecycle  | Child created (bubbles)                        |
| `LV_EVENT_CHILD_DELETED`         | Lifecycle  | Child deleted (bubbles)                        |
| `LV_EVENT_SIZE_CHANGED`          | Lifecycle  | Size/position changed                          |
| `LV_EVENT_STYLE_CHANGED`         | Lifecycle  | Style changed                                  |
| `LV_EVENT_BASE_DIR_CHANGED`      | Lifecycle  | Base direction changed                         |
| `LV_EVENT_GET_SELF_SIZE`         | Lifecycle  | Get widget internal size                       |
| `LV_EVENT_SCREEN_UNLOAD_START`   | Screen     | Screen unload started                          |
| `LV_EVENT_SCREEN_LOAD_START`     | Screen     | Screen load started                            |
| `LV_EVENT_SCREEN_LOADED`         | Screen     | Screen loaded (animations done)                |
| `LV_EVENT_SCREEN_UNLOADED`       | Screen     | Screen unloaded (animations done)              |

---

## 6. Animation API

```c
void lv_anim_init(lv_anim_t * anim);
lv_anim_t * lv_anim_start(const lv_anim_t * anim);
bool lv_anim_del(void * var, lv_anim_exec_xcb_t exec_cb);
void lv_anim_del_all(void);
lv_anim_t * lv_anim_get(void * var, lv_anim_exec_xcb_t exec_cb);
uint16_t lv_anim_count_running(void);
uint32_t lv_anim_speed_to_time(uint32_t speed, int32_t start, int32_t end);

// Configuration setters
void lv_anim_set_var(lv_anim_t * anim, void * var);
void lv_anim_set_exec_cb(lv_anim_t * anim, lv_anim_exec_xcb_t exec_cb);
void lv_anim_set_time(lv_anim_t * anim, uint32_t duration);
void lv_anim_set_delay(lv_anim_t * anim, uint32_t delay);
void lv_anim_set_values(lv_anim_t * anim, int32_t start, int32_t end);
void lv_anim_set_path_cb(lv_anim_t * anim, lv_anim_path_cb_t path_cb);
void lv_anim_set_start_cb(lv_anim_t * anim, lv_anim_start_cb_t cb);
void lv_anim_set_ready_cb(lv_anim_t * anim, lv_anim_ready_cb_t cb);
void lv_anim_set_deleted_cb(lv_anim_t * anim, lv_anim_deleted_cb_t cb);
void lv_anim_set_playback_time(lv_anim_t * anim, uint32_t time);
void lv_anim_set_playback_delay(lv_anim_t * anim, uint32_t delay);
void lv_anim_set_repeat_count(lv_anim_t * anim, uint16_t cnt);
void lv_anim_set_repeat_delay(lv_anim_t * anim, uint32_t delay);
void lv_anim_set_early_apply(lv_anim_t * anim, bool en);
```

### Built-in Path Functions

| Function                         | Curve Type                |
|----------------------------------|---------------------------|
| `lv_anim_path_linear`            | Linear                    |
| `lv_anim_path_ease_in`           | Slow start                |
| `lv_anim_path_ease_out`          | Slow end                  |
| `lv_anim_path_ease_in_out`       | Slow start and end        |
| `lv_anim_path_overshoot`         | Overshoot target          |
| `lv_anim_path_bounce`            | Bounce at target          |
| `lv_anim_path_step`              | Instant change at end     |

### Animation Timeline

```c
lv_anim_timeline_t * lv_anim_timeline_create(void);
void lv_anim_timeline_del(lv_anim_timeline_t * timeline);
void lv_anim_timeline_add(lv_anim_timeline_t * timeline, uint32_t start_time, lv_anim_t * anim);
uint32_t lv_anim_timeline_start(lv_anim_timeline_t * timeline);
void lv_anim_timeline_stop(lv_anim_timeline_t * timeline);
void lv_anim_timeline_set_reverse(lv_anim_timeline_t * timeline, bool reverse);
void lv_anim_timeline_set_progress(lv_anim_timeline_t * timeline, uint16_t progress);
uint16_t lv_anim_timeline_get_playtime(lv_anim_timeline_t * timeline);
bool lv_anim_timeline_get_reverse(lv_anim_timeline_t * timeline);
```

---

## 7. File System API

```c
// Driver registration
void lv_fs_drv_init(lv_fs_drv_t * drv);
void lv_fs_drv_register(lv_fs_drv_t * drv);
lv_fs_drv_t * lv_fs_get_drv(char letter);

// File operations
lv_fs_res_t lv_fs_open(lv_fs_file_t * file, const char * path, lv_fs_mode_t mode);
lv_fs_res_t lv_fs_close(lv_fs_file_t * file);
lv_fs_res_t lv_fs_read(lv_fs_file_t * file, void * buf, uint32_t btr, uint32_t * br);
lv_fs_res_t lv_fs_write(lv_fs_file_t * file, const void * buf, uint32_t btw, uint32_t * bw);
lv_fs_res_t lv_fs_seek(lv_fs_file_t * file, uint32_t pos, lv_fs_whence_t whence);
lv_fs_res_t lv_fs_tell(lv_fs_file_t * file, uint32_t * pos);

// Directory operations
lv_fs_res_t lv_fs_dir_open(lv_fs_dir_t * dir, const char * path);
lv_fs_res_t lv_fs_dir_read(lv_fs_dir_t * dir, char * fn);
lv_fs_res_t lv_fs_dir_close(lv_fs_dir_t * dir);

// Utility
bool lv_fs_is_ready(char letter);
char * lv_fs_get_letters(char * buf);
const char * lv_fs_get_ext(const char * fn);
void lv_fs_up(char * path);
const char * lv_fs_get_last(const char * path);
```

### File System Result Codes

| Code                    | Description              |
|-------------------------|--------------------------|
| `LV_FS_RES_OK`          | Success                  |
| `LV_FS_RES_HW_ERR`      | Hardware error           |
| `LV_FS_RES_FS_ERR`      | File system error        |
| `LV_FS_RES_NOT_EX`      | File not found           |
| `LV_FS_RES_FULL`        | Storage full             |
| `LV_FS_RES_LOCKED`      | File locked              |
| `LV_FS_RES_DENIED`      | Access denied            |
| `LV_FS_RES_BUSY`        | Driver busy              |
| `LV_FS_RES_TOUT`        | Timeout                  |
| `LV_FS_RES_NOT_IMP`     | Not implemented          |
| `LV_FS_RES_OUT_OF_MEM`  | Out of memory            |
| `LV_FS_RES_INV_PARAM`   | Invalid parameter        |
| `LV_FS_RES_UNKNOWN`     | Unknown error            |

### File Modes

| Mode                    | Description              |
|-------------------------|--------------------------|
| `LV_FS_MODE_WR`         | Write                    |
| `LV_FS_MODE_RD`         | Read                     |

### Seek Whence

| Whence                  | Description              |
|-------------------------|--------------------------|
| `LV_FS_SEEK_SET`        | From beginning           |
| `LV_FS_SEEK_CUR`        | From current position    |
| `LV_FS_SEEK_END`        | From end                 |

---

## 8. Image Decoder API

### Decoder Registration

```c
lv_img_decoder_t * lv_img_decoder_create(void);
void lv_img_decoder_delete(lv_img_decoder_t * decoder);
void lv_img_decoder_set_info_cb(lv_img_decoder_t * dec, lv_img_decoder_info_f_t info_cb);
void lv_img_decoder_set_open_cb(lv_img_decoder_t * dec, lv_img_decoder_open_f_t open_cb);
void lv_img_decoder_set_read_line_cb(lv_img_decoder_t * dec, lv_img_decoder_read_line_f_t read_line_cb);
void lv_img_decoder_set_close_cb(lv_img_decoder_t * dec, lv_img_decoder_close_f_t close_cb);
```

### Image Cache

```c
void lv_img_cache_set_size(uint16_t new_slot_num);
void lv_img_cache_invalidate_src(const void * src);
```

### Image Descriptor

```c
typedef struct {
    lv_img_header_t header;  // Width, height, color format
    uint32_t data_size;
    const uint8_t * data;
} lv_img_dsc_t;

typedef struct {
    uint32_t cf : 5;    // Color format (LV_IMG_CF_*)
    uint32_t always_zero : 3;
    uint32_t reserved : 2;
    uint32_t w : 11;    // Width (max 2048)
    uint32_t h : 11;    // Height (max 2048)
} lv_img_header_t;
```

---

## 9. Font API

```c
// Runtime font loading
lv_font_t * lv_font_load(const char * font_name);
void lv_font_free(lv_font_t * font);

// Font declaration (for compiled-in fonts)
LV_FONT_DECLARE(lv_font_montserrat_14);

// Tiny TTF (new in v8.3.11)
lv_font_t * lv_tiny_ttf_create_file(const char * path, lv_coord_t font_size);
lv_font_t * lv_tiny_ttf_create_data(const void * data, size_t data_size, lv_coord_t font_size);
void lv_tiny_ttf_set_size(lv_font_t * font, lv_coord_t font_size);
void lv_tiny_ttf_destroy(lv_font_t * font);

// Image font (new in v8.3.0)
lv_font_t * lv_imgfont_create(lv_coord_t height, lv_imgfont_get_path_cb_t path_cb);
void lv_imgfont_destroy(lv_font_t * font);
```

---

## 10. Messaging API (New in 8.3)

Enable with `LV_USE_MSG 1` in lv_conf.h.

```c
void lv_msg_init(void);

// Subscribe
void * lv_msg_subscribe(uint32_t msg_id, lv_msg_subscribe_cb_t cb, void * user_data);
void * lv_msg_subscribe_obj(uint32_t msg_id, lv_obj_t * obj, void * user_data);

// Unsubscribe
void lv_msg_unsubscribe(void * subscribe_id);
void lv_msg_unsubscribe_obj(uint32_t msg_id, lv_obj_t * obj);  // Added v8.3.6

// Publish
void lv_msg_send(uint32_t msg_id, const void * payload);

// In callback
uint32_t lv_msg_get_id(lv_msg_t * msg);
const void * lv_msg_get_payload(lv_msg_t * msg);
void * lv_msg_get_user_data(lv_msg_t * msg);

// Register custom message ID
uint32_t lv_msg_register_id(void);
```

### Usage Pattern

```c
#define MSG_TEMPERATURE_CHANGED  lv_msg_register_id()

// Publisher
void sensor_update(float temp) {
    lv_msg_send(MSG_TEMPERATURE_CHANGED, &temp);
}

// Subscriber
void temp_display_cb(lv_msg_t * msg) {
    float temp = *(float *)lv_msg_get_payload(msg);
    lv_label_set_text_fmt(label, "%.1f C", temp);
}

lv_msg_subscribe(MSG_TEMPERATURE_CHANGED, temp_display_cb, NULL);
```

---

## 11. Fragment Manager API (New in 8.3)

Enable with `LV_USE_FRAGMENT 1` in lv_conf.h.

```c
lv_fragment_manager_t * lv_fragment_manager_create(lv_fragment_t * parent);
void lv_fragment_manager_del(lv_fragment_manager_t * manager);

lv_fragment_t * lv_fragment_create(const lv_fragment_class_t * cls, void * args);
void lv_fragment_del(lv_fragment_t * fragment);

void lv_fragment_manager_add(lv_fragment_manager_t * manager, lv_fragment_t * fragment,
                             lv_obj_t ** container);
void lv_fragment_manager_remove(lv_fragment_manager_t * manager, lv_fragment_t * fragment);
void lv_fragment_manager_replace(lv_fragment_manager_t * manager, lv_fragment_t * fragment,
                                 lv_obj_t ** container);
void lv_fragment_manager_push(lv_fragment_manager_t * manager, lv_fragment_t * fragment,
                              lv_obj_t ** container);
bool lv_fragment_manager_pop(lv_fragment_manager_t * manager);
bool lv_fragment_manager_send_event(lv_fragment_manager_t * manager, int code, void * data);

lv_obj_t * lv_fragment_get_container(lv_fragment_t * fragment);
lv_fragment_t * lv_fragment_get_parent(lv_fragment_t * fragment);
lv_fragment_manager_t * lv_fragment_get_manager(lv_fragment_t * fragment);
```

---

## 12. GridNav API (New in 8.3)

Enable with `LV_USE_GRIDNAV 1` in lv_conf.h.

```c
void lv_gridnav_add(lv_obj_t * obj, lv_gridnav_ctrl_t ctrl);
void lv_gridnav_remove(lv_obj_t * obj);
```

### Control Flags

| Flag                                    | Description                              |
|-----------------------------------------|------------------------------------------|
| `LV_GRIDNAV_CTRL_NONE`                  | Default behavior                         |
| `LV_GRIDNAV_CTRL_ROLLOVER`              | Wrap around at edges                     |
| `LV_GRIDNAV_CTRL_SCROLL_FIRST`          | Scroll before focus change               |

---

## 13. Snapshot API (New in 8.3)

Enable with `LV_USE_SNAPSHOT 1` in lv_conf.h.

```c
lv_img_dsc_t * lv_snapshot_take(lv_obj_t * obj, lv_img_cf_t cf);
uint32_t lv_snapshot_buf_size_needed(lv_obj_t * obj, lv_img_cf_t cf);
lv_res_t lv_snapshot_take_to_buf(lv_obj_t * obj, lv_img_cf_t cf, lv_img_dsc_t * dsc,
                                 void * buf, uint32_t buf_size);
void lv_snapshot_free(lv_img_dsc_t * dsc);
```

---

## 14. IME Pinyin API (New in 8.3)

Enable with `LV_USE_IME_PINYIN 1` in lv_conf.h.

```c
lv_obj_t * lv_ime_pinyin_create(lv_obj_t * parent);
void lv_ime_pinyin_set_keyboard(lv_obj_t * obj, lv_obj_t * keyboard);
void lv_ime_pinyin_set_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict);
void lv_ime_pinyin_set_mode(lv_obj_t * obj, lv_ime_pinyin_mode_t mode);

lv_obj_t * lv_ime_pinyin_get_kb(lv_obj_t * obj);
const char * lv_ime_pinyin_get_cand_panel(lv_obj_t * obj);
const lv_pinyin_dict_t * lv_ime_pinyin_get_dict(lv_obj_t * obj);
```

### Pinyin Modes

| Mode                              | Description                |
|-----------------------------------|----------------------------|
| `LV_IME_PINYIN_MODE_K26`          | 26-key QWERTY layout       |
| `LV_IME_PINYIN_MODE_K9`           | 9-key T9 layout            |

---

## 15. Timer API

```c
lv_timer_t * lv_timer_create(lv_timer_cb_t timer_xcb, uint32_t period, void * user_data);
void lv_timer_del(lv_timer_t * timer);
void lv_timer_pause(lv_timer_t * timer);
void lv_timer_resume(lv_timer_t * timer);
void lv_timer_set_cb(lv_timer_t * timer, lv_timer_cb_t timer_cb);
void lv_timer_set_period(lv_timer_t * timer, uint32_t period);
void lv_timer_set_repeat_count(lv_timer_t * timer, int32_t repeat_count);
void lv_timer_ready(lv_timer_t * timer);
void lv_timer_reset(lv_timer_t * timer);
lv_timer_t * lv_timer_get_next(lv_timer_t * timer);
void lv_timer_enable(bool en);
uint8_t lv_timer_get_idle(void);
uint32_t lv_timer_handler(void);  // Must be called periodically (~5ms)
```

---

## 16. Group API

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

void lv_group_set_focus_cb(lv_group_t * group, lv_group_focus_cb_t focus_cb);
void lv_group_set_edge_cb(lv_group_t * group, lv_group_edge_cb_t edge_cb);
void lv_group_set_editing(lv_group_t * group, bool edit);
void lv_group_set_wrap(lv_group_t * group, bool en);

uint32_t lv_group_get_obj_count(lv_group_t * group);
bool lv_group_get_editing(const lv_group_t * group);
bool lv_group_get_wrap(lv_group_t * group);
```

---

## 17. Layout API (Flex and Grid)

### Flex Layout

```c
void lv_obj_set_flex_flow(lv_obj_t * obj, lv_flex_flow_t flow);
void lv_obj_set_flex_align(lv_obj_t * obj, lv_flex_align_t main_place,
                           lv_flex_align_t cross_place, lv_flex_align_t track_cross_place);
void lv_obj_set_flex_grow(lv_obj_t * obj, uint8_t grow);
```

**Flex Flow:**

| Flow                           | Description                |
|--------------------------------|----------------------------|
| `LV_FLEX_FLOW_ROW`             | Left to right              |
| `LV_FLEX_FLOW_COLUMN`          | Top to bottom              |
| `LV_FLEX_FLOW_ROW_WRAP`        | Row with wrapping          |
| `LV_FLEX_FLOW_COLUMN_WRAP`     | Column with wrapping       |
| `LV_FLEX_FLOW_ROW_REVERSE`     | Right to left              |
| `LV_FLEX_FLOW_COLUMN_REVERSE`  | Bottom to top              |
| `LV_FLEX_FLOW_ROW_WRAP_REVERSE`| Row wrap reversed          |
| `LV_FLEX_FLOW_COLUMN_WRAP_REVERSE`| Column wrap reversed    |

**Flex Align:**

| Align                          | Description                |
|--------------------------------|----------------------------|
| `LV_FLEX_ALIGN_START`          | Start of axis              |
| `LV_FLEX_ALIGN_END`            | End of axis                |
| `LV_FLEX_ALIGN_CENTER`         | Center of axis             |
| `LV_FLEX_ALIGN_SPACE_EVENLY`   | Equal spacing              |
| `LV_FLEX_ALIGN_SPACE_AROUND`   | Space around items         |
| `LV_FLEX_ALIGN_SPACE_BETWEEN`  | Space between items        |

### Grid Layout

```c
void lv_obj_set_grid_dsc_array(lv_obj_t * obj,
                               const lv_coord_t col_dsc[], const lv_coord_t row_dsc[]);
void lv_obj_set_grid_align(lv_obj_t * obj, lv_grid_align_t column_align,
                           lv_grid_align_t row_align);
void lv_obj_set_grid_cell(lv_obj_t * obj, lv_grid_align_t column_align,
                          uint8_t col_pos, uint8_t col_span,
                          lv_grid_align_t row_align,
                          uint8_t row_pos, uint8_t row_span);
```

**Grid Track Sizing:**

| Value                          | Description                |
|--------------------------------|----------------------------|
| `LV_GRID_CONTENT`              | Size to content            |
| `LV_GRID_FR(x)`                | Fraction of free space     |
| `LV_GRID_TEMPLATE_LAST`        | End of descriptor array    |
| Pixel value                    | Fixed size in pixels       |

---

## 18. Theme API

```c
lv_theme_t * lv_theme_default_init(lv_disp_t * disp, lv_color_t primary, lv_color_t secondary,
                                    bool dark, const lv_font_t * font);
lv_theme_t * lv_theme_basic_init(lv_disp_t * disp);
lv_theme_t * lv_theme_mono_init(lv_disp_t * disp, bool dark_bg, const lv_font_t * font);

void lv_disp_set_theme(lv_disp_t * disp, lv_theme_t * theme);
lv_theme_t * lv_disp_get_theme(lv_disp_t * disp);

// Custom theme creation
void lv_theme_set_parent(lv_theme_t * theme, lv_theme_t * parent);
void lv_theme_set_apply_cb(lv_theme_t * theme, lv_theme_apply_cb_t apply_cb);
```

---

## 19. Async API

```c
lv_res_t lv_async_call(lv_async_cb_t async_xcb, void * user_data);
lv_res_t lv_async_call_cancel(lv_async_cb_t async_xcb, void * user_data);  // New in 8.3
```

---

## Color Utility Functions

```c
lv_color_t lv_color_hex(uint32_t c);            // e.g., lv_color_hex(0xFF0000)
lv_color_t lv_color_hex3(uint32_t c);            // e.g., lv_color_hex3(0xF00)
lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b);
lv_color_t lv_color_black(void);
lv_color_t lv_color_white(void);
lv_color_hsv_t lv_color_rgb_to_hsv(uint8_t r, uint8_t g, uint8_t b);
lv_color_t lv_color_hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v);
lv_color_t lv_palette_main(lv_palette_t p);
lv_color_t lv_palette_lighten(lv_palette_t p, uint8_t lvl);  // 1-5
lv_color_t lv_palette_darken(lv_palette_t p, uint8_t lvl);   // 1-4
```

### Palette Constants

| Palette                  | Hex (approx)  |
|--------------------------|---------------|
| `LV_PALETTE_RED`          | #F44336       |
| `LV_PALETTE_PINK`         | #E91E63       |
| `LV_PALETTE_PURPLE`       | #9C27B0       |
| `LV_PALETTE_DEEP_PURPLE`  | #673AB7       |
| `LV_PALETTE_INDIGO`       | #3F51B5       |
| `LV_PALETTE_BLUE`         | #2196F3       |
| `LV_PALETTE_LIGHT_BLUE`   | #03A9F4       |
| `LV_PALETTE_CYAN`         | #00BCD4       |
| `LV_PALETTE_TEAL`         | #009688       |
| `LV_PALETTE_GREEN`        | #4CAF50       |
| `LV_PALETTE_LIGHT_GREEN`  | #8BC34A       |
| `LV_PALETTE_LIME`         | #CDDC39       |
| `LV_PALETTE_YELLOW`       | #FFEB3B       |
| `LV_PALETTE_AMBER`        | #FFC107       |
| `LV_PALETTE_ORANGE`       | #FF9800       |
| `LV_PALETTE_DEEP_ORANGE`  | #FF5722       |
| `LV_PALETTE_BROWN`        | #795548       |
| `LV_PALETTE_BLUE_GREY`    | #607D8B       |
| `LV_PALETTE_GREY`         | #9E9E9E       |

---

## Sources

- [LVGL 8.3 Documentation](https://docs.lvgl.io/8.3/)
- [LVGL 8.3 Changelog](https://docs.lvgl.io/8.3/CHANGELOG.html)
- [LVGL GitHub - release/v8.3](https://github.com/lvgl/lvgl/tree/release/v8.3)
- [lv_conf_template.h](https://github.com/lvgl/lvgl/blob/release/v8.3/lv_conf_template.h)
