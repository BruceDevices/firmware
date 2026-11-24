# LVGL v8.3.x Comprehensive Reference

> LVGL (Light and Versatile Graphics Library) version 8.3 release series.
> 12 releases from v8.3.0 (July 2022) through v8.3.11 (December 2023).

---

## 1. Release Dates and Patch Versions

| Version   | Release Date       | Type        | Key Focus                                      |
|-----------|--------------------|-------------|-------------------------------------------------|
| v8.3.0    | 2022-07-06         | Feature     | Major feature release (layers, color fonts, GPU)|
| v8.3.1    | 2022-07-25         | Patch       | Early fixes, example corrections                |
| v8.3.2    | 2022-09-27         | Patch       | CI/test improvements, documentation updates     |
| v8.3.3    | 2022-10-06         | Patch       | Core functionality fixes                         |
| v8.3.4    | 2022-12-15         | Patch       | Targeted maintenance fixes                       |
| v8.3.5    | 2023-02-07         | Patch       | Performance optimizations, stability             |
| v8.3.6    | 2023-04-03         | Patch       | Messaging system expansion, FS fixes             |
| v8.3.7    | 2023-05-03         | Patch       | Btnmatrix width limits, ARM-2D fixes             |
| v8.3.8    | 2023-07-05         | Patch       | PXP/VGLite GPU improvements, RTL fixes           |
| v8.3.9    | 2023-08-06         | Patch       | Alpha image decoder, font optimizer fixes        |
| v8.3.10   | 2023-09-20         | Patch       | Double-buffer sync, layer opacity backport       |
| v8.3.11   | 2023-12-06         | Patch       | Tiny TTF backport, LittleFS, table user_data     |

---

## 2. New Features in 8.3 (Changes from 8.2)

### Layer-Based Transformations
Widgets and their children can be transformed (zoom and rotate) by rendering into intermediate layers that are drawn as images with transformations applied. This enables complex visual effects without per-widget rendering logic.

```c
// Layer transformation is automatic when transform styles are applied
lv_obj_set_style_transform_zoom(obj, 512, 0);   // 2x zoom (256 = 1x)
lv_obj_set_style_transform_angle(obj, 450, 0);   // 45.0 degrees (0.1 deg units)
```

### Color Font Support (imgfont)
Emojis and images can be embedded directly in text labels via the imgfont system. Enable with `LV_USE_IMGFONT 1` in `lv_conf.h`.

```c
lv_imgfont_init(); // Initialize imgfont system
// Register callback to provide image data for Unicode code points
```

### PubSub Messaging System
Publisher-subscriber messaging framework for inter-component communication. Enable with `LV_USE_MSG 1`.

```c
lv_msg_subscribe(MSG_ID, callback, user_data);       // Subscribe
lv_msg_send(MSG_ID, &payload);                         // Publish
lv_msg_subscribe_obj(MSG_ID, obj, user_data);          // Object-bound subscription
lv_msg_unsubscribe_obj(MSG_ID, obj);                   // Added in v8.3.6
```

### Pinyin IME (Chinese Input)
Chinese input method editor with standard and 9-key modes. Enable with `LV_USE_IME_PINYIN 1`.

```c
lv_obj_t * ime = lv_ime_pinyin_create(parent);
lv_ime_pinyin_set_keyboard(ime, keyboard);
lv_ime_pinyin_set_mode(ime, LV_IME_PINYIN_MODE_K9);  // 9-key mode
```

### Fragment Manager
UI controller concept for managing screen fragments. Enable with `LV_USE_FRAGMENT 1`.

```c
lv_fragment_manager_t * manager = lv_fragment_manager_create(NULL);
lv_fragment_t * frag = lv_fragment_create(&fragment_class, args);
lv_fragment_manager_replace(manager, frag, &container);
```

### GPU Acceleration Additions
| GPU Backend               | Config Macro                   | Hardware Target           |
|---------------------------|--------------------------------|---------------------------|
| NXP PXP                   | `LV_USE_GPU_NXP_PXP`          | i.MX RT processors       |
| NXP VGLite                | `LV_USE_GPU_NXP_VG_LITE`      | i.MX RT/i.MX processors  |
| ARM-2D                    | `LV_USE_GPU_ARM2D`            | Cortex-M with Helium/DMA |
| Synwit SWM341 DMA2D       | `LV_USE_GPU_SWM341_DMA2D`     | SWM341 MCU               |
| STM32 DMA2D               | `LV_USE_GPU_STM32_DMA2D`      | STM32 with Chrom-ART     |
| Renesas RA6M3 G2D         | `LV_USE_GPU_RA6M3_G2D`        | RA6M3 MCU                |
| SDL GPU                   | `LV_USE_GPU_SDL`              | Desktop simulation        |

### Style Property Inheritance and Initial Values
Style properties can be set to `inherit` (use parent's value) or `initial` (system default) in addition to normal values.

```c
lv_obj_set_style_bg_color(obj, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
// New in 8.3: inherit and initial value support
```

### Render Start Callback
New `render_start_cb` in `lv_disp_drv_t` for synchronization (e.g., TE signal wait).

```c
void my_render_start_cb(lv_disp_drv_t * drv) {
    // Wait for TE signal or sync event
}
disp_drv.render_start_cb = my_render_start_cb;
```

### Screen-Out Animations
Support for animating screens when they are unloaded, in addition to load animations.

### Vertical Scrolling in Tabview
Tabview widget now supports vertical tab scrolling.

### GridNav Focus Management
Enhanced grid-based keyboard navigation for widget grids. Enable with `LV_USE_GRIDNAV 1`.

---

## 3. New Widgets and Components in 8.3

### New in v8.3.0

| Widget/Component          | Type   | Config Macro         | Description                                    |
|---------------------------|--------|----------------------|------------------------------------------------|
| IME Pinyin                | Extra  | `LV_USE_IME_PINYIN`  | Chinese Pinyin input method editor             |
| imgfont                   | Other  | `LV_USE_IMGFONT`     | Image-based font for emojis in text            |
| Message (msg)             | Other  | `LV_USE_MSG`         | PubSub messaging system                        |
| Fragment                  | Other  | `LV_USE_FRAGMENT`    | UI fragment/controller manager                 |
| GridNav                   | Other  | `LV_USE_GRIDNAV`     | Grid-based keyboard navigation                 |
| Snapshot                  | Other  | `LV_USE_SNAPSHOT`    | Capture widget as image                        |
| Monkey                    | Other  | `LV_USE_MONKEY`      | Random input generator for stress testing      |

### Backported in Patch Releases

| Feature                   | Version  | Config Macro         | Description                                    |
|---------------------------|----------|----------------------|------------------------------------------------|
| Tiny TTF                  | v8.3.11  | `LV_USE_TINY_TTF`    | Lightweight TrueType font rendering            |
| LittleFS driver           | v8.3.11  | `LV_USE_FS_LITTLEFS` | LittleFS file system integration               |
| Layer opacity (opa_layered)| v8.3.10 | (style property)     | Per-layer opacity control                      |

---

## 4. API Changes

### New Functions Added in v8.3.0

#### Arc Widget
```c
void lv_arc_align_obj_to_angle(lv_obj_t * arc, lv_obj_t * obj, lv_coord_t r_offset);
void lv_arc_rotate_obj_to_angle(lv_obj_t * arc, lv_obj_t * obj, lv_coord_t r_offset);
```

#### Dropdown Widget
```c
uint16_t lv_dropdown_get_option_index(lv_obj_t * dropdown, const char * option);
```

#### Tabview Widget
```c
void lv_tabview_rename_tab(lv_obj_t * tabview, uint32_t tab_idx, const char * new_name);
```

#### Display Driver
```c
// New callback field in lv_disp_drv_t
void (*render_start_cb)(struct _lv_disp_drv_t * disp_drv);
```

#### Messaging System (new module)
```c
void lv_msg_init(void);
uint32_t lv_msg_register_id(void);
void lv_msg_subscribe(uint32_t msg_id, lv_msg_subscribe_cb_t cb, void * user_data);
void lv_msg_subscribe_obj(uint32_t msg_id, lv_obj_t * obj, void * user_data);
void lv_msg_unsubscribe(uint32_t msg_id);
void lv_msg_send(uint32_t msg_id, const void * payload);
```

#### Async Call
```c
lv_result_t lv_async_call_cancel(lv_async_cb_t async_xcb, void * user_data);
```

### Functions Added in Patch Releases

#### v8.3.6
```c
void lv_msg_unsubscribe_obj(uint32_t msg_id, lv_obj_t * obj);
```

#### v8.3.11
```c
// Table cell user_data
void lv_table_set_cell_user_data(lv_obj_t * table, uint16_t row, uint16_t col, void * user_data);
void * lv_table_get_cell_user_data(lv_obj_t * table, uint16_t row, uint16_t col);
```

### Deprecated/Changed Functions
- No functions were formally deprecated in 8.3.x; the API is additive over 8.2
- Button matrix width values capped at maximum 15 (enforced from v8.3.7)
- `lv_disp_drv_t` struct extended with `render_start_cb` field (backward compatible)

---

## 5. Display and Input Driver Improvements

### Display Driver (`lv_disp_drv_t`)

#### Complete Field Reference

| Field                | Type          | Description                                         |
|----------------------|---------------|-----------------------------------------------------|
| `hor_res`            | `lv_coord_t`  | Horizontal resolution in pixels                     |
| `ver_res`            | `lv_coord_t`  | Vertical resolution in pixels                       |
| `physical_hor_res`   | `lv_coord_t`  | Full physical width (for partial screen use)        |
| `physical_ver_res`   | `lv_coord_t`  | Full physical height                                |
| `offset_x`           | `lv_coord_t`  | Horizontal offset for partial displays              |
| `offset_y`           | `lv_coord_t`  | Vertical offset for partial displays                |
| `draw_buf`           | pointer       | Initialized `lv_disp_draw_buf_t`                    |
| `flush_cb`           | callback      | **Required.** Copy buffer to display hardware       |
| `rounder_cb`         | callback      | Round redraw area coordinates (for aligned refresh) |
| `set_px_cb`          | callback      | Custom pixel write (for non-standard depths)        |
| `monitor_cb`         | callback      | Refresh statistics (pixels, time elapsed)           |
| `wait_cb`            | callback      | Wait/yield during rendering                         |
| `clean_dcache_cb`    | callback      | Clear CPU data cache for display buffers            |
| `render_start_cb`    | callback      | **New in 8.3.** Notify when rendering begins        |
| `gpu_fill_cb`        | callback      | Hardware-accelerated rectangle fill                 |
| `gpu_wait_cb`        | callback      | Wait for GPU operation completion                   |
| `color_chroma_key`   | `lv_color_t`  | Transparent color for chroma-keyed images           |
| `anti_aliasing`      | `uint32_t:1`  | Edge smoothing (default on for 16+ bit)             |
| `rotated`            | `uint32_t:2`  | Display rotation flag                               |
| `sw_rotate`          | `uint32_t:1`  | Software rotation enable                            |
| `screen_transp`      | `uint32_t:1`  | Transparent screen background                       |
| `full_refresh`       | `uint32_t:1`  | Force full screen redraw every frame                |
| `direct_mode`        | `uint32_t:1`  | Draw directly into framebuffer                      |
| `user_data`          | `void *`      | Custom driver context                               |

#### Buffering Strategies

```c
// Single buffer (minimum RAM, slower)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[DISP_HOR_RES * 10];
lv_disp_draw_buf_init(&draw_buf, buf1, NULL, DISP_HOR_RES * 10);

// Double buffer (recommended for DMA)
static lv_color_t buf1[DISP_HOR_RES * 10];
static lv_color_t buf2[DISP_HOR_RES * 10];
lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISP_HOR_RES * 10);

// Full framebuffer (fastest, most RAM)
static lv_color_t buf1[DISP_HOR_RES * DISP_VER_RES];
static lv_color_t buf2[DISP_HOR_RES * DISP_VER_RES];
lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISP_HOR_RES * DISP_VER_RES);
```

#### v8.3.10: Double-Buffered Direct Mode Sync
Efficient synchronization algorithm for double-buffered direct-mode rendering, reducing tearing artifacts.

### Input Driver (`lv_indev_drv_t`)

| Input Type               | Enum                      | Data Fields                    |
|--------------------------|---------------------------|--------------------------------|
| Touchpad/Mouse           | `LV_INDEV_TYPE_POINTER`   | `point.x`, `point.y`, `state` |
| Keyboard                 | `LV_INDEV_TYPE_KEYPAD`    | `key`, `state`                 |
| Rotary Encoder           | `LV_INDEV_TYPE_ENCODER`   | `enc_diff`, `state`            |
| Hardware Buttons          | `LV_INDEV_TYPE_BUTTON`    | `btn_id`, `state`              |

#### Configuration Parameters

| Parameter               | Default | Description                                    |
|-------------------------|---------|------------------------------------------------|
| `scroll_limit`          | 10      | Pixels before scroll activates                 |
| `scroll_throw`          | 10      | Momentum deceleration (%)                      |
| `long_press_time`       | 400ms   | Time for `LV_EVENT_LONG_PRESSED`               |
| `long_press_repeat_time`| 100ms   | Repeat interval during long press              |
| `gesture_min_velocity`  | 3       | Min velocity to register gesture               |
| `gesture_limit`         | 50      | Min travel to register gesture                 |

---

## 6. File System Abstraction

### Supported Backends

| Backend     | Config Macro          | Drive Letter | Description                         |
|-------------|-----------------------|-------------|-------------------------------------|
| STDIO       | `LV_USE_FS_STDIO`    | Configurable| Standard C file I/O                 |
| POSIX       | `LV_USE_FS_POSIX`    | Configurable| POSIX file operations               |
| WIN32       | `LV_USE_FS_WIN32`    | Configurable| Windows file API                    |
| FATFS       | `LV_USE_FS_FATFS`    | Configurable| FatFs for embedded (SD cards)       |
| LittleFS    | `LV_USE_FS_LITTLEFS` | Configurable| LittleFS (added in v8.3.11)         |

### File System Driver API

```c
// Register a custom file system driver
lv_fs_drv_t drv;
lv_fs_drv_init(&drv);
drv.letter = 'S';                    // Drive letter
drv.open_cb = my_open_cb;            // Open file
drv.close_cb = my_close_cb;          // Close file
drv.read_cb = my_read_cb;            // Read data
drv.write_cb = my_write_cb;          // Write data
drv.seek_cb = my_seek_cb;            // Seek position
drv.tell_cb = my_tell_cb;            // Get position
drv.dir_open_cb = my_dir_open_cb;    // Open directory
drv.dir_read_cb = my_dir_read_cb;    // Read directory entry
drv.dir_close_cb = my_dir_close_cb;  // Close directory
lv_fs_drv_register(&drv);

// File operations
lv_fs_file_t file;
lv_fs_open(&file, "S:path/to/file.txt", LV_FS_MODE_RD);
lv_fs_read(&file, buf, buf_size, &bytes_read);
lv_fs_close(&file);
```

### v8.3.6 Fix
- POSIX file system permissions corrected

### v8.3.11 Addition
- LittleFS integration as native file system driver, particularly useful for ESP32 flash storage

---

## 7. Image Decoder

### Built-in Color Formats

| Format                              | Enum                                | Description                     |
|-------------------------------------|-------------------------------------|---------------------------------|
| True Color                          | `LV_IMG_CF_TRUE_COLOR`              | RGB only                        |
| True Color + Alpha                  | `LV_IMG_CF_TRUE_COLOR_ALPHA`        | RGB with per-pixel alpha        |
| True Color + Chroma Key             | `LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED` | Color-based transparency        |
| Indexed 1/2/4/8-bit                 | `LV_IMG_CF_INDEXED_1/2/4/8BIT`      | Palette-based compression       |
| Alpha 1/2/4/8-bit                   | `LV_IMG_CF_ALPHA_1/2/4/8BIT`        | Monochrome with variable alpha  |
| Raw                                 | `LV_IMG_CF_RAW`                     | External decoder required       |
| Raw + Alpha                         | `LV_IMG_CF_RAW_ALPHA`              | External decoder + alpha        |
| Raw + Chroma Key                    | `LV_IMG_CF_RAW_CHROMA_KEYED`       | External decoder + chroma key   |

### External Decoder Libraries

| Library    | Config Macro    | Format Support         |
|------------|----------------|------------------------|
| libpng     | `LV_USE_PNG`    | PNG files              |
| libbmp     | `LV_USE_BMP`    | BMP files              |
| SJPG       | `LV_USE_SJPG`   | Split-JPEG (streaming) |
| GIF        | `LV_USE_GIF`    | GIF animation          |
| QR Code    | `LV_USE_QRCODE` | QR code generation     |
| FreeType   | `LV_USE_FREETYPE`| TTF/OTF fonts         |
| Tiny TTF   | `LV_USE_TINY_TTF`| Lightweight TTF (v8.3.11)|
| RLottie    | `LV_USE_RLOTTIE`| Lottie animations      |
| FFmpeg     | `LV_USE_FFMPEG` | Video/audio formats    |

### Custom Image Decoder API

```c
lv_img_decoder_t * dec = lv_img_decoder_create();
lv_img_decoder_set_info_cb(dec, my_info_cb);    // Get image dimensions/format
lv_img_decoder_set_open_cb(dec, my_open_cb);    // Decode full image or prepare streaming
lv_img_decoder_set_read_line_cb(dec, my_read_cb); // Read decoded line (streaming)
lv_img_decoder_set_close_cb(dec, my_close_cb);  // Free resources
```

### Image Caching
- Controlled by `LV_IMG_CACHE_DEF_SIZE` (default: 0, disabled)
- Each cache entry holds one decoded image in RAM
- Reduces repeated decode overhead for frequently used images

### v8.3.8 Fixes
- PNG decode image sizing corrected
- Big-endian image type detection fixed

### v8.3.9 Fixes
- Alpha 8-bit image decoder corrected

---

## 8. Font Handling

### Built-in Fonts

| Font                        | Config Macro                   | Size  | BPP | Character Set              |
|-----------------------------|-------------------------------|-------|-----|----------------------------|
| Montserrat 8-48             | `LV_FONT_MONTSERRAT_8..48`    | 8-48px| 4   | ASCII + symbols + FontAwesome|
| Montserrat 12 Subpx         | `LV_FONT_MONTSERRAT_12_SUBPX` | 12px  | 4   | ASCII (subpixel rendered)  |
| Montserrat 28 Compressed    | `LV_FONT_MONTSERRAT_28_COMPRESSED`| 28px| 3   | ASCII (RLE compressed)     |
| DejaVu 16 Persian/Hebrew    | `LV_FONT_DEJAVU_16_PERSIAN_HEBREW`| 16px| 4  | Persian + Hebrew           |
| Simsun 16 CJK               | `LV_FONT_SIMSUN_16_CJK`      | 16px  | 4   | CJK characters             |
| UNSCII 8/16                 | `LV_FONT_UNSCII_8/16`         | 8/16px| 1   | ASCII (pixel perfect)      |

### Font System Features

| Feature                  | Config Macro                     | Description                          |
|--------------------------|----------------------------------|--------------------------------------|
| UTF-8 Encoding           | `LV_TXT_ENC`                     | Default text encoding                |
| Bidirectional Text       | `LV_USE_BIDI`                    | RTL/LTR mixed text support           |
| Arabic/Persian Shaping   | `LV_USE_ARABIC_PERSIAN_CHARS`    | Character joining/shaping            |
| Font Compression         | `LV_USE_FONT_COMPRESSED`         | RLE compression (~30% size reduction)|
| Subpixel Rendering       | `LV_USE_FONT_SUBPX`             | 3x horizontal resolution via RGB     |
| Placeholder Character    | `LV_USE_FONT_PLACEHOLDER`        | Show placeholder for missing glyphs  |
| Large Font Support       | `LV_FONT_FMT_TXT_LARGE`          | Fonts > 400KB or > 16-bit glyph IDs |

### Font Fallback Chain
```c
// Chain fonts: primary delegates missing glyphs to fallback
lv_font_t my_font = { ... };
my_font.fallback = &lv_font_montserrat_14;  // Fallback font
```

### Runtime Font Loading
```c
lv_font_t * font = lv_font_load("S:path/to/font.bin");
// Use with styles
lv_obj_set_style_text_font(label, font, 0);
// Free when done
lv_font_free(font);
```

### New in 8.3: Color Fonts (imgfont)
```c
// Enable in lv_conf.h: LV_USE_IMGFONT 1
// Register image font with callback
lv_font_t * imgfont = lv_imgfont_create(height, img_src_cb);
// Callback provides image source for each Unicode code point
// Supports emojis and inline images in labels
```

### v8.3.11: Tiny TTF Backport
```c
// Enable in lv_conf.h: LV_USE_TINY_TTF 1
// Lightweight TrueType rendering without FreeType dependency
lv_font_t * font = lv_tiny_ttf_create_file("S:font.ttf", font_size);
// Or from memory
lv_font_t * font = lv_tiny_ttf_create_data(ttf_data, data_len, font_size);
lv_tiny_ttf_destroy(font);
```

### v8.3.8 Fix
- Hebrew RTL character detection enhanced

### v8.3.9 Fix
- Font optimizer issues resolved

---

## 9. ESP32-Specific Notes

### Supported ESP32 Variants

| Chip       | CPU           | Max Freq | PSRAM   | Best Use Case              |
|------------|---------------|----------|---------|----------------------------|
| ESP32      | Dual Xtensa   | 240 MHz  | 4-8 MB  | General purpose with LVGL  |
| ESP32-S2   | Single Xtensa | 240 MHz  | 2-8 MB  | Cost-effective displays    |
| ESP32-S3   | Dual Xtensa   | 240 MHz  | 2-8 MB  | Best for LVGL (SIMD, RGB) |
| ESP32-C3   | Single RISC-V | 160 MHz  | None    | Simple UIs only            |
| ESP32-C6   | Single RISC-V | 160 MHz  | None    | Simple UIs, WiFi 6        |

### Performance Optimization for ESP32

#### Compiler and CPU Settings
```ini
# sdkconfig optimizations
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=240
CONFIG_COMPILER_OPTIMIZATION_PERF=y    # Performance-priority compilation
```
- Performance mode compilation can yield up to 30% faster LVGL execution
- SIMD instructions available on ESP32-S3 for graphics operations

#### Memory Configuration
```c
// lv_conf.h recommended for ESP32
#define LV_MEM_SIZE (48U * 1024U)        // Internal heap (adjust based on available RAM)
#define LV_MEM_CUSTOM 1                   // Use ESP-IDF heap (recommended)
#define LV_MEM_CUSTOM_ALLOC heap_caps_malloc
#define LV_MEM_CUSTOM_FREE heap_caps_free
```

#### Display Buffer Sizing
```c
// Minimum: 1/10 of screen height
static lv_color_t buf1[SCREEN_WIDTH * (SCREEN_HEIGHT / 10)];

// Recommended for DMA: double buffer
static lv_color_t buf1[SCREEN_WIDTH * 40];
static lv_color_t buf2[SCREEN_WIDTH * 40];

// Use DMA-capable memory on ESP32
heap_caps_malloc(size, MALLOC_CAP_DMA);
```

#### Multi-Core Processing
- LVGL task can run on Core 1 (the second core) for improved performance
- Main application logic stays on Core 0
- Available only on dual-core chips (ESP32, ESP32-S3)

```c
// Pin LVGL task to Core 1
xTaskCreatePinnedToCore(lvgl_task, "LVGL", 4096, NULL, 5, NULL, 1);
```

#### SWAR Pixel Optimization
SIMD-within-a-Register optimization in flush drivers: swap and invert two 16-bit pixels simultaneously (32-bit operation), doubling pixel processing throughput.

#### ESP-IDF Component Integration
LVGL 8.3.x is available as an ESP-IDF component:
```
idf.py add-dependency "lvgl/lvgl^8.3"
```

#### Recommended Display Drivers for ESP32
| Interface | Driver           | Notes                                      |
|-----------|------------------|--------------------------------------------|
| SPI       | ST7789, ILI9341  | Most common, up to 80MHz SPI               |
| I2C       | SSD1306          | OLED, slow refresh                         |
| 8080/i80  | ST7789, ILI9488  | Parallel 8/16-bit, faster than SPI         |
| RGB       | ST7701, EK9716   | ESP32-S3 only, direct framebuffer          |

### ESP-BSP (Board Support Packages)
Espressif provides `esp_lvgl_port` component for simplified LVGL integration:
- Handles LVGL task creation and tick
- Mutex protection for thread safety
- Display and touch driver abstraction

---

## 10. Bug Fixes Across Patch Releases

### v8.3.1 (2022-07-25)
- Example corrections and documentation fixes
- Early core functionality patches

### v8.3.2 (2022-09-27)
- CI/test improvements
- Documentation updates
- Various widget fixes

### v8.3.3 (2022-10-06)
- Core functionality corrections
- Rendering fixes

### v8.3.4 (2022-12-15)
- Targeted maintenance fixes across multiple subsystems

### v8.3.5 (2023-02-07)
- Performance optimizations
- Stability improvements across multiple subsystems

### v8.3.6 (2023-04-03)
- Default group wild pointer fix on deletion
- File system POSIX permissions corrected
- Image negative angle support added
- Arc knob area invalidation fix
- Animation cleanup on value changes
- Added `lv_msg_unsubscribe_obj()` function

### v8.3.7 (2023-05-03)
- Spinbox keyboard input character duplication fixed
- ARM-2D transform chroma-keying corrected
- Menu page re-selection prevention
- Color mixing with byte-swapped mode fixed
- Background gradient color transitions improved
- Button matrix width values capped at max 15

### v8.3.8 (2023-07-05)
- PXP and VGLite GPU feature expansion
- PNG decode image sizing corrected
- Hebrew RTL character detection enhanced
- Big-endian image type detection fixed
- Button matrix width value constraints enforced
- Flex grow style registration fix

### v8.3.9 (2023-08-06)
- Alpha 8-bit image decoder corrected
- Font optimizer issues resolved
- Hidden objects no longer retain focus
- Button matrix array bounds addressing
- Chart division-by-zero prevention

### v8.3.10 (2023-09-20)
- Windows MinGW build compatibility
- Invalid area copying in non-double-buffered mode fixed
- SDL renderer texture parameter corrections
- Button matrix tap detection refined
- Double-buffered direct-mode sync algorithm added

### v8.3.11 (2023-12-06)
- Scroll readjustment after layout when children removed
- RT-Thread compatibility improvements
- Table cell value formatting issues resolved
- Arc widget click detection outside background angle range
- Dropdown option matching refined (no more partial matches)

---

## 11. Configuration Changes in lv_conf.h

### New Configuration Options in 8.3

| Macro                               | Default | Category     | Description                                |
|--------------------------------------|---------|--------------|--------------------------------------------|
| `LV_USE_GPU_ARM2D`                   | 0       | GPU          | ARM-2D GPU acceleration                    |
| `LV_USE_GPU_SWM341_DMA2D`            | 0       | GPU          | Synwit SWM341 DMA2D acceleration           |
| `LV_USE_GPU_RA6M3_G2D`               | 0       | GPU          | Renesas RA6M3 G2D acceleration             |
| `LV_USE_IMGFONT`                     | 0       | Font         | Image-based font (emojis in text)          |
| `LV_USE_MSG`                         | 0       | Feature      | PubSub messaging system                    |
| `LV_USE_FRAGMENT`                    | 0       | Feature      | UI fragment manager                        |
| `LV_USE_GRIDNAV`                     | 0       | Feature      | Grid-based keyboard navigation             |
| `LV_USE_SNAPSHOT`                    | 0       | Feature      | Widget snapshot to image                   |
| `LV_USE_MONKEY`                      | 0       | Testing      | Random input generator                     |
| `LV_USE_IME_PINYIN`                  | 0       | Input        | Chinese Pinyin IME                         |
| `LV_USE_TINY_TTF`                    | 0       | Font         | Tiny TTF renderer (v8.3.11)                |
| `LV_USE_FS_LITTLEFS`                | 0       | File System  | LittleFS driver (v8.3.11)                  |
| `LV_LAYER_SIMPLE_BUF_SIZE`           | 24*1024 | Drawing      | Layer rendering buffer size                |
| `LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE`  | 3*1024  | Drawing      | Layer fallback buffer size                 |
| `LV_GRADIENT_MAX_STOPS`              | 2       | Drawing      | Max gradient color stops                   |
| `LV_GRAD_CACHE_DEF_SIZE`             | 0       | Drawing      | Gradient cache size                        |
| `LV_DITHER_GRADIENT`                 | 0       | Drawing      | Gradient dithering for smoother transitions|
| `LV_DISP_ROT_MAX_BUF`               | 10*1024 | Display      | Max buffer for software rotation           |

### Complete lv_conf.h Section Reference

| Section                    | Key Macros                                                    |
|----------------------------|---------------------------------------------------------------|
| Color Settings             | `LV_COLOR_DEPTH`, `LV_COLOR_16_SWAP`, `LV_COLOR_SCREEN_TRANSP`, `LV_COLOR_CHROMA_KEY` |
| Memory                     | `LV_MEM_CUSTOM`, `LV_MEM_SIZE`, `LV_MEM_ADR`, `LV_MEM_BUF_MAX_NUM` |
| HAL Settings               | `LV_DISP_DEF_REFR_PERIOD`, `LV_INDEV_DEF_READ_PERIOD`, `LV_TICK_CUSTOM`, `LV_DPI_DEF` |
| Drawing                    | `LV_DRAW_COMPLEX`, `LV_SHADOW_CACHE_SIZE`, `LV_CIRCLE_CACHE_SIZE`, `LV_IMG_CACHE_DEF_SIZE` |
| GPU Acceleration           | `LV_USE_GPU_ARM2D`, `LV_USE_GPU_STM32_DMA2D`, `LV_USE_GPU_NXP_PXP`, `LV_USE_GPU_NXP_VG_LITE`, `LV_USE_GPU_SDL` |
| Logging                    | `LV_USE_LOG`, `LV_LOG_LEVEL`, `LV_LOG_PRINTF`, `LV_LOG_TRACE_*` |
| Assertions                 | `LV_USE_ASSERT_NULL`, `LV_USE_ASSERT_MALLOC`, `LV_USE_ASSERT_STYLE`, `LV_USE_ASSERT_OBJ` |
| Fonts                      | `LV_FONT_MONTSERRAT_*`, `LV_FONT_DEFAULT`, `LV_USE_FONT_COMPRESSED`, `LV_USE_FONT_SUBPX` |
| Text                       | `LV_TXT_ENC`, `LV_TXT_BREAK_CHARS`, `LV_USE_BIDI`, `LV_TXT_COLOR_CMD` |
| Core Widgets               | `LV_USE_ARC..LV_USE_TABLE` (15 widgets)                      |
| Extra Widgets              | `LV_USE_ANIMIMG..LV_USE_WIN` (17 widgets)                    |
| Themes                     | `LV_USE_THEME_DEFAULT`, `LV_USE_THEME_BASIC`, `LV_USE_THEME_MONO` |
| Layouts                    | `LV_USE_FLEX`, `LV_USE_GRID`                                 |
| File Systems               | `LV_USE_FS_STDIO`, `LV_USE_FS_POSIX`, `LV_USE_FS_WIN32`, `LV_USE_FS_FATFS`, `LV_USE_FS_LITTLEFS` |
| Image Decoders             | `LV_USE_PNG`, `LV_USE_BMP`, `LV_USE_SJPG`, `LV_USE_GIF`, `LV_USE_FREETYPE`, `LV_USE_TINY_TTF` |
| Others                     | `LV_USE_SNAPSHOT`, `LV_USE_MONKEY`, `LV_USE_GRIDNAV`, `LV_USE_FRAGMENT`, `LV_USE_IMGFONT`, `LV_USE_MSG`, `LV_USE_IME_PINYIN` |
| Demos                      | `LV_USE_DEMO_WIDGETS`, `LV_USE_DEMO_BENCHMARK`, `LV_USE_DEMO_STRESS`, `LV_USE_DEMO_MUSIC` |

---

## Sources

- [LVGL 8.3 Documentation](https://docs.lvgl.io/8.3/)
- [LVGL 8.3 Changelog](https://docs.lvgl.io/8.3/CHANGELOG.html)
- [LVGL GitHub Releases](https://github.com/lvgl/lvgl/releases)
- [LVGL 8.3 Branch](https://github.com/lvgl/lvgl/tree/release/v8.3)
- [LVGL v8.3.0 Release Blog](https://lvgl.io/blog/release-v8-3-0)
- [LVGL ESP32 Drivers](https://github.com/lvgl/lvgl_esp32_drivers)
- [ESP-BSP LVGL Port](https://github.com/espressif/esp-bsp/blob/master/components/esp_lvgl_port/docs/performance.md)
- [LVGL ESP Component Registry](https://components.espressif.com/components/lvgl/lvgl/versions/8.3.0)
- [lv_conf_template.h](https://github.com/lvgl/lvgl/blob/release/v8.3/lv_conf_template.h)
