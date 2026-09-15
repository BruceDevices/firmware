#include "rf_spectrum.h"
#include "core/display.h"
#include "protocols/rf_config.h"
#include "protocols/rf_decoder.h"
#include "rf_utils.h"
#include "structs.h"

// Plot band, derived from the panel so the graph never collides with the title
// bar or the status line on any of the supported screens.
static inline int rf_plot_top() { return BORDER_PAD_Y + 8 * FM + 2; }
static inline int rf_plot_bot() { return tftHeight - 8 * FP - 8; }
// Side margins keep the graph clear of the rounded theme border at x = 5.
static inline int rf_plot_left() { return 8; }
static inline int rf_plot_width() { return tftWidth - 16; }
static inline uint16_t rf_grid_color() {
    return blendColors(bruceConfig.bgColor, bruceConfig.priColor, 55);
}
static inline uint16_t rf_label_color() {
    return blendColors(bruceConfig.bgColor, bruceConfig.priColor, 170);
}

// Frame plus a single status line at the bottom. Drawn once, and again only
// when the tuning changes, so the live graph never flickers.
static void draw_rf_header(const String &title, const String &info) {
    drawMainBorderWithTitle(title);
    tft.setTextSize(FP);
    tft.fillRect(rf_plot_left(), rf_plot_bot() + 2, rf_plot_width(), 8 * FP, bruceConfig.bgColor);
    tft.setTextColor(rf_label_color(), bruceConfig.bgColor);
    tft.drawString(info, rf_plot_left(), rf_plot_bot() + 2, 1);
}

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
    const int top = rf_plot_top();
    const int bot = rf_plot_bot();
    const uint16_t grid = rf_grid_color();

    const int left = rf_plot_left();
    const int w = rf_plot_width();

    tft.fillRect(left, top, w, bot - top, bruceConfig.bgColor);
    tft.drawFastHLine(left, (top + bot) / 2, w, grid);
    for (int i = 1; i < 4; i++) tft.drawFastVLine(left + (i * w) / 4, top, bot - top, grid);
}

// Centres a pulse-width bar on the mid line of the plot band.
static void draw_rf_pulse(int x, int magnitude) {
    const int top = rf_plot_top();
    const int bot = rf_plot_bot();
    const int mid = (top + bot) / 2;
    int h = map(magnitude, 0, SIGNAL_STRENGTH_THRESHOLD, 0, bot - top);
    int startY = constrain(mid - h / 2, top, bot);
    int endY = constrain(mid + h / 2, top, bot);
    tft.drawLine(x, startY, x, endY, bruceConfig.priColor);
}

void rf_spectrum() {
    draw_rf_header("RF Spectrum", String(bruceConfigPins.rfFreq, 2) + " MHz");
    draw_tf_spectrum_grid();
    if (bruceConfigPins.rfModule == M5_RF_MODULE) {
        RfRxSession rx;
        if (!rx.begin()) {
            deinitRfModule();
            return;
        }

        std::vector<int> durations;
        while (1) {
            if (rx.poll(durations)) {
                draw_tf_spectrum_grid();
                for (size_t i = 0; i < durations.size(); i++) {
                    int lineX =
                        rf_plot_left() +
                        map(i, 0, durations.size() > 1 ? durations.size() - 1 : 1, 0, rf_plot_width() - 1);
                    draw_rf_pulse(lineX, abs(durations[i]));
                }
                RF_DBG("m5 spectrum: durations=%u", (unsigned)durations.size());
            }

            if (check(EscPress)) { break; }
            if (setMHZMenu()) {
                rx.end();
                rx.begin();
                draw_rf_header("RF Spectrum", String(bruceConfigPins.rfFreq, 2) + " MHz");
                draw_tf_spectrum_grid();
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        rx.end();
        returnToMenu = true;
        deinitRfModule();
        return;
    }

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
                int lineX = rf_plot_left() + map(i, 0, rx_size - 1, 0, rf_plot_width() - 1);
                draw_rf_pulse(lineX, rx_items[i].duration0 + rx_items[i].duration1);
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

#define TIME_DIVIDER (rf_plot_width() / 10)
//@Pirata
void rf_SquareWave() {
    if (!initRfModule("rx", bruceConfigPins.rfFreq)) return;

    RfRxSession rx;
    if (!rx.begin()) {
        deinitRfModule();
        return;
    }
    const int top = rf_plot_top();
    const int bot = rf_plot_bot();
    const int traceH = 6; // height of one logic level
    int line_w = rf_plot_left();
    int line_h = top;
    std::vector<int> durations;
PRINT:
    tft.drawPixel(0, 0, 0);
    line_w = rf_plot_left();
    line_h = top;
    draw_rf_header("RF SquareWave", String(bruceConfigPins.rfFreq, 2) + " MHz");
    tft.fillRect(rf_plot_left(), top, rf_plot_width(), bot - top, bruceConfig.bgColor);

    while (1) {
        if (rx.poll(durations)) {
            // Draw the captured square wave (HIGH width then LOW width per pair).
            for (size_t i = 0; i + 1 < durations.size(); i += 2) {
                int high = abs(durations[i]);
                int low = abs(durations[i + 1]);
                if (high == 0) break;

                if (high > 20000) high = 20000;
                if (low > 20000) low = 20000;
                if (line_w + (high + low) / TIME_DIVIDER > rf_plot_left() + rf_plot_width()) {
                    line_w = rf_plot_left();
                    line_h += traceH + 4;
                }
                if (line_h + traceH > bot) {
                    line_h = top;
                    tft.fillRect(rf_plot_left(), top, rf_plot_width(), bot - top, bruceConfig.bgColor);
                }
                tft.drawFastVLine(line_w, line_h, traceH, bruceConfig.priColor);
                tft.drawFastHLine(line_w, line_h, high / TIME_DIVIDER, bruceConfig.priColor);

                tft.drawFastVLine(line_w + high / TIME_DIVIDER, line_h, traceH, bruceConfig.priColor);
                tft.drawFastHLine(
                    line_w + high / TIME_DIVIDER,
                    line_h + traceH,
                    low / TIME_DIVIDER,
                    bruceConfig.priColor
                );
                line_w += (high + low) / TIME_DIVIDER;
            }
        }
        // Checks to leave while
        if (check(EscPress)) { break; }
        if (setMHZMenu()) goto PRINT;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    rx.end();
    returnToMenu = true;
}

void rf_CC1101_rssi() {
#if !defined(LITE_VERSION)
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("only for CC1101 module", true);
        return;
    }
    // Left gutter wide enough for the "-95" scale labels, band derived from the
    // panel instead of assuming a 120px tall screen.
    const int top = rf_plot_top();
    const int bot = rf_plot_bot();
    const int axisX = rf_plot_left() + 3 * FP * LW + 2;
    const int graph_size = rf_plot_left() + rf_plot_width() - axisX - 2;
    std::vector<int> signal(graph_size, -95);
    const size_t freq_count = sizeof(subghz_frequency_list) / sizeof(float);
    std::vector<int> bar_size(freq_count, 0);
    const int max_bar_size = bot - top;
    bool redraw = true;
    const int min_value = map(-70, -95, -20, 0, max_bar_size);
    // dBm -> screen row, so the scale follows the plot band on every device
    auto rssiY = [&](int rssi) { return (int)map(constrain(rssi, -95, -20), -95, -20, bot, top); };

    while (1) {
        if (redraw) {
            redraw = false;
            tft.drawPixel(0, 0, 0);
            tft.setTextSize(FP);
            // Fixed frequency sees a dot running grafic, showing RSSI over time
            if (bruceConfigPins.rfFxdFreq) {
                if (!initRfModule("rx", bruceConfigPins.rfFreq))
                    displayError("Error setting frequency", true);
                draw_rf_header("RF RSSI", String(bruceConfigPins.rfFreq, 2) + " MHz");
                tft.fillRect(rf_plot_left(), top, rf_plot_width(), bot - top, bruceConfig.bgColor);
                tft.drawFastVLine(axisX, top, bot - top, bruceConfig.priColor);
                tft.setTextColor(rf_label_color(), bruceConfig.bgColor);
                for (int dbm = -95; dbm <= -20; dbm += 15) {
                    int y = rssiY(dbm) - (8 * FP) / 2;
                    tft.drawString(String(dbm), 8, y, 1);
                    tft.drawFastHLine(axisX - 2, rssiY(dbm), 3, rf_grid_color());
                }
                // resets signal array
                std::fill(signal.begin(), signal.end(), -95);
            }
            // Range Scan Sees a bargraph simillar to NRF24 grafic, using RSSI across frequencies
            else {
                if (!initRfModule("rx", bruceConfigPins.rfFreq)) displayError("Error starting module", true);
                // the band edges are drawn on the bottom row, so keep it empty here
                draw_rf_header(
                    String("RF RSSI ") + subghz_frequency_ranges[bruceConfigPins.rfScanRange], ""
                );
                tft.fillRect(rf_plot_left(), top, rf_plot_width(), bot - top, bruceConfig.bgColor);
                tft.drawFastHLine(rf_plot_left(), bot, rf_plot_width(), bruceConfig.priColor);
                tft.setTextColor(rf_label_color(), bruceConfig.bgColor);
                char buf[8];
                float var = subghz_frequency_list[range_limits[bruceConfigPins.rfScanRange][0]];
                snprintf(buf, sizeof(buf), "%.3f", var);
                tft.drawString(buf, rf_plot_left(), bot + 2, 1);
                var = subghz_frequency_list[range_limits[bruceConfigPins.rfScanRange][1]];
                snprintf(buf, sizeof(buf), "%.3f", var);
                tft.drawRightString(buf, rf_plot_left() + rf_plot_width(), bot + 2, 1);
                int range = range_limits[bruceConfigPins.rfScanRange][1] -
                            range_limits[bruceConfigPins.rfScanRange][0] + 1;
                int space = rf_plot_width() / range;
                for (int i = 0; i < range; i++) {
                    tft.drawFastVLine(rf_plot_left() + space * i, bot - 4, 4, rf_grid_color());
                }
                std::fill(bar_size.begin(), bar_size.end(), 0);
            }
        }

        // draw dot graph for fixed frequency
        if (bruceConfigPins.rfFxdFreq) {
            int rssi = ELECHOUSE_cc1101.getRssi();
            tft.drawPixel(0, 0, 0); // To make sure CC1101 shared with TFT works properly
            int prev = signal[0];
            for (int i = 1; i < graph_size; i++) {
                if (EscPress || SelPress) break;
                const int x0 = axisX + (i - 1);
                const int x1 = axisX + i;
                const int curr = signal[i];
                // erase old segment between previous and current points
                tft.drawLine(x0, rssiY(prev), x1, rssiY(curr), bruceConfig.bgColor);
                const int next_val = (i == graph_size - 1) ? rssi : signal[i + 1];
                // shift buffer left by one
                signal[i - 1] = curr;
                if (i == graph_size - 1) signal[i] = rssi;
                // draw updated segment using new values
                tft.drawLine(x0, rssiY(curr), x1, rssiY(next_val), bruceConfig.priColor);
                prev = curr;
            }
            tft.drawFastVLine(axisX, top, bot - top, bruceConfig.priColor);
            vTaskDelay(pdMS_TO_TICKS(75));
        }
        // draw a bargraph similar to nrf24 across the range
        else {
            int range = range_limits[bruceConfigPins.rfScanRange][1] -
                        range_limits[bruceConfigPins.rfScanRange][0] + 1;

            int space = rf_plot_width() / range;
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
                    rf_plot_left() + i * space, bot - bar_size[i], space - 2, bar_size[i], bruceConfig.priColor
                );
                tft.fillRect(
                    rf_plot_left() + i * space, top, space, max_bar_size - bar_size[i], bruceConfig.bgColor
                );
                if (bar_size[i] > bar_size[max_idx] && bar_size[i] > min_value) max_idx = i;
            }
            if (bar_size[max_idx] > min_value) {
                char buf[8];
                float var = subghz_frequency_list[range_limits[bruceConfigPins.rfScanRange][0] + max_idx];
                snprintf(buf, sizeof(buf), "%.2f", var);
                tft.setTextColor(rf_label_color(), bruceConfig.bgColor);
                tft.drawCentreString("Max=       ", tftWidth / 2, bot + 2, 1);
                tft.drawCentreString("Max=" + String(buf), tftWidth / 2, bot + 2, 1);
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
