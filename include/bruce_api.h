#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Bruce API Version — increment when struct layout changes.
// Apps compiled against a newer version will be rejected by older firmware.
#define BRUCE_API_VERSION 2

// BAP (Bruce App Package) API Structure
// This struct is populated by the firmware and passed to the app's entry point.
// All function pointers here provide access to firmware services.

typedef struct {
    // -------------------------------------------------------------------------
    // Version (MUST be the first field — never reorder)
    // -------------------------------------------------------------------------
    uint32_t api_version;

    // -------------------------------------------------------------------------
    // System & Memory Management
    // -------------------------------------------------------------------------
    // The app must use these memory functions instead of standard malloc/free
    // so the firmware can track allocations and prevent memory leaks.
    void* (*bruce_malloc)(size_t size);
    void (*bruce_free)(void* ptr);

    uint32_t (*bruce_millis)(void);
    void (*bruce_delay)(uint32_t ms);

    // Logging
    void (*bruce_log)(const char* format, ...);

    // -------------------------------------------------------------------------
    // Input
    // -------------------------------------------------------------------------
    bool (*check_next_press)(void);
    bool (*check_prev_press)(void);
    bool (*check_select_press)(void);
    bool (*check_escape_press)(void);
    bool (*check_any_key_press)(void);

    // -------------------------------------------------------------------------
    // Display & UI
    // -------------------------------------------------------------------------
    void (*draw_string)(const char* text, int x, int y, uint16_t color);
    void (*fill_rect)(int x, int y, int w, int h, uint16_t color);
    void (*draw_rect)(int x, int y, int w, int h, uint16_t color);
    void (*clear_screen)(uint16_t color);
    void (*draw_main_border_with_title)(const char* title);

    // Draw a raw RGB565 pixel buffer to the display
    void (*draw_image)(int x, int y, const uint16_t* data, int w, int h);
    // Draw a single pixel
    void (*draw_pixel)(int x, int y, uint16_t color);

    int (*get_tft_width)(void);
    int (*get_tft_height)(void);

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------
    uint32_t (*bruce_random)(void);
    uint32_t (*bruce_get_time)(void);

} BruceAPI;

// Firmware-internal only — apps receive the API pointer via app_main() parameter.
BruceAPI* get_bruce_api(void);

#ifdef __cplusplus
}
#endif
