#include "record.h"
#include "rf_utils.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
static bool
record_rmt_rx_done_callback(rmt_channel_t *channel, const rmt_rx_done_event_data_t *edata, void *user_data) {
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    // send the received RMT symbols to the parser task
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}
float phase = 0.0;
float lastPhase = 2 * PI;
unsigned long lastAnimationUpdate = 0;
void sinewave_animation() {
    if (millis() - lastAnimationUpdate < 10) return;

    tft.drawPixel(0, 0, 0);

    int centerY = (tftHeight / 2) + 20;
    int amplitude = (tftHeight / 2) - 40;
    int sinewaveWidth = 5;

    for (int x = 20; x < tftWidth - 20; x++) {
        int lastY = centerY + amplitude * sin(lastPhase + x * 0.05);
        int y = centerY + amplitude * sin(phase + x * 0.05);
        tft.drawFastVLine(x, lastY, sinewaveWidth, bruceConfig.bgColor);
        tft.drawFastVLine(x, y, sinewaveWidth, bruceConfig.priColor);
    }

    lastPhase = phase;
    phase += 0.15;
    if (phase >= 2 * PI) { phase = 0.0; }
    lastAnimationUpdate = millis();
}

void rf_raw_record_draw(RawRecordingStatus status) {
    tft.setCursor(20, 38);
    tft.setTextSize(FP);
    if (status.frequency <= 0) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.print("Looking for frequency...");
        tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
        tft.println("   Press [ESC] to exit  ");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        // The frequency scan function calls the animation
    } else if (!status.recordingStarted) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.print("Waiting for signal...");
        sinewave_animation();
    } else if (status.recordingFinished) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.print("Recording finished.");
        tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
        tft.println("   Press [OK] to save   ");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    } else if (status.latestRssi < 0) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.print("Recording: ");
        tft.print(status.frequency);
        tft.print(" MHz");
        tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
        tft.println("   Press [OK] to stop ");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        // Calculate bar dimensions
        int centerY = (TFT_WIDTH / 2) + 20;      // Center axis for the bars
        int maxBarHeight = (TFT_WIDTH / 2) - 50; // Maximum height of the bars

        // Draw the latest bar
        int rssi = status.latestRssi;
        // Normalize RSSI to bar height (RSSI values are typically negative)
        int barHeight = map(rssi, -90, -45, 1, maxBarHeight);

        // Calculate bar position
        int x = 20 + (int)(status.rssiCount * 1.35);
        int yTop = centerY - barHeight;

        // Draw the bar
        tft.drawFastVLine(x, yTop, barHeight * 2, bruceConfig.priColor);
    }
}

// TODO: replace frequency scans throughout rf.cpp with this unified function
#define FREQUENCY_SCAN_MAX_TRIES 5
float rf_freq_scan() {
    float frequency = 0;
    int idx = range_limits[bruceConfigPins.rfScanRange][0];
    uint8_t attempt = 0;
    int rssi = -80, rssiThreshold = -65;

    FreqFound best_frequencies[FREQUENCY_SCAN_MAX_TRIES];
    for (int i = 0; i < FREQUENCY_SCAN_MAX_TRIES; i++) {
        best_frequencies[i].freq = 433.92;
        best_frequencies[i].rssi = -75;
    }

    while (frequency <= 0 && !check(EscPress)) { // FastScan
        sinewave_animation();
        previousMillis = millis();
        if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
            if (idx < range_limits[bruceConfigPins.rfScanRange][0] ||
                idx > range_limits[bruceConfigPins.rfScanRange][1]) {
                idx = range_limits[bruceConfigPins.rfScanRange][0];
            }
            float checkFrequency = subghz_frequency_list[idx];
            setMHZ(checkFrequency);
            tft.drawPixel(0, 0, 0); // To make sure CC1101 shared with TFT works properly
            vTaskDelay(5 / portTICK_PERIOD_MS);
            rssi = ELECHOUSE_cc1101.getRssi();
            if (rssi > rssiThreshold) {
                best_frequencies[attempt].freq = checkFrequency;
                best_frequencies[attempt].rssi = rssi;
                attempt++;
                if (attempt >= FREQUENCY_SCAN_MAX_TRIES) {
                    int max_index = 0;
                    for (int i = 1; i < FREQUENCY_SCAN_MAX_TRIES; ++i) {
                        if (best_frequencies[i].rssi > best_frequencies[max_index].rssi) { max_index = i; }
                    }

                    bruceConfigPins.setRfFreq(best_frequencies[max_index].freq, 0);
                    frequency = best_frequencies[max_index].freq;
                    Serial.println("Frequency Found: " + String(frequency));
                    deinitRfModule();
                    initRfModule("rx", frequency);
                }
            }
            ++idx;
        } else {

            frequency = 433.92;
            bruceConfigPins.setRfFreq(433.92, 2);
        }
    }
    return frequency;
}

void rf_raw_record_create(RawRecording &recorded, bool &returnToMenu) {
    RawRecordingStatus status;

    bool fakeRssiPresent = false;
    bool rssiFeature = false;
    rssiFeature = bruceConfigPins.rfModule == CC1101_SPI_MODULE;

    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();

    if (rssiFeature) rf_range_selection(bruceConfigPins.rfFreq);

    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();
    rf_raw_record_draw(status);

    // Initialize RF module and update display
    initRfModule(
        "rx", 433.92
    ); // Frequency scan doesnt work when initializing the module with a different frequency
    Serial.println("RF Module Initialized");

    // Set frequency if fixed frequency mode is enabled
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        if (bruceConfigPins.rfFxdFreq || !rssiFeature) status.frequency = bruceConfigPins.rfFreq;
        else status.frequency = rf_freq_scan();
    } else status.frequency = bruceConfigPins.rfFreq;

    // Something went wrong with scan, probably it was cancelled
    if (status.frequency < 300) return;
    recorded.frequency = status.frequency;
    setMHZ(status.frequency);

    // Erase sinewave animation
    tft.drawPixel(0, 0, 0);
    tft.fillRect(10, 30, tftWidth - 20, tftHeight - 40, bruceConfig.bgColor);
    rf_raw_record_draw(status);

    // Start recording
    delay(200);
    rmt_channel_handle_t rx_ch = NULL;
    rx_ch = setup_rf_rx();
    if (rx_ch == NULL) return;
    ESP_LOGI("RMT_SPECTRUM", "register RX done callback");
    QueueHandle_t receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    assert(receive_queue);
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = record_rmt_rx_done_callback,
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_ch, &cbs, receive_queue));
    ESP_ERROR_CHECK(rmt_enable(rx_ch));
    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 3000,     // 10us minimum signal duration
        .signal_range_max_ns = 12000000, // 24ms maximum signal duration
    };
    rmt_symbol_word_t item[64];
    rmt_rx_done_event_data_t rx_data;
    ESP_ERROR_CHECK(rmt_receive(rx_ch, item, sizeof(item), &receive_config));
    Serial.println("RMT Initialized");

    while (!status.recordingFinished) {
        previousMillis = millis();
        size_t rx_size = 0;
        rmt_symbol_word_t *rx_items = NULL;
        if (xQueueReceive(receive_queue, &rx_data, 0) == pdPASS) {
            rx_size = rx_data.num_symbols;
            rx_items = rx_data.received_symbols;
        }
        if (rx_size != 0) {
            bool valid_signal = false;
            if (rx_size >= 5) valid_signal = true;
            if (valid_signal) {         // ignore codes shorter than 5 items
                fakeRssiPresent = true; // For rssi display on single-pinned RF Modules
                rmt_symbol_word_t *code = (rmt_symbol_word_t *)malloc(rx_size * sizeof(rmt_symbol_word_t));

                // Gap calculation
                unsigned long receivedTime = millis();
                unsigned long long signalDuration = 0;
                for (size_t i = 0; i < rx_size; i++) {
                    code[i] = rx_items[i];
                    signalDuration += rx_items[i].duration0 + rx_items[i].duration1;
                }
                recorded.codes.push_back(code);
                recorded.codeLengths.push_back(rx_size);

                if (status.lastSignalTime != 0) {
                    unsigned long signalDurationMs = signalDuration / RMT_1MS_TICKS;
                    uint16_t gap = (uint16_t)(receivedTime - status.lastSignalTime - signalDurationMs - 5);
                    recorded.gaps.push_back(gap);
                } else {
                    status.firstSignalTime = receivedTime;
                    status.recordingStarted = true;
                    // Erase sinewave animation
                    tft.drawPixel(0, 0, 0);
                    tft.fillRect(10, 30, tftWidth - 20, tftHeight - 40, bruceConfig.bgColor);
                }
                status.lastSignalTime = receivedTime;
            }
            ESP_ERROR_CHECK(rmt_receive(rx_ch, item, sizeof(item), &receive_config));
            rx_size = 0;
        }

        // Periodically update RSSI
        if (status.recordingStarted &&
            (status.lastRssiUpdate == 0 || millis() - status.lastRssiUpdate >= 100)) {
            if (fakeRssiPresent) status.latestRssi = -45;
            else status.latestRssi = -90;
            fakeRssiPresent = false;

            if (rssiFeature) status.latestRssi = ELECHOUSE_cc1101.getRssi();

            status.rssiCount++;
            status.lastRssiUpdate = millis();
        }

        // Stop recording after 20 seconds
        if (status.firstSignalTime > 0 && millis() - status.firstSignalTime >= 20000)
            status.recordingFinished = true;
        if (check(SelPress) && status.recordingStarted) status.recordingFinished = true;
        if (check(EscPress)) {
            status.recordingFinished = true;
            returnToMenu = true;
        }
        rf_raw_record_draw(status);
    }
    Serial.println("Recording stopped.");
    rmt_disable(rx_ch);
    rmt_del_channel(rx_ch);
    vQueueDelete(receive_queue);
    deinitRfModule();
}

int rf_raw_record_options(bool saved) {
    int option = 0;
    options = {
        {"Replay", [&]() { option = 1; }},
        {"Save",   [&]() { option = 2; }},
        {"Exit",   [&]() { option = 4; }},
    };
    if (saved) {
        options.erase(options.begin() + 1);
        options.insert(options.begin() + 1, {"Record another", [&]() { option = 3; }});
    } else {
        options.insert(options.begin() + 1, {"Discard", [&]() { option = 3; }});
    }
    loopOptions(options);

    return option;
}

void rf_raw_record() {
    bool replaying = false;
    bool returnToMenu = false;
    bool saved = false;
    int option = 3;
    RawRecording recorded;
    while (option != 4) {
        if (option == 1) { // Replay
            rf_raw_emit(recorded, returnToMenu);
        } else if (option == 2) { // Save
            saved = true;
            rf_raw_save(recorded);
        } else if (option == 3) { // Discard
            saved = false;
            for (auto &code : recorded.codes) free(code);
            recorded.codes.clear();
            recorded.codeLengths.clear();
            recorded.gaps.clear();
            recorded.frequency = 0;
            rf_raw_record_create(recorded, returnToMenu);
        }

        if (returnToMenu || check(EscPress)) return;
        option = rf_raw_record_options(saved);
    }
    for (auto &code : recorded.codes) free(code);
    recorded.codes.clear();
    recorded.codeLengths.clear();
    recorded.gaps.clear();
    recorded.frequency = 0;
    return;
}
