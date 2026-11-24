# LVGL v8.3 Widget Reference

> Complete widget documentation for LVGL 8.3.x.
> All widgets with their key properties, events, parts, and usage patterns.

---

## Widget Categories

| Category       | Count | Widgets                                                                        |
|----------------|-------|--------------------------------------------------------------------------------|
| Base           | 1     | lv_obj                                                                         |
| Core           | 15    | arc, bar, btn, btnmatrix, canvas, checkbox, dropdown, img, label, line, roller, slider, switch, table, textarea |
| Extra          | 17    | animimg, calendar, chart, colorwheel, imgbtn, keyboard, led, list, menu, meter, msgbox, span, spinbox, spinner, tabview, tileview, win |
| New in 8.3     | 2     | ime_pinyin, fragment (non-widget controller)                                   |

---

## Base Widget

### lv_obj (Base Object)

The foundation for all widgets. Can be used directly as a simple rectangle container.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | Always available                                           |
| **Header**        | `lv_obj.h`                                                 |
| **Create**        | `lv_obj_t * lv_obj_create(lv_obj_t * parent)`              |
| **Default Size**  | `LV_SIZE_CONTENT` or theme-defined                         |

**Parts:**

| Part                | Description                    |
|---------------------|--------------------------------|
| `LV_PART_MAIN`      | Background rectangle           |
| `LV_PART_SCROLLBAR`  | Scrollbar indicator            |

**Key Events:**

| Event                      | Trigger                           |
|----------------------------|-----------------------------------|
| `LV_EVENT_CLICKED`         | Click/tap                         |
| `LV_EVENT_PRESSED`         | Touch down                        |
| `LV_EVENT_SCROLL`          | Scrolling                         |
| `LV_EVENT_VALUE_CHANGED`   | Checkable toggle (with flag)      |

**Usage:**
```c
lv_obj_t * container = lv_obj_create(lv_scr_act());
lv_obj_set_size(container, 200, 100);
lv_obj_center(container);
lv_obj_set_style_bg_color(container, lv_palette_main(LV_PALETTE_BLUE), 0);
```

---

## Core Widgets

### lv_arc (Arc)

Circular arc with adjustable value via knob. Used for progress indicators, loaders, and rotational controls.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_ARC`                                               |
| **Create**        | `lv_obj_t * lv_arc_create(lv_obj_t * parent)`              |
| **Default Range** | 0-100                                                      |

**Parts:**

| Part                | Description                    |
|---------------------|--------------------------------|
| `LV_PART_MAIN`      | Background arc                 |
| `LV_PART_INDICATOR`  | Foreground arc (value)        |
| `LV_PART_KNOB`       | Draggable handle              |

**Key Functions:**
```c
void lv_arc_set_value(lv_obj_t * arc, int16_t value);
void lv_arc_set_range(lv_obj_t * arc, int16_t min, int16_t max);
void lv_arc_set_bg_angles(lv_obj_t * arc, uint16_t start, uint16_t end);
void lv_arc_set_rotation(lv_obj_t * arc, uint16_t rotation);
void lv_arc_set_mode(lv_obj_t * arc, lv_arc_mode_t mode);
void lv_arc_set_change_rate(lv_obj_t * arc, uint16_t rate);  // degrees/sec

// New in 8.3
void lv_arc_align_obj_to_angle(lv_obj_t * arc, lv_obj_t * obj, lv_coord_t r_offset);
void lv_arc_rotate_obj_to_angle(lv_obj_t * arc, lv_obj_t * obj, lv_coord_t r_offset);

int16_t lv_arc_get_value(const lv_obj_t * arc);
int16_t lv_arc_get_min_value(const lv_obj_t * arc);
int16_t lv_arc_get_max_value(const lv_obj_t * arc);
uint16_t lv_arc_get_angle_start(lv_obj_t * arc);
uint16_t lv_arc_get_angle_end(lv_obj_t * arc);
```

**Modes:**

| Mode                       | Description                              |
|----------------------------|------------------------------------------|
| `LV_ARC_MODE_NORMAL`       | Indicator from min to current value      |
| `LV_ARC_MODE_REVERSE`      | Counter-clockwise from max               |
| `LV_ARC_MODE_SYMMETRICAL`  | From midpoint to current value           |

**Events:** `LV_EVENT_VALUE_CHANGED`
**Keys:** `LV_KEY_RIGHT/UP` (increment), `LV_KEY_LEFT/DOWN` (decrement)

**Usage:**
```c
lv_obj_t * arc = lv_arc_create(lv_scr_act());
lv_arc_set_range(arc, 0, 100);
lv_arc_set_value(arc, 75);
lv_arc_set_rotation(arc, 135);
lv_arc_set_bg_angles(arc, 0, 270);
lv_obj_set_size(arc, 150, 150);
lv_obj_center(arc);
```

---

### lv_bar (Bar)

Horizontal or vertical bar indicator showing a value range.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_BAR`                                               |
| **Create**        | `lv_obj_t * lv_bar_create(lv_obj_t * parent)`              |
| **Default Range** | 0-100                                                      |

**Parts:**

| Part                | Description                    |
|---------------------|--------------------------------|
| `LV_PART_MAIN`      | Background bar                 |
| `LV_PART_INDICATOR`  | Filled portion                |

**Key Functions:**
```c
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

**Modes:** `LV_BAR_MODE_NORMAL`, `LV_BAR_MODE_SYMMETRICAL`, `LV_BAR_MODE_RANGE`

**Usage:**
```c
lv_obj_t * bar = lv_bar_create(lv_scr_act());
lv_bar_set_value(bar, 70, LV_ANIM_ON);
lv_obj_set_size(bar, 200, 20);
lv_obj_center(bar);
```

---

### lv_btn (Button)

Simple clickable button. Extends lv_obj with non-scrollable defaults and content-based sizing.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_BTN`                                               |
| **Create**        | `lv_obj_t * lv_btn_create(lv_obj_t * parent)`              |
| **Default Size**  | `LV_SIZE_CONTENT`                                          |

**Parts:** `LV_PART_MAIN` (background)

**Key Events:**

| Event                      | Trigger                           |
|----------------------------|-----------------------------------|
| `LV_EVENT_CLICKED`         | Click/tap completed               |
| `LV_EVENT_VALUE_CHANGED`   | Toggle (with `LV_OBJ_FLAG_CHECKABLE`) |

**Keys:** `LV_KEY_ENTER` (activate)

**Usage:**
```c
lv_obj_t * btn = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn, 120, 50);
lv_obj_center(btn);
lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

lv_obj_t * label = lv_label_create(btn);
lv_label_set_text(label, "Click Me");
lv_obj_center(label);

// Toggle button
lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
```

---

### lv_btnmatrix (Button Matrix)

Memory-efficient virtual button grid (~8 bytes per button vs ~100-150 for real buttons).

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_BTNMATRIX`                                         |
| **Create**        | `lv_obj_t * lv_btnmatrix_create(lv_obj_t * parent)`        |
| **Width Range**   | 1-15 per button (v8.3.7+)                                 |

**Parts:**

| Part                | Description                    |
|---------------------|--------------------------------|
| `LV_PART_MAIN`      | Background (pad_row, pad_column)|
| `LV_PART_ITEMS`      | Individual buttons             |

**Key Functions:**
```c
void lv_btnmatrix_set_map(lv_obj_t * btnm, const char * map[]);
void lv_btnmatrix_set_btn_ctrl(lv_obj_t * btnm, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_clear_btn_ctrl(lv_obj_t * btnm, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_set_btn_ctrl_all(lv_obj_t * btnm, lv_btnmatrix_ctrl_t ctrl);
void lv_btnmatrix_set_btn_width(lv_obj_t * btnm, uint16_t btn_id, uint8_t width);
void lv_btnmatrix_set_one_checked(lv_obj_t * btnm, bool en);
void lv_btnmatrix_set_ctrl_map(lv_obj_t * btnm, const lv_btnmatrix_ctrl_t ctrl_map[]);

uint16_t lv_btnmatrix_get_selected_btn(const lv_obj_t * btnm);
const char * lv_btnmatrix_get_btn_text(const lv_obj_t * btnm, uint16_t btn_id);
bool lv_btnmatrix_has_btn_ctrl(lv_obj_t * btnm, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl);
bool lv_btnmatrix_get_one_checked(const lv_obj_t * btnm);
```

**Control Flags:**

| Flag                                | Description                   |
|-------------------------------------|-------------------------------|
| `LV_BTNMATRIX_CTRL_HIDDEN`          | Hide button (keep space)      |
| `LV_BTNMATRIX_CTRL_NO_REPEAT`       | Disable long-press repeat     |
| `LV_BTNMATRIX_CTRL_DISABLED`        | Disable interaction           |
| `LV_BTNMATRIX_CTRL_CHECKABLE`       | Enable toggle                 |
| `LV_BTNMATRIX_CTRL_CHECKED`         | Set checked state             |
| `LV_BTNMATRIX_CTRL_CLICK_TRIG`      | Trigger on click vs press     |
| `LV_BTNMATRIX_CTRL_POPOVER`         | Show label in popover         |
| `LV_BTNMATRIX_CTRL_RECOLOR`         | Enable text color codes       |
| `LV_BTNMATRIX_CTRL_CUSTOM_1`        | User-defined flag 1           |
| `LV_BTNMATRIX_CTRL_CUSTOM_2`        | User-defined flag 2           |

**Events:** `LV_EVENT_VALUE_CHANGED` (button pressed)

**Usage:**
```c
static const char * map[] = {"Btn1", "Btn2", "Btn3", "\n", "Btn4", "Btn5", ""};
lv_obj_t * btnm = lv_btnmatrix_create(lv_scr_act());
lv_btnmatrix_set_map(btnm, map);
lv_btnmatrix_set_btn_width(btnm, 0, 2);  // First button 2x wide
lv_btnmatrix_set_btn_ctrl(btnm, 3, LV_BTNMATRIX_CTRL_CHECKABLE);
```

---

### lv_canvas (Canvas)

Drawing surface for pixel-level manipulation and rendering primitives.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_CANVAS`                                            |
| **Create**        | `lv_obj_t * lv_canvas_create(lv_obj_t * parent)`           |

**Key Functions:**
```c
void lv_canvas_set_buffer(lv_obj_t * canvas, void * buf, lv_coord_t w, lv_coord_t h,
                          lv_img_cf_t cf);
void lv_canvas_set_px_color(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y, lv_color_t c);
void lv_canvas_set_px_opa(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y, lv_opa_t opa);
lv_color_t lv_canvas_get_px(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y);
lv_img_dsc_t * lv_canvas_get_img(lv_obj_t * canvas);

// Drawing primitives
void lv_canvas_draw_rect(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y,
                         lv_coord_t w, lv_coord_t h, const lv_draw_rect_dsc_t * dsc);
void lv_canvas_draw_text(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y,
                         lv_coord_t max_w, lv_draw_label_dsc_t * dsc, const char * txt);
void lv_canvas_draw_img(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y,
                        const void * src, const lv_draw_img_dsc_t * dsc);
void lv_canvas_draw_line(lv_obj_t * canvas, const lv_point_t points[], uint32_t point_cnt,
                         const lv_draw_line_dsc_t * dsc);
void lv_canvas_draw_polygon(lv_obj_t * canvas, const lv_point_t points[], uint32_t point_cnt,
                            const lv_draw_rect_dsc_t * dsc);
void lv_canvas_draw_arc(lv_obj_t * canvas, lv_coord_t x, lv_coord_t y, lv_coord_t r,
                        int32_t start_angle, int32_t end_angle, const lv_draw_arc_dsc_t * dsc);

// Transform
void lv_canvas_copy_buf(lv_obj_t * canvas, const void * to_copy,
                        lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h);
void lv_canvas_transform(lv_obj_t * canvas, lv_img_dsc_t * img, int16_t angle,
                         uint16_t zoom, lv_coord_t offset_x, lv_coord_t offset_y,
                         int32_t pivot_x, int32_t pivot_y, bool antialias);
void lv_canvas_blur_hor(lv_obj_t * canvas, const lv_area_t * area, uint16_t r);
void lv_canvas_blur_ver(lv_obj_t * canvas, const lv_area_t * area, uint16_t r);
void lv_canvas_fill_bg(lv_obj_t * canvas, lv_color_t color, lv_opa_t opa);
```

---

### lv_checkbox (Checkbox)

Toggle checkbox with integrated label text.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_CHECKBOX`                                          |
| **Create**        | `lv_obj_t * lv_checkbox_create(lv_obj_t * parent)`         |

**Parts:**

| Part                | Description                    |
|---------------------|--------------------------------|
| `LV_PART_MAIN`      | Label text area                |
| `LV_PART_INDICATOR`  | Checkbox square                |

**Key Functions:**
```c
void lv_checkbox_set_text(lv_obj_t * cb, const char * txt);
void lv_checkbox_set_text_static(lv_obj_t * cb, const char * txt);
const char * lv_checkbox_get_text(const lv_obj_t * cb);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (toggled)
**State:** Use `lv_obj_has_state(cb, LV_STATE_CHECKED)` to read

**Usage:**
```c
lv_obj_t * cb = lv_checkbox_create(lv_scr_act());
lv_checkbox_set_text(cb, "Enable WiFi");
lv_obj_add_event_cb(cb, checkbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
```

---

### lv_dropdown (Drop-down List)

Expandable selection list.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_DROPDOWN`                                          |
| **Create**        | `lv_obj_t * lv_dropdown_create(lv_obj_t * parent)`         |

**Parts (button):** `LV_PART_MAIN`, `LV_PART_INDICATOR`
**Parts (list):** `LV_PART_MAIN`, `LV_PART_SCROLLBAR`, `LV_PART_SELECTED`

**Key Functions:**
```c
void lv_dropdown_set_options(lv_obj_t * dd, const char * options);  // "\n" separated
void lv_dropdown_set_options_static(lv_obj_t * dd, const char * options);
void lv_dropdown_add_option(lv_obj_t * dd, const char * option, uint32_t pos);
void lv_dropdown_clear_options(lv_obj_t * dd);
void lv_dropdown_set_selected(lv_obj_t * dd, uint16_t sel_opt);
void lv_dropdown_set_dir(lv_obj_t * dd, lv_dir_t dir);
void lv_dropdown_set_symbol(lv_obj_t * dd, const void * symbol);
void lv_dropdown_set_text(lv_obj_t * dd, const char * txt);
void lv_dropdown_open(lv_obj_t * dd);
void lv_dropdown_close(lv_obj_t * dd);

uint16_t lv_dropdown_get_selected(const lv_obj_t * dd);
void lv_dropdown_get_selected_str(const lv_obj_t * dd, char * buf, uint32_t buf_size);
uint16_t lv_dropdown_get_option_cnt(const lv_obj_t * dd);
lv_obj_t * lv_dropdown_get_list(lv_obj_t * dd);

// New in 8.3
uint16_t lv_dropdown_get_option_index(lv_obj_t * dd, const char * option);
```

**Events:** `LV_EVENT_VALUE_CHANGED`, `LV_EVENT_READY` (opened), `LV_EVENT_CANCEL` (closed)
**Keys:** Arrows navigate, `LV_KEY_ENTER` confirms

**Usage:**
```c
lv_obj_t * dd = lv_dropdown_create(lv_scr_act());
lv_dropdown_set_options(dd, "Option 1\nOption 2\nOption 3");
lv_dropdown_set_selected(dd, 1);
lv_obj_set_width(dd, 150);
```

---

### lv_img (Image)

Display images from C arrays or files.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_IMG`                                               |
| **Create**        | `lv_obj_t * lv_img_create(lv_obj_t * parent)`              |

**Key Functions:**
```c
void lv_img_set_src(lv_obj_t * img, const void * src);  // C array, file path, or symbol
void lv_img_set_offset_x(lv_obj_t * img, lv_coord_t x);
void lv_img_set_offset_y(lv_obj_t * img, lv_coord_t y);
void lv_img_set_angle(lv_obj_t * img, int16_t angle);    // 0.1 degree units
void lv_img_set_zoom(lv_obj_t * img, uint16_t zoom);     // 256 = 1x
void lv_img_set_pivot(lv_obj_t * img, lv_coord_t x, lv_coord_t y);
void lv_img_set_antialias(lv_obj_t * img, bool antialias);
void lv_img_set_size_mode(lv_obj_t * img, lv_img_size_mode_t mode);

const void * lv_img_get_src(lv_obj_t * img);
lv_coord_t lv_img_get_offset_x(lv_obj_t * img);
lv_coord_t lv_img_get_offset_y(lv_obj_t * img);
uint16_t lv_img_get_angle(lv_obj_t * img);
uint16_t lv_img_get_zoom(lv_obj_t * img);
bool lv_img_get_antialias(lv_obj_t * img);
lv_img_size_mode_t lv_img_get_size_mode(lv_obj_t * img);
```

**Image sources:**
- C array: `LV_IMG_DECLARE(my_img); lv_img_set_src(img, &my_img);`
- File: `lv_img_set_src(img, "S:path/to/image.bin");`
- Symbol: `lv_img_set_src(img, LV_SYMBOL_OK);`

**Usage:**
```c
LV_IMG_DECLARE(my_icon);
lv_obj_t * img = lv_img_create(lv_scr_act());
lv_img_set_src(img, &my_icon);
lv_img_set_zoom(img, 512);  // 2x zoom
lv_img_set_angle(img, 450); // 45 degrees
lv_obj_center(img);
```

---

### lv_label (Label)

Text display widget with wrapping, scrolling, and recoloring.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_LABEL`                                             |
| **Create**        | `lv_obj_t * lv_label_create(lv_obj_t * parent)`            |

**Parts:** `LV_PART_MAIN`, `LV_PART_SCROLLBAR`, `LV_PART_SELECTED`

**Key Functions:**
```c
void lv_label_set_text(lv_obj_t * label, const char * text);
void lv_label_set_text_fmt(lv_obj_t * label, const char * fmt, ...);
void lv_label_set_text_static(lv_obj_t * label, const char * text);
void lv_label_set_long_mode(lv_obj_t * label, lv_label_long_mode_t mode);
void lv_label_set_recolor(lv_obj_t * label, bool en);
void lv_label_set_text_selection_start(lv_obj_t * label, uint32_t index);
void lv_label_set_text_selection_end(lv_obj_t * label, uint32_t index);

char * lv_label_get_text(const lv_obj_t * label);
lv_label_long_mode_t lv_label_get_long_mode(const lv_obj_t * label);
bool lv_label_get_recolor(const lv_obj_t * label);
void lv_label_get_letter_pos(const lv_obj_t * label, uint32_t index, lv_point_t * pos);
uint32_t lv_label_get_letter_on(const lv_obj_t * label, lv_point_t * pos);
bool lv_label_is_char_under_pos(const lv_obj_t * label, lv_point_t * pos);
void lv_label_ins_text(lv_obj_t * label, uint32_t pos, const char * txt);
void lv_label_cut_text(lv_obj_t * label, uint32_t pos, uint32_t cnt);
```

**Long Modes:**

| Mode                               | Description                   |
|------------------------------------|-------------------------------|
| `LV_LABEL_LONG_WRAP`               | Wrap text, expand height      |
| `LV_LABEL_LONG_DOT`                | Replace end with "..."        |
| `LV_LABEL_LONG_SCROLL`             | Scroll back and forth         |
| `LV_LABEL_LONG_SCROLL_CIRCULAR`    | Continuous loop scroll        |
| `LV_LABEL_LONG_CLIP`               | Clip at boundaries            |

**Recoloring:** `"Write a #ff0000 red# word"` (enable with `lv_label_set_recolor()`)

**Usage:**
```c
lv_obj_t * label = lv_label_create(lv_scr_act());
lv_label_set_text(label, "Hello LVGL!");
lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
lv_obj_set_width(label, 100);
lv_obj_center(label);
```

---

### lv_line (Line)

Draw line segments from point arrays.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_LINE`                                              |
| **Create**        | `lv_obj_t * lv_line_create(lv_obj_t * parent)`             |

**Key Functions:**
```c
void lv_line_set_points(lv_obj_t * line, const lv_point_t points[], uint16_t point_num);
void lv_line_set_y_invert(lv_obj_t * line, bool en);  // Invert Y axis
```

**Usage:**
```c
static lv_point_t points[] = {{5, 5}, {70, 70}, {120, 10}, {180, 60}};
lv_obj_t * line = lv_line_create(lv_scr_act());
lv_line_set_points(line, points, 4);
lv_obj_set_style_line_width(line, 4, 0);
lv_obj_set_style_line_color(line, lv_palette_main(LV_PALETTE_BLUE), 0);
lv_obj_set_style_line_rounded(line, true, 0);
```

---

### lv_roller (Roller)

Scrollable selection wheel with snap.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_ROLLER`                                            |
| **Create**        | `lv_obj_t * lv_roller_create(lv_obj_t * parent)`           |

**Parts:** `LV_PART_MAIN`, `LV_PART_SELECTED` (highlighted row)

**Key Functions:**
```c
void lv_roller_set_options(lv_obj_t * roller, const char * options, lv_roller_mode_t mode);
void lv_roller_set_selected(lv_obj_t * roller, uint16_t sel_opt, lv_anim_enable_t anim);
void lv_roller_set_visible_row_count(lv_obj_t * roller, uint8_t row_cnt);

uint16_t lv_roller_get_selected(const lv_obj_t * roller);
void lv_roller_get_selected_str(const lv_obj_t * roller, char * buf, uint32_t buf_size);
uint16_t lv_roller_get_option_cnt(const lv_obj_t * roller);
```

**Modes:** `LV_ROLLER_MODE_NORMAL`, `LV_ROLLER_MODE_INFINITE`

**Events:** `LV_EVENT_VALUE_CHANGED`

**Usage:**
```c
lv_obj_t * roller = lv_roller_create(lv_scr_act());
lv_roller_set_options(roller, "Jan\nFeb\nMar\nApr\nMay\nJun", LV_ROLLER_MODE_NORMAL);
lv_roller_set_visible_row_count(roller, 3);
lv_roller_set_selected(roller, 2, LV_ANIM_ON);
```

---

### lv_slider (Slider)

Draggable bar for value selection.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_SLIDER`                                            |
| **Create**        | `lv_obj_t * lv_slider_create(lv_obj_t * parent)`           |
| **Default Range** | 0-100                                                      |

**Parts:** `LV_PART_MAIN` (background), `LV_PART_INDICATOR` (filled), `LV_PART_KNOB` (handle)

**Key Functions:**
```c
void lv_slider_set_value(lv_obj_t * slider, int32_t value, lv_anim_enable_t anim);
void lv_slider_set_range(lv_obj_t * slider, int32_t min, int32_t max);
void lv_slider_set_mode(lv_obj_t * slider, lv_slider_mode_t mode);
void lv_bar_set_start_value(lv_obj_t * slider, int32_t start_value, lv_anim_enable_t anim);

int32_t lv_slider_get_value(const lv_obj_t * slider);
int32_t lv_slider_get_min_value(const lv_obj_t * slider);
int32_t lv_slider_get_max_value(const lv_obj_t * slider);
bool lv_slider_is_dragged(const lv_obj_t * slider);
```

**Modes:** `LV_SLIDER_MODE_NORMAL`, `LV_SLIDER_MODE_SYMMETRICAL`, `LV_SLIDER_MODE_RANGE`

**Events:** `LV_EVENT_VALUE_CHANGED` (continuous), `LV_EVENT_RELEASED`
**Keys:** Arrows increment/decrement by 1

**Usage:**
```c
lv_obj_t * slider = lv_slider_create(lv_scr_act());
lv_slider_set_range(slider, 0, 255);
lv_slider_set_value(slider, 128, LV_ANIM_OFF);
lv_obj_set_width(slider, 200);
lv_obj_center(slider);
// Knob-only mode
lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST);
```

---

### lv_switch (Switch)

Toggle switch (on/off).

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_SWITCH`                                            |
| **Create**        | `lv_obj_t * lv_switch_create(lv_obj_t * parent)`           |

**Parts:** `LV_PART_MAIN` (background), `LV_PART_INDICATOR` (fill), `LV_PART_KNOB` (circle)

**Key Functions:**
```c
// No widget-specific functions; uses base object state management
lv_obj_add_state(sw, LV_STATE_CHECKED);    // Turn on
lv_obj_clear_state(sw, LV_STATE_CHECKED);  // Turn off
bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
```

**Events:** `LV_EVENT_VALUE_CHANGED`

**Usage:**
```c
lv_obj_t * sw = lv_switch_create(lv_scr_act());
lv_obj_add_state(sw, LV_STATE_CHECKED);
lv_obj_add_event_cb(sw, switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
```

---

### lv_table (Table)

Grid-based table with rows and columns of text cells.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_TABLE`                                             |
| **Create**        | `lv_obj_t * lv_table_create(lv_obj_t * parent)`            |

**Parts:** `LV_PART_MAIN`, `LV_PART_ITEMS` (cells)

**Key Functions:**
```c
void lv_table_set_cell_value(lv_obj_t * table, uint16_t row, uint16_t col, const char * txt);
void lv_table_set_cell_value_fmt(lv_obj_t * table, uint16_t row, uint16_t col,
                                 const char * fmt, ...);
void lv_table_set_row_cnt(lv_obj_t * table, uint16_t row_cnt);
void lv_table_set_col_cnt(lv_obj_t * table, uint16_t col_cnt);
void lv_table_set_col_width(lv_obj_t * table, uint16_t col_id, lv_coord_t w);

const char * lv_table_get_cell_value(lv_obj_t * table, uint16_t row, uint16_t col);
uint16_t lv_table_get_row_cnt(lv_obj_t * table);
uint16_t lv_table_get_col_cnt(lv_obj_t * table);
lv_coord_t lv_table_get_col_width(lv_obj_t * table, uint16_t col);
void lv_table_get_selected_cell(lv_obj_t * table, uint16_t * row, uint16_t * col);

// New in v8.3.11
void lv_table_set_cell_user_data(lv_obj_t * table, uint16_t row, uint16_t col, void * user_data);
void * lv_table_get_cell_user_data(lv_obj_t * table, uint16_t row, uint16_t col);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (cell selected), `LV_EVENT_DRAW_PART_BEGIN/END`

---

### lv_textarea (Text Area)

Text input field with cursor, password mode, and accepted character filtering.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_TEXTAREA`                                          |
| **Create**        | `lv_obj_t * lv_textarea_create(lv_obj_t * parent)`         |

**Parts:** `LV_PART_MAIN`, `LV_PART_SCROLLBAR`, `LV_PART_SELECTED`, `LV_PART_CURSOR`, `LV_PART_TEXTAREA_PLACEHOLDER`

**Key Functions:**
```c
void lv_textarea_add_char(lv_obj_t * ta, uint32_t c);
void lv_textarea_add_text(lv_obj_t * ta, const char * txt);
void lv_textarea_del_char(lv_obj_t * ta);
void lv_textarea_del_char_forward(lv_obj_t * ta);
void lv_textarea_set_text(lv_obj_t * ta, const char * txt);
const char * lv_textarea_get_text(const lv_obj_t * ta);

void lv_textarea_set_placeholder_text(lv_obj_t * ta, const char * txt);
void lv_textarea_set_cursor_pos(lv_obj_t * ta, int32_t pos);
void lv_textarea_set_cursor_click_pos(lv_obj_t * ta, bool en);
void lv_textarea_cursor_right(lv_obj_t * ta);
void lv_textarea_cursor_left(lv_obj_t * ta);
void lv_textarea_cursor_up(lv_obj_t * ta);
void lv_textarea_cursor_down(lv_obj_t * ta);

void lv_textarea_set_one_line(lv_obj_t * ta, bool en);
void lv_textarea_set_password_mode(lv_obj_t * ta, bool en);
void lv_textarea_set_password_bullet(lv_obj_t * ta, const char * bullet);
void lv_textarea_set_password_show_time(lv_obj_t * ta, uint16_t time);
void lv_textarea_set_accepted_chars(lv_obj_t * ta, const char * list);
void lv_textarea_set_max_length(lv_obj_t * ta, uint32_t max_length);
void lv_textarea_set_insert_replace(lv_obj_t * ta, const char * txt);
void lv_textarea_set_text_selection(lv_obj_t * ta, bool en);

uint32_t lv_textarea_get_cursor_pos(const lv_obj_t * ta);
bool lv_textarea_get_one_line(const lv_obj_t * ta);
bool lv_textarea_get_password_mode(const lv_obj_t * ta);
const char * lv_textarea_get_accepted_chars(lv_obj_t * ta);
uint32_t lv_textarea_get_max_length(lv_obj_t * ta);
bool lv_textarea_text_is_selected(const lv_obj_t * ta);
bool lv_textarea_get_text_selection(lv_obj_t * ta);
```

**Events:** `LV_EVENT_INSERT`, `LV_EVENT_VALUE_CHANGED`, `LV_EVENT_READY` (Enter in one-line mode)

**Usage:**
```c
lv_obj_t * ta = lv_textarea_create(lv_scr_act());
lv_textarea_set_one_line(ta, true);
lv_textarea_set_placeholder_text(ta, "Enter name...");
lv_textarea_set_max_length(ta, 32);
lv_obj_set_width(ta, 200);

// Numeric input
lv_textarea_set_accepted_chars(ta, "0123456789.");
```

---

## Extra Widgets

### lv_animimg (Animation Image)

Displays a sequence of images as animation frames.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_ANIMIMG`                                           |
| **Create**        | `lv_obj_t * lv_animimg_create(lv_obj_t * parent)`          |

**Key Functions:**
```c
void lv_animimg_set_src(lv_obj_t * img, const void * dsc[], uint8_t num);
void lv_animimg_set_duration(lv_obj_t * img, uint32_t duration);
void lv_animimg_set_repeat_count(lv_obj_t * img, uint16_t count);
void lv_animimg_start(lv_obj_t * img);
```

---

### lv_calendar (Calendar)

Date picker with month/year navigation.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_CALENDAR`                                          |
| **Create**        | `lv_obj_t * lv_calendar_create(lv_obj_t * parent)`         |

**Key Functions:**
```c
void lv_calendar_set_today_date(lv_obj_t * calendar, uint32_t year, uint32_t month, uint32_t day);
void lv_calendar_set_showed_date(lv_obj_t * calendar, uint32_t year, uint32_t month);
void lv_calendar_set_highlighted_dates(lv_obj_t * calendar,
                                       lv_calendar_date_t highlighted[], uint16_t date_num);
void lv_calendar_set_day_names(const char ** day_names);

lv_res_t lv_calendar_get_pressed_date(const lv_obj_t * calendar, lv_calendar_date_t * date);
const lv_calendar_date_t * lv_calendar_get_today_date(const lv_obj_t * calendar);
const lv_calendar_date_t * lv_calendar_get_showed_date(const lv_obj_t * calendar);

lv_obj_t * lv_calendar_header_arrow_create(lv_obj_t * parent);
lv_obj_t * lv_calendar_header_dropdown_create(lv_obj_t * parent);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (date selected)

---

### lv_chart (Chart)

Data visualization with line, bar, and scatter chart types.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_CHART`                                             |
| **Create**        | `lv_obj_t * lv_chart_create(lv_obj_t * parent)`            |

**Parts:** `LV_PART_MAIN`, `LV_PART_ITEMS` (bars/lines), `LV_PART_INDICATOR` (points), `LV_PART_CURSOR`, `LV_PART_TICKS`, `LV_PART_SCROLLBAR`

**Key Functions:**
```c
// Type and configuration
void lv_chart_set_type(lv_obj_t * chart, lv_chart_type_t type);
void lv_chart_set_point_count(lv_obj_t * chart, uint16_t cnt);
void lv_chart_set_range(lv_obj_t * chart, lv_chart_axis_t axis, lv_coord_t min, lv_coord_t max);
void lv_chart_set_update_mode(lv_obj_t * chart, lv_chart_update_mode_t update_mode);
void lv_chart_set_div_line_count(lv_obj_t * chart, uint8_t hdiv, uint8_t vdiv);

// Series
lv_chart_series_t * lv_chart_add_series(lv_obj_t * chart, lv_color_t color,
                                         lv_chart_axis_t axis);
void lv_chart_remove_series(lv_obj_t * chart, lv_chart_series_t * series);
void lv_chart_hide_series(lv_obj_t * chart, lv_chart_series_t * series, bool hide);
void lv_chart_set_series_color(lv_obj_t * chart, lv_chart_series_t * series, lv_color_t color);

// Data
void lv_chart_set_next_value(lv_obj_t * chart, lv_chart_series_t * ser, lv_coord_t value);
void lv_chart_set_next_value2(lv_obj_t * chart, lv_chart_series_t * ser,
                               lv_coord_t x, lv_coord_t y);
void lv_chart_set_value_by_id(lv_obj_t * chart, lv_chart_series_t * ser,
                               uint16_t id, lv_coord_t value);
void lv_chart_set_value_by_id2(lv_obj_t * chart, lv_chart_series_t * ser,
                                uint16_t id, lv_coord_t x, lv_coord_t y);
void lv_chart_set_all_value(lv_obj_t * chart, lv_chart_series_t * ser, lv_coord_t value);
void lv_chart_set_ext_y_array(lv_obj_t * chart, lv_chart_series_t * ser, lv_coord_t array[]);
void lv_chart_set_ext_x_array(lv_obj_t * chart, lv_chart_series_t * ser, lv_coord_t array[]);
lv_coord_t * lv_chart_get_y_array(const lv_obj_t * chart, lv_chart_series_t * ser);
lv_coord_t * lv_chart_get_x_array(const lv_obj_t * chart, lv_chart_series_t * ser);
void lv_chart_refresh(lv_obj_t * chart);

// Axes
void lv_chart_set_axis_tick(lv_obj_t * chart, lv_chart_axis_t axis,
                            lv_coord_t major_len, lv_coord_t minor_len,
                            lv_coord_t major_cnt, lv_coord_t minor_cnt,
                            bool label_en, lv_coord_t draw_size);

// Zoom
void lv_chart_set_zoom_x(lv_obj_t * chart, uint16_t zoom_x);  // 256 = 1x
void lv_chart_set_zoom_y(lv_obj_t * chart, uint16_t zoom_y);

// Cursors
lv_chart_cursor_t * lv_chart_add_cursor(lv_obj_t * chart, lv_color_t color, lv_dir_t dir);
void lv_chart_set_cursor_pos(lv_obj_t * chart, lv_chart_cursor_t * cursor, lv_point_t * pos);
void lv_chart_set_cursor_point(lv_obj_t * chart, lv_chart_cursor_t * cursor,
                                lv_chart_series_t * ser, uint16_t point_id);
lv_point_t lv_chart_get_point_pos_by_id(lv_obj_t * chart, lv_chart_series_t * ser, uint16_t id);
uint16_t lv_chart_get_pressed_point(const lv_obj_t * chart);
```

**Chart Types:** `LV_CHART_TYPE_NONE`, `LV_CHART_TYPE_LINE`, `LV_CHART_TYPE_BAR`, `LV_CHART_TYPE_SCATTER`

**Update Modes:** `LV_CHART_UPDATE_MODE_SHIFT`, `LV_CHART_UPDATE_MODE_CIRCULAR`

**Events:** `LV_EVENT_VALUE_CHANGED` (point pressed)

**Usage:**
```c
lv_obj_t * chart = lv_chart_create(lv_scr_act());
lv_obj_set_size(chart, 200, 150);
lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);

lv_chart_series_t * ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED),
                                                LV_CHART_AXIS_PRIMARY_Y);
lv_chart_set_next_value(chart, ser, 10);
lv_chart_set_next_value(chart, ser, 50);
lv_chart_set_next_value(chart, ser, 30);
```

---

### lv_colorwheel (Color Wheel)

HSV color picker wheel.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_COLORWHEEL`                                        |
| **Create**        | `lv_obj_t * lv_colorwheel_create(lv_obj_t * parent, bool knob_recolor)` |

**Key Functions:**
```c
void lv_colorwheel_set_hsv(lv_obj_t * cw, lv_color_hsv_t hsv);
void lv_colorwheel_set_rgb(lv_obj_t * cw, lv_color_t color);
void lv_colorwheel_set_mode(lv_obj_t * cw, lv_colorwheel_mode_t mode);
void lv_colorwheel_set_mode_fixed(lv_obj_t * cw, bool fixed);

lv_color_hsv_t lv_colorwheel_get_hsv(lv_obj_t * cw);
lv_color_t lv_colorwheel_get_rgb(lv_obj_t * cw);
```

**Modes:** `LV_COLORWHEEL_MODE_HUE`, `LV_COLORWHEEL_MODE_SATURATION`, `LV_COLORWHEEL_MODE_VALUE`

**Events:** `LV_EVENT_VALUE_CHANGED`

---

### lv_imgbtn (Image Button)

Button using images for different states (released, pressed, disabled, etc.).

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_IMGBTN`                                            |
| **Create**        | `lv_obj_t * lv_imgbtn_create(lv_obj_t * parent)`           |

**Key Functions:**
```c
void lv_imgbtn_set_src(lv_obj_t * imgbtn, lv_imgbtn_state_t state, const void * src_left,
                       const void * src_mid, const void * src_right);
```

**States:** `LV_IMGBTN_STATE_RELEASED`, `LV_IMGBTN_STATE_PRESSED`, `LV_IMGBTN_STATE_DISABLED`, `LV_IMGBTN_STATE_CHECKED_RELEASED`, `LV_IMGBTN_STATE_CHECKED_PRESSED`, `LV_IMGBTN_STATE_CHECKED_DISABLED`

---

### lv_keyboard (Keyboard)

On-screen virtual keyboard, typically paired with textarea.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_KEYBOARD`                                          |
| **Create**        | `lv_obj_t * lv_keyboard_create(lv_obj_t * parent)`         |

**Key Functions:**
```c
void lv_keyboard_set_textarea(lv_obj_t * kb, lv_obj_t * ta);
void lv_keyboard_set_mode(lv_obj_t * kb, lv_keyboard_mode_t mode);
void lv_keyboard_set_map(lv_obj_t * kb, lv_keyboard_mode_t mode, const char * map[],
                         const lv_btnmatrix_ctrl_t ctrl_map[]);
void lv_keyboard_set_popovers(lv_obj_t * kb, bool en);

lv_obj_t * lv_keyboard_get_textarea(const lv_obj_t * kb);
lv_keyboard_mode_t lv_keyboard_get_mode(const lv_obj_t * kb);
```

**Modes:** `LV_KEYBOARD_MODE_TEXT_LOWER`, `LV_KEYBOARD_MODE_TEXT_UPPER`, `LV_KEYBOARD_MODE_SPECIAL`, `LV_KEYBOARD_MODE_NUMBER`, `LV_KEYBOARD_MODE_USER_1..4`

**Events:** `LV_EVENT_VALUE_CHANGED` (key pressed), `LV_EVENT_READY` (OK pressed), `LV_EVENT_CANCEL` (close)

**Usage:**
```c
lv_obj_t * kb = lv_keyboard_create(lv_scr_act());
lv_obj_t * ta = lv_textarea_create(lv_scr_act());
lv_keyboard_set_textarea(kb, ta);
```

---

### lv_led (LED)

Simple LED indicator with brightness control.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_LED`                                               |
| **Create**        | `lv_obj_t * lv_led_create(lv_obj_t * parent)`              |

**Key Functions:**
```c
void lv_led_set_color(lv_obj_t * led, lv_color_t color);
void lv_led_set_brightness(lv_obj_t * led, uint8_t bright);  // 0-255
void lv_led_on(lv_obj_t * led);
void lv_led_off(lv_obj_t * led);
void lv_led_toggle(lv_obj_t * led);
uint8_t lv_led_get_brightness(const lv_obj_t * led);
```

---

### lv_list (List)

Vertical scrollable list with text items and buttons.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_LIST`                                              |
| **Create**        | `lv_obj_t * lv_list_create(lv_obj_t * parent)`             |

**Key Functions:**
```c
lv_obj_t * lv_list_add_text(lv_obj_t * list, const char * txt);
lv_obj_t * lv_list_add_btn(lv_obj_t * list, const void * icon, const char * txt);
const char * lv_list_get_btn_text(lv_obj_t * list, lv_obj_t * btn);
```

**Usage:**
```c
lv_obj_t * list = lv_list_create(lv_scr_act());
lv_obj_set_size(list, 180, 220);
lv_list_add_text(list, "Settings");
lv_obj_t * btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "WiFi");
lv_obj_add_event_cb(btn, list_btn_cb, LV_EVENT_CLICKED, NULL);
```

---

### lv_menu (Menu)

Hierarchical page-based navigation menu with sidebar support.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_MENU`                                              |
| **Create**        | `lv_obj_t * lv_menu_create(lv_obj_t * parent)`             |

**Key Functions:**
```c
lv_obj_t * lv_menu_page_create(lv_obj_t * menu, char * title);
lv_obj_t * lv_menu_cont_create(lv_obj_t * parent);
lv_obj_t * lv_menu_section_create(lv_obj_t * parent);
lv_obj_t * lv_menu_separator_create(lv_obj_t * parent);

void lv_menu_set_page(lv_obj_t * menu, lv_obj_t * page);
void lv_menu_set_sidebar_page(lv_obj_t * menu, lv_obj_t * page);
void lv_menu_set_mode_header(lv_obj_t * menu, lv_menu_mode_header_t mode);
void lv_menu_set_mode_root_back_btn(lv_obj_t * menu, lv_menu_mode_root_back_btn_t mode);
void lv_menu_set_load_page_event(lv_obj_t * menu, lv_obj_t * obj, lv_obj_t * page);

lv_obj_t * lv_menu_get_cur_main_page(lv_obj_t * menu);
lv_obj_t * lv_menu_get_cur_sidebar_page(lv_obj_t * menu);
lv_obj_t * lv_menu_get_main_header_back_btn(lv_obj_t * menu);
bool lv_menu_back_btn_is_root(lv_obj_t * menu, lv_obj_t * btn);
```

**Header Modes:** `LV_MENU_HEADER_TOP_FIXED`, `LV_MENU_HEADER_TOP_UNFIXED`, `LV_MENU_HEADER_BOTTOM_FIXED`

**Events:** `LV_EVENT_VALUE_CHANGED` (page displayed), `LV_EVENT_CLICKED` (back button)

**Usage:**
```c
lv_obj_t * menu = lv_menu_create(lv_scr_act());
lv_obj_set_size(menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));

lv_obj_t * page1 = lv_menu_page_create(menu, "Settings");
lv_obj_t * cont = lv_menu_cont_create(page1);
lv_obj_t * label = lv_label_create(cont);
lv_label_set_text(label, "WiFi Settings");

lv_obj_t * page2 = lv_menu_page_create(menu, "WiFi");
// ... add content to page2

lv_menu_set_load_page_event(menu, cont, page2);  // Navigate on click
lv_menu_set_page(menu, page1);  // Show initial page
```

---

### lv_meter (Meter)

Gauge/meter with scales, needles, arcs, and tick indicators.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_METER`                                             |
| **Create**        | `lv_obj_t * lv_meter_create(lv_obj_t * parent)`            |

**Parts:** `LV_PART_MAIN`, `LV_PART_TICKS`, `LV_PART_INDICATOR`, `LV_PART_ITEMS`

**Key Functions:**
```c
// Scales
lv_meter_scale_t * lv_meter_add_scale(lv_obj_t * meter);
void lv_meter_set_scale_ticks(lv_obj_t * meter, lv_meter_scale_t * scale,
                               uint16_t cnt, uint16_t width, uint16_t len, lv_color_t color);
void lv_meter_set_scale_major_ticks(lv_obj_t * meter, lv_meter_scale_t * scale,
                                     uint16_t nth, uint16_t width, uint16_t len,
                                     lv_color_t color, int16_t label_gap);
void lv_meter_set_scale_range(lv_obj_t * meter, lv_meter_scale_t * scale,
                               int32_t min, int32_t max, uint32_t angle_range, uint32_t rotation);

// Indicators
lv_meter_indicator_t * lv_meter_add_needle_line(lv_obj_t * meter, lv_meter_scale_t * scale,
                                                 uint16_t width, lv_color_t color, int16_t r_mod);
lv_meter_indicator_t * lv_meter_add_needle_img(lv_obj_t * meter, lv_meter_scale_t * scale,
                                                const void * src, lv_coord_t pivot_x,
                                                lv_coord_t pivot_y);
lv_meter_indicator_t * lv_meter_add_arc(lv_obj_t * meter, lv_meter_scale_t * scale,
                                         uint16_t width, lv_color_t color, int16_t r_mod);
lv_meter_indicator_t * lv_meter_add_scale_lines(lv_obj_t * meter, lv_meter_scale_t * scale,
                                                 lv_color_t color_start, lv_color_t color_end,
                                                 bool local, int16_t width_mod);

void lv_meter_set_indicator_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value);
void lv_meter_set_indicator_start_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value);
void lv_meter_set_indicator_end_value(lv_obj_t * meter, lv_meter_indicator_t * indic, int32_t value);
```

**Usage:**
```c
lv_obj_t * meter = lv_meter_create(lv_scr_act());
lv_obj_set_size(meter, 200, 200);
lv_obj_center(meter);

lv_meter_scale_t * scale = lv_meter_add_scale(meter);
lv_meter_set_scale_ticks(meter, scale, 41, 2, 10, lv_palette_main(LV_PALETTE_GREY));
lv_meter_set_scale_major_ticks(meter, scale, 8, 4, 15, lv_color_black(), 10);
lv_meter_set_scale_range(meter, scale, 0, 100, 270, 135);

lv_meter_indicator_t * needle = lv_meter_add_needle_line(meter, scale, 4,
                                    lv_palette_main(LV_PALETTE_RED), -10);
lv_meter_set_indicator_value(meter, needle, 75);
```

---

### lv_msgbox (Message Box)

Modal dialog with title, text, close button, and action buttons.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_MSGBOX`                                            |
| **Create**        | `lv_obj_t * lv_msgbox_create(lv_obj_t * parent, const char * title, const char * txt, const char * btn_txts[], bool add_close_btn)` |

**Key Functions:**
```c
lv_obj_t * lv_msgbox_get_title(lv_obj_t * mbox);
lv_obj_t * lv_msgbox_get_close_btn(lv_obj_t * mbox);
lv_obj_t * lv_msgbox_get_text(lv_obj_t * mbox);
lv_obj_t * lv_msgbox_get_btns(lv_obj_t * mbox);
uint16_t lv_msgbox_get_active_btn(lv_obj_t * mbox);
const char * lv_msgbox_get_active_btn_text(lv_obj_t * mbox);
void lv_msgbox_close(lv_obj_t * mbox);
void lv_msgbox_close_async(lv_obj_t * mbox);
```

**Usage:**
```c
static const char * btns[] = {"OK", "Cancel", ""};
lv_obj_t * mbox = lv_msgbox_create(NULL, "Title", "Message text", btns, true);
lv_obj_center(mbox);
lv_obj_add_event_cb(mbox, msgbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
```

---

### lv_span (Span)

Rich text with multiple fonts, colors, and sizes in a single widget.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_SPAN`                                              |
| **Create**        | `lv_obj_t * lv_spangroup_create(lv_obj_t * parent)`        |

**Key Functions:**
```c
lv_span_t * lv_spangroup_new_span(lv_obj_t * spangroup);
void lv_span_set_text(lv_span_t * span, const char * text);
void lv_span_set_text_static(lv_span_t * span, const char * text);
void lv_spangroup_set_align(lv_obj_t * spangroup, lv_text_align_t align);
void lv_spangroup_set_overflow(lv_obj_t * spangroup, lv_span_overflow_t overflow);
void lv_spangroup_set_indent(lv_obj_t * spangroup, lv_coord_t indent);
void lv_spangroup_set_mode(lv_obj_t * spangroup, lv_span_mode_t mode);
void lv_spangroup_del_span(lv_obj_t * spangroup, lv_span_t * span);

// Style spans individually via span->style
lv_style_t * lv_span_get_style(lv_span_t * span);
```

**Modes:** `LV_SPAN_MODE_FIXED`, `LV_SPAN_MODE_EXPAND`, `LV_SPAN_MODE_BREAK`

---

### lv_spinbox (Spinbox)

Numeric input with increment/decrement buttons.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_SPINBOX`                                           |
| **Create**        | `lv_obj_t * lv_spinbox_create(lv_obj_t * parent)`          |

**Key Functions:**
```c
void lv_spinbox_set_value(lv_obj_t * spinbox, int32_t value);
void lv_spinbox_set_range(lv_obj_t * spinbox, int32_t min, int32_t max);
void lv_spinbox_set_step(lv_obj_t * spinbox, uint32_t step);
void lv_spinbox_set_digit_format(lv_obj_t * spinbox, uint8_t digit_count, uint8_t separator_position);
void lv_spinbox_set_rollover(lv_obj_t * spinbox, bool rollover);
void lv_spinbox_set_cursor_pos(lv_obj_t * spinbox, uint8_t pos);

void lv_spinbox_increment(lv_obj_t * spinbox);
void lv_spinbox_decrement(lv_obj_t * spinbox);
void lv_spinbox_step_next(lv_obj_t * spinbox);
void lv_spinbox_step_prev(lv_obj_t * spinbox);

int32_t lv_spinbox_get_value(const lv_obj_t * spinbox);
int32_t lv_spinbox_get_step(lv_obj_t * spinbox);
```

**Events:** `LV_EVENT_VALUE_CHANGED`

---

### lv_spinner (Spinner)

Rotating loading indicator.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_SPINNER`                                           |
| **Create**        | `lv_obj_t * lv_spinner_create(lv_obj_t * parent, uint32_t time, uint32_t arc_length)` |

**Usage:**
```c
lv_obj_t * spinner = lv_spinner_create(lv_scr_act(), 1000, 60);
lv_obj_set_size(spinner, 80, 80);
lv_obj_center(spinner);
```

---

### lv_tabview (Tabview)

Tabbed container with switchable content pages.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_TABVIEW`                                           |
| **Create**        | `lv_obj_t * lv_tabview_create(lv_obj_t * parent, lv_dir_t tab_pos, lv_coord_t tab_size)` |

**Key Functions:**
```c
lv_obj_t * lv_tabview_add_tab(lv_obj_t * tabview, const char * name);
void lv_tabview_set_act(lv_obj_t * tabview, uint32_t id, lv_anim_enable_t anim);
uint16_t lv_tabview_get_tab_act(lv_obj_t * tabview);
lv_obj_t * lv_tabview_get_content(lv_obj_t * tabview);
lv_obj_t * lv_tabview_get_tab_btns(lv_obj_t * tabview);

// New in 8.3
void lv_tabview_rename_tab(lv_obj_t * tabview, uint32_t tab_idx, const char * new_name);
```

**Tab Positions:** `LV_DIR_TOP`, `LV_DIR_BOTTOM`, `LV_DIR_LEFT`, `LV_DIR_RIGHT`

**Events:** `LV_EVENT_VALUE_CHANGED` (tab switched)

**Usage:**
```c
lv_obj_t * tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);
lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Home");
lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "Settings");

lv_obj_t * label = lv_label_create(tab1);
lv_label_set_text(label, "Home content");
```

---

### lv_tileview (Tile View)

Tile-based swipeable container (like a phone home screen).

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_TILEVIEW`                                          |
| **Create**        | `lv_obj_t * lv_tileview_create(lv_obj_t * parent)`         |

**Key Functions:**
```c
lv_obj_t * lv_tileview_add_tile(lv_obj_t * tv, uint8_t col_id, uint8_t row_id, lv_dir_t dir);
void lv_obj_set_tile(lv_obj_t * tv, lv_obj_t * tile_obj, lv_anim_enable_t anim);
void lv_obj_set_tile_id(lv_obj_t * tv, uint32_t col_id, uint32_t row_id, lv_anim_enable_t anim);
lv_obj_t * lv_tileview_get_tile_act(lv_obj_t * tv);
```

**Events:** `LV_EVENT_VALUE_CHANGED` (tile changed)

---

### lv_win (Window)

Window container with header (title + buttons) and content area.

| Property          | Details                                                    |
|-------------------|------------------------------------------------------------|
| **Config Macro**  | `LV_USE_WIN`                                               |
| **Create**        | `lv_obj_t * lv_win_create(lv_obj_t * parent, lv_coord_t header_height)` |

**Key Functions:**
```c
lv_obj_t * lv_win_add_title(lv_obj_t * win, const char * txt);
lv_obj_t * lv_win_add_btn(lv_obj_t * win, const void * icon, lv_coord_t btn_w);
lv_obj_t * lv_win_get_header(lv_obj_t * win);
lv_obj_t * lv_win_get_content(lv_obj_t * win);
```

**Usage:**
```c
lv_obj_t * win = lv_win_create(lv_scr_act(), 40);
lv_win_add_title(win, "Settings");
lv_obj_t * close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE, 40);
lv_obj_add_event_cb(close_btn, close_cb, LV_EVENT_CLICKED, NULL);

lv_obj_t * content = lv_win_get_content(win);
lv_obj_t * label = lv_label_create(content);
lv_label_set_text(label, "Window content goes here");
```

---

## Common Symbols (FontAwesome Subset)

Available with Montserrat fonts for use with labels and buttons:

| Symbol                  | Appearance | Symbol                  | Appearance |
|-------------------------|------------|-------------------------|------------|
| `LV_SYMBOL_AUDIO`       | speaker    | `LV_SYMBOL_VIDEO`        | camera     |
| `LV_SYMBOL_LIST`        | list       | `LV_SYMBOL_OK`           | checkmark  |
| `LV_SYMBOL_CLOSE`       | X          | `LV_SYMBOL_POWER`        | power      |
| `LV_SYMBOL_SETTINGS`    | gear       | `LV_SYMBOL_HOME`         | house      |
| `LV_SYMBOL_DOWNLOAD`    | down arrow | `LV_SYMBOL_DRIVE`        | disk       |
| `LV_SYMBOL_REFRESH`     | refresh    | `LV_SYMBOL_MUTE`         | muted      |
| `LV_SYMBOL_VOLUME_MID`  | volume     | `LV_SYMBOL_VOLUME_MAX`   | loud       |
| `LV_SYMBOL_IMAGE`       | picture    | `LV_SYMBOL_EDIT`         | pencil     |
| `LV_SYMBOL_PREV`        | back       | `LV_SYMBOL_PLAY`         | play       |
| `LV_SYMBOL_PAUSE`       | pause      | `LV_SYMBOL_STOP`         | stop       |
| `LV_SYMBOL_NEXT`        | forward    | `LV_SYMBOL_EJECT`        | eject      |
| `LV_SYMBOL_LEFT`        | left       | `LV_SYMBOL_RIGHT`        | right      |
| `LV_SYMBOL_PLUS`        | plus       | `LV_SYMBOL_MINUS`        | minus      |
| `LV_SYMBOL_EYE_OPEN`    | eye        | `LV_SYMBOL_EYE_CLOSE`    | closed eye |
| `LV_SYMBOL_WARNING`     | warning    | `LV_SYMBOL_SHUFFLE`      | shuffle    |
| `LV_SYMBOL_UP`          | up         | `LV_SYMBOL_DOWN`         | down       |
| `LV_SYMBOL_LOOP`        | loop       | `LV_SYMBOL_DIRECTORY`    | folder     |
| `LV_SYMBOL_UPLOAD`      | up arrow   | `LV_SYMBOL_CALL`         | phone      |
| `LV_SYMBOL_CUT`         | scissors   | `LV_SYMBOL_COPY`         | copy       |
| `LV_SYMBOL_SAVE`        | floppy     | `LV_SYMBOL_CHARGE`       | battery    |
| `LV_SYMBOL_PASTE`       | paste      | `LV_SYMBOL_BELL`         | bell       |
| `LV_SYMBOL_KEYBOARD`    | keyboard   | `LV_SYMBOL_GPS`          | location   |
| `LV_SYMBOL_FILE`        | document   | `LV_SYMBOL_WIFI`         | wifi       |
| `LV_SYMBOL_BATTERY_FULL`| battery    | `LV_SYMBOL_BATTERY_3`    | 75%        |
| `LV_SYMBOL_BATTERY_2`   | 50%        | `LV_SYMBOL_BATTERY_1`    | 25%        |
| `LV_SYMBOL_BATTERY_EMPTY`| empty     | `LV_SYMBOL_USB`          | USB        |
| `LV_SYMBOL_BLUETOOTH`   | bluetooth  | `LV_SYMBOL_TRASH`        | trash      |
| `LV_SYMBOL_BACKSPACE`   | backspace  | `LV_SYMBOL_SD_CARD`      | SD card    |
| `LV_SYMBOL_NEW_LINE`    | newline    | `LV_SYMBOL_DUMMY`        | no glyph   |
| `LV_SYMBOL_BULLET`      | bullet     |                         |            |

---

## Sources

- [LVGL 8.3 Widget Documentation](https://docs.lvgl.io/8.3/widgets/index.html)
- [LVGL 8.3 Base Object](https://docs.lvgl.io/8.3/widgets/obj.html)
- [LVGL 8.3 Changelog](https://docs.lvgl.io/8.3/CHANGELOG.html)
- [LVGL GitHub - release/v8.3](https://github.com/lvgl/lvgl/tree/release/v8.3)
