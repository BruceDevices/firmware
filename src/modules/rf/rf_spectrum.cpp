#include "rf_spectrum.h"
#include "rf_utils.h"
#include "structs.h"
#include <RCSwitch.h>

static bool spectrum_rmt_rx_done_callback(
    rmt_channel_t *channel, const rmt_rx_done_event_data_t *edata, void *user_data
) {
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    // send the received RMT symbols to the parser task
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

void draw_tf_spectrum_grid() {
    tft.setTextSize(1);
    tft.setCursor(3, 2);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.printf(" RF - Spectrum (%.2f Mhz)", bruceConfigPins.rfFreq);
    tft.fillRect(0, 20, tftWidth, tftHeight - 20, bruceConfig.bgColor);
    tft.drawFastHLine(0, 20 + tftHeight / 2, tftWidth, TFT_DARKGREY);
    tft.drawFastVLine((1 * tftWidth) / 4, 20, tftHeight - 20, TFT_DARKGREY);
    tft.drawFastVLine((2 * tftWidth) / 4, 20, tftHeight - 20, TFT_DARKGREY);
    tft.drawFastVLine((3 * tftWidth) / 4, 20, tftHeight - 20, TFT_DARKGREY);
}

void rf_spectrum() {
    tft.fillScreen(bruceConfig.bgColor);
    draw_tf_spectrum_grid();
    rmt_channel_handle_t rx_ch = NULL;
    rx_ch = setup_rf_rx();
    if (rx_ch == NULL) return;
    ESP_LOGI("RMT_SPECTRUM", "register RX done callback");
    QueueHandle_t receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    assert(receive_queue);
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = spectrum_rmt_rx_done_callback,
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_ch, &cbs, receive_queue));
    ESP_ERROR_CHECK(rmt_enable(rx_ch));
    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 3000,     // 6us minimum signal duration
        .signal_range_max_ns = 12000000, // 24ms maximum signal duration
    };
    rmt_symbol_word_t item[64];
    rmt_rx_done_event_data_t rx_data;
    ESP_ERROR_CHECK(rmt_receive(rx_ch, item, sizeof(item), &receive_config));

    size_t rx_size = 0;
    while (1) {
        rmt_symbol_word_t *rx_items = NULL;
        if (xQueueReceive(receive_queue, &rx_data, 0) == pdPASS) {
            rx_size = rx_data.num_symbols;
            rx_items = rx_data.received_symbols;
        }
        if (rx_size != 0) {
            // Draw grid and info
            draw_tf_spectrum_grid();
            // Draw waveform based on signal strength
            for (size_t i = 0; i < rx_size; i++) {
                int lineHeight =
                    map(rx_items[i].duration0 + rx_items[i].duration1,
                        0,
                        SIGNAL_STRENGTH_THRESHOLD,
                        0,
                        tftHeight / 2);
                int lineX = map(i, 0, rx_size - 1, 0, tftWidth - 1); // Map i to within the display width
                int startY = constrain(20 + tftHeight / 2 - lineHeight / 2, 20, 20 + tftHeight);
                int endY = constrain(20 + tftHeight / 2 + lineHeight / 2, 20, 20 + tftHeight);
                tft.drawLine(lineX, startY, lineX, endY, bruceConfig.priColor);
            }

            ESP_ERROR_CHECK(rmt_receive(rx_ch, item, sizeof(item), &receive_config));
            rx_size = 0;
        }
        // Checks to leave while
        if (check(EscPress)) { break; }
        if (setMHZMenu()) yield();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    returnToMenu = true;
    rmt_disable(rx_ch);
    rmt_del_channel(rx_ch);
    vQueueDelete(receive_queue);
    deinitRfModule();
}

#define TIME_DIVIDER (tftWidth / 10)
//@Pirata
void rf_SquareWave() {
    RCSwitch rcswitch;
    if (!initRfModule("rx", bruceConfigPins.rfFreq)) return;

    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) rcswitch.enableReceive(bruceConfigPins.CC1101_bus.io0);
    else rcswitch.enableReceive(bruceConfigPins.rfRx);
    int line_w = 0;
    int line_h = 15;
    unsigned int *raw;
PRINT:
    tft.drawPixel(0, 0, 0);
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(1);
    tft.setCursor(3, 2);
    tft.printf("  RF - SquareWave (%.2f Mhz)", bruceConfigPins.rfFreq);

    while (1) {
        if (rcswitch.RAWavailable()) {
            raw = rcswitch.getRAWReceivedRawdata();
            // Clear the display area
            // tft.fillRect(0, 0, tftWidth, tftHeight, bruceConfig.bgColor);
            // Draw waveform based on signal strength
            for (int i = 0; i < RCSWITCH_RAW_MAX_CHANGES - 1; i += 2) {
                if (raw[i] == 0) break;

                if (raw[i] > 20000) raw[i] = 20000;
                if (raw[i + 1] > 20000) raw[i + 1] = 20000;
                if (line_w + (raw[i] + raw[i + 1]) / TIME_DIVIDER > tftWidth) {
                    line_w = 10;
                    line_h += 10;
                }
                if (line_h > tftHeight) {
                    line_h = 15;
                    tft.fillRect(0, 12, tftWidth, tftHeight, bruceConfig.bgColor);
                }
                tft.drawFastVLine(line_w, line_h, 6, bruceConfig.priColor);
                tft.drawFastHLine(line_w, line_h, raw[i] / TIME_DIVIDER, bruceConfig.priColor);

                tft.drawFastVLine(line_w + raw[i] / TIME_DIVIDER, line_h, 6, bruceConfig.priColor);
                tft.drawFastHLine(
                    line_w + raw[i] / TIME_DIVIDER,
                    line_h + 6,
                    raw[i + 1] / TIME_DIVIDER,
                    bruceConfig.priColor
                );
                line_w += (raw[i] + raw[i + 1]) / TIME_DIVIDER;
            }
            rcswitch.resetAvailable();
        }
        // Checks to leave while
        if (check(EscPress)) { break; }
        if (setMHZMenu()) goto PRINT;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    returnToMenu = true;
}

void rf_CC1101_rssi() {
#if !defined(LITE_VERSION)
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("only for CC1101 module", true);
        return;
    }
    int graph_size = tftWidth - 20;
    std::vector<int> signal(graph_size, -95);
    const size_t freq_count = sizeof(subghz_frequency_list) / sizeof(float);
    std::vector<int> bar_size(freq_count, 0);
    int max_bar_size = tftHeight - 20 /*bottom margin*/ - 20 /*top margin*/;
    bool redraw = true;
    const int min_value = map(-70, -95, -20, 0, max_bar_size);
    while (1) {
        if (redraw) {
            redraw = false;
            tft.drawPixel(0, 0, 0);
            tft.fillScreen(bruceConfig.bgColor);
            tft.setTextSize(1);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setCursor(3, 2);
            // Fixed frequency sees a dot running grafic, showing RSSI over time
            if (bruceConfigPins.rfFxdFreq) {
                if (!initRfModule("rx", bruceConfigPins.rfFreq))
                    displayError("Error setting frequency", true);
                tft.printf(" RF - RSSI spectrum (%.2f Mhz)", bruceConfigPins.rfFreq);
                tft.drawFastVLine(20, 20, tftHeight, bruceConfig.priColor);
                tft.drawString("-95", 0, (tftHeight - 120) + 95);
                tft.drawString("-80", 0, (tftHeight - 120) + 80);
                tft.drawString("-65", 0, (tftHeight - 120) + 65);
                tft.drawString("-50", 0, (tftHeight - 120) + 50);
                tft.drawString("-35", 0, (tftHeight - 120) + 35);
                tft.drawString("-20", 0, (tftHeight - 120) + 20);
                // resets signal array
                std::fill(signal.begin(), signal.end(), -95);
            }
            // Range Scan Sees a bargraph simillar to NRF24 grafic, using RSSI across frequencies
            else {
                if (!initRfModule("rx", bruceConfigPins.rfFreq)) displayError("Error starting module", true);
                tft.printf(" RF - RSSI spectrum (%s)", subghz_frequency_ranges[bruceConfigPins.rfScanRange]);
                tft.drawFastHLine(0, tftHeight - 20, tftWidth, bruceConfig.priColor);
                char buf[7];
                float var = subghz_frequency_list[range_limits[bruceConfigPins.rfScanRange][0]];
                snprintf(buf, sizeof(buf), "%.3f", var);
                tft.drawString(buf, 2, tftHeight - 10);
                var = subghz_frequency_list[range_limits[bruceConfigPins.rfScanRange][1]];
                snprintf(buf, sizeof(buf), "%.3f", var);
                tft.drawRightString(buf, tftWidth - 2, tftHeight - 10);
                int range = range_limits[bruceConfigPins.rfScanRange][1] -
                            range_limits[bruceConfigPins.rfScanRange][0] + 1;
                int space = tftWidth / range;
                for (int i = 0; i < range; i++) {
                    tft.drawFastVLine(space * i, tftHeight - 20, 5, bruceConfig.priColor);
                }
                std::fill(bar_size.begin(), bar_size.end(), 0);
            }
        }

        // draw dot graph for fixed frequency
        if (bruceConfigPins.rfFxdFreq) {
            int rssi = ELECHOUSE_cc1101.getRssi();
            tft.drawPixel(0, 0, 0); // To make sure CC1101 shared with TFT works properly
            const int base_y = tftHeight - 120;
            int prev = signal[0];
            for (int i = 1; i < graph_size; i++) {
                if (EscPress || SelPress) break;
                const int x0 = 20 + (i - 1);
                const int x1 = 20 + i;
                const int curr = signal[i];
                // erase old segment between previous and current points
                tft.drawLine(x0, base_y - prev, x1, base_y - curr, bruceConfig.bgColor);
                const int next_val = (i == graph_size - 1) ? rssi : signal[i + 1];
                // shift buffer left by one
                signal[i - 1] = curr;
                if (i == graph_size - 1) signal[i] = rssi;
                // draw updated segment using new values
                tft.drawLine(x0, base_y - curr, x1, base_y - next_val, bruceConfig.priColor);
                prev = curr;
            }
            tft.drawFastVLine(20, 20, tftHeight, bruceConfig.priColor);
            vTaskDelay(pdMS_TO_TICKS(75));
        }
        // draw a bargraph similar to nrf24 across the range
        else {
            int range = range_limits[bruceConfigPins.rfScanRange][1] -
                        range_limits[bruceConfigPins.rfScanRange][0] + 1;

            int space = tftWidth / range;
            int max_idx = 0;
            for (int i = 0; i < range; i++) {
                if (EscPress || SelPress) break;
                setMHZ(subghz_frequency_list[range_limits[bruceConfigPins.rfScanRange][0] + i]);
                vTaskDelay(pdMS_TO_TICKS(5));
                int rssi = ELECHOUSE_cc1101.getRssi();
                tft.drawPixel(0, 0, 0); // To make sure CC1101 shared with TFT works properly
                int size = map(rssi, -95, -20, 0, max_bar_size);
                if (size > bar_size[i]) bar_size[i] = size;
                else bar_size[i] = bar_size[i] - (bar_size[i] - size) / 2; // slow down decrease
                tft.fillRect(
                    i * space, tftHeight - 20 - bar_size[i], space - 2, bar_size[i], bruceConfig.priColor
                );
                tft.fillRect(i * space, 20, space, max_bar_size - bar_size[i], bruceConfig.bgColor);
                if (bar_size[i] > bar_size[max_idx] && bar_size[i] > min_value) max_idx = i;
            }
            if (bar_size[max_idx] > min_value) {
                char buf[7];
                float var = subghz_frequency_list[range_limits[bruceConfigPins.rfScanRange][0] + max_idx];
                snprintf(buf, sizeof(buf), "%.2f", var);
                tft.drawCentreString("Max=      ", tftWidth / 2, tftHeight - 10);
                tft.drawCentreString("Max=" + String(buf), tftWidth / 2, tftHeight - 10);
            }
        }
        if (check(EscPress)) { break; }
        if (check(SelPress)) {
            deinitRfModule();
            rf_range_selection(bruceConfigPins.rfFreq);
            redraw = true;
        }
    }
    deinitRfModule();
#else
    displayError("Not available on Launcher version");
#endif
}
