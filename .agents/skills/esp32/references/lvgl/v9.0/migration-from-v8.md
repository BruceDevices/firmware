# LVGL v8 to v9 Migration Guide

> Detailed migration guide with before/after code examples for every breaking change.
> Structured for AI agent consumption during ESP32 firmware migration.

---

## Migration Severity: CRITICAL

LVGL v9 is a complete architectural overhaul. This is NOT a drop-in replacement.
Expect to modify nearly every file that interacts with LVGL.

### Migration Strategy
1. Start fresh `lv_conf.h` from `lv_conf_template.h`
2. Fix display driver initialization first
3. Fix input device initialization second
4. Global find-and-replace for renamed prefixes
5. Fix individual API changes
6. Test thoroughly -- some changes cause silent runtime bugs

---

## 1. Configuration File (lv_conf.h)

### CRITICAL: Do NOT reuse v8 lv_conf.h

```c
// WRONG: Trying to reuse v8 config will cause mysterious errors
// RIGHT: Copy lv_conf_template.h fresh and reconfigure

// Step 1: Copy template
// cp lvgl/lv_conf_template.h lv_conf.h

// Step 2: Enable the file
// Change first line from:
#if 0  // Set to 1 to enable
// To:
#if 1

// Step 3: CRITICAL - Do NOT include <stdint.h> in lv_conf.h
// v9 has assembly parts that break with random includes
```

### Key Config Changes

```c
// ==========================================
// v8 lv_conf.h
// ==========================================
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U)
#define LV_DISP_DEF_REFR_PERIOD 30
#define LV_USE_GPU_STM32_DMA2D 0
#define LV_USE_METER 1
#define LV_USE_MSG 1

// ==========================================
// v9 lv_conf.h
// ==========================================
#define LV_COLOR_DEPTH 16
// LV_COLOR_16_SWAP removed -- handle in flush_cb or use LV_COLOR_FORMAT_NATIVE_REVERSED
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN   // replaces LV_MEM_CUSTOM
#define LV_MEM_SIZE (48U * 1024U)
#define LV_DEF_REFR_PERIOD 30                    // renamed from LV_DISP_DEF_REFR_PERIOD
#define LV_USE_DRAW_DMA2D 0                      // replaces LV_USE_GPU_STM32_DMA2D
// LV_USE_METER removed -- use LV_USE_SCALE instead
// LV_USE_MSG removed -- use LV_USE_OBSERVER instead
#define LV_USE_OBSERVER 1
#define LV_USE_OS LV_OS_NONE                     // NEW: OS/threading support

// NEW v9 config sections:
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
#define LV_USE_DRAW_SW 1
#define LV_DRAW_SW_DRAW_UNIT_CNT 1               // SW render threads
#define LV_DRAW_BUF_STRIDE_ALIGN 1
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE (24 * 1024)
#define LV_USE_VECTOR_GRAPHIC 0                   // ThorVG support
#define LV_USE_FLOAT 0                            // Float support
```

### Widget Enable/Disable Renames

```c
// v8                        v9
#define LV_USE_BTN 1         #define LV_USE_BUTTON 1
#define LV_USE_BTNMATRIX 1   #define LV_USE_BUTTONMATRIX 1
#define LV_USE_IMG 1         #define LV_USE_IMAGE 1
#define LV_USE_IMGBTN 1      #define LV_USE_IMAGEBUTTON 1
#define LV_USE_METER 1       // REMOVED -- use lv_scale
#define LV_USE_MSG 1         #define LV_USE_OBSERVER 1
```

---

## 2. Display Driver Migration

This is the most impactful change. The entire driver registration pattern changed.

### v8 Display Setup (BEFORE)
```c
#include "lvgl.h"

static lv_disp_draw_buf_t draw_buf_dsc;
static lv_color_t buf1[320 * 40];
static lv_color_t buf2[320 * 40];
static lv_disp_drv_t disp_drv;

void lvgl_display_init(void) {
    // Initialize draw buffer
    lv_disp_draw_buf_init(&draw_buf_dsc, buf1, buf2, 320 * 40);

    // Initialize driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_flush_cb;
    disp_drv.draw_buf = &draw_buf_dsc;
    disp_drv.sw_rotate = 0;
    disp_drv.rotated = LV_DISP_ROT_NONE;
    disp_drv.monitor_cb = my_monitor_cb;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
}

// v8 flush callback
void my_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    // Send pixels to display...
    lv_disp_flush_ready(drv);
}

// v8 monitor callback
void my_monitor_cb(lv_disp_drv_t *drv, uint32_t time, uint32_t px) {
    printf("Refresh: %d ms, %d px\n", time, px);
}
```

### v9 Display Setup (AFTER)
```c
#include "lvgl.h"

// Buffer size in BYTES, not pixels
// For RGB565: width * height * 2 bytes per pixel
static uint8_t buf1[320 * 40 * 2];
static uint8_t buf2[320 * 40 * 2];

void lvgl_display_init(void) {
    // Create display with resolution
    lv_display_t *disp = lv_display_create(320, 240);

    // Set flush callback
    lv_display_set_flush_cb(disp, my_flush_cb);

    // Set buffers (size in BYTES, not pixels!)
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set color format
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

    // Rotation (replaces sw_rotate + rotated)
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);

    // Monitor callback replaced by event
    lv_display_add_event_cb(disp, my_render_ready_cb,
                            LV_EVENT_RENDER_READY, NULL);
}

// v9 flush callback -- NOTE: uint8_t* not lv_color_t*
void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // Send pixels to display...

    // For SPI displays that need byte-swapped RGB565:
    // uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);
    // lv_draw_sw_rgb565_swap(px_map, size);

    lv_display_flush_ready(disp);
}

// v9 render ready event (replaces monitor_cb)
void my_render_ready_cb(lv_event_t *e) {
    // Render complete notification
}
```

### ESP32 Specific Display Migration

```c
// ==========================================
// v8: ESP32 with SPI display
// ==========================================
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;
static lv_color_t *buf2;

void esp32_display_init_v8(void) {
    buf1 = (lv_color_t *)heap_caps_malloc(
        320 * 40 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    buf2 = (lv_color_t *)heap_caps_malloc(
        320 * 40 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 320 * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = esp_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

// ==========================================
// v9: ESP32 with SPI display
// ==========================================
static uint8_t *buf1;
static uint8_t *buf2;
#define BUF_SIZE (320 * 40 * 2)  // RGB565 = 2 bytes/pixel

void esp32_display_init_v9(void) {
    buf1 = (uint8_t *)heap_caps_malloc(BUF_SIZE, MALLOC_CAP_DMA);
    buf2 = (uint8_t *)heap_caps_malloc(BUF_SIZE, MALLOC_CAP_DMA);

    lv_display_t *disp = lv_display_create(320, 240);
    lv_display_set_flush_cb(disp, esp_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, BUF_SIZE,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
}
```

---

## 3. Input Device Migration

### v8 Input Device (BEFORE)
```c
static lv_indev_drv_t indev_drv;

void lvgl_indev_init(void) {
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
}

// v8 read callback
void my_touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    if (touchpad_is_pressed()) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touchpad_get_x();
        data->point.y = touchpad_get_y();
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
```

### v9 Input Device (AFTER)
```c
void lvgl_indev_init(lv_display_t *disp) {
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    lv_indev_set_display(indev, disp);  // Associate with display
}

// v9 read callback -- NOTE: lv_indev_t* not lv_indev_drv_t*
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    if (touchpad_is_pressed()) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touchpad_get_x();
        data->point.y = touchpad_get_y();
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
```

### Encoder Input (v8 vs v9)
```c
// v8
static lv_indev_drv_t enc_drv;
lv_indev_drv_init(&enc_drv);
enc_drv.type = LV_INDEV_TYPE_ENCODER;
enc_drv.read_cb = encoder_read;
lv_indev_t *enc_indev = lv_indev_drv_register(&enc_drv);
lv_indev_set_group(enc_indev, group);

// v9
lv_indev_t *enc_indev = lv_indev_create();
lv_indev_set_type(enc_indev, LV_INDEV_TYPE_ENCODER);
lv_indev_set_read_cb(enc_indev, encoder_read);
lv_indev_set_group(enc_indev, group);
lv_indev_set_display(enc_indev, disp);
```

---

## 4. Type Changes

### lv_coord_t -> int32_t

```c
// v8
lv_coord_t x = lv_obj_get_x(obj);
lv_coord_t w = lv_obj_get_width(obj);
void my_func(lv_coord_t value);
static lv_coord_t col_dsc[] = {100, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

// v9
int32_t x = lv_obj_get_x(obj);
int32_t w = lv_obj_get_width(obj);
void my_func(int32_t value);
static int32_t col_dsc[] = {100, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

// MIGRATION: Global find-replace lv_coord_t with int32_t
```

### lv_color_t Internal Representation

```c
// v8: lv_color_t size depends on LV_COLOR_DEPTH
//     LV_COLOR_DEPTH 16 -> lv_color_t is 2 bytes (RGB565)
//     LV_COLOR_DEPTH 32 -> lv_color_t is 4 bytes (ARGB8888)
sizeof(lv_color_t);  // v8: 2 or 4 bytes depending on config

// v9: lv_color_t is ALWAYS 3 bytes (RGB888)
//     LV_COLOR_DEPTH only affects display OUTPUT format
sizeof(lv_color_t);  // v9: always 3 bytes (RGB888)

// IMPACT: Any code that calculates buffer sizes using sizeof(lv_color_t) is WRONG
// v8: buf_size = width * height * sizeof(lv_color_t)  // correct in v8
// v9: buf_size depends on display color format, NOT sizeof(lv_color_t)
//     RGB565: buf_size = width * height * 2
//     RGB888: buf_size = width * height * 3
//     ARGB8888: buf_size = width * height * 4
```

---

## 5. Naming Convention Changes (Find-Replace)

### Prefix Renames (Apply globally)

```
FIND                          REPLACE
----                          -------
lv_disp_                     lv_display_
lv_btn_create                lv_button_create
lv_btn_                      lv_button_
lv_btnmatrix_create          lv_buttonmatrix_create
lv_btnmatrix_                lv_buttonmatrix_
lv_img_create                lv_image_create
lv_img_set_src               lv_image_set_src
lv_img_set_zoom              lv_image_set_scale
lv_img_set_angle             lv_image_set_rotation
lv_img_                      lv_image_
lv_imgbtn_                   lv_imagebutton_
LV_IMG_DECLARE               LV_IMAGE_DECLARE
LV_IMG_CF_                   LV_COLOR_FORMAT_
```

### Specific Function Renames

```c
// v8                                v9
lv_scr_act()                        lv_screen_active()
lv_disp_get_scr_act(disp)           lv_display_get_screen_active(disp)
lv_scr_load(scr)                    lv_screen_load(scr)
lv_scr_load_anim(...)               lv_screen_load_anim(...)
lv_obj_del(obj)                     lv_obj_delete(obj)
lv_obj_del_async(obj)               lv_obj_delete_async(obj)
lv_obj_clear_flag(obj, f)           lv_obj_remove_flag(obj, f)
lv_obj_clear_state(obj, s)          lv_obj_remove_state(obj, s)
lv_obj_set_zoom(obj, z)             // REMOVED -- use style transform
lv_img_set_zoom(obj, z)             lv_image_set_scale(obj, z)
lv_img_set_angle(obj, a)            lv_image_set_rotation(obj, a)
```

### Property Name Changes in Style

```c
// v8                                          v9
lv_style_set_img_opa(&s, v)                   lv_style_set_image_opa(&s, v)
lv_style_set_img_recolor(&s, v)               lv_style_set_image_recolor(&s, v)
lv_style_set_img_recolor_opa(&s, v)           lv_style_set_image_recolor_opa(&s, v)

// Transform renames
lv_obj_set_style_transform_zoom(obj, v, sel)  lv_obj_set_style_transform_scale_x(obj, v, sel)
                                               lv_obj_set_style_transform_scale_y(obj, v, sel)
lv_obj_set_style_transform_angle(obj, v, sel) lv_obj_set_style_transform_rotation(obj, v, sel)

// Background image
lv_style_set_bg_img_src(&s, v)                lv_style_set_bg_image_src(&s, v)
lv_style_set_bg_img_opa(&s, v)                lv_style_set_bg_image_opa(&s, v)
lv_style_set_bg_img_recolor(&s, v)            lv_style_set_bg_image_recolor(&s, v)
lv_style_set_bg_img_recolor_opa(&s, v)        lv_style_set_bg_image_recolor_opa(&s, v)
lv_style_set_bg_img_tiled(&s, v)              lv_style_set_bg_image_tiled(&s, v)
```

---

## 6. Event System Migration

### Event Callback Changes

```c
// v8 event callback
void my_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);  // Returns lv_obj_t*
    void *user_data = lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        printf("Clicked!\n");
    }
}
lv_obj_add_event_cb(btn, my_event_cb, LV_EVENT_CLICKED, NULL);

// v9 event callback
void my_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target_obj(e);  // CHANGED: use _obj suffix
    // lv_event_get_target(e) returns void* in v9!
    void *user_data = lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        printf("Clicked!\n");
    }
}
lv_obj_add_event_cb(btn, my_event_cb, LV_EVENT_CLICKED, NULL);

// CRITICAL: lv_event_get_target() return type changed from lv_obj_t* to void*
// This will NOT produce a compiler error in C but may cause subtle bugs
// Always use lv_event_get_target_obj() in v9
```

### Custom Event Draw Handlers

```c
// v8: Custom drawing in events
void draw_event_cb(lv_event_t *e) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    // Use dsc to customize drawing...
}

// v9: Draw task system replaces draw part descriptors
void draw_event_cb(lv_event_t *e) {
    lv_draw_task_t *task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t *base = lv_draw_task_get_draw_dsc(task);
    // Use draw task + descriptors...
}

// NOTE: lv_obj_draw_part_dsc_t is REMOVED in v9
// Use lv_draw_task_t and lv_draw_dsc_base_t instead
```

---

## 7. Widget Migration Examples

### Button Creation

```c
// v8
lv_obj_t *btn = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn, 120, 50);
lv_obj_t *label = lv_label_create(btn);
lv_label_set_text(label, "Click me");
lv_obj_center(label);
lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

// v9
lv_obj_t *btn = lv_button_create(lv_screen_active());  // btn->button, scr_act->screen_active
lv_obj_set_size(btn, 120, 50);
lv_obj_t *label = lv_label_create(btn);
lv_label_set_text(label, "Click me");
lv_obj_center(label);
lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
```

### Image Widget

```c
// v8
LV_IMG_DECLARE(my_icon);
lv_obj_t *img = lv_img_create(lv_scr_act());
lv_img_set_src(img, &my_icon);
lv_img_set_zoom(img, 512);    // 2x zoom (256 = 1x)
lv_img_set_angle(img, 450);   // 45.0 degrees (1/10 degree units)

// v9
LV_IMAGE_DECLARE(my_icon);                          // IMG -> IMAGE
lv_obj_t *img = lv_image_create(lv_screen_active()); // img->image, scr_act->screen_active
lv_image_set_src(img, &my_icon);                     // img->image
lv_image_set_scale(img, 512);                        // zoom -> scale
lv_image_set_rotation(img, 450);                     // angle -> rotation

// NEW v9: Image alignment/stretch/tile
lv_image_set_inner_align(img, LV_IMAGE_ALIGN_STRETCH);
lv_image_set_inner_align(img, LV_IMAGE_ALIGN_TILE);
```

### Switch

```c
// v8
lv_obj_t *sw = lv_switch_create(lv_scr_act());
lv_obj_add_state(sw, LV_STATE_CHECKED);     // Turn on
lv_obj_clear_state(sw, LV_STATE_CHECKED);   // Turn off

// v9
lv_obj_t *sw = lv_switch_create(lv_screen_active());
lv_obj_add_state(sw, LV_STATE_CHECKED);     // Turn on (same)
lv_obj_remove_state(sw, LV_STATE_CHECKED);  // clear -> remove
```

### Meter -> Scale Migration

```c
// v8: lv_meter (REMOVED in v9)
lv_obj_t *meter = lv_meter_create(lv_scr_act());
lv_meter_set_scale_range(meter, scale, 0, 100, 270, 135);
lv_meter_set_scale_ticks(meter, scale, 41, 2, 10, lv_palette_main(LV_PALETTE_GREY));
lv_meter_set_scale_major_ticks(meter, scale, 8, 4, 15, lv_color_black(), 10);
lv_meter_indicator_t *indic = lv_meter_add_needle_line(meter, scale, 4,
    lv_palette_main(LV_PALETTE_RED), -10);
lv_meter_set_indicator_value(meter, indic, 50);

// v9: lv_scale (replacement)
lv_obj_t *scale = lv_scale_create(lv_screen_active());
lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
lv_scale_set_range(scale, 0, 100);
lv_scale_set_angle_range(scale, 270);
lv_scale_set_rotation(scale, 135);
lv_scale_set_total_tick_count(scale, 41);
lv_scale_set_major_tick_every(scale, 8);
lv_scale_set_label_show(scale, true);

// For needle/indicator, use custom drawing or overlaid widgets
// lv_scale does NOT have built-in needle support like lv_meter did
```

### Button Matrix

```c
// v8
static const char *btnm_map[] = {"Btn1", "Btn2", "\n", "Btn3", ""};
lv_obj_t *btnm = lv_btnmatrix_create(lv_scr_act());
lv_btnmatrix_set_map(btnm, btnm_map);
lv_btnmatrix_set_btn_ctrl(btnm, 0, LV_BTNMATRIX_CTRL_CHECKABLE);

// v9
static const char *btnm_map[] = {"Btn1", "Btn2", "\n", "Btn3", ""};
lv_obj_t *btnm = lv_buttonmatrix_create(lv_screen_active());  // btnmatrix->buttonmatrix
lv_buttonmatrix_set_map(btnm, btnm_map);
lv_buttonmatrix_set_button_ctrl(btnm, 0, LV_BUTTONMATRIX_CTRL_CHECKABLE);
// All lv_btnmatrix_* -> lv_buttonmatrix_*
// LV_BTNMATRIX_CTRL_* -> LV_BUTTONMATRIX_CTRL_*
```

### Canvas

```c
// v8
static lv_color_t buf[320 * 240];
lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
lv_canvas_set_buffer(canvas, buf, 320, 240, LV_IMG_CF_TRUE_COLOR);
lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);

// v9
LV_DRAW_BUF_DEFINE(draw_buf, 320, 240, LV_COLOR_FORMAT_ARGB8888);
lv_obj_t *canvas = lv_canvas_create(lv_screen_active());
lv_canvas_set_draw_buf(canvas, &draw_buf);  // set_buffer -> set_draw_buf
lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);

// Canvas drawing in v9 uses layer API:
lv_layer_t *layer = lv_canvas_init_layer(canvas);
lv_draw_rect_dsc_t rect_dsc;
lv_draw_rect_dsc_init(&rect_dsc);
rect_dsc.bg_color = lv_color_hex(0xFF0000);
lv_area_t area = {10, 10, 100, 100};
lv_draw_rect(layer, &rect_dsc, &area);
lv_canvas_finish_layer(canvas, layer);
```

---

## 8. Color Format Migration

### Image Color Format Constants

```c
// v8                              v9
LV_IMG_CF_TRUE_COLOR              LV_COLOR_FORMAT_NATIVE
LV_IMG_CF_TRUE_COLOR_ALPHA        LV_COLOR_FORMAT_NATIVE_ALPHA
LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED // REMOVED (no replacement)
LV_IMG_CF_INDEXED_1BIT            LV_COLOR_FORMAT_I1
LV_IMG_CF_INDEXED_2BIT            LV_COLOR_FORMAT_I2
LV_IMG_CF_INDEXED_4BIT            LV_COLOR_FORMAT_I4
LV_IMG_CF_INDEXED_8BIT            LV_COLOR_FORMAT_I8
LV_IMG_CF_ALPHA_1BIT              // REMOVED
LV_IMG_CF_ALPHA_2BIT              // REMOVED
LV_IMG_CF_ALPHA_4BIT              // REMOVED
LV_IMG_CF_RAW                     // REMOVED
LV_IMG_CF_RAW_ALPHA               // REMOVED
LV_IMG_CF_RAW_CHROMA_KEYED        // REMOVED
```

### Color Swap for SPI Displays

```c
// v8: Global config
#define LV_COLOR_16_SWAP 1  // In lv_conf.h

// v9: Per-display or manual
// Option A: Set display format
lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);

// Option B: Swap in flush callback
void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);
    lv_draw_sw_rgb565_swap(px_map, size);
    // ... send to display
    lv_display_flush_ready(disp);
}
```

---

## 9. Observer Pattern Migration (lv_msg -> lv_observer)

```c
// v8: lv_msg (message passing)
#define MSG_TEMPERATURE_CHANGED 1

// Publishing
lv_msg_send(MSG_TEMPERATURE_CHANGED, &temperature);

// Subscribing
lv_msg_subscribe(MSG_TEMPERATURE_CHANGED, msg_handler, NULL);

void msg_handler(void *subscriber, lv_msg_t *msg) {
    float *temp = (float *)lv_msg_get_payload(msg);
    lv_label_set_text_fmt(temp_label, "%.1f C", *temp);
}

// v9: lv_observer (reactive data binding)
static lv_subject_t temperature_subject;

// Initialize
lv_subject_init_int(&temperature_subject, 0);

// Subscribe with auto-cleanup on widget deletion
lv_subject_add_observer_obj(&temperature_subject, temp_observer_cb, temp_label, NULL);

void temp_observer_cb(lv_observer_t *observer, lv_subject_t *subject) {
    int32_t temp = lv_subject_get_int(subject);
    lv_obj_t *label = lv_observer_get_target_obj(observer);
    lv_label_set_text_fmt(label, "%d C", temp);
}

// Update (notifies all observers)
lv_subject_set_int(&temperature_subject, new_temperature);

// Two-way binding example
lv_obj_t *sw = lv_switch_create(lv_screen_active());
lv_obj_bind_checked(sw, &enabled_subject);  // Auto sync switch <-> subject
```

---

## 10. Memory and Allocation Migration

```c
// v8
#define LV_MEM_CUSTOM 0            // 0=built-in, 1=custom
#define LV_MEM_SIZE (48 * 1024)
// With custom:
#define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC malloc
#define LV_MEM_CUSTOM_FREE free
#define LV_MEM_CUSTOM_REALLOC realloc

// v9
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN  // or LV_STDLIB_CLIB, LV_STDLIB_CUSTOM
#define LV_MEM_SIZE (48 * 1024)
// With clib:
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
// With custom: provide lv_malloc, lv_free, lv_realloc implementations
```

---

## 11. Tick and Timer Migration

```c
// v8: Same concept, slightly different in practice
lv_tick_inc(tick_ms);      // Call from ISR/timer
lv_timer_handler();        // Call from main loop

// v9: Same API, but timing is more critical
lv_tick_inc(tick_ms);      // Call from ISR/timer
uint32_t next_ms = lv_timer_handler();  // Returns time until next call needed

// CRITICAL for ESP32 Arduino migration:
// You MUST track time properly or widgets behave erratically
unsigned long prev_millis = millis();
void handle_lvgl(void) {
    lv_timer_handler();
    unsigned long tick_ms = millis() - prev_millis;
    prev_millis = millis();
    lv_tick_inc(tick_ms);
}
```

---

## 12. Draw Pipeline Migration

### Custom Draw Hooks

```c
// v8: Draw events used lv_obj_draw_part_dsc_t
lv_obj_add_event_cb(chart, draw_hook, LV_EVENT_DRAW_PART_BEGIN, NULL);

void draw_hook(lv_event_t *e) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (dsc->part == LV_PART_ITEMS) {
        // Modify drawing...
    }
}

// v9: Draw task system
lv_obj_add_event_cb(chart, draw_hook, LV_EVENT_DRAW_TASK_ADDED, NULL);
lv_obj_add_flag(chart, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

void draw_hook(lv_event_t *e) {
    lv_draw_task_t *task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t *base_dsc = lv_draw_task_get_draw_dsc(task);

    if (base_dsc->part == LV_PART_ITEMS) {
        // Modify drawing via draw task...
    }
}
```

---

## 13. File Path Migration

```c
// v8
lv_img_set_src(img, "S:path/to/file.bin");  // Drive letter + path

// v9
lv_image_set_src(img, "S:path/to/file.bin");  // Same pattern, just img->image
// Drive letters still work the same way
```

---

## 14. Removed Features

| Feature | Status | Migration Path |
|---------|--------|---------------|
| `lv_meter` widget | REMOVED | Use `lv_scale` + custom drawing |
| `lv_msg` (messaging) | REMOVED | Use `lv_observer` |
| `lv_coord_t` type | REMOVED | Use `int32_t` |
| `lv_disp_drv_t` | REMOVED | Use `lv_display_t` + setter functions |
| `lv_disp_draw_buf_t` | REMOVED | Integrated into `lv_display_set_buffers()` |
| `lv_indev_drv_t` | REMOVED | Use `lv_indev_t` + setter functions |
| `LV_COLOR_16_SWAP` | REMOVED | Use color format or manual swap |
| `LV_IMG_CF_*` constants | REMOVED | Use `LV_COLOR_FORMAT_*` |
| `LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED` | REMOVED | No replacement |
| `LV_IMG_CF_ALPHA_*BIT` | REMOVED | Use indexed formats |
| `lv_obj_draw_part_dsc_t` | REMOVED | Use `lv_draw_task_t` + `lv_draw_dsc_base_t` |
| `monitor_cb` (display) | REMOVED | Use `LV_EVENT_RENDER_READY` event |
| Chart ticks | REMOVED | Use `lv_scale` widget |
| Display background color/image | REMOVED | Use `lv_layer_bottom()` |

---

## 15. Common Compiler Errors and Fixes

### Error: `unknown type name 'lv_disp_drv_t'`
**Fix**: Replace entire display driver init with new API (see Section 2).

### Error: `unknown type name 'lv_indev_drv_t'`
**Fix**: Replace entire input device init with new API (see Section 3).

### Error: `unknown type name 'lv_coord_t'`
**Fix**: Replace with `int32_t`.

### Error: `implicit declaration of function 'lv_btn_create'`
**Fix**: Use `lv_button_create()`.

### Error: `implicit declaration of function 'lv_img_create'`
**Fix**: Use `lv_image_create()`.

### Error: `implicit declaration of function 'lv_obj_del'`
**Fix**: Use `lv_obj_delete()`.

### Error: `implicit declaration of function 'lv_obj_clear_flag'`
**Fix**: Use `lv_obj_remove_flag()`.

### Error: `'LV_IMG_CF_TRUE_COLOR' undeclared`
**Fix**: Use `LV_COLOR_FORMAT_NATIVE`.

### Error: `implicit declaration of function 'lv_scr_act'`
**Fix**: Use `lv_screen_active()`.

### Error: `'lv_disp_t' undeclared`
**Fix**: Use `lv_display_t`.

### Error: `too few arguments to function 'lv_disp_draw_buf_init'`
**Fix**: Function removed. Use `lv_display_set_buffers()`.

### Warning: `assignment from incompatible pointer type` (event target)
**Fix**: `lv_event_get_target()` returns `void*` in v9. Use `lv_event_get_target_obj()`.

---

## 16. Silent Runtime Issues (No Compiler Error)

These changes compile fine but cause bugs at runtime:

### 1. Buffer Size in Bytes vs Pixels
```c
// WRONG: Using pixel count (v8 habit)
lv_display_set_buffers(disp, buf1, buf2, 320 * 40, mode);  // Bug! Too small!

// RIGHT: Using byte count
lv_display_set_buffers(disp, buf1, buf2, 320 * 40 * 2, mode);  // RGB565: 2 bytes/px
```

### 2. sizeof(lv_color_t) Changed
```c
// WRONG: v8 buffer allocation pattern
lv_color_t *buf = malloc(320 * 40 * sizeof(lv_color_t));  // Always 3 bytes now!

// RIGHT: Calculate based on display color depth
uint8_t *buf = malloc(320 * 40 * 2);  // For RGB565 output
```

### 3. Event Target Type
```c
// COMPILES but WRONG: lv_event_get_target returns void* in v9
lv_obj_t *target = lv_event_get_target(e);  // Silent cast in C

// RIGHT:
lv_obj_t *target = lv_event_get_target_obj(e);  // Explicit obj return
```

### 4. Tick Timing
```c
// Missing or improper tick handling causes erratic widget behavior
// ALWAYS ensure lv_tick_inc() is called with accurate elapsed time
```

---

## 17. Complete Minimal ESP32 Example (v9)

```c
#include "lvgl.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"

#define DISP_HOR_RES 320
#define DISP_VER_RES 240
#define DISP_BUF_SIZE (DISP_HOR_RES * 40 * 2)  // RGB565: 2 bytes/pixel

static uint8_t *buf1;
static uint8_t *buf2;
static lv_display_t *disp;

// Flush callback
static void flush_cb(lv_display_t *d, const lv_area_t *area, uint8_t *px_map) {
    // Your ESP-IDF LCD panel write function here
    // e.g., esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1,
    //        area->x2 + 1, area->y2 + 1, px_map);
    lv_display_flush_ready(d);
}

// Touch read callback
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    // Your touch panel read here
    data->state = LV_INDEV_STATE_RELEASED;
}

// Tick callback
static void tick_cb(void *arg) {
    lv_tick_inc(1);
}

void app_main(void) {
    // 1. Initialize LVGL
    lv_init();

    // 2. Create display
    disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

    // 3. Allocate DMA buffers
    buf1 = heap_caps_malloc(DISP_BUF_SIZE, MALLOC_CAP_DMA);
    buf2 = heap_caps_malloc(DISP_BUF_SIZE, MALLOC_CAP_DMA);
    lv_display_set_buffers(disp, buf1, buf2, DISP_BUF_SIZE,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 4. Create input device
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
    lv_indev_set_display(indev, disp);

    // 5. Create tick timer (1ms)
    const esp_timer_create_args_t tick_args = {
        .callback = tick_cb, .name = "lv_tick"
    };
    esp_timer_handle_t tick_timer;
    esp_timer_create(&tick_args, &tick_timer);
    esp_timer_start_periodic(tick_timer, 1000);  // 1ms

    // 6. Create UI
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello LVGL v9!");
    lv_obj_center(label);

    // 7. Main loop
    while (1) {
        uint32_t next_ms = lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(next_ms));
    }
}
```

---

## 18. Find-Replace Migration Script

Apply these in order. Use whole-word matching where possible.

```
# Phase 1: Types
lv_coord_t -> int32_t
lv_disp_t -> lv_display_t

# Phase 2: Display (order matters -- do specific first)
lv_disp_draw_buf_init -> // REMOVED: rewrite display init
lv_disp_drv_init -> // REMOVED: rewrite display init
lv_disp_drv_register -> // REMOVED: rewrite display init
lv_disp_flush_ready -> lv_display_flush_ready
lv_disp_get_scr_act -> lv_display_get_screen_active

# Phase 3: Input
lv_indev_drv_init -> // REMOVED: rewrite input init
lv_indev_drv_register -> // REMOVED: rewrite input init

# Phase 4: Screen
lv_scr_act() -> lv_screen_active()
lv_scr_load -> lv_screen_load
lv_scr_load_anim -> lv_screen_load_anim

# Phase 5: Object functions
lv_obj_del( -> lv_obj_delete(
lv_obj_del_async( -> lv_obj_delete_async(
lv_obj_clear_flag -> lv_obj_remove_flag
lv_obj_clear_state -> lv_obj_remove_state

# Phase 6: Widgets (do longer matches first)
lv_btnmatrix_ -> lv_buttonmatrix_
LV_BTNMATRIX_ -> LV_BUTTONMATRIX_
lv_btn_create -> lv_button_create
lv_imgbtn_ -> lv_imagebutton_
lv_img_set_src -> lv_image_set_src
lv_img_set_zoom -> lv_image_set_scale
lv_img_set_angle -> lv_image_set_rotation
lv_img_create -> lv_image_create
lv_img_ -> lv_image_
LV_IMG_DECLARE -> LV_IMAGE_DECLARE

# Phase 7: Style properties
lv_style_set_img_ -> lv_style_set_image_
bg_img_src -> bg_image_src
bg_img_opa -> bg_image_opa
bg_img_recolor -> bg_image_recolor
bg_img_tiled -> bg_image_tiled
transform_zoom -> transform_scale_x  // NOTE: may need both _x and _y
transform_angle -> transform_rotation

# Phase 8: Color formats
LV_IMG_CF_TRUE_COLOR_ALPHA -> LV_COLOR_FORMAT_NATIVE_ALPHA
LV_IMG_CF_TRUE_COLOR -> LV_COLOR_FORMAT_NATIVE
LV_IMG_CF_ -> LV_COLOR_FORMAT_  // Catch remaining

# Phase 9: Events
lv_event_get_target( -> lv_event_get_target_obj(  // CRITICAL
```

---

## Quick Reference Card

| v8 | v9 | Category |
|----|----| -------- |
| `lv_disp_drv_t` | `lv_display_t` + setters | Display |
| `lv_disp_draw_buf_t` | `lv_display_set_buffers()` | Display |
| `lv_indev_drv_t` | `lv_indev_t` + setters | Input |
| `lv_coord_t` | `int32_t` | Types |
| `lv_color_t` (16/32 bit) | `lv_color_t` (always RGB888) | Types |
| `lv_scr_act()` | `lv_screen_active()` | Screen |
| `lv_obj_del()` | `lv_obj_delete()` | Object |
| `lv_obj_clear_flag()` | `lv_obj_remove_flag()` | Object |
| `lv_obj_clear_state()` | `lv_obj_remove_state()` | Object |
| `lv_btn_create()` | `lv_button_create()` | Widgets |
| `lv_img_create()` | `lv_image_create()` | Widgets |
| `lv_btnmatrix_create()` | `lv_buttonmatrix_create()` | Widgets |
| `lv_meter` | `lv_scale` | Widgets |
| `lv_msg` | `lv_observer` | Messaging |
| `LV_IMG_CF_*` | `LV_COLOR_FORMAT_*` | Image |
| `LV_IMG_DECLARE` | `LV_IMAGE_DECLARE` | Image |
| `zoom` | `scale` | Properties |
| `angle` | `rotation` | Properties |
| `monitor_cb` | `LV_EVENT_RENDER_READY` | Display |
| Buffer size: pixels | Buffer size: bytes | Display |
| `lv_event_get_target()` -> `lv_obj_t*` | `lv_event_get_target_obj()` -> `lv_obj_t*` | Events |
