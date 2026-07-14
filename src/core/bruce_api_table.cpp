#include "bruce_api.h"
#include <Arduino.h>
#include <globals.h>
#include "core/display.h"
#include <stdarg.h>

// -------------------------------------------------------------------------
// Memory tracking for garbage collection
// -------------------------------------------------------------------------

#define BAP_MAX_ALLOCS 64
#define BAP_MAX_MEMORY_BYTES (64 * 1024) // 64KB hard limit for an app

static void* bap_alloc_table[BAP_MAX_ALLOCS] = {0};
static size_t bap_alloc_sizes[BAP_MAX_ALLOCS] = {0};
static uint32_t bap_allocated_memory = 0;

static int bap_track_alloc(void* ptr, size_t size) {
    for (int i = 0; i < BAP_MAX_ALLOCS; i++) {
        if (bap_alloc_table[i] == NULL) {
            bap_alloc_table[i] = ptr;
            bap_alloc_sizes[i] = size;
            bap_allocated_memory += size;
            return i;
        }
    }
    return -1; // table full
}

static void bap_untrack_alloc(void* ptr) {
    for (int i = 0; i < BAP_MAX_ALLOCS; i++) {
        if (bap_alloc_table[i] == ptr) {
            bap_alloc_table[i] = NULL;
            bap_allocated_memory -= bap_alloc_sizes[i];
            bap_alloc_sizes[i] = 0;
            return;
        }
    }
}

// Called by bap_loader after app exits to free any leaked memory
extern "C" void bap_free_tracked_allocs(void) {
    for (int i = 0; i < BAP_MAX_ALLOCS; i++) {
        if (bap_alloc_table[i] != NULL) {
            Serial.printf("[BAP GC] Leaked alloc at slot %d (%d bytes), freeing %p\n", i, bap_alloc_sizes[i], bap_alloc_table[i]);
            free(bap_alloc_table[i]);
            bap_alloc_table[i] = NULL;
            bap_alloc_sizes[i] = 0;
        }
    }
    bap_allocated_memory = 0;
}

// -------------------------------------------------------------------------
// Wrappers for system & memory management
// -------------------------------------------------------------------------

static void* bruce_malloc_wrapper(size_t size) {
    if (bap_allocated_memory + size > BAP_MAX_MEMORY_BYTES) {
        Serial.printf("[BAP] ERROR: App requested %d bytes, exceeds quota of %d\n", size, BAP_MAX_MEMORY_BYTES);
        return NULL;
    }
    void* ptr = malloc(size);
    if (ptr) {
        if (bap_track_alloc(ptr, size) == -1) {
            free(ptr); // Reject if table is full
            return NULL;
        }
    }
    return ptr;
}

static void bruce_free_wrapper(void* ptr) {
    if (ptr) {
        bap_untrack_alloc(ptr);
        free(ptr);
    }
}

static uint32_t bruce_millis_wrapper(void) {
    return millis();
}

static void bruce_delay_wrapper(uint32_t ms) {
    delay(ms);
}

static void bruce_log_wrapper(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    Serial.print(buf);
}

// -------------------------------------------------------------------------
// Wrappers for Input
// -------------------------------------------------------------------------

static bool check_next_press_wrapper(void) {
    return check(NextPress);
}

static bool check_prev_press_wrapper(void) {
    return check(PrevPress);
}

static bool check_select_press_wrapper(void) {
    return check(SelPress);
}

static bool check_escape_press_wrapper(void) {
    return check(EscPress);
}

static bool check_any_key_press_wrapper(void) {
    return check(AnyKeyPress, false);
}

// -------------------------------------------------------------------------
// Wrappers for Display & UI
// -------------------------------------------------------------------------

static void draw_string_wrapper(const char* text, int x, int y, uint16_t color) {
    uint16_t old_color = tft.getTextColor();
    tft.setTextColor(color);
    tft.drawString(text, x, y);
    tft.setTextColor(old_color);
}

static void fill_rect_wrapper(int x, int y, int w, int h, uint16_t color) {
    tft.fillRect(x, y, w, h, color);
}

static void draw_rect_wrapper(int x, int y, int w, int h, uint16_t color) {
    tft.drawRect(x, y, w, h, color);
}

static void clear_screen_wrapper(uint16_t color) {
    tft.fillScreen(color);
}

static void draw_main_border_with_title_wrapper(const char* title) {
    drawMainBorderWithTitle(String(title), true);
}

static void draw_image_wrapper(int x, int y, const uint16_t* data, int w, int h) {
    if (!data || w <= 0 || h <= 0) return;
    tft.pushImage(x, y, w, h, data);
}

static void draw_pixel_wrapper(int x, int y, uint16_t color) {
    tft.drawPixel(x, y, color);
}

static int get_tft_width_wrapper(void) {
    return tftWidth;
}

static int get_tft_height_wrapper(void) {
    return tftHeight;
}

// -------------------------------------------------------------------------
// Wrappers for Utilities
// -------------------------------------------------------------------------

static uint32_t bruce_random_wrapper(void) {
    return esp_random();
}

static uint32_t bruce_get_time_wrapper(void) {
    return (uint32_t)time(NULL);
}

// -------------------------------------------------------------------------
// API Table Instantiation
// -------------------------------------------------------------------------

static BruceAPI bruce_api_instance = {
    .api_version = BRUCE_API_VERSION,

    .bruce_malloc = bruce_malloc_wrapper,
    .bruce_free = bruce_free_wrapper,
    .bruce_millis = bruce_millis_wrapper,
    .bruce_delay = bruce_delay_wrapper,
    .bruce_log = bruce_log_wrapper,

    .check_next_press = check_next_press_wrapper,
    .check_prev_press = check_prev_press_wrapper,
    .check_select_press = check_select_press_wrapper,
    .check_escape_press = check_escape_press_wrapper,
    .check_any_key_press = check_any_key_press_wrapper,

    .draw_string = draw_string_wrapper,
    .fill_rect = fill_rect_wrapper,
    .draw_rect = draw_rect_wrapper,
    .clear_screen = clear_screen_wrapper,
    .draw_main_border_with_title = draw_main_border_with_title_wrapper,

    .draw_image = draw_image_wrapper,
    .draw_pixel = draw_pixel_wrapper,

    .get_tft_width = get_tft_width_wrapper,
    .get_tft_height = get_tft_height_wrapper,

    .bruce_random = bruce_random_wrapper,
    .bruce_get_time = bruce_get_time_wrapper
};

// Return singleton pointer
extern "C" BruceAPI* get_bruce_api(void) {
    return &bruce_api_instance;
}
