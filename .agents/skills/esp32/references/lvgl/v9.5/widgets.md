# LVGL v9.5 Widget Catalog

> Complete widget reference for LVGL v9.5.x with properties, styles, events, and usage examples.
> Official docs: https://docs.lvgl.io/9.5/widgets/index.html

---

## Table of Contents

1. [Base Object (lv_obj)](#1-base-object-lv_obj)
2. [3D Texture (lv_3dtexture)](#2-3d-texture-lv_3dtexture)
3. [Animation Image (lv_animimg)](#3-animation-image-lv_animimg)
4. [Arc (lv_arc)](#4-arc-lv_arc)
5. [Arc Label (lv_arclabel)](#5-arc-label-lv_arclabel)
6. [Bar (lv_bar)](#6-bar-lv_bar)
7. [Button (lv_button)](#7-button-lv_button)
8. [Button Matrix (lv_buttonmatrix)](#8-button-matrix-lv_buttonmatrix)
9. [Calendar (lv_calendar)](#9-calendar-lv_calendar)
10. [Canvas (lv_canvas)](#10-canvas-lv_canvas)
11. [Chart (lv_chart)](#11-chart-lv_chart)
12. [Checkbox (lv_checkbox)](#12-checkbox-lv_checkbox)
13. [Dropdown (lv_dropdown)](#13-dropdown-lv_dropdown)
14. [GIF (lv_gif)](#14-gif-lv_gif)
15. [Image (lv_image)](#15-image-lv_image)
16. [Image Button (lv_imagebutton)](#16-image-button-lv_imagebutton)
17. [Keyboard (lv_keyboard)](#17-keyboard-lv_keyboard)
18. [Label (lv_label)](#18-label-lv_label)
19. [LED (lv_led)](#19-led-lv_led)
20. [Line (lv_line)](#20-line-lv_line)
21. [List (lv_list)](#21-list-lv_list)
22. [Lottie (lv_lottie)](#22-lottie-lv_lottie)
23. [Menu (lv_menu)](#23-menu-lv_menu)
24. [Message Box (lv_msgbox)](#24-message-box-lv_msgbox)
25. [Pinyin IME (lv_ime_pinyin)](#25-pinyin-ime-lv_ime_pinyin)
26. [Roller (lv_roller)](#26-roller-lv_roller)
27. [Scale (lv_scale)](#27-scale-lv_scale)
28. [Slider (lv_slider)](#28-slider-lv_slider)
29. [Span Group (lv_spangroup)](#29-span-group-lv_spangroup)
30. [Spinbox (lv_spinbox)](#30-spinbox-lv_spinbox)
31. [Spinner (lv_spinner)](#31-spinner-lv_spinner)
32. [Switch (lv_switch)](#32-switch-lv_switch)
33. [Table (lv_table)](#33-table-lv_table)
34. [Tab View (lv_tabview)](#34-tab-view-lv_tabview)
35. [Text Area (lv_textarea)](#35-text-area-lv_textarea)
36. [Tile View (lv_tileview)](#36-tile-view-lv_tileview)
37. [Window (lv_win)](#37-window-lv_win)

---

## Common Widget Properties

All widgets inherit from `lv_obj` and share these capabilities:
- Position, size, alignment
- Styles (background, border, outline, shadow, text, etc.)
- Events (click, focus, value change, etc.)
- States (default, pressed, focused, checked, disabled, etc.)
- Flags (clickable, scrollable, hidden, etc.)
- Parent-child tree structure
- Flex and Grid layout participation

### Common Parts

| Part | Description |
|------|-------------|
| `LV_PART_MAIN` | Primary background area |
| `LV_PART_SCROLLBAR` | Scrollbar area |
| `LV_PART_INDICATOR` | Widget-specific indicator |
| `LV_PART_KNOB` | Draggable handle |
| `LV_PART_SELECTED` | Selected item highlight |
| `LV_PART_ITEMS` | Container items |
| `LV_PART_CURSOR` | Text cursor |
| `LV_PART_TICKS` | Scale tick marks |

---

## 1. Base Object (lv_obj)

Foundation for all widgets. Can be used as a container, panel, or card.

**Config:** `LV_USE_OBJ` (always enabled)

**Parts:** `LV_PART_MAIN`, `LV_PART_SCROLLBAR`

**Example:**
```c
lv_obj_t * panel = lv_obj_create(lv_screen_active());
lv_obj_set_size(panel, 200, 150);
lv_obj_center(panel);
lv_obj_set_style_bg_color(panel, lv_palette_main(LV_PALETTE_BLUE), 0);
lv_obj_set_style_radius(panel, 10, 0);
lv_obj_set_style_shadow_width(panel, 10, 0);  // Drop shadow (v9.5)
lv_obj_set_style_shadow_opa(panel, LV_OPA_30, 0);
```

---

## 2. 3D Texture (lv_3dtexture)

Renders 3D content using glTF models. New in v9 series.

**Config:** `LV_USE_3DTEXTURE` (default: 0, must enable)

**Parts:** `LV_PART_MAIN`

**Key API:**
```c
lv_obj_t * tex = lv_3dtexture_create(parent);
lv_3dtexture_set_src(tex, "A:model.glb");

// Runtime glTF manipulation (new in v9.5)
// Access and modify nodes: scale, rotation, translation, animation state
```

---

## 3. Animation Image (lv_animimg)

Displays a sequence of images as animation frames.

**Config:** `LV_USE_ANIMIMG`

**Parts:** `LV_PART_MAIN`

**Key API:**
```c
lv_obj_t * animimg = lv_animimg_create(parent);
lv_animimg_set_src(animimg, (const void **) img_srcs, frame_count);
lv_animimg_set_duration(animimg, 1000);  // ms
lv_animimg_set_repeat_count(animimg, LV_ANIM_REPEAT_INFINITE);
lv_animimg_start(animimg);
```

---

## 4. Arc (lv_arc)

Circular arc with background arc, indicator arc, and adjustable knob.

**Config:** `LV_USE_ARC`

**Parts:** `LV_PART_MAIN` (background arc), `LV_PART_INDICATOR` (active arc), `LV_PART_KNOB`

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * arc = lv_arc_create(parent);
lv_arc_set_range(arc, 0, 100);
lv_arc_set_value(arc, 75);
lv_arc_set_bg_angles(arc, 0, 360);        // Background arc range
lv_arc_set_angles(arc, 0, 270);           // Indicator arc range
lv_arc_set_rotation(arc, 135);            // Start angle offset
lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL); // NORMAL, REVERSE, SYMMETRICAL
lv_arc_set_change_rate(arc, 5);           // Animation speed for value changes
int32_t val = lv_arc_get_value(arc);
int32_t angle_start = lv_arc_get_angle_start(arc);
int32_t angle_end = lv_arc_get_angle_end(arc);
```

**Events:** `LV_EVENT_VALUE_CHANGED`

**Style notes:** Arc-specific styles apply: `arc_width`, `arc_rounded`, `arc_color`, `arc_opa`, `arc_image_src`

**Example:**
```c
lv_obj_t * arc = lv_arc_create(lv_screen_active());
lv_arc_set_range(arc, 0, 100);
lv_arc_set_value(arc, 40);
lv_obj_set_size(arc, 150, 150);
lv_obj_center(arc);
lv_obj_add_event_cb(arc, arc_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
```

---

## 5. Arc Label (lv_arclabel)

Renders text along a curved arc path.

**Config:** `LV_USE_ARCLABEL`

**Parts:** `LV_PART_MAIN`

**Key API:**
```c
lv_obj_t * arclabel = lv_arclabel_create(parent);
lv_arclabel_set_text(arclabel, "Hello Arc!");
lv_arclabel_set_radius(arclabel, 100);
lv_arclabel_set_start_angle(arclabel, 180);

// New in v9.5
float angle = lv_arclabel_get_text_angle(arclabel);
lv_arclabel_set_overflow_mode(arclabel, mode);  // Overflow handling
```

---

## 6. Bar (lv_bar)

Horizontal or vertical progress/value indicator.

**Config:** `LV_USE_BAR`

**Parts:** `LV_PART_MAIN` (background), `LV_PART_INDICATOR` (filled portion)

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * bar = lv_bar_create(parent);
lv_bar_set_range(bar, 0, 100);
lv_bar_set_value(bar, 70, LV_ANIM_ON);      // With animation
lv_bar_set_start_value(bar, 20, LV_ANIM_ON); // For range mode
lv_bar_set_mode(bar, LV_BAR_MODE_NORMAL);    // NORMAL, SYMMETRICAL, RANGE
int32_t val = lv_bar_get_value(bar);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (when animated value reaches target)

**Example:**
```c
lv_obj_t * bar = lv_bar_create(lv_screen_active());
lv_obj_set_size(bar, 200, 20);
lv_obj_center(bar);
lv_bar_set_value(bar, 70, LV_ANIM_ON);
lv_obj_set_style_bg_color(bar, lv_palette_lighten(LV_PALETTE_BLUE, 3), LV_PART_MAIN);
lv_obj_set_style_bg_color(bar, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
```

---

## 7. Button (lv_button)

Simple clickable button. Differs from base object with checkable behavior.

**Config:** `LV_USE_BUTTON`

**Parts:** `LV_PART_MAIN`

**Key API:**
```c
lv_obj_t * btn = lv_button_create(parent);
lv_obj_set_size(btn, 120, 50);
// Add label as child
lv_obj_t * label = lv_label_create(btn);
lv_label_set_text(label, "Click me");
lv_obj_center(label);
```

**Events:** `LV_EVENT_CLICKED`, `LV_EVENT_VALUE_CHANGED` (when checkable)

**Example:**
```c
lv_obj_t * btn = lv_button_create(lv_screen_active());
lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
lv_obj_center(btn);
lv_obj_t * label = lv_label_create(btn);
lv_label_set_text(label, "Button");
lv_obj_center(label);
```

---

## 8. Button Matrix (lv_buttonmatrix)

Grid of text buttons from a string array. Memory-efficient for many buttons.

**Config:** `LV_USE_BUTTONMATRIX`

**Parts:** `LV_PART_MAIN` (background), `LV_PART_ITEMS` (individual buttons)

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * btnm = lv_buttonmatrix_create(parent);
static const char * map[] = {"Btn1", "Btn2", "\n", "Btn3", ""};
lv_buttonmatrix_set_map(btnm, map);
lv_buttonmatrix_set_btn_ctrl(btnm, 0, LV_BUTTONMATRIX_CTRL_CHECKABLE);
lv_buttonmatrix_set_btn_ctrl(btnm, 1, LV_BUTTONMATRIX_CTRL_DISABLED);
lv_buttonmatrix_set_one_checked(btnm, true);
lv_buttonmatrix_set_btn_width(btnm, 0, 2);  // Relative width
uint32_t id = lv_buttonmatrix_get_selected_button(btnm);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (param: button index)

**Button control flags:**
```
LV_BUTTONMATRIX_CTRL_HIDDEN
LV_BUTTONMATRIX_CTRL_NO_REPEAT
LV_BUTTONMATRIX_CTRL_DISABLED
LV_BUTTONMATRIX_CTRL_CHECKABLE
LV_BUTTONMATRIX_CTRL_CHECKED
LV_BUTTONMATRIX_CTRL_CLICK_TRIG
LV_BUTTONMATRIX_CTRL_POPOVER
LV_BUTTONMATRIX_CTRL_CUSTOM_1 / _2
```

---

## 9. Calendar (lv_calendar)

Date picker with month navigation.

**Config:** `LV_USE_CALENDAR`

**Sub-configs:**
- `LV_CALENDAR_WEEK_STARTS_MONDAY` - Week start day
- `LV_USE_CALENDAR_HEADER_ARROW` - Arrow-based navigation
- `LV_USE_CALENDAR_HEADER_DROPDOWN` - Dropdown-based navigation
- `LV_USE_CALENDAR_CHINESE` - Chinese calendar support

**Parts:** `LV_PART_MAIN`, `LV_PART_ITEMS` (day cells)

**Key API:**
```c
lv_obj_t * cal = lv_calendar_create(parent);
lv_calendar_set_today_date(cal, 2026, 4, 13);
lv_calendar_set_showed_date(cal, 2026, 4);
static lv_calendar_date_t highlighted[] = {{2026, 4, 15}, {2026, 4, 20}};
lv_calendar_set_highlighted_dates(cal, highlighted, 2);
lv_calendar_header_arrow_create(cal);      // Add arrow header
// or
lv_calendar_header_dropdown_create(cal);   // Add dropdown header
lv_calendar_date_t date;
lv_calendar_get_pressed_date(cal, &date);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (date selected)

---

## 10. Canvas (lv_canvas)

Pixel-level drawing surface backed by a buffer.

**Config:** `LV_USE_CANVAS`

**Parts:** `LV_PART_MAIN`

**Key API:**
```c
// Allocate buffer
static uint8_t buf[LV_CANVAS_BUF_SIZE(200, 200, 16, LV_DRAW_BUF_STRIDE_ALIGN)];
lv_obj_t * canvas = lv_canvas_create(parent);
lv_canvas_set_buffer(canvas, buf, 200, 200, LV_COLOR_FORMAT_RGB565);

// Drawing
lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);
lv_canvas_set_px(canvas, x, y, color, opa);

// Layer drawing
lv_layer_t layer;
lv_canvas_init_layer(canvas, &layer);
// Use lv_draw_* functions on layer
lv_canvas_finish_layer(canvas, &layer);

// New in v9.5: skip invalidation
lv_canvas_set_invalidate_on_set_px(canvas, false);
```

---

## 11. Chart (lv_chart)

Data visualization with line, bar, scatter, and curve types.

**Config:** `LV_USE_CHART`

**Parts:** `LV_PART_MAIN` (background), `LV_PART_ITEMS` (data points/bars), `LV_PART_INDICATOR` (point indicators), `LV_PART_CURSOR` (cursor), `LV_PART_TICKS` (axis ticks)

**Property interface:** Yes (new in v9.5)

**Chart Types:**
```
LV_CHART_TYPE_NONE
LV_CHART_TYPE_LINE
LV_CHART_TYPE_BAR
LV_CHART_TYPE_SCATTER
LV_CHART_TYPE_CURVE    // New in v9.5: Bezier curves (requires VG)
```

**Key API:**
```c
lv_obj_t * chart = lv_chart_create(parent);
lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
lv_chart_set_point_count(chart, 50);
lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, 200);
lv_chart_set_div_line_count(chart, 5, 7);

// Series
lv_chart_series_t * ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED),
                                               LV_CHART_AXIS_PRIMARY_Y);
lv_chart_set_next_value(chart, ser, value);
lv_chart_set_value_by_id(chart, ser, id, value);
lv_chart_set_ext_y_array(chart, ser, array);

// Cursor
lv_chart_cursor_t * cur = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE),
                                               LV_DIR_LEFT | LV_DIR_BOTTOM);
lv_chart_set_cursor_point(chart, cur, ser, point_id);

// Zoom
lv_chart_set_zoom_x(chart, 256);  // 256 = 1x, 512 = 2x
lv_chart_set_zoom_y(chart, 256);

lv_chart_refresh(chart);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (cursor position changed), `LV_EVENT_DRAW_TASK_ADDED`

**Example:**
```c
lv_obj_t * chart = lv_chart_create(lv_screen_active());
lv_obj_set_size(chart, 200, 150);
lv_obj_center(chart);
lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
lv_chart_series_t * ser = lv_chart_add_series(chart,
    lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
for(int i = 0; i < 10; i++) {
    lv_chart_set_next_value(chart, ser, lv_rand(10, 90));
}
```

---

## 12. Checkbox (lv_checkbox)

Toggle checkbox with text label.

**Config:** `LV_USE_CHECKBOX`

**Parts:** `LV_PART_MAIN` (text + background), `LV_PART_INDICATOR` (tick box)

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * cb = lv_checkbox_create(parent);
lv_checkbox_set_text(cb, "Accept terms");
lv_checkbox_set_text_static(cb, "Static text");
bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (toggled)

**Example:**
```c
lv_obj_t * cb = lv_checkbox_create(lv_screen_active());
lv_checkbox_set_text(cb, "Enable feature");
lv_obj_center(cb);
lv_obj_add_event_cb(cb, checkbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
```

---

## 13. Dropdown (lv_dropdown)

Expandable list selection. Requires `LV_USE_LABEL`.

**Config:** `LV_USE_DROPDOWN`

**Parts:** Button: `LV_PART_MAIN`, `LV_PART_INDICATOR` (arrow). List: `LV_PART_MAIN` (list bg), `LV_PART_SELECTED` (selected item)

**Key API:**
```c
lv_obj_t * dd = lv_dropdown_create(parent);
lv_dropdown_set_options(dd, "Option 1\nOption 2\nOption 3");
lv_dropdown_add_option(dd, "Option 4", LV_DROPDOWN_POS_LAST);
lv_dropdown_set_selected(dd, 1);
lv_dropdown_set_dir(dd, LV_DIR_BOTTOM);
lv_dropdown_set_symbol(dd, LV_SYMBOL_DOWN);
lv_dropdown_open(dd);
lv_dropdown_close(dd);
uint32_t sel = lv_dropdown_get_selected(dd);
char buf[64];
lv_dropdown_get_selected_str(dd, buf, sizeof(buf));

// New in v9.5: static/non-static text setter
lv_dropdown_set_options_static(dd, static_options);
```

**Events:** `LV_EVENT_VALUE_CHANGED`, `LV_EVENT_READY` (closed), `LV_EVENT_CANCEL` (closed without change)

---

## 14. GIF (lv_gif)

Animated GIF playback.

**Config:** `LV_USE_GIF`

**Parts:** `LV_PART_MAIN`

**Key API:**
```c
lv_obj_t * gif = lv_gif_create(parent);
lv_gif_set_src(gif, "A:animation.gif");
lv_gif_restart(gif);
lv_gif_pause(gif);
lv_gif_resume(gif);
```

---

## 15. Image (lv_image)

Displays images from file, variable, or symbol.

**Config:** `LV_USE_IMAGE`

**Parts:** `LV_PART_MAIN`

**Key API:**
```c
lv_obj_t * img = lv_image_create(parent);
lv_image_set_src(img, &my_image_dsc);         // From variable
lv_image_set_src(img, "A:path/image.png");     // From file
lv_image_set_src(img, LV_SYMBOL_OK);          // Symbol
lv_image_set_offset_x(img, x);
lv_image_set_offset_y(img, y);
lv_image_set_pivot(img, px, py);
lv_image_set_rotation(img, angle);            // 0.1 degree units
lv_image_set_scale(img, zoom);               // 256 = 1x
lv_image_set_scale_x(img, zoom);
lv_image_set_scale_y(img, zoom);
lv_image_set_antialias(img, true);
lv_image_set_inner_align(img, align);
```

**Supported formats:** PNG, JPEG (with orientation/CMYK in v9.5), BMP, GIF, WebP (new in v9.5), SVG, Lottie

---

## 16. Image Button (lv_imagebutton)

Button that uses separate images for each state.

**Config:** `LV_USE_IMAGEBUTTON`

**Parts:** `LV_PART_MAIN`

**Key API:**
```c
lv_obj_t * imgbtn = lv_imagebutton_create(parent);
lv_imagebutton_set_src(imgbtn, LV_IMAGEBUTTON_STATE_RELEASED, &left, &mid, &right);
lv_imagebutton_set_src(imgbtn, LV_IMAGEBUTTON_STATE_PRESSED, &left_pr, &mid_pr, &right_pr);
lv_imagebutton_set_src(imgbtn, LV_IMAGEBUTTON_STATE_DISABLED, &left_dis, &mid_dis, &right_dis);
lv_imagebutton_set_src(imgbtn, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, ...);
lv_imagebutton_set_src(imgbtn, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, ...);
lv_imagebutton_set_src(imgbtn, LV_IMAGEBUTTON_STATE_CHECKED_DISABLED, ...);
```

---

## 17. Keyboard (lv_keyboard)

On-screen keyboard, typically used with Text Area.

**Config:** `LV_USE_KEYBOARD`

**Parts:** `LV_PART_MAIN`, `LV_PART_ITEMS` (keys)

**Key API:**
```c
lv_obj_t * kb = lv_keyboard_create(parent);
lv_keyboard_set_textarea(kb, textarea);
lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
// Modes: TEXT_LOWER, TEXT_UPPER, SPECIAL, NUMBER
// New in v9.5: control mode button definitions
```

**Events:** `LV_EVENT_VALUE_CHANGED` (key pressed), `LV_EVENT_READY` (OK pressed), `LV_EVENT_CANCEL` (Close pressed)

---

## 18. Label (lv_label)

Text display widget. Required by many other widgets.

**Config:** `LV_USE_LABEL`

**Sub-configs:**
- `LV_LABEL_TEXT_SELECTION` - Text selection support
- `LV_LABEL_LONG_TXT_HINT` - Performance optimization for long text
- `LV_LABEL_WAIT_CHAR_COUNT` - Wait character count

**Parts:** `LV_PART_MAIN`, `LV_PART_SCROLLBAR`, `LV_PART_SELECTED`

**Key API:**
```c
lv_obj_t * label = lv_label_create(parent);
lv_label_set_text(label, "Hello World");
lv_label_set_text_fmt(label, "Value: %d", 42);
lv_label_set_text_static(label, static_text);  // No copy
lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
// Long modes: WRAP, DOT, SCROLL, SCROLL_CIRCULAR, CLIP
lv_label_set_text_selection_start(label, 0);
lv_label_set_text_selection_end(label, 5);
const char * text = lv_label_get_text(label);
```

**Style notes:** Text color, font, letter_space, line_space, text_align, text_decor all apply.

**Example:**
```c
lv_obj_t * label = lv_label_create(lv_screen_active());
lv_label_set_text(label, "Temperature: 24.5 C");
lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_BLUE), 0);
lv_obj_center(label);
```

---

## 19. LED (lv_led)

Visual indicator that simulates an LED light.

**Config:** `LV_USE_LED`

**Parts:** `LV_PART_MAIN`

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * led = lv_led_create(parent);
lv_led_set_color(led, lv_palette_main(LV_PALETTE_RED));
lv_led_set_brightness(led, 150);  // 0-255
lv_led_on(led);    // Full brightness
lv_led_off(led);   // Minimum brightness
lv_led_toggle(led);
```

---

## 20. Line (lv_line)

Draws connected line segments between points.

**Config:** `LV_USE_LINE`

**Parts:** `LV_PART_MAIN`

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
static lv_point_precise_t points[] = {{5, 5}, {70, 70}, {120, 10}, {180, 60}};
lv_obj_t * line = lv_line_create(parent);
lv_line_set_points(line, points, 4);
lv_line_set_y_invert(line, true);  // Invert Y axis
```

**Style notes:** `line_width`, `line_color`, `line_opa`, `line_dash_width`, `line_dash_gap`, `line_rounded`

**Example:**
```c
static lv_point_precise_t points[] = {{0, 0}, {50, 30}, {100, 0}};
lv_obj_t * line = lv_line_create(lv_screen_active());
lv_line_set_points(line, points, 3);
lv_obj_set_style_line_width(line, 3, 0);
lv_obj_set_style_line_color(line, lv_palette_main(LV_PALETTE_BLUE), 0);
lv_obj_set_style_line_rounded(line, true, 0);
lv_obj_center(line);
```

---

## 21. List (lv_list)

Scrollable list with text items and optional icons.

**Config:** `LV_USE_LIST`

**Parts:** `LV_PART_MAIN` (background), `LV_PART_SCROLLBAR`

**Key API:**
```c
lv_obj_t * list = lv_list_create(parent);
lv_list_add_text(list, "Section Header");
lv_obj_t * btn = lv_list_add_button(list, LV_SYMBOL_FILE, "Item 1");
lv_obj_t * btn2 = lv_list_add_button(list, NULL, "No icon item");
const char * text = lv_list_get_button_text(list, btn);
```

---

## 22. Lottie (lv_lottie)

Plays Lottie JSON animations. Requires Canvas and ThorVG.

**Config:** `LV_USE_LOTTIE` (default: 0, requires `LV_USE_CANVAS` + ThorVG)

**Parts:** `LV_PART_MAIN`

**Key API:**
```c
lv_obj_t * lottie = lv_lottie_create(parent);
lv_lottie_set_src_data(lottie, json_data, json_size);
lv_lottie_set_src_file(lottie, "A:anim.json");
lv_lottie_set_draw_buf(lottie, &draw_buf);
```

---

## 23. Menu (lv_menu)

Navigation menu with sidebar and page hierarchy.

**Config:** `LV_USE_MENU`

**Parts:** `LV_PART_MAIN` (background)

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * menu = lv_menu_create(parent);

// Create pages
lv_obj_t * page = lv_menu_page_create(menu, "Page Title");
lv_obj_t * section = lv_menu_section_create(page);
lv_obj_t * cont = lv_menu_cont_create(section);
lv_obj_t * label = lv_label_create(cont);
lv_label_set_text(label, "Menu Item");

// Set pages
lv_menu_set_page(menu, page);
lv_menu_set_sidebar_page(menu, sidebar_page);

// Navigation
lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
lv_menu_set_mode_header(menu, LV_MENU_HEADER_TOP_FIXED);
lv_menu_back(menu);
```

---

## 24. Message Box (lv_msgbox)

Modal dialog with title, message, and buttons.

**Config:** `LV_USE_MSGBOX`

**Parts:** Main container parts

**Key API:**
```c
// v9.5: supports formatted text
static const char * btns[] = {"OK", "Cancel", ""};
lv_obj_t * mbox = lv_msgbox_create(NULL);  // NULL = modal overlay
lv_msgbox_add_title(mbox, "Title");
lv_msgbox_add_text(mbox, "Message body text");
lv_msgbox_add_close_button(mbox);
lv_obj_t * btn = lv_msgbox_add_footer_button(mbox, "OK");
lv_obj_add_event_cb(btn, ok_cb, LV_EVENT_CLICKED, mbox);
lv_msgbox_close(mbox);
```

---

## 25. Pinyin IME (lv_ime_pinyin)

Chinese Pinyin input method editor.

**Config:** `LV_USE_IME_PINYIN`

**Key API:**
```c
lv_obj_t * ime = lv_ime_pinyin_create(parent);
lv_ime_pinyin_set_keyboard(ime, keyboard);
lv_ime_pinyin_set_dict(ime, dict);
```

---

## 26. Roller (lv_roller)

Scrolling drum/wheel selector. Requires `LV_USE_LABEL`.

**Config:** `LV_USE_ROLLER`

**Parts:** `LV_PART_MAIN` (background), `LV_PART_SELECTED` (highlighted row)

**Key API:**
```c
lv_obj_t * roller = lv_roller_create(parent);
lv_roller_set_options(roller, "Option 1\nOption 2\nOption 3", LV_ROLLER_MODE_NORMAL);
// Modes: NORMAL, INFINITE (circular)
lv_roller_set_selected(roller, 1, LV_ANIM_ON);
lv_roller_set_visible_row_count(roller, 3);
uint32_t sel = lv_roller_get_selected(roller);
char buf[32];
lv_roller_get_selected_str(roller, buf, sizeof(buf));
```

**Events:** `LV_EVENT_VALUE_CHANGED`

---

## 27. Scale (lv_scale)

Measurement scale with tick marks and labels.

**Config:** `LV_USE_SCALE`

**Parts:** `LV_PART_MAIN`, `LV_PART_INDICATOR` (needle/section), `LV_PART_ITEMS` (tick labels), `LV_PART_TICKS`

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * scale = lv_scale_create(parent);
lv_scale_set_mode(scale, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
// Modes: HORIZONTAL_TOP, HORIZONTAL_BOTTOM, VERTICAL_LEFT, VERTICAL_RIGHT, ROUND_INNER, ROUND_OUTER
lv_scale_set_range(scale, 0, 100);
lv_scale_set_total_tick_count(scale, 21);
lv_scale_set_major_tick_every(scale, 5);
lv_scale_set_label_show(scale, true);

// Sections (colored ranges)
lv_scale_section_t * section = lv_scale_add_section(scale);
lv_scale_section_set_range(section, 0, 30);
lv_scale_section_set_style(section, LV_PART_INDICATOR, &style_green);
lv_scale_section_set_style(section, LV_PART_ITEMS, &style_green);

// New in v9.5: needle updates on transform
```

---

## 28. Slider (lv_slider)

Draggable value selector. Requires `LV_USE_BAR`.

**Config:** `LV_USE_SLIDER`

**Parts:** `LV_PART_MAIN` (background track), `LV_PART_INDICATOR` (filled track), `LV_PART_KNOB`

**Key API:**
```c
lv_obj_t * slider = lv_slider_create(parent);
lv_slider_set_range(slider, 0, 100);
lv_slider_set_value(slider, 50, LV_ANIM_ON);
lv_slider_set_mode(slider, LV_SLIDER_MODE_NORMAL);
// Modes: NORMAL, SYMMETRICAL, RANGE

// New in v9.5: min/max value setters
lv_slider_set_left_value(slider, 20, LV_ANIM_ON);  // For RANGE mode

int32_t val = lv_slider_get_value(slider);
bool dragging = lv_slider_is_dragged(slider);
```

**Events:** `LV_EVENT_VALUE_CHANGED`

**Example:**
```c
lv_obj_t * slider = lv_slider_create(lv_screen_active());
lv_obj_set_width(slider, 200);
lv_obj_center(slider);
lv_slider_set_range(slider, 0, 100);
lv_slider_set_value(slider, 50, LV_ANIM_OFF);
lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
```

---

## 29. Span Group (lv_spangroup)

Rich text with mixed formatting (different fonts, colors, sizes in one block).

**Config:** `LV_USE_SPAN`

**Sub-config:** `LV_SPAN_SNIPPET_STACK_SIZE` - Span descriptor stack size

**Parts:** `LV_PART_MAIN`

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * spangroup = lv_spangroup_create(parent);
lv_span_t * span1 = lv_spangroup_new_span(spangroup);
lv_span_set_text(span1, "Hello ");
lv_style_set_text_color(&span1->style, lv_palette_main(LV_PALETTE_RED));
lv_style_set_text_font(&span1->style, &lv_font_montserrat_20);

lv_span_t * span2 = lv_spangroup_new_span(spangroup);
lv_span_set_text(span2, "World!");
lv_style_set_text_color(&span2->style, lv_palette_main(LV_PALETTE_BLUE));

lv_spangroup_set_align(spangroup, LV_TEXT_ALIGN_CENTER);
lv_spangroup_set_overflow(spangroup, LV_SPAN_OVERFLOW_CLIP);
lv_spangroup_set_mode(spangroup, LV_SPAN_MODE_BREAK);
// Modes: FIXED, BREAK, EXPAND
lv_spangroup_refr_mode(spangroup);
```

---

## 30. Spinbox (lv_spinbox)

Numeric input with increment/decrement buttons.

**Config:** `LV_USE_SPINBOX`

**Parts:** `LV_PART_MAIN`, `LV_PART_CURSOR` (selected digit)

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * spinbox = lv_spinbox_create(parent);
lv_spinbox_set_range(spinbox, -1000, 25000);
lv_spinbox_set_digit_format(spinbox, 5, 2);  // 5 digits, 2 decimal
lv_spinbox_set_step(spinbox, 10);
lv_spinbox_set_value(spinbox, 1234);
lv_spinbox_set_rollover(spinbox, true);
lv_spinbox_increment(spinbox);
lv_spinbox_decrement(spinbox);
int32_t val = lv_spinbox_get_value(spinbox);
```

**Events:** `LV_EVENT_VALUE_CHANGED`

---

## 31. Spinner (lv_spinner)

Loading/activity indicator with rotating arc.

**Config:** `LV_USE_SPINNER`

**Parts:** `LV_PART_MAIN` (background arc), `LV_PART_INDICATOR` (spinning arc)

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * spinner = lv_spinner_create(parent);
lv_spinner_set_anim_params(spinner, 1000, 300);  // spin_time, arc_length
```

**Example:**
```c
lv_obj_t * spinner = lv_spinner_create(lv_screen_active());
lv_obj_set_size(spinner, 80, 80);
lv_obj_center(spinner);
lv_spinner_set_anim_params(spinner, 1200, 240);
lv_obj_set_style_arc_width(spinner, 8, LV_PART_MAIN);
lv_obj_set_style_arc_width(spinner, 8, LV_PART_INDICATOR);
lv_obj_set_style_arc_color(spinner, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
```

---

## 32. Switch (lv_switch)

Toggle on/off control.

**Config:** `LV_USE_SWITCH`

**Parts:** `LV_PART_MAIN` (background), `LV_PART_INDICATOR` (fill when on), `LV_PART_KNOB`

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * sw = lv_switch_create(parent);
// Toggle programmatically
lv_obj_add_state(sw, LV_STATE_CHECKED);
lv_obj_remove_state(sw, LV_STATE_CHECKED);
bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
```

**Events:** `LV_EVENT_VALUE_CHANGED`

**Example:**
```c
lv_obj_t * sw = lv_switch_create(lv_screen_active());
lv_obj_center(sw);
lv_obj_add_event_cb(sw, switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

void switch_event_cb(lv_event_t * e) {
    lv_obj_t * sw = lv_event_get_target(e);
    if(lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        // Turned ON
    } else {
        // Turned OFF
    }
}
```

---

## 33. Table (lv_table)

Data grid with rows and columns.

**Config:** `LV_USE_TABLE`

**Parts:** `LV_PART_MAIN` (background), `LV_PART_ITEMS` (cells)

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * table = lv_table_create(parent);
lv_table_set_row_count(table, 5);
lv_table_set_column_count(table, 3);
lv_table_set_column_width(table, 0, 100);
lv_table_set_cell_value(table, 0, 0, "Name");
lv_table_set_cell_value_fmt(table, 1, 0, "Row %d", 1);
lv_table_add_cell_ctrl(table, 0, 0, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
const char * val = lv_table_get_cell_value(table, row, col);
uint32_t rows = lv_table_get_row_count(table);
uint32_t cols = lv_table_get_column_count(table);
```

**Cell control flags:**
```
LV_TABLE_CELL_CTRL_MERGE_RIGHT
LV_TABLE_CELL_CTRL_TEXT_CROP
LV_TABLE_CELL_CTRL_CUSTOM_1..4
```

**Events:** `LV_EVENT_VALUE_CHANGED` (cell selected), `LV_EVENT_DRAW_TASK_ADDED`

---

## 34. Tab View (lv_tabview)

Tabbed interface with multiple content pages.

**Config:** `LV_USE_TABVIEW`

**Parts:** Tab buttons use buttonmatrix parts

**Property interface:** Yes (new in v9.5)

**Key API:**
```c
lv_obj_t * tabview = lv_tabview_create(parent);
lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Tab 1");
lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "Tab 2");

// Add content to tabs (they are regular lv_obj containers)
lv_obj_t * label = lv_label_create(tab1);
lv_label_set_text(label, "Content of Tab 1");

lv_tabview_set_active(tabview, 0, LV_ANIM_ON);
lv_obj_t * content = lv_tabview_get_content(tabview);
lv_obj_t * btns = lv_tabview_get_tab_bar(tabview);

// Tab bar position
lv_tabview_set_tab_bar_position(tabview, LV_DIR_TOP);  // TOP, BOTTOM, LEFT, RIGHT
lv_tabview_set_tab_bar_size(tabview, 50);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (tab changed)

---

## 35. Text Area (lv_textarea)

Multi-line text input with cursor. Requires `LV_USE_LABEL`.

**Config:** `LV_USE_TEXTAREA`

**Sub-config:** `LV_TEXTAREA_DEF_PWD_SHOW_TIME` - Password reveal timeout (ms)

**Parts:** `LV_PART_MAIN` (background), `LV_PART_SCROLLBAR`, `LV_PART_CURSOR`, `LV_PART_SELECTED`

**Key API:**
```c
lv_obj_t * ta = lv_textarea_create(parent);
lv_textarea_set_text(ta, "Initial text");
lv_textarea_add_char(ta, 'A');
lv_textarea_add_text(ta, "more text");
lv_textarea_delete_char(ta);
lv_textarea_delete_char_forward(ta);
lv_textarea_set_placeholder_text(ta, "Enter value...");
lv_textarea_set_one_line(ta, true);
lv_textarea_set_password_mode(ta, true);
lv_textarea_set_accepted_chars(ta, "0123456789.");
lv_textarea_set_max_length(ta, 64);
lv_textarea_set_cursor_pos(ta, pos);  // LV_TEXTAREA_CURSOR_LAST
lv_textarea_set_text_selection(ta, true);
const char * text = lv_textarea_get_text(ta);

// New in v9.5: static/non-static accepted_chars setter
lv_textarea_set_accepted_chars_static(ta, chars);
```

**Events:** `LV_EVENT_INSERT` (before text insertion), `LV_EVENT_VALUE_CHANGED` (text changed), `LV_EVENT_READY` (enter pressed in one-line mode)

**Example:**
```c
lv_obj_t * ta = lv_textarea_create(lv_screen_active());
lv_textarea_set_one_line(ta, true);
lv_textarea_set_placeholder_text(ta, "Type here...");
lv_obj_set_width(ta, 200);
lv_obj_center(ta);

// Pair with keyboard
lv_obj_t * kb = lv_keyboard_create(lv_screen_active());
lv_keyboard_set_textarea(kb, ta);
```

---

## 36. Tile View (lv_tileview)

Grid-based page navigation with swipe gestures.

**Config:** `LV_USE_TILEVIEW`

**Parts:** `LV_PART_MAIN`, `LV_PART_SCROLLBAR`

**Key API:**
```c
lv_obj_t * tv = lv_tileview_create(parent);
lv_obj_t * tile1 = lv_tileview_add_tile(tv, 0, 0, LV_DIR_RIGHT | LV_DIR_BOTTOM);
lv_obj_t * tile2 = lv_tileview_add_tile(tv, 1, 0, LV_DIR_LEFT);
lv_obj_t * tile3 = lv_tileview_add_tile(tv, 0, 1, LV_DIR_TOP);

// Add content to tiles
lv_obj_t * label = lv_label_create(tile1);
lv_label_set_text(label, "Tile 1");

lv_tileview_set_tile(tv, tile2, LV_ANIM_ON);
lv_obj_t * active = lv_tileview_get_tile_active(tv);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (tile changed)

---

## 37. Window (lv_win)

Container with header bar and content area.

**Config:** `LV_USE_WIN`

**Parts:** `LV_PART_MAIN`

**Key API:**
```c
lv_obj_t * win = lv_win_create(parent);
lv_win_add_title(win, "Window Title");
lv_obj_t * btn = lv_win_add_button(win, LV_SYMBOL_CLOSE, 40);
lv_obj_t * content = lv_win_get_content(win);
lv_obj_t * header = lv_win_get_header(win);

// Add content
lv_obj_t * label = lv_label_create(content);
lv_label_set_text(label, "Window content here");
```

---

## Symbols (Built-in Icons)

Available when using built-in fonts:

```
LV_SYMBOL_AUDIO         LV_SYMBOL_VIDEO
LV_SYMBOL_LIST          LV_SYMBOL_OK
LV_SYMBOL_CLOSE         LV_SYMBOL_POWER
LV_SYMBOL_SETTINGS      LV_SYMBOL_HOME
LV_SYMBOL_DOWNLOAD      LV_SYMBOL_DRIVE
LV_SYMBOL_REFRESH       LV_SYMBOL_MUTE
LV_SYMBOL_VOLUME_MID    LV_SYMBOL_VOLUME_MAX
LV_SYMBOL_IMAGE         LV_SYMBOL_TINT
LV_SYMBOL_PREV          LV_SYMBOL_PLAY
LV_SYMBOL_PAUSE         LV_SYMBOL_STOP
LV_SYMBOL_NEXT          LV_SYMBOL_EJECT
LV_SYMBOL_LEFT          LV_SYMBOL_RIGHT
LV_SYMBOL_PLUS          LV_SYMBOL_MINUS
LV_SYMBOL_EYE_OPEN      LV_SYMBOL_EYE_CLOSE
LV_SYMBOL_WARNING       LV_SYMBOL_SHUFFLE
LV_SYMBOL_UP            LV_SYMBOL_DOWN
LV_SYMBOL_LOOP          LV_SYMBOL_DIRECTORY
LV_SYMBOL_UPLOAD        LV_SYMBOL_CALL
LV_SYMBOL_CUT           LV_SYMBOL_COPY
LV_SYMBOL_SAVE          LV_SYMBOL_BARS
LV_SYMBOL_ENVELOPE      LV_SYMBOL_CHARGE
LV_SYMBOL_PASTE         LV_SYMBOL_BELL
LV_SYMBOL_KEYBOARD      LV_SYMBOL_GPS
LV_SYMBOL_FILE          LV_SYMBOL_WIFI
LV_SYMBOL_BATTERY_FULL  LV_SYMBOL_BATTERY_3
LV_SYMBOL_BATTERY_2     LV_SYMBOL_BATTERY_1
LV_SYMBOL_BATTERY_EMPTY LV_SYMBOL_USB
LV_SYMBOL_BLUETOOTH     LV_SYMBOL_TRASH
LV_SYMBOL_EDIT          LV_SYMBOL_BACKSPACE
LV_SYMBOL_SD_CARD       LV_SYMBOL_NEW_LINE
```

---

## v9.5 Widget Changes Summary

### New Widget Features
- **LV_CHART_TYPE_CURVE** - Bezier curved charts (requires VG)
- **LV_OBJ_FLAG_RADIO_BUTTON** - Native radio button groups
- **LV_STATE_ALT** - Dark/light mode widget state

### Property Interface Added (v9.5)
Consistent get/set property API for: Arc, Bar, Switch, Checkbox, LED, Line, Scale, Spinbox, Spinner, Table, Tabview, Buttonmatrix, Span, Menu, Chart

### Widget-Specific v9.5 Additions
- **Arclabel**: text angle getter, overflow mode
- **Canvas**: invalidation skip API
- **Dropdown**: static/non-static text setter, symbol property fix
- **Msgbox**: formatted text support
- **Scale**: needle updates on transform
- **Slider**: min_value/max_value setters
- **Textarea**: static/non-static accepted_chars setter

### Deprecated (v9.5)
- **lv_fragment** - Plan migration to alternative view lifecycle management

---

## Source Links

- Widgets index: https://docs.lvgl.io/9.5/widgets/index.html
- Common features: https://docs.lvgl.io/9.5/common-widget-features/index.html
- Examples: https://docs.lvgl.io/9.5/getting_started/examples.html
