# API Reference

Complete reference for all functions available in the `BruceAPI` struct (API version 2).

The `BruceAPI` struct is defined in [`bruce_api.h`](../include/bruce_api.h). It is passed to your app's entry point as a pointer:

```c
void app_main(BruceAPI* api) {
    // Use api-> to call all functions
}
```

---

## Version

### `api_version`
```c
uint32_t api_version;
```
The API version of the firmware. Compare with `BRUCE_API_VERSION` to check compatibility.

**Example:**
```c
if (api->api_version < BRUCE_API_VERSION) {
    api->bruce_log("Firmware too old for this app!\n");
    return;
}
```

---

## System & Memory

### `bruce_malloc`
```c
void* bruce_malloc(size_t size);
```
Allocate memory from the heap. The firmware tracks this allocation and enforces a **64 KB** per-app quota.

**Parameters:**
- `size` — Number of bytes to allocate.

**Returns:** Pointer to allocated memory, or `NULL` if:
- The quota would be exceeded.
- The allocation table is full (max 64 active allocations).
- The system is out of memory.

> **Important:** Always use this instead of `malloc()`.

---

### `bruce_free`
```c
void bruce_free(void* ptr);
```
Free memory previously allocated by `bruce_malloc`. If `ptr` is `NULL`, this function does nothing.

> **Important:** Always use this instead of `free()`.

---

### `bruce_millis`
```c
uint32_t bruce_millis(void);
```
Returns the number of milliseconds since the device powered on. Wraps around after ~49 days.

---

### `bruce_delay`
```c
void bruce_delay(uint32_t ms);
```
Pause execution for `ms` milliseconds. This yields the CPU to FreeRTOS during the delay.

> **Tip:** Always include at least `bruce_delay(20)` in your main loop to prevent watchdog timeouts.

---

### `bruce_log`
```c
void bruce_log(const char* format, ...);
```
Print a formatted message to the Serial port (115200 baud). Supports `printf`-style format specifiers.

**Safety:** Output is capped at 256 characters via `vsnprintf` to prevent buffer overflows.

**Parameters:**
- `format` — printf-style format string.
- `...` — Variable arguments.

**Example:**
```c
api->bruce_log("x=%d, y=%d, name=%s\n", x, y, name);
```

---

## Input

All input functions return `true` if the corresponding button was pressed since the last check, `false` otherwise.

### `check_next_press`
```c
bool check_next_press(void);
```
Check if the **NEXT** button (down/right) was pressed.

---

### `check_prev_press`
```c
bool check_prev_press(void);
```
Check if the **PREV** button (up/left) was pressed.

---

### `check_select_press`
```c
bool check_select_press(void);
```
Check if the **SELECT** button (OK/Enter) was pressed.

---

### `check_escape_press`
```c
bool check_escape_press(void);
```
Check if the **ESC** button (back) was pressed. Use this to exit your app's main loop.

---

### `check_any_key_press`
```c
bool check_any_key_press(void);
```
Check if **any** button was pressed.

---

## Display & UI

### `draw_string`
```c
void draw_string(const char* text, int x, int y, uint16_t color);
```
Draw a text string at position (x, y) using the firmware's default font.

**Parameters:**
- `text` — Null-terminated string.
- `x`, `y` — Top-left corner position in pixels.
- `color` — Text color in RGB565 format.

---

### `fill_rect`
```c
void fill_rect(int x, int y, int w, int h, uint16_t color);
```
Draw a **filled** rectangle.

---

### `draw_rect`
```c
void draw_rect(int x, int y, int w, int h, uint16_t color);
```
Draw a rectangle **outline** (1px border).

---

### `clear_screen`
```c
void clear_screen(uint16_t color);
```
Fill the entire screen with a solid color. Use `0x0000` for black.

---

### `draw_main_border_with_title`
```c
void draw_main_border_with_title(const char* title);
```
Draw the firmware's standard UI border with a title bar. This gives your app the same look-and-feel as built-in modules.

---

### `draw_image`
```c
void draw_image(int x, int y, const uint16_t* data, int w, int h);
```
Draw a raw **RGB565 pixel buffer** to the display. This is the primary way to render compiled image assets.

**Parameters:**
- `x`, `y` — Top-left corner position.
- `data` — Pointer to the pixel data array (from `bruce_assets.h`).
- `w`, `h` — Image dimensions in pixels.

**Example:**
```c
#include "bruce_assets.h"
api->draw_image(0, 0, logo_data, logo_width, logo_height);
```

> **Added in API v2**

---

### `draw_pixel`
```c
void draw_pixel(int x, int y, uint16_t color);
```
Draw a single pixel at position (x, y).

> **Added in API v2**

---

### `get_tft_width`
```c
int get_tft_width(void);
```
Returns the screen width in pixels (e.g., 240 for Cardputer).

---

### `get_tft_height`
```c
int get_tft_height(void);
```
Returns the screen height in pixels (e.g., 135 for Cardputer).

---

## Utilities

### `bruce_random`
```c
uint32_t bruce_random(void);
```
Returns a **hardware-generated** random 32-bit integer using ESP32's true random number generator.

---

### `bruce_get_time`
```c
uint32_t bruce_get_time(void);
```
Returns the current **Unix timestamp** (seconds since 1970-01-01). Requires the device to have synced its clock (e.g., via NTP).

---

## RGB565 Color Reference

The display uses **RGB565** (16-bit) color format. Common colors:

| Color | Hex Value | Preview |
|-------|-----------|---------|
| Black | `0x0000` | ⬛ |
| White | `0xFFFF` | ⬜ |
| Red | `0xF800` | 🟥 |
| Green | `0x07E0` | 🟩 |
| Blue | `0x001F` | 🟦 |
| Yellow | `0xFFE0` | 🟨 |
| Cyan | `0x07FF` | 🔵 |
| Magenta | `0xF81F` | 🟪 |
| Orange | `0xFD20` | 🟧 |

### Converting from RGB888

```c
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
```

---

## Struct Layout

For reference, here is the complete `BruceAPI` struct layout:

```c
typedef struct {
    uint32_t api_version;                                          // offset 0

    void*    (*bruce_malloc)(size_t size);                          // offset 4
    void     (*bruce_free)(void* ptr);                              // offset 8
    uint32_t (*bruce_millis)(void);                                 // offset 12
    void     (*bruce_delay)(uint32_t ms);                           // offset 16
    void     (*bruce_log)(const char* format, ...);                 // offset 20

    bool     (*check_next_press)(void);                             // offset 24
    bool     (*check_prev_press)(void);                             // offset 28
    bool     (*check_select_press)(void);                           // offset 32
    bool     (*check_escape_press)(void);                           // offset 36
    bool     (*check_any_key_press)(void);                          // offset 40

    void     (*draw_string)(const char*, int, int, uint16_t);       // offset 44
    void     (*fill_rect)(int, int, int, int, uint16_t);            // offset 48
    void     (*draw_rect)(int, int, int, int, uint16_t);            // offset 52
    void     (*clear_screen)(uint16_t);                             // offset 56
    void     (*draw_main_border_with_title)(const char*);           // offset 60
    void     (*draw_image)(int, int, const uint16_t*, int, int);    // offset 64 (v2)
    void     (*draw_pixel)(int, int, uint16_t);                     // offset 68 (v2)

    int      (*get_tft_width)(void);                                // offset 72
    int      (*get_tft_height)(void);                               // offset 76

    uint32_t (*bruce_random)(void);                                 // offset 80
    uint32_t (*bruce_get_time)(void);                               // offset 84
} BruceAPI;
```

> **Important:** New fields are always appended at the end. Existing fields are never removed or reordered.
