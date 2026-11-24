# LVGL v8 to v9 Migration Guide

> Definitive reference for migrating LVGL projects from v8.x to v9.x.
> Sources: Official LVGL changelog, lv_api_map_v8.h, GitHub issues #3298 and #4011, LVGL forum migration threads.

---

## Table of Contents

1. [Critical Breaking Changes](#critical-breaking-changes)
2. [General Naming Convention Changes](#general-naming-convention-changes)
3. [Complete Renamed Functions Table](#complete-renamed-functions-table)
4. [Removed Functions and Replacements](#removed-functions-and-replacements)
5. [Display Driver Migration](#display-driver-migration)
6. [Input Device Driver Migration](#input-device-driver-migration)
7. [Event System Changes](#event-system-changes)
8. [Style System Changes](#style-system-changes)
9. [Widget-by-Widget Migration](#widget-by-widget-migration)
10. [Color Format Migration](#color-format-migration)
11. [Drawing Pipeline Changes](#drawing-pipeline-changes)
12. [Build System Changes](#build-system-changes)
13. [lv_conf.h Migration](#lv_confh-migration)
14. [Code Examples: Before and After](#code-examples-before-and-after)
15. [Compatibility Layer](#compatibility-layer)
16. [Memory Impact Notes](#memory-impact-notes)

---

## Critical Breaking Changes

These changes will NOT cause compiler errors but will cause runtime failures if not addressed:

| Change | Impact | Action Required |
|--------|--------|-----------------|
| Buffer size now in **bytes** not pixels | Display corruption or crash | Multiply pixel count by bytes-per-pixel |
| `lv_color_t` is always RGB888 internally | Color calculations differ | Review all direct color manipulation |
| `lv_conf.h` restructured | Missing config options | Regenerate from `lv_conf_template.h` |
| `<stdint.h>` NOT included in `lv_conf.h` | Assembly optimization preserved | Include manually if needed in your code |
| Image converter changed | Old converted images incompatible | Re-convert images using `LVGLImage.py` |
| `lv_coord_t` removed | Was typedef, now `int32_t` | Replace all `lv_coord_t` with `int32_t` |

---

## General Naming Convention Changes

v9 standardizes abbreviated names to full words:

| v8 Pattern | v9 Pattern | Scope |
|------------|------------|-------|
| `btn` | `button` | Widget names, functions |
| `btnmatrix` | `buttonmatrix` | Widget names, functions |
| `img` | `image` | Widget names, functions, styles |
| `imgbtn` | `imagebutton` | Widget names, functions |
| `disp` | `display` | All display functions |
| `scr` | `screen` | Screen management functions |
| `act` | `active` | Active screen/tab/tile getters |
| `del` | `delete` | Object/timer/anim deletion |
| `col` | `column` | Table column functions |
| `cnt` | `count` | Count getters |
| `zoom` | `scale` | Transform property |
| `angle` | `rotation` | Transform property |
| `clear` (flag/state) | `remove` | Flag/state management |
| `ofs` | `offset` | Shadow offset styles |
| `anim_time` | `anim_duration` | Animation timing |

---

## Complete Renamed Functions Table

### Core Object Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_obj_del()` | `lv_obj_delete()` |
| `lv_obj_del_async()` | `lv_obj_delete_async()` |
| `lv_obj_clear_flag()` | `lv_obj_remove_flag()` |
| `lv_obj_clear_state()` | `lv_obj_remove_state()` |
| `lv_obj_get_child_cnt()` | `lv_obj_get_child_count()` |
| `lv_obj_get_disp()` | `lv_obj_get_display()` |
| `lv_obj_move_foreground()` | Move to last child index (inline wrapper) |
| `lv_obj_move_background()` | Move to index 0 (inline wrapper) |
| `lv_obj_delete_anim_ready_cb()` | `lv_obj_delete_anim_completed_cb()` |

### Display Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_disp_remove()` | `lv_display_delete()` |
| `lv_disp_set_default()` | `lv_display_set_default()` |
| `lv_disp_get_default()` | `lv_display_get_default()` |
| `lv_disp_get_next()` | `lv_display_get_next()` |
| `lv_disp_set_rotation()` | `lv_display_set_rotation()` |
| `lv_disp_get_hor_res()` | `lv_display_get_horizontal_resolution()` |
| `lv_disp_get_ver_res()` | `lv_display_get_vertical_resolution()` |
| `lv_disp_get_physical_hor_res()` | `lv_display_get_physical_horizontal_resolution()` |
| `lv_disp_get_physical_ver_res()` | `lv_display_get_physical_vertical_resolution()` |
| `lv_disp_get_offset_x()` | `lv_display_get_offset_x()` |
| `lv_disp_get_offset_y()` | `lv_display_get_offset_y()` |
| `lv_disp_get_rotation()` | `lv_display_get_rotation()` |
| `lv_disp_get_dpi()` | `lv_display_get_dpi()` |
| `lv_disp_get_antialiasing()` | `lv_display_get_antialiasing()` |
| `lv_disp_flush_ready()` | `lv_display_flush_ready()` |
| `lv_disp_flush_is_last()` | `lv_display_flush_is_last()` |
| `lv_disp_send_event()` | `lv_display_send_event()` |
| `lv_disp_set_theme()` | `lv_display_set_theme()` |
| `lv_disp_get_theme()` | `lv_display_get_theme()` |
| `lv_disp_get_inactive_time()` | `lv_display_get_inactive_time()` |
| `lv_disp_trig_activity()` | `lv_display_trigger_activity()` |
| `lv_disp_enable_invalidation()` | `lv_display_enable_invalidation()` |
| `lv_disp_is_invalidation_enabled()` | `lv_display_is_invalidation_enabled()` |
| `lv_disp_refr_timer()` | `lv_display_refr_timer()` |
| `lv_disp_get_refr_timer()` | `lv_display_get_refr_timer()` |
| `lv_disp_get_layer_top()` | `lv_display_get_layer_top()` |
| `lv_disp_get_layer_sys()` | `lv_display_get_layer_sys()` |

### Screen Management Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_scr_act()` | `lv_screen_active()` |
| `lv_disp_get_scr_act()` | `lv_display_get_screen_active()` |
| `lv_disp_get_scr_prev()` | `lv_display_get_screen_prev()` |
| `lv_disp_load_scr()` | `lv_screen_load()` |
| `lv_scr_load()` | `lv_screen_load()` |
| `lv_scr_load_anim()` | `lv_screen_load_anim()` |

### Input Device Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_indev_set_disp()` | `lv_indev_set_display()` |
| `lv_indev_get_act()` | `lv_indev_active()` |

### Timer and Animation Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_task_handler()` | `lv_timer_handler()` (inline wrapper provided) |
| `lv_timer_del()` | `lv_timer_delete()` |
| `lv_anim_del()` | `lv_anim_delete()` |
| `lv_anim_del_all()` | `lv_anim_delete_all()` |
| `lv_anim_set_ready_cb()` | `lv_anim_set_completed_cb()` |

### Group Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_group_del()` | `lv_group_delete()` |

### Text Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_txt_get_size()` | `lv_text_get_size()` |
| `lv_txt_get_width()` | `lv_text_get_width()` |

### Image Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_img_create()` | `lv_image_create()` |
| `lv_img_set_src()` | `lv_image_set_src()` |
| `lv_img_set_offset_x()` | `lv_image_set_offset_x()` |
| `lv_img_set_offset_y()` | `lv_image_set_offset_y()` |
| `lv_img_set_angle()` | `lv_image_set_rotation()` |
| `lv_img_set_pivot()` | `lv_image_set_pivot()` |
| `lv_img_set_zoom()` | `lv_image_set_scale()` |
| `lv_img_set_antialias()` | `lv_image_set_antialias()` |
| `lv_img_get_src()` | `lv_image_get_src()` |
| `lv_img_get_offset_x()` | `lv_image_get_offset_x()` |
| `lv_img_get_offset_y()` | `lv_image_get_offset_y()` |
| `lv_img_get_angle()` | `lv_image_get_rotation()` |
| `lv_img_get_pivot()` | `lv_image_get_pivot()` |
| `lv_img_get_zoom()` | `lv_image_get_scale()` |
| `lv_img_get_antialias()` | `lv_image_get_antialias()` |

### Image Button Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_imgbtn_create()` | `lv_imagebutton_create()` |
| `lv_imgbtn_set_src()` | `lv_imagebutton_set_src()` |
| `lv_imgbtn_set_state()` | `lv_imagebutton_set_state()` |
| `lv_imgbtn_get_src_left()` | `lv_imagebutton_get_src_left()` |
| `lv_imgbtn_get_src_middle()` | `lv_imagebutton_get_src_middle()` |
| `lv_imgbtn_get_src_right()` | `lv_imagebutton_get_src_right()` |

### Button Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_btn_create()` | `lv_button_create()` |

### Button Matrix Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_btnmatrix_create()` | `lv_buttonmatrix_create()` |
| `lv_btnmatrix_set_map()` | `lv_buttonmatrix_set_map()` |
| `lv_btnmatrix_set_ctrl_map()` | `lv_buttonmatrix_set_ctrl_map()` |
| `lv_btnmatrix_set_selected_btn()` | `lv_buttonmatrix_set_selected_button()` |
| `lv_btnmatrix_set_btn_ctrl()` | `lv_buttonmatrix_set_button_ctrl()` |
| `lv_btnmatrix_clear_btn_ctrl()` | `lv_buttonmatrix_clear_button_ctrl()` |
| `lv_btnmatrix_set_btn_ctrl_all()` | `lv_buttonmatrix_set_button_ctrl_all()` |
| `lv_btnmatrix_clear_btn_ctrl_all()` | `lv_buttonmatrix_clear_button_ctrl_all()` |
| `lv_btnmatrix_set_btn_width()` | `lv_buttonmatrix_set_button_width()` |
| `lv_btnmatrix_set_one_checked()` | `lv_buttonmatrix_set_one_checked()` |
| `lv_btnmatrix_get_map()` | `lv_buttonmatrix_get_map()` |
| `lv_btnmatrix_get_selected_btn()` | `lv_buttonmatrix_get_selected_button()` |
| `lv_btnmatrix_get_btn_text()` | `lv_buttonmatrix_get_button_text()` |
| `lv_btnmatrix_has_button_ctrl()` | `lv_buttonmatrix_has_button_ctrl()` |
| `lv_btnmatrix_get_one_checked()` | `lv_buttonmatrix_get_one_checked()` |

### List Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_list_add_btn()` | `lv_list_add_button()` |
| `lv_list_set_btn_text()` | `lv_list_set_button_text()` |
| `lv_list_get_btn_text()` | `lv_list_get_button_text()` |

### Tab View Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_tabview_get_tab_btns()` | `lv_tabview_get_tab_bar()` |
| `lv_tabview_get_tab_act()` | `lv_tabview_get_tab_active()` |
| `lv_tabview_set_act()` | `lv_tabview_set_active()` |

### Tile View Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_tileview_get_tile_act()` | `lv_tileview_get_tile_active()` |
| `lv_obj_set_tile_id()` | `lv_tileview_set_tile_by_index()` |
| `lv_obj_set_tile()` | `lv_tileview_set_tile()` |

### Roller Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_roller_set_visible_row_cnt()` | `lv_roller_set_visible_row_count()` |
| `lv_roller_get_option_cnt()` | `lv_roller_get_option_count()` |

### Table Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_table_set_col_cnt()` | `lv_table_set_column_count()` |
| `lv_table_set_row_cnt()` | `lv_table_set_row_count()` |
| `lv_table_get_col_cnt()` | `lv_table_get_column_count()` |
| `lv_table_get_row_cnt()` | `lv_table_get_row_count()` |
| `lv_table_set_col_width()` | `lv_table_set_column_width()` |
| `lv_table_get_col_width()` | `lv_table_get_column_width()` |

### Dropdown Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_dropdown_get_option_cnt()` | `lv_dropdown_get_option_count()` |

### Keyboard Widget Functions

| v8 Name | v9 Name |
|---------|---------|
| `lv_keyboard_get_selected_btn()` | `lv_keyboard_get_selected_button()` |
| `lv_keyboard_get_btn_text()` | `lv_keyboard_get_button_text()` |

---

## Removed Functions and Replacements

### Structures Removed Entirely

| v8 Structure | v9 Replacement |
|--------------|----------------|
| `lv_disp_drv_t` | `lv_display_t*` (opaque, use setter functions) |
| `lv_disp_draw_buf_t` | Integrated into `lv_display_set_buffers()` |
| `lv_indev_drv_t` | `lv_indev_t*` (opaque, use setter functions) |
| `lv_obj_draw_part_dsc_t` | `lv_draw_task_t` + `lv_draw_dsc_base_t` |

### Functions Removed Without Direct Rename

| v8 Function | v9 Replacement | Notes |
|-------------|----------------|-------|
| `lv_disp_drv_init()` | `lv_display_create()` | Complete API redesign |
| `lv_disp_drv_register()` | `lv_display_create()` | Creation and registration merged |
| `lv_disp_draw_buf_init()` | `lv_display_set_buffers()` | Buffer size now in bytes |
| `lv_indev_drv_init()` | `lv_indev_create()` | Complete API redesign |
| `lv_indev_drv_register()` | `lv_indev_create()` + setters | Creation and registration merged |
| `monitor_cb` (display driver) | `LV_EVENT_RENDER_READY` event | Attach via `lv_display_add_event()` |
| `feedback_cb` (indev driver) | `LV_EVENT_PRESSED/CLICKED/etc.` | Events sent to input device |
| `set_px_cb` (display driver) | Convert in `flush_cb` | Custom pixel format conversion |
| `lv_event_get_draw_part_dsc()` | `lv_event_get_draw_task()` | New drawing architecture |
| `lv_msg_*` functions | `lv_observer_*` functions | Observer pattern replaces messaging |
| `LV_IMG_DECLARE()` | `extern const lv_image_dsc_t var;` | Simple extern declaration |

### Widgets Removed

| v8 Widget | v9 Replacement | Notes |
|-----------|----------------|-------|
| `lv_meter` | `lv_scale` | Different API; not a drop-in replacement |

---

## Display Driver Migration

### v8 Display Initialization (BEFORE)

```c
/* v8: Display driver setup */
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * 10];
lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * 10);

static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = SCREEN_WIDTH;
disp_drv.ver_res = SCREEN_HEIGHT;
disp_drv.flush_cb = my_flush_cb;
disp_drv.draw_buf = &draw_buf;
disp_drv.monitor_cb = my_monitor_cb;  /* Performance monitoring */
lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
```

### v9 Display Initialization (AFTER)

```c
/* v9: Display setup */
lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
lv_display_set_flush_cb(disp, my_flush_cb);

static uint8_t buf1[SCREEN_WIDTH * 10 * sizeof(lv_color_t)];  /* Size in BYTES */
lv_display_set_buffers(disp, buf1, NULL,
                       sizeof(buf1),
                       LV_DISPLAY_RENDER_MODE_PARTIAL);

/* Performance monitoring: use event instead of monitor_cb */
lv_display_add_event(disp, my_render_ready_cb, LV_EVENT_RENDER_READY, NULL);
```

### Render Modes

| v9 Mode | Description | Buffer Requirement |
|---------|-------------|-------------------|
| `LV_DISPLAY_RENDER_MODE_PARTIAL` | Small buffers, RAM-efficient | Minimum 1/10 of screen |
| `LV_DISPLAY_RENDER_MODE_DIRECT` | Screen-sized buffers with dirty-area sync | Full screen buffer(s) |
| `LV_DISPLAY_RENDER_MODE_FULL` | Full screen redrawn each cycle | Full screen buffer(s) |

### Flush Callback Signature

```c
/* v8 flush callback */
void my_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    /* ... draw pixels ... */
    lv_disp_flush_ready(drv);
}

/* v9 flush callback */
void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    /* ... draw pixels ... */
    lv_display_flush_ready(disp);
}
```

### Display Feature Migration

| v8 Feature | v9 Replacement |
|------------|----------------|
| `disp_drv.scr_transp` | `lv_display_set_color_format(disp, LV_COLOR_FORMAT_NATIVE_ALPHA)` |
| `LV_COLOR_16_SWAP` | Call `lv_draw_sw_rgb565_swap()` in `flush_cb` |
| Display background color/image | Use `lv_layer_bottom()` |
| `lv_display_set_color_format()` | Runtime color format adjustment (new in v9) |

---

## Input Device Driver Migration

### v8 Input Device Setup (BEFORE)

```c
/* v8: Input device driver setup */
static lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type = LV_INDEV_TYPE_POINTER;
indev_drv.read_cb = my_touchpad_read;
indev_drv.feedback_cb = my_feedback_cb;
lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
```

### v9 Input Device Setup (AFTER)

```c
/* v9: Input device setup */
lv_indev_t *indev = lv_indev_create();
lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
lv_indev_set_read_cb(indev, my_touchpad_read);

/* feedback_cb replaced with events on the input device */
lv_indev_add_event(indev, my_feedback_cb, LV_EVENT_PRESSED, NULL);
```

### Read Callback Signature

```c
/* v8 read callback */
void my_touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    data->point.x = tp_x;
    data->point.y = tp_y;
    data->state = LV_INDEV_STATE_PR;  /* Abbreviated */
}

/* v9 read callback */
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    data->point.x = tp_x;
    data->point.y = tp_y;
    data->state = LV_INDEV_STATE_PRESSED;  /* Full word */
}
```

---

## Event System Changes

### Event Callback Registration

```c
/* v8 */
lv_obj_add_event_cb(btn, my_event_cb, LV_EVENT_CLICKED, user_data);

/* v9 - function renamed but signature similar */
lv_obj_add_event_cb(btn, my_event_cb, LV_EVENT_CLICKED, user_data);
/* Note: lv_api_map_v8.h provides compatibility; new name is lv_obj_add_event() */
```

### Event Callback Signature

```c
/* v8 callback */
static void my_event_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
}

/* v9 callback - target accessor changed */
static void my_event_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target_obj(e);  /* Changed */
    lv_event_code_t code = lv_event_get_code(e);
}
```

### Draw Event Migration

```c
/* v8: Custom drawing in events */
static void draw_event_cb(lv_event_t *e) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    /* Access dsc->part, dsc->line_dsc, dsc->p1, dsc->p2 */
}

/* v9: New draw task architecture */
static void draw_event_cb(lv_event_t *e) {
    lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t *base_dsc = draw_task->draw_dsc;
    /* Use base_dsc and draw_task for custom drawing */
}
```

### Event Target Behavior Change

In v9, the `target` parameter of the event is always the **current target widget** (the widget the event handler is attached to). To get the **original target** (the widget that originally received the event), use `lv_event_get_target_obj(e)`.

### Display and Input Device Events (New in v9)

```c
/* Events can now be attached to displays */
lv_display_add_event(disp, my_cb, LV_EVENT_RENDER_READY, NULL);

/* Events can now be attached to input devices */
lv_indev_add_event(indev, my_cb, LV_EVENT_PRESSED, NULL);
```

---

## Style System Changes

### Style Property Renames

| v8 Property | v9 Property |
|-------------|-------------|
| `LV_STYLE_ANIM_TIME` | `LV_STYLE_ANIM_DURATION` |
| `LV_STYLE_IMG_OPA` | `LV_STYLE_IMAGE_OPA` |
| `LV_STYLE_IMG_RECOLOR` | `LV_STYLE_IMAGE_RECOLOR` |
| `LV_STYLE_IMG_RECOLOR_OPA` | `LV_STYLE_IMAGE_RECOLOR_OPA` |
| `LV_STYLE_SHADOW_OFS_X` | `LV_STYLE_SHADOW_OFFSET_X` |
| `LV_STYLE_SHADOW_OFS_Y` | `LV_STYLE_SHADOW_OFFSET_Y` |
| `LV_STYLE_TRANSFORM_ANGLE` | `LV_STYLE_TRANSFORM_ROTATION` |
| `LV_STYLE_TRANSFORM_ZOOM` | `LV_STYLE_TRANSFORM_SCALE` |

### Style Setter Function Renames

| v8 Function | v9 Function |
|-------------|-------------|
| `lv_style_set_anim_time()` | `lv_style_set_anim_duration()` |
| `lv_style_set_img_opa()` | `lv_style_set_image_opa()` |
| `lv_style_set_img_recolor()` | `lv_style_set_image_recolor()` |
| `lv_style_set_img_recolor_opa()` | `lv_style_set_image_recolor_opa()` |
| `lv_style_set_shadow_ofs_x()` | `lv_style_set_shadow_offset_x()` |
| `lv_style_set_shadow_ofs_y()` | `lv_style_set_shadow_offset_y()` |
| `lv_style_set_transform_angle()` | `lv_style_set_transform_rotation()` |
| `lv_style_set_transform_zoom()` | `lv_style_set_transform_scale()` |
| `lv_style_set_bg_img_src()` | `lv_style_set_bg_image_src()` |
| `lv_style_set_bg_img_recolor()` | `lv_style_set_bg_image_recolor()` |
| `lv_style_set_bg_img_recolor_opa()` | `lv_style_set_bg_image_recolor_opa()` |

### Object Style Setter Renames

| v8 Function | v9 Function |
|-------------|-------------|
| `lv_obj_set_style_anim_time()` | `lv_obj_set_style_anim_duration()` |
| `lv_obj_set_style_img_opa()` | `lv_obj_set_style_image_opa()` |
| `lv_obj_set_style_img_recolor()` | `lv_obj_set_style_image_recolor()` |
| `lv_obj_set_style_img_recolor_opa()` | `lv_obj_set_style_image_recolor_opa()` |
| `lv_obj_set_style_shadow_ofs_x()` | `lv_obj_set_style_shadow_offset_x()` |
| `lv_obj_set_style_shadow_ofs_y()` | `lv_obj_set_style_shadow_offset_y()` |
| `lv_obj_set_style_transform_zoom()` | `lv_obj_set_style_transform_scale()` |
| `lv_obj_set_style_transform_angle()` | `lv_obj_set_style_transform_rotation()` |
| `lv_obj_set_style_bg_img_src()` | `lv_obj_set_style_bg_image_src()` |
| `lv_obj_set_style_bg_img_recolor()` | `lv_obj_set_style_bg_image_recolor()` |
| `lv_obj_set_style_bg_img_recolor_opa()` | `lv_obj_set_style_bg_image_recolor_opa()` |

### Object Style Getter Renames

| v8 Function | v9 Function |
|-------------|-------------|
| `lv_obj_get_style_anim_time()` | `lv_obj_get_style_anim_duration()` |
| `lv_obj_get_style_img_opa()` | `lv_obj_get_style_image_opa()` |
| `lv_obj_get_style_img_recolor()` | `lv_obj_get_style_image_recolor()` |
| `lv_obj_get_style_img_recolor_filtered()` | `lv_obj_get_style_image_recolor_filtered()` |
| `lv_obj_get_style_img_recolor_opa()` | `lv_obj_get_style_image_recolor_opa()` |
| `lv_obj_get_style_shadow_ofs_x()` | `lv_obj_get_style_shadow_offset_x()` |
| `lv_obj_get_style_shadow_ofs_y()` | `lv_obj_get_style_shadow_offset_y()` |
| `lv_obj_get_style_transform_angle()` | `lv_obj_get_style_transform_rotation()` |
| `lv_obj_get_style_bg_img_src()` | `lv_obj_get_style_bg_image_src()` |
| `lv_obj_get_style_bg_img_recolor()` | `lv_obj_get_style_bg_image_recolor()` |
| `lv_obj_get_style_bg_img_recolor_opa()` | `lv_obj_get_style_bg_image_recolor_opa()` |

---

## Widget-by-Widget Migration

### lv_meter (REMOVED)

The `lv_meter` widget was completely removed in v9. Use `lv_scale` as a replacement.

```c
/* v8: Meter widget */
lv_obj_t *meter = lv_meter_create(parent);
lv_meter_scale_t *scale = lv_meter_add_scale(meter);
lv_meter_set_scale_range(meter, scale, 0, 100, 270, 135);
lv_meter_indicator_t *indic = lv_meter_add_needle_line(meter, scale, 2, lv_color_red(), -10);
lv_meter_set_indicator_value(meter, indic, 75);

/* v9: Scale widget (different API, not a drop-in) */
lv_obj_t *scale = lv_scale_create(parent);
lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
lv_scale_set_range(scale, 0, 100);
lv_scale_set_total_tick_count(scale, 21);
lv_scale_set_major_tick_every(scale, 5);
/* Needle indicators require custom drawing or additional widgets */
```

**Key differences**: `lv_scale` does not have built-in needle/arc indicators. Needles must be implemented with `lv_line` or custom drawing overlaid on the scale.

### lv_msgbox (REFACTORED)

```c
/* v8: Message box with button matrix */
static const char *btns[] = {"OK", "Cancel", ""};
lv_obj_t *mbox = lv_msgbox_create(NULL, "Title", "Message text", btns, true);

/* v9: Message box with regular buttons */
lv_obj_t *mbox = lv_msgbox_create(parent);
lv_msgbox_add_title(mbox, "Title");
lv_msgbox_add_text(mbox, "Message text");
lv_msgbox_add_close_button(mbox);
lv_obj_t *btn_ok = lv_msgbox_add_footer_button(mbox, "OK");
lv_obj_t *btn_cancel = lv_msgbox_add_footer_button(mbox, "Cancel");
```

### lv_tabview (UPDATED)

```c
/* v8 */
lv_obj_t *tabview = lv_tabview_create(parent, LV_DIR_TOP, 50);
lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "Tab 1");
uint16_t active = lv_tabview_get_tab_act(tabview);
lv_tabview_set_act(tabview, 0, LV_ANIM_ON);

/* v9 */
lv_obj_t *tabview = lv_tabview_create(parent);
lv_tabview_set_tab_bar_position(tabview, LV_DIR_TOP);
lv_tabview_set_tab_bar_size(tabview, 50);
lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "Tab 1");
uint32_t active = lv_tabview_get_tab_active(tabview);
lv_tabview_set_active(tabview, 0, LV_ANIM_ON);
```

### lv_chart (TICK SUPPORT REMOVED)

Chart tick labels are no longer built into the chart widget. Use `lv_scale` adjacent to the chart for axis labels.

### lv_image (WAS lv_img)

The image widget gained new capabilities in v9:
- Alignment support (inner alignment of the image)
- Stretching modes
- Tiling support

```c
/* v8 */
lv_obj_t *img = lv_img_create(parent);
lv_img_set_src(img, &my_img);
lv_img_set_zoom(img, 256);    /* 256 = 100% */
lv_img_set_angle(img, 450);   /* 0.1 degree units */

/* v9 */
lv_obj_t *img = lv_image_create(parent);
lv_image_set_src(img, &my_img);
lv_image_set_scale(img, 256);     /* 256 = 100% */
lv_image_set_rotation(img, 450);  /* 0.1 degree units */
```

### lv_btn -> lv_button

Only the name changed; API is otherwise identical.

### lv_btnmatrix -> lv_buttonmatrix

All functions renamed from `btnmatrix` to `buttonmatrix` and `btn` to `button` within those functions.

---

## Color Format Migration

### Constant Renames

| v8 Constant | v9 Constant |
|-------------|-------------|
| `LV_IMG_CF_TRUE_COLOR` | `LV_COLOR_FORMAT_NATIVE` |
| `LV_IMG_CF_TRUE_COLOR_ALPHA` | `LV_COLOR_FORMAT_NATIVE_ALPHA` |
| `LV_IMG_CF_ALPHA_1BIT` | Removed (use 8-bit alpha indexed) |
| `LV_IMG_CF_ALPHA_2BIT` | Removed (use 8-bit alpha indexed) |
| `LV_IMG_CF_ALPHA_4BIT` | Removed (use 8-bit alpha indexed) |
| `LV_IMG_CF_INDEXED_1BIT` | Removed |
| `LV_IMG_CF_INDEXED_2BIT` | Removed |
| `LV_IMG_CF_INDEXED_4BIT` | Removed |
| `LV_IMG_CF_INDEXED_8BIT` | Removed |
| All `LV_IMG_CF_*` | `LV_COLOR_FORMAT_*` |

### Key Color Changes

- `lv_color_t` is **always RGB888** in v9 regardless of `LV_COLOR_DEPTH`
- `LV_COLOR_DEPTH 24` is now supported for direct RGB888 rendering
- `LV_COLOR_16_SWAP` macro removed; use `lv_draw_sw_rgb565_swap()` in flush callback

### Type Renames

| v8 Type | v9 Type |
|---------|---------|
| `lv_img_dsc_t` | `lv_image_dsc_t` |
| `lv_img_decoder_*` | `lv_image_decoder_*` |
| `lv_image_decoder_built_in_open()` | `lv_bin_decoder_open()` |
| `lv_image_decoder_built_in_close()` | `lv_bin_decoder_close()` |

---

## Drawing Pipeline Changes

v9 introduces a completely new drawing architecture based on Draw Tasks and Draw Units.

### Concepts

- **Draw Task**: A rendering operation (rectangle, line, image, etc.) queued for execution
- **Draw Unit**: A logic entity that executes draw tasks (software renderer, GPU, etc.)
- **Draw Pipeline**: Assembly-line architecture routing tasks to appropriate units

### Key Differences from v8

| Aspect | v8 | v9 |
|--------|----|----|
| Rendering | Single-threaded, immediate | Pipeline with parallel capability |
| Custom drawing | `lv_obj_draw_part_dsc_t` | `lv_draw_task_t` |
| GPU integration | Limited | First-class via custom Draw Units |
| Layers | Basic | Full layer system with blending |
| Vector graphics | None | ThorVG via Canvas widget |

---

## Build System Changes

### CMake

v9 provides improved CMake integration:
- `LV_BUILD_USE_KCONFIG ON` enables Kconfig-based configuration
- `LV_BUILD_DEFCONFIG_PATH` points to a defconfig file
- LVGL is designed to integrate into higher-level CMake projects

### Kconfig

- Kconfig support now available (CMake only)
- Uses `kconfiglib` (Python port) for cross-platform support
- `menuconfig` for console-based configuration
- `guiconfig` for graphical configuration
- Kconfig values are overridden by `lv_conf.h` or build settings

### ESP-IDF Integration

- LVGL available as ESP-IDF component via ESP Component Registry
- `esp_lvgl_port` supports both v8 and v9
- Compatible with ESP-IDF v4.4+
- Kconfig integrates natively with ESP-IDF's menuconfig

### Built-in Drivers (New in v9)

v9 includes native drivers previously requiring external code:
- SDL (with window resize and multi-window support)
- Linux Framebuffer
- NuttX LCD
- TFT_eSPI wrapper
- Display controllers: ST7789, ILI9341
- Touch controllers

---

## lv_conf.h Migration

**Do NOT attempt to reuse your v8 lv_conf.h.** Generate a fresh one from `lv_conf_template.h`.

### Key Configuration Changes

| v8 Option | v9 Status |
|-----------|-----------|
| `LV_COLOR_DEPTH` | Still present; behavior changed (internal always RGB888) |
| `LV_COLOR_16_SWAP` | Removed; handle in flush_cb |
| `LV_MEM_SIZE` | Renamed / restructured; v9 uses more memory |
| `LV_USE_GPU_*` | Removed; replaced by Draw Unit architecture |
| `LV_USE_LOG` | Still present |
| `LV_USE_PERF_MONITOR` | Still present |
| `LV_USE_MEM_MONITOR` | Still present |
| `LV_USE_METER` | Removed (widget removed) |
| `LV_USE_MSG` | Removed; replaced by `LV_USE_OBSERVER` |

### New Options in v9

- `LV_USE_OBSERVER` - Observer pattern for data binding
- `LV_USE_OS` - OS abstraction (none, pthread, FreeRTOS, etc.)
- `LV_USE_STDLIB_*` - Standard library selection (LVGL built-in, C lib, or custom)
- `LV_USE_DRAW_SW_*` - Software draw unit configuration
- `LV_USE_SDL` / `LV_USE_LINUX_FB` / etc. - Built-in driver enables
- `LV_COLOR_DEPTH 24` - RGB888 rendering support

### Critical Note

`<stdint.h>` is NOT included in `lv_conf.h` in v9 to preserve assembly compilation optimization. If your configuration relies on stdint types, include it separately.

---

## Code Examples: Before and After

### Example 1: Basic GUI Setup

```c
/* ========== v8 ========== */
#include "lvgl.h"

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 10];

void my_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    /* Send pixels to display */
    lv_disp_flush_ready(drv);
}

void my_touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    data->point.x = touch_x;
    data->point.y = touch_y;
    data->state = touch_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

void setup_gui(void) {
    lv_init();

    /* Display */
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 10);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Input device */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    /* Create a button */
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 100, 50);
    lv_obj_center(btn);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Click me");
    lv_obj_center(label);
}
```

```c
/* ========== v9 ========== */
#include "lvgl.h"

void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    /* Send pixels to display */
    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    data->point.x = touch_x;
    data->point.y = touch_y;
    data->state = touch_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void setup_gui(void) {
    lv_init();

    /* Display */
    lv_display_t *disp = lv_display_create(320, 240);
    lv_display_set_flush_cb(disp, my_flush_cb);
    static uint8_t buf[320 * 10 * 2];  /* Size in bytes (e.g., 2 for RGB565) */
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Input device */
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    /* Create a button */
    lv_obj_t *btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn, 100, 50);
    lv_obj_center(btn);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Click me");
    lv_obj_center(label);
}
```

### Example 2: Timer/Tick Handling (ESP32 Arduino)

```c
/* ========== v8 ========== */
void loop() {
    lv_timer_handler();
    delay(5);
}

/* ========== v9 (explicit tick tracking required) ========== */
unsigned long prior_tick_millis = millis();

void loop() {
    lv_timer_handler();
    unsigned long tick_millis = millis() - prior_tick_millis;
    prior_tick_millis = millis();
    lv_tick_inc(tick_millis);
    yield();
}
```

### Example 3: Styling

```c
/* ========== v8 ========== */
static lv_style_t style;
lv_style_init(&style);
lv_style_set_bg_img_src(&style, &my_bg_img);
lv_style_set_shadow_ofs_x(&style, 5);
lv_style_set_shadow_ofs_y(&style, 5);
lv_style_set_transform_zoom(&style, 512);   /* 2x zoom */
lv_style_set_transform_angle(&style, 450);  /* 45 degrees */
lv_style_set_anim_time(&style, 300);
lv_style_set_img_opa(&style, LV_OPA_50);

/* ========== v9 ========== */
static lv_style_t style;
lv_style_init(&style);
lv_style_set_bg_image_src(&style, &my_bg_img);
lv_style_set_shadow_offset_x(&style, 5);
lv_style_set_shadow_offset_y(&style, 5);
lv_style_set_transform_scale(&style, 512);     /* 2x scale */
lv_style_set_transform_rotation(&style, 450);  /* 45 degrees */
lv_style_set_anim_duration(&style, 300);
lv_style_set_image_opa(&style, LV_OPA_50);
```

---

## Compatibility Layer

LVGL v9 provides `lv_api_map_v8.h` which defines macros and inline functions mapping old v8 names to new v9 names. This allows gradual migration.

### Enabling the Compatibility Layer

Include `lv_api_map_v8.h` or enable `LV_USE_API_MAP_V8` in `lv_conf.h` (if available).

### What It Covers

- Macro-based renames (display, widget, style functions)
- Typedef mappings (`lv_coord_t` -> `int32_t`, `lv_disp_t` -> `lv_display_t`, etc.)
- Inline function wrappers (`lv_task_handler()`, `lv_obj_move_foreground()`, etc.)
- Constant mappings (rotation, render mode, button matrix control flags)

### What It Does NOT Cover

- Structural changes (`lv_disp_drv_t`, `lv_indev_drv_t` removal)
- New display/indev creation API
- Removed widgets (`lv_meter`)
- Drawing architecture changes
- Observer pattern (replacing `lv_msg`)
- Buffer size unit change (pixels -> bytes)

### Recommendation

Use the compatibility layer for initial compilation, then migrate to native v9 API incrementally. The compatibility layer is intended as a transition aid, not a permanent solution.

---

## Memory Impact Notes

- v9 uses approximately **50-100% more RAM** than v8 for equivalent GUIs
- Users migrating from v8 with 55KB `LV_MEM_SIZE` may need 80-110KB+ in v9
- On ESP32, consider using PSRAM for LVGL buffers if internal RAM is tight
- `lv_color_t` being always RGB888 (3 bytes) vs potentially RGB565 (2 bytes) in v8 increases color-related memory usage
- The new draw pipeline architecture uses additional memory for task queuing

### ESP32 Memory Recommendations

| ESP32 Variant | Internal RAM | PSRAM | v9 Viability |
|---------------|-------------|-------|--------------|
| ESP32 (no PSRAM) | 320KB | None | Tight; minimal UI only |
| ESP32 + PSRAM | 320KB | 4-8MB | Good; place buffers in PSRAM |
| ESP32-S3 | 512KB | 2-8MB | Excellent |
| ESP32-C3 | 400KB | None | Tight; minimal UI only |
| ESP32-P4 | 768KB | Up to 32MB | Excellent |

---

## Macro and Constant Renames (Quick Reference)

| v8 Constant | v9 Constant |
|-------------|-------------|
| `LV_RES_OK` | `LV_RESULT_OK` |
| `LV_RES_INV` | `LV_RESULT_INVALID` |
| `LV_INDEV_STATE_PR` | `LV_INDEV_STATE_PRESSED` |
| `LV_INDEV_STATE_REL` | `LV_INDEV_STATE_RELEASED` |
| `LV_DISP_ROTATION_0` | `LV_DISPLAY_ROTATION_0` |
| `LV_DISP_ROTATION_90` | `LV_DISPLAY_ROTATION_90` |
| `LV_DISP_ROTATION_180` | `LV_DISPLAY_ROTATION_180` |
| `LV_DISP_ROTATION_270` | `LV_DISPLAY_ROTATION_270` |
| `LV_DISP_RENDER_MODE_PARTIAL` | `LV_DISPLAY_RENDER_MODE_PARTIAL` |
| `LV_DISP_RENDER_MODE_DIRECT` | `LV_DISPLAY_RENDER_MODE_DIRECT` |
| `LV_DISP_RENDER_MODE_FULL` | `LV_DISPLAY_RENDER_MODE_FULL` |
| `LV_ZOOM_NONE` | `LV_SCALE_NONE` |
| `LV_BTNMATRIX_BTN_NONE` | `LV_BUTTONMATRIX_BUTTON_NONE` |
| `LV_IMGBTN_STATE_RELEASED` | `LV_IMAGEBUTTON_STATE_RELEASED` |
| `LV_IMGBTN_STATE_PRESSED` | `LV_IMAGEBUTTON_STATE_PRESSED` |
| `LV_IMGBTN_STATE_DISABLED` | `LV_IMAGEBUTTON_STATE_DISABLED` |
| `LV_IMGBTN_STATE_CHECKED_RELEASED` | `LV_IMAGEBUTTON_STATE_CHECKED_RELEASED` |
| `LV_IMGBTN_STATE_CHECKED_PRESSED` | `LV_IMAGEBUTTON_STATE_CHECKED_PRESSED` |
| `LV_IMGBTN_STATE_CHECKED_DISABLED` | `LV_IMAGEBUTTON_STATE_CHECKED_DISABLED` |

### Type Renames (Typedef Mappings)

| v8 Type | v9 Type |
|---------|---------|
| `lv_coord_t` | `int32_t` |
| `lv_res_t` | `lv_result_t` |
| `lv_img_dsc_t` | `lv_image_dsc_t` |
| `lv_disp_t` | `lv_display_t` |
| `lv_disp_rotation_t` | `lv_display_rotation_t` |
| `lv_disp_render_t` | `lv_display_render_mode_t` |
| `lv_anim_ready_cb_t` | `lv_anim_completed_cb_t` |
| `lv_scr_load_anim_t` | `lv_screen_load_anim_t` |
| `lv_btnmatrix_ctrl_t` | `lv_buttonmatrix_ctrl_t` |

---

## Sources

- [LVGL v9.0 Changelog (GitHub)](https://github.com/lvgl/lvgl/blob/release/v9.0/docs/CHANGELOG.rst)
- [lv_api_map_v8.h (GitHub)](https://github.com/lvgl/lvgl/blob/master/src/lv_api_map_v8.h)
- [v9 Discussion and Changes (Issue #3298)](https://github.com/lvgl/lvgl/issues/3298)
- [Changes in master v9 development (Issue #4011)](https://github.com/lvgl/lvgl/issues/4011)
- [LVGL v8 to v9 Migration Forum Thread](https://forum.lvgl.io/t/lvgl-v8-to-v9-migration-guide/21647)
- [Personal Migration Experience v8.3 to v9.0](https://forum.lvgl.io/t/personal-experience-migrating-from-v-8-3-x-to-v-9-0-in-the-arduino-environment/14451)
- [lv_event_send Migration Thread](https://forum.lvgl.io/t/lv-event-send-migration-from-v8-to-v9/15075)
- [LVGL v9 Official Documentation](https://docs.lvgl.io/master/index.html)
