#include "nrf_spectrum.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/spectrum_plot.h"

#define CHANNELS 80
uint8_t channel[CHANNELS];

// The RPD accumulator settles toward 125, so that is full scale for the plot.
#define NRF_FULL_SCALE 125

#if defined(CARDPUTER_ADV_3IN1_MUX)
// Implemented by boards/m5stack-cardputer/interface.cpp.
// The mutex prevents the TCA8418 keyboard task from taking GPIO8/9 while
// an NRF24 receive window is in progress.
extern void adv3in1Nrf24CriticalBegin();
extern void adv3in1Nrf24CriticalEnd();
#endif

// Sweeps the whole 2.4GHz band once and updates the smoothed per-channel
// levels. Drawing lives in nrf_draw() so the WebUI can scan without a screen.
String scanChannels(bool web) {
    String result = "{";

    uint8_t rpdValues[CHANNELS] = {0};

#if defined(CARDPUTER_ADV_3IN1_MUX)
    adv3in1Nrf24CriticalBegin();
#endif

    // CE must be inactive before changing channels.
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);

    for (int i = 0; i < CHANNELS; i++) {
        NRFradio.setChannel(i);
        NRFradio.startListening();

#if defined(CARDPUTER_ADV_3IN1_MUX)
        // The 3-in-1 expansion needs the RPD sample while CE still owns GPIO8.
        // Keep the hardware-validated tolerant detection strategy scoped to
        // this board so other NRF24 devices retain the upstream scanner.
        delayMicroseconds(130);
        bool foundSignal = NRFradio.testRPD();
        NRFradio.stopListening();
        foundSignal = foundSignal || NRFradio.testRPD() || NRFradio.available();

        if (foundSignal) { NRFradio.flush_rx(); }

        int rpd = foundSignal ? 1 : 0;
#else
        delayMicroseconds(128);
        NRFradio.stopListening();

        int rpd = NRFradio.testRPD() ? 1 : 0;
#endif
        channel[i] = (channel[i] * 3 + rpd * NRF_FULL_SCALE) / 4;
        rpdValues[i] = channel[i];
    }

#if defined(CARDPUTER_ADV_3IN1_MUX)
    // Leave NRF24 electrically inactive while the TCA8418 borrows GPIO8/9.
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
    adv3in1Nrf24CriticalEnd();
#else
    // Preserve upstream behaviour for every other NRF24 configuration.
    digitalWrite(bruceConfigPins.NRF24_bus.io0, HIGH);
#endif

    if (web) {
        for (int i = 0; i < CHANNELS; i++) {
            if (i > 0) result += ",";
            result += String(rpdValues[i]);
        }
        result += "}";
    }
    return result; // "{1,32,45,...}" with 80 values, for the WebUI
}

// Spreads the 80 channel levels across the plot columns, interpolating between
// carriers so the trace reads as a continuous band instead of 80 blocks.
static void nrf_envelope(const uint8_t *lvl, uint8_t *env, int plotW) {
    for (int i = 0; i < plotW; i++) {
        int32_t pos = (int32_t)i * (CHANNELS - 1) * 256 / (plotW - 1);
        int ci = pos >> 8;
        int frac = pos & 0xff;
        if (ci >= CHANNELS - 1) {
            ci = CHANNELS - 2;
            frac = 256;
        }
        int v = lvl[ci] + (lvl[ci + 1] - lvl[ci]) * frac / 256;
        v = v * 100 / NRF_FULL_SCALE;
        env[i] = (uint8_t)(v < 0 ? 0 : (v > 100 ? 100 : v));
    }
}

void nrf_spectrum() {
    SpectrumPlot plot;
    if (!plot.begin("NRF Spectrum")) {
        displayError("Out of memory", true);
        return;
    }

    const int plotW = plot.width();
    uint8_t *env = (uint8_t *)malloc(plotW);
    uint8_t *envPeak = (uint8_t *)malloc(plotW);
    uint8_t peak[CHANNELS] = {0};
    if (!env || !envPeak) {
        free(env);
        free(envPeak);
        plot.end();
        displayError("Out of memory", true);
        return;
    }

    // 2.400GHz to 2.479GHz, one tick every 20 channels
    const int tickCount = 5;
    int cols[tickCount];
    String labels[tickCount];
    for (int i = 0; i < tickCount; i++) {
        int ch = i * (CHANNELS - 1) / (tickCount - 1);
        cols[i] = ch * (plotW - 1) / (CHANNELS - 1);
        labels[i] = String(2.400f + ch * 0.001f, 2);
    }
    plot.ruler(cols, labels, tickCount);
    plot.status("starting radio...");

#if defined(CARDPUTER_ADV_3IN1_MUX)
    adv3in1Nrf24CriticalBegin();
#endif

    bool nrfStarted = nrf_start(NRF_MODE_SPI); // This function only works on SPI

    if (nrfStarted) {
        NRFradio.setAutoAck(false);
        NRFradio.disableCRC();       // accept any signal we find
        NRFradio.setAddressWidth(2); // a reverse engineering tactic (not typically recommended)
        const uint8_t noiseAddress[][2] = {
            {0x55, 0x55},
            {0xAA, 0xAA},
            {0xA0, 0xAA},
            {0xAB, 0xAA},
            {0xAC, 0xAA},
            {0xAD, 0xAA}
        };
        for (uint8_t i = 0; i < 6; ++i) { NRFradio.openReadingPipe(i, noiseAddress[i]); }
        NRFradio.setDataRate(RF24_1MBPS);
    }

#if defined(CARDPUTER_ADV_3IN1_MUX)
    adv3in1Nrf24CriticalEnd();
#endif

    if (!nrfStarted) {
        Serial.println("Fail Starting radio");
        free(env);
        free(envPeak);
        plot.end();
        displayError("NRF24 not found");
        delay(500);
        return;
    }

    uint32_t lastFrame = 0, lastRow = 0;
    while (!check(EscPress)) {
        // The NRF24 mux is released inside scanChannels() before this yield.
        // Pending TCA8418 events can therefore be serviced without corrupting
        // the RF scan.
        scanChannels();

        int maxCh = 0;
        for (int i = 0; i < CHANNELS; i++) {
            if (channel[i] > peak[i]) peak[i] = channel[i];
            else if (peak[i]) peak[i]--; // slow decay keeps the hold line readable
            if (channel[i] > channel[maxCh]) maxCh = i;
        }

        // A full sweep is far quicker than the panel needs to be repainted, so
        // cap the redraw rate and let the radio keep integrating in between.
        if (millis() - lastFrame >= 40) {
            lastFrame = millis();
            nrf_envelope(channel, env, plotW);
            nrf_envelope(peak, envPeak, plotW);

            // highlight the busiest carrier and its immediate neighbours
            int hlC = maxCh * (plotW - 1) / (CHANNELS - 1);
            int hlSpan = (2 * (plotW - 1)) / (CHANNELS - 1);
            plot.trace(env, envPeak, hlC - hlSpan, hlC + hlSpan);

            if (millis() - lastRow >= 120) {
                lastRow = millis();
                plot.pushRow(env);
                plot.status(
                    "peak ch" + String(maxCh) + "  " + String(2.400f + maxCh * 0.001f, 3) + "GHz  " +
                    String(env[hlC]) + "%"
                );
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

#if defined(CARDPUTER_ADV_3IN1_MUX)
    adv3in1Nrf24CriticalBegin();
#endif
    NRFradio.stopListening();
    NRFradio.powerDown();
#if defined(CARDPUTER_ADV_3IN1_MUX)
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
    adv3in1Nrf24CriticalEnd();
#endif
    free(env);
    free(envPeak);
    plot.end();
    delay(250);
}
