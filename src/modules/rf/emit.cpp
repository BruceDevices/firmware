#include "emit.h"
#include "modules/rf/rf_utils.h" // for initRfModule
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// Global variables for shared state
volatile bool outputState = false;
volatile uint16_t rssiCount = 0;
volatile bool selPressed = false;
volatile bool escPressed = false;
volatile float frequency = 0.0;

// Task handle for the periodic task
TaskHandle_t rf_raw_emit_draw_handle = NULL;

// FreeRTOS task to handle periodic updates
void rf_raw_emit_draw(void *parameter) {
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();
    tft.setCursor(20, 38);
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.print("Emitting: ");
    tft.print(frequency);
    tft.print(" MHz");
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    tft.println("   Press [OK] to stop ");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    while (1) {
        previousMillis = millis(); // Prevent screen power-saving

        rssiCount = static_cast<uint16_t>(rssiCount + 1);
        if (rssiCount >= 200) selPressed = true; // Stop the emission after 20 seconds

        // Check for button presses
        if (check(SelPress)) selPressed = true;
        if (check(EscPress)) escPressed = true;

        // Call the draw function
        // Calculate bar dimensions
        int centerY = (TFT_WIDTH / 2) + 20;      // Center axis for the bars
        int maxBarHeight = (TFT_WIDTH / 2) - 50; // Maximum height of the bars

        // Draw the latest bar
        int rssi = outputState ? -45 : -90; // Use outputState to determine RSSI
        int barHeight = map(rssi, -90, -45, 1, maxBarHeight);

        // Calculate bar position
        int x = 20 + (int)(rssiCount * 1.35);
        int yTop = centerY - barHeight;

        // Draw the bar
        tft.drawFastVLine(x, yTop, barHeight * 2, bruceConfig.priColor);

        // Delay for 100ms
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void rf_raw_emit(RawRecording &recorded, bool &returnToMenu) {
    rssiCount = 0;
    selPressed = false;
    escPressed = false;
    frequency = recorded.frequency;

    initRfModule("tx", recorded.frequency);

    gpio_num_t txPin = gpio_num_t(bruceConfigPins.rfTx);
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) txPin = gpio_num_t(bruceConfigPins.CC1101_bus.io0);

    pinMode(txPin, OUTPUT);

    // Create the FreeRTOS task for periodic updates
    // Larger stack prevents stack canary resets while drawing
    xTaskCreate(rf_raw_emit_draw, "RawEmitDraw", 4096, NULL, 1, &rf_raw_emit_draw_handle);

    for (size_t i = 0; i < recorded.codes.size(); ++i) {
        // Send the RMT code
        for (int j = 0; j < recorded.codeLengths[i]; j++) {
            outputState = true;
            if (recorded.codes[i][j].level0 == 1) digitalWrite(txPin, HIGH);
            else digitalWrite(txPin, LOW);
            delayMicroseconds(recorded.codes[i][j].duration0);
            if (recorded.codes[i][j].level1 == 1) digitalWrite(txPin, HIGH);
            else digitalWrite(txPin, LOW);
            delayMicroseconds(recorded.codes[i][j].duration1);
            if (selPressed || escPressed) break;
        }
        outputState = false;
        if (i < recorded.codes.size() - 1) {
            unsigned long startTime = millis();
            while (millis() - startTime < recorded.gaps[i]) {
                delay(10); // Small delay to avoid busy-waiting
                if (selPressed || escPressed) break;
            }
        }
        if (selPressed || escPressed) break;
    }

    // Stop the FreeRTOS task
    if (rf_raw_emit_draw_handle != NULL) {
        vTaskDelete(rf_raw_emit_draw_handle); // Delete only the periodic task
        rf_raw_emit_draw_handle = NULL;       // Reset the handle
    }

    deinitRfModule();

    if (escPressed) returnToMenu = true;
}
