#include "rf_waterfall.h"
#include "core/display.h"
#ifndef TFT_MOSI
#define TFT_MOSI -1
#endif
float m_rf_waterfall_start_freq = 433.0;
float m_rf_waterfall_end_freq = 435.0;

// Height of one text row in the header block.
#define WF_ROW_H (8 * FP + 1)

void rf_waterfall() {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Waterfall needs a CC1101!", true);
        return;
    }
    if (!initRfModule("rx", m_rf_waterfall_start_freq)) {
        displayError("CC1101 not found!", true);
        return;
    }

    ELECHOUSE_cc1101.setRxBW(200);
    int option, idx = 0;
select:

    option = 0;
    options = {
        {"Start Waterfall", [&]() { option = 3; }},
        {"Start Freq.",     [&]() { option = 1; }},
        {"End Freq.",       [&]() { option = 2; }},
        {"Main Menu",       [&]() { option = 4; }},
    };
    idx = loopOptions(options, idx);

    tft.fillScreen(bruceConfig.bgColor);

    if (option == 4) {
        return;
    } else if (option == 3) {
        rf_waterfall_run();
        goto select;
    } else if (option == 1) {
        rf_waterfall_boundary_freq(m_rf_waterfall_start_freq);
        goto select;
    } else if (option == 2) {
        rf_waterfall_boundary_freq(m_rf_waterfall_end_freq);
        goto select;
    }
}
void rf_waterfall_boundary_freq(float &boundary) {
    options = {};
    int ind = 0;
    int arraySize = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);
    for (int i = 0; i < arraySize; i++) {
        if (subghz_frequency_list[i] - boundary < 0.1) ind = i;
        String tmp = String(subghz_frequency_list[i], 2) + "Mhz";
        options.push_back({tmp.c_str(), [&boundary, i]() { boundary = subghz_frequency_list[i]; }});
    }
    loopOptions(options, ind);
    options.clear();
}

uint16_t swapBytes(uint16_t c) { return (c >> 8) | (c << 8); }

void rf_waterfall_run() {
    float f_start = m_rf_waterfall_start_freq;
    float f_end = m_rf_waterfall_end_freq;
    const int screen_width = tftWidth;
    const int screen_height = tftHeight;
    // three header rows (frequency ruler, peak readout, controls) above the fall
    const int display_top = max(screen_height / 5, 3 * WF_ROW_H + 2);
    float f_freq_step;

    // Theme-derived chrome. The waterfall itself keeps a full-gamut ramp because
    // there the colour *is* the measurement, not decoration.
    const uint16_t bg = bruceConfig.bgColor;
    const uint16_t label = blendColors(bg, bruceConfig.priColor, 170);
    const uint16_t gridCol = blendColors(bg, bruceConfig.priColor, 55);

    // Alloc framebuffer
    uint16_t frameBuffer[screen_width] = {0};

    int current_line = display_top;
    initRfModule("rx", f_start);

    float max_freq = f_start;
    int max_rssi = -100;
    unsigned long lastMaxUpdate = millis();

    tft.fillRect(0, 0, screen_width, display_top, bg);

    int selected_item = 0;
    // Header rows are static between edits, so only repaint them when something
    // they show actually changed — otherwise they flicker once per sweep.
    int drawn_item = -1;
    float drawn_start = NAN, drawn_end = NAN;

    while (1) {
        bool headerDirty =
            (drawn_item != selected_item || drawn_start != f_start || drawn_end != f_end);
        if (headerDirty) {
            drawn_item = selected_item;
            drawn_start = f_start;
            drawn_end = f_end;
            tft.fillRect(0, 0, screen_width, WF_ROW_H, bg);
        }
        for (int i = 0; i < 4; i++) {
            int x = i * (screen_width / 4);
            // the ruler runs over the fall, so it is repainted every pass
            tft.drawFastVLine(x, 0, screen_height, gridCol);
            if (!headerDirty) continue;

            float f_freq = f_start + (f_end - f_start) * i / 4.0;
            tft.setCursor(x + 1, 0);
            tft.setTextSize(FP);

            // the first and last ticks are the editable band edges
            bool editing = (i == 0 && selected_item == 0) || (i == 3 && selected_item == 1);
            if (editing) tft.setTextColor(bg, bruceConfig.priColor);
            else tft.setTextColor(label, bg);
            tft.print(String(f_freq, 1));
        }

        f_freq_step = (f_end - f_start) / screen_width;

        float temp_max_freq = f_start;
        int temp_max_rssi = -100;

        float range = abs(f_end - f_start);
        float step;

        if (range > 100) step = 10;
        else if (range > 10) step = 1;
        else if (range > 1) step = 0.1;
        else if (range > 0.1) step = 0.01;
        else step = 0.001;

        for (int i = 0; i < screen_width; ++i) {
            float f_freq = f_start + i * f_freq_step;
            setMHZ(f_freq);
            // To make sure CC1101 shared with TFT works properly on T-Embed
            if (bruceConfigPins.CC1101_bus.mosi == TFT_MOSI) {
                tft.drawPixel(0, 0, 0);
                delayMicroseconds(150); // T-Embed case, need more time to process
            } else delayMicroseconds(100);

            int i_rssi = ELECHOUSE_cc1101.getRssi();
            // To make sure CC1101 shared with TFT works properly on T-Embed
            if (bruceConfigPins.CC1101_bus.mosi == TFT_MOSI) tft.drawPixel(0, 0, 0);
            if (i_rssi > temp_max_rssi) {
                temp_max_rssi = i_rssi;
                temp_max_freq = f_freq;
            }

            // Cold-to-hot ramp: the noise floor stays dark and fades into the
            // theme background, strong carriers burn through to red.
            int level = constrain(map(i_rssi, -100, -30, 0, 255), 0, 255);

            uint8_t r = 0, g = 0, b = 0;
            if (level <= 63) {
                b = map(level, 0, 63, 0, 255);
            } else if (level <= 127) {
                g = map(level, 64, 127, 0, 255);
                b = map(level, 64, 127, 255, 0);
            } else if (level <= 191) {
                r = map(level, 128, 191, 0, 255);
                g = 255;
            } else {
                r = 255;
                g = map(level, 192, 255, 255, 0);
            }

            uint16_t color = tft.color565(r, g, b);
            // blend the bottom of the ramp into the background so an idle band
            // looks like the rest of the UI instead of a solid slab
            if (level < 40) color = blendColors(bg, color, level * 255 / 40);
            frameBuffer[i] = swapBytes(color);
            if (check(SelPress)) {
                selected_item++;
                if (selected_item > 2) selected_item = 0;
            }

            if (check(UpPress) || check(NextPress)) {
                switch (selected_item) {
                    case 0: f_start += step; break;
                    case 1: f_end += step; break;
                    case 2: return;
                }
                delay(100);
            } else if (check(DownPress) || check(PrevPress)) {
                switch (selected_item) {
                    case 0: f_start -= step; break;
                    case 1: f_end -= step; break;
                    case 2: return;
                }
                if (EscPress) EscPress = false; // Reset for StickCs
                delay(100);
            }
        }
        tft.drawPixel(0, 0, 0); // Cardputer Case, need to call something to the tft.
        tft.pushImage(0, current_line, screen_width, 1, frameBuffer);
        tft.drawFastHLine(0, current_line + 1, screen_width, gridCol);

        if (millis() - lastMaxUpdate >= 5000) {
            max_rssi = temp_max_rssi;
            max_freq = temp_max_freq;
            tft.fillRect(0, WF_ROW_H, screen_width, WF_ROW_H, bg);
            tft.setCursor(3, WF_ROW_H);
            tft.setTextSize(FP);
            tft.setTextColor(bruceConfig.priColor, bg);
            tft.printf("%d dBm @ %.3f", max_rssi, max_freq);

            lastMaxUpdate = millis();
        }

        if (headerDirty) {
            tft.fillRect(0, 2 * WF_ROW_H, screen_width, WF_ROW_H, bg);
            tft.setCursor(3, 2 * WF_ROW_H);
            tft.setTextSize(FP);
            tft.setTextColor(label, bg);
            tft.print("[OK]Item [</>]Val ");

            if (selected_item == 2) tft.setTextColor(bg, bruceConfig.priColor);
            else tft.setTextColor(label, bg);
            tft.print("EXIT");
        }

        if (check(EscPress)) break;

        current_line++;
        if (current_line >= screen_height) current_line = display_top;
    }

    returnToMenu = true;
    deinitRfModule();
    delay(10);
}
