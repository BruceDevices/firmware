// AutoClicker - v1.1 (Updated by Senape3000)
// Optimized for stability, readability, and dynamic GUI scaling

#include "clicker.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "globals.h"
#include <USB.h>

#ifdef USB_as_HID
#include <USBHIDMouse.h>

// ===== STATIC GLOBALS =====
// These variables persist across function calls and are safe from stack corruption
// during USB initialization (which can corrupt local stack variables)

static LayoutConfig layout;  // Screen layout configuration
static ClickerConfig config; // User configuration (delay, button, clicks)
static USBHIDMouse *Mouse = nullptr; // Lazy init to avoid global HID descriptor registration

// Runtime tracking for CPS calculation
static unsigned long prevMillisec = 0; // Last second timestamp
static unsigned long currMillisec = 0; // Current timestamp
static int cpsClickCount = 0;          // Clicks performed in current second

// ===== CONSTANTS =====

// Mouse button display names
static const char *BUTTON_NAMES[] = {"LEFT", "RIGHT", "MID"};

// Preset click count values for quick selection (0 = infinite)
static const int PRESET_CLICKS[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,   10,  15,  20,  25,
                                    30, 35, 40, 45, 50, 60, 70, 80, 90, 100, 200, 300, 400, 500};
static const int NUM_PRESETS = sizeof(PRESET_CLICKS) / sizeof(PRESET_CLICKS[0]);

// ===== LAYOUT CONSTRUCTOR =====

/**
 * @brief Initializes layout dimensions based on screen resolution
 *
 * Dynamically adapts margins, fonts, and spacing to ensure
 * the UI works correctly on both small (135x240) and larger screens
 */
LayoutConfig::LayoutConfig() {
    margin = (tftWidth > 200) ? 10 : 5;
    header_height = (tftHeight > 200) ? 35 : 25;
    item_height = (tftHeight > 200) ? 30 : 22;
    button_height = (tftHeight > 200) ? 40 : 30;
    text_size_large = (tftWidth > 200) ? 2 : 1;
    text_size_small = 1;
    start_y = header_height + ((tftHeight > 200) ? 15 : 8);
    item_spacing = (tftHeight > 200) ? 14 : 10;
}

// ===== USB LIFECYCLE MANAGEMENT =====

/**
 * @brief Initializes USB HID Mouse functionality
 *
 * Must be called before any mouse operations.
 * WARNING: This can corrupt stack variables, use static/global storage only.
 */
void initClickerUSB() {
    USB.begin();
    if (Mouse == nullptr) Mouse = new USBHIDMouse();
    if (Mouse != nullptr) Mouse->begin();
    // Serial.println("[USB] Clicker initialized");
}

/**
 * @brief Safely tears down USB HID Mouse
 *
 * Properly releases USB resources and re-enables DFU/Serial for flashing.
 * Must be called before exiting or restarting the app.
 */
void cleanupClickerUSB() {
    // Serial.println("[USB] Starting cleanup...");
    if (Mouse != nullptr) {
        Mouse->end();
        // Keep instance alive to avoid deleting a polymorphic type with
        // non-virtual dtor and to preserve lazy HID registration behavior.
    }
    USB.~ESPUSB(); // Explicit destructor call
    delay(100);
    USB.enableDFU(); // Re-enable DFU for future uploads
    delay(100);
    // Serial.println("[USB] Cleanup complete");
}
// ===== UI DRAWING FUNCTIONS =====

/**
 * @brief Draws a single menu item with proper styling and values
 *
 * @param layout Screen layout configuration
 * @param itemIndex Which menu item to draw
 * @param isSelected True if this item is currently selected
 * @param isEdit True if this item is in edit mode
 * @param config Current configuration values
 * @param custom_mode True if user is entering custom click count
 */
void drawMenuItem(
    const LayoutConfig &layout, MenuItem itemIndex, bool isSelected, bool isEdit, const ClickerConfig &config,
    bool custom_mode
) {
    int yPos = 0;

    // Calculate Y position dynamically
    if (itemIndex < ITEM_START) {
        // Regular menu items stacked vertically
        yPos = layout.start_y + itemIndex * (layout.item_height + layout.item_spacing);
    } else {
        // START button positioned at bottom with spacing
        yPos =
            layout.start_y + 3 * (layout.item_height + layout.item_spacing) + ((tftHeight > 200) ? 30 : 15);

        // Ensure button doesn't overflow screen
        int bottomMargin = (tftHeight > 200) ? 10 : 5;
        if (yPos + layout.button_height > tftHeight - bottomMargin) {
            yPos = tftHeight - layout.button_height - bottomMargin;
        }
    }

    // Clear item area to prevent visual artifacts
    int clearHeight = (itemIndex == ITEM_START) ? layout.button_height : layout.item_height;
    tft.fillRect(
        layout.margin - 5, yPos - 5, tftWidth - 2 * layout.margin + 10, clearHeight + 10, bruceConfig.bgColor
    );

    tft.setTextSize(layout.text_size_large);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    // Draw selection border (green when editing, white when selected)
    if (isSelected && itemIndex != ITEM_START) {
        uint16_t borderColor = isEdit ? TFT_GREEN : bruceConfig.priColor;
        int borderWidth = isEdit ? 2 : 1;

        for (int i = 0; i < borderWidth; i++) {
            tft.drawRoundRect(
                layout.margin - 3 + i,
                yPos - 3 + i,
                tftWidth - 2 * layout.margin + 6 - 2 * i,
                layout.item_height + 6 - 2 * i,
                5,
                borderColor
            );
        }
    }

    // Calculate vertical centering for text
    int contentY = yPos + (layout.item_height - layout.text_size_large * 8) / 2;
    int rightMargin = 15;
    int unitX = tftWidth - layout.margin - rightMargin;

    // Draw item-specific content
    switch (itemIndex) {
        case ITEM_DELAY: {
            // Label on left
            tft.setCursor(layout.margin + 2, contentY);
            tft.print("Delay:");

            // Value on right with unit
            char delayStr[16];
            snprintf(delayStr, sizeof(delayStr), "%lu", config.delay_ms);
            int numWidth = strlen(delayStr) * 6 * layout.text_size_large;

            tft.setCursor(unitX - numWidth - 12, contentY);
            if (isEdit && isSelected) tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            tft.print(delayStr);

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.print("ms");
            break;
        }

        case ITEM_BUTTON: {
            // Label on left
            tft.setCursor(layout.margin + 2, contentY);
            tft.print("Button:");

            // Button name on right
            const char *btnName = BUTTON_NAMES[config.button_type];
            int textWidth = strlen(btnName) * 6 * layout.text_size_large;

            tft.setCursor(unitX - textWidth, contentY);
            if (isEdit && isSelected) tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            tft.print(btnName);
            break;
        }

        case ITEM_CLICKS: {
            // Label on left
            tft.setCursor(layout.margin + 2, contentY);
            tft.print("Clicks:");

            // Display value based on mode
            if (custom_mode) {
                // Show "Custom" when user is entering manual value
                const char *customText = "Custom";
                int textWidth = strlen(customText) * 6 * layout.text_size_large;
                tft.setCursor(unitX - textWidth, contentY);
                if (isEdit && isSelected) tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                tft.print(customText);
            } else if (config.max_clicks == 0) {
                // Show "Infinite" or "INF" for unlimited clicks
                const char *infText = (tftWidth > 200) ? "Infinite" : "INF";
                int textWidth = strlen(infText) * 6 * layout.text_size_large;
                tft.setCursor(unitX - textWidth, contentY);
                if (isEdit && isSelected) tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                tft.print(infText);
            } else {
                // Show numeric value
                char clicksStr[16];
                snprintf(clicksStr, sizeof(clicksStr), "%d", config.max_clicks);
                int numWidth = strlen(clicksStr) * 6 * layout.text_size_large;
                tft.setCursor(unitX - numWidth, contentY);
                if (isEdit && isSelected) tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                tft.print(clicksStr);
            }
            break;
        }

        case ITEM_START: {
            // Draw styled button
            uint16_t btnColor = isSelected ? TFT_DARKGREEN : TFT_DARKGREY;
            tft.fillRoundRect(
                layout.margin, yPos, tftWidth - 2 * layout.margin, layout.button_height, 8, btnColor
            );

            // Centered text
            tft.setTextColor(TFT_WHITE, btnColor);
            const char *btnText = (tftWidth > 200) ? "START CLICK" : "START";
            int textWidth = strlen(btnText) * 6 * layout.text_size_large;
            tft.setCursor(
                (tftWidth - textWidth) / 2, yPos + (layout.button_height - layout.text_size_large * 8) / 2
            );
            tft.print(btnText);
            break;
        }

        default: break;
    }
}

/**
 * @brief Draws the main configuration screen with header and footer
 *
 * @param layout Screen layout configuration
 */
void drawConfigScreen(const LayoutConfig &layout) {
    tft.fillScreen(bruceConfig.bgColor);

    // Header bar with app title
    tft.fillRect(0, 0, tftWidth, layout.header_height, bruceConfig.priColor);
    tft.setTextColor(bruceConfig.bgColor, bruceConfig.priColor);
    tft.setTextSize(layout.text_size_large);
    tft.setCursor(layout.margin, (layout.header_height - (layout.text_size_large * 8)) / 2);
    tft.println("AUTO CLICKER v1.1");

    // Footer with control hints (only on larger screens)
    if (tftHeight > 200) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setTextSize(layout.text_size_small);
        tft.setCursor(layout.margin, tftHeight - 15);
        tft.print("NAV: ^v | EDIT: Sel | START: Sel");
    }
}

/**
 * @brief Draws the active clicking screen with status info
 *
 * @param layout Screen layout configuration
 * @param buttonName Name of the button being clicked (LEFT/RIGHT/MID)
 */
void drawClickingScreen(const LayoutConfig &layout, const char *buttonName) {
    tft.fillScreen(bruceConfig.bgColor);

    // Animated header to show active state
    const int headerHeight = (tftHeight > 200) ? 40 : 30;
    tft.fillRect(0, 0, tftWidth, headerHeight, TFT_DARKGREEN);
    tft.setTextSize((tftWidth > 200) ? 2 : 1);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);

    const char *headerText = (tftWidth > 200) ? "-> CLICKING <-" : "-> CLICK <-";
    int headerWidth = strlen(headerText) * 6 * ((tftWidth > 200) ? 2 : 1);
    tft.setCursor((tftWidth - headerWidth) / 2, (headerHeight - 16) / 2);
    tft.print(headerText);

    // Static configuration info
    const int infoY = headerHeight + 10;
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(layout.text_size_small);

    // Line 1: Delay and button type
    tft.setCursor(layout.margin, infoY);
    tft.print("Delay: ");
    tft.print(config.delay_ms);
    tft.print("ms | Btn: ");
    tft.print(buttonName);

    // Line 2: Target (if finite)
    if (config.max_clicks > 0) {
        tft.setCursor(layout.margin, infoY + 12);
        tft.print("Target: ");
        tft.print(config.max_clicks);
        tft.print(" clicks");
    }

    // Line 3: Stop instructions
    tft.setCursor(layout.margin, infoY + ((config.max_clicks > 0) ? 24 : 12));
    tft.print("Press SEL to stop");
}

/**
 * @brief Updates the real-time CPS display during clicking
 *
 * @param layout Screen layout configuration
 * @param currentCPS Clicks per second in the last interval
 * @param totalClicks Total clicks performed so far
 */
void updateCPSDisplay(const LayoutConfig &layout, int currentCPS, unsigned long totalClicks) {
    const int cpsY = tftHeight / 2 + 10;
    const int cpsSize = (tftWidth > 200) ? 3 : 2;

    // Clear previous CPS area to prevent overlap
    tft.fillRect(0, cpsY - 5, tftWidth, cpsSize * 8 + 30, bruceConfig.bgColor);

    // Large CPS value centered
    char cpsStr[16];
    snprintf(cpsStr, sizeof(cpsStr), "%d", currentCPS);
    tft.setTextSize(cpsSize);
    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
    int cpsWidth = strlen(cpsStr) * 6 * cpsSize;
    tft.setCursor((tftWidth - cpsWidth) / 2, cpsY);
    tft.print(cpsStr);

    // Label below CPS value
    tft.setTextSize(layout.text_size_small);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    const char *label = "Clicks/Second";
    int labelWidth = strlen(label) * 6;
    tft.setCursor((tftWidth - labelWidth) / 2, cpsY + cpsSize * 8 + 5);
    tft.print(label);

    // Progress indicator
    if (config.max_clicks > 0) {
        // Show progress towards target
        char progressStr[32];
        snprintf(progressStr, sizeof(progressStr), "%lu / %d", totalClicks, config.max_clicks);
        int progressWidth = strlen(progressStr) * 6;
        tft.setCursor((tftWidth - progressWidth) / 2, cpsY + cpsSize * 8 + 18);
        tft.print(progressStr);
    } else {
        // Show total count for infinite mode
        char totalStr[32];
        snprintf(totalStr, sizeof(totalStr), "Tot: %lu", totalClicks);
        int totalWidth = strlen(totalStr) * 6;
        tft.setCursor((tftWidth - totalWidth) / 2, cpsY + cpsSize * 8 + 18);
        tft.print(totalStr);
    }
}

/**
 * @brief Draws the summary screen after clicking completes
 *
 * @param layout Screen layout configuration
 * @param totalClicks Total clicks performed
 * @param buttonName Button that was clicked
 * @param completed True if target was reached, false if user stopped
 * @param delay_ms Delay value used (from snapshot)
 * @param max_clicks Max clicks value (from snapshot)
 */
void drawSummaryScreen(
    const LayoutConfig &layout, unsigned long totalClicks, const char *buttonName, bool completed,
    unsigned long delay_ms, int max_clicks
) {
    tft.fillScreen(bruceConfig.bgColor);

    // Header shows completion status
    uint16_t headerColor = completed ? TFT_DARKGREEN : TFT_DARKGREY;
    tft.fillRect(0, 0, tftWidth, layout.header_height, headerColor);
    tft.setTextColor(TFT_WHITE, headerColor);
    tft.setTextSize(layout.text_size_large);

    const char *statusText = completed ? "COMPLETED" : "STOPPED";
    int statusWidth = strlen(statusText) * 6 * layout.text_size_large;
    tft.setCursor((tftWidth - statusWidth) / 2, (layout.header_height - 16) / 2);
    tft.print(statusText);

    // Display statistics
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(layout.text_size_small);
    int infoY = layout.header_height + 20;

    // Total clicks with target (if applicable)
    tft.setCursor(layout.margin, infoY);
    tft.print("Clicks: ");
    tft.print(totalClicks);
    if (max_clicks > 0) {
        tft.print(" / ");
        tft.print(max_clicks);
    }

    // Button used
    tft.setCursor(layout.margin, infoY + 15);
    tft.print("Button: ");
    tft.print(buttonName);

    // Delay configuration
    tft.setCursor(layout.margin, infoY + 30);
    tft.print("Delay: ");
    tft.print(delay_ms);
    tft.print("ms");
}

/**
 * @brief Shows USB initialization screen with countdown
 *
 * Displays a waiting message while USB driver loads.
 * Typical OS driver recognition time: 1-3 seconds
 *
 * @param layout Screen layout configuration
 */
void drawUSBInitScreen(const LayoutConfig &layout) {
    tft.fillScreen(bruceConfig.bgColor);

    // Header
    const int headerHeight = (tftHeight > 200) ? 40 : 30;
    tft.fillRect(0, 0, tftWidth, headerHeight, TFT_ORANGE);
    tft.setTextSize((tftWidth > 200) ? 2 : 1);
    tft.setTextColor(TFT_WHITE, TFT_ORANGE);

    const char *headerText = "USB INIT";
    int headerWidth = strlen(headerText) * 6 * ((tftWidth > 200) ? 2 : 1);
    tft.setCursor((tftWidth - headerWidth) / 2, (headerHeight - 16) / 2);
    tft.print(headerText);

    // Message
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(layout.text_size_small);
    int msgY = tftHeight / 2 - 20;

    const char *msg1 = "Initializing USB HID...";
    int msg1Width = strlen(msg1) * 6;
    tft.setCursor((tftWidth - msg1Width) / 2, msgY);
    tft.print(msg1);

    const char *msg2 = "Please wait";
    int msg2Width = strlen(msg2) * 6;
    tft.setCursor((tftWidth - msg2Width) / 2, msgY + 15);
    tft.print(msg2);

    // Countdown animation (3 seconds)
    tft.setTextSize((tftWidth > 200) ? 3 : 2);
    tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);

    for (int i = 3; i > 0; i--) {
        // Clear previous number
        int numY = tftHeight / 2 + 20;
        tft.fillRect(0, numY - 5, tftWidth, 30, bruceConfig.bgColor);

        // Draw countdown number
        char numStr[2];
        snprintf(numStr, sizeof(numStr), "%d", i);
        int numSize = (tftWidth > 200) ? 3 : 2;
        int numWidth = strlen(numStr) * 6 * numSize;
        tft.setCursor((tftWidth - numWidth) / 2, numY);
        tft.print(numStr);

        delay(1000);
    }

    // Serial.println("[USB] Initialization delay completed");
}

// ===== INPUT HANDLING FUNCTIONS =====

/**
 * @brief Handles navigation through menu items
 *
 * @param selected_item Current selected item index (modified by reference)
 * @param prev_selected Previous selected item (for redraw optimization)
 * @param editing Current edit mode state (modified by reference)
 * @param last_input Timestamp of last input (for timeout)
 * @return true if user wants to start clicking or exit, false otherwise
 */
bool handleMenuNavigation(int &selected_item, int &prev_selected, bool &editing, unsigned long &last_input) {
    // Navigate up
    if (check(PrevPress)) {
        prev_selected = selected_item;
        selected_item = (selected_item - 1 + NUM_ITEMS) % NUM_ITEMS;
        last_input = millis();
        return false;
    }

    // Navigate down
    if (check(NextPress)) {
        prev_selected = selected_item;
        selected_item = (selected_item + 1) % NUM_ITEMS;
        last_input = millis();
        return false;
    }

    // Select action
    if (check(SelPress)) {
        if (selected_item == ITEM_START) {
            return true; // Signal to start clicking
        }
        editing = true; // Enter edit mode for current item
        last_input = millis();
        return false;
    }

    // Exit application
    if (check(EscPress)) { return true; }

    return false;
}

/**
 * @brief Handles value editing for the selected menu item
 *
 * @param selected_item Which item is being edited
 * @param custom_mode Custom keyboard input mode for clicks (modified by reference)
 * @param clicks_preset_index Current index in preset array (modified by reference)
 * @param editing Edit mode state (modified by reference)
 * @param last_input Timestamp of last input
 * @return true if value was changed (triggers redraw)
 */
bool handleValueEditing(
    MenuItem selected_item, bool &custom_mode, int &clicks_preset_index, bool &editing,
    unsigned long &last_input
) {
    bool value_changed = false;

    switch (selected_item) {
        case ITEM_DELAY:
            // Adjust delay in 10ms increments
            if (check(PrevPress)) {
                if (config.delay_ms > 10) { config.delay_ms -= 10; }
                value_changed = true;
            }
            if (check(NextPress)) {
                if (config.delay_ms < 60000) { // Cap at 60 seconds
                    config.delay_ms += 10;
                }
                value_changed = true;
            }
            break;

        case ITEM_BUTTON:
            // Cycle through button types: LEFT -> RIGHT -> MIDDLE -> LEFT
            if (check(PrevPress) || check(NextPress)) {
                config.button_type = (config.button_type + 1) % 3;
                value_changed = true;
            }
            break;

        case ITEM_CLICKS:
            // Navigate through preset values or enter custom mode
            if (check(PrevPress)) {
                if (custom_mode) {
                    // Exit custom mode, go to last preset
                    custom_mode = false;
                    clicks_preset_index = NUM_PRESETS - 1;
                    config.max_clicks = PRESET_CLICKS[clicks_preset_index];
                } else {
                    // Move to previous preset
                    clicks_preset_index = (clicks_preset_index - 1 + NUM_PRESETS) % NUM_PRESETS;
                    config.max_clicks = PRESET_CLICKS[clicks_preset_index];
                }
                value_changed = true;
            }

            if (check(NextPress)) {
                if (clicks_preset_index == NUM_PRESETS - 1 && !custom_mode) {
                    // Reached end of presets, enter custom mode
                    custom_mode = true;
                } else if (custom_mode) {
                    // Exit custom mode, wrap to first preset
                    custom_mode = false;
                    clicks_preset_index = 0;
                    config.max_clicks = PRESET_CLICKS[clicks_preset_index];
                } else {
                    // Move to next preset
                    clicks_preset_index = (clicks_preset_index + 1) % NUM_PRESETS;
                    config.max_clicks = PRESET_CLICKS[clicks_preset_index];
                }
                value_changed = true;
            }

            // Open keyboard for custom input
            if (custom_mode && check(SelPress)) {
                String customValue = num_keyboard(String(config.max_clicks).c_str(), 6, "Custom Click Count");
                int val = atoi(customValue.c_str());
                config.max_clicks = (val < 0) ? 0 : val;
                custom_mode = false;
                editing = false;
                value_changed = true;
                return true; // Force full redraw after keyboard
            }
            break;

        default: break;
    }

    // Exit edit mode when SEL or ESC pressed (unless in custom keyboard)
    if (check(SelPress) || check(EscPress)) {
        if (selected_item != ITEM_CLICKS || !custom_mode) {
            editing = false;
            last_input = millis();
        }
    }

    return value_changed;
}
// ===== CLICKING ENGINE =====

/**
 * @brief Performs the actual clicking loop with real-time feedback
 *
 * This function:
 * - Executes mouse clicks at the configured interval
 * - Updates CPS display every second
 * - Checks for user interrupt (SEL/ESC)
 * - Respects max_clicks limit if set
 * - Uses responsive delay to allow quick exit
 *
 * @param btnNameStr Button name for logging
 * @return Total number of clicks performed
 */
unsigned long performClicking(const char *btnNameStr) {
    // Reset CPS tracking
    cpsClickCount = 0;
    prevMillisec = millis();
    unsigned long totalClicks = 0;
    bool shouldStop = false;

    // Map button type to USB HID constant
    uint8_t mouseButton = MOUSE_LEFT;
    if (config.button_type == 1) {
        mouseButton = MOUSE_RIGHT;
    } else if (config.button_type == 2) {
        mouseButton = MOUSE_MIDDLE;
    }

    // Main clicking loop
    while (!shouldStop) {
        // Step 1: Perform click
        if (Mouse != nullptr) Mouse->click(mouseButton);
        cpsClickCount++;
        totalClicks++;

        // Step 2: Check if target reached (immediate check)
        if (config.max_clicks > 0 && totalClicks >= (unsigned long)config.max_clicks) {
            shouldStop = true;
            break;
        }

        // Step 3: Update statistics every second
        currMillisec = millis();
        if (currMillisec - prevMillisec >= 1000) {
            updateCPSDisplay(layout, cpsClickCount, totalClicks);
            // Serial.printf("[CLICKER] CPS: %d | Total: %lu\n", cpsClickCount, totalClicks);

            // Reset counter for next second
            cpsClickCount = 0;
            prevMillisec = currMillisec;
        }

        // Step 4: Responsive delay with user interrupt check
        // Instead of blocking delay(), we check input every 1ms
        // This allows quick response to stop button
        unsigned long delayStart = millis();
        while (millis() - delayStart < config.delay_ms) {
            InputHandler();
            wakeUpScreen(); // Keep display active

            // Check for stop signal
            if (check(EscPress) || check(SelPress) || returnToMenu) {
                shouldStop = true;
                break;
            }

            delay(1); // Small sleep to prevent watchdog timeout
        }
    }

    return totalClicks;
}
// ===== MAIN APPLICATION ENTRY POINT =====

/**
 * @brief Main clicker application loop
 *
 * Flow:
 * 1. Configuration menu (navigate and edit settings)
 * 2. USB initialization
 * 3. Clicking execution
 * 4. Summary screen with restart option
 * 5. USB cleanup
 *
 * Note: config, layout, and Mouse are static globals to prevent
 * stack corruption during USB init/cleanup operations
 */
void clicker_setup() {
    // UI state variables
    int selected_item = 0;
    int prev_selected = 0;
    bool editing = false;
    bool custom_mode = false;
    int clicks_preset_index = 0;
    unsigned long last_input = millis();

    // Draw initial configuration screen
    drawConfigScreen(layout);
    for (int i = 0; i < NUM_ITEMS; i++) {
        drawMenuItem(layout, (MenuItem)i, (i == selected_item), false, config, custom_mode);
    }

    // ===== CONFIGURATION PHASE =====
    bool exit_menu = false;
    while (!exit_menu) {
        InputHandler();
        wakeUpScreen();

        // Track what needs redrawing
        bool selection_changed = false;
        bool value_changed = false;
        bool edit_mode_changed = false;
        bool need_full_redraw = false;

        if (!editing) {
            // Handle navigation mode
            int old_selected = selected_item;
            bool old_editing = editing;
            bool shouldExit = handleMenuNavigation(selected_item, prev_selected, editing, last_input);

            if (shouldExit) {
                if (selected_item == ITEM_START) {
                    exit_menu = true; // Proceed to clicking
                } else {
                    return; // Exit to main menu
                }
            }

            selection_changed = (old_selected != selected_item);
            edit_mode_changed = (old_editing != editing);
        } else {
            // Handle editing mode
            bool old_editing = editing;
            value_changed = handleValueEditing(
                (MenuItem)selected_item, custom_mode, clicks_preset_index, editing, last_input
            );

            // Force full redraw after custom keyboard
            if (value_changed && !editing && selected_item == ITEM_CLICKS) { need_full_redraw = true; }

            edit_mode_changed = (old_editing != editing);
        }

        // Optimized redraw logic (only update what changed)
        if (need_full_redraw) {
            drawConfigScreen(layout);
            for (int i = 0; i < NUM_ITEMS; i++) {
                drawMenuItem(layout, (MenuItem)i, (i == selected_item), false, config, custom_mode);
            }
        } else {
            if (selection_changed) {
                // Redraw old and new selected items
                drawMenuItem(layout, (MenuItem)prev_selected, false, false, config, custom_mode);
                drawMenuItem(layout, (MenuItem)selected_item, true, false, config, custom_mode);
            }
            if (edit_mode_changed || value_changed) {
                // Redraw current item with updated state
                drawMenuItem(layout, (MenuItem)selected_item, true, editing, config, custom_mode);
                if (value_changed) { last_input = millis(); }
            }
        }

        delay(20); // Prevent tight loop

        // Auto-exit after 2 minutes of inactivity
        if (millis() - last_input > 120000) {
            // Serial.println("[CLICKER] Timeout, returning to menu");
            return;
        }
    }

    // ===== EXECUTION PHASE =====

    // CRITICAL: Snapshot config values BEFORE USB init
    // USB initialization can corrupt stack variables, so we save
    // all values to const locals for safe access later
    const unsigned long final_delay_ms = config.delay_ms;
    const int final_max_clicks = config.max_clicks;
    const int final_button_type = config.button_type;
    const char *btnNameStr = BUTTON_NAMES[final_button_type];

    // Serial.printf(
    //     "[CLICKER] Starting with: delay=%lums, max=%d, button=%s\n",
    //     final_delay_ms,
    //     final_max_clicks,
    //     btnNameStr
    //);

    // Initialize USB HID
    initClickerUSB();

    // Show USB initialization screen with countdown
    drawUSBInitScreen(layout);

    // Draw clicking screen
    drawClickingScreen(layout, btnNameStr);

    // Execute clicking loop (config is static global, safe to use)
    unsigned long totalClicks = performClicking(btnNameStr);

    // Serial.printf("[CLICKER] Finished: %lu clicks performed\n", totalClicks);

    // ===== SUMMARY PHASE =====

    // Determine completion status
    bool completed = (final_max_clicks > 0 && totalClicks >= (unsigned long)final_max_clicks);

    // Draw summary with safe snapshot values (NOT config members)
    drawSummaryScreen(layout, totalClicks, btnNameStr, completed, final_delay_ms, final_max_clicks);

    // Wait briefly for user to read summary
    delay(2000);

    // Show restart/exit prompt
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(layout.text_size_small);
    int promptY = tftHeight - 20;
    tft.fillRect(0, promptY - 5, tftWidth, 25, bruceConfig.bgColor);
    tft.setCursor(layout.margin, promptY);
    tft.print("OK: Restart | ESC: Exit");

    // Wait for user decision
    bool userChoice = false;
    bool restart = false;
    while (!userChoice) {
        InputHandler();
        wakeUpScreen();

        if (check(SelPress)) {
            restart = true;
            userChoice = true;
        }
        if (check(EscPress)) {
            delay(600);
            displayWarning(
                "Turn-off to restore USB", true
            ); // BUG (?) - Need to restore Usb without Power-Off (reboot seems uneffective)
            restart = false;
            userChoice = true;
        }

        delay(50);
    }

    // ===== CLEANUP PHASE =====

    // Always cleanup USB before exiting or restarting
    cleanupClickerUSB();

    // Handle user choice
    if (restart) {
        // Serial.println("[CLICKER] Restarting application");
        clicker_setup(); // Recursive call to restart
        return;
    } else {
        // Serial.println("[CLICKER] Exiting to main menu");
        return;
    }
}

/**
 * @brief Alias for USB clicker mode
 */
void usbClickerSetup() { clicker_setup(); }

/**
 * @brief Placeholder for BLE clicker mode
 *
 * TODO: Implement Bluetooth HID functionality
 * Will require BLE HID library and pairing logic
 */
void bleClickerSetup() {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(2);
    tft.setCursor(10, tftHeight / 2 - 8);
    tft.print("BLE Mode");

    tft.setTextSize(1);
    tft.setCursor(10, tftHeight / 2 + 20);
    tft.print("Not yet implemented");

    delay(2000);
    // Serial.println("[CLICKER] BLE mode requested but not implemented");
}

#endif // USB_as_HID
