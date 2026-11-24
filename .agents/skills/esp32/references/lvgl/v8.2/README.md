# LVGL v8.2.x Comprehensive Reference

> LVGL (Light and Versatile Graphics Library) -- Embedded graphics library for MCUs and MPUs.
> License: MIT | Language: C99 | Repository: https://github.com/lvgl/lvgl

---

## 1. Release Dates and Patch Versions

| Version | Release Date | Notes |
|---------|-------------|-------|
| v8.2.0 | 2022-01-31 | Major minor release; only release in 8.2.x series |

LVGL v8.2.0 was the sole release in the 8.2 branch. The next minor release was v8.3.0 (2022-07-13). Bug fixes after v8.2.0 were rolled into v8.3.x rather than producing 8.2.1+ patches.

### Version Timeline Context

| Version | Date | Significance |
|---------|------|-------------|
| v8.0.0 | 2021-06-01 | Major v8 architecture rewrite |
| v8.1.0 | 2021-10-06 | First minor update |
| **v8.2.0** | **2022-01-31** | **Abstract render layer, FFmpeg, Menu widget** |
| v8.3.0 | 2022-07-13 | Next minor (observer pattern, image rotate/zoom) |
| v8.4.0 | 2024-02-06 | Last 8.x release before v9 |

---

## 2. Key Features Introduced in v8.2 (vs v8.1)

### Major Additions

| Feature | Description |
|---------|-------------|
| Abstract Draw Layer | Pluggable rendering backend -- allows replacing the built-in SW renderer with custom draw engines |
| FFmpeg Decoder | Video playback and image decoding via FFmpeg (LV_USE_FFMPEG) |
| Menu Widget | New `lv_menu` widget for hierarchical navigation structures |
| Grid Navigation | `lv_gridnav` for keyboard/remote-control UI navigation |
| Font Fallback | Chain multiple fonts -- if a glyph is missing in the primary font, fallback fonts are searched |
| FreeType Bold/Italic | Synthetic bold and italic rendering when using FreeType |
| Gradient Dithering | Smoother color gradients on low-color-depth displays (LV_DITHER_GRADIENT) |
| Monkey Test | Automated random UI stress testing (LV_USE_MONKEY) |
| Snapshot API | Capture rendered object to image buffer (LV_USE_SNAPSHOT) |
| LV_OBJ_FLAG_OVERFLOW_VISIBLE | Allow children to be drawn outside parent boundaries |
| CMSIS-Pack Support | ARM Keil development environment integration |
| Span Widget | `lv_spangroup` for mixed-style text runs in a single label area |

### Environment Changes

- RT-Thread, CMake, and Zephyr support files moved into `env_support/` folder
- SDL renderer abstraction improved (optimized clip rectangles)
- `lv_color_hex()` function optimized for faster color conversion

---

## 3. Complete Widget/Object List

### Base Object

| Widget | Create Function | Description |
|--------|----------------|-------------|
| Base Object | `lv_obj_create(parent)` | Foundation for all widgets; supports coordinates, styles, events, scrolling, layouts |

### Core Widgets (always available)

| Widget | Create Function | Header | Description |
|--------|----------------|--------|-------------|
| Arc | `lv_arc_create(parent)` | `lv_arc.h` | Circular arc indicator/selector |
| Bar | `lv_bar_create(parent)` | `lv_bar.h` | Linear progress bar |
| Button | `lv_btn_create(parent)` | `lv_btn.h` | Clickable button (simple rectangle) |
| Button Matrix | `lv_btnmatrix_create(parent)` | `lv_btnmatrix.h` | Grid of lightweight buttons sharing one object |
| Canvas | `lv_canvas_create(parent)` | `lv_canvas.h` | Pixel-level drawing surface |
| Checkbox | `lv_checkbox_create(parent)` | `lv_checkbox.h` | Check/uncheck toggle with label |
| Dropdown | `lv_dropdown_create(parent)` | `lv_dropdown.h` | Expandable list selector |
| Image | `lv_img_create(parent)` | `lv_img.h` | Display images from various sources |
| Label | `lv_label_create(parent)` | `lv_label.h` | Text display with wrapping, scrolling, recolor |
| Line | `lv_line_create(parent)` | `lv_line.h` | Draw line segments from point arrays |
| Roller | `lv_roller_create(parent)` | `lv_roller.h` | Scrollable drum-style selector |
| Slider | `lv_slider_create(parent)` | `lv_slider.h` | Draggable value selector |
| Switch | `lv_switch_create(parent)` | `lv_switch.h` | On/off toggle |
| Table | `lv_table_create(parent)` | `lv_table.h` | Data grid with rows and columns |
| Text Area | `lv_textarea_create(parent)` | `lv_textarea.h` | Multi-line editable text input |

### Extra Widgets (enabled via lv_conf.h)

| Widget | Create Function | Config Macro | Description |
|--------|----------------|-------------|-------------|
| Anim Image | `lv_animimg_create(parent)` | `LV_USE_ANIMIMG` | Animated image sequence player |
| Calendar | `lv_calendar_create(parent)` | `LV_USE_CALENDAR` | Date picker with month view |
| Chart | `lv_chart_create(parent)` | `LV_USE_CHART` | Line, bar, scatter, area charts |
| Color Wheel | `lv_colorwheel_create(parent, knob_recolor)` | `LV_USE_COLORWHEEL` | HSV color picker |
| Image Button | `lv_imgbtn_create(parent)` | `LV_USE_IMGBTN` | Button with separate images per state |
| Keyboard | `lv_keyboard_create(parent)` | `LV_USE_KEYBOARD` | On-screen keyboard for text areas |
| LED | `lv_led_create(parent)` | `LV_USE_LED` | LED indicator with brightness control |
| List | `lv_list_create(parent)` | `LV_USE_LIST` | Scrollable list with text/icon items |
| Menu | `lv_menu_create(parent)` | `LV_USE_MENU` | Hierarchical navigation menu (NEW in v8.2) |
| Meter | `lv_meter_create(parent)` | `LV_USE_METER` | Gauge/dial indicator with scales and needles |
| Message Box | `lv_msgbox_create(parent, title, txt, btns, add_close)` | `LV_USE_MSGBOX` | Modal dialog with title, text, buttons |
| Span Group | `lv_spangroup_create(parent)` | `LV_USE_SPAN` | Mixed-style inline text |
| Spinbox | `lv_spinbox_create(parent)` | `LV_USE_SPINBOX` | Numeric input with increment/decrement |
| Spinner | `lv_spinner_create(parent, time, arc_length)` | `LV_USE_SPINNER` | Loading/busy animation |
| Tab View | `lv_tabview_create(parent, tab_pos, tab_size)` | `LV_USE_TABVIEW` | Tabbed container with swipe navigation |
| Tile View | `lv_tileview_create(parent)` | `LV_USE_TILEVIEW` | Full-screen swipeable pages |
| Window | `lv_win_create(parent, header_height)` | `LV_USE_WIN` | Window with header bar and content area |

---

## 4. API Reference Summary

### Core Lifecycle

```c
void lv_init(void);                          // Initialize LVGL (call once at startup)
void lv_deinit(void);                        // De-initialize LVGL
void lv_timer_handler(void);                 // Process pending LVGL tasks (call in main loop, ~5ms)
uint32_t lv_tick_get(void);                  // Get elapsed ms since boot
void lv_tick_inc(uint32_t tick_period);      // Advance tick counter (call from timer ISR)
```

### Object Management

```c
// Creation and deletion
lv_obj_t * lv_obj_create(lv_obj_t * parent);
void lv_obj_del(lv_obj_t * obj);
void lv_obj_del_async(lv_obj_t * obj);       // Delete on next lv_timer_handler call
void lv_obj_clean(lv_obj_t * obj);           // Delete all children

// Flags
void lv_obj_add_flag(lv_obj_t * obj, lv_obj_flag_t f);
void lv_obj_clear_flag(lv_obj_t * obj, lv_obj_flag_t f);
bool lv_obj_has_flag(lv_obj_t * obj, lv_obj_flag_t f);

// States
void lv_obj_add_state(lv_obj_t * obj, lv_state_t state);
void lv_obj_clear_state(lv_obj_t * obj, lv_state_t state);
bool lv_obj_has_state(lv_obj_t * obj, lv_state_t state);

// Hierarchy
lv_obj_t * lv_obj_get_parent(lv_obj_t * obj);
lv_obj_t * lv_obj_get_child(lv_obj_t * obj, int32_t id);  // id: 0=first, -1=last
uint32_t lv_obj_get_child_cnt(lv_obj_t * obj);
void lv_obj_set_parent(lv_obj_t * obj, lv_obj_t * parent);
void lv_obj_move_foreground(lv_obj_t * obj);
void lv_obj_move_background(lv_obj_t * obj);

// Positioning and sizing
void lv_obj_set_pos(lv_obj_t * obj, lv_coord_t x, lv_coord_t y);
void lv_obj_set_x(lv_obj_t * obj, lv_coord_t x);
void lv_obj_set_y(lv_obj_t * obj, lv_coord_t y);
void lv_obj_set_size(lv_obj_t * obj, lv_coord_t w, lv_coord_t h);
void lv_obj_set_width(lv_obj_t * obj, lv_coord_t w);
void lv_obj_set_height(lv_obj_t * obj, lv_coord_t h);
void lv_obj_set_content_width(lv_obj_t * obj, lv_coord_t w);
void lv_obj_set_content_height(lv_obj_t * obj, lv_coord_t h);
void lv_obj_set_align(lv_obj_t * obj, lv_align_t align);
void lv_obj_align(lv_obj_t * obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
void lv_obj_align_to(lv_obj_t * obj, const lv_obj_t * base, lv_align_t align,
                     lv_coord_t x_ofs, lv_coord_t y_ofs);

// Scrolling
void lv_obj_set_scroll_dir(lv_obj_t * obj, lv_dir_t dir);
void lv_obj_set_scroll_snap_x(lv_obj_t * obj, lv_scroll_snap_t align);
void lv_obj_set_scroll_snap_y(lv_obj_t * obj, lv_scroll_snap_t align);
void lv_obj_scroll_by(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_anim_enable_t anim_en);
void lv_obj_scroll_to(lv_obj_t * obj, lv_coord_t x, lv_coord_t y, lv_anim_enable_t anim_en);
void lv_obj_scroll_to_view(lv_obj_t * obj, lv_anim_enable_t anim_en);

// Screen management
lv_obj_t * lv_scr_act(void);                           // Get active screen
void lv_scr_load(lv_obj_t * scr);                      // Load screen immediately
void lv_scr_load_anim(lv_obj_t * scr, lv_scr_load_anim_t anim,
                      uint32_t time, uint32_t delay, bool auto_del);
lv_obj_t * lv_layer_top(void);                         // Get top layer
lv_obj_t * lv_layer_sys(void);                         // Get system layer
```

### Object Flags Reference

| Flag | Description |
|------|-------------|
| `LV_OBJ_FLAG_HIDDEN` | Object is invisible and does not receive events |
| `LV_OBJ_FLAG_CLICKABLE` | Object can be clicked by input devices |
| `LV_OBJ_FLAG_CLICK_FOCUSABLE` | Add focused state when clicked |
| `LV_OBJ_FLAG_CHECKABLE` | Toggle checked state on click |
| `LV_OBJ_FLAG_SCROLLABLE` | Object content is scrollable |
| `LV_OBJ_FLAG_SCROLL_ELASTIC` | Elastic overscroll effect |
| `LV_OBJ_FLAG_SCROLL_MOMENTUM` | Inertial scrolling after swipe |
| `LV_OBJ_FLAG_SCROLL_ONE` | Scroll only one snap-point child at a time |
| `LV_OBJ_FLAG_SCROLL_CHAIN_HOR` | Propagate horizontal scroll to parent |
| `LV_OBJ_FLAG_SCROLL_CHAIN_VER` | Propagate vertical scroll to parent |
| `LV_OBJ_FLAG_SCROLL_CHAIN` | Both HOR and VER chaining |
| `LV_OBJ_FLAG_SCROLL_ON_FOCUS` | Auto-scroll to make focused child visible |
| `LV_OBJ_FLAG_SNAP_DIR` | Snappable by parent scroll snap |
| `LV_OBJ_FLAG_PRESS_LOCK` | Keep pressed state even if press slides off |
| `LV_OBJ_FLAG_EVENT_BUBBLE` | Propagate events to parent |
| `LV_OBJ_FLAG_GESTURE_BUBBLE` | Propagate gestures to parent |
| `LV_OBJ_FLAG_ADV_HITTEST` | Use advanced hit testing (non-rectangular) |
| `LV_OBJ_FLAG_IGNORE_LAYOUT` | Exempt from parent layout engine |
| `LV_OBJ_FLAG_FLOATING` | Do not scroll with parent, ignore layout |
| `LV_OBJ_FLAG_OVERFLOW_VISIBLE` | Allow children to render outside parent bounds (NEW in v8.2) |
| `LV_OBJ_FLAG_LAYOUT_1` | Custom layout flag 1 |
| `LV_OBJ_FLAG_LAYOUT_2` | Custom layout flag 2 |
| `LV_OBJ_FLAG_WIDGET_1` | Custom widget flag 1 |
| `LV_OBJ_FLAG_WIDGET_2` | Custom widget flag 2 |
| `LV_OBJ_FLAG_USER_1` through `LV_OBJ_FLAG_USER_4` | User-defined flags |

---

## 5. Display and Input Driver API

### Display Driver Registration

```c
// Step 1: Initialize draw buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[DISP_HOR_RES * 10];             // Buffer for 10 rows
static lv_color_t buf2[DISP_HOR_RES * 10];             // Optional second buffer (DMA)
lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISP_HOR_RES * 10);

// Step 2: Initialize and configure display driver
static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);                           // Set defaults
disp_drv.hor_res = 320;                                // Horizontal resolution
disp_drv.ver_res = 240;                                // Vertical resolution
disp_drv.draw_buf = &draw_buf;                         // Assign draw buffer
disp_drv.flush_cb = my_flush_cb;                       // MANDATORY: flush callback

// Step 3: Register
lv_disp_t * disp = lv_disp_drv_register(&disp_drv);   // Returns display handle
```

#### lv_disp_drv_t Fields

| Field | Type | Description |
|-------|------|-------------|
| `hor_res` | `lv_coord_t` | Horizontal resolution in pixels |
| `ver_res` | `lv_coord_t` | Vertical resolution in pixels |
| `draw_buf` | `lv_disp_draw_buf_t *` | Pointer to initialized draw buffer |
| `flush_cb` | callback | **Required.** Copy pixel buffer to display hardware |
| `rounder_cb` | callback | Adjust invalidated area to hardware requirements |
| `set_px_cb` | callback | Custom pixel format writer (monochrome, grayscale) |
| `monitor_cb` | callback | Performance monitoring (pixels count, elapsed ms) |
| `gpu_fill_cb` | callback | GPU-accelerated area fill |
| `clean_dcache_cb` | callback | Clean data cache before DMA transfer |
| `wait_cb` | callback | Called during wait periods (e.g., for RTOS yield) |
| `rotated` | `lv_disp_rot_t` | Display rotation: NONE, 90, 180, 270 |
| `sw_rotate` | `uint32_t` | 1 = software rotation in LVGL; 0 = hardware rotation |
| `full_refresh` | `uint32_t` | 1 = always redraw entire screen |
| `direct_mode` | `uint32_t` | 1 = draw directly into frame buffer |
| `antialiasing` | `uint32_t` | 1 = enable anti-aliasing |
| `color_chroma_key` | `lv_color_t` | Color treated as transparent |
| `user_data` | `void *` | Custom pointer passed to callbacks |
| `dpi` | `uint32_t` | Dots per inch (affects default sizes) |

#### Flush Callback Signature

```c
void my_flush_cb(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
    // Transfer pixels from color_p to display for area [x1,y1]-[x2,y2]
    int32_t x, y;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            set_pixel(x, y, *color_p);
            color_p++;
        }
    }
    lv_disp_flush_ready(drv);  // MUST be called when transfer complete
}
```

#### Display Runtime Functions

```c
void lv_disp_set_rotation(lv_disp_t * disp, lv_disp_rot_t rotation);
lv_disp_rot_t lv_disp_get_rotation(lv_disp_t * disp);
lv_disp_t * lv_disp_get_default(void);
void lv_disp_set_default(lv_disp_t * disp);
lv_coord_t lv_disp_get_hor_res(lv_disp_t * disp);
lv_coord_t lv_disp_get_ver_res(lv_disp_t * disp);
void lv_disp_set_bg_color(lv_disp_t * disp, lv_color_t color);
void lv_disp_set_bg_image(lv_disp_t * disp, const void * img);
void lv_disp_set_bg_opa(lv_disp_t * disp, lv_opa_t opa);
```

### Input Device Driver Registration

```c
// Initialize and configure
static lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type = LV_INDEV_TYPE_POINTER;      // POINTER, KEYPAD, ENCODER, or BUTTON
indev_drv.read_cb = my_touchpad_read;
lv_indev_t * indev = lv_indev_drv_register(&indev_drv);
```

#### Input Device Types

| Type | Constant | Use Case |
|------|----------|----------|
| Pointer | `LV_INDEV_TYPE_POINTER` | Touchpad, mouse, touch screen |
| Keypad | `LV_INDEV_TYPE_KEYPAD` | Physical keyboard, remote control |
| Encoder | `LV_INDEV_TYPE_ENCODER` | Rotary encoder with push button |
| Button | `LV_INDEV_TYPE_BUTTON` | External hardware buttons mapped to screen coordinates |

#### Read Callback Examples

```c
// Pointer (touch) read callback
void my_touchpad_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    data->point.x = touchpad_x;
    data->point.y = touchpad_y;
    data->state = touchpad_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// Keypad read callback
void my_keypad_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    data->key = last_key;           // LV_KEY_UP, LV_KEY_DOWN, LV_KEY_LEFT, LV_KEY_RIGHT,
                                     // LV_KEY_ENTER, LV_KEY_NEXT, LV_KEY_PREV, LV_KEY_HOME,
                                     // LV_KEY_END, LV_KEY_ESC, LV_KEY_DEL, LV_KEY_BACKSPACE
    data->state = key_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// Encoder read callback
void my_encoder_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    data->enc_diff = encoder_diff;   // Positive = clockwise, negative = counter-clockwise
    data->state = btn_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
```

#### Input Device Runtime Functions

```c
void lv_indev_set_cursor(lv_indev_t * indev, lv_obj_t * cur_obj);
void lv_indev_set_group(lv_indev_t * indev, lv_group_t * group);
void lv_indev_set_button_points(lv_indev_t * indev, const lv_point_t points[]);
void lv_indev_drv_update(lv_indev_t * indev, lv_indev_drv_t * new_drv);
void lv_indev_delete(lv_indev_t * indev);
lv_indev_t * lv_indev_get_next(lv_indev_t * indev);   // NULL to start iteration
```

#### Groups (for Keypad/Encoder navigation)

```c
lv_group_t * group = lv_group_create();
lv_group_add_obj(group, obj);
lv_group_remove_obj(obj);
lv_indev_set_group(encoder_indev, group);
lv_group_focus_next(group);
lv_group_focus_prev(group);
lv_obj_t * lv_group_get_focused(const lv_group_t * group);
```

---

## 6. Theme and Style System

### Style Lifecycle

```c
// Initialize a style (MUST be done before use)
static lv_style_t style;
lv_style_init(&style);

// Set properties
lv_style_set_bg_color(&style, lv_color_hex(0x115588));
lv_style_set_border_width(&style, 2);
lv_style_set_radius(&style, 5);
lv_style_set_text_color(&style, lv_color_white());

// Add to object: lv_obj_add_style(obj, &style, selector)
// selector = LV_PART_xxx | LV_STATE_xxx
lv_obj_add_style(btn, &style, LV_PART_MAIN | LV_STATE_DEFAULT);
lv_obj_add_style(btn, &pressed_style, LV_STATE_PRESSED);

// Remove styles
lv_obj_remove_style(obj, &style, selector);
lv_obj_remove_style_all(obj);

// Local (inline) styles -- no shared style variable needed
lv_obj_set_style_bg_color(obj, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);

// Get computed style value
lv_color_t c = lv_obj_get_style_bg_color(obj, LV_PART_MAIN);
```

### Style States

| State | Value | Description |
|-------|-------|-------------|
| `LV_STATE_DEFAULT` | 0x0000 | Normal/idle state |
| `LV_STATE_CHECKED` | 0x0001 | Toggled or checked |
| `LV_STATE_FOCUSED` | 0x0002 | Focused via keypad or encoder |
| `LV_STATE_FOCUS_KEY` | 0x0004 | Focused specifically via keypad |
| `LV_STATE_EDITED` | 0x0008 | Being edited via encoder |
| `LV_STATE_HOVERED` | 0x0010 | Mouse hover |
| `LV_STATE_PRESSED` | 0x0020 | Being pressed |
| `LV_STATE_SCROLLED` | 0x0040 | Being scrolled |
| `LV_STATE_DISABLED` | 0x0080 | Disabled (grayed out) |
| `LV_STATE_USER_1` | 0x1000 | Custom user state 1 |
| `LV_STATE_USER_2` | 0x2000 | Custom user state 2 |
| `LV_STATE_USER_3` | 0x4000 | Custom user state 3 |
| `LV_STATE_USER_4` | 0x8000 | Custom user state 4 |

### Style Parts

| Part | Description | Used by |
|------|-------------|---------|
| `LV_PART_MAIN` | Primary background rectangle | All widgets |
| `LV_PART_SCROLLBAR` | Scrollbar appearance | Scrollable objects |
| `LV_PART_INDICATOR` | Value indicator | Bar, Slider, Switch, Checkbox |
| `LV_PART_KNOB` | Draggable handle | Slider, Arc, Switch |
| `LV_PART_SELECTED` | Currently selected item | Roller, Dropdown list |
| `LV_PART_ITEMS` | Multiple similar elements | Calendar days, Chart data |
| `LV_PART_TICKS` | Scale tick marks | Meter, Chart |
| `LV_PART_CURSOR` | Cursor/position marker | Text area, Chart |
| `LV_PART_CUSTOM_FIRST` | Base for custom widget parts | User widgets |

### Style Properties Quick Reference

#### Size and Position
`width`, `min_width`, `max_width`, `height`, `min_height`, `max_height`, `x`, `y`, `align`, `transform_width`, `transform_height`, `translate_x`, `translate_y`, `transform_zoom` (256=1x), `transform_angle` (0.1 deg units)

#### Padding
`pad_top`, `pad_bottom`, `pad_left`, `pad_right`, `pad_row`, `pad_column`

#### Background
`bg_color`, `bg_opa`, `bg_grad_color`, `bg_grad_dir` (NONE/HOR/VER), `bg_main_stop`, `bg_grad_stop`, `bg_grad`, `bg_dither_mode`, `bg_img_src`, `bg_img_opa`, `bg_img_recolor`, `bg_img_recolor_opa`, `bg_img_tiled`

#### Border
`border_color`, `border_opa`, `border_width`, `border_side` (TOP/BOTTOM/LEFT/RIGHT/INTERNAL), `border_post`

#### Outline
`outline_width`, `outline_color`, `outline_opa`, `outline_pad`

#### Shadow
`shadow_width`, `shadow_ofs_x`, `shadow_ofs_y`, `shadow_spread`, `shadow_color`, `shadow_opa`

#### Text (inheritable)
`text_color`, `text_opa`, `text_font`, `text_letter_space`, `text_line_space`, `text_decor` (NONE/UNDERLINE/STRIKETHROUGH), `text_align` (LEFT/CENTER/RIGHT/AUTO)

#### Image
`img_opa`, `img_recolor`, `img_recolor_opa`

#### Line
`line_width`, `line_dash_width`, `line_dash_gap`, `line_rounded`, `line_color`, `line_opa`

#### Arc
`arc_width`, `arc_rounded`, `arc_color`, `arc_opa`, `arc_img_src`

#### Miscellaneous
`radius` (px or `LV_RADIUS_CIRCLE`), `clip_corner`, `opa`, `color_filter_dsc`, `color_filter_opa`, `anim_time`, `anim_speed`, `transition`, `blend_mode` (NORMAL/ADDITIVE/SUBTRACTIVE), `layout`, `base_dir` (LTR/RTL/AUTO)

### Style Transitions

```c
// Define which properties to animate
static const lv_style_prop_t props[] = {LV_STYLE_BG_COLOR, LV_STYLE_BG_OPA, 0};

// Create transition descriptor
static lv_style_transition_dsc_t tr;
lv_style_transition_dsc_init(&tr, props, lv_anim_path_ease_in_out,
                             300,    // duration_ms
                             0);     // delay_ms

// Apply to a style
lv_style_set_transition(&style, &tr);
```

### Themes

```c
// Initialize default theme
lv_theme_t * th = lv_theme_default_init(
    disp,                           // Display to apply theme to
    lv_palette_main(LV_PALETTE_BLUE),  // Primary color
    lv_palette_main(LV_PALETTE_CYAN),  // Secondary color
    false,                          // true = dark mode
    LV_FONT_DEFAULT                 // Default font
);
lv_disp_set_theme(disp, th);

// Available themes
// lv_theme_default_init()  -- Full-featured material-like theme
// lv_theme_basic_init()    -- Minimal, low-resource theme
// lv_theme_mono_init()     -- Monochrome displays

// Theme chaining (extend a base theme)
lv_theme_set_parent(new_theme, base_theme);
```

---

## 7. Memory Management

### Built-in Allocator (lv_mem)

LVGL includes a custom memory allocator optimized for small embedded allocations.

```c
// Allocation
void * lv_mem_alloc(size_t size);
void * lv_mem_realloc(void * data, size_t new_size);
void lv_mem_free(void * data);

// Monitoring
lv_mem_monitor_t mon;
lv_mem_monitor(&mon);
// mon.total_size    -- Total pool size
// mon.free_cnt      -- Number of free chunks
// mon.free_size     -- Total free bytes
// mon.free_biggest_size -- Largest contiguous free block
// mon.used_cnt      -- Number of used chunks
// mon.used_pct      -- Used percentage
// mon.frag_pct      -- Fragmentation percentage
```

### Configuration

| Option | Default | Description |
|--------|---------|-------------|
| `LV_MEM_CUSTOM` | 0 | 0 = built-in allocator; 1 = use custom malloc/free |
| `LV_MEM_SIZE` | 48*1024 | Pool size in bytes (minimum 2KB, recommended 48KB+) |
| `LV_MEM_ADR` | 0 | Fixed address for memory pool (0 = auto, can point to external SRAM) |
| `LV_MEM_BUF_MAX_NUM` | 16 | Maximum number of intermediate rendering buffers |
| `LV_MEMCPY_MEMSET_STD` | 0 | 1 = use standard C memcpy/memset (faster on some platforms) |

### Custom Allocator Setup

```c
// In lv_conf.h:
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC   malloc
#define LV_MEM_CUSTOM_FREE    free
#define LV_MEM_CUSTOM_REALLOC realloc
```

### Memory Budget Guidelines

| Component | Typical RAM Usage |
|-----------|------------------|
| LVGL core static | ~2 KB |
| Minimum heap (LV_MEM_SIZE) | 2 KB (bare minimum) |
| Recommended heap | 32-48 KB |
| Display buffer (10 rows @ 320px, 16bpp) | 6.4 KB |
| Display buffer (full frame 320x240, 16bpp) | 150 KB |
| Per widget overhead | 100-300 bytes |
| Style instance | ~40 bytes |
| Animation instance | ~60 bytes |

---

## 8. Configuration Options (lv_conf.h)

### Setup Instructions

1. Copy `lv_conf_template.h` to `lv_conf.h` (place next to `lvgl/` directory)
2. Change the first `#if 0` to `#if 1`
3. Configure options as needed

### Complete Configuration Sections

#### Color Settings

| Option | Default | Values | Description |
|--------|---------|--------|-------------|
| `LV_COLOR_DEPTH` | 16 | 1, 8, 16, 32 | Bits per pixel |
| `LV_COLOR_16_SWAP` | 0 | 0/1 | Swap bytes in RGB565 (needed for SPI displays) |
| `LV_COLOR_SCREEN_TRANSP` | 0 | 0/1 | Enable screen transparency (requires 32-bit color) |
| `LV_COLOR_MIX_ROUND_OFS` | 128 | 0-255 | Color mixing rounding offset |
| `LV_COLOR_CHROMA_KEY` | `0x00ff00` | hex | Transparent color key (green by default) |

#### HAL Settings

| Option | Default | Description |
|--------|---------|-------------|
| `LV_DISP_DEF_REFR_PERIOD` | 30 | Display refresh period in ms |
| `LV_INDEV_DEF_READ_PERIOD` | 30 | Input device polling period in ms |
| `LV_TICK_CUSTOM` | 0 | Use custom tick source instead of lv_tick_inc |
| `LV_DPI_DEF` | 130 | Default DPI (affects default widget sizes) |

#### Drawing

| Option | Default | Description |
|--------|---------|-------------|
| `LV_DRAW_COMPLEX` | 1 | Enable shadows, gradients, clip corner, rotation |
| `LV_SHADOW_CACHE_SIZE` | 0 | Cache shadow calculations (bytes) |
| `LV_CIRCLE_CACHE_SIZE` | 4 | Cache circle anti-alias masks |
| `LV_IMG_CACHE_DEF_SIZE` | 0 | Number of cached decoded images |
| `LV_GRADIENT_MAX_STOPS` | 2 | Maximum gradient color stops |
| `LV_GRAD_CACHE_DEF_SIZE` | 0 | Gradient dither map cache (bytes) |
| `LV_DITHER_GRADIENT` | 0 | Enable gradient dithering |
| `LV_DITHER_ERROR_DIFFUSION` | 0 | Error diffusion dithering (higher quality, slower) |
| `LV_DISP_ROT_MAX_BUF` | 10240 | Max buffer for software rotation (bytes) |

#### GPU Acceleration

| Option | Default | Description |
|--------|---------|-------------|
| `LV_USE_GPU_STM32_DMA2D` | 0 | STM32 Chrom-ART (DMA2D) GPU |
| `LV_USE_GPU_NXP_PXP` | 0 | NXP PXP GPU |
| `LV_USE_GPU_NXP_VG_LITE` | 0 | NXP VG-Lite GPU |
| `LV_USE_GPU_SDL` | 0 | SDL GPU rendering |

#### Logging

| Option | Default | Description |
|--------|---------|-------------|
| `LV_USE_LOG` | 0 | Enable logging module |
| `LV_LOG_LEVEL` | `LV_LOG_LEVEL_WARN` | TRACE, INFO, WARN, ERROR, USER, NONE |
| `LV_LOG_PRINTF` | 0 | Use printf for log output |
| `LV_LOG_USE_TIMESTAMP` | 1 | Include timestamps in logs |

#### Debugging

| Option | Default | Description |
|--------|---------|-------------|
| `LV_USE_ASSERT_NULL` | 1 | Check for NULL pointers |
| `LV_USE_ASSERT_MALLOC` | 1 | Check malloc success |
| `LV_USE_ASSERT_STYLE` | 0 | Validate style initialization |
| `LV_USE_ASSERT_MEM_INTEGRITY` | 0 | Check memory integrity on critical ops |
| `LV_USE_ASSERT_OBJ` | 0 | Check object type validity |
| `LV_USE_PERF_MONITOR` | 0 | Show FPS/CPU overlay |
| `LV_USE_MEM_MONITOR` | 0 | Show memory usage overlay |
| `LV_USE_REFR_DEBUG` | 0 | Highlight redrawn areas |

#### Font Configuration

| Option | Default | Description |
|--------|---------|-------------|
| `LV_FONT_MONTSERRAT_8` .. `_48` | varies | Built-in Montserrat fonts (14 enabled by default) |
| `LV_FONT_DEFAULT` | `&lv_font_montserrat_14` | Default font pointer |
| `LV_FONT_FMT_TXT_LARGE` | 0 | Support fonts >16KB |
| `LV_USE_FONT_COMPRESSED` | 0 | Compressed font glyph support |
| `LV_USE_FONT_SUBPX` | 0 | Sub-pixel rendering |
| `LV_FONT_SUBPX_BGR` | 0 | BGR sub-pixel order |

#### Text

| Option | Default | Description |
|--------|---------|-------------|
| `LV_TXT_ENC` | `LV_TXT_ENC_UTF8` | UTF8 or ASCII encoding |
| `LV_TXT_BREAK_CHARS` | `" ,.;:-_"` | Characters for word wrap |
| `LV_TXT_LINE_BREAK_LONG_LEN` | 0 | Break long words (0 = disabled) |
| `LV_USE_BIDI` | 0 | Bidirectional text support |
| `LV_USE_ARABIC_PERSIAN_CHARS` | 0 | Arabic/Persian character shaping |

#### Widget Enables

All core widgets are enabled by default. Extra widgets can be toggled:

```c
// Core (all 1 by default)
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TABLE      1
#define LV_USE_TEXTAREA   1

// Extra (all 1 by default, disable to save flash)
#define LV_USE_ANIMIMG    1
#define LV_USE_CALENDAR   1
#define LV_USE_CHART      1
#define LV_USE_COLORWHEEL 1
#define LV_USE_IMGBTN     1
#define LV_USE_KEYBOARD   1
#define LV_USE_LED        1
#define LV_USE_LIST       1
#define LV_USE_MENU       1    // NEW in v8.2
#define LV_USE_METER      1
#define LV_USE_MSGBOX     1
#define LV_USE_SPAN       1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_TABVIEW    1
#define LV_USE_TILEVIEW   1
#define LV_USE_WIN        1
```

#### Themes and Layouts

```c
#define LV_USE_THEME_DEFAULT     1
#define LV_THEME_DEFAULT_DARK    0      // 0 = light, 1 = dark
#define LV_THEME_DEFAULT_GROW    1      // Grow effect on press
#define LV_THEME_DEFAULT_TRANSITION_TIME 80  // ms

#define LV_USE_THEME_BASIC       1      // Minimal theme
#define LV_USE_THEME_MONO        1      // Monochrome theme

#define LV_USE_FLEX              1      // Flexbox layout
#define LV_USE_GRID              1      // CSS Grid layout
```

#### File System and Decoders

```c
#define LV_USE_FS_STDIO   0    // C stdio file access
#define LV_USE_FS_POSIX   0    // POSIX file access
#define LV_USE_FS_WIN32   0    // Windows file access
#define LV_USE_FS_FATFS   0    // FatFS file access

#define LV_USE_PNG        0    // PNG decoder
#define LV_USE_BMP        0    // BMP decoder
#define LV_USE_SJPG       0    // Split-JPEG decoder
#define LV_USE_GIF        0    // GIF decoder
#define LV_USE_QRCODE     0    // QR code generator
#define LV_USE_FREETYPE   0    // FreeType font engine
#define LV_USE_RLOTTIE    0    // Lottie animation
#define LV_USE_FFMPEG     0    // FFmpeg video/image (NEW in v8.2)
```

#### Miscellaneous Features

```c
#define LV_USE_SNAPSHOT   0    // Object screenshot to buffer (NEW in v8.2)
#define LV_USE_MONKEY     0    // Automated stress testing (NEW in v8.2)
#define LV_USE_GRIDNAV    0    // Grid-based navigation (NEW in v8.2)
#define LV_BUILD_EXAMPLES 1    // Compile built-in examples
```

---

## 9. ESP32 Specific Notes

### Recommended ESP32 Variants for LVGL

| Variant | SRAM | PSRAM | Best For |
|---------|------|-------|----------|
| ESP32 (original) | 520 KB | Up to 4 MB (SPI) | Basic displays, low-cost |
| ESP32-S2 | 320 KB | Up to 4 MB (SPI) | USB displays, cost-sensitive |
| ESP32-S3 | 512 KB | Up to 8 MB (Octal) | High performance, large displays |
| ESP32-C3 | 400 KB | None | Ultra-low-cost, simple UIs |

### SPI Display Driver Integration

```c
// ESP-IDF SPI display with LVGL v8.2
// Common displays: ILI9341, ST7789, ST7796, ILI9488

#include "driver/spi_master.h"

// SPI bus configuration
spi_bus_config_t buscfg = {
    .mosi_io_num = GPIO_MOSI,
    .miso_io_num = GPIO_MISO,
    .sclk_io_num = GPIO_SCLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = DISP_HOR_RES * 40 * sizeof(lv_color_t),
};

// Recommended SPI clock speeds
// ILI9341: up to 40 MHz (read), 80 MHz (write)
// ST7789:  up to 80 MHz
// ST7796:  up to 80 MHz

// LVGL flush callback with ESP-IDF SPI
void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

    // Set display window
    lcd_set_window(area->x1, area->y1, area->x2, area->y2);

    // DMA transfer
    spi_transaction_t trans = {
        .length = size * sizeof(lv_color_t) * 8,  // bits
        .tx_buffer = color_p,
    };
    spi_device_queue_trans(spi_handle, &trans, portMAX_DELAY);

    lv_disp_flush_ready(drv);
}
```

### I2C Display Driver Integration

```c
// Common I2C displays: SSD1306 (128x64 OLED), SH1106
// I2C clock: typically 400 kHz (fast mode)

// For monochrome I2C displays, use set_px_cb
void my_set_px_cb(lv_disp_drv_t * drv, uint8_t * buf, lv_coord_t buf_w,
                  lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa)
{
    uint16_t byte_idx = x + (y >> 3) * buf_w;
    uint8_t bit_idx = y & 0x7;
    if (lv_color_brightness(color) > 128) {
        buf[byte_idx] |= (1 << bit_idx);
    } else {
        buf[byte_idx] &= ~(1 << bit_idx);
    }
}
```

### DMA Considerations

| Scenario | Recommendation |
|----------|---------------|
| Internal SRAM buffer | DMA works directly -- best performance |
| PSRAM buffer with SPI DMA | SPI DMA cannot access PSRAM directly; use bounce buffer |
| PSRAM with GDMA (ESP32-S3) | Set `bounce_buffer_size_px` in esp_lcd panel config |
| Buffer size with DMA | Use double-buffering: 2 buffers, each 10-20 rows |

**Critical**: `LV_COLOR_16_SWAP` must be set to `1` for SPI displays using big-endian byte order (most SPI LCDs).

### PSRAM Usage

```c
// Allocate LVGL draw buffers in PSRAM (ESP-IDF)
lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(
    DISP_HOR_RES * 40 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

// Route LVGL heap to PSRAM via custom allocator
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE "esp_heap_caps.h"
// Then implement wrappers using heap_caps_malloc(size, MALLOC_CAP_SPIRAM)

// WARNING: PSRAM is ~3-10x slower than internal SRAM
// Keep display buffers in internal SRAM when possible
// Use PSRAM for: image assets, fonts, large data structures
```

### Performance Optimization for ESP32

| Setting | Recommendation |
|---------|---------------|
| CPU frequency | 240 MHz (`CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240`) |
| Flash frequency | 80-120 MHz QIO mode |
| `LV_DISP_DEF_REFR_PERIOD` | 10 ms (for smooth animations) |
| Display buffer | 10-25% of screen in internal SRAM |
| Compiler optimization | Performance (`CONFIG_COMPILER_OPTIMIZATION_PERF`) |
| LVGL fast memory | `CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM` |
| Main task core | CPU1 on dual-core chips |
| `LV_MEMCPY_MEMSET_STD` | 1 (use ESP-IDF optimized memcpy, ~1-3 FPS gain) |
| Double buffer + DMA | Enables parallel rendering and transfer |

### Typical Performance (ESP32-S3)

| Display Interface | Default FPS | Optimized FPS |
|-------------------|------------|---------------|
| SPI (320x240) | 15-20 | 30-45 |
| Parallel 8080 | 17 | 41 |
| RGB LCD | 12 | 16 |
| PSRAM without DMA | ~12 | ~12 (bandwidth-limited) |

### ESP32 lv_conf.h Recommendations

```c
#define LV_COLOR_DEPTH     16          // RGB565 for most SPI/parallel LCDs
#define LV_COLOR_16_SWAP   1           // Required for SPI displays
#define LV_MEM_SIZE        (48 * 1024) // 48 KB minimum, more if PSRAM available
#define LV_DPI_DEF         130         // Typical for 2.4"-3.5" displays
#define LV_DISP_DEF_REFR_PERIOD 10     // 10ms for smooth animations
#define LV_INDEV_DEF_READ_PERIOD 15    // 15ms touch polling
#define LV_DRAW_COMPLEX    1           // Keep enabled unless extremely RAM-constrained
#define LV_USE_PERF_MONITOR 1          // Enable during development
#define LV_USE_MEM_MONITOR  1          // Enable during development
```

---

## 10. Known Limitations and Bugs Fixed

### Bugs Fixed in v8.2.0 (100+ fixes from v8.1.0)

#### Rendering Fixes
- Gradient alignment issues resolved
- Canvas rendering corrections
- Indexed image drawing fixes
- Border rendering crash fix
- Arc drawing memory leak eliminated
- Snapshot memory leak fixed

#### Widget Fixes
- Roller text clipping corrected
- Dropdown crash on rapid interaction fixed
- Label dot truncation rendering fixed
- Switch size calculation corrected
- Button matrix button persistence issues resolved
- Table widget memory leak fixed
- Chart widget memory leak fixed

#### Platform Fixes
- ESP32 build system issues resolved
- RT-Thread compatibility improvements
- Various build system fixes for CMake, Keil, and GCC

### Known Limitations of v8.2.x

| Limitation | Description | Workaround |
|-----------|-------------|------------|
| No hardware GPU for ESP32 | ESP32 has no 2D GPU; all rendering is software | Use DMA for SPI transfer, double-buffer |
| PSRAM + DMA conflict | SPI DMA cannot read from PSRAM on ESP32/S2 | Use bounce buffer or internal SRAM for display buffers |
| Software rotation cost | `sw_rotate` copies entire buffer, high CPU | Use hardware rotation in display controller when possible |
| Gradient dithering RAM | Error diffusion dithering needs extra line buffer | Use ordered dithering for lower RAM cost |
| Image cache disabled by default | `LV_IMG_CACHE_DEF_SIZE` = 0, repeated decoding | Set cache size >= number of unique images on screen |
| No partial font loading | Entire font is loaded into memory | Use smaller font subsets, binary font files |
| Single-thread rendering | LVGL is not thread-safe for concurrent drawing | Use mutex around `lv_timer_handler()` and all LVGL API calls |
| Max 2 gradient stops | `LV_GRADIENT_MAX_STOPS` = 2 by default | Increase in lv_conf.h (uses more RAM) |

---

## 11. Breaking Changes from v8.1 to v8.2

### API Breaking Changes

| Change | Migration |
|--------|-----------|
| `lv_fs_read()` signature changed | File system caching added; update `lv_fs_drv_t.read_cb` signature |
| `lv_spangroup_get_expand_width()` | Now requires an additional parameter (max width) |
| Draw architecture refactored | Custom draw backends must implement new abstract draw interface; `lv_draw_ctx_t` introduced |

### Structural Changes

| Change | Impact |
|--------|--------|
| `env_support/` directory created | RT-Thread, CMake, and Zephyr configs moved from root; update include paths |
| New draw context architecture | Internal draw functions receive `lv_draw_ctx_t *` instead of direct buffer access |
| `lv_draw_sw_ctx_t` introduced | Software renderer now wrapped in context structure |

### Migration Checklist (v8.1 to v8.2)

1. Update file system driver callbacks if using `lv_fs` -- `read_cb` signature changed
2. Update any calls to `lv_spangroup_get_expand_width()` to include the new parameter
3. If using custom draw backends, refactor to implement `lv_draw_ctx_t` interface
4. Update build system include paths for `env_support/` relocation
5. Review any code that directly accesses draw buffers (now behind draw context abstraction)
6. Set `LV_COLOR_16_SWAP = 1` if using SPI displays (was already needed but now more critical with draw refactoring)

---

## Quick Reference: Common Patterns

### Minimal LVGL Application

```c
#include "lvgl.h"

// Display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 10];

void my_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color) {
    // ... send pixels to display ...
    lv_disp_flush_ready(drv);
}

void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    data->point.x = touch_x;
    data->point.y = touch_y;
    data->state = touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void app_main(void) {
    lv_init();

    // Display
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 10);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = my_flush;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    // Touch input
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);

    // Create UI
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Hello LVGL!");

    // Main loop
    while (1) {
        lv_timer_handler();
        usleep(5000);  // 5ms
    }
}
```

### Thread Safety (FreeRTOS/ESP-IDF)

```c
static SemaphoreHandle_t lvgl_mutex;

void lvgl_lock(void) { xSemaphoreTake(lvgl_mutex, portMAX_DELAY); }
void lvgl_unlock(void) { xSemaphoreGive(lvgl_mutex); }

// In LVGL task:
void lvgl_task(void *arg) {
    while (1) {
        lvgl_lock();
        lv_timer_handler();
        lvgl_unlock();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// When updating UI from another task:
lvgl_lock();
lv_label_set_text(label, "Updated!");
lvgl_unlock();
```

---

## Related Files

- [API Reference (detailed)](./api-reference.md) -- Comprehensive function signatures for widgets, styles, events, animations, and layouts
