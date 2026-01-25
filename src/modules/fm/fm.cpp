#include "fm.h"
#include "Tea5767.h" // Include the library for the TEA5767 module
#include "core/utils.h"
#include <Wire.h>

// Create an instance of the TEA5767 radio
TEA5767 radio;

// Global variables definition
bool auto_scan = false;
bool is_running = false;
uint16_t fm_station = 8950; // 89.50 MHz

// --- New functions implementations ---

// Display a simple banner for FM mode
void fm_banner() { displayTextLine("FM Radio"); }

// Build frequency options menu
void fm_options(uint16_t f_min, uint16_t f_max, bool reserved) {
    char f_str[12];
    // Assume options is a globally available vector of menu options
    options.clear();
    // Convert frequency values: multiply by 100 to use integer arithmetic (e.g., 87 => 8700)
    for (uint16_t f = f_min * 100; f < f_max * 100; f += 10) {
        sprintf(f_str, "%d.%02d MHz", f / 100, f % 100);
        options.push_back({f_str, [=]() {
                               set_frq(f);
                               tft.fillScreen(bruceConfig.bgColor);
                               displayTextLine("Tuning...");
                               fm_tune(false);
                               delay(1000);
                           }});
    }
    addOptionToMainMenu();
    loopOptions(options);
}

// --- Updated function with timeout ---
void fm_live_run(bool reserved) {
    fm_banner();
    fm_options(87, 108, reserved);
    if (fm_station != 0) {
        fm_tune(false);
        unsigned long startTime = millis();
        // Wait up to 5 seconds or until Esc or Sel is pressed
        while (!check(EscPress) && !check(SelPress) && (millis() - startTime < 5000)) { delay(100); }
    }
}

void fm_ta_run() {
    fm_station = 10770;
    fm_tune(false);
    while (!check(EscPress) && !check(SelPress)) delay(100);
}

void fm_spectrum() {
    tft.fillScreen(bruceConfig.bgColor);
    displayTextLine("Signal Spectrum");
    fm_begin();

    while (!check(EscPress)) {
        for (uint16_t f = 8750; f < 10800; f += 10) {
            set_frq(f);
            fm_tune(true);
            delay(20);

            Wire.requestFrom(0x60, 5);
            if (Wire.available() >= 5) {
                byte status[5];
                for (int i = 0; i < 5; i++) status[i] = Wire.read();
                int rssi = (status[3] >> 4) * 10;

                int barHeight = map(rssi, 0, 150, 0, tftHeight - 40);
                tft.drawFastVLine(map(f, 8750, 10800, 0, tftWidth), 40, tftHeight - 40, bruceConfig.bgColor);
                tft.drawFastVLine(
                    map(f, 8750, 10800, 0, tftWidth), tftHeight - barHeight, barHeight, bruceConfig.priColor
                );
            }
        }
    }
    fm_stop();
}

// Extra frequency options function with fixed buffer size
void fm_options_frq(uint16_t f_min, bool reserved) {
    char f_str[12]; // Increased buffer size from 10 to 12
    uint16_t f_max = (f_min + 1) * 100;
    f_min *= 100;

    options.clear();
    for (uint16_t f = f_min; f < f_max; f += 10) {
        sprintf(f_str, "%d.%02d MHz", f / 100, f % 100);
        options.push_back({f_str, [=]() {
                               set_frq(f);
                               tft.fillScreen(bruceConfig.bgColor);
                               displayTextLine("Tuning...");
                               fm_tune(false);
                               delay(1000);
                           }});
    }
    addOptionToMainMenu();
    loopOptions(options);
}

// --- Updated FM Hardware Functions ---
// These functions now use the TEA5767 library to control a real FM module.

// Initialize the FM hardware and radio module
bool fm_begin() {
    Wire.begin();
    // Initialize the radio (you might need to call radio.setFrequency() after init)
    radio.init();
    is_running = true;
    displayTextLine("FM Initialized");
    return true;
}

// Tune the radio to the current fm_station frequency
bool fm_tune(bool silent) {
    float freq = fm_station / 100.0; // Convert from integer (e.g., 8950) to float (89.50)
    radio.setFrequency(freq);
    // Give the radio a moment to settle
    delay(100);
    if (!silent) {
        char buf[30];
        sprintf(buf, "Tuning to %.2f MHz", freq);
        displayTextLine(buf);
    }
    return true;
}

// Stop FM radio operation
void fm_stop() {
    // Depending on your module, you might want to power it down
    // For TEA5767, there is no explicit "power off" so we simply mark it as stopped.
    is_running = false;
    displayTextLine("FM Stopped");
}

// Dummy scanning: In a real implementation, you would search for stations
uint16_t fm_scan() {
    // For demonstration, simply return the current station.
    return fm_station;
}

// Set the current frequency (in integer format, e.g., 8950 for 89.50 MHz)
void set_frq(uint16_t frq) { fm_station = frq; }

// Set auto scan mode (if implemented)
void set_auto_scan(bool new_value) { auto_scan = new_value; }
