# LVGL v9.5 ESP32 Integration Guide

> Detailed guide for integrating LVGL v9.5 with ESP32 chips.
> Covers SPI displays, I2C touch, PSRAM, DMA, FreeRTOS tasks, and performance tuning.

---

## Table of Contents

1. [Project Setup](#1-project-setup)
2. [SPI Display Driver Setup](#2-spi-display-driver-setup)
3. [I2C Touch Controller Setup](#3-i2c-touch-controller-setup)
4. [PSRAM Configuration](#4-psram-configuration)
5. [DMA Buffer Configuration](#5-dma-buffer-configuration)
6. [FreeRTOS Task Setup](#6-freertos-task-setup)
7. [Performance Optimization](#7-performance-optimization)
8. [Complete Integration Example](#8-complete-integration-example)
9. [Troubleshooting](#9-troubleshooting)

---

## 1. Project Setup

### Method A: esp_lvgl_port (Recommended)

The `esp_lvgl_port` component handles LVGL initialization, tick management, and FreeRTOS task creation automatically. This is the preferred method.

```bash
# Add dependencies
idf.py add-dependency "espressif/esp_lvgl_port^2.3.0"
idf.py add-dependency "lvgl/lvgl^9.5.*"
```

### Method B: Direct ESP-IDF Component

```bash
idf.py add-dependency "lvgl/lvgl^9.5.*"
```

### Method C: Git Submodule

```bash
git submodule add https://github.com/lvgl/lvgl.git components/lvgl
cd components/lvgl && git checkout v9.5.0
```

### Method D: Arduino Framework

Install via Arduino Library Manager or PlatformIO:

**Arduino IDE:**
1. Library Manager -> search "lvgl" -> Install v9.5.x
2. Copy `lv_conf_template.h` to Arduino libraries root as `lv_conf.h`
3. Change `#if 0` to `#if 1` in `lv_conf.h`

**PlatformIO (platformio.ini):**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = espidf
lib_deps =
    lvgl/lvgl@^9.5.0
```

### Configuration via menuconfig (ESP-IDF)

```bash
idf.py menuconfig
```

Navigate to: **Component config -> LVGL configuration**

Or create `sdkconfig.defaults`:
```
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_LV_DPI_DEF=130
CONFIG_LV_USE_LOG=y
CONFIG_LV_LOG_LEVEL_WARN=y
```

Chip-specific defaults use `sdkconfig.defaults.esp32s3` naming.

---

## 2. SPI Display Driver Setup

### Supported Display Controllers (SPI)

| Controller | Resolution | Interface | Color Depth | Notes |
|------------|------------|-----------|-------------|-------|
| ILI9341    | 320x240   | SPI       | 16/18-bit   | Most common |
| ILI9488    | 480x320   | SPI       | 16/18-bit   | Larger displays |
| ST7789     | 240x240/320| SPI      | 16-bit      | Popular small displays |
| ST7735     | 128x160   | SPI       | 16-bit      | Small TFT |
| SSD1306    | 128x64    | SPI/I2C   | 1-bit       | OLED |
| GC9A01     | 240x240   | SPI       | 16-bit      | Round displays |
| SH1107     | 128x128   | SPI/I2C   | 1-bit       | OLED |

### Adding Display Driver (ESP-IDF)

```bash
# Example: ILI9341
idf.py add-dependency "espressif/esp_lcd_ili9341"
# Example: ST7789
idf.py add-dependency "espressif/esp_lcd_st7789"
# Example: Round display
idf.py add-dependency "espressif/esp_lcd_gc9a01^2.0.0"
```

### SPI Bus Initialization

```c
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

// Pin definitions
#define LCD_HOST        SPI2_HOST
#define LCD_PIXEL_CLK   (40 * 1000 * 1000)  // 40 MHz
#define PIN_NUM_MOSI    23
#define PIN_NUM_CLK     18
#define PIN_NUM_CS      5
#define PIN_NUM_DC      21
#define PIN_NUM_RST     22
#define PIN_NUM_BCKL    19

// SPI bus configuration
spi_bus_config_t bus_cfg = {
    .sclk_io_num = PIN_NUM_CLK,
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = -1,          // Not needed for display
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 320 * 240 * 2 + 8,  // Full frame + overhead
};
ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
```

### Panel IO Configuration (SPI)

```c
esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num = PIN_NUM_DC,
    .cs_gpio_num = PIN_NUM_CS,
    .pclk_hz = LCD_PIXEL_CLK,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
    .spi_mode = 0,
    .trans_queue_depth = 10,
    .on_color_trans_done = notify_lvgl_flush_ready,  // Callback for async
};
ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));
```

### Panel Creation (ILI9341 Example)

```c
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = PIN_NUM_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
    .bits_per_pixel = 16,
};
ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

// Turn on backlight
gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
gpio_set_level(PIN_NUM_BCKL, 1);
```

### LVGL Display Integration (Manual, without esp_lvgl_port)

```c
#include "lvgl.h"

#define DISP_HOR_RES    320
#define DISP_VER_RES    240

// Draw buffers
#define DRAW_BUF_SIZE   (DISP_HOR_RES * 40)  // 40 lines
static lv_color_t buf1[DRAW_BUF_SIZE];
static lv_color_t buf2[DRAW_BUF_SIZE];  // Double buffering

// Flush callback
static void disp_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;

    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, px_map);
    // flush_ready will be called from SPI transfer done callback
}

// SPI transfer done callback
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_io_event_data_t * edata,
                                     void * user_ctx) {
    lv_display_t * disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

// Initialize LVGL display
void lvgl_display_init(void) {
    lv_init();

    lv_display_t * disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
}
```

### Arduino Display Setup (LovyanGFX)

```cpp
#include <lvgl.h>
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Light_PWM _light;

public:
    LGFX(void) {
        auto cfg = _bus.config();
        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;
        cfg.pin_sclk = 18;
        cfg.pin_mosi = 23;
        cfg.pin_miso = -1;
        cfg.pin_dc = 21;
        _bus.config(cfg);
        _panel.setBus(&_bus);

        auto pcfg = _panel.config();
        pcfg.pin_cs = 5;
        pcfg.pin_rst = 22;
        pcfg.memory_width = 240;
        pcfg.memory_height = 320;
        pcfg.panel_width = 240;
        pcfg.panel_height = 320;
        _panel.config(pcfg);
        setPanel(&_panel);

        auto lcfg = _light.config();
        lcfg.pin_bl = 19;
        _light.config(lcfg);
        _panel.setLight(&_light);
    }
};

LGFX tft;
static lv_display_t * disp;
static uint16_t buf[320 * 40];

void my_disp_flush(lv_display_t * display, const lv_area_t * area, uint8_t * px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((uint16_t *)px_map, w * h);
    tft.endWrite();
    lv_display_flush_ready(display);
}
```

---

## 3. I2C Touch Controller Setup

### Supported Touch Controllers

| Controller | Interface | Multi-touch | Common Displays |
|------------|-----------|-------------|-----------------|
| GT911      | I2C       | 5 points    | 7", 4.3", 5" TFT |
| FT6336/FT5x06 | I2C  | 2 points    | 3.5", 4" TFT |
| CST816S    | I2C       | 1 point     | Small round/square |
| XPT2046    | SPI       | 1 point     | Resistive panels |

### Adding Touch Driver (ESP-IDF)

```bash
# Capacitive touch controllers
idf.py add-dependency "espressif/esp_lcd_touch_gt911"
idf.py add-dependency "espressif/esp_lcd_touch_ft5x06"
idf.py add-dependency "espressif/esp_lcd_touch_cst816s"

# Resistive
idf.py add-dependency "espressif/esp_lcd_touch_xpt2046"
```

### I2C Bus Initialization

```c
#include "driver/i2c.h"
#include "esp_lcd_touch.h"

#define TOUCH_I2C_NUM       I2C_NUM_0
#define TOUCH_I2C_CLK_HZ    400000   // 400 KHz
#define PIN_NUM_TOUCH_SDA   4
#define PIN_NUM_TOUCH_SCL   16
#define PIN_NUM_TOUCH_INT   17
#define PIN_NUM_TOUCH_RST   -1       // Optional

i2c_config_t i2c_cfg = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = PIN_NUM_TOUCH_SDA,
    .scl_io_num = PIN_NUM_TOUCH_SCL,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = TOUCH_I2C_CLK_HZ,
};
ESP_ERROR_CHECK(i2c_param_config(TOUCH_I2C_NUM, &i2c_cfg));
ESP_ERROR_CHECK(i2c_driver_install(TOUCH_I2C_NUM, I2C_MODE_MASTER, 0, 0, 0));
```

### GT911 Touch Controller

```c
#include "esp_lcd_touch_gt911.h"

esp_lcd_touch_handle_t touch_handle = NULL;

esp_lcd_panel_io_handle_t tp_io_handle = NULL;
esp_lcd_panel_io_i2c_config_t tp_io_config =
    ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(
    (esp_lcd_i2c_bus_handle_t)TOUCH_I2C_NUM, &tp_io_config, &tp_io_handle));

esp_lcd_touch_config_t tp_cfg = {
    .x_max = 800,
    .y_max = 480,
    .rst_gpio_num = PIN_NUM_TOUCH_RST,
    .int_gpio_num = PIN_NUM_TOUCH_INT,
    .levels = {
        .reset = 0,
        .interrupt = 0,
    },
    .flags = {
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    },
};
ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle));
```

### FT6336 / FT5x06 Touch Controller

```c
#include "esp_lcd_touch_ft5x06.h"

esp_lcd_panel_io_i2c_config_t tp_io_config =
    ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(
    (esp_lcd_i2c_bus_handle_t)TOUCH_I2C_NUM, &tp_io_config, &tp_io_handle));

esp_lcd_touch_config_t tp_cfg = {
    .x_max = 320,
    .y_max = 240,
    .rst_gpio_num = -1,
    .int_gpio_num = PIN_NUM_TOUCH_INT,
    .flags = {
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    },
};
ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &touch_handle));
```

### CST816S Touch Controller

```c
#include "esp_lcd_touch_cst816s.h"

esp_lcd_panel_io_i2c_config_t tp_io_config =
    ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();

ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(
    (esp_lcd_i2c_bus_handle_t)TOUCH_I2C_NUM, &tp_io_config, &tp_io_handle));

esp_lcd_touch_config_t tp_cfg = {
    .x_max = 240,
    .y_max = 240,
    .rst_gpio_num = PIN_NUM_TOUCH_RST,
    .int_gpio_num = PIN_NUM_TOUCH_INT,
    .flags = {
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    },
};
ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, &touch_handle));
```

### LVGL Touch Input Integration

```c
static void touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data) {
    uint16_t touch_x[1];
    uint16_t touch_y[1];
    uint16_t touch_strength[1];
    uint8_t touch_cnt = 0;

    esp_lcd_touch_read_data(touch_handle);
    bool touched = esp_lcd_touch_get_coordinates(touch_handle,
        touch_x, touch_y, touch_strength, &touch_cnt, 1);

    if (touched && touch_cnt > 0) {
        data->point.x = touch_x[0];
        data->point.y = touch_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// Register with LVGL
lv_indev_t * indev = lv_indev_create();
lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
lv_indev_set_read_cb(indev, touch_read_cb);
```

### Touch Calibration / Adjustment

If touch coordinates don't match display:

```c
// In touch config
.flags = {
    .swap_xy = 1,     // Swap X and Y axes
    .mirror_x = 1,    // Mirror X axis
    .mirror_y = 0,    // Mirror Y axis
},
```

---

## 4. PSRAM Configuration

### When to Use PSRAM

| Use Case | Recommendation |
|----------|---------------|
| Small display (320x240, 16-bit) | Internal SRAM sufficient |
| Medium display (480x320) | PSRAM for framebuffers |
| Large display (800x480) | PSRAM required |
| Multiple screens/widgets | PSRAM for heap |
| Image-heavy UI | PSRAM for image cache |

### ESP32-S3 PSRAM Configuration

```
# sdkconfig.defaults
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_BOOT_INIT=y

# Place BSS/malloc in PSRAM
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=4096
CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768

# Move read-only data to PSRAM (frees Flash bandwidth)
CONFIG_SPIRAM_RODATA=y
```

### ESP32-P4 PSRAM Configuration

```
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_HEX=y
CONFIG_SPIRAM_SPEED_200M=y
CONFIG_SPIRAM_RODATA=y
```

### PSRAM Buffer Allocation

```c
// Allocate LVGL draw buffers in PSRAM (for large displays)
#define DRAW_BUF_SIZE   (DISP_HOR_RES * DISP_VER_RES)  // Full framebuffer
static lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(
    DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
static lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(
    DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

// For direct mode (full framebuffer in PSRAM)
lv_display_set_buffers(disp, buf1, buf2,
    DRAW_BUF_SIZE * sizeof(lv_color_t),
    LV_DISPLAY_RENDER_MODE_DIRECT);
```

### PSRAM Performance Considerations

**Important:** Internal SRAM provides significantly better performance for draw buffers than PSRAM.

| Buffer Location | Relative Performance |
|-----------------|---------------------|
| Internal SRAM   | Baseline (best)     |
| PSRAM (Octal)   | ~60-70% of SRAM     |
| PSRAM (Quad)    | ~40-50% of SRAM     |

**Best practice:** Use small buffers in internal SRAM with partial rendering, rather than full framebuffers in PSRAM. Exception: when using DMA with bounce buffers (see Section 5).

### Memory Allocation Strategy

```c
// Prefer internal SRAM for small, frequently accessed buffers
void *fast_buf = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

// Use PSRAM for large, less frequently accessed data
void *large_buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);

// LVGL heap can use PSRAM for widget data
// Set in lv_conf.h or menuconfig:
// LV_MEM_SIZE = 256*1024 or larger when using PSRAM
```

---

## 5. DMA Buffer Configuration

### SPI DMA for Display

DMA allows the CPU to render the next frame while the previous frame is being sent to the display via SPI.

**Key constraint:** SPI DMA on ESP32 cannot directly access PSRAM. Use bounce buffers to work around this.

```c
// SPI bus with DMA
spi_bus_config_t bus_cfg = {
    .sclk_io_num = PIN_NUM_CLK,
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = -1,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 320 * 80 * 2,  // Max DMA transfer size
};
// SPI_DMA_CH_AUTO lets the driver pick the best DMA channel
ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
```

### Bounce Buffer Mode

For displays connected via RGB interface (ESP32-S3, ESP32-P4), use bounce buffers to transfer PSRAM data via internal SRAM:

1. CPU copies PSRAM framebuffer data to small internal SRAM bounce buffer
2. DMA transfers bounce buffer to LCD peripheral
3. Repeat until frame complete

```c
// Bounce buffer configuration for RGB displays
esp_lcd_rgb_panel_config_t panel_config = {
    .data_width = 16,
    .bounce_buffer_size_px = 10 * DISP_HOR_RES,  // 10 lines in internal SRAM
    // ... other config
};
```

### DMA-Aligned Buffers

```c
// Ensure buffer alignment for DMA
// sdkconfig.defaults:
// CONFIG_LV_DRAW_BUF_ALIGN=4         (default, fine for most)
// CONFIG_LV_DRAW_BUF_ALIGN=64        (required when using PPA on ESP32-P4)
// CONFIG_LV_DRAW_BUF_STRIDE_ALIGN=1  (default)

// Allocate DMA-capable buffer
void *dma_buf = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
```

### Double Buffering

Double buffering is critical for performance. While one buffer is being flushed to the display, LVGL renders into the other.

```c
// Partial mode with double buffering (recommended for SPI displays)
#define BUF_LINES   40  // 10-25% of vertical resolution
static lv_color_t buf1[DISP_HOR_RES * BUF_LINES];
static lv_color_t buf2[DISP_HOR_RES * BUF_LINES];

lv_display_set_buffers(disp, buf1, buf2,
    sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
```

### Buffer Size Guidelines

| Screen Size | Min Buffer (10%) | Optimal Buffer (25%) | Notes |
|-------------|-------------------|----------------------|-------|
| 128x128     | 128x13 = 3.3KB   | 128x32 = 8KB        | Internal SRAM ok |
| 240x240     | 240x24 = 11.5KB  | 240x60 = 28.8KB     | Internal SRAM ok |
| 320x240     | 320x24 = 15.4KB  | 320x60 = 38.4KB     | Internal SRAM ok |
| 480x320     | 480x32 = 30.7KB  | 480x80 = 76.8KB     | May need PSRAM |
| 800x480     | 800x48 = 76.8KB  | 800x120 = 192KB     | PSRAM required |

Buffer sizes assume RGB565 (2 bytes per pixel). Multiply by 2 for double buffering.

---

## 6. FreeRTOS Task Setup

### Using esp_lvgl_port (Recommended)

```c
#include "esp_lvgl_port.h"

const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

// Display configuration
const lvgl_port_display_cfg_t disp_cfg = {
    .io_handle = io_handle,
    .panel_handle = panel_handle,
    .buffer_size = DISP_HOR_RES * 40,
    .double_buffer = true,
    .hres = DISP_HOR_RES,
    .vres = DISP_VER_RES,
    .monochrome = false,
    .rotation = {
        .swap_xy = false,
        .mirror_x = false,
        .mirror_y = false,
    },
};
lv_display_t * disp = lvgl_port_add_disp(&disp_cfg);

// Touch configuration
const lvgl_port_touch_cfg_t touch_cfg = {
    .disp = disp,
    .handle = touch_handle,
};
lv_indev_t * indev = lvgl_port_add_touch(&touch_cfg);

// Thread-safe LVGL access
if (lvgl_port_lock(0)) {
    // Create UI here
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello ESP32!");
    lv_obj_center(label);
    lvgl_port_unlock();
}
```

**Important:** With `esp_lvgl_port`, you do NOT call `lv_timer_handler()` manually. The component runs it in a background task.

### Manual FreeRTOS Task Setup

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t lvgl_mutex = NULL;

// LVGL tick source (use esp_timer for accuracy)
static void lv_tick_task(void *arg) {
    (void)arg;
    lv_tick_inc(2);  // Called every 2ms
}

// Or use tick callback (preferred in v9.5)
static uint32_t my_tick_get_cb(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// LVGL task
static void lvgl_task(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            uint32_t time_till_next = lv_timer_handler();
            xSemaphoreGive(lvgl_mutex);

            if (time_till_next > 500) time_till_next = 500;
            vTaskDelay(pdMS_TO_TICKS(time_till_next));
        }
    }
}

void lvgl_init_with_task(void) {
    lv_init();
    lv_tick_set_cb(my_tick_get_cb);

    lvgl_mutex = xSemaphoreCreateMutex();

    // Create LVGL task
    xTaskCreatePinnedToCore(
        lvgl_task,          // Task function
        "LVGL",             // Name
        8192,               // Stack size (bytes)
        NULL,               // Parameters
        5,                  // Priority (higher = more priority)
        NULL,               // Task handle
        1                   // Core ID (1 = APP core on dual-core ESP32)
    );
}

// Thread-safe UI access from other tasks
void ui_update(void) {
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Update UI here
        xSemaphoreGive(lvgl_mutex);
    }
}
```

### Task Configuration Guidelines

| Parameter | Recommended Value | Notes |
|-----------|-------------------|-------|
| Stack size | 8192-16384 bytes | Larger for complex UI |
| Priority | 5 (medium-high) | Below WiFi/BT tasks |
| Core affinity | Core 1 | Keep Core 0 for WiFi/BT |
| Refresh period | 10-33 ms | 30-100 FPS target |
| Mutex timeout | 100 ms | Prevents deadlocks |

### FreeRTOS sdkconfig Settings

```
CONFIG_FREERTOS_HZ=1000
CONFIG_ESP_MAIN_TASK_AFFINITY_CPU1=y
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
```

---

## 7. Performance Optimization

### Compiler and CPU

```
# sdkconfig.defaults (biggest impact settings first)
CONFIG_COMPILER_OPTIMIZATION_PERF=y           # Up to 30% faster
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y         # Maximum CPU speed
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y              # Fast flash (120M on P4)
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y              # Quad I/O flash
CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM=y       # Critical LVGL code in IRAM
```

### LVGL-Specific Optimizations

```
# sdkconfig.defaults
CONFIG_LV_DEF_REFR_PERIOD=10                 # 10ms = ~100 FPS max
CONFIG_LV_DRAW_SW_COMPLEX=y                   # Enable complex renderer
CONFIG_LV_OBJ_STYLE_CACHE=y                  # Style cache (64-bit per object)
CONFIG_LV_LABEL_LONG_TXT_HINT=y              # Long text performance
```

### Display-Specific Optimizations

**SPI Clock Speed:**
```c
// Maximize SPI clock (check display datasheet for max)
.pclk_hz = 80 * 1000 * 1000,  // 80 MHz (ESP32-S3 max)
// ESP32 vanilla: 40 MHz typical max
// ESP32-S3: 80 MHz typical max
```

**Partial Refresh:**
```c
// Only redraw changed areas (huge savings for mostly static UIs)
lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1),
                       LV_DISPLAY_RENDER_MODE_PARTIAL);
```

**Reduce Color Depth:**
```c
// Use 8-bit color for simple UIs (halves bandwidth)
#define LV_COLOR_DEPTH  8  // RGB332
```

### Widget-Level Optimizations

```c
// Disable scrollbar on static containers
lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

// Use LV_LABEL_LONG_CLIP instead of SCROLL for labels that don't need animation
lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);

// Avoid transparency when possible (software blending is expensive)
lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);  // Fully opaque

// Reduce shadow and blur usage on slow MCUs (new in v9.5)
// Blur and drop shadow work in software but are computationally expensive

// Use static text when possible (avoids string copy)
lv_label_set_text_static(label, "Static text");
```

### Benchmark Reference (ESP32-S3 + SPI LCD)

| Configuration | Average FPS | Weighted FPS |
|---------------|-------------|--------------|
| Default settings | 9-12 | 7-10 |
| + Performance compiler | 10-13 | 9-11 |
| + 240 MHz CPU | 11-14 | 10-12 |
| + IRAM fast mem | 13-16 | 12-14 |
| + All optimizations | 14-18 | 13-16 |

### PPA Accelerator (ESP32-P4)

```
CONFIG_LV_USE_PPA=y
CONFIG_LV_DRAW_BUF_ALIGN=64           # Match cache line size
CONFIG_LV_PPA_BURST_LENGTH=128         # 128/64/32/16/8
```

### Memory Monitoring

```c
// Check heap usage
size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
ESP_LOGI(TAG, "Free internal: %d, Free PSRAM: %d", free_internal, free_psram);

// LVGL memory monitor
lv_mem_monitor_t mon;
lv_mem_monitor(&mon);
ESP_LOGI(TAG, "LVGL mem: used %d%%, frag %d%%",
         (int)mon.used_pct, (int)mon.frag_pct);
```

---

## 8. Complete Integration Example

### ESP-IDF with esp_lvgl_port (Recommended)

```c
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#define TAG "main"

// Pin Definitions
#define LCD_SPI_HOST    SPI2_HOST
#define LCD_H_RES       320
#define LCD_V_RES       240
#define LCD_CLK         18
#define LCD_MOSI        23
#define LCD_CS          5
#define LCD_DC          21
#define LCD_RST         22
#define LCD_BL          19
#define TOUCH_SDA       4
#define TOUCH_SCL       16
#define TOUCH_INT       17

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

static void init_spi_display(void) {
    // SPI bus
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = LCD_CLK,
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    // Panel IO
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_DC,
        .cs_gpio_num = LCD_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_SPI_HOST, &io_config, &io_handle));

    // Panel
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    // Backlight
    gpio_set_direction(LCD_BL, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_BL, 1);
}

static void init_i2c_touch(void) {
    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TOUCH_SDA,
        .scl_io_num = TOUCH_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config =
        ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(
        (esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &tp_io_config, &tp_io));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = TOUCH_INT,
        .flags = { .swap_xy = 0, .mirror_x = 0, .mirror_y = 0 },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io, &tp_cfg, &touch_handle));
}

static void create_ui(void) {
    // Background style with drop shadow (v9.5 feature)
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xE0E0E0), 0);

    // Card panel with shadow
    lv_obj_t * card = lv_obj_create(scr);
    lv_obj_set_size(card, 280, 180);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_shadow_width(card, 20, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_shadow_offset_y(card, 5, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    // Title
    lv_obj_t * title = lv_label_create(card);
    lv_label_set_text(title, "ESP32 + LVGL 9.5");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    // Slider
    lv_obj_t * slider = lv_slider_create(card);
    lv_obj_set_width(slider, 200);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);

    // Value label
    lv_obj_t * val_label = lv_label_create(card);
    lv_label_set_text(val_label, "50");
}

void app_main(void) {
    ESP_LOGI(TAG, "Initializing display...");
    init_spi_display();

    ESP_LOGI(TAG, "Initializing touch...");
    init_i2c_touch();

    ESP_LOGI(TAG, "Initializing LVGL...");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // Add display
    const lvgl_port_display_cfg_t disp_cfg = {
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 40,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
    };
    lv_display_t * disp = lvgl_port_add_disp(&disp_cfg);

    // Add touch
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = touch_handle,
    };
    lvgl_port_add_touch(&touch_cfg);

    // Create UI (thread-safe)
    if (lvgl_port_lock(0)) {
        create_ui();
        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "UI running!");
}
```

### Minimal sdkconfig.defaults

```
# Performance
CONFIG_COMPILER_OPTIMIZATION_PERF=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y

# LVGL
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_LV_DPI_DEF=130
CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM=y
CONFIG_LV_FONT_MONTSERRAT_14=y
CONFIG_LV_FONT_MONTSERRAT_20=y

# FreeRTOS
CONFIG_FREERTOS_HZ=1000
CONFIG_ESP_MAIN_TASK_AFFINITY_CPU1=y

# PSRAM (uncomment if available)
# CONFIG_SPIRAM=y
# CONFIG_SPIRAM_MODE_OCT=y
# CONFIG_SPIRAM_SPEED_80M=y

# Logging
CONFIG_LV_USE_LOG=y
CONFIG_LV_LOG_LEVEL_WARN=y
CONFIG_LV_LOG_PRINTF=y
```

---

## 9. Troubleshooting

### Display Not Working

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Blank screen | Wrong SPI pins or clock | Verify pin mapping, reduce SPI clock |
| White screen | Missing init commands | Check panel_init, add invert_color if needed |
| Garbled display | Wrong color format | Match LV_COLOR_DEPTH with panel bits_per_pixel |
| Colors inverted | RGB/BGR mismatch | Toggle `rgb_ele_order` in panel config |
| Upside down | Mirror/rotation wrong | Use `esp_lcd_panel_mirror()` or `esp_lcd_panel_swap_xy()` |
| Partial rendering | Buffer too small | Increase draw buffer size |

### Touch Not Working

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No response | Wrong I2C address/pins | Check I2C scan, verify SDA/SCL pins |
| Wrong position | Need calibration | Adjust swap_xy, mirror_x, mirror_y flags |
| Inverted X or Y | Mirror flags | Toggle mirror_x or mirror_y |
| Offset coordinates | Resolution mismatch | Match x_max/y_max to display resolution |

### Performance Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Low FPS (<10) | Buffer too small | Increase to 10-25% of screen |
| Laggy scrolling | No double buffering | Enable second buffer |
| Slow animations | CPU at 80/160 MHz | Set to 240 MHz |
| Memory overflow | Too many widgets | Use PSRAM, reduce widget count |
| Flickering | Single buffer mode | Switch to double buffering |

### PSRAM Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| PSRAM not detected | Wrong config | Verify CONFIG_SPIRAM settings for your chip |
| DMA fails with PSRAM | SPI DMA limitation | Use bounce buffers or internal SRAM for DMA buffers |
| Crash on PSRAM access | Cache conflict | Set CONFIG_LV_DRAW_BUF_ALIGN=64 |
| Slow with PSRAM | Expected behavior | Use partial rendering with internal SRAM buffers |

### GPIO12 Boot Trap (ESP32 Vanilla)

**Critical:** On original ESP32, GPIO12 controls flash voltage at boot. If pulled HIGH during boot, the chip may fail to start. Avoid using GPIO12 for display and touch signals that can pull it high at reset.

### Stack Overflow

If LVGL task crashes with stack overflow:
```c
// Increase stack size
xTaskCreatePinnedToCore(lvgl_task, "LVGL", 16384, NULL, 5, NULL, 1);
//                                          ^^^^^
// Or in esp_lvgl_port config
```

---

## Source Links

- ESP-IDF LVGL Integration: https://docs.lvgl.io/9.5/integration/chip_vendors/espressif/add_lvgl_to_esp32_idf_project.html
- ESP32 Tips and Tricks: https://docs.lvgl.io/master/integration/chip_vendors/espressif/tips_and_tricks.html
- esp_lvgl_port Performance: https://github.com/espressif/esp-bsp/blob/master/components/esp_lvgl_port/docs/performance.md
- Arduino Integration: https://docs.lvgl.io/9.5/integration/frameworks/arduino.html
- PlatformIO Integration: https://docs.lvgl.io/9.5/integration/frameworks/platformio.html
- ESP-BSP Repository: https://github.com/espressif/esp-bsp
- LVGL ESP32 Blog Tutorial: https://lvgl.io/blog/tutorial-esp32-getting-started
