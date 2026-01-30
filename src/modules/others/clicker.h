// AutoClicker - HID Mouse Automation Tool
// Author: Senape3000
// Version: 1.1

#pragma once

// ===== PUBLIC API =====
void clicker_setup();   // Main entry point for USB clicker
void usbClickerSetup(); // Alias for USB mode
void bleClickerSetup(); // BLE mode (not yet implemented)

// ===== CONFIGURATION STRUCTURES =====

/**
 * @brief Stores all user-configurable click parameters
 *
 * This structure holds the clicking behavior settings.
 * Default values: 100ms delay, LEFT button, infinite clicks
 */
struct ClickerConfig {
    unsigned long delay_ms; // Delay between each click (milliseconds)
    int button_type;        // Mouse button: 0=LEFT, 1=RIGHT, 2=MIDDLE
    int max_clicks;         // Click limit: 0=infinite, >0=stop after N clicks

    // Constructor with sensible defaults
    ClickerConfig() : delay_ms(100), button_type(0), max_clicks(0) {}
};

/**
 * @brief Layout configuration for dynamic screen adaptation
 *
 * All dimensions are calculated at runtime based on screen resolution
 * to ensure the GUI works on different display sizes (e.g., 135x240, 320x240)
 */
struct LayoutConfig {
    int margin;          // Horizontal margin from screen edges
    int header_height;   // Top bar height
    int item_height;     // Height of each menu item
    int button_height;   // Height of the START button
    int text_size_large; // Font size for titles (1 or 2)
    int text_size_small; // Font size for details (always 1)
    int start_y;         // Y position where menu items begin
    int item_spacing;    // Vertical gap between menu items

    // Constructor calculates values based on tftWidth/tftHeight globals
    LayoutConfig();
};

/**
 * @brief Menu item identifiers
 *
 * Used to track which item is selected and being edited
 */
enum MenuItem {
    ITEM_DELAY = 0,  // Delay configuration
    ITEM_BUTTON = 1, // Button type selection
    ITEM_CLICKS = 2, // Click count limit
    ITEM_START = 3,  // Start execution button
    NUM_ITEMS = 4    // Total number of items (for bounds checking)
};
