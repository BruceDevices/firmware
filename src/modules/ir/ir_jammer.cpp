/**
 * IR Broadband Noise Jammer Implementation - Aggressive Continuous Mode
 *
 * Primary: Continuous 38kHz carrier to saturate receiver AGC
 * Secondary: Broadband noise on other frequencies
 * Configurable strength via PWM duty cycle
 */

#include "ir_jammer.h"
#include "core/display.h"
#include "TV-B-Gone.h"
#include "ir_utils.h"
#include "core/mykeyboard.h"
#include <globals.h>
#include <interface.h>

// Common IR frequencies in Hz
const uint16_t IR_FREQUENCIES[] = {30000, 33000, 36000, 38000, 40000, 42000, 56000};
const int NUM_FREQS = sizeof(IR_FREQUENCIES) / sizeof(IR_FREQUENCIES[0]);


void initJammerState(JammerState &state) {
    state.jamCount = 0;
    state.startTime = millis();
    state.lastUIUpdate = 0;
    state.redraw = true;
    state.strength = 50;
    state.strengthMode = false;
    randomSeed(millis());
}

void setupJammer(IRsend &irsend) {
    checkIrTxPin();
    irsend.begin();
    setup_ir_pin(bruceConfigPins.irTx, OUTPUT);
    digitalWrite(bruceConfigPins.irTx, LOW);
    drawMainBorder();
}

void renderJammerUI(JammerState &state) {
    uint32_t currentMillis = millis();
    if (!state.redraw && currentMillis - state.lastUIUpdate < 300) return;
    state.lastUIUpdate = currentMillis;

    tft.fillRect(10, 35, tftWidth - 20, tftHeight - 55, bruceConfig.bgColor);

    // Title
    tft.setCursor(10, 40);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
    padprint("IR AGGRESSIVE JAMMER");

    // Status
    int curY = 70;
    tft.setTextSize(FP);
    if ((currentMillis / 300) % 2 == 0) {
        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        padprint(">>> JAMMING ACTIVE <<<");
    } else {
        tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
        padprint("    JAMMING ACTIVE     ");
    }
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    curY += 20;
    tft.setCursor(10, curY);
    tft.printf("Primary: 38 kHz (carrier)");
    curY += 15;
    tft.setCursor(10, curY);
    tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
    padprint("Secondary: 30|33|36|40|42|56 kHz");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    curY += 20;

    // Strength indicator
    tft.setCursor(10, curY);
    tft.setTextColor(state.strengthMode ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor);
    tft.printf("Strength: %d%%", state.strength);
    // Visual bar
    tft.setCursor(10, curY + 15);
    int barWidth = (tftWidth - 20) * state.strength / 100;
    tft.fillRect(10, curY + 15, tftWidth - 20, 8, TFT_DARKGREY);
    tft.fillRect(10, curY + 15, barWidth, 8, TFT_GREEN);
    curY += 30;

    // Statistics
    uint32_t runtime = (currentMillis - state.startTime) / 1000;
    float rate = runtime > 0 ? (float)state.jamCount / runtime : 0;

    tft.setCursor(10, curY);
    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
    tft.printf("Carrier pulses: %u", state.jamCount);
    curY += 15;
    tft.setCursor(10, curY);
    tft.printf("Runtime: %02d:%02d", runtime / 60, runtime % 60);
    curY += 15;
    tft.setCursor(10, curY);
    tft.printf("Rate: %.1f k pulses/s", rate / 1000);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    // Instructions
    curY += 15;
    tft.setCursor(10, curY);
    tft.setTextColor(TFT_BLUE, bruceConfig.bgColor);
    if (state.strengthMode) {
        padprint("[NEXT/PREV] Adjust  [SEL] Done");
    } else {
        padprint("[SEL] Strength  [ESC] Exit");
    }

    // Exit instruction
    tft.setCursor(tftWidth - 70, 30);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.print("[ESC] Exit");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    state.redraw = false;
}

void handleJammerInput(JammerState &state) {
    if (check(SelPress)) {
        state.strengthMode = !state.strengthMode;
        state.redraw = true;
        delay(200);
    }

    if (state.strengthMode) {
        if (check(NextPress)) {
            state.strength = min(state.strength + 5, 100);
            state.redraw = true;
            delay(100);
        }
        if (check(PrevPress)) {
            state.strength = max(state.strength - 5, 10);
            state.redraw = true;
            delay(100);
        }
    }
}

void performJamming(JammerState &state, IRsend &irsend) {
    uint32_t now = millis();
    uint16_t period38 = 1000000 / 38000 / 2;  // 13us half-period

    // Calculate ON/OFF times based on strength (duty cycle)
    uint16_t onTime = period38 * state.strength / 100;
    uint16_t offTime = period38 * (100 - state.strength) / 100;
    if (onTime < 1) onTime = 1;
    if (offTime < 1) offTime = 1;

    // PRIMARY: Continuous 38kHz carrier with adjustable duty cycle
    // Run ~2000 cycles = ~50ms at 38kHz
    for (int i = 0; i < 2000; i++) {
        if (check(EscPress)) return;

        digitalWrite(bruceConfigPins.irTx, HIGH);
        delayMicroseconds(onTime);
        digitalWrite(bruceConfigPins.irTx, LOW);
        delayMicroseconds(offTime);

        // Allow input handling during long jam sessions
        if (i % 500 == 0) {
            handleJammerInput(state);
        }
    }
    state.jamCount += 2000;

    // SECONDARY: Broadband noise bursts on other frequencies (every ~50ms)
    if (state.jamCount % 10000 == 0) {
        for (int f = 0; f < NUM_FREQS; f++) {
            if (IR_FREQUENCIES[f] == 38000) continue;
            uint16_t freq = IR_FREQUENCIES[f];
            uint16_t halfPeriod = 1000000 / freq / 2;

            // Short noise burst on each frequency
            for (int i = 0; i < 100; i++) {
                if (check(EscPress)) return;
                digitalWrite(bruceConfigPins.irTx, HIGH);
                delayMicroseconds(halfPeriod);
                digitalWrite(bruceConfigPins.irTx, LOW);
                delayMicroseconds(halfPeriod);
            }
        }
    }

    // Update UI periodically
    if (now - state.lastUIUpdate > 500) {
        state.redraw = true;
    }
}

void updateStats(JammerState &state) {
    // Called from performJamming
    state.redraw = true;
}

void cleanupJammer(IRsend &irsend) {
    digitalWrite(bruceConfigPins.irTx, LOW);

#ifdef USE_BOOST
    PPM.disableOTG();
#endif

    displayRedStripe("IR Jamming Stopped");
    delay(1000);
}

void startIrJammer() {
#ifdef USE_BOOST
    PPM.enableOTG();
#endif

    IRsend irsend(bruceConfigPins.irTx);

    JammerState state;
    initJammerState(state);

    setupJammer(irsend);

    // Main jamming loop - runs until ESC pressed
    while (!check(EscPress)) {
        renderJammerUI(state);
        performJamming(state, irsend);
        vTaskDelay(1);
    }

    cleanupJammer(irsend);
}