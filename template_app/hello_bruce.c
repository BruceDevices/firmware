#include "bruce_api.h"

// The entry point MUST be named app_main and take BruceAPI* as its single parameter.
void app_main(BruceAPI* api) {
    if (!api) return;

    // Verify API version compatibility
    if (api->api_version < BRUCE_API_VERSION) {
        api->bruce_log("[HelloBruce] Firmware API too old!\n");
        return;
    }

    int w = api->get_tft_width();
    int counter = 0;

    api->clear_screen(0x0000);
    api->draw_main_border_with_title("Hello Bruce");

    api->draw_string("Dynamically loaded BAP!", 20, 50, 0xFFFF);
    api->draw_string("NEXT/PREV = count", 20, 70, 0x07E0);
    api->draw_string("Press ESC to exit", 20, 90, 0xF800);

    api->bruce_log("Hello Bruce App started! Time: %u\n", api->bruce_get_time());

    // Allocate some memory to demonstrate tracking and quota
    void* test_mem = api->bruce_malloc(1024);
    if (test_mem) {
        api->bruce_log("Allocated 1KB at %p\n", test_mem);
        // Deliberately not freeing it to test the Garbage Collector on exit!
    }

    // Interactive loop — exits when user presses ESC
    while (1) {
        if (api->check_escape_press()) {
            break;
        }
        if (api->check_next_press()) {
            counter++;
        }
        if (api->check_prev_press()) {
            counter--;
        }

        // Redraw counter area
        api->fill_rect(20, 120, w - 40, 40, 0x0000);

        // Use bruce_log for the value (it goes to Serial)
        api->bruce_log("Counter: %d, Rand: %u\n", counter, api->bruce_random());

        // Simple format to screen using log wrappers or basic logic
        api->draw_string("Count:", 20, 120, 0xFFE0);

        api->bruce_delay(100);
    }

    api->bruce_log("Hello Bruce App exiting.\n");
    // App returns here — firmware handles cleanup automatically
}
