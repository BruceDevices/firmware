/**
 * Advanced IR Jammer Implementation
 *
 * This module provides multiple IR jamming techniques to disrupt IR remote control
 * signals across various frequencies and protocols. Features include:
 * - Multiple jamming algorithms (basic, enhanced, sweep, random, empty)
 * - Adjustable frequency (30-56kHz)
 * - Configurable timing parameters
 * - Real-time statistics
 *
 * Originally based on work by Bruce Lee/EA7KDO with significant enhancements
 * for effectiveness and configurability.
 */

#include "ir_jammer.h"
#include "TV-B-Gone.h" // for checkIrTxPin()
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/settings.h"
#include "core/utils.h"
#include "ir_utils.h"
#include <globals.h>
#include <interface.h>

// Common IR remote control frequencies in Hz
// Covers most consumer devices (30-56kHz range)
const uint16_t IR_FREQUENCIES[] = {30000, 33000, 36000, 38000, 40000, 42000, 56000};
const int NUM_FREQS = sizeof(IR_FREQUENCIES) / sizeof(IR_FREQUENCIES[0]);

// Human-readable mode names for display
const char *IR_MODE_NAMES[] = {"BASIC", "ENH. BASIC", "SWEEP", "RANDOM", "EMPTY"};

/**
 * Initialize the jammer state with safe default values
 * Sets up timing parameters, patterns, and resets statistics
 */
void initJammerState(JammerState &state) {
    // Initialize timing values with safe defaults
    state.markTiming = 12;
    state.spaceTiming = 12;
    state.minTiming = 8;
    state.maxTiming = 70;
    state.dutyCycle = 50;
    state.jamDensity = 5;
    state.sweepSpeed = 1;
    state.sweepDirection = 1;
    state.current_freq_idx = 3; // Start with 38kHz (most common)

    // Initialize basic pattern
    for (int i = 0; i < 20; i += 2) {
        state.basicPattern[i] = state.markTiming;
        state.basicPattern[i + 1] = state.spaceTiming;
    }

    // Initialize random pattern
    randomSeed(millis());
    for (int i = 0; i < 30; i++) { state.randomPattern[i] = random(10, 1000); }

    // Reset stats
    state.jamCount = 0;
    state.startTime = millis();

    // Update settings based on mode
    updateMaxSettings(state);
}

/**
 * Adjust mode-specific parameters based on the currently selected setting
 *
 * @param state Current jammer state
 * @param settingIndex Which parameter is being adjusted (3+ are mode-specific)
 * @param adjustment Amount and direction of change (+1 or -1)
 */
void adjustModeSpecificSetting(JammerState &state, uint8_t settingIndex, int adjustment) {
    switch (state.currentMode) {
        case BASIC:
            if (settingIndex == 3) {
                state.markTiming = constrain(state.markTiming + adjustment, 5, 100);
                state.spaceTiming = state.markTiming; // Keep in sync for basic mode
            }
            break;

        case ENHANCED_BASIC:
            switch (settingIndex) {
                case 3: state.markTiming = constrain(state.markTiming + adjustment, 5, 100); break;
                case 4: state.spaceTiming = constrain(state.spaceTiming + adjustment, 1, 100); break;
                case 5: state.jamDensity = constrain(state.jamDensity + adjustment, 1, 20); break;
            }
            break;

        case SWEEP:
            switch (settingIndex) {
                case 3:
                    state.minTiming = constrain(state.minTiming + adjustment, 1, state.maxTiming - 5);
                    break;
                case 4:
                    state.maxTiming = constrain(state.maxTiming + adjustment, state.minTiming + 5, 150);
                    break;
                case 5: state.sweepSpeed = constrain(state.sweepSpeed + adjustment, 1, 10); break;
                case 6: state.jamDensity = constrain(state.jamDensity + adjustment, 1, 20); break;
            }
            break;

        case RANDOM:
        case EMPTY:
            if (settingIndex == 3) { state.jamDensity = constrain(state.jamDensity + adjustment, 1, 20); }
            break;
    }
}

/**
 * Render the mode-specific settings UI elements based on the current jamming mode
 * Each mode has different configurable parameters that need to be displayed
 *
 * @param state Current jammer configuration
 * @param curY Current Y position for drawing (will be updated as content is added)
 * @param ySpacing Vertical spacing between UI elements
 */
void renderModeSettings(JammerState &state, int &curY, int ySpacing) {
    switch (state.currentMode) {
        case BASIC:
            // Basic mode has a single timing parameter that controls both mark and space
            curY += ySpacing;
            tft.setCursor(10, curY);
            tft.setTextColor(
                (state.settingIndex == 3) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor
            );
            padprint("TIMING: ");
            tft.print(String(state.markTiming));
            tft.println(" us    ");
            break;

        case ENHANCED_BASIC:
            // Enhanced Basic mode has separate mark/space timing and power control
            curY += ySpacing;
            tft.setCursor(10, curY);
            tft.setTextColor(
                (state.settingIndex == 3) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor
            );
            padprint("MARK: ");
            tft.print(String(state.markTiming));
            tft.println(" us    ");

            curY += ySpacing;
            tft.setCursor(10, curY);
            tft.setTextColor(
                (state.settingIndex == 4) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor
            );
            padprint("SPACE: ");
            tft.print(String(state.spaceTiming));
            tft.println(" us    ");

            curY += ySpacing;
            tft.setCursor(10, curY);
            tft.setTextColor(
                (state.settingIndex == 5) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor
            );
            padprint("POWER: ");
            tft.println(String(state.jamDensity));
            break;

        case SWEEP:
            // Sweep mode continuously varies timing between min and max values
            curY += ySpacing;
            tft.setCursor(10, curY);
            tft.setTextColor(
                (state.settingIndex == 3) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor
            );
            padprint("MIN: ");
            tft.print(String(state.minTiming));
            tft.println(" us    ");

            curY += ySpacing;
            tft.setCursor(10, curY);
            tft.setTextColor(
                (state.settingIndex == 4) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor
            );
            padprint("MAX: ");
            tft.print(String(state.maxTiming));
            tft.println(" us    ");

            curY += ySpacing;
            tft.setCursor(10, curY);
            tft.setTextColor(
                (state.settingIndex == 5) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor
            );
            padprint("SPEED: ");
            tft.println(String(state.sweepSpeed));

            curY += ySpacing;
            tft.setCursor(10, curY);
            tft.setTextColor(
                (state.settingIndex == 6) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor
            );
            padprint("POWER: ");
            tft.println(String(state.jamDensity));
            break;

        case RANDOM:
        case EMPTY:
            // Random and Empty mode only have power/density control
            curY += ySpacing;
            tft.setCursor(10, curY);
            tft.setTextColor(
                (state.settingIndex == 3) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor
            );
            padprint("POWER: ");
            tft.println(String(state.jamDensity));
            break;
    }
}

/**
 * Display performance statistics for the jammer
 * Shows jam count, runtime, and jams per second
 *
 * @param state Current jammer state containing statistics
 * @param x X position for rendering the stats
 * @param y Y position for rendering the stats
 */
void displayStats(JammerState &state, int x, int y) {
    // Set text properties for the stats display
    tft.setTextSize(FP);
    tft.setCursor(tftWidth / 2, tftHeight / 2);
    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);

    // Calculate running time in seconds
    uint32_t runtime = (millis() - state.startTime) / 1000;

    // Only update runtime if actively jamming
    if (state.jamming_active) { state.runtime = runtime; }

    // Calculate jams per second (efficiency metric)
    float jps = state.runtime > 0 ? (float)state.jamCount / state.runtime : 0;

    // Display jam count
    tft.print("Jams : ");
    tft.println(state.jamCount);

    // Display runtime in MM:SS format
    tft.setCursor(tftWidth / 2, tft.getCursorY() + 5);
    tft.printf("Time : %02d:%02d", state.runtime / 60, state.runtime % 60);

    // Display efficiency metric
    tft.setCursor(tftWidth / 2, tft.getCursorY() + 12);
    tft.printf("J/s  : %.1f", jps);
}

/**
 * Process user input for navigating and changing jammer settings
 * Handles Selection button for cycling through settings
 *
 * @param state Current jammer state to be updated based on input
 */
void handleJammerInput(JammerState &state) {
    // Handle Select button press to cycle through settings
    if (check(SelPress)) {
        if (!state.selPressHandled) {
            // Cycle to next setting (wraps around using modulo)
            state.settingIndex = (state.settingIndex + 1) % state.maxSettings;
            state.redraw = true;
            state.selPressHandled = true;
            delay(150); // Debounce to prevent multiple triggers
        }
    } else {
        // Reset the handled flag when button is released
        state.selPressHandled = false;
    }

    // Handle Next/Prev buttons to change the selected setting's value
    handleSettingChange(state, check(NextPress), check(PrevPress));
}

/**
 * Process changes to setting values based on Next/Prev button presses
 *
 * @param state Current jammer state to be updated
 * @param nextPressed Whether the Next button was pressed
 * @param prevPressed Whether the Previous button was pressed
 */
void handleSettingChange(JammerState &state, bool nextPressed, bool prevPressed) {
    // Exit if neither button is pressed
    if (!nextPressed && !prevPressed) return;

    // Determine adjustment direction based on which button was pressed
    int adjustment = nextPressed ? 1 : -1;

    // Handle changes based on which setting is currently selected
    switch (state.settingIndex) {
        case 0: // Status (active/paused)
            state.jamming_active = !state.jamming_active;
            if (state.jamming_active) {
                // Reset statistics when starting a new jamming session
                state.startTime = millis();
                state.jamCount = 0;
            }
            break;

        case 1: // Frequency selection
            // Cycle through available frequencies with wrap-around
            state.current_freq_idx = (state.current_freq_idx + adjustment + NUM_FREQS) % NUM_FREQS;
            break;

        case 2: // Jamming mode selection
            // Cycle through available modes with wrap-around
            state.currentMode = (JamMode)((state.currentMode + adjustment + 5) % 5);
            // Update available settings based on new mode
            updateMaxSettings(state);
            updatePatterns(state);
            break;

        // Mode-specific settings (handled by the helper function)
        default: adjustModeSpecificSetting(state, state.settingIndex, adjustment); break;
    }

    // Mark UI for redrawing and update pattern arrays
    state.redraw = true;
    updatePatterns(state);
    delay(100); // Prevent too rapid changes when button is held
}

/**
 * Update pattern arrays based on current timing settings
 * These patterns are used by the IR library for signal generation
 *
 * @param state Current jammer state containing timing parameters
 */
void updatePatterns(JammerState &state) {
    // Update the basic pattern with current mark/space timings
    // Even indices are mark durations, odd indices are space durations
    for (int i = 0; i < 20; i += 2) {
        state.basicPattern[i] = state.markTiming;
        state.basicPattern[i + 1] = state.spaceTiming;
    }

    // Note: Random pattern is updated separately in the random jamming function
    // because it needs to change with each transmission
}

/**
 * Update the maximum number of settings available based on the current mode
 * Different modes have different numbers of configurable parameters
 *
 * @param state Current jammer state to update
 */
void updateMaxSettings(JammerState &state) {
    switch (state.currentMode) {
        case BASIC:
            // Basic mode: status toggle, frequency, mode selection, timing
            state.maxSettings = 4;
            break;
        case ENHANCED_BASIC:
            // Enhanced Basic: adds separate mark/space timing and power control
            state.maxSettings = 6;
            break;
        case SWEEP:
            // Sweep mode: adds min/max bounds, sweep speed, and power control
            state.maxSettings = 7;
            break;
        case RANDOM:
        case EMPTY:
            // Random and Empty modes: only basic controls plus power
            state.maxSettings = 4;
            break;
    }
}

/**
 * Initialize the IR transmitter hardware
 *
 * @param irsend Reference to the IR transmitter object
 */
void setupJammer(IRsend &irsend) {
    // Validate IR transmitter pin configuration
    checkIrTxPin();

    // Initialize IR transmission library
    irsend.begin();

    // Configure IR LED pin as output
    setup_ir_pin(bruceConfigPins.irTx, OUTPUT);

    // Draw UI border on the display
    drawMainBorder();
}

/**
 * Render the jammer user interface based on current state
 * Updates display with current settings, mode, and performance statistics
 *
 * @param state Current jammer configuration and state
 */
void renderJammerUI(JammerState &state) {
    // Throttle UI updates to reduce flicker and improve performance
    uint32_t currentMillis = millis();
    if (!state.redraw && currentMillis - state.lastUIUpdate < 300) return;

    // Create blinking effect for active status indicator
    bool blinkState = (currentMillis % 600 < 300);
    state.lastUIUpdate = currentMillis;

    // Calculate layout dimensions based on screen size
    int contentWidth = tftWidth - 20;
    int yStart = 35;
    int ySpacing = 10;
    int rightColumnX = tftWidth / 2 + 10;

    // Full screen redraw only when necessary
    if (state.redraw) {
        // Clear content area
        tft.fillRect(10, yStart, contentWidth, tftHeight - 55, bruceConfig.bgColor);

        // Draw title
        tft.setCursor(10, yStart);
        tft.setTextSize(FM);
        tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
        padprint("IR Jammer");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    }

    // Show activity indicator when jamming is active
    if (state.jamming_active && blinkState) {
        tft.setCursor(tftWidth / 2, tft.getCursorY());
        tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
        tft.println("*");
    } else {
        tft.setCursor(tftWidth / 2, tft.getCursorY());
        tft.println(" ");
    }

    // Display current status with highlighting for selected setting
    int curY = yStart + 20;
    tft.setCursor(10, curY);
    tft.setTextSize(FP);
    tft.setTextColor((state.settingIndex == 0) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor);
    padprint("STATUS: ");
    tft.setTextColor(state.jamming_active ? TFT_RED : TFT_WHITE, bruceConfig.bgColor);
    tft.println(state.jamming_active ? "ACTIVE " : "PAUSED ");

    // Display frequency setting
    curY += ySpacing + 10;
    tft.setCursor(10, curY);
    tft.setTextColor((state.settingIndex == 1) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor);
    padprint("FREQ: ");
    tft.print(String(getFrequency(state.current_freq_idx) / 1000));
    tft.println(" kHz    ");

    // Display mode selection
    curY += ySpacing;
    tft.setCursor(10, curY);
    tft.setTextColor((state.settingIndex == 2) ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor);
    padprint("MODE: ");
    tft.println(getModeName(state.currentMode));

    // Display mode-specific settings
    renderModeSettings(state, curY, ySpacing);

    // Show performance statistics
    displayStats(state, rightColumnX, tftHeight / 2 - 25);

    // Display user instructions at the bottom of the screen
    int instructionsY = tftHeight - 20;
    tft.setCursor(10, instructionsY);
    tft.setTextSize(FP);
    tft.setTextColor(TFT_BLUE, bruceConfig.bgColor);
    padprintln("[SEL] Change Set. | [NEXT/PREV] Adjust Val. ");

    // Display exit instruction in top-right corner
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.setCursor(tftWidth - 70, 30);
    tft.print("[ESC] Exit");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    // Reset redraw flag now that UI is updated
    state.redraw = false;
}

/**
 * Update jamming statistics after transmitting signals
 * Increments jam count and triggers UI update when needed
 *
 * @param state Current jammer state to update
 */
void updateStats(JammerState &state) {
    // Increment jam count for each signal transmitted
    state.jamCount++;

    // Only update UI every 10 jams to reduce display flicker
    if (state.jamCount % 10 == 0) { state.redraw = true; }
}

/**
 * Execute jamming operations based on current mode and settings
 * Controls the frequency of signals based on jam density
 *
 * @param state Current jammer configuration
 * @param irsend IR transmitter interface
 */
void performJamming(JammerState &state, IRsend &irsend) {
    // Skip if jamming is paused
    if (!state.jamming_active) return;

    uint32_t currentMillis = millis();

    // Control signal frequency based on density setting
    // Higher density = more frequent signals
    if (currentMillis - state.lastJamTime >= (10 / state.jamDensity)) {
        state.lastJamTime = currentMillis;

        // Select appropriate jamming implementation based on mode
        switch (state.currentMode) {
            case BASIC: performBasicJamming(state, irsend); break;
            case ENHANCED_BASIC: performEnhancedBasicJamming(state, irsend); break;
            case SWEEP: performSweepJamming(state, irsend); break;
            case RANDOM: performRandomJamming(state, irsend); break;
            case EMPTY: performEmptyJamming(state, irsend); break;
        }

        // Update statistics after sending jam signals
        updateStats(state);
    }
}

/**
 * Implement basic jamming mode with fixed, equal mark/space timing
 * Uses both direct LED control and IR library for maximum effectiveness
 *
 * @param state Current jammer configuration
 * @param irsend IR transmitter interface
 */
void performBasicJamming(JammerState &state, IRsend &irsend) {
    uint32_t currentMillis = millis();

    // Throttle transmission rate to prevent hardware overload
    if (currentMillis - state.last_update > 20) {
        // Method 1: Direct LED control for precise timing
        // Generates simple square wave with equal mark/space duration
        for (int i = 0; i < 50 * state.jamDensity; i++) {
            digitalWrite(bruceConfigPins.irTx, HIGH);
            delayMicroseconds(state.markTiming);
            digitalWrite(bruceConfigPins.irTx, LOW);
            delayMicroseconds(state.markTiming);
        }

        // Method 2: Use IR library for compatibility with different protocols
        irsend.sendRaw(state.basicPattern, 20, getFrequency(state.current_freq_idx));
        state.last_update = currentMillis;
    }
}

/**
 * Implement enhanced basic jamming with separate mark/space timing control
 * This mode provides more precise control over IR signal characteristics
 *
 * @param state Current jammer configuration
 * @param irsend IR transmitter interface
 */
void performEnhancedBasicJamming(JammerState &state, IRsend &irsend) {
    uint32_t currentMillis = millis();

    // Throttle transmission rate
    if (currentMillis - state.last_update > 20) {
        // Method 1: Direct LED control with separate mark/space timing
        // Provides more flexibility to target specific IR protocols
        for (int i = 0; i < 25 * state.jamDensity; i++) {
            digitalWrite(bruceConfigPins.irTx, HIGH);
            delayMicroseconds(state.markTiming);
            digitalWrite(bruceConfigPins.irTx, LOW);
            delayMicroseconds(state.spaceTiming);
        }

        // Method 2: Use IR library with custom pattern
        irsend.sendRaw(state.basicPattern, 20, getFrequency(state.current_freq_idx));
        state.last_update = currentMillis;
    }
}

/**
 * Implement sweep jamming mode that varies timing continuously
 * Effective against IR protocols with dynamic timing adaptation
 *
 * @param state Current jammer configuration
 * @param irsend IR transmitter interface
 */
void performSweepJamming(JammerState &state, IRsend &irsend) {
    uint32_t currentMillis = millis();

    // Faster update rate for smooth sweep
    if (currentMillis - state.last_update > 30) {
        // Update timing values based on current direction and speed
        state.markTiming += state.sweepDirection * state.sweepSpeed;

        // Change direction if we hit the min/max bounds
        if (state.markTiming > state.maxTiming || state.markTiming < state.minTiming) {
            state.sweepDirection *= -1; // Reverse direction
            // Constrain to prevent exceeding bounds
            state.markTiming = constrain(state.markTiming, state.minTiming, state.maxTiming);
        }

        // For sweep mode, keep mark and space timings equal
        state.spaceTiming = state.markTiming;

        // Update pattern array with new timings
        updatePatterns(state);

        // Direct LED control for precise timing
        for (int i = 0; i < 20 * state.jamDensity; i++) {
            digitalWrite(bruceConfigPins.irTx, HIGH);
            delayMicroseconds(state.markTiming);
            digitalWrite(bruceConfigPins.irTx, LOW);
            delayMicroseconds(state.markTiming);
        }

        // Also use IR library for protocol compatibility
        irsend.sendRaw(state.basicPattern, 20, getFrequency(state.current_freq_idx));
        state.last_update = currentMillis;
    }
}

/**
 * Implement random jamming mode with unpredictable patterns
 * Most effective against smart/learning remotes and adaptive systems
 *
 * @param state Current jammer configuration
 * @param irsend IR transmitter interface
 */
void performRandomJamming(JammerState &state, IRsend &irsend) {
    uint32_t currentMillis = millis();

    // Slower update rate to allow for more random pattern generation
    if (currentMillis - state.last_update > 100) {
        // Generate new random pattern for each transmission
        for (int i = 0; i < 30; i++) {
            state.randomPattern[i] = random(5, 1000); // Random durations between 5-1000µs
        }

        // Send multiple random patterns based on jam density
        for (int i = 0; i < state.jamDensity / 2 + 1; i++) {
            // Randomly change carrier frequency to disrupt more protocols
            if (random(10) < 3) { state.current_freq_idx = random(NUM_FREQS); }

            // Send the random pattern at the selected frequency
            irsend.sendRaw(state.randomPattern, 30, getFrequency(state.current_freq_idx));
        }

        state.last_update = currentMillis;
    }
}

/**
 * Implement empty packet jamming mode with minimal valid IR signals
 * Effective at confusing IR receivers while using minimal power
 *
 * @param state Current jammer configuration
 * @param irsend IR transmitter interface
 */
void performEmptyJamming(JammerState &state, IRsend &irsend) {
    uint32_t currentMillis = millis();

    // Medium update rate optimized for empty packet transmission
    if (currentMillis - state.last_update > 50) {
        // Send multiple empty packets at the current frequency
        for (int i = 0; i < state.jamDensity; i++) {
            irsend.sendRaw(state.emptyPattern, 4, getFrequency(state.current_freq_idx));
        }

        // Occasionally change frequency (40% chance)
        if (random(5) < 2) { state.current_freq_idx = (state.current_freq_idx + 1) % NUM_FREQS; }

        state.last_update = currentMillis;
    }
}

/**
 * Clean up resources and show exit message when jammer is stopped
 *
 * @param irsend IR transmitter interface to reset
 */
void cleanupJammer(IRsend &irsend) {

#ifdef USE_BOOST /// ENABLE 5V OUTPUT
    PPM.disableOTG();
#endif
    // Ensure IR LED is turned off
    digitalWrite(bruceConfigPins.irTx, LOW);

    // Display exit message
    displayRedStripe("IR Jamming Stopped");

    // Short delay for user to see the message
    delay(1000);
}

/**
 * Main entry point for IR jammer functionality
 * Initializes hardware, runs the main loop, and handles cleanup
 */
void startIrJammer() {
#ifdef USE_BOOST /// ENABLE 5V OUTPUT
    PPM.enableOTG();
#endif
    // Initialize IR transmitter with configured pin
    IRsend irsend(bruceConfigPins.irTx);

    // Initialize jammer state structure
    JammerState state;

    // Set up hardware and state
    setupJammer(irsend);
    initJammerState(state);

    // Main jammer loop - runs until ESC is pressed
    while (!check(EscPress)) {
        renderJammerUI(state);         // Update display
        performJamming(state, irsend); // Execute jamming if active
        handleJammerInput(state);      // Process user input

        // Small delay to prevent system overload
        delay(5);
    }

    // Clean up when exiting
    cleanupJammer(irsend);
}

/**
 * Get IR carrier frequency in Hz for the given index
 * Uses program memory for efficient storage of frequency values
 *
 * @param index Index into the IR_FREQUENCIES array
 * @return Frequency value in Hz
 */
uint16_t getFrequency(uint8_t index) { return pgm_read_word(&IR_FREQUENCIES[index]); }

/**
 * Get human-readable name for the current jamming mode
 *
 * @param index Index from JamMode enum
 * @return String containing the mode name
 */
const char *getModeName(uint8_t index) { return IR_MODE_NAMES[index]; }
