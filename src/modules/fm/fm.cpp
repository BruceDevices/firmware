#ifndef LITE_VERSION
#include "fm.h"
#include "core/utils.h"

bool auto_scan = false;
bool is_running = false;
uint16_t fm_station = 10230; // Default set to 102.30 MHz
Adafruit_Si4713 radio = Adafruit_Si4713();

void set_auto_scan(bool new_value) { auto_scan = new_value; }

void set_frq(uint16_t frq) { fm_station = frq; }

void fm_banner() {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setCursor(10, 10);
    tft.drawCentreString("~== Bruce Radio ==~", tftWidth / 2, 10, SMOOTH_FONT);
    delay(500);
}

uint16_t fm_scan() {
    if (!fm_begin()) { return 0; }
    char display_freq[16];
    uint16_t f = 8750;
    uint16_t min_noise;
    uint16_t freq_candidate = f;

    // Check for first noise level
    radio.readTuneMeasure(f);
    radio.readTuneStatus();
    min_noise = radio.currNoiseLevel;

    tft.fillScreen(bruceConfig.bgColor);
    displayTextLine("Scanning...");
    for (f = 8750; f < 10800; f += 10) {
        Serial.print("Measuring ");
        Serial.print(f);
        Serial.print("...");
        radio.readTuneMeasure(f);
        radio.readTuneStatus();
        Serial.println(radio.currNoiseLevel);

        // Set best freq candidate
        if (radio.currNoiseLevel < min_noise) {
            min_noise = radio.currNoiseLevel;
            freq_candidate = f;
        }
    }

    sprintf(display_freq, "Found %d MHz", freq_candidate);
    tft.fillScreen(bruceConfig.bgColor);
    displayTextLine(display_freq);
    while (!check(EscPress) && !check(SelPress)) { delay(100); }

    return freq_candidate;
}

// Choose between 92.0 - 92.1 - 92.2 - 92.3 etc.
void fm_options_frq(uint16_t f_min, bool reserved) {
    char f_str[9];
    uint16_t f_max;
    // Choose between scan for best freq or select freq
    displayTextLine("Choose frequency");
    delay(1000);

    // Handle min / max frequency
    if (f_min == 87) {
        f_min = 8750;
        f_max = 9000;
    } else {
        f_max = (f_min + 1) * 100;
        f_min = f_min * 100;
    }

    if (f_max > 10800) { f_max = 10800; }

    options = {};
    for (uint16_t f = f_min; f < f_max; f += 10) {
        sprintf(f_str, "%d Hz", f);
        options.push_back({f_str, [=]() { set_frq(f); }});
    }
    addOptionToMainMenu();

    loopOptions(options);
    options.clear();
}

// Choose between 91 - 92 - 93 etc.
void fm_options_digit(uint16_t f_min, bool reserved) {
    char f_str[10];
    uint16_t f_max;
    // Choose between scan for best freq or select freq
    displayTextLine("Choose digit");
    delay(1000);

    // Handle min / max frequency
    if (reserved and f_min < 80) {
        f_min = 76;
        f_max = 80;
    } else if (reserved and f_min >= 80) {
        f_min = 80;
        f_max = 87;
    } else if (f_min < 90) {
        f_min = 87;
        f_max = 90;
    } else if (f_min >= 100) {
        f_max = 108;
    } else {
        f_max = f_min + 10;
    }

    options = {};
    for (uint16_t f = f_min; f < f_max; f += 1) {
        sprintf(f_str, "%d MHz", f);
        options.push_back({f_str, [=]() { fm_options_frq(f, reserved); }});
    }
    addOptionToMainMenu();

    loopOptions(options);
    options.clear();
}

// Choose between 80 - 90 - 100
void fm_options(uint16_t f_min, uint16_t f_max, bool reserved) {
    char f_str[15];
    // Choose between scan for best freq or select freq
    displayTextLine("Choose tens");
    delay(1000);

    options = {};
    if (!reserved) {
        options.push_back({"Auto", [=]() { set_auto_scan(true); }});
    }
    for (uint16_t f = f_min; f < f_max; f += 10) {
        sprintf(f_str, "%d MHz", f);
        options.push_back({f_str, [=]() { fm_options_digit(f, reserved); }});
    }
    addOptionToMainMenu();

    loopOptions(options);
    options.clear();

    if (auto_scan == true) { fm_station = fm_scan(); }
}

void fm_live_run(bool reserved) {
    // Test if FM is attached, if not, close menu
    if (!fm_begin()) { return; }
    uint16_t f_min = 80;
    uint16_t f_max = 110;
    fm_banner();

    if (reserved) {
        f_min = 70;
        f_max = 90;
    }

    // Display choose frequency menu
    fm_options(f_min, f_max, reserved);

    // Run radio broadcast
    if (!returnToMenu and fm_station != 0) {
        fm_setup(false, true); // Don't know why but IT WORKS ONLY when launched 2 times...
        while (!check(EscPress) && !check(SelPress)) { delay(100); }
    }
}

void fm_ta_run() {
    // Set Info Traffic
    fm_station = 10770;
    // Run radio broadcast
    fm_setup(true);
    delay(10);
    fm_setup(true); // Don't know why but IT WORKS ONLY when launched 2 times...
    while (!check(EscPress) && !check(SelPress)) { delay(100); }
}

void fm_spectrum() {
    // Test if FM is attached, if not, close menu
    if (!fm_begin()) { return; }
    uint16_t f_min = 80;
    uint16_t f_max = 110;
    int noise_level = 0;
    int SIGNAL_STRENGTH_THRESHOLD = 35;

    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(1);

    fm_options(f_min, f_max, false);
    delay(10);

    if (!returnToMenu) {
        fm_begin();
        fm_banner();
        while (!check(EscPress) && !check(SelPress)) {
            radio.readTuneMeasure(fm_station);
            radio.readTuneStatus();
            noise_level = radio.currNoiseLevel;
            if (noise_level != 0) {
                // Clear the display area
                tft.fillRect(0, 40, tftWidth, tftHeight, bruceConfig.bgColor);
                // Draw waveform based on signal strength
                for (size_t i = 0; i < noise_level; i++) {
                    int lineHeight = map(noise_level, 0, SIGNAL_STRENGTH_THRESHOLD, 0, tftHeight / 2);
                    int lineX =
                        map(i, 0, noise_level - 1, 0, tftWidth - 1); // Map i to within the display width
                    // Ensure drawing coordinates stay within the box bounds
                    int startY = constrain(20 + tftHeight / 2 - lineHeight / 2, 20, 20 + tftHeight);
                    int endY = constrain(20 + tftHeight / 2 + lineHeight / 2, 20, 20 + tftHeight);
                    tft.drawLine(lineX, startY, lineX, endY, bruceConfig.priColor);
                }
            }
        }
        fm_stop();
        delay(100);
    }
}

bool fm_begin() {
    if (!radio.begin()) { // begin with address 0x63 (CS high default)
        tft.fillScreen(bruceConfig.bgColor);
        Serial.println("Cannot find radio");
        displayTextLine("Cannot find radio", true);
        return false;
    }

    return true;
}

bool fm_setup(bool traffic_alert, bool silent) {
    int tx_power = 115;

    // Set module as running
    is_running = true;

    // Clear screen
    if (!silent) {
        fm_banner();
        tft.setCursor(10, 30);
        Serial.println("Setup Si4713");
        tft.println(" Setup Si4713");
        delay(1000);
    }

    if (!fm_begin()) { // begin with address 0x63 (CS high default)
        return false;
    }

    if (!silent) {
        Serial.print("\nTX power: ");
        Serial.println(tx_power);
        tft.print("\n TX power: ");
        tft.println(tx_power);
    }
    radio.setTXpower(tx_power); // dBuV, 88-115 max

    if (!silent) {
        Serial.print("Tuning: ");
        Serial.print(fm_station / 100);
        Serial.print('.');
        Serial.print(fm_station % 100);
        Serial.println(" MHz");

        tft.print(" Tuning: ");
        tft.print(fm_station / 100);
        tft.print('.');
        tft.print(fm_station % 100);
        tft.println(" MHz");
    }

    radio.tuneFM(fm_station); // Specified frequency

    // Begin the RDS/RDBS transmission
    radio.beginRDS();
    if (traffic_alert) {
        radio.setRDSstation("BruceTraffic");
        radio.setRDSbuffer("Traffic Info");
    } else {
        radio.setRDSstation("BruceRadio");
        radio.setRDSbuffer("Pwned by Bruce Radio!");
    }

    if (!silent) {
        Serial.println("RDS on!");
        tft.println(" RDS on!");
    }
    // Set traffic announcement
    if (traffic_alert) {
        if (!silent) {
            Serial.println("TA on!");
            tft.println(" TA on!");
        }
        radio.setProperty(SI4713_PROP_TX_RDS_PS_MISC, 0x1018);
    } else {
        // Default value
        radio.setProperty(SI4713_PROP_TX_RDS_PS_MISC, 0x1008);
    }

    delay(1000);
    return true;
}

void fm_stop() {
    if (is_running) {
        // Stop radio
        radio.setTXpower(0); // dBuV
        radio.reset();
        is_running = false;
    }
}
#endif
